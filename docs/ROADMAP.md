# Roadmap

A meta é **120 fps estáveis**, medidos pelo tempo de frame e não pela média,
em qualquer área do jogo, e depois **4K a 120 com upscaling**. Estado em
07/11/2026 (v0.1.0).

## Feito

- **O jogo inteiro roda:** boot, vídeos, menus, cutscenes e gameplay de Empire
  City até New Marais, com áudio, controle, saves e dublagem em PT-BR.
- **Todo o código SPU traduzido para C:**
  - o gerenciador de jobs;
  - os 27 módulos de job;
  - os plugins de oclusão;
  - as cadeias de job do Wwise;
  - o Bink e o zlib.
- **Jobs nativos.** Versões em C escritas à mão, cada uma com modo de
  comparação:
  - `ec5f`, o walker de cena `bc11` e o culler de instâncias `207`;
  - a geometria Edge (skin e cull);
  - a descompressão e a compressão de vértices Edge. Estas estão ligadas por
    padrão desde a v0.1.0: zero diferenças em ~2,5 milhões de chamadas.
- **Na GPU no lugar dos jobs de SPU:** oclusão ambiente e iluminação deferida.
- **Imagem:**
  - escala de 1× a 4× (até 5K);
  - FXAA, SMAA 1x, FSR 1, redução por área e nitidez;
  - filtro anisotrópico 16x e partículas em resolução cheia;
  - campo de visão ajustável;
  - água, partículas e profundidade de fragmento corretas.
- **Menu principal** (Continuar, Novo jogo, Carregar jogo, Opções, Sair) e
  opções no jogo (F1/Esc), gravadas em `user/settings.ini`.
- **Controles:** controle via SDL2 com vibração completa, e teclado e mouse
  (fase 1).
- **Desempenho:**
  - frame em voo na GPU;
  - geometria residente na VRAM;
  - thread de gravação Vulkan;
  - sem esperas da GPU ao descartar texturas;
  - afinidade de threads por núcleo físico;
  - leva 1 (07/11): fast path da FIFO, estado de draw repetido pulado,
    aquecimento de pipelines;
  - leva 2 (07/11): pipelines novos fora do frame, esperas nativas da thread
    principal (opt-in);
  - leva 3 (07/11): custo por draw da thread do RSX, +19% na área pesada
    (90,6 para 107,7 fps).
- **Estabilidade:**
  - limite de 60 fps por padrão;
  - cache de shaders do driver próprio, que acabou com os frames de 2,7 s
    depois de update do Mesa;
  - saves atômicos;
  - caixa-preta das travas (estado do runtime a cada stall);
  - linha `[vk hitch]` com a causa de cada frame lento.

## Em andamento

| Item | Situação |
|---|---|
| Deadlock raro no meio do jogo | A thread principal espera uma event flag, e a job chain `0x00B19600` fica parada no guard `0x00B15000`: o evento de "job pronto" do SPU para o PPU se perde. Visto em 16, 17 e 18/10, uma vez a cada 15–60 min. A caixa-preta já grava fences, guards, chains e flags na trava; falta uma trava acontecer com ela ligada |
| Teto de CPU na área pesada | Com a leva 3, a thread principal, a do RSX, a de gravação e os SPUs ficam todos em 85–91%. Nenhuma otimização isolada sobe muito mais o teto |
| Kernel de índices Edge (0xC7B8) | Escrito e testado fora do jogo (`PS3RECOMP_NATIVE_EDGE_IDX`); falta a comparação em jogo (modo 2) antes de ligar por padrão |
| `PS3_MAIN_WAITS` | Com limite de 60, a main cai de 99% para 46% de CPU, com p99 de 16,9 para 17,7 ms. Falta decidir se vira padrão |
| `PS3RECOMP_BQ_TEXBUILD` | Ligado no fork, fixado em 0 no port até um A/B de p99 numa área nova |
| Ultrawide Hor+ | Fases 1 (FOV), 2 (escala por eixo) e 3 (HUD, menus e vídeos em 16:9 no centro, opção no F1) feitas; custa ~7% a 3440×1440 na área pesada. Falta conferir o HUD numa janela ultrawide de verdade e a fase 3b (marcadores presos ao mundo) |
| Planos de corte e PCF largo | Implementados e opt-in (`PS3RECOMP_CLIP_PLANES`, `PS3RECOMP_SHADOW_PCF`); falta conferir em jogo se o reflexo da água usa o 0x1478 e qual é o sinal certo |

## Próximos

**Base:**

1. Achar e corrigir o deadlock, com o estado que a caixa-preta grava.
2. Medir o teste do relógio do jogo (`0x9BFC74`) antes de liberar 120 como
   padrão: confirmar que a lógica não acelera acima de 60.
3. Presenter fora da thread do RSX: o limitador hoje dorme na thread que
   drena a FIFO.
4. Um rebuild PPU total só, com o VMX sem stall de store-forwarding e o
   inline do `ps3_indirect_call`, para aliviar a thread principal.
5. Reescrita do sistema de objetos (`rpi/decomp-scene`), que é o caminho
   para 120 nas vistas de 11 mil draws.

**Ultrawide e imagem:**

