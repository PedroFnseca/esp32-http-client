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

**Example:**
```cpp
client.get("/health");
if (client.getStatusCode() == 200) {
    Serial.println("OK");
}
```

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
