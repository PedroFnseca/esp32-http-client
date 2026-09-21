#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>

#include "HTTPClient.h"

#define private public
#include "ESP32HTTPClient.h"
#include "SoapRequest.h"
#undef private

namespace {
int failures = 0;
int checks = 0;
int passedChecks = 0;
int suitesRun = 0;
int suitesPassed = 0;

void expectTrue(bool condition, const char* message) {
  checks++;
  if (!condition) {
    std::cerr << "[FAIL] " << message << "\n";
    failures++;
    return;
  }
  passedChecks++;
}

void expectEq(const std::string& actual, const std::string& expected, const char* message) {
  checks++;
  if (actual != expected) {
    std::cerr << "[FAIL] " << message << " (expected: " << expected << ", got: " << actual << ")\n";
    failures++;
    return;
  }
  passedChecks++;
}

void expectEqInt(long long actual, long long expected, const char* message) {
  checks++;
  if (actual != expected) {
    std::cerr << "[FAIL] " << message << " (expected: " << expected << ", got: " << actual << ")\n";
    failures++;
    return;
  }
  passedChecks++;
}

void expectNear(double actual, double expected, double tolerance, const char* message) {
  checks++;
  if (std::fabs(actual - expected) > tolerance) {
    std::cerr << "[FAIL] " << message << " (expected: " << expected << ", got: " << actual << ")\n";
    failures++;
    return;
  }
  passedChecks++;
}

void runSuite(const char* name, void (*suiteFn)()) {
  const int failuresBefore = failures;
  std::cout << "[RUN ] " << name << "\n";
  suiteFn();
  suitesRun++;

  if (failures == failuresBefore) {
    suitesPassed++;
    std::cout << "[PASS] " << name << "\n";
  } else {
    std::cout << "[FAIL] " << name << " (" << (failures - failuresBefore) << " failure(s))\n";
  }
}

class StringStream : public Stream {
 public:
  explicit StringStream(const std::string& content) : _content(content), _cursor(0) {}

  int available() override {
    return _cursor < _content.size() ? static_cast<int>(_content.size() - _cursor) : 0;
  }

  int read() override {
    if (!available()) return -1;
    return static_cast<unsigned char>(_content[_cursor++]);
  }

  int peek() override {
    if (!available()) return -1;
    return static_cast<unsigned char>(_content[_cursor]);
  }

 private:
  std::string _content;
  size_t _cursor;
};

void testSoap11RequestFormatting() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\"><soap:Body><GetPriceResponse><Price>99.95</Price></GetPriceResponse></soap:Body></soap:Envelope>");

  ESP32HTTPClient client("https://example.com");
  float price = 0.0f;
  {
    client.soap("/ws")
        .soapAction("http://example.org/GetPrice")
        .body("<m:GetPrice xmlns:m=\"http://example.org\"><m:Item>ESP32</m:Item></m:GetPrice>")
        .getBody("Price", &price);
  }

  expectEq(HttpClientStub::lastMethod, "POST", "SOAP should use POST method");
  expectEq(HttpClientStub::lastUrl, "https://example.com/ws", "SOAP url");

  bool foundContentType = false;
  bool foundSoapAction = false;
  for (const auto& h : HttpClientStub::lastHeaders) {
    if (h.first == "Content-Type" && h.second == "text/xml; charset=utf-8") {
      foundContentType = true;
    }
    if (h.first == "SOAPAction" && h.second == "\"http://example.org/GetPrice\"") {
      foundSoapAction = true;
    }
  }
  expectTrue(foundContentType, "SOAP 1.1 Content-Type must be text/xml; charset=utf-8");
  expectTrue(foundSoapAction, "SOAP 1.1 must send quoted SOAPAction header");

  std::string expectedPayload = "<?xml version=\"1.0\" encoding=\"utf-8\"?><soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"><soap:Body><m:GetPrice xmlns:m=\"http://example.org\"><m:Item>ESP32</m:Item></m:GetPrice></soap:Body></soap:Envelope>";
  expectEq(HttpClientStub::lastPayload, expectedPayload, "SOAP 1.1 Envelope payload");
  expectNear(price, 99.95, 0.01, "Parsed price from response");
}

