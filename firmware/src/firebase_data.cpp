#include "firebase_data.h"
#include <ArduinoJson.h>

FirebaseData::FirebaseData(A7670Modem& modem, FirebaseAuth& auth, const char* dbUrl, const char* stationId) : _modem(modem), _auth(auth) {
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
      Serial.println("[Data] Auth token invalid and refresh failed. Skipping telemetry write.");
      return false;
    }
  }

  String url = _dbUrl + String("/stations/") + _stationId + String("/live.json?auth=") + String(_auth.getIdToken());
  
  StaticJsonDocument<768> doc;
  // Core current readings
  doc["current"]     = current;
  doc["voltage"]     = voltage;
  doc["power"]       = power;
  doc["energy"]      = energy;
  doc["frequency"]   = frequency;
  doc["powerFactor"] = powerFactor;

  // Alert state
  doc["alert"] = alert;
  if (alert && alertType) {
    doc["alertType"] = alertType;
  } else {
    doc["alertType"] = nullptr;
  }

  // Modem / device metadata
  doc["rssi"]            = rssi;
  doc["uptimeSeconds"]   = uptimeSec;
  doc["firmwareVersion"] = firmwareVersion;

  if (battVolts >= 0.0) {
    doc["battVolts"] = battVolts;
  }
  if (battPercent >= 0.0) {
    doc["battPercent"] = battPercent;
  }
  
  // Server-side timestamp
  JsonObject tsObj = doc.createNestedObject("timestamp");
  tsObj[".sv"] = "timestamp";

  String body;
  serializeJson(doc, body);

  HttpResponse res = _modem.httpsRequest("PUT", url.c_str(), body.c_str());
  
  if (res.status == 200) {
    Serial.println("[Data] Live readings written successfully to RTDB.");
    return true;
  } else {
    Serial.printf("[Data] Failed to write live readings. HTTP Code: %d, Response: %s\n", res.status, res.body.c_str());
    return false;
  }
}

bool FirebaseData::getConfiguration(DeviceConfig& configOut) {
  if (!_auth.isTokenValid()) {
    if (!_auth.refreshToken()) {
      Serial.println("[Data] Auth token invalid and refresh failed. Skipping configuration read.");
      return false;
    }
  }

  String url = _dbUrl + String("/stations/") + _stationId + String("/config.json?auth=") + String(_auth.getIdToken());
  HttpResponse res = _modem.httpsRequest("GET", url.c_str());

  if (res.status == 200) {
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, res.body);
    
    if (!error) {
      configOut.highThreshold = doc["highThreshold"].as<float>();
      configOut.lowThreshold = doc["lowThreshold"].as<float>();
      configOut.reportIntervalSec = doc["reportIntervalSec"].as<int>();
      configOut.configPollIntervalSec = doc["configPollIntervalSec"].as<int>();
      configOut.stationName = doc["stationName"].as<String>();
      configOut.pumpPowerKW = doc["pumpPowerKW"].as<float>();
      configOut.calibration = doc["calibration"].as<float>();
      
      if (doc.containsKey("latestFirmware")) {
        JsonObject ota = doc["latestFirmware"];
        configOut.otaVersion = ota["version"].as<String>();
        configOut.otaUrl = ota["url"].as<String>();
        configOut.otaChecksum = ota["checksum"].as<String>();
      }
      
      Serial.println("[Data] Configurations synced successfully from RTDB.");
      return true;
    } else {
      Serial.printf("[Data] Configuration JSON parsing failed: %s\n", error.c_str());
    }
  } else {
    Serial.printf("[Data] Configuration read failed. HTTP Code: %d\n", res.status);
  }
  
  return false;
}

bool FirebaseData::updateOnlineStatus(bool online, double lat, double lng) {
  if (!_auth.isTokenValid()) {
    if (!_auth.refreshToken()) {
      return false;
    }
  }

  String url = _dbUrl + String("/stations/") + _stationId + String("/status.json?auth=") + String(_auth.getIdToken());
  
  StaticJsonDocument<256> doc;
  doc["online"] = online;
  
  JsonObject tsObj = doc.createNestedObject("lastSeen");
  tsObj[".sv"] = "timestamp";
  
  if (online) {
    if (lat != 0.0 && lng != 0.0) {
      doc["lat"] = lat;
      doc["lng"] = lng;
    }
  }

  String body;
  serializeJson(doc, body);

  HttpResponse res = _modem.httpsRequest("PUT", url.c_str(), body.c_str());
  
  return (res.status == 200);
}
