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
  char firstUserName[64];
  char secondUserCity[64];

  // Example: GET request binding array fields using dot notation and index
  Serial.println("Send GET request to fetch array of users...");
  
  // URL: https://jsonplaceholder.typicode.com/users
  // Response contains an array of users:
  // [
  //   {
  //     "id": 1,
  //     "name": "Leanne Graham",
  //     "address": { "city": "Gwenborough" }
  //   },
  //   {
  //     "id": 2,
  //     "name": "Ervin Howell",
  //     "address": { "city": "Wisokyburgh" }
  //   }
  // ]
  client.get("/users")
        .getBody("0.name", firstUserName, sizeof(firstUserName))
        .getBody("1.address.city", secondUserCity, sizeof(secondUserCity));

  // Check Status
  if (client.getStatusCode() == 200) {
    Serial.printf("First User's Name: %s\n", firstUserName);
    Serial.printf("Second User's City: %s\n", secondUserCity);
  } else {
    Serial.printf("Error: %d\n", client.getStatusCode());
  }

  delay(10000);
}
