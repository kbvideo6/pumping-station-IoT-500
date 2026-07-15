#include "nvs_storage.h"
#include "config.h"
#include <Preferences.h>

static Preferences prefs;
static const char* NS = "iot";

void nvs_load() {
    prefs.begin(NS, false);
    Serial.println("[NVS] Loaded namespace: " + String(NS));
}

// ── Station ID ───────────────────────────────────────────────
String nvs_get_station_id() {
    return prefs.getString("station_id", DEFAULT_STATION_ID);
}
void nvs_set_station_id(const String& id) {
    prefs.putString("station_id", id);
}

// ── Device Token (provisioning secret, never expires) ────────
String nvs_get_device_token() {
    return prefs.getString("device_token", DEFAULT_DEVICE_TOKEN);
}
void nvs_set_device_token(const String& t) {
    prefs.putString("device_token", t);
}

// ── Firebase idToken ─────────────────────────────────────────
String nvs_get_id_token() {
    return prefs.getString("id_token", "");
}
void nvs_set_id_token(const String& t) {
    prefs.putString("id_token", t);
}

// ── Firebase refreshToken ────────────────────────────────────
String nvs_get_refresh_token() {
    return prefs.getString("refresh_token", "");
}
void nvs_set_refresh_token(const String& t) {
    prefs.putString("refresh_token", t);
}

bool nvs_has_refresh_token() {
    return nvs_get_refresh_token().length() > 0;
}

// ── Token Expiry (unix epoch) ────────────────────────────────
uint32_t nvs_get_token_expiry() {
    return prefs.getUInt("token_expiry", 0);
}
void nvs_set_token_expiry(uint32_t epoch) {
    prefs.putUInt("token_expiry", epoch);
}

// ── GPS Cache ────────────────────────────────────────────────
float nvs_get_last_lat() {
    return prefs.getFloat("last_lat", 0.0f);
}
float nvs_get_last_lng() {
    return prefs.getFloat("last_lng", 0.0f);
}
void nvs_set_last_gps(float lat, float lng) {
    prefs.putFloat("last_lat", lat);
    prefs.putFloat("last_lng", lng);
}

// ── Boot Failure Counter ─────────────────────────────────────
uint8_t nvs_get_boot_fail() {
    return prefs.getUChar("boot_fail", 0);
}
void nvs_set_boot_fail(uint8_t count) {
    prefs.putUChar("boot_fail", count);
}
void nvs_increment_boot_fail() {
    uint8_t c = nvs_get_boot_fail();
    nvs_set_boot_fail(c + 1);
    Serial.printf("[NVS] boot_fail = %u\n", c + 1);
}
