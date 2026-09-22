#include "GraphQLRequest.h"

#include <HTTPClient.h>
#include <cctype>
#include <cstdlib>

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

String GraphQLRequest::normalizeDataPath(const char* path) {
  if (!path || path[0] == '\0') {
    return "data";
  }
  if (strcmp(path, "data") == 0 || strncmp(path, "data.", 5) == 0) {
    return String(path);
  }
  return "data." + String(path);
}

String GraphQLRequest::escapeJsonString(const String& str) {
  String out;
  out.reserve(str.length() + 32);
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

String GraphQLRequest::urlEncode(const String& str) {
  String encoded;
  encoded.reserve(str.length() * 2);
  static const char hexChars[] = "0123456789ABCDEF";

  for (size_t i = 0; i < str.length(); i++) {
    char c = str[i];
    if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      encoded += '%';
      encoded += hexChars[((unsigned char)c >> 4) & 0x0F];
      encoded += hexChars[(unsigned char)c & 0x0F];
    }
  }
  return encoded;
}

GraphQLRequest::GraphQLRequest(ESP32HTTPClient* client, const char* path, HttpMethod method)
    : _client(client),
      _path(path),
      _method(method),
      _executed(false),
      _isBatchMember(false),
      _timeout(0),
      _maxRetry(-1),
      _rawDataTarget(nullptr),
      _rawResponseTarget(nullptr),
      _firstErrorTarget(nullptr),
      _allErrorsTarget(nullptr),
      _errorMessageTarget(nullptr),
      _onSuccessCb(nullptr),
      _onErrorCb(nullptr),
      _onResponseCb(nullptr),
      _onMultiErrorCb(nullptr),
      _onSingleErrorCb(nullptr),
      _onIncrementalCb(nullptr) {
}

GraphQLRequest::GraphQLRequest(GraphQLRequest&& other)
    : _client(other._client),
      _path(other._path),
      _method(other._method),
      _executed(other._executed),
      _isBatchMember(other._isBatchMember),
      _timeout(other._timeout),
      _maxRetry(other._maxRetry),
      _document(std::move(other._document)),
      _operationName(std::move(other._operationName)),
      _rawVariables(std::move(other._rawVariables)),
      _acceptHeader(std::move(other._acceptHeader)),
      _variables(std::move(other._variables)),
      _pathParams(std::move(other._pathParams)),
      _queryParams(std::move(other._queryParams)),
      _customHeaders(std::move(other._customHeaders)),
      _responseBindings(std::move(other._responseBindings)),
      _headerBindings(std::move(other._headerBindings)),
      _keyStorage(std::move(other._keyStorage)),
      _rawDataTarget(other._rawDataTarget),
      _rawResponseTarget(other._rawResponseTarget),
      _firstErrorTarget(other._firstErrorTarget),
      _allErrorsTarget(other._allErrorsTarget),
      _errorMessageTarget(other._errorMessageTarget),
      _errors(std::move(other._errors)),
      _onSuccessCb(std::move(other._onSuccessCb)),
      _onErrorCb(std::move(other._onErrorCb)),
      _onResponseCb(std::move(other._onResponseCb)),
      _onMultiErrorCb(std::move(other._onMultiErrorCb)),
      _onSingleErrorCb(std::move(other._onSingleErrorCb)),
      _onIncrementalCb(std::move(other._onIncrementalCb)) {
  other._executed = true;
}

GraphQLRequest::~GraphQLRequest() {
  if (!_executed && !_isBatchMember) {
    execute();
  }
}

GraphQLRequest& GraphQLRequest::query(const char* queryDocument) {
  _document = queryDocument ? queryDocument : "";
  return *this;
}

GraphQLRequest& GraphQLRequest::query(const String& queryDocument) {
  _document = queryDocument;
  return *this;
}

GraphQLRequest& GraphQLRequest::mutation(const char* mutationDocument) {
  _document = mutationDocument ? mutationDocument : "";
  _method = HTTP_POST_METHOD;
  return *this;
}

GraphQLRequest& GraphQLRequest::mutation(const String& mutationDocument) {
  _document = mutationDocument;
  _method = HTTP_POST_METHOD;
  return *this;
}

GraphQLRequest& GraphQLRequest::document(const char* document) {
  _document = document ? document : "";
  return *this;
}

GraphQLRequest& GraphQLRequest::document(const String& document) {
  _document = document;
  return *this;
}

GraphQLRequest& GraphQLRequest::operationName(const char* name) {
  _operationName = name ? name : "";
  return *this;
}

GraphQLRequest& GraphQLRequest::operationName(const String& name) {
  _operationName = name;
  return *this;
}

