#ifndef MODEM_H
#define MODEM_H

#include <stdint.h>
#include <string>
#include "driver/uart.h"
#include "driver/gpio.h"

struct HttpResponse {
  int status;
  std::string body;
};

class A7670Modem {
public:
  A7670Modem(int uartPort, int pwrPin, int rstPin, int flightPin);
  void begin();
  bool powerOn();
  void powerOff();
  void hardReset();
  bool ping();
  bool softReset();
  
  bool initializeModem();
  bool checkSIMStatus();
  int getSignalQuality();
  bool connectNetwork(const char* apn);
  bool isNetworkConnected();

  bool enableGPS(bool enable);
  bool getGPS(double& latitude, double& longitude);

  HttpResponse httpsRequest(const char* method, const char* url, const char* body = "", const char* bearerToken = "");

private:
  uart_port_t _uartPort;
  int _pwrPin;
  int _rstPin;
  int _flightPin;
  
  bool sendATCommand(const std::string& cmd, std::string& response, uint32_t timeoutMs = 2000);
  bool waitForResponse(const std::string& expected, uint32_t timeoutMs);
  void clearSerialBuffer();
};

#endif // MODEM_H
