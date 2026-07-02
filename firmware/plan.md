# Firmware Implementation Plan — ESP32-S3 + A7670E

## Overview

The firmware runs on the **Waveshare ESP32-S3 A7670E 4G Dev Board**, reads current from an **SCT-013-020 CT clamp**, authenticates with Firebase using a custom token, and transmits data over **HTTPS via the A7670E LTE modem**.

---

## Hardware Pin Mapping

| Function | Pin | Notes |
|---|---|---|
| CT Clamp Analog Input | GPIO1 (ADC1_CH0) | Via burden resistor + bias circuit |
| A7670E TX (modem → ESP) | GPIO17 (UART1 RX) | Waveshare default |
| A7670E RX (ESP → modem) | GPIO18 (UART1 TX) | Waveshare default |
| A7670E PWRKEY | GPIO4 | Power on/off modem |
| A7670E RESET | GPIO5 | Hardware reset modem |
| Status LED | GPIO2 | Onboard LED for status indication |

> [!NOTE]
> Pin mapping must be verified against the exact Waveshare ESP32-S3 A7670E schematic. The above is based on typical Waveshare 4G board configurations.

---

## Development Environment

- **Framework**: Arduino (via PlatformIO) or ESP-IDF
- **Recommended**: PlatformIO with Arduino framework for faster prototyping
- **Libraries**:
  - No TinyGSM — use raw AT commands for full control of A7670E
  - `ArduinoJson` — JSON serialization/deserialization
  - Custom AT command handler (see below)

---

## Module Breakdown

### 1. `main.cpp` — Entry Point & Main Loop

**Responsibilities:**
- Initialize all modules
- Run the main state machine
- Handle watchdog feed

**State Machine:**
```
BOOT → MODEM_INIT → NETWORK_CONNECT → AUTH → RUNNING → [ERROR → RECOVERY → MODEM_INIT]
                                                  ↓
                                          ┌───────────────┐
                                          │  Every 30s:   │
                                          │  read sensor   │
                                          │  check thresh  │
                                          │  send data     │
                                          ├───────────────┤
                                          │  Every 5min:  │
                                          │  poll config   │
                                          │  refresh token │
                                          ├───────────────┤
                                          │  Every 24hr:  │
                                          │  check OTA     │
                                          └───────────────┘
```

**Pseudo-code:**
```cpp
void setup() {
    Serial.begin(115200);
    initWatchdog(120);        // 120s hardware watchdog
    initStatusLED();
    initCTSensor();
    initModem();
    connectNetwork();
    authenticateFirebase();
    loadConfigFromFirebase();
    feedWatchdog();
}

void loop() {
    feedWatchdog();
    
    if (millis() - lastRead >= 5000) {
        currentRMS = readCTSensor();
        addToBuffer(currentRMS);
    }
    
    if (millis() - lastSend >= config.reportIntervalMs) {
        float avgCurrent = computeAverage(buffer);
        bool alert = checkThresholds(avgCurrent);
        sendToFirebase(avgCurrent, alert);
        clearBuffer();
    }
    
    if (millis() - lastConfigPoll >= config.configPollIntervalMs) {
        pollConfig();
    }
    
    if (millis() - lastTokenRefresh >= 3000000) { // 50 min
        refreshFirebaseToken();
    }
    
    if (millis() - lastOTACheck >= 86400000) { // 24h
        checkOTAUpdate();
    }
}
```

---

### 2. `modem.h / modem.cpp` — A7670E AT Command Driver

**Responsibilities:**
- Power on/off the A7670E modem
- Send AT commands and parse responses
- Manage network registration
- Execute HTTPS requests

**Key AT Commands:**

| Command | Purpose |
|---|---|
| `AT` | Basic check — modem alive |
| `AT+CPIN?` | Check SIM card status |
| `AT+CSQ` | Signal quality (RSSI) |
| `AT+CREG?` | Network registration status |
| `AT+CGDCONT=1,"IP","iot.1nce.net"` | Set APN for 1NCE SIM |
| `AT+CGACT=1,1` | Activate PDP context |
| `AT+HTTPINIT` | Init HTTP client |
| `AT+HTTPPARA="URL","https://..."` | Set request URL |
| `AT+HTTPPARA="CONTENT","application/json"` | Set content type |
| `AT+HTTPDATA=<len>,<timeout>` | Prepare POST/PUT body |
| `AT+HTTPACTION=1` | Execute POST |
| `AT+HTTPACTION=0` | Execute GET |
| `AT+HTTPREAD` | Read response |
| `AT+HTTPTERM` | Close HTTP client |
| `AT+HTTPTOFS` | Download file (for OTA) |

