---
title: Exemplo de Mapeamento Struct para JSON no ESP32
description: Aprenda como mapear structs C++ diretamente para payloads JSON e ler respostas HTTP em variáveis de struct no ESP32 sem alocação dinâmica.
keywords: mapeamento struct JSON ESP32, serializacao struct C++ ESP32, desserializacao JSON ESP32, payload HTTP struct Arduino
tags:
  - example
  - struct
  - json
  - serialization
  - deserialization
---
# Mapeamento Struct <-> JSON

Demonstra a serialização e desserialização bidirecional entre `struct`s C++ e formato JSON sem alocações dinâmicas intermediárias.

**Código-Fonte:** [`examples/StructJson/StructJson.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/StructJson/StructJson.ino)

---

## Visão Geral

Utilizando a macro `REST_JSON_MAP` dentro da sua struct, você pode:
1. **Enviar structs diretamente no corpo da requisição:** `client.post("/todos").body(newTodo);`
2. **Preencher structs diretamente a partir da resposta HTTP:** `client.get("/todos/1").getBody(&todo);`
3. **Converter de forma independente:** `ESP32HTTPClient::toJson(obj)` e `ESP32HTTPClient::fromJson(json, &obj)`.

---

## Código Completo

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://jsonplaceholder.typicode.com");

// Declaração da struct mapeada
struct Todo {
    int id = 0;
    int userId = 0;
    char title[64] = {0};
    bool completed = false;

    REST_JSON_MAP(
        REST_FIELD(id),
        REST_FIELD(userId),
        REST_FIELD(title),
        REST_FIELD(completed)
    )
};

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
    Todo todo;

    // 1. Obtendo JSON remoto diretamente para a struct
    Serial.println("\n--- [1] Obtendo Todo como Struct ---");
    client.get("/todos/1").getBody(&todo);

    if (client.isSuccess()) {
        Serial.printf("Struct Preenchida -> ID: %d, Usuário: %d, Título: %s, Concluído: %s\n",
                      todo.id, todo.userId, todo.title, todo.completed ? "true" : "false");
    }

    // 2. Enviando uma struct no corpo JSON da requisição
    Serial.println("\n--- [2] Enviando Todo a partir de Struct ---");
    Todo newTodo;
    newTodo.userId = 1;
    newTodo.id = 101;
    strncpy(newTodo.title, "Construir dispositivo IoT incrível", sizeof(newTodo.title));
    newTodo.completed = false;

    Todo responseTodo;
    client.post("/todos")
          .body(newTodo)
          .getBody(&responseTodo);

    if (client.isSuccess()) {
        Serial.printf("Struct Enviada -> ID do Recurso Criado: %d\n", responseTodo.id);
    }

    // 3. Conversão de JSON independente de requisição HTTP
    Serial.println("\n--- [3] Conversão Struct para String JSON ---");
    String jsonString = ESP32HTTPClient::toJson(newTodo);
    Serial.printf("JSON Serializado: %s\n", jsonString.c_str());

    delay(15000);
}
```

---

## Saída Serial Esperada

```
Conectado ao WiFi

--- [1] Obtendo Todo como Struct ---
Struct Preenchida -> ID: 1, Usuário: 1, Título: delectus aut autem, Concluído: false

--- [2] Enviando Todo a partir de Struct ---
Struct Enviada -> ID do Recurso Criado: 101

--- [3] Conversão Struct para String JSON ---
JSON Serializado: {"id":101,"userId":1,"title":"Construir dispositivo IoT incrível","completed":false}
```

---

## Pontos Importantes

- Campos ausentes no JSON mantêm os valores pré-existentes/padrão da struct.
- Valores `null` no JSON são tratados com segurança sem corromper memória.
- Propriedades desconhecidas extras no JSON são automaticamente ignoradas.
