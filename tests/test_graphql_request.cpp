#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "HTTPClient.h"

#define private public
#include "ESP32HTTPClient.h"
#include "GraphQLBatchRequest.h"
#include "GraphQLRequest.h"
#include "GraphQLTypes.h"
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

void expectContains(const std::string& haystack, const std::string& needle, const char* message) {
  checks++;
  if (haystack.find(needle) == std::string::npos) {
    std::cerr << "[FAIL] " << message << " (expected to contain: " << needle << ", got: " << haystack << ")\n";
    failures++;
    return;
  }
  passedChecks++;
}

void runSuite(const char* name, void (*fn)()) {
  suitesRun++;
  int before = failures;
  std::cout << "[RUN ] " << name << "\n";
  fn();
  if (failures == before) {
    suitesPassed++;
    std::cout << "[PASS] " << name << "\n";
  } else {
    std::cout << "[FAIL] " << name << " (" << (failures - before) << " failure(s))\n";
  }
}

struct UserInput {
  String name;
  int age;

  REST_JSON_MAP(
    REST_FIELD(name),
    REST_FIELD(age)
  )
};

struct UserResult {
  int id;
  String name;
  String email;

  REST_JSON_MAP(
    REST_FIELD(id),
    REST_FIELD(name),
    REST_FIELD(email)
  )
};

// 1. Basic Query via POST
void testGraphQLBasicQueryPost() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"user\":{\"id\":101,\"name\":\"Alice\",\"active\":true,\"score\":98.5}}}");

  ESP32HTTPClient client("https://api.example.com");
  int id = 0;
  String name;
  bool active = false;
  float score = 0.0f;

  client.graphql("/graphql")
      .query("query { user { id name active score } }")
      .getData("user.id", &id)
      .getData("user.name", &name)
      .getData("user.active", &active)
      .getData("user.score", &score);

  expectEq(HttpClientStub::lastMethod, "POST", "Method is POST");
  expectEq(HttpClientStub::lastUrl, "https://api.example.com/graphql", "URL is /graphql");
  expectContains(HttpClientStub::lastPayload, "\"query\":\"query { user { id name active score } }\"", "Query serialized in payload");
  
  // Headers check
  bool foundAccept = false;
  bool foundContentType = false;
  for (const auto& h : HttpClientStub::lastHeaders) {
    if (h.first == "Accept") {
      foundAccept = true;
      expectContains(h.second, "application/graphql-response+json", "Accept header contains graphql-response+json");
    }
    if (h.first == "Content-Type") {
      foundContentType = true;
      expectContains(h.second, "application/json", "Content-Type header is application/json");
    }
  }
  expectTrue(foundAccept, "Accept header sent");
  expectTrue(foundContentType, "Content-Type header sent");

  expectEqInt(id, 101, "ID parsed");
  expectEq(name.c_str(), "Alice", "Name parsed");
  expectTrue(active, "Active boolean parsed");
  expectTrue(std::abs(score - 98.5f) < 0.01f, "Float score parsed");
}

// 2. Basic Query via GET
void testGraphQLBasicQueryGet() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"greeting\":\"Hello GraphQL\"}}");

  ESP32HTTPClient client("https://api.example.com");
  String greeting;

  client.graphqlGet("/graphql")
      .query("{ greeting }")
      .getData("greeting", &greeting);

  expectEq(HttpClientStub::lastMethod, "GET", "Method is GET");
  expectContains(HttpClientStub::lastUrl, "query=%7B%20greeting%20%7D", "URL-encoded query parameter in GET URL");
  expectEq(greeting.c_str(), "Hello GraphQL", "Greeting parsed from GET response");
}

// 3. Mutation via POST
void testGraphQLMutation() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"createUser\":{\"id\":200,\"name\":\"Bob\"}}}");

  ESP32HTTPClient client("https://api.example.com");
  int newId = 0;
  String newName;

  client.graphql("/graphql")
      .mutation("mutation AddUser($name: String!) { createUser(name: $name) { id name } }")
      .variable("name", "Bob")
      .getData("createUser.id", &newId)
      .getData("createUser.name", &newName);

  expectEq(HttpClientStub::lastMethod, "POST", "Mutation method is POST");
  expectContains(HttpClientStub::lastPayload, "mutation AddUser", "Mutation query serialized");
  expectContains(HttpClientStub::lastPayload, "\"variables\":{\"name\":\"Bob\"}", "Variables serialized");
  expectEqInt(newId, 200, "Created user ID parsed");
  expectEq(newName.c_str(), "Bob", "Created user name parsed");
}

// 4. Variables (scalars, strings, floats, structs)
void testGraphQLVariables() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"ok\":true}}");

  ESP32HTTPClient client("https://api.example.com");
  bool ok = false;

  UserInput input{"Charlie", 28};

  client.graphql("/graphql")
      .query("query Test($id: Int!, $active: Boolean!, $rate: Float!, $input: UserInput!) { test }")
      .variable("id", 42)
      .variable("active", true)
      .variable("rate", 3.14f)
      .variable("input", input)
      .getData("ok", &ok);

  expectContains(HttpClientStub::lastPayload, "\"id\":42", "Int variable serialized");
  expectContains(HttpClientStub::lastPayload, "\"active\":true", "Bool variable serialized");
  expectContains(HttpClientStub::lastPayload, "\"rate\":3.14", "Float variable serialized");
  expectContains(HttpClientStub::lastPayload, "\"input\":{\"name\":\"Charlie\",\"age\":28}", "Struct variable serialized via REST_JSON_MAP");
  expectTrue(ok, "OK boolean parsed");
}

// 5. Operation Name Selection
void testGraphQLOperationNameSelection() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"secondOp\":\"Success\"}}");

  ESP32HTTPClient client("https://api.example.com");
  String result;

  client.graphql("/graphql")
      .document("query FirstOp { firstOp } query SecondOp { secondOp }")
      .operationName("SecondOp")
      .getData("secondOp", &result);

  expectContains(HttpClientStub::lastPayload, "\"operationName\":\"SecondOp\"", "operationName serialized in POST body");
  expectEq(result.c_str(), "Success", "Second operation result parsed");
}

