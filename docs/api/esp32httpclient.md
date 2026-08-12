---
title: ESP32HTTPClient Class Reference - C++ Methods & Signatures
description: Complete API documentation for the ESP32HTTPClient class: constructor, HTTP verb methods, persistent headers, authentication, and timeouts.
keywords: ESP32HTTPClient class, ESP32HTTPClient constructor, C++ ESP32 HTTP library reference, ESP32 HTTP client methods
tags:
  - api
  - client
  - class
---
# ESP32HTTPClient

The main entry point for the library. Create one instance per server base URL and reuse it across all requests.

**Header:** `#include "ESP32HTTPClient.h"`

---

## Constructor

### `ESP32HTTPClient(baseUrl)`

Creates a client with automatic port selection (80 for HTTP, 443 for HTTPS).

```cpp
ESP32HTTPClient(const char* baseUrl);
```

**Parameters:**

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `baseUrl` | `const char*` | The base URL including protocol (e.g., `"https://api.example.com"`). Do not include a trailing slash. |

**Example:**
```cpp
ESP32HTTPClient client("https://api.example.com");
```

---

### `ESP32HTTPClient(baseUrl, port)`

Creates a client targeting a specific port.

```cpp
ESP32HTTPClient(const char* baseUrl, int port);
```

**Parameters:**

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `baseUrl` | `const char*` | The base URL including protocol. |
| `port` | `int` | The target TCP port (e.g., `8080`, `443`). |

**Example:**
```cpp
ESP32HTTPClient client("http://192.168.1.100", 8080);
```

---

## HTTP Request Methods

Each method returns a [`RestRequest`](restrequest.md) that can be chained with `.query()`, `.body()`, and `.getBody()`. The HTTP request is dispatched when the `RestRequest` object goes out of scope or when the first `.getBody()` is added.

---

### `get(path)`

Sends a `GET` request to `baseUrl + path`.

```cpp
RestRequest get(const char* path);
```

**Example:**
```cpp
client.get("/todos/1").getBody("title", title, sizeof(title));
```

---

### `post(path)`

Sends a `POST` request to `baseUrl + path`.

```cpp
RestRequest post(const char* path);
```

**Example:**
```cpp
client.post("/users").body("name", "Pedro").body("age", 21).getBody("id", &newId);
```

---

### `put(path)`

Sends a `PUT` request to `baseUrl + path`.

```cpp
RestRequest put(const char* path);
```

**Example:**
```cpp
client.put("/posts/1").body("title", "new title");
```

---

### `update(path)`

Semantic alias for `put()`. Sends an identical `HTTP PUT` request.

```cpp
RestRequest update(const char* path);
```

**Example:**
```cpp
client.update("/lights/1").body("state", "OFF");
```

---

### `patch(path)`

Sends a `PATCH` request to `baseUrl + path` for partial updates.

```cpp
RestRequest patch(const char* path);
```

**Example:**
```cpp
client.patch("/config").body("timeout", 30);
```

---

### `del(path)`

Sends a `DELETE` request to `baseUrl + path`.

```cpp
RestRequest del(const char* path);
```

**Example:**
```cpp
client.del("/sessions/42");
```

---

## Configuration Methods

---

### `setHeader(name, value)`

Registers a custom HTTP header that is sent with **every subsequent request**.

```cpp
void setHeader(const char* name, const char* value);
```

| Parameter | Limit |
| :--- | :--- |
| `name` | Up to 63 characters |
| `value` | Up to 255 characters |

**Example:**
```cpp
client.setHeader("Authorization", "Bearer my-token");
client.setHeader("X-Device-ID",   "ESP32-001");
```

!!! note
    Headers persist for the lifetime of the client instance. Call `setHeader()` again with the same name to overwrite.

