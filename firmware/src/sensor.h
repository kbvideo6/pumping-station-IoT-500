#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>

class CTSensor {
public:
  CTSensor(uint8_t adcPin, float calibration, float vref, float resolution, float biasVoltage);
  void begin();
  float readCurrentRMS(uint16_t sampleCount = 1000, uint16_t samplePeriodUs = 200);

private:
  uint8_t _adcPin;
  float _calibration;
  float _vref;
  float _resolution;
  float _biasVoltage;
};

#endif // SENSOR_H
