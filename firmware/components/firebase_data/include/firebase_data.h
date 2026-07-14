#ifndef FIREBASE_DATA_H
#define FIREBASE_DATA_H

#include <string>
#include "modem.h"
#include "firebase_auth.h"

struct DeviceConfig {
  float highThreshold;
  float lowThreshold;
  int reportIntervalSec;
  int configPollIntervalSec;
  std::string stationName;
  float pumpPowerKW;
  float calibration;
  std::string otaVersion;
  std::string otaUrl;
  std::string otaChecksum;
};

class FirebaseData {
public:
  FirebaseData(A7670Modem& modem, FirebaseAuth& auth, const char* dbUrl, const char* stationId);
  
  bool putLiveData(
    float current, float voltage, float power, float energy,
    float frequency, float powerFactor,
    bool alert, const char* alertType,
    int rssi, uint32_t uptimeSec, const char* firmwareVersion,
    float battVolts = -1.0, float battPercent = -1.0
  );
  bool getConfiguration(DeviceConfig& configOut);
  bool updateOnlineStatus(bool online, double lat = 0.0, double lng = 0.0);

private:
  A7670Modem& _modem;
  FirebaseAuth& _auth;
  std::string _dbUrl;
  std::string _stationId;
};

#endif // FIREBASE_DATA_H
