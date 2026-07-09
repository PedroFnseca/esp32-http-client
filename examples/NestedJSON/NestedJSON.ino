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
  char street[64];
  char city[64];
  char lat[32];
  char lng[32];

  // Example: GET request binding nested fields using dot notation
  Serial.println("Send GET request to fetch nested user address...");
  
  // URL: https://jsonplaceholder.typicode.com/users/1
  // Response contains:
  // "address": {
  //   "street": "Kulas Light",
  //   "suite": "Apt. 556",
  //   "city": "Gwenborough",
  //   "zipcode": "92998-3874",
  //   "geo": {
  //     "lat": "-37.3159",
  //     "lng": "81.1496"
  //   }
  // }
  client.get("/users/1")
        .getBody("address.street", street, sizeof(street))
        .getBody("address.city", city, sizeof(city))
        .getBody("address.geo.lat", lat, sizeof(lat))
        .getBody("address.geo.lng", lng, sizeof(lng));

  // Check Status
  if (client.getStatusCode() == 200) {
    Serial.printf("Street: %s\n", street);
    Serial.printf("City: %s\n", city);
    Serial.printf("Latitude: %s\n", lat);
    Serial.printf("Longitude: %s\n", lng);
  } else {
    Serial.printf("Error: %d\n", client.getStatusCode());
  }

  delay(10000);
}
