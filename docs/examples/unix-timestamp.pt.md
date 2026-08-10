---
title: Exemplo de Leitura de Timestamps Unix de 64 bits em API REST no ESP32
description: Exemplo demonstrando como ler timestamps Unix de 64 bits de uma API de tempo diretamente em uma variável long no ESP32.
keywords: timestamp Unix ESP32, obter hora por API HTTP ESP32, hora Unix 64 bits Arduino ESP32, sincronizar hora REST ESP32
tags:
  - example
  - timestamp
  - time
---
# Timestamp Unix

Demonstra como buscar um **timestamp Unix** (um inteiro grande do tipo `long`) de uma API pública de horário.

**Fonte:** [`examples/UnixTimestamp/UnixTimestamp.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/UnixTimestamp/UnixTimestamp.ino)

---

## Por que `long`?

Timestamps Unix representam o número de segundos transcorridos desde 1º de janeiro de 1970. Atualmente, esse valor ultrapassa `1.700.000.000` — o que estoura um `int` de 32 bits em algumas plataformas. Sempre use `long` para timestamps.

```cpp
long timestamp = 0;
// ✅ Correto — número grande cabe em um long
client.get("/time").getBody("unix_timestamp", &timestamp);

int bad = 0;
// ❌ Evitar — estourará para datas após 19 de janeiro de 2038
client.get("/time").getBody("unix_timestamp", &bad);
```

---

## API Utilizada

**Endpoint:** `GET https://timeapi.io/api/v1/time/current/unix`

**Resposta:**
```json
{
  "unix_timestamp": 1721156604
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

ESP32HTTPClient client("https://timeapi.io", 443);

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
    long ts = 0;

    Serial.println("Buscando timestamp Unix...");

    client.get("/api/v1/time/current/unix")
          .getBody("unix_timestamp", &ts);

    if (client.getStatusCode() == 200) {
        Serial.printf("Timestamp Unix: %ld\n", ts);
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
Buscando timestamp Unix...
Timestamp Unix: 1721156604
```

---

## Pontos Chave

- Use `%ld` (e não `%d`) no `printf` para valores do tipo `long`.
- A porta explícita `443` é passada para o construtor — útil ao conectar-se a APIs HTTPS.
- Vinculações `long` suportam valores de até `2^63 - 1` no ESP32 (`long` de 64 bits).
