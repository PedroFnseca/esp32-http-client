---
tags:
  - example
  - json
  - raw
---
# Raw JSON

Demonstrates how to capture an **entire JSON object or array** into an Arduino `String` for manual processing.

**Source:** [`examples/RawArrayJSON/RawArrayJSON.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/RawArrayJSON/RawArrayJSON.ino)

---

## When to Use This

Use raw JSON capture when:

- You need to pass the JSON string to another library.
- You need to log or display the raw response.
- The response structure is dynamic and cannot be statically bound.

For most use cases, prefer direct bindings with typed `getBody()` calls — they avoid heap allocations entirely.

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
    String entireArray;
    String secondUser;

    client.get("/users")
          .getBody("",  &entireArray)  // captures the entire root array
          .getBody("1", &secondUser);  // captures the second user object

    if (client.getStatusCode() == 200) {
        Serial.printf("Second user JSON:\n%s\n", secondUser.c_str());
    } else {
        Serial.printf("Error: %d\n", client.getStatusCode());
    }

    delay(30000);
}
```

---

## Capturing the Root Response

Pass an **empty string** `""` as the key to capture the entire root-level object or array:

```cpp
String raw;
client.get("/users").getBody("", &raw); // captures the entire JSON response
```

## Capturing a Sub-Object or Sub-Array

Pass the path to the object/array you want to capture:

```cpp
String addressObj;
client.get("/users/1").getBody("address", &addressObj);
// addressObj = {"street":"Kulas Light","suite":"Apt. 556","city":"Gwenborough",...}

String secondUser;
client.get("/users").getBody("1", &secondUser);
// secondUser = the full second element of the array
```

---

## Expected Serial Output

```json
Second user JSON:
{"id":2,"name":"Ervin Howell","username":"Antonette","email":"...","address":{...},...}
```

---

!!! warning "Memory impact"
    Each character of the captured JSON is appended to the `String` on the heap one byte at a time, causing repeated reallocations. Avoid this pattern with payloads larger than a few kilobytes on memory-constrained boards.
