#include "sensor.h"
#include <math.h>
#include "esp_rom_sys.h"

namespace {
adc_channel_t get_adc1_channel(uint8_t gpio) {
  switch (gpio) {
    case 1:  return ADC_CHANNEL_0;
    case 2:  return ADC_CHANNEL_1;
    case 3:  return ADC_CHANNEL_2;
    case 4:  return ADC_CHANNEL_3;
    case 5:  return ADC_CHANNEL_4;
    case 6:  return ADC_CHANNEL_5;
    case 7:  return ADC_CHANNEL_6;
    case 8:  return ADC_CHANNEL_7;
    case 9:  return ADC_CHANNEL_8;
    case 10: return ADC_CHANNEL_9;
    default: return ADC_CHANNEL_0;
  }
}
} // namespace

CTSensor::CTSensor(uint8_t adcPin, float calibration, float vref, float resolution, float biasVoltage) {
  _adcPin = adcPin;
  _calibration = calibration;
  _vref = vref;
  _resolution = resolution;
  _biasVoltage = biasVoltage;
  _adc_handle = nullptr;
  _adc_channel = get_adc1_channel(adcPin);
}

void CTSensor::begin() {
  adc_oneshot_unit_init_cfg_t init_config = {
      .unit_id = ADC_UNIT_1,
      .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
      .ulp_mode = ADC_ULP_MODE_DISABLE,
  };
  adc_oneshot_new_unit(&init_config, &_adc_handle);

  adc_oneshot_chan_cfg_t config = {
      .atten = ADC_ATTEN_DB_12,
      .bitwidth = ADC_BITWIDTH_12,
  };
  adc_oneshot_config_channel(_adc_handle, _adc_channel, &config);
}

float CTSensor::readCurrentRMS(uint16_t sampleCount, uint16_t samplePeriodUs) {
  double sumSquares = 0;

  for (uint16_t i = 0; i < sampleCount; i++) {
    int raw = 0;
    if (_adc_handle) {
      adc_oneshot_read(_adc_handle, _adc_channel, &raw);
    }
    double voltage = (raw / _resolution) * _vref;
    double currentVal = (voltage - _biasVoltage) * _calibration;

    sumSquares += currentVal * currentVal;
    esp_rom_delay_us(samplePeriodUs);
  }

  double rms = sqrt(sumSquares / sampleCount);

  if (rms < 0.15) {
    rms = 0.0;
  }

  return (float)rms;
}
