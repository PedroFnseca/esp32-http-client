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
client.del("/users/15");
```

---

### `soap(path)`

Initiates a SOAP request targeting `baseUrl + path`, returning a [`SoapRequest`](soaprequest.md) builder.

```cpp
SoapRequest soap(const char* path = "");
```

**Example:**
```cpp
client.soap("/ws")
      .soapAction("http://example.org/GetPrice")
      .body("<m:GetPrice xmlns:m=\"http://example.org\"><m:Item>Widget</m:Item></m:GetPrice>")
      .getBody("Price", &price);
```

---

### `graphql(path)` / `graphqlPost(path)`

Initiates a GraphQL request targeting `baseUrl + path` (defaults to `POST`), returning a [`GraphQLRequest`](graphqlrequest.md) builder.

```cpp
GraphQLRequest graphql(const char* path = "/graphql");
GraphQLRequest graphqlPost(const char* path = "/graphql");
```

**Example:**
```cpp
client.graphql("/graphql")
      .query("query { user { id name } }")
      .getData("user.name", &name);
```

---

### `graphqlGet(path)`

Initiates a GraphQL query targeting `baseUrl + path` via HTTP GET, formatting the query and variables as URL query parameters.

```cpp
GraphQLRequest graphqlGet(const char* path = "/graphql");
```

**Example:**
```cpp
client.graphqlGet("/graphql")
      .query("{ systemStatus }")
      .getData("systemStatus", &status);
```


---

### `jsonRpc(path)` / `jsonrpc(path)`

Initiates a JSON-RPC 2.0 request targeting `baseUrl + path`, returning a [`JsonRpcRequest`](jsonrpcrequest.md) builder.

```cpp
JsonRpcRequest jsonRpc(const char* path = "");
JsonRpcRequest jsonrpc(const char* path = "");
```

**Example:**
```cpp
int sum = 0;
client.jsonRpc("/rpc")
      .method("add")
      .param(15)
      .param(27)
      .id(1)
      .getResult(&sum);
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

### `setFollowRedirects(follow)`

Configures whether HTTP redirects are followed. Redirects are disabled by default.

```cpp
void setFollowRedirects(followRedirects_t follow);
```

Supported modes are:

- `HTTPC_DISABLE_FOLLOW_REDIRECTS`: do not follow redirects.
- `HTTPC_STRICT_FOLLOW_REDIRECTS`: follow only redirects for `GET` and `HEAD`
  requests, using the same method.
- `HTTPC_FORCE_FOLLOW_REDIRECTS`: follow redirects for every HTTP method,
  preserving the method, request body, and headers.

**Example:**
```cpp
client.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
```

---

### `setRedirectLimit(limit)`

Sets the maximum number of redirects followed per request. The default is `10`.

```cpp
void setRedirectLimit(uint16_t limit);
```

**Example:**
```cpp
client.setRedirectLimit(5);
```

---

### `getRedirectLimit()`

Returns the configured maximum number of redirects.

```cpp
uint16_t getRedirectLimit() const;
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

## TLS / HTTPS Configuration

The library provides robust TLS/HTTPS support powered by the underlying ESP32 mbedTLS stack (`NetworkClientSecure` on core 3.x, `WiFiClientSecure` on core 2.x).

> [!IMPORTANT]
> **Pointer Lifetime Semantics:** Certificates and keys are stored as **pointers**, not copied into internal memory. The buffers MUST remain valid for the entire lifetime of the `ESP32HTTPClient` instance (e.g., declared as `static const char*`, global constants, or stored in PROGMEM).
>
> **NTP Time Synchronization:** TLS certificate validation verifies the certificate's validity period (`notBefore` and `notAfter`). If the system clock is not synchronized via NTP (defaulting to Unix epoch 0 in 1970), certificate validation **will fail**. Synchronize time using `configTime(0, 0, "pool.ntp.org")` after WiFi connects.
>
> **Memory Impact:** TLS connections allocate mbedTLS buffers (~45 KB of heap) on demand during the first handshake. Plain `http://` requests or unconfigured clients incur zero TLS heap overhead. Call `client.end()` when done to immediately close the socket and free mbedTLS buffers.

---

### `setCACert(rootCA)`

Configures a custom root CA certificate in PEM format for validating the server's certificate chain.

```cpp
void setCACert(const char* rootCA);
```

**Parameters:**

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `rootCA` | `const char*` | Null-terminated string containing the Root CA certificate in PEM format (`"-----BEGIN CERTIFICATE-----\n..."`). Pointer is stored, not copied. |

**Example:**
```cpp
static const char* isrg_root_x1 = "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----\n";
client.setCACert(isrg_root_x1);
```

---

### `setCACertBundle(bundle, size)`

Configures an X.509 certificate bundle (ESP-IDF `esp_crt_bundle` format).

```cpp
void setCACertBundle(const uint8_t* bundle, size_t size = 0);
```

**Parameters:**

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `bundle` | `const uint8_t*` | Pointer to the certificate bundle data. |
| `size` | `size_t` | Optional size of the bundle buffer in bytes (used in arduino-esp32 >= 3.0.4). |

---

### `setClientCert(cert, key)`

Configures mutual TLS (mTLS) client certificate and private key.

```cpp
void setClientCert(const char* cert, const char* key);
```

**Parameters:**

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `cert` | `const char*` | Client certificate in PEM format. Stored by pointer. |
| `key` | `const char*` | Client private key in PEM format. Stored by pointer. |

---

### `setInsecure()`

Explicitly disables server certificate and hostname verification, and emits a security warning log (`log_w`).

