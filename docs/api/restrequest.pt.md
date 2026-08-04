---
tags:
  - api
  - request
  - class
---
# RestRequest

O construtor fluente de requisições retornado por todos os métodos HTTP no [`ESP32HTTPClient`](esp32httpclient.pt.md). Todos os métodos do construtor retornam `RestRequest&`, permitindo o encadeamento fluente.

**A requisição HTTP é enviada automaticamente** quando o objeto `RestRequest` sai de escopo (ao final da instrução).

!!! note "Semântica de cópia"
    `RestRequest` é um objeto **exclusivo de movimentação (move-only)** — ele não pode ser copiado. Ele foi projetado para ser usado em uma única expressão encadeada.

---

## Construindo a Requisição

### `path(key, value)`

Substitui um espaço reservado (placeholder, ex: `{id}` ou `id`) no caminho da URL. Encadeável.

```cpp
template <typename T>
RestRequest& path(const char* key, T value);
```

**Tipos suportados para `value`:** `int`, `long`, `float`, `double`, `bool`, `const char*`, `char*`

**Exemplo:**
```cpp
// Produz: GET /users/15
client.get("/users/{id}")
      .path("id", 15);
```

---

### `query(key, value)`

Anexa um parâmetro de consulta na URL. Encadeável.

```cpp
template <typename T>
RestRequest& query(const char* key, T value);
```

**Tipos suportados para `value`:** `int`, `long`, `float`, `double`, `bool`, `const char*`, `char*`

**Exemplo:**
```cpp
// Produz: GET /users?page=2&limit=20&search=pedro
client.get("/users")
      .query("page",   2)
      .query("limit",  20)
      .query("search", "pedro");
```

---

### `body(key, value)`

Adiciona um campo ao corpo JSON da requisição. Define automaticamente `Content-Type: application/json`. Encadeável.

```cpp
template <typename T>
RestRequest& body(const char* key, T value);
```

**Tipos suportados para `value`:** `int`, `long`, `float`, `double`, `bool`, `const char*`, `char*`

**Serialização de tipos:**

| Tipo C++ | Exemplo de Saída JSON |
| :--- | :--- |
| `const char*` / `char*` | `"valor string"` |
| `int` | `42` |
| `long` | `1721156604` |
| `float` | `24.5` (até 5 dígitos significativos) |
| `double` | `3.14159265` (até 9 dígitos significativos) |
| `bool` | `true` ou `false` |

**Exemplo:**
```cpp
// Corpo: {"name":"Pedro","age":21,"active":true,"score":9.87}
client.post("/users")
      .body("name",   "Pedro")
      .body("age",    21)
      .body("active", true)
      .body("score",  9.87f);
```

---

## Extraindo a Resposta

`getBody()` é sobrecarregado para cada tipo C suportado. Ele registra uma vinculação entre um **caminho de chave JSON** e uma **variável alvo**. Todas as sobrecargas são encadeáveis.

A vinculação usa o caminho para localizar o valor no fluxo JSON analisado. Se a chave não for encontrada, o valor alvo **permanece inalterado**.

---

### `getBody(key, int* target)`

Vincula um número JSON a um `int` C.

```cpp
RestRequest& getBody(const char* key, int* target);
```

**Exemplo:**
```cpp
int count;
client.get("/stats").getBody("count", &count);
```

---

### `getBody(key, float* target)`

Vincula um número JSON a um `float` C.

```cpp
RestRequest& getBody(const char* key, float* target);
```

**Exemplo:**
```cpp
float temp;
client.get("/sensor").getBody("temperature", &temp);
```

---

### `getBody(key, double* target)`

Vincula um número JSON a um `double` C.

```cpp
RestRequest& getBody(const char* key, double* target);
```

**Exemplo:**
```cpp
double voltage;
client.get("/meter").getBody("voltage", &voltage);
```

---

### `getBody(key, long* target)`

Vincula um inteiro JSON a um `long` C. Use para timestamps Unix e outros inteiros grandes.

```cpp
RestRequest& getBody(const char* key, long* target);
```

**Exemplo:**
```cpp
long unixTs;
client.get("/time").getBody("unix_timestamp", &unixTs);
```

---

### `getBody(key, bool* target)`

Vincula um booleano JSON (`true` / `false`) a um `bool` C.

```cpp
RestRequest& getBody(const char* key, bool* target);
```

**Exemplo:**
```cpp
bool active;
client.get("/status").getBody("active", &active);
```

---

### `getBody(key, char* target, size_t maxLength)`

Copia uma string JSON para um buffer `char` C. Grava no máximo `maxLength - 1` bytes e sempre inclui o caractere nulo final.

```cpp
RestRequest& getBody(const char* key, char* target, size_t maxLength);
```

**Exemplo:**
```cpp
char city[64];
client.get("/user/1").getBody("address.city", city, sizeof(city));
```

---

### `getBody(key, String* target)`

Copia uma string, objeto ou array JSON para uma `String` do Arduino.

- Para strings JSON primitivas, o valor da string é copiado.
- Para objetos ou arrays JSON, o JSON bruto é capturado caractere por caractere.
- Passe `""` (string vazia) como `key` para capturar todo o valor no nível raiz.

```cpp
RestRequest& getBody(const char* key, String* target);
```

**Exemplo:**
```cpp
String raw;
client.get("/users").getBody("",  &raw);      // toda a resposta
client.get("/users").getBody("0", &firstUser); // primeiro elemento do array
```

!!! warning
    Use com cuidado para respostas grandes. Cada caractere é alocado individualmente na heap, o que pode causar fragmentação de memória.

---

## Referência da Sintaxe de Caminho

| String de Caminho | O que ela acessa |
| :--- | :--- |
| `"name"` | Campo `name` no nível raiz |
| `"address.city"` | Campo aninhado: `address` → `city` |
| `"address.geo.lat"` | Profundamente aninhado: `address` → `geo` → `lat` |
| `"0.name"` | Campo `name` do primeiro elemento do array |
| `"1.address.city"` | Campo `city` aninhado do segundo elemento do array |
| `""` | O objeto ou array raiz completo (use com `String*`) |
