#include "ota.h"
#include "esp_log.h"
#include "esp_system.h"

static const char* TAG = "OTAUpdater";

OTAUpdater::OTAUpdater(A7670Modem& modem, const char* currentVersion) : _modem(modem) {
  _currentVersion = currentVersion;
}

bool OTAUpdater::checkAndPerformUpdate(const std::string& latestVersion, const std::string& binaryUrl, const std::string& expectedChecksum) {
  if (latestVersion.empty() || binaryUrl.empty()) {
    return false;
  }

  if (latestVersion == _currentVersion) {
    ESP_LOGI(TAG, "Firmware is already up to date.");
    return false;
  }

  ESP_LOGI(TAG, "New version found: %s. Current: %s. Starting update...", latestVersion.c_str(), _currentVersion.c_str());
  ESP_LOGI(TAG, "Simulated OTA step: firmware version matched, but download skipped in simulation.");
  return false;
}

bool OTAUpdater::downloadFirmware(const std::string& url, const std::string& filename) {
  return true;
}

bool OTAUpdater::applyUpdate(const std::string& filename) {
  return true;
}
