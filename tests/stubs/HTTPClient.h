#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "Arduino.h"

#include <deque>
#include <string>
#include <vector>
#include <utility>

#define HTTPC_ERROR_CONNECTION_REFUSED  (-1)
#define HTTPC_ERROR_SEND_HEADER_FAILED  (-2)
#define HTTPC_ERROR_SEND_PAYLOAD_FAILED (-3)
#define HTTPC_ERROR_NOT_CONNECTED       (-4)
#define HTTPC_ERROR_CONNECTION_LOST     (-5)
#define HTTPC_ERROR_NO_STREAM           (-6)
#define HTTPC_ERROR_NO_HTTP_SERVER      (-7)
#define HTTPC_ERROR_TOO_LESS_RAM        (-8)
#define HTTPC_ERROR_ENCODING            (-9)
#define HTTPC_ERROR_STREAM_WRITE        (-10)
#define HTTPC_ERROR_READ_TIMEOUT        (-11)

namespace HttpClientStub {
inline std::string lastUrl;
inline std::string lastPayload;
inline std::string lastMethod;
inline std::vector<std::pair<std::string, std::string>> lastHeaders;
inline int lastTimeout = 0;
inline int requestCount = 0;
inline int nextStatusCode = 200;
inline std::string nextResponseBody = "{}";
inline std::deque<std::pair<int, std::string>> responseQueue;
inline std::vector<std::pair<std::string, std::string>> nextResponseHeaders;
inline std::deque<std::vector<std::pair<std::string, std::string>>> responseHeaderQueue;
inline std::vector<std::string> collectedHeaderKeys;
inline Stream* customStream = nullptr;
inline int customSize = 0;

class InMemoryStream : public Stream {
 public:
  explicit InMemoryStream(const std::string& data) : _data(data), _position(0) {
  }

  int available() override {
    return _position < _data.size() ? static_cast<int>(_data.size() - _position) : 0;
  }

  int read() override {
    if (!available()) return -1;
    return static_cast<unsigned char>(_data[_position++]);
  }

  int peek() override {
    if (!available()) return -1;
    return static_cast<unsigned char>(_data[_position]);
  }

 private:
  std::string _data;
  size_t _position;
};

inline void reset() {
  lastUrl.clear();
  lastPayload.clear();
  lastMethod.clear();
  lastHeaders.clear();
  lastTimeout = 0;
  requestCount = 0;
  nextStatusCode = 200;
  nextResponseBody = "{}";
  responseQueue.clear();
  nextResponseHeaders.clear();
  responseHeaderQueue.clear();
  collectedHeaderKeys.clear();
  customStream = nullptr;
  customSize = 0;
}

inline void setResponse(int statusCode, const std::string& body) {
  nextStatusCode = statusCode;
  nextResponseBody = body;
  responseQueue.clear();
}

inline void setResponseHeaders(const std::vector<std::pair<std::string, std::string>>& headers) {
  nextResponseHeaders = headers;
  responseHeaderQueue.clear();
}

inline void queueResponse(int statusCode, const std::string& body) {
  responseQueue.push_back({statusCode, body});
}

inline void queueResponseHeaders(const std::vector<std::pair<std::string, std::string>>& headers) {
  responseHeaderQueue.push_back(headers);
}

inline int getNextStatusAndBody(std::string& body, std::vector<std::pair<std::string, std::string>>& headers) {
  requestCount++;
  if (!responseHeaderQueue.empty()) {
    headers = responseHeaderQueue.front();
    responseHeaderQueue.pop_front();
  } else {
    headers = nextResponseHeaders;
  }

  if (!responseQueue.empty()) {
    auto item = responseQueue.front();
    responseQueue.pop_front();
    body = item.second;
    return item.first;
  }
  body = nextResponseBody;
  return nextStatusCode;
}
}  // namespace HttpClientStub

class HTTPClient {
 public:
  HTTPClient() : _stream(HttpClientStub::nextResponseBody) {
  }

  bool begin(String url) {
    HttpClientStub::lastUrl = url.c_str();
    return true;
  }

  void useHTTP10(bool usehttp10 = true) {}

  void setReuse(bool reuse) {}

  void setTimeout(uint16_t timeout) {
    HttpClientStub::lastTimeout = timeout;
  }

  void addHeader(const String& name, const String& value) {
    HttpClientStub::lastHeaders.push_back({name.str(), value.str()});
  }

