---
tags:
  - example
  - put
  - delete
  - http
---
# PUT & DELETE

Demonstra como atualizar e excluir recursos remotos usando os métodos `PUT` e `DELETE`.

**Fontes:**
- [`examples/PutRequest/PutRequest.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/PutRequest/PutRequest.ino)
- [`examples/DeleteRequest/DeleteRequest.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/DeleteRequest/DeleteRequest.ino)

---

## PUT — Atualizar um Recurso

O método `put()` e seu alias `update()` enviam uma requisição HTTP `PUT`. O corpo é construído da mesma forma que no `POST`, utilizando chamadas a `.body()`.

```cpp
// Atualizar o post #1 com um novo título e corpo
client.put("/posts/1")
      .body("id", 1)
      .body("title", "título atualizado")
      .body("body",  "conteúdo atualizado")
      .body("userId", 1);

if (client.getStatusCode() == 200) {
    Serial.println("Recurso atualizado com sucesso.");
}
```

### Usando o alias `update()`

`update()` é um alias semântico para `put()` — ambos produzem exatamente a mesma requisição HTTP `PUT`:

```cpp
// Estas duas chamadas são idênticas
client.put("/lights/1").body("state", "OFF");
client.update("/lights/1").body("state", "OFF");
```

---

## PATCH — Atualização Parcial

Use `patch()` para atualizações parciais de recursos (apenas os campos enviados são alterados):

```cpp
client.patch("/posts/1")
      .body("title", "apenas novo título");
```

---

## DELETE — Remover um Recurso

O método `del()` envia uma requisição HTTP `DELETE`. Ele pode ser usado sem corpo:

```cpp
client.del("/posts/1");

if (client.getStatusCode() == 200) {
    Serial.println("Recurso excluído.");
}
```

Requisições DELETE podem incluir um corpo opcional se a API exigir:

```cpp
client.del("/sessions")
      .body("userId", 42);
```

---

## Sketch Completo — PUT depois DELETE

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
    // Atualização
    client.put("/posts/1")
          .body("id",     1)
          .body("title",  "título atualizado")
          .body("body",   "conteúdo atualizado")
          .body("userId", 1);

    Serial.printf("Status PUT   : %d\n", client.getStatusCode());

    // Exclusão
    client.del("/posts/1");
    Serial.printf("Status DELETE: %d\n", client.getStatusCode());

    delay(30000);
}
```

---

## Saída Esperada no Monitor Serial

```
WiFi Conectado
Status PUT   : 200
Status DELETE: 200
```
