---
tags:
  - example
  - get
  - http
---
# GET Simples

Demonstra uma requisição HTTP GET básica e a vinculação de múltiplos campos de uma resposta JSON em variáveis C tipadas.

**Fonte:** [`examples/SimpleGET/SimpleGET.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/SimpleGET/SimpleGET.ino)

---

## API Utilizada

**Endpoint:** `GET https://jsonplaceholder.typicode.com/todos/1`

**Resposta:**
```json
{
  "userId": 1,
  "id": 1,
  "title": "delectus aut autem",
  "completed": false
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
    int userId;
    int id;
    char title[64];
    bool completed;

    Serial.println("Enviando requisição GET...");

    client.get("/todos/1")
          .getBody("userId",    &userId)
          .getBody("id",        &id)
          .getBody("title",     title, sizeof(title))
          .getBody("completed", &completed);

    if (client.getStatusCode() == 200) {
        Serial.printf("ID do Usuário: %d\n", userId);
        Serial.printf("ID           : %d\n", id);
        Serial.printf("Título       : %s\n", title);
        Serial.printf("Concluído    : %s\n", completed ? "true" : "false");
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
Enviando requisição GET...
ID do Usuário: 1
ID           : 1
Título       : delectus aut autem
Concluído    : false
```

---

## Pontos Chave

- `int` e `bool` são passados por ponteiro usando `&`.
- Buffers de `char` são passados com o ponteiro do buffer e seu tamanho — a biblioteca nunca grava além do limite.
- `getStatusCode()` é verificado após a requisição para proteger contra erros de rede.