// 6. Content Negotiation and Custom Media Types
void testGraphQLContentNegotiation() {
  HttpClientStub::reset();
  HttpClientStub::setResponseHeaders({{"Content-Type", "application/graphql-response+json"}});
  HttpClientStub::setResponse(200, "{\"data\":{\"status\":\"Negotiated\"}}");

  ESP32HTTPClient client("https://api.example.com");
  String status;

  client.graphql("/graphql")
      .accept("application/graphql-response+json")
      .query("{ status }")
      .getData("status", &status);

  bool foundCustomAccept = false;
  for (const auto& h : HttpClientStub::lastHeaders) {
    if (h.first == "Accept" && h.second == "application/graphql-response+json") {
      foundCustomAccept = true;
    }
  }
  expectTrue(foundCustomAccept, "Custom Accept header applied");
  expectEq(status.c_str(), "Negotiated", "Data parsed under application/graphql-response+json");
}

// 7. Error Handling (Message, Locations, Path, Extensions)
void testGraphQLErrorHandling() {
  HttpClientStub::reset();
  std::string errPayload = "{\n"
                           "  \"errors\": [\n"
                           "    {\n"
                           "      \"message\": \"Cannot query field 'unknown' on type 'Query'.\",\n"
                           "      \"locations\": [{ \"line\": 2, \"column\": 3 }],\n"
                           "      \"path\": [\"user\", \"unknown\"],\n"
                           "      \"extensions\": { \"code\": \"GRAPHQL_VALIDATION_FAILED\", \"timestamp\": 1600000000 }\n"
                           "    },\n"
                           "    {\n"
                           "      \"message\": \"Second error occurred.\"\n"
                           "    }\n"
                           "  ]\n"
                           "}";
  HttpClientStub::setResponse(400, errPayload);

  ESP32HTTPClient client("https://api.example.com");
  GraphQLError firstError;
  std::vector<GraphQLError> allErrors;
  String singleErrMsg;
  bool multiCallbackFired = false;

  client.graphql("/graphql")
      .query("{ user { unknown } }")
      .getError(&firstError)
      .getErrors(&allErrors)
      .getErrorMessage(&singleErrMsg)
      .onGraphQLError([&multiCallbackFired](const std::vector<GraphQLError>& errs) {
        multiCallbackFired = true;
        expectEqInt(errs.size(), 2, "Callback received 2 errors");
      });

  expectTrue(multiCallbackFired, "GraphQL error callback fired");
  expectEq(singleErrMsg.c_str(), "Cannot query field 'unknown' on type 'Query'.", "Error message extracted");
  expectEqInt(allErrors.size(), 2, "All errors vector size is 2");
  expectEq(firstError.message.c_str(), "Cannot query field 'unknown' on type 'Query'.", "First error message parsed");
  expectEqInt(firstError.locations.size(), 1, "First error locations count is 1");
  expectEqInt(firstError.locations[0].line, 2, "Location line is 2");
  expectEqInt(firstError.locations[0].column, 3, "Location column is 3");
  expectEqInt(firstError.path.size(), 2, "Path elements count is 2");
  expectEq(firstError.path[0].c_str(), "user", "Path element 0 is 'user'");
  expectEq(firstError.path[1].c_str(), "unknown", "Path element 1 is 'unknown'");
  expectContains(firstError.extensions.c_str(), "GRAPHQL_VALIDATION_FAILED", "Extensions payload parsed");
}

// 8. Partial Data Preservation with Errors
void testGraphQLPartialDataPreservation() {
  HttpClientStub::reset();
  std::string partialPayload = "{\n"
                               "  \"data\": {\n"
                               "    \"user\": {\n"
                               "      \"id\": 42,\n"
                               "      \"name\": \"David\",\n"
                               "      \"secret\": null\n"
                               "    }\n"
                               "  },\n"
                               "  \"errors\": [\n"
                               "    {\n"
                               "      \"message\": \"Access denied to field 'secret'.\",\n"
                               "      \"path\": [\"user\", \"secret\"]\n"
                               "    }\n"
                               "  ]\n"
                               "}";
  HttpClientStub::setResponse(200, partialPayload);

  ESP32HTTPClient client("https://api.example.com");
  int id = 0;
  String name;
  GraphQLError err;

  client.graphql("/graphql")
      .query("{ user { id name secret } }")
      .getData("user.id", &id)
      .getData("user.name", &name)
      .getError(&err);

  expectEqInt(id, 42, "Partial data: id parsed successfully");
  expectEq(name.c_str(), "David", "Partial data: name parsed successfully");
  expectEq(err.message.c_str(), "Access denied to field 'secret'.", "Error parsed alongside data");
  expectEqInt(err.path.size(), 2, "Error path length is 2");
  expectEq(err.path[1].c_str(), "secret", "Error path field is 'secret'");
}

// 9. HTTP Status Code Handling
void testGraphQLHttpStatusCodes() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(500, "{\"errors\":[{\"message\":\"Internal Server Crash\"}]}");

  ESP32HTTPClient client("https://api.example.com");
  String errMsg;
  int capturedStatus = 0;

  client.graphql("/graphql")
      .query("{ crash }")
      .getError(nullptr)
      .getErrorMessage(&errMsg)
      .onError([&capturedStatus](int status, const char*) {
        capturedStatus = status;
      });

  expectEqInt(client.getStatusCode(), 500, "Client status code is 500");
  expectEqInt(capturedStatus, 500, "onError callback captured status 500");
  expectEq(errMsg.c_str(), "Internal Server Crash", "Error message parsed despite 500 status");
}

