#include <Arduino.h>
#include <esp_task_wdt.h>
#include <Wire.h>
#include "config.h"
#include "led.h"
#include "pzem_sensor.h"
#include "modem.h"
#include "firebase_auth.h"
#include "firebase_data.h"
#include "alerts.h"
#include "ota.h"

// Hardware instances
StatusLED statusLed(STATUS_LED_PIN);
PZEMSensor pzemSensor(Serial2, PZEM_UART_RX_PIN, PZEM_UART_TX_PIN, PZEM_MODBUS_ADDR);
A7670Modem modem(Serial1, MODEM_PWRKEY_PIN, MODEM_RESET_PIN);
FirebaseAuth firebaseAuth(modem, FIREBASE_API_KEY, DEFAULT_CUSTOM_TOKEN);
FirebaseData firebaseData(modem, firebaseAuth, FIREBASE_DB_URL, DEFAULT_STATION_ID);
EdgeAlert edgeAlert(ALERT_DEBOUNCE_COUNT, ALERT_COOLDOWN_MS);
OTAUpdater ota(modem, "1.0.0"); // Current firmware version is 1.0.0

// Active configurations (loaded dynamically from Firebase)
DeviceConfig deviceConfig = {
  DEFAULT_HIGH_CURRENT_THRESHOLD,
  DEFAULT_LOW_CURRENT_THRESHOLD,
  DEFAULT_REPORT_INTERVAL_MS / 1000,
  DEFAULT_CONFIG_POLL_MS / 1000,
  "Default Station",
  1.5,
  1.0, // calibration (unused with PZEM but kept for config schema compatibility)
  "", "", ""
};

// State timing tracking
unsigned long lastSensorReadMs = 0;
unsigned long lastDataReportMs = 0;
unsigned long lastConfigPollMs = 0;
unsigned long lastTokenRefreshMs = 0;
unsigned long bootTimeMs = 0;

// Sensor reading average buffers (one per PZEM metric)
float accumCurrent     = 0;
float accumVoltage     = 0;
float accumPower       = 0;
float accumFrequency   = 0;
float accumPowerFactor = 0;
float lastEnergy       = 0; // kWh is cumulative — take last reading, not average
int   sensorSampleCount = 0;

// GPS tracking
double gpsLat = 0.0;
double gpsLng = 0.0;
bool gpsHasFix = false;
unsigned long lastGPSCheckMs = 0;
const unsigned long GPS_CHECK_INTERVAL_MS = 60000; // Check GPS every 1 minute

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("[System] Booting Pumping Station IoT Controller v1.0.0...");

  // Initialize I2C for Battery Fuel Gauge
  Wire.begin(3, 2); // SDA = GPIO 3, SCL = GPIO 2

  // Initialize Status Indicator LED
  statusLed.begin();
  statusLed.setPattern(LED_PATTERN_SOLID);

  // Initialize PZEM-004T Power Meter
  pzemSensor.begin();

  // Initialize UART communication with A7670E Modem
  Serial1.begin(MODEM_UART_BAUDRATE, SERIAL_8N1, MODEM_UART_RX_PIN, MODEM_UART_TX_PIN);
  modem.begin();

  // Power on the modem
  statusLed.setPattern(LED_PATTERN_SLOW_BLINK);
  if (!modem.powerOn()) {
    Serial.println("[System] Modem initialization failed. Resetting system in 10s...");
    delay(10000);
    ESP.restart();
  }

  if (!modem.initializeModem()) {
    Serial.println("[System] SIM missing or modem init failed. Resetting...");
    delay(5000);
    ESP.restart();
  }

  // Register PDP and establish 4G connection
  if (!modem.connectNetwork(CELLULAR_APN)) {
    Serial.println("[System] Network connection failed. Retrying power cycle...");
    modem.hardReset();
    delay(5000);
    ESP.restart();
  }

  // Enable GPS GNSS receiver
  modem.enableGPS(true);

  // Authenticate with Firebase Custom Token
  statusLed.setPattern(LED_PATTERN_FAST_BLINK);
  if (!firebaseAuth.begin()) {
    Serial.println("[System] Firebase Authentication failed. Retrying...");
    delay(10000);
    ESP.restart();
  }

  // Establish online status markers
  if (firebaseData.updateOnlineStatus(true)) {
    Serial.println("[System] Device status updated to ONLINE.");
  }

  // Fetch initial remote configurations
  if (firebaseData.getConfiguration(deviceConfig)) {
    Serial.printf("[System] Config Loaded. High: %.1fA, Low: %.1fA, Int: %ds\n",
                  deviceConfig.highThreshold, deviceConfig.lowThreshold, deviceConfig.reportIntervalSec);
  }

  // Configure Hardware Watchdog
  esp_task_wdt_init(WATCHDOG_TIMEOUT_SECONDS, true);
  esp_task_wdt_add(NULL); // Add current thread to WDT

  // Initialize loop triggers
  bootTimeMs = millis();
  lastSensorReadMs = millis();
  lastDataReportMs = millis();
  lastConfigPollMs = millis();
  lastTokenRefreshMs = millis();

  statusLed.setPattern(LED_PATTERN_HEARTBEAT);
  Serial.println("[System] System ready. Entering main loop.");
}

