#ifndef REST_REQUEST_H
#define REST_REQUEST_H

#include <Arduino.h>

#include <vector>

#include "RestTypes.h"

class ESP32HTTPClient;

class RestRequest {
  friend class ESP32HTTPClient;

 public:
  RestRequest(ESP32HTTPClient* client, const char* path, HttpMethod method);
  ~RestRequest();

  RestRequest(const RestRequest&) = delete;
  RestRequest& operator=(const RestRequest&) = delete;
  RestRequest(RestRequest&& other);

  template <typename T>
  RestRequest& query(const char* key, T value);

  template <typename T>
  RestRequest& body(const char* key, T value);

  RestRequest& getBody(const char* key, int* target);
  RestRequest& getBody(const char* key, float* target);
  RestRequest& getBody(const char* key, double* target);
  RestRequest& getBody(const char* key, bool* target);
  RestRequest& getBody(const char* key, char* target, size_t maxLength);
  RestRequest& getBody(const char* key, long* target);

 private:
  ESP32HTTPClient* _client;
  const char* _path;

  HttpMethod _method;
  bool _executed;

  std::vector<KeyValue> _queryParams;
  std::vector<KeyValue> _bodyParams;
  std::vector<ResponseBinding> _responseBindings;

  void execute();
  void parseResponse(Stream* stream);

  template <typename T>
  void addParam(std::vector<KeyValue>& list, const char* key, T value);
};

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
void RestRequest::addParam(std::vector<KeyValue>& list, const char* key, T value) {
  KeyValue kv;
  kv.key = key;
  kv.quoteValue = false;

  if constexpr (std::is_same<T, const char*>::value || std::is_same<T, char*>::value) {
    strncpy(kv.valueBuffer, (const char*)value, 31);
    kv.quoteValue = true;
  } else if constexpr (std::is_same<T, bool>::value) {
    strncpy(kv.valueBuffer, value ? "true" : "false", 31);
    kv.quoteValue = false;
  } else {
    snprintf(kv.valueBuffer, 31, "%.2f", (float)value);
  }
  list.push_back(kv);
}

template <>
inline void RestRequest::addParam<int>(std::vector<KeyValue>& list, const char* key, int value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, 31, "%d", value);
  kv.valueBuffer[31] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

template <>
inline void RestRequest::addParam<long>(std::vector<KeyValue>& list, const char* key, long value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, 31, "%ld", value);
  kv.valueBuffer[31] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

template <>
inline void RestRequest::addParam<float>(std::vector<KeyValue>& list, const char* key, float value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, 31, "%.5g", value);
  kv.valueBuffer[31] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

template <>
inline void RestRequest::addParam<double>(std::vector<KeyValue>& list, const char* key, double value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, 31, "%.9g", value);
  kv.valueBuffer[31] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

template <>
inline void RestRequest::addParam<const char*>(std::vector<KeyValue>& list, const char* key, const char* value) {
  KeyValue kv;
  kv.key = key;
  strncpy(kv.valueBuffer, value, 31);
  kv.valueBuffer[31] = 0;
  kv.quoteValue = true;
  list.push_back(kv);
}

#endif
