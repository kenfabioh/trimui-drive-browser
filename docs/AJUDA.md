# Configuração e solução de problemas

[Voltar ao README](../README.md)

## Configurar o Google Drive

**Criando seu Client ID e Client Secret**

Antes de configurar um novo remote do Google Drive no rclone, crie seu próprio
**Client ID** e **Client Secret**, seguindo o
[guia oficial do rclone](https://rclone.org/drive/#making-your-own-client-id).

Durante o comando `rclone config`, informe essas credenciais nos campos
`client_id` e `client_secret` e prossiga com a autorização da conta Google.

**Já possui um `rclone.conf` autenticado e funcionando?** Você pode continuar
utilizando esse arquivo no Drive Browser, sem refazer a configuração.

**Já usa rclone?** No computador em que ele funciona, execute:

```sh
rclone config file
```

Esse comando informa onde está a configuração. Copie esse arquivo para
`Apps/DriveBrowser/config/rclone.conf`. Mantenha os nomes dos remotes, token e,
quando usados, `client_id` e `client_secret`. Não cole esses valores em chats,
issues ou arquivos públicos. Guarde uma cópia privada antes de mover o cartão.

**Ainda não tem configuração?** Instale rclone de [rclone.org](https://rclone.org/downloads/)
no computador, execute `rclone config` e siga o
[guia oficial do Google Drive](https://rclone.org/drive/):

1. Crie um remote com um nome simples, como `meu-drive`.
2. Escolha Google Drive como serviço.
3. Prefira o escopo **somente leitura** (`drive.readonly`). Os números das opções
   podem mudar entre versões: confira a descrição, não um número de tutorial.
4. Complete a autorização pelo navegador e confirme a configuração.
5. Verifique que o remote lista seus arquivos e localize `rclone.conf` com
   `rclone config file`. Copie-o para o cartão, no caminho acima.

Essa preparação é feita uma vez. No console, o app reutiliza a configuração e
renova tokens quando possível. Não há login na interface do Drive Browser.
Um token revogado/expirado que não possa ser renovado precisa ser corrigido no
rclone e copiado novamente. Arquivos protegidos com senha de configuração não
são suportados: use o próprio rclone para preparar uma cópia descriptografada
local, guardada com cuidado. Não é necessário criar uma conta de serviço.

O exemplo `rclone.conf.example` é estrutural, sem credenciais válidas. Renomeá-lo
não autentica uma conta. Remotes que não sejam `type = drive` aparecem, mas não
podem ser abertos nesta versão.

## Problemas comuns

| Sintoma | O que conferir |
| --- | --- |
| App não aparece | Caminho `Apps/DriveBrowser/config.json`; evite `Apps/Apps`; atualize a lista/reinicie a interface |
| Ícone antigo | Substitua `Apps/DriveBrowser/icon.png` e atualize o cache/lista do launcher |
| Nenhum remote | Arquivo chamado exatamente `rclone.conf`, dentro de `config/`, legível e autenticado |
| Tela preta / app não abre | Modelo original e CrossMix 1.3.0; copie o ZIP completo, incluindo `lib/`, `res/` e `bin/` |
| Sem conexão / tempo esgotado | Wi-Fi, sinal, acesso ao Google e data/hora do console |
| Autorização recusada / token inválido | Teste a configuração no rclone original, corrija a autorização e copie novamente |
| Certificado TLS recusado | Confira relógio e integridade de `res/cacert.pem`; não desative a verificação TLS |
| Destino já existe | Use outra pasta ou renomeie/remova a cópia com o gerenciador de arquivos |
| Destino recusado | Deve ficar sob uma raiz de `allowed_roots`, dentro do microSD e sem links simbólicos |
| Download grande falha | Espaço livre, limite FAT32 de menos de 4 GiB e conexão estável |
| Arquivo Google Docs não aparece | Documentos nativos são omitidos; exporte antes para um arquivo normal |
| Nomes duplicados | Renomeie os itens ambíguos no Drive; o app não escolhe arbitrariamente por nome |
| Busca grande falha | Pesquise numa subpasta; limite de 5 minutos e 16 MiB de listagem |
| Analógico move sozinho | Centralize; se persistir, relate modelo/firmware e comportamento, sem alterar drivers do sistema |
| Botões trocados | Ajuste `buttons` em `config/settings.json`; defaults A=1, B=0, X=3, Start=6, Select=4 |

### Downloads interrompidos

B cancela a operação; uma nova tentativa começa do zero. Nas falhas tratadas,
o parcial é removido e não aparece como download completo. Depois de desligamento
brusco, pode sobrar `.drivebrowser-*.part` no destino: remova-o com o app fechado.
Se o sistema precisou da finalização por cópia exclusiva, uma queda de energia
nessa etapa pode deixar um arquivo final incompleto. Confira tamanho e remova
antes de repetir. Não remova arquivos pessoais desconhecidos para fazer esse teste.

### Desempenho

O download usa uma leitura sequencial, sem limite de banda imposto pelo app.
A velocidade exibida é **média desde o início**, não instantânea. Uma taxa inicial
alta que se estabiliza não prova defeito do cartão. Wi-Fi/sinal, Internet/Google,
CPU e escrita no microSD influenciam. Compare o mesmo arquivo com buffers de
16 e 32 MiB antes de mudar o padrão; mais buffer não garante mais velocidade.

## Pedir ajuda ou relatar um erro

Abra uma [issue](https://github.com/kenfabioh/trimui-drive-browser/issues) com:

- Modelo exato do console, CrossMix/firmware e versão do Drive Browser.
- Passos para reproduzir, o resultado esperado e o ocorrido.
- Mensagem de erro e captura de tela, com dados privados ocultados.
- Para desempenho: modelo/formato do microSD, Wi-Fi e taxa exibida.

**Nunca anexe `rclone.conf`, tokens OAuth, segredos de cliente ou dados privados.**
Para manutenção, `./launch.sh --diagnose` no console informa SDL e drivers sem
ler o token; isso não exige nem substitui autenticação. Não é necessário usar
terminal no fluxo normal do aplicativo.
