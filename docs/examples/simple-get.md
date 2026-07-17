---
tags:
  - example
  - get
  - http
---
# Simple GET

Demonstrates a basic HTTP GET request and binding multiple fields from a JSON response into typed C variables.

**Source:** [`examples/SimpleGET/SimpleGET.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/SimpleGET/SimpleGET.ino)

---

## API Used

**Endpoint:** `GET https://jsonplaceholder.typicode.com/todos/1`

**Response:**
```json
{
  "userId": 1,
  "id": 1,
  "title": "delectus aut autem",
  "completed": false
}
```

---

## Sketch

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
}

void loop() {
    int userId;
    int id;
    char title[64];
    bool completed;

    Serial.println("Sending GET request...");

    client.get("/todos/1")
          .getBody("userId",    &userId)
          .getBody("id",        &id)
          .getBody("title",     title, sizeof(title))
          .getBody("completed", &completed);

    if (client.getStatusCode() == 200) {
        Serial.printf("User ID  : %d\n", userId);
        Serial.printf("ID       : %d\n", id);
        Serial.printf("Title    : %s\n", title);
        Serial.printf("Completed: %s\n", completed ? "true" : "false");
    } else {
        Serial.printf("Error: %d\n", client.getStatusCode());
    }

    delay(10000);
}
```

---

## Expected Serial Output

```
Connected to WiFi
Sending GET request...
User ID  : 1
ID       : 1
Title    : delectus aut autem
Completed: false
```

---

## Key Points

- `int` and `bool` are passed by pointer with `&`.
- `char` buffers are passed with the buffer pointer and its size — the library never writes past the boundary.
- `getStatusCode()` is checked after the request to guard against network errors.
