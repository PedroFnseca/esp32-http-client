---
title: ESP32 Struct to JSON Serialization & Deserialization Example
description: Learn how to map C++ structs directly to JSON payloads and parse HTTP responses into struct fields on ESP32 without heap allocation.
keywords: ESP32 struct JSON mapping, C++ struct serialization ESP32, ESP32 JSON deserialization, Arduino struct HTTP payload
tags:
  - example
  - struct
  - json
  - serialization
  - deserialization
---
# Struct <-> JSON Mapping

Demonstrates bi-directional serialization and deserialization between C++ `struct`s and JSON payloads with zero dynamic document allocations.

**Source:** [`examples/StructJson/StructJson.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/StructJson/StructJson.ino)

---

## Overview

By adding the `REST_JSON_MAP` macro inside your struct definition, you can:
1. **Send structs directly in request bodies:** `client.post("/todos").body(newTodo);`
2. **Populate structs directly from HTTP responses:** `client.get("/todos/1").getBody(&todo);`
3. **Convert outside HTTP requests:** `ESP32HTTPClient::toJson(obj)` and `ESP32HTTPClient::fromJson(json, &obj)`.

---

## Full Sketch

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://jsonplaceholder.typicode.com");

// Declare mapped struct
struct Todo {
    int id = 0;
    int userId = 0;
    char title[64] = {0};
    bool completed = false;

    REST_JSON_MAP(
        REST_FIELD(id),
        REST_FIELD(userId),
        REST_FIELD(title),
        REST_FIELD(completed)
    )
};

void setup() {
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
}

void loop() {
    Todo todo;

    // 1. Fetching remote JSON directly into a struct
    Serial.println("\n--- [1] Fetching Todo as Struct ---");
    client.get("/todos/1").getBody(&todo);

    if (client.isSuccess()) {
        Serial.printf("Struct Populated -> ID: %d, User: %d, Title: %s, Completed: %s\n",
                      todo.id, todo.userId, todo.title, todo.completed ? "true" : "false");
    }

    // 2. Sending a struct as JSON request body
    Serial.println("\n--- [2] Posting Todo from Struct ---");
    Todo newTodo;
    newTodo.userId = 1;
    newTodo.id = 101;
    strncpy(newTodo.title, "Build awesome IoT device", sizeof(newTodo.title));
    newTodo.completed = false;

    Todo responseTodo;
    client.post("/todos")
          .body(newTodo)
          .getBody(&responseTodo);

    if (client.isSuccess()) {
        Serial.printf("Struct Posted -> Created Resource ID: %d\n", responseTodo.id);
    }

    // 3. Standalone JSON conversion
    Serial.println("\n--- [3] Standalone Struct to JSON String ---");
    String jsonString = ESP32HTTPClient::toJson(newTodo);
    Serial.printf("Serialized JSON: %s\n", jsonString.c_str());

    delay(15000);
}
```

---

## Expected Serial Output

```
Connected to WiFi

--- [1] Fetching Todo as Struct ---
Struct Populated -> ID: 1, User: 1, Title: delectus aut autem, Completed: false

--- [2] Posting Todo from Struct ---
Struct Posted -> Created Resource ID: 101

--- [3] Standalone Struct to JSON String ---
Serialized JSON: {"id":101,"userId":1,"title":"Build awesome IoT device","completed":false}
```

---

## Key Takeaways

- Missing fields in JSON responses leave the struct member default values intact.
- JSON `null` values are automatically handled without crashes or corruption.
- Extra unknown fields in JSON are safely ignored.
