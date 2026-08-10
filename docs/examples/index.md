---
title: ESP32 HTTP Client Code Examples & Arduino Sketches
description: Collection of complete C++ code examples for ESP32-HTTP-Client: CRUD operations, struct serialization, JSON arrays, Auth headers, and error handling.
keywords: ESP32 HTTP client code examples, Arduino ESP32 HTTP sketch, ESP32 REST API example C++, PlatformIO ESP32 sample code
tags:
  - example
  - overview
---
# Examples

All examples are available in the [`examples/`](https://github.com/PedroFnseca/esp32-http-client/tree/main/examples) directory and can be opened directly from the Arduino IDE via **File → Examples → ESP32-HTTP-Client**.

| Example | Description |
| :--- | :--- |
| [CRUD Operations](crud-operations.md) | Full suite of REST methods (`GET`, `POST`, `PUT`, `PATCH`, `DELETE`) in a single sketch. |
| [Struct <-> JSON](struct-json.md) | Bi-directional C++ `struct` to JSON serialization and deserialization. |
| [URL Parameters](url-parameters.md) | Dynamic path parameter replacement (`/users/{id}`) and query parameters. |
| [Authentication](auth-helpers.md) | Authenticating requests using Bearer / JWT, HTTP Basic Auth, and API Key headers. |
| [Callbacks & Errors](callbacks-and-errors.md) | Request callbacks, timeouts, automatic retries, error handling, and runtime URL change. |
| [Nested JSON](nested-json.md) | Extracting deeply nested fields using dot-notation paths. |
| [Array JSON](array-json.md) | Addressing specific elements of a JSON array by index. |
| [Raw JSON](raw-json.md) | Capturing entire objects or sub-arrays into an Arduino `String`. |
| [Unix Timestamp](unix-timestamp.md) | Fetching a `long` Unix timestamp from a time API. |
| [Custom Port](port-selection.md) | Connecting to a server running on a non-standard port. |
