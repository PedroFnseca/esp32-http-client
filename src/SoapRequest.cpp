#include "SoapRequest.h"

#include <HTTPClient.h>
#include <cctype>
#include <cstdlib>
#include <cstring>

#include "BufferedStreamReader.h"
#include "ESP32HTTPClient.h"

static String stripNamespace(const String& tag) {
  int colon = tag.indexOf(':');
  if (colon >= 0) {
    return tag.substring(colon + 1);
  }
  return tag;
}

static String decodeXmlEntities(const String& src) {
  String out;
  out.reserve(src.length());
  size_t len = src.length();
  for (size_t i = 0; i < len; i++) {
    if (src[i] == '&') {
      int semi = src.indexOf(';', i);
      if (semi > (int)i) {
        String entity = src.substring(i + 1, semi);
        if (entity == "lt") {
          out += '<';
          i = semi;
          continue;
        } else if (entity == "gt") {
          out += '>';
          i = semi;
          continue;
        } else if (entity == "amp") {
          out += '&';
          i = semi;
          continue;
        } else if (entity == "quot") {
          out += '"';
          i = semi;
          continue;
        } else if (entity == "apos") {
          out += '\'';
          i = semi;
          continue;
        } else if (entity.startsWith("#x") || entity.startsWith("#X")) {
          long val = strtol(entity.c_str() + 2, nullptr, 16);
          if (val > 0 && val < 256) {
            out += (char)val;
            i = semi;
            continue;
          }
        } else if (entity.startsWith("#")) {
          long val = strtol(entity.c_str() + 1, nullptr, 10);
          if (val > 0 && val < 256) {
            out += (char)val;
            i = semi;
            continue;
          }
        }
      }
    }
    out += src[i];
  }
  return out;
}

static String trimString(const String& str) {
  int start = 0;
  while (start < (int)str.length() && (str[start] == ' ' || str[start] == '\t' || str[start] == '\r' || str[start] == '\n')) {
    start++;
  }
  int end = (int)str.length() - 1;
  while (end >= start && (str[end] == ' ' || str[end] == '\t' || str[end] == '\r' || str[end] == '\n')) {
    end--;
  }
  if (start > end) return "";
  return str.substring(start, end + 1);
}

static void applyBindingValue(ResponseBinding& binding, const String& rawValue) {
  if (!binding.target) return;
  String val = trimString(rawValue);

  if (binding.type == TYPE_ARDUINO_STRING) {
    *((String*)binding.target) = rawValue;
  } else if (binding.type == TYPE_STRING) {
    char* dst = (char*)binding.target;
    if (binding.size > 0) {
      strncpy(dst, rawValue.c_str(), binding.size - 1);
      dst[binding.size - 1] = '\0';
    }
  } else if (binding.type == TYPE_INT) {
    *(int*)binding.target = atoi(val.c_str());
  } else if (binding.type == TYPE_LONG) {
    *(long*)binding.target = atol(val.c_str());
  } else if (binding.type == TYPE_FLOAT) {
    *(float*)binding.target = strtof(val.c_str(), nullptr);
  } else if (binding.type == TYPE_DOUBLE) {
    *(double*)binding.target = strtod(val.c_str(), nullptr);
  } else if (binding.type == TYPE_BOOL) {
    *(bool*)binding.target = (val.equalsIgnoreCase("true") || val == "1");
  }
}

SoapRequest::SoapRequest(ESP32HTTPClient* client, const char* path, SoapVersion version)
    : _client(client),
      _path(path),
      _version(version),
      _executed(false),
      _timeout(0),
      _maxRetry(-1),
      _onSuccessCb(nullptr),
      _onErrorCb(nullptr),
      _onResponseCb(nullptr),
      _onFaultCb(nullptr),
      _faultTarget(nullptr),
      _rawResponseTarget(nullptr) {
}

SoapRequest::SoapRequest(SoapRequest&& other)
    : _client(other._client),
      _path(other._path),
      _version(other._version),
      _executed(other._executed),
      _timeout(other._timeout),
      _maxRetry(other._maxRetry),
      _soapAction(std::move(other._soapAction)),
      _bodyXml(std::move(other._bodyXml)),
      _headerXml(std::move(other._headerXml)),
      _rawEnvelope(std::move(other._rawEnvelope)),
      _onSuccessCb(std::move(other._onSuccessCb)),
      _onErrorCb(std::move(other._onErrorCb)),
      _onResponseCb(std::move(other._onResponseCb)),
      _onFaultCb(std::move(other._onFaultCb)),
      _faultTarget(other._faultTarget),
      _rawResponseTarget(other._rawResponseTarget),
      _pathParams(std::move(other._pathParams)),
      _queryParams(std::move(other._queryParams)),
      _responseBindings(std::move(other._responseBindings)),
      _headerBindings(std::move(other._headerBindings)) {
  other._executed = true;
}

