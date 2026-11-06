#include "ESP32HTTPClient.h"

#include <Arduino.h>

ESP32HTTPClient::ESP32HTTPClient(const char* baseUrl, int port)
    : _baseUrl(baseUrl),
      _port(port),
      _lastStatusCode(0),
      _timeout(60000),
      _maxRetry(1),
      _followRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS),
      _redirectLimit(10),
      _contentType("application/json"),
      _soapVersion(SOAP_1_1),
      _tlsCaCert(nullptr),
      _tlsCaBundle(nullptr),
      _tlsCaBundleSize(0),
      _tlsClientCert(nullptr),
      _tlsClientKey(nullptr),
      _tlsTrust(TLS_TRUST_NONE),
      _transport(TRANSPORT_NONE),
      _requireValidatedTls(ESP32HTTPCLIENT_REQUIRE_VALIDATED_TLS != 0),
      _onSuccessCb(nullptr),
      _onErrorCb(nullptr),
      _onResponseCb(nullptr),
      _observabilityCb(nullptr) {
  _http.setReuse(true);
}

void ESP32HTTPClient::setBaseUrl(const char* baseUrl, int port) {
  end();
  _baseUrl = baseUrl;
  _port = port;
}

void ESP32HTTPClient::setUrl(const char* baseUrl, int port) {
  setBaseUrl(baseUrl, port);
}

void ESP32HTTPClient::setPort(int port) {
  end();
  _port = port;
}

const char* ESP32HTTPClient::getBaseUrl() const {
  return _baseUrl;
}

int ESP32HTTPClient::getPort() const {
  return _port;
}

void ESP32HTTPClient::setTimeout(uint16_t timeoutMs) {
  _timeout = timeoutMs;
}

uint16_t ESP32HTTPClient::getTimeout() const {
  return _timeout;
}

void ESP32HTTPClient::setMaxRetry(int maxRetry) {
  _maxRetry = (maxRetry < 0) ? 0 : maxRetry;
}

int ESP32HTTPClient::getMaxRetry() const {
  return _maxRetry;
}

void ESP32HTTPClient::setFollowRedirects(followRedirects_t follow) {
  _http.setFollowRedirects(follow);
}

void ESP32HTTPClient::setRedirectLimit(uint16_t limit) {
  _redirectLimit = limit;
  _http.setRedirectLimit(limit);
}

uint16_t ESP32HTTPClient::getRedirectLimit() const {
  return _redirectLimit;
}

void ESP32HTTPClient::setContentType(const char* contentType) {
  _contentType = contentType;
}

static size_t base64Encode(const unsigned char* src, size_t len, char* dst, size_t dstLen) {
  static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  size_t olen = 4 * ((len + 2) / 3);
  if (dstLen < olen + 1) return 0;

  size_t i = 0, j = 0;
  for (i = 0; i < len - (len % 3); i += 3) {
    dst[j++] = b64[(src[i] >> 2) & 0x3F];
    dst[j++] = b64[((src[i] & 0x3) << 4) | ((src[i + 1] >> 4) & 0x0F)];
    dst[j++] = b64[((src[i + 1] & 0x0F) << 2) | ((src[i + 2] >> 6) & 0x03)];
    dst[j++] = b64[src[i + 2] & 0x3F];
  }

  if (len % 3 == 1) {
    dst[j++] = b64[(src[i] >> 2) & 0x3F];
    dst[j++] = b64[(src[i] & 0x3) << 4];
    dst[j++] = '=';
    dst[j++] = '=';
  } else if (len % 3 == 2) {
    dst[j++] = b64[(src[i] >> 2) & 0x3F];
    dst[j++] = b64[((src[i] & 0x3) << 4) | ((src[i + 1] >> 4) & 0x0F)];
    dst[j++] = b64[(src[i + 1] & 0x0F) << 2];
    dst[j++] = '=';
  }

  dst[j] = '\0';
  return j;
}