**Functions to Implement:**

```cpp
class Modem {
public:
    bool init();                          // Power on, wait for ready
    bool checkSIM();                      // Verify SIM inserted
    int  getSignalQuality();              // Return RSSI (0-31 scale)
    bool connectNetwork(const char* apn); // Register + activate PDP
    bool isConnected();                   // Check network status
    
    // HTTPS operations
    HttpResponse httpsPUT(const char* url, const char* json, const char* authToken);
    HttpResponse httpsGET(const char* url, const char* authToken);
    HttpResponse httpsPOST(const char* url, const char* json, const char* authToken);
    
    // Low level
    String sendAT(const char* cmd, uint32_t timeoutMs = 5000);
    bool   waitForResponse(const char* expected, uint32_t timeoutMs);
    void   powerOn();
    void   powerOff();
    void   hardReset();
    
private:
    HardwareSerial& serial;
    bool networkReady;
};
```

**Error Handling:**
- Retry AT commands up to 3 times with exponential backoff (1s, 2s, 4s)
- If modem unresponsive after 3 retries → hardware reset via PWRKEY
- If still unresponsive → full power cycle
- Log all errors to serial for debugging

---

### 3. `sensor.h / sensor.cpp` — CT Clamp Current Sensor

**Responsibilities:**
- Read analog voltage from SCT-013-020
- Compute RMS current from AC waveform
- Calibrate for accuracy

**Hardware Circuit:**
```
CT Clamp (SCT-013-020, 20A/1V output)
    │
    ├──── Burden Resistor (if needed, SCT-013-020 has built-in)
    │
    ├──── Bias Voltage (1.65V via voltage divider: 2x 10kΩ from 3.3V)
    │         │
    │         ├── 10kΩ ── 3.3V
    │         ├── 10kΩ ── GND
    │         └── 10µF cap to GND (smoothing)
    │
    └──── ADC Input (GPIO1)
```

**RMS Calculation:**
```cpp
struct SensorConfig {
    int adcPin = 1;
    float calibration = 20.0;       // SCT-013-020: 20A range
    float adcVref = 3.3;
    int adcResolution = 4095;       // 12-bit ADC
    float biasVoltage = 1.65;       // Mid-point bias
    int sampleCount = 1000;         // Samples per RMS calculation
    int samplePeriodUs = 200;       // 200µs between samples = 5kHz
};

float readCurrentRMS() {
    float sumSquares = 0;
    
    for (int i = 0; i < config.sampleCount; i++) {
        int raw = analogRead(config.adcPin);
        float voltage = (raw / (float)config.adcResolution) * config.adcVref;
        float current = (voltage - config.biasVoltage) * config.calibration;
        sumSquares += current * current;
        delayMicroseconds(config.samplePeriodUs);
    }
    
    return sqrt(sumSquares / config.sampleCount);
}
```

**Calibration:**
- At deployment, compare reading against a known clamp meter
- Store calibration factor in NVS (non-volatile storage)
- Adjustable via Firebase config (`/stations/{id}/config/calibration`)

---

### 4. `firebase_auth.h / firebase_auth.cpp` — Device Authentication

**Responsibilities:**
- Exchange custom token for Firebase ID token
- Refresh token before expiry
- Store tokens securely

**Authentication Flow:**
```
1. Device boots with pre-loaded:
   - API_KEY (Firebase Web API Key)
   - CUSTOM_TOKEN (generated during provisioning via Cloud Function)

2. Exchange custom token for ID + refresh tokens:
   POST https://identitytoolkit.googleapis.com/v1/accounts:signInWithCustomToken?key={API_KEY}
   Body: { "token": "{CUSTOM_TOKEN}", "returnSecureToken": true }
   
   Response: { "idToken": "...", "refreshToken": "...", "expiresIn": "3600" }

3. Use idToken in all Firebase HTTPS calls:
   Authorization: Bearer {idToken}

4. Before expiry (every ~50 min), refresh:
   POST https://securetoken.googleapis.com/v1/token?key={API_KEY}
   Body: { "grant_type": "refresh_token", "refresh_token": "{REFRESH_TOKEN}" }
   
   Response: { "id_token": "...", "refresh_token": "...", "expires_in": "3600" }
```

