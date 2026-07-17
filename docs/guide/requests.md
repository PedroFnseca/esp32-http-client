---
tags:
  - guide
  - request
  - http
---
# Making Requests

Every HTTP method on `ESP32HTTPClient` returns a `RestRequest` object. You can chain `.query()`, `.body()`, and `.getBody()` calls on it. The request is dispatched automatically when the chain goes out of scope (i.e., at the end of the statement).

---

## GET

```cpp
client.get("/todos/1");
```

### With Query Parameters

Use `.query(key, value)` to append URL parameters. Supports `int`, `float`, `double`, `long`, `bool`, and `const char*`.

```cpp
// Produces: GET /climate?room=living_room&sensor=dht22
client.get("/climate")
      .query("room", "living_room")
      .query("sensor", "dht22")
      .getBody("temp", &temperature);
```

---

## POST

Use `.body(key, value)` to build the JSON body. The `Content-Type` is automatically set to `application/json`.

```cpp
// Body: {"title": "foo", "body": "bar", "userId": 1}
int newId;

client.post("/posts")
      .body("title", "foo")
      .body("body", "bar")
      .body("userId", 1)
      .getBody("id", &newId);

// Check creation status
if (client.getStatusCode() == 201) {
    Serial.printf("Created with ID: %d\n", newId);
}
```

---

## PUT / update

`put()` and `update()` are aliases for the same `HTTP PUT` method:

```cpp
client.put("/lights/1").body("state", "OFF");
// or equivalently:
client.update("/lights/1").body("state", "OFF");
```

---

## PATCH

```cpp
client.patch("/config/wifi")
      .body("ssid", "NewNetwork")
      .body("password", "secret");
```

---

## DELETE

```cpp
client.del("/logs/old.log");
```

DELETE requests can optionally include a body:

```cpp
client.del("/sessions")
      .body("userId", 42);
```

---

## Supported Value Types for `.query()` and `.body()`

| C Type | JSON output | Example |
| :--- | :--- | :--- |
| `const char*` / `char*` | `"string"` (quoted) | `.body("name", "Pedro")` |
| `int` | `42` | `.body("age", 21)` |
| `long` | `1721000000` | `.body("ts", unixTime)` |
| `float` | `24.5` | `.body("temp", 24.5f)` |
| `double` | `3.14159265` | `.body("pi", 3.14159265)` |
| `bool` | `true` / `false` | `.body("active", true)` |

---

## Checking the HTTP Status Code

After any request, use `getStatusCode()` to inspect the result:

```cpp
client.get("/api/status");

int code = client.getStatusCode();
if (code == 200) {
    Serial.println("OK");
} else if (code < 0) {
    Serial.println("Connection error");
} else {
    Serial.printf("HTTP error: %d\n", code);
}
```

!!! note "Status code availability"
    `getStatusCode()` always returns the code from the **last completed request**. Negative values indicate a network-level error (e.g., no connection).

---

## Next Step

→ [Reading Responses](responses.md)