void ESP32HTTPClient::setHeader(const char* name, const char* value) {
  for (size_t i = 0; i < _headers.size(); i++) {
    if (strcasecmp(_headers[i].name, name) == 0) {
      strncpy(_headers[i].value, value, sizeof(_headers[i].value) - 1);
      _headers[i].value[sizeof(_headers[i].value) - 1] = '\0';
      return;
    }
  }
  HttpHeader header;
  strncpy(header.name, name, sizeof(header.name) - 1);
  header.name[sizeof(header.name) - 1] = '\0';
  strncpy(header.value, value, sizeof(header.value) - 1);
  header.value[sizeof(header.value) - 1] = '\0';
  _headers.push_back(header);
}

void ESP32HTTPClient::bearer(const char* token) {
  char authHeader[256];
  snprintf(authHeader, sizeof(authHeader), "Bearer %s", token ? token : "");
  setHeader("Authorization", authHeader);
}

void ESP32HTTPClient::basic(const char* user, const char* password) {
  char creds[128];
  snprintf(creds, sizeof(creds), "%s:%s", user ? user : "", password ? password : "");
  char encoded[192];
  base64Encode((const unsigned char*)creds, strlen(creds), encoded, sizeof(encoded));
  char authHeader[256];
  snprintf(authHeader, sizeof(authHeader), "Basic %s", encoded);
  setHeader("Authorization", authHeader);
}

void ESP32HTTPClient::apiKey(const char* name, const char* key) {
  setHeader(name, key ? key : "");
}

ESP32HTTPClient& ESP32HTTPClient::cookie(const char* name, const char* value) {
  if (!name || !value) return *this;
  String currentCookies = "";
  for (size_t i = 0; i < _headers.size(); i++) {
    if (strcasecmp(_headers[i].name, "Cookie") == 0) {
      currentCookies = _headers[i].value;
      break;
    }
  }
  if (currentCookies.length() > 0) {
    currentCookies += "; ";
  }
  currentCookies += name;
  currentCookies += "=";
  currentCookies += value;
  setHeader("Cookie", currentCookies.c_str());
  return *this;
}

void ESP32HTTPClient::invalidateTlsSession() {
  _http.setReuse(false);
  _http.end();
  _http.setReuse(true);
#if ESP32HTTPCLIENT_HAS_TLS
  if (_secure.get()) {
    _secure.get()->stop();
  }
#endif
  _transport = TRANSPORT_NONE;
}

void ESP32HTTPClient::end() {
  invalidateTlsSession();
}

void ESP32HTTPClient::setCACert(const char* rootCA) {
  invalidateTlsSession();
  _tlsCaCert = rootCA;
  _tlsCaBundle = nullptr;
  _tlsCaBundleSize = 0;
  _tlsTrust = TLS_TRUST_CA;
#if ESP32HTTPCLIENT_HAS_TLS
  esp32_http_client_detail::SecureClient* sec = _secure.get();
  if (!sec) {
    sec = &_secure.create();
  }
  sec->setCACert(rootCA);
#endif
}

void ESP32HTTPClient::setCACertBundle(const uint8_t* bundle, size_t size) {
  invalidateTlsSession();
  _tlsCaBundle = bundle;
  _tlsCaBundleSize = size;
  _tlsCaCert = nullptr;
  _tlsTrust = TLS_TRUST_BUNDLE;
#if ESP32HTTPCLIENT_HAS_TLS
  esp32_http_client_detail::SecureClient* sec = _secure.get();
  if (!sec) {
    sec = &_secure.create();
  }
  esp32_http_client_detail::applyCACertBundle(*sec, bundle, size, 0);
#endif
}

void ESP32HTTPClient::setClientCert(const char* cert, const char* key) {
  invalidateTlsSession();
  _tlsClientCert = cert;
  _tlsClientKey = key;
#if ESP32HTTPCLIENT_HAS_TLS
  esp32_http_client_detail::SecureClient* sec = _secure.get();
  if (!sec) {
    sec = &_secure.create();
  }
  sec->setCertificate(cert);
  sec->setPrivateKey(key);
#endif
}

