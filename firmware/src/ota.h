#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include "modem.h"

class OTAUpdater {
public:
  OTAUpdater(A7670Modem& modem, const char* currentVersion);
  
  bool checkAndPerformUpdate(const String& latestVersion, const String& binaryUrl, const String& expectedChecksum);

private:
  A7670Modem& _modem;
  String _currentVersion;
  
  bool downloadFirmware(const String& url, const String& filename);
  bool applyUpdate(const String& filename);
};

#endif // OTA_H
