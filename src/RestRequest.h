#ifndef REST_REQUEST_H
#define REST_REQUEST_H

#include <Arduino.h>

#include <vector>

#include "RestTypes.h"

class ESP32HTTPClient;
class BufferedStreamReader;

class RestRequest {
  friend class ESP32HTTPClient;

 public:
  RestRequest(ESP32HTTPClient* client, const char* path, HttpMethod method);
  ~RestRequest();

  RestRequest(const RestRequest&) = delete;
  RestRequest& operator=(const RestRequest&) = delete;
  RestRequest(RestRequest&& other);

  RestRequest& timeout(uint16_t timeoutMs);
  RestRequest& maxRetry(int maxRetry);
  RestRequest& retry(int maxRetry);

  RestRequest& onSuccess(HttpResponseCallback cb);
  RestRequest& onError(HttpErrorCallback cb);
  RestRequest& onError(HttpResponseCallback cb);
  RestRequest& onResponse(HttpResponseCallback cb);

  template <typename T>
  RestRequest& path(const char* key, T value);

  template <typename T>
  RestRequest& query(const char* key, T value);

  template <typename T>
  RestRequest& body(const char* key, T value);

  template <typename T>
  typename std::enable_if<HasRestJsonMap<T>::value, RestRequest&>::type
  body(const T& obj);

  RestRequest& getBody(const char* key, int* target);
  RestRequest& getBody(const char* key, float* target);
  RestRequest& getBody(const char* key, double* target);
  RestRequest& getBody(const char* key, bool* target);
  RestRequest& getBody(const char* key, char* target, size_t maxLength);
  RestRequest& getBody(const char* key, long* target);
  RestRequest& getBody(const char* key, String* target);

  template <typename T>
  typename std::enable_if<HasRestJsonMap<T>::value, RestRequest&>::type
  getBody(T* target);

  template <typename T>
  typename std::enable_if<HasRestJsonMap<T>::value, RestRequest&>::type
  getBody(const char* key, T* target);

  RestRequest& getHeader(const char* name, int* target);
  RestRequest& getHeader(const char* name, float* target);
  RestRequest& getHeader(const char* name, double* target);
  RestRequest& getHeader(const char* name, bool* target);
  RestRequest& getHeader(const char* name, char* target, size_t maxLength);
  RestRequest& getHeader(const char* name, long* target);
  RestRequest& getHeader(const char* name, String* target);

  template <size_t N>
  RestRequest& getHeader(const char* name, char (&target)[N]);

  static void parseJsonWithBindings(BufferedStreamReader& r, std::vector<ResponseBinding>& bindings);
  static void parseObjectWithBindings(BufferedStreamReader& r, const char* basePath, std::vector<ResponseBinding>& bindings);
  static void parseArrayWithBindings(BufferedStreamReader& r, const char* basePath, std::vector<ResponseBinding>& bindings);
  static void parsePrimitiveWithBinding(BufferedStreamReader& r, ResponseBinding* match);
  static void readRawJson(BufferedStreamReader& r, String* target, char openingBrace);

 private:
  ESP32HTTPClient* _client;
  const char* _path;

  HttpMethod _method;
  bool _executed;
  uint16_t _timeout;
  int _maxRetry;

  HttpResponseCallback _onSuccessCb;
  HttpErrorCallback _onErrorCb;
  HttpResponseCallback _onResponseCb;

  String _rawBody;
  std::vector<String> _keyStorage;

  std::vector<KeyValue> _pathParams;
  std::vector<KeyValue> _queryParams;
  std::vector<KeyValue> _bodyParams;
  std::vector<ResponseBinding> _responseBindings;
  std::vector<ResponseBinding> _headerBindings;

  void execute();
  void parseResponse(BufferedStreamReader& r);

  template <typename T>
  void addParam(std::vector<KeyValue>& list, const char* key, T value);
};

template <size_t N>
inline RestRequest& RestRequest::getHeader(const char* name, char (&target)[N]) {
  return getHeader(name, target, N);
}

template <typename T>
RestRequest& RestRequest::path(const char* key, T value) {
  addParam(_pathParams, key, value);
  return *this;
}

template <typename T>
RestRequest& RestRequest::query(const char* key, T value) {
  addParam(_queryParams, key, value);
  return *this;
}

template <typename T>
RestRequest& RestRequest::body(const char* key, T value) {
  addParam(_bodyParams, key, value);
  return *this;
}

template <typename T>
typename std::enable_if<HasRestJsonMap<T>::value, RestRequest&>::type
RestRequest::body(const T& obj) {
  RestJsonSerializer serializer;
  invokeRestJsonMap(obj, serializer);
  _rawBody = serializer.finish();
  return *this;
}

template <typename T>
typename std::enable_if<HasRestJsonMap<T>::value, RestRequest&>::type
RestRequest::getBody(T* target) {
  if (target) {
    RestJsonBinder binder(_responseBindings, _keyStorage, "");
    invokeRestJsonMap(*target, binder);
  }
  return *this;
}

template <typename T>
typename std::enable_if<HasRestJsonMap<T>::value, RestRequest&>::type
RestRequest::getBody(const char* key, T* target) {
  if (target) {
    RestJsonBinder binder(_responseBindings, _keyStorage, key);
    invokeRestJsonMap(*target, binder);
  }
  return *this;
}

template <typename T>
void RestRequest::addParam(std::vector<KeyValue>& list, const char* key, T value) {
  KeyValue kv;
  kv.key = key;
  kv.quoteValue = false;

  if constexpr (std::is_same<T, const char*>::value || std::is_same<T, char*>::value) {
    strncpy(kv.valueBuffer, (const char*)value, sizeof(kv.valueBuffer) - 1);
    kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
    kv.quoteValue = true;
  } else if constexpr (std::is_same<T, bool>::value) {
    strncpy(kv.valueBuffer, value ? "true" : "false", sizeof(kv.valueBuffer) - 1);
    kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
    kv.quoteValue = false;
  } else {
    snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%.2f", (float)value);
  }
  list.push_back(kv);
}

template <>
inline void RestRequest::addParam<int>(std::vector<KeyValue>& list, const char* key, int value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%d", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

template <>
inline void RestRequest::addParam<long>(std::vector<KeyValue>& list, const char* key, long value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%ld", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

template <>
inline void RestRequest::addParam<float>(std::vector<KeyValue>& list, const char* key, float value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%.5g", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

template <>
inline void RestRequest::addParam<double>(std::vector<KeyValue>& list, const char* key, double value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%.9g", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

template <>
inline void RestRequest::addParam<const char*>(std::vector<KeyValue>& list, const char* key, const char* value) {
  KeyValue kv;
  kv.key = key;
  strncpy(kv.valueBuffer, value ? value : "", sizeof(kv.valueBuffer) - 1);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = true;
  list.push_back(kv);
}

#endif
