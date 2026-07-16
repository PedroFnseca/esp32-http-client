# Custom Port

Demonstrates how to connect to a server running on a **non-standard port** (anything other than 80 for HTTP or 443 for HTTPS).

**Source:** [`examples/PortSelection/PortSelection.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/PortSelection/PortSelection.ino)

---

## When Do You Need This?

- Local development servers (e.g., `http://192.168.1.100:8080`)
- Docker containers exposing mapped ports
- Custom IoT gateway backends
- APIs hosted on non-standard ports

---

## Usage

Pass the port as the second argument to the `ESP32HTTPClient` constructor:

```cpp
// HTTP server on port 8080
ESP32HTTPClient client("http://192.168.1.100", 8080);

// HTTPS server on port 8443
ESP32HTTPClient client("https://my-server.local", 8443);

// Explicit HTTPS standard port (optional but explicit)
ESP32HTTPClient client("https://timeapi.io", 443);
```

---

## Sketch

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Connect to a local server on port 8080
ESP32HTTPClient client("http://192.168.1.100", 8080);

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
    float sensorValue;

    client.get("/api/sensor")
          .getBody("value", &sensorValue);

    if (client.getStatusCode() == 200) {
        Serial.printf("Sensor value: %.2f\n", sensorValue);
    } else {
        Serial.printf("Error: %d\n", client.getStatusCode());
    }

    delay(5000);
}
```

---

## Key Points

- Port `0` (the default when no port is specified) means "use the standard port for the protocol" — 80 for HTTP and 443 for HTTPS.
- Once set in the constructor, the port applies to **all requests** made by that client instance.
- The URL passed to the constructor should **not** include the port — use the second constructor argument instead.