void testSoap11HeaderAndEmptyAction() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "<Envelope><Body><Result>OK</Result></Body></Envelope>");

  ESP32HTTPClient client("https://example.com");
  char result[16] = {0};
  {
    client.soap("/service")
        .headerXml("<Auth><Token>12345</Token></Auth>")
        .body("<Ping/>")
        .getBody("Result", result);
  }

  bool foundSoapAction = false;
  for (const auto& h : HttpClientStub::lastHeaders) {
    if (h.first == "SOAPAction" && h.second == "\"\"") {
      foundSoapAction = true;
    }
  }
  expectTrue(foundSoapAction, "Empty SOAPAction formatted as \"\"");
  expectTrue(HttpClientStub::lastPayload.find("<soap:Header><Auth><Token>12345</Token></Auth></soap:Header>") != std::string::npos, "Envelope contains Header XML");
  expectEq(result, "OK", "Result parsed correctly");
}

void testSoap12RequestFormatting() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "<soap12:Envelope xmlns:soap12=\"http://www.w3.org/2003/05/soap-envelope\"><soap12:Body><GetPriceResponse><Price>149.50</Price></GetPriceResponse></soap12:Body></soap12:Envelope>");

  ESP32HTTPClient client("https://example.com");
  double price = 0.0;
  {
    client.soap("/ws")
        .version(SOAP_1_2)
        .action("http://example.org/GetPrice")
        .body("<m:GetPrice xmlns:m=\"http://example.org\"><m:Item>ESP32-S3</m:Item></m:GetPrice>")
        .getBody("Price", &price);
  }

  bool foundContentType = false;
  bool foundSoapAction = false;
  for (const auto& h : HttpClientStub::lastHeaders) {
    if (h.first == "Content-Type" && h.second == "application/soap+xml; charset=utf-8; action=\"http://example.org/GetPrice\"") {
      foundContentType = true;
    }
    if (h.first == "SOAPAction") {
      foundSoapAction = true;
    }
  }
  expectTrue(foundContentType, "SOAP 1.2 Content-Type includes action parameter");
  expectTrue(!foundSoapAction, "SOAP 1.2 must NOT send SOAPAction HTTP header");

  std::string expectedPayload = "<?xml version=\"1.0\" encoding=\"utf-8\"?><soap12:Envelope xmlns:soap12=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"><soap12:Body><m:GetPrice xmlns:m=\"http://example.org\"><m:Item>ESP32-S3</m:Item></m:GetPrice></soap12:Body></soap12:Envelope>";
  expectEq(HttpClientStub::lastPayload, expectedPayload, "SOAP 1.2 Envelope payload");
  expectNear(price, 149.50, 0.01, "Parsed price from SOAP 1.2 response");
}

void testSoap12HeaderAndEmptyAction() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "<soap12:Envelope xmlns:soap12=\"http://www.w3.org/2003/05/soap-envelope\"><soap12:Body><Status>Online</Status></soap12:Body></soap12:Envelope>");

  ESP32HTTPClient client("https://example.com");
  client.setSoapVersion(SOAP_1_2);
  expectEqInt(client.getSoapVersion(), SOAP_1_2, "client getSoapVersion is SOAP_1_2");

  String status;
  {
    client.soap("/v2")
        .headerXml("<Sec>key</Sec>")
        .body("<Check/>")
        .getBody("Status", &status);
  }

  bool foundContentType = false;
  for (const auto& h : HttpClientStub::lastHeaders) {
    if (h.first == "Content-Type" && h.second == "application/soap+xml; charset=utf-8") {
      foundContentType = true;
    }
  }
  expectTrue(foundContentType, "SOAP 1.2 Content-Type without action parameter when empty");
  expectTrue(HttpClientStub::lastPayload.find("<soap12:Header><Sec>key</Sec></soap12:Header>") != std::string::npos, "SOAP 1.2 Header XML present");
  expectEq(status.str(), "Online", "Status extracted");
}

