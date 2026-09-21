---
title: Referência da Classe SoapRequest - Fluent Builder para SOAP 1.1 e 1.2 em C++
description: Referência de API da classe SoapRequest: encadeamento de métodos para ações SOAP, envelopes, payloads XML, extração tipada de dados, detecção de falhas e callbacks.
keywords: SoapRequest ESP32, cliente SOAP ESP32, SOAP 1.1, SOAP 1.2, SOAP Fault, streaming XML ESP32
tags:
  - api
  - soap
  - xml
  - request
  - class
---
# SoapRequest

O builder fluente de requisições retornado por `.soap(path)` no [`ESP32HTTPClient`](esp32httpclient.pt.md). Todos os métodos do builder retornam `SoapRequest&`, permitindo encadeamento fluente.

**A requisição SOAP é disparada automaticamente** quando o objeto `SoapRequest` sai de escopo (ao final da instrução) ou quando `.execute()` é chamado explicitamente.

!!! note "Semântica de cópia"
    `SoapRequest` é **move-only** — não pode ser copiado. Ele foi projetado para uso em uma única instrução encadeada.

---

## Versões do SOAP

O `ESP32HTTPClient` suporta **SOAP 1.1** e **SOAP 1.2**. Por padrão, as requisições usam SOAP 1.1 a menos que configurado o contrário.

```cpp
// Versão explícita por requisição:
client.soap("/ws").version(SOAP_1_1);
client.soap("/ws").version(SOAP_1_2);

// Ou definir padrão na instância do cliente:
client.setSoapVersion(SOAP_1_2);
```

| Versão SOAP | Content-Type | Mecanismo de Ação | Elementos de Falha |
| :--- | :--- | :--- | :--- |
| **SOAP 1.1** | `text/xml; charset=utf-8` | Cabeçalho `SOAPAction: "actionUri"` | `<faultcode>`, `<faultstring>`, `<faultactor>`, `<detail>` |
| **SOAP 1.2** | `application/soap+xml; charset=utf-8; action="actionUri"` | Parâmetro no cabeçalho `Content-Type` | `<Code><Value>`, `<Reason><Text>`, `<Node>`, `<Role>`, `<Detail>` |

---

## Construindo a Requisição

### `version(soapVersion)` / `setVersion(soapVersion)`

Define a versão do protocolo SOAP (`SOAP_1_1` ou `SOAP_1_2`) para esta requisição. Encadeável.

```cpp
SoapRequest& version(SoapVersion version);
SoapRequest& setVersion(SoapVersion version);
```

---

### `soapAction(action)` / `action(action)`

Define o URI da ação SOAP. Encadeável.
- Para SOAP 1.1, envia o cabeçalho HTTP `SOAPAction: "actionUri"`.
- Para SOAP 1.2, adiciona `; action="actionUri"` ao cabeçalho `Content-Type`.

```cpp
SoapRequest& soapAction(const char* action);
SoapRequest& action(const char* action);
```

---

### `body(xmlPayload)`

Define o payload XML colocado automaticamente dentro do `<soap:Body>` (ou `<soap12:Body>`) do envelope gerado. Encadeável.

```cpp
SoapRequest& body(const char* xmlPayload);
SoapRequest& body(const String& xmlPayload);
```

---

### `headerXml(headerXml)`

Define XML customizado opcional colocado na seção `<soap:Header>` (ou `<soap12:Header>`) do envelope. Encadeável.

```cpp
SoapRequest& headerXml(const char* headerXml);
SoapRequest& headerXml(const String& headerXml);
```

---

### `rawEnvelope(envelopeXml)`

Sobrescreve a geração automática do envelope e envia uma string XML customizada completa. Encadeável.

```cpp
SoapRequest& rawEnvelope(const char* envelopeXml);
SoapRequest& rawEnvelope(const String& envelopeXml);
```

---

### `path(key, value)` e `query(key, value)`

Substitui placeholders na URL e adiciona parâmetros de busca. Encadeável.

```cpp
template <typename T> SoapRequest& path(const char* key, const T& value);
template <typename T> SoapRequest& query(const char* key, const T& value);
```

---

### `timeout(timeoutMs)` e `retry(maxRetry)`

Configura timeout e retentativas automáticas por requisição.

```cpp
SoapRequest& timeout(uint16_t timeoutMs);
SoapRequest& retry(int maxRetry);
SoapRequest& maxRetry(int maxRetry);
```

---

## Callbacks

- `onSuccess(callback)`: executado em código 2xx sem falhas SOAP.
- `onError(callback)`: executado em falha HTTP ou SOAP Fault.
- `onResponse(callback)`: executado ao término da requisição.
- `onFault(callback)`: executado especificamente ao detectar um SOAP Fault (`<Fault>`).

```cpp
client.soap("/ws")
      .body("<m:DoAction/>")
      .onFault([](const SoapFault& fault) {
          Serial.printf("Fault [%s]: %s\n", fault.faultCode.c_str(), fault.faultString.c_str());
          Serial.printf("Detail: %s\n", fault.detail.c_str());
      });
```

---

## Extraindo a Resposta

### `getBody(xmlPath, target)`

Extrai o texto ou valor primitivo de um elemento XML do fluxo de resposta em streaming.

```cpp
SoapRequest& getBody(const char* xmlPath, int* target);
SoapRequest& getBody(const char* xmlPath, long* target);
SoapRequest& getBody(const char* xmlPath, float* target);
SoapRequest& getBody(const char* xmlPath, double* target);
SoapRequest& getBody(const char* xmlPath, bool* target);
SoapRequest& getBody(const char* xmlPath, char* target, size_t maxLength);
template <size_t N> SoapRequest& getBody(const char* xmlPath, char (&target)[N]);
SoapRequest& getBody(const char* xmlPath, String* target);
```

### `getFault(faultTarget)`

Vincula uma struct `SoapFault` para capturar os dados da falha (`faultCode`, `faultString`, `faultActor`, `detail`, `matched`).

### `getRawResponse(rawXmlTarget)`

Captura a resposta XML completa em uma `String`.

### `getHeader(name, target)`

Extrai cabeçalhos da resposta HTTP.
