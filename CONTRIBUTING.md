# Desenvolvimento e contribuições / Development

[README](README.md) · [English user guide](README.en.md)

O alvo é o **TrimUI Smart Pro original com CrossMix OS 1.3.0**. Não altere o
firmware para instalar ou desenvolver este aplicativo. Preserve configurações
pessoais, credenciais e downloads; use fixtures locais nos testes.

## Preparar o build (Linux x86_64)

Requisitos do computador: Python 3, curl, tar, make, gcc/g++, binutils/readelf.
No checkout Git, execute os comandos na raiz do repositório. Se estiver usando
os fontes incluídos no pacote instalável, entre primeiro na pasta `source/`.
Os testes gráficos nativos precisam dos runtimes SDL2 e FreeType. Em Debian/
Ubuntu, os nomes usuais são `libsdl2-2.0-0` e `libfreetype6`. O console não precisa
de pacotes instalados: as dependências de execução acompanham o ZIP.

```sh
make deps
make pager
make arm64 native
make test test-fallback test-arm64
make test-input test-input-arm64
make test-ui test-ui-arm64
make smoke-native smoke-arm64
make package
python3 scripts/verify_package.py
```

`make deps` baixa artefatos oficiais com hashes fixados para `.deps/`, sem root ou
Docker. O SDK ocupa aproximadamente 440 MiB de downloads e 1,6–2 GiB extraídos.
`make pager` também baixa Go 1.24.0, o fonte rclone 1.69.1 e módulos Go fixados;
reserve vários GiB adicionais para fontes e caches. O pacote ARM não precisa
do compilador Go em execução. [Construção e testes da paginação](docs/PAGINATION.md). Para
somente compilar ARM, use `python3 scripts/fetch_deps.py --skip-native` e
`make arm64`. Não desative TLS, assinaturas ou verificações de hash.

Para reutilizar um SDK/cache fora do checkout, passe os dois caminhos em todos
os comandos, por exemplo:

```sh
make test-ui SDK_ROOT=/workspace/toolchains DEP_CACHE=/workspace/downloads
```

A toolchain é a Linaro 7.5.0 do SDK oficial TrimUI 20231018; SDL2 é a variante
2.26.1 GE8300. A ordem de link do Makefile evita misturar bibliotecas base da
glibc da toolchain com o `libpthread` do SDK. Não substitua por um compilador ARM
genérico sem verificar símbolos GLIBC e testar a execução.

## O que os testes verificam

- `test`, `test-fallback`, `test-arm64`: JSON, nomes Unicode, leitura com rclone,
  cancelamento, falhas, tamanho, espaço, caminhos confinados e não sobrescrita.
- `test-input`, `test-input-arm64`: repetição, zona morta, diagonais e parada.
- `test-ui`, `test-ui-arm64`: eventos SDL, teclado, pesquisa, seleção, destino,
  download, retorno ao item, faixa de letras, paginação, cache e preferências.
  A descoberta é simulada como Drive; listagem/leitura usam rclone real sobre
  arquivos locais. O transporte paginado é uma fixture; `make pager` testa o
  helper de produção contra respostas HTTP simuladas, inclusive em ARM64.
- `pager`: testes HTTP do helper, limite real por consulta, token, Drive
  compartilhado, duplicatas, erros e resposta incompleta; builds estáticos Go.
- `smoke-*`: SDL, fontes, renderização, menu e captura. ARM64 usa `offscreen`.
- `verify_package.py`: estrutura, CRC, hashes, ícone RGBA 300x300, executáveis
  AArch64, símbolos GLIBC e ausência de configurações privadas.

A CI compila e testa; **não publica releases automaticamente**. Os testes locais
não comprovam acesso à API Google ou funcionamento em todo firmware.
[Validação e roteiro no aparelho](docs/VALIDATION.md).

## Estrutura e alterações

`src/` contém interface/backend/entrada; `app/` os recursos e exemplos;
`scripts/` as ferramentas; `tests/` os testes; `docs/` os guias e evidências.
`VERSION` define a versão do pacote; mantenha o texto de versão da interface e
as instruções sincronizados ao preparar uma nova release.

Mantenha os comandos remotos restritos à leitura e os destinos dentro das raízes
permitidas. Não adicione shell para interpretar nomes de arquivos. Não esconda
falhas de teste nem remova verificações de segurança para fazer o build passar.

Use configurações de exemplo sem tokens. Nunca envie `rclone.conf`, arquivos
`.env`, chaves, segredos OAuth, dumps de ambiente ou logs com dados privados.
Ao abrir um PR, descreva a mudança, como foi testada e o que exige hardware.
O ícone aprovado está em `app/icon.png`; `make_icon.py` valida, não redesenha a arte.

## English summary

Use Linux x86_64, Python 3, curl/tar, make, GCC/G++, binutils and the native SDL2/
FreeType runtimes. Run the commands above. Dependencies are downloaded locally
with pinned checksums; the ARM build uses the official TrimUI SDK. QEMU tests use
real ARM64 rclone over local fixtures, not a Google account. Never commit secrets,
disable TLS/checksums, allow remote mutations or bypass destination confinement.
Include meaningful validation and hardware limitations in pull requests.

Project license: GPL-3.0. Bundled dependencies retain their own licenses.
