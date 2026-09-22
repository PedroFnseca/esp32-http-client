#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://api.example.com");

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
  Serial.println("\n--- [1] Batched GraphQL Operations ---");

  // Create a batch request that bundles multiple queries into one HTTP POST request
  auto batch = client.graphqlBatch("/graphql");

  String deviceName;
  int unreadNotifications = 0;
  String weatherSummary;

  // Operation 1: Get device status
  auto& op1 = batch.addQuery("query { device(id: \"esp32-01\") { name } }");
  op1.getData("device.name", &deviceName);

  // Operation 2: Get notification count
  auto& op2 = batch.addQuery("query { notificationCount }");
  op2.getData("notificationCount", &unreadNotifications);

  // Operation 3: Get weather report
  auto& op3 = batch.addQuery("query { weather(city: \"Lisbon\") { summary } }");
  op3.getData("weather.summary", &weatherSummary);

  // Execute batch
  batch.execute();

  if (client.isSuccess()) {
    Serial.println("Batch request executed successfully in a single HTTP round-trip:");
    Serial.printf(" - Device: %s\n", deviceName.c_str());
    Serial.printf(" - Unread Notifications: %d\n", unreadNotifications);
    Serial.printf(" - Weather: %s\n", weatherSummary.c_str());
  } else {
    Serial.printf("Batch request failed with HTTP status: %d\n", client.getStatusCode());
  }

  delay(20000);
}
