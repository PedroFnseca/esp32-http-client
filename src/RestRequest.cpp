#include "RestRequest.h"

#include <HTTPClient.h>

#include "BufferedStreamReader.h"
#include "ESP32HTTPClient.h"

static void skipWhitespace(BufferedStreamReader& r) {
  while (r.available()) {
    char c = (char)r.peek();
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      r.read();
    } else {
      break;
    }
  }
}

static void skipValue(BufferedStreamReader& r) {
  skipWhitespace(r);
  char c = (char)r.peek();

  if (c == '{' || c == '[') {
    char opening = (char)r.read();
    char closing = (opening == '{') ? '}' : ']';
    int  depth   = 1;
    bool inStr   = false;
    bool esc     = false;

    while (r.available() && depth > 0) {
      char x = (char)r.read();
      if (inStr) {
        if (x == '\\' && !esc) {
          esc = true;
        } else {
          if (x == '"' && !esc) inStr = false;
          esc = false;
        }
      } else {
        if (x == '"') {
          inStr = true;
        } else if (x == opening) {
          depth++;
        } else if (x == closing) {
          depth--;
        }
      }
    }
  } else if (c == '"') {
    r.read();
    bool esc = false;
    while (r.available()) {
      char x = (char)r.read();
      if (esc) {
        esc = false;
      } else if (x == '\\') {
        esc = true;
      } else if (x == '"') {
        break;
      }
    }
  } else {
    while (r.available()) {
      char x = (char)r.peek();
      if (x == ',' || x == '}' || x == ']' || x == ' ' || x == '\t' || x == '\n' || x == '\r') {
        break;
      }
      r.read();
    }
  }
}

static void readRawPrimitive(BufferedStreamReader& r, char* out, size_t cap) {
  size_t n = 0;
  auto put = [&](char c) { if (n < cap - 1) out[n++] = c; };

  if (r.peek() == '"') {
    put((char)r.read());
    bool escaped = false;
    while (r.available()) {
      char c = (char)r.read();
      put(c);
      if (escaped) escaped = false;
      else if (c == '\\') escaped = true;
      else if (c == '"') break;
    }
  } else {
    while (r.available()) {
      char c = (char)r.peek();
      if (c == ',' || c == '}' || c == ']' || isspace((unsigned char)c)) break;
      put((char)r.read());
    }
  }
  out[n] = '\0';
}

static void readStringIntoBuffer(BufferedStreamReader& r, char* buffer, size_t maxLen) {
  r.read();
  size_t idx = 0;
  bool   esc = false;

  while (r.available()) {
    char c = (char)r.read();
    if (esc) {
      if (idx < maxLen - 1) buffer[idx++] = c;
      esc = false;
    } else if (c == '\\') {
      esc = true;
    } else if (c == '"') {
      break;
    } else {
      if (idx < maxLen - 1) buffer[idx++] = c;
    }
  }
  buffer[idx] = 0;
}

static inline bool isCollecting(const RestFilter* f) {
  return f && f->active && f->targetCollector;
}

static inline bool isDescendantPath(const char* key, const char* path) {
  const size_t len = strlen(path);
  return strncmp(key, path, len) == 0 && key[len] == '.';
}

static ResponseBinding* findBinding(std::vector<ResponseBinding>& bindings, const char* path) {
  for (auto& binding : bindings) {
    if (strcmp(binding.key, path) == 0) return &binding;
  }
  return nullptr;
}

static bool hasChildBinding(const std::vector<ResponseBinding>& bindings, const FilterCapture* capture, const char* path) {
  for (const auto& binding : bindings) {
    if (isDescendantPath(binding.key, path)) return true;
  }
  return capture && isDescendantPath(capture->key, path);
}

static void joinPath(char* out, size_t cap, const char* base, const char* key) {
  if (base[0] == '\0') {
    snprintf(out, cap, "%s", key);
  } else {
    snprintf(out, cap, "%s.%s", base, key);
  }
}

