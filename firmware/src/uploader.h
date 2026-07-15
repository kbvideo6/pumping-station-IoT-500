#pragma once
#include "sensor_pzem.h"
#include "sensor_battery.h"
#include "gps.h"

// Ring-buffer upload queue — holds up to QUEUE_MAX_ENTRIES packets.
// When the modem is unavailable, readings are queued locally.
// When the connection recovers, queued readings are uploaded in FIFO order.

struct QueueEntry {
    PzemReading   pzem;
    BatteryReading batt;
    GpsReading    gps;
    uint32_t      timestamp_ms;  // millis() when queued
};

void  queue_init();
bool  queue_push(const QueueEntry& e);   // Returns false if queue is full
bool  queue_pop (QueueEntry& out);        // Returns false if queue is empty
bool  queue_is_empty();
uint8_t queue_size();
