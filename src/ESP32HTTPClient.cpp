#include "ESP32HTTPClient.h"

#include <Arduino.h>

ESP32HTTPClient::ESP32HTTPClient(const char* baseUrl) : _baseUrl(baseUrl), _lastStatusCode(0), _contentType("application/json") {
}

void ESP32HTTPClient::setContentType(const char* contentType) {
  _contentType = contentType;
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
