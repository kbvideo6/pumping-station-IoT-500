#ifndef MODEM_H
#define MODEM_H

#include <Arduino.h>

struct HttpResponse {
  int status;
  String body;
};

class A7670Modem {
public:
  A7670Modem(HardwareSerial& serialPort, int pwrPin, int rstPin);
  void begin();
  bool powerOn();
  void powerOff();
  void hardReset();
  
  bool initializeModem();
  bool checkSIMStatus();
  int getSignalQuality(); // Returns RSSI in dBm
  bool connectNetwork(const char* apn);
  bool isNetworkConnected();

  // GPS/GNSS methods
  bool enableGPS(bool enable);
  bool getGPS(double& latitude, double& longitude);

  HttpResponse httpsRequest(const char* method, const char* url, const char* body = "", const char* bearerToken = "");

private:
  HardwareSerial& _serial;
  int _pwrPin;
  int _rstPin;
  
  bool sendATCommand(const String& cmd, String& response, uint32_t timeoutMs = 2000);
  bool waitForResponse(const String& expected, uint32_t timeoutMs);
  void clearSerialBuffer();
};

#endif // MODEM_H
