#pragma once

// ============================================================
//  config.h — Central configuration for Pumping Station IoT
//  Edit STATION_ID and DEVICE_TOKEN before flashing each board.
//  All other values are defaults and should not need changing
//  for a standard deployment.
// ============================================================

// ── Station Identity (unique per board) ─────────────────────
#define DEFAULT_STATION_ID "STATION_001" // Change per board
#define DEFAULT_DEVICE_TOKEN ""          // Paste provisioning token here

// ── Firebase Project ─────────────────────────────────────────
#ifndef FIREBASE_WEB_API_KEY
#define FIREBASE_WEB_API_KEY "" // Should be injected via platformio.ini build_flags
#endif

#define FIREBASE_RTDB_URL                                                      \
  "https://argus360-c0496-default-rtdb.europe-west1.firebasedatabase.app"
#define FIREBASE_PROJECT_ID "argus360-c0496"
#define FIREBASE_REGION "europe-west1"

// Derived Firebase URLs — do not edit
#define FIREBASE_CF_TOKEN_URL                                                  \
  "https://" FIREBASE_REGION "-" FIREBASE_PROJECT_ID                           \
  ".cloudfunctions.net/getDeviceCustomToken"
#define FIREBASE_SIGNIN_URL                                                    \
  "https://identitytoolkit.googleapis.com/v1/"                                 \
  "accounts:signInWithCustomToken?key=" FIREBASE_WEB_API_KEY
#define FIREBASE_REFRESH_URL                                                   \
  "https://securetoken.googleapis.com/v1/token?key=" FIREBASE_WEB_API_KEY

// ── Cellular — A1 Telekom Austria ────────────────────────────
#define CELLULAR_APN "a1.net"
#define CELLULAR_USER "ppp@a1plus.at"
#define CELLULAR_PASS "ppp"

// ── GPIO Pin Map ─────────────────────────────────────────────
// A7670E Modem (hardwired on Waveshare board — do not change)
#define MODEM_TX_PIN 17  // ESP TX → Modem RX
#define MODEM_RX_PIN 18  // ESP RX ← Modem TX
#define MODEM_DTR_PIN 45 // Modem DTR
#define MODEM_RI_PIN 40  // Modem RI (ring indicator — input)

// PZEM-004T v3.0  (UART2, external wiring)
#define PZEM_TX_PIN 13 // ESP TX → PZEM RX
#define PZEM_RX_PIN 14 // ESP RX ← PZEM TX

// MAX17048G Battery Gauge (I2C — primary bus)
// These are the ESP32-S3 default Wire pins; change if I2C scan
// finds no 0x36 device and you need to try the secondary bus.
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

// WS2812B onboard NeoPixel
#define LED_PIN 38
#define LED_COUNT 1

// ── Timing ───────────────────────────────────────────────────
#define PZEM_SAMPLE_INTERVAL_MS 5000UL   // Sample PZEM every 5 s
#define UPLOAD_INTERVAL_MS 30000UL       // Upload every 30 s
#define CONFIG_POLL_INTERVAL_MS 300000UL // Re-fetch RTDB config every 5 min
#define KEEPALIVE_INTERVAL_MS 300000UL   // AT echo + PDP check every 5 min
#define TOKEN_REFRESH_HEADROOM_S 300     // Refresh idToken 5 min before expiry

// GPS
#define GPS_POLL_INTERVAL_MS 10000UL // Poll GNSS fix every 10 s
#define GPS_MAX_WAIT_MS 120000UL     // Give up after 2 min if no fix

// Modem boot / AT echo
#define MODEM_BOOT_WAIT_MS 7000    // Wait after first AT before commands
#define MODEM_AT_ECHO_RETRIES 15   // AT echo retries at 1 s each
#define MODEM_CRESET_WAIT_MS 15000 // Wait after AT+CRESET

// ── Watchdog ─────────────────────────────────────────────────
#define WDT_TIMEOUT_S 120 // 2-minute hardware watchdog

// ── Upload Queue ─────────────────────────────────────────────
#define QUEUE_MAX_ENTRIES 10

// ── Retry Ladder ─────────────────────────────────────────────
#define RETRY_MAX_ATTEMPTS 3
#define CRESET_MAX_ATTEMPTS 2
#define DEGRADED_THRESHOLD 10      // boot failures before degraded mode
#define DEGRADED_RETRY_MS 600000UL // retry modem every 10 min in degraded

// ── Alert Thresholds (overridden by RTDB config at runtime) ──
#define DEFAULT_HIGH_CURRENT_A 18.0f
#define DEFAULT_LOW_CURRENT_A 2.0f
#define DEFAULT_HIGH_VOLTAGE_V 250.0f
#define DEFAULT_LOW_VOLTAGE_V 200.0f

// ── Firmware ─────────────────────────────────────────────────
#define FW_VERSION "1.1.0"
