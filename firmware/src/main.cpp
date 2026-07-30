// ============================================================
//  main.cpp — Pumping Station IoT Firmware
//  Waveshare ESP32-S3-A7670E-4G
//  Version: 1.0.0
// ============================================================

#include "Arduino.h"
#include "config.h"
#include "watchdog.h"
#include "nvs_storage.h"
#include "led.h"
#include "modem.h"
#include "gps.h"
#include "firebase_auth.h"
#include "firebase_rtdb.h"
#include "sensor_pzem.h"
#include "sensor_battery.h"
#include "uploader.h"

// ── State machine ─────────────────────────────────────────────
enum class AppState : uint8_t {
    BOOT,
    CONNECTING_MODEM,
    REGISTERING_NETWORK,
    OPENING_DATA,
    AUTH,
    RUNNING,
    RETRY_SOFT_RESET,
    DEGRADED,
};

static AppState _state = AppState::BOOT;

// ── Runtime config (updated from RTDB) ───────────────────────
static StationConfig _cfg = {
    .highThreshold        = DEFAULT_HIGH_CURRENT_A,
    .lowThreshold         = DEFAULT_LOW_CURRENT_A,
    .highVoltageThreshold = DEFAULT_HIGH_VOLTAGE_V,
    .lowVoltageThreshold  = DEFAULT_LOW_VOLTAGE_V,
    .reportIntervalSec    = 30.0f,
};

// ── Counters & timers ─────────────────────────────────────────
static uint8_t  _retry_count       = 0;
static uint8_t  _auth_retry_count  = 0;  // separate counter — only for AUTH
static uint8_t  _creset_count      = 0;
static uint32_t _last_upload_ms    = 0;
static uint32_t _last_config_ms    = 0;
static uint32_t _last_keepalive_ms = 0;
static uint32_t _degraded_retry_ms = 0;

static String   _station_id;
static String   _device_token;

// ── Forward declarations ──────────────────────────────────────
static void transition(AppState s);
static void run_retry_ladder(const char* reason);
static void check_thresholds(PzemReading& r);

// ═════════════════════════════════════════════════════════════
//  setup()
// ═════════════════════════════════════════════════════════════
void setup() {
    // Serial = UART0 → CH343 USB bridge → COM3.
    // Always ready immediately — no CDC wait needed.
    Serial.begin(115200);
    delay(100);

    Serial.println("\n\n=== Pumping Station IoT v" FW_VERSION " ===");
    Serial.println("Build: " __DATE__ " " __TIME__);
    Serial.println("=========================================\n");

    // 1. Watchdog (must be first)
    wdt_init(WDT_TIMEOUT_S);

    // 2. Persistent storage
    nvs_load();
    _station_id   = nvs_get_station_id();
    _device_token = nvs_get_device_token();

    Serial.printf("[MAIN] Station: %s\n", _station_id.c_str());

    // 3. Check boot fail counter
    uint8_t bf = nvs_get_boot_fail();
    Serial.printf("[MAIN] boot_fail_count = %u\n", bf);
    if (bf >= DEGRADED_THRESHOLD) {
        Serial.println("[MAIN] Entering DEGRADED MODE (too many boot failures)");
        transition(AppState::DEGRADED);
    }

    // 4. Hardware peripherals
    led_init();
    led_set(LedState::BOOTING);

    pzem_init();
    battery_init();
    queue_init();

    // 5. Modem boot
    led_set(LedState::CONNECTING);
    // Clear boot_fail on a fresh boot so accumulation only counts real crashes
    nvs_set_boot_fail(0);
    transition(AppState::CONNECTING_MODEM);
}

