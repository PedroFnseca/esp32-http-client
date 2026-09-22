#ifndef GRAPHQL_REQUEST_H
#define GRAPHQL_REQUEST_H

#include <Arduino.h>
#include <deque>
#include <vector>

#include "GraphQLTypes.h"
#include "RestTypes.h"

class ESP32HTTPClient;
class BufferedStreamReader;

class GraphQLJsonBinder {
 public:
  std::vector<ResponseBinding>& bindings;
  std::deque<String>& storage;
  String basePath;

  GraphQLJsonBinder(std::vector<ResponseBinding>& b, std::deque<String>& s, const char* base = "")
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

class GraphQLRequest {
  friend class ESP32HTTPClient;
  friend class GraphQLBatchRequest;

 public:
  GraphQLRequest(ESP32HTTPClient* client, const char* path = "/graphql", HttpMethod method = HTTP_POST_METHOD);
  ~GraphQLRequest();

  GraphQLRequest(const GraphQLRequest&) = delete;
  GraphQLRequest& operator=(const GraphQLRequest&) = delete;
  GraphQLRequest(GraphQLRequest&& other);

  GraphQLRequest& query(const char* queryDocument);
  GraphQLRequest& query(const String& queryDocument);
  GraphQLRequest& mutation(const char* mutationDocument);
  GraphQLRequest& mutation(const String& mutationDocument);
  GraphQLRequest& document(const char* document);
  GraphQLRequest& document(const String& document);

  GraphQLRequest& operationName(const char* name);
  GraphQLRequest& operationName(const String& name);

  GraphQLRequest& method(HttpMethod method);
  GraphQLRequest& asGet();
  GraphQLRequest& asPost();
  GraphQLRequest& get();
  GraphQLRequest& post();

  GraphQLRequest& variable(const char* name, const char* value);
  GraphQLRequest& variable(const char* name, const String& value);
  GraphQLRequest& variable(const char* name, bool value);
  GraphQLRequest& variable(const char* name, int value);
  GraphQLRequest& variable(const char* name, unsigned int value);
  GraphQLRequest& variable(const char* name, long value);
  GraphQLRequest& variable(const char* name, unsigned long value);
  GraphQLRequest& variable(const char* name, long long value);
  GraphQLRequest& variable(const char* name, unsigned long long value);
  GraphQLRequest& variable(const char* name, float value);
  GraphQLRequest& variable(const char* name, double value);

  template <typename T>
  typename std::enable_if<HasRestJsonMap<T>::value, GraphQLRequest&>::type
  variable(const char* name, const T& obj) {
    RestJsonSerializer serializer;
    invokeRestJsonMap(obj, serializer);
    return rawVariable(name, serializer.finish());
  }

  GraphQLRequest& rawVariable(const char* name, const char* jsonValue);
  GraphQLRequest& rawVariable(const char* name, const String& jsonValue);
  GraphQLRequest& rawVariables(const char* jsonObject);
  GraphQLRequest& rawVariables(const String& jsonObject);

  template <typename T>
  GraphQLRequest& path(const char* key, const T& value);

  template <typename T>
  GraphQLRequest& queryParam(const char* key, const T& value);

  GraphQLRequest& header(const char* name, const char* value);
  GraphQLRequest& accept(const char* acceptHeader);
  GraphQLRequest& timeout(uint16_t timeoutMs);
  GraphQLRequest& maxRetry(int maxRetry);
  GraphQLRequest& retry(int maxRetry);

  GraphQLRequest& getData(const char* path, int* target);
  GraphQLRequest& getData(const char* path, float* target);
  GraphQLRequest& getData(const char* path, double* target);
  GraphQLRequest& getData(const char* path, bool* target);
  GraphQLRequest& getData(const char* path, long* target);
  GraphQLRequest& getData(const char* path, char* target, size_t maxLength);
  GraphQLRequest& getData(const char* path, String* target);

  template <size_t N>
  GraphQLRequest& getData(const char* path, char (&target)[N]);

  template <typename T>
  typename std::enable_if<HasRestJsonMap<T>::value, GraphQLRequest&>::type
  getData(T* target) {
    if (!target) return *this;
    GraphQLJsonBinder binder(_responseBindings, _keyStorage, "data");
    invokeRestJsonMap(*target, binder);
    return *this;
  }

  template <typename T>
  typename std::enable_if<HasRestJsonMap<T>::value, GraphQLRequest&>::type
  getData(const char* path, T* target) {
    if (!target) return *this;
    String fullPath = normalizeDataPath(path);
    _keyStorage.push_back(fullPath);
    GraphQLJsonBinder binder(_responseBindings, _keyStorage, _keyStorage.back().c_str());
    invokeRestJsonMap(*target, binder);
    return *this;
  }

  GraphQLRequest& getRawData(String* target);
  GraphQLRequest& getRawResponse(String* target);

