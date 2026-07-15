#pragma once
#include "Arduino.h"

// A7670E AT-command driver.
//
// All functions return true on success, false on timeout or unexpected response.
// No function blocks longer than MODEM_AT_TIMEOUT_MS (10 000 ms).
// AT+CPOF is intentionally absent — using it would brick this board.

// ── Lifecycle ─────────────────────────────────────────────────
bool modem_init();             // Begin UART, wait for AT echo (15 retries × 1s)
bool modem_soft_reset();       // AT+CRESET, wait 15s, re-echo test
bool modem_is_alive();         // Send AT, expect OK within 2s

// ── Network registration + data session ──────────────────────
bool modem_register_network(); // AT+CREG loop until registered (max 90s)
bool modem_open_data_session();// APN + PDP context activate
bool modem_close_data_session();
bool modem_data_session_active(); // Quick check: AT+CGACT? returns 1

// ── Signal quality ────────────────────────────────────────────
int  modem_get_rssi();         // Returns -1 on failure, else dBm (approx)

// ── HTTP operations ───────────────────────────────────────────
// Returns HTTP status code (200, 201, etc.) or -1 on failure.
// body_in:  request body string (may be empty)
// resp_out: response body written here (pass String&)
int modem_http_get (const String& url,
                    const String& auth_header,
                    String&       resp_out);

int modem_http_post(const String& url,
                    const String& content_type,
                    const String& body,
                    const String& auth_header,
                    String&       resp_out);

// Firebase RTDB PATCH via POST + x-http-method-override header
int modem_http_patch(const String& url,
                     const String& body,
                     const String& id_token,
                     String&       resp_out);
