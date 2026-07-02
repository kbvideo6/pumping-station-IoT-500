#ifndef ALERTS_H
#define ALERTS_H

#include <Arduino.h>

enum CurrentAlertType {
  ALERT_NONE,
  ALERT_HIGH,
  ALERT_LOW,
  ALERT_NO_CURRENT
};

class EdgeAlert {
public:
  EdgeAlert(int debounceCount, unsigned long cooldownMs);
  
  CurrentAlertType processReading(float currentVal, float highThreshold, float lowThreshold);
  void clearAlert();
  
  bool isAlertActive();
  const char* getAlertTypeString();

private:
  int _debounceCount;
  unsigned long _cooldownMs;
  
  CurrentAlertType _currentAlert;
  int _consecutiveBreaches;
  unsigned long _lastAlertTimeMs;
  CurrentAlertType _candidateAlert;
};

#endif // ALERTS_H
