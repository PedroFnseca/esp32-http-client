---
tags:
  - FAQ
  - Help
---

# :material-help-circle-outline: Frequently Asked Questions

Find quick answers to the most common questions about **ESP32-HTTP-Client**. Use the categories below to jump to what you need.

<div class="grid cards" markdown>

- :material-download: **[Installation & Setup](#installation-requirements-and-initial-setup)**
- :material-lightbulb-outline: **[Core Concepts & Architecture](#core-concepts-and-architecture)**
- :material-code-braces: **[Syntax & JSON Parsing](#syntax-data-binding-and-json-parsing)**
- :material-server-network: **[HTTP Methods & Advanced Features](#http-methods-and-advanced-features)**
- :material-speedometer: **[Performance & Troubleshooting](#performance-memory-and-troubleshooting)**

</div>

## :material-download: Installation, Requirements, and Initial Setup

??? question "How do I install the ESP32-HTTP-Client library in the Arduino IDE?"

    1. Go to **Sketch** > **Include Library** > **Manage Libraries...**
    2. Search for `ESP32-HTTP-Client` and click **Install**.

    Alternatively, download the ZIP from GitHub and add it via **Sketch** > **Include Library** > **Add .ZIP Library...**

??? question "What are the hardware prerequisites and required dependencies?"

    | Requirement | Details |
    |---|---|
    | **Hardware** | ESP32 or compatible microcontroller |
    | **Core** | Standard Arduino core for ESP32 |
    | **Dependencies** | None — no `ArduinoJson` or other external library required |

    > The library ships with its own built-in on-the-fly stream parser, so no extra dependencies are needed.

??? question "Is this library compatible with ESP8266 boards?"

    !!! success "Yes, it is compatible"
        **Yes**, the library is compatible with ESP8266 and other Arduino-compatible boards that provide standard `Client` interfaces. However, please note that **the main focus and optimizations are specifically targeted at the ESP32 ecosystem**.

??? question "How do I include the library and initialize it in my sketch?"

    ```cpp
    #include "ESP32HTTPClient.h"

    // Instantiate once with your base URL
    ESP32HTTPClient client("https://api.example.com");

    void setup() {
        Serial.begin(115200);
        // WiFi setup here...

        int value;
        client.get("/endpoint").getBody("field", &value);
    }

    void loop() {}
    ```

??? question "What configuration is needed to make secure HTTPS requests?"

    !!! tip "Zero configuration needed"
        Simply prefix your URL with `https://`. The library will automatically use port **443** and handle the secure TLS handshake.

    ```cpp
    // HTTP  → port 80 (default)
    ESP32HTTPClient client("http://api.example.com");

    // HTTPS → port 443 (automatic)
    ESP32HTTPClient client("https://api.example.com");
    ```

## :material-lightbulb-outline: Core Concepts and Architecture

??? question "What is the concept of *Direct Variable Binding*?"

    *Direct Variable Binding* means you link a JSON key from the API response **directly to a C/C++ variable**. The library writes the extracted value straight into your variable's memory address — no intermediate `String` or `JsonDocument` is ever created.

    ```cpp
    float temperature; // Your variable
    //       ↓ key in JSON          ↓ memory address
    client.get("/sensor").getBody("temp", &temperature);
    //                              └─ value injected directly here
    ```

??? question "How does the on-the-fly byte-by-byte stream parsing work?"

    The library reads the HTTP response stream **byte-by-byte** directly from the network buffer:

    ```
    Network Stream  →  Parser looks for key  →  Injects value into variable  →  Discards the rest
    ```

    Once the target key is found and its value is copied to your variable, the remaining bytes are consumed and discarded without ever being stored in RAM.

??? question "What is the main architectural difference versus traditional ArduinoJson usage?"

    | Step | Traditional Approach | ESP32-HTTP-Client |
    |---|---|---|
    | 1 | `http.getString()` — allocates a large `String` | Reads directly from network stream |
    | 2 | `DynamicJsonDocument(N)` — allocates JSON heap | No document allocation needed |
    | 3 | `deserializeJson()` — parses full payload | Values extracted on the fly |
    | 4 | `doc["key"]` — manual extraction | Variables filled automatically |
    | **RAM overhead** | ~58 KB per request | ~15 bytes per request |

??? question "Why does the library not require heap buffer allocations for the HTTP response?"

    Because it **never stores the response**. The parser scans the incoming character stream and immediately injects each desired value into your pre-allocated variable. This results in:

    !!! success "Only ~15 bytes of extra heap per request"
        Compared to ~58 KB with the standard `HTTPClient` + `ArduinoJson` approach.

??? question "In which IoT scenarios is this library most recommended?"

    - :material-memory: **Memory-constrained applications** — devices with limited free heap
    - :material-refresh: **High-frequency polling** — tight `loop()` cycles that make many requests
    - :material-cloud-outline: **Cloud backends** — Firebase, AWS API Gateway, REST APIs
    - :material-shield-lock-outline: **HTTPS-heavy projects** — Keep-Alive avoids costly repeated TLS handshakes
    - :material-leaf: **Long-running devices** — avoids heap fragmentation over time

## :material-code-braces: Syntax, Data Binding, and JSON Parsing

??? question "How do I map a simple field (int, float, or string) directly into a variable?"

    === "Integer"
        ```cpp
        int count;
        client.get("/data").getBody("count", &count);
        ```

    === "Float"
        ```cpp
        float temperature;
        client.get("/sensor").getBody("temp", &temperature);
        ```

    === "String (char[])"
        ```cpp
        char name[64];
        client.get("/user").getBody("name", name, sizeof(name));
        ```

    === "Boolean"
        ```cpp
        bool isActive;
        client.get("/device").getBody("active", &isActive);
        ```

??? question "What is the syntax for extracting data from nested JSON objects?"

    Use **dot notation** to navigate nested objects. Each segment separated by `.` represents one level of depth.

    ```json
    // API Response
    { "sensor": { "location": { "city": "Lisbon" } } }
    ```

    ```cpp
    char city[32];
    client.get("/data").getBody("sensor.location.city", city, sizeof(city));
    //                          └────────────────────┘
    //                            Dot notation path
    ```

??? question "How can I extract values from inside JSON arrays?"

    Use a **numeric index** as a path segment to address array elements (zero-indexed).

    ```json
    // API Response
    [
      { "address": { "city": "Gwenborough" } },
      { "address": { "city": "Wisokyburgh" } }
    ]
    ```

    ```cpp
    char city[32];
    // Get city of the SECOND element (index 1)
    client.get("/users").getBody("1.address.city", city, sizeof(city));
    ```

??? question "What happens if the key specified in getBody() is missing from the response?"

    !!! note "Safe by design"
        If the key is **missing, misspelled, or the path does not exist**, the target variable is left **completely unchanged**. The library will not crash, throw exceptions, or corrupt memory.

    This makes it easy to detect missing fields — pre-fill your variables with sentinel values:

    ```cpp
    int userId = -1; // sentinel

    client.get("/data").getBody("userId", &userId);

    if (userId == -1) {
        Serial.println("Key not found in response!");
    }
    ```

??? question "How does method chaining (Fluent API) work to bind multiple JSON fields?"

    Every `.getBody()` call returns a reference to the same request builder, allowing you to chain as many bindings as needed in a single expression:

    ```cpp
    int userId;
    float temperature;
    char city[32];

    client.post("/report")
          .body("device", "esp32-cam")
          .body("floor", 3)
          .getBody("userId",          &userId)                  // int
          .getBody("sensor.temp",     &temperature)             // float — nested
          .getBody("0.address.city",  city, sizeof(city));      // char* — array + nested
    ```

## :material-server-network: HTTP Methods and Advanced Features

??? question "How do I perform POST, PUT, and DELETE requests with a payload?"

    === "POST"
        ```cpp
        int newId;
        client.post("/users")
              .body("name", "Pedro")
              .body("role", "admin")
              .body("age", 21)
              .getBody("id", &newId);
        ```

    === "PUT / Update"
        ```cpp
        client.put("/lights/1").body("state", "OFF");
        // or using the alias:
        client.update("/lights/1").body("state", "OFF");
        ```

    === "DELETE"
        ```cpp
        client.del("/logs/system_error.log");
        ```

    === "PATCH"
        ```cpp
        client.patch("/config/wifi").body("ssid", "MyNetwork");
        ```

??? question "How can I add custom HTTP headers like Authorization or Content-Type?"

    Use `.setHeader()` on the client instance. The header is sent with **every subsequent request**.

    ```cpp
    // Custom Authorization header
    client.setHeader("Authorization", "Bearer mytoken123");

    // Override Content-Type
    client.setContentType("application/x-www-form-urlencoded");
    ```

    !!! tip
        Call `setHeader()` once during `setup()` and it will persist for all requests.

??? question "Is it possible to set a custom port for the connection?"

    Yes, pass the port as the **second argument** to the constructor:

    ```cpp
    // Default ports: 80 for HTTP, 443 for HTTPS
    ESP32HTTPClient client("https://api.example.com");

    // Custom port
    ESP32HTTPClient client("http://my-local-server.local", 8080);
    ```

??? question "How do I read the HTTP response status code and handle server errors?"

    Call `getStatusCode()` on the client **after** a request is dispatched (i.e., after calling `.getBody()` or when the `RestRequest` goes out of scope):

    ```cpp
    int userId;
    client.get("/users/1").getBody("id", &userId);

    int status = client.getStatusCode();

    if (status == 200) {
        Serial.println("Success!");
    } else {
        Serial.printf("Error: HTTP %d\n", status);
    }
    ```

??? question "How can I capture the full raw response body (Raw JSON or plain text)?"

    Bind to an Arduino `String` by passing `""` as the key to capture the entire response:

    ```cpp
    String fullResponse;
    client.get("/data").getBody("", &fullResponse);
    ```

    To capture a nested object or specific array element:

    ```cpp
    String secondUser;
    client.get("/users").getBody("1", &secondUser);
    ```

    !!! warning "Heap allocation warning"
        Binding to `String` causes dynamic memory reallocation as the raw JSON is copied character by character. **Avoid this with large payloads**, as it can fragment or exhaust the device heap.

## :material-speedometer: Performance, Memory, and Troubleshooting

??? question "What are the actual savings in heap memory and execution speed?"

    The following data is from a benchmark running **100 consecutive HTTP GET requests** against the JSONPlaceholder `/users` endpoint:

    | Metric | Standard (HTTPClient + ArduinoJson) | ESP32-HTTP-Client |
    |---|---|---|
    | **Heap per request** | ~58.2 KB | **~0.0 KB** (15 bytes) |
    | **RAM Footprint** | 34.2% | **24.3%** |
    | **Min. Free Heap** | 114.3 KB | **128.6 KB** |
    | **Avg. Execution Time** | ~750 ms | **~59 ms** |

    !!! success "12x faster, 99.9% less RAM per request"
        Keep-Alive reuses the TLS connection, avoiding repeated handshakes. Stream parsing eliminates all intermediate buffer allocations.

??? question "What happens if my char[] buffer is smaller than the JSON string value?"

    !!! success "Buffer overflow protected"
        The library will safely copy **only as many characters as fit** into the buffer, up to the `maxLen` you provide. It will never write past the end of the buffer.

    ```cpp
    char name[8]; // Small buffer
    // If API returns "name": "Pedro Fonseca" (13 chars), only "Pedro F" is copied
    client.get("/user").getBody("name", name, sizeof(name));
    ```

    Always allocate enough space for the expected maximum value length.

??? question "How does the library perform during continuous requests in loop()?"

    Outstandingly. By maintaining a persistent **TCP/TLS Keep-Alive** connection, subsequent requests to the same host skip the expensive handshake:

    ```
    First request:  Connect + TLS Handshake + Request → ~750ms
    Next requests:  Reuse connection + Request         → ~59ms  ⚡
    ```

    ```cpp
    void loop() {
        float temp;
        client.get("/sensor").getBody("temp", &temp); // Fast on every iteration
        delay(1000);
    }
    ```

??? question "What are the best practices for debugging field binding?"

    **1. Use sentinel values** — pre-fill variables with an obviously invalid value:

    ```cpp
    int userId = -999;
    client.get("/data").getBody("userId", &userId);

    if (userId == -999) Serial.println("⚠ Key not found!");
    else Serial.printf("✔ userId = %d\n", userId);
    ```

    **2. Check the status code** — confirm the request itself succeeded:

    ```cpp
    int code = client.getStatusCode();
    Serial.printf("HTTP Status: %d\n", code); // Should be 200
    ```

    **3. Capture the raw response** — temporarily bind to a `String` to inspect the full payload:

    ```cpp
    String raw;
    client.get("/data").getBody("", &raw);
    Serial.println(raw); // Print the full JSON
    ```

??? question "How can I prevent memory issues during heavy HTTPS requests?"

    Follow these best practices:

    - :white_check_mark: **Prefer direct binding** — use `getBody("key", &var)` for primitives and fixed `char[]` arrays
    - :white_check_mark: **Avoid `String` for large payloads** — it fragments heap on repeated reallocation
    - :white_check_mark: **Call `client.end()`** — frees the ~45 KB TLS connection buffer when you're done with a burst of requests
    - :x: **Avoid creating multiple client instances** — instantiate `ESP32HTTPClient` once and reuse it

    ```cpp
    // After a burst of requests, free TLS memory
    client.end();
    ```

---

## :material-help-network: Still have questions?

If you couldn't find the answer to your question here, feel free to reach out!

- :material-github: **GitHub Issues**: Open an issue on the [official repository](https://github.com/PedroFnseca/esp32-http-client/issues).
