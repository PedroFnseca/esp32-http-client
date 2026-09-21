#ifndef SOAP_REQUEST_H
#define SOAP_REQUEST_H

#include <Arduino.h>
#include <vector>

#include "RestTypes.h"
#include "SoapTypes.h"

class ESP32HTTPClient;
class BufferedStreamReader;

class SoapRequest {
  friend class ESP32HTTPClient;

 public:
  SoapRequest(ESP32HTTPClient* client, const char* path, SoapVersion version = SOAP_1_1);
  ~SoapRequest();

  SoapRequest(const SoapRequest&) = delete;
  SoapRequest& operator=(const SoapRequest&) = delete;
  SoapRequest(SoapRequest&& other);

  SoapRequest& version(SoapVersion version);
  SoapRequest& setVersion(SoapVersion version);

  SoapRequest& soapAction(const char* action);
  SoapRequest& action(const char* action);

  SoapRequest& body(const char* xmlPayload);
  SoapRequest& body(const String& xmlPayload);

  SoapRequest& headerXml(const char* headerXml);
  SoapRequest& headerXml(const String& headerXml);

  SoapRequest& rawEnvelope(const char* envelopeXml);
  SoapRequest& rawEnvelope(const String& envelopeXml);

  SoapRequest& timeout(uint16_t timeoutMs);
  SoapRequest& maxRetry(int maxRetry);
  SoapRequest& retry(int maxRetry);

  SoapRequest& onSuccess(HttpResponseCallback cb);
  SoapRequest& onError(HttpErrorCallback cb);
  SoapRequest& onError(HttpResponseCallback cb);
  SoapRequest& onResponse(HttpResponseCallback cb);
  SoapRequest& onFault(SoapFaultCallback cb);

  template <typename T>
  SoapRequest& path(const char* key, const T& value);

  template <typename T>
  SoapRequest& query(const char* key, const T& value);

  SoapRequest& getBody(const char* xmlPath, int* target);
  SoapRequest& getBody(const char* xmlPath, float* target);
  SoapRequest& getBody(const char* xmlPath, double* target);
  SoapRequest& getBody(const char* xmlPath, bool* target);
  SoapRequest& getBody(const char* xmlPath, long* target);
  SoapRequest& getBody(const char* xmlPath, char* target, size_t maxLength);
  SoapRequest& getBody(const char* xmlPath, String* target);

  template <size_t N>
  SoapRequest& getBody(const char* xmlPath, char (&target)[N]);

  SoapRequest& getFault(SoapFault* target);
  SoapRequest& getRawResponse(String* target);

  SoapRequest& getHeader(const char* name, int* target);
  SoapRequest& getHeader(const char* name, float* target);
  SoapRequest& getHeader(const char* name, double* target);
  SoapRequest& getHeader(const char* name, bool* target);
  SoapRequest& getHeader(const char* name, long* target);
  SoapRequest& getHeader(const char* name, char* target, size_t maxLength);
  SoapRequest& getHeader(const char* name, String* target);

  template <size_t N>
  SoapRequest& getHeader(const char* name, char (&target)[N]);

  void execute();

  static void parseXmlWithBindings(BufferedStreamReader& r,
                                   std::vector<ResponseBinding>& bindings,
                                   SoapFault* fault = nullptr,
                                   String* rawXml = nullptr);

 private:
  ESP32HTTPClient* _client;
  const char* _path;
  SoapVersion _version;
  bool _executed;
  uint16_t _timeout;
  int _maxRetry;

  String _soapAction;
  String _bodyXml;
  String _headerXml;
  String _rawEnvelope;

  HttpResponseCallback _onSuccessCb;
  HttpErrorCallback _onErrorCb;
  HttpResponseCallback _onResponseCb;
  SoapFaultCallback _onFaultCb;

  SoapFault* _faultTarget;
  String* _rawResponseTarget;

  std::vector<KeyValue> _pathParams;
  std::vector<KeyValue> _queryParams;
  std::vector<ResponseBinding> _responseBindings;
  std::vector<ResponseBinding> _headerBindings;

  String buildEnvelope() const;
  void parseResponse(BufferedStreamReader& r);

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
};

template <size_t N>
inline SoapRequest& SoapRequest::getBody(const char* xmlPath, char (&target)[N]) {
  return getBody(xmlPath, target, N);
}

template <size_t N>
inline SoapRequest& SoapRequest::getHeader(const char* name, char (&target)[N]) {
  return getHeader(name, target, N);
}

template <typename T>
SoapRequest& SoapRequest::path(const char* key, const T& value) {
  addParam(_pathParams, key, value);
  return *this;
}

template <typename T>
SoapRequest& SoapRequest::query(const char* key, const T& value) {
  addParam(_queryParams, key, value);
  return *this;
}

inline void SoapRequest::addParam(std::vector<KeyValue>& list, const char* key, const char* value) {
  KeyValue kv;
  kv.key = key;
  strncpy(kv.valueBuffer, value ? value : "", sizeof(kv.valueBuffer) - 1);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

inline void SoapRequest::addParam(std::vector<KeyValue>& list, const char* key, const String& value) {
  KeyValue kv;
  kv.key = key;
  strncpy(kv.valueBuffer, value.c_str(), sizeof(kv.valueBuffer) - 1);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

inline void SoapRequest::addParam(std::vector<KeyValue>& list, const char* key, bool value) {
  KeyValue kv;
  kv.key = key;
  strncpy(kv.valueBuffer, value ? "true" : "false", sizeof(kv.valueBuffer) - 1);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

inline void SoapRequest::addParam(std::vector<KeyValue>& list, const char* key, int value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%d", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

inline void SoapRequest::addParam(std::vector<KeyValue>& list, const char* key, unsigned int value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%u", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

inline void SoapRequest::addParam(std::vector<KeyValue>& list, const char* key, long value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%ld", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

inline void SoapRequest::addParam(std::vector<KeyValue>& list, const char* key, unsigned long value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%lu", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

inline void SoapRequest::addParam(std::vector<KeyValue>& list, const char* key, long long value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%lld", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

inline void SoapRequest::addParam(std::vector<KeyValue>& list, const char* key, unsigned long long value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%llu", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

inline void SoapRequest::addParam(std::vector<KeyValue>& list, const char* key, float value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%.5g", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

inline void SoapRequest::addParam(std::vector<KeyValue>& list, const char* key, double value) {
  KeyValue kv;
  kv.key = key;
  snprintf(kv.valueBuffer, sizeof(kv.valueBuffer), "%.9g", value);
  kv.valueBuffer[sizeof(kv.valueBuffer) - 1] = 0;
  kv.quoteValue = false;
  list.push_back(kv);
}

#endif