SoapRequest::~SoapRequest() {
  if (!_executed) {
    execute();
  }
}

SoapRequest& SoapRequest::version(SoapVersion version) {
  _version = version;
  return *this;
}

SoapRequest& SoapRequest::setVersion(SoapVersion version) {
  return this->version(version);
}

SoapRequest& SoapRequest::soapAction(const char* action) {
  _soapAction = action ? action : "";
  return *this;
}

SoapRequest& SoapRequest::action(const char* action) {
  return soapAction(action);
}

SoapRequest& SoapRequest::body(const char* xmlPayload) {
  _bodyXml = xmlPayload ? xmlPayload : "";
  return *this;
}

SoapRequest& SoapRequest::body(const String& xmlPayload) {
  _bodyXml = xmlPayload;
  return *this;
}

SoapRequest& SoapRequest::headerXml(const char* headerXml) {
  _headerXml = headerXml ? headerXml : "";
  return *this;
}

SoapRequest& SoapRequest::headerXml(const String& headerXml) {
  _headerXml = headerXml;
  return *this;
}

SoapRequest& SoapRequest::rawEnvelope(const char* envelopeXml) {
  _rawEnvelope = envelopeXml ? envelopeXml : "";
  return *this;
}

SoapRequest& SoapRequest::rawEnvelope(const String& envelopeXml) {
  _rawEnvelope = envelopeXml;
  return *this;
}

SoapRequest& SoapRequest::timeout(uint16_t timeoutMs) {
  _timeout = timeoutMs;
  return *this;
}

SoapRequest& SoapRequest::maxRetry(int maxRetry) {
  _maxRetry = (maxRetry < 0) ? 0 : maxRetry;
  return *this;
}

SoapRequest& SoapRequest::retry(int maxRetry) {
  return this->maxRetry(maxRetry);
}

SoapRequest& SoapRequest::onSuccess(HttpResponseCallback cb) {
  _onSuccessCb = cb;
  return *this;
}

SoapRequest& SoapRequest::onError(HttpErrorCallback cb) {
  _onErrorCb = cb;
  return *this;
}

SoapRequest& SoapRequest::onError(HttpResponseCallback cb) {
  if (cb) {
    _onErrorCb = [cb](int code, const char*) { cb(code); };
  } else {
    _onErrorCb = nullptr;
  }
  return *this;
}

SoapRequest& SoapRequest::onResponse(HttpResponseCallback cb) {
  _onResponseCb = cb;
  return *this;
}

SoapRequest& SoapRequest::onFault(SoapFaultCallback cb) {
  _onFaultCb = cb;
  return *this;
}

SoapRequest& SoapRequest::getBody(const char* xmlPath, int* target) {
  _responseBindings.push_back({xmlPath, target, TYPE_INT, 0});
  return *this;
}

SoapRequest& SoapRequest::getBody(const char* xmlPath, float* target) {
  _responseBindings.push_back({xmlPath, target, TYPE_FLOAT, 0});
  return *this;
}

SoapRequest& SoapRequest::getBody(const char* xmlPath, double* target) {
  _responseBindings.push_back({xmlPath, target, TYPE_DOUBLE, 0});
  return *this;
}

SoapRequest& SoapRequest::getBody(const char* xmlPath, bool* target) {
  _responseBindings.push_back({xmlPath, target, TYPE_BOOL, 0});
  return *this;
}

SoapRequest& SoapRequest::getBody(const char* xmlPath, long* target) {
  _responseBindings.push_back({xmlPath, target, TYPE_LONG, 0});
  return *this;
}

SoapRequest& SoapRequest::getBody(const char* xmlPath, char* target, size_t maxLength) {
  _responseBindings.push_back({xmlPath, target, TYPE_STRING, maxLength});
  return *this;
}

SoapRequest& SoapRequest::getBody(const char* xmlPath, String* target) {
  _responseBindings.push_back({xmlPath, target, TYPE_ARDUINO_STRING, 0});
  return *this;
}

SoapRequest& SoapRequest::getFault(SoapFault* target) {
  _faultTarget = target;
  return *this;
}

