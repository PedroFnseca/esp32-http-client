---
title: Tratamento de Erros e Respostas Parciais GraphQL - ESP32-HTTP-Client
description: Como tratar erros GraphQL, inspecionar mensagens, locais, caminhos e extensões preservando dados parciais.
keywords: erros GraphQL ESP32, dados parciais GraphQL, GraphQLError
tags:
  - examples
  - graphql
  - errors
---
# Tratamento de Erros e Respostas Parciais GraphQL

No GraphQL, falhas em campos específicos não cancelam obrigatoriamente toda a requisição. Servidores retornam frequentemente tanto `"data"` (com os campos resolvidos) quanto `"errors"` no mesmo documento.

O `ESP32-HTTP-Client` **preserva todas as variáveis com dados parciais resolvidos com sucesso**, disponibilizando as estruturas de erro para inspeção.

---

## Acessando Erros e Dados Parciais

```cpp
int userId = 0;
String userName;
GraphQLError error;

client.graphql("/graphql")
    .query("query { user { id name restrictedProfile } }")
    .getData("user.id", &userId)
    .getData("user.name", &userName)
    .getError(&error)
    .onGraphQLError([](const std::vector<GraphQLError>& errors) {
      for (const auto& err : errors) {
        Serial.printf("Erro: %s\n", err.message.c_str());
        for (const auto& loc : err.locations) {
          Serial.printf("  na linha %d, coluna %d\n", loc.line, loc.column);
        }
      }
    });

// Dados parciais são mantidos!
if (userId > 0) {
  Serial.printf("ID do Usuário: %d, Nome: %s\n", userId, userName.c_str());
}
```

---

## Tratamento de Códigos de Status HTTP

Na especificação GraphQL over HTTP, erros de sintaxe ou validação retornam HTTP 400 com payload contendo `"errors"`. O cliente processa o corpo independentemente do código 4xx/5xx:

```cpp
String mensagemErro;

client.graphql("/graphql")
    .query("query { sintaxeInvalida ")
    .getErrorMessage(&mensagemErro);

if (!mensagemErro.isEmpty()) {
  Serial.printf("Erro GraphQL: %s\n", mensagemErro.c_str());
}
```
