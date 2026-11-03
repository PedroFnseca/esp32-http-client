---
title: Lendo Respostas HTTP e Parsing JSON Zero Heap no ESP32
description: Aprenda como vincular campos de resposta JSON diretamente a tipos primitivos, structs e arrays em C++ sem alocação dinâmica no ESP32.
keywords: vinculo JSON ESP32, parser JSON zero heap ESP32, ler resposta HTTP Arduino ESP32, getBody ESP32HTTPClient
tags:
  - guide
  - response
  - json
---
# Lendo Respostas

O núcleo do ESP32-HTTP-Client é o seu sistema de **vinculação de respostas** (response binding). Em vez de você mesmo analisar uma string JSON, você registra vínculos que dizem à biblioteca exatamente quais variáveis devem ser preenchidas.

---

## Vinculação Básica com `getBody()`

`getBody()` é sobrecarregado para cada tipo C suportado. Ele registra um vínculo entre um **caminho de chave JSON** e um **ponteiro de variável alvo**.

```cpp
int count;
float temperature;
bool active;
char name[64];
long timestamp;

client.get("/sensor")
      .getBody("count",       &count)
      .getBody("temperature", &temperature)
      .getBody("active",      &active)
      .getBody("name",        name, sizeof(name))
      .getBody("timestamp",   &timestamp);
```

---

## Tipos Suportados

| Assinatura do método | Tipo JSON correspondente | Notas |
| :--- | :--- | :--- |
| `getBody(chave, int* alvo)` | número | Truncamento inteiro aplicado |
| `getBody(chave, float* alvo)` | número | Até 5 dígitos significativos. Suporta notação científica (ex.: `6.02e+2`, `-2.5E-3`) |
| `getBody(chave, double* alvo)` | número | Até 9 dígitos significativos. Suporta notação científica (ex.: `6.02e+2`, `-2.5E-3`) |
| `getBody(chave, long* alvo)` | número | Use para timestamps Unix |
| `getBody(chave, bool* alvo)` | booleano | Corresponde a `true` / `false` |
| `getBody(chave, char* alvo, size_t maxLen)` | string | Copia até `maxLen-1` bytes |
| `getBody(chave, String* alvo)` | string / objeto / array | `String` do Arduino, veja abaixo |

### Suporte a Notação Científica

Números formatados em notação científica / exponencial são convertidos diretamente para destinos `float` e `double`:

```cpp
float  expoentePositivo;
double expoenteNegativo;

// Resposta: { "positive_exp": 6.02e+2, "negative_exp": -2.5E-3 }
client.get("/sensor")
      .getBody("positive_exp", &expoentePositivo) // 602.0
      .getBody("negative_exp", &expoenteNegativo); // -0.0025
```

Tanto `e` minúsculo quanto `E` maiúsculo, com expoentes positivos (`+`) ou negativos (`-`) opcionais, são suportados.

---

## Notação de Ponto para Campos Aninhados

Use `.` como separador de caminho para alcançar objetos JSON aninhados:

```cpp
// Resposta: { "address": { "city": "Gwenborough", "geo": { "lat": "-37.3159" } } }

char city[64];
char lat[32];

client.get("/users/1")
      .getBody("address.city",    city, sizeof(city))
      .getBody("address.geo.lat", lat,  sizeof(lat));
```

Não há limite prático para a profundidade de aninhamento.

---

## Índices Numéricos para Arrays

Use um número inteiro como segmento do caminho para endereçar um elemento específico de um array JSON:

```cpp
// Resposta: [{ "name": "Alice" }, { "name": "Bob" }]

char first[32], second[32];

client.get("/users")
      .getBody("0.name", first,  sizeof(first))   // primeiro elemento
      .getBody("1.name", second, sizeof(second));  // segundo elemento
```

Você pode combinar índices de array livremente com a notação de ponto:

```cpp
// Resposta: [{ "address": { "city": "Gwenborough" } }, ...]
char city[64];
client.get("/users")
      .getBody("1.address.city", city, sizeof(city));
```

