---
tags:
  - example
  - crud
  - get
  - post
  - put
  - patch
  - delete
  - http
---
# Operações CRUD

Demonstra os principais métodos HTTP REST (`GET`, `POST`, `PUT`, `PATCH`, `DELETE`) em um único exemplo consolidado.

**Código-Fonte:** [`examples/RestCrud/RestCrud.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/RestCrud/RestCrud.ino)

---

## Visão Geral

| Método | Função | Descrição |
| :--- | :--- | :--- |
| `GET` | `client.get(path)` | Obtém e vincula dados de recursos remotos. |
| `POST` | `client.post(path)` | Envia dados JSON para criar um novo recurso. |
| `PUT` | `client.put(path)` / `client.update(path)` | Atualiza integralmente um recurso com os novos dados. |
| `PATCH` | `client.patch(path)` | Atualiza parcialmente campos de um recurso existente. |
| `DELETE` | `client.del(path)` | Remove um recurso no servidor remoto. |

---

## Código Completo

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

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
    // 1. GET: Ler recurso
    Serial.println("\n--- [GET] Lendo Todo #1 ---");
    int id = 0;
    int userId = 0;
    char title[64] = {0};
    bool completed = false;

    client.get("/todos/1")
          .getBody("id", &id)
          .getBody("userId", &userId)
          .getBody("title", title, sizeof(title))
          .getBody("completed", &completed);

    if (client.isSuccess()) {
        Serial.printf("GET 200 OK -> ID: %d, Usuário: %d, Título: %s, Concluído: %s\n",
                      id, userId, title, completed ? "true" : "false");
    }

    // 2. POST: Criar recurso
    Serial.println("\n--- [POST] Criando Post ---");
    int newId = 0;
    client.post("/posts")
          .body("title",  "ESP32 REST Client")
          .body("body",   "Cliente HTTP fluente para sistemas embarcados")
          .body("userId", 1)
          .getBody("id", &newId);

    if (client.isSuccess()) {
        Serial.printf("POST Criado -> Novo ID: %d\n", newId);
    }

    // 3. PUT: Atualização completa do recurso
    Serial.println("\n--- [PUT] Atualizando Post #1 ---");
    client.put("/posts/1")
          .body("id",     1)
          .body("title",  "Título Atualizado")
          .body("body",   "Conteúdo Atualizado")
          .body("userId", 1);

    if (client.isSuccess()) {
        Serial.println("PUT Atualizado -> Recurso atualizado com sucesso");
    }

    // 4. PATCH: Atualização parcial
    Serial.println("\n--- [PATCH] Atualização parcial do Post #1 ---");
    client.patch("/posts/1")
          .body("title", "Apenas Título Atualizado");

    if (client.isSuccess()) {
        Serial.println("PATCH Sucesso -> Campo de título modificado");
    }

    // 5. DELETE: Remover recurso
    Serial.println("\n--- [DELETE] Excluindo Post #1 ---");
    client.del("/posts/1");

    if (client.isSuccess()) {
        Serial.println("DELETE Sucesso -> Recurso excluído");
    }

    delay(15000);
}
```

---

## Saída Serial Esperada

```
Conectado ao WiFi

--- [GET] Lendo Todo #1 ---
GET 200 OK -> ID: 1, Usuário: 1, Título: delectus aut autem, Concluído: false

--- [POST] Criando Post ---
POST Criado -> Novo ID: 101

--- [PUT] Atualizando Post #1 ---
PUT Atualizado -> Recurso atualizado com sucesso

--- [PATCH] Atualização parcial do Post #1 ---
PATCH Sucesso -> Campo de título modificado

--- [DELETE] Excluindo Post #1 ---
DELETE Sucesso -> Recurso excluído
```

---

## Pontos Importantes

- Todos os métodos HTTP retornam um [`RestRequest`](../api/restrequest.pt.md) encadeável que é disparado ao final da linha de comando ou na chamada de `.getBody()`.
- O método `client.isSuccess()` verifica de forma simplificada se o código de resposta HTTP está entre `200` e `299`.
- O método `.body(chave, valor)` pode ser encadeado múltiplas vezes para serializar corpos JSON diretamente sem alocar documentos em memória heap.
