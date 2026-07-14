#ifndef OTA_H
#define OTA_H

#include <string>
#include "modem.h"

class OTAUpdater {
public:
  OTAUpdater(A7670Modem& modem, const char* currentVersion);
  bool checkAndPerformUpdate(const std::string& latestVersion, const std::string& binaryUrl, const std::string& expectedChecksum);

private:
  A7670Modem& _modem;
  std::string _currentVersion;
  
  bool downloadFirmware(const std::string& url, const std::string& filename);
  bool applyUpdate(const std::string& filename);
};

#endif // OTA_H
