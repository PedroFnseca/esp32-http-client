# Unix Timestamp

Demonstrates how to fetch a **Unix timestamp** (a large `long` integer) from a public time API.

**Source:** [`examples/UnixTimestamp/UnixTimestamp.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/UnixTimestamp/UnixTimestamp.ino)

---

## Why `long`?

Unix timestamps represent the number of seconds since January 1, 1970. As of 2024, this value exceeds `1,700,000,000` — which overflows a 32-bit `int` on most platforms. Always use `long` for timestamps.

```cpp
long timestamp = 0;
// ✅ Correct — large number fits in a long
client.get("/time").getBody("unix_timestamp", &timestamp);

int bad = 0;
// ❌ Avoid — will overflow for dates after Jan 19, 2038
client.get("/time").getBody("unix_timestamp", &bad);
```

---

## API Used

**Endpoint:** `GET https://timeapi.io/api/v1/time/current/unix`

**Response:**
```json
{
  "unix_timestamp": 1721156604
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

ESP32HTTPClient client("https://timeapi.io", 443);

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
    long ts = 0;

    Serial.println("Fetching Unix timestamp...");

    client.get("/api/v1/time/current/unix")
          .getBody("unix_timestamp", &ts);

    if (client.getStatusCode() == 200) {
        Serial.printf("Unix timestamp: %ld\n", ts);
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
Fetching Unix timestamp...
Unix timestamp: 1721156604
```

---

## Key Points

- Use `%ld` (not `%d`) in `printf` for `long` values.
- The explicit port `443` is passed to the constructor — useful when connecting to HTTPS APIs that may not follow standard detection.
- `long` bindings handle values up to `2^63 - 1` on ESP32 (64-bit `long`).
