---
tags:
  - example
  - auth
  - bearer
  - basic
  - api-key
---
# Authentication Helpers

Demonstrates how to authenticate requests using Bearer / JWT tokens, HTTP Basic Auth, and custom API Key headers.

**Source:** [`examples/AuthHelpers/AuthHelpers.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/AuthHelpers/AuthHelpers.ino)

---

## Overview

The client provides dedicated helpers that configure persistent authorization headers for all subsequent requests:

| Helper Method | Header Generated | Description |
| :--- | :--- | :--- |
| `client.bearer(token)` | `Authorization: Bearer <token>` | For JWT and OAuth2 Bearer tokens. |
| `client.basic(user, pass)` | `Authorization: Basic <base64>` | Automatically encodes credentials into Base64. |
| `client.apiKey(name, key)` | `<name>: <key>` | Sets any API Key header (e.g., `X-API-Key`). |
| `client.setHeader(name, val)` | `<name>: <val>` | Sets any generic persistent HTTP header. |

---

## Full Sketch

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://httpbin.org");

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
    char authHeader[128] = {0};

    // 1. Bearer Token Authentication
    Serial.println("\n--- [1] Testing Bearer Token ---");
    client.bearer("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.sampleToken");
    client.get("/bearer").getBody("token", authHeader, sizeof(authHeader));

    if (client.isSuccess()) {
        Serial.printf("Bearer Authenticated -> Token verified by server\n");
    }

    // 2. HTTP Basic Authentication (Username + Password)
    Serial.println("\n--- [2] Testing HTTP Basic Auth ---");
    client.basic("admin", "secret123");
    client.get("/basic-auth/admin/secret123");

    if (client.isSuccess()) {
        Serial.println("Basic Auth Success -> 200 OK");
    }

    // 3. API Key Header Authentication
    Serial.println("\n--- [3] Testing API Key Header ---");
    client.apiKey("X-API-KEY", "my-super-secret-api-key-9988");
    client.get("/headers").getBody("headers.X-Api-Key", authHeader, sizeof(authHeader));

    if (client.isSuccess()) {
        Serial.printf("API Key Sent -> Server received: %s\n", authHeader);
    }

    delay(15000);
}
```

---

## Expected Serial Output

```
Connected to WiFi

--- [1] Testing Bearer Token ---
Bearer Authenticated -> Token verified by server

--- [2] Testing HTTP Basic Auth ---
Basic Auth Success -> 200 OK

--- [3] Testing API Key Header ---
API Key Sent -> Server received: my-super-secret-api-key-9988
```

---

## Key Takeaways

- Authentication headers configured via `bearer()`, `basic()`, `apiKey()`, or `setHeader()` **persist** across all requests made by that client instance.
- `client.basic()` encodes username and password on the fly, eliminating the need for manual Base64 encoding.
