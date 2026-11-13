# Viabilidade, pesquisa e arquitetura

Pesquisa realizada em 13/11/2026. Alvo: **Smart Pro original A133 Plus**,
CrossMix OS **1.3.0**, Linux AArch64; nao Smart Pro S, Brick ou outros modelos.

## Evolucao 1.1.0

A navegacao usa paginas reais e tokens do Drive por uma extensao minima ao
backend rclone; `lsjson` permanece na pesquisa recursiva. O helper, cache,
preferencias e os limites estao descritos em [PAGINATION.md](PAGINATION.md).
L/R e L2/R2 trocam paginas sem repeticao ao segurar; as iniciais operam sobre
a pagina atual. Bookmarks incluem o indice de pagina. A quantidade pode ser
salva pelo menu Start ou configurada no arquivo pessoal.

## Evolucao 0.4.0

A selecao passa a ter bookmarks em memoria por tipo de tela, remote/pasta ou
raiz/pasta local e termo de pesquisa. Cada bookmark armazena a identidade do item
(caminho remoto completo nos resultados) e indice de fallback. Recarregar a
listagem procura a identidade antes de usar o indice. Isso preserva pagina e
cursor ao voltar do destino, do menu e de uma subpasta. A conclusao do download
restaura como bookmark de origem o arquivo efetivamente baixado, mesmo quando o
cursor havia mudado depois da marcacao com A. Nenhum historico e persistido.

