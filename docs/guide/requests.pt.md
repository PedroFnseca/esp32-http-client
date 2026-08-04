---
tags:
  - guide
  - request
  - http
---
# Fazendo Requisições

Cada método HTTP no `ESP32HTTPClient` retorna um objeto `RestRequest`. Você pode encadear chamadas `.query()`, `.body()` e `.getBody()` nele. A requisição é despachada automaticamente quando o encadeamento sai de escopo (isto é, ao final da instrução).

---

## GET

```cpp
client.get("/todos/1");
```

### Parâmetros de Rota (Path Parameters)

Use `.path(chave, valor)` para substituir dinamicamente parâmetros `{placeholder}` no caminho da URL. Suporta `int`, `float`, `double`, `long`, `bool` e `const char*`.

```cpp
// Produz: GET /users/15
client.get("/users/{id}")
      .path("id", 15);
```

### Parâmetros de Consulta (Query Parameters)

Use `.query(chave, valor)` para anexar parâmetros na URL. Suporta `int`, `float`, `double`, `long`, `bool` e `const char*`.

```cpp
// Produz: GET /users?page=2&limit=20&search=pedro
client.get("/users")
      .query("page", 2)
      .query("limit", 20)
      .query("search", "pedro");
```

---

## POST

Use `.body(chave, valor)` para construir o corpo JSON. O cabeçalho `Content-Type` é definido automaticamente para `application/json`.

```cpp
// Corpo: {"title": "foo", "body": "bar", "userId": 1}
int newId;

client.post("/posts")
      .body("title", "foo")
      .body("body", "bar")
      .body("userId", 1)
      .getBody("id", &newId);

// Verificar status de criação
if (client.getStatusCode() == 201) {
    Serial.printf("Criado com ID: %d\n", newId);
}
```

---

## PUT / update

`put()` e `update()` são aliases para o mesmo método `HTTP PUT`:

```cpp
client.put("/lights/1").body("state", "OFF");
// ou de forma equivalente:
client.update("/lights/1").body("state", "OFF");
```

---

## PATCH

```cpp
client.patch("/config/wifi")
      .body("ssid", "NovaRede")
      .body("password", "segredo");
```

---

## DELETE

```cpp
client.del("/logs/old.log");
```

Requisições DELETE também podem incluir opcionalmente um corpo JSON:

```cpp
client.del("/sessions")
      .body("userId", 42);
```

---

## Tipos de Valores Suportados para `.path()`, `.query()` e `.body()`

| Tipo C | Saída JSON | Exemplo |
| :--- | :--- | :--- |
| `const char*` / `char*` | `"string"` (entre aspas) | `.body("name", "Pedro")` |
| `int` | `42` | `.body("age", 21)` |
| `long` | `1721000000` | `.body("ts", unixTime)` |
| `float` | `24.5` | `.body("temp", 24.5f)` |
| `double` | `3.14159265` | `.body("pi", 3.14159265)` |
| `bool` | `true` / `false` | `.body("active", true)` |

---

## Verificando o Código de Status HTTP

Após qualquer requisição, use `getStatusCode()` para inspecionar o resultado:

```cpp
client.get("/api/status");

int code = client.getStatusCode();
if (code == 200) {
    Serial.println("OK");
} else if (code < 0) {
    Serial.println("Erro de conexão");
} else {
    Serial.printf("Erro HTTP: %d\n", code);
}
```

!!! note "Disponibilidade do código de status"
    `getStatusCode()` sempre retorna o código da **última requisição concluída**. Valores negativos indicam um erro no nível de rede (por exemplo, sem conexão).

---

## Próximo Passo

→ [Lendo Respostas](responses.pt.md)
