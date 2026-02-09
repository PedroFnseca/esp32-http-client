# ESP32-HTTP-Client

A lightweight, fluent HTTP client for ESP32 focused on simple REST consumption with direct response binding.

[![License](https://img.shields.io/github/license/PedroFnseca/esp32-http-client)](LICENSE)
[![Language](https://img.shields.io/github/languages/top/PedroFnseca/esp32-http-client)](https://github.com/PedroFnseca/esp32-http-client)

---

## Overview

`ESP32-HTTP-Client` provides an object-oriented API to build HTTP requests and map JSON fields directly into native variables.

Core goals:
- Reduce boilerplate compared to manual `HTTPClient` usage.
- Keep API usage intuitive with fluent chaining.
- Offer lightweight streaming parsing for common JSON response patterns.

---

## Installation

### Arduino IDE (manual)
1. Download the repository ZIP.
2. Open **Sketch > Include Library > Add .ZIP Library...**
3. Select the ZIP file.

### PlatformIO
Add this repository as a dependency in `platformio.ini`.

---

## Quick Start

```cpp
#include <WiFi.h>
#include "ESP32HTTPClient.h"

ESP32HTTPClient client("https://jsonplaceholder.typicode.com");

void setup() {
  Serial.begin(115200);
  WiFi.begin("SSID", "PASSWORD");
  while (WiFi.status() != WL_CONNECTED) delay(200);

  int userId = 0;
  int status = client.get("/todos/1")
                   .getBody("userId", &userId)
                   .send();

  Serial.printf("HTTP: %d | userId: %d\n", status, userId);
}

void loop() {}
```

---

## API Highlights

### HTTP Methods
- `get(path)`
- `post(path)`
- `put(path)` / `update(path)`
- `patch(path)`
- `del(path)`

### Request Builder
- `.query(key, value)` for query string parameters.
- `.body(key, value)` for JSON payload fields.
- `.send()` for explicit request execution.

> If `.send()` is not called, the request is executed automatically when the temporary request object is destroyed.

### Response Binding
Bind fields directly from a JSON response object:
- `getBody(const char*, int*)`
- `getBody(const char*, long*)`
- `getBody(const char*, float*)`
- `getBody(const char*, double*)`
- `getBody(const char*, bool*)`
- `getBody(const char*, char*, size_t)`

### Global Client Configuration
- `setContentType("application/json")`
- `setHeader("Authorization", "Bearer <token>")`
- `clearHeaders()`
- `getStatusCode()`

---

## Query Parameters (Improved)

Query keys and values are URL-encoded automatically. This allows safe usage of reserved characters and spaces.

```cpp
float temp = 0;

client.get("/weather")
      .query("city", "São Paulo")
      .query("units", "metric")
      .getBody("temperature", &temp)
      .send();
```

---

## Headers and Content Type

```cpp
client.setHeader("Authorization", "Bearer token-123");
client.setHeader("X-Device-Id", "esp32-kitchen");
client.setContentType("application/json");

int id = 0;
client.post("/devices")
      .body("name", "sensor-node")
      .body("enabled", true)
      .getBody("id", &id)
      .send();
```

---

## Limitations

- The response parser is optimized for JSON objects and direct field extraction.
- Deep traversal by path (e.g. `data.items[0].id`) is not currently supported.
- String bindings require pre-allocated buffers.

---


## Quality and CI

This repository includes host-side unit tests for critical encoding utilities (`urlEncode`, `escapeJson`, `buildUrl`).

- Local run:
  - `g++ -std=c++17 -Wall -Wextra -pedantic tests/http_encoding_test.cpp src/HttpEncoding.cpp -o tests/http_encoding_test`
  - `./tests/http_encoding_test`
- CI run: tests execute automatically on every Pull Request via GitHub Actions (`.github/workflows/unit-tests.yml`).

---

## Examples

- [`examples/SimpleGET/SimpleGET.ino`](examples/SimpleGET/SimpleGET.ino)
- [`examples/PostRequest/PostRequest.ino`](examples/PostRequest/PostRequest.ino)
- [`examples/PutRequest/PutRequest.ino`](examples/PutRequest/PutRequest.ino)
- [`examples/DeleteRequest/DeleteRequest.ino`](examples/DeleteRequest/DeleteRequest.ino)
- [`examples/PortSelection/PortSelection.ino`](examples/PortSelection/PortSelection.ino)

---

## License

MIT License. See [LICENSE](LICENSE).
