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
  // 1. Path Parameters: GET /users/{id} -> GET /users/15
  Serial.println("\n--- [1] Path Parameters Example ---");
  int userId = 15;
  char name[64] = {0};

  // Replaces {id} in the path with 15
  client.get("/users/{id}")
        .path("id", userId)
        .getBody("name", name, sizeof(name));

  if (client.isSuccess()) {
    Serial.printf("Path param resolved -> User: %s\n", name);
  }

  // 2. Query Parameters: GET /posts?userId=1&_limit=2
  Serial.println("\n--- [2] Query Parameters Example ---");
  char firstTitle[64] = {0};

  // Builds URL: /posts?userId=1&_limit=2
  client.get("/posts")
        .query("userId", 1)
        .query("_limit", 2)
        .getBody("0.title", firstTitle, sizeof(firstTitle));

  if (client.isSuccess()) {
    Serial.printf("Query params filtered -> First Post Title: %s\n", firstTitle);
  }

  // 3. Combining Path and Query Parameters
  Serial.println("\n--- [3] Path + Query Combined ---");
  // Builds URL: /users/1/posts?_limit=5&sort=desc
  client.get("/users/{userId}/posts")
        .path("userId", 1)
        .query("_limit", 5)
        .query("sort", "desc");

  if (client.isSuccess()) {
    Serial.println("Combined Path and Query executed successfully!");
  }

  delay(15000);
}
