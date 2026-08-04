#ifndef ESP32_HTTP_CLIENT_H
#define ESP32_HTTP_CLIENT_H

#include <Arduino.h>
#include <vector>
#include <HTTPClient.h>

#include "BufferedStreamReader.h"
#include "RestRequest.h"
#include "RestTypes.h"

class ESP32HTTPClient {
  friend class RestRequest;

 public:
  ESP32HTTPClient(const char* baseUrl, int port = 0);

  RestRequest get(const char* path);
  RestRequest post(const char* path);
  RestRequest update(const char* path);
  RestRequest put(const char* path);
  RestRequest patch(const char* path);
  RestRequest del(const char* path);

  void setBaseUrl(const char* baseUrl, int port = 0);
  void setUrl(const char* baseUrl, int port = 0);
  void setPort(int port);
  const char* getBaseUrl() const;
  int getPort() const;

  void setTimeout(uint16_t timeoutMs);
  uint16_t getTimeout() const;

  void setMaxRetry(int maxRetry);
  int getMaxRetry() const;

  void setContentType(const char* contentType);
  void setHeader(const char* name, const char* value);
  void bearer(const char* token);
  void basic(const char* user, const char* password);
  void apiKey(const char* name, const char* key);
  void end();

  int getStatusCode() const;
  String getErrorMessage() const;
  static String errorToString(int code);
  bool isSuccess() const;
  bool hasError() const;

  void onSuccess(HttpResponseCallback cb);
  void onError(HttpErrorCallback cb);
  void onError(HttpResponseCallback cb);
  void onResponse(HttpResponseCallback cb);

  template <typename T>
  static String toJson(const T& obj) {
    RestJsonSerializer serializer;
    invokeRestJsonMap(obj, serializer);
    return serializer.finish();
  }

  template <typename T>
  static bool fromJson(const char* json, T* target) {
    if (!target || !json || json[0] == '\0') return false;
    BufferedStreamReader reader(json);
    std::vector<ResponseBinding> bindings;
    std::vector<String> storage;
    RestJsonBinder binder(bindings, storage);
    invokeRestJsonMap(*target, binder);
    RestRequest::parseJsonWithBindings(reader, bindings);
    return true;
  }

  template <typename T>
  static bool fromJson(const String& json, T* target) {
    return fromJson(json.c_str(), target);
  }

 private:
  const char* _baseUrl;
  int _port;
  int _lastStatusCode;
  uint16_t _timeout;
  int _maxRetry;
  const char* _contentType;
  std::vector<HttpHeader> _headers;
  HTTPClient _http;

  HttpResponseCallback _onSuccessCb;
  HttpErrorCallback _onErrorCb;
  HttpResponseCallback _onResponseCb;
};

namespace RestJson {
  template <typename T>
  inline String stringify(const T& obj) {
    return ESP32HTTPClient::toJson(obj);
  }

  template <typename T>
  inline String toJson(const T& obj) {
    return ESP32HTTPClient::toJson(obj);
  }

  template <typename T>
  inline bool parse(const String& json, T* target) {
    return ESP32HTTPClient::fromJson(json, target);
  }

  template <typename T>
  inline bool fromJson(const String& json, T* target) {
    return ESP32HTTPClient::fromJson(json, target);
  }
}

#endif
