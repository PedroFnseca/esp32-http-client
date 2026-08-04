---
tags:
  - getting-started
  - install
  - setup
---
# Instalação

## Requisitos

Antes de instalar, certifique-se de que seu ambiente atende aos seguintes requisitos:

| Requisito | Detalhes |
| :--- | :--- |
| **Placa** | ESP32 (qualquer variante: ESP32-S2, S3, C3, etc.) |
| **IDE / Core** | Arduino IDE 1.8.x/2.x ou PlatformIO |
| **ESP32 Arduino Core** | v2.x ou posterior |
| **Dependências** | Nenhuma — nenhuma biblioteca externa é necessária |

---

## Método 1: Gerenciador de Bibliotecas do Arduino (Recomendado)

1. Abra o **Arduino IDE**.
2. Vá em **Sketch → Incluir Biblioteca → Gerenciar Bibliotecas...**.
3. Procure por `ESP32-HTTP-Client`.
4. Clique em **Instalar** no resultado por `Pedro Fonseca`.

---

## Método 2: PlatformIO

Adicione `ESP32-HTTP-Client` à seção `lib_deps` do seu arquivo de configuração `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    PedroFnseca/ESP32-HTTP-Client@^1.4.0
```

Alternativamente, instale via CLI do PlatformIO:

```bash
pio pkg install --library "PedroFnseca/ESP32-HTTP-Client"
```

---

## Método 3: Instalação Manual

1. Baixe o [arquivo ZIP da última versão](https://github.com/PedroFnseca/esp32-http-client/releases/latest) no GitHub.
2. No Arduino IDE, vá em **Sketch → Incluir Biblioteca → Adicionar Biblioteca .ZIP...**.
3. Selecione o arquivo ZIP baixado.

Alternativamente, extraia diretamente para a sua pasta `Arduino/libraries/`:

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

## Verificar a Instalação

Após instalar, verifique carregando um dos exemplos integrados:

**Arquivo → Exemplos → ESP32-HTTP-Client → RestCrud**

Envie para o seu ESP32 (com suas credenciais de WiFi preenchidas) e abra o Monitor Serial a `115200` baud.

---

## Próximo Passo

→ [Guia de Início Rápido](quickstart.pt.md)
