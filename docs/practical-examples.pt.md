---
title: Exemplos Práticos - Exemplos com ESP32 HTTP Client
description: Exemplos práticos para projetos IoT e embarcados usando ESP32HTTPClient, incluindo estações meteorológicas, monitor do GitHub, sensores IoT e APIs REST.
keywords: exemplos práticos ESP32, projetos API ESP32, casos de uso cliente HTTP ESP32, exemplos REST Arduino, estação meteorológica IoT ESP32
tags:
  - examples
  - practical
  - iot
  - rest
  - projects
---

# Exemplos Práticos

Exemplos práticos demonstrando a integração da biblioteca `ESP32HTTPClient` em dispositivos embarcados e projetos IoT.

---

## 1. Estação Meteorológica

Consulta dados meteorológicos em tempo real, como temperatura, umidade relativa do ar e velocidade do vento para uma coordenada geográfica.

**API:** [Open-Meteo](https://open-meteo.com/) (`https://api.open-meteo.com/v1/forecast`)

??? example "Ver Código"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "SUA_REDE_WIFI";
    const char* password = "SUA_SENHA_WIFI";

    // --- Assinaturas para lógica de hardware / específica ---
    void exibirClima(float temp, int umidade, float vento); // implementação específica

    ESP32HTTPClient weatherClient("https://api.open-meteo.com");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        float temperatura = 0.0f;
        int umidade = 0;
        float vento = 0.0f;

        weatherClient.get("/v1/forecast")
            .query("latitude", "-23.5505")
            .query("longitude", "-46.6333")
            .query("current", "temperature_2m,relative_humidity_2m,wind_speed_10m")
            .getBody("current.temperature_2m", &temperatura)
            .getBody("current.relative_humidity_2m", &umidade)
            .getBody("current.wind_speed_10m", &vento);

        if (weatherClient.isSuccess()) {
            exibirClima(temperatura, umidade, vento);
        } else {
            Serial.printf("Erro ao consultar clima: %s (HTTP %d)\n",
                          weatherClient.getLastError(), weatherClient.getStatusCode());
        }

        delay(600000); // Consulta a cada 10 minutos
    }
    ```

---

## 2. Terminal Pokémon

Pesquisa informações de um Pokémon pelo nome ou identificador, obtendo atributos como experiência base, peso, altura e tipo através de parâmetros de rota fluentes.

**API:** [PokéAPI](https://pokeapi.co/) (`https://pokeapi.co/api/v2/pokemon/{name}`)

??? example "Ver Código"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "SUA_REDE_WIFI";
    const char* password = "SUA_SENHA_WIFI";

    // --- Assinaturas para lógica de hardware / específica ---
    void desenharCardPokemon(const char* nome, int altura, int peso, int xp); // implementação específica
    const char* obterPokemonSelecionado();                                    // implementação específica

    ESP32HTTPClient pokeClient("https://pokeapi.co");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        const char* pokemonAlvo = obterPokemonSelecionado();

        char nome[32] = {0};
        int altura = 0;
        int peso = 0;
        int xp = 0;

        pokeClient.get("/api/v2/pokemon/{name}")
            .path("name", pokemonAlvo)
            .getBody("name", nome, sizeof(nome))
            .getBody("height", &altura)
            .getBody("weight", &peso)
            .getBody("base_experience", &xp);

        if (pokeClient.isSuccess()) {
            desenharCardPokemon(nome, altura, peso, xp);
        }

        delay(30000);
    }
    ```

---

## 3. Monitor de Repositório GitHub

Monitora métricas públicas de um repositório no GitHub, incluindo número de estrelas, forks e issues abertas.

**API:** [GitHub REST API](https://docs.github.com/en/rest/repos/repos#get-a-repository) (`https://api.github.com/repos/{owner}/{repo}`)

??? example "Ver Código"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "SUA_REDE_WIFI";
    const char* password = "SUA_SENHA_WIFI";

    // --- Assinaturas para lógica de hardware / específica ---
    void atualizarPainelStats(int estrelas, int forks, int issues); // implementação específica

    ESP32HTTPClient githubClient("https://api.github.com");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }

        // A API do GitHub exige o cabeçalho User-Agent
        githubClient.header("User-Agent", "ESP32HTTPClient-Monitor");
    }

    void loop() {
        int estrelas = 0;
        int forks = 0;
        int issues = 0;

        githubClient.get("/repos/{owner}/{repo}")
            .path("owner", "PedroFnseca")
            .path("repo", "esp32-http-client")
            .getBody("stargazers_count", &estrelas)
            .getBody("forks_count", &forks)
            .getBody("open_issues_count", &issues);

        if (githubClient.isSuccess()) {
            atualizarPainelStats(estrelas, forks, issues);
        }

        delay(300000); // Consulta a cada 5 minutos
    }
    ```

---

## 4. Verificador de Releases do GitHub

Verifica periodicamente a versão da última release lançada em um repositório GitHub para detecção de atualizações de firmware (OTA).

**API:** [GitHub REST API](https://docs.github.com/en/rest/releases/releases#get-the-latest-release) (`https://api.github.com/repos/{owner}/{repo}/releases/latest`)