void ESP32HTTPClient::setInsecure() {
  invalidateTlsSession();
  _tlsCaCert = nullptr;
  _tlsCaBundle = nullptr;
  _tlsCaBundleSize = 0;
  _tlsTrust = TLS_TRUST_INSECURE;
  ESP32HTTPCLIENT_LOG_W("[ESP32HTTPClient] WARNING: setInsecure() disables TLS certificate and hostname validation (MITM risk).");
#if ESP32HTTPCLIENT_HAS_TLS
  esp32_http_client_detail::SecureClient* sec = _secure.get();
  if (!sec) {
    sec = &_secure.create();
  }
  sec->setInsecure();
#endif
}

void ESP32HTTPClient::requireValidatedTls(bool require) {
  _requireValidatedTls = require;
}

bool ESP32HTTPClient::isValidatedTlsRequired() const {
  return _requireValidatedTls;
}

bool ESP32HTTPClient::hasValidatedTlsConfig() const {
  return (_tlsTrust == TLS_TRUST_CA && _tlsCaCert != nullptr && _tlsCaCert[0] != '\0') ||
         (_tlsTrust == TLS_TRUST_BUNDLE && _tlsCaBundle != nullptr);
}

int ESP32HTTPClient::getTlsError() const {
#if ESP32HTTPCLIENT_HAS_TLS
  if (_secure.get()) {
    char buf[64] = {0};
    return _secure.get()->lastError(buf, sizeof(buf));
  }
#endif
  return 0;
}

String ESP32HTTPClient::getTlsErrorMessage() const {
#if ESP32HTTPCLIENT_HAS_TLS
  if (_secure.get()) {
    char buf[128] = {0};
    int err = _secure.get()->lastError(buf, sizeof(buf));
    if (err != 0 || buf[0] != '\0') {
      return String(buf);
    }
  }
#endif
  return "";
}

String ESP32HTTPClient::tlsErrorToHint(int tlsError, long currentEpoch) {
  if (tlsError == 0) {
    return "";
  }

  int absErr = (tlsError < 0) ? -tlsError : tlsError;
  int highErr = absErr & 0xFF80;
  int lowErr = absErr & 0x007F;

  time_t epoch = (currentEpoch >= 0)
                     ? static_cast<time_t>(currentEpoch)
                     : esp32_http_client_detail::currentEpoch();
  bool clockBefore2023 = (epoch < esp32_http_client_detail::kMinValidTlsEpoch);

  // MBEDTLS_ERR_X509_CERT_VERIFY_FAILED (0x2700)
  if (absErr == 0x2700 || highErr == 0x2700) {
    if (clockBefore2023) {
      return "System clock is before 2023 (not synchronized via NTP); call configTime() before HTTPS. Other causes: expired certificate, wrong Root CA, or hostname mismatch.";
    }
    return "Certificate verification failed: check if the certificate or Root CA expired, the Root CA is wrong (inspect with openssl s_client -showcerts), or the URL hostname does not match the certificate CN/SAN.";
  }

  // Invalid PEM / X.509 / PK format or bad input data
  if (absErr == 0x2180 || highErr == 0x2180 || absErr == 0x2200 || highErr == 0x2200 ||
      absErr == 0x2300 || highErr == 0x2300 || absErr == 0x2500 || highErr == 0x2500 ||
      absErr == 0x2600 || highErr == 0x2600 || absErr == 0x3D00 || highErr == 0x3D00 ||
      absErr == 0x3E80 || highErr == 0x3E80) {
    return "Invalid certificate or private key format: check PEM headers (-----BEGIN CERTIFICATE-----), line breaks, null-termination, and ensure the buffer pointer stays valid in memory.";
  }

  // Memory allocation failure during TLS handshake
  if (absErr == 0x7F00 || highErr == 0x7F00 || absErr == 0x2880 || highErr == 0x2880 ||
      absErr == 0x4D80 || highErr == 0x4D80 || lowErr == 0x0010) {
    return "Insufficient free heap for TLS (~45 KB required): call client.end() on idle connections to release mbedTLS buffers.";
  }

  // Fatal TLS alert from server (0x7780)
  if (absErr == 0x7780 || highErr == 0x7780) {
    return "Fatal TLS alert from server: check if mutual TLS (setClientCert) is required, the Root CA/hostname is wrong, or the TLS version/cipher is unsupported.";
  }

  // Peer closed connection during handshake (0x7280 / 0x7880)
  if (absErr == 0x7280 || highErr == 0x7280 || absErr == 0x7880 || highErr == 0x7880) {
    return "Peer closed connection during TLS handshake: verify target port, SNI hostname (use domain name instead of raw IP), or mTLS client certificate.";
  }

  // Client certificate required (mTLS) (0x7100 / 0x7680)
  if (absErr == 0x7100 || highErr == 0x7100 || absErr == 0x7680 || highErr == 0x7680) {
    return "Client certificate required by server (mTLS): configure setClientCert(cert, key).";
  }

  // Socket timeout or reset during handshake
  if (absErr == 0x6800 || highErr == 0x6800 || lowErr == 0x004C || lowErr == 0x004E || lowErr == 0x0050) {
    return "Network timeout or connection reset during TLS handshake: check WiFi signal, firewall, or increase timeout with setTimeout().";
  }

  // TCP connect or DNS failure
  if (lowErr == 0x0042 || lowErr == 0x0052) {
    return "TCP connection or DNS lookup failed before TLS handshake: verify hostname, DNS, and port.";
  }

  if (clockBefore2023) {
    return "System clock is before 2023 (synchronize via NTP with configTime); also verify Root CA, certificate expiration, and hostname.";
  }
  return "TLS handshake failed: verify Root CA (setCACert/setCACertBundle), certificate expiration, hostname match, and NTP time sync.";
}

