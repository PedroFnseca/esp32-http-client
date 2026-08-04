---
tags:
  - example
  - callbacks
  - timeout
  - retry
  - errors
---
# Callbacks & Error Handling

Demonstrates global and per-request callbacks (`onSuccess`, `onError`, `onResponse`), configuring timeouts, automatic retries on network failures, runtime URL updates, and checking error status codes.

**Source:** [`examples/CallbacksAndErrors/CallbacksAndErrors.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/CallbacksAndErrors/CallbacksAndErrors.ino)

---

## Overview

| Feature | Syntax | Description |
| :--- | :--- | :--- |
| **Success Callback** | `.onSuccess([](int code){ ... })` | Executed when request returns HTTP 2xx. |
| **Error Callback** | `.onError([](int code, const char* msg){ ... })` | Executed when request fails (`code < 200 \|\| code >= 400`). |
| **Response Callback** | `.onResponse([](int code){ ... })` | Executed upon request completion regardless of outcome. |
| **Timeout** | `.timeout(3000)` / `client.setTimeout(5000)` | Configures network timeout in milliseconds. |
| **Max Retry** | `.retry(2)` / `client.setMaxRetry(3)` | Number of automatic retries upon network failure. |
| **Runtime URL** | `client.setUrl("https://api.v2.com")` | Dynamically redirects client to a new host URL. |
| **Status Check** | `client.isSuccess()`, `client.hasError()` | Boolean helpers for inspecting response state. |

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

    // Global Callbacks on the client instance
    client.onSuccess([](int statusCode) {
        Serial.printf("[Global Callback] Success with HTTP %d\n", statusCode);
    });

    client.onError([](int errorCode, const char* message) {
        Serial.printf("[Global Callback] Error (%d): %s\n", errorCode, message);
    });

    client.onResponse([](int statusCode) {
        Serial.printf("[Global Callback] Request complete, status: %d\n", statusCode);
    });
}

void loop() {
    // 1. Per-request callbacks, custom timeout and retry configuration
    Serial.println("\n--- [1] Request with Callbacks and Timeout/Retry ---");
    int id = 0;

    client.get("/todos/1")
          .timeout(3000)   // 3 second timeout for this request
          .retry(2)        // Retry up to 2 times on network failure
          .onSuccess([](int code) {
              Serial.printf("[Request Callback] Todo fetched successfully (HTTP %d)\n", code);
          })
          .onError([](int code, const char* message) {
              Serial.printf("[Request Callback] Failed (%d): %s\n", code, message);
          })
          .getBody("id", &id);

    // 2. Error handling checks
    Serial.println("\n--- [2] Handling Error (404 Not Found) ---");
    client.get("/non_existent_endpoint");

    if (client.hasError()) {
        Serial.printf("Detected Error: HTTP %d (%s)\n",
                      client.getStatusCode(),
                      client.getErrorMessage().c_str());
    }

    // 3. Changing target URL at runtime
    Serial.println("\n--- [3] Changing URL at runtime ---");
    client.setUrl("https://httpbin.org");
    client.get("/status/200");

    if (client.isSuccess()) {
        Serial.printf("Switched to %s successfully (HTTP %d)\n",
                      client.getBaseUrl(),
                      client.getStatusCode());
    }

    // Reset back for next loop iteration
    client.setUrl("https://jsonplaceholder.typicode.com");

    delay(15000);
}
```

---

## Expected Serial Output

```
Connected to WiFi

--- [1] Request with Callbacks and Timeout/Retry ---
[Global Callback] Success with HTTP 200
[Request Callback] Todo fetched successfully (HTTP 200)
[Global Callback] Request complete, status: 200

--- [2] Handling Error (404 Not Found) ---
[Global Callback] Error (404): Not Found
[Global Callback] Request complete, status: 404
Detected Error: HTTP 404 (Not Found)

--- [3] Changing URL at runtime ---
[Global Callback] Success with HTTP 200
[Global Callback] Request complete, status: 200
Switched to https://httpbin.org successfully (HTTP 200)
```

---

## Key Takeaways

- Callbacks receive the exact HTTP status code (e.g. `200`, `404`, `500`) or standard negative error code (e.g. `HTTPC_ERROR_CONNECTION_REFUSED`).
- `client.getErrorMessage()` provides human-readable context for any error without cryptic numeric codes.
