#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://jsonplaceholder.typicode.com");

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

void loop() {
  // 1. GET Request: Read resource
  Serial.println("\n--- [GET] Reading Todo #1 ---");
  int id = 0;
  int userId = 0;
  char title[64] = {0};
  bool completed = false;

  client.get("/todos/1")
        .getBody("id", &id)
        .getBody("userId", &userId)
        .getBody("title", title, sizeof(title))
        .getBody("completed", &completed);

  if (client.isSuccess()) {
    Serial.printf("GET 200 OK -> ID: %d, User: %d, Title: %s, Completed: %s\n",
                  id, userId, title, completed ? "true" : "false");
  }

  // 2. POST Request: Create resource
  Serial.println("\n--- [POST] Creating Post ---");
  int newId = 0;
  client.post("/posts")
        .body("title", "ESP32 REST Client")
        .body("body", "Fluent embedded HTTP client")
        .body("userId", 1)
        .getBody("id", &newId);

  if (client.isSuccess()) {
    Serial.printf("POST Created -> New ID: %d\n", newId);
  }

  // 3. PUT Request: Full update
  Serial.println("\n--- [PUT] Updating Post #1 ---");
  client.put("/posts/1")
        .body("id", 1)
        .body("title", "Updated Title")
        .body("body", "Updated Content")
        .body("userId", 1);

  if (client.isSuccess()) {
    Serial.println("PUT Updated -> Resource updated successfully");
  }

  // 4. PATCH Request: Partial update
  Serial.println("\n--- [PATCH] Partial update Post #1 ---");
  client.patch("/posts/1")
        .body("title", "Patched Title Only");

  if (client.isSuccess()) {
    Serial.println("PATCH Success -> Title field modified");
  }

  // 5. DELETE Request: Remove resource
  Serial.println("\n--- [DELETE] Deleting Post #1 ---");
  client.del("/posts/1");

  if (client.isSuccess()) {
    Serial.println("DELETE Success -> Resource deleted");
  }

  delay(15000);
}
