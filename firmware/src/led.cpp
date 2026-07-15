#include "led.h"
#include "config.h"
#include <Adafruit_NeoPixel.h>
#include "Arduino.h"

static Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
static LedState _state      = LedState::OFF;
static uint32_t _phase_ms   = 0;
static bool     _phase_high = false;

// ── Colour helpers ────────────────────────────────────────────
static inline uint32_t dim(uint32_t c, uint8_t scale) {
    // scale: 0=off, 255=full
    uint8_t r = ((c >> 16) & 0xFF) * scale / 255;
    uint8_t g = ((c >>  8) & 0xFF) * scale / 255;
    uint8_t b = ((c      ) & 0xFF) * scale / 255;
    return strip.Color(r, g, b);
}
static uint32_t C_WHITE   = 0xFFFFFF;
static uint32_t C_BLUE    = 0x0066FF;
static uint32_t C_GREEN   = 0x00CC44;
static uint32_t C_CYAN    = 0x00FFCC;
static uint32_t C_YELLOW  = 0xFFCC00;
static uint32_t C_ORANGE  = 0xFF6600;
static uint32_t C_RED     = 0xFF0000;
static uint32_t C_PURPLE  = 0x8800FF;

void led_init() {
    strip.begin();
    strip.setBrightness(80);   // 0-255 — comfortable for indoor viewing
    strip.clear();
    strip.show();
    Serial.printf("[LED] Init on GPIO %d\n", LED_PIN);
}

void led_set(LedState state) {
    if (_state != state) {
        _state    = state;
        _phase_ms = millis();
        _phase_high = false;
    }
}

LedState led_get() { return _state; }

// ── Tick — called every loop() iteration ─────────────────────
// All timings relative to _phase_ms so no delay() needed.
void led_tick() {
    uint32_t now     = millis();
    uint32_t elapsed = now - _phase_ms;
    uint32_t colour  = 0;

    switch (_state) {

        case LedState::OFF:
            strip.clear();
            strip.show();
            return;

        // Slow white pulse 1 Hz (sine-ish approximation via linear sweep)
        case LedState::BOOTING: {
            uint32_t period = 1000;
            uint8_t  bright = (elapsed % period < period / 2)
                ? map(elapsed % period, 0, period / 2, 20, 200)
                : map(elapsed % period, period / 2, period, 200, 20);
            colour = dim(C_WHITE, bright);
            break;
        }

        // Fast blue blink 2 Hz (250ms on / 250ms off)
        case LedState::CONNECTING: {
            bool on = (elapsed % 500) < 250;
            colour  = on ? C_BLUE : 0;
            break;
        }

        // Fast blue blink 4 Hz (125ms on / 125ms off)
        case LedState::AUTH: {
            bool on = (elapsed % 250) < 125;
            colour  = on ? C_BLUE : 0;
            break;
        }

        // Green heartbeat: 100ms on, 900ms off
        case LedState::ONLINE: {
            bool on = (elapsed % 1000) < 100;
            colour  = on ? C_GREEN : 0;
            break;
        }

        // Brief cyan flash 150ms on, then back to current (main.cpp handles)
        case LedState::UPLOADING: {
            bool on = (elapsed % 600) < 150;
            colour  = on ? C_CYAN : 0;
            break;
        }

        // Yellow slow pulse 0.5 Hz
        case LedState::GPS_SEARCHING: {
            uint32_t period = 2000;
            uint8_t  bright = (elapsed % period < period / 2)
                ? map(elapsed % period, 0, period / 2, 20, 180)
                : map(elapsed % period, period / 2, period, 180, 20);
            colour = dim(C_YELLOW, bright);
            break;
        }

        // Orange fast blink 4 Hz
        case LedState::WARN_THRESHOLD: {
            bool on = (elapsed % 250) < 125;
            colour  = on ? C_ORANGE : 0;
            break;
        }

        // Red slow blink 1 Hz
        case LedState::ERROR_SOFT: {
            bool on = (elapsed % 1000) < 500;
            colour  = on ? C_RED : 0;
            break;
        }

        // Purple dim constant (very low brightness)
        case LedState::ERROR_DEGRADED: {
            uint32_t period = 3000;
            uint8_t  bright = (elapsed % period < period / 2)
                ? map(elapsed % period, 0, period / 2, 5, 60)
                : map(elapsed % period, period / 2, period, 60, 5);
            colour = dim(C_PURPLE, bright);
            break;
        }

        // White strobe 5× then off (200ms on / 200ms off, 10 cycles)
        case LedState::ALERT_FLASH: {
            if (elapsed > 2000) {
                _state = LedState::ONLINE;  // restore after strobe
                return;
            }
            bool on = (elapsed % 400) < 200;
            colour  = on ? C_WHITE : 0;
            break;
        }

        default:
            return;
    }

    strip.setPixelColor(0, colour);
    strip.show();
}
