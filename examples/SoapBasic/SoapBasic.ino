#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://www.dataaccess.com");

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
  // Example 1: SOAP 1.1 request to convert a number to words
  Serial.println("\n--- [1] SOAP 1.1 Number Conversion ---");
  String resultText;

  client.soap("/webservicesserver/numberconversion.wso")
      .soapAction("http://www.dataaccess.com/webservicesserver/NumberToWords")
      .body("<m:NumberToWords xmlns:m=\"http://www.dataaccess.com/webservicesserver/\"><m:ubiNum>42</m:ubiNum></m:GetStockPrice>")
      .getBody("NumberToWordsResult", &resultText);

  if (client.isSuccess()) {
    Serial.printf("SOAP 1.1 Response -> %s\n", resultText.c_str());
  } else {
    Serial.printf("SOAP 1.1 Error -> Status %d: %s\n", client.getStatusCode(), client.getErrorMessage().c_str());
  }

  // Example 2: SOAP 1.2 request with version setter
  Serial.println("\n--- [2] SOAP 1.2 Request ---");
  String resultSoap12;

  client.soap("/webservicesserver/numberconversion.wso")
      .version(SOAP_1_2)
      .action("http://www.dataaccess.com/webservicesserver/NumberToWords")
      .body("<m:NumberToWords xmlns:m=\"http://www.dataaccess.com/webservicesserver/\"><m:ubiNum>100</m:ubiNum></m:GetStockPrice>")
      .getBody("NumberToWordsResult", &resultSoap12);

  if (client.isSuccess()) {
    Serial.printf("SOAP 1.2 Response -> %s\n", resultSoap12.c_str());
  } else {
    Serial.printf("SOAP 1.2 Error -> Status %d: %s\n", client.getStatusCode(), client.getErrorMessage().c_str());
  }

  delay(15000);
}
