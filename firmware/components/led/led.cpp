#include "led.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"

static const char* TAG = "StatusLED";

namespace {
constexpr uint8_t kBrightness = 32;

inline uint32_t get_ccount(void) {
    uint32_t ccount;
    __asm__ __volatile__("rsr %0, ccount" : "=r"(ccount));
    return ccount;
}

void IRAM_ATTR ws2812_send_pixel(gpio_num_t pin, uint8_t r, uint8_t g, uint8_t b) {
    r = (r * kBrightness) >> 8;
    g = (g * kBrightness) >> 8;
    b = (b * kBrightness) >> 8;

    uint32_t data = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);

    for (int i = 23; i >= 0; i--) {
        uint32_t start = get_ccount();
        gpio_set_level(pin, 1);
        if (data & (1 << i)) {
            while ((get_ccount() - start) < 216);
            gpio_set_level(pin, 0);
            while ((get_ccount() - start) < 288);
        } else {
            while ((get_ccount() - start) < 72);
            gpio_set_level(pin, 0);
            while ((get_ccount() - start) < 288);
        }
    }
    
    portEXIT_CRITICAL(&mux);
}

void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b) {
    if (s == 0) {
        r = v; g = v; b = v;
        return;
    }
    uint8_t region = h / 43;
    uint8_t remainder = (h - (region * 43)) * 6;
    uint8_t p = (v * (255 - s)) >> 8;
    uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:  r = v; g = t; b = p; break;
        case 1:  r = q; g = v; b = p; break;
        case 2:  r = p; g = v; b = t; break;
        case 3:  r = p; g = q; b = v; break;
        case 4:  r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
}
} // namespace

StatusLED::StatusLED(gpio_num_t pin) {
  _pin = pin;
  _currentPattern = LED_PATTERN_OFF;
  _lastToggleTimeMs = 0;
  _ledState = false;
  _stepIndex = 0;
}

void StatusLED::begin() {
  gpio_reset_pin(_pin);
  gpio_set_direction(_pin, GPIO_MODE_OUTPUT);
  gpio_set_level(_pin, 0);
  
  ws2812_send_pixel(_pin, 0, 0, 0);
  esp_rom_delay_us(300);
  
  _lastToggleTimeMs = 0;
  _ledState = false;
  _stepIndex = 0;
  ESP_LOGI(TAG, "Status LED initialized on GPIO %d", _pin);
}

void StatusLED::setPattern(LedPattern pattern) {
  if (_currentPattern != pattern) {
    _currentPattern = pattern;
    _lastToggleTimeMs = 0;
    _ledState = false;
    _stepIndex = 0;
    ws2812_send_pixel(_pin, 0, 0, 0);
    esp_rom_delay_us(300);
  }
}

void StatusLED::update() {
  uint64_t now = esp_timer_get_time() / 1000;

  switch (_currentPattern) {
    case LED_PATTERN_SOLID:
      ws2812_send_pixel(_pin, 0, 80, 255);
      _ledState = true;
      break;

    case LED_PATTERN_SLOW_BLINK:
      if (now - _lastToggleTimeMs >= 1000) {
        _ledState = !_ledState;
        if (_ledState) {
          ws2812_send_pixel(_pin, 0, 180, 160);
        } else {
          ws2812_send_pixel(_pin, 0, 0, 0);
        }
        _lastToggleTimeMs = now;
      }
      break;

    case LED_PATTERN_FAST_BLINK:
      if (now - _lastToggleTimeMs >= 200) {
        _ledState = !_ledState;
        if (_ledState) {
          ws2812_send_pixel(_pin, 255, 140, 0);
        } else {
          ws2812_send_pixel(_pin, 0, 0, 0);
        }
        _lastToggleTimeMs = now;
      }
      break;

    case LED_PATTERN_HEARTBEAT: {
      uint16_t hue = static_cast<uint16_t>((now / 12) % 255);
      uint8_t r, g, b;
      hsv_to_rgb(hue, 255, 200, r, g, b);
      ws2812_send_pixel(_pin, r, g, b);
      _ledState = true;
      break;
    }

    case LED_PATTERN_SOS: {
      const uint16_t intervals[] = { 150, 150, 150, 150, 150, 450, 450, 150, 450, 150, 450, 450, 150, 150, 150, 150, 150, 1000 };
      if (now - _lastToggleTimeMs >= intervals[_stepIndex]) {
        _ledState = (_stepIndex % 2 == 0 && _stepIndex < 18);
        if (_ledState) {
          ws2812_send_pixel(_pin, 255, 0, 0);
        } else {
          ws2812_send_pixel(_pin, 0, 0, 0);
        }
        _stepIndex = (_stepIndex + 1) % 18;
        _lastToggleTimeMs = now;
      }
      break;
    }

    case LED_PATTERN_OFF:
    default:
      if (_ledState) {
        _ledState = false;
        ws2812_send_pixel(_pin, 0, 0, 0);
      }
      break;
  }
}
