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

`getBody()` registers a binding between a **JSON key path** and a **target variable**. If the key is not found, the target is **left unchanged**. All overloads are chainable.

---

### `getBody(key, target)`

Binds a JSON response field to a local variable using type-specific overloads:

```cpp
RestRequest& getBody(const char* key, int* target);
RestRequest& getBody(const char* key, long* target);
RestRequest& getBody(const char* key, float* target);
RestRequest& getBody(const char* key, double* target);
RestRequest& getBody(const char* key, bool* target);
RestRequest& getBody(const char* key, char* target, size_t maxLength);
template <size_t N> RestRequest& getBody(const char* key, char (&target)[N]);
RestRequest& getBody(const char* key, String* target);
```

**Supported target types:**

| Variable Type | Expected JSON Type | Behavior |
| :--- | :--- | :--- |
| `int*` | Integer number | Truncates decimals if present |
| `long*` | Integer number | Useful for Unix timestamps and large IDs |
| `float*` | Floating-point number | Up to 5 significant digits |
| `double*` | Floating-point number | Up to 9 significant digits |
| `bool*` | Boolean | `true` or `false` |
| `char*`, `size_t` / `char[N]` | String | Copies up to `N-1` characters, null-terminated |
| `String*` | String / Object / Array | Copies value as text or raw JSON |

**Example:**
```cpp
int count;
float temp;
bool active;
char city[64];
long timestamp;

client.get("/sensor/data")
      .getBody("count",       &count)
      .getBody("temperature", &temp)
      .getBody("active",      &active)
      .getBody("location",    city)
      .getBody("timestamp",   &timestamp);
```

---

### Capturing Raw JSON with `String`

Pass a `String*` to capture entire objects, arrays, or the complete response without deserialization:

```cpp
String entireResponse;
String firstUser;

client.get("/users")
      .getBody("",  &entireResponse) // Captures entire response from root
      .getBody("0", &firstUser);     // Captures first array element as raw JSON
```

!!! warning "Memory Warning"
    Using `String` with large payloads allocates dynamically on heap. Prefer typed primitives or mapped structs where possible.

---

### Struct Mapping with `getBody(&struct)`

Binds and populates a C++ struct mapped with `REST_JSON_MAP` directly from root or a nested path:

```cpp
template <typename T> RestRequest& getBody(T* target);
template <typename T> RestRequest& getBody(const char* key, T* target);
```

**Example:**
```cpp
User user;
UserProfile profile;

client.get("/user/1").getBody(&user);
client.get("/dashboard").getBody("data.profile", &profile);
```

---

## Extracting Response Headers

`getHeader()` registers the extraction of HTTP response headers. Header lookup is **case-insensitive** (e.g., `"token"`, `"TOKEN"`, and `"Token"` are equivalent). If the header is missing from the response, the target variable **remains unchanged**. All overloads are chainable.

### `getHeader(name, target)`

```cpp
RestRequest& getHeader(const char* name, String* target);
RestRequest& getHeader(const char* name, char* target, size_t maxLength);
template <size_t N> RestRequest& getHeader(const char* name, char (&target)[N]);
RestRequest& getHeader(const char* name, int* target);
RestRequest& getHeader(const char* name, long* target);
RestRequest& getHeader(const char* name, float* target);
RestRequest& getHeader(const char* name, double* target);
RestRequest& getHeader(const char* name, bool* target);
```

**Supported target types:**

| Variable Type | Header Format | Conversion / Behavior |
| :--- | :--- | :--- |
| `String*` | Text | Copies full header value |
| `char*`, `size_t` / `char[N]` | Text | Copies up to `N-1` characters, null-terminated |
| `int*` | Numeric | Parsed via `atoi` (e.g., `Content-Length`) |
| `long*` | Numeric | Parsed via `atol` (e.g., `X-Timestamp`) |
| `float*` | Numeric | Parsed via `strtof` (e.g., `X-Rate-Limit`) |
| `double*` | Numeric | Parsed via `strtod` |
| `bool*` | Text / Boolean | `true` for `"true"` or `"1"`, `false` otherwise |

**Example:**
```cpp
String token;
char contentType[64];
int contentLength;
bool isCached;

client.get("/auth")
      .getHeader("token",          &token)
      .getHeader("Content-Type",   contentType)
      .getHeader("Content-Length", &contentLength)
      .getHeader("X-Cache-Hit",    &isCached)
      .getBody("id", &id);
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
