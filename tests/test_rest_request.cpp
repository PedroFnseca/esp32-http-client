#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>

#include "HTTPClient.h"

#define private public
#include "ESP32HTTPClient.h"
#include "RestRequest.h"
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
  explicit StringStream(const std::string& content) : _content(content), _cursor(0) {
  }

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

void testAddParamFormatting() {
  ESP32HTTPClient client("https://jsonplaceholder.typicode.com");
  RestRequest req(&client, "/sensor", HTTP_GET_METHOD);

  req.query("count", 42).query("ratio", 1.5f).query("enabled", true).query("name", "esp32");

  expectEqInt(static_cast<long long>(req._queryParams.size()), 4, "query should store 4 params");
  expectEq(req._queryParams[0].valueBuffer, "42", "int should be formatted without quotes");
  expectTrue(!req._queryParams[0].quoteValue, "int should not be quoted");

  expectEq(req._queryParams[1].valueBuffer, "1.5", "float should use compact format");
  expectTrue(!req._queryParams[1].quoteValue, "float should not be quoted");

  expectEq(req._queryParams[2].valueBuffer, "true", "bool should serialize to true/false");
  expectTrue(!req._queryParams[2].quoteValue, "bool should not be quoted");

  expectEq(req._queryParams[3].valueBuffer, "esp32", "const char* should be copied to buffer");
  expectTrue(req._queryParams[3].quoteValue, "string should be quoted");

  req._executed = true;
}

void testExecuteBuildsUrlAndPayload() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(201, R"({"ok":true})");

  ESP32HTTPClient client("https://jsonplaceholder.typicode.com");
  {
    RestRequest req = client.post("/users");
    req.query("tenant", 7).body("name", "Pedro").body("age", 21).body("active", true);
  }

  expectEq(HttpClientStub::lastMethod, "POST", "POST should call HTTPClient::POST");
  expectEq(HttpClientStub::lastUrl, "https://jsonplaceholder.typicode.com/users?tenant=7", "url should include base URL and query");
  expectEq(HttpClientStub::lastPayload, "{\"name\":\"Pedro\",\"age\":21,\"active\":true}", "payload should be JSON with proper quoting");
  expectEqInt(client.getStatusCode(), 201, "client should store last response code");
}

void testMethodAliases() {
  ESP32HTTPClient client("https://jsonplaceholder.typicode.com");

  RestRequest putReq = client.put("/resource");
  expectEqInt(putReq._method, HTTP_PUT_METHOD, "put() should map to HTTP_PUT_METHOD");
  putReq._executed = true;

  RestRequest updateReq = client.update("/resource");
  expectEqInt(updateReq._method, HTTP_PUT_METHOD, "update() should map to HTTP_PUT_METHOD");
  updateReq._executed = true;

  RestRequest patchReq = client.patch("/resource");
  expectEqInt(patchReq._method, HTTP_PATCH_METHOD, "patch() should map to HTTP_PATCH_METHOD");
  patchReq._executed = true;

  RestRequest delReq = client.del("/resource");
  expectEqInt(delReq._method, HTTP_DELETE_METHOD, "del() should map to HTTP_DELETE_METHOD");
  delReq._executed = true;
}

void testParseResponseBindsTypes() {
  ESP32HTTPClient client("https://jsonplaceholder.typicode.com");
  RestRequest req(&client, "/x", HTTP_GET_METHOD);

  int count = 0;
  float temperature = 0.0f;
  double voltage = 0.0;
  bool active = false;
  long timestamp = 0;
  char label[16] = {0};

  req.getBody("count", &count)
      .getBody("temperature", &temperature)
      .getBody("voltage", &voltage)
      .getBody("active", &active)
      .getBody("timestamp", &timestamp)
      .getBody("label", label, sizeof(label));

  StringStream stream(
      R"({"count":7,"temperature":24.5,"voltage":3.3001,"active":true,"timestamp":1710000010,"label":"sensor\"A","ignore":{"k":1}})");
  BufferedStreamReader reader(&stream);
  req.parseResponse(reader);

  expectEqInt(count, 7, "count should be parsed as int");
  expectNear(temperature, 24.5, 0.001, "temperature should be parsed as float");
  expectNear(voltage, 3.3001, 0.00001, "voltage should be parsed as double");
  expectTrue(active, "active should be parsed as bool");
  expectEqInt(timestamp, 1710000010L, "timestamp should be parsed as long");
  expectEq(label, "sensor\"A", "escaped string should be unescaped and copied");

  req._executed = true;
}

void testParseNestedJSON() {
  ESP32HTTPClient client("https://jsonplaceholder.typicode.com");
  RestRequest req(&client, "/users/1", HTTP_GET_METHOD);

  char street[64] = {0};
  float lat = 0.0f;

  req.getBody("address.street", street, sizeof(street))
      .getBody("address.geo.lat", &lat);

  StringStream stream(
      R"({
        "id": 1,
        "name": "Leanne Graham",
        "username": "Bret",
        "email": "Sincere@april.biz",
        "address": {
          "street": "Kulas Light",
          "suite": "Apt. 556",
          "city": "Gwenborough",
          "zipcode": "92998-3874",
          "geo": {
            "lat": "-37.3159",
            "lng": "81.1496"
          }
        },
        "phone": "1-770-736-8031 x56442",
        "website": "hildegard.org",
        "company": {
          "name": "Romaguera-Crona",
          "catchPhrase": "Multi-layered client-server neural-net",
          "bs": "harness real-time e-markets"
        }
      })");
  
  BufferedStreamReader reader(&stream);
  req.parseResponse(reader);

  expectTrue(strlen(street) > 0, "street should be populated");
  expectTrue(lat != 0.0f, "lat should be populated and parsed as float");

  req._executed = true;
}

void testParseNestedJSONMissingFields() {
  ESP32HTTPClient client("https://jsonplaceholder.typicode.com");
  RestRequest req(&client, "/users/1", HTTP_GET_METHOD);

  char invalidStreet[64] = {0};
  float invalidLat = 0.0f;

  // Requesting fields that don't exist in the JSON
  req.getBody("address.unknown_street", invalidStreet, sizeof(invalidStreet))
      .getBody("address.not_found.deep", &invalidLat);

  StringStream stream(
      R"({
        "address": {
          "street": "Kulas Light",
          "geo": {
            "lat": "-37.3159"
          }
        }
      })");
  
  BufferedStreamReader reader(&stream);
  req.parseResponse(reader);

  expectTrue(strlen(invalidStreet) == 0, "missing string field should remain empty");
  expectTrue(invalidLat == 0.0f, "missing float field should remain unchanged (0.0f)");

  req._executed = true;
}

void testParseRawArrayJSON() {
  ESP32HTTPClient client("https://jsonplaceholder.typicode.com");
  RestRequest req(&client, "/users", HTTP_GET_METHOD);

  String entireArray = "";
  String specificObject = "";
  String notFound = "original";

  req.getBody("", &entireArray) // match the root array
     .getBody("5.unknown", &notFound);

  StringStream stream1(
      R"([
        {
          "name": "Leanne Graham",
          "address": { "city": "Gwenborough" }
        },
        {
          "name": "Ervin Howell",
          "address": {
            "city": "Wisokyburgh",
            "geo": { "lat": "-43.9509" }
          }
        }
      ])");
  
  BufferedStreamReader reader1(&stream1);
  req.parseResponse(reader1);

  // Asserting successful cases for root array
  expectTrue(entireArray.str().length() > 50, "entireArray should contain the raw JSON string of the root array");
  expectTrue(entireArray.str().substr(0, 1) == "[", "entireArray should start with [");
  
  // Asserting failure case
  expectEq(notFound.str(), "original", "notFound should remain unchanged if path does not exist");

  // Now test nested object raw extraction independently (as root extraction consumes the whole stream)
  RestRequest req2(&client, "/users", HTTP_GET_METHOD);
  req2.getBody("1.address", &specificObject);

  StringStream stream2(
      R"([
        {
          "name": "Leanne Graham",
          "address": { "city": "Gwenborough" }
        },
        {
          "name": "Ervin Howell",
          "address": {
            "city": "Wisokyburgh",
            "geo": { "lat": "-43.9509" }
          }
        }
      ])");
  
  BufferedStreamReader reader2(&stream2);
  req2.parseResponse(reader2);

  expectTrue(specificObject.str().length() > 20, "specificObject should contain the raw JSON string of the nested object");
  expectTrue(specificObject.str().substr(0, 1) == "{", "specificObject should start with {");

  req._executed = true;
}
void testUnixTimestampFetch() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, R"({"unix_timestamp":1781301337})");

  ESP32HTTPClient client("https://timeapi.io");
  
  long ts = 0;
  {
    client.get("/api/v1/time/current/unix")
          .getBody("unix_timestamp", &ts);
  }

  expectEq(HttpClientStub::lastMethod, "GET", "should use GET method");
  expectEq(HttpClientStub::lastUrl, "https://timeapi.io/api/v1/time/current/unix", "url should be correct");
  expectEqInt(ts, 1781301337L, "unix_timestamp should be successfully parsed");
}
void testClientConfiguration() {
  ESP32HTTPClient client("https://example.com");
  client.setContentType("text/plain");
  client.setHeader("X-Custom", "value1");
  
  HttpClientStub::reset();
  client.post("/upload").body("data", "hello").getBody("ignored", (int*)nullptr);
  
  expectEq(HttpClientStub::lastUrl, "https://example.com/upload", "url matches");
  expectEq(HttpClientStub::lastMethod, "POST", "method matches");
  
  client.end(); // covers the end() method
}

