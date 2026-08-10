---
title: Exemplo de Parsing de Arrays JSON por Índice no ESP32
description: Aprenda como acessar elementos específicos de um array JSON por índice diretamente em variáveis C++ no ESP32 sem carregar todo o payload.
keywords: array JSON ESP32, parser array JSON indice ESP32, elemento array HTTP ESP32, vinculo array JSON Arduino
tags:
  - example
  - json
  - array
---
# JSON Array

Demonstra como extrair elementos específicos de uma resposta em **array JSON** usando **segmentos de caminho com índices numéricos**.

**Fonte:** [`examples/ArrayJSON/ArrayJSON.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/ArrayJSON/ArrayJSON.ino)

---

## API Utilizada

**Endpoint:** `GET https://jsonplaceholder.typicode.com/users`

**Trecho relevante da resposta (array de objetos):**
```json
[
  {
    "id": 1,
    "name": "Leanne Graham",
    "address": { "city": "Gwenborough" }
  },
  {
    "id": 2,
    "name": "Ervin Howell",
    "address": { "city": "Wisokyburgh" }
  }
]
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
    char firstUserName[64];
    char secondUserCity[64];

    Serial.println("Buscando array de usuários...");

    client.get("/users")
          .getBody("0.name",         firstUserName,  sizeof(firstUserName))
          .getBody("1.address.city", secondUserCity, sizeof(secondUserCity));

    if (client.getStatusCode() == 200) {
        Serial.printf("Nome do primeiro usuário : %s\n", firstUserName);
        Serial.printf("Cidade do segundo usuário: %s\n", secondUserCity);
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
Buscando array de usuários...
Nome do primeiro usuário : Leanne Graham
Cidade do segundo usuário: Wisokyburgh
```

---

## Referência de Sintaxe de Caminho

| Caminho | O que ele resolve |
| :--- | :--- |
| `"0.name"` | Campo `name` do **primeiro** elemento (índice 0) |
| `"1.name"` | Campo `name` do **segundo** elemento (índice 1) |
| `"1.address.city"` | Campo `city` aninhado em `address` do segundo elemento |
| `"0"` (com `String*`) | O primeiro elemento inteiro como uma string JSON bruta |

!!! note "Indexação baseada em zero"
    Os índices de array são **baseados em zero**, seguindo as convenções padrão da linguagem C.
