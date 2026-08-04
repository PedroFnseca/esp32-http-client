---
tags:
  - example
  - url
  - query
  - path
  - parameters
---
# Parâmetros de URL

Demonstra como utilizar Parâmetros de Caminho (Path Parameters, ex: `/users/{id}`) e Parâmetros de Consulta (Query Parameters, ex: `?page=2&limit=20`) de forma fluente.

**Código-Fonte:** [`examples/UrlParameters/UrlParameters.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/UrlParameters/UrlParameters.ino)

---

## Visão Geral

| Método | Sintaxe | Descrição |
| :--- | :--- | :--- |
| `.path(chave, valor)` | `client.get("/users/{id}").path("id", 15)` | Substitui placeholders na rota pelos valores fornecidos (`/users/15`). |
| `.query(chave, valor)` | `client.get("/users").query("page", 2)` | Anexa parâmetros de query string na URL (`/users?page=2`). |

Ambos os métodos suportam todos os tipos de dados fundamentais em C/C++: `int`, `long`, `float`, `double`, `bool`, `const char*` e `char*`.

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
    // 1. Parâmetros de Caminho: GET /users/{id} -> GET /users/15
    Serial.println("\n--- [1] Exemplo de Path Parameters ---");
    int userId = 15;
    char name[64] = {0};

    // Substitui {id} na rota por 15
    client.get("/users/{id}")
          .path("id", userId)
          .getBody("name", name, sizeof(name));

    if (client.isSuccess()) {
        Serial.printf("Path param resolvido -> Usuário: %s\n", name);
    }

    // 2. Parâmetros de Consulta: GET /posts?userId=1&_limit=2
    Serial.println("\n--- [2] Exemplo de Query Parameters ---");
    char firstTitle[64] = {0};

    // Constrói a URL: /posts?userId=1&_limit=2
    client.get("/posts")
          .query("userId", 1)
          .query("_limit", 2)
          .getBody("0.title", firstTitle, sizeof(firstTitle));

    if (client.isSuccess()) {
        Serial.printf("Query params filtrados -> Título do 1º Post: %s\n", firstTitle);
    }

    // 3. Combinação de Path e Query Parameters
    Serial.println("\n--- [3] Path + Query Combinados ---");
    // Constrói a URL: /users/1/posts?_limit=5&sort=desc
    client.get("/users/{userId}/posts")
          .path("userId", 1)
          .query("_limit", 5)
          .query("sort", "desc");

    if (client.isSuccess()) {
        Serial.println("Path e Query combinados executados com sucesso!");
    }

    delay(15000);
}
```

---

## Saída Serial Esperada

```
Conectado ao WiFi

--- [1] Exemplo de Path Parameters ---
Path param resolvido -> Usuário: Chelsey Dietrich

--- [2] Exemplo de Query Parameters ---
Query params filtrados -> Título do 1º Post: sunt aut facere repellat provident occaecati excepturi optio reprehenderit

--- [3] Path + Query Combinados ---
Path e Query combinados executados com sucesso!
```

---

## Pontos Importantes

- O método `.path("chave", val)` substitui tanto `{chave}` quanto `chave` na URL.
- Múltiplas chamadas a `.query("chave", val)` gerenciam o caractere `?` inicial e `&` subsequentes automaticamente.
- Elimina a necessidade de concatenações manuais de strings ou buffers com `sprintf`.
