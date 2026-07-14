#include "modem.h"
#include "config.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char* TAG = "A7670Modem";

A7670Modem::A7670Modem(int uartPort, int pwrPin, int rstPin, int flightPin) {
  _uartPort = (uart_port_t)uartPort;
  _pwrPin = pwrPin;
  _rstPin = rstPin;
  _flightPin = flightPin;
}

void A7670Modem::begin() {
  uart_config_t uart_config;
  memset(&uart_config, 0, sizeof(uart_config));
  uart_config.baud_rate = 115200;
  uart_config.data_bits = UART_DATA_8_BITS;
  uart_config.parity = UART_PARITY_DISABLE;
  uart_config.stop_bits = UART_STOP_BITS_1;
  uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
  uart_config.rx_flow_ctrl_thresh = 0;
  uart_config.source_clk = UART_SCLK_DEFAULT;
  
  // Safe UART deletion and installation to heal the serial port state on re-init
  uart_driver_delete(_uartPort);
  ESP_ERROR_CHECK(uart_driver_install(_uartPort, 1024 * 4, 1024 * 4, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(_uartPort, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(_uartPort, MODEM_UART_TX_PIN, MODEM_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  if (_pwrPin >= 0) {
    gpio_reset_pin((gpio_num_t)_pwrPin);
    gpio_set_direction((gpio_num_t)_pwrPin, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level((gpio_num_t)_pwrPin, 1);
  }
  if (_rstPin >= 0) {
    gpio_reset_pin((gpio_num_t)_rstPin);
    gpio_set_direction((gpio_num_t)_rstPin, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level((gpio_num_t)_rstPin, 1);
  }
  if (_flightPin >= 0) {
    gpio_reset_pin((gpio_num_t)_flightPin);
    gpio_set_direction((gpio_num_t)_flightPin, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)_flightPin, 0);
  }
  
  ESP_LOGI(TAG, "Modem UART initialized on port %d", _uartPort);
}

bool A7670Modem::powerOn() {
  ESP_LOGI(TAG, "Diagnosing A7670E modem power state...");
  std::string resp;

  if (ping()) {
    ESP_LOGI(TAG, "[Modem Diagnostic] Modem already powered on and responsive.");
    sendATCommand("ATE0", resp, 1000);
    return true;
  }

  if (_pwrPin >= 0) {
    ESP_LOGI(TAG, "[Modem Diagnostic] Modem offline. Pulsing PWRKEY (GPIO %d) LOW...", _pwrPin);
    gpio_set_level((gpio_num_t)_pwrPin, 0);
    vTaskDelay(pdMS_TO_TICKS(1200));
    gpio_set_level((gpio_num_t)_pwrPin, 1);

    ESP_LOGI(TAG, "[Modem Diagnostic] Waiting 15 seconds for boot...");
    for (int i = 0; i < 15; i++) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      if (ping()) {
        ESP_LOGI(TAG, "[Modem Diagnostic] Success! Modem responsive after PWRKEY pulse.");
        sendATCommand("ATE0", resp, 1000);
        return true;
      }
      ESP_LOGI(TAG, "Polling modem... (attempt %d/15)", i + 1);
    }
  }

  if (_rstPin >= 0) {
    ESP_LOGW(TAG, "[Modem Diagnostic] PWRKEY failed. Pulsing RESET (GPIO %d) LOW...", _rstPin);
    gpio_set_level((gpio_num_t)_rstPin, 0);
    vTaskDelay(pdMS_TO_TICKS(1500));
    gpio_set_level((gpio_num_t)_rstPin, 1);

    ESP_LOGI(TAG, "[Modem Diagnostic] Waiting 15 seconds for boot...");
    for (int i = 0; i < 15; i++) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      if (ping()) {
        ESP_LOGI(TAG, "[Modem Diagnostic] Success! Modem responsive after hard RESET.");
        sendATCommand("ATE0", resp, 1000);
        return true;
      }
      ESP_LOGI(TAG, "Polling modem... (attempt %d/15)", i + 1);
    }
  }

  ESP_LOGW(TAG, "[Modem Diagnostic] Hard RESET failed. Trying fallback RESET (GPIO 5) LOW...");
  gpio_reset_pin(GPIO_NUM_5);
  gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_5, 0);
  vTaskDelay(pdMS_TO_TICKS(1500));
  gpio_set_level(GPIO_NUM_5, 1);

  ESP_LOGI(TAG, "[Modem Diagnostic] Waiting 15 seconds for boot...");
  for (int i = 0; i < 15; i++) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    if (ping()) {
      ESP_LOGI(TAG, "[Modem Diagnostic] Success! Modem responsive after fallback RESET (GPIO 5).");
      sendATCommand("ATE0", resp, 1000);
      return true;
    }
    ESP_LOGI(TAG, "Polling modem... (attempt %d/15)", i + 1);
  }

  ESP_LOGE(TAG, "[Modem Diagnostic] FATAL ERROR: Modem is completely unresponsive!");
  ESP_LOGE(TAG, "[Modem Diagnostic] Please verify:");
  ESP_LOGE(TAG, "  1) Physical DIP switches: the '4G' switch MUST be in the ON position.");
  ESP_LOGE(TAG, "  2) Power Supply: Ensure you are using a stable 5V external power source (2A+). Low-power USB power can brown out the module.");
  ESP_LOGE(TAG, "  3) Hardware connection and SIM insertion.");
  return false;
}

void A7670Modem::powerOff() {
  ESP_LOGI(TAG, "Powering off cellular module...");
  std::string resp;
  sendATCommand("AT+CPOWD", resp, 2000);
}

void A7670Modem::hardReset() {
  ESP_LOGI(TAG, "Sending soft reboot command (AT+CRESET)...");
  std::string resp;
  sendATCommand("AT+CRESET", resp, 2000);
  vTaskDelay(pdMS_TO_TICKS(4000));
}

bool A7670Modem::ping() {
  std::string resp;
  return sendATCommand("AT", resp, 1000);
}

bool A7670Modem::softReset() {
  ESP_LOGI(TAG, "Too many failures. Resetting A7670E radio (AT+CFUN=1,1)...");
  std::string resp;
  sendATCommand("AT+CFUN=1,1", resp, 2000);
  vTaskDelay(pdMS_TO_TICKS(MODEM_SOFT_RESET_RECOVERY_WAIT_MS));
  clearSerialBuffer();
  return true;
}

bool A7670Modem::initializeModem() {
  std::string resp;
  if (!sendATCommand("AT", resp, 1000)) return false;
  sendATCommand("ATE0", resp, 1000);
  sendATCommand("AT+CMEE=2", resp, 1000);
  return checkSIMStatus();
}

bool A7670Modem::checkSIMStatus() {
  std::string resp;
  for (int i = 0; i < 5; i++) {
    if (sendATCommand("AT+CPIN?", resp, 1000)) {
      if (resp.find("READY") != std::string::npos) {
        ESP_LOGI(TAG, "SIM card detected and ready.");
        return true;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  ESP_LOGW(TAG, "SIM card not ready or missing.");
  return false;
}

int A7670Modem::getSignalQuality() {
  std::string resp;
  if (sendATCommand("AT+CSQ", resp, 1000)) {
    size_t index = resp.find("+CSQ:");
    if (index != std::string::npos) {
      size_t comma = resp.find(",", index);
      if (comma != std::string::npos) {
        std::string rssiStr = resp.substr(index + 6, comma - (index + 6));
        int rssi = atoi(rssiStr.c_str());
        if (rssi == 99) return -113;
        return -113 + (rssi * 2);
      }
    }
  }
  return -113;
}

bool A7670Modem::connectNetwork(const char* apn) {
  std::string resp;
  ESP_LOGI(TAG, "Connecting to cellular APN: %s...", apn);

  sendATCommand("AT+CNMP=38", resp, 1000);

  for (int i = 0; i < 15; i++) {
    if (sendATCommand("AT+CGREG?", resp, 1000)) {
      if (resp.find(",1") != std::string::npos || resp.find(",5") != std::string::npos) {
        ESP_LOGI(TAG, "Network registration completed successfully.");
        break;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  sendATCommand("AT+CGDCONT=1,\"IP\",\"" + std::string(apn) + "\"", resp, 1000);
  sendATCommand("AT+CGACT=1,1", resp, 3000);

  if (isNetworkConnected()) {
    ESP_LOGI(TAG, "Network connected successfully.");
    return true;
  }
  return false;
}

bool A7670Modem::isNetworkConnected() {
  std::string resp;
  if (sendATCommand("AT+CGACT?", resp, 1000)) {
    if (resp.find("1,1") != std::string::npos) {
      return true;
    }
  }
  return false;
}

bool A7670Modem::enableGPS(bool enable) {
  std::string resp;
  if (enable) {
    ESP_LOGI(TAG, "Powering on GPS...");
    return sendATCommand("AT+CGNSSPWR=1", resp, 2000);
  } else {
    ESP_LOGI(TAG, "Powering off GPS...");
    return sendATCommand("AT+CGNSSPWR=0", resp, 2000);
  }
}

bool A7670Modem::getGPS(double& latitude, double& longitude) {
  std::string resp;
  if (!sendATCommand("AT+CGNSSINFO", resp, 2000)) return false;

  size_t infoIdx = resp.find("+CGNSSINFO:");
  if (infoIdx == std::string::npos) return false;

  std::string data = resp.substr(infoIdx + 11);
  
  std::string parts[12];
  int partCount = 0;
  size_t prevComma = 0;

  for (size_t i = 0; i < data.length(); i++) {
    if (data[i] == ',' || i == data.length() - 1) {
      size_t endIdx = (data[i] == ',') ? i : i + 1;
      parts[partCount++] = data.substr(prevComma, endIdx - prevComma);
      if (parts[partCount-1].rfind(",", 0) == 0) {
        parts[partCount-1].erase(0, 1);
      }
      prevComma = i;
      if (partCount >= 12) break;
    }
  }

  if (partCount < 9) return false;

  std::string latStr = parts[5];
  std::string nsStr = parts[6];
  std::string lonStr = parts[7];
  std::string ewStr = parts[8];

  if (latStr.length() == 0 || lonStr.length() == 0 || nsStr.length() == 0 || ewStr.length() == 0) {
    return false;
  }

  double rawLat = atof(latStr.c_str());
  int latDeg = (int)(rawLat / 100);
  double latMin = rawLat - (latDeg * 100);
  latitude = latDeg + (latMin / 60.0);
  if (nsStr.find("S") != std::string::npos) latitude = -latitude;

  double rawLon = atof(lonStr.c_str());
  int lonDeg = (int)(rawLon / 100);
  double lonMin = rawLon - (lonDeg * 100);
  longitude = lonDeg + (lonMin / 60.0);
  if (ewStr.find("W") != std::string::npos) longitude = -longitude;

  return true;
}

HttpResponse A7670Modem::httpsRequest(const char* method, const char* url, const char* body, const char* bearerToken) {
  HttpResponse httpResp = { -1, "" };
  std::string resp;

  if (!sendATCommand("AT+HTTPINIT", resp, 2000)) {
    sendATCommand("AT+HTTPTERM", resp, 1000);
    if (!sendATCommand("AT+HTTPINIT", resp, 2000)) {
      return httpResp;
    }
  }

  sendATCommand("AT+HTTPPARA=\"URL\",\"" + std::string(url) + "\"", resp, 1000);
  sendATCommand("AT+HTTPPARA=\"CONTENT\",\"application/json\"", resp, 1000);

  if (bearerToken && strlen(bearerToken) > 0) {
    sendATCommand("AT+HTTPPARA=\"USERHDR\",\"Authorization: Bearer " + std::string(bearerToken) + "\"", resp, 1000);
  }

  sendATCommand("AT+HTTPPARA=\"SSLCFG\",1", resp, 1000);

  int actionType = 0;
  if (strcmp(method, "POST") == 0) actionType = 1;
  else if (strcmp(method, "PUT") == 0) actionType = 2;
  else if (strcmp(method, "DELETE") == 0) actionType = 3;

  if ((actionType == 1 || actionType == 2) && body && strlen(body) > 0) {
    int bodyLength = strlen(body);
    char cmd[40];
    sprintf(cmd, "AT+HTTPDATA=%d,10000", bodyLength);
    if (sendATCommand(cmd, resp, 2000)) {
      if (resp.find("DOWNLOAD") != std::string::npos) {
        uart_write_bytes(_uartPort, body, bodyLength);
        waitForResponse("OK", 5000);
      }
    }
  }

  char actionCmd[20];
  sprintf(actionCmd, "AT+HTTPACTION=%d", actionType);
  if (sendATCommand(actionCmd, resp, 10000)) {
    std::string actionResp = "";
    uint64_t startTime = esp_timer_get_time() / 1000;
    bool actionCompleted = false;

    while ((esp_timer_get_time() / 1000) - startTime < 15000) {
      char c;
      int len = uart_read_bytes(_uartPort, &c, 1, pdMS_TO_TICKS(10));
      if (len > 0) {
        actionResp += c;
        if (actionResp.find("+HTTPACTION:") != std::string::npos && actionResp.back() == '\n') {
          actionCompleted = true;
          break;
        }
      }
    }

    if (actionCompleted) {
      size_t pos = actionResp.find("+HTTPACTION:");
      if (pos != std::string::npos) {
        size_t firstComma = actionResp.find(",", pos);
        size_t secondComma = actionResp.find(",", firstComma + 1);
        if (firstComma != std::string::npos && secondComma != std::string::npos) {
          std::string statusStr = actionResp.substr(firstComma + 1, secondComma - (firstComma + 1));
          httpResp.status = atoi(statusStr.c_str());
          
          std::string lenStr = actionResp.substr(secondComma + 1);
          int bodyLen = atoi(lenStr.c_str());

          if (bodyLen > 0) {
            char readCmd[32];
            sprintf(readCmd, "AT+HTTPREAD=0,%d", bodyLen);
            std::string readResp;
            sendATCommand(readCmd, readResp, 5000);
            
            size_t dataPos = readResp.find("+HTTPREAD: DATA,");
            if (dataPos != std::string::npos) {
              size_t newline = readResp.find("\n", dataPos);
              if (newline != std::string::npos) {
                size_t okPos = readResp.find("OK", newline);
                if (okPos != std::string::npos) {
                  httpResp.body = readResp.substr(newline + 1, okPos - (newline + 2));
                } else {
                  httpResp.body = readResp.substr(newline + 1);
                }
              }
            }
          }
        }
      }
    }
  }

  sendATCommand("AT+HTTPTERM", resp, 2000);
  return httpResp;
}

bool A7670Modem::sendATCommand(const std::string& cmd, std::string& response, uint32_t timeoutMs) {
  std::string cmd_str = cmd + "\r\n";
  uart_write_bytes(_uartPort, cmd_str.c_str(), cmd_str.length());
  
  response.clear();
  char buf[128];
  uint64_t startTime = esp_timer_get_time() / 1000;
  
  while ((esp_timer_get_time() / 1000) - startTime < timeoutMs) {
    int len = uart_read_bytes(_uartPort, buf, sizeof(buf) - 1, pdMS_TO_TICKS(50));
    if (len > 0) {
      buf[len] = '\0';
      response += buf;
      if (response.find("OK\r") != std::string::npos || response.find("ERROR\r") != std::string::npos) {
        return (response.find("OK\r") != std::string::npos);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  
  return false;
}

bool A7670Modem::waitForResponse(const std::string& expected, uint32_t timeoutMs) {
  std::string response = "";
  char c;
  uint64_t startTime = esp_timer_get_time() / 1000;
  
  while ((esp_timer_get_time() / 1000) - startTime < timeoutMs) {
    int len = uart_read_bytes(_uartPort, &c, 1, pdMS_TO_TICKS(10));
    if (len > 0) {
      response += c;
      if (response.find(expected) != std::string::npos) {
        return true;
      }
    }
  }
  return false;
}

void A7670Modem::clearSerialBuffer() {
  uart_flush(_uartPort);
}
