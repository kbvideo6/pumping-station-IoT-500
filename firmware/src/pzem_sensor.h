#ifndef PZEM_SENSOR_H
#define PZEM_SENSOR_H

#include <Arduino.h>
#include <PZEM004Tv30.h>

// All measurements delivered in one Modbus read from the PZEM-004T
struct PZEMReading {
  float voltage;     // V  (e.g. 230.4)
  float current;     // A  (e.g. 12.35)
  float power;       // W  (e.g. 2850.0)
  float energy;      // kWh (accumulated, persists across power cycles)
  float frequency;   // Hz (e.g. 50.0)
  float powerFactor; // 0.0-1.0 (e.g. 0.93)
};

class PZEMSensor {
public:
  // Pass the HardwareSerial port and the TX/RX pins to use
  PZEMSensor(HardwareSerial& serial, uint8_t rxPin, uint8_t txPin, uint8_t addr = PZEM_DEFAULT_ADDR);

  // Initialize Serial2 and PZEM library
  void begin();

  // Read all measurements into 'out'. Returns false if sensor is offline or
  // any value is NaN (PZEM returns NaN when it cannot communicate).
  bool read(PZEMReading& out);

  // Reset the energy accumulator (kWh counter) on the PZEM chip
  bool resetEnergy();

private:
  PZEM004Tv30 _pzem;
};

#endif // PZEM_SENSOR_H
