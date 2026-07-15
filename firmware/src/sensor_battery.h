#pragma once
#include "Arduino.h"

struct BatteryReading {
    float   voltage;    // V  (cell voltage)
    float   percent;    // % SOC  (0–100)
    bool    valid;
};

void           battery_init();
BatteryReading battery_read();
bool           battery_is_ok();
