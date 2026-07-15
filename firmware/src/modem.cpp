#include "modem.h"
#include "config.h"
#include "watchdog.h"
#include "Arduino.h"

// A7670E is on hardware Serial1
static HardwareSerial& _ser = Serial1;

// ── Internal helpers ──────────────────────────────────────────
static const uint32_t AT_TIMEOUT_MS = 10000; // hard limit per command

// Drain the RX buffer completely
static void _flush_rx() {
    uint32_t t = millis();
    while (millis() - t < 100) {
        while (_ser.available()) _ser.read();
    }
}

// Send a command and wait for an expected response substring.
// Returns true if found within timeout_ms.
static bool _at_wait(const String& cmd,
                     const String& expect,
                     uint32_t      timeout_ms = AT_TIMEOUT_MS) {
    _ser.print(cmd + "\r\n");
    Serial.println("[MDM>] " + cmd);

    String buf;
    uint32_t start = millis();
    while (millis() - start < timeout_ms) {
        while (_ser.available()) {
            char c = _ser.read();
            buf   += c;
        }
        if (buf.indexOf(expect) != -1) {
            Serial.println("[MDM<] " + buf);
            return true;
        }
        if (buf.indexOf("ERROR") != -1) {
            Serial.println("[MDM<] ERROR: " + buf);
            return false;
        }
        delay(5);
    }
    Serial.println("[MDM] Timeout waiting for: " + expect);
    return false;
}

// Send a command and collect the full response within timeout_ms.
// Stops collecting when response contains terminator.
static String _at_read(const String& cmd,
                        const String& terminator = "OK",
                        uint32_t      timeout_ms = AT_TIMEOUT_MS) {
    _ser.print(cmd + "\r\n");
    Serial.println("[MDM>] " + cmd);

    String   buf;
    uint32_t start = millis();
    while (millis() - start < timeout_ms) {
        while (_ser.available()) buf += (char)_ser.read();
        if (buf.indexOf(terminator) != -1) break;
        if (buf.indexOf("ERROR")    != -1) break;
        delay(5);
    }
    Serial.println("[MDM<] " + buf);
    return buf;
}

// ── Lifecycle ─────────────────────────────────────────────────
bool modem_init() {
    _ser.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
    delay(100);

    // Set DTR pin as output, pull low (wake modem if sleeping)
    pinMode(MODEM_DTR_PIN, OUTPUT);
    digitalWrite(MODEM_DTR_PIN, LOW);

    // RI is input only
    pinMode(MODEM_RI_PIN, INPUT);

    Serial.println("[MDM] UART started — waiting for AT echo...");

    // The modem auto-powers when the 4G DIP switch is ON.
    // Wait for it to boot, then echo test.
    delay(MODEM_BOOT_WAIT_MS);
    wdt_reset();

    for (int i = 0; i < MODEM_AT_ECHO_RETRIES; i++) {
        _flush_rx();
        if (_at_wait("AT", "OK", 2000)) {
            Serial.println("[MDM] Modem alive!");
            // Disable echo, enable verbose errors
            _at_wait("ATE0", "OK");
            _at_wait("AT+CMEE=2", "OK");
            return true;
        }
        Serial.printf("[MDM] AT echo attempt %d/%d...\n", i + 1, MODEM_AT_ECHO_RETRIES);
        wdt_reset();
        delay(1000);
    }

    Serial.println("[MDM] Modem not responding after boot wait");
    return false;
}

bool modem_is_alive() {
    _flush_rx();
    return _at_wait("AT", "OK", 2000);
}

