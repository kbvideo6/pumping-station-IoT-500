#pragma once
#include <stdint.h>

// WS2812B LED state machine (non-blocking).
// All patterns are driven by led_tick() which must be called from loop().
// Patterns are priority-ordered — higher-priority states override lower ones.

enum class LedState : uint8_t {
    OFF             = 0,
    BOOTING,          // Slow white pulse — initialising
    CONNECTING,       // Fast blue blink 2Hz — modem/network connecting
    AUTH,             // Fast blue blink 4Hz — Firebase auth in progress
    ONLINE,           // Green heartbeat 1Hz — fully operational
    UPLOADING,        // Brief cyan flash — data upload in progress
    GPS_SEARCHING,    // Yellow slow pulse — waiting for GNSS fix
    WARN_THRESHOLD,   // Orange fast blink — reading crossed threshold
    ERROR_SOFT,       // Red slow blink — AT+CRESET attempted
    ERROR_DEGRADED,   // Purple dim pulse — hard-locked modem, degraded mode
    ALERT_FLASH,      // White strobe 5× — critical alert
};

void led_init();
void led_set(LedState state);
void led_tick();       // Call every loop() — non-blocking
LedState led_get();