RestRequest::RestRequest(ESP32HTTPClient* client, const char* path, HttpMethod method)
    : _client(client),
      _path(path),
      _method(method),
      _executed(false),
      _timeout(0),
      _maxRetry(-1),
      _onSuccessCb(nullptr),
      _onErrorCb(nullptr),
      _onResponseCb(nullptr) {
  _keyStorage.reserve(128);
}

RestRequest::RestRequest(RestRequest&& other)
    : _client(other._client),
      _path(other._path),
      _method(other._method),
      _executed(other._executed),
      _timeout(other._timeout),
      _maxRetry(other._maxRetry),
      _filter(std::move(other._filter)),
      _onSuccessCb(std::move(other._onSuccessCb)),
      _onErrorCb(std::move(other._onErrorCb)),
      _onResponseCb(std::move(other._onResponseCb)),
      _rawBody(std::move(other._rawBody)),
      _keyStorage(std::move(other._keyStorage)),
      _pathParams(std::move(other._pathParams)),
      _queryParams(std::move(other._queryParams)),
      _bodyParams(std::move(other._bodyParams)),
      _responseBindings(std::move(other._responseBindings)),
      _headerBindings(std::move(other._headerBindings)) {
  other._executed = true;
}

RestRequest::~RestRequest() {
  if (!_executed) {
    execute();
  }
}

RestRequest& RestRequest::timeout(uint16_t timeoutMs) {
  _timeout = timeoutMs;
  return *this;
}

RestRequest& RestRequest::maxRetry(int maxRetry) {
  _maxRetry = (maxRetry < 0) ? 0 : maxRetry;
  return *this;
}

RestRequest& RestRequest::retry(int maxRetry) {
  return this->maxRetry(maxRetry);
}

RestRequest& RestRequest::onSuccess(HttpResponseCallback cb) {
  _onSuccessCb = cb;
  return *this;
}

RestRequest& RestRequest::onError(HttpErrorCallback cb) {
  _onErrorCb = cb;
  return *this;
}

RestRequest& RestRequest::onError(HttpResponseCallback cb) {
  if (cb) {
    _onErrorCb = [cb](int code, const char*) { cb(code); };
  } else {
    _onErrorCb = nullptr;
  }
  return *this;
}

RestRequest& RestRequest::onResponse(HttpResponseCallback cb) {
  _onResponseCb = cb;
  return *this;
}

RestRequest& RestRequest::getBody(const char* key, int* target) {
  _responseBindings.push_back({key, target, TYPE_INT, 0});
  return *this;
}

RestRequest& RestRequest::getBody(const char* key, float* target) {
  _responseBindings.push_back({key, target, TYPE_FLOAT, 0});
  return *this;
}

RestRequest& RestRequest::getBody(const char* key, double* target) {
  _responseBindings.push_back({key, target, TYPE_DOUBLE, 0});
  return *this;
}

RestRequest& RestRequest::getBody(const char* key, bool* target) {
  _responseBindings.push_back({key, target, TYPE_BOOL, 0});
  return *this;
}

RestRequest& RestRequest::getBody(const char* key, char* target, size_t maxLength) {
  _responseBindings.push_back({key, target, TYPE_STRING, maxLength});
  return *this;
}

RestRequest& RestRequest::getBody(const char* key, long* target) {
  _responseBindings.push_back({key, target, TYPE_LONG, 0});
  return *this;
}

RestRequest& RestRequest::getBody(const char* key, String* target) {
  _responseBindings.push_back({key, target, TYPE_ARDUINO_STRING, 0});
  return *this;
}

RestRequest& RestRequest::getHeader(const char* name, int* target) {
  _headerBindings.push_back({name, target, TYPE_INT, 0});
  return *this;
}

RestRequest& RestRequest::getHeader(const char* name, float* target) {
  _headerBindings.push_back({name, target, TYPE_FLOAT, 0});
  return *this;
}

RestRequest& RestRequest::getHeader(const char* name, double* target) {
  _headerBindings.push_back({name, target, TYPE_DOUBLE, 0});
  return *this;
}

RestRequest& RestRequest::getHeader(const char* name, bool* target) {
  _headerBindings.push_back({name, target, TYPE_BOOL, 0});
  return *this;
}

