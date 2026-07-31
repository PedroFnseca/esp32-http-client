---
tags:
  - example
  - port
  - network
---
# Porta Personalizada

Demonstra como conectar a um servidor rodando em uma **porta não padrão** (qualquer porta diferente de 80 para HTTP ou 443 para HTTPS).

**Fonte:** [`examples/PortSelection/PortSelection.ino`](https://github.com/PedroFnseca/esp32-http-client/blob/main/examples/PortSelection/PortSelection.ino)

---

## Quando Você Precisa Disto?

- Servidores de desenvolvimento local (ex: `http://192.168.1.100:8080`)
- Contêineres Docker expondo portas mapeadas
- Backends de gateway IoT personalizados
- APIs hospedadas em portas não padrão

---

## Uso

Passe a porta como o segundo argumento para o construtor do `ESP32HTTPClient`:

```cpp
// Servidor HTTP na porta 8080
ESP32HTTPClient client("http://192.168.1.100", 8080);

// Servidor HTTPS na porta 8443
ESP32HTTPClient client("https://meu-servidor.local", 8443);

// Porta padrão HTTPS explícita (opcional)
ESP32HTTPClient client("https://timeapi.io", 443);
```

---

## Sketch

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "SEU_SSID";
const char* password = "SUA_SENHA";

// Conectar a um servidor local na porta 8080
ESP32HTTPClient client("http://192.168.1.100", 8080);

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
    float sensorValue;

    client.get("/api/sensor")
          .getBody("value", &sensorValue);

    if (client.getStatusCode() == 200) {
        Serial.printf("Valor do sensor: %.2f\n", sensorValue);
    } else {
        Serial.printf("Erro: %d\n", client.getStatusCode());
    }

    delay(5000);
}
```

---

## Pontos Chave

- Porta `0` (o padrão quando nenhuma porta é especificada) significa "usar a porta padrão do protocolo" — 80 para HTTP e 443 para HTTPS.
- Uma vez definida no construtor, a porta se aplica a **todas as requisições** feitas por essa instância do cliente.
- A URL passada ao construtor **não** deve incluir a porta — use o segundo argumento do construtor em seu lugar.
