# POST Request

Demonstrates sending a JSON body with a POST request and reading the ID of the newly created resource from the response.

**Source:** [`examples/PostRequest/PostRequest.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/PostRequest/PostRequest.ino)

---

## API Used

**Endpoint:** `POST https://jsonplaceholder.typicode.com/posts`

**Request Body:**
```json
{ "title": "foo", "body": "bar", "userId": 1 }
```

**Response:**
```json
{ "title": "foo", "body": "bar", "userId": 1, "id": 101 }
```

---

## Sketch

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://jsonplaceholder.typicode.com");

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) delay(500);
    Serial.println("\nWiFi Connected");
}

void loop() {
    int newId;

    Serial.println("Sending POST...");

    client.post("/posts")
          .body("title", "foo")
          .body("body",  "bar")
          .body("userId", 1)
          .getBody("id", &newId);

    if (client.getStatusCode() == 201) { // 201 Created
        Serial.printf("Created Post ID: %d\n", newId);
    } else {
        Serial.printf("Error: %d\n", client.getStatusCode());
    }

    delay(10000);
}
```

---

## Expected Serial Output

```
WiFi Connected
Sending POST...
Created Post ID: 101
```

---

## Key Points

- Each `.body()` call adds a field to the JSON body: `{"title":"foo","body":"bar","userId":1}`.
- The `Content-Type: application/json` header is set automatically when `.body()` is used.
- A successful creation returns HTTP `201 Created`, not `200 OK`.
- `.getBody()` can be chained after `.body()` to read fields from the server's response.
