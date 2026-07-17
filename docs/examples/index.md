---
tags:
  - example
  - overview
---
# Examples

All examples are available in the [`examples/`](https://github.com/PedroFnseca/esp32-http-client/tree/main/examples) directory and can be opened directly from the Arduino IDE via **File → Examples → ESP32-HTTP-Client**.

| Example | Description |
| :--- | :--- |
| [Simple GET](simple-get.md) | Basic data fetching, binding multiple fields from a JSON response. |
| [POST Request](post-request.md) | Sending a JSON payload and reading the created resource's ID. |
| [Nested JSON](nested-json.md) | Extracting deeply nested fields using dot-notation paths. |
| [Array JSON](array-json.md) | Addressing specific elements of a JSON array by index. |
| [Raw JSON](raw-json.md) | Capturing entire objects or sub-arrays into an Arduino `String`. |
| [PUT & DELETE](put-delete.md) | Updating and deleting remote resources. |
| [Unix Timestamp](unix-timestamp.md) | Fetching a `long` Unix timestamp from a time API. |
| [Custom Port](port-selection.md) | Connecting to a server running on a non-standard port. |
