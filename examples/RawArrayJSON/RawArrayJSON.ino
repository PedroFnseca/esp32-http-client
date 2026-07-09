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
  String entireArray;
  String specificUser;

  // Example: GET request binding raw arrays/objects using Arduino String
  Serial.println("Send GET request to fetch raw array and raw object...");
  
  // NOTE: Pulling complete arrays or objects into a String results in dynamic memory reallocation.
  // Use cautiously on large payloads to prevent heap fragmentation.
  
  // URL: https://jsonplaceholder.typicode.com/users
  // Response contains an array of users:
  // [
  //   {
  //     "id": 1,
  //     "name": "Leanne Graham",
  //     "address": { "city": "Gwenborough" }
  //   },
  //   { ... }
  // ]

  // Request 1: Fetch the entire array
  client.get("/users")
        .getBody("", &entireArray); // Binding to an empty path matches the root array

  if (client.getStatusCode() == 200) {
    Serial.printf("Raw Array (length: %d):\n%s\n\n", entireArray.length(), entireArray.c_str());
  } else {
    Serial.printf("Error fetching array: %d\n", client.getStatusCode());
  }

  // Request 2: Fetch a specific object inside the array
  client.get("/users")
        .getBody("1", &specificUser); // Extract the second user object entirely

  if (client.getStatusCode() == 200) {
    Serial.printf("Raw User Object (length: %d):\n%s\n\n", specificUser.length(), specificUser.c_str());
  } else {
    Serial.printf("Error fetching object: %d\n", client.getStatusCode());
  }

  delay(15000);
}
