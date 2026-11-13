# Paginação do Drive — 1.1.0

## Uso

O padrão é **50 itens por página**, contando arquivos e pastas. **L** volta e
**R** avança; L2/R2 também funcionam. Cada pressionamento troca uma página, sem
repetição ao segurar. A tela indica a página atual e a quantidade configurada.
Não mostra um total que exigiria percorrer a pasta inteira.

Em **Start → Itens por página**, esquerda/direita escolhe 10, 25, 50, 100, 200,
500 ou 1.000. **A** grava em `config/settings.json`; **B** descarta uma alteração
não salva. As demais preferências são preservadas. Depois de salvar, B retorna
à primeira página com o novo limite. O valor permanece na próxima execução.

Para escolher outro inteiro de **1 a 1.000**, feche o app e ajuste `page_size`
nesse arquivo. Sem o campo, configurações antigas usam 50. Valores inválidos
são recusados com mensagem. Aumentar o limite pode aumentar memória e latência.

O retorno de um download preserva página e arquivo. **Start → Atualizar** limpa
o cache da pasta e reinicia na primeira página. Use essa opção se o conteúdo do
Drive mudou ou o token de continuação expirou. Cancelar ou falhar ao carregar
outra página mantém a lista anterior.

A faixa Select opera na **página atual**. A pesquisa continua recursiva em toda
a subárvore, com seus limites de cinco minutos/16 MiB; não é acelerada por esta
paginação. Listas locais do microSD também mantêm o comportamento anterior.

## Implementação

O `rclone lsjson` comum percorre todas as páginas internamente. A opção
`--drive-list-chunk` muda o lote por chamada, mas não expõe um cursor nem evita
a enumeração completa. Por isso, apenas cortar a lista na interface não
resolveria a demora ao abrir uma pasta grande.

O pacote mantém `bin/rclone` oficial 1.69.1 para descoberta, pesquisa e leitura.
Adiciona `bin/rclone-page`, compilado do mesmo fonte fixado com uma extensão MIT
em `third_party/rclone-page/`. O comando específico é somente leitura:

```text
rclone-page backend drivebrowser-page remote:pasta TOKEN QUANTIDADE
```

A extensão reutiliza o backend Drive do rclone: configuração/token existentes,
resolução das pastas, codificação de nomes, atalhos, resource keys, Drives
compartilhados, pacing e repetição de falhas. Não cria um cliente OAuth paralelo.
A única alteração no laço de listagem define `pageSize`, `pageToken` e
`orderBy=name,folder` e retorna depois de **um lote**, com `NextPageToken`.
Os nomes são a chave principal para manter nomes iguais adjacentes mesmo entre
arquivos e pastas. Dentro de cada página, a interface coloca pastas primeiro.
Uma resposta `incompleteSearch` é recusada.

O limite é por lote da API. O Google pode retornar menos itens ou uma página
vazia com continuação; documentos nativos omitidos e filtros também reduzem a
quantidade visível. Nesse caso R continua quando há token. Resolver o caminho
pode exigir consultas adicionais proporcionais à profundidade. Conferências
dos nomes nas bordas podem fazer até duas buscas exatas adicionais por página,
com lote de dois resultados, interrompendo ao detectar duplicatas. Atalhos podem
exigir consultas de metadados. Não é feita uma varredura das páginas seguintes.

A interface valida nomes, limite de itens e tokens, detecta duplicatas dentro
da página e recusa tokens imediatamente repetidos. As conferências de borda
protegem contra duplicatas separadas por um limite de página, pois a navegação
e os downloads continuam identificando arquivos pelo caminho. Mudanças remotas
durante a navegação podem deslocar resultados; atualizar reinicia a consulta.

Até **32 páginas de conteúdo** ficam em memória entre as pastas visitadas.
Ao exceder isso, os itens das páginas antigas são descartados, preservando os
tokens para recarregamento. Tokens e bookmarks duram só a execução do app.
Alterar a quantidade invalida esse estado e volta à primeira página. Não há
pré-carregamento das próximas páginas nem serviço instalado no firmware.

## Build e verificação

```sh
make deps
make pager
make package
```

`scripts/build_pager.py` fixa Go **1.24.0** e rclone **1.69.1** com SHA256; módulos
seguem o `go.mod`/`go.sum` upstream, com verificação Go ativa. O helper é estático
(`CGO_ENABLED=0`) e compilado para Linux amd64 e ARM64. `make pager` executa os
testes nativos e ARM64 via QEMU. As licenças dos módulos usados são coletadas
em `licenses/pager/` no pacote.

Em ambientes cujo proxy bloqueia o armazenamento do Go, existe o recurso
opcional `--github-module-cache` no script: dois módulos grandes são obtidos
dos repositórios oficiais no GitHub e convertidos para ZIP canônico de módulo
Go. O cache só aceita o resultado se o hash **h1 coincidir exatamente com o
go.sum** do rclone fixado. TLS e verificação dos módulos não são desativados.

Os testes Go usam servidor HTTP local para verificar limite real de chamada,
continuação opaca, página vazia, erro de token, resultado incompleto, duplicatas
e parâmetros de Drive compartilhado. Os testes SDL usam uma fixture paginada
com rclone real sobre arquivos locais: navegação, última página, cache, falha,
cancelamento, gatilhos, download/retorno e preferência persistida.

Esses testes não medem latência real do Google, Wi-Fi ou cartão. O mantenedor relatou que a paginação funcionou no seu aparelho após receber o
ZIP 1.1.0. Não houve teste direto pelo agente nem benchmark independente;
outros aparelhos, remotes e mapeamentos ainda precisam de validação.
Nenhuma credencial real é incluída ou necessária para os testes automatizados.
