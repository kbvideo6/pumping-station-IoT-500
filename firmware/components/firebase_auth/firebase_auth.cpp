#include "firebase_auth.h"
#include "config.h"
#include "cJSON.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "FirebaseAuth";

FirebaseAuth::FirebaseAuth(A7670Modem& modem, const char* apiKey, const char* customToken, const char* stationId) : _modem(modem) {
  _apiKey = apiKey;
  _customToken = customToken;
  _stationId = stationId;
  _tokenExpiryMs = 0;
  _tokenAcquiredTimeMs = 0;
}

bool FirebaseAuth::begin() {
  loadCachedCredentials();
  if (!_refreshToken.empty()) {
    ESP_LOGI(TAG, "Found cached refresh token. Attempting token renew...");
    if (refreshToken()) {
      return true;
    }
    ESP_LOGW(TAG, "Cached refresh token renew failed. Falling back to full custom token auth.");
  }
  return authenticate();
}

bool FirebaseAuth::authenticate() {
  ESP_LOGI(TAG, "Exchanging device token for Firebase custom token...");

  std::string tokenExchangeUrl = "https://europe-west1-" + std::string(FIREBASE_PROJECT_ID) + ".cloudfunctions.net/getDeviceCustomToken";

  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "stationId", _stationId);
  cJSON_AddStringToObject(root, "deviceToken", _customToken);
  char *reqBody = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  HttpResponse tokenExchangeRes = _modem.httpsRequest("POST", tokenExchangeUrl.c_str(), reqBody);
  free(reqBody);

  if (tokenExchangeRes.status != 200) {
    ESP_LOGE(TAG, "Device token exchange failed with code %d. Response: %s", tokenExchangeRes.status, tokenExchangeRes.body.c_str());
    return false;
  }

  cJSON *exchangeJson = cJSON_Parse(tokenExchangeRes.body.c_str());
  if (!exchangeJson) {
    ESP_LOGE(TAG, "Failed to parse token exchange response JSON: %s", tokenExchangeRes.body.c_str());
    return false;
  }

  cJSON *firebaseCustomTokenItem = cJSON_GetObjectItemCaseSensitive(exchangeJson, "customToken");
  if (!firebaseCustomTokenItem || !cJSON_IsString(firebaseCustomTokenItem)) {
    ESP_LOGE(TAG, "Token exchange response does not contain customToken string.");
    cJSON_Delete(exchangeJson);
    return false;
  }

  std::string firebaseCustomToken = firebaseCustomTokenItem->valuestring;
  cJSON_Delete(exchangeJson);

  ESP_LOGI(TAG, "Exchanging custom token for Firebase ID token...");
  
  std::string url = "https://identitytoolkit.googleapis.com/v1/accounts:signInWithCustomToken?key=" + std::string(_apiKey);
  
  cJSON *authRoot = cJSON_CreateObject();
  cJSON_AddStringToObject(authRoot, "token", firebaseCustomToken.c_str());
  cJSON_AddBoolToObject(authRoot, "returnSecureToken", true);
  char *authBody = cJSON_PrintUnformatted(authRoot);
  cJSON_Delete(authRoot);

  HttpResponse res = _modem.httpsRequest("POST", url.c_str(), authBody);
  free(authBody);

  if (res.status == 200) {
    if (parseAuthResponse(res.body)) {
      ESP_LOGI(TAG, "Initial custom token authentication succeeded.");
      saveCachedCredentials();
      return true;
    }
  } else {
    ESP_LOGE(TAG, "Exchange failed with code %d. Response: %s", res.status, res.body.c_str());
  }
  
  return false;
}

