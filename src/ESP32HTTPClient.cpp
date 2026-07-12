#include "ESP32HTTPClient.h"

#include <Arduino.h>

ESP32HTTPClient::ESP32HTTPClient(const char* baseUrl, int port)
    : _baseUrl(baseUrl), _port(port), _lastStatusCode(0), _contentType("application/json") {
  _http.setReuse(true);
}

void ESP32HTTPClient::setContentType(const char* contentType) {
  _contentType = contentType;
}

void ESP32HTTPClient::setHeader(const char* name, const char* value) {
  HttpHeader header;
  strncpy(header.name, name, sizeof(header.name) - 1);
  header.name[sizeof(header.name) - 1] = '\0';
  strncpy(header.value, value, sizeof(header.value) - 1);
  header.value[sizeof(header.value) - 1] = '\0';
  _headers.push_back(header);
}

void ESP32HTTPClient::end() {
  _http.setReuse(false);
  _http.end();
  _http.setReuse(true);
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
