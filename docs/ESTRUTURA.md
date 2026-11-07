# Estrutura do repositório

O repositório guarda só o que é deste projeto. O `.gitignore` é uma lista de
permissões: tudo fica de fora, menos o que está nomeado nele. O jogo, o
código traduzido e os builds nunca entram, mesmo que estejam na mesma pasta.

## O que está no git

| Caminho | O que é |
|---|---|
| `README.md`, `CHANGELOG.md`, `CREDITS.md`, `LICENSE` | Apresentação, histórico de versões, créditos e licença (MIT) |
| `docs/` | Documentação. [ARQUIVOS_NECESSARIOS](ARQUIVOS_NECESSARIOS.md) (o que tirar do PS3), [OPCOES](OPCOES.md) (todas as variáveis), [ROADMAP](ROADMAP.md), [MODS](MODS.md), [NOTES](NOTES.md) (engenharia reversa e medições), [GITHUB_SETUP](GITHUB_SETUP.md) (configurações do repositório e como fazer um release), `RELEASE_NOTES_<versão>.md`, e `screenshots/` (os únicos binários permitidos: JPEGs pequenos) |
| `scripts/` | `setup.sh` (ps3recomp + patch + ImGui + venv), `lift.sh` (traduz o executável), `build.sh` (compila), `make_patch.sh` (regenera o patch), `check_clean.sh` e `install-hooks.sh` (a barreira contra arquivos do jogo) |
| `work/play.sh` | Abre o jogo: janela, controle, som, logs e as capturas automáticas de travada |
| `work/port/` | O projeto do jogo, compilado junto com o código traduzido (detalhe abaixo) |
| `work/tools/` | Ferramentas de tradução e diagnóstico (detalhe abaixo) |
| `work/tests/` | Regressões em shell: boot (`run_all.sh`), primeiro draw, segundo frame, contenção do SPURS, cadeias do Wwise, e o benchmark do píer (`bench_pier.sh`) |
| `work/patches/ps3recomp.patch` | Todas as mudanças no ps3recomp, sobre o upstream `42f5f8f` (veja abaixo) |
| `rpi/<tema>/` | Documentos de projeto das mudanças maiores (`REQUEST.md`, `RESEARCH.md`, `PLAN.md`): o porquê, as alternativas e as medições de cada uma |

### `work/port/`

| Arquivo | O que faz |
|---|---|
| `CMakeLists.txt` | O build: PPU traduzido com clang (x86-64-v3), SPU com `PORT_SPU_MARCH`, MinGW opcional |
| `main.cpp` | Entrada, padrões do inFAMOUS 2 (`env_default`), log em memória, captura de stalls |
| `settings.cpp`, `settings.h` | `user/settings.ini` ↔ variáveis de ambiente; mods |
| `ui.cpp`, `ui.h` | Menu principal e opções (Dear ImGui) |
| `stubs.cpp` | Ganchos e substituições específicos do jogo |
| `native_passes.c` | SSAO e iluminação deferida na GPU, no lugar dos jobs de SPU |
| `ec5f_native.c`, `jobbc11_native.c`, `job207_native.c`, `job220_native.c`, `job2025_native.c`, `job2025_kernels_native.c`, `edge_kernels_native.c` | Jobs de SPU reescritos em C, cada um com modo de comparação contra a tradução |
| `ppu_native.cpp`, `ppu_native.h` | Funções PPU do jogo trocadas por código nativo (por exemplo o `CView::SetFov` do campo de visão) |
| `port_cpu_check.c` | Confere no início se a CPU tem o conjunto de instruções do build |
| `re_trace.c` | Ganchos de rastreio para engenharia reversa (desligados sem variável) |
| `tests/settings_test.cpp` | Teste do leitor e gravador do `settings.ini` |
| `tools/spu_cc.sh`, `tools/ppu_cxx.sh` | Launchers de compilador: SPU para o clang, PPU em pedaços com memória limitada |
| `tools/kgen.py`, `kwide.py`, `kabi.py`, `gen_anim_kernels.sh`, `kw/` | Geração dos kernels nativos do job de animação |
| `windows/play.bat` | Equivalente do `play.sh` para o build MinGW (não testado) |

### `work/tools/`

