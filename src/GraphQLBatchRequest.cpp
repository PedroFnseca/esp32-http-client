#include "GraphQLBatchRequest.h"

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

GraphQLBatchRequest::GraphQLBatchRequest(ESP32HTTPClient* client, const char* path)
    : _client(client),
      _path(path),
      _executed(false),
      _timeout(0),
      _maxRetry(-1),
      _rawResponseTarget(nullptr),
      _onSuccessCb(nullptr),
      _onErrorCb(nullptr),
      _onResponseCb(nullptr),
      _onMultiErrorCb(nullptr) {
}

GraphQLBatchRequest::GraphQLBatchRequest(GraphQLBatchRequest&& other)
    : _client(other._client),
      _path(other._path),
      _executed(other._executed),
      _timeout(other._timeout),
      _maxRetry(other._maxRetry),
      _acceptHeader(std::move(other._acceptHeader)),
      _customHeaders(std::move(other._customHeaders)),
      _operations(std::move(other._operations)),
      _rawResponseTarget(other._rawResponseTarget),
      _onSuccessCb(std::move(other._onSuccessCb)),
      _onErrorCb(std::move(other._onErrorCb)),
      _onResponseCb(std::move(other._onResponseCb)),
      _onMultiErrorCb(std::move(other._onMultiErrorCb)) {
  other._executed = true;
}

GraphQLBatchRequest::~GraphQLBatchRequest() {
  if (!_executed) {
    execute();
  }
}

GraphQLRequest& GraphQLBatchRequest::add() {
  GraphQLRequest req(_client, _path, HTTP_POST_METHOD);
  req._isBatchMember = true;
  _operations.push_back(std::move(req));
  return _operations.back();
}

GraphQLRequest& GraphQLBatchRequest::addQuery(const char* queryDocument) {
  GraphQLRequest& req = add();
  req.query(queryDocument);
  return req;
}

GraphQLRequest& GraphQLBatchRequest::addMutation(const char* mutationDocument) {
  GraphQLRequest& req = add();
  req.mutation(mutationDocument);
  return req;
}

size_t GraphQLBatchRequest::size() const {
  return _operations.size();
}

GraphQLRequest& GraphQLBatchRequest::operation(size_t index) {
  return _operations.at(index);
}

