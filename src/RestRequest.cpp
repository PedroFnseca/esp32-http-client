#include "RestRequest.h"

#include <HTTPClient.h>

#include "ESP32HTTPClient.h"
#include "HttpEncoding.h"

namespace {

void skipWhitespace(Stream* stream) {
  while (stream->available()) {
    char c = stream->peek();
    if (isspace(c))
      stream->read();
    else
      break;
  }
}

void readStringIntoBuffer(Stream* stream, char* buffer, size_t maxLen) {
  size_t idx = 0;
  if (stream->read() != '"') {
    if (maxLen > 0) buffer[0] = 0;
    return;
  }

  while (stream->available()) {
    char c = stream->read();
    if (c == '\\') {
      if (stream->available()) {
        char escaped = stream->read();
        if (idx + 1 < maxLen) buffer[idx++] = escaped;
      }
    } else if (c == '"') {
      break;
    } else if (idx + 1 < maxLen) {
      buffer[idx++] = c;
    }
  }

  if (maxLen > 0) buffer[idx] = 0;
}

void skipValue(Stream* stream) {
  skipWhitespace(stream);
  if (!stream->available()) return;

  char c = stream->read();

  if (c == '"') {
    while (stream->available()) {
      char ch = stream->read();
      if (ch == '\\') {
        if (stream->available()) stream->read();
      } else if (ch == '"') {
        break;
      }
    }
  } else if (c == '{') {
    int depth = 1;
    while (depth > 0 && stream->available()) {
      char ch = stream->read();
      if (ch == '"') {
        while (stream->available()) {
          char sc = stream->read();
          if (sc == '\\') {
            if (stream->available()) stream->read();
          } else if (sc == '"') {
            break;
          }
        }
      } else if (ch == '{') {
        depth++;
      } else if (ch == '}') {
        depth--;
      }
    }
  } else if (c == '[') {
    int depth = 1;
    while (depth > 0 && stream->available()) {
      char ch = stream->read();
      if (ch == '"') {
        while (stream->available()) {
          char sc = stream->read();
          if (sc == '\\') {
            if (stream->available()) stream->read();
          } else if (sc == '"') {
            break;
          }
        }
      } else if (ch == '[') {
        depth++;
      } else if (ch == ']') {
        depth--;
      }
    }
  } else {
    while (stream->available()) {
      char ch = stream->peek();
      if (ch == ',' || ch == '}' || ch == ']' || isspace(ch)) break;
      stream->read();
    }
  }
}

}  // namespace

RestRequest::RestRequest(ESP32HTTPClient* client, const char* path, HttpMethod method)
    : _client(client), _path(path), _method(method), _executed(false) {
}

RestRequest::RestRequest(RestRequest&& other)
    : _client(other._client),
      _path(other._path),
      _method(other._method),
      _executed(other._executed),
      _queryParams(std::move(other._queryParams)),
      _bodyParams(std::move(other._bodyParams)),
      _responseBindings(std::move(other._responseBindings)) {
  other._executed = true;
}

