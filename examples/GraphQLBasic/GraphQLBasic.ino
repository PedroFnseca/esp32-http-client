#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://countries.trevorblades.com");

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
  Serial.println("\n--- [1] GraphQL Query (POST) ---");
  String countryName;
  String countryCapital;

  client.graphql("/")
      .query("query { country(code: \"BR\") { name capital } }")
      .getData("country.name", &countryName)
      .getData("country.capital", &countryCapital);

  if (client.isSuccess()) {
    Serial.printf("Country: %s, Capital: %s\n", countryName.c_str(), countryCapital.c_str());
  } else {
    Serial.printf("Error: Status %d - %s\n", client.getStatusCode(), client.getErrorMessage().c_str());
  }

  Serial.println("\n--- [2] GraphQL Query via HTTP GET ---");
  String countryCurrency;

  client.graphqlGet("/")
      .query("query { country(code: \"US\") { currency } }")
      .getData("country.currency", &countryCurrency);

  if (client.isSuccess()) {
    Serial.printf("US Currency: %s\n", countryCurrency.c_str());
  }

  Serial.println("\n--- [3] GraphQL Mutation (POST) ---");
  int createdId = 0;
  String status;

  ESP32HTTPClient mutationClient("https://api.example.com");
  mutationClient.graphql("/graphql")
      .mutation("mutation AddDevice { registerDevice(type: \"ESP32\", label: \"Sensor-01\") { id status } }")
      .getData("registerDevice.id", &createdId)
      .getData("registerDevice.status", &status);

  delay(15000);
}
