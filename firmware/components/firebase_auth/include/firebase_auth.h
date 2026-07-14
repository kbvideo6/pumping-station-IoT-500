#ifndef FIREBASE_AUTH_H
#define FIREBASE_AUTH_H

#include <stdint.h>
#include <string>
#include "modem.h"

class FirebaseAuth {
public:
  FirebaseAuth(A7670Modem& modem, const char* apiKey, const char* customToken, const char* stationId);
  
  bool begin();
  bool authenticate();
  bool refreshToken();
  
  bool isTokenValid();
  const char* getIdToken();

private:
  A7670Modem& _modem;
  const char* _apiKey;
  const char* _customToken;
  const char* _stationId;
  
  std::string _idToken;
  std::string _refreshToken;
  uint64_t _tokenExpiryMs;
  uint64_t _tokenAcquiredTimeMs;
  
  bool parseAuthResponse(const std::string& responseBody);
  void loadCachedCredentials();
  void saveCachedCredentials();
};

#endif // FIREBASE_AUTH_H
