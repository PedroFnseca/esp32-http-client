#include "ESP32HTTPClient.h"

#include <Arduino.h>

ESP32HTTPClient::ESP32HTTPClient(const char* baseUrl, int port)
    : _baseUrl(baseUrl),
      _port(port),
      _lastStatusCode(0),
      _timeout(60000),
      _maxRetry(1),
      _contentType("application/json"),
      _onSuccessCb(nullptr),
      _onErrorCb(nullptr),
      _onResponseCb(nullptr),
      _observabilityCb(nullptr) {
  _http.setReuse(true);
}

void ESP32HTTPClient::setBaseUrl(const char* baseUrl, int port) {
  end();
  _baseUrl = baseUrl;
  _port = port;
}

void ESP32HTTPClient::setUrl(const char* baseUrl, int port) {
  setBaseUrl(baseUrl, port);
}

void ESP32HTTPClient::setPort(int port) {
  end();
  _port = port;
}

const char* ESP32HTTPClient::getBaseUrl() const {
  return _baseUrl;
}

int ESP32HTTPClient::getPort() const {
  return _port;
}

void ESP32HTTPClient::setTimeout(uint16_t timeoutMs) {
  _timeout = timeoutMs;
}

uint16_t ESP32HTTPClient::getTimeout() const {
  return _timeout;
}

void ESP32HTTPClient::setMaxRetry(int maxRetry) {
  _maxRetry = (maxRetry < 0) ? 0 : maxRetry;
}

int ESP32HTTPClient::getMaxRetry() const {
  return _maxRetry;
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

ESP32HTTPClient& ESP32HTTPClient::cookie(const char* name, const char* value) {
  if (!name || !value) return *this;
  String currentCookies = "";
  for (size_t i = 0; i < _headers.size(); i++) {
    if (strcasecmp(_headers[i].name, "Cookie") == 0) {
      currentCookies = _headers[i].value;
      break;
    }
  }
  if (currentCookies.length() > 0) {
    currentCookies += "; ";
  }
  currentCookies += name;
  currentCookies += "=";
  currentCookies += value;
  setHeader("Cookie", currentCookies.c_str());
  return *this;
}

void ESP32HTTPClient::end() {
  _http.setReuse(false);
  _http.end();
  _http.setReuse(true);
}

int ESP32HTTPClient::getStatusCode() const {
  return _lastStatusCode;
}

String ESP32HTTPClient::errorToString(int code) {
  switch (code) {
    case -1: return "Connection Refused";
    case -2: return "Send Header Failed";
    case -3: return "Send Payload Failed";
    case -4: return "Not Connected";
    case -5: return "Connection Lost";
    case -6: return "No Stream";
    case -7: return "No HTTP Server";
    case -8: return "Too Less RAM";
    case -9: return "Encoding Error";
    case -10: return "Stream Write Error";
    case -11: return "Read Timeout";
    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 204: return "No Content";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 408: return "Request Timeout";
    case 409: return "Conflict";
    case 429: return "Too Many Requests";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    case 504: return "Gateway Timeout";
    case 0: return "Not Executed";
    default:
      if (code < 0) return "Unknown Client Error";
      if (code >= 200 && code < 300) return "Success";
      if (code >= 300 && code < 400) return "Redirection";
      if (code >= 400 && code < 500) return "Client Error";
      if (code >= 500 && code < 600) return "Server Error";
      return "Unknown HTTP Status";
  }
}

String ESP32HTTPClient::getErrorMessage() const {
  return errorToString(_lastStatusCode);
}

bool ESP32HTTPClient::isSuccess() const {
  return _lastStatusCode >= 200 && _lastStatusCode < 300;
}

bool ESP32HTTPClient::hasError() const {
  return _lastStatusCode < 200 || _lastStatusCode >= 400;
}

void ESP32HTTPClient::onSuccess(HttpResponseCallback cb) {
  _onSuccessCb = cb;
}

void ESP32HTTPClient::onError(HttpErrorCallback cb) {
  _onErrorCb = cb;
}

void ESP32HTTPClient::onError(HttpResponseCallback cb) {
  if (cb) {
    _onErrorCb = [cb](int code, const char*) { cb(code); };
  } else {
    _onErrorCb = nullptr;
  }
}

void ESP32HTTPClient::onResponse(HttpResponseCallback cb) {
  _onResponseCb = cb;
}

void ESP32HTTPClient::onObservability(ObservabilityCallback cb) {
  _observabilityCb = cb;
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
