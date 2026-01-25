# ESP32-REST-Client  
> **Object-Oriented REST and HTTP Client for ESP32**
> 
> ESP32-REST-Client is a **lightweight, object-oriented REST library for ESP32 and Arduino** that simplifies HTTP and JSON handling for embedded systems.
> 
> It allows you to communicate with REST APIs using **clients, endpoints and variables** instead of raw HTTP and manual JSON parsing.

<p align="center">
  <img src="https://img.shields.io/github/languages/top/PedroFnseca/esp32-rest-client" alt="Language">
  <img src="https://hits.sh/github.com/PedroFnseca/esp32-rest-client.svg?view=today-total" alt="Hits">
  <img src="https://img.shields.io/github/license/PedroFnseca/esp32-rest-client" alt="License">
  <img src="https://img.shields.io/github/stars/PedroFnseca/esp32-rest-client?style=social" alt="Stars">
</p>


## Core Concept

```cpp
client.get("/endpoint").getBody("value", &var);
```

You request → it parses → it fills your variables.

- No JSON decoding.
- No String objects.
- No boilerplate.

---

### Creating a Client
Create a REST client once and reuse it across your entire firmware. 

> **Note:** You must include `<WiFi.h>` and ensure the device is connected to WiFi.

```cpp
#include <WiFi.h>
#include "ESP32RestClient.h"

RestClient client("https://api.example.com");
```

<br>

### GET with Query Parameters
Send query parameters and map response fields directly into native C variables.
```cpp
int temp;

client.get("/sensor")
      .query("id", 10)
      .getBody("temperature", &temp);
```

<br>

### POST with Body Data
Build request bodies fluently and receive structured responses without parsing JSON manually.
```cpp
int userId;

client.post("/users")
      .body("name", "Pedro")
      .body("age", 20)
      .getBody("id", &userId);
```

<br>

### Update (PUT)
Send updates using REST semantics with clean, readable code.
```cpp
client.update("/device/1")
      .body("status", "on");
```

<br>

### Delete
Simple and explicit endpoint deletion.
```cpp
client.delete("/users/5");
```

---

Why this model?

Instead of handling HTTP and JSON manually, this library **binds JSON fields directly** to C variables.
```cpp
client.get("/status").getBody("online", &isOnline);
```

## This makes code:
- [x] Easier to read
- [x] Faster to write
- [x] More memory efficient
- [x] Ideal for ESP32

## Classic ESP32 HTTP vs ESP32-REST-Client

### Classic way
```cpp
HTTPClient http;
http.begin("https://api.example.com/sensor");
http.GET();

String payload = http.getString();

DynamicJsonDocument doc(1024);
deserializeJson(doc, payload);

int temp = doc["temperature"];
```

This approach uses:
- String buffers
- Large JSON objects
- More RAM
- More code

ESP32-REST-Client way
```cpp
int temp;
client.get("/sensor").getBody("temperature", &temp);
```

- No strings.
- No JSON objects.
- Direct memory binding.

---

Designed for IoT

## ESP32-REST-Client is built for:

- [x] Low RAM devices
- [x] IoT APIs
- [x] Fast firmware development
- [x] REST-based cloud systems


---

## Examples

Check the `/examples` folder for full working sketches demonstrating various features:

- [**SimpleGET**](examples/SimpleGET/SimpleGET.ino): How to fetch and bind variables.
- [**PostRequest**](examples/PostRequest/PostRequest.ino): How to send JSON data.
- [**PutRequest**](examples/PutRequest/PutRequest.ino): How to update resources.
- [**DeleteRequest**](examples/DeleteRequest/DeleteRequest.ino): How to delete resources.

---

### Keywords

ESP32 REST Client, ESP32 HTTP Library, Arduino REST API, ESP32 JSON Parser, IoT REST Library, Embedded HTTP Client
