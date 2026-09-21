---
title: Consumindo Web Services SOAP - SOAP 1.1 e SOAP 1.2
description: Como consumir serviços web SOAP 1.1 e SOAP 1.2 sobre HTTP/HTTPS com ESP32-HTTP-Client usando streaming XML, envelopes e tipos tipados.
keywords: SOAP ESP32 exemplo, ESP32 SOAP 1.1, ESP32 SOAP 1.2, SOAPAction ESP32, web service XML ESP32
tags:
  - examples
  - soap
  - xml
---
# Consumindo Web Services SOAP

O `ESP32-HTTP-Client` oferece suporte nativo e leve a **SOAP 1.1** e **SOAP 1.2** sem dependências pesadas de bibliotecas XML de terceiros. Envelopes e namespaces são gerados automaticamente, e a extração dos campos de resposta ocorre em streaming durante a leitura do socket.

---

## 1. Requisição SOAP 1.1 Básica

No SOAP 1.1:
- `Content-Type` é definido como `text/xml; charset=utf-8`.
- O cabeçalho HTTP `SOAPAction` é enviado automaticamente formatado (ex: `"http://example.org/Action"`).
- O payload é envolvido no envelope SOAP 1.1 padrão com `<soap:Envelope>` e `<soap:Body>`.

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

ESP32HTTPClient client("https://www.dataaccess.com");

void setup() {
  Serial.begin(115200);
  // Conectar ao WiFi...

  String words;
  client.soap("/webservicesserver/numberconversion.wso")
        .soapAction("http://www.dataaccess.com/webservicesserver/NumberToWords")
        .body("<m:NumberToWords xmlns:m=\"http://www.dataaccess.com/webservicesserver/\"><m:ubiNum>42</m:ubiNum></m:GetStockPrice>")
        .getBody("NumberToWordsResult", &words);

  if (client.isSuccess()) {
    Serial.printf("Resultado: %s\n", words.c_str());
  }
}

void loop() {}
```

---

## 2. Requisição SOAP 1.2

No SOAP 1.2:
- `Content-Type` é definido como `application/soap+xml; charset=utf-8; action="actionUri"`.
- Nenhum cabeçalho `SOAPAction` separado é enviado (a ação vai no parâmetro do Content-Type).
- O envelope usa o namespace `http://www.w3.org/2003/05/soap-envelope` com `<soap12:Envelope>`.

Para usar SOAP 1.2, utilize `.version(SOAP_1_2)` ou defina o padrão com `client.setSoapVersion(SOAP_1_2)`.

```cpp
ESP32HTTPClient client("https://api.example.com");

float temperatura = 0.0f;
int status = 0;

client.soap("/weather/soap12")
      .version(SOAP_1_2)
      .action("http://example.com/GetWeather")
      .body("<m:GetWeather xmlns:m=\"http://example.com/\"><m:City>London</m:City></m:GetWeather>")
      .getBody("Temperature", &temperatura)
      .getBody("Status", &status);

if (client.isSuccess()) {
  Serial.printf("Temperatura: %.1f C, Status: %d\n", temperatura, status);
}
```

---

## 3. Adicionando Cabeçalho SOAP XML (`<soap:Header>`)

```cpp
client.soap("/ws")
      .headerXml("<wsse:Security xmlns:wsse=\"...\"><wsse:UsernameToken>...</wsse:UsernameToken></wsse:Security>")
      .body("<m:DoOperation/>");
```

---

## 4. Parâmetros de Rota e Consulta

```cpp
client.soap("/services/{serviceId}/endpoint")
      .path("serviceId", 101)
      .query("tenant", "acme")
      .body("<m:FetchData/>")
      .getBody("Result", &resultado);
```
