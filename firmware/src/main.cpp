#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/i2c.h"

#include "config.h"
#include "led.h"
#include "pzem_sensor.h"
#include "modem.h"
#include "battery_gauge.h"
#include "firebase_auth.h"
#include "firebase_data.h"
#include "alerts.h"
#include "ota.h"

static const char* TAG = "Main";

// Hardware instances
StatusLED statusLed((gpio_num_t)STATUS_LED_PIN);
PZEMSensor pzemSensor(UART_NUM_2, PZEM_UART_RX_PIN, PZEM_UART_TX_PIN, PZEM_MODBUS_ADDR);
A7670Modem modem(UART_NUM_1, MODEM_PWRKEY_PIN, MODEM_RESET_PIN, MODEM_FLIGHT_PIN);
Max17048BatteryGauge batteryGauge(BATTERY_GAUGE_I2C_ADDR);
FirebaseAuth firebaseAuth(modem, FIREBASE_API_KEY, DEFAULT_CUSTOM_TOKEN, DEFAULT_STATION_ID);
FirebaseData firebaseData(modem, firebaseAuth, FIREBASE_DB_URL, DEFAULT_STATION_ID);
EdgeAlert edgeAlert(ALERT_DEBOUNCE_COUNT, ALERT_COOLDOWN_MS);
OTAUpdater ota(modem, "1.0.0"); // Current version 1.0.0

// Active configurations
DeviceConfig deviceConfig = {
  DEFAULT_HIGH_CURRENT_THRESHOLD,
  DEFAULT_LOW_CURRENT_THRESHOLD,
  DEFAULT_REPORT_INTERVAL_MS / 1000,
  DEFAULT_CONFIG_POLL_MS / 1000,
  "Default Station",
  1.5,
  1.0,
  "", "", ""
};

// Timing tracking
uint64_t lastSensorReadMs = 0;
uint64_t lastDataReportMs = 0;
uint64_t lastConfigPollMs = 0;
uint64_t bootTimeMs = 0;
uint64_t lastModemWatchdogMs = 0;
int modemFailCount = 0;
bool modemRecoveryPending = false;
uint64_t modemRecoveryReadyMs = 0;

// Sensor reading accumulation buffers
float accumCurrent     = 0;
float accumVoltage     = 0;
float accumPower       = 0;
float accumFrequency   = 0;
float accumPowerFactor = 0;
float lastEnergy       = 0;
int   sensorSampleCount = 0;

// GPS tracking
double gpsLat = 0.0;
double gpsLng = 0.0;
bool gpsHasFix = false;
uint64_t lastGPSCheckMs = 0;
const uint64_t GPS_CHECK_INTERVAL_MS = 60000;