void testAllHttpMethods() {
  HttpClientStub::reset();
  ESP32HTTPClient client("http://test", 8080);
  
  client.put("/").getBody("i", (int*)nullptr);
  expectEq(HttpClientStub::lastMethod, "PUT", "put uses PUT");
  
  client.patch("/").getBody("i", (int*)nullptr);
  expectEq(HttpClientStub::lastMethod, "PATCH", "patch uses PATCH");
  
  client.del("/").getBody("i", (int*)nullptr);
  expectEq(HttpClientStub::lastMethod, "DELETE", "del uses DELETE");
}

void testAutoRetry() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(-1, ""); // first attempt fails
  
  ESP32HTTPClient client("http://retry");
  client.get("/").getBody("i", (int*)nullptr);
  
  expectEqInt(client.getStatusCode(), -1, "status code propagates from retry loop");
}

void testChunkedTransferEncoding() {
  ESP32HTTPClient client("https://example.com");
  RestRequest req(&client, "/chunked", HTTP_GET_METHOD);
  char buf[32] = {0};
  req.getBody("msg", buf, sizeof(buf));
  
  StringStream stream("e\r\n{\"msg\":\"hello \r\n7\r\nworld\"}\r\n0\r\n\r\n");
  BufferedStreamReader reader(&stream, true);
  req.parseResponse(reader);
  
  expectEq(buf, "hello world", "chunked data should be correctly reassembled inside parseResponse");
  req._executed = true;
}

void testLargePayloadRefill() {
  ESP32HTTPClient client("https://example.com");
  RestRequest req(&client, "/large", HTTP_GET_METHOD);
  char buf[32] = {0};
  req.getBody("end", buf, sizeof(buf));
  
  std::string largeData = "{\"pad\":\"";
  largeData.append(600, 'A');
  largeData += "\",\"end\":\"found\"}";
  
  StringStream stream(largeData);
  BufferedStreamReader reader(&stream, false);
  req.parseResponse(reader);
  
  expectEq(buf, "found", "should successfully read beyond 512 bytes buffer size in parseResponse");
  req._executed = true;
}

void testMoreParseEdgeCases() {
  ESP32HTTPClient client("https://example.com");
  RestRequest req(&client, "/edge", HTTP_GET_METHOD);

  bool active = true;
  String strVal;
  long lval = 0;
  char charbuf[10] = {0};

  req.getBody("active", &active)
     .getBody("strVal", &strVal)
     .getBody("lval", &lval)
     .getBody("charbuf", charbuf, sizeof(charbuf));

  StringStream stream(R"({"active":false,"strVal":"hello","lval":-12345,"charbuf":"1234567890123","ignore_arr":[1,2,3],"ignore_obj":{"a":1}})");
  BufferedStreamReader reader(&stream);
  req.parseResponse(reader);

  expectTrue(!active, "boolean false should be parsed");
  expectEq(strVal.str(), "hello", "arduino string should be parsed");
  expectEqInt(lval, -12345, "negative long should be parsed");
  expectEq(charbuf, "123456789", "char buffer should be truncated to max length");
  req._executed = true;
}

void testParseEmptyObjectOrArray() {
  ESP32HTTPClient client("https://example.com");
  RestRequest req(&client, "/empty", HTTP_GET_METHOD);

  int x = 10;
  req.getBody("x", &x);

  StringStream stream(R"({})");
  BufferedStreamReader reader1(&stream);
  req.parseResponse(reader1);
  expectEqInt(x, 10, "empty object shouldn't crash");

  StringStream stream2(R"([])");
  BufferedStreamReader reader2(&stream2);
  req.parseResponse(reader2);
  expectEqInt(x, 10, "empty array shouldn't crash");
  
  req._executed = true;
}

void testAddParamAdvanced() {
  ESP32HTTPClient client("https://example.com");
  RestRequest req(&client, "/test", HTTP_GET_METHOD);
  
  req.query("d", 3.14159265);
  req.body("f", 2.5f);
  
  expectEqInt((long long)req._queryParams.size(), 1, "query params size");
  expectEqInt((long long)req._bodyParams.size(), 1, "body params size");
  
  expectEq(req._queryParams[0].valueBuffer, "3.14159265", "double formatting");
  expectEq(req._bodyParams[0].valueBuffer, "2.5", "float formatting");
  req._executed = true;
}

void testExecuteCustomPortWithPath() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{}");
  ESP32HTTPClient client("http://test.com/api", 8080);
  client.get("/data").getBody("x", (int*)nullptr);
  
  expectEq(HttpClientStub::lastUrl, "http://test.com:8080/api/data", "port injection with path");
}

void testParseEscapedStringsSimple() {
  ESP32HTTPClient client("https://example.com");
  RestRequest req(&client, "/edge", HTTP_GET_METHOD);
  char buf[32] = {0};
  req.getBody("escaped", buf, sizeof(buf));
  StringStream stream(R"({"escaped":"a\"b\\c"})");
  BufferedStreamReader reader(&stream);
  req.parseResponse(reader);
  expectEq(buf, "a\"b\\c", "escaped char buffer");
  req._executed = true;
}

void testSkipValueAdvanced() {
  ESP32HTTPClient client("https://example.com");
  RestRequest req(&client, "/skip", HTTP_GET_METHOD);

  int x = 0;
  req.getBody("x", &x);

  StringStream stream(R"({"skip_obj":{"a":"b\"c", "d":[1,2,{"e":"f"}]},"skip_arr":[1,"[","]",{"x":"y"}],"x":42})");
  BufferedStreamReader reader(&stream);
  req.parseResponse(reader);

  expectEqInt(x, 42, "x should be parsed after skipping complex structures");
  req._executed = true;
}

void testMoveConstructor() {
  ESP32HTTPClient client("https://example.com");
  RestRequest req1(&client, "/move", HTTP_GET_METHOD);
  req1.query("a", 1);
  
  RestRequest req2(std::move(req1));
  
  expectTrue(req1._executed, "req1 should be marked executed after move");
  expectEqInt((long long)req2._queryParams.size(), 1, "req2 should have params");
  req2._executed = true;
}

void testParsePrimitiveTypes() {
  ESP32HTTPClient client("https://example.com");
  RestRequest req(&client, "/prim", HTTP_GET_METHOD);
  
  double dVal = 0.0;
  String bStr;
  
  req.getBody("dVal", &dVal)
     .getBody("bStr", &bStr);
     
  StringStream stream(R"({"dVal":3.1415926535,"bStr":true})");
  BufferedStreamReader reader(&stream);
  req.parseResponse(reader);
  
  expectNear(dVal, 3.1415926535, 0.000001, "double precision parsing");
  expectEq(bStr.str(), "true", "boolean to String parsing");
  req._executed = true;
}

void testAuthBearer() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://example.com");
  client.bearer("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9");

  expectEqInt(static_cast<long long>(client._headers.size()), 1, "headers size after bearer");
  expectEq(client._headers[0].name, "Authorization", "header name should be Authorization");
  expectEq(client._headers[0].value, "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9", "bearer token format");

  int id = 0;
  client.get("/secure").getBody("id", &id);

  bool found = false;
  for (const auto& h : HttpClientStub::lastHeaders) {
    if (h.first == "Authorization" && h.second == "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9") {
      found = true;
      break;
    }
  }
  expectTrue(found, "Authorization header should be sent with request");

  client.bearer("new-token-456");
  expectEqInt(static_cast<long long>(client._headers.size()), 1, "bearer overwrite should not duplicate header");
  expectEq(client._headers[0].value, "Bearer new-token-456", "bearer header updated");
}

void testAuthBasic() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://example.com");
  client.basic("admin", "123456");

  expectEqInt(static_cast<long long>(client._headers.size()), 1, "headers size after basic");
  expectEq(client._headers[0].name, "Authorization", "header name should be Authorization");
  expectEq(client._headers[0].value, "Basic YWRtaW46MTIzNDU2", "basic auth encoding for admin:123456");

  int id = 0;
  client.get("/admin").getBody("id", &id);

  bool found = false;
  for (const auto& h : HttpClientStub::lastHeaders) {
    if (h.first == "Authorization" && h.second == "Basic YWRtaW46MTIzNDU2") {
      found = true;
      break;
    }
  }
  expectTrue(found, "Basic Authorization header should be sent with request");

  client.basic("user", "pass");
  expectEq(client._headers[0].value, "Basic dXNlcjpwYXNz", "basic auth encoding for user:pass");

  client.basic("Aladdin", "open sesame");
  expectEq(client._headers[0].value, "Basic QWxhZGRpbjpvcGVuIHNlc2FtZQ==", "basic auth encoding for Aladdin:open sesame");
}

void testAuthApiKey() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://example.com");
  client.apiKey("x-api-key", "secret-key-123");

  expectEqInt(static_cast<long long>(client._headers.size()), 1, "headers size after apiKey");
  expectEq(client._headers[0].name, "x-api-key", "api key header name");
  expectEq(client._headers[0].value, "secret-key-123", "api key header value");

  int val = 0;
  client.get("/api/v1").getBody("v", &val);

  bool found = false;
  for (const auto& h : HttpClientStub::lastHeaders) {
    if (h.first == "x-api-key" && h.second == "secret-key-123") {
      found = true;
      break;
    }
  }
  expectTrue(found, "API key header should be sent with request");

  client.apiKey("x-api-key", "new-secret-456");
  expectEqInt(static_cast<long long>(client._headers.size()), 1, "apiKey overwrite should not duplicate header");
  expectEq(client._headers[0].value, "new-secret-456", "apiKey header updated");
}

