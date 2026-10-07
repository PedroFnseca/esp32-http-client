#ifndef JSON_RPC_REQUEST_H
#define JSON_RPC_REQUEST_H

#include <Arduino.h>
#include <deque>
#include <vector>

#include "JsonRpcTypes.h"
#include "RestTypes.h"

class ESP32HTTPClient;
class BufferedStreamReader;

class JsonRpcJsonBinder {
 public:
  std::vector<ResponseBinding>& bindings;
  std::deque<String>& storage;
  String basePath;

  JsonRpcJsonBinder(std::vector<ResponseBinding>& b, std::deque<String>& s, const char* base = "result")
      : bindings(b), storage(s), basePath(base ? base : "") {}

  const char* makeKey(const char* key) {
    if (basePath.isEmpty()) {
      return key;
    }
    storage.push_back(basePath + "." + key);
    return storage.back().c_str();
  }

  void field(const char* key, int& val) {
    bindings.push_back({makeKey(key), &val, TYPE_INT, sizeof(int)});
  }
  void field(const char* key, long& val) {
    bindings.push_back({makeKey(key), &val, TYPE_LONG, sizeof(long)});
  }
  void field(const char* key, float& val) {
    bindings.push_back({makeKey(key), &val, TYPE_FLOAT, sizeof(float)});
  }
  void field(const char* key, double& val) {
    bindings.push_back({makeKey(key), &val, TYPE_DOUBLE, sizeof(double)});
  }
  void field(const char* key, bool& val) {
    bindings.push_back({makeKey(key), &val, TYPE_BOOL, sizeof(bool)});
  }
  template <size_t N>
  void field(const char* key, char (&val)[N]) {
    bindings.push_back({makeKey(key), val, TYPE_STRING, N});
  }
  void field(const char* key, char* val, size_t maxLen = 64) {
    bindings.push_back({makeKey(key), val, TYPE_STRING, maxLen});
  }
  void field(const char* key, String& val) {
    bindings.push_back({makeKey(key), &val, TYPE_ARDUINO_STRING, 0});
  }
};

struct JsonRpcParam {
  String name;
  char valueBuffer[64];
  String dynamicValue;
  bool isDynamic;
  bool quoteValue;
  bool isRawJson;

  JsonRpcParam() : isDynamic(false), quoteValue(false), isRawJson(false) {
    valueBuffer[0] = '\0';
  }

  const char* getValue() const {
    return isDynamic ? dynamicValue.c_str() : valueBuffer;
  }

  void setValue(const char* val, bool quote = false, bool raw = false) {
    quoteValue = quote;
    isRawJson = raw;
    if (!val) val = "null";
    size_t len = strlen(val);
    if (len < sizeof(valueBuffer)) {
      memcpy(valueBuffer, val, len + 1);
      isDynamic = false;
      dynamicValue = "";
    } else {
      dynamicValue = val;
      isDynamic = true;
      valueBuffer[0] = '\0';
    }
  }
};

class JsonRpcRequest {
  friend class ESP32HTTPClient;

 public:
  JsonRpcRequest(ESP32HTTPClient* client, const char* path = "");
  ~JsonRpcRequest();

  JsonRpcRequest(const JsonRpcRequest&) = delete;
  JsonRpcRequest& operator=(const JsonRpcRequest&) = delete;
  JsonRpcRequest(JsonRpcRequest&& other);

  // Method configuration
  JsonRpcRequest& method(const char* methodName);
  JsonRpcRequest& method(const String& methodName);
  const String& getMethod() const;

  // Request ID & Notification configuration
  JsonRpcRequest& id(int reqId);
  JsonRpcRequest& id(long reqId);
  JsonRpcRequest& id(const char* reqId);
  JsonRpcRequest& id(const String& reqId);
  JsonRpcRequest& nullId();
  JsonRpcRequest& notification(bool enable = true);
  JsonRpcRequest& asNotification();
  bool isNotification() const;
  bool hasRequestId() const;
  String getRequestId() const;
  int getRequestIdInt() const;

