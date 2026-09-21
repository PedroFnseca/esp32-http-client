---
title: SoapRequest Class Reference - Fluent SOAP 1.1 and 1.2 Request Builder
description: API reference for SoapRequest class: method chaining for SOAP actions, envelopes, XML body payloads, typed field bindings, fault detection, and callbacks.
keywords: SoapRequest class ESP32, SOAP client ESP32, SOAP 1.1, SOAP 1.2, SOAP Fault, XML streaming ESP32
tags:
  - api
  - soap
  - xml
  - request
  - class
---
# SoapRequest

The fluent request builder returned by `.soap(path)` on [`ESP32HTTPClient`](esp32httpclient.md). All builder methods return `SoapRequest&`, enabling fluent chaining.

**The SOAP request is dispatched automatically** when the `SoapRequest` object goes out of scope (at the end of the statement) or when `.execute()` is called explicitly.

!!! note "Copy semantics"
    `SoapRequest` is **move-only** — it cannot be copied. It is designed to be used in a single chained expression.

---

## SOAP Versions

`ESP32HTTPClient` supports both **SOAP 1.1** and **SOAP 1.2**. By default, requests use SOAP 1.1 unless configured otherwise.

```cpp
// Explicit per-request version:
client.soap("/ws").version(SOAP_1_1);
client.soap("/ws").version(SOAP_1_2);

// Or set default on client instance:
client.setSoapVersion(SOAP_1_2);
```

| SOAP Version | Content-Type | Action Mechanism | Fault Elements |
| :--- | :--- | :--- | :--- |
| **SOAP 1.1** | `text/xml; charset=utf-8` | `SOAPAction: "actionUri"` header | `<faultcode>`, `<faultstring>`, `<faultactor>`, `<detail>` |
| **SOAP 1.2** | `application/soap+xml; charset=utf-8; action="actionUri"` | Parameter in `Content-Type` header | `<Code><Value>`, `<Reason><Text>`, `<Node>`, `<Role>`, `<Detail>` |

---

## Building the Request

### `version(soapVersion)` / `setVersion(soapVersion)`

Sets the SOAP protocol version (`SOAP_1_1` or `SOAP_1_2`) for this request. Chainable.

```cpp
SoapRequest& version(SoapVersion version);
SoapRequest& setVersion(SoapVersion version);
```

**Example:**
```cpp
client.soap("/service").version(SOAP_1_2);
```

---

### `soapAction(action)` / `action(action)`

Sets the SOAP action URI. Chainable.
- For SOAP 1.1, this sends the `SOAPAction: "actionUri"` HTTP header.
- For SOAP 1.2, this appends `; action="actionUri"` to the `Content-Type` header.

```cpp
SoapRequest& soapAction(const char* action);
SoapRequest& action(const char* action);
```

**Example:**
```cpp
client.soap("/ws").soapAction("http://example.org/GetPrice");
```

---

### `body(xmlPayload)`

Sets the inner XML payload placed automatically inside the `<soap:Body>` (or `<soap12:Body>`) of the generated SOAP envelope. Chainable.

```cpp
SoapRequest& body(const char* xmlPayload);
SoapRequest& body(const String& xmlPayload);
```

**Example:**
```cpp
client.soap("/ws")
      .soapAction("http://example.org/GetPrice")
      .body("<m:GetPrice xmlns:m=\"http://example.org\"><m:Item>ESP32</m:Item></m:GetPrice>");
```

---

### `headerXml(headerXml)`

Sets optional custom XML placed inside the `<soap:Header>` (or `<soap12:Header>`) section of the envelope. Chainable.

```cpp
SoapRequest& headerXml(const char* headerXml);
SoapRequest& headerXml(const String& headerXml);
```

**Example:**
```cpp
client.soap("/ws")
      .headerXml("<AuthHeader><ApiKey>secret</ApiKey></AuthHeader>")
      .body("<m:GetData/>");
```

---

### `rawEnvelope(envelopeXml)`

Overrides automatic envelope generation completely and sends a custom XML envelope string. Chainable.

```cpp
SoapRequest& rawEnvelope(const char* envelopeXml);
SoapRequest& rawEnvelope(const String& envelopeXml);
```