void testAuthCookie() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://example.com");
  client.cookie("session_id", "abc123")
        .cookie("device_id", "esp32-01");

  expectEqInt(static_cast<long long>(client._headers.size()), 1, "headers size after multiple cookies should be 1 (appended)");
  expectEq(client._headers[0].name, "Cookie", "cookie header name");
  expectEq(client._headers[0].value, "session_id=abc123; device_id=esp32-01", "cookie header value correctly formatted");

  int val = 0;
  client.get("/api/v1").getBody("v", &val);

  bool found = false;
  for (const auto& h : HttpClientStub::lastHeaders) {
    if (h.first == "Cookie" && h.second == "session_id=abc123; device_id=esp32-01") {
      found = true;
      break;
    }
  }
  expectTrue(found, "Cookie header should be sent with request");
}

void testObservability() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://example.com");

  bool callbackFired = false;
  client.onObservability([&callbackFired](const ObservabilityMetrics& metrics) {
    callbackFired = true;
    expectTrue(metrics.totalTimeMs >= 0, "Total time should be tracked");
    expectTrue(metrics.ttfbMs >= 0, "TTFB should be tracked");
    expectTrue(metrics.txBytes > 0, "TX bytes should be > 0");
    expectEqInt(static_cast<long long>(metrics.rxBytes), 11, "RX bytes should match response size");
    expectEqInt(metrics.retries, 0, "Retries should be 0");
    expectTrue(metrics.freeHeapBefore > 0, "Free heap before should be > 0");
    expectTrue(metrics.freeHeapAfter > 0, "Free heap after should be > 0");
  });

  HttpClientStub::setResponse(200, R"({"ok":true})");
  client.get("/api").execute();

  expectTrue(callbackFired, "Observability callback should have been fired");
}

void testAuthEdgeCases() {
  ESP32HTTPClient client("https://example.com");
  client.bearer(nullptr);
  expectEq(client._headers[0].value, "Bearer ", "bearer with null token");

  client.basic(nullptr, nullptr);
  expectEq(client._headers[0].value, "Basic Og==", "basic with null creds");

  client.apiKey("X-Custom-Key", nullptr);
  expectEq(client._headers[1].value, "", "apiKey with null key");
}

void testQueryParameters() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://example.com");

  int id = 0;
  client.get("/users")
      .query("page", 2)
      .query("limit", 20)
      .query("search", "pedro")
      .getBody("id", &id);

  expectEq(HttpClientStub::lastUrl, "https://example.com/users?page=2&limit=20&search=pedro", "query params built in url");

  HttpClientStub::reset();
  client.get("/sensors")
      .query("active", true)
      .query("threshold", 4.5f)
      .getBody("id", &id);

  expectEq(HttpClientStub::lastUrl, "https://example.com/sensors?active=true&threshold=4.5", "typed query params built in url");
}

void testPathParameters() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://example.com");

  int id = 0;
  client.get("/users/{id}")
      .path("id", 15)
      .getBody("id", &id);

  expectEq(HttpClientStub::lastUrl, "https://example.com/users/15", "path parameter {id} replaced with 15");

  HttpClientStub::reset();
  client.get("/users/{id}")
      .path("{id}", 15)
      .getBody("id", &id);

  expectEq(HttpClientStub::lastUrl, "https://example.com/users/15", "path parameter with explicit {id} format");

  HttpClientStub::reset();
  client.get("/orgs/{org}/users/{userId}/posts/{postId}")
      .path("org", "google")
      .path("userId", 42)
      .path("postId", 101)
      .getBody("id", &id);

  expectEq(HttpClientStub::lastUrl, "https://example.com/orgs/google/users/42/posts/101", "multiple path parameters");

  HttpClientStub::reset();
  client.get("/devices/{uuid}/telemetry")
      .path("uuid", "123e4567-e89b-12d3-a456-426614174000")
      .getBody("id", &id);

  expectEq(HttpClientStub::lastUrl, "https://example.com/devices/123e4567-e89b-12d3-a456-426614174000/telemetry", "uuid path parameter");
}

void testPathAndQueryCombined() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://example.com");

  int id = 0;
  client.get("/users/{id}/orders")
      .path("id", 15)
      .query("page", 2)
      .query("limit", 20)
      .query("search", "pedro")
      .getBody("id", &id);

  expectEq(HttpClientStub::lastUrl, "https://example.com/users/15/orders?page=2&limit=20&search=pedro", "combined path and query params");
}

void testPathEdgeCases() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://example.com");

  int id = 0;
  client.get("/static/path")
      .path("none", 123)
      .getBody("id", &id);

  expectEq(HttpClientStub::lastUrl, "https://example.com/static/path", "path without placeholders unaffected");

  HttpClientStub::reset();
  client.get(nullptr)
      .path("id", 10)
      .getBody("id", &id);

  expectEq(HttpClientStub::lastUrl, "https://example.com", "null path handled safely");

  HttpClientStub::reset();
  {
    RestRequest req1(&client, "/users/{id}", HTTP_GET_METHOD);
    req1.path("id", 99);
    RestRequest req2(std::move(req1));
    req2.getBody("id", &id);
  }
  expectEq(HttpClientStub::lastUrl, "https://example.com/users/99", "move constructor preserves path params");
}

void testRuntimeUrlChange() {
  ESP32HTTPClient client("https://api.initial.com", 80);
  expectEq(client.getBaseUrl(), "https://api.initial.com", "initial baseUrl");
  expectEqInt(client.getPort(), 80, "initial port");

  HttpClientStub::reset();
  int id = 0;
  client.get("/users").getBody("id", &id);
  expectEq(HttpClientStub::lastUrl, "https://api.initial.com:80/users", "request to initial url");

  client.setBaseUrl("https://api.newhost.org", 443);
  expectEq(client.getBaseUrl(), "https://api.newhost.org", "updated baseUrl");
  expectEqInt(client.getPort(), 443, "updated port");

  HttpClientStub::reset();
  client.get("/v1/data").getBody("id", &id);
  expectEq(HttpClientStub::lastUrl, "https://api.newhost.org:443/v1/data", "request to updated url and port");

  client.setUrl("http://192.168.1.50");
  client.setPort(8080);
  HttpClientStub::reset();
  client.get("/sensors").getBody("id", &id);
  expectEq(HttpClientStub::lastUrl, "http://192.168.1.50:8080/sensors", "request after setUrl and setPort");
}

void testTimeout() {
  ESP32HTTPClient client("https://example.com");
  expectEqInt(client.getTimeout(), 60000, "default timeout is 60000ms (1 minute)");

  client.setTimeout(8000);
  expectEqInt(client.getTimeout(), 8000, "custom client timeout");

  HttpClientStub::reset();
  int id = 0;
  client.get("/data").getBody("id", &id);
  expectEqInt(HttpClientStub::lastTimeout, 8000, "client timeout applied to request");

  HttpClientStub::reset();
  client.get("/quick").timeout(1500).getBody("id", &id);
  expectEqInt(HttpClientStub::lastTimeout, 1500, "per-request timeout overrides client timeout");
}

void testMaxRetry() {
  ESP32HTTPClient client("https://example.com");
  expectEqInt(client.getMaxRetry(), 1, "default maxRetry is 1");

  client.setMaxRetry(3);
  expectEqInt(client.getMaxRetry(), 3, "custom maxRetry");

  HttpClientStub::reset();
  HttpClientStub::queueResponse(-1, "");
  HttpClientStub::queueResponse(-1, "");
  HttpClientStub::queueResponse(200, "{\"success\":true}");

  bool success = false;
  client.get("/retry-ok").getBody("success", &success);
  expectTrue(success, "succeeded after retrying");
  expectEqInt(HttpClientStub::requestCount, 3, "made 3 attempts");
  expectEqInt(client.getStatusCode(), 200, "status code 200 after recovery");

  HttpClientStub::reset();
  HttpClientStub::queueResponse(-1, "");
  HttpClientStub::queueResponse(-1, "");
  HttpClientStub::queueResponse(200, "{\"success\":true}");

  success = false;
  client.get("/retry-fail").retry(0).getBody("success", &success);
  expectTrue(!success, "failed when retry is 0");
  expectEqInt(HttpClientStub::requestCount, 1, "only 1 attempt when retry is 0");
  expectEqInt(client.getStatusCode(), -1, "status code is -1");
}