void testSoapRawEnvelopeOverride() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "<r><val>42</val></r>");

  ESP32HTTPClient client("https://example.com");
  int val = 0;
  {
    client.soap("/custom")
        .rawEnvelope("<CustomEnvelope><CustomBody>123</CustomBody></CustomEnvelope>")
        .getBody("val", &val);
  }

  expectEq(HttpClientStub::lastPayload, "<CustomEnvelope><CustomBody>123</CustomBody></CustomEnvelope>", "raw envelope sent as-is");
  expectEqInt(val, 42, "val parsed");
}

void testSoapResponseParsingTypes() {
  std::string xml =
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!-- This is a sample response -->\n"
      "<soapenv:Envelope xmlns:soapenv=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:ws=\"http://ws.example.com\">\n"
      "  <soapenv:Header>\n"
      "    <ws:TransactionId>TX-987654</ws:TransactionId>\n"
      "  </soapenv:Header>\n"
      "  <soapenv:Body>\n"
      "    <ws:GetDataResponse>\n"
      "      <ws:Count>150</ws:Count>\n"
      "      <ws:Timestamp>1726900000</ws:Timestamp>\n"
      "      <ws:Ratio>3.14159</ws:Ratio>\n"
      "      <ws:Active>true</ws:Active>\n"
      "      <ws:Name>ESP32-Node</ws:Name>\n"
      "      <ws:Data><![CDATA[Raw <embedded> & unescaped data]]></ws:Data>\n"
      "      <ws:Escaped>Entities &amp; &lt;brackets&gt; &quot;quotes&quot; &apos;apos&apos; &#65;</ws:Escaped>\n"
      "      <ws:EmptyTag/>\n"
      "    </ws:GetDataResponse>\n"
      "  </soapenv:Body>\n"
      "</soapenv:Envelope>";

  StringStream stream(xml);
  BufferedStreamReader reader(&stream);

  int count = 0;
  long timestamp = 0;
  float ratio = 0.0f;
  bool active = false;
  char name[32] = {0};
  String txId;
  String cdata;
  String escaped;
  String emptyVal = "initial";

  std::vector<ResponseBinding> bindings = {
      {"Count", &count, TYPE_INT, 0},
      {"Timestamp", &timestamp, TYPE_LONG, 0},
      {"Ratio", &ratio, TYPE_FLOAT, 0},
      {"Active", &active, TYPE_BOOL, 0},
      {"Name", name, TYPE_STRING, sizeof(name)},
      {"Header.TransactionId", &txId, TYPE_ARDUINO_STRING, 0},
      {"Data", &cdata, TYPE_ARDUINO_STRING, 0},
      {"Escaped", &escaped, TYPE_ARDUINO_STRING, 0},
      {"EmptyTag", &emptyVal, TYPE_ARDUINO_STRING, 0},
  };

  SoapFault fault;
  String rawXml;
  SoapRequest::parseXmlWithBindings(reader, bindings, &fault, &rawXml);

  expectEqInt(count, 150, "Count parsed as int");
  expectEqInt(timestamp, 1726900000L, "Timestamp parsed as long");
  expectNear(ratio, 3.14159, 0.0001, "Ratio parsed as float");
  expectTrue(active, "Active parsed as bool");
  expectEq(name, "ESP32-Node", "Name parsed as char buffer");
  expectEq(txId.str(), "TX-987654", "Dotted path Header.TransactionId parsed");
  expectEq(cdata.str(), "Raw <embedded> & unescaped data", "CDATA content parsed");
  expectEq(escaped.str(), "Entities & <brackets> \"quotes\" 'apos' A", "XML entities decoded");
  expectEq(emptyVal.str(), "", "Empty self-closing tag binds empty string");
  expectTrue(!fault.matched, "No fault in successful response");
  expectTrue(rawXml.length() > 100, "Raw XML captured");
}

