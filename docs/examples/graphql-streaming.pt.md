---
title: Entrega Incremental e Streaming (@defer) - ESP32-HTTP-Client
description: Como consumir respostas GraphQL com @defer e multipart/mixed diretamente no ESP32 sem bufferizar o payload completo.
keywords: GraphQL defer ESP32, multipart mixed streaming ESP32, incremental GraphQL
tags:
  - examples
  - graphql
  - streaming
---
# Entrega Incremental e Streaming (`@defer`)

Servidores GraphQL modernos podem transmitir campos caros ou lentos de forma incremental utilizando a diretiva `@defer` através de conexões HTTP `multipart/mixed`.

O `ESP32-HTTP-Client` processa cada pedaço (chunk) diretamente do fluxo da rede utilizando o `BufferedStreamReader`, mantendo o consumo de heap extremamente baixo.

---

## Exemplo com `@defer`

```cpp
const char* queryComDefer =
    "query GetDashboard {\n"
    "  status\n"
    "  ... @defer {\n"
    "    heavyLogData { timestamp message }\n"
    "  }\n"
    "}";

String statusInicial;

client.graphql("/graphql")
    .query(queryComDefer)
    .getData("status", &statusInicial)
    .onIncremental([](const GraphQLIncrementalPayload& payload) {
      Serial.printf("Chunk recebido para caminho '%s' (hasNext: %d)\n",
                    payload.path.c_str(),
                    payload.hasNext);
    });
```
