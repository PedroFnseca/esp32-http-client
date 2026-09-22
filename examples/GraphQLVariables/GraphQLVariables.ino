#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://api.example.com");

struct Telemetry {
  float temperature;
  float humidity;
  int batteryLevel;

  REST_JSON_MAP(
    REST_FIELD(temperature),
    REST_FIELD(humidity),
    REST_FIELD(batteryLevel)
  )
};

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
  // 1. Primitive variables
  Serial.println("\n--- [1] Query with Primitive Variables ---");
  String userName;
  bool isOnline = false;

  client.graphql("/graphql")
      .query("query GetUser($userId: ID!, $activeOnly: Boolean) { user(id: $userId, active: $activeOnly) { name online } }")
      .variable("userId", "usr_9981")
      .variable("activeOnly", true)
      .getData("user.name", &userName)
      .getData("user.online", &isOnline);

  if (client.isSuccess()) {
    Serial.printf("User: %s (Online: %s)\n", userName.c_str(), isOnline ? "yes" : "no");
  }

  // 2. Struct variable serialized via REST_JSON_MAP
  Serial.println("\n--- [2] Mutation with Struct Variable ---");
  Telemetry sensorData{24.5f, 60.2f, 95};
  bool acknowledged = false;

  client.graphql("/graphql")
      .mutation("mutation LogData($telemetry: TelemetryInput!) { recordTelemetry(input: $telemetry) { acknowledged } }")
      .variable("telemetry", sensorData)
      .getData("recordTelemetry.acknowledged", &acknowledged);

  if (client.isSuccess()) {
    Serial.printf("Telemetry record status: %s\n", acknowledged ? "Acknowledged" : "Pending");
  }

  // 3. Operation name selection from multi-operation document
  Serial.println("\n--- [3] Operation Name Selection ---");
  const char* multiDoc =
      "query GetProfile { profile { displayName } }\n"
      "query GetSettings { settings { theme autoUpdate } }";

  String theme;
  bool autoUpdate = false;

  client.graphql("/graphql")
      .document(multiDoc)
      .operationName("GetSettings")
      .getData("settings.theme", &theme)
      .getData("settings.autoUpdate", &autoUpdate);

  if (client.isSuccess()) {
    Serial.printf("Theme: %s, Auto-update: %s\n", theme.c_str(), autoUpdate ? "enabled" : "disabled");
  }

  delay(20000);
}
