---
tags:
  - guide
  - advanced
---
# Uso Avançado

## Helpers de Autenticação
 
O `ESP32HTTPClient` fornece métodos auxiliares dedicados para os esquemas de autenticação mais comuns. Assim como o `setHeader()`, esses helpers configuram cabeçalhos persistentes enviados com **todas as requisições subsequentes**.

### Bearer Token (JWT / OAuth)

```cpp
client.bearer("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...");
client.get("/api/profile").getBody("username", username, sizeof(username));
```

### HTTP Basic Auth

Passe seu usuário e senha; a biblioteca formata e codifica as credenciais em Base64 automaticamente:

```cpp
client.basic("admin", "secret123");
client.get("/admin/metrics").getBody("uptime", &uptime);
```

### Chave de API (API Key)

Passe o nome do cabeçalho e sua chave de API:

```cpp
client.apiKey("x-api-key", "minha-chave-api-secreta");
client.get("/v1/sensors").getBody("temperature", &temp);
```

---

## Cabeçalhos HTTP Personalizados

Use `setHeader()` para registrar qualquer cabeçalho persistente personalizado que será enviado com **todas as requisições subsequentes** dessa instância do cliente:

```cpp
// Cabeçalhos personalizados
client.setHeader("X-Device-ID", "ESP32-001");
client.setHeader("X-Custom-Header", "custom-value");

// Cabeçalhos personalizados são enviados em todas as requisições subsequentes
client.get("/protected/resource").getBody("data", &minhaVar);
```

!!! note "Persistência de cabeçalhos"
    Cabeçalhos registrados com `setHeader()`, `bearer()`, `basic()` ou `apiKey()` persistem durante todo o ciclo de vida do objeto cliente. Eles são enviados a cada requisição. Para alterar um cabeçalho ou token, chame o método novamente com o novo valor.

---

## Content-Type Personalizado

Por padrão, o cabeçalho `Content-Type` é definido como `application/json` para todas as requisições com corpo. Substitua-o com `setContentType()`:

```cpp
client.setContentType("application/x-www-form-urlencoded");
client.post("/form").body("field", "value");
```

---

## Gerenciamento de Conexão Keep-Alive

Por padrão, o `ESP32HTTPClient` habilita o HTTP **Keep-Alive**, reutilizando a conexão TCP/TLS subjacente entre as requisições. Este é o principal motivo pelo qual a biblioteca é **~12x mais rápida** do que a abordagem padrão, já que os dispendiosos handshakes TLS acontecem apenas uma vez.

### Quando chamar `end()`

Após uma sequência de requisições, se o seu sketch entrar em um longo período de inatividade ou se você quiser liberar os buffers de memória TLS (~45KB para uma conexão ativa), chame `end()`:

```cpp
// Fazer várias requisições
client.get("/data1").getBody("val", &v1);
client.get("/data2").getBody("val", &v2);
client.get("/data3").getBody("val", &v3);

// Concluído por enquanto — liberar memória TLS
client.end();

delay(60000); // aguardar 60 segundos

// A próxima requisição restabelecerá a conexão automaticamente
client.get("/data4").getBody("val", &v4);
```

!!! tip "Você não precisa chamar `end()` entre as requisições"
    O Keep-Alive é automático. Chame `end()` apenas quando quiser liberar explicitamente a memória da conexão após um período de inatividade.

---

## Alterando a URL do Servidor em Tempo de Execução

Você pode alterar a URL base ou porta de destino sem precisar recriar a instância do `ESP32HTTPClient`:

```cpp
ESP32HTTPClient client("https://api.v1.exemplo.com");

// Alterna para o endpoint v2 ou servidor de homologação local
client.setBaseUrl("https://api.v2.exemplo.com", 443);

// Ou altera a porta independentemente
client.setUrl("http://192.168.1.100");
client.setPort(8080);
```

---

## Configuração de Timeout

Por padrão, o cliente utiliza um tempo limite (timeout) de **60000 ms** (1 minuto / 60 segundos). Você pode definir o padrão global ou sobrescrevê-lo em requisições individuais:

### Timeout Global

```cpp
// Define timeout padrão de 10 segundos para requisições futuras
client.setTimeout(10000);
```

### Timeout por Requisição

```cpp
// Requisição rápida com timeout de 1.5s
client.get("/quick-ping")
      .timeout(1500)
      .getBody("ok", &isOk);
```

---

## Tentativas Automáticas (Retries) & Recuperação de Rede

O `ESP32HTTPClient` gerencia automaticamente quedas de rede e conexões Keep-Alive inativas repetindo as tentativas com falha até um limite configurado. O padrão é `1` tentativa extra (retry).

### Max Retry Global

```cpp
// Permite até 3 tentativas adicionais em caso de falha de conexão
client.setMaxRetry(3);
```

