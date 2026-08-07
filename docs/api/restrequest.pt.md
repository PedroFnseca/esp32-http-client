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

### `body(struct)`

Serializa uma struct C++ mapeada com `REST_JSON_MAP` e a define como o corpo JSON da requisição. Encadeável.

```cpp
template <typename T>
RestRequest& body(const T& obj);
```

**Exemplo:**
```cpp
User user = {15, "Pedro", 9.8f, true};
client.post("/users").body(user);
```

---

### `timeout(timeoutMs)`

Define um tempo limite (timeout) de rede em milissegundos específico para esta requisição, sobrescrevendo o padrão do cliente. Encadeável.

```cpp
RestRequest& timeout(uint16_t timeoutMs);
```

**Exemplo:**
```cpp
client.get("/quick-data").timeout(1500);
```

---

### `retry(maxRetry)` / `maxRetry(maxRetry)`

Define a quantidade máxima de tentativas automáticas em caso de falha de rede para esta requisição específica. Encadeável.

```cpp
RestRequest& retry(int maxRetry);
RestRequest& maxRetry(int maxRetry);
```

**Exemplo:**
```cpp
client.get("/critical-data").retry(3);
```

---

### `onSuccess(callback)`

Registra um callback específico da requisição executado se o código de status HTTP for 2xx (`200 <= code < 300`). Encadeável.

```cpp
RestRequest& onSuccess(HttpResponseCallback cb);
```

**Exemplo:**
```cpp
client.get("/users")
      .onSuccess([](int code) {
          Serial.printf("Requisição bem-sucedida com status %d\n", code);
      })
      .getBody("id", &id);
```

---

### `onError(callback)`

Registra um callback específico da requisição executado se a requisição falhar (`code < 200 || code >= 400`). Recebe o código de status e a descrição do erro. Encadeável.

```cpp
RestRequest& onError(HttpErrorCallback cb);
RestRequest& onError(HttpResponseCallback cb);
```

**Exemplo:**
```cpp
client.get("/users")
      .onError([](int code, const char* message) {
          Serial.printf("Requisição falhou (%d): %s\n", code, message);
      })
      .getBody("id", &id);
```

---

### `onResponse(callback)`

Registra um callback específico executado quando a requisição for concluída, independentemente de sucesso ou falha. Encadeável.

```cpp
RestRequest& onResponse(HttpResponseCallback cb);
```

**Exemplo:**
```cpp
client.get("/users")
      .onResponse([](int code) {
          Serial.printf("Finalizada com código %d\n", code);
      })
      .getBody("id", &id);
```

---

## Extraindo a Resposta

`getBody()` registra uma vinculação entre um **caminho de chave JSON** e uma **variável alvo**. Se a chave não for encontrada, o valor alvo **permanece inalterado**. Todas as sobrecargas são encadeáveis.

---

### `getBody(key, target)`

Vincula um campo da resposta JSON a uma variável local através de sobrecargas de tipo:

```cpp
RestRequest& getBody(const char* key, int* target);
RestRequest& getBody(const char* key, long* target);
RestRequest& getBody(const char* key, float* target);
RestRequest& getBody(const char* key, double* target);
RestRequest& getBody(const char* key, bool* target);
RestRequest& getBody(const char* key, char* target, size_t maxLength);
template <size_t N> RestRequest& getBody(const char* key, char (&target)[N]);
RestRequest& getBody(const char* key, String* target);
```

**Tipos suportados para `target`:**

| Tipo da Variável | Tipo JSON Esperado | Comportamento |
| :--- | :--- | :--- |
| `int*` | Número inteiro | Trunca decimais se houver |
| `long*` | Número inteiro | Útil para timestamps Unix e IDs grandes |
| `float*` | Número de ponto flutuante | Até 5 dígitos significativos |
| `double*` | Número de ponto flutuante | Até 9 dígitos significativos |
| `bool*` | Booleano | `true` ou `false` |
| `char*`, `size_t` / `char[N]` | String | Copia até `N-1` caracteres com terminador nulo |
| `String*` | String / Objeto / Array | Copia o valor como texto ou JSON bruto |

**Exemplo:**
```cpp
int count;
float temp;
bool active;
char city[64];
long timestamp;

client.get("/sensor/data")
      .getBody("count",       &count)
      .getBody("temperature", &temp)
      .getBody("active",      &active)
      .getBody("location",    city)
      .getBody("timestamp",   &timestamp);
```

---

### Captura de JSON Bruto com `String`

Passe uma `String*` para capturar objetos, arrays ou a resposta inteira sem desserialização:

```cpp
String entireResponse;
String firstUser;

client.get("/users")
      .getBody("",  &entireResponse) // Captura a resposta inteira da raiz
      .getBody("0", &firstUser);     // Captura o primeiro elemento como JSON bruto
```

!!! warning "Aviso de Memória"
    O uso de `String` com payloads muito grandes aloca memória na heap. Prefira tipos primitivos ou structs mapeadas sempre que possível.

---

### Mapeamento de Structs com `getBody(&struct)`

Vincula e preenche uma struct C++ mapeada com `REST_JSON_MAP` diretamente na raiz ou a partir de um caminho aninhado:

```cpp
template <typename T> RestRequest& getBody(T* target);
template <typename T> RestRequest& getBody(const char* key, T* target);
```

**Exemplo:**
```cpp
User user;
UserProfile profile;

client.get("/user/1").getBody(&user);
client.get("/dashboard").getBody("data.profile", &profile);
```

---

## Extraindo Cabeçalhos da Resposta

`getHeader()` registra a extração de cabeçalhos HTTP da resposta. A busca é **não sensível a maiúsculas/minúsculas (case-insensitive)** (ex: `"token"`, `"TOKEN"` e `"Token"` são equivalentes). Se o cabeçalho não estiver presente na resposta, a variável alvo **permanece inalterada**. Todas as sobrecargas são encadeáveis.

### `getHeader(name, target)`

```cpp
RestRequest& getHeader(const char* name, String* target);
RestRequest& getHeader(const char* name, char* target, size_t maxLength);
template <size_t N> RestRequest& getHeader(const char* name, char (&target)[N]);
RestRequest& getHeader(const char* name, int* target);
RestRequest& getHeader(const char* name, long* target);
RestRequest& getHeader(const char* name, float* target);
RestRequest& getHeader(const char* name, double* target);
RestRequest& getHeader(const char* name, bool* target);
```

**Tipos suportados para `target`:**

| Tipo da Variável | Formato do Cabeçalho | Conversão / Comportamento |
| :--- | :--- | :--- |
| `String*` | Texto | Copia o valor completo do cabeçalho |
| `char*`, `size_t` / `char[N]` | Texto | Copia até `N-1` caracteres com terminador nulo |
| `int*` | Numérico | Converte via `atoi` (ex: `Content-Length`) |
| `long*` | Numérico | Converte via `atol` (ex: `X-Timestamp`) |
| `float*` | Numérico | Converte via `strtof` (ex: `X-Rate-Limit`) |
| `double*` | Numérico | Converte via `strtod` |
| `bool*` | Texto / Booleano | `true` para `"true"` ou `"1"`, `false` caso contrário |

**Exemplo:**
```cpp
String token;
char contentType[64];
int contentLength;
bool isCached;

client.get("/auth")
      .getHeader("token",          &token)
      .getHeader("Content-Type",   contentType)
      .getHeader("Content-Length", &contentLength)
      .getHeader("X-Cache-Hit",    &isCached)
      .getBody("id", &id);
```

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
