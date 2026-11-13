# Preparar e publicar uma versão

A publicação no GitHub é uma etapa separada: **somente executar push, envio de
tags e criação de release depois da autorização final do responsável**. Uma
autorização explícita na conversa já atende esse requisito. Builds e CI não
publicam automaticamente. A publicação do ambiente na nuvem é independente.

1. Atualize `VERSION`, versão da interface, README, changelog e notas da release.
2. Execute os testes do CONTRIBUTING e audite o ZIP. Confira ícone RGBA 300x300,
   arte dentro da área CrossMix 170x170, estrutura e ausência de credenciais.
3. Atualize o alvo fixado em `.github/workflows/publish.yml` para a versão
   aprovada. Esse workflow só é iniciado manualmente e exige uma release draft.
4. Revise e faça commit; prepare a tag anotada local da versão nesse commit.
5. Confira a main remota; envie commit e tag com push normal, sem `--force`.

Depois da autorização, para a release 1.1.0:

```sh
git fetch origin main --tags
git push origin HEAD:main
git push origin v1.1.0
gh release create v1.1.0 --repo kenfabioh/trimui-drive-browser --verify-tag --draft --title 'Drive Browser 1.1.0 — TrimUI Smart Pro' --notes-file docs/RELEASE-v1.1.0.md
gh workflow run publish.yml --repo kenfabioh/trimui-drive-browser --ref main
```

O workflow recompila e testa nativo/ARM64, audita o pacote, envia ZIP e SHA256
ao draft, baixa os arquivos e confere hash e conteúdo antes de publicar como
latest. Usa o token temporário do próprio GitHub Actions. Falha em qualquer
etapa interrompe a publicação. Confira o resultado e os assets pela API e
verifique que `isDraft` é falso antes de declarar a release publicada.

Se criar as notas por CLI, adapte links relativos para URLs de arquivos da tag
no GitHub, preservando os links relativos no documento do repositório.

A API precisa de acesso a `api.github.com`; o envio de assets usa
`uploads.github.com`. O proxy do ambiente já fornece autenticação Git. Não
solicite nem imprima tokens. Se o upload pelo cliente do ambiente recusar a
credencial, o workflow manual acima realiza a operação com seu token temporário.
