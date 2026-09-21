---
title: Tratamento de SOAP Faults - Inspeção de Erros e Callbacks
description: Como detectar e extrair elementos SOAP 1.1 e 1.2 Fault, inspecionar códigos de erro e motivos, e tratar falhas no ESP32.
keywords: SOAP Fault ESP32, erro SOAP, callback onFault, faultcode faultstring ESP32
tags:
  - examples
  - soap
  - error-handling
---
# Tratamento de SOAP Faults

Quando um serviço web SOAP não consegue processar uma requisição, ele retorna uma resposta XML contendo um elemento `<Fault>` (ou `<soap:Fault>`), geralmente acompanhado de status HTTP 500.

O `ESP32-HTTP-Client` detecta automaticamente SOAP Faults nos formatos SOAP 1.1 e SOAP 1.2, extrai os campos em streaming e dispara o callback `.onFault(...)`.

---

## 1. Inspecionando a struct `SoapFault`

Vincule uma instância de `SoapFault` via `.getFault(&fault)`:

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
    Serial.println("\n--- Falha SOAP Detectada ---");
    Serial.printf("Código da Falha : %s\n", fault.faultCode.c_str());
    Serial.printf("Mensagem        : %s\n", fault.faultString.c_str());
    Serial.printf("Ator / Nó       : %s\n", fault.faultActor.c_str());
    Serial.printf("Detalhes        : %s\n", fault.detail.c_str());
  } else if (client.isSuccess()) {
    Serial.println("Pedido enviado com sucesso!");
  }
}

void loop() {}
```

---

## 2. Usando o Callback `.onFault(...)`

```cpp
client.soap("/payments")
      .version(SOAP_1_2)
      .action("http://example.com/Charge")
      .body("<m:Charge><m:Amount>100</m:Amount></m:Charge>")
      .onFault([](const SoapFault& fault) {
          Serial.printf("Falha SOAP no Pagamento: [%s] %s\n",
                        fault.faultCode.c_str(),
                        fault.faultString.c_str());
      })
      .onError([](int statusCode, const char* message) {
          Serial.printf("Erro HTTP %d: %s\n", statusCode, message);
      })
      .onSuccess([](int statusCode) {
          Serial.println("Pagamento processado com sucesso!");
      });
```

---

## 3. Mapeamento entre SOAP 1.1 e SOAP 1.2

| Campo em `SoapFault` | Elemento SOAP 1.1 | Elemento SOAP 1.2 |
| :--- | :--- | :--- |
| `faultCode` | `<faultcode>` | `<Code><Value>` |
| `faultString` | `<faultstring>` | `<Reason><Text>` |
| `faultActor` | `<faultactor>` | `<Node>` ou `<Role>` |
| `detail` | `<detail>` | `<Detail>` |
| `matched` | `true` se `<Fault>` encontrado | `true` se `<Fault>` encontrado |