void testErrorHandlingAndMessages() {
  ESP32HTTPClient client("https://example.com");

  expectEq(ESP32HTTPClient::errorToString(-1).c_str(), "Connection Refused", "errorToString -1");
  expectEq(ESP32HTTPClient::errorToString(-2).c_str(), "Send Header Failed", "errorToString -2");
  expectEq(ESP32HTTPClient::errorToString(-11).c_str(), "Read Timeout", "errorToString -11");
  expectEq(ESP32HTTPClient::errorToString(200).c_str(), "OK", "errorToString 200");
  expectEq(ESP32HTTPClient::errorToString(201).c_str(), "Created", "errorToString 201");
  expectEq(ESP32HTTPClient::errorToString(400).c_str(), "Bad Request", "errorToString 400");
  expectEq(ESP32HTTPClient::errorToString(401).c_str(), "Unauthorized", "errorToString 401");
  expectEq(ESP32HTTPClient::errorToString(404).c_str(), "Not Found", "errorToString 404");
  expectEq(ESP32HTTPClient::errorToString(500).c_str(), "Internal Server Error", "errorToString 500");

  HttpClientStub::reset();
  HttpClientStub::setResponse(404, "{\"error\":\"not found\"}");
  int id = 0;
  client.get("/missing").getBody("id", &id);

  expectEqInt(client.getStatusCode(), 404, "statusCode is 404");
  expectTrue(client.hasError(), "hasError() is true on 404");
  expectTrue(!client.isSuccess(), "isSuccess() is false on 404");
  expectEq(client.getErrorMessage().c_str(), "Not Found", "getErrorMessage() on 404");

  HttpClientStub::reset();
  HttpClientStub::setResponse(-1, "");
  client.get("/offline").retry(0).getBody("id", &id);

  expectEqInt(client.getStatusCode(), -1, "statusCode is -1");
  expectTrue(client.hasError(), "hasError() is true on -1");
  expectTrue(!client.isSuccess(), "isSuccess() is false on -1");
  expectEq(client.getErrorMessage().c_str(), "Connection Refused", "getErrorMessage() on -1");

  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"id\":10}");
  client.get("/ok").getBody("id", &id);

  expectEqInt(client.getStatusCode(), 200, "statusCode is 200");
  expectTrue(!client.hasError(), "hasError() is false on 200");
  expectTrue(client.isSuccess(), "isSuccess() is true on 200");
  expectEq(client.getErrorMessage().c_str(), "OK", "getErrorMessage() on 200");
}

void testCallbacks() {
  ESP32HTTPClient client("https://example.com");

  int successCode = 0;
  int errCode = 0;
  int respCode = 0;
  std::string errMsg;

  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"id\":123}");
  int id = 0;

  client.get("/success")
      .onSuccess([&](int code) { successCode = code; })
      .onError([&](int code, const char* msg) { errCode = code; errMsg = msg; })
      .onResponse([&](int code) { respCode = code; })
      .getBody("id", &id);

  expectEqInt(successCode, 200, "onSuccess called on 200");
  expectEqInt(respCode, 200, "onResponse called on 200");
  expectEqInt(errCode, 0, "onError not called on 200");
  expectEqInt(id, 123, "body parsed on 200");

  successCode = 0;
  errCode = 0;
  respCode = 0;
  errMsg.clear();

  HttpClientStub::reset();
  HttpClientStub::setResponse(500, "{\"error\":\"fail\"}");

  client.get("/error")
      .onSuccess([&](int code) { successCode = code; })
      .onError([&](int code, const char* msg) { errCode = code; errMsg = msg; })
      .onResponse([&](int code) { respCode = code; })
      .getBody("id", &id);

  expectEqInt(successCode, 0, "onSuccess not called on 500");
  expectEqInt(respCode, 500, "onResponse called on 500");
  expectEqInt(errCode, 500, "onError called on 500");
  expectEq(errMsg, "Internal Server Error", "onError received correct message on 500");

  int clientSuccess = 0;
  int clientError = 0;
  int clientResponse = 0;
  client.onSuccess([&](int code) { clientSuccess = code; });
  client.onError([&](int code, const char*) { clientError = code; });
  client.onResponse([&](int code) { clientResponse = code; });

  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"id\":456}");
  client.get("/client-level").getBody("id", &id);

  expectEqInt(clientSuccess, 200, "client onSuccess triggered");
  expectEqInt(clientResponse, 200, "client onResponse triggered");
  expectEqInt(clientError, 0, "client onError not triggered on 200");
}

struct UserTestStruct {
  int id = 0;
  char name[32] = {0};
  float score = 0.0f;
  bool active = false;
  String email = "";

  REST_JSON_MAP(
    REST_FIELD(id),
    REST_FIELD(name),
    REST_FIELD(score),
    REST_FIELD(active),
    REST_FIELD(email)
  )
};

struct CustomKeyStruct {
  int userId = 0;
  char fullName[32] = {0};

  REST_JSON_MAP(
    REST_FIELD_NAMED("user_id", userId),
    REST_FIELD_NAMED("full_name", fullName)
  )
};

struct ExternalUserStruct {
  int id = 0;
  String role = "";
};

REST_JSON_MAP_EXT(ExternalUserStruct,
  REST_FIELD_EXT(id),
  REST_FIELD_EXT(role)
)

void testStructSerializationToJson() {
  UserTestStruct user;
  user.id = 15;
  strncpy(user.name, "Pedro", sizeof(user.name));
  user.score = 9.5f;
  user.active = true;
  user.email = "pedro@test.com";

  String json = ESP32HTTPClient::toJson(user);
  expectEq(json.c_str(), "{\"id\":15,\"name\":\"Pedro\",\"score\":9.5,\"active\":true,\"email\":\"pedro@test.com\"}", "toJson produces correct json");

  String json2 = RestJson::toJson(user);
  expectEq(json2.c_str(), json.c_str(), "RestJson::toJson matches ESP32HTTPClient::toJson");
}

void testStructDeserializationFromJson() {
  const char* json = "{\"id\":42,\"name\":\"Ana\",\"score\":8.75,\"active\":true,\"email\":\"ana@domain.com\"}";
  UserTestStruct user;
  bool ok = ESP32HTTPClient::fromJson(json, &user);

  expectTrue(ok, "fromJson returned true");
  expectEqInt(user.id, 42, "deserialized id");
  expectEq(user.name, "Ana", "deserialized name");
  expectTrue(std::abs(user.score - 8.75f) < 0.001f, "deserialized score");
  expectTrue(user.active, "deserialized active");
  expectEq(user.email.c_str(), "ana@domain.com", "deserialized email");
}

void testStructMissingFieldsAndDefaults() {
  const char* json = "{\"id\":100,\"extra_unmapped\":\"ignored\",\"tags\":[1,2,3]}";
  UserTestStruct user;
  user.id = 0;
  strncpy(user.name, "DefaultName", sizeof(user.name));
  user.score = 5.0f;
  user.active = true;
  user.email = "default@email.com";

  bool ok = ESP32HTTPClient::fromJson(json, &user);
  expectTrue(ok, "fromJson parsed partially matching json");
  expectEqInt(user.id, 100, "updated id");
  expectEq(user.name, "DefaultName", "missing name field preserved as default");
  expectTrue(std::abs(user.score - 5.0f) < 0.001f, "missing score field preserved");
  expectTrue(user.active, "missing active field preserved");
  expectEq(user.email.c_str(), "default@email.com", "missing email field preserved");
}

void testStructNullFields() {
  const char* json = "{\"id\":null,\"name\":null,\"score\":null,\"active\":null,\"email\":null}";
  UserTestStruct user;
  user.id = 99;
  strncpy(user.name, "Old", sizeof(user.name));
  user.score = 7.0f;
  user.active = true;
  user.email = "old@test.com";

  bool ok = ESP32HTTPClient::fromJson(json, &user);
  expectTrue(ok, "fromJson parsed null values");
  expectEqInt(user.id, 0, "null id set to 0");
  expectEq(user.name, "", "null char array set to empty string");
  expectTrue(std::abs(user.score - 0.0f) < 0.001f, "null score set to 0");
  expectTrue(!user.active, "null active set to false");
  expectEq(user.email.c_str(), "", "null email String set to empty");
}

void testStructRequestBody() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://example.com");

  UserTestStruct user;
  user.id = 7;
  strncpy(user.name, "Lucas", sizeof(user.name));
  user.score = 10.0f;
  user.active = false;
  user.email = "lucas@test.com";

  int resId = 0;
  HttpClientStub::setResponse(201, "{\"id\":7}");
  client.post("/users").body(user).getBody("id", &resId);

  expectEq(HttpClientStub::lastUrl, "https://example.com/users", "post url");
  expectEq(HttpClientStub::lastPayload, "{\"id\":7,\"name\":\"Lucas\",\"score\":10,\"active\":false,\"email\":\"lucas@test.com\"}", "struct serialized as post payload");
  expectEqInt(resId, 7, "response bound successfully");
}

void testStructResponseBodyGetBody() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://example.com");

  HttpClientStub::setResponse(200, "{\"id\":88,\"name\":\"Julia\",\"score\":9.9,\"active\":true,\"email\":\"j@test.com\"}");
  UserTestStruct user;
  client.get("/users/88").getBody(&user);

  expectEqInt(user.id, 88, "getBody(&struct) populated id");
  expectEq(user.name, "Julia", "getBody(&struct) populated name");
  expectTrue(user.active, "getBody(&struct) populated active");
  expectEq(user.email.c_str(), "j@test.com", "getBody(&struct) populated email");

  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"status\":\"success\",\"data\":{\"user\":{\"id\":99,\"name\":\"Carlos\",\"score\":8.0,\"active\":false,\"email\":\"c@test.com\"}}}");
  UserTestStruct nestedUser;
  client.get("/profile").getBody("data.user", &nestedUser);

  expectEqInt(nestedUser.id, 99, "nested getBody(\"data.user\", &struct) id");
  expectEq(nestedUser.name, "Carlos", "nested getBody(\"data.user\", &struct) name");
  expectTrue(!nestedUser.active, "nested getBody(\"data.user\", &struct) active");
  expectEq(nestedUser.email.c_str(), "c@test.com", "nested getBody(\"data.user\", &struct) email");
}

