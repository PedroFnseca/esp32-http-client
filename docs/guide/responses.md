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
| `getBody(key, float* target)` | number | Up to 5 significant digits |
| `getBody(key, double* target)` | number | Up to 9 significant digits |
| `getBody(key, long* target)` | number | Use for Unix timestamps |
| `getBody(key, bool* target)` | boolean | Matches `true` / `false` |
| `getBody(key, char* target, size_t maxLen)` | string | Copies up to `maxLen-1` bytes |
| `getBody(key, String* target)` | string / object / array | Arduino `String`, see below |

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

## Safety Guarantees

!!! note "Missing keys are safe"
    If a key path does not exist in the response, is misspelled, or the type doesn't match, the target variable is **left unchanged**. No exception is thrown, no crash occurs, and the library continues parsing the rest of the response.

This makes it safe to add optional fields to your bindings without defensive checks:

```cpp
int optionalField = -1; // default value

client.get("/data")
      .getBody("requiredField", &myVar)
      .getBody("optionalField", &optionalField); // unchanged if missing
```

---

## Next Step

→ [Advanced Usage](advanced.md)
