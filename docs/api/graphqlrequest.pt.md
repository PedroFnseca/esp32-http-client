---
title: Referência da Classe GraphQLRequest - Cliente GraphQL Fluente para ESP32
description: Referência da API para a classe GraphQLRequest: encadeamento de métodos para queries, mutações, variáveis, nomes de operação, preservação de dados parciais e tratamento de erros.
keywords: GraphQLRequest class ESP32, cliente GraphQL ESP32, GraphQL HTTP, variáveis GraphQL, erros GraphQL, streaming GraphQL
tags:
  - api
  - graphql
  - request
  - class
---
# GraphQLRequest

O construtor fluente de requisições retornado por `.graphql(path)`, `.graphqlGet(path)` e `.graphqlPost(path)` no [`ESP32HTTPClient`](esp32httpclient.pt.md). Todos os métodos retornam `GraphQLRequest&`, permitindo encadeamento fluente.

**A requisição GraphQL é enviada automaticamente** quando o objeto `GraphQLRequest` sai de escopo (ao final da instrução) ou quando `.execute()` é chamado explicitamente.

!!! note "Semântica de cópia"
    `GraphQLRequest` é **move-only** — não pode ser copiado. Ele foi desenhado para ser utilizado em uma única expressão encadeada.

---

## Exemplo Rápido

```cpp
client.graphql("/graphql")
    .query("query GetUser($id: ID!) { user(id: $id) { name email } }")
    .operationName("GetUser")
    .variable("id", "usr_100")
    .getData("user.name", &name)
    .getData("user.email", &email);
```

---

## Construção de Operações

### `query(doc)` / `document(doc)`
Define a string do documento GraphQL de consulta (query). Encadeável.
```cpp
GraphQLRequest& query(const char* queryDocument);
GraphQLRequest& query(const String& queryDocument);
GraphQLRequest& document(const char* document);
GraphQLRequest& document(const String& document);
```

### `mutation(doc)`
Define a string do documento GraphQL de mutação (mutation) e define o método HTTP como POST. Encadeável.
```cpp
GraphQLRequest& mutation(const char* mutationDocument);
GraphQLRequest& mutation(const String& mutationDocument);
```

### `operationName(name)`
Define o nome da operação a ser executada quando o documento possui múltiplas operações declaradas. Encadeável.
```cpp
GraphQLRequest& operationName(const char* name);
GraphQLRequest& operationName(const String& name);
```

---

## Métodos de Transporte HTTP

### `asGet()` / `get()`
Configura a requisição para executar como HTTP GET. Query, operationName e variables serão codificados na URL de acordo com a especificação GraphQL over HTTP.
```cpp
GraphQLRequest& asGet();
GraphQLRequest& get();
```

### `asPost()` / `post()`
Configura a requisição para executar como HTTP POST (padrão). O corpo é construído em formato JSON:
`{"query": "...", "operationName": "...", "variables": {...}}`.
```cpp
GraphQLRequest& asPost();
GraphQLRequest& post();
```

---

## Variáveis

Suporta tipos primitivos, `String` do Arduino, strings JSON brutas e structs em C++ mapeadas com `REST_JSON_MAP`:

```cpp
client.graphql("/graphql")
    .query("query Q($id: Int!, $active: Boolean!) { ... }")
    .variable("id", 10)
    .variable("active", true);
```

Exemplo com struct:
```cpp
struct Telemetria {
  float temp;
  REST_JSON_MAP(REST_FIELD(temp))
};

Telemetria dados{23.8f};
client.graphql("/graphql")
    .mutation("mutation Log($d: TelemetriaInput!) { record(input: $d) { ok } }")
    .variable("d", dados);
```

---

## Vinculação de Dados (`getData`)

Vincula campos JSON diretamente a variáveis locais. Os caminhos são relativos ao campo `"data"` da resposta (ex.: `"user.name"` corresponde a `"data.user.name"`).

```cpp
int id;
String name;

client.graphql("/graphql")
    .query("{ user { id name } }")
    .getData("user.id", &id)
    .getData("user.name", &name);
```

---

## Tratamento de Erros e Dados Parciais

Respostas GraphQL podem retornar simultaneamente `"data"` e `"errors"`. O `GraphQLRequest` extrai os erros na estrutura `GraphQLError` e **preserva todos os dados parciais resolvidos com sucesso**.

```cpp
GraphQLError error;
std::vector<GraphQLError> allErrors;

client.graphql("/graphql")
    .query("{ user { id name restrictedField } }")
    .getData("user.id", &id)
    .getData("user.name", &name)
    .getError(&error)
    .getErrors(&allErrors)
    .onGraphQLError([](const std::vector<GraphQLError>& errors) {
      for (const auto& err : errors) {
        Serial.printf("Erro: %s\n", err.message.c_str());
      }
    });
```
