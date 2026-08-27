---
title: ESP32 HTTP Client - Fast Zero-Heap REST Library
description: High-performance, zero-heap ESP32 HTTP client library for Arduino and PlatformIO. Fluent C++ API, direct JSON response binding, low memory footprint.
keywords: ESP32 HTTP client, ESP32 REST client, Arduino ESP32 HTTP GET POST, ESP32 JSON parser, PlatformIO ESP32, zero heap HTTP client
tags:
  - home
  - overview
---
# ESP32 HTTP Client Library

> A fluent, object-oriented HTTP client for ESP32 that **binds JSON response fields directly into your variables** — no `ArduinoJson`, no intermediate strings, no boilerplate.

[![Arduino Library](https://img.shields.io/github/v/release/PedroFnseca/esp32-http-client?color=00979D&label=Arduino&logo=arduino&logoColor=white){: width="120" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client)
[![PlatformIO Registry](https://img.shields.io/github/v/release/PedroFnseca/esp32-http-client?color=f58220&label=PlatformIO&logo=platformio&logoColor=white){: width="130" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client)
[![Language](https://img.shields.io/github/languages/top/PedroFnseca/esp32-http-client){: width="80" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client)
[![Coverage](https://img.shields.io/badge/Coverage-99.22%25-brightgreen){: width="116" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client)
[![License](https://img.shields.io/github/license/PedroFnseca/esp32-http-client){: width="80" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client/blob/main/LICENSE)
[![Stars](https://img.shields.io/github/stars/PedroFnseca/esp32-http-client?style=social){: width="80" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client/stargazers)
[![Downloads](https://img.shields.io/endpoint?url=https://esp32-http-stats.esp32httpclient.com/downloads)](https://github.com/PedroFnseca/esp32-http-client)

---

## What is it?

**ESP32-HTTP-Client** is a lightweight Arduino library for the ESP32 that rethinks how you interact with REST APIs. Instead of fetching a raw JSON string and then parsing it, you simply tell the client _where_ to put the data, it handles the rest.

```cpp
int userId;
float temperature;
char city[32];

client.get("/report")
      .getBody("userId", &userId)
      .getBody("sensor.temp", &temperature)
      .getBody("0.address.city", city, sizeof(city));
```

One fluent chain. Direct memory binding. Zero heap allocations for the response.

---

## Performance at a Glance

Benchmarked over **100 consecutive HTTP GET requests** with JSON payloads on a real ESP32 device:

| Metric | Standard (HTTPClient + ArduinoJson) | ESP32-HTTP-Client |
| :--- | :---: | :---: |
| **Heap allocation per request** | ~58.2 KB | **~15 bytes** |
| **Average RAM footprint** | 34.2% | **24.3%** |
| **Minimum free heap** | 114.3 KB | **128.6 KB** |
| **Average execution time** | ~750 ms | **~59 ms** |

→ [See the full performance analysis](performance.md)

---

## Quick Install

=== "Arduino Library Manager"

    Search for **ESP32-HTTP-Client** in the Arduino IDE Library Manager and click **Install**.

=== "PlatformIO"

    Add `ESP32-HTTP-Client` to your `platformio.ini`:
    ```ini
    lib_deps =
        PedroFnseca/ESP32-HTTP-Client@^1.4.0
    ```

=== "Manual"

    Download the [latest release](https://github.com/PedroFnseca/esp32-http-client/releases) and place the folder inside your `Arduino/libraries/` directory.

→ [Full installation guide](getting-started/installation.md)

---

## 30-Second Quick Start

```cpp
#include <WiFi.h>
#include "ESP32HTTPClient.h"

ESP32HTTPClient client("https://jsonplaceholder.typicode.com");

void setup() {
    Serial.begin(115200);
    WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");
    while (WiFi.status() != WL_CONNECTED) delay(100);

    int userId = 0;

    // API returns: { "userId": 1, "id": 1, "title": "...", "completed": false }
    client.get("/todos/1").getBody("userId", &userId);

    Serial.printf("User ID: %d\n", userId);
}

void loop() {}
```

→ [See all examples](examples/index.md)

---

<p align="center">
  If this library saved you time, consider leaving a ⭐ on <a href="https://github.com/PedroFnseca/esp32-http-client">GitHub</a>.
</p>
