# Guia detalhado — Drive Browser

Versao **1.1.0** de um navegador grafico do Google Drive
para o TrimUI Smart Pro original com CrossMix OS 1.3.0. Interface C++/SDL2 em
1280 × 720, backend rclone 1.69.1 ARM64 e dados no microSD.

O mantenedor relatou funcionamento do conjunto atual no modelo alvo.
A paginacao da versao 1.1 tambem foi confirmada pelo mantenedor no seu aparelho; os testes
automatizados usam fixtures locais e nao substituem testes em outros aparelhos.
Backend, interface e dependencias foram testados em Linux e
emulacao ARM64, com fixtures locais, sem credenciais reais do Google.
Veja [evidencias e validacao pendente](VALIDATION.md) e
[pesquisa e arquitetura](ARCHITECTURE.md).

## Instalar

1. Extraia `DriveBrowser-1.1.0-arm64.zip` na **raiz do microSD**. O resultado deve
   ser `Apps/DriveBrowser/launch.sh` (nao `Apps/Apps/DriveBrowser`).
2. Copie o seu **rclone.conf autenticado existente** para
   `Apps/DriveBrowser/config/rclone.conf`. Nao renomeie o exemplo como se ele
   fosse uma configuracao valida. Nenhuma credencial real e fornecida.
3. Ligue o Wi-Fi, confira data/hora do console e abra **Drive Browser** em Apps.
   Se o item nao aparecer, atualize a lista de aplicativos/reinicie a interface.
4. Escolha um remote `type = drive`, navegue e selecione um arquivo.

O aplicativo nao exibe login nem exige autenticar novamente um token valido.
O token precisa estar valido/renovavel e os `client_id`/`client_secret` precisam
acompanhar a configuracao quando utilizados. Remotes sao detectados pelo rclone;
tipos diferentes de `drive` sao exibidos, mas nao abertos nesta versao.
Arquivos de configuracao criptografados por senha nao sao suportados: forneca
uma copia local descriptografada pelo proprio rclone. Nao compartilhe essa copia.
Depois da instalacao, uso normal, navegacao e downloads usam somente os botoes.

Para atualizar de uma versao anterior, feche o app e extraia o novo ZIP sobre `Apps/DriveBrowser`,
substituindo os arquivos do programa. **Preserve `config/rclone.conf`,
`config/settings.json`, `data/` e seus downloads.** O ZIP nao inclui os dois
arquivos de configuracao pessoais e nao exige refazer o login.

## Controles

| Botao fisico | Acao |
| --- | --- |
| Direcional cima/baixo | Mover cursor |
| Direcional esquerda/direita | Avancar/voltar dez itens |
| Analogico esquerdo | Mesmas acoes do direcional |
| A | Abrir pasta/remote ou marcar arquivo com `[*]` |
| B | Voltar; durante consulta ou transferencia, cancelar |
| X | Escolher destino do arquivo marcado; sem marca, usa o arquivo sob o cursor |
| X no seletor de destino | Baixar na pasta exibida |
| Start | Menu: destino, atualizar, remotes, informacoes, pesquisa, sair |
| Select | Alternar foco entre arquivos e faixa de letras no rodape |
| L / R (tambem L2 / R2) | Pagina anterior / proxima no Drive |

**Segurar** uma direcao do direcional ou do analogico esquerdo continua movendo
o cursor. O primeiro passo e imediato; a repeticao comeca apos 350 ms, a cada
90 ms. Solte o botao ou centralize o analogico para parar. Isso funciona nas
listas do Drive, resultados, destinos locais, menus e teclado de pesquisa.
Nas listas, cima/baixo move um item e esquerda/direita pagina dez itens;
no teclado, move entre teclas. O cursor das listas para no primeiro/ultimo item.

O analogico tem zona morta e histerese para evitar drift e oscilar entre eixos
nas diagonais. O analogico direito nao navega. Somente direcoes se repetem:
A/B/X/Start continuam sendo acoes por pressionamento. Trocar de tela, iniciar
uma consulta/download, perder foco ou desconectar o controle limpa o movimento
pendente; solte/recentralize antes de navegar novamente.