??? example "Ver Código"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid          = "SUA_REDE_WIFI";
    const char* password      = "SUA_SENHA_WIFI";
    const char* versaoCorrente = "v1.0.0";

    // --- Assinaturas para lógica de hardware / específica ---
    void notificarNovaVersao(const char* novaTag, const char* tituloRelease); // implementação específica

    ESP32HTTPClient githubClient("https://api.github.com");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }

        githubClient.header("User-Agent", "ESP32-OTA-Checker");
    }

    void loop() {
        char tagRecente[32] = {0};
        char titulo[64] = {0};

        githubClient.get("/repos/{owner}/{repo}/releases/latest")
            .path("owner", "PedroFnseca")
            .path("repo", "esp32-http-client")
            .getBody("tag_name", tagRecente, sizeof(tagRecente))
            .getBody("name", titulo, sizeof(titulo));

        if (githubClient.isSuccess()) {
            if (strcmp(tagRecente, versaoCorrente) != 0) {
                Serial.printf("Nova versao encontrada: %s (Atual: %s)\n", tagRecente, versaoCorrente);
                notificarNovaVersao(tagRecente, titulo);
            }
        }

        delay(3600000); // Checa 1 vez por hora
    }
    ```

---

## 5. Rastreador da ISS

Acompanha a trajetória orbital e as coordenadas geográficas (latitude e longitude) da Estação Espacial Internacional em tempo real.

**API:** [Open Notify](http://open-notify.org/) (`http://api.open-notify.org/iss-now.json`)

??? example "Ver Código"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "SUA_REDE_WIFI";
    const char* password = "SUA_SENHA_WIFI";

    // --- Assinaturas para lógica de hardware / específica ---
    void plotarPosicaoISS(float latitude, float longitude); // implementação específica

    ESP32HTTPClient issClient("http://api.open-notify.org");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        float latitude = 0.0f;
        float longitude = 0.0f;

        issClient.get("/iss-now.json")
            .getBody("iss_position.latitude", &latitude)
            .getBody("iss_position.longitude", &longitude);

        if (issClient.isSuccess()) {
            plotarPosicaoISS(latitude, longitude);
        }

        delay(10000); // Atualiza a cada 10 segundos
    }
    ```

---

## 6. Conversor de Moedas

Consulta taxas de câmbio atualizadas em tempo real e calcula conversões monetárias entre pares de moedas (ex: EUR, USD, BRL).

**API:** [Frankfurter API](https://www.frankfurter.app/) (`https://api.frankfurter.app/latest`)

??? example "Ver Código"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "SUA_REDE_WIFI";
    const char* password = "SUA_SENHA_WIFI";

    // --- Assinaturas para lógica de hardware / específica ---
    void exibirCotacao(const char* base, const char* destino, float taxa); // implementação específica

    ESP32HTTPClient fxClient("https://api.frankfurter.app");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        float taxaUSD = 0.0f;
        float taxaBRL = 0.0f;

        fxClient.get("/latest")
            .query("from", "EUR")
            .query("to", "USD,BRL")
            .getBody("rates.USD", &taxaUSD)
            .getBody("rates.BRL", &taxaBRL);

        if (fxClient.isSuccess()) {
            exibirCotacao("EUR", "USD", taxaUSD);
            exibirCotacao("EUR", "BRL", taxaBRL);
        }

        delay(1800000); // Atualiza a cada 30 minutos
    }
    ```

---

## 7. Curiosidades Aleatórias

Busca e exibe fatos e curiosidades aleatórias para displays de mesa, relógios inteligentes ou painéis interativos.

**API:** [Useless Facts API](https://uselessfacts.jsph.pl/) (`https://uselessfacts.jsph.pl/api/v2/facts/random`)