RestRequest::~RestRequest() {
  if (!_executed) {
    execute();
  }
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

int RestRequest::send() {
  return execute();
}

int RestRequest::execute() {
  if (_executed) {
    return _client ? _client->_lastStatusCode : 0;
  }

  _executed = true;
  if (!_client) return 0;

  HTTPClient http;
  String url = String(http_encoding::buildUrl(_client->_baseUrl.c_str(), _client->_port, _path ? _path : "").c_str());

  if (!_queryParams.empty()) {
    url += "?";
    for (size_t i = 0; i < _queryParams.size(); i++) {
      url += String(http_encoding::urlEncode(_queryParams[i].key.c_str()).c_str());
      url += "=";
      url += String(http_encoding::urlEncode(_queryParams[i].value.c_str()).c_str());
      if (i + 1 < _queryParams.size()) url += "&";
    }
  }

  http.begin(url);

  for (const auto& header : _client->_headers) {
    if (header.name.length() > 0) {
      http.addHeader(header.name, header.value);
    }
  }

  String payload;
  if (!_bodyParams.empty()) {
    http.addHeader("Content-Type", _client->_contentType);
    payload += "{";
    for (size_t i = 0; i < _bodyParams.size(); i++) {
      payload += "\"";
      payload += String(http_encoding::escapeJson(_bodyParams[i].key.c_str()).c_str());
      payload += "\":";

      if (_bodyParams[i].quoteValue) {
        payload += "\"";
        payload += String(http_encoding::escapeJson(_bodyParams[i].value.c_str()).c_str());
        payload += "\"";
      } else {
        payload += _bodyParams[i].value;
      }

      if (i + 1 < _bodyParams.size()) payload += ",";
    }
    payload += "}";
  }

  int code = 0;
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

  _client->_lastStatusCode = code;

  if (code > 0 && http.getStreamPtr()) {
    parseResponse(http.getStreamPtr());
  }

  http.end();
  return code;
}

void RestRequest::parseResponse(Stream* stream) {
  if (_responseBindings.empty()) return;

  skipWhitespace(stream);
  if (stream->read() != '{') return;

  char keyBuffer[64];
  char valueBuffer[128];

  while (stream->available()) {
    skipWhitespace(stream);
    char c = stream->read();

    if (c == '}') break;
    if (c != '"') continue;

    size_t kIdx = 0;
    while (stream->available()) {
      char k = stream->read();
      if (k == '"') break;
      if (kIdx < sizeof(keyBuffer) - 1) keyBuffer[kIdx++] = k;
    }
    keyBuffer[kIdx] = 0;

    skipWhitespace(stream);
    if (stream->read() != ':') continue;

    ResponseBinding* match = nullptr;
    for (auto& binding : _responseBindings) {
      if (strcmp(keyBuffer, binding.key) == 0) {
        match = &binding;
        break;
      }
    }

    if (!match) {
      skipValue(stream);
      skipWhitespace(stream);
      if (stream->peek() == ',') stream->read();
      continue;
    }

    skipWhitespace(stream);
    char nextChar = stream->peek();

    if (nextChar == '"') {
      if (match->type == TYPE_STRING) {
        readStringIntoBuffer(stream, (char*)match->target, match->size);
      } else {
        readStringIntoBuffer(stream, valueBuffer, sizeof(valueBuffer));
        if (match->type == TYPE_INT)
          *(int*)match->target = atoi(valueBuffer);
        else if (match->type == TYPE_FLOAT)
          *(float*)match->target = strtof(valueBuffer, nullptr);
        else if (match->type == TYPE_DOUBLE)
          *(double*)match->target = strtod(valueBuffer, nullptr);
        else if (match->type == TYPE_LONG)
          *(long*)match->target = atol(valueBuffer);
      }
    } else if (nextChar == 't' || nextChar == 'f') {
      size_t bIdx = 0;
      while (stream->available()) {
        char b = stream->peek();
        if (!isalpha(b)) break;
        if (bIdx < sizeof(valueBuffer) - 1) valueBuffer[bIdx++] = stream->read();
        else stream->read();
      }
      valueBuffer[bIdx] = 0;
      if (match->type == TYPE_BOOL) *(bool*)match->target = strcmp(valueBuffer, "true") == 0;
    } else if (nextChar == 'n') {
      skipValue(stream);
    } else {
      size_t nIdx = 0;
      while (stream->available()) {
        char b = stream->peek();
        if (isdigit(b) || b == '.' || b == '-' || b == '+' || b == 'e' || b == 'E') {
          if (nIdx < sizeof(valueBuffer) - 1) valueBuffer[nIdx++] = stream->read();
          else stream->read();
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
    }

    skipWhitespace(stream);
    if (stream->peek() == ',') stream->read();
  }
}