void testStructCustomAndExternalMapping() {
  CustomKeyStruct custom;
  custom.userId = 55;
  strncpy(custom.fullName, "Custom Name", sizeof(custom.fullName));

  String json = ESP32HTTPClient::toJson(custom);
  expectEq(json.c_str(), "{\"user_id\":55,\"full_name\":\"Custom Name\"}", "custom named field serialization");

  CustomKeyStruct customIn;
  ESP32HTTPClient::fromJson("{\"user_id\":77,\"full_name\":\"Another Name\"}", &customIn);
  expectEqInt(customIn.userId, 77, "custom named field deserialization userId");
  expectEq(customIn.fullName, "Another Name", "custom named field deserialization fullName");

  ExternalUserStruct ext;
  ext.id = 12;
  ext.role = "admin";
  String extJson = ESP32HTTPClient::toJson(ext);
  expectEq(extJson.c_str(), "{\"id\":12,\"role\":\"admin\"}", "external struct mapping serialization");

  ExternalUserStruct extIn;
  ESP32HTTPClient::fromJson("{\"id\":34,\"role\":\"guest\"}", &extIn);
  expectEqInt(extIn.id, 34, "external struct mapping deserialization id");
  expectEq(extIn.role.c_str(), "guest", "external struct mapping deserialization role");
}

void testGetHeaderString() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://api.example.com");

  HttpClientStub::setResponse(200, "{\"status\":\"ok\"}");
  HttpClientStub::setResponseHeaders({
      {"token", "\"33a64df551425fcc55e4d42a148795d9f25f89f4\""},
      {"Content-Type", "application/json; charset=utf-8"},
      {"X-Server-Id", "srv-node-01"}
  });

  String token;
  String contentType;
  String serverId;

  client.get("/resource")
      .getHeader("token", &token)
      .getHeader("Content-Type", &contentType)
      .getHeader("X-Server-Id", &serverId);

  expectEq(token.c_str(), "\"33a64df551425fcc55e4d42a148795d9f25f89f4\"", "getHeader token String");
  expectEq(contentType.c_str(), "application/json; charset=utf-8", "getHeader Content-Type String");
  expectEq(serverId.c_str(), "srv-node-01", "getHeader custom header String");
}

void testGetHeaderCharArray() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://api.example.com");

  HttpClientStub::setResponse(200, "{}");
  HttpClientStub::setResponseHeaders({
      {"token", "W/\"123456789\""},
      {"X-Auth-Token", "secret-token-value"}
  });

  char tokenArr[32] = {0};
  char token[64] = {0};

  client.get("/item")
      .getHeader("token", tokenArr)
      .getHeader("X-Auth-Token", token, sizeof(token));

  expectEq(tokenArr, "W/\"123456789\"", "getHeader char array reference");
  expectEq(token, "secret-token-value", "getHeader char pointer with max size");
}

void testGetHeaderNumericAndBool() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://api.example.com");

  HttpClientStub::setResponse(200, "{}");
  HttpClientStub::setResponseHeaders({
      {"X-Rate-Limit-Remaining", "42"},
      {"X-Timestamp", "1710000050"},
      {"X-Temperature", "25.75"},
      {"X-Factor", "1.234567"},
      {"X-Is-Cached", "true"},
      {"X-Is-Admin", "1"}
  });

  int remaining = 0;
  long timestamp = 0;
  float temp = 0.0f;
  double factor = 0.0;
  bool isCached = false;
  bool isAdmin = false;

  client.get("/status")
      .getHeader("X-Rate-Limit-Remaining", &remaining)
      .getHeader("X-Timestamp", &timestamp)
      .getHeader("X-Temperature", &temp)
      .getHeader("X-Factor", &factor)
      .getHeader("X-Is-Cached", &isCached)
      .getHeader("X-Is-Admin", &isAdmin);

  expectEqInt(remaining, 42, "getHeader int parsed");
  expectEqInt(timestamp, 1710000050L, "getHeader long parsed");
  expectNear(temp, 25.75f, 0.01f, "getHeader float parsed");
  expectNear(factor, 1.234567, 0.00001, "getHeader double parsed");
  expectTrue(isCached, "getHeader bool true string");
  expectTrue(isAdmin, "getHeader bool 1 string");
}

void testGetHeaderCaseInsensitiveAndMissing() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://api.example.com");

  HttpClientStub::setResponse(200, "{}");
  HttpClientStub::setResponseHeaders({
      {"token", "token-lowercase-val"}
  });

  String token;
  String missing = "default_val";
  int missingInt = 999;

  client.get("/data")
      .getHeader("TOKEN", &token)
      .getHeader("X-Missing-Header", &missing)
      .getHeader("X-Missing-Int", &missingInt);

  expectEq(token.c_str(), "token-lowercase-val", "getHeader case-insensitive match");
  expectEq(missing.c_str(), "default_val", "missing header leaves string untouched");
  expectEqInt(missingInt, 999, "missing header leaves int untouched");
}

void testGetHeaderCombinedWithGetBody() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://api.example.com");

  HttpClientStub::setResponse(200, "{\"id\":101,\"name\":\"Sensor Node\"}");
  HttpClientStub::setResponseHeaders({
      {"token", "\"token-abc\""},
      {"X-Server", "nginx"}
  });

  int id = 0;
  char name[32] = {0};
  String token;
  String server;

  client.post("/todos/1")
      .body("title", "Check status")
      .getBody("id", &id)
      .getBody("name", name, sizeof(name))
      .getHeader("token", &token)
      .getHeader("X-Server", &server);

  expectEqInt(id, 101, "getBody id extracted alongside headers");
  expectEq(name, "Sensor Node", "getBody name extracted alongside headers");
  expectEq(token.c_str(), "\"token-abc\"", "getHeader token extracted with getBody");
  expectEq(server.c_str(), "nginx", "getHeader server extracted with getBody");
}

void testGetHeaderRetries() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://api.example.com");

  HttpClientStub::queueResponse(HTTPC_ERROR_CONNECTION_LOST, "");
  HttpClientStub::queueResponseHeaders({});

  HttpClientStub::queueResponse(200, "{\"success\":true}");
  HttpClientStub::queueResponseHeaders({{"X-Retry-Header", "after-retry-value"}});

  String headerVal;
  bool success = false;

  client.get("/retry-target")
      .retry(2)
      .getHeader("X-Retry-Header", &headerVal)
      .getBody("success", &success);

  expectTrue(success, "request succeeded on retry");
  expectEq(headerVal.c_str(), "after-retry-value", "header parsed on retry");
  expectEqInt(HttpClientStub::requestCount, 2, "2 attempts made");
}

void testGetHeaderMoveConstructor() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://api.example.com");

  HttpClientStub::setResponse(200, "{\"msg\":\"ok\"}");
  HttpClientStub::setResponseHeaders({{"token", "moved-token"}});

  String token;
  String msg;

  {
    RestRequest req1 = client.get("/moved");
    req1.getHeader("token", &token).getBody("msg", &msg);
    RestRequest req2 = std::move(req1);
  }

  expectEq(token.c_str(), "moved-token", "header binding moved and executed");
  expectEq(msg.c_str(), "ok", "body binding moved and executed");
}

void testGetHeaderAllHttpMethods() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://api.example.com");

  String postToken;
  HttpClientStub::setResponse(201, "{\"id\":1}");
  HttpClientStub::setResponseHeaders({{"token", "post-token-1"}});
  client.post("/items").body("name", "widget").getHeader("token", &postToken);
  expectEq(postToken.c_str(), "post-token-1", "getHeader on POST");
  expectEq(HttpClientStub::lastMethod.c_str(), "POST", "HTTP method POST");

  String putToken;
  HttpClientStub::setResponse(200, "{\"id\":1}");
  HttpClientStub::setResponseHeaders({{"token", "put-token-2"}});
  client.put("/items/1").body("name", "widget-updated").getHeader("token", &putToken);
  expectEq(putToken.c_str(), "put-token-2", "getHeader on PUT");
  expectEq(HttpClientStub::lastMethod.c_str(), "PUT", "HTTP method PUT");

  String patchToken;
  HttpClientStub::setResponse(200, "{\"id\":1}");
  HttpClientStub::setResponseHeaders({{"token", "patch-token-3"}});
  client.patch("/items/1").body("name", "widget-patched").getHeader("token", &patchToken);
  expectEq(patchToken.c_str(), "patch-token-3", "getHeader on PATCH");
  expectEq(HttpClientStub::lastMethod.c_str(), "PATCH", "HTTP method PATCH");

  String deleteToken;
  HttpClientStub::setResponse(200, "{\"deleted\":true}");
  HttpClientStub::setResponseHeaders({{"token", "delete-token-4"}});
  client.del("/items/1").getHeader("token", &deleteToken);
  expectEq(deleteToken.c_str(), "delete-token-4", "getHeader on DELETE");
  expectEq(HttpClientStub::lastMethod.c_str(), "DELETE", "HTTP method DELETE");

  String updateToken;
  HttpClientStub::setResponse(200, "{\"id\":1}");
  HttpClientStub::setResponseHeaders({{"token", "update-token-5"}});
  client.update("/items/1").body("name", "widget-updated-alias").getHeader("token", &updateToken);
  expectEq(updateToken.c_str(), "update-token-5", "getHeader on update alias");
  expectEq(HttpClientStub::lastMethod.c_str(), "PUT", "HTTP method update calls PUT");
}