RestRequest& RestRequest::getHeader(const char* name, char* target, size_t maxLength) {
  _headerBindings.push_back({name, target, TYPE_STRING, maxLength});
  return *this;
}

RestRequest& RestRequest::getHeader(const char* name, long* target) {
  _headerBindings.push_back({name, target, TYPE_LONG, 0});
  return *this;
}

RestRequest& RestRequest::getHeader(const char* name, String* target) {
  _headerBindings.push_back({name, target, TYPE_ARDUINO_STRING, 0});
  return *this;
}

void RestRequest::execute() {
  _executed = true;

  if (!_client) return;

  HTTPClient& http = _client->_http;

  uint16_t effectiveTimeout = (_timeout > 0) ? _timeout : _client->_timeout;
  if (effectiveTimeout > 0) {
    http.setTimeout(effectiveTimeout);
  }

  String urlBase;
  urlBase.reserve(128);
  urlBase = _client->_baseUrl;

  if (_client->_port > 0) {
    int protoEnd = urlBase.indexOf("://");
    if (protoEnd != -1) {
      int pathStart = urlBase.indexOf('/', protoEnd + 3);
      if (pathStart != -1) {
        urlBase = urlBase.substring(0, pathStart) + ":" + String(_client->_port) + urlBase.substring(pathStart);
      } else {
        urlBase = urlBase + ":" + String(_client->_port);
      }
    }
  }

  String resolvedPath = _path ? _path : "";
  for (const auto& param : _pathParams) {
    if (!param.key) continue;
    String placeholder;
    if (param.key[0] == '{') {
      placeholder = param.key;
    } else {
      placeholder = "{" + String(param.key) + "}";
    }
    resolvedPath.replace(placeholder, param.value.c_str());
  }

  String url;
  url.reserve(256);
  url = urlBase;
  url += resolvedPath;

  if (!_queryParams.empty()) {
    url += "?";
    for (size_t i = 0; i < _queryParams.size(); i++) {
      url += _queryParams[i].key;
      url += "=";
      url += _queryParams[i].value.c_str();
      if (i < _queryParams.size() - 1) url += "&";
    }
  }

  std::vector<const char*> headerKeys;
  if (!_headerBindings.empty()) {
    headerKeys.reserve(_headerBindings.size());
    for (const auto& binding : _headerBindings) {
      if (binding.key && binding.key[0] != '\0') {
        headerKeys.push_back(binding.key);
      }
    }
  }

  bool connected = _client->beginConnection(url, effectiveTimeout);

  if (connected && !headerKeys.empty()) {
    http.collectHeaders(headerKeys.data(), headerKeys.size());
  }

  if (connected) {
    for (const auto& header : _client->_headers) {
      http.addHeader(header.name, header.value);
    }
  }

  String payload = "";
  if (!_rawBody.isEmpty()) {
    payload = _rawBody;
    if (connected) http.addHeader("Content-Type", _client->_contentType);
  } else if (!_bodyParams.empty()) {
    payload.reserve(_bodyParams.size() * 64);
    if (connected) http.addHeader("Content-Type", _client->_contentType);
    payload += "{";
    for (size_t i = 0; i < _bodyParams.size(); i++) {
      payload += "\"";
      payload += _bodyParams[i].key;
      payload += "\":";

      if (_bodyParams[i].quoteValue) {
        payload += "\"";
        payload += _bodyParams[i].value.c_str();
        payload += "\"";
      } else {
        payload += _bodyParams[i].value.c_str();
      }

      if (i < _bodyParams.size() - 1) payload += ",";
    }
    payload += "}";
  }

  int code = connected ? 0 : _client->_lastStatusCode;
  int retries = 0;
  int maxRetries = (_maxRetry >= 0) ? _maxRetry : _client->_maxRetry;
  int maxAttempts = 1 + maxRetries;

  uint32_t freeHeapBefore = 0;
  unsigned long startTime = 0;
  if (_client->_observabilityCb) {
    freeHeapBefore = ESP.getFreeHeap();
    startTime = millis();
  }

  while (connected && retries < maxAttempts) {
    switch (_method) {
      case HTTP_GET_METHOD:
        code = http.GET();
        break;
      case HTTP_POST_METHOD:
        code = http.POST(payload);
        break;
      case HTTP_PUT_METHOD:
        code = http.PUT(payload);
        break;
      case HTTP_PATCH_METHOD:
        code = http.PATCH(payload);
        break;
      case HTTP_DELETE_METHOD:
        code = http.sendRequest("DELETE", payload);
        break;
    }

    if (code < 0 && retries < maxRetries) {
      http.end();
      _client->beginConnection(url, effectiveTimeout);
      if (!headerKeys.empty()) {
        http.collectHeaders(headerKeys.data(), headerKeys.size());
      }
      for (const auto& header : _client->_headers) {
        http.addHeader(header.name, header.value);
      }
      if (!_rawBody.isEmpty() || !_bodyParams.empty()) {
        http.addHeader("Content-Type", _client->_contentType);
      }
      retries++;
    } else {
      break;
    }
  }

  unsigned long ttfbTime = 0;
  if (_client->_observabilityCb) {
    ttfbTime = millis();
  }

  _client->_lastStatusCode = code;

  if (code > 0) {
    for (const auto& binding : _headerBindings) {
      if (binding.key && binding.target && http.hasHeader(binding.key)) {
        String val = http.header(binding.key);
        if (binding.type == TYPE_ARDUINO_STRING) {
          *((String*)binding.target) = val;
        } else if (binding.type == TYPE_STRING) {
          char* dst = (char*)binding.target;
          if (binding.size > 0) {
            strncpy(dst, val.c_str(), binding.size - 1);
            dst[binding.size - 1] = '\0';
          }
        } else if (binding.type == TYPE_INT) {
          *(int*)binding.target = atoi(val.c_str());
        } else if (binding.type == TYPE_LONG) {
          *(long*)binding.target = atol(val.c_str());
        } else if (binding.type == TYPE_FLOAT) {
          *(float*)binding.target = strtof(val.c_str(), nullptr);
        } else if (binding.type == TYPE_DOUBLE) {
          *(double*)binding.target = strtod(val.c_str(), nullptr);
        } else if (binding.type == TYPE_BOOL) {
          *(bool*)binding.target = (val.equalsIgnoreCase("true") || val == "1");
        }
      }
    }

    if (http.getSize() > 0 || http.getStreamPtr()) {
      bool isChunked = (http.getSize() == -1);
      BufferedStreamReader reader(http.getStreamPtr(), isChunked);
      parseResponse(reader);
    }
  }

  http.end();

  if (_onResponseCb) {
    _onResponseCb(code);
  }
  if (_client->_onResponseCb) {
    _client->_onResponseCb(code);
  }

  if (code >= 200 && code < 300) {
    if (_onSuccessCb) {
      _onSuccessCb(code);
    }
    if (_client->_onSuccessCb) {
      _client->_onSuccessCb(code);
    }
  } else {
    String errMsg = _client->getErrorMessage();
    if (_onErrorCb) {
      _onErrorCb(code, errMsg.c_str());
    }
    if (_client->_onErrorCb) {
      _client->_onErrorCb(code, errMsg.c_str());
    }
  }

  if (_client->_observabilityCb) {
    ObservabilityMetrics metrics;
    metrics.totalTimeMs = millis() - startTime;
    metrics.ttfbMs = ttfbTime > 0 ? (ttfbTime - startTime) : 0;
    metrics.txBytes = url.length() + payload.length() + 150; 
    metrics.rxBytes = http.getSize() > 0 ? http.getSize() : 0;
    metrics.retries = retries;
    metrics.freeHeapBefore = freeHeapBefore;
    metrics.freeHeapAfter = ESP.getFreeHeap();
    _client->_observabilityCb(metrics);
  }
}

