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
  Serial.println("\n--- [1] Partial Data with GraphQL Errors ---");
  int userId = 0;
  String userName;
  GraphQLError firstError;
  std::vector<GraphQLError> allErrors;

  // The server may resolve "id" and "name" successfully while failing on another field
  client.graphql("/graphql")
      .query("query { user { id name restrictedField } }")
      .getData("user.id", &userId)
      .getData("user.name", &userName)
      .getError(&firstError)
      .getErrors(&allErrors)
      .onGraphQLError([](const std::vector<GraphQLError>& errors) {
        Serial.printf("GraphQL returned %d error(s):\n", (int)errors.size());
        for (const auto& err : errors) {
          Serial.printf(" - Message: %s\n", err.message.c_str());
          for (const auto& loc : err.locations) {
            Serial.printf("   Location: line %d, col %d\n", loc.line, loc.column);
          }
          if (!err.path.empty()) {
            Serial.print("   Path: ");
            for (size_t i = 0; i < err.path.size(); i++) {
              if (i > 0) Serial.print(".");
              Serial.print(err.path[i]);
            }
            Serial.println();
          }
          if (!err.extensions.isEmpty()) {
            Serial.printf("   Extensions: %s\n", err.extensions.c_str());
          }
        }
      });

  // Partial data is preserved!
  if (userId > 0) {
    Serial.printf("Partial data preserved -> User ID: %d, Name: %s\n", userId, userName.c_str());
  }

  Serial.println("\n--- [2] Handling HTTP 400 with GraphQL Syntax/Validation Errors ---");
  String syntaxErrorMsg;

  client.graphql("/graphql")
      .query("query InvalidSyntax { unclosedBrace")
      .getErrorMessage(&syntaxErrorMsg)
      .onError([](int statusCode, const char* statusMsg) {
        Serial.printf("HTTP Error %d: %s\n", statusCode, statusMsg);
      });

  if (!syntaxErrorMsg.isEmpty()) {
    Serial.printf("GraphQL Error Message: %s\n", syntaxErrorMsg.c_str());
  }

  delay(20000);
}