??? example "Ver Código"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "SUA_REDE_WIFI";
    const char* password = "SUA_SENHA_WIFI";

    // --- Assinaturas para lógica de hardware / específica ---
    void exibirFatoNoDisplay(const char* textoFato); // implementação específica

    ESP32HTTPClient factsClient("https://uselessfacts.jsph.pl");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        char fato[256] = {0};

        factsClient.get("/api/v2/facts/random")
            .query("language", "en")
            .getBody("text", fato, sizeof(fato));

        if (factsClient.isSuccess()) {
            exibirFatoNoDisplay(fato);
        }

        delay(60000); // Nova curiosidade a cada minuto
    }
    ```

---

## 8. Visualizador de Fotos de Cachorros

Obtém URLs de fotos aleatórias de cachorros a partir de uma API aberta para exibição em telas TFT, LCD ou painéis e-paper.

**API:** [Dog CEO Dog API](https://dog.ceo/dog-api/) (`https://dog.ceo/api/breeds/image/random`)

??? example "Ver Código"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "SUA_REDE_WIFI";
    const char* password = "SUA_SENHA_WIFI";

    // --- Assinaturas para lógica de hardware / específica ---
    void baixarERenderizarImagem(const char* urlImagem); // implementação específica

    ESP32HTTPClient dogClient("https://dog.ceo");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        char urlImagem[128] = {0};
        char status[16] = {0};

        dogClient.get("/api/breeds/image/random")
            .getBody("message", urlImagem, sizeof(urlImagem))
            .getBody("status", status, sizeof(status));

        if (dogClient.isSuccess() && strcmp(status, "success") == 0) {
            baixarERenderizarImagem(urlImagem);
        }

        delay(30000);
    }
    ```

---

## 9. Foto Astronômica do Dia (NASA)

Consulta metadados da Foto Astronômica do Dia da NASA, incluindo título, data, explicação e a URL direta da imagem.

**API:** [NASA Open APIs](https://api.nasa.gov/) (`https://api.nasa.gov/planetary/apod`)

??? example "Ver Código"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid      = "SUA_REDE_WIFI";
    const char* password  = "SUA_SENHA_WIFI";
    const char* chaveNasa = "DEMO_KEY"; // Substitua pela sua chave da NASA

    // --- Assinaturas para lógica de hardware / específica ---
    void exibirApod(const char* titulo, const char* data, const char* url); // implementação específica

    ESP32HTTPClient nasaClient("https://api.nasa.gov");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        char titulo[128] = {0};
        char data[16]    = {0};
        char url[256]    = {0};

        nasaClient.get("/planetary/apod")
            .query("api_key", chaveNasa)
            .getBody("title", titulo, sizeof(titulo))
            .getBody("date", data, sizeof(date))
            .getBody("url", url, sizeof(url));

        if (nasaClient.isSuccess()) {
            exibirApod(titulo, data, url);
        }

        delay(86400000); // Consulta diária
    }
    ```

---

## 10. Pesquisa de Livros

Consulta o catálogo do Open Library para buscar detalhes de uma publicação através do ISBN ou título (título, autor, número de páginas e ano).

**API:** [Open Library](https://openlibrary.org/developers/api) (`https://openlibrary.org/api/books`)

??? example "Ver Código"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "SUA_REDE_WIFI";
    const char* password = "SUA_SENHA_WIFI";

    // --- Assinaturas para lógica de hardware / específica ---
    void exibirDetalhesLivro(const char* titulo, int paginas, const char* dataPub); // implementação específica

    ESP32HTTPClient libraryClient("https://openlibrary.org");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        char titulo[64] = {0};
        char dataPub[32] = {0};
        int paginas = 0;

        // Consulta pelo ISBN: 0451526538 (ex: 1984 de George Orwell)
        libraryClient.get("/api/books")
            .query("bibkeys", "ISBN:0451526538")
            .query("format", "json")
            .query("jscmd", "data")
            .getBody("ISBN:0451526538.title", titulo, sizeof(titulo))
            .getBody("ISBN:0451526538.publish_date", dataPub, sizeof(dataPub))
            .getBody("ISBN:0451526538.number_of_pages", &paginas);

        if (libraryClient.isSuccess()) {
            exibirDetalhesLivro(titulo, paginas, dataPub);
        }

        delay(60000);
    }
    ```

---

## 11. Monitor de Planta Inteligente

Realiza a leitura de múltiplos sensores (umidade do solo, temperatura, luminosidade) e envia telemetria via requisição POST JSON para a nuvem.

**API:** Backend IoT Customizado (`https://api.exemplo-iot.com/v1/telemetry`)