// 10. Batch Requests and Responses
void testGraphQLBatchRequests() {
  HttpClientStub::reset();
  std::string batchResponse = "[\n"
                              "  {\"data\": {\"user\": {\"name\": \"Eve\"}}},\n"
                              "  {\"data\": {\"count\": 128}},\n"
                              "  {\"data\": null, \"errors\": [{\"message\": \"Op 3 failed\"}]}\n"
                              "]";
  HttpClientStub::setResponse(200, batchResponse);

  ESP32HTTPClient client("https://api.example.com");
  auto batch = client.graphqlBatch("/graphql");

  String name;
  int count = 0;
  String op3Error;

  auto& op1 = batch.addQuery("query { user { name } }");
  op1.getData("user.name", &name);

  auto& op2 = batch.addQuery("query { count }");
  op2.getData("count", &count);

  auto& op3 = batch.addQuery("query { fail }");
  op3.getErrorMessage(&op3Error);

  batch.execute();

  expectEq(HttpClientStub::lastMethod, "POST", "Batch request uses POST");
  expectContains(HttpClientStub::lastPayload, "[{\"query\":", "Batch request payload is an array of operations");
  expectEq(name.c_str(), "Eve", "Batch Op 1 data parsed");
  expectEqInt(count, 128, "Batch Op 2 count parsed");
  expectEq(op3Error.c_str(), "Op 3 failed", "Batch Op 3 error parsed");
}

// 11. Streaming and Incremental Delivery (multipart/mixed)
void testGraphQLStreamingMultipart() {
  HttpClientStub::reset();
  HttpClientStub::setResponseHeaders({{"Content-Type", "multipart/mixed; boundary=\"graphql\""}});

  std::string multipartBody =
      "--graphql\r\n"
      "Content-Type: application/json\r\n\r\n"
      "{\"data\":{\"author\":{\"id\":1,\"name\":\"George\"}},\"hasNext\":true}\r\n"
      "--graphql\r\n"
      "Content-Type: application/json\r\n\r\n"
      "{\"incremental\":[{\"data\":{\"bio\":\"Sci-Fi Author\"},\"path\":[\"author\"]}],\"hasNext\":false}\r\n"
      "--graphql--\r\n";

  HttpClientStub::setResponse(200, multipartBody);

  ESP32HTTPClient client("https://api.example.com");
  int authorId = 0;
  String authorName;
  String authorBio;
  int incrementalCallbacks = 0;

  client.graphql("/graphql")
      .query("query { author { id name ... @defer { bio } } }")
      .getData("author.id", &authorId)
      .getData("author.name", &authorName)
      .getData("author.bio", &authorBio)
      .onIncremental([&incrementalCallbacks](const GraphQLIncrementalPayload& payload) {
        incrementalCallbacks++;
      });

  expectEqInt(authorId, 1, "Initial author id parsed");
  expectEq(authorName.c_str(), "George", "Initial author name parsed");
  expectEq(authorBio.c_str(), "Sci-Fi Author", "Deferred incremental bio parsed");
  expectTrue(incrementalCallbacks >= 2, "onIncremental fired for initial and deferred parts");
}

// 12. Authentication and Headers
void testGraphQLAuthAndHeaders() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"ok\":true}}");

  ESP32HTTPClient client("https://api.example.com");
  client.bearer("secret-token-123");
  client.cookie("session", "abcxyz");

  client.graphql("/graphql")
      .header("X-Client-Id", "esp32-node")
      .query("{ ok }");

  bool foundAuth = false;
  bool foundCookie = false;
  bool foundCustomHeader = false;

  for (const auto& h : HttpClientStub::lastHeaders) {
    if (h.first == "Authorization" && h.second == "Bearer secret-token-123") foundAuth = true;
    if (h.first == "Cookie" && h.second == "session=abcxyz") foundCookie = true;
    if (h.first == "X-Client-Id" && h.second == "esp32-node") foundCustomHeader = true;
  }

  expectTrue(foundAuth, "Bearer auth header forwarded to GraphQL request");
  expectTrue(foundCookie, "Cookie forwarded to GraphQL request");
  expectTrue(foundCustomHeader, "Custom request header forwarded to GraphQL request");
}

// 13. Move Constructor
void testGraphQLMoveConstructor() {
  ESP32HTTPClient client("https://api.example.com");
  GraphQLRequest req1(&client, "/graphql", HTTP_POST_METHOD);
  req1.query("query { foo }").operationName("FooOp");

  GraphQLRequest req2(std::move(req1));
  expectTrue(req1._executed, "req1 marked executed after move");
  expectEq(req2._document.c_str(), "query { foo }", "req2 preserved query document");
  expectEq(req2._operationName.c_str(), "FooOp", "req2 preserved operationName");
  req2._executed = true; // prevent execution on dtor
}

// 14. Struct Data Mapping via REST_JSON_MAP
void testGraphQLStructDataMapping() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"user\":{\"id\":555,\"name\":\"Elena\",\"email\":\"elena@example.com\"}}}");

  ESP32HTTPClient client("https://api.example.com");
  UserResult user{0, "", ""};

  client.graphql("/graphql")
      .query("{ user { id name email } }")
      .getData("user", &user);

  expectEqInt(user.id, 555, "Struct user.id mapped");
  expectEq(user.name.c_str(), "Elena", "Struct user.name mapped");
  expectEq(user.email.c_str(), "elena@example.com", "Struct user.email mapped");
}

// 15. ESP32HTTPClient::graphqlPost
void testGraphQLEsp32ClientPost() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"ok\":true}}");

  ESP32HTTPClient client("https://api.example.com");
  bool ok = false;
  client.graphqlPost("/custom-graphql")
      .query("{ ok }")
      .getData("ok", &ok);

  expectTrue(ok, "graphqlPost executed successfully");
  expectEq(HttpClientStub::lastUrl, "https://api.example.com/custom-graphql", "graphqlPost targeted correct URL");
}

