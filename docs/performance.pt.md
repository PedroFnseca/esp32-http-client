---
title: Desempenho e Benchmarks de Memória do Cliente HTTP ESP32
description: Comparativo de benchmark entre ESP32-HTTP-Client e HTTPClient + ArduinoJson. Alocação de RAM 99,9% menor, zero fragmentação de memória heap e 12x mais rápido.
keywords: Benchmark ESP32, desempenho ArduinoJson, otimização de memória ESP32, consumo RAM HTTP ESP32
tags:
  - performance
  - benchmark
---
# Desempenho

## Configuração do Benchmark

Todos os dados foram coletados executando **100 requisições HTTP GET consecutivas** com uma carga JSON em um dispositivo ESP32 real contra o endpoint público [`/users` do JSONPlaceholder](https://jsonplaceholder.typicode.com/users). Ambas as bibliotecas foram configuradas com as definições padrão. A conexão WiFi já estava estabelecida antes do início do teste.

---

## Resultados

| Métrica / Recurso | Padrão (HTTPClient + ArduinoJson) | ESP32-HTTP-Client | Comparação |
| :--- | :---: | :---: | :---: |
| **Alocação de heap por requisição** | ~58.2 KB | **~15 bytes** | ⬇ ~99.9% menos RAM |
| **Pegada média de RAM** | 34.2% | **24.3%** | ⬇ ~29% menos no geral |
| **Heap livre mínima** | 114.3 KB | **128.6 KB** | ⬆ Margem mais segura |
| **Tempo médio de execução** | ~750 ms | **~59 ms** | 🚀 ~12× mais rápido |
| **Verbosidade do código** | ~15 linhas de boilerplate | **1 cadeia fluente** | ⬇ Limpo & legível |
| **Análise de JSON** | `deserializeJson()` | **Automático, vinculação direta** | ⬆ Sem alocação de documento |

---

## Por que é tanto mais rápido?

### HTTP Keep-Alive

O `ESP32HTTPClient` habilita o HTTP/1.1 Keep-Alive por padrão através de `_http.setReuse(true)`. Isso permite que a **conexão TCP/TLS subjacente seja reutilizada** em múltiplas requisições para o mesmo servidor.

A abordagem padrão normalmente fecha e reabre a conexão a cada chamada, pagando o custo total do handshake TLS (~700–800 ms em uma rede WiFi típica) toda vez.

### Análise de Fluxo com Cópia Zero (Zero-Copy Stream Parsing)

Em vez de chamar `http.getString()` (que aloca a resposta inteira na memória heap) e depois passá-la para `deserializeJson()` (que aloca um `DynamicJsonDocument`), esta biblioteca:

1. Obtém um ponteiro bruto `Stream*` diretamente do `HTTPClient`.
2. Analisa o fluxo JSON **byte a byte** através do `BufferedStreamReader`.
3. Grava os valores correspondentes diretamente nas suas variáveis C pré-declaradas.

O corpo completo da resposta **nunca é armazenado na memória**. A alocação total de heap para uma resposta típica é de ~15 bytes de estado interno.

### Decodificação Chunked (Chunked Transfer Encoding)

APIs modernas frequentemente respondem com `Transfer-Encoding: chunked`. O `BufferedStreamReader` da biblioteca decodifica automaticamente respostas em chunks sem exigir que a carga útil completa seja armazenada em buffer primeiro.

---

## Compensações de Memória (Trade-offs)

| Cenário | RAM liberada por `end()` |
| :--- | :--- |
| Conexão TLS (HTTPS) ativa | ~45 KB |
| Conexão HTTP simples ativa | ~5–10 KB |

Chame `client.end()` após uma rajada de requisições se o seu sketch entrar em um longo período de inatividade:

```cpp
// Rajada de requisições
for (int i = 0; i < 10; i++) {
    client.get("/data").getBody("v", &val);
    // ... processar val
}

client.end(); // libera ~45KB de buffers TLS
delay(60 * 1000); // aguarda 1 minuto
```

---

## Código-Fonte do Benchmark

Os sketches de benchmark completos e scripts de comparação estão no diretório [`bench/`](https://github.com/PedroFnseca/esp32-http-client/tree/main/bench) do repositório.
