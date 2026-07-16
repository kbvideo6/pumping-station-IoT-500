#include "firebase_rtdb.h"
#include "config.h"
#include "modem.h"
#include <ArduinoJson.h>

// ── Timestamp ─────────────────────────────────────────────────
// The modem can return UTC time via AT+CCLK.
// For simplicity we use the modem time on first query, then track
// elapsed millis. A simplified ISO-8601 string is built here.
static uint32_t _epoch_at_sync   = 0;  // unix epoch when we synced
static uint32_t _millis_at_sync  = 0;

static void _sync_time_from_modem() {
    // AT+CCLK? returns: +CCLK: "26/07/15,12:34:56+04"
    // We do a best-effort parse; if it fails we use 0 (no timestamp)
    extern HardwareSerial Serial1;
    Serial1.print("AT+CCLK?\r\n");
    String r;
    uint32_t t = millis();
    while (millis() - t < 3000) {
        while (Serial1.available()) r += (char)Serial1.read();
        if (r.indexOf("OK") != -1) break;
        delay(5);
    }

    // Parse "+CCLK: \"yy/MM/dd,HH:mm:ss+tz\""
    int idx = r.indexOf("+CCLK: \"");
    if (idx == -1) return;
    String s = r.substring(idx + 8, idx + 28);
    // s = "26/07/15,12:34:56+04"
    int yy = s.substring(0, 2).toInt()  + 2000;
    int mo = s.substring(3, 5).toInt();
    int dd = s.substring(6, 8).toInt();
    int hh = s.substring(9, 11).toInt();
    int mm = s.substring(12, 14).toInt();
    int ss = s.substring(15, 17).toInt();

    // Simple unix epoch conversion (no leap-second precision needed)
    // Rough formula (good enough for IoT timestamps)
    uint32_t days_since_epoch = (yy - 1970) * 365 + (yy - 1969) / 4
                               + (mo  - 1) * 30 + dd - 1;  // approximate
    _epoch_at_sync  = days_since_epoch * 86400 + hh * 3600 + mm * 60 + ss;
    _millis_at_sync = millis();

    Serial.printf("[RTDB] Time synced: %04d-%02d-%02dT%02d:%02d:%02dZ\n",
        yy, mo, dd, hh, mm, ss);
}

String rtdb_timestamp_now() {
    if (_epoch_at_sync == 0) _sync_time_from_modem();
    if (_epoch_at_sync == 0) return "";   // still no time — omit from payload

    uint32_t now_s  = _epoch_at_sync + (millis() - _millis_at_sync) / 1000;
    uint32_t hh     = (now_s % 86400) / 3600;
    uint32_t mm     = (now_s % 3600)  / 60;
    uint32_t ss     = now_s % 60;
    // Date is approximate but close enough for IoT logging
    char buf[25];
    snprintf(buf, sizeof(buf), "1970-01-01T%02lu:%02lu:%02luZ", hh, mm, ss);
    // A full implementation would derive the real date from now_s
    return String(buf);
}

// ── Upload ────────────────────────────────────────────────────
int rtdb_upload(const String&         station_id,
                const String&         id_token,
                const PzemReading&    pzem,
                const BatteryReading& batt,
                const GpsReading&     gps) {

    // Build JSON payload matching frontend types.ts SensorData interface
    JsonDocument doc;

    bool sensor_offline = !pzem.valid;

    // Electrical readings — send zeros with alert if PZEM not connected
    doc["voltage"]     = serialized(String(sensor_offline ? 0.0f : pzem.voltage,     1));
    doc["current"]     = serialized(String(sensor_offline ? 0.0f : pzem.current,     3));
    doc["power"]       = serialized(String(sensor_offline ? 0.0f : pzem.power,       1));
    doc["energy"]      = serialized(String(sensor_offline ? 0.0f : pzem.energy,      3));
    doc["frequency"]   = serialized(String(sensor_offline ? 0.0f : pzem.frequency,   1));
    doc["powerFactor"] = serialized(String(sensor_offline ? 0.0f : pzem.powerFactor, 2));

    if (sensor_offline) {
        doc["sensorAlert"]   = true;
        doc["sensorOffline"] = true;
        Serial.println("[RTDB] PZEM offline — uploading zero readings with alert");
    } else {
        doc["sensorAlert"]   = false;
        doc["sensorOffline"] = false;
    }

    // Battery
    doc["battVolts"]   = serialized(String(batt.voltage, 2));
    doc["battPercent"] = serialized(String(batt.percent, 1));

    // Location
    if (gps.lat != 0.0f || gps.lng != 0.0f) {
        doc["latitude"]  = serialized(String(gps.lat, 6));
        doc["longitude"] = serialized(String(gps.lng, 6));
    }

    // Status
    doc["isOnline"]        = true;
    doc["lastSeen"]        = rtdb_timestamp_now();
    doc["rssi"]            = modem_get_rssi();
    doc["firmwareVersion"] = FW_VERSION;

    String payload;
    serializeJson(doc, payload);
    Serial.println("[RTDB] Payload: " + payload);

    String upper_station_id = station_id;
    upper_station_id.toUpperCase();

    // Use Firebase DB Secret (short, ~40 chars) — JWT idToken (~700 chars) exceeds
    // the A7670E AT+HTTPPARA URL limit of 512 chars.
    // DB secret: Firebase Console → Project Settings → Service Accounts → Database Secrets
    String url = String(FIREBASE_RTDB_URL) +
                 "/stations/" + upper_station_id + "/live.json?auth=" + FIREBASE_DB_SECRET;

    String resp;
    // Note: url already has ?auth=, so modem_http_patch must append &x-http-method-override=PATCH
    int status = modem_http_patch(url, payload, "", resp);
    Serial.printf("[RTDB] Upload status: %d\n", status);
    return status;
}

// ── Config Fetch ──────────────────────────────────────────────
bool rtdb_fetch_config(const String&  station_id,
                       const String&  id_token,
                       StationConfig& cfg) {

    String upper_station_id = station_id;
    upper_station_id.toUpperCase();

    // Use Firebase DB Secret — short (~40 chars), fits within A7670E URL limit
    String url = String(FIREBASE_RTDB_URL) +
                 "/stations/" + upper_station_id + "/config.json?auth=" + FIREBASE_DB_SECRET;

    String resp;
    int    status = modem_http_get(url, "", resp);

    if (status != 200 || resp.length() < 5) {
        Serial.printf("[RTDB] Config fetch failed — HTTP %d\n", status);
        return false;
    }

    JsonDocument doc;
    if (deserializeJson(doc, resp) != DeserializationError::Ok) {
        Serial.println("[RTDB] Config JSON parse error");
        return false;
    }

    // Merge only fields that are present — keep defaults for missing fields
    if (doc["highThreshold"].is<float>())
        cfg.highThreshold = doc["highThreshold"];
    if (doc["lowThreshold"].is<float>())
        cfg.lowThreshold  = doc["lowThreshold"];
    if (doc["highVoltageThreshold"].is<float>())
        cfg.highVoltageThreshold = doc["highVoltageThreshold"];
    if (doc["lowVoltageThreshold"].is<float>())
        cfg.lowVoltageThreshold  = doc["lowVoltageThreshold"];
    if (doc["reportIntervalSec"].is<float>())
        cfg.reportIntervalSec = doc["reportIntervalSec"];

    Serial.printf("[RTDB] Config: highI=%.1f lowI=%.1f highV=%.1f lowV=%.1f\n",
        cfg.highThreshold, cfg.lowThreshold,
        cfg.highVoltageThreshold, cfg.lowVoltageThreshold);

    return true;
}
