# Validacao e historico

## Versao 1.0.0 — consolidacao para distribuicao

O mantenedor relatou funcionamento do conjunto de recursos ate a v0.4 no modelo
alvo. A versao 1.0.0 consolida esse comportamento, a identificacao da versao,
documentacao e icone final. O canvas do icone foi ajustado para **300x300** com
a arte anterior de 256x256 centralizada e margens transparentes, conforme pedido;
o alinhamento do nome no launcher ainda precisa de verificacao visual no aparelho.
Nao houve teste direto do hardware pelo agente nem homologacao de outros modelos.

Na bateria final local, passaram:

* **65 verificacoes de backend** em cada modo: nativo x86_64, fallback de copia
  exclusiva e AArch64 emulado com rclone ARM64 real sobre arquivos locais.
* **33 verificacoes de navegacao** e **95 de interface** por arquitetura,
  incluindo teclado QWERTY, faixa Select, cancelamento, origem correta do download,
  retorno ao arquivo e preservacao de dados existentes.
* Builds Cortex-A53 e nativo com `-Werror`; smoke SDL nativo e ARM64 `offscreen`;
  verificacao de estrutura, CRC, manifesto completo SHA256, icone RGBA 300x300,
  ELF AArch64 e requisitos GLIBC <=2.23 no pacote.
* Documentacao portuguesa/inglesa e links locais verificados no checkout e nas
  duas arvores de documentacao do ZIP (instalacao e source/). O ZIP nao inclui
  rclone.conf, settings.json, dados de teste ou credenciais pessoais.

As capturas publicas sao demonstracoes de teste SDL nativo com arquivos ficticios.
A CI do GitHub foi configurada, mas sua execucao remota nao foi verificada neste
registro. Preparar artefatos/commit/tag locais nao confirma publicacao no GitHub;
o estado publico deve ser consultado em Releases depois da operacao autorizada.

## Versao 0.4.0

O usuario confirmou funcionamento geral ate a v0.3. Esta revisao acrescenta
restauracao de selecao, faixa de letras com Select/Inicio/0-9 e teclado QWERTY.
**As novidades da v0.4 ainda exigem teste no console fisico.**

Resultados no ambiente de desenvolvimento:

* **65 verificacoes de backend** passaram em x86_64 e AArch64 emulado com rclone
  real sobre fixtures locais. As cinco verificacoes novas cobrem agrupamento
  de iniciais com acentos, acentos decompostos, numeros e outros caracteres.
* **95 verificacoes de interface por arquitetura** passaram. Incluem voltar com
  B ao arquivo baixado numa pagina posterior, mesmo quando o cursor foi movido
  apos a marcacao; resultado correto entre nomes iguais em subpastas diferentes;
  retorno do menu; selecao pelo caminho apos reordenacao; fallback quando o
  arquivo foi removido; retorno a pasta filha no pai; Select e salto por inicial
  no Drive, na pesquisa e no destino local; prioridade de arquivos sobre pastas;
  Inicio, 0-9, inicial sem itens, lista vazia, confirmacao com A, saida com B e
  bloqueio de download com X durante escolha de letras; tres linhas QWERTY e
  digitacao/apagamento com acentos. O conteudo dos dois downloads e conferido
  pelo script, preservando o arquivo preexistente e sem deixar novos parciais.
* Navegacao continua tem **33 verificacoes** nativas/ARM64, alem dos testes
  integrados acima. SDL nativo e ARM64 `offscreen`, builds com `-Werror` e
  auditoria do ZIP continuam disponiveis nos mesmos alvos do Makefile.
* Capturas da faixa e QWERTY foram inspecionadas visualmente em 1280x720.
  `scripts/ui_test.py` gera `data/ui-letters.bmp`, `data/ui-qwerty.bmp` e
  `data/ui-download.bmp` dentro da fixture temporaria de cada execucao.

No TrimUI, conferir Select fisico (default SDL BACK=4), troca entre letras e
arquivos, saltos com toque/analogico/segurar, Inicio e 0-9, legibilidade do rodape,
QWERTY e retorno com B depois de baixar um arquivo distante do inicio da pasta.
O ambiente publicado nao foi republicado por esta tarefa; o ZIP e um artefato
local para o usuario instalar. Configuracoes privadas nao sao incluidas.

## Historico: versao 0.3.0

Novidade: repetir navegacao ao segurar o direcional e usar o stick esquerdo nas
mesmas telas. O backend, parametros de transferencia e configuracao pessoal nao
foram alterados. As mudancas de entrada ainda precisam de teste no TrimUI fisico.

Resultados desta versao no ambiente de desenvolvimento:

* **33 verificacoes de navegacao** passaram em x86_64 e AArch64 emulado:
  passo imediato, atraso de 350 ms, intervalo de 90 ms, soltar/centralizar,
  ausencia de rajadas apos frame lento, prioridades de fontes, D-pad e analogico
  simultaneos, zona morta/histerese, diagonais, inversao e stick direito ignorado.
