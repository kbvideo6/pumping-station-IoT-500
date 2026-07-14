#include "battery_gauge.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

namespace {
constexpr uint8_t kCellVoltageReg = 0x02;
constexpr uint8_t kSocReg = 0x04;
constexpr uint8_t kReadRetries = 3;
constexpr uint8_t kReadDelayMs = 10;
constexpr float kCellVoltageScale = 0.000078125f;
} // namespace

Max17048BatteryGauge::Max17048BatteryGauge(uint8_t address) {
  _i2cPort = I2C_NUM_0;
  _address = address;
}

bool Max17048BatteryGauge::begin(i2c_port_t i2cPort) {
  _i2cPort = i2cPort;
  uint16_t value = 0;
  return readRegister16(kCellVoltageReg, value);
}

bool Max17048BatteryGauge::read(float& volts, float& percent) {
  uint16_t cellRaw = 0;
  uint16_t socRaw = 0;

  if (!readRegister16(kCellVoltageReg, cellRaw)) {
    return false;
  }

  if (!readRegister16(kSocReg, socRaw)) {
    return false;
  }

  volts = static_cast<float>(cellRaw) * kCellVoltageScale;
  percent = static_cast<float>((socRaw >> 8) & 0xFF);
  percent += static_cast<float>(socRaw & 0xFF) / 256.0f;
  return true;
}

bool Max17048BatteryGauge::readRegister16(uint8_t reg, uint16_t& value) {
  uint8_t data[2] = {0};

  for (uint8_t attempt = 0; attempt < kReadRetries; ++attempt) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_address << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &data[0], I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &data[1], I2C_MASTER_NACK);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(_i2cPort, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (err == ESP_OK) {
      value = (static_cast<uint16_t>(data[0]) << 8) | data[1];
      return true;
    }

    vTaskDelay(pdMS_TO_TICKS(kReadDelayMs));
  }

  return false;
}
