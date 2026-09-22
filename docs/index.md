---
title: ESP32 HTTP Client - Fluent, Zero-Heap HTTP Client Library
description: A lightweight, low-allocation, high-performance HTTP client library for ESP32 (Arduino and PlatformIO). Fluent C++ API, direct response binding for REST APIs, SOAP 1.1/1.2 web services, GraphQL, and extensible HTTP communication.
keywords: ESP32 HTTP client, ESP32 REST client, ESP32 SOAP client, ESP32 GraphQL client, Arduino ESP32 HTTP GET POST SOAP GraphQL, ESP32 API client, PlatformIO ESP32, zero heap HTTP client
tags:
  - home
  - overview
---
# ESP32 HTTP Client Library

> A lightweight, low-allocation, high-performance HTTP client library for ESP32 that **binds response data directly into your variables** featuring native zero-heap streaming engines for **REST APIs**, **SOAP 1.1 / 1.2 Web Services**, **GraphQL Services**, and extensible HTTP communication.

[![Arduino Library](https://img.shields.io/github/v/release/PedroFnseca/esp32-http-client?color=00979D&label=Arduino&logo=arduino&logoColor=white){: width="120" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client)
[![PlatformIO Registry](https://img.shields.io/github/v/release/PedroFnseca/esp32-http-client?color=f58220&label=PlatformIO&logo=platformio&logoColor=white){: width="130" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client)
[![Language](https://img.shields.io/github/languages/top/PedroFnseca/esp32-http-client){: width="80" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client)
[![Coverage](https://img.shields.io/badge/Coverage-93.73%25-brightgreen){: width="116" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client)
[![License](https://img.shields.io/github/license/PedroFnseca/esp32-http-client){: width="80" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client/blob/main/LICENSE)
[![Stars](https://img.shields.io/github/stars/PedroFnseca/esp32-http-client?style=social){: width="80" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client/stargazers)
[![Downloads](https://img.shields.io/endpoint?url=https://esp32-http-stats.esp32httpclient.com/downloads)](https://github.com/PedroFnseca/esp32-http-client)

---

## What is it?

**ESP32-HTTP-Client** is a modern, modular HTTP client for the ESP32 designed to bridge web services and device memory efficiently. Instead of treating HTTP communication as raw string manipulation followed by heavy DOM document parsing, it streams and extracts response fields directly into your C++ variables on-the-fly.

Built on a shared high-efficiency transport core (TLS, connection reuse, authentication, timeouts, and retries), the client provides dedicated, fluent builders tailored for standard web communication patterns:

=== "REST (JSON)"

    Consume modern RESTful endpoints with intuitive verb methods (`get`, `post`, `put`, `patch`, `del`), path/query parameters, and zero-allocation JSON extraction or bidirectional struct mapping:

    ```cpp
    int userId;
    float temperature;
    char city[32];

    client.get("/report")
          .query("format", "compact")
          .getBody("userId", &userId)
          .getBody("sensor.temp", &temperature)
          .getBody("0.address.city", city, sizeof(city));
    ```

=== "GraphQL (Queries & Mutations)"

    Execute GraphQL operations with typed variables, operation selection, batching (`GraphQLBatchRequest`), partial data preservation, and `@defer` streaming:

    ```cpp
    String name;
    int id = 0;

    client.graphql("/graphql")
          .query("query GetUser($id: ID!) { user(id: $id) { id name } }")
          .variable("id", 101)
          .getData("user.id", &id)
          .getData("user.name", &name);
    ```

=== "SOAP (XML 1.1 / 1.2)"

    Connect to enterprise SOAP web services with automated envelope generation, `SOAPAction` / `Content-Type` handling, streaming XML token parsing, and native SOAP Fault inspection:

    ```cpp
    float price = 0.0f;
    SoapFault fault;

    client.soap("/ws")
          .soapAction("http://example.org/GetPrice")
          .body("<m:GetPrice xmlns:m=\"http://example.org\"><m:Item>ESP32</m:Item></m:GetPrice>")
          .getFault(&fault)
          .getBody("Price", &price);
    ```

=== "Extensible Core"

    A unified client instance manages persistent configuration across all requests — including TLS security, custom headers, authentication (Bearer, Basic, API Key, Cookies), network retries, and telemetry observability:

    ```cpp
    ESP32HTTPClient client("https://api.example.com");
    client.bearer("token_xyz");
    client.setTimeout(5000);
    client.setMaxRetry(2);

    // Reuse client seamlessly for REST, SOAP, or GraphQL endpoints
    client.get("/api/v1/health");
    client.soap("/ws/service");
    client.graphql("/graphql");
    ```

One unified client. Direct memory binding. Minimal RAM footprint.

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
 
=== "REST API (JSON)"

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

=== "SOAP Web Service (XML)"

    ```cpp
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    ESP32HTTPClient client("https://www.dataaccess.com");

    void setup() {
        Serial.begin(115200);
        WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");
        while (WiFi.status() != WL_CONNECTED) delay(100);

        char result[64] = {0};

        // Sends SOAP 1.1 request and extracts <m:NumberToWordsResult> tag directly
        client.soap("/webservicesserver/NumberConversion.wso")
              .soapAction("http://www.dataaccess.com/webservicesserver/NumberToWords")
              .body("<NumberToWords xmlns=\"http://www.dataaccess.com/webservicesserver/\">"
                    "<ubiNum>500</ubiNum>"
                    "</NumberToWords>")
              .getBody("NumberToWordsResult", result, sizeof(result));

        Serial.printf("Result: %s\n", result);
    }

    void loop() {}
    ```

=== "GraphQL API"

    ```cpp
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    ESP32HTTPClient client("https://countries.trevorblades.com");

    void setup() {
        Serial.begin(115200);
        WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");
        while (WiFi.status() != WL_CONNECTED) delay(100);

        char countryName[64] = {0};

        // Queries country data with variables and binds directly to variable
        client.graphql("/graphql")
              .query("query GetCountry($code: ID!) { country(code: $code) { name } }")
              .variable("code", "BR")
              .getData("country.name", countryName, sizeof(countryName));

        Serial.printf("Country: %s\n", countryName);
    }

    void loop() {}
    ```

→ [See all examples](examples/index.md)

---

<p align="center">
  If this library saved you time, consider leaving a ⭐ on <a href="https://github.com/PedroFnseca/esp32-http-client">GitHub</a>.
</p>
