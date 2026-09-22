---
title: Variáveis e Seleção de Operações GraphQL - ESP32-HTTP-Client
description: Uso de variáveis primitivas e structs no GraphQL, além da seleção de operações em documentos com múltiplas declarações.
keywords: variáveis GraphQL ESP32, operationName GraphQL, struct variables ESP32
tags:
  - examples
  - graphql
  - variables
---
# Variáveis GraphQL e Seleção de Operações

---

## Passando Variáveis Escalares

As variáveis evitam concatenações manuais de strings e facilitam a reutilização da query:

```cpp
String nomeUsuario;

client.graphql("/graphql")
    .query("query GetUser($id: ID!) { user(id: $id) { name } }")
    .variable("id", "usr_9912")
    .getData("user.name", &nomeUsuario);
```

---

## Variáveis a partir de Structs com `REST_JSON_MAP`

Structs em C++ podem ser transmitidas diretamente como variáveis:

```cpp
struct TelemetriaSensor {
  float temp;
  int bateria;

  REST_JSON_MAP(
    REST_FIELD(temp),
    REST_FIELD(bateria)
  )
};

TelemetriaSensor dados{24.2f, 98};

client.graphql("/graphql")
    .mutation("mutation Log($input: TelemetryInput!) { record(data: $input) { id } }")
    .variable("input", dados);
```

---

## Seleção de Operação com `operationName`

Ao definir múltiplas operações em um mesmo documento GraphQL, selecione qual executar:

```cpp
const char* doc =
    "query GetProfile { profile { name } }\n"
    "query GetConfig { config { refreshRate } }";

int taxa = 0;

client.graphql("/graphql")
    .document(doc)
    .operationName("GetConfig")
    .getData("config.refreshRate", &taxa);
```