void RestRequest::parseResponse(BufferedStreamReader& r) {
  parseJsonWithBindings(r, _responseBindings, &_filter);
}

void RestRequest::parseJsonWithBindings(BufferedStreamReader& r, std::vector<ResponseBinding>& bindings, const RestFilter* filter) {
  if (bindings.empty() && !isCollecting(filter)) return;

  const RestFilter* activeFilter = isCollecting(filter) ? filter : nullptr;

  skipWhitespace(r);
  const char opening = (char)r.read();
  if (opening != '{' && opening != '[') return;

  ResponseBinding* root = findBinding(bindings, "");
  if (root && root->type == TYPE_ARDUINO_STRING) {
    readRawJson(r, (String*)root->target, opening);
  } else if (opening == '{') {
    parseObjectWithBindings(r, "", bindings, nullptr, activeFilter);
  } else {
    parseArrayWithBindings(r, "", bindings, nullptr, activeFilter);
  }
}

void RestRequest::readRawJson(BufferedStreamReader& r, String* target, char openingBrace) {
  int  depth    = 1;
  bool inString = false;
  bool escaped  = false;

  target->reserve(256);
  *target = openingBrace;

  while (r.available() && depth > 0) {
    char c = (char)r.read();
    *target += c;

    if (inString) {
      if (c == '\\' && !escaped) {
        escaped = true;
      } else {
        if (c == '"' && !escaped) inString = false;
        escaped = false;
      }
    } else {
      if (c == '"') {
        inString = true;
      } else if (c == '{' || c == '[') {
        depth++;
      } else if (c == '}' || c == ']') {
        depth--;
      }
    }
  }
}