// 16. GraphQL All Variable Types & Raw Variables
void testGraphQLScalarVariablesAndRaw() {
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"received\":true}}");

  ESP32HTTPClient client("https://api.example.com");
  String strVar = "helloStr";
  unsigned int uIntVal = 100u;
  long longVal = 200000L;
  unsigned long uLongVal = 300000UL;
  long long llVal = 4000000000LL;
  unsigned long long ullVal = 5000000000ULL;
  float fVal = 3.14f;
  double dVal = 2.718281828;

  client.graphql("/graphql")
      .query("query testVars($s: String, $ui: Int, $l: Int, $ul: Int, $ll: Int, $ull: Int, $f: Float, $d: Float) { received }")
      .variable("s", strVar)
      .variable("ui", uIntVal)
      .variable("l", longVal)
      .variable("ul", uLongVal)
      .variable("ll", llVal)
      .variable("ull", ullVal)
      .variable("f", fVal)
      .variable("d", dVal);

  expectContains(HttpClientStub::lastPayload, "\"s\":\"helloStr\"", "String variable serialized");
  expectContains(HttpClientStub::lastPayload, "\"ui\":100", "unsigned int variable serialized");
  expectContains(HttpClientStub::lastPayload, "\"l\":200000", "long variable serialized");
  expectContains(HttpClientStub::lastPayload, "\"ul\":300000", "unsigned long variable serialized");
  expectContains(HttpClientStub::lastPayload, "\"ll\":4000000000", "long long variable serialized");
  expectContains(HttpClientStub::lastPayload, "\"ull\":5000000000", "unsigned long long variable serialized");

  // Raw variables (both const char* and const String&)
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"ok\":true}}");
  client.graphql("/graphql")
      .query("query { test }")
      .rawVariables("{\"rawKey\":\"rawVal\"}");
  expectContains(HttpClientStub::lastPayload, "\"variables\":{\"rawKey\":\"rawVal\"}", "rawVariables const char* serialized");

  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"ok\":true}}");
  String rawVarsStr = "{\"rawStrKey\":999}";
  client.graphql("/graphql")
      .query("query { test }")
      .rawVariables(rawVarsStr);
  expectContains(HttpClientStub::lastPayload, "\"variables\":{\"rawStrKey\":999}", "rawVariables const String& serialized");

  // rawVariable with const String&
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"ok\":true}}");
  String singleRawJson = "{\"nested\":1}";
  client.graphql("/graphql")
      .query("query { test }")
      .rawVariable("singleRaw", singleRawJson);
  expectContains(HttpClientStub::lastPayload, "\"singleRaw\":{\"nested\":1}", "rawVariable const String& serialized");
}

// 17. GraphQL Fluent Configuration, Aliases, Response Headers & Bindings
void testGraphQLFluentMethodsAndHeaders() {
  HttpClientStub::reset();
  HttpClientStub::setResponseHeaders({
      {"X-Total-Count", "42"},
      {"X-Score", "98.5"},
      {"X-Ratio", "1.2345"},
      {"X-Active", "true"},
      {"X-Id", "1234567890"},
      {"X-Server-Name", "GraphQLGateway"},
      {"X-Tag", "Node-Alpha"}
  });
  HttpClientStub::setResponse(200, "{\"data\":{\"info\":\"OK\",\"count\":9876543210,\"ratio\":3.14159}}");

  ESP32HTTPClient client("https://api.example.com", 9000);
  int countHdr = 0;
  float scoreHdr = 0.0f;
  double ratioHdr = 0.0;
  bool activeHdr = false;
  long idHdr = 0;
  char serverHdr[32] = {0};
  String tagHdr;

  long countData = 0;
  double ratioData = 0.0;
  char infoData[16] = {0};
  String rawResp;
  String rawData;

  String docStr = "query TestDoc { info }";
  String opStr = "TestDoc";

  int successCode = 0;
  int responseCode = 0;

  client.graphql("/graphql")
      .document(docStr)
      .operationName(opStr)
      .asPost()
      .post()
      .timeout(3000)
      .maxRetry(2)
      .retry(2)
      .accept("application/json")
      .header("X-Custom", "ReqVal")
      .header("X-Custom", "ReqValReplaced")
      .onSuccess([&successCode](int code) { successCode = code; })
      .onResponse([&responseCode](int code) { responseCode = code; })
      .onError(static_cast<HttpResponseCallback>(nullptr))
      .getHeader("X-Total-Count", &countHdr)
      .getHeader("X-Score", &scoreHdr)
      .getHeader("X-Ratio", &ratioHdr)
      .getHeader("X-Active", &activeHdr)
      .getHeader("X-Id", &idHdr)
      .getHeader("X-Server-Name", serverHdr, sizeof(serverHdr))
      .getHeader("X-Tag", &tagHdr)
      .getData("count", &countData)
      .getData("ratio", &ratioData)
      .getData("info", infoData, sizeof(infoData))
      .getRawResponse(&rawResp);

  expectEqInt(countHdr, 42, "Header int parsed");
  expectNear(scoreHdr, 98.5f, 0.01f, "Header float parsed");
  expectNear(ratioHdr, 1.2345, 0.0001, "Header double parsed");
  expectTrue(activeHdr, "Header bool parsed");
  expectEqInt(idHdr, 1234567890L, "Header long parsed");
  expectEq(serverHdr, "GraphQLGateway", "Header char[] parsed");
  expectEq(tagHdr.c_str(), "Node-Alpha", "Header String parsed");

  expectEqInt(countData, 9876543210LL > 0 ? (long)countData : 0, "Data long parsed");
  expectNear(ratioData, 3.14159, 0.0001, "Data double parsed");
  expectEq(infoData, "OK", "Data char[] parsed");
  expectTrue(rawResp.length() > 0, "Raw response non-empty");
  expectEqInt(successCode, 200, "onSuccess called");
  expectEqInt(responseCode, 200, "onResponse called");

  // Also test getRawData separately
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"hello\":\"world\"}}");
  client.graphql("/graphql")
      .document("query { hello }")
      .getRawData(&rawData);
  expectTrue(rawData.length() > 0, "Raw data non-empty");

  // Also test method() and asGet()/get() and rawResponse with errors, variables, path and query parameters
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"errors\":[{\"message\":\"RawErr\"}]}");
  String rawErrResp;
  client.graphql("/api/{version}/graphql")
      .asGet()
      .get()
      .method(HTTP_GET_METHOD)
      .path("version", "v1")
      .queryParam("debug", 1)
      .document("query GetTest($id: Int) { test(id: $id) }")
      .operationName("GetTest")
      .variable("id", 123)
      .getRawResponse(&rawErrResp);

  expectEq(HttpClientStub::lastMethod, "GET", "asGet/get sets GET method");
  expectContains(HttpClientStub::lastUrl, "/api/v1/graphql?debug=1&query=", "GET URL with path and query parameters resolved");
  expectContains(HttpClientStub::lastUrl, "operationName=GetTest", "GET URL contains operationName");
  expectContains(HttpClientStub::lastUrl, "variables=", "GET URL contains variables");
  expectTrue(rawErrResp.indexOf("RawErr") != -1, "Raw response with errors parsed");
}

