---
title: "ESP32 Ecosystem and Related Libraries - ESP32-MCP | ESP32-HTTP-Client"
description: "Explore high-performance, zero-allocation open-source libraries for the ESP32 and Arduino ecosystem, including ESP32-MCP (Model Context Protocol server for Claude, Cursor, and AI agents)."
keywords: "ESP32 MCP, Model Context Protocol ESP32, ESP32 AI tools, Claude Desktop ESP32, Cursor ESP32, embedded MCP server, ESP32 AI agent, Arduino MCP, zero heap ESP32 libraries, IoT Model Context Protocol, Pedro Fonseca"
tags:
  - ecosystem
  - related-libraries
  - mcp
  - ai
  - esp32
  - arduino
---

# :material-view-grid-plus-outline: Related Libraries & Ecosystem

Welcome to the **ESP32 Ecosystem** showcase. Our libraries share a unified engineering philosophy: **low memory footprint, zero-heap allocations on critical paths, fluent C++ APIs, and production-grade reliability** for microcontrollers.

Whether your ESP32 needs to consume remote cloud APIs or expose its own physical hardware directly to autonomous AI agents, these complementary libraries provide a modern embedded development experience.

---

## :material-robot-industrial: ESP32-MCP: Model Context Protocol Server {: #esp32-mcp }

**ESP32-MCP** transforms your ESP32 into a native, high-performance **Model Context Protocol (MCP)** server over HTTP. Instead of setting up complex Python or Node.js bridge gateways on a separate computer, your microcontroller speaks MCP directly across your local network.

This means AI assistants like **Claude Desktop**, **Cursor**, and autonomous LLM agents can discover, inspect, and invoke your hardware tools, read live sensors, and control GPIOs with zero intermediate server overhead.

### Simple Usage Example

Here is how straightforward it is to register a hardware tool and start the MCP server. With fluent variable binding, you never have to write JSON parsers or manual schemas:

```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <ESP32MCP.h>

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Initialize server with name and version
MCPServer server("ESP32-Controller", "1.0.0");
static bool ledState = false;

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); }

    // Register a hardware control tool with direct variable binding
    server.tool("set_led", "Turns the onboard LED ON or OFF")
          .destructive()
          .param("state", &ledState, "true for ON, false for OFF")
          .handle([](MCPResult& r) {
              digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
              r.text("LED is now %s", ledState ? "ON" : "OFF");
          });

    // Enables built-in system telemetry tool (uptime, free heap, RSSI)
    server.enableStatus();

    // Starts HTTP server and creates FreeRTOS background task on Core 1
    server.begin();
}

void loop() {
    // Your main loop remains 100% free for sensor reading and real-time tasks
    delay(1000);
}
```

---

### Why Run MCP Directly on ESP32?

