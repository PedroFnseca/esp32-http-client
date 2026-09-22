---
title: GraphQL Error Handling and Partial Responses - ESP32-HTTP-Client
description: How to handle GraphQL errors, inspect messages, locations, paths, and extensions while preserving partial data.
keywords: GraphQL errors ESP32, partial data GraphQL, GraphQLError
tags:
  - examples
  - graphql
  - errors
---
# GraphQL Error Handling and Partial Responses

In GraphQL, field errors do not necessarily cancel the entire request. Servers frequently return both `"data"` (containing resolved fields) and `"errors"` in the same response.

`ESP32-HTTP-Client` **preserves all successfully resolved data bindings** while making error objects available for inspection.

---

## Accessing Errors and Partial Data

```cpp
int userId = 0;
String userName;
GraphQLError error;

client.graphql("/graphql")
    .query("query { user { id name restrictedProfile } }")
    .getData("user.id", &userId)
    .getData("user.name", &userName)
    .getError(&error)
    .onGraphQLError([](const std::vector<GraphQLError>& errors) {
      for (const auto& err : errors) {
        Serial.printf("Error: %s\n", err.message.c_str());
        for (const auto& loc : err.locations) {
          Serial.printf("  at line %d, col %d\n", loc.line, loc.column);
        }
        if (!err.extensions.isEmpty()) {
          Serial.printf("  extensions: %s\n", err.extensions.c_str());
        }
      }
    });

// Partial data is retained and populated!
if (userId > 0) {
  Serial.printf("User ID: %d, Name: %s\n", userId, userName.c_str());
}
```

---

## HTTP Status Code Handling

Under the GraphQL over HTTP specification, validation or syntax errors return HTTP 400 with a GraphQL response body containing `"errors"`. `GraphQLRequest` parses errors regardless of 4xx/5xx status codes:

```cpp
String errorMessage;

client.graphql("/graphql")
    .query("query { syntaxError ")
    .getErrorMessage(&errorMessage);

if (!errorMessage.isEmpty()) {
  Serial.printf("Parsed error: %s\n", errorMessage.c_str());
}
```
