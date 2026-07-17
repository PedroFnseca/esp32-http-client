---
tags:
  - example
  - put
  - delete
  - http
---
# PUT & DELETE

Demonstrates how to update and delete remote resources using `PUT` and `DELETE` methods.

**Sources:**
- [`examples/PutRequest/PutRequest.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/PutRequest/PutRequest.ino)
- [`examples/DeleteRequest/DeleteRequest.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/DeleteRequest/DeleteRequest.ino)

---

## PUT — Update a Resource

The `put()` and its alias `update()` send an HTTP `PUT` request. The body is constructed the same way as `POST`, using `.body()` calls.

```cpp
// Update post #1 with a new title and body
client.put("/posts/1")
      .body("id", 1)
      .body("title", "updated title")
      .body("body",  "updated body")
      .body("userId", 1);

if (client.getStatusCode() == 200) {
    Serial.println("Resource updated successfully.");
}
```

### Using the `update()` alias

`update()` is a semantic alias for `put()` — both produce the exact same HTTP `PUT` request:

```cpp
// These two are identical
client.put("/lights/1").body("state", "OFF");
client.update("/lights/1").body("state", "OFF");
```

---

## PATCH — Partial Update

Use `patch()` for partial resource updates (only the fields you send are changed):

```cpp
client.patch("/posts/1")
      .body("title", "new title only");
```

---

## DELETE — Remove a Resource

The `del()` method sends an HTTP `DELETE` request. It can be used without a body:

```cpp
client.del("/posts/1");

if (client.getStatusCode() == 200) {
    Serial.println("Resource deleted.");
}
```

DELETE can optionally include a body if the API requires it:

```cpp
client.del("/sessions")
      .body("userId", 42);
```

---

## Full Sketch — PUT then DELETE

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
    while (WiFi.status() != WL_CONNECTED) delay(500);
    Serial.println("\nWiFi Connected");
}

void loop() {
    // Update
    client.put("/posts/1")
          .body("id",     1)
          .body("title",  "updated title")
          .body("body",   "updated content")
          .body("userId", 1);

    Serial.printf("PUT status : %d\n", client.getStatusCode());

    // Delete
    client.del("/posts/1");
    Serial.printf("DELETE status: %d\n", client.getStatusCode());

    delay(30000);
}
```

---

## Expected Serial Output

```
WiFi Connected
PUT status : 200
DELETE status: 200
```
