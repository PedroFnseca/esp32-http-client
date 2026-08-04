---
tags:
  - example
  - crud
  - get
  - post
  - put
  - patch
  - delete
  - http
---
# CRUD Operations

Demonstrates the core HTTP REST methods (`GET`, `POST`, `PUT`, `PATCH`, `DELETE`) in a single consolidated sketch.

**Source:** [`examples/RestCrud/RestCrud.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/RestCrud/RestCrud.ino)

---

## Overview

| Method | Function | Description |
| :--- | :--- | :--- |
| `GET` | `client.get(path)` | Fetches and binds data from remote resources. |
| `POST` | `client.post(path)` | Sends a JSON payload to create a new resource. |
| `PUT` | `client.put(path)` / `client.update(path)` | Replaces a resource with an updated payload. |
| `PATCH` | `client.patch(path)` | Partially updates fields on an existing resource. |
| `DELETE` | `client.del(path)` | Removes a remote resource. |

---

## Full Sketch

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://jsonplaceholder.typicode.com");

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
    // 1. GET: Read resource
    Serial.println("\n--- [GET] Reading Todo #1 ---");
    int id = 0;
    int userId = 0;
    char title[64] = {0};
    bool completed = false;

    client.get("/todos/1")
          .getBody("id", &id)
          .getBody("userId", &userId)
          .getBody("title", title, sizeof(title))
          .getBody("completed", &completed);

    if (client.isSuccess()) {
        Serial.printf("GET 200 OK -> ID: %d, User: %d, Title: %s, Completed: %s\n",
                      id, userId, title, completed ? "true" : "false");
    }

    // 2. POST: Create resource
    Serial.println("\n--- [POST] Creating Post ---");
    int newId = 0;
    client.post("/posts")
          .body("title",  "ESP32 REST Client")
          .body("body",   "Fluent embedded HTTP client")
          .body("userId", 1)
          .getBody("id", &newId);

    if (client.isSuccess()) {
        Serial.printf("POST Created -> New ID: %d\n", newId);
    }

    // 3. PUT: Full resource update
    Serial.println("\n--- [PUT] Updating Post #1 ---");
    client.put("/posts/1")
          .body("id",     1)
          .body("title",  "Updated Title")
          .body("body",   "Updated Content")
          .body("userId", 1);

    if (client.isSuccess()) {
        Serial.println("PUT Updated -> Resource updated successfully");
    }

    // 4. PATCH: Partial update
    Serial.println("\n--- [PATCH] Partial update Post #1 ---");
    client.patch("/posts/1")
          .body("title", "Patched Title Only");

    if (client.isSuccess()) {
        Serial.println("PATCH Success -> Title field modified");
    }

    // 5. DELETE: Remove resource
    Serial.println("\n--- [DELETE] Deleting Post #1 ---");
    client.del("/posts/1");

    if (client.isSuccess()) {
        Serial.println("DELETE Success -> Resource deleted");
    }

    delay(15000);
}
```

---

## Expected Serial Output

```
Connected to WiFi

--- [GET] Reading Todo #1 ---
GET 200 OK -> ID: 1, User: 1, Title: delectus aut autem, Completed: false

--- [POST] Creating Post ---
POST Created -> New ID: 101

--- [PUT] Updating Post #1 ---
PUT Updated -> Resource updated successfully

--- [PATCH] Partial update Post #1 ---
PATCH Success -> Title field modified

--- [DELETE] Deleting Post #1 ---
DELETE Success -> Resource deleted
```

---

## Key Takeaways

- All HTTP methods return a fluent [`RestRequest`](../api/restrequest.md) that is executed at the end of the statement or when `.getBody()` is called.
- `client.isSuccess()` conveniently checks if the response code is within `200..299`.
- `.body(key, value)` can be chained multiple times to serialize JSON request bodies directly without allocating JSON documents.
