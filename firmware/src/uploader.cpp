#include "uploader.h"
#include "config.h"
#include "Arduino.h"

static QueueEntry _buf[QUEUE_MAX_ENTRIES];
static uint8_t    _head  = 0;   // next read position
static uint8_t    _tail  = 0;   // next write position
static uint8_t    _count = 0;

void queue_init() {
    _head = _tail = _count = 0;
    Serial.printf("[QUEUE] Initialised — capacity %u slots\n", QUEUE_MAX_ENTRIES);
}

bool queue_push(const QueueEntry& e) {
    if (_count >= QUEUE_MAX_ENTRIES) {
        Serial.println("[QUEUE] Full — oldest entry dropped");
        // Discard oldest to make room
        _head = (_head + 1) % QUEUE_MAX_ENTRIES;
        _count--;
    }
    _buf[_tail] = e;
    _tail = (_tail + 1) % QUEUE_MAX_ENTRIES;
    _count++;
    Serial.printf("[QUEUE] Pushed — %u/%u slots used\n", _count, QUEUE_MAX_ENTRIES);
    return true;
}

bool queue_pop(QueueEntry& out) {
    if (_count == 0) return false;
    out   = _buf[_head];
    _head = (_head + 1) % QUEUE_MAX_ENTRIES;
    _count--;
    Serial.printf("[QUEUE] Popped — %u/%u slots remaining\n", _count, QUEUE_MAX_ENTRIES);
    return true;
}

bool queue_is_empty() { return _count == 0; }
uint8_t queue_size()  { return _count; }