void testSoap11FaultHandling() {
  HttpClientStub::reset();
  std::string faultXml =
      "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
      "  <soap:Body>"
      "    <soap:Fault>"
      "      <faultcode>soap:Server</faultcode>"
      "      <faultstring>Internal error processing request</faultstring>"
      "      <faultactor>http://example.com/order-service</faultactor>"
      "      <detail>Database connection timeout</detail>"
      "    </soap:Fault>"
      "  </soap:Body>"
      "</soap:Envelope>";
  HttpClientStub::setResponse(500, faultXml);

  ESP32HTTPClient client("https://example.com");
  SoapFault fault;
  bool faultCbFired = false;
  bool errorCbFired = false;
  int errCode = 0;

  {
    client.soap("/ws")
        .soapAction("DoFail")
        .body("<Fail/>")
        .getFault(&fault)
        .onFault([&](const SoapFault& f) {
          faultCbFired = true;
          expectEq(f.faultCode.str(), "soap:Server", "Callback faultCode");
          expectEq(f.faultString.str(), "Internal error processing request", "Callback faultString");
        })
        .onError([&](int code, const char* msg) {
          errorCbFired = true;
          errCode = code;
        });
  }

  expectTrue(fault.matched, "Fault matched");
  expectEq(fault.faultCode.str(), "soap:Server", "Fault code extracted");
  expectEq(fault.faultString.str(), "Internal error processing request", "Fault string extracted");
  expectEq(fault.faultActor.str(), "http://example.com/order-service", "Fault actor extracted");
  expectEq(fault.detail.str(), "Database connection timeout", "Fault detail extracted");
  expectTrue(faultCbFired, "onFault callback fired");
  expectTrue(errorCbFired, "onError callback fired on 500");
  expectEqInt(errCode, 500, "Error code is 500");
}

void testSoap12FaultHandling() {
  HttpClientStub::reset();
  std::string faultXml =
      "<env:Envelope xmlns:env=\"http://www.w3.org/2003/05/soap-envelope\" xmlns:rpc=\"http://www.w3.org/2003/05/soap-rpc\">"
      "  <env:Body>"
      "    <env:Fault>"
      "      <env:Code>"
      "        <env:Value>env:Sender</env:Value>"
      "      </env:Code>"
      "      <env:Reason>"
      "        <env:Text xml:lang=\"en-US\">Invalid argument supplied</env:Text>"
      "      </env:Reason>"
      "      <env:Node>http://example.com/node1</env:Node>"
      "      <env:Role>http://example.com/role1</env:Role>"
      "      <env:Detail>Parameter 'id' cannot be negative</env:Detail>"
      "    </env:Fault>"
      "  </env:Body>"
      "</env:Envelope>";
  HttpClientStub::setResponse(500, faultXml);

  ESP32HTTPClient client("https://example.com");
  SoapFault fault;
  bool faultCbFired = false;

  {
    client.soap("/v2")
        .version(SOAP_1_2)
        .body("<GetItem id=\"-1\"/>")
        .getFault(&fault)
        .onFault([&](const SoapFault& f) {
          faultCbFired = true;
        });
  }

  expectTrue(fault.matched, "SOAP 1.2 Fault matched");
  expectEq(fault.faultCode.str(), "env:Sender", "SOAP 1.2 Code/Value extracted");
  expectEq(fault.faultString.str(), "Invalid argument supplied", "SOAP 1.2 Reason/Text extracted");
  expectEq(fault.faultActor.str(), "http://example.com/node1", "SOAP 1.2 Node extracted");
  expectEq(fault.detail.str(), "Parameter 'id' cannot be negative", "SOAP 1.2 Detail extracted");
  expectTrue(faultCbFired, "SOAP 1.2 onFault callback fired");
}

void testSoapHeadersAndAuth() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "<Envelope><Body><Res>1</Res></Body></Envelope>");
  HttpClientStub::setResponseHeaders({{"X-Server-Id", "srv-42"}});

  ESP32HTTPClient client("https://example.com");
  client.basic("admin", "secret123");
  client.apiKey("X-API-Key", "api-val-99");
  client.cookie("session", "xyz");

  int res = 0;
  String serverId;
  {
    client.soap("/secure")
        .body("<Req/>")
        .getBody("Res", &res)
        .getHeader("X-Server-Id", &serverId);
  }

  bool foundAuth = false;
  bool foundApiKey = false;
  bool foundCookie = false;
  for (const auto& h : HttpClientStub::lastHeaders) {
    if (h.first == "Authorization" && h.second == "Basic YWRtaW46c2VjcmV0MTIz") foundAuth = true;
    if (h.first == "X-API-Key" && h.second == "api-val-99") foundApiKey = true;
    if (h.first == "Cookie" && h.second == "session=xyz") foundCookie = true;
  }
  expectTrue(foundAuth, "Basic auth header sent with SOAP");
  expectTrue(foundApiKey, "API key header sent with SOAP");
  expectTrue(foundCookie, "Cookie sent with SOAP");
  expectEqInt(res, 1, "Result parsed");
  expectEq(serverId.str(), "srv-42", "Response header captured");
}