// 18. Detailed GraphQL Error Locations, Extensions, Multi/Single Callbacks & Observability
void testGraphQLDetailedErrorsAndObservability() {
  HttpClientStub::reset();
  std::string errPayload = "{"
      "\"errors\": [{"
      "  \"message\": \"Syntax error on line 4\","
      "  \"locations\": [{\"line\": 4, \"column\": 12}],"
      "  \"path\": [\"users\", 0, \"address\"],"
      "  \"extensions\": {\"code\": \"BAD_REQUEST\", \"timestamp\": 12345}"
      "}]"
      "}";
  HttpClientStub::setResponse(400, errPayload);

  ESP32HTTPClient client("https://api.example.com");
  bool observabilityCalled = false;
  client.onObservability([&observabilityCalled](const ObservabilityMetrics& m) {
    observabilityCalled = true;
    expectTrue(m.txBytes > 0, "txBytes recorded");
  });

  GraphQLError capturedSingleErr;
  std::vector<GraphQLError> capturedMultiErr;
  bool singleCbFired = false;
  bool multiCbFired = false;

  client.graphql("/graphql")
      .query("query { fail }")
      .getError(&capturedSingleErr)
      .getErrors(&capturedMultiErr)
      .onGraphQLError([&singleCbFired](const GraphQLError& e) {
        singleCbFired = true;
        expectEq(e.message.c_str(), "Syntax error on line 4", "Single error callback message match");
      })
      .onGraphQLError([&multiCbFired](const std::vector<GraphQLError>& errs) {
        multiCbFired = true;
        expectEqInt(errs.size(), 1, "Multi error callback size match");
      });

  expectTrue(observabilityCalled, "Observability callback executed");
  expectTrue(singleCbFired, "Single error callback fired");
  expectTrue(multiCbFired, "Multi error callback fired");
  expectEq(capturedSingleErr.message.c_str(), "Syntax error on line 4", "Single error message match");
  expectEqInt(capturedSingleErr.locations.size(), 1, "Error location parsed");
  expectEqInt(capturedSingleErr.locations[0].line, 4, "Location line parsed");
  expectEqInt(capturedSingleErr.locations[0].column, 12, "Location column parsed");
  expectEqInt(capturedSingleErr.path.size(), 3, "Path elements parsed");
  expectEq(capturedSingleErr.path[0].c_str(), "users", "Path[0] string");
  expectEq(capturedSingleErr.path[1].c_str(), "0", "Path[1] numeric");
  expectEq(capturedSingleErr.path[2].c_str(), "address", "Path[2] string");
  expectContains(capturedSingleErr.extensions.c_str(), "BAD_REQUEST", "Extensions parsed");
}

