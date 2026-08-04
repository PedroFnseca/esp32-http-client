---
tags:
  - api
  - request
  - class
---
# RestRequest

The fluent request builder returned by every HTTP method on [`ESP32HTTPClient`](esp32httpclient.md). All builder methods return `RestRequest&`, enabling fluent chaining.

**The HTTP request is dispatched automatically** when the `RestRequest` object goes out of scope (at the end of the statement).

!!! note "Copy semantics"
    `RestRequest` is **move-only** — it cannot be copied. It is designed to be used in a single chained expression.

---

## Building the Request

### `path(key, value)`

Replaces a placeholder (e.g. `{id}` or `id`) in the URL path. Chainable.

```cpp
template <typename T>
RestRequest& path(const char* key, T value);
```

**Supported types for `value`:** `int`, `long`, `float`, `double`, `bool`, `const char*`, `char*`

**Example:**
```cpp
// Produces: GET /users/15
client.get("/users/{id}")
      .path("id", 15);
```

---

### `query(key, value)`

Appends a URL query parameter. Chainable.

```cpp
template <typename T>
RestRequest& query(const char* key, T value);
```

**Supported types for `value`:** `int`, `long`, `float`, `double`, `bool`, `const char*`, `char*`

**Example:**
```cpp
// Produces: GET /users?page=2&limit=20&search=pedro
client.get("/users")
      .query("page",   2)
      .query("limit",  20)
      .query("search", "pedro");
```

---

### `body(key, value)`

Adds a field to the JSON request body. Automatically sets `Content-Type: application/json`. Chainable.

```cpp
template <typename T>
RestRequest& body(const char* key, T value);
```

**Supported types for `value`:** `int`, `long`, `float`, `double`, `bool`, `const char*`, `char*`

**Type serialization:**

| C++ Type | JSON Output Example |
| :--- | :--- |
| `const char*` / `char*` | `"string value"` |
| `int` | `42` |
| `long` | `1721156604` |
| `float` | `24.5` (up to 5 significant digits) |
| `double` | `3.14159265` (up to 9 significant digits) |
| `bool` | `true` or `false` |

**Example:**
```cpp
// Body: {"name":"Pedro","age":21,"active":true,"score":9.87}
client.post("/users")
      .body("name",   "Pedro")
      .body("age",    21)
      .body("active", true)
      .body("score",  9.87f);
```

---

### `body(struct)`

Serializes a C++ struct mapped with `REST_JSON_MAP` and sets it as the full JSON request body. Chainable.

```cpp
template <typename T>
RestRequest& body(const T& obj);
```

**Example:**
```cpp
User user = {15, "Pedro", 9.8f, true};
client.post("/users").body(user);
```

---

### `timeout(timeoutMs)`

Sets a per-request network timeout in milliseconds, overriding the client's default timeout. Chainable.

```cpp
RestRequest& timeout(uint16_t timeoutMs);
```

**Example:**
```cpp
client.get("/quick-data").timeout(1500);
```

---

### `retry(maxRetry)` / `maxRetry(maxRetry)`

Sets the maximum number of automatic retries on network failure for this specific request, overriding the client default. Chainable.

```cpp
RestRequest& retry(int maxRetry);
RestRequest& maxRetry(int maxRetry);
```

**Example:**
```cpp
client.get("/critical-data").retry(3);
```

---

### `onSuccess(callback)`

Registers a per-request callback executed if the response HTTP status code is 2xx (`200 <= code < 300`). Chainable.

```cpp
RestRequest& onSuccess(HttpResponseCallback cb);
```

**Example:**
```cpp
client.get("/users")
      .onSuccess([](int code) {
          Serial.printf("Request succeeded with HTTP %d\n", code);
      })
      .getBody("id", &id);
```

---

### `onError(callback)`

Registers a per-request callback executed if the request fails (`code < 200 || code >= 400`). Receives status code and an optional error description message. Chainable.

```cpp
RestRequest& onError(HttpErrorCallback cb);
RestRequest& onError(HttpResponseCallback cb);
```