### Retry por Requisição

```cpp
// Desativa tentativas automáticas para operações não-idempotentes
client.post("/payment/charge")
      .retry(0)
      .body("amount", 100);
```

---

## Callbacks (`onSuccess`, `onError`, `onResponse`)

Callbacks permitem associar funções de tratamento de forma limpa às requisições ou a nível de cliente.

### Callbacks na Requisição

```cpp
client.get("/sensors/temp")
      .onSuccess([](int code) {
          Serial.printf("Sucesso: HTTP %d\n", code);
      })
      .onError([](int code, const char* message) {
          Serial.printf("Falha na requisição (%d): %s\n", code, message);
      })
      .onResponse([](int code) {
          Serial.printf("Concluído com código %d\n", code);
      })
      .getBody("temperature", &temp);
```

### Callbacks no Cliente

Callbacks registrados no cliente são acionados em todas as requisições executadas por aquela instância:

```cpp
client.onError([](int code, const char* message) {
    Serial.printf("[Tratamento Global de Erro] Código %d: %s\n", code, message);
});
```

---

## Tratamento e Inspeção de Erros

Você pode inspecionar o resultado de qualquer requisição usando os métodos auxiliares integrados:

```cpp
client.get("/users/1").getBody("name", name, sizeof(name));

if (client.isSuccess()) {
    Serial.println("Usuário carregado com sucesso");
} else if (client.hasError()) {
    int code = client.getStatusCode();
    String errorMsg = client.getErrorMessage();
    Serial.printf("Falha com código %d: %s\n", code, errorMsg.c_str());
}
```

### Tabela de Referência de Códigos de Erro

#### Erros de Rede / Cliente (`code < 0`)

| Código | Constante | Descrição |
| :--- | :--- | :--- |
| `-1` | `HTTPC_ERROR_CONNECTION_REFUSED` | O host de destino recusou a conexão TCP. |
| `-2` | `HTTPC_ERROR_SEND_HEADER_FAILED` | Falha ao enviar cabeçalhos HTTP pelo socket. |
| `-3` | `HTTPC_ERROR_SEND_PAYLOAD_FAILED` | Falha ao transmitir o corpo da requisição. |
| `-4` | `HTTPC_ERROR_NOT_CONNECTED` | Cliente não está conectado à rede/socket. |
| `-5` | `HTTPC_ERROR_CONNECTION_LOST` | A conexão TCP foi interrompida inesperadamente. |
| `-6` | `HTTPC_ERROR_NO_STREAM` | Nenhum stream de resposta disponível. |
| `-7` | `HTTPC_ERROR_NO_HTTP_SERVER` | Servidor não respondeu com HTTP válido. |
| `-8` | `HTTPC_ERROR_TOO_LESS_RAM` | Memória RAM (Heap) insuficiente no ESP32. |
| `-9` | `HTTPC_ERROR_ENCODING` | Erro de codificação ou decodificação de transferência. |
| `-10` | `HTTPC_ERROR_STREAM_WRITE` | Falha na operação de escrita no stream. |
| `-11` | `HTTPC_ERROR_READ_TIMEOUT` | Tempo limite esgotado aguardando resposta do servidor. |

#### Códigos de Status HTTP (`code > 0`)

| Código | Status | Descrição |
| :--- | :--- | :--- |
| `200` | OK | Requisição concluída com sucesso. |
| `201` | Created | Recurso criado com sucesso no servidor. |
| `202` | Accepted | Requisição aceita para processamento. |
| `204` | No Content | Sucesso, servidor não retornou conteúdo. |
| `400` | Bad Request | Requisição malformada ou parâmetros inválidos. |
| `401` | Unauthorized | Credenciais de autenticação ausentes ou inválidas. |
| `403` | Forbidden | Autenticado, mas sem permissão de acesso. |
| `404` | Not Found | Endpoint solicitado não existe no servidor. |
| `408` | Request Timeout | Servidor expirou o tempo de espera. |
| `429` | Too Many Requests | Limite de taxa de requisições excedido. |
| `500` | Internal Server Error | Erro interno genérico no servidor. |
| `502` | Bad Gateway | Resposta inválida do servidor upstream. |
| `503` | Service Unavailable | Servidor sobrecarregado ou em manutenção. |
| `504` | Gateway Timeout | Gateway upstream expirou aguardando resposta. |

---

## Timestamps Unix Grandes (`long`)

Para timestamps Unix e outros inteiros grandes (maiores que `2^31 - 1`), use uma vinculação do tipo `long`:

```cpp
long unixTimestamp = 0;

client.get("/api/v1/time/current/unix")
      .getBody("unix_timestamp", &unixTimestamp);

Serial.printf("Tempo Unix: %ld\n", unixTimestamp);
```