bool modem_soft_reset() {
    Serial.println("[MDM] Sending AT+CRESET...");

    // Drain RX of any stale bytes before the reset command
    _flush_rx();
    _ser.print("AT+CRESET\r\n");

    // Don't wait for response — module will go silent immediately
    delay(MODEM_CRESET_WAIT_MS);
    wdt_reset();

    // Re-echo test
    for (int i = 0; i < MODEM_AT_ECHO_RETRIES; i++) {
        _flush_rx();
        if (_at_wait("AT", "OK", 2000)) {
            Serial.println("[MDM] Modem alive after CRESET");
            _at_wait("ATE0",    "OK");
            _at_wait("AT+CMEE=2", "OK");
            return true;
        }
        wdt_reset();
        delay(1000);
    }
    Serial.println("[MDM] Modem still unresponsive after CRESET");
    return false;
}

// ── Network ───────────────────────────────────────────────────
bool modem_register_network() {
    // Verify SIM is ready
    if (!_at_wait("AT+CPIN?", "READY", 5000)) {
        Serial.println("[MDM] SIM not ready");
        return false;
    }

    // Set full functionality
    _at_wait("AT+CFUN=1", "OK", 5000);

    // Wait for network registration (max 90s)
    Serial.println("[MDM] Waiting for network registration...");
    uint32_t deadline = millis() + 90000;
    while (millis() < deadline) {
        String r = _at_read("AT+CREG?", "OK", 3000);
        // Registered: +CREG: 0,1 (home) or +CREG: 0,5 (roaming)
        if (r.indexOf(",1") != -1 || r.indexOf(",5") != -1) {
            Serial.println("[MDM] Network registered!");
            return true;
        }
        wdt_reset();
        delay(3000);
    }
    Serial.println("[MDM] Network registration timed out");
    return false;
}

bool modem_open_data_session() {
    // Configure APN (A1 Telekom Austria)
    if (!_at_wait("AT+CGDCONT=1,\"IP\",\"" CELLULAR_APN "\"", "OK"))
        return false;

    // PAP authentication
    if (!_at_wait("AT+CGAUTH=1,1,\"" CELLULAR_PASS "\",\"" CELLULAR_USER "\"", "OK"))
        return false;

    // Activate PDP context
    if (!_at_wait("AT+CGACT=1,1", "OK", 15000)) {
        // Try manual dial as fallback
        if (!_at_wait("AT+CGACT=1,1", "OK", 15000))
            return false;
    }

    // Confirm IP address assigned
    String ip = _at_read("AT+CIFSR", "OK", 5000);
    if (ip.indexOf("ERROR") != -1 || ip.length() < 10) {
        Serial.println("[MDM] No IP assigned");
        return false;
    }
    Serial.println("[MDM] Data session open. IP response: " + ip);
    return true;
}

bool modem_close_data_session() {
    return _at_wait("AT+CGACT=0,1", "OK", 10000);
}

bool modem_data_session_active() {
    String r = _at_read("AT+CGACT?", "OK", 3000);
    // +CGACT: 1,1 means context 1 is active
    return (r.indexOf("+CGACT: 1,1") != -1);
}

int modem_get_rssi() {
    String r = _at_read("AT+CSQ", "OK", 3000);
    // +CSQ: 20,0  → first number is RSSI code
    int idx = r.indexOf("+CSQ: ");
    if (idx == -1) return -1;
    int code = r.substring(idx + 6).toInt();
    if (code == 99) return -1;           // Unknown
    return -113 + code * 2;             // Convert to dBm
}

// ── HTTP helpers ──────────────────────────────────────────────

// Initialise the A7670E built-in HTTP stack for one request.
// Returns false if initialization fails.
static bool _http_init(const String& url,
                        const String& extra_headers = "") {
    if (!_at_wait("AT+HTTPINIT", "OK")) {
        // Already initialized — terminate and retry
        _at_wait("AT+HTTPTERM", "OK", 3000);
        if (!_at_wait("AT+HTTPINIT", "OK")) return false;
    }
    // Set URL
    if (!_at_wait("AT+HTTPPARA=\"URL\",\"" + url + "\"", "OK")) return false;

    // Content-type default
    _at_wait("AT+HTTPPARA=\"CONTENT\",\"application/json\"", "OK");

    // Custom headers (e.g. Authorization, x-http-method-override)
    if (extra_headers.length() > 0) {
        // The A7670E supports AT+HTTPPARA="USERDATA" for custom headers
        if (!_at_wait("AT+HTTPPARA=\"USERDATA\",\"" + extra_headers + "\"", "OK"))
            return false;
    }

    // Enable SSL/TLS for https
    _at_wait("AT+HTTPSSL=1", "OK");

    return true;
}

