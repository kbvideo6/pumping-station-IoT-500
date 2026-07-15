#include "watchdog.h"
#include "esp_task_wdt.h"
#include "Arduino.h"

void wdt_init(uint32_t timeout_s) {
    esp_task_wdt_init(timeout_s, true);
    esp_task_wdt_add(NULL);            // watch the current (main) task
    Serial.printf("[WDT] Initialized — timeout %lu s\n", timeout_s);
}

void wdt_reset() {
    esp_task_wdt_reset();
}

void wdt_disable() {
    esp_task_wdt_delete(NULL);
}
