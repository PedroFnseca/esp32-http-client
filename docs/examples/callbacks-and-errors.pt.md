---
title: Exemplo de Callbacks HTTP, Retries por Timeout e Erros no ESP32
description: Aprenda como tratar erros HTTP, configurar retries automáticos por timeout e registrar callbacks de término de requisição no ESP32.
keywords: tratamento de erro HTTP ESP32, retry timeout HTTP ESP32, callbacks HTTP ESP32, codigo status HTTP Arduino ESP32
tags:
  - example
  - callbacks
  - timeout
  - retry
  - errors
---
# Callbacks e Tratamento de Erros

Demonstra o uso de callbacks globais e por requisição (`onSuccess`, `onError`, `onResponse`), configuração de tempo limite (timeout), tentativas automáticas (retries), alteração de URL em runtime e diagnóstico de erros.

**Código-Fonte:** [`examples/CallbacksAndErrors/CallbacksAndErrors.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/CallbacksAndErrors/CallbacksAndErrors.ino)

---

## Visão Geral

| Recurso | Sintaxe | Descrição |
| :--- | :--- | :--- |
| **Callback de Sucesso** | `.onSuccess([](int code){ ... })` | Executado quando a requisição retorna HTTP 2xx. |
| **Callback de Erro** | `.onError([](int code, const char* msg){ ... })` | Executado em falhas (`code < 200 \|\| code >= 400`). |
| **Callback de Resposta** | `.onResponse([](int code){ ... })` | Executado no término de qualquer requisição. |
| **Timeout** | `.timeout(3000)` / `client.setTimeout(5000)` | Configura o tempo limite de rede em milissegundos. |
| **Max Retry** | `.retry(2)` / `client.setMaxRetry(3)` | Número de tentativas automáticas em caso de falha de conexão. |
| **URL em Runtime** | `client.setUrl("https://api.v2.com")` | Redireciona dinamicamente a base URL do cliente. |
| **Verificação de Estado** | `client.isSuccess()`, `client.hasError()` | Métodos booleanos auxiliares para checar o status. |

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

    // Callbacks Globais registrados na instância do cliente
    client.onSuccess([](int statusCode) {
        Serial.printf("[Callback Global] Sucesso com HTTP %d\n", statusCode);
    });

    client.onError([](int errorCode, const char* message) {
        Serial.printf("[Callback Global] Erro (%d): %s\n", errorCode, message);
    });

    client.onResponse([](int statusCode) {
        Serial.printf("[Callback Global] Requisição concluída, status: %d\n", statusCode);
    });
}

void loop() {
    // 1. Callbacks por requisição com timeout e retries personalizados
    Serial.println("\n--- [1] Requisição com Callbacks, Timeout e Retry ---");
    int id = 0;

    client.get("/todos/1")
          .timeout(3000)   // 3 segundos de timeout para esta requisição
          .retry(2)        // Até 2 tentativas adicionais em caso de falha
          .onSuccess([](int code) {
              Serial.printf("[Callback Req] Todo obtido com sucesso (HTTP %d)\n", code);
          })
          .onError([](int code, const char* message) {
              Serial.printf("[Callback Req] Falha (%d): %s\n", code, message);
          })
          .getBody("id", &id);

    // 2. Tratamento e inspeção de erros (404 Not Found)
    Serial.println("\n--- [2] Tratando Erro (404 Not Found) ---");
    client.get("/non_existent_endpoint");

    if (client.hasError()) {
        Serial.printf("Erro Detectado: HTTP %d (%s)\n",
                      client.getStatusCode(),
                      client.getErrorMessage().c_str());
    }

    // 3. Alterando a URL do cliente em tempo de execução
    Serial.println("\n--- [3] Alterando URL em Runtime ---");
    client.setUrl("https://httpbin.org");
    client.get("/status/200");

    if (client.isSuccess()) {
        Serial.printf("Redirecionado para %s com sucesso (HTTP %d)\n",
                      client.getBaseUrl(),
                      client.getStatusCode());
    }

    // Restaura a URL para o próximo ciclo do loop
    client.setUrl("https://jsonplaceholder.typicode.com");

    delay(15000);
}
```

---

## Saída Serial Esperada

```
Conectado ao WiFi

--- [1] Requisição com Callbacks, Timeout e Retry ---
[Callback Global] Sucesso com HTTP 200
[Callback Req] Todo obtido com sucesso (HTTP 200)
[Callback Global] Requisição concluída, status: 200

--- [2] Tratando Erro (404 Not Found) ---
[Callback Global] Erro (404): Not Found
[Callback Global] Requisição concluída, status: 404
Erro Detectado: HTTP 404 (Not Found)

--- [3] Alterando URL em Runtime ---
[Callback Global] Sucesso com HTTP 200
[Callback Global] Requisição concluída, status: 200
Redirecionado para https://httpbin.org com sucesso (HTTP 200)
```

---

## Pontos Importantes

- Os callbacks recebem o código de status HTTP real (`200`, `404`, `500`) ou os códigos negativos de rede padrão da biblioteca.
- O método `client.getErrorMessage()` fornece uma descrição textual clara e amigável sem códigos numéricos enigmáticos.