// Terminate the HTTP stack cleanly after each request
static void _http_term() {
    _at_wait("AT+HTTPTERM", "OK", 3000);
}

// Parse HTTP status code from +HTTPACTION response
// Format: +HTTPACTION: <method>,<status>,<data_len>
static int _parse_status(const String& resp) {
    int idx = resp.indexOf("+HTTPACTION:");
    if (idx == -1) return -1;
    String s = resp.substring(idx + 12);  // skip "+HTTPACTION:"
    s.trim();
    // s is now: " <method>,<status>,<len>"
    int c1 = s.indexOf(',');
    if (c1 == -1) return -1;
    int c2 = s.indexOf(',', c1 + 1);
    String code_str = (c2 == -1) ? s.substring(c1 + 1) : s.substring(c1 + 1, c2);
    return code_str.toInt();
}

// Read body after a successful HTTPACTION
static String _http_read_body() {
    String r = _at_read("AT+HTTPREAD=0,4096", "OK", 10000);
    // Response: +HTTPREAD: <len>\r\n<body>\r\nOK
    int idx = r.indexOf("+HTTPREAD:");
    if (idx == -1) return "";
    int nl = r.indexOf('\n', idx);
    if (nl == -1) return "";
    String body = r.substring(nl + 1);
    // Strip trailing OK and whitespace
    int ok = body.lastIndexOf("OK");
    if (ok != -1) body = body.substring(0, ok);
    body.trim();
    return body;
}

int modem_http_get(const String& url,
                   const String& auth_header,
                   String&       resp_out) {
    if (!_http_init(url, auth_header)) { _http_term(); return -1; }

    // Action: GET = 0
    String r = _at_read("AT+HTTPACTION=0", "+HTTPACTION:", 30000);
    int status = _parse_status(r);
    if (status > 0) resp_out = _http_read_body();
    _http_term();
    return status;
}

int modem_http_post(const String& url,
                    const String& content_type,
                    const String& body,
                    const String& auth_header,
                    String&       resp_out) {
    String headers = "";
    if (auth_header.length() > 0) headers = auth_header;

    if (!_http_init(url, headers)) { _http_term(); return -1; }

    _at_wait("AT+HTTPPARA=\"CONTENT\",\"" + content_type + "\"", "OK");

    // Send body data
    String cmd = "AT+HTTPDATA=" + String(body.length()) + ",10000";
    if (!_at_wait(cmd, "DOWNLOAD", 5000)) { _http_term(); return -1; }
    _ser.print(body);
    delay(500);  // give modem time to buffer
    if (!_at_wait("", "OK", 5000)) { _http_term(); return -1; }

    // Action: POST = 1
    String r = _at_read("AT+HTTPACTION=1", "+HTTPACTION:", 30000);
    int status = _parse_status(r);
    if (status > 0) resp_out = _http_read_body();
    _http_term();
    return status;
}

int modem_http_patch(const String& url,
                     const String& body,
                     const String& id_token,
                     String&       resp_out) {
    // Firebase RTDB does not accept PATCH via the A7670E HTTP stack directly,
    // so we POST to the URL with x-http-method-override: PATCH.
    // Firebase honors this override header.
    String patch_url = url + "?x-http-method-override=PATCH";
    String auth = "Authorization: Bearer " + id_token + "\r\n"
                  "x-http-method-override: PATCH";
    return modem_http_post(patch_url,
                           "application/json",
                           body,
                           auth,
                           resp_out);
}
