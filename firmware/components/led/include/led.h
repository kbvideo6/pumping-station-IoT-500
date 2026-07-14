#ifndef LED_H
#define LED_H

#include <stdint.h>
#include "driver/gpio.h"

enum LedPattern {
  LED_PATTERN_SOLID,         // Booting/Initializing (blue)
  LED_PATTERN_SLOW_BLINK,    // Connecting to network (cyan blink)
  LED_PATTERN_FAST_BLINK,    // Authenticating (amber blink)
  LED_PATTERN_HEARTBEAT,     // Running normally (smooth green/cyan gradient)
  LED_PATTERN_SOS,           // Error / Recovery mode (red pulse)
  LED_PATTERN_OFF            // Disabled
};

class StatusLED {
public:
  StatusLED(gpio_num_t pin);
  void begin();
  void setPattern(LedPattern pattern);
  void update(); // Call periodically to run led state updates

private:
  gpio_num_t _pin;
  LedPattern _currentPattern;
  uint64_t _lastToggleTimeMs;
  bool _ledState;
  uint16_t _stepIndex;
};

#endif // LED_H
