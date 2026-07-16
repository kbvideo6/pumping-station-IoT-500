#include "firebase_auth.h"
#include "config.h"
#include "modem.h"
#include "nvs_storage.h"
#include <ArduinoJson.h>

static String _id_token = "";

// ── Internal: refresh idToken using stored refreshToken ───────
static bool _refresh_id_token() {
    String refresh_token = nvs_get_refresh_token();
    if (refresh_token.length() == 0) {
        Serial.println("[AUTH] No refresh_token in NVS");
        return false;
    }

    // POST to Firebase token refresh endpoint
    // Body: grant_type=refresh_token&refresh_token=<token>
    String body = "grant_type=refresh_token&refresh_token=" + refresh_token;
    String resp;
    int    status = modem_http_post(
        FIREBASE_REFRESH_URL,
        "application/x-www-form-urlencoded",
        body,
        "",
        resp
    );

    if (status != 200) {
        Serial.printf("[AUTH] Token refresh failed — HTTP %d\n", status);
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, resp);
    if (err) {
        Serial.println("[AUTH] JSON parse error on refresh response");
        return false;
    }

    const char* id_token      = doc["id_token"];
    const char* refresh_token_new = doc["refresh_token"];
    uint32_t    expires_in    = doc["expires_in"] | 3600;

    if (!id_token || strlen(id_token) < 10) {
        Serial.println("[AUTH] No id_token in refresh response");
        return false;
    }

    _id_token = String(id_token);
    nvs_set_id_token(_id_token);
    nvs_set_refresh_token(String(refresh_token_new));
    nvs_set_token_expiry((uint32_t)(millis() / 1000) + expires_in);

    Serial.println("[AUTH] idToken refreshed successfully");
    return true;
}

// ── Internal: full two-step auth from deviceToken ─────────────
static bool _full_auth(const String& device_token) {
    if (device_token.length() == 0) {
        Serial.println("[AUTH] device_token is empty — cannot authenticate");
        return false;
    }

    // Step 1: POST stationId and deviceToken to Cloud Function → get customToken
    Serial.println("[AUTH] Step 1: Getting customToken from Cloud Function...");
    String station_id = nvs_get_station_id();
    String body1 = "{\"stationId\":\"" + station_id + "\",\"deviceToken\":\"" + device_token + "\"}";
    String resp1;
    int    status1 = modem_http_post(
        FIREBASE_CF_TOKEN_URL,
        "application/json",
        body1,
        "",
        resp1
    );

    if (status1 != 200) {
        Serial.printf("[AUTH] Step 1 failed — HTTP %d\n", status1);
        return false;
    }

    JsonDocument doc1;
    if (deserializeJson(doc1, resp1) != DeserializationError::Ok) {
        Serial.println("[AUTH] Step 1 JSON parse error");
        return false;
    }
    const char* custom_token = doc1["customToken"];
    if (!custom_token || strlen(custom_token) < 10) {
        Serial.println("[AUTH] No customToken in response");
        return false;
    }
    Serial.println("[AUTH] Step 1 OK — customToken received");

    // Step 2: POST customToken to Firebase Auth REST → idToken + refreshToken
    Serial.println("[AUTH] Step 2: Signing in with customToken...");
    String body2 = "{\"token\":\"" + String(custom_token) +
                   "\",\"returnSecureToken\":true}";
    String resp2;
    int    status2 = modem_http_post(
        FIREBASE_SIGNIN_URL,
        "application/json",
        body2,
        "",
        resp2
    );

    if (status2 != 200) {
        Serial.printf("[AUTH] Step 2 failed — HTTP %d\n", status2);
        return false;
    }

    JsonDocument doc2;
    if (deserializeJson(doc2, resp2) != DeserializationError::Ok) {
        Serial.println("[AUTH] Step 2 JSON parse error");
        return false;
    }

    const char* id_tok  = doc2["idToken"];
    const char* ref_tok = doc2["refreshToken"];
    uint32_t    exp_in  = doc2["expiresIn"] | 3600;

    if (!id_tok || !ref_tok || strlen(id_tok) < 10) {
        Serial.println("[AUTH] Missing idToken or refreshToken in Step 2 response");
        return false;
    }

    _id_token = String(id_tok);
    nvs_set_id_token(_id_token);
    nvs_set_refresh_token(String(ref_tok));
    nvs_set_token_expiry((uint32_t)(millis() / 1000) + exp_in);

    Serial.println("[AUTH] Full two-step auth complete — tokens saved to NVS");
    return true;
}

// ── Public API ────────────────────────────────────────────────
bool auth_begin(const String& device_token) {
    if (nvs_has_refresh_token()) {
        Serial.println("[AUTH] Refresh token found — attempting fast refresh");
        if (_refresh_id_token()) return true;
        // If refresh fails (token may be revoked), fall through to full auth
        Serial.println("[AUTH] Fast refresh failed — falling back to full auth");
    }
    return _full_auth(device_token);
}

bool auth_ensure_valid() {
    uint32_t now_s  = (uint32_t)(millis() / 1000);
    uint32_t expiry = nvs_get_token_expiry();

    if (expiry > 0 && (expiry - now_s) > (uint32_t)TOKEN_REFRESH_HEADROOM_S) {
        // Token still fresh
        _id_token = nvs_get_id_token();
        return true;
    }

    Serial.println("[AUTH] idToken expiring soon — refreshing...");
    return _refresh_id_token();
}

String auth_get_id_token() {
    if (_id_token.length() == 0) _id_token = nvs_get_id_token();
    return _id_token;
}
