#include "led.h"

StatusLED::StatusLED(uint8_t pin) {
  _pin = pin;
  _currentPattern = LED_PATTERN_OFF;
  _lastToggleTime = 0;
  _ledState = false;
  _stepIndex = 0;
}

void StatusLED::begin() {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, LOW);
}

void StatusLED::setPattern(LedPattern pattern) {
  if (_currentPattern != pattern) {
    _currentPattern = pattern;
    _lastToggleTime = 0;
    _ledState = false;
    _stepIndex = 0;
    digitalWrite(_pin, LOW);
  }
}

void StatusLED::update() {
  unsigned long now = millis();
  
  switch (_currentPattern) {
    case LED_PATTERN_SOLID:
      if (!_ledState) {
        _ledState = true;
        digitalWrite(_pin, HIGH);
      }
      break;

    case LED_PATTERN_SLOW_BLINK:
      if (now - _lastToggleTime >= 1000) {
        _ledState = !_ledState;
        digitalWrite(_pin, _ledState ? HIGH : LOW);
        _lastToggleTime = now;
      }
      break;

    case LED_PATTERN_FAST_BLINK:
      if (now - _lastToggleTime >= 200) {
        _ledState = !_ledState;
        digitalWrite(_pin, _ledState ? HIGH : LOW);
        _lastToggleTime = now;
      }
      break;

    case LED_PATTERN_HEARTBEAT: {
      // Pattern steps (duration in ms):
      // 0: ON (100ms), 1: OFF (150ms), 2: ON (100ms), 3: OFF (1000ms)
      const uint16_t intervals[] = { 100, 150, 100, 1000 };
      if (now - _lastToggleTime >= intervals[_stepIndex]) {
        _ledState = (_stepIndex == 0 || _stepIndex == 2);
        digitalWrite(_pin, _ledState ? HIGH : LOW);
        _stepIndex = (_stepIndex + 1) % 4;
        _lastToggleTime = now;
      }
      break;
    }

    case LED_PATTERN_SOS: {
      // S.O.S. pattern in pulses (dot=150ms, dash=450ms, pause=150ms, letter-pause=600ms)
      // Dots: step 0-5 (on, off, on, off, on, off)
      // Dashes: step 6-11 (on, off, on, off, on, off)
      // Dots: step 12-17 (on, off, on, off, on, off)
      // Word pause: step 18
      const uint16_t intervals[] = {
        150, 150, 150, 150, 150, 450, // S (3 dots + space to letters)
        450, 150, 450, 150, 450, 450, // O (3 dashes + space to letters)
        150, 150, 150, 150, 150, 1000 // S (3 dots + long word space)
      };
      if (now - _lastToggleTime >= intervals[_stepIndex]) {
        _ledState = (_stepIndex % 2 == 0 && _stepIndex < 18);
        digitalWrite(_pin, _ledState ? HIGH : LOW);
        _stepIndex = (_stepIndex + 1) % 18;
        _lastToggleTime = now;
      }
      break;
    }

    case LED_PATTERN_OFF:
    default:
      if (_ledState) {
        _ledState = false;
        digitalWrite(_pin, LOW);
      }
      break;
  }
}
