---
title: Integrações de IA com ESP32 e HTTP Client
description: Aprenda a conectar dispositivos ESP32 a provedores de IA em nuvem e LLMs locais auto-hospedados usando o ESP32-HTTP-Client.
keywords: ESP32 IA, ESP32 LLM, ESP32 OpenAI, ESP32 Gemini, ESP32 Claude Anthropic, ESP32 DeepSeek, ESP32 Ollama, ESP32 vLLM, ESP32 LM Studio, IA embarcada HTTP
tags:
  - ai
  - llm
  - openai
  - gemini
  - anthropic
  - deepseek
  - ollama
  - vllm
  - lmstudio
  - rest
---

# Integrações de IA com ESP32 e HTTP Client

Integre microcontroladores ESP32 com provedores de IA em nuvem e LLMs auto-hospedados utilizando o `ESP32HTTPClient`.

Com o `ESP32HTTPClient`, conectar o ESP32 a Modelos de Linguagem (LLMs) é tão simples e direto quanto consumir qualquer API REST convencional. A biblioteca gerencia o transporte HTTP e extrai os campos da resposta diretamente para as suas variáveis usando **notação de ponto** — sem SDKs proprietários pesados, sem manipulação manual de strings e sem árvores DOM consumindo a memória RAM.

!!! tip "Deseja expor o hardware do ESP32 como servidor de ferramentas para IA?"
    Enquanto o **ESP32-HTTP-Client** é focado em requisições de **saída** (ESP32 &rarr; LLMs na Nuvem/Locais), conheça nossa biblioteca parceira [**ESP32-MCP**](related-libraries.md#esp32-mcp) caso queira um servidor **MCP (Model Context Protocol)** que permita a assistentes como **Claude Desktop** e **Cursor** descobrirem e executarem comandos e leituras de sensores diretamente no seu microcontrolador.

---

## Provedores de IA em Nuvem

Conecte-se diretamente às principais APIs de IA em nuvem. Configure sua chave de API, monte o corpo da requisição com `.body()` e extraia o texto gerado via `.getBody()`.

---

## 1. OpenAI

Consulta a **Responses API** da OpenAI para geração de texto com modelos GPT modernos (ex.: `gpt-6-luna`, `gpt-6.1-sol` ou `gpt-6-astra`).

**Endpoint:** `POST https://api.openai.com/v1/responses`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "SEU_WIFI_SSID";
const char* password = "SUA_SENHA_WIFI";
const char* apiKey   = "SUA_CHAVE_OPENAI";

ESP32HTTPClient openaiClient("https://api.openai.com");

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }

    openaiClient.bearer(apiKey);
    openaiClient.setTimeout(30000); // 30s de timeout para inferência
}

void loop() {
    char answer[512] = {0};
    int tokens = 0;

    openaiClient.post("/v1/responses")
        .body("model", "gpt-6-luna")
        .body("input", "Explique em ate 20 palavras o que e um ESP32.")
        .getBody("output.0.content.0.text", answer, sizeof(answer))
        .getBody("usage.total_tokens", &tokens);

    if (openaiClient.isSuccess()) {
        Serial.printf("OpenAI (%d tokens):\n%s\n", tokens, answer);
    } else {
        Serial.printf("Erro: %s (HTTP %d)\n",
                      openaiClient.getErrorMessage().c_str(), openaiClient.getStatusCode());
    }

    delay(60000);
}
```

---

## 2. Google Gemini

Conecta-se à **Interactions API** do Gemini utilizando o cabeçalho `x-goog-api-key` para gerar texto com modelos Gemini (como `gemini-3.8-flash`).

**Endpoint:** `POST https://generativelanguage.googleapis.com/v1beta/interactions`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "SEU_WIFI_SSID";
const char* password = "SUA_SENHA_WIFI";
const char* apiKey   = "SUA_CHAVE_GEMINI";

ESP32HTTPClient geminiClient("https://generativelanguage.googleapis.com");

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }

    geminiClient.setHeader("x-goog-api-key", apiKey);
    geminiClient.setTimeout(30000);
}

void loop() {
    char reply[512] = {0};

    geminiClient.post("/v1beta/interactions")
        .body("model", "gemini-3.8-flash")
        .body("input", "Resuma 3 protocolos de comunicacao IoT em 25 palavras.")
        .getBody("steps.0.content.0.text", reply, sizeof(reply));

    if (geminiClient.isSuccess()) {
        Serial.println("Resposta Gemini:");
        Serial.println(reply);
    } else {
        Serial.printf("Erro: %s (HTTP %d)\n",
                          geminiClient.getErrorMessage().c_str(), geminiClient.getStatusCode());
    }

    delay(60000);
}
```

---

## 3. Anthropic Claude

Interage com a **Claude Messages API** utilizando `x-api-key` e a versão de protocolo `anthropic-version` com modelos como `claude-haiku-5-5`.

**Endpoint:** `POST https://api.anthropic.com/v1/messages`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "SEU_WIFI_SSID";
const char* password = "SUA_SENHA_WIFI";
const char* apiKey   = "SUA_CHAVE_ANTHROPIC";

