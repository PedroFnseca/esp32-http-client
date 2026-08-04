---
tags:
  - api
  - client
  - class
---
# ESP32HTTPClient

O principal ponto de entrada da biblioteca. Crie uma instância por URL base de servidor e reutilize-a em todas as requisições.

**Cabeçalho:** `#include "ESP32HTTPClient.h"`

---

## Construtor

### `ESP32HTTPClient(baseUrl)`

Cria um cliente com seleção automática de porta (80 para HTTP, 443 para HTTPS).

```cpp
ESP32HTTPClient(const char* baseUrl);
```

**Parâmetros:**

| Parâmetro | Tipo | Descrição |
| :--- | :--- | :--- |
| `baseUrl` | `const char*` | A URL base incluindo protocolo (ex: `"https://api.example.com"`). Não inclua uma barra no final. |

**Exemplo:**
```cpp
ESP32HTTPClient client("https://api.example.com");
```

---

### `ESP32HTTPClient(baseUrl, port)`

Cria um cliente direcionado a uma porta específica.

```cpp
ESP32HTTPClient(const char* baseUrl, int port);
```

**Parâmetros:**

| Parâmetro | Tipo | Descrição |
| :--- | :--- | :--- |
| `baseUrl` | `const char*` | A URL base incluindo o protocolo. |
| `port` | `int` | A porta TCP alvo (ex: `8080`, `443`). |

**Exemplo:**
```cpp
ESP32HTTPClient client("http://192.168.1.100", 8080);
```

---

## Métodos de Requisição HTTP

Cada método retorna um [`RestRequest`](restrequest.pt.md) que pode ser encadeado com `.query()`, `.body()` e `.getBody()`. A requisição HTTP é enviada quando o objeto `RestRequest` sai de escopo ou quando o primeiro `.getBody()` é adicionado.

---

### `get(path)`

Envia uma requisição `GET` para `baseUrl + path`.

```cpp
RestRequest get(const char* path);
```

**Exemplo:**
```cpp
client.get("/todos/1").getBody("title", title, sizeof(title));
```

---

### `post(path)`

Envia uma requisição `POST` para `baseUrl + path`.

```cpp
RestRequest post(const char* path);
```

**Exemplo:**
```cpp
client.post("/users").body("name", "Pedro").body("age", 21).getBody("id", &newId);
```

---

### `put(path)`

Envia uma requisição `PUT` para `baseUrl + path`.

```cpp
RestRequest put(const char* path);
```

**Exemplo:**
```cpp
client.put("/posts/1").body("title", "novo título");
```

---

### `update(path)`

Alias semântico para `put()`. Envia uma requisição `HTTP PUT` idêntica.

```cpp
RestRequest update(const char* path);
```

**Exemplo:**
```cpp
client.update("/lights/1").body("state", "OFF");
```

---

### `patch(path)`

Envia uma requisição `PATCH` para `baseUrl + path` para atualizações parciais.

```cpp
RestRequest patch(const char* path);
```

**Exemplo:**
```cpp
client.patch("/config").body("timeout", 30);
```

---

### `del(path)`

Envia uma requisição `DELETE` para `baseUrl + path`.

```cpp
RestRequest del(const char* path);
```

**Exemplo:**
```cpp
client.del("/sessions/42");
```

---

## Métodos de Configuração

---

### `setHeader(name, value)`

Registra um cabeçalho HTTP personalizado enviado com **todas as requisições subsequentes**.

```cpp
void setHeader(const char* name, const char* value);
```

| Parâmetro | Limite |
| :--- | :--- |
| `name` | Até 63 caracteres |
| `value` | Até 255 caracteres |

**Exemplo:**
```cpp
client.setHeader("Authorization", "Bearer meu-token");
client.setHeader("X-Device-ID",   "ESP32-001");
```

!!! note
    Cabeçalhos persistem durante todo o ciclo de vida da instância do cliente. Chame `setHeader()` novamente com o mesmo nome para sobrescrevê-lo.

---

### `bearer(token)`

Define o cabeçalho `Authorization: Bearer <token>` enviado com **todas as requisições subsequentes**.

```cpp
void bearer(const char* token);
```

| Parâmetro | Tipo | Descrição |
| :--- | :--- | :--- |
| `token` | `const char*` | A string do token Bearer / JWT. |

**Exemplo:**
```cpp
client.bearer("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...");
```

---

### `basic(user, password)`

Codifica as credenciais em Base64 e define o cabeçalho `Authorization: Basic <base64>` enviado com **todas as requisições subsequentes**.

```cpp
void basic(const char* user, const char* password);
```

| Parâmetro | Tipo | Descrição |
| :--- | :--- | :--- |
| `user` | `const char*` | Nome de usuário. |
| `password` | `const char*` | Senha. |

**Exemplo:**
```cpp
client.basic("admin", "secret123");
```

---

### `apiKey(name, key)`

Define um cabeçalho de chave de API (ex: `x-api-key`) enviado com **todas as requisições subsequentes**.

```cpp
void apiKey(const char* name, const char* key);
```

| Parâmetro | Tipo | Descrição |
| :--- | :--- | :--- |
| `name` | `const char*` | Nome do cabeçalho (ex: `"X-API-Key"` ou `"x-api-key"`). |
| `key` | `const char*` | String da chave de API. |

**Exemplo:**
```cpp
client.apiKey("x-api-key", "minha-chave-api-secreta");
```

---

### `setContentType(contentType)`

Sobrescreve o cabeçalho `Content-Type` usado no corpo das requisições. O padrão é `application/json`.

```cpp
void setContentType(const char* contentType);
```

**Exemplo:**
```cpp
client.setContentType("application/x-www-form-urlencoded");
```

---

### `getStatusCode()`

Retorna o código de status HTTP da **última requisição concluída**.

```cpp
int getStatusCode() const;
```

**Valores de retorno:**

| Valor | Significado |
| :--- | :--- |
| `> 0` | Código de status HTTP padrão (200, 201, 404, 500…) |
| `< 0` | Erro no nível de rede (sem conexão, tempo limite esgotado, etc.) |
| `0` | Nenhuma requisição foi realizada ainda |

**Exemplo:**
```cpp
client.get("/health");
if (client.getStatusCode() == 200) {
    Serial.println("OK");
}
```

---

### `end()`

Fecha a conexão TCP/TLS Keep-Alive persistente e libera seus buffers de memória.

```cpp
void end();
```

Chame este método após uma sequência de requisições para liberar ~45KB de memória TLS durante um período de inatividade longo. A próxima requisição restabelecerá a conexão automaticamente.

**Exemplo:**
```cpp
client.get("/data1").getBody("v", &v1);
client.get("/data2").getBody("v", &v2);

client.end(); // libera memória TLS
delay(60000);

client.get("/data3").getBody("v", &v3); // reconecta automaticamente
```
