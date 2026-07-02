#include "sensor.h"
#include <math.h>

CTSensor::CTSensor(uint8_t adcPin, float calibration, float vref, float resolution, float biasVoltage) {
  _adcPin = adcPin;
  _calibration = calibration;
  _vref = vref;
  _resolution = resolution;
  _biasVoltage = biasVoltage;
}

void CTSensor::begin() {
  pinMode(_adcPin, INPUT);
  analogReadResolution(12); // ESP32 supports 12-bit ADC
}

float CTSensor::readCurrentRMS(uint16_t sampleCount, uint16_t samplePeriodUs) {
  double sumSquares = 0;
  
  // Read sampleCount samples at samplePeriodUs spacing (200us sampling is 5kHz, captures 50Hz/60Hz waves)
  for (uint16_t i = 0; i < sampleCount; i++) {
    int raw = analogRead(_adcPin);
    double voltage = (raw / _resolution) * _vref;
    
    // Subtract midpoint offset (e.g. 1.65V bias)
    double currentVal = (voltage - _biasVoltage) * _calibration;
    
    sumSquares += currentVal * currentVal;
    delayMicroseconds(samplePeriodUs);
  }
  
  double rms = sqrt(sumSquares / sampleCount);
  
  // Low-current cutoff threshold to eliminate ADC noise floor when pump is fully off
  if (rms < 0.15) {
    rms = 0.0;
  }
  
  return (float)rms;
}