bool FirebaseAuth::refreshToken() {
  if (_refreshToken.empty()) {
    return authenticate();
  }

  ESP_LOGI(TAG, "Refreshing Firebase authentication token...");
  std::string url = "https://securetoken.googleapis.com/v1/token?key=" + std::string(_apiKey);

  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "grant_type", "refresh_token");
  cJSON_AddStringToObject(root, "refresh_token", _refreshToken.c_str());
  char *body = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  HttpResponse res = _modem.httpsRequest("POST", url.c_str(), body);
  free(body);

  if (res.status == 200) {
    cJSON *json = cJSON_Parse(res.body.c_str());
    if (json) {
      cJSON *idTokenItem = cJSON_GetObjectItemCaseSensitive(json, "id_token");
      cJSON *refreshTokenItem = cJSON_GetObjectItemCaseSensitive(json, "refresh_token");
      cJSON *expiresInItem = cJSON_GetObjectItemCaseSensitive(json, "expires_in");

      if (idTokenItem && cJSON_IsString(idTokenItem) && refreshTokenItem && cJSON_IsString(refreshTokenItem)) {
        _idToken = idTokenItem->valuestring;
        _refreshToken = refreshTokenItem->valuestring;
        
        long expiresInSec = 3600;
        if (expiresInItem && cJSON_IsString(expiresInItem)) {
          expiresInSec = atol(expiresInItem->valuestring);
        } else if (expiresInItem && cJSON_IsNumber(expiresInItem)) {
          expiresInSec = (long)expiresInItem->valuedouble;
        }
        
        _tokenExpiryMs = expiresInSec * 1000;
        _tokenAcquiredTimeMs = esp_timer_get_time() / 1000;
        
        ESP_LOGI(TAG, "Token refreshed successfully.");
        cJSON_Delete(json);
        saveCachedCredentials();
        return true;
      }
      cJSON_Delete(json);
    }
  } else {
    ESP_LOGE(TAG, "Token refresh failed with code %d. Attempting full auth...", res.status);
    return authenticate();
  }

  return false;
}

bool FirebaseAuth::isTokenValid() {
  if (_idToken.empty()) return false;
  
  unsigned long safetyMarginMs = 5 * 60 * 1000;
  uint64_t nowMs = esp_timer_get_time() / 1000;
  return (nowMs - _tokenAcquiredTimeMs < (_tokenExpiryMs - safetyMarginMs));
}

const char* FirebaseAuth::getIdToken() {
  return _idToken.c_str();
}

bool FirebaseAuth::parseAuthResponse(const std::string& responseBody) {
  cJSON *json = cJSON_Parse(responseBody.c_str());
  if (!json) {
    ESP_LOGE(TAG, "JSON Parsing error: %s", responseBody.c_str());
    return false;
  }

  cJSON *idTokenItem = cJSON_GetObjectItemCaseSensitive(json, "idToken");
  cJSON *refreshTokenItem = cJSON_GetObjectItemCaseSensitive(json, "refreshToken");
  cJSON *expiresInItem = cJSON_GetObjectItemCaseSensitive(json, "expiresIn");

  if (idTokenItem && cJSON_IsString(idTokenItem) && refreshTokenItem && cJSON_IsString(refreshTokenItem)) {
    _idToken = idTokenItem->valuestring;
    _refreshToken = refreshTokenItem->valuestring;
    
    long expiresInSec = 3600;
    if (expiresInItem && cJSON_IsString(expiresInItem)) {
      expiresInSec = atol(expiresInItem->valuestring);
    } else if (expiresInItem && cJSON_IsNumber(expiresInItem)) {
      expiresInSec = (long)expiresInItem->valuedouble;
    }

    _tokenExpiryMs = expiresInSec * 1000;
    _tokenAcquiredTimeMs = esp_timer_get_time() / 1000;
    cJSON_Delete(json);
    return true;
  }

  cJSON_Delete(json);
  return false;
}

void FirebaseAuth::loadCachedCredentials() {
  nvs_handle_t handle;
  esp_err_t err = nvs_open("fb_auth", NVS_READONLY, &handle);
  if (err == ESP_OK) {
    char buf[512];
    size_t len = sizeof(buf);
    if (nvs_get_str(handle, "refresh_token", buf, &len) == ESP_OK) {
      _refreshToken = buf;
    }
    nvs_close(handle);
  }
}

void FirebaseAuth::saveCachedCredentials() {
  nvs_handle_t handle;
  esp_err_t err = nvs_open("fb_auth", NVS_READWRITE, &handle);
  if (err == ESP_OK) {
    nvs_set_str(handle, "refresh_token", _refreshToken.c_str());
    nvs_commit(handle);
    nvs_close(handle);
  }
}
