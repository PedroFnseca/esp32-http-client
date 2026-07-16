#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>

#include "HTTPClient.h"

#define private public
#include "ESP32HTTPClient.h"
#include "RestRequest.h"
#include "BufferedStreamReader.h"
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
}  // namespace

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
  runSuite("ExecuteCustomPortWithPath", testExecuteCustomPortWithPath);
  runSuite("ParseEscapedStringsSimple", testParseEscapedStringsSimple);
  runSuite("SkipValueAdvanced", testSkipValueAdvanced);
  runSuite("MoveConstructor", testMoveConstructor);
  runSuite("ParsePrimitiveTypes", testParsePrimitiveTypes);

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