ESP32HTTPClient claudeClient("https://api.anthropic.com");

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }

    claudeClient.setHeader("x-api-key", apiKey);
    claudeClient.setHeader("anthropic-version", "2026-10-21");
    claudeClient.setTimeout(30000);
}

void loop() {
    char response[512] = {0};
    char stopReason[32] = {0};

    claudeClient.post("/v1/messages")
        .body("model", "claude-haiku-5-5")
        .body("max_tokens", 100)
        .getBody("content.0.text", response, sizeof(response))
        .getBody("stop_reason", stopReason, sizeof(stopReason));

    if (claudeClient.isSuccess()) {
        Serial.printf("Claude (stop: %s):\n%s\n", stopReason, response);
    } else {
        Serial.printf("Erro: %s (HTTP %d)\n",
                          claudeClient.getErrorMessage().c_str(), claudeClient.getStatusCode());
    }

    delay(60000);
}
```

---

## 4. DeepSeek

Conecta-se à API do DeepSeek (`deepseek-flash` ou `deepseek-v4-pro`) usando autenticação Bearer e requisições HTTP POST padronizadas.

**Endpoint:** `POST https://api.deepseek.com/v1/responses`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "SEU_WIFI_SSID";
const char* password = "SUA_SENHA_WIFI";
const char* apiKey   = "SUA_CHAVE_DEEPSEEK";

ESP32HTTPClient deepseekClient("https://api.deepseek.com");

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }

    deepseekClient.bearer(apiKey);
    deepseekClient.setTimeout(35000);
}

void loop() {
    char output[512] = {0};

    deepseekClient.post("/v1/responses")
        .body("model", "deepseek-flash")
        .body("input", "De 2 dicas para economizar energia no WiFi do ESP32.")
        .getBody("output.0.content.0.text", output, sizeof(output));

    if (deepseekClient.isSuccess()) {
        Serial.println("Resposta DeepSeek:");
        Serial.println(output);
    } else {
        Serial.printf("Erro: %s (HTTP %d)\n",
                          deepseekClient.getErrorMessage().c_str(), deepseekClient.getStatusCode());
    }

    delay(60000);
}
```

---

## Modelos Auto-Hospedados (Self-Hosted)

Execute modelos em hardware próprio na sua rede local (LAN), sem custos por token ou preocupações de privacidade. Conexões HTTP puro eliminam o processamento TLS e economizam ~45 KB de heap no ESP32.

---

## 5. Ollama

Consulta uma instância local do [Ollama](https://ollama.com/) na rede (porta padrão `11434`) através do endpoint nativo `/api/generate`. O parâmetro `"stream", false` garante o recebimento de uma resposta completa em objeto único.

**Endpoint:** `POST http://<HOST_IP>:11434/api/generate`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "SEU_WIFI_SSID";
const char* password = "SUA_SENHA_WIFI";

// Configure o IP do host na rede local e a porta 11434
ESP32HTTPClient ollamaClient("http://192.168.1.150", 11434);

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }

    ollamaClient.setTimeout(45000);
}

void loop() {
    char answer[512] = {0};
    int tokens = 0;

    ollamaClient.post("/api/generate")
        .body("model", "gemma4")
        .body("prompt", "Qual e o ponto de ebulicao da agua em Celsius?")
        .body("stream", false)
        .getBody("response", answer, sizeof(answer))
        .getBody("eval_count", &tokens);

    if (ollamaClient.isSuccess()) {
        Serial.printf("Ollama (%d tokens):\n%s\n", tokens, answer);
    } else {
        Serial.printf("Erro: %s (HTTP %d)\n",
                          ollamaClient.getErrorMessage().c_str(), ollamaClient.getStatusCode());
    }

    delay(30000);
}
```

!!! tip "Acesso em Rede no Ollama"
    Por padrão o Ollama escuta apenas em `127.0.0.1`. Para aceitar requisições de outros dispositivos da sua rede local (como o ESP32), inicie o serviço com a variável de ambiente `OLLAMA_HOST=0.0.0.0:11434`.

---

## 6. vLLM

Envia requisições para um servidor [vLLM](https://docs.vllm.ai/) auto-hospedado através do seu endpoint compatível com OpenAI.

**Endpoint:** `POST http://<HOST_IP>:8000/v1/chat/completions`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "SEU_WIFI_SSID";
const char* password = "SUA_SENHA_WIFI";

ESP32HTTPClient vllmClient("http://192.168.1.120", 8000);

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }

    vllmClient.setTimeout(30000);
}

