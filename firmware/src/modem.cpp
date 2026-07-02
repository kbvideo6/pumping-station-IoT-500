#include "modem.h"

A7670Modem::A7670Modem(HardwareSerial& serialPort, int pwrPin, int rstPin) : _serial(serialPort) {
  _pwrPin = pwrPin;
  _rstPin = rstPin;
}

void A7670Modem::begin() {
  pinMode(_pwrPin, OUTPUT);
  pinMode(_rstPin, OUTPUT);
  digitalWrite(_pwrPin, HIGH); // Default state
  digitalWrite(_rstPin, HIGH); // Default state
}

bool A7670Modem::powerOn() {
  Serial.println("[Modem] Powering on...");
  digitalWrite(_pwrPin, LOW);
  delay(1500); // Hold low for 1.5s
  digitalWrite(_pwrPin, HIGH);
  delay(4000); // Wait for module bootup

  // Test communication
  String resp;
  for (int i = 0; i < 5; i++) {
    if (sendATCommand("AT", resp, 1000)) {
      sendATCommand("ATE0", resp, 1000); // Echo off
      Serial.println("[Modem] Powered on and responsive.");
      return true;
    }
    delay(1000);
  }
  Serial.println("[Modem] Failed to power on via PWRKEY.");
  return false;
}

void A7670Modem::powerOff() {
  Serial.println("[Modem] Powering off...");
  String resp;
  sendATCommand("AT+CPOWD", resp, 2000);
  digitalWrite(_pwrPin, LOW);
  delay(2000);
  digitalWrite(_pwrPin, HIGH);
}

void A7670Modem::hardReset() {
  Serial.println("[Modem] Hardware Reset!");
  digitalWrite(_rstPin, LOW);
  delay(500);
  digitalWrite(_rstPin, HIGH);
  delay(4000);
}

bool A7670Modem::initializeModem() {
  String resp;
  if (!sendATCommand("AT", resp, 1000)) return false;
  sendATCommand("ATE0", resp, 1000); // Echo off
  sendATCommand("AT+CMEE=2", resp, 1000); // Verbose error codes
  return checkSIMStatus();
}

bool A7670Modem::checkSIMStatus() {
  String resp;
  for (int i = 0; i < 5; i++) {
    if (sendATCommand("AT+CPIN?", resp, 1000)) {
      if (resp.indexOf("READY") != -1) {
        Serial.println("[Modem] SIM Card ready.");
        return true;
      }
    }
    delay(1000);
  }
  Serial.println("[Modem] SIM Card not ready or missing.");
  return false;
}

int A7670Modem::getSignalQuality() {
  String resp;
  if (sendATCommand("AT+CSQ", resp, 1000)) {
    // Response format: +CSQ: <rssi>,<ber>
    int index = resp.indexOf("+CSQ:");
    if (index != -1) {
      String rssiStr = resp.substring(index + 6, resp.indexOf(",", index));
      int rssi = rssiStr.toInt();
      if (rssi == 99) return -113; // Unknown or not detectable
      // Map RSSI (0-31) to dBm
      return -113 + (rssi * 2);
    }
  }
  return -113;
}

bool A7670Modem::connectNetwork(const char* apn) {
  String resp;
  Serial.printf("[Modem] Connecting to network APN: %s...\n", apn);

  // Set LTE only or auto mode
  sendATCommand("AT+CNMP=38", resp, 1000); // LTE Only
  
  // Wait for registration
  for (int i = 0; i < 15; i++) {
    if (sendATCommand("AT+CGREG?", resp, 1000)) {
      // 1 = Registered, home network. 5 = Registered, roaming
      if (resp.indexOf(",1") != -1 || resp.indexOf(",5") != -1) {
        Serial.println("[Modem] Registered on network.");
        break;
      }
    }
    delay(1000);
  }

  // Setup APN context
  sendATCommand("AT+CGDCONT=1,\"IP\",\"" + String(apn) + "\"", resp, 1000);
  
  // Activate context
  sendATCommand("AT+CGACT=1,1", resp, 3000);

  if (isNetworkConnected()) {
    Serial.println("[Modem] Cellular network data connection established.");
    return true;
  }
  return false;
}

bool A7670Modem::isNetworkConnected() {
  String resp;
  if (sendATCommand("AT+CGACT?", resp, 1000)) {
    if (resp.indexOf("1,1") != -1) {
      return true;
    }
  }
  return false;
}

bool A7670Modem::enableGPS(bool enable) {
  String resp;
  if (enable) {
    Serial.println("[Modem] Enabling GNSS/GPS Power...");
    return sendATCommand("AT+CGNSSPWR=1", resp, 2000);
  } else {
    Serial.println("[Modem] Disabling GNSS/GPS Power...");
    return sendATCommand("AT+CGNSSPWR=0", resp, 2000);
  }
}

bool A7670Modem::getGPS(double& latitude, double& longitude) {
  String resp;
  if (!sendATCommand("AT+CGNSSINFO", resp, 2000)) return false;

  int infoIdx = resp.indexOf("+CGNSSINFO:");
  if (infoIdx == -1) return false;

  String data = resp.substring(infoIdx + 11);
  data.trim();

  // Split by comma
  int commaIndex = -1;
  String parts[12];
  int partCount = 0;
  
  for (int i = 0; i < data.length(); i++) {
    if (data[i] == ',' || i == data.length() - 1) {
      int endIdx = (data[i] == ',') ? i : i + 1;
      parts[partCount++] = data.substring(commaIndex + 1, endIdx);
      commaIndex = i;
      if (partCount >= 12) break;
    }
  }

  if (partCount < 9) return false;

  String latStr = parts[5];
  String nsStr = parts[6];
  String lonStr = parts[7];
  String ewStr = parts[8];

  if (latStr.length() == 0 || lonStr.length() == 0 || nsStr.length() == 0 || ewStr.length() == 0) {
    return false; // No GPS fix yet
  }

  // Parse Latitude: DDMM.MMMM
  double rawLat = latStr.toDouble();
  int latDeg = (int)(rawLat / 100);
  double latMin = rawLat - (latDeg * 100);
  latitude = latDeg + (latMin / 60.0);
  if (nsStr.indexOf("S") != -1) latitude = -latitude;

  // Parse Longitude: DDDMM.MMMM
  double rawLon = lonStr.toDouble();
  int lonDeg = (int)(rawLon / 100);
  double lonMin = rawLon - (lonDeg * 100);
  longitude = lonDeg + (lonMin / 60.0);
  if (ewStr.indexOf("W") != -1) longitude = -longitude;

  return true;
}

