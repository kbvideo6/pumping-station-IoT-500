#pragma once
#include "Arduino.h"

// NVS namespace: "iot"
// Keys stored:
//   station_id    — string
//   device_token  — string (64-char hex provisioning secret, kept permanently)
//   id_token      — string (Firebase JWT, ~1hr TTL)
//   refresh_token — string (Firebase long-lived refresh token)
//   token_expiry  — uint32 (unix epoch when id_token expires)
//   last_lat      — float  (last confirmed GPS latitude)
//   last_lng      — float  (last confirmed GPS longitude)
//   boot_fail     — uint8  (consecutive boot failures)

void     nvs_load();                         // Must be called first in setup()

// Station identity
String   nvs_get_station_id();
void     nvs_set_station_id(const String& id);

// Auth tokens
String   nvs_get_device_token();
void     nvs_set_device_token(const String& t);

String   nvs_get_id_token();
void     nvs_set_id_token(const String& t);

String   nvs_get_refresh_token();
void     nvs_set_refresh_token(const String& t);

uint32_t nvs_get_token_expiry();
void     nvs_set_token_expiry(uint32_t epoch);

bool     nvs_has_refresh_token();            // True if refresh_token is non-empty

// GPS cache
float    nvs_get_last_lat();
float    nvs_get_last_lng();
void     nvs_set_last_gps(float lat, float lng);

// Boot failure counter
uint8_t  nvs_get_boot_fail();
void     nvs_set_boot_fail(uint8_t count);
void     nvs_increment_boot_fail();