| Arquivo | O que faz |
|---|---|
| `lift_spu.py` | Traduz todas as imagens SPU que o jogo roda |
| `ppu_native_entries.py` | Pós-passo no código PPU traduzido: insere os ganchos do `ppu_native.cpp` |
| `hitch_classify.py` | Agrupa as linhas `[vk hitch]` de um log por causa |
| `rsx_decode.py` | Decodifica um dump de command buffer do RSX |
| `ppm2png.py` | Converte os frames despejados em PPM |
| `re/` | Índice de símbolos do executável (`gen_symbols.py`, `who.py`, `manual_names.json`) |

## O que fica só na sua máquina

Ignorado pelo git, e nada disso pode ser distribuído:

| Caminho | O que é |
|---|---|
| `disc/` | O dump do seu disco, com o `EBOOT.elf` descriptografado |
| `ps3recomp/` | O ps3recomp com o patch aplicado (`scripts/setup.sh`; ou `PS3RECOMP_DIR`) |
| `.venv/` | O Python das ferramentas de tradução |
| `work/src/recomp/`, `work/port/spu/`, `work/out/` | O código do jogo traduzido |
| `work/port/build-vk/` | O build (`MyGameRecomp`) |
| `work/runs/` | Logs, perfis, capturas de travada (`freezes/`), dumps de job SPU |
| `gamedata/`, `hdd0/` | Saves e os dados que o jogo instala na primeira vez |
| `user/` | `settings.ini` e os backups dos saves |
| `mods/` | Mods do jogador |
| `~/.cache/ps3recomp/` | Caches de SPIR-V, pipelines e shaders do driver |

## O ps3recomp e o patch

O port é o [ps3recomp](https://github.com/sp00nznet/ps3recomp) no commit
`42f5f8f` mais o `work/patches/ps3recomp.patch`. O patch traz o runtime, o
backend Vulkan, o tradutor de shaders e as mudanças nos tradutores. Não existe
um fork publicado: o patch é a fonte.

- **Aplicar:** `scripts/setup.sh` clona o upstream, faz checkout do commit
  base e aplica o patch. Também baixa o Dear ImGui numa tag fixa.
- **Desenvolver:** as mudanças são feitas numa cópia de trabalho do ps3recomp,
  com o patch já aplicado, apontada por `PS3RECOMP_DIR`.
- **Regenerar:** `scripts/make_patch.sh [pasta]`, padrão `$PS3RECOMP_DIR` ou
  `./ps3recomp`.
  - Clona o upstream no commit base e copia a cópia de trabalho por cima.
  - Grava o diff completo em `work/patches/ps3recomp.patch`.
  - Deixa de fora: builds, `fw_spu/` (firmware), saves, configuração local
    (`.mcp.json`, `.env`), o driver de conformidade que os testes regeneram e
    o ImGui.
- **Commitar:** rode o `make_patch.sh` depois de qualquer mudança no
  ps3recomp, e commite o patch junto com a mudança do port que depende dele.
  Uma mudança que fica só na cópia de trabalho some no próximo
  `setup.sh`. O `check_clean.sh` aceita o patch até 8 MB.

## A barreira contra arquivos do jogo

`scripts/check_clean.sh` roda antes de cada commit (staged) e de cada push
(`--tracked`), pelos hooks que `scripts/install-hooks.sh` instala. Ele recusa:

- formatos do jogo e de firmware (`.elf`, `.self`, `.sprx`, `.psarc`, `.pkg`,
  `.sfo`, `.bin`, `.dat`…) e nomes como `EBOOT*`;
- binários fora de `docs/screenshots/*.jpg`;
- arquivos acima de 1 MB (512 KB para as capturas, 8 MB para o patch);
- código traduzido do jogo (as funções `func_XXXXXXXX` e `jobmod_*` que os
  tradutores geram) e saída dos tradutores;
- qualquer coisa parecida com credencial.

---

## Summary (English)

- **In git:** documentation (`docs/`), the build and safety scripts
  (`scripts/`), the game project (`work/port/`), the lifting and diagnostic
  tools (`work/tools/`), regression tests (`work/tests/`), `work/play.sh`, the
  design notes of each larger change (`rpi/`), and
  `work/patches/ps3recomp.patch`.
- **Local only:** the disc, the patched ps3recomp, the translated code,
  builds, logs, saves, settings and mods. They are ignored and must never be
  shared.
- **The patch:** the port is ps3recomp at `42f5f8f` plus
  `work/patches/ps3recomp.patch`. `scripts/setup.sh` applies it. After any
  change in the ps3recomp working tree, `scripts/make_patch.sh [dir]`
  regenerates it, and it is committed together with the port change that
  needs it.
- **The gate:** `scripts/check_clean.sh` refuses game formats, binaries,
  translated code and credentials before every commit and push.