void RestRequest::parsePrimitiveWithBinding(BufferedStreamReader& r, ResponseBinding* match) {
  char valueBuffer[128];
  char nextChar = (char)r.peek();

  if (nextChar == 'n') {
    char nBuf[8];
    size_t nIdx = 0;
    while (r.available()) {
      char b = (char)r.peek();
      if (isalpha((unsigned char)b)) {
        char x = (char)r.read();
        if (nIdx < sizeof(nBuf) - 1) nBuf[nIdx++] = x;
      } else {
        break;
      }
    }
    nBuf[nIdx] = 0;

    if (match->type == TYPE_STRING) {
      if (match->size > 0) ((char*)match->target)[0] = 0;
    } else if (match->type == TYPE_ARDUINO_STRING) {
      *((String*)match->target) = "";
    } else if (match->type == TYPE_INT) {
      *(int*)match->target = 0;
    } else if (match->type == TYPE_FLOAT) {
      *(float*)match->target = 0.0f;
    } else if (match->type == TYPE_DOUBLE) {
      *(double*)match->target = 0.0;
    } else if (match->type == TYPE_LONG) {
      *(long*)match->target = 0L;
    } else if (match->type == TYPE_BOOL) {
      *(bool*)match->target = false;
    }
    return;
  }

  if (nextChar == '"') {
    if (match->type == TYPE_STRING) {
      readStringIntoBuffer(r, (char*)match->target, match->size);
    } else if (match->type == TYPE_ARDUINO_STRING) {
      char tmp[256];
      readStringIntoBuffer(r, tmp, sizeof(tmp));
      *((String*)match->target) = tmp;
    } else {
      readStringIntoBuffer(r, valueBuffer, sizeof(valueBuffer));
      if (match->type == TYPE_INT)
        *(int*)match->target = atoi(valueBuffer);
      else if (match->type == TYPE_FLOAT)
        *(float*)match->target = strtof(valueBuffer, nullptr);
    }
  } else if (nextChar == 't' || nextChar == 'f') {
    size_t bIdx = 0;
    while (r.available()) {
      char b = (char)r.peek();
      if (isalpha((unsigned char)b)) {
        char x = (char)r.read();
        if (bIdx < 10) valueBuffer[bIdx++] = x;
      } else {
        break;
      }
    }
    valueBuffer[bIdx] = 0;
    bool bVal = (strcmp(valueBuffer, "true") == 0);
    if (match->type == TYPE_BOOL) *(bool*)match->target = bVal;
    if (match->type == TYPE_ARDUINO_STRING) *((String*)match->target) = valueBuffer;
  } else {
    size_t nIdx = 0;
    while (r.available()) {
      char b = (char)r.peek();
      if (isdigit((unsigned char)b) || b == '.' || b == '-' || b == '+' || b == 'e' || b == 'E') {
        char x = (char)r.read();
        if (nIdx < 63) valueBuffer[nIdx++] = x;
      } else {
        break;
      }
    }
    valueBuffer[nIdx] = 0;

    if (match->type == TYPE_INT)
      *(int*)match->target = atoi(valueBuffer);
    else if (match->type == TYPE_FLOAT)
      *(float*)match->target = strtof(valueBuffer, nullptr);
    else if (match->type == TYPE_DOUBLE)
      *(double*)match->target = strtod(valueBuffer, nullptr);
    else if (match->type == TYPE_LONG)
      *(long*)match->target = atol(valueBuffer);
    else if (match->type == TYPE_ARDUINO_STRING)
      *((String*)match->target) = valueBuffer;
  }
}