void testStringAndNumericTypesInParams() {
  ESP32HTTPClient client("https://jsonplaceholder.typicode.com");

  String title = "asd";
  String desc = "test-desc";
  String route = "items";
  String idStr = "42";

  RestRequest req(&client, "/{route}/{id}", HTTP_POST_METHOD);
  req.path("route", route)
     .path("id", idStr)
     .query("filter", String("active"))
     .query("u_int", 500u)
     .query("u_long", 12345678UL)
     .query("i64", 9876543210LL)
     .query("u64", 18446744073709551615ULL)
     .body("title", title)
     .body("desc", String("inline-string"))
     .body("u_int", 42u)
     .body("u_long", 99999999UL)
     .body("i64", 12345678901234LL)
     .body("u64", 99999999999999ULL);

  expectEqInt(static_cast<long long>(req._pathParams.size()), 2, "path params count");
  expectEq(req._pathParams[0].valueBuffer, "items", "String path param route");
  expectEq(req._pathParams[1].valueBuffer, "42", "String path param id");

  expectEqInt(static_cast<long long>(req._queryParams.size()), 5, "query params count");
  expectEq(req._queryParams[0].valueBuffer, "active", "String query param");
  expectTrue(req._queryParams[0].quoteValue, "String query quoteValue is true");
  expectEq(req._queryParams[1].valueBuffer, "500", "unsigned int query param");
  expectEq(req._queryParams[2].valueBuffer, "12345678", "unsigned long query param");
  expectEq(req._queryParams[3].valueBuffer, "9876543210", "long long query param");
  expectEq(req._queryParams[4].valueBuffer, "18446744073709551615", "unsigned long long query param");

  expectEqInt(static_cast<long long>(req._bodyParams.size()), 6, "body params count");
  expectEq(req._bodyParams[0].valueBuffer, "asd", "String body param");
  expectTrue(req._bodyParams[0].quoteValue, "String body quoteValue is true");
  expectEq(req._bodyParams[1].valueBuffer, "inline-string", "temporary String body param");
  expectTrue(req._bodyParams[1].quoteValue, "temporary String body quoteValue is true");
  expectEq(req._bodyParams[2].valueBuffer, "42", "unsigned int body param");
  expectTrue(!req._bodyParams[2].quoteValue, "unsigned int body quoteValue is false");
  expectEq(req._bodyParams[3].valueBuffer, "99999999", "unsigned long body param");
  expectTrue(!req._bodyParams[3].quoteValue, "unsigned long body quoteValue is false");
  expectEq(req._bodyParams[4].valueBuffer, "12345678901234", "long long body param");
  expectTrue(!req._bodyParams[4].quoteValue, "long long body quoteValue is false");
  expectEq(req._bodyParams[5].valueBuffer, "99999999999999", "unsigned long long body param");
  expectTrue(!req._bodyParams[5].quoteValue, "unsigned long long body quoteValue is false");

  req._executed = true;

  // Test full POST execution with String body param (exact user reproduction)
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"success\":true}");
  {
    String postTitle = "asd";
    client.post("/todos/1")
          .body("title", postTitle);
  }
  expectEq(HttpClientStub::lastMethod, "POST", "POST method called");
  expectEq(HttpClientStub::lastUrl, "https://jsonplaceholder.typicode.com/todos/1", "POST url");
  expectEq(HttpClientStub::lastPayload, "{\"title\":\"asd\"}", "POST payload matches user example");

  // Test getBody char array overload
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"name\":\"device-1\"}");
  char nameBuffer[32] = {0};
  client.get("/device").getBody("name", nameBuffer);
  expectEq(nameBuffer, "device-1", "getBody into char array buffer");
}

void testBase64EncodeEdgeCases() {
  HttpClientStub::reset();
  ESP32HTTPClient client("https://example.com");

  // Length % 3 == 2: "use:p" -> 5 characters -> "dXNlOnA="
  client.basic("use", "p");
  expectEqInt(static_cast<long long>(client._headers.size()), 1, "basic auth header added for len % 3 == 2");
  expectEq(client._headers[0].value, "Basic dXNlOnA=", "base64 encoding with len % 3 == 2");

  // Length % 3 == 1: "a:bc" -> 4 characters -> "YTpiYw=="
  client.basic("a", "bc");
  expectEq(client._headers[0].value, "Basic YTpiYw==", "base64 encoding with len % 3 == 1");

  // Length % 3 == 0: "ab:def" -> 6 characters -> "YWI6ZGVm"
  client.basic("ab", "def");
  expectEq(client._headers[0].value, "Basic YWI6ZGVm", "base64 encoding with len % 3 == 0");
}

void testCookieNullParams() {
  ESP32HTTPClient client("https://example.com");
  client.cookie(nullptr, "value");
  client.cookie("name", nullptr);
  client.cookie(nullptr, nullptr);
  expectEqInt(static_cast<long long>(client._headers.size()), 0, "null cookie name/value should not add header");
}

void testErrorToStringAllCodes() {
  expectEq(ESP32HTTPClient::errorToString(-1).c_str(), "Connection Refused", "-1");
  expectEq(ESP32HTTPClient::errorToString(-2).c_str(), "Send Header Failed", "-2");
  expectEq(ESP32HTTPClient::errorToString(-3).c_str(), "Send Payload Failed", "-3");
  expectEq(ESP32HTTPClient::errorToString(-4).c_str(), "Not Connected", "-4");
  expectEq(ESP32HTTPClient::errorToString(-5).c_str(), "Connection Lost", "-5");
  expectEq(ESP32HTTPClient::errorToString(-6).c_str(), "No Stream", "-6");
  expectEq(ESP32HTTPClient::errorToString(-7).c_str(), "No HTTP Server", "-7");
  expectEq(ESP32HTTPClient::errorToString(-8).c_str(), "Too Less RAM", "-8");
  expectEq(ESP32HTTPClient::errorToString(-9).c_str(), "Encoding Error", "-9");
  expectEq(ESP32HTTPClient::errorToString(-10).c_str(), "Stream Write Error", "-10");
  expectEq(ESP32HTTPClient::errorToString(-11).c_str(), "Read Timeout", "-11");

  expectEq(ESP32HTTPClient::errorToString(200).c_str(), "OK", "200");
  expectEq(ESP32HTTPClient::errorToString(201).c_str(), "Created", "201");
  expectEq(ESP32HTTPClient::errorToString(202).c_str(), "Accepted", "202");
  expectEq(ESP32HTTPClient::errorToString(204).c_str(), "No Content", "204");

  expectEq(ESP32HTTPClient::errorToString(400).c_str(), "Bad Request", "400");
  expectEq(ESP32HTTPClient::errorToString(401).c_str(), "Unauthorized", "401");
  expectEq(ESP32HTTPClient::errorToString(403).c_str(), "Forbidden", "403");
  expectEq(ESP32HTTPClient::errorToString(404).c_str(), "Not Found", "404");
  expectEq(ESP32HTTPClient::errorToString(405).c_str(), "Method Not Allowed", "405");
  expectEq(ESP32HTTPClient::errorToString(408).c_str(), "Request Timeout", "408");
  expectEq(ESP32HTTPClient::errorToString(409).c_str(), "Conflict", "409");
  expectEq(ESP32HTTPClient::errorToString(429).c_str(), "Too Many Requests", "429");

  expectEq(ESP32HTTPClient::errorToString(500).c_str(), "Internal Server Error", "500");
  expectEq(ESP32HTTPClient::errorToString(501).c_str(), "Not Implemented", "501");
  expectEq(ESP32HTTPClient::errorToString(502).c_str(), "Bad Gateway", "502");
  expectEq(ESP32HTTPClient::errorToString(503).c_str(), "Service Unavailable", "503");
  expectEq(ESP32HTTPClient::errorToString(504).c_str(), "Gateway Timeout", "504");
  expectEq(ESP32HTTPClient::errorToString(0).c_str(), "Not Executed", "0");

  expectEq(ESP32HTTPClient::errorToString(-99).c_str(), "Unknown Client Error", "-99");
  expectEq(ESP32HTTPClient::errorToString(299).c_str(), "Success", "299");
  expectEq(ESP32HTTPClient::errorToString(302).c_str(), "Redirection", "302");
  expectEq(ESP32HTTPClient::errorToString(418).c_str(), "Client Error", "418");
  expectEq(ESP32HTTPClient::errorToString(599).c_str(), "Server Error", "599");
  expectEq(ESP32HTTPClient::errorToString(600).c_str(), "Unknown HTTP Status", "600");
}

void testClientAndRequestSingleParamOnError() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(500, "error");
  ESP32HTTPClient client("https://example.com");

  int clientErrCode = 0;
  HttpResponseCallback cbClient = [&](int code) {
    clientErrCode = code;
  };
  client.onError(cbClient);
  client.get("/test");
  expectEqInt(clientErrCode, 500, "client single-param onError invoked");

  client.onError(static_cast<HttpResponseCallback>(nullptr));
  expectTrue(!client._onErrorCb, "client onError cleared");

  int reqErrCode = 0;
  HttpResponseCallback cbReq = [&](int code) {
    reqErrCode = code;
  };
  {
    RestRequest req = client.get("/test");
    req.onError(cbReq);
  }
  expectEqInt(reqErrCode, 500, "request single-param onError invoked");

  RestRequest req2 = client.get("/test");
  req2.onError(static_cast<HttpResponseCallback>(nullptr));
  req2._executed = true;
  expectTrue(!req2._onErrorCb, "request onError cleared");
}

