#include "RestRequest.h"

#include <HTTPClient.h>

#include "ESP32HTTPClient.h"
#include "BufferedStreamReader.h"

static void skipWhitespace(BufferedStreamReader& r) {
  while (r.available()) {
    char c = (char)r.peek();
    if (isspace((unsigned char)c))
      r.read();
    else
      break;
  }
}

static void readStringIntoBuffer(BufferedStreamReader& r, char* buffer, size_t maxLen) {
  size_t idx = 0;
  if (r.read() != '"') {
    if (maxLen > 0) buffer[0] = 0;
    return;
  }

  while (r.available()) {
    char c = (char)r.read();
    if (c == '\\') {
      if (r.available()) {
        char escaped = (char)r.read();
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

static void skipValue(BufferedStreamReader& r) {
  skipWhitespace(r);
  if (!r.available()) return;

  char c = (char)r.read();

  if (c == '"') {
    while (r.available()) {
      char ch = (char)r.read();
      if (ch == '\\') {
        if (r.available()) r.read();
      } else if (ch == '"') {
        break;
      }
    }
  } else if (c == '{') {
    int depth = 1;
    while (depth > 0 && r.available()) {
      char ch = (char)r.read();
      if (ch == '"') {
        while (r.available()) {
          char sc = (char)r.read();
          if (sc == '\\') {
            if (r.available()) r.read();
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
    while (depth > 0 && r.available()) {
      char ch = (char)r.read();
      if (ch == '"') {
        while (r.available()) {
          char sc = (char)r.read();
          if (sc == '\\') {
            if (r.available()) r.read();
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
    while (r.available()) {
      char ch = (char)r.peek();
      if (ch == ',' || ch == '}' || ch == ']' || isspace((unsigned char)ch)) {
        break;
      }
      r.read();
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

RestRequest& RestRequest::getBody(const char* key, String* target) {
  ResponseBinding binding;
  binding.key    = key;
  binding.target = target;
  binding.type   = TYPE_ARDUINO_STRING;
  binding.size   = 0;
  _responseBindings.push_back(binding);
  return *this;
}

void RestRequest::execute() {
  _executed = true;

  if (!_client) return;

  HTTPClient& http = _client->_http;
  // HTTP/1.1 is used to allow Connection: keep-alive.
  // The BufferedStreamReader now handles Transfer-Encoding: chunked automatically.

  String urlBase;
  urlBase.reserve(128);
  urlBase = _client->_baseUrl;

  if (_client->_port != 0) {
    int protoEnd    = urlBase.indexOf("://");
    int startSearch = (protoEnd != -1) ? protoEnd + 3 : 0;
    int slashPos    = urlBase.indexOf("/", startSearch);

    if (slashPos != -1) {
      urlBase = urlBase.substring(0, slashPos) + ":" + String(_client->_port) + urlBase.substring(slashPos);
    } else {
      urlBase += ":" + String(_client->_port);
    }
  }

  String url;
  url.reserve(256);
  url = urlBase;
  url += _path;

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

  for (const auto& header : _client->_headers) {
    http.addHeader(header.name, header.value);
  }

  String payload = "";
  if (!_bodyParams.empty()) {
    payload.reserve(_bodyParams.size() * 64);
    http.addHeader("Content-Type", _client->_contentType);
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
  int retries = 0;

  while (retries < 2) {
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

    // If connection fails (code < 0), it might be a stale Keep-Alive connection.
    // Close the socket, re-initiate the connection, and retry once.
    if (code < 0 && retries == 0) {
      http.end();
      http.begin(url);
      for (const auto& header : _client->_headers) {
        http.addHeader(header.name, header.value);
      }
      if (!_bodyParams.empty()) {
        http.addHeader("Content-Type", _client->_contentType);
      }
      retries++;
    } else {
      break;
    }
  }

  _client->_lastStatusCode = code;

  if (code > 0) {
    if (http.getSize() > 0 || http.getStreamPtr()) {
      bool isChunked = (http.getSize() == -1);
      BufferedStreamReader reader(http.getStreamPtr(), isChunked);
      parseResponse(reader);
    }
  }

  http.end();
}

void RestRequest::parseResponse(BufferedStreamReader& r) {
  if (_responseBindings.empty()) return;

  skipWhitespace(r);
  char c = (char)r.read();

  if (c == '{') {
    ResponseBinding* match = nullptr;
    for (auto& binding : _responseBindings) {
      if (binding.key[0] == '\0') {
        match = &binding;
        break;
      }
    }
    if (match && match->type == TYPE_ARDUINO_STRING) {
      readRawJsonIntoString(r, (String*)match->target, '{');
    } else {
      parseObject(r, "");
    }
  } else if (c == '[') {
    ResponseBinding* match = nullptr;
    for (auto& binding : _responseBindings) {
      if (binding.key[0] == '\0') {
        match = &binding;
        break;
      }
    }
    if (match && match->type == TYPE_ARDUINO_STRING) {
      readRawJsonIntoString(r, (String*)match->target, '[');
    } else {
      parseArray(r, "");
    }
  }
}

void RestRequest::readRawJsonIntoString(BufferedStreamReader& r, String* target, char openingBrace) {
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

void RestRequest::parsePrimitive(BufferedStreamReader& r, ResponseBinding* match) {
  char valueBuffer[128];
  char nextChar = (char)r.peek();

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
        *(float*)match->target = atof(valueBuffer);
    }
  } else if (nextChar == 't' || nextChar == 'f') {
    size_t bIdx = 0;
    while (r.available()) {
      char b = (char)r.peek();
      if (isalpha((unsigned char)b)) {
        char x = (char)r.read();
        if (bIdx < 10) valueBuffer[bIdx++] = x;
      } else
        break;
    }
    valueBuffer[bIdx] = 0;
    bool bVal = (strcmp(valueBuffer, "true") == 0);
    if (match->type == TYPE_BOOL)           *(bool*)match->target   = bVal;
    if (match->type == TYPE_ARDUINO_STRING) *((String*)match->target) = valueBuffer;
  } else {
    size_t nIdx = 0;
    while (r.available()) {
      char b = (char)r.peek();
      if (isdigit((unsigned char)b) || b == '.' || b == '-') {
        char x = (char)r.read();
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
    else if (match->type == TYPE_ARDUINO_STRING)
      *((String*)match->target) = valueBuffer;
  }
}

void RestRequest::parseObject(BufferedStreamReader& r, const char* basePath) {
  char keyBuffer[64];
  char fullPath[128];

  while (r.available()) {
    skipWhitespace(r);
    char c = (char)r.read();

    if (c == '}') break;
    if (c == '"') {
      size_t kIdx = 0;
      while (r.available()) {
        char k = (char)r.read();
        if (k == '"') break;
        if (kIdx < 63) keyBuffer[kIdx++] = k;
      }
      keyBuffer[kIdx] = 0;

      if (basePath[0] == '\0') {
        strncpy(fullPath, keyBuffer, sizeof(fullPath) - 1);
        fullPath[sizeof(fullPath) - 1] = 0;
      } else {
        snprintf(fullPath, sizeof(fullPath), "%s.%s", basePath, keyBuffer);
      }

      skipWhitespace(r);
      if (r.read() != ':') continue;

      skipWhitespace(r);
      char nextChar = (char)r.peek();

      if (nextChar == '{' || nextChar == '[') {
        ResponseBinding* match = nullptr;
        for (auto& binding : _responseBindings) {
          if (strcmp(fullPath, binding.key) == 0) {
            match = &binding;
            break;
          }
        }

        if (match && match->type == TYPE_ARDUINO_STRING) {
          r.read();  // consume '{' or '['
          readRawJsonIntoString(r, (String*)match->target, nextChar);
        } else {
          r.read();
          if (nextChar == '{') {
            parseObject(r, fullPath);
          } else {
            parseArray(r, fullPath);
          }
        }
      } else {
        ResponseBinding* match = nullptr;
        for (auto& binding : _responseBindings) {
          if (strcmp(fullPath, binding.key) == 0) {
            match = &binding;
            break;
          }
        }

        if (match) {
          parsePrimitive(r, match);
        } else {
          skipValue(r);
        }
      }

      skipWhitespace(r);
      if (r.peek() == ',') r.read();
    }
  }
}

void RestRequest::parseArray(BufferedStreamReader& r, const char* basePath) {
  int  index = 0;
  char fullPath[128];

  while (r.available()) {
    skipWhitespace(r);
    char nextChar = (char)r.peek();

    if (nextChar == ']') {
      r.read();
      break;
    }

    if (basePath[0] == '\0') {
      snprintf(fullPath, sizeof(fullPath), "%d", index);
    } else {
      snprintf(fullPath, sizeof(fullPath), "%s.%d", basePath, index);
    }

    if (nextChar == '{' || nextChar == '[') {
      ResponseBinding* match = nullptr;
      for (auto& binding : _responseBindings) {
        if (strcmp(fullPath, binding.key) == 0) {
          match = &binding;
          break;
        }
      }

      if (match && match->type == TYPE_ARDUINO_STRING) {
        r.read();  // consume '{' or '['
        readRawJsonIntoString(r, (String*)match->target, nextChar);
      } else {
        r.read();
        if (nextChar == '{') {
          parseObject(r, fullPath);
        } else {
          parseArray(r, fullPath);
        }
      }
    } else {
      ResponseBinding* match = nullptr;
      for (auto& binding : _responseBindings) {
        if (strcmp(fullPath, binding.key) == 0) {
          match = &binding;
          break;
        }
      }

      if (match) {
        parsePrimitive(r, match);
      } else {
        skipValue(r);
      }
    }

    skipWhitespace(r);
    if (r.peek() == ',') {
      r.read();
    }
    index++;
  }
}