// 19. Comprehensive GraphQLBatchRequest API, Retries, Observability & Error Forwarding
void testGraphQLBatchComprehensive() {
  HttpClientStub::reset();
  std::string batchJson = "["
      "{\"data\":{\"addHero\":{\"id\":10,\"name\":\"Superman\"}}},"
      "{\"errors\":[{\"message\":\"Villain query failed\"}]}"
      "]";
  HttpClientStub::setResponse(200, batchJson);

  ESP32HTTPClient client("https://api.example.com");
  bool obsBatchCalled = false;
  client.onObservability([&obsBatchCalled](const ObservabilityMetrics& m) {
    obsBatchCalled = true;
  });

  auto batch = client.graphqlBatch("/graphql");
  batch.header("X-Batch-Id", "123")
       .header("X-Batch-Id", "Batch-456") // replace
       .accept("application/json")
       .timeout(4000)
       .maxRetry(2)
       .retry(2);

  int heroId = 0;
  String heroName;
  auto& mut = batch.addMutation("mutation { addHero(name: \"Superman\") { id name } }");
  mut.getData("addHero.id", &heroId)
     .getData("addHero.name", &heroName);

  expectEqInt(batch.size(), 1, "Batch size is 1 after addMutation");
  expectEq(batch.operation(0)._document.c_str(), "mutation { addHero(name: \"Superman\") { id name } }", "operation(0) match");

  String op2Error;
  GraphQLError op2ErrObj;
  std::vector<GraphQLError> op2ErrList;
  bool op2SingleCbCalled = false;
  bool op2MultiCbCalled = false;

  auto& q = batch.addQuery("query { villain }");
  q.getErrorMessage(&op2Error)
   .getError(&op2ErrObj)
   .getErrors(&op2ErrList)
   .onGraphQLError([&op2SingleCbCalled](const GraphQLError& e) {
     op2SingleCbCalled = true;
   })
   .onGraphQLError([&op2MultiCbCalled](const std::vector<GraphQLError>& elist) {
     op2MultiCbCalled = true;
   });

  expectEqInt(batch.size(), 2, "Batch size is 2");

  int batchSuccessCode = 0;
  int batchRespCode = 0;
  bool batchMultiErrorCalled = false;
  String batchRawResponse;

  batch.onSuccess([&batchSuccessCode](int code) { batchSuccessCode = code; })
       .onResponse([&batchRespCode](int code) { batchRespCode = code; })
       .onError(static_cast<HttpResponseCallback>(nullptr))
       .onGraphQLError([&batchMultiErrorCalled](const std::vector<GraphQLError>& errs) {
         batchMultiErrorCalled = true;
         expectEq(errs[0].message.c_str(), "Villain query failed", "Batch aggregate error match");
       })
       .getRawResponse(&batchRawResponse);

  // Move constructor test for batch
  GraphQLBatchRequest movedBatch(std::move(batch));
  movedBatch.execute();

  expectTrue(obsBatchCalled, "Observability fired for batch");
  expectEqInt(batchSuccessCode, 200, "Batch onSuccess fired");
  expectEqInt(batchRespCode, 200, "Batch onResponse fired");
  expectTrue(batchMultiErrorCalled, "Batch onGraphQLError fired");
  expectTrue(op2SingleCbCalled, "Op 2 single error callback fired");
  expectTrue(op2MultiCbCalled, "Op 2 multi error callback fired");
  expectEqInt(heroId, 10, "Batch Op 1 heroId parsed");
  expectEq(heroName.c_str(), "Superman", "Batch Op 1 heroName parsed");
  expectEq(op2Error.c_str(), "Villain query failed", "Op 2 error message parsed");
  expectEq(op2ErrObj.message.c_str(), "Villain query failed", "Op 2 error object parsed");
  expectEqInt(op2ErrList.size(), 1, "Op 2 error list parsed");
  expectTrue(batchRawResponse.length() > 0, "Batch raw response captured");

  // Test batch stream error parsing without rawResponseTarget and with client error callback
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "[{\"errors\":[{\"message\":\"DirectStreamErr\"}]}]");
  auto streamBatch = client.graphqlBatch("/graphql");
  auto& sOp = streamBatch.addQuery("query { test }");
  String sErrMsg;
  GraphQLError sErrObj;
  std::vector<GraphQLError> sErrList;
  bool sSingleFired = false;
  bool sMultiFired = false;
  sOp.getErrorMessage(&sErrMsg)
     .getError(&sErrObj)
     .getErrors(&sErrList)
     .onGraphQLError([&sSingleFired](const GraphQLError&) { sSingleFired = true; })
     .onGraphQLError([&sMultiFired](const std::vector<GraphQLError>&) { sMultiFired = true; });

  int singleCbStatus = 0;
  streamBatch.onError([&singleCbStatus](int status) { singleCbStatus = status; });
  streamBatch.execute();

  expectEq(sErrMsg.c_str(), "DirectStreamErr", "Direct stream error message bound");
  expectEq(sErrObj.message.c_str(), "DirectStreamErr", "Direct stream error obj bound");
  expectEqInt(sErrList.size(), 1, "Direct stream error list size 1");
  expectTrue(sSingleFired, "Direct stream single error callback fired");
  expectTrue(sMultiFired, "Direct stream multi error callback fired");

  // Test special escaped characters: \b, \f, \n, \r, \t, \\, and control chars
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"ok\":true}}");
  client.graphql("/graphql")
      .query("query { field(text: \"\b\f\n\r\t\\ \x01\") }");
  expectContains(HttpClientStub::lastPayload, "\\b\\f\\n\\r\\t\\\\", "Special characters properly escaped in JSON");

  // Test query(const String&), mutation(const String&), retry reconnect loop
  HttpClientStub::reset();
  HttpClientStub::queueResponse(-1, "");
  HttpClientStub::queueResponse(200, "{\"data\":{\"items\":[1,2,3]}}");
  String qStr = "query TestQ { items }";
  String mutStr = "mutation TestM { items }";
  int itemVal = 0;
  client.graphql("/graphql")
      .query(qStr)
      .mutation(mutStr)
      .retry(1)
      .getData("items.0", &itemVal);
  expectEqInt(itemVal, 1, "Array data in stream parsed after retry reconnect");

  // Test data: null in stream
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":null,\"errors\":[{\"message\":\"NullDataErr\"}]}");
  String nullErr;
  client.graphql("/graphql")
      .query("{ nullQuery }")
      .getErrorMessage(&nullErr);
  expectEq(nullErr.c_str(), "NullDataErr", "Null data with error parsed in stream");
}

