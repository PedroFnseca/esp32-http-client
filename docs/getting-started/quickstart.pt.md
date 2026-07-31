---
tags:
  - getting-started
  - quickstart
  - tutorial
---
# Início Rápido

Este guia orienta você na sua primeira requisição HTTP usando o **ESP32-HTTP-Client** em menos de 2 minutos.

---

## Passo 1: Incluir a biblioteca

```cpp
#include <WiFi.h>
#include "ESP32HTTPClient.h"
```

## Passo 2: Conectar ao WiFi

```cpp
WiFi.begin("SEU_SSID", "SUA_SENHA");
while (WiFi.status() != WL_CONNECTED) delay(100);
```

## Passo 3: Criar o cliente

```cpp
// Passe a URL base do seu servidor (com o protocolo)
ESP32HTTPClient client("https://jsonplaceholder.typicode.com");
```

## Passo 4: Fazer uma requisição

```cpp
int userId = 0;
char title[64];
bool completed;

// GET https://jsonplaceholder.typicode.com/todos/1
// Resposta: { "userId": 1, "id": 1, "title": "...", "completed": false }
client.get("/todos/1")
      .getBody("userId", &userId)
      .getBody("title", title, sizeof(title))
      .getBody("completed", &completed);
```

## Código Completo (Sketch)

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
    Serial.println("\nConectado!");
}

void loop() {
    int userId = 0;
    char title[64];
    bool completed;

    client.get("/todos/1")
          .getBody("userId", &userId)
          .getBody("title", title, sizeof(title))
          .getBody("completed", &completed);

    if (client.getStatusCode() == 200) {
        Serial.printf("ID do Usuário: %d\n", userId);
        Serial.printf("Título       : %s\n", title);
        Serial.printf("Concluído    : %s\n", completed ? "sim" : "não");
    } else {
        Serial.printf("Erro HTTP: %d\n", client.getStatusCode());
    }

    delay(10000);
}
```

---

## O que acabou de acontecer?

1. Você criou um `ESP32HTTPClient` apontando para uma URL base.
2. Você chamou `.get("/todos/1")` para iniciar a construção de uma requisição GET.
3. Você encadeou chamadas `.getBody()` para declarar *onde* cada campo JSON deve ser armazenado.
4. Quando o encadeamento saiu de escopo, a biblioteca enviou automaticamente a requisição, analisou o fluxo de dados e escreveu os valores diretamente em suas variáveis.

!!! tip "Sem strings intermediárias"
    O corpo da resposta **nunca é armazenado por inteiro na memória**. A biblioteca analisa o fluxo JSON byte a byte à medida que chega pela rede, gravando cada valor correspondente diretamente na variável alvo.

---

## Próximos Passos

- [Guia: Fazendo Requisições](../guide/requests.pt.md)
- [Guia: Lendo Respostas](../guide/responses.pt.md)
- [Todos os Exemplos](../examples/index.pt.md)
