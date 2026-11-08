# v0.2.0

Menos engasgos e CI verde no Linux, Windows e macOS. O texto completo está no
[CHANGELOG](../CHANGELOG.md).

- Shaders e texturas novos saem do caminho do frame.
- Jobs nativos de Edge ligados por padrão; relógio TSC nas threads SPU.
- `play.sh` roda o jogo com prioridade de CPU própria.
- Corrigido vazamento de objetos Vulkan no desligamento.
- GitHub Actions: higiene, Linux, Windows (MinGW-w64), macOS arm64 e um job
  opcional de build completo no seu computador.

Não inclui arquivos do jogo nem binários. Você precisa do seu próprio disco
(veja `docs/ARQUIVOS_NECESSARIOS.md`). O jogo só roda no Linux x86-64 com
Vulkan.
