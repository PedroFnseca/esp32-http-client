---
title: Advanced ESP32 HTTP Client Features - Auth, Callbacks & Retries
description: Advanced HTTP client patterns on ESP32: Bearer JWT & Basic Auth, request callbacks, custom headers, Keep-Alive, timeout retries, and dynamic URLs.
keywords: ESP32 HTTP authentication, ESP32 Bearer token, HTTP headers ESP32, Keep-Alive ESP32, request callbacks ESP32
tags:
  - guide
  - advanced
---
# Advanced Usage

## Authentication Helpers

`ESP32HTTPClient` provides dedicated helper methods for common authentication schemes. Like `setHeader()`, these helpers configure persistent headers sent with **every subsequent request**.

### Bearer Token (JWT / OAuth)

```cpp
client.bearer("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...");
client.get("/api/profile").getBody("username", username, sizeof(username));
```

### HTTP Basic Auth

Pass your username and password; the library automatically formats and Base64-encodes the credentials:

```cpp
client.basic("admin", "secret123");
client.get("/admin/metrics").getBody("uptime", &uptime);
```

### API Key

Pass the header name and your API key:

```cpp
client.apiKey("x-api-key", "my-secret-api-key");
client.get("/v1/sensors").getBody("temperature", &temp);
```

---

## Custom HTTP Headers

Use `setHeader()` to register any arbitrary persistent header that will be sent with **every subsequent request** from that client instance:

```cpp
// Custom headers
client.setHeader("X-Device-ID", "ESP32-001");
client.setHeader("X-Custom-Header", "custom-value");

// Custom headers are sent on all subsequent requests
client.get("/protected/resource").getBody("data", &myVar);
```

!!! note "Header persistence"
    Headers registered with `setHeader()`, `bearer()`, `basic()`, or `apiKey()` persist for the lifetime of the client object. They are sent on every request. To change a header, call the method again with the new value.