The [Model Context Protocol (MCP)](https://modelcontextprotocol.io/) is an open standard created by Anthropic that standardizes how AI applications interact with external tools, contextual data, and execution environments.

Until now, connecting embedded electronics to MCP required running an intermediary bridge script on a PC. **ESP32-MCP removes the middleman completely**:

- **Real-Time Physical Control**: Give AI assistants safe, structured hands to interact with the physical world.
- **Natural Language Hardware Debugging**: Ask Claude or Cursor to inspect memory, verify Wi-Fi signal quality, or toggle test pins during firmware bring-up.
- **Agentic IoT Automation**: Allow multi-agent systems to query sensor readings on demand without continuously polling your microcontrollers.

---

### Key Architectural Advantages

#### 1. Zero-Heap Allocations on Hot Path
Embedded stability depends on deterministic memory. ESP32-MCP contains **zero heap allocations** (`malloc`, `new`, `free`) during tool invocation and discovery.
All request parsers and response builders operate over fixed-size static buffers. In stress benchmarks of 10,000 consecutive tool invocations, heap drift was measured at **exactly 0 bytes**, completely eliminating memory fragmentation crashes.

#### 2. Fluent Declarative API with Direct Variable Binding
Forget writing manual JSON schemas, type validation, or string manipulation. By binding your C++ variables directly with `.param("state", &ledState)`, the library automatically handles deserialization, bounds checking, and type conversion for `bool`, `int`, `float`, `double`, and `char*`.

#### 3. Autonomous FreeRTOS Execution (Core 1)
When you call `server.begin()`, ESP32-MCP automatically creates a dedicated FreeRTOS background task pinned to Core 1. Your primary Arduino `loop()` never blocks, allowing your high-frequency PID loops, display rendering, or sensor sampling to run uninterrupted.

#### 4. Built-in Diagnostics & Safe GPIO Access
- `server.enableStatus()`: Instantly registers `get_status` exposing uptime, free heap, chip revision, firmware version, and Wi-Fi RSSI.
- `server.enableGpio({12, 13, 14})`: Safely exposes hardware pin control restricted exclusively to your pre-approved whitelist.

#### 5. Hardened Production Security
- **Constant-Time Bearer Authentication**: Prevent side-channel timing attacks with `server.bearer("your-secret-token")`.
- **Strict CORS Policy**: All cross-origin browser requests are rejected by default unless origins are explicitly granted with `server.allowOrigin(...)`.
- **Payload Guard**: Strict request body boundaries return HTTP 413 on buffer overflow attempts.

#### 6. Full Protocol Specification Compliance
Supports the modern MCP specification (stateless HTTP transport, `server/discover`, and `MCP-Protocol-Version` headers) alongside seamless backward compatibility for earlier MCP clients (`initialize`, `notifications/initialized`, and `ping`).

---

### The Complete Embedded AI Stack: Outbound & Inbound

Combining **ESP32-HTTP-Client** and **ESP32-MCP** equips your IoT project with a complete, bidirectional AI architecture:

| Role | Library | Traffic Direction | Primary Objective |
|---|---|---|---|
| **Outbound AI Client** | [ESP32-HTTP-Client](https://esp32httpclient.com/) | ESP32 &rarr; Remote LLM | Microcontroller initiates requests to OpenAI, Claude, Gemini, DeepSeek, or Ollama, binding JSON responses directly into variables. |
| **Inbound AI Tool Server** | [ESP32-MCP](https://github.com/PedroFnseca/esp32-mcp) | AI Assistant &rarr; ESP32 Hardware | Claude Desktop, Cursor, or local agents initiate requests to discover tools, read sensor values, and actuate GPIOs. |

---

### Connecting Claude Desktop or Cursor to Your ESP32

Configure your local AI client config file (such as `claude_desktop_config.json`):

```json
{
  "mcpServers": {
    "esp32_device": {
      "command": "npx",
      "args": [
        "-y",
        "mcp-remote",
        "http://192.168.1.150/mcp"
      ]
    }
  }
}
```

Once connected, Claude or Cursor immediately discovers `set_led` and `get_status`. You can prompt naturally:

> *"Check if the ESP32 free heap is above 150 KB and turn on the onboard LED."*

The AI assistant will formulate tool calls and execute them sequentially against your microcontroller in real time.

---

### Installation

=== "PlatformIO"

    Add `ESP32-MCP` to your `platformio.ini` dependencies:

    ```ini
    [env:esp32dev]
    platform = espressif32
    board = esp32dev
    framework = arduino
    lib_deps =
        https://github.com/PedroFnseca/esp32-mcp.git
    ```

=== "Arduino IDE"

    1. In the Arduino IDE, go to **Sketch** > **Include Library** > **Manage Libraries...**
    2. Search for `ESP32-MCP` and click **Install**.
    3. Alternatively, download the repository as a ZIP from GitHub and add it via **Sketch** > **Include Library** > **Add .ZIP Library...**

---

### Project Links & Resources

<div class="grid cards" markdown>

- :material-brain: **Model Context Protocol (MCP) for ESP32**

    ---

    Explore the source code, open issues, and join the development of ESP32-MCP on GitHub.

    [:octicons-mark-github-16: GitHub Repository](https://github.com/PedroFnseca/esp32-mcp){ .md-button .md-button--primary }
    [:material-tag-outline: Latest Releases](https://github.com/PedroFnseca/esp32-mcp/releases){ .md-button }

</div>

[![GitHub Release](https://img.shields.io/github/v/release/PedroFnseca/esp32-mcp?color=00796b&label=Release&logo=github)](https://github.com/PedroFnseca/esp32-mcp/releases)
[![Stars](https://img.shields.io/github/stars/PedroFnseca/esp32-mcp?style=social)](https://github.com/PedroFnseca/esp32-mcp/stargazers)
[![License](https://img.shields.io/github/license/PedroFnseca/esp32-mcp?color=00796b)](https://github.com/PedroFnseca/esp32-mcp/blob/main/LICENSE)
[![C++14 / C++17](https://img.shields.io/badge/C%2B%2B-14%20%2F%2017-00796b)](https://github.com/PedroFnseca/esp32-mcp)

- [ESP32-MCP GitHub Repository](https://github.com/PedroFnseca/esp32-mcp)
- [Releases and Changelog](https://github.com/PedroFnseca/esp32-mcp/releases)
- [Issue Tracker & Bug Reports](https://github.com/PedroFnseca/esp32-mcp/issues)
- [Official Model Context Protocol Specification](https://modelcontextprotocol.io/)
- [Anthropic Model Context Protocol GitHub](https://github.com/modelcontextprotocol)
