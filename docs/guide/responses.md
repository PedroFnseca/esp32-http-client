---
title: Reading HTTP Responses & Direct Zero-Allocation JSON Parsing on ESP32
description: Learn how to bind JSON response fields directly to C++ primitive types, structs, and arrays without dynamic memory allocation on ESP32.
keywords: ESP32 JSON binding, zero heap JSON parser ESP32, read HTTP response Arduino ESP32, getBody ESP32HTTPClient
tags:
  - guide
  - response
  - json
---
# Reading Responses

The core of ESP32-HTTP-Client is the **response binding** system. Instead of parsing a JSON string yourself, you register bindings that tell the library exactly which variables to populate.

---

## Basic Binding with `getBody()`

`getBody()` is overloaded for each supported C type. It registers a binding between a **JSON key path** and a **target variable pointer**.

```cpp
int count;
float temperature;
bool active;
char name[64];
long timestamp;

client.get("/sensor")
      .getBody("count",       &count)
      .getBody("temperature", &temperature)
      .getBody("active",      &active)
      .getBody("name",        name, sizeof(name))
      .getBody("timestamp",   &timestamp);
```

---

## Supported Types

| Method signature | JSON type matched | Notes |
| :--- | :--- | :--- |
| `getBody(key, int* target)` | number | Integer truncation applied |
| `getBody(key, float* target)` | number | Up to 5 significant digits. Supports scientific notation (e.g. `6.02e+2`, `-2.5E-3`) |
| `getBody(key, double* target)` | number | Up to 9 significant digits. Supports scientific notation (e.g. `6.02e+2`, `-2.5E-3`) |
| `getBody(key, long* target)` | number | Use for Unix timestamps |
| `getBody(key, bool* target)` | boolean | Matches `true` / `false` |
| `getBody(key, char* target, size_t maxLen)` | string | Copies up to `maxLen-1` bytes |
| `getBody(key, String* target)` | string / object / array | Arduino `String`, see below |

### Scientific Notation Support

Numbers formatted in scientific / exponential notation are parsed directly into `float` and `double` targets:

```cpp
float  positiveExp;
double negativeExp;

// Response: { "positive_exp": 6.02e+2, "negative_exp": -2.5E-3 }
client.get("/sensor")
      .getBody("positive_exp", &positiveExp) // 602.0
      .getBody("negative_exp", &negativeExp); // -0.0025
```

Both lowercase `e` and uppercase `E`, with optional positive (`+`) or negative (`-`) exponents, are supported.

---

## Dot Notation for Nested Fields

Use `.` as a path separator to reach nested JSON objects:

```cpp
// Response: { "address": { "city": "Gwenborough", "geo": { "lat": "-37.3159" } } }

char city[64];
char lat[32];

client.get("/users/1")
      .getBody("address.city",    city, sizeof(city))
      .getBody("address.geo.lat", lat,  sizeof(lat));
```

There is no practical limit on nesting depth.

---

## Numeric Indices for Arrays

Use an integer as a path segment to address a specific element of a JSON array:

```cpp
// Response: [{ "name": "Alice" }, { "name": "Bob" }]

char first[32], second[32];

client.get("/users")
      .getBody("0.name", first,  sizeof(first))   // first element
      .getBody("1.name", second, sizeof(second));  // second element
```

You can combine array indices with dot notation freely:

```cpp
// Response: [{ "address": { "city": "Gwenborough" } }, ...]
char city[64];
client.get("/users")
      .getBody("1.address.city", city, sizeof(city));
```

---

## Capturing Raw JSON with `String`

Bind a `String*` to capture an entire JSON object or array as a raw string for manual processing:

```cpp
String entireResponse;
String secondUser;

client.get("/users")
      .getBody("",  &entireResponse) // captures the root array
      .getBody("1", &secondUser);    // captures the second user object
```

!!! warning "Memory warning"
    Pulling complete objects or arrays into an Arduino `String` causes dynamic heap allocation. Avoid this pattern with large payloads, as it can fragment or exhaust the device heap. Use typed bindings where possible.

---

## Reading Response Headers with `getHeader()`

In addition to JSON body fields, you can extract HTTP response headers directly using `getHeader()`.

```cpp
String token;
char contentType[64];
int contentLength;

client.get("/auth")
      .getHeader("token",          &token)
      .getHeader("Content-Type",   contentType)
      .getHeader("Content-Length", &contentLength)
      .getBody("id", &id);
```

Header lookups are **case-insensitive**, so `"token"`, `"TOKEN"`, and `"Token"` all work identically.

---

## Reading JSON-RPC 2.0 Responses with `getResult()`

When working with JSON-RPC 2.0 requests, the result payload is extracted using `.getResult()` rather than `.getBody()`. This supports root primitives, nested fields, mapped structs, and raw JSON extraction directly:

```cpp
// 1. Primitive binding to root "result"
int sum = 0;
client.jsonRpc("/rpc").method("add").param(10).param(20).getResult(&sum);

// 2. Nested field inside "result" object
char city[32] = {0};
client.jsonRpc("/rpc").method("getUser").param("id", 1).getResult("address.city", city, sizeof(city));

// 3. Mapped struct binding
User profile;
client.jsonRpc("/rpc").method("getUser").param("id", 1).getResult(&profile);

// 4. Raw JSON result string
String rawResult;
client.jsonRpc("/rpc").method("getMetrics").getRawResult(&rawResult);
```

### Inspecting JSON-RPC Errors

If the server returns a JSON-RPC 2.0 error object (`{"error": {"code": -32601, "message": "Method not found"}}`), standard HTTP status codes might still be `200 OK`. You can detect and inspect JSON-RPC errors using `.getError()` or `.onJsonRpcError()`:

```cpp
JsonRpcError error;
client.jsonRpc("/rpc")
      .method("unknownMethod")
      .getError(&error)
      .onJsonRpcError([](const JsonRpcError& err) {
          Serial.printf("Error code: %d, Message: %s\n", err.code, err.message.c_str());
      });

if (error.code != 0) {
    Serial.printf("JSON-RPC error encountered: %s\n", error.message.c_str());
}
```

---

## Safety Guarantees

!!! note "Missing keys and headers are safe"
    If a JSON key path or response header does not exist in the response, is misspelled, or the type doesn't match, the target variable is **left unchanged**. No exception is thrown, no crash occurs, and the library continues parsing the rest of the response.

This makes it safe to add optional fields or optional headers to your bindings without defensive checks:

```cpp
int optionalField = -1; // default value
String optionalToken = "";

client.get("/data")
      .getHeader("token",         &optionalToken)
      .getBody("requiredField",   &myVar)
      .getBody("optionalField",   &optionalField); // unchanged if missing
```

---

## Next Step

→ [Advanced Usage](advanced.md)
