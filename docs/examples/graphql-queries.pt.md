---
title: Consultas e Mutações GraphQL - ESP32-HTTP-Client
description: Como executar consultas (queries) e mutações GraphQL via HTTP e HTTPS no ESP32 com baixo uso de memória.
keywords: GraphQL ESP32 query, GraphQL ESP32 mutação, GraphQL GET POST
tags:
  - examples
  - graphql
  - queries
  - mutations
---
# Consultas e Mutações GraphQL

O `ESP32-HTTP-Client` disponibiliza uma API nativa e fluente para operações GraphQL, realizando streaming direto para variáveis sem alocação do payload na memória heap.

---

## Consulta Básica (POST)

```cpp
#include <WiFi.h>
#include "ESP32HTTPClient.h"

ESP32HTTPClient client("https://api.example.com");

String nome;
int id;

void setup() {
  Serial.begin(115200);

  client.graphql("/graphql")
      .query("query { user { id name } }")
      .getData("user.id", &id)
      .getData("user.name", &nome);

  if (client.isSuccess()) {
    Serial.printf("Usuário: %s (ID: %d)\n", nome.c_str(), id);
  }
}
```

---

## Consulta via HTTP GET

Consultas somente de leitura podem ser realizadas via HTTP GET:

```cpp
String status;

client.graphqlGet("/graphql")
    .query("{ systemHealth }")
    .getData("systemHealth", &status);
```

---

## Mutações GraphQL (POST)

Mutações alteram dados no servidor e são sempre executadas via HTTP POST:

```cpp
int deviceId = 0;
String status;

client.graphql("/graphql")
    .mutation("mutation Register { registerNode(type: \"ESP32\") { id status } }")
    .getData("registerNode.id", &deviceId)
    .getData("registerNode.status", &status);
```