> [!WARNING]
> Calling `setInsecure()` disables TLS authentication and exposes all communication to Man-in-the-Middle (MITM) attacks. Use this only for local development, testing, or environments where certificate validation is impossible. If `requireValidatedTls(true)` is active, HTTPS requests will be refused (`-12`) even if `setInsecure()` was called.

```cpp
void setInsecure();
```

---

### `requireValidatedTls(require)`

Enables or disables **fail-closed TLS mode**. When enabled (`true`), any `https://` request made without a configured Root CA (`setCACert`) or CA bundle (`setCACertBundle`), or while `setInsecure()` is active, is immediately refused before opening a socket and sets status code `HTTPC_ERROR_TLS_NOT_VALIDATED` (`-12`).

Can also be enabled globally at compile time via `-DESP32HTTPCLIENT_REQUIRE_VALIDATED_TLS=1`.

```cpp
void requireValidatedTls(bool require = true);
```

**Parameters:**

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `require` | `bool` | `true` (default) to enforce validated TLS on all `https://` requests; `false` to allow fallback with a warning log. |

**Example:**
```cpp
client.requireValidatedTls(true);
client.setCACert(root_ca_pem);
```

---

### `isValidatedTlsRequired()`

Returns `true` if fail-closed TLS validation mode is enabled on this client instance.

```cpp
bool isValidatedTlsRequired() const;
```

---

### `hasValidatedTlsConfig()`

Returns `true` if a non-empty Root CA (`setCACert`) or CA bundle (`setCACertBundle`) is configured and `setInsecure()` is not active.

```cpp
bool hasValidatedTlsConfig() const;
```

---

### `end()`

Closes the active connection and releases associated resources. For TLS connections, this closes the secure socket and frees mbedTLS internal buffers (~45 KB). The client remains valid and can open a new connection on the next request.

```cpp
void end();
```

---

### `getTlsError()`

Returns the underlying mbedTLS error code (`int`) from the last TLS handshake or connection attempt (e.g., negative values such as `MBEDTLS_ERR_X509_CERT_VERIFY_FAILED`). Returns `0` if no TLS error occurred.

```cpp
int getTlsError() const;
```

---

### `getTlsErrorMessage()`

Returns a human-readable description of the last mbedTLS error code, formatted with hexadecimal error code representation.

```cpp
String getTlsErrorMessage() const;
```

---

### `getTlsErrorHint()`

Returns an actionable troubleshooting hint for the last TLS handshake failure (or fail-closed rejection), inspecting both the mbedTLS error code and the current system clock (`time(nullptr)`). For example, if the system clock is before 2023, it advises synchronizing time via NTP (`configTime()`). Returns `""` if no hint applies.

```cpp
String getTlsErrorHint() const;
```

**Example:**
```cpp
if (client.hasError() && client.getTlsError() != 0) {
    Serial.printf("TLS Hint: %s\n", client.getTlsErrorHint().c_str());
}
```

---

### `tlsErrorToHint(tlsError, currentEpoch)`

Static helper that maps an mbedTLS error code (and optional Unix timestamp `currentEpoch`; pass `-1` to query `time(nullptr)` automatically) to an actionable diagnostic hint string.

```cpp
static String tlsErrorToHint(int tlsError, long currentEpoch = -1);
```

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

Returns a human-readable description of the last status or error code. When an HTTPS request fails with a negative status code, `getErrorMessage()` automatically appends the mbedTLS error message, hex code, and actionable diagnostic hint (`getTlsErrorHint()`).

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
});
```

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

## Error Codes and HTTP Status Codes

| Code | Meaning | Category |
| :--- | :--- | :--- |
| `-1` | Connection Refused | Client error |
| `-2` | Send Header Failed | Client error |
| `-3` | Send Payload Failed | Client error |
| `-4` | Not Connected | Client error |
| `-5` | Connection Lost | Client error |
| `-6` | No Stream | Client error |
| `-7` | No HTTP Server | Client error |
| `-8` | Too Less RAM | Client error |
| `-9` | Encoding Error | Client error |
| `-10` | Stream Write Error | Client error |
| `-11` | Read Timeout | Client error |
| `-12` | TLS Validation Required | Client error (fail-closed TLS) |
| `200` | OK | HTTP success |
| `201` | Created | HTTP success |
| `202` | Accepted | HTTP success |
| `204` | No Content | HTTP success |
| `400` | Bad Request | HTTP client error |
| `401` | Unauthorized | HTTP client error |
| `403` | Forbidden | HTTP client error |
| `404` | Not Found | HTTP client error |
| `405` | Method Not Allowed | HTTP client error |
| `408` | Request Timeout | HTTP client error |
| `409` | Conflict | HTTP client error |
| `429` | Too Many Requests | HTTP client error |
| `500` | Internal Server Error | HTTP server error |
| `501` | Not Implemented | HTTP server error |
| `502` | Bad Gateway | HTTP server error |
| `503` | Service Unavailable | HTTP server error |
| `504` | Gateway Timeout | HTTP server error |
| `0` | Not Executed | Internal state |

### Generic Fallback Behavior

- **Negative unknown codes** → Unknown Client Error
- **200–299** → Success
- **300–399** → Redirection
- **400–499** → Client Error
- **500–599** → Server Error
- **Other values** → Unknown HTTP Status

### Example

A short usage example showing how applications can handle both transport errors and HTTP errors:

```cpp
int status = client.get("/api/data").getStatusCode();

if (client.isSuccess()) {
    // Handle successful response
} else {
    Serial.println(client.getErrorMessage());
}
```