void testClientErrorCallbackTriggered() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(404, "not found");
  ESP32HTTPClient client("https://example.com");
  int capturedCode = 0;
  std::string capturedMsg;
  client.onError([&](int code, const char* msg) {
    capturedCode = code;
    capturedMsg = msg ? msg : "";
  });

  client.get("/missing");
  expectEqInt(capturedCode, 404, "client 2-param onError code");
  expectEq(capturedMsg, "Not Found", "client 2-param onError msg");
}

void testRetryWithClientHeadersAndBody() {
  HttpClientStub::reset();
  HttpClientStub::queueResponse(-1, "");
  HttpClientStub::queueResponse(200, "{\"ok\":true}");

  ESP32HTTPClient client("https://example.com");
  client.setHeader("X-Custom", "Val");
  client.setContentType("application/json");

  {
    RestRequest req = client.post("/items");
    req.maxRetry(1);
    req.body("name", "item1");
  }

  expectEqInt(client.getStatusCode(), 200, "status code 200 after retry");
  expectEqInt(HttpClientStub::requestCount, 2, "2 attempts made");
}

void testExecuteNullClient() {
  RestRequest req(nullptr, "/path", HTTP_GET_METHOD);
  req.execute();
  expectTrue(req._executed, "executed flag set for null client");
}

void testRawJsonRootObjectAndEscapedStrings() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"msg\": \"hello \\\"world\\\" \\\\ test\", \"num\": 123}");

  ESP32HTTPClient client("https://example.com");
  String rawJson;
  client.get("/data").getBody("", &rawJson);

  expectEq(rawJson.c_str(), "{\"msg\": \"hello \\\"world\\\" \\\\ test\", \"num\": 123}", "raw JSON root object captured with escaped chars");
}

void testParsePrimitiveEdgeCases() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"d_null\": null, \"l_null\": null, \"str_int\": \"1234\", \"num_str\": 9876}");

  ESP32HTTPClient client("https://example.com");
  double dVal = 123.45;
  long lVal = 999L;
  int strIntVal = 0;
  String numStrVal;

  client.get("/primitives")
        .getBody("d_null", &dVal)
        .getBody("l_null", &lVal)
        .getBody("str_int", &strIntVal)
        .getBody("num_str", &numStrVal);

  expectNear(dVal, 0.0, 0.001, "null double set to 0.0");
  expectEqInt(lVal, 0L, "null long set to 0L");
  expectEqInt(strIntVal, 1234, "string coerced to int");
  expectEq(numStrVal.c_str(), "9876", "number coerced to Arduino String");
}

void testArrayBindingsAdvanced() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "[10, 20, 30]");

  ESP32HTTPClient client("https://example.com");
  int item0 = 0, item1 = 0, item2 = 0;
  client.get("/array")
        .getBody("0", &item0)
        .getBody("1", &item1)
        .getBody("2", &item2);

  expectEqInt(item0, 10, "array[0] primitive");
  expectEqInt(item1, 20, "array[1] primitive");
  expectEqInt(item2, 30, "array[2] primitive");

  // Raw array element
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "[{\"a\":1}, {\"b\":2}]");
  String elem0;
  client.get("/raw_array_elem")
        .getBody("0", &elem0);
  expectEq(elem0.c_str(), "{\"a\":1}", "array element raw json string");

  // Nested array in array
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "[[100, 200], [300, 400]]");
  int nestedVal = 0;
  client.get("/nested_array")
        .getBody("1.0", &nestedVal);
  expectEqInt(nestedVal, 300, "nested array element 1.0");
}

void testSkipValueObjectAndArray() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"skip_obj\": {\"a\": 1, \"b\": \"str with \\\"quote\\\" and \\\\ slash\", \"nested\": [10, 20]}, \"skip_arr\": [{\"x\": true}, [1, 2]], \"keep\": 42}");

  ESP32HTTPClient client("https://example.com");
  int keep = 0;
  client.get("/skip")
        .getBody("keep", &keep);

  expectEqInt(keep, 42, "unmapped complex object and array skipped properly");
}

void testMalformedJsonHandling() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"badkey\" 123, \"goodkey\": 456}");

  ESP32HTTPClient client("https://example.com");
  int good = 0;
  client.get("/malformed")
        .getBody("goodkey", &good);

  expectEqInt(good, 456, "malformed key missing colon is skipped");
}

class MockDirectStream : public Stream {
 public:
  std::string data;
  size_t cursor = 0;
  bool availableTimesOut = false;
  int availableLimit = -1;
  int readBytesLimit = -1;
  bool simulateZeroBytesRead = false;

  explicit MockDirectStream(const std::string& d = "") : data(d), cursor(0) {}

  int available() override {
    if (availableTimesOut) return 0;
    if (availableLimit >= 0) {
      if (availableLimit > 0) {
        availableLimit--;
        return static_cast<int>(data.size() - cursor);
      }
      return 0;
    }
    return cursor < data.size() ? static_cast<int>(data.size() - cursor) : 0;
  }

  int read() override {
    if (cursor < data.size()) {
      return static_cast<unsigned char>(data[cursor++]);
    }
    return -1;
  }

  int peek() override {
    if (cursor < data.size()) {
      return static_cast<unsigned char>(data[cursor]);
    }
    return -1;
  }

  size_t readBytes(char* buffer, size_t length) override {
    if (simulateZeroBytesRead) {
      simulateZeroBytesRead = false;
      return 0;
    }
    size_t toRead = length;
    if (readBytesLimit >= 0 && toRead > static_cast<size_t>(readBytesLimit)) {
      toRead = static_cast<size_t>(readBytesLimit);
    }
    size_t avail = cursor < data.size() ? data.size() - cursor : 0;
    if (toRead > avail) toRead = avail;
    for (size_t i = 0; i < toRead; i++) {
      buffer[i] = data[cursor++];
    }
    return toRead;
  }
};

struct NullStrStruct {
  const char* text = nullptr;
  REST_JSON_MAP(
    REST_FIELD(text)
  )
};

void testStructNullStringField() {
  NullStrStruct s;
  String json = ESP32HTTPClient::toJson(s);
  expectEq(json.c_str(), "{\"text\":null}", "null string field serializes to null");
}

void testSkipValueEscapedString() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"skip_str\": \"hello \\\"world\\\" \\\\ test\", \"keep\": 123}");
  ESP32HTTPClient client("https://example.com");
  int keep = 0;
  client.get("/skip_esc").getBody("keep", &keep);
  expectEqInt(keep, 123, "skipValue skips escaped string");
}

void testObjectWithNestedArrayChildBinding() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"items\": [100, 200], \"flag\": true}");
  ESP32HTTPClient client("https://example.com");
  int item0 = 0;
  bool flag = false;
  client.get("/nested_arr_in_obj")
        .getBody("items.0", &item0)
        .getBody("flag", &flag);
  expectEqInt(item0, 100, "items.0 extracted from object");
  expectTrue(flag, "flag extracted");
}

