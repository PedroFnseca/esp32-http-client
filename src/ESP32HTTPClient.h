#ifndef ESP32_HTTP_CLIENT_H
#define ESP32_HTTP_CLIENT_H

#include <Arduino.h>

#include "RestRequest.h"

class ESP32HTTPClient {
  friend class RestRequest;

 public:
  ESP32HTTPClient(const char* baseUrl, int port = 0);

  RestRequest get(const char* path);
  RestRequest post(const char* path);
  RestRequest update(const char* path);
  RestRequest put(const char* path);
  RestRequest patch(const char* path);
  RestRequest del(const char* path);

  void setContentType(const char* contentType);
  void setHeader(const char* name, const char* value);

  int getStatusCode() const {
    return _lastStatusCode;
  }

 private:
  const char* _baseUrl;
  int _port;
  int _lastStatusCode;
  const char* _contentType;
};

#endif
