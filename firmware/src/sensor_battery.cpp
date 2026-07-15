#include "sensor_battery.h"
#include "config.h"
#include <Adafruit_MAX1704X.h>
#include <Wire.h>

static Adafruit_MAX17048 _gauge;
static bool              _ok = false;

void battery_init() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    if (!_gauge.begin()) {
        Serial.println("[BATT] MAX17048 not found at 0x36 — check SDA/SCL wiring");
        _ok = false;
        return;
    }
    _ok = true;
    Serial.printf("[BATT] MAX17048 found — %.2f V  %.1f%%\n",
        _gauge.cellVoltage(), _gauge.cellPercent());
}

BatteryReading battery_read() {
    BatteryReading r = {};
    if (!_ok) { r.valid = false; return r; }

    r.voltage = _gauge.cellVoltage();
    r.percent = _gauge.cellPercent();
    r.valid   = true;

    // Clamp percent to 0–100
    if (r.percent < 0.0f)   r.percent = 0.0f;
    if (r.percent > 100.0f) r.percent = 100.0f;

    Serial.printf("[BATT] %.2f V  %.1f%%\n", r.voltage, r.percent);
    return r;
}

bool battery_is_ok() { return _ok; }
