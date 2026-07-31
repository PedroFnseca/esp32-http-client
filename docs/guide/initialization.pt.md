---
tags:
  - guide
  - init
  - setup
---
# Inicialização

## Criando o Cliente

`ESP32HTTPClient` é o ponto de entrada para todas as requisições. Crie uma instância por servidor e reutilize-a — a biblioteca mantém a conexão ativa entre as chamadas automaticamente.

```cpp
#include "ESP32HTTPClient.h"

ESP32HTTPClient client("https://api.example.com");
```

### Assinaturas do Construtor

```cpp
// Porta padrão (80 para HTTP, 443 para HTTPS)
ESP32HTTPClient(const char* baseUrl);

// Porta personalizada
ESP32HTTPClient(const char* baseUrl, int port);
```

---

## Exemplos

### HTTPS Padrão (porta 443)

```cpp
ESP32HTTPClient client("https://api.example.com");
```

### HTTP Padrão (porta 80)

```cpp
ESP32HTTPClient client("http://192.168.1.100");
```

### Porta Personalizada

Útil para servidores locais, ambientes de desenvolvimento ou gateways IoT:

```cpp
ESP32HTTPClient client("http://meu-servidor-local.local", 8080);
```

### Porta Explícita em HTTPS

```cpp
ESP32HTTPClient client("https://timeapi.io", 443);
```

---

## Escopo e Declaração

Declare o cliente no **escopo global** para que ele persista durante as iterações do `loop()` e a conexão Keep-Alive seja preservada:

```cpp
// ✅ Correto — o cliente vive durante toda a execução do programa
ESP32HTTPClient client("https://api.example.com");

void setup() { ... }
void loop() {
    // cliente é reutilizado a cada iteração
    client.get("/data").getBody("value", &minhaVar);
}
```

```cpp
// ❌ Evitar — um novo cliente (e nova conexão TLS) é criado a cada iteração do loop
void loop() {
    ESP32HTTPClient client("https://api.example.com"); // novo handshake toda vez!
    client.get("/data").getBody("value", &minhaVar);
}
```

---

## Próximo Passo

→ [Fazendo Requisições](requests.pt.md)
