---
title: GraphQL Variables and Operation Selection - ESP32-HTTP-Client
description: Using variables with primitive types, structs, and selecting operations from multi-operation documents.
keywords: GraphQL variables ESP32, operationName GraphQL, struct variables ESP32
tags:
  - examples
  - graphql
  - variables
---
# GraphQL Variables & Operation Selection

---

## Passing Scalar Variables

Variables avoid string concatenation and guard against injection issues:

```cpp
String userName;
bool active = false;

client.graphql("/graphql")
    .query("query GetUser($id: ID!, $online: Boolean) { user(id: $id, online: $online) { name } }")
    .variable("id", "usr_9912")
    .variable("online", true)
    .getData("user.name", &userName);
```

---

## Struct Variables with `REST_JSON_MAP`

Custom C++ structs can be passed as variables directly:

```cpp
struct SensorTelemetry {
  float temp;
  float pressure;
  int battery;

  REST_JSON_MAP(
    REST_FIELD(temp),
    REST_FIELD(pressure),
    REST_FIELD(battery)
  )
};

SensorTelemetry data{24.2f, 1013.25f, 98};

client.graphql("/graphql")
    .mutation("mutation Log($input: TelemetryInput!) { record(data: $input) { id } }")
    .variable("input", data);
```

---

## Multi-Operation Documents and `operationName`

When a single query document contains multiple operations, select which one to execute:

```cpp
const char* doc =
    "query GetProfile { profile { name } }\n"
    "query GetConfig { config { refreshRate } }";

int rate = 0;

client.graphql("/graphql")
    .document(doc)
    .operationName("GetConfig")
    .getData("config.refreshRate", &rate);
```
