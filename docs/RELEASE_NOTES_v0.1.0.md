<!-- Texto para colar na página do release do GitHub (tag v0.1.0). Título
sugerido: "v0.1.0 — primeiro release". Marcar como pre-release. NÃO anexar
nenhum arquivo: o release é só o código-fonte que o GitHub gera sozinho. -->

> [!IMPORTANT]
> **Este release não inclui o jogo, nem dados do jogo, nem binários.** É só o
> código-fonte do port e o patch do ps3recomp. Para jogar, você precisa do
> **seu próprio disco** do inFAMOUS 2 (BCUS98125, EUA, versão 01.00), em dump
> tirado do seu PS3, e compila o port na sua máquina. O executável gerado
> contém o código do jogo traduzido e **não pode ser compartilhado**.
>
> **This release contains no game, no game data and no binaries**, only the
> port's source code and the ps3recomp patch. To play you need **your own
> inFAMOUS 2 disc** (BCUS98125, US, version 01.00), dumped from your own PS3,
> and you build the port on your machine. The resulting executable contains
> the game's translated code and **must not be shared**.

## Português

Primeiro release do port nativo do inFAMOUS 2 para PC, por recompilação
estática com o [ps3recomp](https://github.com/sp00nznet/ps3recomp). Roda no
Linux x86-64 com Vulkan, sem emulador.

### O que funciona

- Boot, vídeos, menus, cutscenes e gameplay de Empire City até New Marais.
- Áudio, saves, dublagem e textos em PT-BR.
- Controle (Xbox, DualShock 4, DualSense), e teclado e mouse.
- De 720p a 5K, com FXAA ou SMAA 1x, FSR 1, redução por área e nitidez.
- Campo de visão ajustável e limite de FPS no menu (F1 ou Esc).

### Desempenho medido

Ryzen 7 9800X3D + Radeon RX 9070 XT, Fedora 44, render em 1280×720:

| Cena | Condição | Resultado |
|---|---|---|
| Área mais pesada da cidade (~9 mil draws por frame) | bench, sem limite | **107,7 fps** (90,6 antes da última leva de otimizações) |
| Sessão de jogo normal | limite de 60 (padrão) | 59–60 fps, p99 médio de 21 ms, nenhuma travada |

Antes deste release, depois de um update do driver, havia frames de 2,7 s e
1% low de 0,7 fps. Os pipelines agora são preparados em segundo plano, e o jogo
tem um cache de shaders próprio.

### Destaques

- +19% na área pesada: a thread do RSX ficou mais barata por draw.
- Kernels Edge de vértices em C nativo, ~4× mais rápidos, sem nenhuma
  diferença em ~2,5 milhões de chamadas comparadas.
- Cadeia de pós (AA, FSR 1, redução) e escala 5K.
- Teclado e mouse (fase 1).
- Ultrawide Hor+ experimental, só por variável de ambiente.
- Caixa-preta das travas e linha `[vk hitch]` com a causa de cada frame lento.
- Saves atômicos.

A lista completa está no [CHANGELOG](https://github.com/ThiagoFraga761/infamous2-pc/blob/v0.1.0/CHANGELOG.md),
e todas as opções em [docs/OPCOES.md](https://github.com/ThiagoFraga761/infamous2-pc/blob/v0.1.0/docs/OPCOES.md).

### Como usar

1. Coloque o dump do seu disco em `disc/PS3_GAME/` e descriptografe o
   `EBOOT.BIN` para `EBOOT.elf`
   ([docs/ARQUIVOS_NECESSARIOS.md](https://github.com/ThiagoFraga761/infamous2-pc/blob/v0.1.0/docs/ARQUIVOS_NECESSARIOS.md)).
2. Compile e jogue:

```sh
scripts/setup.sh
scripts/lift.sh
scripts/build.sh
work/play.sh
```

Requisitos:

- Linux x86-64;
- CPU com AVX2 (AVX-512 por padrão para o código SPU; sem ele,
  `PORT_SPU_MARCH=x86-64-v3`);
- GPU com Vulkan;
- ~14 GB de RAM livres para compilar.

### Problemas conhecidos

- Deadlock raro no meio do jogo (a imagem para e o áudio continua), ainda sem
  causa. Se acontecer, mande o log e a pasta `work/runs/freezes/`.
- No Hor+ experimental, o HUD, os menus e os vídeos saem esticados.
- Teclado e mouse: a mira ainda passa pela aceleração do analógico do jogo.
- **Windows não testado**: o build com MinGW linka, mas ninguém o rodou.

---

## English

First release of the native inFAMOUS 2 PC port, by static recompilation with
[ps3recomp](https://github.com/sp00nznet/ps3recomp). It runs on x86-64 Linux
with Vulkan, with no emulator.

### What works

- Boot, videos, menus, cutscenes and gameplay from Empire City into New
  Marais.
- Audio, saves, and the Brazilian Portuguese dub and text.
- Controller (Xbox, DualShock 4, DualSense), keyboard and mouse.
- 720p up to 5K, with FXAA or SMAA 1x, FSR 1, area downscale and sharpening.
- Adjustable field of view and FPS limit in the menu (F1 or Esc).

### Measured performance

Ryzen 7 9800X3D + Radeon RX 9070 XT, Fedora 44, rendering at 1280×720:

| Scene | Condition | Result |
|---|---|---|
| Heaviest city area (~9,000 draws a frame) | bench, uncapped | **107.7 fps** (90.6 before the last optimization wave) |
| Normal play session | 60 cap (default) | 59–60 fps, 21 ms average p99, no hitch |

Before this release, after a driver update, there were 2.7 s frames and a
0.7 fps 1% low. Pipelines are now prepared in the background, and the game
has a shader cache of its own.

### Highlights

- +19% in the heavy area: cheaper draws on the RSX thread.
- Native C Edge vertex kernels, ~4x faster, with no difference over ~2.5
  million compared calls.
- A post chain (AA, FSR 1, downscale) and a 5K scale.
- Keyboard and mouse (phase 1).
- Experimental ultrawide Hor+, through environment variables only.
- A freeze black box, and a `[vk hitch]` line with the cause of every slow
  frame.
- Atomic saves.

Full list in the [CHANGELOG](https://github.com/ThiagoFraga761/infamous2-pc/blob/v0.1.0/CHANGELOG.md);
every option in [docs/OPCOES.md](https://github.com/ThiagoFraga761/infamous2-pc/blob/v0.1.0/docs/OPCOES.md)
(Portuguese, with an English summary).

### How to use

1. Put your disc dump in `disc/PS3_GAME/` and decrypt `EBOOT.BIN` to
   `EBOOT.elf`
   ([docs/ARQUIVOS_NECESSARIOS.md](https://github.com/ThiagoFraga761/infamous2-pc/blob/v0.1.0/docs/ARQUIVOS_NECESSARIOS.md)).
2. Build and play:

```sh
scripts/setup.sh
scripts/lift.sh
scripts/build.sh
work/play.sh
```

Requirements:

- x86-64 Linux;
- a CPU with AVX2 (AVX-512 by default for the SPU code; without it,
  `PORT_SPU_MARCH=x86-64-v3`);
- a Vulkan GPU;
- ~14 GB of free RAM to build.

### Known problems

- A rare mid-game deadlock (the picture stops, the audio goes on), cause not
  found yet. If it happens, send the log and the `work/runs/freezes/` folder.
- In the experimental Hor+, the HUD, menus and videos are stretched.
- Keyboard and mouse: aiming still goes through the game's stick
  acceleration.
- **Windows untested**: the MinGW build links, but nobody has run it.

inFAMOUS 2 is © Sony Interactive Entertainment / Sucker Punch Productions.
This project is not affiliated with or endorsed by them.