**Implementation:**
```cpp
class FirebaseAuth {
public:
    bool signInWithCustomToken(const char* customToken);
    bool refreshToken();
    bool isTokenValid();
    const char* getIdToken();
    
private:
    String idToken;
    String refreshTokenStr;
    unsigned long tokenExpiryMs;
    Modem& modem;
};
```

**Token Storage:**
- Store `refreshToken` in ESP32 NVS (persists across reboots)
- On boot: try refresh first, fall back to custom token if refresh fails
- `customToken` also stored in NVS during provisioning

---

### 5. `firebase_data.h / firebase_data.cpp` — Data Transmission

**Responsibilities:**
- Format sensor data as JSON
- PUT live data to RTDB
- GET config from RTDB

**Firebase RTDB REST API:**

**Write live data:**
```
PUT https://{PROJECT}.firebaseio.com/stations/{STATION_ID}/live.json?auth={ID_TOKEN}

Body:
{
  "current": 12.4,
  "alert": false,
  "alertType": null,
  "rssi": -67,
  "timestamp": {".sv": "timestamp"},
  "firmwareVersion": "1.0.0",
  "uptimeSeconds": 86400
}
```

**Read config:**
```
GET https://{PROJECT}.firebaseio.com/stations/{STATION_ID}/config.json?auth={ID_TOKEN}

Response:
{
  "highThreshold": 18.0,
  "lowThreshold": 2.0,
  "reportIntervalSec": 30,
  "configPollIntervalSec": 300,
  "stationName": "Pumpstation Graz-Ost"
}
```

**Update status:**
```
PATCH https://{PROJECT}.firebaseio.com/stations/{STATION_ID}/status.json?auth={ID_TOKEN}

Body:
{
  "online": true,
  "lastSeen": {".sv": "timestamp"}
}
```

---

### 6. `alerts.h / alerts.cpp` — Edge Threshold Detection