void loop() {
    char answer[512] = {0};
    int tokens = 0;

    vllmClient.post("/v1/chat/completions")
        .body("model", "meta-llama/Llama-3.1-8B-Instruct")
        .body("temperature", 0.3f)
        .getBody("choices.0.message.content", answer, sizeof(answer))
        .getBody("usage.total_tokens", &tokens);

    if (vllmClient.isSuccess()) {
        Serial.printf("vLLM (%d tokens):\n%s\n", tokens, answer);
    } else {
        Serial.printf("Erro: %s (HTTP %d)\n",
                          vllmClient.getErrorMessage().c_str(), vllmClient.getStatusCode());
    }

    delay(30000);
}
```

---

## 7. LM Studio

Executa inferência com modelos locais servidos pelo [LM Studio](https://lmstudio.ai/) (porta padrão `1234`).

**Endpoint:** `POST http://<HOST_IP>:1234/v1/chat/completions`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "SEU_WIFI_SSID";
const char* password = "SUA_SENHA_WIFI";

ESP32HTTPClient lmStudioClient("http://192.168.1.105", 1234);

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }

    lmStudioClient.setTimeout(30000);
}

void loop() {
    char message[512] = {0};

    lmStudioClient.post("/v1/chat/completions")
        .body("max_tokens", 80)
        .getBody("choices.0.message.content", message, sizeof(message));

    if (lmStudioClient.isSuccess()) {
        Serial.println("Resposta LM Studio:");
        Serial.println(message);
    } else {
        Serial.printf("Erro: %s (HTTP %d)\n",
                          lmStudioClient.getErrorMessage().c_str(), lmStudioClient.getStatusCode());
    }

    delay(30000);
}
```

---

## 8. Backend Customizado / Proxy de Borda

Em projetos de produção com dispositivos distribuídos, um backend intermediário ou proxy (Node.js, FastAPI, Cloudflare Workers) protege as credenciais de nuvem contra extração da memória flash e entrega um payload JSON compacto e filtrado para o ESP32.

**Endpoint:** `POST https://api.exemplo-ai.com/v1/analyze`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid        = "SEU_WIFI_SSID";
const char* password    = "SUA_SENHA_WIFI";
const char* deviceToken = "TOKEN_SECRETO_DISPOSITIVO";

// --- Assinaturas de rotinas de hardware ---
float readTemperature(); // implementação de hardware
float readHumidity();    // implementação de hardware

ESP32HTTPClient aiClient("https://api.exemplo-ai.com");

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }

    aiClient.bearer(deviceToken);
    aiClient.setTimeout(20000);
}

void loop() {
    float temp = readTemperature();
    float hum  = readHumidity();

    char insight[256] = {0};
    int alertLevel = 0;

    aiClient.post("/v1/analyze")
        .body("temperature", temp)
        .body("humidity", hum)
        .body("deviceId", "esp32-sensor-01")
        .getBody("insight", insight, sizeof(insight))
        .getBody("alertLevel", &alertLevel);

    if (aiClient.isSuccess()) {
        Serial.printf("Nivel de Alerta: %d\nInsight: %s\n", alertLevel, insight);
    } else {
        Serial.printf("Erro: %s (HTTP %d)\n",
                          aiClient.getErrorMessage().c_str(), aiClient.getStatusCode());
    }

    delay(60000);
}
```

---

## Boas Práticas e Dicas para Embarcados

- **Extração com Buffers Estáticos:** Sempre priorize `getBody("caminho", buffer, sizeof(buffer))` com vetores `char[]` normais. Isso transfere os dados do socket diretamente para a variável sem manter o documento JSON inteiro na memória heap.
- **Defina Timeouts Apropriados:** Respostas de LLMs podem levar entre 2 e 20 segundos para serem geradas. Use `client.setTimeout(30000)` ou `client.setTimeout(45000)` para evitar que o ESP32 encerre a conexão prematuramente.
- **Tratamento de Rate Limits (HTTP 429):** Se a cota por minuto for excedida, leia o cabeçalho `Retry-After` usando `.getHeader("Retry-After", &retrySec)` para aguardar o intervalo necessário antes da próxima chamada.
- **Segurança TLS em Produção:** Caso o seu projeto corporativo exija validação estrita com certificados raiz (Root CA pinning) ou fail-closed TLS, confira o guia completo em [Segurança e Certificados TLS / HTTPS](guide/advanced.pt.md#seguranca-e-certificados-tls-https).
- **Liberação de Sockets:** Chame `client.end()` quando não houver requisições ativas para fechar conexões e liberar recursos.

---

## Documentação Relacionada

- [Bibliotecas Relacionadas: ESP32-MCP](related-libraries.md): Exponha ferramentas de hardware para agentes de IA via Model Context Protocol.
- [Exemplos Práticos](practical-examples.pt.md) — Exemplos para IoT (estações meteorológicas, monitor GitHub, REST).
- [Referência da API: ESP32HTTPClient](api/esp32httpclient.pt.md) — Métodos de construtor, timeout, cabeçalhos e autenticação.
- [Referência da API: RestRequest](api/restrequest.pt.md) — Encadeamento de métodos fluentes, `.body()` e extração `.getBody()`.
- [Guia de Uso Avançado](guide/advanced.pt.md) — Detalhes sobre TLS, bundles de certificados e métricas de observabilidade.
