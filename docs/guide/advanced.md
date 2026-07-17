---
tags:
  - guide
  - advanced
---
# Advanced Usage

## Custom HTTP Headers

Use `setHeader()` to register a persistent header that will be sent with **every subsequent request** from that client instance. This is the standard way to pass authentication tokens, API keys, or any custom header your server requires.

```cpp
// Authorization header (Bearer token)
client.setHeader("Authorization", "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...");

// API key header
client.setHeader("X-API-Key", "my-secret-api-key");

// Custom headers are sent on all subsequent requests
client.get("/protected/resource").getBody("data", &myVar);
```

!!! note "Header persistence"
    Headers registered with `setHeader()` persist for the lifetime of the client object. They are sent on every request. To change a header, call `setHeader()` again with the same name and a new value.

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

## Automatic Stale Connection Recovery

If a Keep-Alive connection becomes stale (e.g., the server closed it on its end), the library automatically detects the failure, closes the socket, re-establishes the connection, and retries the request **once** — transparently to your code.

```cpp
// This works even if the server dropped the connection
client.get("/sensor").getBody("temp", &temperature);
```

---

## Handling `Transfer-Encoding: chunked`

Many modern servers (including those behind load balancers or API gateways) respond with `Transfer-Encoding: chunked`. The library's internal `BufferedStreamReader` handles chunked decoding automatically and transparently. No configuration is needed.

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

## Checking Connection Status

While the library manages connections automatically, you can always inspect the result of a request using `getStatusCode()`:

```cpp
client.get("/health");
int code = client.getStatusCode();

if (code == 200) {
    Serial.println("Server is healthy");
} else if (code < 0) {
    Serial.println("Network error — could not reach server");
} else {
    Serial.printf("Server responded with HTTP %d\n", code);
}
```