Na escolha do destino, A entra na pasta e B sobe ate a lista de raizes.
Cada download copia **um arquivo**, sem baixar pastas recursivamente.
O rodape mostra bytes, tamanho esperado, velocidade media e resultado/erro.
Ao concluir, a pasta local e atualizada para mostrar **pastas e arquivos**, com
tamanhos; o arquivo baixado fica sob o cursor e seu nome aparece na confirmacao.
A em um arquivo local exibe informacoes; nao o executa nem o abre como pasta.
Ao voltar com B para o Drive depois de baixar, o cursor retorna ao **arquivo
baixado**, inclusive na pagina em que ele estava ou no resultado correto da
pesquisa. A posicao e lembrada pelo caminho completo, nao apenas pelo indice
ou nome: se a ordem mudar, o mesmo arquivo continua selecionado. Se ele tiver
sido removido, o cursor fica num item proximo valido. A memoria de selecao vale
durante a execucao do app, sem gravar um historico no cartao.
Depois de falha/cancelamento, X na pasta de destino tenta de novo. Se o tamanho
remoto mudou, volte ao Drive e atualize a listagem primeiro.
Durante uma operacao, B cancela; o menu fica disponivel depois de ela terminar.

## Paginas do Google Drive

Por padrao sao consultados ate **50 itens por pagina**, incluindo pastas.
Use **L** para voltar e **R** para avancar. A primeira pagina nao exige listar
o restante da pasta. Pastas ja visitadas e o retorno do download reutilizam
paginas em memoria; B retorna ao arquivo baixado na pagina correspondente.
Start > Atualizar limpa o cache da pasta e recomeca na pagina 1.

**Start > Itens por pagina** permite ajustar com esquerda/direita e salvar com
**A**, sem precisar de computador. Opcoes: 10, 25, 50, 100, 200, 500 e 1000.
**B** sai sem aplicar uma alteracao nao salva. A escolha salva fica em
`config/settings.json`, preservando as outras preferencias. Ao voltar ao Drive,
a lista recomeca na pagina 1 com a nova quantidade.

Para uma quantidade personalizada, feche o app e defina `"page_size": 75` nesse
arquivo; sao aceitos inteiros de 1 a 1000. Sem o campo, usa 50. Valores maiores
aumentam a memoria e podem demorar mais. O indicador mostra pagina atual e
limite escolhido, sem inventar uma contagem total que exigiria listar tudo.

A API pode retornar menos itens ou uma pagina vazia com continuacao, especialmente
com documentos Google omitidos; R continua quando ha proxima pagina. Falha ou
cancelamento preserva a pagina anterior. Tokens expirados exigem atualizar.
A pesquisa recursiva continua abrangendo toda a subarvore, sem esse limite por
pagina. [Detalhes tecnicos e limites](PAGINATION.md).

## Saltar pela inicial

Nas listas do Drive, resultados de pesquisa e pasta local, aperte **Select**
para selecionar a faixa de letras. No Drive, ela opera apenas na pagina atual. Use direcional ou analogico esquerdo para
escolher um grupo; o cursor salta ao primeiro arquivo correspondente:

* **Inicio**: primeiro arquivo da lista, mesmo quando ha pastas antes dele.
* **0-9**: primeiro nome iniciado por um numero.
* **A-Z**: inicial sem distinguir maiusculas ou acentos latinos; `Á` entra em A
  e `Ç` em C. A comparacao usa o nome do arquivo, nao o nome da pasta do resultado.
* **\***: nomes que comecam com simbolos ou caracteres fora desses grupos.

Se nao houver arquivo da inicial escolhida, o app tenta uma pasta dessa inicial.
Se nao houver nenhum item, exibe uma mensagem e mantem o cursor. Nao muda a
ordenacao nem faz outra consulta ao Drive. A/Select volta ao foco nos arquivos;
A tambem confirma o salto para o primeiro item da inicial atual. B sai da faixa
sem voltar de pasta; X nao inicia download enquanto o foco esta nas letras.
Depois de voltar aos arquivos, os controles A/B/X funcionam normalmente.
Segurar uma direcao tambem funciona na faixa de letras.

## Pesquisa

Abra a pasta do Google Drive onde deseja procurar e use **Start > Pesquisar
nesta pasta e subpastas**. No teclado **QWERTY** da tela, use o direcional para escolher
uma tecla e A para digitar; ha teclas de espaco, apagar, limpar e acentos.
**X ou Buscar** inicia a pesquisa, e **B** volta/cancela a consulta.

A pesquisa localiza parte do **nome do arquivo** em toda a subarvore atual,
ignorando maiusculas e acentos latinos: `acao` encontra `Áção.zip`. Outros
caracteres Unicode sao comparados literalmente. `*`, colchetes e outros sinais
sao texto literal, nao filtros executaveis. Os resultados mostram o caminho
relativo para distinguir arquivos com o mesmo nome em pastas diferentes.
Use A para marcar o resultado e X para escolher o destino e baixar normalmente.
B nos resultados volta a pasta de origem da pesquisa.

