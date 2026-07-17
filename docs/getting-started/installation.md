---
tags:
  - getting-started
  - install
  - setup
---
# Installation

## Requirements

Before installing, make sure your environment meets the following requirements:

| Requirement | Details |
| :--- | :--- |
| **Board** | ESP32 (any variant: ESP32-S2, S3, C3, etc.) |
| **Arduino IDE** | 1.8.x or 2.x |
| **ESP32 Arduino Core** | v2.x or later |
| **Dependencies** | None — no external libraries required |

---

## Method 1: Arduino Library Manager (Recommended)

1. Open the **Arduino IDE**.
2. Go to **Sketch → Include Library → Manage Libraries...**.
3. Search for `ESP32-HTTP-Client`.
4. Click **Install** on the result by `Pedro Fonseca`.

---

## Method 2: Manual Installation

1. Download the [latest release ZIP](https://github.com/PedroFnseca/esp32-http-client/releases/latest) from GitHub.
2. In the Arduino IDE, go to **Sketch → Include Library → Add .ZIP Library...**.
3. Select the downloaded ZIP file.

Alternatively, extract it directly into your `Arduino/libraries/` folder:

```
Arduino/
└── libraries/
    └── ESP32-HTTP-Client/
        ├── src/
        │   ├── ESP32HTTPClient.h
        │   ├── ESP32HTTPClient.cpp
        │   ├── RestRequest.h
        │   ├── RestRequest.cpp
        │   ├── RestTypes.h
        │   └── BufferedStreamReader.h
        ├── examples/
        ├── library.properties
        └── ...
```

---

## Verify Installation

After installing, verify by loading one of the built-in examples:

**File → Examples → ESP32-HTTP-Client → SimpleGET**

Upload it to your ESP32 (with your WiFi credentials filled in) and open the Serial Monitor at `115200` baud.

---

## Next Step

→ [Quick Start guide](quickstart.md)
