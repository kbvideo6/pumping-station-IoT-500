#ifndef LED_H
#define LED_H

#include <Arduino.h>

enum LedPattern {
  LED_PATTERN_SOLID,         // Booting/Initializing
  LED_PATTERN_SLOW_BLINK,    // Connecting to network (1s on, 1s off)
  LED_PATTERN_FAST_BLINK,    // Authenticating (200ms on, 200ms off)
  LED_PATTERN_HEARTBEAT,     // Running normally (2 quick blinks, pause)
  LED_PATTERN_SOS,           // Error / Recovery mode (S.O.S. pattern)
  LED_PATTERN_OFF            // Disabled
};

class StatusLED {
public:
  StatusLED(uint8_t pin);
  void begin();
  void setPattern(LedPattern pattern);
  void update(); // Call frequently in main loop to handle timing asynchronously

private:
  uint8_t _pin;
  LedPattern _currentPattern;
  unsigned long _lastToggleTime;
  bool _ledState;
  uint16_t _stepIndex;
};

#endif // LED_H
