---
title: Cliente HTTP ESP32 - Biblioteca REST Zero Heap
description: Biblioteca cliente HTTP de alto desempenho e zero alocação de memória na heap para ESP32 (Arduino e PlatformIO). API fluente C++ e vínculo direto de JSON.
keywords: Cliente HTTP ESP32, API REST Arduino ESP32, Parser JSON ESP32, Requisição GET POST ESP32, PlatformIO ESP32, biblioteca C++ ESP32
tags:
  - home
  - overview
---
# Biblioteca ESP32 HTTP Client

> Um cliente HTTP fluente e orientado a objetos para ESP32 que **vincula os campos da resposta JSON diretamente às suas variáveis** — sem `ArduinoJson`, sem strings intermediárias, sem código clichê.

[![Arduino Library](https://img.shields.io/github/v/release/PedroFnseca/esp32-http-client?color=00979D&label=Arduino&logo=arduino&logoColor=white)](https://github.com/PedroFnseca/esp32-http-client)
[![PlatformIO Registry](https://img.shields.io/github/v/release/PedroFnseca/esp32-http-client?color=f58220&label=PlatformIO&logo=platformio&logoColor=white)](https://github.com/PedroFnseca/esp32-http-client)
[![Idioma](https://img.shields.io/github/languages/top/PedroFnseca/esp32-http-client)](https://github.com/PedroFnseca/esp32-http-client)
[![Cobertura](https://img.shields.io/badge/Coverage-88.18%25-brightgreen)](https://github.com/PedroFnseca/esp32-http-client)
[![Licença](https://img.shields.io/github/license/PedroFnseca/esp32-http-client)](https://github.com/PedroFnseca/esp32-http-client/blob/main/LICENSE)
[![Estrelas](https://img.shields.io/github/stars/PedroFnseca/esp32-http-client?style=social)](https://github.com/PedroFnseca/esp32-http-client/stargazers)

---

## O que é?

**ESP32-HTTP-Client** é uma biblioteca leve do Arduino para o ESP32 que repensa como você interage com APIs REST. Em vez de obter uma string JSON bruta e depois analisá-la, você simplesmente diz ao cliente *onde* colocar os dados, e ele cuida do resto.

```cpp
int userId;
float temperature;
char city[32];

client.get("/report")
      .getBody("userId", &userId)
      .getBody("sensor.temp", &temperature)
      .getBody("0.address.city", city, sizeof(city));
```

Uma única cadeia fluente. Vinculação direta de memória. Zero alocações na heap para a resposta.

### Como Funciona

```mermaid
sequenceDiagram
    box rgba(0, 150, 136, 0.15) Dispositivo ESP32
    participant App as Sua App
    participant Client as ESP32HTTPClient
    end
    box rgba(255, 152, 0, 0.15) API Externa
    participant Server as API
    end

    App->>Client: 1. Configurar Requisição & Vincular Variáveis
    Note over App,Client: ex: getBody("temp", &temperature)
    Client->>Server: 2. Enviar Requisição HTTP
    Server-->>Client: 3. Streaming da Resposta JSON
    
    rect rgba(0, 150, 136, 0.1)
    loop Parsing com Alocação Zero
        Client->>Client: Ler Stream Byte a Byte
        Client->>Client: Identificar Chaves em Tempo Real
        rect rgba(255, 152, 0, 0.15)
        alt Chave Corresponde ao Alvo
            Client->>App: 4. Injetar Valor Diretamente na Variável
        end
        end
    end
    end
    Client-->>App: 5. Requisição Concluída
```

---

## Desempenho em Destaque

Comparativo medido em **100 requisições HTTP GET consecutivas** com cargas JSON em um dispositivo ESP32 real:

| Métrica | Padrão (HTTPClient + ArduinoJson) | ESP32-HTTP-Client |
| :--- | :---: | :---: |
| **Alocação de heap por requisição** | ~58.2 KB | **~15 bytes** |
| **Pegada média de RAM** | 34.2% | **24.3%** |
| **Heap livre mínima** | 114.3 KB | **128.6 KB** |
| **Tempo médio de execução** | ~750 ms | **~59 ms** |

→ [Veja a análise de desempenho completa](performance.pt.md)

---

## Instalação Rápida

=== "Gerenciador de Bibliotecas do Arduino"

    Procure por **ESP32-HTTP-Client** no Gerenciador de Bibliotecas do Arduino IDE e clique em **Instalar**.

=== "PlatformIO"

    Adicione `ESP32-HTTP-Client` ao seu `platformio.ini`:
    ```ini
    lib_deps =
        PedroFnseca/ESP32-HTTP-Client@^1.4.0
    ```

=== "Manual"

    Baixe a [última versão](https://github.com/PedroFnseca/esp32-http-client/releases) e coloque a pasta dentro do seu diretório `Arduino/libraries/`.

→ [Guia de instalação completo](getting-started/installation.pt.md)

---

## Início Rápido em 30 Segundos

```cpp
#include <WiFi.h>
#include "ESP32HTTPClient.h"

ESP32HTTPClient client("https://jsonplaceholder.typicode.com");

void setup() {
    Serial.begin(115200);
    WiFi.begin("SEU_SSID", "SUA_SENHA");
    while (WiFi.status() != WL_CONNECTED) delay(100);

    int userId = 0;

    // A API retorna: { "userId": 1, "id": 1, "title": "...", "completed": false }
    client.get("/todos/1").getBody("userId", &userId);

    Serial.printf("ID do Usuário: %d\n", userId);
}

void loop() {}
```

→ [Ver todos os exemplos](examples/index.pt.md)

---

<p align="center">
  Se esta biblioteca economizou seu tempo, considere deixar uma ⭐ no <a href="https://github.com/PedroFnseca/esp32-http-client">GitHub</a>.
</p>
