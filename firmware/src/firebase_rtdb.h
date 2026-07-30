#pragma once
#include "Arduino.h"
#include "sensor_pzem.h"
#include "sensor_battery.h"
#include "gps.h"

// Threshold config pulled from Firebase RTDB /stations/<id>/config node
struct StationConfig {
    float highThreshold;       // current A
    float lowThreshold;        // current A
    float highVoltageThreshold;// V
    float lowVoltageThreshold; // V
    float reportIntervalSec;   // seconds
};

// Upload one averaged reading to RTDB.
// Builds the JSON payload matching the frontend types.ts schema and
// PATCHes it to /stations/<station_id>/realtime
// Returns HTTP status code, or -1 on modem failure.
int  rtdb_upload(const String&        station_id,
                 const String&        id_token,
                 const PzemReading&   pzem,
                 const BatteryReading& batt,
                 const GpsReading&    gps);

// Fetch station config from RTDB /stations/<station_id>/config
// Merges remote values into the provided cfg struct.
// Returns true on success.
bool rtdb_fetch_config(const String&  station_id,
                       const String&  id_token,
                       StationConfig& cfg);

// Helper: build ISO-8601 UTC timestamp from millis() + compile-time epoch
String rtdb_timestamp_now();