**Responsibilities:**
- Compare current reading against thresholds
- Debounce alerts (don't fire on single spike)
- Set alert flag in payload

**Logic:**
```cpp
enum AlertType { NONE, HIGH_CURRENT, LOW_CURRENT, NO_CURRENT };

struct AlertState {
    AlertType type = NONE;
    int consecutiveBreaches = 0;
    int debounceCount = 3;           // Need 3 consecutive breaches
    unsigned long lastAlertMs = 0;
    unsigned long cooldownMs = 300000; // 5 min cooldown between alerts
};

AlertType checkThresholds(float current, float high, float low) {
    AlertType detected = NONE;
    
    if (current < 0.1) detected = NO_CURRENT;
    else if (current > high) detected = HIGH_CURRENT;
    else if (current < low) detected = LOW_CURRENT;
    
    if (detected != NONE) {
        state.consecutiveBreaches++;
        if (state.consecutiveBreaches >= state.debounceCount) {
            if (millis() - state.lastAlertMs > state.cooldownMs) {
                state.lastAlertMs = millis();
                state.type = detected;
                return detected;
            }
        }
    } else {
        state.consecutiveBreaches = 0;
        state.type = NONE;
    }
    
    return NONE;
}
```

---

### 7. `ota.h / ota.cpp` — Over-The-Air Updates

**Responsibilities:**
- Check Firebase Storage for new firmware binary
- Download and flash via A7670E HTTP download
- Verify checksum before applying

**Flow:**
```
1. GET /stations/{id}/config/latestFirmware from RTDB
   → Returns: { "version": "1.1.0", "url": "https://firebasestorage.../firmware_1.1.0.bin", "checksum": "abc123" }

2. Compare with current firmware version
   → If same, skip

3. Download binary via A7670E AT+HTTPTOFS command
   → Saves to modem filesystem

4. Read file from modem, verify SHA256 checksum

5. Write to ESP32 OTA partition via Update library

6. Reboot into new firmware

7. On first boot after OTA, report new version to Firebase
   → If boot fails, ESP32 rolls back to previous partition automatically
```

---

### 8. `config.h` — Configuration & Constants

```cpp
// Firebase
#define FIREBASE_PROJECT_ID   "pumping-station-iot"
#define FIREBASE_API_KEY      "AIzaSy..."
#define FIREBASE_DB_URL       "https://pumping-station-iot-default-rtdb.europe-west1.firebasedatabase.app"

// Station (burned during provisioning)
#define STATION_ID            "STATION_001"

// Network
#define APN                   "iot.1nce.net"
#define APN_USER              ""
#define APN_PASS              ""

// Timing defaults (overridden by Firebase config)
#define DEFAULT_REPORT_INTERVAL_MS    30000   // 30 seconds
#define DEFAULT_CONFIG_POLL_MS        300000  // 5 minutes
#define DEFAULT_SENSOR_READ_MS        5000    // 5 seconds
#define TOKEN_REFRESH_MS              3000000 // 50 minutes
#define OTA_CHECK_MS                  86400000 // 24 hours

// Sensor
#define CT_CLAMP_PIN          1
#define CT_CLAMP_CALIBRATION  20.0
#define ADC_VREF              3.3
#define BIAS_VOLTAGE          1.65
#define RMS_SAMPLE_COUNT      1000

// Alerts
#define DEFAULT_HIGH_THRESHOLD  18.0
#define DEFAULT_LOW_THRESHOLD   2.0
#define ALERT_DEBOUNCE_COUNT    3
#define ALERT_COOLDOWN_MS       300000  // 5 minutes

// Watchdog
#define WDT_TIMEOUT_SEC       120

// Modem
#define MODEM_UART            Serial1
#define MODEM_BAUD            115200
#define MODEM_TX_PIN          18
#define MODEM_RX_PIN          17
#define MODEM_PWRKEY_PIN      4
#define MODEM_RESET_PIN       5
```

---

### 9. `led.h / led.cpp` — Status LED Indicator

| Pattern | Meaning |
|---|---|
| Solid ON | Booting / initializing |
| Slow blink (1s) | Connecting to network |
| Fast blink (200ms) | Authenticating |
| Heartbeat (2 quick + pause) | Running normally |
| SOS pattern | Error / recovery mode |

---

## Provisioning Procedure (For Scaling)

1. Open provisioning tool (web page or serial terminal)
2. Enter: `stationId`, `stationName`, `pumpPowerKW`
3. Tool calls Cloud Function → generates custom token
4. Tool flashes firmware with `STATION_ID` and `CUSTOM_TOKEN` to NVS
5. Device boots, authenticates, appears on dashboard
6. Set thresholds via dashboard

**For the customer to self-provision (future):**
- Pre-flash firmware with a "setup mode"
- Device creates a WiFi AP on first boot
- Customer connects, enters station details via a simple web form
- Device stores config and switches to normal mode

---

## File Structure

```
firmware/
├── plan.md                  ← This file
├── src/
│   ├── main.cpp             ← Entry point, state machine
│   ├── modem.h / modem.cpp  ← A7670E AT command driver
│   ├── sensor.h / sensor.cpp ← CT clamp reader
│   ├── firebase_auth.h / .cpp ← Token auth
│   ├── firebase_data.h / .cpp ← RTDB read/write
│   ├── alerts.h / alerts.cpp  ← Threshold detection
│   ├── ota.h / ota.cpp        ← OTA update handler
│   ├── led.h / led.cpp        ← Status LED
│   └── config.h              ← All constants
├── platformio.ini            ← PlatformIO project config
└── wiring_guide.md           ← Physical wiring instructions
```

---

## Testing Plan

| Test | Method |
|---|---|
| CT clamp reads correctly | Compare with clamp meter, log via Serial |
| Modem connects to 4G | Monitor AT responses, verify IP assignment |
| Firebase auth works | Log token exchange, verify write succeeds |
| Data appears in RTDB | Check Firebase console after first transmission |
| Config polling works | Change threshold in console, verify device picks up |
| Alert triggers correctly | Simulate overcurrent, check alert flag in RTDB |
| Watchdog reboots on hang | Intentionally block loop, verify reboot |
| OTA update works | Upload new binary, verify device updates |
| Token refresh works | Wait 50 min, verify new token obtained |
| Power cycle recovery | Unplug and replug, verify reconnect + resume |