  void collectHeaders(const char* headerKeys[], const size_t headerKeysCount) {
    HttpClientStub::collectedHeaderKeys.clear();
    _collectedKeys.clear();
    for (size_t i = 0; i < headerKeysCount; i++) {
      if (headerKeys[i]) {
        HttpClientStub::collectedHeaderKeys.push_back(headerKeys[i]);
        _collectedKeys.push_back(headerKeys[i]);
      }
    }
  }

  bool hasHeader(const char* name) {
    if (!name) return false;
    for (const auto& h : _currentHeaders) {
      if (strcasecmp(h.first.c_str(), name) == 0) {
        return true;
      }
    }
    return false;
  }

  String header(const char* name) {
    if (!name) return "";
    for (const auto& h : _currentHeaders) {
      if (strcasecmp(h.first.c_str(), name) == 0) {
        return String(h.second.c_str());
      }
    }
    return "";
  }

  String header(String name) {
    return header(name.c_str());
  }

  String header(size_t i) {
    if (i < _currentHeaders.size()) {
      return String(_currentHeaders[i].second.c_str());
    }
    return "";
  }

  String headerName(size_t i) {
    if (i < _currentHeaders.size()) {
      return String(_currentHeaders[i].first.c_str());
    }
    return "";
  }

  int headers() {
    return static_cast<int>(_currentHeaders.size());
  }

  int GET() {
    HttpClientStub::lastMethod = "GET";
    HttpClientStub::lastPayload.clear();
    return executeRequest();
  }

  int POST(const String& payload) {
    HttpClientStub::lastMethod = "POST";
    HttpClientStub::lastPayload = payload.str();
    return executeRequest();
  }

  int PUT(const String& payload) {
    HttpClientStub::lastMethod = "PUT";
    HttpClientStub::lastPayload = payload.str();
    return executeRequest();
  }

  int PATCH(const String& payload) {
    HttpClientStub::lastMethod = "PATCH";
    HttpClientStub::lastPayload = payload.str();
    return executeRequest();
  }

  int sendRequest(const char* method, const String& payload) {
    HttpClientStub::lastMethod = method ? method : "";
    HttpClientStub::lastPayload = payload.str();
    return executeRequest();
  }

  int getSize() {
    if (HttpClientStub::customStream) return HttpClientStub::customSize;
    return static_cast<int>(HttpClientStub::nextResponseBody.size());
  }

  Stream* getStreamPtr() {
    if (HttpClientStub::customStream) return HttpClientStub::customStream;
    return &_stream;
  }

  void end() {
    _currentHeaders.clear();
  }

  static String errorToString(int error) {
    switch (error) {
      case HTTPC_ERROR_CONNECTION_REFUSED: return "connection refused";
      case HTTPC_ERROR_SEND_HEADER_FAILED: return "send header failed";
      case HTTPC_ERROR_SEND_PAYLOAD_FAILED: return "send payload failed";
      case HTTPC_ERROR_NOT_CONNECTED: return "not connected";
      case HTTPC_ERROR_CONNECTION_LOST: return "connection lost";
      case HTTPC_ERROR_NO_STREAM: return "no stream";
      case HTTPC_ERROR_NO_HTTP_SERVER: return "no HTTP server";
      case HTTPC_ERROR_TOO_LESS_RAM: return "too less ram";
      case HTTPC_ERROR_ENCODING: return "encoding";
      case HTTPC_ERROR_STREAM_WRITE: return "stream write";
      case HTTPC_ERROR_READ_TIMEOUT: return "read Timeout";
      default: return "";
    }
  }

 private:
  int executeRequest() {
    std::string body;
    std::vector<std::pair<std::string, std::string>> respHeaders;
    int code = HttpClientStub::getNextStatusAndBody(body, respHeaders);
    _stream = HttpClientStub::InMemoryStream(body);

    _currentHeaders.clear();
    for (const auto& h : respHeaders) {
      for (const auto& k : _collectedKeys) {
        if (strcasecmp(h.first.c_str(), k.c_str()) == 0) {
          _currentHeaders.push_back({k, h.second});
          break;
        }
      }
    }
    return code;
  }

  HttpClientStub::InMemoryStream _stream;
  std::vector<std::string> _collectedKeys;
  std::vector<std::pair<std::string, std::string>> _currentHeaders;
};

#endif
