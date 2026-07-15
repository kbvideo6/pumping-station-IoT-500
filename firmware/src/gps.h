#pragma once
#include "Arduino.h"

struct GpsReading {
    float lat;
    float lng;
    bool  fixed;   // true if a valid fix was obtained
};

// gps_init()  — enable GNSS power (AT+CGNSPWR=1)
// gps_tick()  — call from loop(); non-blocking poll every GPS_POLL_INTERVAL_MS
//               Stores lat/lng in NVS when a fix is received.
// gps_get()   — returns last known position (from NVS if no live fix yet)
// gps_has_fix()— true if at least one fix has been obtained this session

void       gps_init();
void       gps_tick();
GpsReading gps_get();
bool       gps_has_fix();