// Asynchronous LED pattern processing task
void led_task(void *pvParameters) {
  while (1) {
    statusLed.update();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

bool bringUpModem() {
  if (!modem.initializeModem()) {
    return false;
  }
  if (!modem.connectNetwork(CELLULAR_APN)) {
    return false;
  }
  modem.enableGPS(true);
  return true;
}

extern "C" void app_main(void) {
  // 1. Initialize NVS (Required for credentials caching & WiFi/BT configuration in ESP-IDF)
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG, "============================================");
  ESP_LOGI(TAG, "Booting Pumping Station IoT Controller v1.0.0...");
  ESP_LOGI(TAG, "============================================");

  // 2. Initialize I2C for MAX17048 Battery Gauge
  i2c_config_t i2c_conf;
  memset(&i2c_conf, 0, sizeof(i2c_conf));
  i2c_conf.mode = I2C_MODE_MASTER;
  i2c_conf.sda_io_num = GPIO_NUM_3;
  i2c_conf.scl_io_num = GPIO_NUM_2;
  i2c_conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  i2c_conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  i2c_conf.master.clk_speed = 100000;
  i2c_conf.clk_flags = 0;
  i2c_param_config(I2C_NUM_0, &i2c_conf);
  i2c_driver_install(I2C_NUM_0, i2c_conf.mode, 0, 0, 0);

  if (batteryGauge.begin(I2C_NUM_0)) {
    ESP_LOGI(TAG, "MAX17048 battery fuel gauge detected.");
  } else {
    ESP_LOGW(TAG, "MAX17048 battery fuel gauge not detected. Battery readings disabled.");
  }

  // 3. Initialize Status LED and start task
  statusLed.begin();
  statusLed.setPattern(LED_PATTERN_SOLID);
  xTaskCreate(led_task, "led_task", 2048, NULL, 5, NULL);

  // 4. Initialize PZEM Power Meter
  pzemSensor.begin();

  // 5. Initialize A7670E Modem
  modem.begin();

  // 6. Power on & Diagnose Modem
  statusLed.setPattern(LED_PATTERN_SLOW_BLINK);
  ESP_LOGI(TAG, "Powering on cellular modem...");
  if (!modem.powerOn()) {
    ESP_LOGE(TAG, "Modem power-on failed. Retrying...");
    vTaskDelay(pdMS_TO_TICKS(5000));
    if (!modem.powerOn()) {
      ESP_LOGE(TAG, "Modem power-on failed again. System will reboot.");
      vTaskDelay(pdMS_TO_TICKS(2000));
      esp_restart();
    }
  }

  if (!bringUpModem()) {
    ESP_LOGW(TAG, "Cellular connection failed. Retrying...");
    vTaskDelay(pdMS_TO_TICKS(5000));
    if (!bringUpModem()) {
      ESP_LOGE(TAG, "Network connection completely failed. System will reboot.");
      vTaskDelay(pdMS_TO_TICKS(2000));
      esp_restart();
    }
  }

  modemRecoveryPending = false;
  modemFailCount = 0;

  // 7. Authenticate with Firebase
  statusLed.setPattern(LED_PATTERN_FAST_BLINK);
  ESP_LOGI(TAG, "Authenticating with Firebase...");
  
  bool authenticated = false;
  for (int attempt = 0; attempt < 3; attempt++) {
    if (firebaseAuth.begin()) {
      authenticated = true;
      break;
    }
    ESP_LOGW(TAG, "Firebase authentication failed (attempt %d/3).", attempt + 1);
    vTaskDelay(pdMS_TO_TICKS(5000));
  }

  if (!authenticated) {
    ESP_LOGE(TAG, "Firebase authentication failed after retries. Restarting board...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
  }

  // Set online status
  if (firebaseData.updateOnlineStatus(true)) {
    ESP_LOGI(TAG, "Device status set to ONLINE.");
  }

  // Fetch remote config parameters
  if (firebaseData.getConfiguration(deviceConfig)) {
    ESP_LOGI(TAG, "Config loaded. High: %.1fA, Low: %.1fA, Int: %ds",
             deviceConfig.highThreshold, deviceConfig.lowThreshold, deviceConfig.reportIntervalSec);
  }

  // 8. Main loop timing initialization
  bootTimeMs = esp_timer_get_time() / 1000;
  lastSensorReadMs = bootTimeMs;
  lastDataReportMs = bootTimeMs;
  lastConfigPollMs = bootTimeMs;
  lastGPSCheckMs = bootTimeMs;

  statusLed.setPattern(LED_PATTERN_HEARTBEAT);
  ESP_LOGI(TAG, "System ready. Entering main execution loop.");

  while (1) {
    uint64_t now = esp_timer_get_time() / 1000;

    // 1. Read PZEM-004T sensor periodically (every 5 seconds)
    if (now - lastSensorReadMs >= SENSOR_SAMPLING_INTERVAL_MS) {
      PZEMReading r;
      if (pzemSensor.read(r)) {
        accumCurrent     += r.current;
        accumVoltage     += r.voltage;
        accumPower       += r.power;
        accumFrequency   += r.frequency;
        accumPowerFactor += r.powerFactor;
        lastEnergy        = r.energy;
        sensorSampleCount++;
        lastSensorReadMs = now;

        ESP_LOGI(TAG, "[PZEM] %.2fA  %.1fV  %.1fW  %.3fkWh  %.1fHz  PF:%.2f (Sample %d)",
                      r.current, r.voltage, r.power, r.energy,
                      r.frequency, r.powerFactor, sensorSampleCount);

        edgeAlert.processReading(r.current, deviceConfig.highThreshold, deviceConfig.lowThreshold);
      } else {
        ESP_LOGW(TAG, "[PZEM] Read failed. Checking sensor communication.");
        lastSensorReadMs = now;
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
      float reportEnergy   = lastEnergy;

      CurrentAlertType activeAlert = edgeAlert.processReading(
        avgCurrent,
        deviceConfig.highThreshold,
        deviceConfig.lowThreshold
      );

      bool hasAlert = (activeAlert != ALERT_NONE);
      const char* alertTypeStr = edgeAlert.getAlertTypeString();

      int rssi = modem.getSignalQuality();
      uint32_t uptimeSec = (esp_timer_get_time() / 1000 - bootTimeMs) / 1000;

      ESP_LOGI(TAG, "[Reporter] Avg: %.2fA  %.1fV  %.1fW  %.3fkWh  PF:%.2f  (Alert: %s, RSSI: %d dBm)",
                    avgCurrent, avgVoltage, avgPower, reportEnergy, avgPowerFactor, alertTypeStr, rssi);

      if (hasAlert) {
        statusLed.setPattern(LED_PATTERN_SOS);
      } else {
        statusLed.setPattern(LED_PATTERN_HEARTBEAT);
      }

      float battVolts   = -1.0;
      float battPercent = -1.0;
      batteryGauge.read(battVolts, battPercent);

      firebaseData.putLiveData(
        avgCurrent, avgVoltage, avgPower, reportEnergy,
        avgFrequency, avgPowerFactor,
        hasAlert, alertTypeStr,
        rssi, uptimeSec, "1.0.0",
        battVolts, battPercent
      );

      // Reset buffers
      accumCurrent     = 0;
      accumVoltage     = 0;
      accumPower       = 0;
      accumFrequency   = 0;
      accumPowerFactor = 0;
      sensorSampleCount = 0;
      lastDataReportMs = now;
    }

    // 3. Poll remote configurations (every 5 minutes default)
    unsigned long configPollMs = deviceConfig.configPollIntervalSec * 1000;
    if (now - lastConfigPollMs >= configPollMs) {
      DeviceConfig freshConfig;
      if (firebaseData.getConfiguration(freshConfig)) {
        deviceConfig = freshConfig;
        ESP_LOGI(TAG, "[Config] Synced parameters. High: %.1fA, Low: %.1fA",
                      deviceConfig.highThreshold, deviceConfig.lowThreshold);
        
        if (!deviceConfig.otaVersion.empty()) {
          ota.checkAndPerformUpdate(deviceConfig.otaVersion, deviceConfig.otaUrl, deviceConfig.otaChecksum);
        }
      }
      lastConfigPollMs = now;
    }

    // 4. Check GPS location (every 1 minute until fix)
    if (!gpsHasFix && (now - lastGPSCheckMs >= GPS_CHECK_INTERVAL_MS)) {
      lastGPSCheckMs = now;
      double lat = 0.0;
      double lng = 0.0;
      if (modem.getGPS(lat, lng)) {
        gpsLat = lat;
        gpsLng = lng;
        gpsHasFix = true;
        ESP_LOGI(TAG, "GPS Fix acquired! Lat: %.6f, Lng: %.6f", gpsLat, gpsLng);
        firebaseData.updateOnlineStatus(true, gpsLat, gpsLng);
      } else {
        ESP_LOGI(TAG, "GPS checking: No fix yet.");
      }
    }

    // 5. Modem AT watchdog and self-healing recovery (every 1 second)
    if (now - lastModemWatchdogMs >= MODEM_WATCHDOG_INTERVAL_MS) {
      lastModemWatchdogMs = now;

      if (modemRecoveryPending) {
        if (now >= modemRecoveryReadyMs) {
          ESP_LOGI(TAG, "Modem recovery window elapsed. Re-initializing UART and modem...");
          uart_driver_delete(UART_NUM_1);
          modem.begin();

          if (bringUpModem()) {
            statusLed.setPattern(LED_PATTERN_HEARTBEAT);
            firebaseData.updateOnlineStatus(true);
            modemRecoveryPending = false;
            modemFailCount = 0;
          } else {
            ESP_LOGE(TAG, "Modem still not ready after reset recovery.");
            modemRecoveryReadyMs = now + MODEM_SOFT_RESET_RECOVERY_WAIT_MS;
          }
        }
      } else {
        if (modem.ping()) {
          modemFailCount = 0;
        } else {
          modemFailCount++;
          ESP_LOGW(TAG, "Modem AT watchdog failure #%d / %d",
                        modemFailCount, MODEM_FAILS_BEFORE_SOFT_RESET);

          if (modemFailCount >= MODEM_FAILS_BEFORE_SOFT_RESET) {
            modem.softReset();
            modemRecoveryPending = true;
            modemRecoveryReadyMs = esp_timer_get_time() / 1000;
            modemFailCount = 0;
          }
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10)); // Yield to CPU
  }
}
