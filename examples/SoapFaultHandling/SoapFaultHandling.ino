#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://example.com");

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
  Serial.println("\n--- Testing SOAP Fault Handling ---");

  SoapFault fault;
  String rawResponse;

  client.soap("/soap-endpoint")
      .soapAction("http://example.com/InvalidOperation")
      .body("<m:InvalidOperation xmlns:m=\"http://example.com/\"/>")
      .getFault(&fault)
      .getRawResponse(&rawResponse)
      .onFault([](const SoapFault& f) {
        Serial.printf("[Fault Callback] Code: %s\n", f.faultCode.c_str());
        Serial.printf("[Fault Callback] String / Reason: %s\n", f.faultString.c_str());
        Serial.printf("[Fault Callback] Actor / Node: %s\n", f.faultActor.c_str());
        Serial.printf("[Fault Callback] Detail: %s\n", f.detail.c_str());
      })
      .onError([](int statusCode, const char* errorMsg) {
        Serial.printf("[Error Callback] HTTP Status %d: %s\n", statusCode, errorMsg);
      })
      .onSuccess([](int statusCode) {
        Serial.printf("[Success Callback] Status %d\n", statusCode);
      });

  if (fault.matched) {
    Serial.println("\nSOAP Fault detected in response:");
    Serial.printf("  Fault Code   : %s\n", fault.faultCode.c_str());
    Serial.printf("  Fault String : %s\n", fault.faultString.c_str());
    Serial.printf("  Fault Actor  : %s\n", fault.faultActor.c_str());
    Serial.printf("  Fault Detail : %s\n", fault.detail.c_str());
  } else if (client.isSuccess()) {
    Serial.println("SOAP request executed successfully without faults.");
  }

  delay(15000);
}
