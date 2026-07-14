#ifndef ALERTS_H
#define ALERTS_H

#include <stdint.h>

enum CurrentAlertType {
  ALERT_NONE,
  ALERT_HIGH,
  ALERT_LOW,
  ALERT_NO_CURRENT
};

class EdgeAlert {
public:
  EdgeAlert(int debounceCount, uint64_t cooldownMs);
  CurrentAlertType processReading(float currentVal, float highThreshold, float lowThreshold);
  void clearAlert();
  bool isAlertActive();
  const char* getAlertTypeString();

private:
  int _debounceCount;
  uint64_t _cooldownMs;
  CurrentAlertType _currentAlert;
  int _consecutiveBreaches;
  uint64_t _lastAlertTimeMs;
  CurrentAlertType _candidateAlert;
};

#endif // ALERTS_H
