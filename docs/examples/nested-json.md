---
tags:
  - example
  - json
  - nested
---
# Nested JSON

Demonstrates extracting fields from deeply nested JSON objects using **dot notation** paths.

**Source:** [`examples/NestedJSON/NestedJSON.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/NestedJSON/NestedJSON.ino)

---

## API Used

**Endpoint:** `GET https://jsonplaceholder.typicode.com/users/1`

**Relevant portion of the response:**
```json
{
  "id": 1,
  "name": "Leanne Graham",
  "address": {
    "street": "Kulas Light",
    "suite": "Apt. 556",
    "city": "Gwenborough",
    "zipcode": "92998-3874",
    "geo": {
      "lat": "-37.3159",
      "lng": "81.1496"
    }
  }
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
    char street[64];
    char city[64];
    char lat[32];
    char lng[32];

    Serial.println("Fetching nested user address...");

    client.get("/users/1")
          .getBody("address.street",      street, sizeof(street))
          .getBody("address.city",        city,   sizeof(city))
          .getBody("address.geo.lat",     lat,    sizeof(lat))
          .getBody("address.geo.lng",     lng,    sizeof(lng));

    if (client.getStatusCode() == 200) {
        Serial.printf("Street   : %s\n", street);
        Serial.printf("City     : %s\n", city);
        Serial.printf("Latitude : %s\n", lat);
        Serial.printf("Longitude: %s\n", lng);
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
Fetching nested user address...
Street   : Kulas Light
City     : Gwenborough
Latitude : -37.3159
Longitude: 81.1496
```

---

## Key Points

- Use `.` as a separator to traverse nested objects: `"address.city"`.
- Nesting can go arbitrarily deep: `"address.geo.lat"` traverses two levels.
- The library resolves each segment of the path in order during stream parsing.
- There is no practical limit on nesting depth.
