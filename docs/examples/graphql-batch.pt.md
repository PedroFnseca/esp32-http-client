---
title: Requisições em Lote (Batching) GraphQL - ESP32-HTTP-Client
description: Como executar consultas GraphQL em lote em uma única requisição HTTP para economizar conexões e energia no ESP32.
keywords: GraphQL batch ESP32, lote GraphQL, múltiplas operações ESP32
tags:
  - examples
  - graphql
  - batch
---
# Requisições em Lote (Batching) GraphQL

Executar várias operações em uma única requisição HTTP reduz o tempo de uso do rádio Wi-Fi, elimina múltiplos handshakes TLS e economiza bateria em projetos com ESP32.

---

## Enviando Múltiplas Queries em Lote

```cpp
auto batch = client.graphqlBatch("/graphql");

String nomeDispositivo;
int totalNotificacoes = 0;

// Operação 1:
auto& op1 = batch.addQuery("query { device { name } }");
op1.getData("device.name", &nomeDispositivo);

// Operação 2:
auto& op2 = batch.addQuery("query { unreadCount }");
op2.getData("unreadCount", &totalNotificacoes);

// Disparado automaticamente ou explicitamente:
batch.execute();

if (client.isSuccess()) {
  Serial.printf("Dispositivo: %s\n", nomeDispositivo.c_str());
  Serial.printf("Notificações: %d\n", totalNotificacoes);
}
```
