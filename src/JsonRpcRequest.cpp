#include "JsonRpcRequest.h"

#include <HTTPClient.h>
#include <cctype>
#include <cstdlib>

#include "BufferedStreamReader.h"
#include "ESP32HTTPClient.h"

static uint32_t s_jsonRpcIdCounter = 1;

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
    int depth = 1;
    bool inStr = false;
    bool esc = false;

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

static void readStringIntoBuffer(BufferedStreamReader& r, char* buffer, size_t maxLen) {
  r.read();  // consume opening quote
  size_t idx = 0;
  bool esc = false;

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

static String readStringToken(BufferedStreamReader& r) {
  char buf[256];
  readStringIntoBuffer(r, buf, sizeof(buf));
  return String(buf);
}

static void readRawJsonFromReader(BufferedStreamReader& r, String* target, char openingBrace) {
  if (!target) return;
  int depth = 1;
  bool inString = false;
  bool escaped = false;

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

String JsonRpcRequest::normalizeResultPath(const char* path) {
  if (!path || path[0] == '\0') return "result";
  String p = path;
  if (p.startsWith("result")) {
    p = p.substring(6);
    if (p.startsWith(".")) p = p.substring(1);
  }
  String out = "result";
  if (!p.isEmpty()) {
    String converted = "";
    for (size_t i = 0; i < p.length(); i++) {
      if (p[i] == '[') {
        if (!converted.isEmpty() && !converted.endsWith(".")) {
          converted += ".";
        }
      } else if (p[i] == ']') {
        // closing bracket ignored
      } else {
        converted += p[i];
      }
    }
    if (converted.startsWith(".")) {
      converted = converted.substring(1);
    }
    if (!converted.isEmpty()) {
      out += "." + converted;
    }
  }
  return out;
}

String JsonRpcRequest::escapeJsonString(const String& str) {
  String out;
  out.reserve(str.length() + 16);
  for (size_t i = 0; i < str.length(); i++) {
    char c = str[i];
    if (c == '"') {
      out += "\\\"";
    } else if (c == '\\') {
      out += "\\\\";
    } else if (c == '\b') {
      out += "\\b";
    } else if (c == '\f') {
      out += "\\f";
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      out += "\\r";
    } else if (c == '\t') {
      out += "\\t";
    } else if ((unsigned char)c < 0x20) {
      char buf[8];
      snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
      out += buf;
    } else {
      out += c;
    }
  }
  return out;
}

JsonRpcRequest::JsonRpcRequest(ESP32HTTPClient* client, const char* path)
    : _client(client),
      _path(path),
      _executed(false),
      _timeout(0),
      _maxRetry(-1),
      _statusCode(0),
      _method(""),
      _isNotification(false),
      _idType(JSONRPC_ID_AUTO),
      _intId(s_jsonRpcIdCounter++),
      _stringId(""),
      _paramMode(JSONRPC_PARAMS_NONE),
      _rawParams(""),
      _contentType("application/json"),
      _rawResultTarget(nullptr),
      _rawResponseTarget(nullptr),
      _jsonRpcVersionTarget(nullptr),
      _idIntTarget(nullptr),
      _idLongTarget(nullptr),
      _idStringTarget(nullptr),
      _errorTarget(nullptr),
      _errorCodeTarget(nullptr),
      _errorMessageTarget(nullptr),
      _errorDataTarget(nullptr),
      _hasError(false),
      _responseId(""),
      _hasResponseId(false),
      _responseJsonRpcVersion(""),
      _onSuccessCb(nullptr),
      _onErrorCb(nullptr),
      _onResponseCb(nullptr),
      _onJsonRpcErrorCb(nullptr) {
}

JsonRpcRequest::JsonRpcRequest(JsonRpcRequest&& other)
    : _client(other._client),
      _path(other._path),
      _executed(other._executed),
      _timeout(other._timeout),
      _maxRetry(other._maxRetry),
      _statusCode(other._statusCode),
      _method(std::move(other._method)),
      _isNotification(other._isNotification),
      _idType(other._idType),
      _intId(other._intId),
      _stringId(std::move(other._stringId)),
      _paramMode(other._paramMode),
      _params(std::move(other._params)),
      _rawParams(std::move(other._rawParams)),
      _contentType(std::move(other._contentType)),
      _pathParams(std::move(other._pathParams)),
      _queryParams(std::move(other._queryParams)),
      _customHeaders(std::move(other._customHeaders)),
      _responseBindings(std::move(other._responseBindings)),
      _headerBindings(std::move(other._headerBindings)),
      _keyStorage(std::move(other._keyStorage)),
      _rawResultTarget(other._rawResultTarget),
      _rawResponseTarget(other._rawResponseTarget),
      _jsonRpcVersionTarget(other._jsonRpcVersionTarget),
      _idIntTarget(other._idIntTarget),
      _idLongTarget(other._idLongTarget),
      _idStringTarget(other._idStringTarget),
      _errorTarget(other._errorTarget),
      _errorCodeTarget(other._errorCodeTarget),
      _errorMessageTarget(other._errorMessageTarget),
      _errorDataTarget(other._errorDataTarget),
      _hasError(other._hasError),
      _error(std::move(other._error)),
      _responseId(std::move(other._responseId)),
      _hasResponseId(other._hasResponseId),
      _responseJsonRpcVersion(std::move(other._responseJsonRpcVersion)),
      _onSuccessCb(std::move(other._onSuccessCb)),
      _onErrorCb(std::move(other._onErrorCb)),
      _onResponseCb(std::move(other._onResponseCb)),
      _onJsonRpcErrorCb(std::move(other._onJsonRpcErrorCb)) {
  other._executed = true;
}

JsonRpcRequest::~JsonRpcRequest() {
  if (!_executed) {
    execute();
  }
}

JsonRpcRequest& JsonRpcRequest::method(const char* methodName) {
  _method = methodName ? methodName : "";
  return *this;
}

JsonRpcRequest& JsonRpcRequest::method(const String& methodName) {
  _method = methodName;
  return *this;
}

const String& JsonRpcRequest::getMethod() const {
  return _method;
}

JsonRpcRequest& JsonRpcRequest::id(int reqId) {
  _idType = JSONRPC_ID_INT;
  _intId = reqId;
  _isNotification = false;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::id(long reqId) {
  _idType = JSONRPC_ID_INT;
  _intId = reqId;
  _isNotification = false;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::id(const char* reqId) {
  if (!reqId) {
    return nullId();
  }
  _idType = JSONRPC_ID_STRING;
  _stringId = reqId;
  _isNotification = false;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::id(const String& reqId) {
  _idType = JSONRPC_ID_STRING;
  _stringId = reqId;
  _isNotification = false;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::nullId() {
  _idType = JSONRPC_ID_NULL;
  _isNotification = false;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::notification(bool enable) {
  _isNotification = enable;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::asNotification() {
  _isNotification = true;
  return *this;
}

bool JsonRpcRequest::isNotification() const {
  return _isNotification;
}

bool JsonRpcRequest::hasRequestId() const {
  return !_isNotification;
}

String JsonRpcRequest::getRequestId() const {
  if (_isNotification) return "";
  if (_idType == JSONRPC_ID_STRING) return _stringId;
  if (_idType == JSONRPC_ID_NULL) return "null";
  return String(_intId);
}

int JsonRpcRequest::getRequestIdInt() const {
  if (_isNotification || _idType == JSONRPC_ID_NULL) return 0;
  if (_idType == JSONRPC_ID_STRING) return atoi(_stringId.c_str());
  return (int)_intId;
}

void JsonRpcRequest::addParamInternal(const char* name, const char* value, bool quote, bool raw) {
  if (name && name[0] != '\0') {
    _paramMode = JSONRPC_PARAMS_NAMED;
  } else if (_paramMode == JSONRPC_PARAMS_NONE) {
    _paramMode = JSONRPC_PARAMS_POSITIONAL;
  }
  JsonRpcParam p;
  if (name) p.name = name;
  p.setValue(value, quote, raw);
  _params.push_back(p);
}

// Positional parameters
JsonRpcRequest& JsonRpcRequest::param(const char* value) {
  addParamInternal(nullptr, value, true, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(const String& value) {
  return param(value.c_str());
}

JsonRpcRequest& JsonRpcRequest::param(bool value) {
  addParamInternal(nullptr, value ? "true" : "false", false, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(int value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%d", value);
  addParamInternal(nullptr, buf, false, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(unsigned int value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%u", value);
  addParamInternal(nullptr, buf, false, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(long value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%ld", value);
  addParamInternal(nullptr, buf, false, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(unsigned long value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%lu", value);
  addParamInternal(nullptr, buf, false, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(long long value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%lld", value);
  addParamInternal(nullptr, buf, false, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(unsigned long long value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%llu", value);
  addParamInternal(nullptr, buf, false, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(float value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%.7g", value);
  addParamInternal(nullptr, buf, false, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(double value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%.14g", value);
  addParamInternal(nullptr, buf, false, false);
  return *this;
}

// Named parameters
JsonRpcRequest& JsonRpcRequest::param(const char* name, const char* value) {
  addParamInternal(name, value, true, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(const char* name, const String& value) {
  return param(name, value.c_str());
}

JsonRpcRequest& JsonRpcRequest::param(const char* name, bool value) {
  addParamInternal(name, value ? "true" : "false", false, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(const char* name, int value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%d", value);
  addParamInternal(name, buf, false, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(const char* name, unsigned int value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%u", value);
  addParamInternal(name, buf, false, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(const char* name, long value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%ld", value);
  addParamInternal(name, buf, false, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(const char* name, unsigned long value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%lu", value);
  addParamInternal(name, buf, false, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(const char* name, long long value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%lld", value);
  addParamInternal(name, buf, false, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(const char* name, unsigned long long value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%llu", value);
  addParamInternal(name, buf, false, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(const char* name, float value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%.7g", value);
  addParamInternal(name, buf, false, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::param(const char* name, double value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%.14g", value);
  addParamInternal(name, buf, false, false);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::rawParams(const char* jsonParams) {
  _rawParams = jsonParams ? jsonParams : "";
  _paramMode = JSONRPC_PARAMS_RAW;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::rawParams(const String& jsonParams) {
  _rawParams = jsonParams;
  _paramMode = JSONRPC_PARAMS_RAW;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::rawParam(const char* rawJson) {
  addParamInternal(nullptr, rawJson, false, true);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::rawParam(const String& rawJson) {
  return rawParam(rawJson.c_str());
}

JsonRpcRequest& JsonRpcRequest::rawParam(const char* name, const char* rawJson) {
  addParamInternal(name, rawJson, false, true);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::rawParam(const char* name, const String& rawJson) {
  return rawParam(name, rawJson.c_str());
}

JsonRpcRequest& JsonRpcRequest::header(const char* name, const char* value) {
  HttpHeader h;
  strncpy(h.name, name ? name : "", sizeof(h.name) - 1);
  h.name[sizeof(h.name) - 1] = 0;
  strncpy(h.value, value ? value : "", sizeof(h.value) - 1);
  h.value[sizeof(h.value) - 1] = 0;
  _customHeaders.push_back(h);
  return *this;
}

JsonRpcRequest& JsonRpcRequest::contentType(const char* type) {
  _contentType = type ? type : "application/json";
  return *this;
}

JsonRpcRequest& JsonRpcRequest::timeout(uint16_t timeoutMs) {
  _timeout = timeoutMs;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::maxRetry(int maxRetry) {
  _maxRetry = (maxRetry < 0) ? 0 : maxRetry;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::retry(int maxRetry) {
  return this->maxRetry(maxRetry);
}

// Result bindings (root result)
JsonRpcRequest& JsonRpcRequest::getResult(int* target) {
  if (target) _responseBindings.push_back({"result", target, TYPE_INT, sizeof(int)});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getResult(float* target) {
  if (target) _responseBindings.push_back({"result", target, TYPE_FLOAT, sizeof(float)});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getResult(double* target) {
  if (target) _responseBindings.push_back({"result", target, TYPE_DOUBLE, sizeof(double)});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getResult(bool* target) {
  if (target) _responseBindings.push_back({"result", target, TYPE_BOOL, sizeof(bool)});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getResult(long* target) {
  if (target) _responseBindings.push_back({"result", target, TYPE_LONG, sizeof(long)});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getResult(char* target, size_t maxLength) {
  if (target && maxLength > 0) _responseBindings.push_back({"result", target, TYPE_STRING, maxLength});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getResult(String* target) {
  if (target) _responseBindings.push_back({"result", target, TYPE_ARDUINO_STRING, 0});
  return *this;
}

// Result bindings (subpath)
JsonRpcRequest& JsonRpcRequest::getResult(const char* subpath, int* target) {
  if (!target) return *this;
  String fullPath = normalizeResultPath(subpath);
  _keyStorage.push_back(fullPath);
  _responseBindings.push_back({_keyStorage.back().c_str(), target, TYPE_INT, sizeof(int)});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getResult(const char* subpath, float* target) {
  if (!target) return *this;
  String fullPath = normalizeResultPath(subpath);
  _keyStorage.push_back(fullPath);
  _responseBindings.push_back({_keyStorage.back().c_str(), target, TYPE_FLOAT, sizeof(float)});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getResult(const char* subpath, double* target) {
  if (!target) return *this;
  String fullPath = normalizeResultPath(subpath);
  _keyStorage.push_back(fullPath);
  _responseBindings.push_back({_keyStorage.back().c_str(), target, TYPE_DOUBLE, sizeof(double)});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getResult(const char* subpath, bool* target) {
  if (!target) return *this;
  String fullPath = normalizeResultPath(subpath);
  _keyStorage.push_back(fullPath);
  _responseBindings.push_back({_keyStorage.back().c_str(), target, TYPE_BOOL, sizeof(bool)});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getResult(const char* subpath, long* target) {
  if (!target) return *this;
  String fullPath = normalizeResultPath(subpath);
  _keyStorage.push_back(fullPath);
  _responseBindings.push_back({_keyStorage.back().c_str(), target, TYPE_LONG, sizeof(long)});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getResult(const char* subpath, char* target, size_t maxLength) {
  if (!target || maxLength == 0) return *this;
  String fullPath = normalizeResultPath(subpath);
  _keyStorage.push_back(fullPath);
  _responseBindings.push_back({_keyStorage.back().c_str(), target, TYPE_STRING, maxLength});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getResult(const char* subpath, String* target) {
  if (!target) return *this;
  String fullPath = normalizeResultPath(subpath);
  _keyStorage.push_back(fullPath);
  _responseBindings.push_back({_keyStorage.back().c_str(), target, TYPE_ARDUINO_STRING, 0});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getRawResult(String* target) {
  _rawResultTarget = target;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getRawResponse(String* target) {
  _rawResponseTarget = target;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getId(int* target) {
  _idIntTarget = target;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getId(long* target) {
  _idLongTarget = target;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getId(String* target) {
  _idStringTarget = target;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getJsonRpcVersion(String* target) {
  _jsonRpcVersionTarget = target;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getHeader(const char* name, int* target) {
  if (target) _headerBindings.push_back({name, target, TYPE_INT, 0});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getHeader(const char* name, float* target) {
  if (target) _headerBindings.push_back({name, target, TYPE_FLOAT, 0});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getHeader(const char* name, double* target) {
  if (target) _headerBindings.push_back({name, target, TYPE_DOUBLE, 0});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getHeader(const char* name, bool* target) {
  if (target) _headerBindings.push_back({name, target, TYPE_BOOL, 0});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getHeader(const char* name, long* target) {
  if (target) _headerBindings.push_back({name, target, TYPE_LONG, 0});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getHeader(const char* name, char* target, size_t maxLength) {
  if (target && maxLength > 0) _headerBindings.push_back({name, target, TYPE_STRING, maxLength});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getHeader(const char* name, String* target) {
  if (target) _headerBindings.push_back({name, target, TYPE_ARDUINO_STRING, 0});
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getError(JsonRpcError* target) {
  _errorTarget = target;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getErrorMessage(String* target) {
  _errorMessageTarget = target;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getErrorCode(int* target) {
  _errorCodeTarget = target;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::getErrorData(String* target) {
  _errorDataTarget = target;
  return *this;
}

bool JsonRpcRequest::hasError() const {
  return _hasError || _statusCode < 200 || _statusCode >= 400;
}

bool JsonRpcRequest::hasJsonRpcError() const {
  return _hasError;
}

bool JsonRpcRequest::isSuccess() const {
  return !hasError() && _statusCode >= 200 && _statusCode < 300;
}

int JsonRpcRequest::getStatusCode() const {
  return _statusCode;
}

const JsonRpcError& JsonRpcRequest::getError() const {
  return _error;
}

int JsonRpcRequest::getErrorCode() const {
  return _error.code;
}

String JsonRpcRequest::getErrorMessage() const {
  return _error.message;
}

String JsonRpcRequest::getErrorData() const {
  return _error.data;
}

String JsonRpcRequest::getResponseId() const {
  return _responseId;
}

int JsonRpcRequest::getResponseIdInt() const {
  return atoi(_responseId.c_str());
}

String JsonRpcRequest::getJsonRpcVersion() const {
  return _responseJsonRpcVersion;
}

JsonRpcRequest& JsonRpcRequest::onSuccess(HttpResponseCallback cb) {
  _onSuccessCb = cb;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::onError(HttpErrorCallback cb) {
  _onErrorCb = cb;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::onError(HttpResponseCallback cb) {
  if (cb) {
    _onErrorCb = [cb](int code, const char*) { cb(code); };
  } else {
    _onErrorCb = nullptr;
  }
  return *this;
}

JsonRpcRequest& JsonRpcRequest::onResponse(HttpResponseCallback cb) {
  _onResponseCb = cb;
  return *this;
}

JsonRpcRequest& JsonRpcRequest::onJsonRpcError(JsonRpcErrorCallback cb) {
  _onJsonRpcErrorCb = cb;
  return *this;
}

String JsonRpcRequest::buildRequestBody() const {
  String body;
  body.reserve(128 + _method.length() + (_params.size() * 32));
  body += "{\"jsonrpc\":\"2.0\",\"method\":\"";
  body += escapeJsonString(_method);
  body += "\"";

  // Parameters
  if (!_rawParams.isEmpty()) {
    body += ",\"params\":";
    body += _rawParams;
  } else if (!_params.empty()) {
    if (_paramMode == JSONRPC_PARAMS_NAMED) {
      body += ",\"params\":{";
      for (size_t i = 0; i < _params.size(); i++) {
        body += "\"";
        body += escapeJsonString(_params[i].name);
        body += "\":";
        if (_params[i].isRawJson) {
          body += _params[i].getValue();
        } else if (_params[i].quoteValue) {
          body += "\"";
          body += escapeJsonString(String(_params[i].getValue()));
          body += "\"";
        } else {
          body += _params[i].getValue();
        }
        if (i < _params.size() - 1) body += ",";
      }
      body += "}";
    } else {  // Positional parameters array
      body += ",\"params\":[";
      for (size_t i = 0; i < _params.size(); i++) {
        if (_params[i].isRawJson) {
          body += _params[i].getValue();
        } else if (_params[i].quoteValue) {
          body += "\"";
          body += escapeJsonString(String(_params[i].getValue()));
          body += "\"";
        } else {
          body += _params[i].getValue();
        }
        if (i < _params.size() - 1) body += ",";
      }
      body += "]";
    }
  }

  // ID member (MUST be omitted for notifications)
  if (!_isNotification) {
    body += ",\"id\":";
    if (_idType == JSONRPC_ID_STRING) {
      body += "\"";
      body += escapeJsonString(_stringId);
      body += "\"";
    } else if (_idType == JSONRPC_ID_NULL) {
      body += "null";
    } else {
      body += String(_intId);
    }
  }

  body += "}";
  return body;
}

ResponseBinding* JsonRpcRequest::findRootBinding() {
  for (auto& b : _responseBindings) {
    if (strcmp(b.key, "result") == 0 || b.key[0] == '\0') {
      return &b;
    }
  }
  return nullptr;
}

void JsonRpcRequest::execute() {
  _executed = true;
  if (!_client) return;

  HTTPClient& http = _client->_http;

  uint16_t effectiveTimeout = (_timeout > 0) ? _timeout : _client->getTimeout();
  if (effectiveTimeout > 0) {
    http.setTimeout(effectiveTimeout);
  }

  String urlBase = _client->getBaseUrl();
  if (_client->getPort() > 0) {
    int protoEnd = urlBase.indexOf("://");
    if (protoEnd != -1) {
      int pathStart = urlBase.indexOf('/', protoEnd + 3);
      if (pathStart != -1) {
        urlBase = urlBase.substring(0, pathStart) + ":" + String(_client->getPort()) + urlBase.substring(pathStart);
      } else {
        urlBase = urlBase + ":" + String(_client->getPort());
      }
    }
  }

  String resolvedPath = _path ? _path : "";
  for (const auto& param : _pathParams) {
    if (!param.key) continue;
    String placeholder = (param.key[0] == '{') ? param.key : ("{" + String(param.key) + "}");
    resolvedPath.replace(placeholder, param.value.c_str());
  }

  String url = urlBase + resolvedPath;
  if (!_queryParams.empty()) {
    url += "?";
    for (size_t i = 0; i < _queryParams.size(); i++) {
      url += _queryParams[i].key;
      url += "=";
      url += _queryParams[i].value.c_str();
      if (i < _queryParams.size() - 1) url += "&";
    }
  }

  String payload = buildRequestBody();

  std::vector<const char*> headerKeys;
  if (!_headerBindings.empty()) {
    headerKeys.reserve(_headerBindings.size() + 2);
    for (const auto& binding : _headerBindings) {
      if (binding.key && binding.key[0] != '\0') {
        headerKeys.push_back(binding.key);
      }
    }
  }
  headerKeys.push_back("Content-Type");

  auto setupConnection = [&]() {
    http.begin(url);
    if (!headerKeys.empty()) {
      http.collectHeaders(headerKeys.data(), headerKeys.size());
    }
    for (const auto& header : _client->_headers) {
      http.addHeader(header.name, header.value);
    }
    for (const auto& header : _customHeaders) {
      http.addHeader(header.name, header.value);
    }
    const char* ct = _contentType.isEmpty() ? "application/json" : _contentType.c_str();
    http.addHeader("Content-Type", ct);
  };

  unsigned long startTime = millis();
  uint32_t freeHeapBefore = ESP.getFreeHeap();

  setupConnection();

  int retries = 0;
  int maxR = (_maxRetry >= 0) ? _maxRetry : _client->getMaxRetry();
  int code = 0;

  while (retries <= maxR) {
    code = http.POST(payload);
    if (code < 0 && retries < maxR) {
      http.end();
      setupConnection();
      retries++;
    } else {
      break;
    }
  }

  unsigned long ttfbTime = 0;
  if (_client->_observabilityCb) {
    ttfbTime = millis();
  }

  _statusCode = code;
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

  if (_hasError) {
    if (_onJsonRpcErrorCb) {
      _onJsonRpcErrorCb(_error);
    }
    if (_onErrorCb) {
      _onErrorCb(_error.code, _error.message.c_str());
    }
    if (_client->_onErrorCb) {
      _client->_onErrorCb(_error.code, _error.message.c_str());
    }
  } else if (code >= 200 && code < 300) {
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

void JsonRpcRequest::parseResponse(BufferedStreamReader& r) {
  if (_rawResponseTarget) {
    _rawResponseTarget->reserve(512);
    while (r.available()) {
      *_rawResponseTarget += (char)r.read();
    }
    BufferedStreamReader strReader(_rawResponseTarget->c_str());
    parseResponseObject(strReader);
    return;
  }
  parseResponseObject(r);
}

void JsonRpcRequest::parseResponseObject(BufferedStreamReader& r) {
  skipWhitespace(r);
  if (!r.available()) return;
  char openChar = (char)r.peek();
  if (openChar != '{') {
    _hasError = true;
    _error.code = JSONRPC_PARSE_ERROR;
    _error.message = "Invalid JSON response: expected object";
    if (_errorTarget) *_errorTarget = _error;
    if (_errorCodeTarget) *_errorCodeTarget = _error.code;
    if (_errorMessageTarget) *_errorMessageTarget = _error.message;
    return;
  }
  r.read();  // consume '{'

  while (r.available()) {
    skipWhitespace(r);
    char next = (char)r.peek();
    if (next == '}') {
      r.read();
      break;
    }
    if (next == ',') {
      r.read();
      continue;
    }
    if (next == '"') {
      String key = readStringToken(r);
      skipWhitespace(r);
      if (r.read() != ':') continue;
      skipWhitespace(r);

      if (key == "jsonrpc") {
        _responseJsonRpcVersion = readStringToken(r);
        if (_jsonRpcVersionTarget) {
          *_jsonRpcVersionTarget = _responseJsonRpcVersion;
        }
      } else if (key == "id") {
        char idStart = (char)r.peek();
        if (idStart == '"') {
          _responseId = readStringToken(r);
        } else if (idStart == 'n') {
          for (int k = 0; k < 4 && r.available(); k++) r.read();
          _responseId = "null";
        } else {
          String numStr = "";
          while (r.available()) {
            char c = (char)r.peek();
            if (isdigit((unsigned char)c) || c == '-' || c == '.') {
              numStr += (char)r.read();
            } else {
              break;
            }
          }
          _responseId = numStr;
        }
        _hasResponseId = true;
        if (_idStringTarget) *_idStringTarget = _responseId;
        if (_idIntTarget) *_idIntTarget = atoi(_responseId.c_str());
        if (_idLongTarget) *_idLongTarget = atol(_responseId.c_str());
      } else if (key == "error") {
        _hasError = true;
        parseErrorObject(r);
      } else if (key == "result") {
        parseResultValue(r);
      } else {
        skipValue(r);
      }

      skipWhitespace(r);
      if (r.peek() == ',') r.read();
    } else {
      r.read();
    }
  }
}

void JsonRpcRequest::parseErrorObject(BufferedStreamReader& r) {
  skipWhitespace(r);
  char c = (char)r.peek();
  if (c != '{') {
    skipValue(r);
    return;
  }
  r.read();  // consume '{'

  while (r.available()) {
    skipWhitespace(r);
    char next = (char)r.peek();
    if (next == '}') {
      r.read();
      break;
    }
    if (next == ',') {
      r.read();
      continue;
    }
    if (next == '"') {
      String errKey = readStringToken(r);
      skipWhitespace(r);
      if (r.read() != ':') continue;
      skipWhitespace(r);

      if (errKey == "code") {
        char numBuf[32];
        size_t nIdx = 0;
        while (r.available()) {
          char b = (char)r.peek();
          if (isdigit((unsigned char)b) || b == '-') {
            if (nIdx < sizeof(numBuf) - 1) numBuf[nIdx++] = (char)r.read();
            else r.read();
          } else {
            break;
          }
        }
        numBuf[nIdx] = '\0';
        _error.code = atoi(numBuf);
        if (_errorCodeTarget) *_errorCodeTarget = _error.code;
      } else if (errKey == "message") {
        _error.message = readStringToken(r);
        if (_errorMessageTarget) *_errorMessageTarget = _error.message;
      } else if (errKey == "data") {
        char dStart = (char)r.peek();
        if (dStart == '{' || dStart == '[') {
          r.read();
          readRawJsonFromReader(r, &_error.data, dStart);
        } else if (dStart == '"') {
          _error.data = readStringToken(r);
        } else {
          _error.data = "";
          while (r.available()) {
            char b = (char)r.peek();
            if (b == ',' || b == '}' || b == ' ' || b == '\t' || b == '\n' || b == '\r') {
              break;
            }
            _error.data += (char)r.read();
          }
        }
        if (_errorDataTarget) *_errorDataTarget = _error.data;
      } else {
        skipValue(r);
      }

      skipWhitespace(r);
      if (r.peek() == ',') r.read();
    } else {
      r.read();
    }
  }

  if (_errorTarget) {
    *_errorTarget = _error;
  }
}

void JsonRpcRequest::applyBindingsToResult(BufferedStreamReader& r) {
  skipWhitespace(r);
  char resStart = (char)r.peek();

  ResponseBinding* rootBinding = findRootBinding();
  bool hasChildBinding = false;
  for (const auto& b : _responseBindings) {
    if (strncmp(b.key, "result.", 7) == 0 || strncmp(b.key, "result[", 7) == 0) {
      hasChildBinding = true;
      break;
    }
  }

  if (resStart == '{' || resStart == '[') {
    if (rootBinding && rootBinding->type == TYPE_ARDUINO_STRING && !hasChildBinding) {
      r.read();
      RestRequest::readRawJson(r, (String*)rootBinding->target, resStart);
    } else {
      r.read();
      if (resStart == '{') {
        RestRequest::parseObjectWithBindings(r, "result", _responseBindings);
      } else {
        RestRequest::parseArrayWithBindings(r, "result", _responseBindings);
      }
    }
  } else {
    std::vector<ResponseBinding*> rootBindings;
    for (auto& b : _responseBindings) {
      if (strcmp(b.key, "result") == 0 || b.key[0] == '\0') {
        rootBindings.push_back(&b);
      }
    }
    if (!rootBindings.empty()) {
      if (rootBindings.size() == 1) {
        RestRequest::parsePrimitiveWithBinding(r, rootBindings[0]);
      } else {
        String primStr = "";
        if (resStart == '"') {
          primStr = "\"";
          primStr += readStringToken(r);
          primStr += "\"";
        } else {
          while (r.available()) {
            char b = (char)r.peek();
            if (b == ',' || b == '}' || b == ' ' || b == '\t' || b == '\n' || b == '\r') break;
            primStr += (char)r.read();
          }
        }
        for (auto* rb : rootBindings) {
          BufferedStreamReader subReader(primStr.c_str());
          RestRequest::parsePrimitiveWithBinding(subReader, rb);
        }
      }
    } else {
      skipValue(r);
    }
  }
}

void JsonRpcRequest::parseResultValue(BufferedStreamReader& r) {
  skipWhitespace(r);
  char resStart = (char)r.peek();

  if (_rawResultTarget) {
    if (resStart == '{' || resStart == '[') {
      r.read();
      readRawJsonFromReader(r, _rawResultTarget, resStart);
    } else if (resStart == '"') {
      *_rawResultTarget = "\"";
      *_rawResultTarget += readStringToken(r);
      *_rawResultTarget += "\"";
    } else {
      *_rawResultTarget = "";
      while (r.available()) {
        char b = (char)r.peek();
        if (b == ',' || b == '}' || b == ' ' || b == '\t' || b == '\n' || b == '\r') break;
        *_rawResultTarget += (char)r.read();
      }
    }

    if (!_responseBindings.empty()) {
      BufferedStreamReader subReader(_rawResultTarget->c_str());
      applyBindingsToResult(subReader);
    }
    return;
  }

  applyBindingsToResult(r);
}
