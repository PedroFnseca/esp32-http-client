---
title: Handling SOAP Faults - Error Inspection & Callbacks
description: How to detect and extract SOAP 1.1 and SOAP 1.2 Fault elements, inspect fault codes and reasons, and handle errors on ESP32.
keywords: SOAP Fault ESP32, SOAP error handling, onFault callback, faultcode faultstring ESP32
tags:
  - examples
  - soap
  - error-handling
---
# Handling SOAP Faults

When a SOAP web service fails to process a request, it returns an XML response containing a `<Fault>` (or `<soap:Fault>`) element, often accompanied by an HTTP 500 status code.

`ESP32-HTTP-Client` automatically detects SOAP Faults in both SOAP 1.1 and SOAP 1.2 formats, extracts the fault fields in a streaming fashion, and dispatches dedicated `.onFault(...)` callbacks.

---

## 1. Inspecting `SoapFault` Struct

You can bind a `SoapFault` instance using `.getFault(&fault)`:

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

ESP32HTTPClient client("https://api.example.com");

void setup() {
  Serial.begin(115200);

  SoapFault fault;

  client.soap("/orders")
        .soapAction("http://example.com/SubmitOrder")
        .body("<m:SubmitOrder><m:OrderId>-1</m:OrderId></m:SubmitOrder>")
        .getFault(&fault);

  if (fault.matched) {
    Serial.println("\n--- SOAP Fault Encountered ---");
    Serial.printf("Fault Code   : %s\n", fault.faultCode.c_str());
    Serial.printf("Fault String : %s\n", fault.faultString.c_str());
    Serial.printf("Fault Actor  : %s\n", fault.faultActor.c_str());
    Serial.printf("Fault Detail : %s\n", fault.detail.c_str());
  } else if (client.isSuccess()) {
    Serial.println("Order submitted successfully!");
  }
}

void loop() {}
```

---

## 2. Using the `.onFault(...)` Callback

Alternatively, attach an asynchronous-style lambda callback:

```cpp
client.soap("/payments")
      .version(SOAP_1_2)
      .action("http://example.com/Charge")
      .body("<m:Charge><m:Amount>100</m:Amount></m:Charge>")
      .onFault([](const SoapFault& fault) {
          Serial.printf("Payment SOAP Fault: [%s] %s\n",
                        fault.faultCode.c_str(),
                        fault.faultString.c_str());
      })
      .onError([](int statusCode, const char* message) {
          Serial.printf("HTTP Error %d: %s\n", statusCode, message);
      })
      .onSuccess([](int statusCode) {
          Serial.println("Payment processed OK!");
      });
```

---

## 3. SOAP 1.1 vs SOAP 1.2 Fault Mapping

The `SoapFault` struct normalizes differences between SOAP 1.1 and SOAP 1.2:

| `SoapFault` Field | SOAP 1.1 Element | SOAP 1.2 Element |
| :--- | :--- | :--- |
| `faultCode` | `<faultcode>` | `<Code><Value>` |
| `faultString` | `<faultstring>` | `<Reason><Text>` |
| `faultActor` | `<faultactor>` | `<Node>` or `<Role>` |
| `detail` | `<detail>` | `<Detail>` |
| `matched` | `true` if `<Fault>` found | `true` if `<Fault>` found |
