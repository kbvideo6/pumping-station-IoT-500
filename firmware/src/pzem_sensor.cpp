#include "pzem_sensor.h"
#include <math.h>

PZEMSensor::PZEMSensor(HardwareSerial& serial, uint8_t rxPin, uint8_t txPin, uint8_t addr)
  : _pzem(serial, rxPin, txPin, addr) {}

void PZEMSensor::begin() {
  // PZEM004Tv30 constructor already begins the serial port.
  // Nothing extra needed here; kept for symmetry with other sensor modules.
  Serial.println("[PZEM] Sensor initialized on Serial2.");
}

bool PZEMSensor::read(PZEMReading& out) {
  float v  = _pzem.voltage();
  float a  = _pzem.current();
  float w  = _pzem.power();
  float wh = _pzem.energy();
  float hz = _pzem.frequency();
  float pf = _pzem.pf();

  // PZEM returns NaN for every register when communication fails
  if (isnan(v) || isnan(a) || isnan(w) || isnan(wh) || isnan(hz) || isnan(pf)) {
    Serial.println("[PZEM] Read failed: NaN received. Check wiring or PZEM power.");
    return false;
  }

  out.voltage     = v;
  out.current     = a;
  out.power       = w;
  out.energy      = wh;
  out.frequency   = hz;
  out.powerFactor = pf;
  return true;
}

bool PZEMSensor::resetEnergy() {
  bool ok = _pzem.resetEnergy();
  if (ok) {
    Serial.println("[PZEM] Energy accumulator reset successfully.");
  } else {
    Serial.println("[PZEM] Energy reset failed.");
  }
  return ok;
}
