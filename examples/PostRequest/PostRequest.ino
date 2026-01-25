#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

// WiFi Credentials
const char* ssid = "YOUR_SSID";
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

  // Sending JSON: {"title": "foo", "body": "bar", "userId": 1}
  // Response contains "id" of the created resource
  client.post("/posts")
        .body("title", "foo")
        .body("body", "bar")
        .body("userId", 1)
        .getBody("id", &newId);

  if (client.getStatusCode() == 201) { // 201 Created
    Serial.printf("Created Post ID: %d\n", newId);
  } else {
    Serial.printf("Error: %d\n", client.getStatusCode());
  }

  delay(10000);
}
