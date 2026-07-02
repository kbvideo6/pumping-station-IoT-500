#include "alerts.h"

EdgeAlert::EdgeAlert(int debounceCount, unsigned long cooldownMs) {
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
  } else if (currentVal > 0.15 && currentVal < lowThreshold) {
    detected = ALERT_LOW;
  } else if (currentVal <= 0.15) {
    // Current is zero or noise floor; the pump is off (idle).
    // Do not trigger alarms since the pump naturally cycles on/off.
    detected = ALERT_NONE;
  }

  // Handle debouncing
  if (detected != ALERT_NONE) {
    if (detected == _candidateAlert) {
      _consecutiveBreaches++;
    } else {
      _candidateAlert = detected;
      _consecutiveBreaches = 1;
    }

    if (_consecutiveBreaches >= _debounceCount) {
      // Debounce limit reached. Apply cooldown to avoid email spamming.
      if (_currentAlert != detected && (millis() - _lastAlertTimeMs > _cooldownMs || _lastAlertTimeMs == 0)) {
        _currentAlert = detected;
        _lastAlertTimeMs = millis();
        return _currentAlert;
      }
    }
  } else {
    // Current returned to normal range
    if (_currentAlert != ALERT_NONE) {
      Serial.println("[Alerts] Current returned to normal operating range.");
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
