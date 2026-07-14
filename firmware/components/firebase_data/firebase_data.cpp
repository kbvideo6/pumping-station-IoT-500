#include "firebase_data.h"
#include "cJSON.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "FirebaseData";

FirebaseData::FirebaseData(A7670Modem& modem, FirebaseAuth& auth, const char* dbUrl, const char* stationId) 
  : _modem(modem), _auth(auth) {
  _dbUrl = dbUrl;
  _stationId = stationId;
}

bool FirebaseData::putLiveData(
    float current, float voltage, float power, float energy,
    float frequency, float powerFactor,
    bool alert, const char* alertType,
    int rssi, uint32_t uptimeSec, const char* firmwareVersion,
    float battVolts, float battPercent) {
    
  if (!_auth.isTokenValid()) {
    if (!_auth.refreshToken()) {
      ESP_LOGE(TAG, "Auth token invalid and refresh failed. Skipping telemetry write.");
      return false;
    }
  }

  std::string url = _dbUrl + "/stations/" + _stationId + "/live.json?auth=" + _auth.getIdToken();
  
  cJSON *root = cJSON_CreateObject();
  cJSON_AddNumberToObject(root, "current", current);
  cJSON_AddNumberToObject(root, "voltage", voltage);
  cJSON_AddNumberToObject(root, "power", power);
  cJSON_AddNumberToObject(root, "energy", energy);
  cJSON_AddNumberToObject(root, "frequency", frequency);
  cJSON_AddNumberToObject(root, "powerFactor", powerFactor);

  cJSON_AddBoolToObject(root, "alert", alert);
  if (alert && alertType) {
    cJSON_AddStringToObject(root, "alertType", alertType);
  } else {
    cJSON_AddNullToObject(root, "alertType");
  }

  cJSON_AddNumberToObject(root, "rssi", rssi);
  cJSON_AddNumberToObject(root, "uptimeSeconds", uptimeSec);
  cJSON_AddStringToObject(root, "firmwareVersion", firmwareVersion);

  if (battVolts >= 0.0) {
    cJSON_AddNumberToObject(root, "battVolts", battVolts);
  }
  if (battPercent >= 0.0) {
    cJSON_AddNumberToObject(root, "battPercent", battPercent);
  }
  
  cJSON *tsObj = cJSON_CreateObject();
  cJSON_AddStringToObject(tsObj, ".sv", "timestamp");
  cJSON_AddItemToObject(root, "timestamp", tsObj);

  char *body = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  HttpResponse res = _modem.httpsRequest("PUT", url.c_str(), body);
  free(body);
  
  if (res.status == 200) {
    ESP_LOGI(TAG, "Live readings written successfully to RTDB.");
    return true;
  } else {
    ESP_LOGE(TAG, "Failed to write live readings. HTTP Code: %d, Response: %s", res.status, res.body.c_str());
    return false;
  }
}

bool FirebaseData::getConfiguration(DeviceConfig& configOut) {
  if (!_auth.isTokenValid()) {
    if (!_auth.refreshToken()) {
      ESP_LOGE(TAG, "Auth token invalid and refresh failed. Skipping configuration read.");
      return false;
    }
  }

  std::string url = _dbUrl + "/stations/" + _stationId + "/config.json?auth=" + _auth.getIdToken();
  HttpResponse res = _modem.httpsRequest("GET", url.c_str());

  if (res.status == 200) {
    cJSON *doc = cJSON_Parse(res.body.c_str());
    if (doc) {
      cJSON *high = cJSON_GetObjectItemCaseSensitive(doc, "highThreshold");
      cJSON *low = cJSON_GetObjectItemCaseSensitive(doc, "lowThreshold");
      cJSON *repInt = cJSON_GetObjectItemCaseSensitive(doc, "reportIntervalSec");
      cJSON *pollInt = cJSON_GetObjectItemCaseSensitive(doc, "configPollIntervalSec");
      cJSON *name = cJSON_GetObjectItemCaseSensitive(doc, "stationName");
      cJSON *power = cJSON_GetObjectItemCaseSensitive(doc, "pumpPowerKW");
      cJSON *calib = cJSON_GetObjectItemCaseSensitive(doc, "calibration");

      if (high && cJSON_IsNumber(high)) configOut.highThreshold = high->valuedouble;
      if (low && cJSON_IsNumber(low)) configOut.lowThreshold = low->valuedouble;
      if (repInt && cJSON_IsNumber(repInt)) configOut.reportIntervalSec = repInt->valueint;
      if (pollInt && cJSON_IsNumber(pollInt)) configOut.configPollIntervalSec = pollInt->valueint;
      if (name && cJSON_IsString(name)) configOut.stationName = name->valuestring;
      if (power && cJSON_IsNumber(power)) configOut.pumpPowerKW = power->valuedouble;
      if (calib && cJSON_IsNumber(calib)) configOut.calibration = calib->valuedouble;

      cJSON *latestFirmware = cJSON_GetObjectItemCaseSensitive(doc, "latestFirmware");
      if (latestFirmware && cJSON_IsObject(latestFirmware)) {
        cJSON *ver = cJSON_GetObjectItemCaseSensitive(latestFirmware, "version");
        cJSON *urlObj = cJSON_GetObjectItemCaseSensitive(latestFirmware, "url");
        cJSON *chk = cJSON_GetObjectItemCaseSensitive(latestFirmware, "checksum");

        if (ver && cJSON_IsString(ver)) configOut.otaVersion = ver->valuestring;
        if (urlObj && cJSON_IsString(urlObj)) configOut.otaUrl = urlObj->valuestring;
        if (chk && cJSON_IsString(chk)) configOut.otaChecksum = chk->valuestring;
      }
      
      ESP_LOGI(TAG, "Configurations synced successfully from RTDB.");
      cJSON_Delete(doc);
      return true;
    } else {
      ESP_LOGE(TAG, "Configuration JSON parsing failed: %s", res.body.c_str());
    }
  } else {
    ESP_LOGE(TAG, "Configuration read failed. HTTP Code: %d", res.status);
  }
  
  return false;
}

bool FirebaseData::updateOnlineStatus(bool online, double lat, double lng) {
  if (!_auth.isTokenValid()) {
    if (!_auth.refreshToken()) {
      return false;
    }
  }

  std::string url = _dbUrl + "/stations/" + _stationId + "/status.json?auth=" + _auth.getIdToken();
  
  cJSON *root = cJSON_CreateObject();
  cJSON_AddBoolToObject(root, "online", online);
  
  cJSON *tsObj = cJSON_CreateObject();
  cJSON_AddStringToObject(tsObj, ".sv", "timestamp");
  cJSON_AddItemToObject(root, "lastSeen", tsObj);
  
  if (online) {
    if (lat != 0.0 && lng != 0.0) {
      cJSON_AddNumberToObject(root, "lat", lat);
      cJSON_AddNumberToObject(root, "lng", lng);
    }
  }

  char *body = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  HttpResponse res = _modem.httpsRequest("PUT", url.c_str(), body);
  free(body);
  
  return (res.status == 200);
}
