#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://httpbin.org");

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
  char authHeader[128] = {0};

  // 1. Bearer Token Authentication
  Serial.println("\n--- [1] Testing Bearer Token ---");
  client.bearer("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.sampleToken");
  client.get("/bearer").getBody("token", authHeader, sizeof(authHeader));

  if (client.isSuccess()) {
    Serial.printf("Bearer Authenticated -> Token verified by server\n");
  }

  // 2. HTTP Basic Authentication (Username + Password)
  Serial.println("\n--- [2] Testing HTTP Basic Auth ---");
  client.basic("admin", "secret123");
  client.get("/basic-auth/admin/secret123");

  if (client.isSuccess()) {
    Serial.println("Basic Auth Success -> 200 OK");
  }

  // 3. API Key Header Authentication
  Serial.println("\n--- [3] Testing API Key Header ---");
  client.apiKey("X-API-KEY", "my-super-secret-api-key-9988");
  client.get("/headers").getBody("headers.X-Api-Key", authHeader, sizeof(authHeader));

  if (client.isSuccess()) {
    Serial.printf("API Key Sent -> Server received: %s\n", authHeader);
  }

  delay(15000);
}
