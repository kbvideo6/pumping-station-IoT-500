#ifndef SENSOR_H
#define SENSOR_H

#include <stdint.h>
#include "esp_adc/adc_oneshot.h"

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
  
  adc_oneshot_unit_handle_t _adc_handle;
  adc_channel_t _adc_channel;
};

#endif // SENSOR_H
