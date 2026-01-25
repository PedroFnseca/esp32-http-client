# ⚡ ESP32-HTTP-Client  

[![Language](https://img.shields.io/github/languages/top/PedroFnseca/esp32-http-client)](https://github.com/PedroFnseca/esp32-http-client)
[![Hits](https://hits.sh/github.com/PedroFnseca/esp32-http-client.svg?view=today-total)](https://hits.sh/github.com/PedroFnseca/esp32-http-client/)
[![License](https://img.shields.io/github/license/PedroFnseca/esp32-http-client)](LICENSE)
[![Stars](https://img.shields.io/github/stars/PedroFnseca/esp32-http-client?style=social)](https://github.com/PedroFnseca/esp32-http-client/stargazers)

> **The modern, object-oriented way to consume REST APIs on ESP32.**

---

## 🚀 Why this library?

Writing HTTP requests on embedded systems shouldn't feel like a chore. The standard approach forces you to manage connection states, handle string buffers manually, and worst of all—allocate massive chunks of RAM just to parse a simple JSON response.

**ESP32-HTTP-Client changes the game.** It acts as a bridge between your variables and your API. You don't "parse" JSON; you simply tell the client where to put the data.

### 🛑 The Problem: The "Standard" Way

You've probably written code like this a hundred times:
1.  Initialize `HTTPClient`.
2.  Make the request.
3.  Check error codes.
4.  `http.getString()` (Allocating a huge String).
5.  Create a `DynamicJsonDocument` (Allocating even more RAM).
6.  `deserializeJson()`.
7.  Extract values.
8.  Hope you didn't run out of heap.

### ✅ The Solution: ESP32-HTTP-Client

```cpp
// One line. Zero intermediate strings. Direct memory binding.
client.get("/sensor").getBody("temperature", &myFloatVariable);
```

---

## 📊 Performance & Comparison

Why switch? Because your ESP32 deserves better memory management.

| Feature | 🐢 Standard (HTTPClient + ArduinoJson) | ⚡ ESP32-HTTP-Client |
| :--- | :--- | :--- |
| **Code Verbosity** | **High** (~15 lines of boilerplate) | **Low** (1 fluent chain) |
| **Memory Usage** | **Heavy** (Stores full payload + JSON Tree) | **Lightweight** (Stream parsing, no buffering) |
| **Syntax** | Procedural & Clunky | Fluent & Object-Oriented |
| **JSON Parsing** | Requires `deserializeJson()` | **Automatic** (Direct binding) |
| **Developer Joy** | 😫 Frustrating | 🤩 Effortless |

---

## ✨ Key Features at a Glance

*   🔗 **Fluent Chaining**: Build requests naturally: `.get().query().getBody()`.
*   💉 **Direct Injection**: Key values from JSON are injected directly into standard C types (`int`, `float`, `bool`, `char*`).
*   📉 **Zero Overhead**: We don't store the JSON. We read it as it arrives and extract what you need.
*   🛠️ **Full REST Support**: `GET`, `POST`, `PUT`, `DELETE` are first-class citizens.
*   🧩 **IoT Ready**: Perfect for connecting your ESP32 to cloud backends, Firebase, AWS API Gateway, or your custom Node.js/Python servers.

---

## ⚡ Hello World

```cpp
#include <WiFi.h>
#include "ESP32HTTPClient.h"

ESP32HTTPClient client("https://jsonplaceholder.typicode.com");

void setup() {
    Serial.begin(115200);
    WiFi.begin("SSID", "PASS");
    
    while (WiFi.status() != WL_CONNECTED) delay(100);

    // Prepare a variable
    int userId = 0;

    // Fetch data
    // API returns: { "userId": 1, "id": 1, "title": "..." }
    client.get("/todos/1").getBody("userId", &userId);

    Serial.printf("User ID fetched from API: %d\n", userId);
}

void loop() {}
```

---

## ⚙️ Initialization Options

### Default Port (80 for HTTP, 443 for HTTPS)
```cpp
ESP32HTTPClient client("https://api.example.com");
```

### Custom Port
Specify the port as the second argument if your API runs on a non-standard port.
```cpp
ESP32HTTPClient client("http://my-local-server.local", 8080);
```

---

## 📖 Deep Dive: Usage Scenarios

### 🔍 GET with Query Params
Don't mess with string concatenation (`?id=` + String(id) + `&val=`...). Let the builder do it.

```cpp
float temperature;
bool active;

// Request: GET /climate?room=living_room&sensor=dht22
client.get("/climate")
      .query("room", "living_room")
      .query("sensor", "dht22")
      .getBody("temp", &temperature)  // Finds "temp": 24.5
      .getBody("active", &active);    // Finds "active": true
```

### 📤 POST JSON Data
Sending data is just as easy as receiving it.

```cpp
int newId;

// Request: POST /users
// Body: { "name": "Pedro", "role": "admin", "age": 21 }
client.post("/users")
      .body("name", "Pedro")
      .body("role", "admin")
      .body("age", 21)
      .getBody("id", &newId); // Get the ID created by the server
```

### 🔄 PUT (Update) & DELETE
Complete control over your resources.

```cpp
// PUT: Update status
client.update("/lights/1").body("state", "OFF");

// DELETE: Remove a log
client.del("/logs/system_error.log");
```

---

## 📂 Examples

Explore the full capabilities in the `examples/` directory:

*   [**SimpleGET**](examples/SimpleGET/SimpleGET.ino) - Basic data fetching.
*   [**PostRequest**](examples/PostRequest/PostRequest.ino) - Sending data to an API.
*   [**PutRequest**](examples/PutRequest/PutRequest.ino) - Updating server resources.
*   [**DeleteRequest**](examples/DeleteRequest/DeleteRequest.ino) - Deleting data.
*   [**PortSelection**](examples/PortSelection/PortSelection.ino) - Connecting to a custom port.

---

<p align="center">
  <b>Don't forget to star ⭐ this repo if it saved you time!</b>
</p>
