#ifndef BATTERY_GAUGE_H
#define BATTERY_GAUGE_H

#include <stdint.h>
#include "driver/i2c.h"

class Max17048BatteryGauge {
public:
  explicit Max17048BatteryGauge(uint8_t address = 0x36);
  bool begin(i2c_port_t i2cPort);
  bool read(float& volts, float& percent);

private:
  bool readRegister16(uint8_t reg, uint16_t& value);

  i2c_port_t _i2cPort;
  uint8_t _address;
};

#endif // BATTERY_GAUGE_H
