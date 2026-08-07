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

!!! tip "Lendo cabeçalhos de resposta"
    `setHeader()` define cabeçalhos a serem enviados na **requisição**. Para ler cabeçalhos retornados pelo servidor na **resposta**, utilize [RestRequest::getHeader](restrequest.pt.md#extraindo-cabecalhos-da-resposta).

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

### `setBaseUrl(baseUrl, port)`

Altera a URL base e a porta de destino em tempo de execução para as requisições subsequentes.

```cpp
void setBaseUrl(const char* baseUrl, int port = 0);
```

**Exemplo:**
```cpp
client.setBaseUrl("https://api.v2.exemplo.com", 443);
```

---

### `setUrl(baseUrl)`

Atalho para atualizar apenas a URL base em tempo de execução.

```cpp
void setUrl(const char* baseUrl);
```

---

### `setPort(port)`

Atualiza a porta TCP de destino em tempo de execução.

```cpp
void setPort(int port);
```

---

### `getBaseUrl()`

Retorna a URL base atual configurada.

```cpp
const char* getBaseUrl() const;
```

---

### `getPort()`

Retorna a porta TCP configurada (ou 0 se padrão).

```cpp
int getPort() const;
```

---

### `setTimeout(timeoutMs)`

Configura o tempo limite (timeout) padrão em milissegundos para todas as requisições deste cliente. O padrão é `60000` ms (1 minuto).

```cpp
void setTimeout(uint16_t timeoutMs);
```

**Exemplo:**
```cpp
client.setTimeout(10000); // 10 segundos
```

---

### `getTimeout()`

Retorna o timeout configurado em milissegundos.

```cpp
uint16_t getTimeout() const;
```

---

### `setMaxRetry(maxRetry)`

Configura o número máximo de tentativas automáticas em caso de falha de rede. O padrão é `1` tentativa.

```cpp
void setMaxRetry(int maxRetry);
```

**Exemplo:**
```cpp
client.setMaxRetry(3); // Até 3 tentativas
```

---

### `getMaxRetry()`

Retorna o número máximo configurado de tentativas.

```cpp
int getMaxRetry() const;
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

## Inspeção de Resposta e Tratamento de Erros

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
| `< 0` | Erro no nível de rede (sem conexão, timeout, etc.) |
| `0` | Nenhuma requisição foi realizada ainda |

---

### `isSuccess()`

Retorna `true` se a última requisição foi concluída com status HTTP de sucesso 2xx (`200 <= code < 300`).

```cpp
bool isSuccess() const;
```

**Exemplo:**
```cpp
client.get("/data").getBody("val", &val);
if (client.isSuccess()) {
    Serial.println("Requisição bem-sucedida!");
}
```

---

### `hasError()`

Retorna `true` se a última requisição falhou devido a um erro de rede (`code < 0`) ou erro HTTP de cliente/servidor (`code >= 400`).

```cpp
bool hasError() const;
```

**Exemplo:**
```cpp
client.get("/data").getBody("val", &val);
if (client.hasError()) {
    Serial.printf("Erro (%d): %s\n", client.getStatusCode(), client.getErrorMessage().c_str());
}
```

---

### `getErrorMessage()`

Retorna uma descrição legível do último código de status ou de erro retornado.

```cpp
String getErrorMessage() const;
```

---

### `errorToString(code)`

Método estático auxiliar que converte qualquer código de status HTTP ou código de erro negativo em uma descrição textual.

```cpp
static String errorToString(int code);
```

---

## Serialização & Desserialização de Structs

Métodos utilitários estáticos para conversão entre structs C++ mapeadas e strings JSON.

---

### `toJson(struct)`

Serializa uma struct mapeada com `REST_JSON_MAP` em uma string JSON.

```cpp
template <typename T>
static String toJson(const T& obj);
```

**Exemplo:**
```cpp
User user = {15, "Pedro", 9.5f, true};
String json = ESP32HTTPClient::toJson(user);
```

---

### `fromJson(json, struct)`

Preenche uma struct mapeada com `REST_JSON_MAP` a partir de uma string JSON.

```cpp
template <typename T>
static void fromJson(const String& json, T* target);
template <typename T>
static void fromJson(const char* json, T* target);
```

**Exemplo:**
```cpp
User user;
ESP32HTTPClient::fromJson("{\"id\":15,\"name\":\"Pedro\"}", &user);
```

---

## Callbacks

Você pode registrar callbacks globais na instância do cliente que são executados sempre que qualquer requisição é finalizada.

---

### `onSuccess(callback)`

Registra um callback executado quando qualquer requisição finaliza com sucesso 2xx (`200 <= code < 300`).

```cpp
void onSuccess(HttpResponseCallback cb);
```

**Exemplo:**
```cpp
client.onSuccess([](int code) {
    Serial.printf("Requisição bem-sucedida com status %d\n", code);
});
```

---

### `onError(callback)`

Registra um callback executado quando qualquer requisição falha com erro (`code < 200 || code >= 400`).

```cpp
void onError(HttpErrorCallback cb);
void onError(HttpResponseCallback cb);
```

**Exemplo:**
```cpp
client.onError([](int code, const char* message) {
    Serial.printf("Requisição falhou (%d): %s\n", code, message);
});
```

---

### `onResponse(callback)`

Registra um callback executado em **qualquer** requisição finalizada, independentemente de sucesso ou falha.

```cpp
void onResponse(HttpResponseCallback cb);
```

**Exemplo:**
```cpp
client.onResponse([](int code) {
    Serial.printf("Resposta recebida com código %d\n", code);
});
```

---

## Gerenciamento de Conexão

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

---

## Tabela de Referência de Códigos de Erro

### Códigos de Erro de Rede / Cliente (`code < 0`)

| Código | Constante | Descrição |
| :--- | :--- | :--- |
| `-1` | `HTTPC_ERROR_CONNECTION_REFUSED` | O host de destino recusou a conexão TCP. |
| `-2` | `HTTPC_ERROR_SEND_HEADER_FAILED` | Falha ao enviar cabeçalhos HTTP pelo socket. |
| `-3` | `HTTPC_ERROR_SEND_PAYLOAD_FAILED` | Falha ao transmitir o corpo da requisição. |
| `-4` | `HTTPC_ERROR_NOT_CONNECTED` | Cliente não está conectado à rede/socket. |
| `-5` | `HTTPC_ERROR_CONNECTION_LOST` | A conexão TCP foi interrompida inesperadamente. |
| `-6` | `HTTPC_ERROR_NO_STREAM` | Nenhum stream de resposta disponível. |
| `-7` | `HTTPC_ERROR_NO_HTTP_SERVER` | Servidor não respondeu com HTTP válido. |
| `-8` | `HTTPC_ERROR_TOO_LESS_RAM` | Memória RAM (Heap) insuficiente para a operação. |
| `-9` | `HTTPC_ERROR_ENCODING` | Erro de codificação ou decodificação de transferência. |
| `-10` | `HTTPC_ERROR_STREAM_WRITE` | Falha na operação de escrita no stream. |
| `-11` | `HTTPC_ERROR_READ_TIMEOUT` | Tempo limite esgotado aguardando resposta do servidor. |

### Códigos de Status HTTP Comuns (`code > 0`)

| Código | Status | Descrição |
| :--- | :--- | :--- |
| `200` | OK | Requisição concluída com sucesso. |
| `201` | Created | Recurso criado com sucesso no servidor. |
| `202` | Accepted | Requisição aceita para processamento assíncrono. |
| `204` | No Content | Sucesso, servidor não retornou conteúdo no corpo. |
| `400` | Bad Request | Requisição malformada ou dados inválidos. |
| `401` | Unauthorized | Credenciais de autenticação ausentes ou inválidas. |
| `403` | Forbidden | Autenticado, mas sem permissão de acesso ao recurso. |
| `404` | Not Found | O endpoint solicitado não existe no servidor. |
| `408` | Request Timeout | Servidor expirou o tempo de espera pela requisição. |
| `429` | Too Many Requests | Limite de taxa de requisições excedido. |
| `500` | Internal Server Error | Erro interno genérico no servidor. |
| `502` | Bad Gateway | Resposta inválida recebida do servidor upstream. |
| `503` | Service Unavailable | Servidor sobrecarregado ou em manutenção. |
| `504` | Gateway Timeout | Gateway upstream expirou aguardando resposta. |
