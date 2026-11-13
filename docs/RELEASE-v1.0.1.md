# Drive Browser 1.0.1 — TrimUI Smart Pro

Gerenciador gráfico para navegar pelo Google Drive e baixar arquivos no microSD,
controlado inteiramente pelos botões do **TrimUI Smart Pro original / CrossMix OS 1.3.0**.

Corrige também o build nativo com compiladores que exigem a conferência do retorno de escrita.

## Incluído

- Navegação de pastas e pesquisa por nome com teclado QWERTY.
- Download individual com destino, progresso, velocidade média e cancelamento.
- Retorno ao arquivo baixado, direcional/analógico esquerdo com repetição e faixa
  Select: Início, 0–9, A–Z e outros caracteres.
- Ícone D na nuvem 300×300, rclone ARM64, bibliotecas, exemplos e fontes correspondentes.
- Documentação em português e inglês, configuração, solução de problemas e licenças.

## Instalar

Baixe **DriveBrowser-1.0.1-arm64.zip**, extraia na **raiz do microSD** e adicione
seu `rclone.conf` autenticado em `Apps/DriveBrowser/config/rclone.conf`.
Não há login na interface. O exemplo não contém credenciais válidas.
Abra Drive Browser em Apps; A abre/seleciona, X escolhe o destino e X confirma.

Ao atualizar, preserve `config/rclone.conf`, `config/settings.json`, `data/` e
seus downloads. O arquivo Source code do GitHub não é o pacote de instalação.
O checksum SHA256 acompanha o ZIP.

## Compatibilidade e limites

Uso real relatado pelo mantenedor no modelo alvo. Testes automatizados em Linux
nativo e ARM64 emulado usam fixtures locais, sem conta Google. Outros modelos e
sistemas não estão confirmados. A versão 1.0 consolida os recursos da 0.4 com o
ícone final e documentação; não altera o fluxo de transferências.

Apenas leitura do Drive, um arquivo por vez, sem sobrescrita ou retomada de
parciais. Google Docs nativos são omitidos. Nomes duplicados na mesma pasta são
recusados. FAT32 limita arquivos a menos de 4 GiB. Não publique seu rclone.conf.

## English

Browse Google Drive and download files directly to microSD on the **original
TrimUI Smart Pro with CrossMix OS 1.3.0**. Includes QWERTY filename search,
controller/left-stick navigation, destination selection, progress/cancellation,
restored file focus, letter jumps and the approved 300px icon.

Extract **DriveBrowser-1.0.1-arm64.zip** to the card root and place your existing
authenticated config at `Apps/DriveBrowser/config/rclone.conf`. Preserve personal
configuration and downloads when updating. No OAuth login is available in the
app. The UI is Portuguese; the README includes an English guide.

Read-only remote access; one file at a time; no overwrite or partial resume.
Native Google Docs and ambiguous duplicate names are not supported. Real-device
use is maintainer-reported; automated tests use local fixtures on native Linux
and emulated ARM64. Other models/OS versions are unconfirmed.