GraphQLRequest& GraphQLRequest::method(HttpMethod method) {
  _method = method;
  return *this;
}

GraphQLRequest& GraphQLRequest::asGet() {
  _method = HTTP_GET_METHOD;
  return *this;
}

GraphQLRequest& GraphQLRequest::asPost() {
  _method = HTTP_POST_METHOD;
  return *this;
}

GraphQLRequest& GraphQLRequest::get() {
  _method = HTTP_GET_METHOD;
  return *this;
}

GraphQLRequest& GraphQLRequest::post() {
  _method = HTTP_POST_METHOD;
  return *this;
}

void GraphQLRequest::addParam(std::vector<KeyValue>& list, const char* key, const char* value) {
  KeyValue kv;
  kv.key = key;
  strncpy(kv.valueBuffer, value ? value : "", sizeof(kv.valueBuffer) - 1);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = true;
  list.push_back(kv);
}

void GraphQLRequest::addParam(std::vector<KeyValue>& list, const char* key, const String& value) {
  addParam(list, key, value.c_str());
}

void GraphQLRequest::addParam(std::vector<KeyValue>& list, const char* key, bool value) {
  KeyValue kv;
  kv.key = key;
  strncpy(kv.valueBuffer, value ? "true" : "false", sizeof(kv.valueBuffer) - 1);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

void GraphQLRequest::addParam(std::vector<KeyValue>& list, const char* key, int value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%d", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

void GraphQLRequest::addParam(std::vector<KeyValue>& list, const char* key, unsigned int value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%u", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

void GraphQLRequest::addParam(std::vector<KeyValue>& list, const char* key, long value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%ld", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

void GraphQLRequest::addParam(std::vector<KeyValue>& list, const char* key, unsigned long value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%lu", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

void GraphQLRequest::addParam(std::vector<KeyValue>& list, const char* key, long long value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%lld", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

void GraphQLRequest::addParam(std::vector<KeyValue>& list, const char* key, unsigned long long value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%llu", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

void GraphQLRequest::addParam(std::vector<KeyValue>& list, const char* key, float value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%.5g", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

void GraphQLRequest::addParam(std::vector<KeyValue>& list, const char* key, double value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%.9g", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

GraphQLRequest& GraphQLRequest::variable(const char* name, const char* value) {
  addParam(_variables, name, value);
  return *this;
}

GraphQLRequest& GraphQLRequest::variable(const char* name, const String& value) {
  addParam(_variables, name, value);
  return *this;
}

GraphQLRequest& GraphQLRequest::variable(const char* name, bool value) {
  addParam(_variables, name, value);
  return *this;
}

GraphQLRequest& GraphQLRequest::variable(const char* name, int value) {
  addParam(_variables, name, value);
  return *this;
}

GraphQLRequest& GraphQLRequest::variable(const char* name, unsigned int value) {
  addParam(_variables, name, value);
  return *this;
}

GraphQLRequest& GraphQLRequest::variable(const char* name, long value) {
  addParam(_variables, name, value);
  return *this;
}

GraphQLRequest& GraphQLRequest::variable(const char* name, unsigned long value) {
  addParam(_variables, name, value);
  return *this;
}

GraphQLRequest& GraphQLRequest::variable(const char* name, long long value) {
  addParam(_variables, name, value);
  return *this;
}

GraphQLRequest& GraphQLRequest::variable(const char* name, unsigned long long value) {
  addParam(_variables, name, value);
  return *this;
}

GraphQLRequest& GraphQLRequest::variable(const char* name, float value) {
  addParam(_variables, name, value);
  return *this;
}

GraphQLRequest& GraphQLRequest::variable(const char* name, double value) {
  addParam(_variables, name, value);
  return *this;
}

GraphQLRequest& GraphQLRequest::rawVariable(const char* name, const char* jsonValue) {
  KeyValue kv;
  kv.key = name;
  strncpy(kv.valueBuffer, jsonValue ? jsonValue : "null", sizeof(kv.valueBuffer) - 1);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  _variables.push_back(kv);
  return *this;
}

GraphQLRequest& GraphQLRequest::rawVariable(const char* name, const String& jsonValue) {
  return rawVariable(name, jsonValue.c_str());
}

GraphQLRequest& GraphQLRequest::rawVariables(const char* jsonObject) {
  _rawVariables = jsonObject ? jsonObject : "";
  return *this;
}

GraphQLRequest& GraphQLRequest::rawVariables(const String& jsonObject) {
  _rawVariables = jsonObject;
  return *this;
}

GraphQLRequest& GraphQLRequest::header(const char* name, const char* value) {
  if (!name || !value) return *this;
  for (auto& h : _customHeaders) {
    if (strcasecmp(h.name, name) == 0) {
      strncpy(h.value, value, sizeof(h.value) - 1);
      h.value[sizeof(h.value) - 1] = '\0';
      return *this;
    }
  }
  HttpHeader h;
  strncpy(h.name, name, sizeof(h.name) - 1);
  h.name[sizeof(h.name) - 1] = '\0';
  strncpy(h.value, value, sizeof(h.value) - 1);
  h.value[sizeof(h.value) - 1] = '\0';
  _customHeaders.push_back(h);
  return *this;
}

GraphQLRequest& GraphQLRequest::accept(const char* acceptHeader) {
  _acceptHeader = acceptHeader ? acceptHeader : "";
  return *this;
}

GraphQLRequest& GraphQLRequest::timeout(uint16_t timeoutMs) {
  _timeout = timeoutMs;
  return *this;
}

GraphQLRequest& GraphQLRequest::maxRetry(int maxRetry) {
  _maxRetry = (maxRetry < 0) ? 0 : maxRetry;
  return *this;
}

GraphQLRequest& GraphQLRequest::retry(int maxRetry) {
  return this->maxRetry(maxRetry);
}

GraphQLRequest& GraphQLRequest::getData(const char* path, int* target) {
  _keyStorage.push_back(normalizeDataPath(path));
  _responseBindings.push_back({_keyStorage.back().c_str(), target, TYPE_INT, 0});
  return *this;
}

GraphQLRequest& GraphQLRequest::getData(const char* path, float* target) {
  _keyStorage.push_back(normalizeDataPath(path));
  _responseBindings.push_back({_keyStorage.back().c_str(), target, TYPE_FLOAT, 0});
  return *this;
}

GraphQLRequest& GraphQLRequest::getData(const char* path, double* target) {
  _keyStorage.push_back(normalizeDataPath(path));
  _responseBindings.push_back({_keyStorage.back().c_str(), target, TYPE_DOUBLE, 0});
  return *this;
}

GraphQLRequest& GraphQLRequest::getData(const char* path, bool* target) {
  _keyStorage.push_back(normalizeDataPath(path));
  _responseBindings.push_back({_keyStorage.back().c_str(), target, TYPE_BOOL, 0});
  return *this;
}

GraphQLRequest& GraphQLRequest::getData(const char* path, long* target) {
  _keyStorage.push_back(normalizeDataPath(path));
  _responseBindings.push_back({_keyStorage.back().c_str(), target, TYPE_LONG, 0});
  return *this;
}

GraphQLRequest& GraphQLRequest::getData(const char* path, char* target, size_t maxLength) {
  _keyStorage.push_back(normalizeDataPath(path));
  _responseBindings.push_back({_keyStorage.back().c_str(), target, TYPE_STRING, maxLength});
  return *this;
}

GraphQLRequest& GraphQLRequest::getData(const char* path, String* target) {
  _keyStorage.push_back(normalizeDataPath(path));
  _responseBindings.push_back({_keyStorage.back().c_str(), target, TYPE_ARDUINO_STRING, 0});
  return *this;
}

GraphQLRequest& GraphQLRequest::getRawData(String* target) {
  _rawDataTarget = target;
  _keyStorage.push_back("data");
  _responseBindings.push_back({_keyStorage.back().c_str(), target, TYPE_ARDUINO_STRING, 0});
  return *this;
}

GraphQLRequest& GraphQLRequest::getRawResponse(String* target) {
  _rawResponseTarget = target;
  return *this;
}

GraphQLRequest& GraphQLRequest::getHeader(const char* name, int* target) {
  _headerBindings.push_back({name, target, TYPE_INT, 0});
  return *this;
}

GraphQLRequest& GraphQLRequest::getHeader(const char* name, float* target) {
  _headerBindings.push_back({name, target, TYPE_FLOAT, 0});
  return *this;
}

GraphQLRequest& GraphQLRequest::getHeader(const char* name, double* target) {
  _headerBindings.push_back({name, target, TYPE_DOUBLE, 0});
  return *this;
}

GraphQLRequest& GraphQLRequest::getHeader(const char* name, bool* target) {
  _headerBindings.push_back({name, target, TYPE_BOOL, 0});
  return *this;
}

GraphQLRequest& GraphQLRequest::getHeader(const char* name, long* target) {
  _headerBindings.push_back({name, target, TYPE_LONG, 0});
  return *this;
}

GraphQLRequest& GraphQLRequest::getHeader(const char* name, char* target, size_t maxLength) {
  _headerBindings.push_back({name, target, TYPE_STRING, maxLength});
  return *this;
}

GraphQLRequest& GraphQLRequest::getHeader(const char* name, String* target) {
  _headerBindings.push_back({name, target, TYPE_ARDUINO_STRING, 0});
  return *this;
}

GraphQLRequest& GraphQLRequest::getError(GraphQLError* target) {
  _firstErrorTarget = target;
  return *this;
}

GraphQLRequest& GraphQLRequest::getErrors(std::vector<GraphQLError>* target) {
  _allErrorsTarget = target;
  return *this;
}

GraphQLRequest& GraphQLRequest::getErrorMessage(String* target) {
  _errorMessageTarget = target;
  return *this;
}

bool GraphQLRequest::hasGraphQLErrors() const {
  return !_errors.empty();
}

const std::vector<GraphQLError>& GraphQLRequest::getErrors() const {
  return _errors;
}

GraphQLRequest& GraphQLRequest::onSuccess(HttpResponseCallback cb) {
  _onSuccessCb = cb;
  return *this;
}

GraphQLRequest& GraphQLRequest::onError(HttpErrorCallback cb) {
  _onErrorCb = cb;
  return *this;
}

GraphQLRequest& GraphQLRequest::onError(HttpResponseCallback cb) {
  if (cb) {
    _onErrorCb = [cb](int code, const char*) { cb(code); };
  } else {
    _onErrorCb = nullptr;
  }
  return *this;
}

GraphQLRequest& GraphQLRequest::onResponse(HttpResponseCallback cb) {
  _onResponseCb = cb;
  return *this;
}

GraphQLRequest& GraphQLRequest::onGraphQLError(GraphQLMultiErrorCallback cb) {
  _onMultiErrorCb = cb;
  return *this;
}

GraphQLRequest& GraphQLRequest::onGraphQLError(GraphQLErrorCallback cb) {
  _onSingleErrorCb = cb;
  return *this;
}

GraphQLRequest& GraphQLRequest::onIncremental(GraphQLIncrementalCallback cb) {
  _onIncrementalCb = cb;
  return *this;
}

String GraphQLRequest::buildRequestBody() const {
  String body;
  body.reserve(128 + _document.length() + (_variables.size() * 32));
  body += "{";

  body += "\"query\":\"";
  body += escapeJsonString(_document);
  body += "\"";

  if (!_operationName.isEmpty()) {
    body += ",\"operationName\":\"";
    body += escapeJsonString(_operationName);
    body += "\"";
  }

  if (!_rawVariables.isEmpty()) {
    body += ",\"variables\":";
    body += _rawVariables;
  } else if (!_variables.empty()) {
    body += ",\"variables\":{";
    for (size_t i = 0; i < _variables.size(); i++) {
      body += "\"";
      body += _variables[i].key;
      body += "\":";
      if (_variables[i].quoteValue) {
        body += "\"";
        body += escapeJsonString(String(_variables[i].valueBuffer));
        body += "\"";
      } else {
        body += _variables[i].valueBuffer;
      }
      if (i < _variables.size() - 1) body += ",";
    }
    body += "}";
  }

  body += "}";
  return body;
}

String GraphQLRequest::buildGetUrl() const {
  String urlBase;
  urlBase.reserve(128);
  urlBase = _client->getBaseUrl();

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
    resolvedPath.replace(placeholder, param.valueBuffer);
  }

  String url;
  url.reserve(256 + _document.length());
  url = urlBase;
  url += resolvedPath;

  bool hasQueryParams = false;
  if (!_queryParams.empty()) {
    url += "?";
    hasQueryParams = true;
    for (size_t i = 0; i < _queryParams.size(); i++) {
      url += _queryParams[i].key;
      url += "=";
      url += _queryParams[i].valueBuffer;
      if (i < _queryParams.size() - 1) url += "&";
    }
  }

  if (!_document.isEmpty()) {
    url += hasQueryParams ? "&query=" : "?query=";
    hasQueryParams = true;
    url += urlEncode(_document);
  }

  if (!_operationName.isEmpty()) {
    url += hasQueryParams ? "&operationName=" : "?operationName=";
    hasQueryParams = true;
    url += urlEncode(_operationName);
  }

  if (!_rawVariables.isEmpty()) {
    url += hasQueryParams ? "&variables=" : "?variables=";
    hasQueryParams = true;
    url += urlEncode(_rawVariables);
  } else if (!_variables.empty()) {
    String varsJson = "{";
    for (size_t i = 0; i < _variables.size(); i++) {
      varsJson += "\"";
      varsJson += _variables[i].key;
      varsJson += "\":";
      if (_variables[i].quoteValue) {
        varsJson += "\"";
        varsJson += escapeJsonString(String(_variables[i].valueBuffer));
        varsJson += "\"";
      } else {
        varsJson += _variables[i].valueBuffer;
      }
      if (i < _variables.size() - 1) varsJson += ",";
    }
    varsJson += "}";
    url += hasQueryParams ? "&variables=" : "?variables=";
    hasQueryParams = true;
    url += urlEncode(varsJson);
  }

  return url;
}

void GraphQLRequest::execute() {
  _executed = true;
  if (!_client || _isBatchMember) return;

  HTTPClient& http = _client->_http;

  uint16_t effectiveTimeout = (_timeout > 0) ? _timeout : _client->getTimeout();
  if (effectiveTimeout > 0) {
    http.setTimeout(effectiveTimeout);
  }

  String url;
  String payload;
  if (_method == HTTP_GET_METHOD) {
    url = buildGetUrl();
  } else {
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
      resolvedPath.replace(placeholder, param.valueBuffer);
    }
    url = urlBase + resolvedPath;
    if (!_queryParams.empty()) {
      url += "?";
      for (size_t i = 0; i < _queryParams.size(); i++) {
        url += _queryParams[i].key;
        url += "=";
        url += _queryParams[i].valueBuffer;
        if (i < _queryParams.size() - 1) url += "&";
      }
    }
    payload = buildRequestBody();
  }

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

    String acceptVal = _acceptHeader.isEmpty() ? GRAPHQL_ACCEPT_HEADER : _acceptHeader;
    http.addHeader("Accept", acceptVal.c_str());

    if (_method != HTTP_GET_METHOD) {
      http.addHeader("Content-Type", GRAPHQL_CONTENT_TYPE_JSON);
    }
  };

  setupConnection();

  int code = 0;
  int retries = 0;
  int maxRetries = (_maxRetry >= 0) ? _maxRetry : _client->getMaxRetry();
  int maxAttempts = 1 + maxRetries;

  uint32_t freeHeapBefore = 0;
  unsigned long startTime = 0;
  if (_client->_observabilityCb) {
    freeHeapBefore = ESP.getFreeHeap();
    startTime = millis();
  }

  while (retries < maxAttempts) {
    if (_method == HTTP_GET_METHOD) {
      code = http.GET();
    } else {
      code = http.POST(payload);
    }

    if (code < 0 && retries < maxRetries) {
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

    String responseContentType = http.hasHeader("Content-Type") ? http.header("Content-Type") : "";

    if (http.getSize() > 0 || http.getStreamPtr()) {
      bool isChunked = (http.getSize() == -1);
      BufferedStreamReader reader(http.getStreamPtr(), isChunked);
      parseResponse(reader, responseContentType);
    }
  }

  http.end();

  if (_onResponseCb) _onResponseCb(code);
  if (_client->_onResponseCb) _client->_onResponseCb(code);

  if (code >= 200 && code < 300) {
    if (_onSuccessCb) _onSuccessCb(code);
    if (_client->_onSuccessCb) _client->_onSuccessCb(code);
  } else {
    String errMsg = _client->getErrorMessage();
    if (_onErrorCb) _onErrorCb(code, errMsg.c_str());
    if (_client->_onErrorCb) _client->_onErrorCb(code, errMsg.c_str());
  }

  if (!_errors.empty()) {
    if (_firstErrorTarget) {
      *_firstErrorTarget = _errors[0];
    }
    if (_allErrorsTarget) {
      *_allErrorsTarget = _errors;
    }
    if (_errorMessageTarget) {
      *_errorMessageTarget = _errors[0].message;
    }
    if (_onSingleErrorCb) {
      _onSingleErrorCb(_errors[0]);
    }
    if (_onMultiErrorCb) {
      _onMultiErrorCb(_errors);
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

void GraphQLRequest::parseResponse(BufferedStreamReader& r, const String& contentType) {
  if (contentType.indexOf("multipart/mixed") != -1) {
    String boundary = "";
    int bIdx = contentType.indexOf("boundary=");
    if (bIdx != -1) {
      boundary = contentType.substring(bIdx + 9);
      int semi = boundary.indexOf(';');
      if (semi != -1) boundary = boundary.substring(0, semi);
      boundary.trim();
      if (boundary.startsWith("\"") && boundary.endsWith("\"")) {
        boundary = boundary.substring(1, boundary.length() - 1);
      }
    }
    parseMultipartResponse(r, boundary);
  } else {
    parseSingleResponse(r);
  }
}

void GraphQLRequest::parseSingleResponse(BufferedStreamReader& r) {
  if (_rawResponseTarget) {
    _rawResponseTarget->reserve(512);
    while (r.available()) {
      *_rawResponseTarget += (char)r.read();
    }
    BufferedStreamReader strReader(_rawResponseTarget->c_str());
    skipWhitespace(strReader);
    char firstChar = (char)strReader.peek();
    if (firstChar == '{') {
      strReader.read(); // consume '{'
      while (strReader.available()) {
        skipWhitespace(strReader);
        if (strReader.peek() == '}') {
          strReader.read();
          break;
        }
        if (strReader.peek() == '"') {
          String key = readStringToken(strReader);
          skipWhitespace(strReader);
          if (strReader.read() != ':') continue;
          skipWhitespace(strReader);

          if (key == "data") {
            if (_rawDataTarget) {
              char open = (char)strReader.peek();
              if (open == '{' || open == '[') {
                strReader.read();
                readRawJsonFromReader(strReader, _rawDataTarget, open);
              } else if (open == 'n') {
                char nbuf[8];
                size_t ni = 0;
                while (strReader.available() && isalpha((unsigned char)strReader.peek())) {
                  if (ni < sizeof(nbuf) - 1) nbuf[ni++] = (char)strReader.read();
                }
                nbuf[ni] = 0;
                *_rawDataTarget = nbuf;
              }
            }
            if (!_responseBindings.empty()) {
              BufferedStreamReader dataSubReader(_rawResponseTarget->c_str());
              RestRequest::parseJsonWithBindings(dataSubReader, _responseBindings);
            }
          } else if (key == "errors") {
            parseErrors(strReader);
          } else {
            skipValue(strReader);
          }
          skipWhitespace(strReader);
          if (strReader.peek() == ',') strReader.read();
        } else {
          strReader.read();
        }
      }
    }
    return;
  }

  // Stream parsing directly without buffering full response
  skipWhitespace(r);
  char c = (char)r.read();
  if (c != '{') return;

  while (r.available()) {
    skipWhitespace(r);
    char next = (char)r.peek();
    if (next == '}') {
      r.read();
      break;
    }

    if (next == '"') {
      String key = readStringToken(r);
      skipWhitespace(r);
      if (r.read() != ':') continue;
      skipWhitespace(r);

      if (key == "data") {
        char dataStart = (char)r.peek();
        if (_rawDataTarget && (dataStart == '{' || dataStart == '[')) {
          r.read();
          readRawJsonFromReader(r, _rawDataTarget, dataStart);
          if (!_responseBindings.empty()) {
            BufferedStreamReader subReader(_rawDataTarget->c_str());
            std::vector<ResponseBinding> strippedBindings;
            for (const auto& b : _responseBindings) {
              if (strncmp(b.key, "data.", 5) == 0) {
                strippedBindings.push_back({b.key + 5, b.target, b.type, b.size});
              } else if (strcmp(b.key, "data") == 0) {
                strippedBindings.push_back({"", b.target, b.type, b.size});
              } else {
                strippedBindings.push_back(b);
              }
            }
            RestRequest::parseJsonWithBindings(subReader, strippedBindings);
          }
        } else if (dataStart == '{' || dataStart == '[') {
          r.read();
          if (dataStart == '{') {
            RestRequest::parseObjectWithBindings(r, "data", _responseBindings);
          } else {
            RestRequest::parseArrayWithBindings(r, "data", _responseBindings);
          }
        } else {
          skipValue(r);
        }
      } else if (key == "errors") {
        parseErrors(r);
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

void GraphQLRequest::parseErrors(BufferedStreamReader& r) {
  skipWhitespace(r);
  if (r.peek() != '[') {
    skipValue(r);
    return;
  }
  r.read(); // consume '['

  while (r.available()) {
    skipWhitespace(r);
    if (r.peek() == ']') {
      r.read();
      break;
    }
    if (r.peek() == '{') {
      GraphQLError err = parseSingleError(r);
      _errors.push_back(err);
    }
    skipWhitespace(r);
    if (r.peek() == ',') r.read();
  }
}

GraphQLError GraphQLRequest::parseSingleError(BufferedStreamReader& r) {
  GraphQLError err;
  skipWhitespace(r);
  if (r.read() != '{') return err;

  while (r.available()) {
    skipWhitespace(r);
    if (r.peek() == '}') {
      r.read();
      break;
    }
    if (r.peek() == '"') {
      String key = readStringToken(r);
      skipWhitespace(r);
      if (r.read() != ':') continue;
      skipWhitespace(r);

      if (key == "message") {
        if (r.peek() == '"') {
          err.message = readStringToken(r);
        } else {
          skipValue(r);
        }
      } else if (key == "locations") {
        if (r.peek() == '[') {
          r.read(); // consume '['
          while (r.available()) {
            skipWhitespace(r);
            if (r.peek() == ']') {
              r.read();
              break;
            }
            if (r.peek() == '{') {
              GraphQLLocation loc;
              parseLocation(r, loc);
              err.locations.push_back(loc);
            }
            skipWhitespace(r);
            if (r.peek() == ',') r.read();
          }
        } else {
          skipValue(r);
        }
      } else if (key == "path") {
        if (r.peek() == '[') {
          parsePath(r, err.path);
        } else {
          skipValue(r);
        }
      } else if (key == "extensions") {
        char extStart = (char)r.peek();
        if (extStart == '{' || extStart == '[') {
          r.read();
          readRawJsonFromReader(r, &err.extensions, extStart);
        } else {
          skipValue(r);
        }
      } else {
        skipValue(r);
      }

      skipWhitespace(r);
      if (r.peek() == ',') r.read();
    } else {
      r.read();
    }
  }
  return err;
}

void GraphQLRequest::parseLocation(BufferedStreamReader& r, GraphQLLocation& loc) {
  skipWhitespace(r);
  if (r.read() != '{') return;

  while (r.available()) {
    skipWhitespace(r);
    if (r.peek() == '}') {
      r.read();
      break;
    }
    if (r.peek() == '"') {
      String key = readStringToken(r);
      skipWhitespace(r);
      if (r.read() != ':') continue;
      skipWhitespace(r);

      char numBuf[32];
      size_t nIdx = 0;
      while (r.available()) {
        char b = (char)r.peek();
        if (isdigit((unsigned char)b) || b == '-') {
          if (nIdx < sizeof(numBuf) - 1) numBuf[nIdx++] = (char)r.read();
        } else {
          break;
        }
      }
      numBuf[nIdx] = '\0';
      int val = atoi(numBuf);

      if (key == "line") {
        loc.line = val;
      } else if (key == "column") {
        loc.column = val;
      }

      skipWhitespace(r);
      if (r.peek() == ',') r.read();
    } else {
      r.read();
    }
  }
}

void GraphQLRequest::parsePath(BufferedStreamReader& r, std::vector<String>& path) {
  r.read(); // consume '['
  while (r.available()) {
    skipWhitespace(r);
    if (r.peek() == ']') {
      r.read();
      break;
    }
    if (r.peek() == '"') {
      path.push_back(readStringToken(r));
    } else {
      char elemBuf[64];
      size_t eIdx = 0;
      while (r.available()) {
        char c = (char)r.peek();
        if (c == ',' || c == ']' || c == ' ' || c == '\t' || c == '\n' || c == '\r') break;
        if (eIdx < sizeof(elemBuf) - 1) elemBuf[eIdx++] = (char)r.read();
      }
      elemBuf[eIdx] = 0;
      if (eIdx > 0) path.push_back(String(elemBuf));
    }
    skipWhitespace(r);
    if (r.peek() == ',') r.read();
  }
}

void GraphQLRequest::parseMultipartResponse(BufferedStreamReader& r, const String& boundary) {
  String delimiter = "--" + boundary;
  bool isEnd = false;

  while (r.available() && !isEnd) {
    // 1. Advance to next boundary
    String line;
    while (r.available()) {
      char c = (char)r.read();
      if (c == '\n') {
        line.trim();
        if (line.startsWith(delimiter)) {
          if (line.startsWith(delimiter + "--")) {
            isEnd = true;
          }
          break;
        }
        line = "";
      } else if (c != '\r') {
        line += c;
      }
    }
    if (isEnd || !r.available()) break;

    // 2. Read MIME part headers until empty line
    while (r.available()) {
      line = "";
      while (r.available()) {
        char c = (char)r.read();
        if (c == '\n') break;
        if (c != '\r') line += c;
      }
      line.trim();
      if (line.isEmpty()) break; // headers finished
    }

    // 3. Read JSON payload of this part until boundary
    String partJson;
    partJson.reserve(512);
    while (r.available()) {
      skipWhitespace(r);
      if (r.peek() == '-') {
        // Potential boundary
        break;
      }
      char c = (char)r.peek();
      if (c == '{') {
        r.read();
        readRawJsonFromReader(r, &partJson, '{');
        break;
      } else {
        r.read();
      }
    }

    if (partJson.isEmpty()) continue;

    // 4. Parse chunk JSON
    BufferedStreamReader chunkReader(partJson.c_str());
    skipWhitespace(chunkReader);
    if (chunkReader.read() == '{') {
      GraphQLIncrementalPayload payload;
      bool hasNextVal = true;

      while (chunkReader.available()) {
        skipWhitespace(chunkReader);
        if (chunkReader.peek() == '}') {
          chunkReader.read();
          break;
        }
        if (chunkReader.peek() == '"') {
          String k = readStringToken(chunkReader);
          skipWhitespace(chunkReader);
          if (chunkReader.read() != ':') continue;
          skipWhitespace(chunkReader);

          if (k == "data") {
            char dStart = (char)chunkReader.peek();
            if (dStart == '{' || dStart == '[') {
              chunkReader.read();
              readRawJsonFromReader(chunkReader, &payload.data, dStart);
              if (!_responseBindings.empty()) {
                BufferedStreamReader dataR(payload.data.c_str());
                std::vector<ResponseBinding> strippedBindings;
                for (const auto& b : _responseBindings) {
                  if (strncmp(b.key, "data.", 5) == 0) {
                    strippedBindings.push_back({b.key + 5, b.target, b.type, b.size});
                  } else {
                    strippedBindings.push_back(b);
                  }
                }
                RestRequest::parseJsonWithBindings(dataR, strippedBindings);
              }
            } else {
              skipValue(chunkReader);
            }
          } else if (k == "incremental") {
            if (chunkReader.peek() == '[') {
              chunkReader.read(); // '['
              while (chunkReader.available()) {
                skipWhitespace(chunkReader);
                if (chunkReader.peek() == ']') {
                  chunkReader.read();
                  break;
                }
                if (chunkReader.peek() == '{') {
                  chunkReader.read(); // '{'
                  GraphQLIncrementalPayload inc;
                  while (chunkReader.available()) {
                    skipWhitespace(chunkReader);
                    if (chunkReader.peek() == '}') {
                      chunkReader.read();
                      break;
                    }
                    if (chunkReader.peek() == '"') {
                      String subK = readStringToken(chunkReader);
                      skipWhitespace(chunkReader);
                      if (chunkReader.read() != ':') continue;
                      skipWhitespace(chunkReader);

                      if (subK == "path") {
                        if (chunkReader.peek() == '[') {
                          std::vector<String> pList;
                          parsePath(chunkReader, pList);
                          String assembledPath;
                          for (size_t pi = 0; pi < pList.size(); pi++) {
                            if (pi > 0) assembledPath += ".";
                            assembledPath += pList[pi];
                          }
                          inc.path = assembledPath;
                        } else {
                          skipValue(chunkReader);
                        }
                      } else if (subK == "data") {
                        char incStart = (char)chunkReader.peek();
                        if (incStart == '{' || incStart == '[') {
                          chunkReader.read();
                          readRawJsonFromReader(chunkReader, &inc.data, incStart);
                        } else {
                          skipValue(chunkReader);
                        }
                      } else if (subK == "errors") {
                        parseErrors(chunkReader);
                        inc.errors = _errors;
                      } else {
                        skipValue(chunkReader);
                      }
                      skipWhitespace(chunkReader);
                      if (chunkReader.peek() == ',') chunkReader.read();
                    } else {
                      chunkReader.read();
                    }
                  }
                  if (!inc.data.isEmpty() && !_responseBindings.empty()) {
                    BufferedStreamReader incDataR(inc.data.c_str());
                    String prefix = "data";
                    if (!inc.path.isEmpty()) {
                      prefix += "." + inc.path;
                    }
                    std::vector<ResponseBinding> matchedBindings;
                    for (const auto& b : _responseBindings) {
                      if (strncmp(b.key, prefix.c_str(), prefix.length()) == 0) {
                        const char* relKey = b.key + prefix.length();
                        if (*relKey == '.') relKey++;
                        matchedBindings.push_back({relKey, b.target, b.type, b.size});
                      }
                    }
                    if (!matchedBindings.empty()) {
                      RestRequest::parseJsonWithBindings(incDataR, matchedBindings);
                    }
                  }
                  inc.hasNext = hasNextVal;
                  if (_onIncrementalCb) _onIncrementalCb(inc);
                }
                skipWhitespace(chunkReader);
                if (chunkReader.peek() == ',') chunkReader.read();
              }
            } else {
              skipValue(chunkReader);
            }
          } else if (k == "hasNext") {
            char valBuf[16];
            size_t vi = 0;
            while (chunkReader.available() && isalpha((unsigned char)chunkReader.peek())) {
              if (vi < sizeof(valBuf) - 1) valBuf[vi++] = (char)chunkReader.read();
            }
            valBuf[vi] = 0;
            hasNextVal = (strcmp(valBuf, "true") == 0);
            payload.hasNext = hasNextVal;
          } else if (k == "errors") {
            parseErrors(chunkReader);
            payload.errors = _errors;
          } else {
            skipValue(chunkReader);
          }
          skipWhitespace(chunkReader);
          if (chunkReader.peek() == ',') chunkReader.read();
        } else {
          chunkReader.read();
        }
      }

      if (!payload.data.isEmpty()) {
        payload.hasNext = hasNextVal;
        if (_onIncrementalCb) _onIncrementalCb(payload);
      }
      if (!hasNextVal) {
        isEnd = true;
      }
    }
  }
}