1. Conferir o HUD centralizado numa janela ultrawide de verdade; depois, os
   marcadores do HUD presos ao mundo (fase 3b) e o culling alargado.
2. AA antes do HUD (o SMAA hoje amolece o texto).
3. Upscaler temporal (FSR 3.1 ou XeSS no Vulkan), se a GPU apertar em 4K ou
   em 3440.
5. Distância de desenho e LOD maiores, HDR e pacote de texturas em alta.

**Controles:**

1. Mira 1:1 no mouse, escrevendo yaw/pitch da câmera (fase 2, precisa de
   engenharia reversa).
2. Remapear teclas pelo menu.
3. Controle da porta 0 fixado pelo `SDL_JoystickID` (um pad virtual da Steam
   não rouba a vaga).

**Mods** (ideias do Thiago, 18/10, em ordem):

| # | Mod | Viabilidade |
|---|---|---|
| 1 | Central de mods: baixar e instalar mods de dentro do jogo | Boa: um catálogo (JSON) num repositório e zips em `mods/`; o carregador já existe (docs/MODS.md) |
| 2 | Mapa mostrando todos os fragmentos (cristais) | Boa, com engenharia reversa: achar a lista de fragmentos e o código dos ícones do mapa; o código do jogo é recompilado, então dá para trocar uma função por C nativo (como no FOV) |
| 3 | Melhorias gerais do mapa (zoom, marcadores) | Mesma base do 2 |
| 4 | Co-op (tela dividida ou online) | Muito difícil: o jogo não tem segundo jogador nem câmera dupla. Só uma investigação curta antes de decidir |

Os mods 2 e 3 precisam de "mods de código": um ponto de extensão para trocar
funções PPU por código nativo, além dos mods de arquivo de hoje. O wrap do
`CView::SetFov` (`work/port/ppu_native.cpp`) já é um exemplo desse mecanismo.

**Windows:** o build cruzado com MinGW linka, mas nunca rodou. Falta testar,
ligar o SDL2 no build e trocar o rastreamento de escrita (hoje feito com
`mprotect`) por um equivalente.

---

# Roadmap (English)

The goal is **a steady 120 fps**, by frame time rather than average,
everywhere in the game; then **4K at 120 with upscaling**. State on
2026-11-07 (v0.1.0).

## Done

- **The whole game runs:** boot, videos, menus, cutscenes and gameplay from
  Empire City into New Marais, with audio, controller, saves and the
  Brazilian Portuguese dub.
- **All SPU code translated to C.** The heaviest jobs also have hand-written C
  versions: `ec5f`, the `bc11` scene walker, the `207` instance culler, Edge
  geometry, and Edge vertex decompression and compression, the last two on by
  default since v0.1.0.
- **AO and lighting on the GPU.**
- **Picture:** 1×–4× scale; FXAA, SMAA 1x, FSR 1, area downscale,
  sharpening; 16x AF; full-resolution particles; adjustable FOV.
- **Main menu and in-game options** (F1/Esc).
- **Controls:** pad with full rumble, keyboard and mouse (phase 1).
- **Performance:**
  - the earlier work: a frame in flight, geometry resident in VRAM, a
    recording thread, no GPU waits on texture release, thread pinning;
  - three waves on 2026-11-07, the last one +19% in the heavy area
    (90.6 -> 107.7 fps).
- **Stability:**
  - a 60 fps cap by default;
  - a driver shader cache of the game's own;
  - atomic saves;
  - a freeze black box;
  - a `[vk hitch]` line for every slow frame.

## In progress

- **Rare mid-game deadlock:** the main thread waits on an event flag while
  job chain `0x00B19600` sits at guard `0x00B15000`. The black box now records
  the state; it needs a freeze to happen with it on.
- **CPU ceiling in the heavy area:** main, RSX, recording thread and the SPUs
  are all at 85–91%.
- **Edge index kernel, `PS3_MAIN_WAITS`, `PS3RECOMP_BQ_TEXBUILD`:** written,
  opt-in, waiting for in-game measurements before they become defaults.
- **Ultrawide Hor+:** phases 1-3 done (FOV, per-axis scale, centred 16:9 HUD, F1 option); ~7% at 3440x1440 in the heavy area. Phase 3b (world-pinned markers) left.
- **User clip planes and wider shadow PCF:** opt-in, not checked in game.

## Next

- **Base:**
  - fix the deadlock;
  - check the game clock before uncapping to 120 by default;
  - move the presenter off the RSX thread;
  - one full PPU rebuild for the VMX and indirect-call work;
  - the object-system rewrite for the 11k-draw views.
- **Ultrawide and picture:**
  - checking the centred HUD in a real ultrawide window, then world-pinned
    markers (phase 3b);
  - AA before the HUD;
  - a temporal upscaler if the GPU gets tight;
  - longer draw distance, HDR, high-resolution textures.
- **Controls:**
  - 1:1 mouse aiming;
  - rebinding in the menu;
  - port 0 pinned by joystick ID.
- **Mods:** an in-game mod hub, a map of every blast shard, map improvements,
  and a short co-op investigation (see the table above).
- **Windows:** the MinGW build links but has never run.