SoapRequest& SoapRequest::getRawResponse(String* target) {
  _rawResponseTarget = target;
  return *this;
}

SoapRequest& SoapRequest::getHeader(const char* name, int* target) {
  _headerBindings.push_back({name, target, TYPE_INT, 0});
  return *this;
}

SoapRequest& SoapRequest::getHeader(const char* name, float* target) {
  _headerBindings.push_back({name, target, TYPE_FLOAT, 0});
  return *this;
}

SoapRequest& SoapRequest::getHeader(const char* name, double* target) {
  _headerBindings.push_back({name, target, TYPE_DOUBLE, 0});
  return *this;
}

SoapRequest& SoapRequest::getHeader(const char* name, bool* target) {
  _headerBindings.push_back({name, target, TYPE_BOOL, 0});
  return *this;
}

SoapRequest& SoapRequest::getHeader(const char* name, long* target) {
  _headerBindings.push_back({name, target, TYPE_LONG, 0});
  return *this;
}

SoapRequest& SoapRequest::getHeader(const char* name, char* target, size_t maxLength) {
  _headerBindings.push_back({name, target, TYPE_STRING, maxLength});
  return *this;
}

SoapRequest& SoapRequest::getHeader(const char* name, String* target) {
  _headerBindings.push_back({name, target, TYPE_ARDUINO_STRING, 0});
  return *this;
}

String SoapRequest::buildEnvelope() const {
  if (!_rawEnvelope.isEmpty()) {
    return _rawEnvelope;
  }

  String envelope;
  envelope.reserve(256 + _headerXml.length() + _bodyXml.length());
  envelope += "<?xml version=\"1.0\" encoding=\"utf-8\"?>";

  if (_version == SOAP_1_2) {
    envelope += "<soap12:Envelope xmlns:soap12=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">";
    if (!_headerXml.isEmpty()) {
      envelope += "<soap12:Header>";
      envelope += _headerXml;
      envelope += "</soap12:Header>";
    }
    envelope += "<soap12:Body>";
    envelope += _bodyXml;
    envelope += "</soap12:Body>";
    envelope += "</soap12:Envelope>";
  } else {
    envelope += "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">";
    if (!_headerXml.isEmpty()) {
      envelope += "<soap:Header>";
      envelope += _headerXml;
      envelope += "</soap:Header>";
    }
    envelope += "<soap:Body>";
    envelope += _bodyXml;
    envelope += "</soap:Body>";
    envelope += "</soap:Envelope>";
  }

  return envelope;
}

