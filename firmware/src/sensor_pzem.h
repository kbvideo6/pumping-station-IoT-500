#pragma once
#include "Arduino.h"

// Averaged PZEM-004T v3.0 reading.
// Averages PZEM_SAMPLE_INTERVAL_MS samples across 30s,
// then the uploader consumes one PzemReading per upload cycle.

struct PzemReading {
    float voltage;      // V
    float current;      // A
    float power;        // W
    float energy;       // kWh  (cumulative from meter)
    float frequency;    // Hz
    float powerFactor;  // 0.00–1.00
    bool  valid;        // false if PZEM not responding
};

void         pzem_init();
void         pzem_tick();          // Call every loop — accumulates samples
PzemReading  pzem_get_average();   // Returns averaged snapshot, resets accumulator
void         pzem_reset_energy();  // Reset PZEM internal energy counter (EEPROM)
bool         pzem_is_ok();         // True if last read was valid