---

### `path(key, value)` and `query(key, value)`

Replaces URL path placeholders and appends query parameters, matching `RestRequest` capabilities. Chainable.

```cpp
template <typename T> SoapRequest& path(const char* key, const T& value);
template <typename T> SoapRequest& query(const char* key, const T& value);
```

---

### `timeout(timeoutMs)`

Sets a per-request network timeout in milliseconds. Chainable.

```cpp
SoapRequest& timeout(uint16_t timeoutMs);
```

---

### `retry(maxRetry)` / `maxRetry(maxRetry)`

Sets the maximum number of automatic retries on network failure for this specific request. Chainable.

```cpp
SoapRequest& retry(int maxRetry);
SoapRequest& maxRetry(int maxRetry);
```

---

## Callbacks

### `onSuccess(callback)`

Executed if the response HTTP status code is 2xx and no SOAP Fault was detected. Chainable.

```cpp
SoapRequest& onSuccess(HttpResponseCallback cb);
```

---

### `onError(callback)`

Executed if the request encounters an HTTP error status code or SOAP Fault. Chainable.

```cpp
SoapRequest& onError(HttpErrorCallback cb);
SoapRequest& onError(HttpResponseCallback cb);
```

---

### `onResponse(callback)`

Executed when the HTTP response completes, regardless of status code. Chainable.

```cpp
SoapRequest& onResponse(HttpResponseCallback cb);
```

---

### `onFault(callback)`

Executed specifically when a SOAP Fault (`<Fault>`) is detected in the XML response. Chainable.

```cpp
SoapRequest& onFault(SoapFaultCallback cb);
```

**Example:**
```cpp
client.soap("/ws")
      .body("<m:DoAction/>")
      .onFault([](const SoapFault& fault) {
          Serial.printf("Fault [%s]: %s\n", fault.faultCode.c_str(), fault.faultString.c_str());
          Serial.printf("Detail: %s\n", fault.detail.c_str());
      });
```

---

## Extracting the Response

### `getBody(xmlPath, target)`

Extracts the text or primitive value of an XML element from the response stream. Matches element names by local tag name or dotted hierarchy path. Chainable.

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

**Supported target types:**

| Target Type | Description |
| :--- | :--- |
| `int*` | Parsed integer value |
| `long*` | Parsed long integer value |
| `float*` | Parsed floating-point value |
| `double*` | Parsed double-precision value |
| `bool*` | Parsed boolean (`true` / `1`) |
| `char*`, `char[N]` | String copied to buffer with null-termination |
| `String*` | Complete text or CDATA content |

**Path Matching Examples:**
- `"Price"`: matches any leaf element with local name `<Price>` or `<m:Price>`.
- `"GetPriceResponse.Price"`: matches `<Price>` under `<GetPriceResponse>`.
- `"Body.GetPriceResponse.Price"`: matches full hierarchical path under Body.

---

### `getFault(faultTarget)`

Binds a `SoapFault` struct to capture extracted fault fields (`faultCode`, `faultString`, `faultActor`, `detail`, `matched`).

```cpp
SoapRequest& getFault(SoapFault* target);
```

**Example:**
```cpp
SoapFault fault;
client.soap("/ws").body("<Request/>").getFault(&fault);

if (fault.matched) {
    Serial.printf("SOAP Fault: %s\n", fault.faultString.c_str());
}
```

---

### `getRawResponse(rawXmlTarget)`

Captures the full unparsed XML response string into an Arduino `String`.

```cpp
SoapRequest& getRawResponse(String* target);
```

---

### `getHeader(name, target)`

Extracts HTTP response headers (case-insensitive lookup).

```cpp
SoapRequest& getHeader(const char* name, String* target);
SoapRequest& getHeader(const char* name, char* target, size_t maxLength);
template <size_t N> SoapRequest& getHeader(const char* name, char (&target)[N]);
SoapRequest& getHeader(const char* name, int* target);
SoapRequest& getHeader(const char* name, long* target);
SoapRequest& getHeader(const char* name, float* target);
SoapRequest& getHeader(const char* name, double* target);
SoapRequest& getHeader(const char* name, bool* target);
```
