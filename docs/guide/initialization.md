# Initialization

## Creating the Client

`ESP32HTTPClient` is the entry point for all requests. Create one instance per server and reuse it — the library keeps the connection alive between calls automatically.

```cpp
#include "ESP32HTTPClient.h"

ESP32HTTPClient client("https://api.example.com");
```

### Constructor Signatures

```cpp
// Standard port (80 for HTTP, 443 for HTTPS)
ESP32HTTPClient(const char* baseUrl);

// Custom port
ESP32HTTPClient(const char* baseUrl, int port);
```

---

## Examples

### Standard HTTPS (port 443)

```cpp
ESP32HTTPClient client("https://api.example.com");
```

### Standard HTTP (port 80)

```cpp
ESP32HTTPClient client("http://192.168.1.100");
```

### Custom Port

Useful for local servers, development environments, or IoT gateways:

```cpp
ESP32HTTPClient client("http://my-local-server.local", 8080);
```

### Explicit Port on HTTPS

```cpp
ESP32HTTPClient client("https://timeapi.io", 443);
```

---

## Placement

Declare the client at **global scope** so it persists across `loop()` iterations and the Keep-Alive connection is preserved:

```cpp
// ✅ Correct — client lives for the lifetime of the program
ESP32HTTPClient client("https://api.example.com");

void setup() { ... }
void loop() {
    // client is reused on every iteration
    client.get("/data").getBody("value", &myVar);
}
```

```cpp
// ❌ Avoid — a new client (and new TLS connection) is created on every loop
void loop() {
    ESP32HTTPClient client("https://api.example.com"); // new handshake every time!
    client.get("/data").getBody("value", &myVar);
}
```

---

## Next Step

→ [Making Requests](requests.md)
