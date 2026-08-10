---
title: Exemplo de Parsing de Objetos JSON Aninhados no ESP32
description: Exemplo demonstrando como extrair campos JSON profundamente aninhados utilizando notação por ponto sem alocação dinâmica no ESP32.
keywords: JSON aninhado ESP32, notacao por ponto JSON ESP32, parser JSON complexo ESP32, parser JSON zero heap
tags:
  - example
  - json
  - nested
---
# JSON Aninhado

Demonstra como extrair campos de objetos JSON profundamente aninhados usando caminhos com **notação de ponto**.

**Fonte:** [`examples/NestedJSON/NestedJSON.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/NestedJSON/NestedJSON.ino)

---

## API Utilizada

**Endpoint:** `GET https://jsonplaceholder.typicode.com/users/1`

**Trecho relevante da resposta:**
```json
{
  "id": 1,
  "name": "Leanne Graham",
  "address": {
    "street": "Kulas Light",
    "suite": "Apt. 556",
    "city": "Gwenborough",
    "zipcode": "92998-3874",
    "geo": {
      "lat": "-37.3159",
      "lng": "81.1496"
    }
  }
}
```

---

## Sketch

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "SEU_SSID";
const char* password = "SUA_SENHA";

ESP32HTTPClient client("https://jsonplaceholder.typicode.com");

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConectado ao WiFi");
}

void loop() {
    char street[64];
    char city[64];
    char lat[32];
    char lng[32];

    Serial.println("Buscando endereço aninhado do usuário...");

    client.get("/users/1")
          .getBody("address.street",      street, sizeof(street))
          .getBody("address.city",        city,   sizeof(city))
          .getBody("address.geo.lat",     lat,    sizeof(lat))
          .getBody("address.geo.lng",     lng,    sizeof(lng));

    if (client.getStatusCode() == 200) {
        Serial.printf("Rua      : %s\n", street);
        Serial.printf("Cidade   : %s\n", city);
        Serial.printf("Latitude : %s\n", lat);
        Serial.printf("Longitude: %s\n", lng);
    } else {
        Serial.printf("Erro: %d\n", client.getStatusCode());
    }

    delay(10000);
}
```

---

## Saída Esperada no Monitor Serial

```
Conectado ao WiFi
Buscando endereço aninhado do usuário...
Rua      : Kulas Light
Cidade   : Gwenborough
Latitude : -37.3159
Longitude: 81.1496
```

---

## Pontos Chave

- Use `.` como separador para navegar por objetos aninhados: `"address.city"`.
- O aninhamento pode ter qualquer profundidade: `"address.geo.lat"` navega por dois níveis.
- A biblioteca resolve cada segmento do caminho em ordem durante a análise do fluxo.
- Não há limite prático para a profundidade de aninhamento.