??? example "Ver Código"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "SUA_REDE_WIFI";
    const char* password = "SUA_SENHA_WIFI";

    // --- Assinaturas para lógica de hardware / específica ---
    float lerSensorUmidadeSolo();   // implementação específica
    float lerTemperaturaAmbiente(); // implementação específica
    int   lerNivelLuminosidade();   // implementação específica

    ESP32HTTPClient iotClient("https://api.exemplo-iot.com");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }

        iotClient.bearerAuth("TOKEN_SECRETO_DO_DISPOSITIVO");
    }

    void loop() {
        float umidade = lerSensorUmidadeSolo();
        float temp    = lerTemperaturaAmbiente();
        int luz       = lerNivelLuminosidade();

        bool sucessoEnvio = false;

        iotClient.post("/v1/telemetry")
            .body("deviceId", "planta-sensor-01")
            .body("soilMoisture", umidade)
            .body("temperature", temp)
            .body("lightLevel", luz)
            .getBody("success", &sucessoEnvio);

        if (iotClient.isSuccess() && sucessoEnvio) {
            Serial.println("Telemetria transmitida com sucesso!");
        }

        delay(60000); // Publica a cada 60 segundos
    }
    ```

---

## 12. Sistema de Alertas IoT

Monitora condições críticas de segurança (como vazamento de gás, fumaça ou intrusão) e dispara imediatamente uma notificação de emergência via webhook POST.

**API:** Webhook / Gateway de Alertas (`https://api.exemplo-alertas.com/v1/notify`)

??? example "Ver Código"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "SUA_REDE_WIFI";
    const char* password = "SUA_SENHA_WIFI";

    // --- Assinaturas para lógica de hardware / específica ---
    bool checarCondicaoEmergencia(); // implementação específica
    void acionarBuzzerLocal();       // implementação específica

    ESP32HTTPClient alertClient("https://api.exemplo-alertas.com");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        if (checarCondicaoEmergencia()) {
            acionarBuzzerLocal();

            char idIncidente[32] = {0};

            alertClient.post("/v1/notify")
                .header("X-Priority", "High")
                .body("source", "ESP32-Seguranca")
                .body("level", "CRITICAL")
                .body("message", "Concentracao elevada de gas detectada!")
                .getBody("incidentId", idIncidente, sizeof(idIncidente));

            if (alertClient.isSuccess()) {
                Serial.printf("Alerta registrado com sucesso! Incidente: %s\n", idIncidente);
            }
        }

        delay(2000); // Checagem frequente de segurança
    }
    ```

---

## 13. Envio de Dados com Requisição POST

Cria novos registros enviando um payload JSON estruturado via POST para um servidor REST e recebe o ID gerado na resposta.

**API:** [JSONPlaceholder](https://jsonplaceholder.typicode.com/) (`https://jsonplaceholder.typicode.com/posts`)

??? example "Ver Código"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "SUA_REDE_WIFI";
    const char* password = "SUA_SENHA_WIFI";

    // --- Assinaturas para lógica de hardware / específica ---
    void salvarRegistroCriado(int novoId, const char* titulo); // implementação específica

    ESP32HTTPClient restClient("https://jsonplaceholder.typicode.com");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }
    }

    void loop() {
        int novoId = 0;
        char titulo[64] = "Relatorio Sensor IoT ESP32";

        restClient.post("/posts")
            .body("title", title)
            .body("body", "Dispositivo online com bateria em 98%")
            .body("userId", 42)
            .getBody("id", &novoId);

        if (restClient.isSuccess()) {
            Serial.printf("Recurso criado com sucesso! Novo ID: %d\n", novoId);
            salvarRegistroCriado(novoId, titulo);
        }

        delay(60000);
    }
    ```

---

## 14. Verificador de Status de API Pública

Verifica periodicamente a disponibilidade, latência e código de status HTTP de um serviço ou API pública remota.

**API:** Public Healthcheck API (`https://httpbin.org/status/200`)

??? example "Ver Código"
    ```cpp
    #include <Arduino.h>
    #include <WiFi.h>
    #include "ESP32HTTPClient.h"

    const char* ssid     = "SUA_REDE_WIFI";
    const char* password = "SUA_SENHA_WIFI";

    // --- Assinaturas para lógica de hardware / específica ---
    void atualizarIndicadorStatus(bool online, int codigoHttp); // implementação específica

    ESP32HTTPClient healthClient("https://httpbin.org");

    void setup() {
        Serial.begin(115200);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) { delay(500); }

        healthClient.setTimeout(5000); // Timeout de 5s
    }

    void loop() {
        healthClient.get("/status/200");

        int codigo = healthClient.getStatusCode();
        bool online = healthClient.isSuccess() && (codigo == 200);

        atualizarIndicadorStatus(online, codigo);

        Serial.printf("Status do Servico: %s (HTTP %d)\n", online ? "DISPONIVEL" : "OFFLINE", codigo);

        delay(30000); // Teste a cada 30 segundos
    }
    ```