void SoapRequest::execute() {
  _executed = true;

  if (!_client) return;

  HTTPClient& http = _client->_http;

  uint16_t effectiveTimeout = (_timeout > 0) ? _timeout : _client->_timeout;
  if (effectiveTimeout > 0) {
    http.setTimeout(effectiveTimeout);
  }

  String urlBase;
  urlBase.reserve(128);
  urlBase = _client->_baseUrl;

  if (_client->_port > 0) {
    int protoEnd = urlBase.indexOf("://");
    if (protoEnd != -1) {
      int pathStart = urlBase.indexOf('/', protoEnd + 3);
      if (pathStart != -1) {
        urlBase = urlBase.substring(0, pathStart) + ":" + String(_client->_port) + urlBase.substring(pathStart);
      } else {
        urlBase = urlBase + ":" + String(_client->_port);
      }
    }
  }

  String resolvedPath = _path ? _path : "";
  for (const auto& param : _pathParams) {
    if (!param.key) continue;
    String placeholder;
    if (param.key[0] == '{') {
      placeholder = param.key;
    } else {
      placeholder = "{" + String(param.key) + "}";
    }
    resolvedPath.replace(placeholder, param.valueBuffer);
  }

  String url;
  url.reserve(256);
  url = urlBase;
  url += resolvedPath;

  if (!_queryParams.empty()) {
    url += "?";
    for (size_t i = 0; i < _queryParams.size(); i++) {
      url += _queryParams[i].key;
      url += "=";
      url += _queryParams[i].valueBuffer;
      if (i < _queryParams.size() - 1) url += "&";
    }
  }

  std::vector<const char*> headerKeys;
  if (!_headerBindings.empty()) {
    headerKeys.reserve(_headerBindings.size());
    for (const auto& binding : _headerBindings) {
      if (binding.key && binding.key[0] != '\0') {
        headerKeys.push_back(binding.key);
      }
    }
  }

  http.begin(url);

  if (!headerKeys.empty()) {
    http.collectHeaders(headerKeys.data(), headerKeys.size());
  }

  for (const auto& header : _client->_headers) {
    http.addHeader(header.name, header.value);
  }

  String contentType;
  if (_version == SOAP_1_2) {
    contentType = "application/soap+xml; charset=utf-8";
    if (!_soapAction.isEmpty()) {
      contentType += "; action=\"";
      if (_soapAction.startsWith("\"") && _soapAction.endsWith("\"")) {
        contentType += _soapAction.substring(1, _soapAction.length() - 1);
      } else {
        contentType += _soapAction;
      }
      contentType += "\"";
    }
    http.addHeader("Content-Type", contentType.c_str());
  } else {
    contentType = "text/xml; charset=utf-8";
    http.addHeader("Content-Type", contentType.c_str());

    String formattedAction;
    if (_soapAction.startsWith("\"") && _soapAction.endsWith("\"")) {
      formattedAction = _soapAction;
    } else {
      formattedAction = "\"" + _soapAction + "\"";
    }
    http.addHeader("SOAPAction", formattedAction.c_str());
  }

  String payload = buildEnvelope();

  int code = 0;
  int retries = 0;
  int maxRetries = (_maxRetry >= 0) ? _maxRetry : _client->_maxRetry;
  int maxAttempts = 1 + maxRetries;

  uint32_t freeHeapBefore = 0;
  unsigned long startTime = 0;
  if (_client->_observabilityCb) {
    freeHeapBefore = ESP.getFreeHeap();
    startTime = millis();
  }

  while (retries < maxAttempts) {
    code = http.POST(payload);

    if (code < 0 && retries < maxRetries) {
      http.end();
      http.begin(url);
      if (!headerKeys.empty()) {
        http.collectHeaders(headerKeys.data(), headerKeys.size());
      }
      for (const auto& header : _client->_headers) {
        http.addHeader(header.name, header.value);
      }
      http.addHeader("Content-Type", contentType.c_str());
      if (_version == SOAP_1_1) {
        String formattedAction = (_soapAction.startsWith("\"") && _soapAction.endsWith("\""))
                                     ? _soapAction
                                     : ("\"" + _soapAction + "\"");
        http.addHeader("SOAPAction", formattedAction.c_str());
      }
      retries++;
    } else {
      break;
    }
  }

  unsigned long ttfbTime = 0;
  if (_client->_observabilityCb) {
    ttfbTime = millis();
  }

  _client->_lastStatusCode = code;

  SoapFault localFault;
  SoapFault* faultPtr = _faultTarget ? _faultTarget : &localFault;

  if (code > 0) {
    for (const auto& binding : _headerBindings) {
      if (binding.key && binding.target && http.hasHeader(binding.key)) {
        String val = http.header(binding.key);
        if (binding.type == TYPE_ARDUINO_STRING) {
          *((String*)binding.target) = val;
        } else if (binding.type == TYPE_STRING) {
          char* dst = (char*)binding.target;
          if (binding.size > 0) {
            strncpy(dst, val.c_str(), binding.size - 1);
            dst[binding.size - 1] = '\0';
          }
        } else if (binding.type == TYPE_INT) {
          *(int*)binding.target = atoi(val.c_str());
        } else if (binding.type == TYPE_LONG) {
          *(long*)binding.target = atol(val.c_str());
        } else if (binding.type == TYPE_FLOAT) {
          *(float*)binding.target = strtof(val.c_str(), nullptr);
        } else if (binding.type == TYPE_DOUBLE) {
          *(double*)binding.target = strtod(val.c_str(), nullptr);
        } else if (binding.type == TYPE_BOOL) {
          *(bool*)binding.target = (val.equalsIgnoreCase("true") || val == "1");
        }
      }
    }

    if (http.getSize() > 0 || http.getStreamPtr()) {
      bool isChunked = (http.getSize() == -1);
      BufferedStreamReader reader(http.getStreamPtr(), isChunked);
      parseXmlWithBindings(reader, _responseBindings, faultPtr, _rawResponseTarget);
    }
  }

  http.end();

  if (_onResponseCb) {
    _onResponseCb(code);
  }
  if (_client->_onResponseCb) {
    _client->_onResponseCb(code);
  }

  if (faultPtr->matched && _onFaultCb) {
    _onFaultCb(*faultPtr);
  }

  if (code >= 200 && code < 300 && !faultPtr->matched) {
    if (_onSuccessCb) {
      _onSuccessCb(code);
    }
    if (_client->_onSuccessCb) {
      _client->_onSuccessCb(code);
    }
  } else {
    String errMsg = _client->getErrorMessage();
    if (_onErrorCb) {
      _onErrorCb(code, errMsg.c_str());
    }
    if (_client->_onErrorCb) {
      _client->_onErrorCb(code, errMsg.c_str());
    }
  }

  if (_client->_observabilityCb) {
    ObservabilityMetrics metrics;
    metrics.totalTimeMs = millis() - startTime;
    metrics.ttfbMs = ttfbTime > 0 ? (ttfbTime - startTime) : 0;
    metrics.txBytes = url.length() + payload.length() + 150;
    metrics.rxBytes = http.getSize() > 0 ? http.getSize() : 0;
    metrics.retries = retries;
    metrics.freeHeapBefore = freeHeapBefore;
    metrics.freeHeapAfter = ESP.getFreeHeap();
    _client->_observabilityCb(metrics);
  }
}

