#include "firebase_auth.h"
#include <ArduinoJson.h>

FirebaseAuth::FirebaseAuth(A7670Modem& modem, const char* apiKey, const char* customToken) : _modem(modem) {
  _apiKey = apiKey;
  _customToken = customToken;
  _tokenExpiryMs = 0;
  _tokenAcquiredTimeMs = 0;
}

bool FirebaseAuth::begin() {
  // Try loading cached refresh token from NVS here if needed, 
  // or fall back to primary authentication.
  return authenticate();
}

bool FirebaseAuth::authenticate() {
  Serial.println("[Auth] Exchanging custom token for Firebase ID token...");
  
  String url = "https://identitytoolkit.googleapis.com/v1/accounts:signInWithCustomToken?key=" + String(_apiKey);
  
  StaticJsonDocument<256> doc;
  doc["token"] = _customToken;
  doc["returnSecureToken"] = true;
  
  String body;
  serializeJson(doc, body);

  HttpResponse res = _modem.httpsRequest("POST", url.c_str(), body.c_str());

  if (res.status == 200) {
    if (parseAuthResponse(res.body)) {
      Serial.println("[Auth] Initial custom token authentication succeeded.");
      return true;
    }
  } else {
    Serial.printf("[Auth] Exchange failed with code %d. Response: %s\n", res.status, res.body.c_str());
  }
  
  return false;
}

bool FirebaseAuth::refreshToken() {
  if (_refreshToken.length() == 0) {
    return authenticate();
  }

  Serial.println("[Auth] Refreshing Firebase authentication token...");
  String url = "https://securetoken.googleapis.com/v1/token?key=" + String(_apiKey);

  StaticJsonDocument<256> doc;
  doc["grant_type"] = "refresh_token";
  doc["refresh_token"] = _refreshToken;

  String body;
  serializeJson(doc, body);

  HttpResponse res = _modem.httpsRequest("POST", url.c_str(), body.c_str());

  if (res.status == 200) {
    StaticJsonDocument<512> responseDoc;
    DeserializationError error = deserializeJson(responseDoc, res.body);
    
    if (!error) {
      _idToken = responseDoc["id_token"].as<String>();
      _refreshToken = responseDoc["refresh_token"].as<String>();
      
      long expiresInSec = responseDoc["expires_in"].as<long>();
      _tokenExpiryMs = expiresInSec * 1000;
      _tokenAcquiredTimeMs = millis();
      
      Serial.println("[Auth] Token refreshed successfully.");
      return true;
    }
  } else {
    Serial.printf("[Auth] Token refresh failed with code %d. Attempting full auth...\n", res.status);
    return authenticate();
  }

  return false;
}

bool FirebaseAuth::isTokenValid() {
  if (_idToken.length() == 0) return false;
  
  // Safe margin of 5 minutes before absolute expiry
  unsigned long safetyMarginMs = 5 * 60 * 1000;
  return (millis() - _tokenAcquiredTimeMs < (_tokenExpiryMs - safetyMarginMs));
}

const char* FirebaseAuth::getIdToken() {
  return _idToken.c_str();
}

bool FirebaseAuth::parseAuthResponse(const String& responseBody) {
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, responseBody);

  if (error) {
    Serial.printf("[Auth] JSON Parsing error: %s\n", error.c_str());
    return false;
  }

  if (doc.containsKey("idToken") && doc.containsKey("refreshToken")) {
    _idToken = doc["idToken"].as<String>();
    _refreshToken = doc["refreshToken"].as<String>();
    
    long expiresInSec = doc["expiresIn"].as<long>();
    _tokenExpiryMs = expiresInSec * 1000;
    _tokenAcquiredTimeMs = millis();
    return true;
  }

  return false;
}
