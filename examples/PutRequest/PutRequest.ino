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
  Serial.println("Sending PUT request to update post #1...");

  // Update existing resource
  // Note: JSONPlaceholder simulates the update but doesn't actually change data on server.
  client.put("/posts/1")
        .body("id", 1)
        .body("title", "foo updated")
        .body("body", "bar updated")
        .body("userId", 1);

  if (client.getStatusCode() == 200) {
    Serial.println("Update successful (200 OK)");
  } else {
    Serial.printf("Error: %d\n", client.getStatusCode());
  }

  delay(10000);
}
