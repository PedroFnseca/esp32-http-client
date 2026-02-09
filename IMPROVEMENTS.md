# Melhorias recomendadas para o ESP32-HTTP-Client

## 1) Corrigir inconsistências de API pública (alta prioridade)
- `setHeader` é declarado em `ESP32HTTPClient.h`, mas não existe implementação em `ESP32HTTPClient.cpp`.
- `setContentType` define `_contentType`, porém a execução da requisição ainda usa string fixa `"application/json"`.

**Impacto:** evita comportamento inesperado e melhora a previsibilidade da API.

## 2) Tornar serialização de parâmetros mais segura
- `addParam` usa buffers de 32 bytes e cópia truncada para strings.
- Chaves/valores de query não são URL-encoded.
- JSON body não escapa caracteres especiais (`"`, `\\`, controle etc.).

**Impacto:** reduz risco de payload inválido e bugs difíceis de diagnosticar.

## 3) Melhorar robustez do parser JSON em stream
- O parser atual cobre casos simples de objeto plano, mas ainda é limitado para cenários reais (nulos, expoentes numéricos, chaves repetidas, estruturas mais profundas e arrays complexos).
- Não há estratégia clara para reportar erro de parsing para o usuário.

**Impacto:** aumenta compatibilidade com APIs reais e facilita troubleshooting.

## 4) Melhorar ergonomia e controle de execução
- O envio automático no destrutor (`RAII`) é prático, mas torna o fluxo implícito.
- Falta método explícito de execução (`send()`/`execute()` público) com retorno detalhado.

**Impacto:** facilita depuração e evita efeitos colaterais por tempo de vida do objeto.

## 5) Adicionar testes automatizados (muito recomendado)
- Cobrir serialização (`query`/`body`), parser para cada tipo suportado, e comportamento em erro HTTP.
- Criar testes de regressão para valores extremos (strings longas, floats especiais, payload vazio, respostas malformadas).

**Impacto:** maior confiabilidade para evolução da biblioteca.

## 6) Expandir observabilidade e documentação
- Expor mensagem de erro (`http.errorToString(code)`) além de status code.
- Documentar limitações atuais do parser e limites de tamanho de buffers.
- Adicionar matriz de compatibilidade (tipos suportados, exemplos de payload e casos não suportados).

**Impacto:** reduz suporte reativo e melhora experiência de adoção.

## Quick wins sugeridos
1. Implementar `setHeader` e aplicar `_contentType` real na requisição.
2. Fazer URL-encoding em query params.
3. Introduzir `send()` explícito mantendo encadeamento.
4. Adicionar suíte mínima de testes de unidade para serialização/parser.
