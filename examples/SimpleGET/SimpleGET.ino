#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

// WiFi Credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Initialize the RestClient
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
  int userId;
  int id;
  char title[64];
  bool completed;

  // Example: GET request binding fields to variables
  Serial.println("Send GET request...");
  
  // URL: https://jsonplaceholder.typicode.com/todos/1
  client.get("/todos/1")
        .getBody("userId", &userId)
        .getBody("id", &id)
        .getBody("title", title, sizeof(title))
        .getBody("completed", &completed);

  // Check Status
  if (client.getStatusCode() == 200) {
    Serial.printf("User ID: %d\n", userId);
    Serial.printf("ID: %d\n", id);
    Serial.printf("Title: %s\n", title);
    Serial.printf("Completed: %s\n", completed ? "true" : "false");
  } else {
    Serial.printf("Error: %d\n", client.getStatusCode());
  }

  delay(10000);
}