  // Positional parameters
  JsonRpcRequest& param(const char* value);
  JsonRpcRequest& param(const String& value);
  JsonRpcRequest& param(bool value);
  JsonRpcRequest& param(int value);
  JsonRpcRequest& param(unsigned int value);
  JsonRpcRequest& param(long value);
  JsonRpcRequest& param(unsigned long value);
  JsonRpcRequest& param(long long value);
  JsonRpcRequest& param(unsigned long long value);
  JsonRpcRequest& param(float value);
  JsonRpcRequest& param(double value);

  JsonRpcRequest& positionalParam(const char* value) { return param(value); }
  JsonRpcRequest& positionalParam(const String& value) { return param(value); }
  JsonRpcRequest& positionalParam(bool value) { return param(value); }
  JsonRpcRequest& positionalParam(int value) { return param(value); }
  JsonRpcRequest& positionalParam(unsigned int value) { return param(value); }
  JsonRpcRequest& positionalParam(long value) { return param(value); }
  JsonRpcRequest& positionalParam(unsigned long value) { return param(value); }
  JsonRpcRequest& positionalParam(long long value) { return param(value); }
  JsonRpcRequest& positionalParam(unsigned long long value) { return param(value); }
  JsonRpcRequest& positionalParam(float value) { return param(value); }
  JsonRpcRequest& positionalParam(double value) { return param(value); }

  // Named parameters
  JsonRpcRequest& param(const char* name, const char* value);
  JsonRpcRequest& param(const char* name, const String& value);
  JsonRpcRequest& param(const char* name, bool value);
  JsonRpcRequest& param(const char* name, int value);
  JsonRpcRequest& param(const char* name, unsigned int value);
  JsonRpcRequest& param(const char* name, long value);
  JsonRpcRequest& param(const char* name, unsigned long value);
  JsonRpcRequest& param(const char* name, long long value);
  JsonRpcRequest& param(const char* name, unsigned long long value);
  JsonRpcRequest& param(const char* name, float value);
  JsonRpcRequest& param(const char* name, double value);

  JsonRpcRequest& namedParam(const char* name, const char* value) { return param(name, value); }
  JsonRpcRequest& namedParam(const char* name, const String& value) { return param(name, value); }
  JsonRpcRequest& namedParam(const char* name, bool value) { return param(name, value); }
  JsonRpcRequest& namedParam(const char* name, int value) { return param(name, value); }
  JsonRpcRequest& namedParam(const char* name, unsigned int value) { return param(name, value); }
  JsonRpcRequest& namedParam(const char* name, long value) { return param(name, value); }
  JsonRpcRequest& namedParam(const char* name, unsigned long value) { return param(name, value); }
  JsonRpcRequest& namedParam(const char* name, long long value) { return param(name, value); }
  JsonRpcRequest& namedParam(const char* name, unsigned long long value) { return param(name, value); }
  JsonRpcRequest& namedParam(const char* name, float value) { return param(name, value); }
  JsonRpcRequest& namedParam(const char* name, double value) { return param(name, value); }

  // Raw and struct parameters
  JsonRpcRequest& rawParams(const char* jsonParams);
  JsonRpcRequest& rawParams(const String& jsonParams);
  JsonRpcRequest& rawParam(const char* rawJson);
  JsonRpcRequest& rawParam(const String& rawJson);
  JsonRpcRequest& rawParam(const char* name, const char* rawJson);
  JsonRpcRequest& rawParam(const char* name, const String& rawJson);

  template <typename T>
  typename std::enable_if<HasRestJsonMap<T>::value, JsonRpcRequest&>::type
  params(const T& obj) {
    RestJsonSerializer serializer;
    invokeRestJsonMap(obj, serializer);
    return rawParams(serializer.finish());
  }