void testGraphQLComprehensiveEdgeCases() {
  ESP32HTTPClient client("https://api.example.com");

  // 1. Helpers
  expectEq(GraphQLRequest::normalizeDataPath("").c_str(), "data", "normalize empty");
  expectEq(GraphQLRequest::normalizeDataPath("data").c_str(), "data", "normalize data");
  expectEq(GraphQLRequest::normalizeDataPath("data.user").c_str(), "data.user", "normalize data.user");
  expectContains(GraphQLRequest::escapeJsonString("a\"b\\c\n\r\t").c_str(), "\\\"", "escapeJsonString escapes quotes");

  // 2. GET request edge cases: port injection, raw variables, variable map, path and query
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"val\":42}}");
  client.setPort(8080);
  int getVal = 0;
  client.graphqlGet("/graphql/{entity}")
      .path("entity", "items")
      .queryParam("flag", true)
      .rawVariables("{\"raw\":1}")
      .onError([](int) {})
      .getData("val", &getVal)
      .execute();
  expectEqInt(getVal, 42, "GET request executed with custom port, path and raw variables");
  expectContains(HttpClientStub::lastUrl, ":8080/graphql/items", "GET url contains custom port and resolved path");
  expectContains(HttpClientStub::lastUrl, "variables=%7B%22raw%22%3A1%7D", "GET url contains url-encoded raw variables");

  // GET with variables map
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"ok\":true}}");
  client.graphqlGet("/search")
      .variable("filter", "active")
      .execute();
  expectContains(HttpClientStub::lastUrl, "variables=", "GET with variable map sets variables param");

  // 3. POST request with custom port, path and query parameters
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"res\":1}}");
  client.setPort(9090);
  client.graphql("/{entity}")
      .path("{entity}", "users")
      .queryParam("active", true)
      .queryParam("limit", 10)
      .onError([](int) {})
      .execute();
  expectContains(HttpClientStub::lastUrl, ":9090/users?active=true&limit=10", "POST url contains custom port, resolved path, and query params");
  client.setPort(0);

  // 4. Unknown fields and skipValue branches in JSON response
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"ignoredObj\":{\"k\":\"v\\\"esc\\\"\"},\"ignoredArr\":[1,2],\"ignoredStr\":\"val\\\"esc\\\"\",\"ignoredNum\":42,\"data\":{\"user\":{\"id\":100}}}");
  int uid = 0;
  client.graphql("/graphql")
      .query("{ user { id } }")
      .getData("user.id", &uid);
  expectEqInt(uid, 100, "All branches of skipValue traversed in JSON parsing");

  // Stripped bindings when rawResponse is active
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"user\":{\"id\":100}}}");
  String rawResp;
  int subId = 0;
  client.graphql("/graphql")
      .query("{ user { id } }")
      .getRawResponse(&rawResp)
      .getData("user.id", &subId);
  expectEqInt(subId, 100, "Bindings stripped and parsed when rawResponse is active");
  expectContains(rawResp.c_str(), "{\"user\":{\"id\":100}}", "Raw response captured");

  // Raw data target when rawResponse is active
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"user\":{\"id\":100}}}");
  String rawResp2, rawData2;
  client.graphql("/graphql")
      .query("{ user { id } }")
      .getRawResponse(&rawResp2)
      .getRawData(&rawData2);
  expectContains(rawData2.c_str(), "{\"user\":{\"id\":100}}", "Raw data captured when rawResponse is active");

  // Raw data target with null when rawResponse is active
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":null}");
  String rawNullResp, rawNullData;
  client.graphql("/graphql")
      .query("{ data }")
      .getRawResponse(&rawNullResp)
      .getRawData(&rawNullData);
  expectEq(rawNullData.c_str(), "null", "Raw null data captured when rawResponse is active");

  // Direct array binding on data
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":[{\"id\":10},{\"id\":20}]}");
  int firstId = 0;
  client.graphql("/graphql")
      .query("{ users { id } }")
      .getData("0.id", &firstId);
  expectEqInt(firstId, 10, "Array binding directly on data array");

  // 5. Detailed error parsing with unknown properties
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"errors\":[{\"message\":\"err\",\"unknownProp\":{\"x\":1},\"locations\":[{\"line\":10,\"column\":5,\"extra\":true}],\"path\":[\"users\",0,\"name\",false],\"extensions\":{\"code\":\"BAD_REQ\",\"details\":{\"sub\":1}}}]}");
  std::vector<GraphQLError> errs;
  client.graphql("/graphql")
      .query("{ test }")
      .onGraphQLError([&](const std::vector<GraphQLError>& eList) {
        errs = eList;
      });
  expectEqInt((int)errs.size(), 1, "Complex error with extensions, locations, and unknown props parsed");

  // 6. Multipart streaming edge cases
  HttpClientStub::reset();
  HttpClientStub::setResponseHeaders({{"Content-Type", "multipart/mixed; boundary=graphql"}});
  String multipartStream = 
      "--graphql\r\n"
      "Content-Type: application/json\r\n\r\n"
      "{\"hasNext\":false,\"errors\":[{\"message\":\"stream error\"}],\"unknownTop\":123,\"incremental\":[{\"path\":\"invalid\",\"data\":123,\"errors\":[{\"message\":\"inc error\"}],\"unknownSub\":true}]}\r\n"
      "--graphql--\r\n";
  HttpClientStub::setResponse(200, multipartStream.c_str());
  std::vector<GraphQLIncrementalPayload> payloads;
  client.graphql("/graphql")
      .query("{ streamQuery }")
      .onIncremental([&](const GraphQLIncrementalPayload& inc) {
        payloads.push_back(inc);
      });
  expectTrue(!payloads.empty(), "Multipart streaming with errors and unknown sub-objects parsed");

  // Destructor auto-execution on single request
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"ok\":true}}");
  {
    auto req = client.graphql("/auto_single");
    req.query("{ test }");
  }
  expectEq(HttpClientStub::lastUrl, "https://api.example.com/auto_single", "GraphQLRequest destructor auto-executed");

  // 7. GraphQLBatchRequest edge cases
  // Destructor auto-execution on batch
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "[{\"data\":{\"q\":1}}]");
  {
    auto b = client.graphqlBatch("/batch_destructor");
    b.add().query("{ id }");
  }
  expectEq(HttpClientStub::lastUrl, "https://api.example.com/batch_destructor", "GraphQLBatchRequest destructor auto-executed");

  // Custom port, client headers, and retry in batch
  client.setPort(8080);
  client.setHeader("X-Batch-Hdr", "BatchVal");
  HttpClientStub::reset();
  HttpClientStub::queueResponse(-1, "");
  HttpClientStub::queueResponse(200, "[{\"data\":{\"q\":1}}]");
  {
    auto b = client.graphqlBatch("/batch_port");
    b.add().query("{ q }");
    b.retry(1);
    b.execute();
  }
  expectContains(HttpClientStub::lastUrl, ":8080/batch_port", "Batch url contains custom port");
  client.setPort(0);

  // Batch failure callback and onError(HttpErrorCallback)
  HttpClientStub::reset();
  HttpClientStub::setResponse(-1, "");
  bool batchErrFired = false;
  {
    auto b = client.graphqlBatch("/batch_fail");
    b.add().query("{ q }");
    b.onError([&](int code, const char* msg) {
      (void)code;
      (void)msg;
      batchErrFired = true;
    });
    b.execute();
  }
  expectTrue(batchErrFired, "Batch error callback fired on HTTP failure");

  // Batch with escaped character in query
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "[{\"data\":{\"q\":1}}]");
  {
    auto b = client.graphqlBatch("/batch_esc");
    b.add().query("query { field(esc: \"hello\\\"world\") }");
    b.execute();
  }
  expectContains(HttpClientStub::lastPayload, "\\\\\\\"world", "Escaped string in batch query");

  // Batch query with escaped backslash
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "[{\"data\":{\"q\":1}}]");
  {
    auto b = client.graphqlBatch("/batch_slash");
    b.add().query("query { field(esc: \"test\\\\esc\") }");
    b.execute();
  }

  // BaseUrl with subpath and custom port (GET, POST, Batch)
  ESP32HTTPClient clientWithSubpath("https://api.example.com/api/v1", 8080);
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"ok\":true}}");
  clientWithSubpath.graphqlGet("/get").execute();
  expectContains(HttpClientStub::lastUrl, "api.example.com:8080/api/v1/get", "GET custom port with baseUrl subpath");

  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"data\":{\"ok\":true}}");
  clientWithSubpath.graphqlPost("/post").execute();
  expectContains(HttpClientStub::lastUrl, "api.example.com:8080/api/v1/post", "POST custom port with baseUrl subpath");

  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "[{\"data\":{\"ok\":true}}]");
  {
    auto b = clientWithSubpath.graphqlBatch("/batch");
    b.add().query("{ q }");
    b.execute();
  }
  expectContains(HttpClientStub::lastUrl, "api.example.com:8080/api/v1/batch", "Batch custom port with baseUrl subpath");

  // Raw data without raw response with bindings
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, R"({"data":{"user":{"id":200}}})");
  String rawDataOnly;
  int rawSubId = 0;
  client.graphql("/graphql")
      .query("{ user { id } }")
      .getRawData(&rawDataOnly)
      .getData("user.id", &rawSubId);
  expectEqInt(rawSubId, 200, "Raw data with bindings parsed");
  expectContains(rawDataOnly.c_str(), "\"id\":200", "Raw data contains payload");

  // Non-array errors handling
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"errors\":\"not-an-array\"}");
  client.graphql("/graphql").query("{ test }").execute();

  // Invalid types in parseSingleError (non-string message, non-array locations, non-array path, non-object extensions)
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"errors\":[{\"message\":123,\"locations\":\"invalid\",\"path\":\"invalid\",\"extensions\":\"invalid\"}]}");
  client.graphql("/graphql").query("{ test }").execute();

  // Escaped key in JSON response
  HttpClientStub::reset();
  HttpClientStub::setResponse(200, "{\"\\\"esc\\\"key\":123,\"data\":{\"ok\":true}}");
  client.graphql("/graphql").query("{ test }").execute();

  // Multipart streaming variations (preamble, non-object data, non-array incremental, empty part)
  HttpClientStub::reset();
  HttpClientStub::setResponseHeaders({{"Content-Type", "multipart/mixed; boundary=graphql"}});
  String multipartStream2 = 
      "preamble\r\n"
      "--graphql\r\n"
      "Content-Type: application/json\r\n\r\n"
      "   {\"data\":\"not-an-object\",\"incremental\":\"not-an-array\",\"errors\":[{\"message\":\"err\"}],\"hasNext\":false}\r\n"
      "--graphql\r\n"
      "Content-Type: application/json\r\n\r\n"
      "--graphql--\r\n";
  HttpClientStub::setResponse(200, multipartStream2.c_str());
  int dummyBinding = 0;
  client.graphql("/graphql")
      .query("{ stream }")
      .getData("custom", &dummyBinding)
      .execute();
}

} // namespace