  GraphQLRequest& getHeader(const char* name, int* target);
  GraphQLRequest& getHeader(const char* name, float* target);
  GraphQLRequest& getHeader(const char* name, double* target);
  GraphQLRequest& getHeader(const char* name, bool* target);
  GraphQLRequest& getHeader(const char* name, long* target);
  GraphQLRequest& getHeader(const char* name, char* target, size_t maxLength);
  GraphQLRequest& getHeader(const char* name, String* target);

  template <size_t N>
  GraphQLRequest& getHeader(const char* name, char (&target)[N]);

  GraphQLRequest& getError(GraphQLError* target);
  GraphQLRequest& getErrors(std::vector<GraphQLError>* target);
  GraphQLRequest& getErrorMessage(String* target);
  bool hasGraphQLErrors() const;
  const std::vector<GraphQLError>& getErrors() const;

  GraphQLRequest& onSuccess(HttpResponseCallback cb);
  GraphQLRequest& onError(HttpErrorCallback cb);
  GraphQLRequest& onError(HttpResponseCallback cb);
  GraphQLRequest& onResponse(HttpResponseCallback cb);
  GraphQLRequest& onGraphQLError(GraphQLMultiErrorCallback cb);
  GraphQLRequest& onGraphQLError(GraphQLErrorCallback cb);
  GraphQLRequest& onIncremental(GraphQLIncrementalCallback cb);

  void execute();
  String buildRequestBody() const;
  String buildGetUrl() const;

  static String urlEncode(const String& str);
  static String escapeJsonString(const String& str);
  static String normalizeDataPath(const char* path);

 private:
  ESP32HTTPClient* _client;
  const char* _path;
  HttpMethod _method;
  bool _executed;
  bool _isBatchMember;
  uint16_t _timeout;
  int _maxRetry;

  String _document;
  String _operationName;
  String _rawVariables;
  String _acceptHeader;

  std::vector<KeyValue> _variables;
  std::vector<KeyValue> _pathParams;
  std::vector<KeyValue> _queryParams;
  std::vector<HttpHeader> _customHeaders;

  std::vector<ResponseBinding> _responseBindings;
  std::vector<ResponseBinding> _headerBindings;
  std::deque<String> _keyStorage;

  String* _rawDataTarget;
  String* _rawResponseTarget;
  GraphQLError* _firstErrorTarget;
  std::vector<GraphQLError>* _allErrorsTarget;
  String* _errorMessageTarget;

  std::vector<GraphQLError> _errors;

  HttpResponseCallback _onSuccessCb;
  HttpErrorCallback _onErrorCb;
  HttpResponseCallback _onResponseCb;
  GraphQLMultiErrorCallback _onMultiErrorCb;
  GraphQLErrorCallback _onSingleErrorCb;
  GraphQLIncrementalCallback _onIncrementalCb;

  void addParam(std::vector<KeyValue>& list, const char* key, const char* value);
  void addParam(std::vector<KeyValue>& list, const char* key, const String& value);
  void addParam(std::vector<KeyValue>& list, const char* key, bool value);
  void addParam(std::vector<KeyValue>& list, const char* key, int value);
  void addParam(std::vector<KeyValue>& list, const char* key, unsigned int value);
  void addParam(std::vector<KeyValue>& list, const char* key, long value);
  void addParam(std::vector<KeyValue>& list, const char* key, unsigned long value);
  void addParam(std::vector<KeyValue>& list, const char* key, long long value);
  void addParam(std::vector<KeyValue>& list, const char* key, unsigned long long value);
  void addParam(std::vector<KeyValue>& list, const char* key, float value);
  void addParam(std::vector<KeyValue>& list, const char* key, double value);

  void parseResponse(BufferedStreamReader& r, const String& contentType);
  void parseSingleResponse(BufferedStreamReader& r);
  void parseMultipartResponse(BufferedStreamReader& r, const String& boundary);
  void parseErrors(BufferedStreamReader& r);
  GraphQLError parseSingleError(BufferedStreamReader& r);
  void parseLocation(BufferedStreamReader& r, GraphQLLocation& loc);
  void parsePath(BufferedStreamReader& r, std::vector<String>& path);
};

template <size_t N>
inline GraphQLRequest& GraphQLRequest::getData(const char* path, char (&target)[N]) {
  return getData(path, target, N);
}

template <size_t N>
inline GraphQLRequest& GraphQLRequest::getHeader(const char* name, char (&target)[N]) {
  return getHeader(name, target, N);
}

template <typename T>
GraphQLRequest& GraphQLRequest::path(const char* key, const T& value) {
  addParam(_pathParams, key, value);
  return *this;
}

template <typename T>
GraphQLRequest& GraphQLRequest::queryParam(const char* key, const T& value) {
  addParam(_queryParams, key, value);
  return *this;
}

#endif
