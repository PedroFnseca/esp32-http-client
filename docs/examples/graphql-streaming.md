---
title: Incremental Delivery & Streaming (@defer) - ESP32-HTTP-Client
description: How to consume GraphQL responses with @defer and multipart/mixed streaming directly on ESP32 without buffering entire payloads.
keywords: GraphQL defer ESP32, multipart mixed streaming ESP32, incremental GraphQL
tags:
  - examples
  - graphql
  - streaming
---
# Incremental Delivery and Streaming (`@defer`)

Modern GraphQL servers can stream slow or expensive fields incrementally using the `@defer` directive over a `multipart/mixed` HTTP connection.

`ESP32-HTTP-Client` processes each chunk directly from the incoming network stream using `BufferedStreamReader`, so memory usage remains minimal even for large responses.

---

## Example with `@defer`

```cpp
const char* queryWithDefer =
    "query GetDashboard {\n"
    "  status\n"
    "  ... @defer {\n"
    "    heavyLogData { timestamp message }\n"
    "  }\n"
    "}";

String initialStatus;

client.graphql("/graphql")
    .query(queryWithDefer)
    .getData("status", &initialStatus)
    .onIncremental([](const GraphQLIncrementalPayload& payload) {
      Serial.printf("Chunk received for path '%s' (hasNext: %d)\n",
                    payload.path.c_str(),
                    payload.hasNext);
      Serial.printf("Data: %s\n", payload.data.c_str());
    });
```
