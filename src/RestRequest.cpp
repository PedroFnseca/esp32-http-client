#include "RestRequest.h"

#include <HTTPClient.h>

#include "ESP32HTTPClient.h"

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
        if (idx < maxLen - 1) buffer[idx++] = escaped;
      }
    } else if (c == '"') {
      break;
    } else {
      if (idx < maxLen - 1) buffer[idx++] = c;
    }
  }
  buffer[idx] = 0;
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
          } else if (sc == '"')
            break;
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
          } else if (sc == '"')
            break;
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
      if (ch == ',' || ch == '}' || ch == ']' || isspace(ch)) {
        break;
      }
      stream->read();
    }
  }
}

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

void RestRequest::execute() {
  _executed = true;

  if (!_client) return;

  HTTPClient http;
  String urlBase = String(_client->_baseUrl);

  if (_client->_port != 0) {
    int protoEnd = urlBase.indexOf("://");
    int startSearch = (protoEnd != -1) ? protoEnd + 3 : 0;
    int slashPos = urlBase.indexOf("/", startSearch);

    if (slashPos != -1) {
      urlBase = urlBase.substring(0, slashPos) + ":" + String(_client->_port) + urlBase.substring(slashPos);
    } else {
      urlBase += ":" + String(_client->_port);
    }
  }

  String url = urlBase + String(_path);

  if (!_queryParams.empty()) {
    url += "?";
    for (size_t i = 0; i < _queryParams.size(); i++) {
      url += _queryParams[i].key;
      url += "=";
      url += _queryParams[i].valueBuffer;
      if (i < _queryParams.size() - 1) url += "&";
    }
  }

  http.begin(url);

  String payload = "";
  if (!_bodyParams.empty()) {
    http.addHeader("Content-Type", "application/json");
    payload += "{";
    for (size_t i = 0; i < _bodyParams.size(); i++) {
      payload += "\"";
      payload += _bodyParams[i].key;
      payload += "\":";

      if (_bodyParams[i].quoteValue) {
        payload += "\"";
        payload += _bodyParams[i].valueBuffer;
        payload += "\"";
      } else {
        payload += _bodyParams[i].valueBuffer;
      }

      if (i < _bodyParams.size() - 1) payload += ",";
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

  if (code > 0) {
    if (http.getSize() > 0 || http.getStreamPtr()) {
      parseResponse(http.getStreamPtr());
    }
  }

  http.end();
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
    if (c == '"') {
      size_t kIdx = 0;
      while (stream->available()) {
        char k = stream->read();
        if (k == '"') break;
        if (kIdx < 63) keyBuffer[kIdx++] = k;
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

      if (match) {
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
              *(float*)match->target = atof(valueBuffer);
          }
        } else if (nextChar == 't' || nextChar == 'f') {
          size_t bIdx = 0;
          while (stream->available()) {
            char b = stream->peek();
            if (isalpha(b)) {
              char x = stream->read();
              if (bIdx < 10) valueBuffer[bIdx++] = x;
            } else
              break;
          }
          valueBuffer[bIdx] = 0;
          bool bVal = (strcmp(valueBuffer, "true") == 0);
          if (match->type == TYPE_BOOL) *(bool*)match->target = bVal;
        } else {
          size_t nIdx = 0;
          while (stream->available()) {
            char b = stream->peek();
            if (isdigit(b) || b == '.' || b == '-') {
              char x = stream->read();
              if (nIdx < 63) valueBuffer[nIdx++] = x;
            } else
              break;
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
      } else {
        skipValue(stream);
      }

      skipWhitespace(stream);
      if (stream->peek() == ',') stream->read();
    }
  }
}
