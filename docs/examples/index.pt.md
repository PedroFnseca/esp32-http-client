---
tags:
  - example
  - overview
---
# Exemplos

Todos os exemplos estão disponíveis no diretório [`examples/`](https://github.com/PedroFnseca/esp32-http-client/tree/main/examples) e podem ser abertos diretamente na Arduino IDE através de **File → Examples → ESP32-HTTP-Client**.

| Exemplo | Descrição |
| :--- | :--- |
| [Operações CRUD](crud-operations.pt.md) | Conjunto completo dos métodos REST (`GET`, `POST`, `PUT`, `PATCH`, `DELETE`) em um único sketch. |
| [Mapeamento Struct <-> JSON](struct-json.pt.md) | Serialização e desserialização bidirecional entre `struct` C++ e JSON. |
| [Parâmetros de URL](url-parameters.pt.md) | Substituição de parâmetros de rota (`/users/{id}`) e parâmetros de consulta. |
| [Autenticação](auth-helpers.pt.md) | Autenticação com Bearer / JWT, HTTP Basic Auth e cabeçalhos de API Key. |
| [Callbacks e Tratamento de Erros](callbacks-and-errors.pt.md) | Callbacks, timeouts, retries automáticos, tratativas de erro e troca de URL em runtime. |
| [JSON Aninhado](nested-json.pt.md) | Extração de campos profundamente aninhados utilizando notação por ponto. |
| [JSON Array](array-json.pt.md) | Acesso direto a elementos de arrays JSON por índice. |
| [JSON Bruto](raw-json.pt.md) | Captura de objetos ou arrays inteiros diretamente em uma `String` Arduino. |
| [Timestamp Unix](unix-timestamp.pt.md) | Leitura de timestamp Unix de 64 bits (`long`) a partir de uma API de tempo. |
| [Porta Personalizada](port-selection.pt.md) | Conexão com servidores executando em portas não padronizadas. |
