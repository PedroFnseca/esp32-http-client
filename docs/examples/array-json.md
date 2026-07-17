---
tags:
  - example
  - json
  - array
---
# Array JSON

Demonstrates how to extract specific elements from a **JSON array** response using **numeric index path segments**.

**Source:** [`examples/ArrayJSON/ArrayJSON.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/ArrayJSON/ArrayJSON.ino)

---

## API Used

**Endpoint:** `GET https://jsonplaceholder.typicode.com/users`

**Relevant portion of the response (array of objects):**
```json
[
  {
    "id": 1,
    "name": "Leanne Graham",
    "address": { "city": "Gwenborough" }
  },
  {
    "id": 2,
    "name": "Ervin Howell",
    "address": { "city": "Wisokyburgh" }
  }
]
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
    char firstUserName[64];
    char secondUserCity[64];

    Serial.println("Fetching array of users...");

    client.get("/users")
          .getBody("0.name",         firstUserName,  sizeof(firstUserName))
          .getBody("1.address.city", secondUserCity, sizeof(secondUserCity));

    if (client.getStatusCode() == 200) {
        Serial.printf("First user's name  : %s\n", firstUserName);
        Serial.printf("Second user's city : %s\n", secondUserCity);
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
Fetching array of users...
First user's name  : Leanne Graham
Second user's city : Wisokyburgh
```

---

## Path Syntax Reference

| Path | What it resolves |
| :--- | :--- |
| `"0.name"` | `name` field of the **first** element (index 0) |
| `"1.name"` | `name` field of the **second** element (index 1) |
| `"1.address.city"` | Nested `city` inside `address` of the second element |
| `"0"` (with `String*`) | The entire first element as a raw JSON string |

!!! note "Zero-based indexing"
    Array indices are **zero-based**, following standard C conventions.