Select (SDL BACK, configuravel) alterna o foco entre lista e faixa de iniciais.
Direcoes continuam usando Navigation e seu temporizador; A confirma e B sai do
modo de letras sem navegar para outra pasta. X fica inativo nesse modo. Inicio
salta ao primeiro arquivo; 0-9/A-Z/* agrupam pelo primeiro codepoint normalizado
com a mesma regra latina da pesquisa. Arquivos tem prioridade sobre pastas;
se um grupo nao tem arquivos, ha fallback para a primeira pasta. Nao ha nova
consulta ao Drive nem alteracao da ordenacao da lista.

O teclado passa a QWERTY em tres linhas centralizadas, seguido por numeros,
acentos, simbolos e comandos. O conjunto de caracteres disponiveis permanece
preservado. O rodape das listas acomoda a faixa e duas linhas para mensagens.
Backend, dependencias e parametros de transferencia nao mudaram.

## Evolucao 0.3.0

`src/navigation.hpp` concentra o estado de navegacao e um unico temporizador
monotonico (SDL_GetTicks64 na interface). Uma direcao gera passo imediato,
repete apos 350 ms e depois a cada 90 ms. Frames lentos nao geram rajadas de
passos atrasados. Soltar remove somente a origem correspondente; D-pad e stick
simultaneos na mesma direcao nao duplicam passos. A direcao ativa mais recente
tem prioridade quando ha origens conflitantes.

O stick esquerdo usa SDL_CONTROLLER_AXIS_LEFTX/LEFTY; joystick sem mapping SDL
usa eixos brutos 0/1. Zona de ativacao: 12000; liberacao: 8000, na escala signed
16-bit SDL. Histerese de eixo 4000 reduz oscilacao em diagonais. Eixos do stick
direito e gatilhos sao ignorados. Alteracoes de valor no stick ja inclinado nao
reiniciam o prazo de repeticao. Hats, botoes de joystick e setas do teclado
usam o mesmo estado; repeticao automatica do teclado do sistema e ignorada.

Eventos JOY equivalentes a um GameController ja aberto sao ignorados para evitar
duplicacao com eventos CONTROLLER. Foco perdido, minimizacao, desconexao,
consultas/transferencias e acoes de tela limpam a navegacao. Os botoes de acao
nao sao inscritos no temporizador. Dependencias e backend rclone nao mudaram.

O comportamento foi testado com eventos sinteticos e relogio deterministico,
incluindo UI AArch64 emulada. O mapeamento fisico do stick no TrimUI ainda exige
validacao pelo usuario nesta versao.

## Evolucao 0.2.0

O usuario relatou funcionamento geral da v0.1 no aparelho. As mudancas da v0.2
continuam exigindo validacao fisica especifica; nao houve acesso direto ao console.

* A pasta de destino agora lista diretorios e arquivos regulares, com tamanho.
  Links e arquivos temporarios do app continuam ocultos. Ao concluir, o conteudo
  e relido, o arquivo baixado recebe foco e a mensagem inclui o nome.
* Pesquisa recursiva a partir da pasta atual por `lsjson --recursive`, incluindo
  diretorios na validacao para detectar nomes de pastas ambiguos. Os caminhos
  relativos de cada resultado sao validados componente a componente e mantidos
  ate a selecao; resultados com mesmo nome em pastas diferentes nao sao confundidos.
* A correspondencia e literal por parte do nome, com case/accent folding latino
  (inclusive acentos decompostos). Outros alfabetos sao comparados literalmente.
  O teclado na tela usa somente direcional/A/X/B; Start abre o menu. Nenhum termo
  digitado e interpolado em shell, regex, glob ou consulta ao servidor.
* A pesquisa enumera a subarvore, nao consulta um indice global do Drive. Tem
  cancelamento e limites de 300s/16 MiB; erros/cancelamentos nao substituem
  resultados anteriores por resultados parciais. A busca nao altera a permissoes
  de leitura remota nem permite caminhos de destino externos.
* `download_buffer_mib` configura 1–64 MiB, padrao 16, exclusivamente em `cat`.
  Esse fluxo permanece sequencial. Nao ha paralelismo por partes nem alegacao de
  ganho de velocidade sem medicao real. A configuracao antiga permanece valida.
* Testes de interface executam a maquina de estados de producao com eventos SDL
  de controle. A descoberta do remote e simulada como Drive; as listagens,
  pesquisa recursiva e leitura usam rclone real sobre fixtures locais. Isso
  valida a integracao interna, nao os endpoints do Google.

## Evidencias consultadas

| Fonte | Evidencia e limite |
| --- | --- |
| [CrossMix v1.3.0](https://github.com/cizia64/CrossMix-OS/tree/611742be7c1965d8c5a0f6d1eac965c916371c71) | Tag oficial inspecionada. `Apps/FileManager/config.json` declara label/icon/launch; `launch.sh` entra na pasta e adiciona bibliotecas locais. `System/lib` e o cartao complementam o firmware. |
| [Plunder](https://github.com/StefanAngelovski/Plunder/tree/fd4668a1aafcd2c4338bf7c023055a8108231c56) | App real SDL2/SDL_ttf distribuido em `/SDCARD/Apps/`; Makefile usa SDK TrimUI. `ControllerButtons.h` troca A/B e X/Y. Referencia tecnica; nenhum codigo do Plunder foi copiado. |
| [Build system](https://github.com/Maxwell-SS/trimui-smart-pro-build-system/tree/6cbfdd5e31decbe6087422f4b79e79a31d9f93c8) | Dockerfile aponta para SDK oficial 20231018, Linaro 7.5, SDL2 2.26.1 GE8300. O testador de controles usa 1280x720 e eventos SDL_CONTROLLERBUTTON. |
| [SDK oficial TrimUI](https://github.com/trimui/toolchain_sdk_smartpro/releases/tag/20231018) | Artefatos `aarch64-linux-gnu-7.5.0-linaro.tgz`, `SDK_usr_tg5040_a133p.tgz`, `SDL2-2.26.1.GE8300.tgz`, baixados e examinados. ELF AArch64 e base SDK glibc 2.23. |
| [rclone 1.69.1](https://github.com/rclone/rclone/releases/tag/v1.69.1) | Binario oficial `linux-arm64`, checksum comparado ao SHA256SUMS oficial, ELF estatico, Go 1.24.0. Executado em QEMU; nao presumido compativel so pelo nome ARM64. |

O repositorio CrossMix nao e uma imagem completa da raiz do firmware instalada
no aparelho. Portanto ele **nao comprova sozinho** a presenca/versao de todas as
bibliotecas em `/usr/trimui/lib`. O SDK e os aplicativos existentes sustentam a
escolha de SDL2, mas a verificacao da instalacao real continua pendente.
Para reduzir dependencias no sistema, o pacote traz SDL2, SDL_ttf, FreeType,
zlib, bzip2 e libgcc_s. Nao substitui a glibc, kernel ou drivers GPU do firmware.

## Decisao de implementacao

* C++14/SDL2 para janela, renderizacao, eventos e controles; SDL_ttf para UTF-8.
  Nao depende de Python, navegador, Node ou instalador de pacotes no console.
* rclone em subprocessos com `fork/execv`, stdin fechado, argumentos separados,
  saidas nao bloqueantes, memoria limitada e cancelamento do grupo de processos.
  A interface permanece responsiva durante consulta/transferencia.
* Descoberta por `listremotes --json --source file`, navegacao por `backend drivebrowser-page`
  (uma pagina), pesquisa por `lsjson --recursive`, e download por `cat` com stdout ligado a um descritor local.
  JSON nao e analisado com expressoes regulares nem nomes com shell.
* Apenas comandos de leitura remota sao aceitos pelo backend. O app nao usa
  `copy`, `sync`, `move`, `delete`, `mkdir` ou servidor RC de controle remoto.
* Destino com caminho absoluto abaixo das raizes configuradas, componentes
  validados e `openat(O_NOFOLLOW)` por componente. Subpastas nao seguem links.
  Download nao recebe caminho de destino arbitrario vindo do Google Drive.
* Arquivo parcial exclusivo, comparacao de tamanho, fsync e publicacao sem
  sobrescrita via `renameat2(RENAME_NOREPLACE)`. Fallback em copia exclusiva e
  limitada por quadro, permitindo cancelar; ressalva para queda de energia no README.
* Progresso medido por `fstat` do arquivo real; velocidade media desde o inicio,
  nao um valor estimado inventado. Tamanho desconhecido exibe `?`.
* Consulta limitada a 16 MiB de stdout e 90s; stderr limitado a 64 KiB, usado
  apenas para classificacao de erro, nunca salvo com dados OAuth.

## Estrutura

```text
src/core.*            processos, rclone, JSON, confinamento, transferencia
src/main.cpp          telas, eventos, menu e renderizacao SDL
app/                  launch.sh, config.json, exemplos, fontes, icone, licencas
scripts/              dependencias fixadas, build/teste ARM, empacotamento/auditoria
tests/core_test.cpp   integracao com rclone local e falhas controladas
docs/                 pesquisa, evidencias e roteiro no dispositivo
dist/                 ZIP gerado (artefato, nao credenciais)
```

## Dependencias de execucao

Linux AArch64 com loader `/lib/ld-linux-aarch64.so.1` e glibc compativel,
dispositivos de video/entrada e bibliotecas GPU do firmware TrimUI, microSD
montado em `/mnt/SDCARD`, Wi-Fi configurado no CrossMix, hora correta, acesso
HTTPS ao Google e um rclone.conf utilizavel. Nao precisa de servico separado.

Destinos usuais de rede: `oauth2.googleapis.com` para renovar token,
`www.googleapis.com`/`*.googleapis.com` para API do Drive, e destinos de conteudo
Google conforme a resposta da API. Nenhuma rede privada/VPN e necessaria.
Nao foram adicionadas credenciais ou permissoes de rede ao ambiente de build:
os testes usam somente fixtures locais. O fluxo Google precisa ser testado no
aparelho com a configuracao privada do usuario.
