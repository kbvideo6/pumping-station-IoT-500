#include "alerts.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char* TAG = "EdgeAlert";

EdgeAlert::EdgeAlert(int debounceCount, uint64_t cooldownMs) {
  _debounceCount = debounceCount;
  _cooldownMs = cooldownMs;
  _currentAlert = ALERT_NONE;
  _consecutiveBreaches = 0;
  _lastAlertTimeMs = 0;
  _candidateAlert = ALERT_NONE;
}

CurrentAlertType EdgeAlert::processReading(float currentVal, float highThreshold, float lowThreshold) {
  CurrentAlertType detected = ALERT_NONE;

  if (currentVal > highThreshold) {
    detected = ALERT_HIGH;
  } else if (currentVal > 0.15f && currentVal < lowThreshold) {
    detected = ALERT_LOW;
  } else if (currentVal <= 0.15f) {
    detected = ALERT_NONE;
  }

  uint64_t nowMs = esp_timer_get_time() / 1000;

  if (detected != ALERT_NONE) {
    if (detected == _candidateAlert) {
      _consecutiveBreaches++;
    } else {
      _candidateAlert = detected;
      _consecutiveBreaches = 1;
    }

    if (_consecutiveBreaches >= _debounceCount) {
      if (_currentAlert != detected && (nowMs - _lastAlertTimeMs > _cooldownMs || _lastAlertTimeMs == 0)) {
        _currentAlert = detected;
        _lastAlertTimeMs = nowMs;
        return _currentAlert;
      }
    }
  } else {
    if (_currentAlert != ALERT_NONE) {
      ESP_LOGI(TAG, "Current returned to normal operating range.");
    }
    _currentAlert = ALERT_NONE;
    _candidateAlert = ALERT_NONE;
    _consecutiveBreaches = 0;
  }

  return _currentAlert;
}

void EdgeAlert::clearAlert() {
  _currentAlert = ALERT_NONE;
  _candidateAlert = ALERT_NONE;
  _consecutiveBreaches = 0;
}

bool EdgeAlert::isAlertActive() {
  return _currentAlert != ALERT_NONE;
}

const char* EdgeAlert::getAlertTypeString() {
  switch (_currentAlert) {
    case ALERT_HIGH:        return "HIGH_CURRENT";
    case ALERT_LOW:         return "LOW_CURRENT";
    case ALERT_NO_CURRENT:  return "NO_CURRENT";
    case ALERT_NONE:
    default:                return "NONE";
  }
}
