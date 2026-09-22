---
title: Referência da Classe GraphQLBatchRequest - Operações em Lote GraphQL para ESP32
description: Referência da API para GraphQLBatchRequest: execute múltiplas operações GraphQL em lote em uma única requisição HTTP, processe respostas em array e inspecione erros.
keywords: GraphQLBatchRequest ESP32, lote GraphQL, batch query ESP32, requisições em lote
tags:
  - api
  - graphql
  - batch
  - class
---
# GraphQLBatchRequest

O construtor fluente de requisições retornado por `.graphqlBatch(path)` no [`ESP32HTTPClient`](esp32httpclient.pt.md). Permite agrupar múltiplas operações GraphQL independentes em uma única requisição HTTP POST.

**A requisição em lote é enviada automaticamente** quando o objeto `GraphQLBatchRequest` sai de escopo ou quando `.execute()` é chamado explicitamente.

---

## Exemplo de Uso

```cpp
auto batch = client.graphqlBatch("/graphql");

String userName;
int unreadCount = 0;

auto& op1 = batch.addQuery("query { user { name } }");
op1.getData("user.name", &userName);

auto& op2 = batch.addQuery("query { unreadNotifications }");
op2.getData("unreadNotifications", &unreadCount);

// Executado automaticamente ao final do escopo ou explicitamente:
batch.execute();
```

---

## Métodos Principais

### `add()`
Adiciona uma nova operação `GraphQLRequest` vazia ao lote e retorna uma referência para configuração.
```cpp
GraphQLRequest& add();
```

### `addQuery(queryDocument)`
Adiciona uma consulta (query) ao lote e retorna a referência da operação.
```cpp
GraphQLRequest& addQuery(const char* queryDocument);
```

### `addMutation(mutationDocument)`
Adiciona uma mutação ao lote.
```cpp
GraphQLRequest& addMutation(const char* mutationDocument);
```

### `execute()`
Envia a requisição em lote imediatamente, transmitindo o array de operações via HTTP POST.
```cpp
void execute();
```