String ESP32HTTPClient::getTlsErrorHint() const {
  if (_lastStatusCode == HTTPC_ERROR_TLS_NOT_VALIDATED) {
    if (_tlsTrust == TLS_TRUST_INSECURE) {
      return "HTTPS blocked by requireValidatedTls(true): setInsecure() is not allowed. Configure a Root CA with setCACert() or a bundle with setCACertBundle().";
    }
    return "HTTPS blocked by requireValidatedTls(true): no Root CA or bundle configured. Call setCACert() or setCACertBundle() before making HTTPS requests.";
  }

  int tlsErr = getTlsError();
  if (tlsErr != 0) {
    return tlsErrorToHint(tlsErr);
  }

  if (_lastStatusCode < 0 && (_transport == TRANSPORT_SECURE || _transport == TRANSPORT_DEFAULT_TLS)) {
    time_t epoch = esp32_http_client_detail::currentEpoch();
    if (epoch < esp32_http_client_detail::kMinValidTlsEpoch) {
      return "System clock is before 2023 (not synchronized via NTP); call configTime() before HTTPS. Also verify Root CA, certificate expiration, and hostname.";
    }
    if (_transport == TRANSPORT_DEFAULT_TLS) {
      return "HTTPS failed without a configured Root CA: call setCACert() or setCACertBundle(), ensure NTP clock sync, and check hostname.";
    }
  }

  return "";
}

bool ESP32HTTPClient::hasTlsConfig() const {
  return (_tlsTrust != TLS_TRUST_NONE) ||
         (_tlsClientCert != nullptr && _tlsClientKey != nullptr);
}

