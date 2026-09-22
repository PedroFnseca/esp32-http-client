---
title: GraphQL Queries and Mutations - ESP32-HTTP-Client
description: How to perform GraphQL queries and mutations over HTTP and HTTPS on ESP32 with zero heap allocations for payload buffers.
keywords: GraphQL ESP32 query, GraphQL ESP32 mutation, GraphQL GET POST
tags:
  - examples
  - graphql
  - queries
  - mutations
---
# GraphQL Queries and Mutations

`ESP32-HTTP-Client` provides a dedicated, fluent API for GraphQL operations with direct stream-to-variable binding and zero-heap overhead.

---

## Basic Query (POST)

```cpp
#include <WiFi.h>
#include "ESP32HTTPClient.h"

ESP32HTTPClient client("https://api.example.com");

String name;
int id;

void setup() {
  Serial.begin(115200);

  client.graphql("/graphql")
      .query("query { user { id name } }")
      .getData("user.id", &id)
      .getData("user.name", &name);

  if (client.isSuccess()) {
    Serial.printf("User: %s (ID: %d)\n", name.c_str(), id);
  }
}
```

---

## Query via HTTP GET

According to the GraphQL over HTTP specification, read-only queries can be executed via HTTP GET:

```cpp
String status;

client.graphqlGet("/graphql")
    .query("{ systemHealth }")
    .getData("systemHealth", &status);
```

---

## GraphQL Mutations (POST)

Mutations modify server-side data and are executed via HTTP POST:

```cpp
int deviceId = 0;
String status;

client.graphql("/graphql")
    .mutation("mutation Register { registerNode(type: \"ESP32\") { id status } }")
    .getData("registerNode.id", &deviceId)
    .getData("registerNode.status", &status);
```
