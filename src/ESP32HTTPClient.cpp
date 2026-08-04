#include "ESP32HTTPClient.h"

#include <Arduino.h>

ESP32HTTPClient::ESP32HTTPClient(const char* baseUrl, int port)
    : _baseUrl(baseUrl), _port(port), _lastStatusCode(0), _contentType("application/json") {
  _http.setReuse(true);
}

void ESP32HTTPClient::setContentType(const char* contentType) {
  _contentType = contentType;
}

static size_t base64Encode(const unsigned char* src, size_t len, char* dst, size_t dstLen) {
  static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  size_t olen = 4 * ((len + 2) / 3);
  if (dstLen < olen + 1) return 0;

  size_t i = 0, j = 0;
  for (i = 0; i < len - (len % 3); i += 3) {
    dst[j++] = b64[(src[i] >> 2) & 0x3F];
    dst[j++] = b64[((src[i] & 0x3) << 4) | ((src[i + 1] >> 4) & 0x0F)];
    dst[j++] = b64[((src[i + 1] & 0x0F) << 2) | ((src[i + 2] >> 6) & 0x03)];
    dst[j++] = b64[src[i + 2] & 0x3F];
  }

  if (len % 3 == 1) {
    dst[j++] = b64[(src[i] >> 2) & 0x3F];
    dst[j++] = b64[(src[i] & 0x3) << 4];
    dst[j++] = '=';
    dst[j++] = '=';
  } else if (len % 3 == 2) {
    dst[j++] = b64[(src[i] >> 2) & 0x3F];
    dst[j++] = b64[((src[i] & 0x3) << 4) | ((src[i + 1] >> 4) & 0x0F)];
    dst[j++] = b64[(src[i + 1] & 0x0F) << 2];
    dst[j++] = '=';
  }

  dst[j] = '\0';
  return j;
}

void ESP32HTTPClient::setHeader(const char* name, const char* value) {
  for (size_t i = 0; i < _headers.size(); i++) {
    if (strcasecmp(_headers[i].name, name) == 0) {
      strncpy(_headers[i].value, value, sizeof(_headers[i].value) - 1);
      _headers[i].value[sizeof(_headers[i].value) - 1] = '\0';
      return;
    }
  }
  HttpHeader header;
  strncpy(header.name, name, sizeof(header.name) - 1);
  header.name[sizeof(header.name) - 1] = '\0';
  strncpy(header.value, value, sizeof(header.value) - 1);
  header.value[sizeof(header.value) - 1] = '\0';
  _headers.push_back(header);
}

void ESP32HTTPClient::bearer(const char* token) {
  char authHeader[256];
  snprintf(authHeader, sizeof(authHeader), "Bearer %s", token ? token : "");
  setHeader("Authorization", authHeader);
}

void ESP32HTTPClient::basic(const char* user, const char* password) {
  char creds[128];
  snprintf(creds, sizeof(creds), "%s:%s", user ? user : "", password ? password : "");
  char encoded[192];
  base64Encode((const unsigned char*)creds, strlen(creds), encoded, sizeof(encoded));
  char authHeader[256];
  snprintf(authHeader, sizeof(authHeader), "Basic %s", encoded);
  setHeader("Authorization", authHeader);
}

void ESP32HTTPClient::apiKey(const char* name, const char* key) {
  setHeader(name, key ? key : "");
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
