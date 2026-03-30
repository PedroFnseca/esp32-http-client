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
  req.parseResponse(&stream);

  expectEqInt(count, 7, "count should be parsed as int");
  expectNear(temperature, 24.5, 0.001, "temperature should be parsed as float");
  expectNear(voltage, 3.3001, 0.00001, "voltage should be parsed as double");
  expectTrue(active, "active should be parsed as bool");
  expectEqInt(timestamp, 1710000010L, "timestamp should be parsed as long");
  expectEq(label, "sensor\"A", "escaped string should be unescaped and copied");

  req._executed = true;
}
}  // namespace

int main() {
  runSuite("AddParamFormatting", testAddParamFormatting);
  runSuite("ExecuteBuildsUrlAndPayload", testExecuteBuildsUrlAndPayload);
  runSuite("MethodAliases", testMethodAliases);
  runSuite("ParseResponseBindsTypes", testParseResponseBindsTypes);

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