// ═════════════════════════════════════════════════════════════
//  loop()
// ═════════════════════════════════════════════════════════════
void loop() {
    uint32_t now = millis();

    // Always tick the LED and sensor regardless of state
    led_tick();
    pzem_tick();
    wdt_reset();

    // ── DEGRADED MODE ─────────────────────────────────────────
    if (_state == AppState::DEGRADED) {
        // Try modem every DEGRADED_RETRY_MS
        if (now - _degraded_retry_ms > DEGRADED_RETRY_MS) {
            _degraded_retry_ms = now;
            Serial.println("[MAIN] Degraded: testing modem...");
            if (modem_is_alive()) {
                Serial.println("[MAIN] Modem alive! Exiting degraded mode");
                nvs_set_boot_fail(0);
                transition(AppState::REGISTERING_NETWORK);
            }
        }
        return;
    }

    // ── CONNECTION STATE MACHINE ──────────────────────────────
    switch (_state) {

        case AppState::CONNECTING_MODEM:
            if (modem_init()) {
                transition(AppState::REGISTERING_NETWORK);
            } else {
                run_retry_ladder("modem_init failed");
            }
            break;

        case AppState::REGISTERING_NETWORK:
            led_set(LedState::CONNECTING);
            if (modem_register_network()) {
                transition(AppState::OPENING_DATA);
            } else {
                run_retry_ladder("network registration failed");
            }
            break;

        case AppState::OPENING_DATA:
            if (modem_open_data_session()) {
                transition(AppState::AUTH);
            } else {
                run_retry_ladder("data session failed");
            }
            break;

        case AppState::AUTH:
            led_set(LedState::AUTH);
            if (auth_begin(_device_token)) {
                _auth_retry_count = 0;
                // Start GPS in background
                gps_init();
                // Fetch initial config
                rtdb_fetch_config(_station_id, auth_get_id_token(), _cfg);
                _last_config_ms    = now;
                _last_keepalive_ms = now;
                led_set(LedState::ONLINE);
                transition(AppState::RUNNING);
                Serial.println("[MAIN] ═══ ONLINE ═══");
            } else {
                _auth_retry_count++;
                Serial.printf("[RETRY] Auth failed — attempt %u/%u\n",
                    _auth_retry_count, RETRY_MAX_ATTEMPTS);

                if (_auth_retry_count <= RETRY_MAX_ATTEMPTS) {
                    // Auth failure: keep data session alive, just retry auth
                    uint32_t delay_ms = (_auth_retry_count == 1) ? 2000
                                      : (_auth_retry_count == 2) ? 5000 : 15000;
                    Serial.printf("[RETRY] Auth backoff %lu ms (data session kept)\n", delay_ms);
                    uint32_t t = millis();
                    while (millis() - t < delay_ms) { led_tick(); wdt_reset(); delay(100); }
                    // Stay in AUTH — do NOT re-open data session
                    transition(AppState::AUTH);
                } else {
                    // Auth is truly broken — check if data session is still up
                    _auth_retry_count = 0;
                    if (!modem_data_session_active()) {
                        Serial.println("[RETRY] Data session lost — reconnecting");
                        run_retry_ladder("auth+data failed");
                    } else {
                        // Data is fine, modem may need reset
                        run_retry_ladder("auth failed");
                    }
                }
            }
            break;

        case AppState::RETRY_SOFT_RESET:
            // Attempt AT+CRESET
            led_set(LedState::ERROR_SOFT);
            Serial.printf("[MAIN] AT+CRESET attempt %u/%u\n",
                _creset_count, CRESET_MAX_ATTEMPTS);

            if (modem_soft_reset()) {
                _creset_count = 0;
                _retry_count  = 0;
                transition(AppState::REGISTERING_NETWORK);
            } else {
                _creset_count++;
                if (_creset_count > CRESET_MAX_ATTEMPTS) {
                    // AT+CRESET has failed — escalate to ESP32 reboot
                    Serial.println("[MAIN] AT+CRESET exhausted — rebooting ESP32");
                    nvs_increment_boot_fail();
                    wdt_disable();
                    delay(1000);
                    esp_restart();
                }
            }
            break;

        case AppState::RUNNING:
            // (handled below)
            break;

        default:
            break;
    }

    if (_state != AppState::RUNNING) return;

    // ══════════════════════════════════════════════════════════
    //  MAIN LOOP — state RUNNING
    // ══════════════════════════════════════════════════════════

    // ── GPS background polling ────────────────────────────────
    gps_tick();

    // ── Keepalive check every 5 min ───────────────────────────
    if (now - _last_keepalive_ms > KEEPALIVE_INTERVAL_MS) {
        _last_keepalive_ms = now;
        if (!modem_is_alive()) {
            Serial.println("[MAIN] Keepalive: modem silent — running retry ladder");
            run_retry_ladder("keepalive failed");
            return;
        }
        if (!modem_data_session_active()) {
            Serial.println("[MAIN] Keepalive: data session dropped — reopening");
            if (!modem_open_data_session()) {
                run_retry_ladder("data session reopen failed");
                return;
            }
        }
    }

    // ── Config refresh every 5 min ────────────────────────────
    if (now - _last_config_ms > CONFIG_POLL_INTERVAL_MS) {
        _last_config_ms = now;
        if (!auth_ensure_valid()) {
            run_retry_ladder("token refresh failed");
            return;
        }
        rtdb_fetch_config(_station_id, auth_get_id_token(), _cfg);
    }

    // ── Upload based on configured interval ───────────────────
    if (now - _last_upload_ms < (uint32_t)(_cfg.reportIntervalSec * 1000)) return;
    _last_upload_ms = now;

    Serial.println("\n[MAIN] ─── Telemetry Cycle Start ───");

    // Collect averaged PZEM reading
    PzemReading pzem  = pzem_get_average();
    BatteryReading batt = battery_read();
    GpsReading    gps  = gps_get();

    // Always upload — rtdb_upload sends zeros + sensorAlert if PZEM is offline
    if (!pzem.valid) {
        Serial.println("[MAIN] PZEM offline — uploading zeros with sensor alert");
    }

    // Threshold alert check
    check_thresholds(pzem);

    // Queue this reading
    QueueEntry entry = { pzem, batt, gps, now };
    queue_push(entry);

    // Ensure token is fresh
    if (!auth_ensure_valid()) {
        Serial.println("[MAIN] Token invalid before upload — queuing");
        run_retry_ladder("token ensure failed");
        return;
    }

    // Upload queued entries (drain queue)
    led_set(LedState::UPLOADING);
    bool upload_ok = true;

    while (!queue_is_empty() && upload_ok) {
        QueueEntry e;
        queue_pop(e);

        int status = rtdb_upload(_station_id,
                                 auth_get_id_token(),
                                 e.pzem, e.batt, e.gps);

        if (status == 200 || status == 204 || status == 201) {
            Serial.printf("[MAIN] Upload OK (HTTP %d) — queue %u remaining\n",
                status, queue_size());
            _retry_count = 0;
        } else {
            // Upload failed — push entry back (best-effort, may be lost if queue full)
            queue_push(e);
            Serial.printf("[MAIN] Upload failed (HTTP %d)\n", status);
            upload_ok = false;
            run_retry_ladder("upload failed");
        }

        wdt_reset();
    }
    
    Serial.println("[MAIN] ─── Telemetry Cycle End ───\n");

    if (upload_ok) led_set(LedState::ONLINE);
}