static void parsePrimitiveValue(BufferedStreamReader& r, const char* fullPath, std::vector<ResponseBinding>& bindings, FilterCapture* filterCapture) {
  const char firstChar = (char)r.peek();
  ResponseBinding* match = findBinding(bindings, fullPath);
  const bool isCapture = filterCapture && strcmp(fullPath, filterCapture->key) == 0;

  auto capture = [&](BufferedStreamReader& source) {
    ResponseBinding captureBinding = filterCapture->binding();
    RestRequest::parsePrimitiveWithBinding(source, &captureBinding);
    filterCapture->matched = true;
  };

  if (!isCapture) {
    if (match) {
      RestRequest::parsePrimitiveWithBinding(r, match);
    } else {
      skipValue(r);
    }
  } else if (match) {
    char raw[256];
    readRawPrimitive(r, raw, sizeof(raw));

    if (filterCapture->accepts(firstChar)) {
      BufferedStreamReader forCapture(raw);
      capture(forCapture);
    }
    BufferedStreamReader forBinding(raw);
    RestRequest::parsePrimitiveWithBinding(forBinding, match);
  } else if (filterCapture->accepts(firstChar)) {
    capture(r);
  } else {
    skipValue(r);
  }
}

static void parseContainerValue(BufferedStreamReader& r, const char* fullPath, std::vector<ResponseBinding>& bindings, FilterCapture* filterCapture, const RestFilter* filter) {
  const char opening = (char)r.peek();
  ResponseBinding* match = findBinding(bindings, fullPath);

  const bool filtering     = isCollecting(filter);
  const bool isTargetPath  = filtering && strcmp(fullPath, filter->targetPath) == 0;
  const bool leadsToTarget = filtering && isDescendantPath(filter->targetPath, fullPath);
  const bool mustDescend   = leadsToTarget || hasChildBinding(bindings, filterCapture, fullPath);

  if (match && match->type == TYPE_ARDUINO_STRING) {
    r.read();
    RestRequest::readRawJson(r, (String*)match->target, opening);
  } else if (isTargetPath) {
    r.read();
    if (opening == '[') {
      RestRequest::parseArrayWithBindings(r, fullPath, bindings, filterCapture, filter);
    } else {
      filter->targetCollector(r, *filter);
    }
  } else if (mustDescend) {
    r.read();
    if (opening == '{') {
      RestRequest::parseObjectWithBindings(r, fullPath, bindings, filterCapture, filter);
    } else {
      RestRequest::parseArrayWithBindings(r, fullPath, bindings, filterCapture, filter);
    }
  } else {
    skipValue(r);
  }
}

void RestRequest::parseValueWithBindings(BufferedStreamReader& r, const char* fullPath, std::vector<ResponseBinding>& bindings, FilterCapture* filterCapture, const RestFilter* filter) {
  const char nextChar = (char)r.peek();
  if (nextChar == '{' || nextChar == '[') {
    parseContainerValue(r, fullPath, bindings, filterCapture, filter);
  } else {
    parsePrimitiveValue(r, fullPath, bindings, filterCapture);
  }
}