  template <typename T>
  typename std::enable_if<HasRestJsonMap<T>::value, JsonRpcRequest&>::type
  param(const char* name, const T& obj) {
    RestJsonSerializer serializer;
    invokeRestJsonMap(obj, serializer);
    return rawParam(name, serializer.finish());
  }

  template <typename T>
  typename std::enable_if<HasRestJsonMap<T>::value, JsonRpcRequest&>::type
  namedParam(const char* name, const T& obj) {
    return param(name, obj);
  }

  // URL path template & query params
  template <typename T>
  JsonRpcRequest& path(const char* key, const T& value);

  template <typename T>
  JsonRpcRequest& queryParam(const char* key, const T& value);

  // Headers and configuration
  JsonRpcRequest& header(const char* name, const char* value);
  JsonRpcRequest& contentType(const char* type);
  JsonRpcRequest& timeout(uint16_t timeoutMs);
  JsonRpcRequest& maxRetry(int maxRetry);
  JsonRpcRequest& retry(int maxRetry);

  // Result bindings (root result)
  JsonRpcRequest& getResult(int* target);
  JsonRpcRequest& getResult(float* target);
  JsonRpcRequest& getResult(double* target);
  JsonRpcRequest& getResult(bool* target);
  JsonRpcRequest& getResult(long* target);
  JsonRpcRequest& getResult(char* target, size_t maxLength);
  JsonRpcRequest& getResult(String* target);
  template <size_t N>
  JsonRpcRequest& getResult(char (&target)[N]);

  // Result bindings (subpath)
  JsonRpcRequest& getResult(const char* subpath, int* target);
  JsonRpcRequest& getResult(const char* subpath, float* target);
  JsonRpcRequest& getResult(const char* subpath, double* target);
  JsonRpcRequest& getResult(const char* subpath, bool* target);
  JsonRpcRequest& getResult(const char* subpath, long* target);
  JsonRpcRequest& getResult(const char* subpath, char* target, size_t maxLength);
  JsonRpcRequest& getResult(const char* subpath, String* target);
  template <size_t N>
  JsonRpcRequest& getResult(const char* subpath, char (&target)[N]);

  // Struct result mapping
  template <typename T>
  typename std::enable_if<HasRestJsonMap<T>::value, JsonRpcRequest&>::type
  getResult(T* target) {
    if (!target) return *this;
    JsonRpcJsonBinder binder(_responseBindings, _keyStorage, "result");
    invokeRestJsonMap(*target, binder);
    return *this;
  }

  template <typename T>
  typename std::enable_if<HasRestJsonMap<T>::value, JsonRpcRequest&>::type
  getResult(const char* subpath, T* target) {
    if (!target) return *this;
    String fullPath = normalizeResultPath(subpath);
    _keyStorage.push_back(fullPath);
    JsonRpcJsonBinder binder(_responseBindings, _keyStorage, _keyStorage.back().c_str());
    invokeRestJsonMap(*target, binder);
    return *this;
  }

  // Raw responses
  JsonRpcRequest& getRawResult(String* target);
  JsonRpcRequest& getRawResponse(String* target);

  // Response ID and Version bindings
  JsonRpcRequest& getId(int* target);
  JsonRpcRequest& getId(long* target);
  JsonRpcRequest& getId(String* target);
  JsonRpcRequest& getJsonRpcVersion(String* target);

  // Response headers
  JsonRpcRequest& getHeader(const char* name, int* target);
  JsonRpcRequest& getHeader(const char* name, float* target);
  JsonRpcRequest& getHeader(const char* name, double* target);
  JsonRpcRequest& getHeader(const char* name, bool* target);
  JsonRpcRequest& getHeader(const char* name, long* target);
  JsonRpcRequest& getHeader(const char* name, char* target, size_t maxLength);
  JsonRpcRequest& getHeader(const char* name, String* target);
  template <size_t N>
  JsonRpcRequest& getHeader(const char* name, char (&target)[N]);

