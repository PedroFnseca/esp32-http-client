#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

// WiFi Credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Initialize the RestClient
ESP32HTTPClient client("https://timeapi.io", 443);

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
  long ts = 0;

  // Example: GET request binding field to variable
  Serial.println("Send GET request...");
  
  // URL: https://timeapi.io/api/v1/time/current/unix
  client.get("/api/v1/time/current/unix")
        .getBody("unix_timestamp", &ts);

  // Check Status
  if (client.getStatusCode() == 200) {
    Serial.printf("unix_timestamp: %ld\n", ts);
  } else {
    Serial.printf("Error: %d\n", client.getStatusCode());
  }

  delay(10000);
}
