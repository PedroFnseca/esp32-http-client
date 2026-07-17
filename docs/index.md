---
tags:
  - home
  - overview
---
# ESP32-HTTP-Client

> A fluent, object-oriented HTTP client for ESP32 that **binds JSON response fields directly into your variables** — no `ArduinoJson`, no intermediate strings, no boilerplate.

[![Language](https://img.shields.io/github/languages/top/PedroFnseca/esp32-http-client)](https://github.com/PedroFnseca/esp32-http-client)
[![Coverage](https://img.shields.io/badge/Coverage-88.18%25-brightgreen)](https://github.com/PedroFnseca/esp32-http-client)
[![License](https://img.shields.io/github/license/PedroFnseca/esp32-http-client)](https://github.com/PedroFnseca/esp32-http-client/blob/main/LICENSE)
[![Stars](https://img.shields.io/github/stars/PedroFnseca/esp32-http-client?style=social)](https://github.com/PedroFnseca/esp32-http-client/stargazers)

---

## What is it?

**ESP32-HTTP-Client** is a lightweight Arduino library for the ESP32 that rethinks how you interact with REST APIs. Instead of fetching a raw JSON string and then parsing it, you simply tell the client _where_ to put the data — it handles the rest.

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
