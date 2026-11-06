#ifndef ESP32_HTTP_CLIENT_H
#define ESP32_HTTP_CLIENT_H

#include <Arduino.h>
#include <vector>
#include <HTTPClient.h>

#include "BufferedStreamReader.h"
#include "ESP32HTTPClientTLS.h"
#include "GraphQLRequest.h"
#include "GraphQLTypes.h"
#include "JsonRpcRequest.h"
#include "JsonRpcTypes.h"
#include "RestRequest.h"
#include "RestTypes.h"
#include "SoapRequest.h"
#include "SoapTypes.h"

class ESP32HTTPClient {
  friend class RestRequest;
  friend class SoapRequest;
  friend class GraphQLRequest;
  friend class JsonRpcRequest;

 public:
  ESP32HTTPClient(const char* baseUrl, int port = 0);

  RestRequest get(const char* path);
  RestRequest post(const char* path);
  RestRequest update(const char* path);
  RestRequest put(const char* path);
  RestRequest patch(const char* path);
  RestRequest del(const char* path);

  SoapRequest soap(const char* path = "");

  GraphQLRequest graphql(const char* path = "/graphql");
  GraphQLRequest graphqlGet(const char* path = "/graphql");
  GraphQLRequest graphqlPost(const char* path = "/graphql");

  JsonRpcRequest jsonRpc(const char* path = "");
  JsonRpcRequest jsonrpc(const char* path = "");

  void setBaseUrl(const char* baseUrl, int port = 0);
  void setUrl(const char* baseUrl, int port = 0);
  void setPort(int port);
  const char* getBaseUrl() const;
  int getPort() const;

  void setFollowRedirects(followRedirects_t follow);
  void setRedirectLimit(uint16_t limit);
  uint16_t getRedirectLimit(void) const {
    return _redirectLimit;
  }

  void setTimeout(uint16_t timeoutMs);
  uint16_t getTimeout() const;

  void setMaxRetry(int maxRetry);
  int getMaxRetry() const;

  void setContentType(const char* contentType);
  void setHeader(const char* name, const char* value);
  void bearer(const char* token);
  void basic(const char* user, const char* password);
  void apiKey(const char* name, const char* key);
  ESP32HTTPClient& cookie(const char* name, const char* value);
  void end();

  void setCACert(const char* rootCA);
  void setCACertBundle(const uint8_t* bundle, size_t size = 0);
  void setClientCert(const char* cert, const char* key);
  void setInsecure();
  void requireValidatedTls(bool require = true);
  bool isValidatedTlsRequired() const;
  bool hasValidatedTlsConfig() const;
  int getTlsError() const;
  String getTlsErrorMessage() const;
  String getTlsErrorHint() const;
  static String tlsErrorToHint(int tlsError, long currentEpoch = -1);

  void setSoapVersion(SoapVersion version);
  SoapVersion getSoapVersion() const;

  int getStatusCode() const;
  String getErrorMessage() const;
  static String errorToString(int code);
  bool isSuccess() const;
  bool hasError() const;

  void onSuccess(HttpResponseCallback cb);
  void onError(HttpErrorCallback cb);
  void onError(HttpResponseCallback cb);
  void onResponse(HttpResponseCallback cb);
  void onObservability(ObservabilityCallback cb);

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
  enum TransportKind : uint8_t {
    TRANSPORT_NONE = 0,
    TRANSPORT_PLAIN,
    TRANSPORT_DEFAULT_TLS,
    TRANSPORT_SECURE
  };

  enum TlsTrustMode : uint8_t {
    TLS_TRUST_NONE = 0,
    TLS_TRUST_CA,
    TLS_TRUST_BUNDLE,
    TLS_TRUST_INSECURE
  };

  bool beginConnection(const String& url, uint16_t timeoutMs = 0);
  bool hasTlsConfig() const;
  void invalidateTlsSession();

  const char* _baseUrl;
  int _port;
  int _lastStatusCode;
  uint16_t _timeout;
  int _maxRetry;
  followRedirects_t _followRedirects;
  uint16_t _redirectLimit;
  const char* _contentType;
  SoapVersion _soapVersion;
  std::vector<HttpHeader> _headers;

  const char* _tlsCaCert;
  const uint8_t* _tlsCaBundle;
  size_t _tlsCaBundleSize;
  const char* _tlsClientCert;
  const char* _tlsClientKey;
  uint8_t _tlsTrust;
  uint8_t _transport;
  bool _requireValidatedTls;

#if ESP32HTTPCLIENT_HAS_TLS
  esp32_http_client_detail::SecureClientSlot _secure;
#endif
  HTTPClient _http;

  HttpResponseCallback _onSuccessCb;
  HttpErrorCallback _onErrorCb;
  HttpResponseCallback _onResponseCb;
  ObservabilityCallback _observabilityCb;
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
