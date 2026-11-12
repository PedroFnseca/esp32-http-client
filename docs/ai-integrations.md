---
title: ESP32 AI Integrations with HTTP Client
description: Learn how to connect ESP32 devices to cloud AI providers and self-hosted LLMs using ESP32-HTTP-Client.
keywords: ESP32 AI, ESP32 LLM, ESP32 OpenAI, ESP32 Gemini, ESP32 Claude Anthropic, ESP32 DeepSeek, ESP32 Ollama, ESP32 vLLM, ESP32 LM Studio, embedded AI HTTP
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

# ESP32 AI Integrations with HTTP Client

Integrate ESP32 microcontrollers with cloud AI providers and self-hosted LLMs using `ESP32HTTPClient`.

With `ESP32HTTPClient`, connecting your ESP32 to Large Language Models is as straightforward as any standard REST call. The library handles the HTTP transport and extracts response fields directly into your variables using **dot notation** — no heavy vendor SDKs, no manual string parsing, and no memory-hogging DOM trees.

---

## Cloud AI Providers

Connect directly to popular cloud AI APIs. Configure your API key, build the JSON payload with `.body()`, and extract the generated response using `.getBody()`.

---

## 1. OpenAI

Queries OpenAI's **Responses API** to generate text using modern GPT models (e.g., `gpt-6-luna`, `gpt-6.1-sol`, or `gpt-6-astra`).

**Endpoint:** `POST https://api.openai.com/v1/responses`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* apiKey   = "YOUR_OPENAI_API_KEY";

ESP32HTTPClient openaiClient("https://api.openai.com");

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }

    openaiClient.bearer(apiKey);
    openaiClient.setTimeout(30000); // 30s timeout for LLM generation
}

void loop() {
    char answer[512] = {0};
    int tokens = 0;

    openaiClient.post("/v1/responses")
        .body("model", "gpt-6-luna")
        .body("input", "In 20 words or less, explain what an ESP32 is.")
        .getBody("output.0.content.0.text", answer, sizeof(answer))
        .getBody("usage.total_tokens", &tokens);

    if (openaiClient.isSuccess()) {
        Serial.printf("OpenAI (%d tokens):\n%s\n", tokens, answer);
    } else {
        Serial.printf("Error: %s (HTTP %d)\n",
                      openaiClient.getErrorMessage().c_str(), openaiClient.getStatusCode());
    }

    delay(60000);
}
```

---

## 2. Google Gemini

Connects to the **Gemini Interactions API** using the `x-goog-api-key` header to generate text with Gemini models (such as `gemini-3.8-flash`).

**Endpoint:** `POST https://generativelanguage.googleapis.com/v1beta/interactions`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* apiKey   = "YOUR_GEMINI_API_KEY";

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
        .body("input", "Summarize 3 essential IoT communication protocols in 25 words.")
        .getBody("steps.0.content.0.text", reply, sizeof(reply));

    if (geminiClient.isSuccess()) {
        Serial.println("Gemini Response:");
        Serial.println(reply);
    } else {
        Serial.printf("Error: %s (HTTP %d)\n",
                      geminiClient.getErrorMessage().c_str(), geminiClient.getStatusCode());
    }

    delay(60000);
}
```

---

## 3. Anthropic Claude

Interacts with the **Claude Messages API** using `x-api-key` and the `anthropic-version` header with models like `claude-haiku-5-5`.

**Endpoint:** `POST https://api.anthropic.com/v1/messages`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* apiKey   = "YOUR_ANTHROPIC_API_KEY";

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
        Serial.printf("Error: %s (HTTP %d)\n",
                      claudeClient.getErrorMessage().c_str(), claudeClient.getStatusCode());
    }

    delay(60000);
}
```

---

## 4. DeepSeek

Connects to the DeepSeek API (`deepseek-flash` or `deepseek-v4-pro`) using Bearer authentication and standard HTTP POST.

**Endpoint:** `POST https://api.deepseek.com/v1/responses`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* apiKey   = "YOUR_DEEPSEEK_API_KEY";

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
        .body("input", "Give 2 tips for low-power WiFi on ESP32 in 20 words.")
        .getBody("output.0.content.0.text", output, sizeof(output));

    if (deepseekClient.isSuccess()) {
        Serial.println("DeepSeek Response:");
        Serial.println(output);
    } else {
        Serial.printf("Error: %s (HTTP %d)\n",
                      deepseekClient.getErrorMessage().c_str(), deepseekClient.getStatusCode());
    }

    delay(60000);
}
```

---

## Self-Hosted AI

Run models locally on your private network (LAN) without cloud API costs or data privacy concerns. Plain HTTP requests eliminate TLS overhead and save ~45 KB of heap on the ESP32.

---

## 5. Ollama

Queries a local [Ollama](https://ollama.com/) instance on your network (port `11434`) using its native `/api/generate` endpoint. Pass `"stream", false` to receive a single complete response.

**Endpoint:** `POST http://<HOST_IP>:11434/api/generate`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Set your local host's LAN IP and port 11434
ESP32HTTPClient ollamaClient("http://192.168.1.150", 11434);

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }

    ollamaClient.setTimeout(45000); // Local inference can take a few seconds
}