void RestRequest::parseObjectWithBindings(BufferedStreamReader& r, const char* basePath, std::vector<ResponseBinding>& bindings, FilterCapture* filterCapture, const RestFilter* filter) {
  char keyBuffer[64];
  char fullPath[128];

  while (r.available()) {
    skipWhitespace(r);
    char c = (char)r.read();

    if (c == '}') break;
    if (c != '"') continue;

    size_t kIdx = 0;
    while (r.available()) {
      char k = (char)r.read();
      if (k == '"') break;
      if (kIdx < sizeof(keyBuffer) - 1) keyBuffer[kIdx++] = k;
    }
    keyBuffer[kIdx] = 0;

    skipWhitespace(r);
    if (r.read() != ':') continue;
    skipWhitespace(r);

    joinPath(fullPath, sizeof(fullPath), basePath, keyBuffer);
    parseValueWithBindings(r, fullPath, bindings, filterCapture, filter);
    
    skipWhitespace(r);
    if (r.peek() == ',') r.read();
  }
}

void RestRequest::parseArrayWithBindings(BufferedStreamReader& r, const char* basePath, std::vector<ResponseBinding>& bindings, FilterCapture* filterCapture, const RestFilter* filter) {
  char fullPath[128];
  const bool isTarget = isCollecting(filter) && strcmp(basePath, filter->targetPath) == 0;

  for (int index = 0; r.available(); index++) {
    skipWhitespace(r);
    const char nextChar = (char)r.peek();

    if (nextChar == ']') {
      r.read();
      break;
    }

    if (isTarget) {
      if (nextChar == '{') {
        r.read();
        filter->targetCollector(r, *filter);
      } else {
        skipValue(r);
      }
    } else {
      char indexKey[12];
      snprintf(indexKey, sizeof(indexKey), "%d", index);
      joinPath(fullPath, sizeof(fullPath), basePath, indexKey);
      parseValueWithBindings(r, fullPath, bindings, filterCapture, filter);
    }

    skipWhitespace(r);
    if (r.peek() == ',') r.read();
  }
}

RestRequest& RestRequest::filterArray(const char* key, const char* arrayPath) {
  _filter.active = true;
  strncpy(_filter.path, key ? key : "", sizeof(_filter.path) - 1);
  _filter.path[sizeof(_filter.path) - 1] = '\0';
  strncpy(_filter.targetPath, arrayPath ? arrayPath : "", sizeof(_filter.targetPath) - 1);
  _filter.targetPath[sizeof(_filter.targetPath) - 1] = '\0';
  return *this;
}

RestRequest& RestRequest::applyFilter(FilterOperator op, DataType type, const char* valStr, double num) {
  _filter.active = true;
  _filter.op = op;
  _filter.type = type;
  _filter.num = num;
  strncpy(_filter.value, valStr ? valStr : "", sizeof(_filter.value) - 1);
  _filter.value[sizeof(_filter.value) - 1] = '\0';
  return *this;
}

RestRequest& RestRequest::collect(String* targetString) {
  if (!targetString) return *this;

  _filter.active = true;
  _filter.targetCollector = [targetString](BufferedStreamReader& reader, const RestFilter& filter) {
    String rawObject;
    RestRequest::readRawJson(reader, &rawObject, '{');

    if (filter.hasPredicate()) {
      BufferedStreamReader rawReader(rawObject.c_str());
      rawReader.read();
      std::vector<ResponseBinding> bindings;
      if (!RestRequest::evaluateFilter(rawReader, bindings, filter)) return;
    }

    if (targetString->length() <= 2) {
      *targetString = "[";
    } else {
      targetString->remove(targetString->length() - 1);
      *targetString += ',';
    }
    *targetString += rawObject;
    *targetString += "]";
  };
  return *this;
}

