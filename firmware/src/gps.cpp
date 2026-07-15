#include "gps.h"
#include "config.h"
#include "nvs_storage.h"
#include "Arduino.h"

static HardwareSerial& _ser = Serial1;  // Shared UART with modem

// State
static bool     _has_fix        = false;
static float    _lat            = 0.0f;
static float    _lng            = 0.0f;
static uint32_t _last_poll_ms   = 0;
static uint32_t _search_start   = 0;
static bool     _search_active  = false;

// Forward: reuse modem serial AT helper (local, no dep on modem.cpp internals)
static String _gps_at(const String& cmd, uint32_t timeout_ms = 3000) {
    _ser.print(cmd + "\r\n");
    String buf;
    uint32_t t = millis();
    while (millis() - t < timeout_ms) {
        while (_ser.available()) buf += (char)_ser.read();
        if (buf.indexOf("OK") != -1 || buf.indexOf("ERROR") != -1) break;
        delay(5);
    }
    return buf;
}

void gps_init() {
    // Load last known fix from NVS as fallback
    _lat = nvs_get_last_lat();
    _lng = nvs_get_last_lng();

    // Power on GNSS (co-processor inside A7670E)
    _gps_at("AT+CGNSPWR=1", 3000);
    _search_start  = millis();
    _search_active = true;
    Serial.printf("[GPS] GNSS powered on. Fallback position: %.6f, %.6f\n", _lat, _lng);
}

void gps_tick() {
    if (!_search_active && _has_fix) return;   // Fix obtained — nothing to do

    uint32_t now = millis();

    // Give up searching after GPS_MAX_WAIT_MS
    if (_search_active && (now - _search_start > GPS_MAX_WAIT_MS)) {
        _search_active = false;
        Serial.println("[GPS] No fix within timeout — using cached position");
        return;
    }

    // Poll on interval
    if (now - _last_poll_ms < GPS_POLL_INTERVAL_MS) return;
    _last_poll_ms = now;

    // AT+CGNSINF format:
    // +CGNSINF: <GNSS run status>,<fix status>,<UTC datetime>,<lat>,<lon>,...
    String r = _gps_at("AT+CGNSINF", 3000);
    if (r.indexOf("+CGNSINF:") == -1) {
        Serial.println("[GPS] No CGNSINF response");
        return;
    }

    // Parse the CSV fields
    int idx = r.indexOf("+CGNSINF:") + 9;
    String fields = r.substring(idx);
    fields.trim();

    // Field 0: GNSS run status (1=running)
    // Field 1: fix status (1=fix)
    // Field 3: lat, Field 4: lon
    int f = 0;
    int prev = 0;
    String fval[10];
    for (int i = 0; i <= fields.length() && f < 10; i++) {
        if (i == (int)fields.length() || fields[i] == ',') {
            fval[f++] = fields.substring(prev, i);
            prev = i + 1;
        }
    }

    // Field 1 == "1" means valid fix
    if (fval[1] != "1") {
        Serial.printf("[GPS] No fix yet (status=%s)\n", fval[1].c_str());
        return;
    }

    float lat = fval[3].toFloat();
    float lng = fval[4].toFloat();

    if (lat != 0.0f && lng != 0.0f) {
        _lat = lat;
        _lng = lng;
        _has_fix       = true;
        _search_active = false;
        nvs_set_last_gps(lat, lng);
        Serial.printf("[GPS] Fix! lat=%.6f lng=%.6f\n", lat, lng);
    }
}

GpsReading gps_get() {
    return { _lat, _lng, _has_fix };
}

bool gps_has_fix() { return _has_fix; }