void loop() {
  // Feed the hardware watchdog timer
  esp_task_wdt_reset();

  // Asynchronously tick the status LED animations
  statusLed.update();

  unsigned long now = millis();

  // 1. Read PZEM-004T sensor periodically (every 5 seconds)
  if (now - lastSensorReadMs >= SENSOR_SAMPLING_INTERVAL_MS) {
    PZEMReading r;
    if (pzemSensor.read(r)) {
      accumCurrent     += r.current;
      accumVoltage     += r.voltage;
      accumPower       += r.power;
      accumFrequency   += r.frequency;
      accumPowerFactor += r.powerFactor;
      lastEnergy        = r.energy; // cumulative kWh — keep latest
      sensorSampleCount++;
      lastSensorReadMs = now;

      Serial.printf("[PZEM] %.2fA  %.1fV  %.1fW  %.3fkWh  %.1fHz  PF:%.2f (Sample %d)\n",
                    r.current, r.voltage, r.power, r.energy,
                    r.frequency, r.powerFactor, sensorSampleCount);

      // Check thresholds on each live sample for edge alerting
      edgeAlert.processReading(r.current, deviceConfig.highThreshold, deviceConfig.lowThreshold);
    } else {
      Serial.println("[PZEM] Read failed — skipping sample.");
      lastSensorReadMs = now; // avoid hammering on failure
    }
  }

  // 2. Report average readings & alerts to Firebase DB
  unsigned long reportIntervalMs = deviceConfig.reportIntervalSec * 1000;
  if (now - lastDataReportMs >= reportIntervalMs && sensorSampleCount > 0) {
    float avgCurrent     = accumCurrent     / sensorSampleCount;
    float avgVoltage     = accumVoltage     / sensorSampleCount;
    float avgPower       = accumPower       / sensorSampleCount;
    float avgFrequency   = accumFrequency   / sensorSampleCount;
    float avgPowerFactor = accumPowerFactor / sensorSampleCount;
    // energy is cumulative from the PZEM chip — use the latest reading
    float reportEnergy   = lastEnergy;

    // Check edge warnings on the reported average
    CurrentAlertType activeAlert = edgeAlert.processReading(
      avgCurrent,
      deviceConfig.highThreshold,
      deviceConfig.lowThreshold
    );

    bool hasAlert = (activeAlert != ALERT_NONE);
    const char* alertTypeStr = edgeAlert.getAlertTypeString();

    int rssi = modem.getSignalQuality();
    uint32_t uptimeSec = (millis() - bootTimeMs) / 1000;

    Serial.printf("[Reporter] Avg: %.2fA  %.1fV  %.1fW  %.3fkWh  PF:%.2f  (Alert: %s, RSSI: %d dBm)\n",
                  avgCurrent, avgVoltage, avgPower, reportEnergy, avgPowerFactor, alertTypeStr, rssi);

    // Set LED pattern to alert if warning triggered
    if (hasAlert) {
      statusLed.setPattern(LED_PATTERN_SOS);
    } else {
      statusLed.setPattern(LED_PATTERN_HEARTBEAT);
    }

    // Read battery voltage & SOC from MAX17048 fuel gauge
    float battVolts   = -1.0;
    float battPercent = -1.0;

    uint16_t vcell = 0xFFFF;
    Wire.beginTransmission(0x36);
    Wire.write(0x02); // VCELL register
    if (Wire.endTransmission(false) == 0) {
      Wire.requestFrom((uint8_t)0x36, (uint8_t)2);
      if (Wire.available() >= 2) {
        uint8_t msb = Wire.read();
        uint8_t lsb = Wire.read();
        vcell = (uint16_t(msb) << 8) | lsb;
        battVolts = (float)vcell * 0.00125 / 16.0;
      }
    }

    uint16_t soc = 0xFFFF;
    Wire.beginTransmission(0x36);
    Wire.write(0x04); // SOC register
    if (Wire.endTransmission(false) == 0) {
      Wire.requestFrom((uint8_t)0x36, (uint8_t)2);
      if (Wire.available() >= 2) {
        uint8_t msb = Wire.read();
        uint8_t lsb = Wire.read();
        soc = (uint16_t(msb) << 8) | lsb;
        battPercent = (float)soc / 256.0;
      }
    }

    if (battVolts >= 0.0) {
      Serial.printf("[Reporter] Battery: %.2f V, Charge: %.1f%%\n", battVolts, battPercent);
    }

    // PUT to DB with all PZEM fields
    firebaseData.putLiveData(
      avgCurrent, avgVoltage, avgPower, reportEnergy,
      avgFrequency, avgPowerFactor,
      hasAlert, alertTypeStr,
      rssi, uptimeSec, "1.0.0",
      battVolts, battPercent
    );

    // Reset accumulation buffers
    accumCurrent     = 0;
    accumVoltage     = 0;
    accumPower       = 0;
    accumFrequency   = 0;
    accumPowerFactor = 0;
    sensorSampleCount = 0;
    lastDataReportMs = now;
  }

  // 3. Poll remote threshold and metadata configurations (every 5 minutes default)
  unsigned long configPollMs = deviceConfig.configPollIntervalSec * 1000;
  if (now - lastConfigPollMs >= configPollMs) {
    DeviceConfig freshConfig;
    if (firebaseData.getConfiguration(freshConfig)) {
      deviceConfig = freshConfig;
      Serial.printf("[Config] Synced remote parameters. High: %.1fA, Low: %.1fA\n",
                    deviceConfig.highThreshold, deviceConfig.lowThreshold);
      
      // Trigger OTA check if new metadata available
      if (deviceConfig.otaVersion.length() > 0) {
        ota.checkAndPerformUpdate(deviceConfig.otaVersion, deviceConfig.otaUrl, deviceConfig.otaChecksum);
      }
    }
    lastConfigPollMs = now;
  }

  // 4. Periodically refresh authorization tokens (every 50 minutes)
  if (now - lastTokenRefreshMs >= TOKEN_REFRESH_INTERVAL_MS) {
    if (firebaseAuth.refreshToken()) {
      Serial.println("[Auth] Expiry token renewed.");
    }
    lastTokenRefreshMs = now;
  }

  // 5. Periodically check GPS location coordinates until a fix is acquired
  if (!gpsHasFix && (now - lastGPSCheckMs >= GPS_CHECK_INTERVAL_MS)) {
    lastGPSCheckMs = now;
    double lat = 0.0;
    double lng = 0.0;
    if (modem.getGPS(lat, lng)) {
      gpsLat = lat;
      gpsLng = lng;
      gpsHasFix = true;
      Serial.printf("[System] GPS Fix acquired! Lat: %.6f, Lng: %.6f\n", gpsLat, gpsLng);
      // Upload location coordinates to the status path
      firebaseData.updateOnlineStatus(true, gpsLat, gpsLng);
    } else {
      Serial.println("[System] GPS checking: No fix yet.");
    }
  }

  // Maintain connectivity checks. Reconnect to network if dropped.
  if (!modem.isNetworkConnected()) {
    Serial.println("[System] Cellular context dropped. Re-establishing link...");
    statusLed.setPattern(LED_PATTERN_SLOW_BLINK);
    
    if (modem.connectNetwork(CELLULAR_APN)) {
      statusLed.setPattern(LED_PATTERN_HEARTBEAT);
      firebaseData.updateOnlineStatus(true);
    } else {
      Serial.println("[System] Failed to reconnect network. Re-triggering hardware power cycle.");
      delay(2000);
      ESP.restart();
    }
  }

  delay(10);
}
