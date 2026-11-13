# Histórico / Changelog

## 1.0.0 — 2026-11-13

Primeira versão pública consolidada; reúne o comportamento desenvolvido e
relatado como funcional no TrimUI Smart Pro original / CrossMix OS 1.3.0.

- Navegação do Google Drive via rclone e configuração autenticada existente.
- Pesquisa recursiva por nome com teclado QWERTY, acentos e caminhos completos.
- Downloads individuais, escolha de destino, progresso, velocidade média e cancelamento.
- Conteúdo local atualizado e cursor restaurado ao arquivo baixado no Drive.
- Direcional/analógico esquerdo com repetição ao segurar e controle de drift.
- Faixa Select com Início, 0–9, A–Z e outros caracteres.
- Leitura remota, proteção de destinos, não sobrescrita e tratamento de parciais.
- Ícone D na nuvem em PNG RGBA **300 × 300**, incluído no pacote completo.
- README em português/inglês, guia de configuração, ajuda, atualização, desenvolvimento,
  licenças, testes e notas de compatibilidade.
- Automação de build/testes e auditoria do ZIP, sem publicação automática.

A promoção para 1.0 não adiciona novos recursos de transferência aos da 0.4.

### English

First consolidated public release: controller-based Google Drive browsing,
QWERTY filename search, individual downloads with destination selection/progress/
cancellation, restored cursor, left-stick/held navigation, initial-letter strip,
read-only remote operations and confined local writes. Includes the approved
300px icon, documentation, tests, license notices and verified installation ZIP.
Transfer behavior is unchanged from 0.4.

## Desenvolvimento anterior

- **0.4:** restauração de seleção, faixa de letras, Início/0–9 e teclado QWERTY.
- **0.3:** repetição de navegação e analógico esquerdo.
- **0.2:** conteúdo local após download, pesquisa recursiva e buffer configurável.
- **0.1:** implementação inicial SDL2/rclone e pacote ARM64 experimental.

Os detalhes e limites dos testes estão em [docs/VALIDATION.md](docs/VALIDATION.md).
