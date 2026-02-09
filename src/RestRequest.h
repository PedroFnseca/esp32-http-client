#ifndef REST_REQUEST_H
#define REST_REQUEST_H

#include <Arduino.h>

#include <type_traits>
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

  int send();

 private:
  ESP32HTTPClient* _client;
  const char* _path;

  HttpMethod _method;
  bool _executed;

  std::vector<KeyValue> _queryParams;
  std::vector<KeyValue> _bodyParams;
  std::vector<ResponseBinding> _responseBindings;

  int execute();
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
  if (!key) return;

  KeyValue kv;
  kv.key = key;

  if constexpr (std::is_same<T, const char*>::value || std::is_same<T, char*>::value) {
    kv.value = value ? String(value) : String("");
    kv.quoteValue = true;
  } else if constexpr (std::is_same<T, bool>::value) {
    kv.value = value ? "true" : "false";
    kv.quoteValue = false;
  } else if constexpr (std::is_integral<T>::value) {
    kv.value = String(value);
    kv.quoteValue = false;
  } else if constexpr (std::is_floating_point<T>::value) {
    kv.value = String(value, 6);
    kv.quoteValue = false;
  } else {
    kv.value = String(value);
    kv.quoteValue = true;
  }

  list.push_back(kv);
}

#endif
