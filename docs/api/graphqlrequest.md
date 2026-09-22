---
title: GraphQLRequest Class Reference - Fluent GraphQL Client for ESP32
description: API reference for GraphQLRequest class: method chaining for GraphQL queries, mutations, variables, operation names, partial data preservation, and error handling.
keywords: GraphQLRequest class ESP32, GraphQL client ESP32, GraphQL HTTP, GraphQL variables, GraphQL errors, GraphQL streaming
tags:
  - api
  - graphql
  - request
  - class
---
# GraphQLRequest

The fluent request builder returned by `.graphql(path)`, `.graphqlGet(path)`, and `.graphqlPost(path)` on [`ESP32HTTPClient`](esp32httpclient.md). All builder methods return `GraphQLRequest&`, enabling fluent chaining.

**The GraphQL request is dispatched automatically** when the `GraphQLRequest` object goes out of scope (at the end of the statement) or when `.execute()` is called explicitly.

!!! note "Copy semantics"
    `GraphQLRequest` is **move-only** — it cannot be copied. It is designed to be used in a single chained expression.

---

## Method Chaining Overview

```cpp
client.graphql("/graphql")
    .query("query GetUser($id: ID!) { user(id: $id) { name email } }")
    .operationName("GetUser")
    .variable("id", "usr_100")
    .getData("user.name", &name)
    .getData("user.email", &email);
```

---

## Building Operations

### `query(doc)` / `document(doc)`
Sets the GraphQL query document string. Chainable.
```cpp
GraphQLRequest& query(const char* queryDocument);
GraphQLRequest& query(const String& queryDocument);
GraphQLRequest& document(const char* document);
GraphQLRequest& document(const String& document);
```

### `mutation(doc)`
Sets the GraphQL mutation document string and sets the HTTP method to POST. Chainable.
```cpp
GraphQLRequest& mutation(const char* mutationDocument);
GraphQLRequest& mutation(const String& mutationDocument);
```

### `operationName(name)`
Sets the operation name to execute when a document contains multiple operations. Chainable.
```cpp
GraphQLRequest& operationName(const char* name);
GraphQLRequest& operationName(const String& name);
```

---

## HTTP Transport and Methods

### `asGet()` / `get()`
Configures the request to execute as HTTP GET. Query, operation name, and variables will be formatted as URL-encoded query parameters according to the GraphQL over HTTP specification.
```cpp
GraphQLRequest& asGet();
GraphQLRequest& get();
```

### `asPost()` / `post()`
Configures the request to execute as HTTP POST (default). The request body is constructed as JSON:
`{"query": "...", "operationName": "...", "variables": {...}}`.
```cpp
GraphQLRequest& asPost();
GraphQLRequest& post();
```

---

## Variables

Supports scalar types, Arduino `String`, custom JSON strings, and C++ structs decorated with `REST_JSON_MAP`:

```cpp
client.graphql("/graphql")
    .query("query Q($id: Int!, $active: Boolean!, $name: String!) { ... }")
    .variable("id", 10)
    .variable("active", true)
    .variable("name", "Alice");
```

Struct variable example:
```cpp
struct Settings {
  int brightness;
  bool nightMode;
  REST_JSON_MAP(
    REST_FIELD(brightness),
    REST_FIELD(nightMode)
  )
};

Settings cfg{80, true};
client.graphql("/graphql")
    .mutation("mutation SetConfig($cfg: ConfigInput!) { updateConfig(input: $cfg) { ok } }")
    .variable("cfg", cfg);
```

Raw variables string:
```cpp
client.graphql("/graphql").rawVariables("{\"id\": 123, \"active\": true}");
```

---

## Data Bindings

Binds JSON fields directly into local variables. Paths are relative to the `"data"` field in the response (e.g. `"user.name"` maps to `"data.user.name"`).

```cpp
int id;
String name;
bool active;
float rating;

client.graphql("/graphql")
    .query("{ user { id name active rating } }")
    .getData("user.id", &id)
    .getData("user.name", &name)
    .getData("user.active", &active)
    .getData("user.rating", &rating);
```

Capturing raw data:
```cpp
String rawData;
client.graphql("/graphql").query("{ user { id name } }").getRawData(&rawData);
```

Struct mapping:
```cpp
struct User {
  int id;
  String name;
  REST_JSON_MAP(REST_FIELD(id), REST_FIELD(name))
};

User user;
client.graphql("/graphql").query("{ user { id name } }").getData("user", &user);
```

---

## Error Handling and Partial Data

GraphQL responses may contain both `"data"` and `"errors"`. `GraphQLRequest` parses errors into `GraphQLError` structures while **preserving all resolved partial data bindings**.

```cpp
GraphQLError error;
std::vector<GraphQLError> allErrors;
String errorMsg;

client.graphql("/graphql")
    .query("{ user { id name restrictedField } }")
    .getData("user.id", &id)
    .getData("user.name", &name)
    .getError(&error)
    .getErrors(&allErrors)
    .getErrorMessage(&errorMsg)
    .onGraphQLError([](const std::vector<GraphQLError>& errors) {
      for (const auto& err : errors) {
        Serial.printf("Error: %s\n", err.message.c_str());
      }
    });
```

### `GraphQLError` Structure
```cpp
struct GraphQLLocation {
  int line;
  int column;
};

struct GraphQLError {
  String message;
  std::vector<GraphQLLocation> locations;
  std::vector<String> path;
  String extensions;
};
```

---

## Streaming and Incremental Delivery (`@defer` / `multipart/mixed`)

When consuming endpoints supporting `@defer` or `@stream`, the response is parsed as a `multipart/mixed` stream directly from the network buffer without buffering the whole response in heap:

```cpp
client.graphql("/graphql")
    .query("query { user { name ... @defer { bio } } }")
    .getData("user.name", &initialName)
    .onIncremental([](const GraphQLIncrementalPayload& payload) {
      Serial.printf("Received chunk for path: %s (hasNext: %d)\n",
                    payload.path.c_str(), payload.hasNext);
    });
```
