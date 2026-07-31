---
tags:
  - example
  - post
  - http
---
# Requisição POST

Demonstra o envio de um corpo JSON em uma requisição POST e a leitura do ID do novo recurso criado na resposta do servidor.

**Fonte:** [`examples/PostRequest/PostRequest.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/PostRequest/PostRequest.ino)

---

## API Utilizada

**Endpoint:** `POST https://jsonplaceholder.typicode.com/posts`

**Corpo da Requisição:**
```json
{ "title": "foo", "body": "bar", "userId": 1 }
```

**Resposta:**
```json
{ "title": "foo", "body": "bar", "userId": 1, "id": 101 }
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
    while (WiFi.status() != WL_CONNECTED) delay(500);
    Serial.println("\nWiFi Conectado");
}

void loop() {
    int newId;

    Serial.println("Enviando POST...");

    client.post("/posts")
          .body("title", "foo")
          .body("body",  "bar")
          .body("userId", 1)
          .getBody("id", &newId);

    if (client.getStatusCode() == 201) { // 201 Created
        Serial.printf("ID do Post Criado: %d\n", newId);
    } else {
        Serial.printf("Erro: %d\n", client.getStatusCode());
    }

    delay(10000);
}
```

---

## Saída Esperada no Monitor Serial

```
WiFi Conectado
Enviando POST...
ID do Post Criado: 101
```

---

## Pontos Chave

- Cada chamada a `.body()` adiciona um campo ao corpo JSON: `{"title":"foo","body":"bar","userId":1}`.
- O cabeçalho `Content-Type: application/json` é definido automaticamente quando `.body()` é utilizado.
- Uma criação bem-sucedida retorna HTTP `201 Created`, não `200 OK`.
- `.getBody()` pode ser encadeado após `.body()` para ler campos da resposta do servidor.
