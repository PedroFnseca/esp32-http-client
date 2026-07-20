# [ESP32 HTTP Client](https://pedrofnseca.github.io/esp32-http-client/) the Easy Way & Low Memory Footprint
**A high-performance, fluent, and object-oriented HTTP client for ESP32 with direct JSON binding capabilities.**

## **[Official Documentation](https://pedrofnseca.github.io/esp32-http-client/)** Get started quickly with the [Quick Start Guide](https://pedrofnseca.github.io/esp32-http-client/getting-started/quickstart) or explore the [API Reference](https://pedrofnseca.github.io/esp32-http-client/api/esp32httpclient/) for detailed usage.

[![Language](https://img.shields.io/github/languages/top/PedroFnseca/esp32-http-client)](https://github.com/PedroFnseca/esp32-http-client)
[![Coverage](https://img.shields.io/badge/Coverage-88.18%25-brightgreen)](https://github.com/PedroFnseca/esp32-http-client)
[![Hits](https://hits.sh/github.com/PedroFnseca/esp32-http-client.svg?view=today-total)](https://hits.sh/github.com/PedroFnseca/esp32-http-client/)
[![License](https://img.shields.io/github/license/PedroFnseca/esp32-http-client)](LICENSE)
[![Stars](https://img.shields.io/github/stars/PedroFnseca/esp32-http-client?style=social)](https://github.com/PedroFnseca/esp32-http-client/stargazers)

---

## Table of Contents

- [Why this library?](#why-this-library)
- [Performance & Comparison](#performance--comparison)
- [Key Features](#key-features)
- [Quick Start](#quick-start)
- [Initialization](#initialization)
- [Usage](#usage)
- [Examples](#examples)
- [API Reference](#api-reference)

---

## Why this library?

Writing HTTP requests on embedded systems shouldn't feel like a chore. The standard approach forces you to manage connection states, handle string buffers manually, and allocate large chunks of RAM just to parse a simple JSON response.

**ESP32-HTTP-Client** acts as a bridge between your variables and your API. You don't "parse" JSON, you tell the client where to put the data.

### The problem: the standard approach

A typical request with the Arduino SDK looks like this:

1. Initialize `HTTPClient`.
2. Make the request.
3. Check error codes.
4. Call `http.getString()`, allocating a large `String` on the heap.
5. Create a `DynamicJsonDocument`, allocating even more RAM.
6. Call `deserializeJson()`.
7. Extract values manually.
8. Hope you didn't run out of heap.

### The solution

```cpp
// One line. Zero intermediate strings. Direct memory binding.
client.get("/sensor").getBody("temperature", &myFloatVariable);
```

---

## Performance & Comparison

The following data is the result of a benchmark running 100 consecutive HTTP GET requests with JSON payloads on an ESP32 using the public `JSONPlaceholder` `/users` endpoint as the test source.
[JSONPlaceholder /users endpoint](https://jsonplaceholder.typicode.com/users?utm_source=chatgpt.com)


| Metric / Feature | Standard (HTTPClient + ArduinoJson) | ESP32-HTTP-Client | Comparison |
| :--- | :--- | :--- | :--- |
| **Memory Usage (Heap per req)** | ~58.2 KB | **~0.0 KB** (15 bytes) | ⬇ **~99.9% less RAM per request** |
| **Avg. RAM Footprint (Estimate)** | 34.2% | **24.3%** | ⬇ **~29% less overall RAM used** |
| **Absolute Min. Free Heap** | 114.3 KB | **128.6 KB** | ⬆ **Safer for large applications** |
| **Execution Time (Average)** | ~750 ms | **~59 ms** | 🚀 **~12x faster (Native Keep-Alive)** |
| **Code Verbosity** | High (~15 lines of boilerplate) | **Low (1 fluent chain)** | ⬇ **Clean & maintainable code** |
| **JSON Parsing** | Requires `deserializeJson()` | **Automatic, direct binding**| ⬆ **No JSON document allocation** |

> [!NOTE]
> **Execution Time & Keep-Alive:** Because `ESP32-HTTP-Client` safely reuses the underlying TLS connection and parses the response directly from the network stream (with native `Transfer-Encoding: chunked` decoding), it avoids the massive penalty of repeatedly establishing TLS handshakes. This makes it over **10x faster** than the traditional approach while keeping the memory footprint exceptionally low.


<img width="2723" height="1949" alt="image" src="https://github.com/user-attachments/assets/f7b84b01-04f1-44c9-a2d1-334f60cb91b0" />

---

## Key Features

- **Fluent chaining** — build requests naturally: `.get().query().getBody()`.
- **Direct injection** — JSON values are written straight into standard C types (`int`, `float`, `bool`, `char*`).
- **Zero buffering** — the response stream is parsed in place; the full payload is never stored.
- **Full REST support** — `GET`, `POST`, `PUT`, `PATCH`, and `DELETE` are all first-class citizens.
- **IoT ready** — designed for connecting ESP32 devices to cloud backends, Firebase, AWS API Gateway, or custom servers.

---

## Quick Start

```cpp
#include <WiFi.h>
#include "ESP32HTTPClient.h"

ESP32HTTPClient client("https://jsonplaceholder.typicode.com");

void setup() {
    Serial.begin(115200);
    WiFi.begin("SSID", "PASS");

    while (WiFi.status() != WL_CONNECTED) delay(100);

    int userId = 0;

    // API returns: { "userId": 1, "id": 1, "title": "..." }
    client.get("/todos/1").getBody("userId", &userId);

    Serial.printf("User ID fetched from API: %d\n", userId);
}

void loop() {}
```

---

## Initialization

### Default port (80 for HTTP, 443 for HTTPS)

```cpp
ESP32HTTPClient client("https://api.example.com");
```

### Custom port

Specify the port as the second argument if your API runs on a non-standard port.

```cpp
ESP32HTTPClient client("http://my-local-server.local", 8080);
```

---

## Usage

### GET with query parameters

```cpp
float temperature;
bool active;

// Produces: GET /climate?room=living_room&sensor=dht22
client.get("/climate")
      .query("room", "living_room")
      .query("sensor", "dht22")
      .getBody("temp", &temperature)  // binds "temp": 24.5
      .getBody("active", &active);    // binds "active": true
```

### POST JSON data

```cpp
int newId;

// Body: { "name": "Pedro", "role": "admin", "age": 21 }
client.post("/users")
      .body("name", "Pedro")
      .body("role", "admin")
      .body("age", 21)
      .getBody("id", &newId);
```

### Extracting nested fields

Use dot notation to navigate nested objects.

```cpp
char val[32];

// Response: { "level0": { "level1": "val2" } }
client.get("/nested")
      .getBody("level0.level1", val, sizeof(val));
```

### Extracting from arrays

Use a numeric index as a path segment to address array elements.

```cpp
char city[32];

// Response: [ { "address": { "city": "Gwenborough" } }, { "address": { "city": "Wisokyburgh" } } ]
client.get("/users")
      .getBody("1.address.city", city, sizeof(city)); // resolves the second element
```

### Extracting complete raw objects or arrays

Bind to an Arduino `String` to capture an entire object or sub-array for manual processing.

```cpp
String entireArray;
String specificUser;

client.get("/users")
      .getBody("", &entireArray)    // captures the root-level array
      .getBody("1", &specificUser); // captures the second user object
```

> [!WARNING]
> Pulling complete objects or arrays into an Arduino `String` causes dynamic memory reallocation as the raw JSON is copied character by character. Avoid this pattern with large payloads, as it can fragment or exhaust the device heap.

> [!NOTE]
> If a key is missing, misspelled, or the path does not exist in the response, the target variable is left unchanged. The library will not crash.

### Managing Connections (Keep-Alive)

By default, the client automatically maintains a persistent TCP/TLS connection across requests (HTTP Keep-Alive). This drastically improves performance for subsequent requests to the same server, but it holds onto the connection memory buffers (e.g., ~45KB for an active TLS tunnel). 
To manually close the connection and free this memory when you are done making requests, call `end()`:

```cpp
client.end(); // Closes the connection and frees TLS RAM
```

### PUT and DELETE

```cpp
// Update a resource
client.update("/lights/1").body("state", "OFF");

// Delete a resource
client.del("/logs/system_error.log");
```

---

## Examples

Runnable sketches are available in the `examples/` directory:

| Sketch | Description |
| :--- | :--- |
| [SimpleGET](examples/SimpleGET/SimpleGET.ino) | Basic data fetching with GET. |
| [PostRequest](examples/PostRequest/PostRequest.ino) | Sending a JSON payload with POST. |
| [PutRequest](examples/PutRequest/PutRequest.ino) | Updating a remote resource with PUT. |
| [DeleteRequest](examples/DeleteRequest/DeleteRequest.ino) | Deleting a remote resource. |
| [PortSelection](examples/PortSelection/PortSelection.ino) | Connecting to a server on a custom port. |
| [NestedJSON](examples/NestedJSON/NestedJSON.ino) | Extracting fields from deeply nested objects. |
| [ArrayJSON](examples/ArrayJSON/ArrayJSON.ino) | Addressing array elements by index. |
| [RawArrayJSON](examples/RawArrayJSON/RawArrayJSON.ino) | Capturing raw arrays or objects into Arduino Strings. |
| [UnixTimestamp](examples/UnixTimestamp/UnixTimestamp.ino) | Fetching the current Unix Timestamp from an API. |

---

## API Reference

### `ESP32HTTPClient` — Client class

The main entry point. Instantiate once with your base URL and reuse across requests.

#### Constructor

| Signature | Description | Example |
| :--- | :--- | :--- |
| `ESP32HTTPClient(baseUrl)` | Creates a client. Port defaults to 80 (HTTP) or 443 (HTTPS). | `ESP32HTTPClient client("https://api.example.com");` |
| `ESP32HTTPClient(baseUrl, port)` | Creates a client targeting a custom port. | `ESP32HTTPClient client("http://192.168.1.100", 8080);` |

#### HTTP request methods

Each method returns a `RestRequest` that can be chained with `.query()`, `.body()`, and `.getBody()`.

| Method | Description | Example |
| :--- | :--- | :--- |
| `get(path)` | Sends a GET request to `baseUrl + path`. | `client.get("/todos/1")` |
| `post(path)` | Sends a POST request to `baseUrl + path`. | `client.post("/users")` |
| `put(path)` | Sends a PUT request to `baseUrl + path`. | `client.put("/users/1")` |
| `update(path)` | Alias for `put()`. | `client.update("/lights/1")` |
| `patch(path)` | Sends a PATCH request to `baseUrl + path`. | `client.patch("/config/wifi")` |
| `del(path)` | Sends a DELETE request to `baseUrl + path`. | `client.del("/logs/old.log")` |

#### Configuration methods

| Method | Description | Example |
| :--- | :--- | :--- |
| `setContentType(contentType)` | Overrides the `Content-Type` header used for request bodies. Defaults to `application/json`. | `client.setContentType("application/x-www-form-urlencoded");` |
| `setHeader(name, value)` | Registers a custom HTTP header that is sent with every subsequent request. | `client.setHeader("Authorization", "Bearer mytoken123");` |
| `getStatusCode()` | Returns the HTTP status code of the last completed request. | `int code = client.getStatusCode();` |
| `end()` | Closes the persistent TCP/TLS connection and frees its memory buffers. Useful after a burst of requests. | `client.end();` |

---

### `RestRequest` — Fluent request builder

Returned by every HTTP method on `ESP32HTTPClient`. All builder methods return `RestRequest&`, enabling fluent chaining. The underlying HTTP request is dispatched on the first call to `.getBody()`, or automatically when the object goes out of scope.

#### Building the request

| Method | Description | Example |
| :--- | :--- | :--- |
| `query(key, value)` | Appends a URL query parameter. Supports `int`, `float`, `double`, `bool`, `long`, and `const char*`. Chainable. | `client.get("/search").query("q", "esp32").query("limit", 10)` |
| `body(key, value)` | Adds a field to the JSON request body. Supports the same types as `query()`. Chainable. | `client.post("/users").body("name", "Pedro").body("age", 21)` |

#### Extracting the response

`getBody()` is overloaded for each supported C type. It registers a binding between a JSON key path and a target variable. Use dot notation for nested fields and numeric segments for array indices.

| Method | Description | Example |
| :--- | :--- | :--- |
| `getBody(key, int* target)` | Binds a JSON integer to `*target`. | `client.get("/data").getBody("count", &myInt)` |
| `getBody(key, float* target)` | Binds a JSON number to a `float`. | `client.get("/sensor").getBody("temp", &myFloat)` |
| `getBody(key, double* target)` | Binds a JSON number to a `double`. | `client.get("/sensor").getBody("voltage", &myDouble)` |
| `getBody(key, bool* target)` | Binds a JSON boolean to `*target`. | `client.get("/status").getBody("active", &myBool)` |
| `getBody(key, long* target)` | Binds a JSON integer to a `long`. | `client.get("/stats").getBody("timestamp", &myLong)` |
| `getBody(key, char* target, size_t maxLen)` | Copies a JSON string into a char buffer, up to `maxLen` bytes. | `client.get("/user").getBody("name", myChar, sizeof(myChar))` |
| `getBody(key, String* target)` | Copies a raw JSON object or array into an Arduino `String`. Pass `""` to capture the entire response. | `client.get("/users").getBody("", &entireJson)` |

> [!NOTE]
> If a key is missing or the path does not exist, the target variable is left unchanged. No exception is thrown and no crash occurs.

#### Full chaining example

```cpp
int userId;
float temperature;
char city[32];

client.post("/report")
      .body("device", "esp32-cam")
      .body("floor", 3)
      .getBody("userId", &userId)            // int — root field
      .getBody("sensor.temp", &temperature)  // float — nested object
      .getBody("0.address.city", city, sizeof(city)); // char* — array index + nested
```

---

<p align="center">
  If this library saved you time, consider leaving a star ⭐ on the repository.
</p>
