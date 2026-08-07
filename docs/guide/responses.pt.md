---
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
| `getBody(chave, float* alvo)` | número | Até 5 dígitos significativos |
| `getBody(chave, double* alvo)` | número | Até 9 dígitos significativos |
| `getBody(chave, long* alvo)` | número | Use para timestamps Unix |
| `getBody(chave, bool* alvo)` | booleano | Corresponde a `true` / `false` |
| `getBody(chave, char* alvo, size_t maxLen)` | string | Copia até `maxLen-1` bytes |
| `getBody(chave, String* alvo)` | string / objeto / array | `String` do Arduino, veja abaixo |

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
