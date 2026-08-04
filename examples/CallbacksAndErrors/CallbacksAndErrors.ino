#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://jsonplaceholder.typicode.com");

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Global Callbacks on the client instance
  client.onSuccess([](int statusCode) {
    Serial.printf("[Global Callback] Success with HTTP %d\n", statusCode);
  });

  client.onError([](int errorCode, const char* message) {
    Serial.printf("[Global Callback] Error (%d): %s\n", errorCode, message);
  });

  client.onResponse([](int statusCode) {
    Serial.printf("[Global Callback] Request complete, status: %d\n", statusCode);
  });
}

void loop() {
  // 1. Per-request callbacks, custom timeout and retry configuration
  Serial.println("\n--- [1] Request with Callbacks and Timeout/Retry ---");
  int id = 0;

  client.get("/todos/1")
        .timeout(3000)   // 3 second timeout for this request
        .retry(2)        // Retry up to 2 times on network failure
        .onSuccess([](int code) {
          Serial.printf("[Request Callback] Todo fetched successfully (HTTP %d)\n", code);
        })
        .onError([](int code, const char* message) {
          Serial.printf("[Request Callback] Failed (%d): %s\n", code, message);
        })
        .getBody("id", &id);

  // 2. Error handling checks
  Serial.println("\n--- [2] Handling Error (404 Not Found) ---");
  client.get("/non_existent_endpoint");

  if (client.hasError()) {
    Serial.printf("Detected Error: HTTP %d (%s)\n",
                  client.getStatusCode(),
                  client.getErrorMessage().c_str());
  }

  // 3. Changing target URL at runtime
  Serial.println("\n--- [3] Changing URL at runtime ---");
  client.setUrl("https://httpbin.org");
  client.get("/status/200");

  if (client.isSuccess()) {
    Serial.printf("Switched to %s successfully (HTTP %d)\n",
                  client.getBaseUrl(),
                  client.getStatusCode());
  }

  // Reset back for next loop iteration
  client.setUrl("https://jsonplaceholder.typicode.com");

  delay(15000);
}
