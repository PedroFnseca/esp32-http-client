---
tags:
  - performance
  - benchmark
---
# Performance

## Benchmark Setup

All data was collected by running **100 consecutive HTTP GET requests** with a JSON payload on a real ESP32 device against the public [JSONPlaceholder `/users` endpoint](https://jsonplaceholder.typicode.com/users). Both libraries were configured with default settings. The WiFi connection was already established before the benchmark started.

---

## Results

| Metric / Feature | Standard (HTTPClient + ArduinoJson) | ESP32-HTTP-Client | Comparison |
| :--- | :---: | :---: | :---: |
| **Heap allocation per request** | ~58.2 KB | **~15 bytes** | ⬇ ~99.9% less RAM |
| **Avg. RAM footprint** | 34.2% | **24.3%** | ⬇ ~29% less overall |
| **Minimum free heap** | 114.3 KB | **128.6 KB** | ⬆ Safer headroom |
| **Avg. execution time** | ~750 ms | **~59 ms** | 🚀 ~12× faster |
| **Code verbosity** | ~15 lines of boilerplate | **1 fluent chain** | ⬇ Clean & maintainable |
| **JSON parsing** | `deserializeJson()` | **Automatic, direct binding** | ⬆ No document allocation |

---

## Why is it so much faster?

### HTTP Keep-Alive

`ESP32HTTPClient` enables `HTTP/1.1` Keep-Alive by default via `_http.setReuse(true)`. This allows the underlying **TCP/TLS connection to be reused** across multiple requests to the same server.

The standard approach typically closes and reopens the connection on every call, paying the full TLS handshake cost (~700–800 ms on a typical WiFi network) each time.

### Zero-Copy Stream Parsing

Instead of calling `http.getString()` (which allocates the entire response in heap memory) and then passing it to `deserializeJson()` (which allocates a `DynamicJsonDocument`), this library:

1. Obtains a raw `Stream*` pointer directly from the `HTTPClient`.
2. Parses the JSON stream **byte by byte** through `BufferedStreamReader`.
3. Writes matched values directly into your pre-declared C variables.

The full response body is **never stored in memory**. The total heap allocation for a typical response is ~15 bytes of internal state.

### Chunked Transfer Encoding

Modern APIs commonly respond with `Transfer-Encoding: chunked`. The library's `BufferedStreamReader` automatically decodes chunked responses without requiring the full payload to be buffered first.

---

## Memory Trade-offs

| Scenario | RAM freed by `end()` |
| :--- | :--- |
| Active TLS (HTTPS) connection | ~45 KB |
| Active plain HTTP connection | ~5–10 KB |

Call `client.end()` after a burst of requests if your sketch enters a long idle period:

```cpp
// Burst of requests
for (int i = 0; i < 10; i++) {
    client.get("/data").getBody("v", &val);
    // ... process val
}

client.end(); // free ~45KB of TLS buffers
delay(60 * 1000); // sleep for 1 minute
```

---

## Benchmark Source

The full benchmark sketches and comparison scripts are in the [`bench/`](https://github.com/PedroFnseca/esp32-http-client/tree/main/bench) directory of the repository.
