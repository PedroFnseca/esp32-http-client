# Quick Start

This guide walks you through your first HTTP request using **ESP32-HTTP-Client** in under 2 minutes.

---

## Step 1: Include the library

```cpp
#include <WiFi.h>
#include "ESP32HTTPClient.h"
```

## Step 2: Connect to WiFi

```cpp
WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");
while (WiFi.status() != WL_CONNECTED) delay(100);
```

## Step 3: Create the client

```cpp
// Pass your server's base URL (with protocol)
ESP32HTTPClient client("https://jsonplaceholder.typicode.com");
```

## Step 4: Make a request

```cpp
int userId = 0;
char title[64];
bool completed;

// GET https://jsonplaceholder.typicode.com/todos/1
// Response: { "userId": 1, "id": 1, "title": "...", "completed": false }
client.get("/todos/1")
      .getBody("userId", &userId)
      .getBody("title", title, sizeof(title))
      .getBody("completed", &completed);
```

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
    Serial.println("\nConnected!");
}

void loop() {
    int userId = 0;
    char title[64];
    bool completed;

    client.get("/todos/1")
          .getBody("userId", &userId)
          .getBody("title", title, sizeof(title))
          .getBody("completed", &completed);

    if (client.getStatusCode() == 200) {
        Serial.printf("User ID : %d\n", userId);
        Serial.printf("Title   : %s\n", title);
        Serial.printf("Done    : %s\n", completed ? "yes" : "no");
    } else {
        Serial.printf("HTTP Error: %d\n", client.getStatusCode());
    }

    delay(10000);
}
```

---

## What just happened?

1. You created an `ESP32HTTPClient` pointing at a base URL.
2. You called `.get("/todos/1")` to start building a GET request.
3. You chained `.getBody()` calls to declare _where_ each JSON field should go.
4. When the chain went out of scope, the library automatically dispatched the request, parsed the stream, and wrote the values directly into your variables.

!!! tip "No intermediate strings"
    The response body is **never stored in memory as a whole**. The library parses the JSON stream byte-by-byte as it arrives from the network, writing each matched value directly into the target variable.

---

## Next Steps

- [Guide: Making Requests](../guide/requests.md)
- [Guide: Reading Responses](../guide/responses.md)
- [All Examples](../examples/index.md)
