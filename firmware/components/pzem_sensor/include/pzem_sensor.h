#ifndef PZEM_SENSOR_H
#define PZEM_SENSOR_H

#include <stdint.h>
#include "driver/uart.h"

struct PZEMReading {
  float voltage;
  float current;
  float power;
  float energy;
  float frequency;
  float powerFactor;
};

class PZEMSensor {
public:
  PZEMSensor(uart_port_t uartPort, int rxPin, int txPin, uint8_t addr = 0xF8);
  void begin();
  bool read(PZEMReading& out);
  bool resetEnergy();

private:
  uart_port_t _uartPort;
  int _rxPin;
  int _txPin;
  uint8_t _addr;

  uint16_t calculateCRC(const uint8_t* data, uint16_t len);
};

#endif // PZEM_SENSOR_H