  // Error handling
  JsonRpcRequest& getError(JsonRpcError* target);
  JsonRpcRequest& getErrorMessage(String* target);
  JsonRpcRequest& getErrorCode(int* target);
  JsonRpcRequest& getErrorData(String* target);

  bool hasError() const;
  bool hasJsonRpcError() const;
  bool isSuccess() const;
  int getStatusCode() const;
  const JsonRpcError& getError() const;
  int getErrorCode() const;
  String getErrorMessage() const;
  String getErrorData() const;
  String getResponseId() const;
  int getResponseIdInt() const;
  String getJsonRpcVersion() const;

  // Callbacks
  JsonRpcRequest& onSuccess(HttpResponseCallback cb);
  JsonRpcRequest& onError(HttpErrorCallback cb);
  JsonRpcRequest& onError(HttpResponseCallback cb);
  JsonRpcRequest& onResponse(HttpResponseCallback cb);
  JsonRpcRequest& onJsonRpcError(JsonRpcErrorCallback cb);

  void execute();
  String buildRequestBody() const;
  static String escapeJsonString(const String& str);
  static String normalizeResultPath(const char* path);

 private:
  ESP32HTTPClient* _client;
  const char* _path;
  bool _executed;
  uint16_t _timeout;
  int _maxRetry;
  int _statusCode;

  String _method;
  bool _isNotification;
  JsonRpcIdType _idType;
  long _intId;
  String _stringId;

  JsonRpcParamMode _paramMode;
  std::vector<JsonRpcParam> _params;
  String _rawParams;
  String _contentType;

  std::vector<KeyValue> _pathParams;
  std::vector<KeyValue> _queryParams;
  std::vector<HttpHeader> _customHeaders;

  std::vector<ResponseBinding> _responseBindings;
  std::vector<ResponseBinding> _headerBindings;
  std::deque<String> _keyStorage;

  String* _rawResultTarget;
  String* _rawResponseTarget;
  String* _jsonRpcVersionTarget;
  int* _idIntTarget;
  long* _idLongTarget;
  String* _idStringTarget;

  JsonRpcError* _errorTarget;
  int* _errorCodeTarget;
  String* _errorMessageTarget;
  String* _errorDataTarget;

  bool _hasError;
  JsonRpcError _error;
  String _responseId;
  bool _hasResponseId;
  String _responseJsonRpcVersion;

  HttpResponseCallback _onSuccessCb;
  HttpErrorCallback _onErrorCb;
  HttpResponseCallback _onResponseCb;
  JsonRpcErrorCallback _onJsonRpcErrorCb;

  void addParamInternal(const char* name, const char* value, bool quote, bool raw);
  void parseResponse(BufferedStreamReader& r);
  void parseResponseObject(BufferedStreamReader& r);
  void parseErrorObject(BufferedStreamReader& r);
  void parseResultValue(BufferedStreamReader& r);
  void applyBindingsToResult(BufferedStreamReader& r);
  ResponseBinding* findRootBinding();
};

template <size_t N>
inline JsonRpcRequest& JsonRpcRequest::getResult(char (&target)[N]) {
  return getResult(target, N);
}

template <size_t N>
inline JsonRpcRequest& JsonRpcRequest::getResult(const char* subpath, char (&target)[N]) {
  return getResult(subpath, target, N);
}

template <size_t N>
inline JsonRpcRequest& JsonRpcRequest::getHeader(const char* name, char (&target)[N]) {
  return getHeader(name, target, N);
}

template <typename T>
JsonRpcRequest& JsonRpcRequest::path(const char* key, const T& value) {
  KeyValue kv;
  kv.key = key;
  kv.value = String(value).c_str();
  _pathParams.push_back(kv);
  return *this;
}

template <typename T>
JsonRpcRequest& JsonRpcRequest::queryParam(const char* key, const T& value) {
  KeyValue kv;
  kv.key = key;
  kv.value = String(value).c_str();
  _queryParams.push_back(kv);
  return *this;
}

#endif