* **63 verificacoes de interface por arquitetura** passaram com eventos SDL
  sinteticos. Incluem os fluxos de pesquisa/download da v0.2 e repeticao nas
  listas/teclado, limites da lista, teclas de computador, fallback de joystick/hat,
  parada ao perder foco ou desconectar controle, mudanca de tela e tela ocupada.
  O conteudo baixado continua sendo conferido com rclone real em fixtures locais.
* Builds nativo e Cortex-A53 com `-Werror`, smoke SDL nativo e ARM64 `offscreen`
  passaram. O pacote segue auditado por CRC, estrutura, hashes, ELF AArch64,
  GLIBC requerida e ausencia de configuracao privada.

No console, verificar: toque curto move uma vez; segurar cima/baixo percorre
varios itens e para ao soltar; os extremos da lista nao sao ultrapassados;
stick esquerdo cima/baixo move e esquerda/direita pagina; centralizar para;
diagonais nao oscilam; menus e teclado tambem respondem; botoes A/B/X/Start nao
se repetem. Confirmar o mapeamento SDL fisico (fallback bruto usa eixos 0/1),
especialmente se o firmware tiver controle com mapeamento diferente.

## Historico: versao 0.2.0

O usuario informou que a v0.1 funcionou no console. Depois relatou microSD
SanDisk Ultra Classe 10, Wi-Fi 2,4 GHz e taxa inicial de 7 MiB/s que cai para
aproximadamente 4 MiB/s. Nao houve benchmark controlado nem resultados individuais
de cada check. A equipe nao teve acesso direto ao dispositivo; nao ha confirmacao
detalhada de todas as funcionalidades da v0.2 no hardware.

Validacoes da v0.2 neste ambiente:

* Backend: **60 verificacoes** em cada modo — nativo x86_64, fallback de
  publicacao exclusivo e AArch64 emulado com rclone ARM64. Incluem a cobertura
  anterior e pesquisa Unicode/acentos, glob literal, resultados com nomes iguais
  em pastas distintas, caminhos inseguros, nomes ambiguos, buffer limitado,
  listagem dos arquivos locais e nome na mensagem de conclusao.
* Fluxo grafico: **37 verificacoes por arquitetura** (nativo e ARM64 emulado),
  com controle SDL simulado: abrir pesquisa, digitar, apagar caractere UTF-8,
  encontrar arquivo recursivamente, escolher o resultado correto entre nomes
  iguais, selecionar subpasta local, baixar, atualizar listagem, destacar arquivo,
  exibir tamanho/nome, voltar aos resultados, busca vazia/sem resultados,
  cancelar consulta e pesquisar a partir de uma subpasta. O script tambem confere
  conteudo exato da origem escolhida, preservacao do arquivo existente,
  ausencia de novos parciais e captura da tela apos download.
* Build ARM64/native com `-Werror` e smoke SDL 1280x720 continuam passando.
  SDL ARM64 usa o driver `offscreen`, nao o driver fisico `mali`.
* ZIP 0.2.0 auditado: estrutura, hashes, executaveis ARM64, versoes GLIBC
  e ausencia dos arquivos privados de configuracao/dados.

Novos testes no console: atualizar preservando a configuracao, baixar e conferir
o destaque/conteudo local; buscar `acao` para arquivo com acento numa subpasta;
usar os botoes do teclado e cancelar pesquisa grande; baixar um resultado e
conferir a origem. Para velocidade, comparar o mesmo arquivo com buffers 16/32
MiB nas mesmas condicoes de rede/cartao. **Nao foi medido ganho de velocidade.**

## Historico: versao 0.1.0

Data: 13/11/2026. Ambiente de build: Debian 13 x86_64, GCC 14 nativo,
SDK oficial TrimUI/Linaro GCC 7.5.0 para AArch64, QEMU user 7.2.

## Resultados

| Verificacao | Resultado / alcance |
| --- | --- |
| Build nativo e ARM64 | Passou com `-Wall -Wextra -Werror`, C++14; alvo Cortex-A53. |
| Backend nativo com rclone oficial amd64 | 38 verificacoes passaram. |
| Fallback de publicacao por copia exclusiva | As mesmas 38 verificacoes passaram em build de teste com `DRIVEBROWSER_TEST_COPY_COMMIT`. Macro nao usada no pacote. |
| Backend AArch64 + rclone oficial ARM64 | 38 verificacoes passaram sob QEMU. Nao usa binario x86 para simular a transferencia ARM. |
| Interface SDL nativa | Passou inicializacao, fonte UTF-8, renderer, eventos simulados Start/direcional/A, tela de ajuda/menu, captura 1280x720 e encerramento. |
| Interface AArch64 com as bibliotecas do pacote | Mesmo teste passou em QEMU usando o driver SDL `offscreen` do SDK; usa renderer por software. |
| Resolucao de bibliotecas ARM64 | `--diagnose` executado: SDL 2.26.1, SDL_ttf 2.0.13; drivers `mali` e `offscreen`. |
| rclone ARM64 | `version` executado em QEMU: 1.69.1, arm64, ARMv8, Go 1.24.0, link estatico; checksum do ZIP oficial conferido. |
| Pacote | Auditoria automatica de CRC, estrutura, manifestos SHA256, executaveis AArch64 e versoes GLIBC requeridas <=2.23. Sem `rclone.conf`, `settings.json`, `.git` ou dados privados. |
| Console fisico e Google Drive real | **Nao executados**. Nenhum aparelho ou credencial real foi disponibilizado. |

