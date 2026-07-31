---
tags:
  - example
  - json
  - raw
---
# JSON Bruto

Demonstra como capturar um **objeto ou array JSON inteiro** em uma `String` do Arduino para processamento manual.

**Fonte:** [`examples/RawArrayJSON/RawArrayJSON.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/RawArrayJSON/RawArrayJSON.ino)

---

## Quando Usar Isto

Use a captura de JSON bruto quando:

- Você precisar passar a string JSON para outra biblioteca.
- Você precisar exibir ou registrar no log a resposta bruta.
- A estrutura da resposta for dinâmica e não puder ser vinculada estaticamente.

Para a maioria dos casos de uso, prefira vinculações diretas com chamadas tipadas a `getBody()` — elas evitam totalmente alocações na heap.

---

## Sketch

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "SEU_SSID";
const char* password = "SUA_SENHA";

ESP32HTTPClient client("https://jsonplaceholder.typicode.com");

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConectado ao WiFi");
}

void loop() {
    String entireArray;
    String secondUser;

    client.get("/users")
          .getBody("",  &entireArray)  // captura todo o array raiz
          .getBody("1", &secondUser);  // captura o segundo objeto de usuário

    if (client.getStatusCode() == 200) {
        Serial.printf("JSON do segundo usuário:\n%s\n", secondUser.c_str());
    } else {
        Serial.printf("Erro: %d\n", client.getStatusCode());
    }

    delay(30000);
}
```

---

## Capturando a Resposta Raiz

Passe uma **string vazia** `""` como chave para capturar todo o objeto ou array no nível raiz:

```cpp
String raw;
client.get("/users").getBody("", &raw); // captura toda a resposta JSON
```

## Capturando um Sub-objeto ou Sub-array

Passe o caminho para o objeto/array que você deseja capturar:

```cpp
String addressObj;
client.get("/users/1").getBody("address", &addressObj);
// addressObj = {"street":"Kulas Light","suite":"Apt. 556","city":"Gwenborough",...}

String secondUser;
client.get("/users").getBody("1", &secondUser);
// secondUser = o segundo elemento completo do array
```

---

## Saída Esperada no Monitor Serial

```json
JSON do segundo usuário:
{"id":2,"name":"Ervin Howell","username":"Antonette","email":"...","address":{...},...}
```

---

!!! warning "Impacto na memória"
    Cada caractere do JSON capturado é anexado à `String` na heap um byte por vez, causando realocações repetidas. Evite este padrão com cargas maiores que alguns kilobytes em placas com restrição de memória.
