# Drive Browser 1.1.0 — TrimUI Smart Pro

- Navegação do Drive em páginas de até **50 itens** por padrão, incluindo pastas.
- **L/R** para página anterior/próxima; aceita também L2/R2.
- **Start → Itens por página**: esquerda/direita ajusta, **A salva** e B descarta
  alteração não salva. Opções: 10, 25, 50, 100, 200, 500 e 1.000.
- Quantidades personalizadas de 1 a 1.000 por `page_size` em
  `config/settings.json`, com o app fechado. Configurações antigas usam 50.
- Cache de páginas visitadas e retorno ao arquivo baixado na página de origem.

Extraia `DriveBrowser-1.1.0-arm64.zip` na raiz do microSD, com o app fechado.
Preserve `config/rclone.conf`, `config/settings.json`, `data/` e downloads.
O pacote não inclui credenciais nem exige novo login para configuração válida.

A faixa de iniciais agora atua apenas na página atual do Drive. A pesquisa
continua recursiva e mantém seus limites anteriores. Atualizar pelo Start
reinicia a pasta quando o conteúdo mudou ou um token expirou. A API pode
retornar menos itens que o limite, inclusive páginas vazias com continuação.

A paginação usa uma extensão mínima compilada do rclone 1.69.1, que reaproveita
seu backend de autenticação/listagem. O rclone oficial continua incluído para
pesquisa e transferência. Download permanece sequencial, sem mudança de taxa.

Builds e testes automatizados nativos/ARM64 são descritos em
[VALIDATION.md](VALIDATION.md). **O mantenedor confirmou o funcionamento desta
versão no seu aparelho.** Não houve teste direto pelo agente nem benchmark
independente de rede/cartão. [Detalhes e configuração](PAGINATION.md).
