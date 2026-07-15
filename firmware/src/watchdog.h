#pragma once
#include <stdint.h>

// Hardware watchdog wrapper around the ESP-IDF task WDT.
// wdt_init()  — call once in setup()
// wdt_reset() — call at the end of every loop() iteration
// wdt_disable() — call before intentional esp_restart()

void wdt_init(uint32_t timeout_s);
void wdt_reset();
void wdt_disable();
