---
title: Consuming SOAP Web Services - SOAP 1.1 & SOAP 1.2
description: How to consume SOAP 1.1 and SOAP 1.2 web services over HTTP/HTTPS with ESP32-HTTP-Client using streaming XML responses, envelopes, and type bindings.
keywords: SOAP ESP32 example, ESP32 SOAP 1.1, ESP32 SOAP 1.2, SOAPAction ESP32, XML web service ESP32
tags:
  - examples
  - soap
  - xml
---
# Consuming SOAP Web Services

`ESP32-HTTP-Client` provides first-class, lightweight SOAP 1.1 and SOAP 1.2 support without heavy third-party XML libraries. Envelopes and namespaces are generated automatically, and response XML fields are extracted on-the-fly via streaming.

---

## 1. Basic SOAP 1.1 Request

In SOAP 1.1:
- `Content-Type` is set to `text/xml; charset=utf-8`.
- The `SOAPAction` HTTP header is automatically formatted and sent (e.g. `"http://example.org/Action"`).
- The payload is automatically wrapped in a standard SOAP 1.1 Envelope with `<soap:Envelope>` and `<soap:Body>`.

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

ESP32HTTPClient client("https://www.dataaccess.com");

void setup() {
  Serial.begin(115200);
  // Connect to WiFi...

  String words;
  client.soap("/webservicesserver/numberconversion.wso")
        .soapAction("http://www.dataaccess.com/webservicesserver/NumberToWords")
        .body("<m:NumberToWords xmlns:m=\"http://www.dataaccess.com/webservicesserver/\"><m:ubiNum>42</m:ubiNum></m:GetStockPrice>")
        .getBody("NumberToWordsResult", &words);

  if (client.isSuccess()) {
    Serial.printf("Result: %s\n", words.c_str());
  }
}

void loop() {}
```

---

## 2. SOAP 1.2 Request

In SOAP 1.2:
- `Content-Type` is set to `application/soap+xml; charset=utf-8; action="actionUri"`.
- No separate `SOAPAction` HTTP header is sent (action is embedded in Content-Type per SOAP 1.2 spec).
- Envelope uses the `http://www.w3.org/2003/05/soap-envelope` namespace with `<soap12:Envelope>`.

To use SOAP 1.2, specify `.version(SOAP_1_2)` or call `client.setSoapVersion(SOAP_1_2)`.

```cpp
ESP32HTTPClient client("https://api.example.com");

float temperature = 0.0f;
int status = 0;

client.soap("/weather/soap12")
      .version(SOAP_1_2)
      .action("http://example.com/GetWeather")
      .body("<m:GetWeather xmlns:m=\"http://example.com/\"><m:City>London</m:City></m:GetWeather>")
      .getBody("Temperature", &temperature)
      .getBody("Status", &status);

if (client.isSuccess()) {
  Serial.printf("Temperature: %.1f C, Status: %d\n", temperature, status);
}
```

---

## 3. Adding SOAP Header XML

If your service requires SOAP headers (such as security tokens, transaction IDs, or routing info):

```cpp
client.soap("/ws")
      .headerXml("<wsse:Security xmlns:wsse=\"...\"><wsse:UsernameToken>...</wsse:UsernameToken></wsse:Security>")
      .body("<m:DoOperation/>");
```

---

## 4. Path and Query Parameters

You can use the same URL interpolation features as `RestRequest`:

```cpp
client.soap("/services/{serviceId}/endpoint")
      .path("serviceId", 101)
      .query("tenant", "acme")
      .body("<m:FetchData/>")
      .getBody("Result", &result);
```
