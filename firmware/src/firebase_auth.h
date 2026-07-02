#ifndef FIREBASE_AUTH_H
#define FIREBASE_AUTH_H

#include <Arduino.h>
#include "modem.h"

class FirebaseAuth {
public:
  FirebaseAuth(A7670Modem& modem, const char* apiKey, const char* customToken);
  
  bool begin();
  bool authenticate();
  bool refreshToken();
  
  bool isTokenValid();
  const char* getIdToken();

private:
  A7670Modem& _modem;
  const char* _apiKey;
  const char* _customToken;
  
  String _idToken;
  String _refreshToken;
  unsigned long _tokenExpiryMs;
  unsigned long _tokenAcquiredTimeMs;
  
  bool parseAuthResponse(const String& responseBody);
};

#endif // FIREBASE_AUTH_H
