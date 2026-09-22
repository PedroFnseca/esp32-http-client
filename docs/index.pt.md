---
title: Cliente HTTP ESP32 - Biblioteca Cliente HTTP Fluente e Zero Heap
description: Uma biblioteca cliente HTTP leve, de baixa alocação e alto desempenho para ESP32 (Arduino e PlatformIO). API fluente C++, vínculo direto de respostas para APIs REST, Web Services SOAP 1.1/1.2, serviços GraphQL e comunicação HTTP extensível.
keywords: Cliente HTTP ESP32, API REST Arduino ESP32, Cliente SOAP ESP32, Cliente GraphQL ESP32, GraphQL Arduino ESP32, SOAP 1.1 1.2 ESP32, Parser JSON ESP32, Parser XML ESP32, PlatformIO ESP32, biblioteca C++ ESP32
tags:
  - home
  - overview
---
# Biblioteca ESP32 HTTP Client

> Uma biblioteca cliente HTTP leve, de baixa alocação e alto desempenho para ESP32 que **vincula dados de resposta diretamente às suas variáveis** contando com mecanismos nativos em streaming e zero heap para **APIs REST**, **Web Services SOAP 1.1 / 1.2**, **Serviços GraphQL** e comunicação HTTP extensível.

[![Arduino Library](https://img.shields.io/github/v/release/PedroFnseca/esp32-http-client?color=00979D&label=Arduino&logo=arduino&logoColor=white){: width="120" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client)
[![PlatformIO Registry](https://img.shields.io/github/v/release/PedroFnseca/esp32-http-client?color=f58220&label=PlatformIO&logo=platformio&logoColor=white){: width="130" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client)
[![Idioma](https://img.shields.io/github/languages/top/PedroFnseca/esp32-http-client){: width="80" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client)
[![Cobertura](https://img.shields.io/badge/Coverage-93.73%25-brightgreen){: width="116" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client)
[![Licença](https://img.shields.io/github/license/PedroFnseca/esp32-http-client){: width="80" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client/blob/main/LICENSE)
[![Estrelas](https://img.shields.io/github/stars/PedroFnseca/esp32-http-client?style=social){: width="80" height="20" loading="lazy" decoding="async" }](https://github.com/PedroFnseca/esp32-http-client/stargazers)
[![Downloads](https://img.shields.io/endpoint?url=https://esp32-http-stats.esp32httpclient.com/downloads)](https://github.com/PedroFnseca/esp32-http-client)

---

## O que é?

**ESP32-HTTP-Client** é um cliente HTTP moderno e modular para o ESP32 projetado para conectar serviços web e a memória do microcontrolador com máxima eficiência. Em vez de tratar a comunicação HTTP como manipulação manual de strings e processamento pesado de árvores DOM, a biblioteca extrai campos de resposta em tempo real diretamente do fluxo da rede para variáveis C++.

Construído sobre um núcleo de transporte compartilhado e eficiente (TLS, reuso de conexão, autenticação, timeouts e retries), o cliente disponibiliza builders dedicados e fluentes para os principais padrões de comunicação web:

=== "REST (JSON)"

    Consuma endpoints RESTful com métodos intuitivos (`get`, `post`, `put`, `patch`, `del`), parâmetros de rota/busca e extração direta de JSON ou mapeamento bidirecional de structs:

    ```cpp
    int userId;
    float temperature;
    char city[32];

    client.get("/report")
          .query("format", "compact")
          .getBody("userId", &userId)
          .getBody("sensor.temp", &temperature)
          .getBody("0.address.city", city, sizeof(city));
    ```

=== "GraphQL (Queries e Mutações)"

    Execute operações GraphQL com variáveis tipadas, seleção de operações, requisições em lote (`GraphQLBatchRequest`), preservação de dados parciais e streaming com `@defer`:

    ```cpp
    String nome;
    int id = 0;

    client.graphql("/graphql")
          .query("query GetUser($id: ID!) { user(id: $id) { id name } }")
          .variable("id", 101)
          .getData("user.id", &id)
          .getData("user.name", &nome);
    ```

=== "SOAP (XML 1.1 / 1.2)"

    Conecte-se a serviços corporativos SOAP com geração automática de envelopes, gerenciamento de `SOAPAction` / `Content-Type`, streaming de tags XML e inspeção nativa de SOAP Faults:

    ```cpp
    float preco = 0.0f;
    SoapFault falha;

    client.soap("/ws")
          .soapAction("http://example.org/GetPrice")
          .body("<m:GetPrice xmlns:m=\"http://example.org\"><m:Item>ESP32</m:Item></m:GetPrice>")
          .getFault(&falha)
          .getBody("Price", &preco);
    ```

=== "Núcleo Extensível"

    Uma única instância de cliente gerencia configurações persistentes para todas as requisições — incluindo segurança TLS, cabeçalhos customizados, autenticação (Bearer, Basic, API Key, Cookies), retentativas automáticas e métricas de observabilidade:

    ```cpp
    ESP32HTTPClient client("https://api.example.com");
    client.bearer("token_xyz");
    client.setTimeout(5000);
    client.setMaxRetry(2);

    // Reutilize o mesmo cliente para endpoints REST, SOAP ou GraphQL
    client.get("/api/v1/health");
    client.soap("/ws/service");
    client.graphql("/graphql");
    ```

Um cliente unificado. Vinculação direta de memória. Mínimo consumo de RAM.

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

=== "REST API (JSON)"

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

=== "SOAP Web Service (XML)"

    ```cpp
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    ESP32HTTPClient client("https://www.dataaccess.com");

    void setup() {
        Serial.begin(115200);
        WiFi.begin("SEU_SSID", "SUA_SENHA");
        while (WiFi.status() != WL_CONNECTED) delay(100);

        char result[64] = {0};

        // Envia requisição SOAP 1.1 e extrai diretamente a tag <m:NumberToWordsResult>
        client.soap("/webservicesserver/NumberConversion.wso")
              .soapAction("http://www.dataaccess.com/webservicesserver/NumberToWords")
              .body("<NumberToWords xmlns=\"http://www.dataaccess.com/webservicesserver/\">"
                    "<ubiNum>500</ubiNum>"
                    "</NumberToWords>")
              .getBody("NumberToWordsResult", result, sizeof(result));

        Serial.printf("Resultado: %s\n", result);
    }

    void loop() {}
    ```

=== "API GraphQL"

    ```cpp
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    ESP32HTTPClient client("https://countries.trevorblades.com");

    void setup() {
        Serial.begin(115200);
        WiFi.begin("SEU_SSID", "SUA_SENHA");
        while (WiFi.status() != WL_CONNECTED) delay(100);

        char countryName[64] = {0};

        // Consulta dados de país com variáveis e vincula diretamente à variável
        client.graphql("/graphql")
              .query("query GetCountry($code: ID!) { country(code: $code) { name } }")
              .variable("code", "BR")
              .getData("country.name", countryName, sizeof(countryName));

        Serial.printf("País: %s\n", countryName);
    }

    void loop() {}
    ```

→ [Ver todos os exemplos](examples/index.pt.md)

---

<p align="center">
  Se esta biblioteca economizou seu tempo, considere deixar uma ⭐ no <a href="https://github.com/PedroFnseca/esp32-http-client">GitHub</a>.
</p>