!!! tip "Reading response headers"
    `setHeader()` sets headers to be sent in the **request**. To read headers returned by the server in the **response**, use [RestRequest::getHeader](restrequest.md#extracting-response-headers).

---

### `bearer(token)`

Sets the `Authorization: Bearer <token>` header sent with **every subsequent request**.

```cpp
void bearer(const char* token);
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `token` | `const char*` | The Bearer / JWT token string. |

**Example:**
```cpp
client.bearer("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...");
```

---

### `basic(user, password)`

Encodes credentials into Base64 and sets the `Authorization: Basic <base64>` header sent with **every subsequent request**.

```cpp
void basic(const char* user, const char* password);
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `user` | `const char*` | Username. |
| `password` | `const char*` | Password. |

**Example:**
```cpp
client.basic("admin", "secret123");
```

---

### `apiKey(name, key)`

Sets an API key header (e.g. `X-API-Key`) sent with **every subsequent request**.

```cpp
void apiKey(const char* name, const char* key);
```

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `name` | `const char*` | Header name (e.g., `"X-API-Key"` or `"x-api-key"`). |
| `key` | `const char*` | API key string. |

**Example:**
```cpp
client.apiKey("x-api-key", "my-secret-api-key");
```

---

### `cookie(name, value)`

Appends or creates a `Cookie` header on the client. Returns a reference to `ESP32HTTPClient`, allowing you to chain `.get()`, `.post()`, or even other `.cookie()` calls directly on the client.

```cpp
ESP32HTTPClient& cookie(const char* name, const char* value);
```

**Parameters:**
*   `name`: The name of the cookie (e.g., `"session_id"`).
*   `value`: The value of the cookie (e.g., `"abc1234"`).

**Example:**
```cpp
client.cookie("session_id", "abc1234")
      .cookie("device_id", "esp32-01")
      .get("/profile");
```

---

### `setBaseUrl(baseUrl, port)`

Changes the base URL and target port at runtime for subsequent requests.

```cpp
void setBaseUrl(const char* baseUrl, int port = 0);
```

**Example:**
```cpp
client.setBaseUrl("https://api.v2.example.com", 443);
```

---

### `setUrl(baseUrl)`

Convenience alias for updating only the base URL at runtime.

```cpp
void setUrl(const char* baseUrl);
```

---

### `setPort(port)`

Updates the target TCP port at runtime.

```cpp
void setPort(int port);
```

---

### `getBaseUrl()`

Returns the current base URL string.

```cpp
const char* getBaseUrl() const;
```

---

### `getPort()`

Returns the current target TCP port (or 0 if default).

```cpp
int getPort() const;
```

---

### `setTimeout(timeoutMs)`

Configures the default network timeout in milliseconds for all requests made by this client. Default is `60000` ms (1 minute).

```cpp
void setTimeout(uint16_t timeoutMs);
```

**Example:**
```cpp
client.setTimeout(10000); // 10 seconds
```

---

### `getTimeout()`

Returns the configured default timeout in milliseconds.

```cpp
uint16_t getTimeout() const;
```

---

### `setMaxRetry(maxRetry)`

Configures the default maximum number of automatic retries on network failures. Default is `1` retry.

```cpp
void setMaxRetry(int maxRetry);
```

**Example:**
```cpp
client.setMaxRetry(3); // Up to 3 retries
```

---

### `getMaxRetry()`

Returns the configured default max retry count.

```cpp
int getMaxRetry() const;
```

---

### `setContentType(contentType)`

Overrides the `Content-Type` header used for request bodies. Defaults to `application/json`.

```cpp
void setContentType(const char* contentType);
```

**Example:**
```cpp
client.setContentType("application/x-www-form-urlencoded");
```

---

## Response & Error Inspection

---

### `getStatusCode()`

Returns the HTTP status code of the **last completed request**.

```cpp
int getStatusCode() const;
```

**Return values:**

| Value | Meaning |
| :--- | :--- |
| `> 0` | Standard HTTP status code (200, 201, 404, 500…) |
| `< 0` | Network-level error (no connection, timeout, etc.) |
| `0` | No request has been made yet |

---

### `isSuccess()`

Returns `true` if the last request completed with a 2xx HTTP status code (`200 <= code < 300`).

```cpp
bool isSuccess() const;
```

**Example:**
```cpp
client.get("/data").getBody("val", &val);
if (client.isSuccess()) {
    Serial.println("Request succeeded!");
}
```

---

### `hasError()`

Returns `true` if the last request failed due to a network error (`code < 0`) or an HTTP client/server error (`code >= 400`).

```cpp
bool hasError() const;
```

**Example:**
```cpp
client.get("/data").getBody("val", &val);
if (client.hasError()) {
    Serial.printf("Error (%d): %s\n", client.getStatusCode(), client.getErrorMessage().c_str());
}
```

---

### `getErrorMessage()`

Returns a human-readable description of the last status or error code.

```cpp
String getErrorMessage() const;
```

---

### `errorToString(code)`

Static helper that converts any HTTP status code or client negative error code into a descriptive string.

```cpp
static String errorToString(int code);
```

---

## Struct Serialization & Deserialization

Static utility methods to convert mapped C++ structs to and from JSON strings.

---

### `toJson(struct)`

Serializes a struct mapped with `REST_JSON_MAP` into a JSON string.

```cpp
template <typename T>
static String toJson(const T& obj);
```

**Example:**
```cpp
User user = {15, "Pedro", 9.5f, true};
String json = ESP32HTTPClient::toJson(user);
```

---

### `fromJson(json, struct)`

Deserializes a JSON string into a target struct mapped with `REST_JSON_MAP`.

```cpp
template <typename T>
static void fromJson(const String& json, T* target);
template <typename T>
static void fromJson(const char* json, T* target);
```

**Example:**
```cpp
User user;
ESP32HTTPClient::fromJson("{\"id\":15,\"name\":\"Pedro\"}", &user);
```

---

## Callbacks

You can register global callbacks on the client instance that are executed whenever any request completes.

---

### `onSuccess(callback)`

Registers a callback executed when any request finishes with a 2xx HTTP status code (`200 <= code < 300`).

```cpp
void onSuccess(HttpResponseCallback cb);
```

**Example:**
```cpp
client.onSuccess([](int code) {
    Serial.printf("Client request succeeded with status %d\n", code);
});
```

---

### `onError(callback)`

Registers a callback executed when any request fails with an error (`code < 200 || code >= 400`).

```cpp
void onError(HttpErrorCallback cb);
void onError(HttpResponseCallback cb);
```

**Example:**
```cpp
client.onError([](int code, const char* message) {
    Serial.printf("Client request failed (%d): %s\n", code, message);
---

### `onResponse(cb)`

Registers a global callback that is invoked after any request finishes, providing the HTTP status code.

```cpp
void onResponse(HttpResponseCallback cb);
```

**Example:**
```cpp
client.onResponse([](int code) {
    Serial.printf("Request completed with status: %d\n", code);
});
```

---

### `onObservability(cb)`

Registers a global callback that is invoked at the end of each request, providing performance metrics (timings, payload sizes, heap usage).

```cpp
void onObservability(ObservabilityCallback cb);
```

**Struct Definition (`ObservabilityMetrics`):**
```cpp
struct ObservabilityMetrics {
    unsigned long totalTimeMs;
    unsigned long ttfbMs;
    size_t txBytes;
    size_t rxBytes;
    int retries;
    uint32_t freeHeapBefore;
    uint32_t freeHeapAfter;
};
```

**Example:**
```cpp
client.onObservability([](const ObservabilityMetrics& m) {
    Serial.printf("TTFB: %lu ms | TX: %d | RX: %d\n", m.ttfbMs, m.txBytes, m.rxBytes);
});
```

---

## Connection Management

---

### `end()`

Closes the persistent TCP/TLS Keep-Alive connection and frees its memory buffers.

```cpp
void end();
```

Call this after a burst of requests to reclaim ~45KB of TLS memory during a long idle period. The next request will automatically re-establish the connection.

**Example:**
```cpp
client.get("/data1").getBody("v", &v1);
client.get("/data2").getBody("v", &v2);

client.end(); // free TLS memory
delay(60000);

client.get("/data3").getBody("v", &v3); // reconnects automatically
```

---

## Error Codes Reference Table

### Network / Client Error Codes (`code < 0`)

| Code | Constant | Description |
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

### Common HTTP Status Codes (`code > 0`)

| Code | Status | Description |
| :--- | :--- | :--- |
| `200` | OK | Request succeeded normally. |
| `201` | Created | Resource was created successfully. |
| `202` | Accepted | Request accepted for asynchronous processing. |
| `204` | No Content | Request succeeded, server returned empty response. |
| `400` | Bad Request | Malformed request or invalid payload syntax. |
| `401` | Unauthorized | Missing or invalid authentication credentials. |
| `403` | Forbidden | Authenticated, but lacking permissions for resource. |
| `404` | Not Found | Requested endpoint path does not exist on server. |
| `408` | Request Timeout | Server timed out waiting for request completion. |
| `429` | Too Many Requests | Rate limit exceeded. |
| `500` | Internal Server Error | Generic unhandled server-side error. |
| `502` | Bad Gateway | Upstream server returned an invalid response. |
| `503` | Service Unavailable | Server overloaded or down for maintenance. |
| `504` | Gateway Timeout | Upstream gateway timed out waiting for response. |