bool ESP32HTTPClient::beginConnection(const String& url, uint16_t timeoutMs) {
  if (timeoutMs > 0) {
    _http.setTimeout(timeoutMs);
  } else if (_timeout > 0) {
    _http.setTimeout(_timeout);
  }

  bool isHttps = false;
  int protoEnd = url.indexOf("://");
  if (protoEnd > 0) {
    String scheme = url.substring(0, protoEnd);
    if (scheme.equalsIgnoreCase("https")) {
      isHttps = true;
    }
  }

  if (isHttps) {
    if (_requireValidatedTls && !hasValidatedTlsConfig()) {
      if (_tlsTrust == TLS_TRUST_INSECURE) {
        ESP32HTTPCLIENT_LOG_E("[ESP32HTTPClient] ERROR: HTTPS connection refused by requireValidatedTls(true): setInsecure() is not allowed. Configure setCACert() or setCACertBundle().");
      } else {
        ESP32HTTPCLIENT_LOG_E("[ESP32HTTPClient] ERROR: HTTPS connection refused by requireValidatedTls(true): no Root CA or CA bundle configured. Call setCACert() or setCACertBundle().");
      }
      _lastStatusCode = HTTPC_ERROR_TLS_NOT_VALIDATED;
      return false;
    }

    if (!hasValidatedTlsConfig()) {
      if (_tlsTrust == TLS_TRUST_INSECURE) {
        ESP32HTTPCLIENT_LOG_W("[ESP32HTTPClient] WARNING: Executing HTTPS request with setInsecure() (certificate validation disabled).");
      } else {
        ESP32HTTPCLIENT_LOG_W("[ESP32HTTPClient] WARNING: HTTPS request without setCACert() or setCACertBundle() (using unvalidated default TLS). Call setCACert(), setCACertBundle(), or requireValidatedTls(true).");
      }
    }
  }

  TransportKind target = TRANSPORT_PLAIN;
  if (isHttps) {
    target = hasTlsConfig() ? TRANSPORT_SECURE : TRANSPORT_DEFAULT_TLS;
  } else {
    target = TRANSPORT_PLAIN;
  }

  if (_transport != TRANSPORT_NONE && _transport != target) {
    end();
  }

#if ESP32HTTPCLIENT_HAS_TLS
  if (target == TRANSPORT_SECURE) {
    esp32_http_client_detail::SecureClient* sec = _secure.get();
    if (!sec) {
      sec = &_secure.create();
    }
    if (_tlsTrust == TLS_TRUST_INSECURE) {
      sec->setInsecure();
    } else if (_tlsTrust == TLS_TRUST_CA && _tlsCaCert) {
      sec->setCACert(_tlsCaCert);
    } else if (_tlsTrust == TLS_TRUST_BUNDLE && _tlsCaBundle) {
      esp32_http_client_detail::applyCACertBundle(*sec, _tlsCaBundle, _tlsCaBundleSize, 0);
    }
    if (_tlsClientCert && _tlsClientKey) {
      sec->setCertificate(_tlsClientCert);
      sec->setPrivateKey(_tlsClientKey);
    }
    _transport = target;
    return _http.begin(*sec, url);
  }
#endif

  _transport = target;
  return _http.begin(url);
}

int ESP32HTTPClient::getStatusCode() const {
  return _lastStatusCode;
}

String ESP32HTTPClient::errorToString(int code) {
  switch (code) {
    case -1: return "Connection Refused";
    case -2: return "Send Header Failed";
    case -3: return "Send Payload Failed";
    case -4: return "Not Connected";
    case -5: return "Connection Lost";
    case -6: return "No Stream";
    case -7: return "No HTTP Server";
    case -8: return "Too Less RAM";
    case -9: return "Encoding Error";
    case -10: return "Stream Write Error";
    case -11: return "Read Timeout";
    case -12: return "TLS Validation Required";
    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 204: return "No Content";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 408: return "Request Timeout";
    case 409: return "Conflict";
    case 429: return "Too Many Requests";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    case 504: return "Gateway Timeout";
    case 0: return "Not Executed";
    default:
      if (code < 0) return "Unknown Client Error";
      if (code >= 200 && code < 300) return "Success";
      if (code >= 300 && code < 400) return "Redirection";
      if (code >= 400 && code < 500) return "Client Error";
      if (code >= 500 && code < 600) return "Server Error";
      return "Unknown HTTP Status";
  }
}