void SoapRequest::parseXmlWithBindings(BufferedStreamReader& r,
                                       std::vector<ResponseBinding>& bindings,
                                       SoapFault* fault,
                                       String* rawXml) {
  std::vector<String> tagStack;
  std::vector<String> localTagStack;
  String currentText = "";
  bool inCData = false;

  auto matchesPath = [](const std::vector<String>& stack, const std::vector<String>& localStack, const char* bindPath) -> bool {
    if (!bindPath || bindPath[0] == '\0') return false;

    // Check 1: single element local name matching leaf of stack
    if (!strchr(bindPath, '.') && !strchr(bindPath, ':')) {
      if (!localStack.empty() && localStack.back().equalsIgnoreCase(bindPath)) {
        return true;
      }
    }

    // Check 2: single element exact name matching leaf of stack
    if (!strchr(bindPath, '.')) {
      if (!stack.empty() && stack.back().equalsIgnoreCase(bindPath)) {
        return true;
      }
    }

    // Check 3: dotted path match from end of stack or localStack
    // Build dotted paths
    String fullDotted = "";
    for (size_t i = 0; i < stack.size(); i++) {
      if (i > 0) fullDotted += ".";
      fullDotted += stack[i];
    }
    if (fullDotted.equalsIgnoreCase(bindPath)) return true;

    String localDotted = "";
    for (size_t i = 0; i < localStack.size(); i++) {
      if (i > 0) localDotted += ".";
      localDotted += localStack[i];
    }
    if (localDotted.equalsIgnoreCase(bindPath)) return true;

    // Check 4: suffix match of dotted path (e.g. "Response.Result" matching "Envelope.Body.Response.Result")
    String bindPathStr = bindPath;
    if (localDotted.endsWith("." + bindPathStr) || fullDotted.endsWith("." + bindPathStr)) {
      return true;
    }

    return false;
  };

  while (r.available()) {
    int byte = r.read();
    if (byte == -1) break;
    char c = (char)byte;

    if (rawXml) {
      *rawXml += c;
    }

    if (c == '<') {
      // Check for CDATA, comments, processing instructions, or end tags
      if (r.peek() == '!') {
        char n1 = (char)r.read();
        if (rawXml) *rawXml += n1;

        if (r.peek() == '-') {
          char n2 = (char)r.read();
          if (rawXml) *rawXml += n2;
          // Comment <!-- ... -->
          int dashCount = 0;
          while (r.available()) {
            char cm = (char)r.read();
            if (rawXml) *rawXml += cm;
            if (cm == '-') {
              dashCount++;
            } else if (cm == '>' && dashCount >= 2) {
              break;
            } else {
              dashCount = 0;
            }
          }
          continue;
        } else if (r.peek() == '[') {
          // Check for CDATA: <![CDATA[ ... ]]>
          String cdataHdr = "[";
          char n2 = (char)r.read();
          if (rawXml) *rawXml += n2;
          while (r.available() && cdataHdr.length() < 7) {
            char ch = (char)r.read();
            if (rawXml) *rawXml += ch;
            cdataHdr += ch;
          }
          if (cdataHdr == "[CDATA[") {
            inCData = true;
            int bracketCount = 0;
            while (r.available()) {
              char cd = (char)r.read();
              if (rawXml) *rawXml += cd;
              if (cd == ']') {
                bracketCount++;
              } else if (cd == '>' && bracketCount >= 2) {
                inCData = false;
                break;
              } else {
                while (bracketCount > 0) {
                  currentText += ']';
                  bracketCount--;
                }
                currentText += cd;
              }
            }
          }
          continue;
        } else {
          // Other <! declaration, read until >
          while (r.available()) {
            char decl = (char)r.read();
            if (rawXml) *rawXml += decl;
            if (decl == '>') break;
          }
          continue;
        }
      } else if (r.peek() == '?') {
        // Processing instruction <? ... ?>
        while (r.available()) {
          char pi = (char)r.read();
          if (rawXml) *rawXml += pi;
          if (pi == '?' && r.peek() == '>') {
            char endPi = (char)r.read();
            if (rawXml) *rawXml += endPi;
            break;
          }
        }
        continue;
      } else if (r.peek() == '/') {
        // Closing tag </tag>
        char slash = (char)r.read();
        if (rawXml) *rawXml += slash;

        String closingTag = "";
        while (r.available()) {
          char ct = (char)r.read();
          if (rawXml) *rawXml += ct;
          if (ct == '>') break;
          if (!isspace((unsigned char)ct)) {
            closingTag += ct;
          }
        }

        String decodedText = decodeXmlEntities(currentText);

        // Process bindings
        for (auto& binding : bindings) {
          if (matchesPath(tagStack, localTagStack, binding.key)) {
            applyBindingValue(binding, decodedText);
          }
        }

        // Process SOAP Fault
        if (fault && !localTagStack.empty()) {
          String localCurrent = localTagStack.back();
          String parentLocal = (localTagStack.size() >= 2) ? localTagStack[localTagStack.size() - 2] : "";

          if (localCurrent.equalsIgnoreCase("faultcode") ||
              (localCurrent.equalsIgnoreCase("Value") && parentLocal.equalsIgnoreCase("Code"))) {
            fault->faultCode = trimString(decodedText);
            fault->matched = true;
          } else if (localCurrent.equalsIgnoreCase("faultstring") ||
                     (localCurrent.equalsIgnoreCase("Text") && parentLocal.equalsIgnoreCase("Reason"))) {
            fault->faultString = trimString(decodedText);
            fault->matched = true;
          } else if (localCurrent.equalsIgnoreCase("faultactor") ||
                     localCurrent.equalsIgnoreCase("Node")) {
            fault->faultActor = trimString(decodedText);
            fault->matched = true;
          } else if (localCurrent.equalsIgnoreCase("Role")) {
            if (fault->faultActor.isEmpty()) {
              fault->faultActor = trimString(decodedText);
            }
            fault->matched = true;
          } else if (localCurrent.equalsIgnoreCase("detail")) {
            fault->detail = trimString(decodedText);
            fault->matched = true;
          } else if (localCurrent.equalsIgnoreCase("Fault")) {
            fault->matched = true;
          }
        }

        if (!tagStack.empty()) tagStack.pop_back();
        if (!localTagStack.empty()) localTagStack.pop_back();
        currentText = "";
      } else {
        // Opening tag <tag attr="val"> or <tag/>
        String tagContent = "";
        bool selfClosing = false;
        bool inQuotes = false;
        char quoteChar = 0;

        while (r.available()) {
          char tc = (char)r.read();
          if (rawXml) *rawXml += tc;

          if (inQuotes) {
            if (tc == quoteChar) inQuotes = false;
          } else {
            if (tc == '"' || tc == '\'') {
              inQuotes = true;
              quoteChar = tc;
            } else if (tc == '/') {
              if (r.peek() == '>') {
                selfClosing = true;
              }
            } else if (tc == '>') {
              break;
            }
          }

          if (!selfClosing || tc != '/') {
            tagContent += tc;
          }
        }

        // Extract tag name (first word in tagContent)
        int spaceIdx = 0;
        while (spaceIdx < (int)tagContent.length() && !isspace((unsigned char)tagContent[spaceIdx])) {
          spaceIdx++;
        }
        String tagName = tagContent.substring(0, spaceIdx);
        tagName.trim();

        if (!tagName.isEmpty()) {
          String localName = stripNamespace(tagName);
          if (selfClosing) {
            // Self-closing element: test for empty text match
            tagStack.push_back(tagName);
            localTagStack.push_back(localName);
            for (auto& binding : bindings) {
              if (matchesPath(tagStack, localTagStack, binding.key)) {
                applyBindingValue(binding, "");
              }
            }
            tagStack.pop_back();
            localTagStack.pop_back();
          } else {
            tagStack.push_back(tagName);
            localTagStack.push_back(localName);
            currentText = "";
          }
        }
      }
    } else {
      currentText += c;
    }
  }
}
