#pragma once
#include "Arduino.h"

// Two-step Firebase device authentication:
//   Step 1: deviceToken → Cloud Function → customToken (short-lived JWT)
//   Step 2: customToken → Firebase Auth REST → idToken + refreshToken
//
// After first auth, tokens are stored in NVS permanently.
// On subsequent boots, auth_ensure_valid() silently refreshes the idToken
// using the stored refreshToken (no Cloud Function call needed).

// auth_begin():
//   Call once after modem is connected.
//   Checks NVS for existing tokens:
//     - If refreshToken exists → refresh idToken only (fast path)
//     - If not → full two-step auth from deviceToken
//   Returns true if a valid idToken is ready for use.
bool auth_begin(const String& device_token);

// auth_ensure_valid():
//   Call before every upload. Checks if idToken expires within
//   TOKEN_REFRESH_HEADROOM_S seconds and refreshes if needed.
//   Returns true if idToken is still valid (no action needed) or refreshed OK.
bool auth_ensure_valid();

// Returns the current idToken for use in Authorization headers.
String auth_get_id_token();