bool RestRequest::evaluateFilter(BufferedStreamReader& r, std::vector<ResponseBinding>& bindings, const RestFilter& filter) {
  FilterCapture filterCapture(filter.path, filter.type);
  parseObjectWithBindings(r, "", bindings, &filterCapture);

  if (!filterCapture.matched) return false;
  if (filter.op == FILTER_NONE) return true;

  bool equal;
  switch (filter.type) {
    case TYPE_BOOL:
      equal = (filterCapture.b == (strcmp(filter.value, "true") == 0));
      break;
    case TYPE_DOUBLE:
      equal = (filterCapture.d == filter.num);
      break;
    default:
      equal = (strcmp(filterCapture.s, filter.value) == 0);
      break;
  }
  return filter.op == FILTER_EQUALS ? equal : !equal;
}

RestRequest& RestRequest::equals(bool value) {
  return applyFilter(FILTER_EQUALS, TYPE_BOOL, value ? "true" : "false");
}

RestRequest& RestRequest::equals(int value) {
  return applyFilter(FILTER_EQUALS, TYPE_DOUBLE, "", (double)value);
}

RestRequest& RestRequest::equals(unsigned int value) {
  return applyFilter(FILTER_EQUALS, TYPE_DOUBLE, "", (double)value);
}

RestRequest& RestRequest::equals(long value) {
  return applyFilter(FILTER_EQUALS, TYPE_DOUBLE, "", (double)value);
}

RestRequest& RestRequest::equals(unsigned long value) {
  return applyFilter(FILTER_EQUALS, TYPE_DOUBLE, "", (double)value);
}

RestRequest& RestRequest::equals(long long value) {
  return applyFilter(FILTER_EQUALS, TYPE_DOUBLE, "", (double)value);
}

RestRequest& RestRequest::equals(unsigned long long value) {
  return applyFilter(FILTER_EQUALS, TYPE_DOUBLE, "", (double)value);
}

RestRequest& RestRequest::equals(float value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%.5g", value);
  return applyFilter(FILTER_EQUALS, TYPE_DOUBLE, "", std::strtod(buf, nullptr));
}

RestRequest& RestRequest::equals(double value) {
  return applyFilter(FILTER_EQUALS, TYPE_DOUBLE, "", value);
}

RestRequest& RestRequest::equals(const char* value) {
  return applyFilter(FILTER_EQUALS, TYPE_STRING, value ? value : "");
}

RestRequest& RestRequest::equals(const String& v) {
  return equals(v.c_str());
}

RestRequest& RestRequest::notEquals(bool value) {
  return applyFilter(FILTER_NOT_EQUALS, TYPE_BOOL, value ? "true" : "false");
}

RestRequest& RestRequest::notEquals(int value) {
  return applyFilter(FILTER_NOT_EQUALS, TYPE_DOUBLE, "", (double)value);
}

RestRequest& RestRequest::notEquals(unsigned int value) {
  return applyFilter(FILTER_NOT_EQUALS, TYPE_DOUBLE, "", (double)value);
}

RestRequest& RestRequest::notEquals(long value) {
  return applyFilter(FILTER_NOT_EQUALS, TYPE_DOUBLE, "", (double)value);
}

RestRequest& RestRequest::notEquals(unsigned long value) {
  return applyFilter(FILTER_NOT_EQUALS, TYPE_DOUBLE, "", (double)value);
}

RestRequest& RestRequest::notEquals(long long value) {
  return applyFilter(FILTER_NOT_EQUALS, TYPE_DOUBLE, "", (double)value);
}

RestRequest& RestRequest::notEquals(unsigned long long value) {
  return applyFilter(FILTER_NOT_EQUALS, TYPE_DOUBLE, "", (double)value);
}

RestRequest& RestRequest::notEquals(float value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%.5g", value);
  return applyFilter(FILTER_NOT_EQUALS, TYPE_DOUBLE, "", std::strtod(buf, nullptr));
}

RestRequest& RestRequest::notEquals(double value) {
  return applyFilter(FILTER_NOT_EQUALS, TYPE_DOUBLE, "", value);
}

RestRequest& RestRequest::notEquals(const char* value) {
  return applyFilter(FILTER_NOT_EQUALS, TYPE_STRING, value ? value : "");
}

RestRequest& RestRequest::notEquals(const String& v) {
  return notEquals(v.c_str());
}