void testSoapPathAndQueryParams() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "<Envelope><Body><Data>10</Data></Body></Envelope>");

  ESP32HTTPClient client("https://example.com");
  int data = 0;
  {
    client.soap("/services/{serviceId}/endpoint")
        .path("serviceId", 456)
        .query("tenant", "acme")
        .query("debug", true)
        .body("<Fetch/>")
        .getBody("Data", &data);
  }

  expectEq(HttpClientStub::lastUrl, "https://example.com/services/456/endpoint?tenant=acme&debug=true", "URL with path and query parameters");
  expectEqInt(data, 10, "Data parsed");
}

void testSoapRetryAndObservability() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(-1, ""); // first attempt fails

  ESP32HTTPClient client("https://example.com");
  bool obsFired = false;
  client.onObservability([&](const ObservabilityMetrics& m) {
    obsFired = true;
    expectTrue(m.retries >= 1, "Retries recorded");
  });

  client.soap("/retry")
      .retry(1)
      .body("<Ping/>")
      .getBody("Ignored", (int*)nullptr);

  expectTrue(obsFired, "Observability callback fired on SOAP request");
  expectEqInt(client.getStatusCode(), -1, "Status code after retry failure");
}

void testSoapMoveConstructor() {
  ESP32HTTPClient client("https://example.com");
  SoapRequest req1(&client, "/move", SOAP_1_2);
  req1.soapAction("MyAction").body("<Ping/>");

  SoapRequest req2(std::move(req1));
  expectTrue(req1._executed, "req1 marked executed after move");
  expectEq(req2._soapAction.str(), "MyAction", "req2 preserved soapAction");
  expectEqInt(req2._version, SOAP_1_2, "req2 preserved version");
  req2._executed = true;
}

void testSoapClientSingleParamOnError() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(404, "<Envelope><Body><Fault><faultcode>404</faultcode><faultstring>Not Found</faultstring></Fault></Body></Envelope>");

  ESP32HTTPClient client("https://example.com");
  int capturedCode = 0;
  client.onError([&capturedCode](int code) {
    capturedCode = code;
  });

  client.soap("/notfound").body("<Req/>");

  expectEqInt(capturedCode, 404, "Client single-parameter onError callback fired for SOAP");
}
} // namespace

int main() {
  std::cout << "=== Running SOAP Unit Tests ===\n\n";

  runSuite("Soap11RequestFormatting", testSoap11RequestFormatting);
  runSuite("Soap11HeaderAndEmptyAction", testSoap11HeaderAndEmptyAction);
  runSuite("Soap12RequestFormatting", testSoap12RequestFormatting);
  runSuite("Soap12HeaderAndEmptyAction", testSoap12HeaderAndEmptyAction);
  runSuite("SoapRawEnvelopeOverride", testSoapRawEnvelopeOverride);
  runSuite("SoapResponseParsingTypes", testSoapResponseParsingTypes);
  runSuite("Soap11FaultHandling", testSoap11FaultHandling);
  runSuite("Soap12FaultHandling", testSoap12FaultHandling);
  runSuite("SoapHeadersAndAuth", testSoapHeadersAndAuth);
  runSuite("SoapPathAndQueryParams", testSoapPathAndQueryParams);
  runSuite("SoapRetryAndObservability", testSoapRetryAndObservability);
  runSuite("SoapMoveConstructor", testSoapMoveConstructor);
  runSuite("SoapClientSingleParamOnError", testSoapClientSingleParamOnError);

  std::cout << "\n=== SOAP Test Summary ===\n";
  std::cout << "Suites: " << suitesRun << " total | " << suitesPassed << " passed | " << failures << " failed\n";
  std::cout << "Checks: " << checks << " total | " << passedChecks << " passed | " << (checks - passedChecks) << " failed\n";

  return failures == 0 ? 0 : 1;
}