**Example:**
```cpp
client.get("/users")
      .onError([](int code, const char* message) {
          Serial.printf("Request failed (%d): %s\n", code, message);
      })
      .getBody("id", &id);
```

---

### `onResponse(callback)`

Registers a per-request callback executed when the request completes, regardless of success or failure. Chainable.

```cpp
RestRequest& onResponse(HttpResponseCallback cb);
```

**Example:**
```cpp
client.get("/users")
      .onResponse([](int code) {
          Serial.printf("Finished with code %d\n", code);
      })
      .getBody("id", &id);
```

---

## Extracting the Response

`getBody()` is overloaded for each supported C type. It registers a binding between a **JSON key path** and a **target variable**. All overloads are chainable.

The binding uses the path to locate the value in the parsed JSON stream. If the key is not found, the target is **left unchanged**.

---

### `getBody(key, int* target)`

Binds a JSON number to a C `int`.

```cpp
RestRequest& getBody(const char* key, int* target);
```

**Example:**
```cpp
int count;
client.get("/stats").getBody("count", &count);
```

---

### `getBody(key, float* target)`

Binds a JSON number to a C `float`.

```cpp
RestRequest& getBody(const char* key, float* target);
```

**Example:**
```cpp
float temp;
client.get("/sensor").getBody("temperature", &temp);
```

---

### `getBody(key, double* target)`

Binds a JSON number to a C `double`.

```cpp
RestRequest& getBody(const char* key, double* target);
```

**Example:**
```cpp
double voltage;
client.get("/meter").getBody("voltage", &voltage);
```

---

### `getBody(key, long* target)`

Binds a JSON integer to a C `long`. Use for Unix timestamps and other large integers.

```cpp
RestRequest& getBody(const char* key, long* target);
```

**Example:**
```cpp
long unixTs;
client.get("/time").getBody("unix_timestamp", &unixTs);
```

---

### `getBody(key, bool* target)`

Binds a JSON boolean (`true` / `false`) to a C `bool`.

```cpp
RestRequest& getBody(const char* key, bool* target);
```

**Example:**
```cpp
bool active;
client.get("/status").getBody("active", &active);
```

---

### `getBody(key, char* target, size_t maxLength)`

Copies a JSON string into a C `char` buffer. Writes at most `maxLength - 1` bytes and always null-terminates.

```cpp
RestRequest& getBody(const char* key, char* target, size_t maxLength);
```

**Example:**
```cpp
char city[64];
client.get("/user/1").getBody("address.city", city, sizeof(city));
```

---

### `getBody(key, String* target)`

Copies a JSON string, object, or array into an Arduino `String`.

- For primitive JSON strings, the string value is copied.
- For JSON objects or arrays, the raw JSON is captured character by character.
- Pass `""` (empty string) as `key` to capture the entire root-level value.

```cpp
RestRequest& getBody(const char* key, String* target);
```

**Example:**
```cpp
String raw;
client.get("/users").getBody("",  &raw);      // entire response
client.get("/users").getBody("0", &firstUser); // first array element
```

!!! warning
    Use this with care for large payloads. Each character is heap-allocated individually, which can cause memory fragmentation.

---

### `getBody(struct* target)`

Binds and populates a C++ struct mapped with `REST_JSON_MAP` directly from the root JSON response stream.

```cpp
template <typename T>
RestRequest& getBody(T* target);
```

**Example:**
```cpp
User user;
client.get("/users/1").getBody(&user);
```

---

### `getBody(key, struct* target)`

Binds and populates a C++ struct mapped with `REST_JSON_MAP` from a nested JSON object path.

```cpp
template <typename T>
RestRequest& getBody(const char* key, T* target);
```

**Example:**
```cpp
User user;
client.get("/profile").getBody("data.user", &user);
```

---

## Path Notation Reference

| Path String | What it targets |
| :--- | :--- |
| `"name"` | Root-level field `name` |
| `"address.city"` | Nested field: `address` → `city` |
| `"address.geo.lat"` | Deeply nested: `address` → `geo` → `lat` |
| `"0.name"` | First array element's `name` field |
| `"1.address.city"` | Second array element's nested `city` |
| `""` | The entire root-level object or array (use with `String*`) |
