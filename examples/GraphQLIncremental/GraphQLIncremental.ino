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
  Serial.println("\n--- [1] Incremental Delivery (@defer / multipart/mixed) ---");

  // Query with @defer directive: fast initial payload, slow fields delivered incrementally
  const char* deferredQuery =
      "query GetDashboard {\n"
      "  systemStatus\n"
      "  ... @defer {\n"
      "    heavyHistoricalMetrics { cpuUsage memoryUsage }\n"
      "  }\n"
      "}";

  String initialStatus;
  int chunkCount = 0;

  client.graphql("/graphql")
      .query(deferredQuery)
      .getData("systemStatus", &initialStatus)
      .onIncremental([&chunkCount](const GraphQLIncrementalPayload& payload) {
        chunkCount++;
        Serial.printf("[Chunk %d received] Path: '%s', hasNext: %s\n",
                      chunkCount,
                      payload.path.c_str(),
                      payload.hasNext ? "true" : "false");
        Serial.printf("Chunk Payload Data: %s\n", payload.data.c_str());
      });

  if (client.isSuccess()) {
    Serial.printf("Initial System Status: %s\n", initialStatus.c_str());
    Serial.printf("Total streaming chunks processed: %d\n", chunkCount);
  } else {
    Serial.printf("Request failed with status %d\n", client.getStatusCode());
  }

  delay(20000);
}