String ESP32HTTPClient::getErrorMessage() const {
  String msg = errorToString(_lastStatusCode);
  if (_lastStatusCode == HTTPC_ERROR_TLS_NOT_VALIDATED) {
    String hint = getTlsErrorHint();
    if (!hint.isEmpty()) {
      msg += " (";
      msg += hint;
      msg += ")";
    }
    return msg;
  }

  if (_lastStatusCode < 0) {
    int tlsErr = getTlsError();
    if (tlsErr != 0) {
      char hexBuf[16];
      snprintf(hexBuf, sizeof(hexBuf), "-0x%04X", tlsErr < 0 ? -tlsErr : tlsErr);
      String tlsMsg = getTlsErrorMessage();
      String hint = getTlsErrorHint();
      msg += " (TLS ";
      msg += hexBuf;
      if (!tlsMsg.isEmpty()) {
        msg += ": ";
        msg += tlsMsg;
      }
      if (!hint.isEmpty()) {
        msg += " | Hint: ";
        msg += hint;
      }
      msg += ")";
    } else if (_transport == TRANSPORT_SECURE || _transport == TRANSPORT_DEFAULT_TLS) {
      time_t epoch = esp32_http_client_detail::currentEpoch();
      if (epoch < esp32_http_client_detail::kMinValidTlsEpoch) {
        String hint = getTlsErrorHint();
        if (!hint.isEmpty()) {
          msg += " (Hint: ";
          msg += hint;
          msg += ")";
        }
      }
    }
  }
  return msg;
}

bool ESP32HTTPClient::isSuccess() const {
  return _lastStatusCode >= 200 && _lastStatusCode < 300;
}

bool ESP32HTTPClient::hasError() const {
  return _lastStatusCode < 200 || _lastStatusCode >= 400;
}

void ESP32HTTPClient::onSuccess(HttpResponseCallback cb) {
  _onSuccessCb = cb;
}

void ESP32HTTPClient::onError(HttpErrorCallback cb) {
  _onErrorCb = cb;
}

void ESP32HTTPClient::onError(HttpResponseCallback cb) {
  if (cb) {
    _onErrorCb = [cb](int code, const char*) { cb(code); };
  } else {
    _onErrorCb = nullptr;
  }
}

void ESP32HTTPClient::onResponse(HttpResponseCallback cb) {
  _onResponseCb = cb;
}

void ESP32HTTPClient::onObservability(ObservabilityCallback cb) {
  _observabilityCb = cb;
}

void ESP32HTTPClient::setSoapVersion(SoapVersion version) {
  _soapVersion = version;
}

SoapVersion ESP32HTTPClient::getSoapVersion() const {
  return _soapVersion;
}

RestRequest ESP32HTTPClient::get(const char* path) {
  return RestRequest(this, path, HTTP_GET_METHOD);
}

RestRequest ESP32HTTPClient::post(const char* path) {
  return RestRequest(this, path, HTTP_POST_METHOD);
}

RestRequest ESP32HTTPClient::update(const char* path) {
  return RestRequest(this, path, HTTP_PUT_METHOD);
}

RestRequest ESP32HTTPClient::put(const char* path) {
  return RestRequest(this, path, HTTP_PUT_METHOD);
}

RestRequest ESP32HTTPClient::patch(const char* path) {
  return RestRequest(this, path, HTTP_PATCH_METHOD);
}

RestRequest ESP32HTTPClient::del(const char* path) {
  return RestRequest(this, path, HTTP_DELETE_METHOD);
}

SoapRequest ESP32HTTPClient::soap(const char* path) {
  return SoapRequest(this, path, _soapVersion);
}

GraphQLRequest ESP32HTTPClient::graphql(const char* path) {
  return GraphQLRequest(this, path, HTTP_POST_METHOD);
}

GraphQLRequest ESP32HTTPClient::graphqlGet(const char* path) {
  return GraphQLRequest(this, path, HTTP_GET_METHOD);
}

GraphQLRequest ESP32HTTPClient::graphqlPost(const char* path) {
  return GraphQLRequest(this, path, HTTP_POST_METHOD);
}

JsonRpcRequest ESP32HTTPClient::jsonRpc(const char* path) {
  return JsonRpcRequest(this, path);
}

JsonRpcRequest ESP32HTTPClient::jsonrpc(const char* path) {
  return JsonRpcRequest(this, path);
}

