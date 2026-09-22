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

  std::cout << "\n=== GraphQL Test Summary ===\n";
  std::cout << "Suites: " << suitesRun << " total | " << suitesPassed << " passed | " << failures << " failed\n";
  std::cout << "Checks: " << checks << " total | " << passedChecks << " passed | " << (checks - passedChecks) << " failed\n";

  return failures == 0 ? 0 : 1;
}
