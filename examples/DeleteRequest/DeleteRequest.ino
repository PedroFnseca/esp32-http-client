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
  Serial.println("Sending DELETE request...");

  // Delete resource #1
  client.del("/posts/1");

  if (client.getStatusCode() == 200 || client.getStatusCode() == 204) {
    Serial.println("Delete successful");
  } else {
    Serial.printf("Error: %d\n", client.getStatusCode());
  }

  delay(10000);
}