HttpResponse A7670Modem::httpsRequest(const char* method, const char* url, const char* body, const char* bearerToken) {
  HttpResponse httpResp = { -1, "" };
  String resp;

  // Initialize HTTP Service
  if (!sendATCommand("AT+HTTPINIT", resp, 2000)) {
    // Terminate and reinit just in case
    sendATCommand("AT+HTTPTERM", resp, 1000);
    if (!sendATCommand("AT+HTTPINIT", resp, 2000)) {
      return httpResp;
    }
  }

  // Setup parameters
  sendATCommand("AT+HTTPPARA=\"URL\",\"" + String(url) + "\"", resp, 1000);
  sendATCommand("AT+HTTPPARA=\"CONTENT\",\"application/json\"", resp, 1000);
  
  if (bearerToken && strlen(bearerToken) > 0) {
    sendATCommand("AT+HTTPPARA=\"USERHDR\",\"Authorization: Bearer " + String(bearerToken) + "\"", resp, 1000);
  }

  // Set SSL configuration
  sendATCommand("AT+HTTPPARA=\"SSLCFG\",1", resp, 1000);

  int actionType = 0; // 0 = GET, 1 = POST, 2 = PUT, 3 = DELETE
  if (strcmp(method, "POST") == 0) actionType = 1;
  else if (strcmp(method, "PUT") == 0) actionType = 2;
  else if (strcmp(method, "DELETE") == 0) actionType = 3;

  // Write Request Body if there is any (POST/PUT)
  if ((actionType == 1 || actionType == 2) && body && strlen(body) > 0) {
    int bodyLength = strlen(body);
    // Write size and timeout
    if (sendATCommand("AT+HTTPDATA=" + String(bodyLength) + ",10000", resp, 2000)) {
      if (resp.indexOf("DOWNLOAD") != -1) {
        _serial.print(body);
        waitForResponse("OK", 5000);
      }
    }
  }

  // Trigger HTTP action
  if (sendATCommand("AT+HTTPACTION=" + String(actionType), resp, 10000)) {
    // Look for response notification code: +HTTPACTION: <method>,<status>,<length>
    unsigned long startTime = millis();
    bool actionCompleted = false;
    String actionResp = "";
    
    while (millis() - startTime < 15000) {
      if (_serial.available()) {
        char c = _serial.read();
        actionResp += c;
        if (actionResp.indexOf("+HTTPACTION:") != -1 && actionResp.endsWith("\n")) {
          actionCompleted = true;
          break;
        }
      }
      delay(5);
    }

    if (actionCompleted) {
      int index = actionResp.indexOf("+HTTPACTION:");
      // Parse: +HTTPACTION: 1,200,45
      int firstComma = actionResp.indexOf(",", index);
      int secondComma = actionResp.indexOf(",", firstComma + 1);
      
      String statusStr = actionResp.substring(firstComma + 1, secondComma);
      httpResp.status = statusStr.toInt();
      
      String lengthStr = actionResp.substring(secondComma + 1);
      lengthStr.trim();
      int dataLength = lengthStr.toInt();

      if (dataLength > 0 && httpResp.status >= 200 && httpResp.status < 300) {
        // Read response body content
        if (sendATCommand("AT+HTTPREAD=0," + String(dataLength), resp, 5000)) {
          int readIndex = resp.indexOf("+HTTPREAD: DATA,");
          if (readIndex != -1) {
            int bodyStartIndex = resp.indexOf("\n", readIndex + 16);
            if (bodyStartIndex != -1) {
              httpResp.body = resp.substring(bodyStartIndex + 1);
              httpResp.body.trim();
            }
          }
        }
      }
    }
  }

  // Clean up HTTP context
  sendATCommand("AT+HTTPTERM", resp, 1000);
  return httpResp;
}

bool A7670Modem::sendATCommand(const String& cmd, String& response, uint32_t timeoutMs) {
  clearSerialBuffer();
  _serial.println(cmd);

  unsigned long start = millis();
  response = "";

  while (millis() - start < timeoutMs) {
    if (_serial.available()) {
      char c = _serial.read();
      response += c;
      if (response.indexOf("OK\r\n") != -1 || response.indexOf("ERROR\r\n") != -1) {
        return response.indexOf("OK\r\n") != -1;
      }
    }
    delay(5);
  }
  
  // Timeout
  return false;
}

bool A7670Modem::waitForResponse(const String& expected, uint32_t timeoutMs) {
  unsigned long start = millis();
  String buffer = "";

  while (millis() - start < timeoutMs) {
    if (_serial.available()) {
      char c = _serial.read();
      buffer += c;
      if (buffer.indexOf(expected) != -1) {
        return true;
      }
    }
    delay(5);
  }
  return false;
}

void A7670Modem::clearSerialBuffer() {
  while (_serial.available()) {
    _serial.read();
  }
}
