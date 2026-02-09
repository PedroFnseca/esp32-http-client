#include "ESP32HTTPClient.h"

#include <Arduino.h>

ESP32HTTPClient::ESP32HTTPClient(const char* baseUrl, int port)
    : _baseUrl(baseUrl ? baseUrl : ""), _port(port), _lastStatusCode(0), _contentType("application/json") {
}

void ESP32HTTPClient::setContentType(const char* contentType) {
  _contentType = contentType ? contentType : "application/json";
}

void ESP32HTTPClient::setHeader(const char* name, const char* value) {
  if (!name || !value || !name[0]) return;

  for (auto& header : _headers) {
    if (header.name.equalsIgnoreCase(name)) {
      header.value = value;
      return;
    }
  }

  _headers.push_back({String(name), String(value)});
}

void ESP32HTTPClient::clearHeaders() {
  _headers.clear();
}

RestRequest ESP32HTTPClient::get(const char* path) {
  return RestRequest(this, path, HTTP_GET_METHOD);
}

RestRequest ESP32HTTPClient::post(const char* path) {
  return RestRequest(this, path, HTTP_POST_METHOD);
}

RestRequest ESP32HTTPClient::update(const char* path) {
  return RestRequest(this, path, HTTP_PUT_METHOD);
}

RestRequest ESP32HTTPClient::put(const char* path) {
  return RestRequest(this, path, HTTP_PUT_METHOD);
}

RestRequest ESP32HTTPClient::patch(const char* path) {
  return RestRequest(this, path, HTTP_PATCH_METHOD);
}

RestRequest ESP32HTTPClient::del(const char* path) {
  return RestRequest(this, path, HTTP_DELETE_METHOD);
}