!!! tip "Reading response headers"
    `setHeader()` sets headers to send with the request. To read or collect headers returned by the server in the response (such as `token`, `Content-Length`, `Date`, etc.), use the `.getHeader()` method on `RestRequest`. See how to collect them in the [Reading Response Headers](responses.md#reading-response-headers-with-getheader) guide or the [RestRequest::getHeader](../api/restrequest.md#extracting-response-headers) API reference.

---

## Custom Content-Type

By default, the `Content-Type` header is set to `application/json` for all requests with a body. Override it with `setContentType()`:

```cpp
client.setContentType("application/x-www-form-urlencoded");
client.post("/form").body("field", "value");
```

---

## Keep-Alive Connection Management

By default, `ESP32HTTPClient` enables HTTP **Keep-Alive**, reusing the underlying TCP/TLS connection across requests. This is the primary reason the library is **~12x faster** than the standard approach, since expensive TLS handshakes only happen once.

### When to call `end()`

After a burst of requests, if your sketch enters a long idle period or you want to free the TLS memory buffers (~45KB for an active connection), call `end()`:

```cpp
// Make several requests
client.get("/data1").getBody("val", &v1);
client.get("/data2").getBody("val", &v2);
client.get("/data3").getBody("val", &v3);

// Done for now — free the TLS memory
client.end();

delay(60000); // sleep for 60 seconds

// The next request will re-establish the connection automatically
client.get("/data4").getBody("val", &v4);
```

!!! tip "You don't need to call `end()` between requests"
    Keep-Alive is automatic. Only call `end()` when you explicitly want to free the connection memory after a period of inactivity.

---

## Changing Server URL at Runtime

You can change the target host URL or port on the fly without recreating the `ESP32HTTPClient` instance:

```cpp
ESP32HTTPClient client("https://api.v1.example.com");

// Switch to v2 endpoint or a local staging server
client.setBaseUrl("https://api.v2.example.com", 443);

// Or change port independently
client.setUrl("http://192.168.1.100");
client.setPort(8080);
```

---

## Timeout Configuration

By default, the client uses a timeout of **60000 ms** (1 minute / 60 seconds). You can configure the global default or override it on individual requests:

### Global Timeout

```cpp
// Set default timeout to 10 seconds for all future requests
client.setTimeout(10000);
```

### Per-Request Timeout

```cpp
// Fast ping with 1.5s timeout
client.get("/quick-ping")
      .timeout(1500)
      .getBody("ok", &isOk);
```

---

## Retries & Network Recovery

`ESP32HTTPClient` automatically handles network drops and stale Keep-Alive connections by retrying failed attempts up to a configured threshold. The default is `1` retry.

### Global Max Retry

```cpp
// Allow up to 3 retry attempts on connection failures
client.setMaxRetry(3);
```

### Per-Request Retry

```cpp
// Disable retry for non-idempotent operation
client.post("/payment/charge")
      .retry(0)
      .body("amount", 100);
```

---

## Callbacks (`onSuccess`, `onError`, `onResponse`)

Callbacks allow you to attach clean, asynchronous-style handlers to your requests or at the client level.

### Request-Level Callbacks

```cpp
client.get("/sensors/temp")
      .onSuccess([](int code) {
          Serial.printf("Success: HTTP %d\n", code);
      })
      .onError([](int code, const char* message) {
          Serial.printf("Request failed (%d): %s\n", code, message);
      })
      .onResponse([](int code) {
          Serial.printf("Completed with code %d\n", code);
      })
      .getBody("temperature", &temp);
```

### Client-Level Callbacks

Client-level callbacks are triggered on every request executed by that client instance:

```cpp
client.onError([](int code, const char* message) {
    Serial.printf("[Global Error Handler] Code %d: %s\n", code, message);
});
```

---

## Error Handling & Inspection

You can inspect the result of any request using the built-in helper methods:

```cpp
client.get("/users/1").getBody("name", name, sizeof(name));

if (client.isSuccess()) {
    Serial.println("User loaded successfully");
} else if (client.hasError()) {
    int code = client.getStatusCode();
    String errorMsg = client.getErrorMessage();
    Serial.printf("Failed with code %d: %s\n", code, errorMsg.c_str());
}
```

### Error Code Reference Table

#### Network / Client Errors (`code < 0`)

| Code | Constant | Description |
| :--- | :--- | :--- |
| `-1` | `HTTPC_ERROR_CONNECTION_REFUSED` | Target host refused the connection. |
| `-2` | `HTTPC_ERROR_SEND_HEADER_FAILED` | Failed to write HTTP headers to the socket. |
| `-3` | `HTTPC_ERROR_SEND_PAYLOAD_FAILED` | Failed to send request body payload. |
| `-4` | `HTTPC_ERROR_NOT_CONNECTED` | Not connected to network or socket. |
| `-5` | `HTTPC_ERROR_CONNECTION_LOST` | TCP connection terminated unexpectedly. |
| `-6` | `HTTPC_ERROR_NO_STREAM` | No response stream available. |
| `-7` | `HTTPC_ERROR_NO_HTTP_SERVER` | Server did not respond with valid HTTP. |
| `-8` | `HTTPC_ERROR_TOO_LESS_RAM` | Insufficient free RAM on ESP32. |
| `-9` | `HTTPC_ERROR_ENCODING` | Transfer encoding error. |
| `-10` | `HTTPC_ERROR_STREAM_WRITE` | Stream write failed. |
| `-11` | `HTTPC_ERROR_READ_TIMEOUT` | Timed out waiting for response data from server. |

#### HTTP Status Codes (`code > 0`)

| Code | Status | Description |
| :--- | :--- | :--- |
| `200` | OK | Request succeeded normally. |
| `201` | Created | Resource created successfully. |
| `202` | Accepted | Request accepted for processing. |
| `204` | No Content | Success, server returned empty response. |
| `400` | Bad Request | Invalid request parameters or payload. |
| `401` | Unauthorized | Missing or invalid authentication credentials. |
| `403` | Forbidden | Insufficient permissions for resource. |
| `404` | Not Found | Requested endpoint path does not exist. |
| `408` | Request Timeout | Server timed out waiting for request. |
| `429` | Too Many Requests | Rate limit exceeded. |
| `500` | Internal Server Error | Generic server-side error. |
| `502` | Bad Gateway | Invalid response from upstream server. |
| `503` | Service Unavailable | Server overloaded or down for maintenance. |
| `504` | Gateway Timeout | Upstream gateway timed out. |

---

## Long Unix Timestamps

For Unix timestamps and other large integer values (greater than `2^31 - 1`), use a `long` binding:

```cpp
long unixTimestamp = 0;

client.get("/api/v1/time/current/unix")
      .getBody("unix_timestamp", &unixTimestamp);

Serial.printf("Unix time: %ld\n", unixTimestamp);
```

---

## Struct <-> JSON Mapping

`ESP32-HTTP-Client` provides direct bidirectional mapping between C++ `struct`s and JSON payloads with zero dynamic document allocations, reflection overhead, or external dependencies.

### Declaring Mappable Structs

Use the `REST_JSON_MAP` macro inside your struct to define its JSON fields:

```cpp
struct User {
    int id = 0;
    char name[32] = {0};
    float score = 0.0f;
    bool active = false;
    String email = "";

    REST_JSON_MAP(
        REST_FIELD(id),
        REST_FIELD(name),
        REST_FIELD(score),
        REST_FIELD(active),
        REST_FIELD(email)
    )
};
```

If your JSON keys differ from the C++ member names, use `REST_FIELD_NAMED`:

```cpp
struct Profile {
    int userId = 0;
    char fullName[32] = {0};

    REST_JSON_MAP(
        REST_FIELD_NAMED("user_id", userId),
        REST_FIELD_NAMED("full_name", fullName)
    )
};
```

For structs defined in external or 3rd-party libraries, use `REST_JSON_MAP_EXT`:

```cpp
struct ExternalDevice {
    int deviceId;
    String status;
};

REST_JSON_MAP_EXT(ExternalDevice,
    REST_FIELD_EXT(deviceId),
    REST_FIELD_EXT(status)
)
```

### Sending Structs in Request Body

Pass the struct directly into `.body()`:

```cpp
User user;
user.id = 15;
strncpy(user.name, "Pedro", sizeof(user.name));
user.score = 9.8f;
user.active = true;
user.email = "pedro@example.com";

// Automatically serialized to JSON: {"id":15,"name":"Pedro","score":9.8,"active":true,"email":"pedro@example.com"}
client.post("/users").body(user);
```

### Receiving Structs from Response Body

Populate your struct directly from the HTTP response stream:

```cpp
User user;
// Binds and populates struct directly from the root JSON response
client.get("/users/15").getBody(&user);

Serial.printf("User: ID=%d, Name=%s, Active=%d\n", user.id, user.name, user.active);
```

You can also bind to nested objects within the response JSON:

```cpp
User nestedUser;
// Binds fields to "data.user.id", "data.user.name", etc.
client.get("/profile").getBody("data.user", &nestedUser);
```

### Missing and Null Fields Handling

- If the JSON response is **missing** fields that exist in the struct, those struct members keep their default values.
- If the JSON contains **extra** fields that are not in the struct, they are safely ignored.
- If a JSON field has a `null` value, primitive numbers are reset to `0`, booleans to `false`, char buffers to `""`, and Strings to `""`.

### Standalone Serialization & Deserialization (`RestJson` / `toJson` / `fromJson`)

You can also convert structs to and from JSON strings independently of HTTP requests:

```cpp
User user;
user.id = 42;
strncpy(user.name, "Ana", sizeof(user.name));

// Struct -> JSON String
String jsonStr = ESP32HTTPClient::toJson(user);
// Or: String jsonStr = RestJson::toJson(user);

// JSON String -> Struct
User parsedUser;
ESP32HTTPClient::fromJson(jsonStr, &parsedUser);
// Or: RestJson::fromJson(jsonStr, &parsedUser);
```
