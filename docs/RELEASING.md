# Preparar e publicar uma versão

A publicação no GitHub é uma etapa separada: **somente executar push, envio de
 tags e criação de release depois da autorização final do responsável**.
Builds e CI não publicam automaticamente. A publicação do ambiente na nuvem
é independente e não faz parte deste procedimento.

1. Atualize `VERSION`, versão da interface, README, changelog e notas da release.
2. Execute todos os testes do CONTRIBUTING e audite o ZIP. Confira ícone 256x256,
   estrutura de instalação e ausência de credenciais/configurações pessoais.
3. Revise e faça commit local; prepare a tag anotada local `v1.0.1` nesse commit.
4. Gere o ZIP a partir desse estado, revise o SHA256 e os arquivos da release.
5. Solicite a autorização final com repositório, commit, tag, notas e assets.

Somente após essa autorização, para a release 1.0.1:

```sh
# Confirmar que a main remota ainda permite um push normal; nunca usar --force.
git ls-remote origin refs/heads/main refs/tags/v1.0.1
git push origin HEAD:main
git push origin v1.0.1
gh release create v1.0.1 \
  dist/DriveBrowser-1.0.1-arm64.zip \
  dist/DriveBrowser-1.0.1-arm64.zip.sha256 \
  --repo kenfabioh/trimui-drive-browser --verify-tag \
  --title 'Drive Browser 1.0.1 — TrimUI Smart Pro' \
  --notes-file docs/RELEASE-v1.0.1.md
```

Reconfira publicação e assets pela resposta do GitHub e por leitura independente.
Não declare publicado só porque o commit, ZIP ou tag existem localmente.
O proxy Git HTTPS já fornece acesso Git; não solicite token por ausência de
login no gh. A API precisa de acesso a `api.github.com` e uploads a
`uploads.github.com`. Se esses destinos estiverem bloqueados, preserve o pacote
local e resolva o acesso nas configurações do ambiente, sem expor credenciais.
