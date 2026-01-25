#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

// WiFi Credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Initialize the RestClient with a custom port
// Example: connecting to a local server running on port 8080
ESP32HTTPClient client("http://192.168.1.100", 8080);

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
  int status;
  
  // Example: GET request to a custom port
  // This will request: http://192.168.1.100:8080/status
  
  Serial.println("Sending GET request to custom port...");
  
  client.get("/status").getBody("status", &status);

  Serial.printf("Status code: %d\n", client.getStatusCode());
  Serial.printf("Value: %d\n", status);
  
  delay(10000);
}
