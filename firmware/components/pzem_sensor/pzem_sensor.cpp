#include "pzem_sensor.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char* TAG = "PZEMSensor";

PZEMSensor::PZEMSensor(uart_port_t uartPort, int rxPin, int txPin, uint8_t addr) {
  _uartPort = uartPort;
  _rxPin = rxPin;
  _txPin = txPin;
  _addr = addr;
}

void PZEMSensor::begin() {
  uart_config_t uart_config;
  memset(&uart_config, 0, sizeof(uart_config));
  uart_config.baud_rate = 9600;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_1;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_config.rx_flow_ctrl_thresh = 0;
  uart_config.source_clk = UART_SCLK_DEFAULT;

  ESP_ERROR_CHECK(uart_driver_install(_uartPort, 256 * 2, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(_uartPort, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(_uartPort, _txPin, _rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  
  ESP_LOGI(TAG, "PZEM UART%d initialized (TX:%d RX:%d)", _uartPort, _txPin, _rxPin);
}

uint16_t PZEMSensor::calculateCRC(const uint8_t* data, uint16_t len) {
  uint16_t crc = 0xFFFF;
  for (uint16_t pos = 0; pos < len; pos++) {
    crc ^= (uint16_t)data[pos];
    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

bool PZEMSensor::read(PZEMReading& out) {
  uint8_t req[8];
  req[0] = _addr;
  req[1] = 0x04;
  req[2] = 0x00;
  req[3] = 0x00;
  req[4] = 0x00;
  req[5] = 0x0A;
  
  uint16_t crc = calculateCRC(req, 6);
  req[6] = crc & 0xFF;
  req[7] = (crc >> 8) & 0xFF;

  uart_flush(_uartPort);
  uart_write_bytes(_uartPort, (const char*)req, 8);

  uint8_t resp[25];
  int len = uart_read_bytes(_uartPort, resp, 25, pdMS_TO_TICKS(1000));

  if (len != 25) {
    ESP_LOGW(TAG, "Read failed: expected 25 bytes, got %d", len);
    return false;
  }

  uint16_t expectedCrc = (resp[24] << 8) | resp[23];
  uint16_t actualCrc = calculateCRC(resp, 23);
  if (expectedCrc != actualCrc) {
    ESP_LOGW(TAG, "CRC verification failed (expected 0x%04X, got 0x%04X)", expectedCrc, actualCrc);
    return false;
  }

  if (resp[0] != _addr || resp[1] != 0x04 || resp[2] != 0x14) {
    ESP_LOGW(TAG, "Invalid response header");
    return false;
  }

  uint16_t rawVoltage = (resp[3] << 8) | resp[4];
  
  uint16_t rawCurrentL = (resp[5] << 8) | resp[6];
  uint16_t rawCurrentH = (resp[7] << 8) | resp[8];
  uint32_t rawCurrent = ((uint32_t)rawCurrentH << 16) | rawCurrentL;

  uint16_t rawPowerL = (resp[9] << 8) | resp[10];
  uint16_t rawPowerH = (resp[11] << 8) | resp[12];
  uint32_t rawPower = ((uint32_t)rawPowerH << 16) | rawPowerL;

  uint16_t rawEnergyL = (resp[13] << 8) | resp[14];
  uint16_t rawEnergyH = (resp[15] << 8) | resp[16];
  uint32_t rawEnergy = ((uint32_t)rawEnergyH << 16) | rawEnergyL;

  uint16_t rawFrequency = (resp[17] << 8) | resp[18];
  uint16_t rawPf = (resp[19] << 8) | resp[20];

  out.voltage = rawVoltage * 0.1f;
  out.current = rawCurrent * 0.001f;
  out.power = rawPower * 0.1f;
  out.energy = rawEnergy * 1.0f;
  out.frequency = rawFrequency * 0.1f;
  out.powerFactor = rawPf * 0.01f;

  return true;
}

bool PZEMSensor::resetEnergy() {
  uint8_t req[4];
  req[0] = _addr;
  req[1] = 0x42;
  uint16_t crc = calculateCRC(req, 2);
  req[2] = crc & 0xFF;
  req[3] = (crc >> 8) & 0xFF;

  uart_flush(_uartPort);
  uart_write_bytes(_uartPort, (const char*)req, 4);

  uint8_t resp[4];
  int len = uart_read_bytes(_uartPort, resp, 4, pdMS_TO_TICKS(1000));

  if (len != 4) {
    ESP_LOGW(TAG, "Reset energy failed: no response");
    return false;
  }

  uint16_t expectedCrc = (resp[3] << 8) | resp[2];
  uint16_t actualCrc = calculateCRC(resp, 2);
  if (expectedCrc != actualCrc) {
    ESP_LOGW(TAG, "Reset energy CRC verification failed");
    return false;
  }

  if (resp[0] != _addr || resp[1] != 0x42) {
    ESP_LOGW(TAG, "Invalid reset energy response");
    return false;
  }

  ESP_LOGI(TAG, "Energy accumulator reset successfully");
  return true;
}