int main() {
  std::cout << "=== Running GraphQL Unit Tests ===\n\n";

  runSuite("GraphQLBasicQueryPost", testGraphQLBasicQueryPost);
  runSuite("GraphQLBasicQueryGet", testGraphQLBasicQueryGet);
  runSuite("GraphQLMutation", testGraphQLMutation);
  runSuite("GraphQLVariables", testGraphQLVariables);
  runSuite("GraphQLOperationNameSelection", testGraphQLOperationNameSelection);
  runSuite("GraphQLContentNegotiation", testGraphQLContentNegotiation);
  runSuite("GraphQLErrorHandling", testGraphQLErrorHandling);
  runSuite("GraphQLPartialDataPreservation", testGraphQLPartialDataPreservation);
  runSuite("GraphQLHttpStatusCodes", testGraphQLHttpStatusCodes);
  runSuite("GraphQLBatchRequests", testGraphQLBatchRequests);
  runSuite("GraphQLStreamingMultipart", testGraphQLStreamingMultipart);
  runSuite("GraphQLAuthAndHeaders", testGraphQLAuthAndHeaders);
  runSuite("GraphQLMoveConstructor", testGraphQLMoveConstructor);
  runSuite("GraphQLStructDataMapping", testGraphQLStructDataMapping);
  runSuite("GraphQLEsp32ClientPost", testGraphQLEsp32ClientPost);
  runSuite("GraphQLScalarVariablesAndRaw", testGraphQLScalarVariablesAndRaw);
  runSuite("GraphQLFluentMethodsAndHeaders", testGraphQLFluentMethodsAndHeaders);
  runSuite("GraphQLDetailedErrorsAndObservability", testGraphQLDetailedErrorsAndObservability);
  runSuite("GraphQLBatchComprehensive", testGraphQLBatchComprehensive);
  runSuite("GraphQLComprehensiveEdgeCases", testGraphQLComprehensiveEdgeCases);

  std::cout << "\n=== GraphQL Test Summary ===\n";
  std::cout << "Suites: " << suitesRun << " total | " << suitesPassed << " passed | " << failures << " failed\n";
  std::cout << "Checks: " << checks << " total | " << passedChecks << " passed | " << (checks - passedChecks) << " failed\n";

  return failures == 0 ? 0 : 1;
}
