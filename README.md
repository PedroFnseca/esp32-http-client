# [ESP32 HTTP Client](https://esp32httpclient.com/) the Easy Way & Low Memory Footprint
**A high-performance, fluent, and object-oriented HTTP client for ESP32 with direct JSON binding capabilities.**

## **[Official Documentation](https://esp32httpclient.com/)** Get started quickly with the [Quick Start Guide](https://esp32httpclient.com/getting-started/quickstart) or explore the [API Reference](https://esp32httpclient.com/api/esp32httpclient/) for detailed usage.

[![Arduino Library](https://img.shields.io/github/v/release/PedroFnseca/esp32-http-client?color=00979D&label=Arduino&logo=arduino&logoColor=white)](https://github.com/PedroFnseca/esp32-http-client)
[![PlatformIO Registry](https://img.shields.io/github/v/release/PedroFnseca/esp32-http-client?color=f58220&label=PlatformIO&logo=platformio&logoColor=white)](https://github.com/PedroFnseca/esp32-http-client)
[![Language](https://img.shields.io/github/languages/top/PedroFnseca/esp32-http-client)](https://github.com/PedroFnseca/esp32-http-client)
[![Coverage](https://img.shields.io/badge/Coverage-88.03%25-brightgreen)](https://github.com/PedroFnseca/esp32-http-client)
[![Hits](https://hits.sh/github.com/PedroFnseca/esp32-http-client.svg?view=today-total)](https://hits.sh/github.com/PedroFnseca/esp32-http-client/)
[![License](https://img.shields.io/github/license/PedroFnseca/esp32-http-client)](LICENSE)
[![Stars](https://img.shields.io/github/stars/PedroFnseca/esp32-http-client?style=social)](https://github.com/PedroFnseca/esp32-http-client/stargazers)

---

## Table of Contents

- [Why this library?](#why-this-library)
- [Performance & Comparison](#performance--comparison)
- [Key Features](#key-features)
- [Installation](#installation)
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
- **Direct injection** — JSON values are written straight into standard C types (`int`, `float`, `bool`, `char*`) or C++ `struct`s.
- **Zero buffering** — the response stream is parsed in place; the full payload is never stored.
- **Struct <-> JSON mapping** — direct bidirectional struct serialization/deserialization without dynamic document allocations.
- **Full REST support** — `GET`, `POST`, `PUT`, `PATCH`, and `DELETE` are all first-class citizens.
- **IoT ready** — designed for connecting ESP32 devices to cloud backends, Firebase, AWS API Gateway, or custom servers.

---

## Installation

### PlatformIO

Add `ESP32-HTTP-Client` to the `lib_deps` section of your `platformio.ini`:

```ini
lib_deps =
    PedroFnseca/ESP32-HTTP-Client@^1.4.0
```

### Arduino Library Manager

1. Open Arduino IDE and go to **Sketch → Include Library → Manage Libraries...**.
2. Search for `ESP32-HTTP-Client`.
3. Click **Install**.

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

### Query Parameters

```cpp
// Produces: GET /users?page=2&limit=20&search=pedro
client.get("/users")
      .query("page", 2)
      .query("limit", 20)
      .query("search", "pedro");
```

### Path Parameters

Replace `{placeholder}` segments dynamically in the request path:

```cpp
// Produces: GET /users/15
client.get("/users/{id}")
      .path("id", 15);
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

### Authentication Helpers

Easily authenticate requests using built-in helpers for Bearer tokens, Basic Auth, or API keys:

```cpp
// Bearer / JWT Token
client.bearer("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...");

// HTTP Basic Auth (automatically Base64 encoded)
client.basic("admin", "secret123");

// API Key header
client.apiKey("x-api-key", "my-secret-api-key");
```

### Struct <-> JSON Mapping

Direct bidirectional mapping between C++ `struct`s and JSON payloads:

```cpp
struct User {
    int id = 0;
    char name[32] = {0};
    bool active = false;

    REST_JSON_MAP(
        REST_FIELD(id),
        REST_FIELD(name),
        REST_FIELD(active)
    )
};

// Send struct as JSON body
User newUser = {1, "Pedro", true};
client.post("/users").body(newUser);

// Receive response directly into struct
User fetchedUser;
client.get("/users/1").getBody(&fetchedUser);
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
| [RestCrud](examples/RestCrud/RestCrud.ino) | Full suite of REST CRUD operations (`GET`, `POST`, `PUT`, `PATCH`, `DELETE`) in a single sketch. |
| [StructJson](examples/StructJson/StructJson.ino) | Bidirectional C++ Struct <-> JSON serialization and HTTP transfer. |
| [UrlParameters](examples/UrlParameters/UrlParameters.ino) | Path parameters (`/users/{id}`) and query parameters (`?page=2`). |
| [AuthHelpers](examples/AuthHelpers/AuthHelpers.ino) | Bearer token, Basic Auth, and API Key authentication headers. |
| [CallbacksAndErrors](examples/CallbacksAndErrors/CallbacksAndErrors.ino) | Callbacks, timeouts, automatic retries, error handling, and runtime URL change. |
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

#### Configuration & Authentication methods

| Method | Description | Example |
| :--- | :--- | :--- |
| `bearer(token)` | Sets persistent Bearer token authorization header. | `client.bearer("my-token");` |
| `basic(user, password)` | Sets persistent Basic authorization header (auto Base64 encoded). | `client.basic("admin", "123456");` |
| `apiKey(name, key)` | Sets persistent API key header. | `client.apiKey("x-api-key", "my-key");` |
| `setHeader(name, value)` | Registers a custom HTTP header that is sent with every subsequent request. | `client.setHeader("Authorization", "Bearer mytoken123");` |
| `setBaseUrl(url, port)` | Changes the base URL and target port at runtime. | `client.setBaseUrl("https://api.v2.com", 443);` |
| `setUrl(url)` | Changes the base URL at runtime. | `client.setUrl("http://192.168.1.100");` |
| `setPort(port)` | Changes the target TCP port at runtime. | `client.setPort(8080);` |
| `setTimeout(timeoutMs)` | Sets default request timeout in milliseconds (default: 60000, 1 min). | `client.setTimeout(10000);` |
| `setMaxRetry(maxRetry)` | Sets default max retries on network failure (default: 1). | `client.setMaxRetry(3);` |
| `setContentType(contentType)` | Overrides the `Content-Type` header used for request bodies. Defaults to `application/json`. | `client.setContentType("application/x-www-form-urlencoded");` |
| `getStatusCode()` | Returns the HTTP status code of the last completed request. | `int code = client.getStatusCode();` |
| `isSuccess()` | Returns `true` if last request status was 2xx (`200 <= code < 300`). | `if (client.isSuccess()) { ... }` |
| `hasError()` | Returns `true` if last request had network error or HTTP error (`code >= 400`). | `if (client.hasError()) { ... }` |
| `getErrorMessage()` | Returns descriptive error string for last status or error code. | `String err = client.getErrorMessage();` |
| `onSuccess(cb)` | Registers client-level callback for successful requests (2xx). | `client.onSuccess([](int code){ ... });` |
| `onError(cb)` | Registers client-level callback for failed requests. | `client.onError([](int code, const char* msg){ ... });` |
| `onResponse(cb)` | Registers client-level callback executed on every completed request. | `client.onResponse([](int code){ ... });` |
| `toJson(struct)` | Static utility to serialize a mapped struct into JSON string. | `String json = ESP32HTTPClient::toJson(user);` |
| `fromJson(json, struct)` | Static utility to populate a struct from a JSON string. | `ESP32HTTPClient::fromJson(json, &user);` |
| `end()` | Closes the persistent TCP/TLS connection and frees its memory buffers. Useful after a burst of requests. | `client.end();` |

---

### `RestRequest` — Fluent request builder

Returned by every HTTP method on `ESP32HTTPClient`. All builder methods return `RestRequest&`, enabling fluent chaining. The underlying HTTP request is dispatched on the first call to `.getBody()`, or automatically when the object goes out of scope.

#### Building the request

| Method | Description | Example |
| :--- | :--- | :--- |
| `path(key, value)` | Replaces a `{placeholder}` in the URL path. Supports `String`, `const char*`, `int`, `long`, `float`, `double`, and `bool`. Chainable. | `client.get("/users/{id}").path("id", 15)` |
| `query(key, value)` | Appends a URL query parameter. Supports `String`, `const char*`, `int`, `long`, `float`, `double`, and `bool`. Chainable. | `client.get("/users").query("page", 2).query("limit", 20)` |
| `body(key, value)` | Adds a field to the JSON request body. Supports the same types as `query()`. Chainable. | `client.post("/users").body("name", "Pedro").body("age", 21)` |
| `body(struct)` | Sets full JSON request body serialized from a mapped struct. Chainable. | `client.post("/users").body(user)` |
| `timeout(ms)` | Overrides timeout for this specific request in milliseconds. Chainable. | `client.get("/data").timeout(2000)` |
| `retry(maxRetry)` | Overrides max retry attempts for this specific request. Chainable. | `client.get("/data").retry(3)` |
| `onSuccess(cb)` | Per-request success callback (2xx). Chainable. | `client.get("/users").onSuccess([](int c){ ... })` |
| `onError(cb)` | Per-request error callback (`code < 200 \|\| code >= 400`). Chainable. | `client.get("/users").onError([](int c, const char* m){ ... })` |
| `onResponse(cb)` | Per-request callback executed on completion. Chainable. | `client.get("/users").onResponse([](int c){ ... })` |

#### Extracting the response

`getBody()` is overloaded for each supported C type. It registers a binding between a JSON key path and a target variable. Use dot notation for nested fields and numeric segments for array indices.

`getHeader()` is overloaded for each supported C type to extract HTTP response headers directly (e.g., `token`, `Content-Type`, `Date`). Header lookups are case-insensitive.

| Method | Description | Example |
| :--- | :--- | :--- |
| `getBody(key, int* target)` | Binds a JSON integer to `*target`. | `client.get("/data").getBody("count", &myInt)` |
| `getBody(key, float* target)` | Binds a JSON number to a `float`. | `client.get("/sensor").getBody("temp", &myFloat)` |
| `getBody(key, double* target)` | Binds a JSON number to a `double`. | `client.get("/sensor").getBody("voltage", &myDouble)` |
| `getBody(key, bool* target)` | Binds a JSON boolean to `*target`. | `client.get("/status").getBody("active", &myBool)` |
| `getBody(key, long* target)` | Binds a JSON integer to a `long`. | `client.get("/stats").getBody("timestamp", &myLong)` |
| `getBody(key, char* target, size_t maxLen)` | Copies a JSON string into a char buffer, up to `maxLen` bytes. | `client.get("/user").getBody("name", myChar, sizeof(myChar))` |
| `getBody(key, String* target)` | Copies a raw JSON object or array into an Arduino `String`. Pass `""` to capture the entire response. | `client.get("/users").getBody("", &entireJson)` |
| `getBody(struct* target)` | Binds and populates a mapped struct directly from root JSON response. | `client.get("/users/1").getBody(&user)` |
| `getBody(key, struct* target)` | Binds and populates a mapped struct from nested JSON object path. | `client.get("/profile").getBody("data.user", &user)` |
| `getHeader(name, target)` | Extracts an HTTP response header into `target` (`String*`, `char*`/`char[N]`, `int*`, `long*`, `float*`, `double*`, `bool*`). Case-insensitive. | `client.get("/auth").getHeader("token", &token)` |

> [!NOTE]
> If a key or header is missing, the target variable is left unchanged. No exception is thrown and no crash occurs.

#### Full chaining example

```cpp
int userId;
float temperature;
char city[32];
String token;

client.post("/report")
      .body("device", "esp32-cam")
      .body("floor", 3)
      .timeout(3000)
      .retry(2)
      .onSuccess([](int code) { Serial.printf("OK: %d\n", code); })
      .onError([](int code, const char* msg) { Serial.printf("Fail (%d): %s\n", code, msg); })
      .getHeader("token", &token)              // String — response header
      .getBody("userId", &userId)            // int — root field
      .getBody("sensor.temp", &temperature)  // float — nested object
      .getBody("0.address.city", city, sizeof(city)); // char* — array index + nested
```

---

## Error Codes Reference

| Error Code | Constant / Status | Description |
| :--- | :--- | :--- |
| `-1` | `HTTPC_ERROR_CONNECTION_REFUSED` | Target host rejected the TCP connection. |
| `-2` | `HTTPC_ERROR_SEND_HEADER_FAILED` | Failed to write HTTP headers to the socket. |
| `-3` | `HTTPC_ERROR_SEND_PAYLOAD_FAILED` | Failed to transmit request body payload. |
| `-4` | `HTTPC_ERROR_NOT_CONNECTED` | Client is not connected to a network/socket. |
| `-5` | `HTTPC_ERROR_CONNECTION_LOST` | TCP connection terminated unexpectedly. |
| `-6` | `HTTPC_ERROR_NO_STREAM` | No response stream available from client. |
| `-7` | `HTTPC_ERROR_NO_HTTP_SERVER` | Server did not respond with valid HTTP. |
| `-8` | `HTTPC_ERROR_TOO_LESS_RAM` | Insufficient free heap memory for operation. |
| `-9` | `HTTPC_ERROR_ENCODING` | Transfer encoding or decoding error. |
| `-10` | `HTTPC_ERROR_STREAM_WRITE` | Stream write operation failed. |
| `-11` | `HTTPC_ERROR_READ_TIMEOUT` | Exceeded timeout waiting for server data. |
| `200` | OK | Request succeeded normally. |
| `400` | Bad Request | Malformed request or invalid payload syntax. |
| `401` | Unauthorized | Missing or invalid authentication credentials. |
| `404` | Not Found | Requested endpoint path does not exist on server. |
| `500` | Internal Server Error | Generic unhandled server-side error. |

---

<p align="center">
  If this library saved you time, consider leaving a star ⭐ on the repository.
</p>