---

## Capturando JSON Bruto com `String`

Vincule um `String*` para capturar um objeto ou array JSON inteiro como uma string bruta para processamento manual:

```cpp
String entireResponse;
String secondUser;

client.get("/users")
      .getBody("",  &entireResponse) // captura o array raiz
      .getBody("1", &secondUser);    // captura o segundo objeto de usuário
```

!!! warning "Aviso de Memória"
    Puxar objetos ou arrays completos para uma `String` do Arduino causa alocação dinâmica na heap. Evite esse padrão com cargas úteis grandes, pois isso pode fragmentar ou esgotar a memória heap do dispositivo. Use vinculações tipadas sempre que possível.

---

## Lendo Cabeçalhos de Resposta com `getHeader()`

Além dos campos do corpo JSON, você pode extrair cabeçalhos HTTP da resposta diretamente usando `getHeader()`.

```cpp
String token;
char contentType[64];
int contentLength;

client.get("/auth")
      .getHeader("token",          &token)
      .getHeader("Content-Type",   contentType)
      .getHeader("Content-Length", &contentLength)
      .getBody("id", &id);
```

As buscas por cabeçalhos **não diferenciam maiúsculas de minúsculas**, portanto `"token"`, `"TOKEN"` e `"Token"` funcionam de forma idêntica.

---

## Lendo Respostas JSON-RPC 2.0 com `getResult()`

Ao trabalhar com requisições JSON-RPC 2.0, os dados do payload são extraídos usando `.getResult()` em vez de `.getBody()`. Suporta primitivos na raiz, campos aninhados, structs mapeadas e extração bruta de JSON diretamente:

```cpp
// 1. Vinculação primitiva na raiz "result"
int soma = 0;
client.jsonRpc("/rpc").method("add").param(10).param(20).getResult(&soma);

// 2. Campo aninhado dentro do objeto "result"
char cidade[32] = {0};
client.jsonRpc("/rpc").method("getUser").param("id", 1).getResult("address.city", cidade, sizeof(cidade));

// 3. Vinculação com struct mapeada
User perfil;
client.jsonRpc("/rpc").method("getUser").param("id", 1).getResult(&perfil);

// 4. String bruta do resultado JSON
String rawResult;
client.jsonRpc("/rpc").method("getMetrics").getRawResult(&rawResult);
```

### Inspecionando Erros JSON-RPC

Se o servidor responder com um objeto de erro JSON-RPC 2.0 (`{"error": {"code": -32601, "message": "Method not found"}}`), os códigos de status HTTP ainda podem ser `200 OK`. Você pode detectar e inspecionar erros JSON-RPC com `.getError()` ou `.onJsonRpcError()`:

```cpp
JsonRpcError erro;
client.jsonRpc("/rpc")
      .method("metodoInexistente")
      .getError(&erro)
      .onJsonRpcError([](const JsonRpcError& err) {
          Serial.printf("Código de erro: %d, Mensagem: %s\n", err.code, err.message.c_str());
      });

if (erro.code != 0) {
    Serial.printf("Erro JSON-RPC detectado: %s\n", erro.message.c_str());
}
```

---

## Garantias de Segurança

!!! note "Chaves e cabeçalhos ausentes são seguros"
    Se o caminho de uma chave ou um cabeçalho HTTP não existir na resposta, estiver com erro de digitação ou o tipo não corresponder, a variável alvo é **mantida inalterada**. Nenhuma exceção é lançada, nenhum travamento ocorre e a biblioteca continua analisando o restante da resposta.

Isso torna seguro adicionar campos ou cabeçalhos opcionais aos seus vínculos sem checagens defensivas:

```cpp
int optionalField = -1; // valor padrão
String optionalToken = "";

client.get("/data")
      .getHeader("token",         &optionalToken)
      .getBody("requiredField",   &myVar)
      .getBody("optionalField",   &optionalField); // inalterado se ausente
```

---

## Próximo Passo

→ [Uso Avançado](advanced.pt.md)