Esta versao percorre a arvore com rclone; nao usa um indice nem busca no conteudo
dos arquivos. Uma arvore grande pode levar tempo: **B cancela**, o limite e de
5 minutos e 16 MiB de listagem. Se o limite for atingido, nenhum resultado parcial
e usado; procure dentro de uma subpasta menor. Nomes duplicados na mesma pasta
continuam bloqueados para evitar baixar um arquivo ambiguo.

## Destinos e configuracao

Por padrao, a unica raiz permitida e `/mnt/SDCARD/Downloads/DriveBrowser`, criada
quando necessario. O seletor permite usar suas subpastas existentes.
Para permitir outras pastas, copie `config/settings.example.json` para
`config/settings.json` e ajuste, por exemplo:

```json
{
  "sd_root": "/mnt/SDCARD",
  "allowed_roots": [
    "/mnt/SDCARD/Downloads/DriveBrowser",
    "/mnt/SDCARD/Roms"
  ],
  "page_size": 50,
  "download_buffer_mib": 16,
  "buttons": {"up":11,"down":12,"left":13,"right":14,"a":1,"b":0,"x":3,"start":6,"select":4,"l":9,"r":10,"l2":16,"r2":17}
}
```

As raizes devem ser subpastas do microSD; a propria raiz, `..`, links simbolicos
e componentes inseguros sao recusados. `launch.sh` exige que `/mnt/SDCARD` esteja
montado para evitar gravar no firmware quando o cartao estiver ausente.
Diretorios sao mantidos abertos por descritores para evitar redirecionamento de
caminhos apos a escolha. Nomes vao ao rclone como argumentos, sem shell.
Nao ha sobrescrita: escolha outro destino ou remova/renomeie o arquivo existente
com seu gerenciador de arquivos. Nomes duplicados na mesma pasta do Google Drive
sao recusados, pois esta versao navega por nomes e nao por IDs.

Os numeros de `buttons` correspondem aos eventos SDL. Os defaults seguem os
exemplos TrimUI pesquisados: A fisico = SDL B, B fisico = SDL A, X fisico = SDL Y.
Caso o firmware mapeie diferente, ajuste o arquivo. A tela de informacoes mostra
o driver e o numero de controles; o mapeamento real precisa ser validado no console.
Select usa SDL BACK (4) por padrao. Arquivos antigos sem `select` continuam
validos com esse default. No computador: setas, Enter=A, Esc=B, x=X, Tab=Start,
F1=Select, PageUp=L e PageDown=R. L/R usam SDL shoulders (9/10);
L2/R2 usam eixos SDL de gatilho ou botoes brutos 16/17.
No teclado de pesquisa, tambem e possivel digitar com teclado fisico: Enter
pesquisa, Backspace apaga e Esc volta.

## Velocidade de download

O app baixa **um arquivo por vez**, usando `rclone cat`, com **uma leitura
sequencial** e buffer de **16 MiB**. Nao define limite de banda. Aumentar
`--transfers` ou `--multi-thread-streams` nao paraleliza esse caminho de leitura.
`--drive-chunk-size` controla upload e nao melhora este download.

O resultado depende do elo mais lento: **Wi-Fi/sinal, Internet/rota ate o Google,
limites do Drive, CPU/TLS e escrita no microSD**. A classe de velocidade impressa
no cartao nao garante a velocidade efetiva no leitor do console. Finalizacao
inclui sincronizacao no cartao; o fallback pode fazer uma copia local adicional.

Para comparar sem mudar o firmware, ajuste `download_buffer_mib` em
`config/settings.json`: padrao **16**, experimente **32**, depois **64** se houver
memoria disponivel; o intervalo aceito e 1–64 MiB. A configuracao pessoal antiga
continua valida sem esse campo. A tela de informacoes mostra o valor ativo.
Um buffer maior pode suavizar variacoes, mas **nao garante maior velocidade** e
consome mais RAM. Nenhuma melhoria de taxa foi medida neste ambiente: fixtures
locais/emulacao nao representam Wi-Fi, Google Drive ou microSD do console.

Compare o mesmo arquivo grande, no mesmo destino e condicoes de rede, com o app
reiniciado entre os testes e 16/32 MiB (sem deixar arquivos existentes bloquearem
a nova tentativa). Observe MiB/s durante o download e o tempo ate a conclusao.
Se a escrita do cartao for o limite, aumentar o buffer nao resolve. Downloads
paralelos por partes seriam uma mudanca de arquitetura futura, com novos testes
de memoria, cancelamento e seguranca dos destinos.

## Dados, seguranca e falhas

* Configuracao em `config/`, cache/temporarios em `data/`, tudo no microSD.
  O rclone pode atualizar o token OAuth no proprio `rclone.conf`.