// ═════════════════════════════════════════════════════════════
//  Helpers
// ═════════════════════════════════════════════════════════════

static void transition(AppState s) {
    _state = s;
}

// Retry ladder:
//   Level 0: retry same operation (handled by caller with retry_count)
//   Level 1: AT+CRESET
//   Level 2: esp_restart()
static void run_retry_ladder(const char* reason) {
    Serial.printf("[RETRY] Reason: %s  retry_count=%u\n", reason, _retry_count);

    _retry_count++;

    if (_retry_count <= RETRY_MAX_ATTEMPTS) {
        // Level 0 — simple retry with backoff
        uint32_t delay_ms = (_retry_count == 1) ? 0
                          : (_retry_count == 2) ? 5000 : 15000;
        if (delay_ms > 0) {
            Serial.printf("[RETRY] Backoff %lu ms\n", delay_ms);
            uint32_t t = millis();
            while (millis() - t < delay_ms) {
                led_tick();
                wdt_reset();
                delay(100);
            }
        }
        // Re-enter connection from network registration
        transition(AppState::REGISTERING_NETWORK);
        return;
    }

    // Level 1 — AT+CRESET
    _retry_count = 0;
    _creset_count = (_creset_count < CRESET_MAX_ATTEMPTS + 1) ? _creset_count : 0;
    transition(AppState::RETRY_SOFT_RESET);
}

static void check_thresholds(PzemReading& r) {
    bool alert = false;
    memset(r.alertType, 0, sizeof(r.alertType));

    if (r.current > _cfg.highThreshold) {
        Serial.printf("[ALERT] HIGH CURRENT: %.3f A > %.1f A\n", r.current, _cfg.highThreshold);
        alert = true;
        strncpy(r.alertType, "HIGH_CURRENT", sizeof(r.alertType) - 1);
    }
    else if (r.current < _cfg.lowThreshold && r.current > 0.01f) {
        Serial.printf("[ALERT] LOW CURRENT: %.3f A < %.1f A\n", r.current, _cfg.lowThreshold);
        alert = true;
        strncpy(r.alertType, "LOW_CURRENT", sizeof(r.alertType) - 1);
    }
    else if (r.voltage > _cfg.highVoltageThreshold) {
        Serial.printf("[ALERT] HIGH VOLTAGE: %.1f V > %.1f V\n", r.voltage, _cfg.highVoltageThreshold);
        alert = true;
        strncpy(r.alertType, "HIGH_VOLTAGE", sizeof(r.alertType) - 1);
    }
    else if (r.voltage < _cfg.lowVoltageThreshold && r.voltage > 50.0f) {
        Serial.printf("[ALERT] LOW VOLTAGE: %.1f V < %.1f V\n", r.voltage, _cfg.lowVoltageThreshold);
        alert = true;
        strncpy(r.alertType, "LOW_VOLTAGE", sizeof(r.alertType) - 1);
    }

    r.alert = alert;

    if (alert) {
        led_set(LedState::ALERT_FLASH);
    }
}