void testBufferedStreamReaderDirect() {
  // Non-chunked with custom stream
  MockDirectStream directStream("{\"val\":\"H\"}");
  HttpClientStub::reset();
  HttpClientStub::customStream = &directStream;
  HttpClientStub::customSize = static_cast<int>(directStream.data.size());
  {
    ESP32HTTPClient client("https://example.com");
    String val;
    client.get("/test").getBody("val", &val);
    expectEq(val.c_str(), "H", "custom directStream read");
  }

  // Timeout in non-chunked refill
  MockDirectStream timeoutStream;
  timeoutStream.availableTimesOut = true;
  HttpClientStub::reset();
  HttpClientStub::customStream = &timeoutStream;
  HttpClientStub::customSize = 10;
  {
    ESP32HTTPClient client("https://example.com");
    String val;
    client.get("/timeout").getBody("val", &val);
    expectEq(val.c_str(), "", "timeout in non-chunked returns empty");
  }

  // Non-chunked large buffer (> BUF_SIZE)
  std::string largePayload(700, 'X');
  std::string largeJson = "{\"large\":\"" + largePayload + "\"}";
  MockDirectStream largeStream(largeJson);
  HttpClientStub::reset();
  HttpClientStub::customStream = &largeStream;
  HttpClientStub::customSize = static_cast<int>(largeJson.size());
  {
    ESP32HTTPClient client("https://example.com");
    String val;
    client.get("/large").getBody("", &val);
    expectEqInt(static_cast<long long>(val.length()), static_cast<long long>(largeJson.size()), "large stream read length 700");
  }

  // Chunked stream with extensions (;) and empty hex lines
  std::string chunkedExtData = "\r\n13;ext=test\r\n{\"msg\":\"WORLD\"}\r\n0\r\n\r\n";
  MockDirectStream chunkedExtStream(chunkedExtData);
  HttpClientStub::reset();
  HttpClientStub::customStream = &chunkedExtStream;
  HttpClientStub::customSize = -1;
  {
    ESP32HTTPClient client("https://example.com");
    String val;
    client.get("/chunked").getBody("msg", &val);
    expectEq(val.c_str(), "WORLD", "chunked with extensions read WORLD");
  }

  // Chunked stream timeout in chunk header
  MockDirectStream chunkTimeoutHeader;
  chunkTimeoutHeader.availableTimesOut = true;
  HttpClientStub::reset();
  HttpClientStub::customStream = &chunkTimeoutHeader;
  HttpClientStub::customSize = -1;
  {
    ESP32HTTPClient client("https://example.com");
    String val;
    client.get("/chunk_to_hdr").getBody("msg", &val);
    expectEq(val.c_str(), "", "chunk timeout header returns empty");
  }

  // Chunked stream timeout in chunk data
  MockDirectStream chunkTimeoutData("5\r\n");
  HttpClientStub::reset();
  HttpClientStub::customStream = &chunkTimeoutData;
  HttpClientStub::customSize = -1;
  {
    ESP32HTTPClient client("https://example.com");
    String val;
    client.get("/chunk_to_data").getBody("msg", &val);
    expectEq(val.c_str(), "", "chunk timeout data returns empty");
  }

  // Chunk larger than BUF_SIZE (triggering toRead > BUF_SIZE - _len and refill loop completion)
  std::string bigChunkVal(600, 'Z');
  std::string bigChunkPayload = "{\"data\":\"" + bigChunkVal + "\"}";
  char hexBuf[16];
  snprintf(hexBuf, sizeof(hexBuf), "%x\r\n", static_cast<unsigned int>(bigChunkPayload.size()));
  std::string bigChunkData = std::string(hexBuf) + bigChunkPayload + "\r\n0\r\n\r\n";
  MockDirectStream bigChunkStream(bigChunkData);
  HttpClientStub::reset();
  HttpClientStub::customStream = &bigChunkStream;
  HttpClientStub::customSize = -1;
  {
    ESP32HTTPClient client("https://example.com");
    String val;
    client.get("/big_chunk").getBody("", &val);
    expectEqInt(static_cast<long long>(val.length()), static_cast<long long>(bigChunkPayload.size()), "big chunk read length 600");
  }

  // Chunked stream with simulateZeroBytesRead
  MockDirectStream zeroBytesStream("10\r\n{\"data\":\"HI\"}\r\n0\r\n\r\n");
  zeroBytesStream.simulateZeroBytesRead = true;
  HttpClientStub::reset();
  HttpClientStub::customStream = &zeroBytesStream;
  HttpClientStub::customSize = -1;
  {
    ESP32HTTPClient client("https://example.com");
    String val;
    client.get("/zero_bytes").getBody("data", &val);
    expectEq(val.c_str(), "", "zero bytes read causes eof");
  }

  // Chunked stream with partial read (avail < toRead)
  MockDirectStream partialStream("10\r\n{\"data\":\"HI\"}\r\n0\r\n\r\n");
  partialStream.readBytesLimit = 2;
  HttpClientStub::reset();
  HttpClientStub::customStream = &partialStream;
  HttpClientStub::customSize = -1;
  {
    ESP32HTTPClient client("https://example.com");
    String val;
    client.get("/partial").getBody("data", &val);
    expectEq(val.c_str(), "HI", "partial chunk read available");
  }

  HttpClientStub::reset();
}
}

int main() {
  runSuite("AddParamFormatting", testAddParamFormatting);
  runSuite("ExecuteBuildsUrlAndPayload", testExecuteBuildsUrlAndPayload);
  runSuite("MethodAliases", testMethodAliases);
  runSuite("ParseResponseBindsTypes", testParseResponseBindsTypes);
  runSuite("ParseNestedJSON", testParseNestedJSON);
  runSuite("ParseNestedJSONMissingFields", testParseNestedJSONMissingFields);
  runSuite("ParseRawArrayJSON", testParseRawArrayJSON);
  runSuite("UnixTimestampFetch", testUnixTimestampFetch);
  runSuite("ClientConfiguration", testClientConfiguration);
  runSuite("AllHttpMethods", testAllHttpMethods);
  runSuite("AutoRetry", testAutoRetry);
  runSuite("ChunkedTransferEncoding", testChunkedTransferEncoding);
  runSuite("LargePayloadRefill", testLargePayloadRefill);
  runSuite("MoreParseEdgeCases", testMoreParseEdgeCases);
  runSuite("ParseEmptyObjectOrArray", testParseEmptyObjectOrArray);
  runSuite("AddParamAdvanced", testAddParamAdvanced);
  runSuite("StringAndNumericTypesInParams", testStringAndNumericTypesInParams);
  runSuite("ExecuteCustomPortWithPath", testExecuteCustomPortWithPath);
  runSuite("ParseEscapedStringsSimple", testParseEscapedStringsSimple);
  runSuite("SkipValueAdvanced", testSkipValueAdvanced);
  runSuite("MoveConstructor", testMoveConstructor);
  runSuite("ParsePrimitiveTypes", testParsePrimitiveTypes);
  runSuite("AuthBearer", testAuthBearer);
  runSuite("AuthBasic", testAuthBasic);
  runSuite("AuthApiKey", testAuthApiKey);
  runSuite("AuthCookie", testAuthCookie);
  runSuite("Observability", testObservability);
  runSuite("AuthEdgeCases", testAuthEdgeCases);
  runSuite("QueryParameters", testQueryParameters);
  runSuite("PathParameters", testPathParameters);
  runSuite("PathAndQueryCombined", testPathAndQueryCombined);
  runSuite("PathEdgeCases", testPathEdgeCases);
  runSuite("RuntimeUrlChange", testRuntimeUrlChange);
  runSuite("Timeout", testTimeout);
  runSuite("MaxRetry", testMaxRetry);
  runSuite("ErrorHandlingAndMessages", testErrorHandlingAndMessages);
  runSuite("Callbacks", testCallbacks);
  runSuite("StructSerializationToJson", testStructSerializationToJson);
  runSuite("StructDeserializationFromJson", testStructDeserializationFromJson);
  runSuite("StructMissingFieldsAndDefaults", testStructMissingFieldsAndDefaults);
  runSuite("StructNullFields", testStructNullFields);
  runSuite("StructRequestBody", testStructRequestBody);
  runSuite("StructResponseBodyGetBody", testStructResponseBodyGetBody);
  runSuite("StructCustomAndExternalMapping", testStructCustomAndExternalMapping);
  runSuite("GetHeaderString", testGetHeaderString);
  runSuite("GetHeaderCharArray", testGetHeaderCharArray);
  runSuite("GetHeaderNumericAndBool", testGetHeaderNumericAndBool);
  runSuite("GetHeaderCaseInsensitiveAndMissing", testGetHeaderCaseInsensitiveAndMissing);
  runSuite("GetHeaderCombinedWithGetBody", testGetHeaderCombinedWithGetBody);
  runSuite("GetHeaderRetries", testGetHeaderRetries);
  runSuite("GetHeaderMoveConstructor", testGetHeaderMoveConstructor);
  runSuite("GetHeaderAllHttpMethods", testGetHeaderAllHttpMethods);
  runSuite("Base64EncodeEdgeCases", testBase64EncodeEdgeCases);
  runSuite("CookieNullParams", testCookieNullParams);
  runSuite("ErrorToStringAllCodes", testErrorToStringAllCodes);
  runSuite("ClientAndRequestSingleParamOnError", testClientAndRequestSingleParamOnError);
  runSuite("ClientErrorCallbackTriggered", testClientErrorCallbackTriggered);
  runSuite("RetryWithClientHeadersAndBody", testRetryWithClientHeadersAndBody);
  runSuite("ExecuteNullClient", testExecuteNullClient);
  runSuite("RawJsonRootObjectAndEscapedStrings", testRawJsonRootObjectAndEscapedStrings);
  runSuite("ParsePrimitiveEdgeCases", testParsePrimitiveEdgeCases);
  runSuite("ArrayBindingsAdvanced", testArrayBindingsAdvanced);
  runSuite("SkipValueObjectAndArray", testSkipValueObjectAndArray);
  runSuite("MalformedJsonHandling", testMalformedJsonHandling);
  runSuite("StructNullStringField", testStructNullStringField);
  runSuite("SkipValueEscapedString", testSkipValueEscapedString);
  runSuite("ObjectWithNestedArrayChildBinding", testObjectWithNestedArrayChildBinding);
  runSuite("BufferedStreamReaderDirect", testBufferedStreamReaderDirect);

  const int suitesFailed = suitesRun - suitesPassed;
  const double suitePassRate = suitesRun > 0 ? (100.0 * static_cast<double>(suitesPassed) / static_cast<double>(suitesRun)) : 0.0;
  const double checkPassRate = checks > 0 ? (100.0 * static_cast<double>(passedChecks) / static_cast<double>(checks)) : 0.0;

  std::cout << "\n=== Test Summary ===\n";
  std::cout << "Suites: " << suitesRun << " total | " << suitesPassed << " passed | " << suitesFailed << " failed\n";
  std::cout << "Checks: " << checks << " total | " << passedChecks << " passed | " << failures << " failed\n";
  std::cout << std::fixed << std::setprecision(1);
  std::cout << "Pass rate: " << suitePassRate << "% suites | " << checkPassRate << "% checks\n";

  if (failures == 0) {
    std::cout << "All unit tests passed.\n";
    return 0;
  }

  std::cerr << failures << " test(s) failed.\n";
  return 1;
}