* O programa so emite `listremotes`, `lsjson`, `cat` e a extensao de leitura
  `backend drivebrowser-page`. Nao cria, edita ou apaga
  arquivos remotos. Isso **nao reduz os privilegios de um token preexistente**;
  use uma configuracao criada com `drive.readonly` se precisar garantir a
  restricao tambem na credencial.
* TLS permanece verificado com certificados Mozilla incluidos em `res/cacert.pem`.
  Sem `--no-check-certificate`. Atualize certificados e rclone por um novo pacote
  revisado quando necessario; este app nao faz autoatualizacao.
* Erros sao classificados para exibir autorizacao, quota, DNS, timeout ou espaco
  quando reconhecidos. URLs e respostas OAuth brutas nao sao gravadas em log.
* Download vai para `.drivebrowser-*.part` na pasta escolhida. Cancelamento,
  erro de rede, processo interrompido ou tamanho divergente impedem a publicacao
  e removem o parcial nas falhas tratadas. O app sincroniza o arquivo antes de
  concluir. Nao ha retomada por intervalo: uma nova tentativa recomeca do zero.
* Desligamento brusco pode deixar `.drivebrowser-*.part`; com o app fechado,
  remova esses arquivos pelo gerenciador de arquivos. Nunca sao executados nem
  considerados downloads concluidos. Em sistemas sem `renameat2(NOREPLACE)`,
  a finalizacao usa copia exclusiva: uma queda de energia nessa etapa pode
  deixar o nome final incompleto; confira o tamanho/remova antes de tentar de novo.
* FAT32 limita arquivos a menos de 4 GiB; o aplicativo nao converte o cartao.
  Espaco e verificado antes da transferencia, mas pode acabar durante o uso.
* Permissoes Unix podem nao ser aplicadas em FAT/exFAT: proteja o cartao que
  contem `rclone.conf`. O ZIP e os testes nao usam credenciais reais.

Limites desta versao: configuracao por arquivo, uma transferencia por vez,
sem exportacao de documentos nativos Google (omitidos da lista), pesquisa por nome,
sem fila, sem login na interface e sem fontes para todos os alfabetos.
Espacos, acentos e Unicode sao preservados no caminho e no arquivo; glifos nao
presentes em DejaVu Sans podem aparecer como quadrados (nao alteram o nome).
Cada resposta JSON tem limite de 16 MiB. Consultas podem ser canceladas
e tem limite total de 90 segundos (pesquisa: 300); leitura de rede usa timeout de inatividade.

## Compilar e testar (Linux x86_64)

Requisitos de desenvolvimento: Python 3, curl, tar, gcc/g++, make, readelf;
para o teste grafico nativo, runtimes SDL2 e FreeType do computador. Nenhum
desses pacotes precisa ser instalado no console.

```sh
make deps                 # artefatos oficiais, hashes fixados, instalacao local .deps/
make pager               # Go/rclone fixados; testa e compila paginacao amd64/ARM64
make arm64               # SDK TrimUI, Cortex-A53, glibc requerida <= 2.23
make test test-fallback   # backend real rclone com remote local de fixture
make test-arm64          # mesmo backend ARM64 via QEMU + rclone ARM64
make test-ui test-ui-arm64 # teclado, pesquisa, selecao e download com rclone local
make test-input test-input-arm64 # repeticao, zona morta e arbitragem das direcoes
make smoke-native        # SDL do computador, botoes simulados/menu/captura
make smoke-arm64         # SDL do pacote, driver offscreen, via QEMU
make package             # dist/DriveBrowser-1.1.0-arm64.zip e SHA256
python3 scripts/verify_package.py
```

O primeiro `make deps` baixa cerca de 440 MiB e extrai aproximadamente 2 GiB;
ele nao usa Docker, root ou instalacao no sistema. Para apenas compilar ARM,
use `python3 scripts/fetch_deps.py --skip-native` e `make arm64`.
`SDK_ROOT` e `DEP_CACHE` podem apontar para caches externos. A compilacao usa
as bibliotecas base da toolchain antes das do SDK para nao misturar a glibc 2.25
da toolchain com `libpthread` 2.23 do SDK durante o link. A aplicacao resultante
usa somente simbolos GLIBC_2.17; o pacote verifica todas as bibliotecas incluidas.

Fontes correspondentes e scripts acompanham o ZIP em `source/`; tambem estao
neste repositorio. Licenca do projeto: GPL-3.0. Dependencias, versoes,
licencas e fontes: `licenses/NOTICE.md` no pacote ou
[app/licenses/NOTICE.md](../app/licenses/NOTICE.md) no repositorio.
