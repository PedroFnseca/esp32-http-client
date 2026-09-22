---
title: GraphQL Batching - ESP32-HTTP-Client
description: How to execute batched GraphQL queries in a single HTTP request to save radio time and power on ESP32.
keywords: GraphQL batch ESP32, batching GraphQL, multiple operations ESP32
tags:
  - examples
  - graphql
  - batch
---
# GraphQL Batching

Sending multiple operations in one HTTP request saves WiFi transmission time, TLS handshake overhead, and battery on ESP32 devices.

---

## Batching Multiple Queries

```cpp
auto batch = client.graphqlBatch("/graphql");

String deviceName;
int notificationCount = 0;
String weatherSummary;

// Operation 1:
auto& op1 = batch.addQuery("query { device { name } }");
op1.getData("device.name", &deviceName);

// Operation 2:
auto& op2 = batch.addQuery("query { unreadCount }");
op2.getData("unreadCount", &notificationCount);

// Operation 3:
auto& op3 = batch.addQuery("query { weather(city: \"Lisbon\") { summary } }");
op3.getData("weather.summary", &weatherSummary);

// Dispatched automatically or explicitly:
batch.execute();

if (client.isSuccess()) {
  Serial.printf("Device: %s\n", deviceName.c_str());
  Serial.printf("Notifications: %d\n", notificationCount);
  Serial.printf("Weather: %s\n", weatherSummary.c_str());
}
```
