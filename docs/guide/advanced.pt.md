---
tags:
  - guide
  - advanced
---
# Uso Avançado

## Cabeçalhos HTTP Personalizados

Use `setHeader()` para registrar um cabeçalho persistente que será enviado com **todas as requisições subsequentes** dessa instância do cliente. Esta é a forma padrão de passar tokens de autenticação, chaves de API ou qualquer cabeçalho personalizado exigido pelo seu servidor.

```cpp
// Cabeçalho de autorização (Bearer token)
client.setHeader("Authorization", "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...");

// Cabeçalho de chave de API
client.setHeader("X-API-Key", "minha-chave-api-secreta");

// Cabeçalhos personalizados são enviados em todas as requisições subsequentes
client.get("/protected/resource").getBody("data", &minhaVar);
```

!!! note "Persistência de cabeçalhos"
    Cabeçalhos registrados com `setHeader()` persistem durante todo o ciclo de vida do objeto cliente. Eles são enviados a cada requisição. Para alterar um cabeçalho, chame `setHeader()` novamente com o mesmo nome e um novo valor.

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

## Recuperação Automática de Conexões Inativas (Stale Connections)

Se uma conexão Keep-Alive se tornar inativa (por exemplo, o servidor a fechou do lado dele), a biblioteca detecta automaticamente a falha, fecha o socket, restabelece a conexão e tenta novamente a requisição **uma vez** — de forma totalmente transparente para o seu código.

```cpp
// Funciona mesmo se o servidor tiver descartado a conexão
client.get("/sensor").getBody("temp", &temperature);
```

---

## Suporte a `Transfer-Encoding: chunked`

Muitos servidores modernos (incluindo aqueles atrás de balanceadores de carga ou API gateways) respondem com `Transfer-Encoding: chunked`. O `BufferedStreamReader` interno da biblioteca lida com a decodificação de chunks automaticamente e de forma transparente. Nenhuma configuração é necessária.

---

## Timestamps Unix Grandes (`long`)

Para timestamps Unix e outros inteiros grandes (maiores que `2^31 - 1`), use uma vinculação do tipo `long`:

```cpp
long unixTimestamp = 0;

client.get("/api/v1/time/current/unix")
      .getBody("unix_timestamp", &unixTimestamp);

Serial.printf("Tempo Unix: %ld\n", unixTimestamp);
```

---

## Verificando o Status da Conexão

Embora a biblioteca gerencie as conexões automaticamente, você sempre pode inspecionar o resultado de uma requisição usando `getStatusCode()`:

```cpp
client.get("/health");
int code = client.getStatusCode();

if (code == 200) {
    Serial.println("Servidor saudável");
} else if (code < 0) {
    Serial.println("Erro de rede — não foi possível alcançar o servidor");
} else {
    Serial.printf("Servidor respondeu com HTTP %d\n", code);
}
```
