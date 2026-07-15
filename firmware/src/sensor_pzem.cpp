#include "sensor_pzem.h"
#include "config.h"
#include <PZEM004Tv30.h>
#include "Arduino.h"

// PZEM on UART2
static PZEM004Tv30 pzem(Serial2, PZEM_RX_PIN, PZEM_TX_PIN);

// Sample accumulator
static float   _sum_v = 0, _sum_i = 0, _sum_p = 0;
static float   _sum_e = 0, _sum_f = 0, _sum_pf = 0;
static uint8_t _sample_count = 0;
static bool    _last_valid   = false;

// Sampling timer
static uint32_t _last_sample_ms = 0;

void pzem_init() {
    Serial2.begin(9600, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);
    delay(100);
    Serial.println("[PZEM] Initialised on UART2 "
        "(TX=" + String(PZEM_TX_PIN) + " RX=" + String(PZEM_RX_PIN) + ")");
}

void pzem_tick() {
    uint32_t now = millis();
    if (now - _last_sample_ms < PZEM_SAMPLE_INTERVAL_MS) return;
    _last_sample_ms = now;

    float v  = pzem.voltage();
    float i  = pzem.current();
    float p  = pzem.power();
    float e  = pzem.energy();
    float f  = pzem.frequency();
    float pf = pzem.pf();

    // NaN returned by library when read fails
    if (isnan(v) || isnan(i) || isnan(p)) {
        Serial.println("[PZEM] Read failed (NaN) — skipping sample");
        _last_valid = false;
        return;
    }

    _sum_v  += v;
    _sum_i  += i;
    _sum_p  += p;
    _sum_e   = e;      // Energy is cumulative — take the last value directly
    _sum_f  += f;
    _sum_pf += pf;
    _sample_count++;
    _last_valid = true;

    Serial.printf("[PZEM] Sample #%u  V=%.1f  I=%.3f  P=%.1f  E=%.3f  F=%.1f  PF=%.2f\n",
        _sample_count, v, i, p, e, f, pf);
}

PzemReading pzem_get_average() {
    PzemReading r = {};

    if (_sample_count == 0) {
        r.valid = false;
        Serial.println("[PZEM] No samples — returning invalid reading");
        return r;
    }

    r.voltage     = _sum_v  / _sample_count;
    r.current     = _sum_i  / _sample_count;
    r.power       = _sum_p  / _sample_count;
    r.energy      = _sum_e;            // cumulative, not averaged
    r.frequency   = _sum_f  / _sample_count;
    r.powerFactor = _sum_pf / _sample_count;
    r.valid       = true;

    Serial.printf("[PZEM] Average over %u samples  V=%.1f  I=%.3f  P=%.1f\n",
        _sample_count, r.voltage, r.current, r.power);

    // Reset accumulator
    _sum_v = _sum_i = _sum_p = _sum_f = _sum_pf = 0;
    _sample_count = 0;

    return r;
}

void pzem_reset_energy() {
    pzem.resetEnergy();
    Serial.println("[PZEM] Energy counter reset");
}

bool pzem_is_ok() { return _last_valid; }