void loop() {
    char answer[512] = {0};
    int tokens = 0;

    ollamaClient.post("/api/generate")
        .body("model", "gemma4")
        .body("prompt", "What is the boiling point of water in Celsius?")
        .body("stream", false)
        .getBody("response", answer, sizeof(answer))
        .getBody("eval_count", &tokens);

    if (ollamaClient.isSuccess()) {
        Serial.printf("Ollama (%d tokens):\n%s\n", tokens, answer);
    } else {
        Serial.printf("Error: %s (HTTP %d)\n",
                      ollamaClient.getErrorMessage().c_str(), ollamaClient.getStatusCode());
    }

    delay(30000);
}
```

!!! tip "Ollama Host Binding"
    By default, Ollama listens on `127.0.0.1`. To allow connections from your ESP32, set `OLLAMA_HOST=0.0.0.0:11434` on your computer before starting Ollama.

---

## 6. vLLM

Sends requests to a self-hosted [vLLM](https://docs.vllm.ai/) inference server using its OpenAI-compatible endpoint.

**Endpoint:** `POST http://<HOST_IP>:8000/v1/chat/completions`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

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
        Serial.printf("Error: %s (HTTP %d)\n",
                      vllmClient.getErrorMessage().c_str(), vllmClient.getStatusCode());
    }

    delay(30000);
}
```

---

## 7. LM Studio

Queries a local model running in [LM Studio](https://lmstudio.ai/) via its local server (default port `1234`).

**Endpoint:** `POST http://<HOST_IP>:1234/v1/chat/completions`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

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
        Serial.println("LM Studio Response:");
        Serial.println(message);
    } else {
        Serial.printf("Error: %s (HTTP %d)\n",
                      lmStudioClient.getErrorMessage().c_str(), lmStudioClient.getStatusCode());
    }

    delay(30000);
}
```

---

## 8. Custom AI Backend / Proxy

In production deployments, using an intermediate backend or edge proxy (e.g. FastAPI, Node.js, or Cloudflare Workers) keeps your cloud API keys safe, prevents key extraction from device flash, and lets you return a clean, compact payload tailored for the ESP32.

**Endpoint:** `POST https://api.example-ai.com/v1/analyze`

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include "ESP32HTTPClient.h"

const char* ssid        = "YOUR_WIFI_SSID";
const char* password    = "YOUR_WIFI_PASSWORD";
const char* deviceToken = "DEVICE_SECRET_TOKEN";

// --- Signatures for specific / hardware logic ---
float readTemperature(); // specific implementation
float readHumidity();    // specific implementation

ESP32HTTPClient aiClient("https://api.example-ai.com");

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
        Serial.printf("Alert Level: %d\nInsight: %s\n", alertLevel, insight);
    } else {
        Serial.printf("Error: %s (HTTP %d)\n",
                      aiClient.getErrorMessage().c_str(), aiClient.getStatusCode());
    }

    delay(60000);
}
```

---

## Best Practices & Embedded Tips

- **Direct Extraction with Fixed Buffers:** Always prefer `getBody("path", buffer, sizeof(buffer))` with standard `char[]` arrays. It streams tokens directly from the socket into your buffer without loading the complete JSON payload into heap memory.
- **Set Realistic Timeouts:** LLM inference can take 2 to 20 seconds. Use `client.setTimeout(30000)` or `client.setTimeout(45000)` to prevent the ESP32 from timing out prematurely.
- **Handling Rate Limits (HTTP 429):** If your API quota is exceeded, read the `Retry-After` response header using `.getHeader("Retry-After", &retrySec)` to delay before retrying.
- **TLS & Certificates:** For mission-critical production environments where strict certificate pinning or fail-closed TLS is required, see the [TLS / HTTPS Security & Certificates](guide/advanced.md#tls-https-security-certificates) section in the Advanced Guide.
- **Freeing Sockets:** Call `client.end()` when you are done making requests or before entering deep sleep to immediately close the connection.

---

## Related Documentation

- [Practical Examples](practical-examples.md) — IoT use cases (weather, GitHub, telemetry, REST).
- [API Reference: ESP32HTTPClient](api/esp32httpclient.md) — Core client constructor, timeout, headers, and authentication.
- [API Reference: RestRequest](api/restrequest.md) — Fluent request builder, `.body()`, and dot-notation `.getBody()` bindings.
- [Advanced Usage Guide](guide/advanced.md) — In-depth look at TLS, custom certificate bundles, and observability.