GraphQLBatchRequest& GraphQLBatchRequest::header(const char* name, const char* value) {
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

GraphQLBatchRequest& GraphQLBatchRequest::accept(const char* acceptHeader) {
  _acceptHeader = acceptHeader ? acceptHeader : "";
  return *this;
}

GraphQLBatchRequest& GraphQLBatchRequest::timeout(uint16_t timeoutMs) {
  _timeout = timeoutMs;
  return *this;
}

GraphQLBatchRequest& GraphQLBatchRequest::maxRetry(int maxRetry) {
  _maxRetry = (maxRetry < 0) ? 0 : maxRetry;
  return *this;
}

GraphQLBatchRequest& GraphQLBatchRequest::retry(int maxRetry) {
  return this->maxRetry(maxRetry);
}

GraphQLBatchRequest& GraphQLBatchRequest::getRawResponse(String* target) {
  _rawResponseTarget = target;
  return *this;
}

GraphQLBatchRequest& GraphQLBatchRequest::onSuccess(HttpResponseCallback cb) {
  _onSuccessCb = cb;
  return *this;
}

GraphQLBatchRequest& GraphQLBatchRequest::onError(HttpErrorCallback cb) {
  _onErrorCb = cb;
  return *this;
}

GraphQLBatchRequest& GraphQLBatchRequest::onError(HttpResponseCallback cb) {
  if (cb) {
    _onErrorCb = [cb](int code, const char*) { cb(code); };
  } else {
    _onErrorCb = nullptr;
  }
  return *this;
}

GraphQLBatchRequest& GraphQLBatchRequest::onResponse(HttpResponseCallback cb) {
  _onResponseCb = cb;
  return *this;
}

GraphQLBatchRequest& GraphQLBatchRequest::onGraphQLError(GraphQLMultiErrorCallback cb) {
  _onMultiErrorCb = cb;
  return *this;
}

String GraphQLBatchRequest::buildRequestBody() const {
  String body;
  body.reserve(256 + _operations.size() * 128);
  body += "[";
  for (size_t i = 0; i < _operations.size(); i++) {
    body += _operations[i].buildRequestBody();
    if (i < _operations.size() - 1) {
      body += ",";
    }
  }
  body += "]";
  return body;
}

void GraphQLBatchRequest::execute() {
  _executed = true;
  if (!_client || _operations.empty()) return;

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
  String url = urlBase + (_path ? _path : "");
  String payload = buildRequestBody();

  auto setupConnection = [&]() {
    http.begin(url);
    for (const auto& header : _client->_headers) {
      http.addHeader(header.name, header.value);
    }
    for (const auto& header : _customHeaders) {
      http.addHeader(header.name, header.value);
    }
    String acceptVal = _acceptHeader.isEmpty() ? GRAPHQL_ACCEPT_HEADER : _acceptHeader;
    http.addHeader("Accept", acceptVal.c_str());
    http.addHeader("Content-Type", GRAPHQL_CONTENT_TYPE_JSON);
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
    code = http.POST(payload);
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
    if (http.getSize() > 0 || http.getStreamPtr()) {
      bool isChunked = (http.getSize() == -1);
      BufferedStreamReader reader(http.getStreamPtr(), isChunked);
      parseBatchResponse(reader);
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

  // Aggregate errors across all operations
  std::vector<GraphQLError> allErrors;
  for (auto& op : _operations) {
    if (op.hasGraphQLErrors()) {
      for (const auto& err : op.getErrors()) {
        allErrors.push_back(err);
      }
    }
  }
  if (!allErrors.empty() && _onMultiErrorCb) {
    _onMultiErrorCb(allErrors);
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

void GraphQLBatchRequest::parseBatchResponse(BufferedStreamReader& r) {
  if (_rawResponseTarget) {
    _rawResponseTarget->reserve(512);
    while (r.available()) {
      *_rawResponseTarget += (char)r.read();
    }
    BufferedStreamReader strReader(_rawResponseTarget->c_str());
    skipWhitespace(strReader);
    if (strReader.peek() != '[') return;
    strReader.read(); // consume '['

    size_t opIdx = 0;
    while (strReader.available()) {
      skipWhitespace(strReader);
      if (strReader.peek() == ']') {
        strReader.read();
        break;
      }
      char c = (char)strReader.peek();
      if (c == '{') {
        strReader.read();
        String itemJson;
        readRawJsonFromReader(strReader, &itemJson, '{');
        if (opIdx < _operations.size()) {
          BufferedStreamReader itemReader(itemJson.c_str());
          _operations[opIdx].parseSingleResponse(itemReader);
          if (_operations[opIdx].hasGraphQLErrors()) {
            if (_operations[opIdx]._firstErrorTarget) {
              *_operations[opIdx]._firstErrorTarget = _operations[opIdx]._errors[0];
            }
            if (_operations[opIdx]._allErrorsTarget) {
              *_operations[opIdx]._allErrorsTarget = _operations[opIdx]._errors;
            }
            if (_operations[opIdx]._errorMessageTarget) {
              *_operations[opIdx]._errorMessageTarget = _operations[opIdx]._errors[0].message;
            }
            if (_operations[opIdx]._onSingleErrorCb) {
              _operations[opIdx]._onSingleErrorCb(_operations[opIdx]._errors[0]);
            }
            if (_operations[opIdx]._onMultiErrorCb) {
              _operations[opIdx]._onMultiErrorCb(_operations[opIdx]._errors);
            }
          }
        }
        opIdx++;
      } else {
        strReader.read();
      }
      skipWhitespace(strReader);
      if (strReader.peek() == ',') strReader.read();
    }
    return;
  }

  skipWhitespace(r);
  if (r.peek() != '[') return;
  r.read(); // consume '['

  size_t opIdx = 0;
  while (r.available()) {
    skipWhitespace(r);
    if (r.peek() == ']') {
      r.read();
      break;
    }
    char c = (char)r.peek();
    if (c == '{') {
      r.read();
      String itemJson;
      readRawJsonFromReader(r, &itemJson, '{');
      if (opIdx < _operations.size()) {
        BufferedStreamReader itemReader(itemJson.c_str());
        _operations[opIdx].parseSingleResponse(itemReader);
        if (_operations[opIdx].hasGraphQLErrors()) {
          if (_operations[opIdx]._firstErrorTarget) {
            *_operations[opIdx]._firstErrorTarget = _operations[opIdx]._errors[0];
          }
          if (_operations[opIdx]._allErrorsTarget) {
            *_operations[opIdx]._allErrorsTarget = _operations[opIdx]._errors;
          }
          if (_operations[opIdx]._errorMessageTarget) {
            *_operations[opIdx]._errorMessageTarget = _operations[opIdx]._errors[0].message;
          }
          if (_operations[opIdx]._onSingleErrorCb) {
            _operations[opIdx]._onSingleErrorCb(_operations[opIdx]._errors[0]);
          }
          if (_operations[opIdx]._onMultiErrorCb) {
            _operations[opIdx]._onMultiErrorCb(_operations[opIdx]._errors);
          }
        }
      }
      opIdx++;
    } else {
      r.read();
    }
    skipWhitespace(r);
    if (r.peek() == ',') r.read();
  }
}
