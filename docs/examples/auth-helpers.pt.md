---
tags:
  - example
  - auth
  - bearer
  - basic
  - api-key
---
# Helpers de Autenticação

Demonstra como autenticar requisições utilizando tokens Bearer / JWT, HTTP Basic Auth e cabeçalhos de API Key customizados.

**Código-Fonte:** [`examples/AuthHelpers/AuthHelpers.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/AuthHelpers/AuthHelpers.ino)

---

## Visão Geral

O cliente disponibiliza métodos dedicados que configuram cabeçalhos de autorização persistentes para todas as requisições subsequentes:

| Método Helper | Cabeçalho Gerado | Descrição |
| :--- | :--- | :--- |
| `client.bearer(token)` | `Authorization: Bearer <token>` | Para tokens JWT e OAuth2 Bearer. |
| `client.basic(user, pass)` | `Authorization: Basic <base64>` | Codifica as credenciais automaticamente em Base64. |
| `client.apiKey(nome, chave)` | `<nome>: <chave>` | Define cabeçalhos de API Key (ex: `X-API-Key`). |
| `client.setHeader(nome, val)` | `<nome>: <val>` | Define qualquer cabeçalho HTTP genérico persistente. |

---

## Código Completo

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

ESP32HTTPClient client("https://httpbin.org");

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
    char authHeader[128] = {0};

    // 1. Autenticação por Token Bearer
    Serial.println("\n--- [1] Testando Token Bearer ---");
    client.bearer("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.sampleToken");
    client.get("/bearer").getBody("token", authHeader, sizeof(authHeader));

    if (client.isSuccess()) {
        Serial.printf("Bearer Autenticado -> Token validado pelo servidor\n");
    }

    // 2. Autenticação HTTP Basic (Usuário + Senha)
    Serial.println("\n--- [2] Testando HTTP Basic Auth ---");
    client.basic("admin", "secret123");
    client.get("/basic-auth/admin/secret123");

    if (client.isSuccess()) {
        Serial.println("Basic Auth Sucesso -> 200 OK");
    }

    // 3. Autenticação por Cabeçalho API Key
    Serial.println("\n--- [3] Testando Cabeçalho API Key ---");
    client.apiKey("X-API-KEY", "my-super-secret-api-key-9988");
    client.get("/headers").getBody("headers.X-Api-Key", authHeader, sizeof(authHeader));

    if (client.isSuccess()) {
        Serial.printf("API Key Enviada -> Servidor recebeu: %s\n", authHeader);
    }

    delay(15000);
}
```

---

## Saída Serial Esperada

```
Conectado ao WiFi

--- [1] Testando Token Bearer ---
Bearer Autenticado -> Token validado pelo servidor

--- [2] Testando HTTP Basic Auth ---
Basic Auth Sucesso -> 200 OK

--- [3] Testando Cabeçalho API Key ---
API Key Enviada -> Servidor recebeu: my-super-secret-api-key-9988
```

---

## Pontos Importantes

- Cabeçalhos de autenticação configurados com `bearer()`, `basic()`, `apiKey()` ou `setHeader()` **persistem** para todas as requisições feitas pela mesma instância de cliente.
- `client.basic()` codifica o usuário e a senha internamente em Base64, dispensando bibliotecas e passos adicionais.