Os 38 checks por modo cobrem nomes com espacos, aspas, acentos, CJK e emoji;
JSON e ordenacao; descoberta de remotes; listagem; copia com conteudo exato;
arquivo vazio; bloqueio de nomes duplicados, `..`, links, prefixos enganosos,
mutacoes remotas e sobrescrita; concorrencia na publicacao; estimativa de espaco;
tamanho divergente; origem ausente; falha de execucao; transferencia interrompida;
cancelamento de processo lento; limpeza de parciais; sanitizacao dos erros;
diretorio renomeado apos a selecao. Remotes de teste usam `type=local` com dados
sinteticos; isto nao prova o comportamento da API Google.

## Diagnosticos resolvidos durante o desenvolvimento

* Link inicial misturou o `libpthread` glibc 2.23 do SDK com a glibc 2.25 da
  toolchain e falhou com simbolo `GLIBC_PRIVATE`. A ordem de busca agora prioriza
  as bibliotecas base da toolchain durante o link; bibliotecas glibc nao sao
  empacotadas. Simbolos exigidos pela aplicacao sao GLIBC_2.17.
* QEMU `-L` remapeava `open("/")` para dentro do sysroot, impedindo a validacao
  dos destinos por descritor. Os testes invocam explicitamente o loader com
  `--library-path` e guest base `-B 0x100000000`. Isso e configuracao da emulacao,
  nao um desvio nas protecoes de caminhos do aplicativo.
* O driver SDL `dummy` nao esta incluido no SDK GE8300: esse teste inicial
  falhou com `dummy not available`. Inspecao de drivers encontrou `offscreen`;
  o mesmo teste passou com ele. O driver `mali` ainda nao foi exercitado.

As capturas de desenvolvimento ficam em `build/smoke-native/data/smoke.bmp` e
`build/smoke-arm64/data/smoke.bmp` quando os alvos sao executados. Elas representam
renderizacao sem tela/controles fisicos, nao fotografias do console.

## Roteiro obrigatorio no TrimUI antes de declarar compatibilidade

1. Usar Smart Pro **original**, CrossMix **1.3.0**, fazer backup do cartao e
   extrair o ZIP. Confirmar que o app aparece e retorna ao menu ao sair, sem
   modificar firmware, governor ou processos do sistema.
2. Abrir a interface real com driver `mali` automatico: verificar orientacao,
   tela 1280x720, legibilidade, GPU/software fallback e ausencia de tela preta.
   Se necessario executar `./launch.sh --diagnose` em um terminal de manutencao;
   esse comando nao le tokens nem tenta autenticar.
3. Verificar direcional, A, B, X e Start, inclusive cancelamento. Confirmar que
   cada pressionamento produz uma acao, sem eventos duplicados. Se houver troca,
   ajustar `buttons` na configuracao; registrar o mapeamento e a versao do firmware.
4. Com Wi-Fi e configuracao privada valida, descobrir remotes, abrir pelo menos
   duas pastas e voltar; incluir nomes com espacos, acentos e nomes longos.
   Verificar arquivos em Drive pessoal e, se utilizado, Drive compartilhado.
5. Baixar arquivo pequeno com hash conhecido, arquivo vazio e arquivo grande
   adequado ao formato do cartao. Conferir bytes/hash no destino e no arquivo
   original. Observar progresso/velocidade e renovacao do token existente.
6. Selecionar subpasta do microSD e validar bloqueio de destino existente,
   destino nao permitido, links simbolicos e pouco espaco. Usar arquivos de
   teste, nunca apagar ou sobrescrever dados pessoais para validar.
7. Cancelar arquivo grande com B, desligar Wi-Fi durante transferencia,
   restabelecer conexao e tentar novamente; confirmar mensagem de erro e limpeza
   do parcial nas falhas tratadas. Medir memoria, uso de CPU e temperatura.
8. Verificar FAT32/exFAT e suporte real a `renameat2(NOREPLACE)`. O fallback foi
   testado em Linux, mas nao em cartao do console. Para queda de energia, usar
   cartao de teste; documentar recuperacao de `.part` e arquivos finais do fallback.
9. Registrar modelo, `uname -a`, versao do firmware base/CrossMix, drivers SDL,
   mapeamento e hashes do pacote. Nao anexar rclone.conf, tokens ou logs OAuth.

Sem esses resultados, a classificacao correta continua sendo **build experimental
com testes locais e emulados**, nao aplicativo homologado para o TrimUI.
