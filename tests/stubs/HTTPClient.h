#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "Arduino.h"

#include <string>

namespace HttpClientStub {
inline std::string lastUrl;
inline std::string lastPayload;
inline std::string lastMethod;
inline int nextStatusCode = 200;
inline std::string nextResponseBody = "{}";

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
  nextStatusCode = 200;
  nextResponseBody = "{}";
}

inline void setResponse(int statusCode, const std::string& body) {
  nextStatusCode = statusCode;
  nextResponseBody = body;
}
}  // namespace HttpClientStub

class HTTPClient {
 public:
  HTTPClient() : _stream(HttpClientStub::nextResponseBody) {
  }

  void begin(const String& url) {
    HttpClientStub::lastUrl = url.str();
  }

  void addHeader(const char* /*name*/, const char* /*value*/) {
  }

  int GET() {
    HttpClientStub::lastMethod = "GET";
    HttpClientStub::lastPayload.clear();
    _stream = HttpClientStub::InMemoryStream(HttpClientStub::nextResponseBody);
    return HttpClientStub::nextStatusCode;
  }

  int POST(const String& payload) {
    HttpClientStub::lastMethod = "POST";
    HttpClientStub::lastPayload = payload.str();
    _stream = HttpClientStub::InMemoryStream(HttpClientStub::nextResponseBody);
    return HttpClientStub::nextStatusCode;
  }

  int PUT(const String& payload) {
    HttpClientStub::lastMethod = "PUT";
    HttpClientStub::lastPayload = payload.str();
    _stream = HttpClientStub::InMemoryStream(HttpClientStub::nextResponseBody);
    return HttpClientStub::nextStatusCode;
  }

  int PATCH(const String& payload) {
    HttpClientStub::lastMethod = "PATCH";
    HttpClientStub::lastPayload = payload.str();
    _stream = HttpClientStub::InMemoryStream(HttpClientStub::nextResponseBody);
    return HttpClientStub::nextStatusCode;
  }

  int sendRequest(const char* method, const String& payload) {
    HttpClientStub::lastMethod = method ? method : "";
    HttpClientStub::lastPayload = payload.str();
    _stream = HttpClientStub::InMemoryStream(HttpClientStub::nextResponseBody);
    return HttpClientStub::nextStatusCode;
  }

  int getSize() {
    return static_cast<int>(HttpClientStub::nextResponseBody.size());
  }

  Stream* getStreamPtr() {
    return &_stream;
  }

  void end() {
  }

 private:
  HttpClientStub::InMemoryStream _stream;
};

#endif
