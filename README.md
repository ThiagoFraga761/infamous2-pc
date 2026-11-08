<p align="center"><img src="docs/screenshots/banner.jpg" alt="inFAMOUS 2 — port nativo para PC" width="100%"></p>

[![CI](https://github.com/ThiagoFraga761/infamous2-pc/actions/workflows/ci.yml/badge.svg)](https://github.com/ThiagoFraga761/infamous2-pc/actions/workflows/ci.yml)


<h1 align="center">inFAMOUS 2 — port nativo para PC (Linux, Vulkan)</h1>

<p align="center">
  <img src="https://img.shields.io/badge/status-jog%C3%A1vel%2C%20em%20desenvolvimento-orange" alt="Status: jogável, em desenvolvimento">
  <a href="https://github.com/ThiagoFraga761/infamous2-pc/releases"><img src="https://img.shields.io/github/v/release/ThiagoFraga761/infamous2-pc?include_prereleases&label=release" alt="Release"></a>
  <img src="https://img.shields.io/badge/plataforma-Linux%20x86--64-1793D1?logo=linux&logoColor=white" alt="Linux x86-64">
  <img src="https://img.shields.io/badge/GPU-Vulkan-AC162C?logo=vulkan&logoColor=white" alt="Vulkan">
  <img src="https://img.shields.io/badge/PS3-BCUS98125-003791?logo=playstation&logoColor=white" alt="PS3 BCUS98125">
  <img src="https://img.shields.io/badge/at%C3%A9-4K-6f42c1" alt="Até 4K">
  <img src="https://img.shields.io/badge/dublagem-PT--BR-009c3b" alt="Dublagem PT-BR">
  <img src="https://img.shields.io/badge/c%C3%B3digo-MIT-blue" alt="Código MIT">
</p>

<p align="center"><b>inFAMOUS 2 PC port</b> · native on Linux · PS3 static recompilation · Vulkan · up to 4K · unlocked frame rate · keyboard and mouse · Brazilian Portuguese dub</p>

<p align="center">
  <a href="#english"><b>English below</b></a> ·
  <a href="docs/ARQUIVOS_NECESSARIOS.md">Arquivos necessários</a> ·
  <a href="docs/OPCOES.md">Opções</a> ·
  <a href="docs/ROADMAP.md">Roadmap</a> ·
  <a href="CHANGELOG.md">Changelog</a> ·
  <a href="docs/NOTES.md">Notas técnicas</a>
</p>

> **Jogável, em desenvolvimento.** O jogo inicia, passa pelos vídeos, menus e
> cutscenes, e o gameplay segue de Empire City até New Marais, com som,
> controle, teclado e mouse, saves e dublagem em português. Na área mais
> pesada medida (~9 mil draws por frame) ele passa de 100 fps sem limite e
> segura os 60 fps do limite padrão. Veja o [Status](#status).

**inFAMOUS 2** (PS3, BCUS98125, versão 01.00) rodando nativo no Linux, sem
emulador. O código do jogo, PowerPC (PPU) e SPU, é traduzido para C antes da
compilação por recompilação estática com o
[ps3recomp](https://github.com/sp00nznet/ps3recomp). O sistema do PS3 é
reimplementado, e a GPU (RSX) roda os shaders do próprio jogo traduzidos para
Vulkan.

> **Nenhum arquivo do jogo vem com este projeto, e não há binários para
> baixar.** Para jogar, você precisa da sua própria cópia, tirada do seu PS3. O
> build transforma o **seu** executável em código na **sua** máquina, e esse
> código não pode ser distribuído.

| | |
|---|---|
| ![Cole no píer de Empire City](docs/screenshots/01-pier.jpg) | ![Cutscene de abertura](docs/screenshots/02-cutscene.jpg) |
| ![Passarela no pântano de New Marais](docs/screenshots/03-new-marais.jpg) | ![Ruas de New Marais](docs/screenshots/04-cidade.jpg) |

![Empire City vista do píer](docs/screenshots/05-empire-city.jpg)

<sub>Capturas direto da GPU no Linux, com uma Radeon RX 9070 XT, em 1280×720.
As de Empire City foram renderizadas em 2× (2560×1440) e reduzidas.</sub>

## Novidades da v0.1.0 (a v0.2.0 está no [CHANGELOG](CHANGELOG.md))

O histórico completo está no [CHANGELOG.md](CHANGELOG.md).

- **+19% na área mais pesada.** A thread do RSX ficou mais barata por draw: a
  FIFO é percorrida em lote, e o estado de render, a superfície, os programas
  e as texturas são reaproveitados enquanto não mudam. Medido na mesma cena:
  de 90,6 para 107,7 fps sem limite.
- **Edge nativo.** A descompressão e a compressão de vértices do Edge, que
  rodavam como código SPU traduzido, agora são C escrito à mão. Em ~2,5
  milhões de chamadas comparadas no jogo não houve nenhuma diferença, e cada
  chamada ficou ~4× mais rápida (1,15 µs contra 4,9 µs).
- **Sem as travadas de segundos.** Em 06/11, depois de um update do driver,
  havia frames de 2,7 s e 1% low de 0,7 fps. Os pipelines já vistos são
  recriados em segundo plano na partida, e os novos são montados fora do
  frame. O `play.sh` também usa um cache de shaders do driver só para o jogo.
  Na sessão de 07/11, com limite de 60: 59–60 fps, p99 médio de 21 ms e
  nenhuma travada.
- **Limite de FPS no menu**, com 60 como padrão. Com `PS3_MAIN_WAITS=1`
  (opcional), a thread principal espera de verdade em vez de girar: com o
  limite de 60, ela cai de 99% para 46% de CPU.
- **Imagem:**
  - anti-aliasing FXAA ou SMAA 1x;
  - FSR 1 para ampliar até a janela;
  - média por área para reduzir de 4K/5K;
  - nitidez ajustável;
  - escala 4× (5K, supersampling).
- **Campo de visão ajustável** no menu, do FOV do jogo até +30%, ao vivo.
- **Teclado e mouse (fase 1):** WASD e o mouse viram os analógicos, junto com
  o controle. Sensibilidade, inversão do eixo e teclas no menu e no
  `settings.ini`.
- **Ultrawide experimental:** F1 > Vídeo > Ultrawide desenha o mundo no formato
  do monitor (Hor+) e põe o HUD, os menus e os vídeos em 16:9 no centro. Na área
  pesada a 3440×1440 custa ~7% (108 → 101 fps). Veja
  [docs/OPCOES.md](docs/OPCOES.md#2-ultrawide-e-fov).
- **Caixa-preta das travas:** a cada parada, o estado do runtime (fences, job
  chains, SPURS, eventos) vai para `work/runs/freezes/`, e cada frame lento
  gera uma linha `[vk hitch]` com a causa.
- **Saves gravados por inteiro ou nada:** um crash no meio do save mantém o
  save anterior.
- **Esc** abre o menu, como o F1, e o motor pequeno da vibração voltou a
  funcionar (ficava em 0,4% da força).

Tudo o que é novo e vem ligado tem um kill switch por variável de ambiente
([docs/OPCOES.md](docs/OPCOES.md#6-kill-switches-das-três-levas-de-07112026)).

## Status

| Área | Estado |
|---|---|
| Boot, vídeos, menus e cutscenes | ✅ Funciona |
| Gameplay | ✅ De Empire City até New Marais, com poderes, escalada e missões do começo do jogo |
| Áudio (Wwise no SPU) | ✅ Funciona |
| Saves | ✅ Salvar, Continuar e autosave; gravação atômica |
| Controle | ✅ Xbox, DualShock 4 e DualSense via SDL2, com vibração |
| Teclado e mouse | 🔧 Fase 1: jogável; a mira ainda passa pela aceleração do analógico do jogo |
| Dublagem e textos em português | ✅ `PS3_SYS_LANG=pt` (padrão) |
| Resolução acima de 720p | ✅ 1440p, 4K e 5K (supersampling) |
| AA e upscaling | ✅ FXAA, SMAA 1x, FSR 1, redução por área |
| Campo de visão | ✅ Ajustável no menu |
| Ultrawide (Hor+) | 🧪 Experimental: F1 > Vídeo > Ultrawide; HUD em 16:9 no centro |
| Desempenho | ✅ 60 fps estáveis com o limite padrão; 100+ sem limite na área mais pesada medida |
| Engasgos | 🔧 As travadas de segundos sumiram; com cache frio, a primeira vez de um efeito ainda pode custar um frame |
| Travamentos | 🔧 Deadlock raro, sem causa achada; a caixa-preta grava o estado quando acontece |
| Imagem | 🔧 Correta na maior parte; falhas pontuais ainda aparecem |
| Linux | ✅ Testado no Fedora 44 |
| Windows e macOS | ❌ Jogo não suportado: o toolkit compila e linka (CI), mas o jogo nunca foi rodado lá |

### Desempenho

Ryzen 7 9800X3D + Radeon RX 9070 XT, Fedora 44, render em 1280×720:

| Cena | Condição | Resultado | Medido em |
|---|---|---|---|
| Área pesada da cidade, na praça onde o save atual começa (~9 mil draws por frame) | bench automático, parado, sem limite | **107,7 fps** (90,6 com as otimizações da v0.1.0 para o RSX desligadas) | 07/11/2026 |
| Sessão de jogo normal | limite de 60 (padrão) | 59–60 fps, p99 médio de 21 ms, nenhuma travada | 07/11/2026 |
| Píer de Empire City (tutorial, andando) | sem limite | ~120–130 fps | out/2026 |
| Pântano de New Marais | sem limite | ~100 fps | out/2026 |
| Menu 3D | sem limite | ~115 fps | out/2026 |

- As cenas de outubro foram medidas antes das otimizações de 07/11 e não
  foram medidas de novo.
- Na área pesada, as quatro threads quentes (principal do PPU, RSX, gravação
  Vulkan e SPUs) ficam todas em 85–91%: nenhuma otimização isolada sobe muito
  mais o teto.
- Quem limita é a CPU, não a GPU. Em outubro, o pântano de New Marais rodou em
  4K no mesmo fps que em 720p.
- Os detalhes de onde vai o tempo de cada frame estão em
  [docs/NOTES.md](docs/NOTES.md).

### PS3 × este port

| | PS3 | Este port |
|---|---|---|
| Resolução | 1280×720 | 720p, 1440p, 4K ou 5K, com FSR 1 ou redução por área até a janela |
| Taxa de quadros | alvo de 30 fps | 60 com o limite padrão; ~100–130 sem limite |
| Anti-aliasing | — | FXAA ou SMAA 1x |
| Campo de visão | fixo | até +30% |
| Monitor ultrawide | — | barras pretas, esticado ou Hor+ experimental |
| Controles | DualShock 3 | controle via SDL2, teclado e mouse |
| Execução | Cell (PPU + SPUs) e RSX | código traduzido para x86-64, GPU em Vulkan |

## Requisitos

**Arquivos do jogo.** Tudo vem do **seu** PS3, e nada disso está no
repositório. O passo a passo está em
[docs/ARQUIVOS_NECESSARIOS.md](docs/ARQUIVOS_NECESSARIOS.md).

- **O disco:** inFAMOUS 2, **BCUS98125 (EUA), versão 01.00**, em dump do seu
  disco, com `disc/PS3_GAME/` na raiz do repositório.
- **O executável:** `disc/PS3_GAME/USRDIR/EBOOT.elf`, o `EBOOT.BIN` do disco
  descriptografado (por exemplo no RPCS3). O `scripts/lift.sh` confere o
  SHA-256.
- **O firmware:** o PUP oficial do PS3 instalado no RPCS3 (`dev_flash`).

**PC:**

- Linux x86-64 (testado no Fedora 44).
- CPU com AVX2. O código SPU é compilado com AVX-512 por padrão; para CPU sem
  ele, `PORT_SPU_MARCH=x86-64-v3`.
- 8 núcleos com SMT para a afinidade de threads (`PS3_HOT_PIN`); com menos, o
  jogo roda, só que sem esse ganho.
- GPU com Vulkan e uma fila gráfica que também faz compute.
- ~14 GB de RAM livres e ~3 GB de disco para compilar, além do jogo.
- Pacotes no Fedora (adapte para a sua distro): `gcc gcc-c++ clang cmake
  ninja-build python3 SDL2-devel vulkan-headers vulkan-loader-devel
  glslang-devel libpng-devel`. O libpng só desenha a arte do disco no menu;
  sem ele, o fundo é liso.

## Como compilar

Não há binários para baixar: o código traduzido é o do jogo e só pode existir
na sua máquina.

```sh
scripts/setup.sh    # ps3recomp no commit base + o patch deste port, Dear ImGui, venv Python
scripts/lift.sh     # traduz o PPU e os SPUs do SEU EBOOT.elf
scripts/build.sh    # compila em work/port/build-vk
```

- A tradução leva alguns minutos. A primeira compilação é demorada, porque o
  código traduzido do PPU é grande.
- O `build.sh` roda num escopo de memória de 14 GB do systemd, para não
  derrubar o desktop.
- `JOBS=<n>` muda o paralelismo. `PORT_PPU_GCC=1` compila o PPU com GCC.
- `PS3RECOMP_DIR=<pasta>` usa um ps3recomp que já está em outro lugar.
- Para contribuir: `scripts/install-hooks.sh` instala os hooks que rodam o
  `scripts/check_clean.sh` antes de cada commit e push. O mapa do repositório
  está em [docs/ESTRUTURA.md](docs/ESTRUTURA.md).

## Como jogar

```sh
work/play.sh            # janela + controle + som, com o menu principal
work/play.sh --play     # direto no jogo, sem o menu do port
work/play.sh --title    # direto na tela de jogo novo do inFAMOUS 2
```

O controle precisa estar ligado antes de abrir. Cada sessão grava um log em
`work/runs/play-<data>.log`.

### Menu principal

Ao abrir, aparece o menu do port, sobre a arte do próprio disco (`PIC1.PNG`).
Ele funciona com controle, teclado ou mouse:

| Item | O que faz |
|---|---|
| **Continuar** | entra no save mais recente (mostra nome, progresso e data) |
| **Novo jogo** | abre a tela de jogo novo do próprio inFAMOUS 2 |
| **Carregar jogo** | lista os seus saves; escolha um |
| **Opções** | as abas abaixo |
| **Sair** | fecha |

O inFAMOUS 2 não tem menu principal: com save, ele entra direto no autosave;
sem save, mostra só "Começar um jogo novo". O menu do port faz esse papel.
Antes de **Novo jogo** ou de carregar um save que não é o autosave, os saves
são copiados para `user/backups`, porque o próximo autosave do jogo grava por
cima do slot de autosave, como no console. Dentro do jogo: **Start → Dados
salvos**.

### Opções (F1 no jogo)

**F1**, **Esc** ou **Select+Start** segurados abrem as opções no meio do jogo.
Tudo fica gravado em `user/settings.ini`.

| Aba | Opções |
|---|---|
| **Vídeo** | Resolução (720p, 1440p, 4K, 5K com supersampling) · Anti-aliasing (desligado, FXAA, SMAA 1x) · Ampliar para a janela (bilinear, FSR 1) · Reduzir para a janela (bilinear, média por área) · Nitidez (automática, ou de 0 a 1) · Tela cheia · VSync · Esticar a imagem (sem barras) · Campo de visão (do jogo até +30%) · Monitor (com mais de um) |
| **Idioma** | Dublagem e legendas/textos, separados (dá para misturar) |
| **Desempenho** | Predefinição (Desempenho 720p, Equilibrado 1440p, Qualidade 4K) · Limite de FPS (sem limite, 30, 40, 60, 80, 120, 240, taxa do monitor) · Contador de FPS na tela · Thread de gravação Vulkan |
| **Mods** | Partículas em resolução cheia · Filtro anisotrópico (do jogo, 2x a 16x) · Mods instalados ([docs/MODS.md](docs/MODS.md)) |
| **Controle e saves** | Controle detectado e teste de botões · Abrir a pasta dos saves · Fazer backup dos saves · Abrir a pasta dos logs |
| **Teclado e mouse** | Jogar com teclado e mouse · Sensibilidade do mouse (0,10–5,00) · Inverter o eixo vertical · Início do giro (0–60%) · Lista de teclas |

- Resolução, monitor, idioma e a thread de gravação valem a partir da próxima
  vez que o jogo abrir. O resto muda na hora.
- Uma variável de ambiente ganha do arquivo, e o campo dela fica cinza no menu
  (`PS3RECOMP_RES_SCALE=2 work/play.sh`). A lista completa está em
  [docs/OPCOES.md](docs/OPCOES.md).

### Controles

**Controle** (Xbox, DualShock 4, DualSense): como no PS3. **Select+Start**
segurados abrem o menu do port.

**Teclado e mouse**, junto com o controle. O mouse é capturado no primeiro
clique ou tecla; Alt+Tab e F1 o soltam.

| Tecla | Botão do PS3 |
|---|---|
| **W A S D** | analógico esquerdo (andar) |
| **Mouse**, ou **I J K L** | analógico direito (câmera) |
| **Botão esquerdo** / **direito** | R1 (atirar) / L1 (mirar) |
| **Espaço** | Xis |
| **Shift esquerdo** ou **Backspace** | Bola |
| **F** ou **botão lateral 4** | Quadrado |
| **E** ou **botão lateral 5** | Triângulo |
| **Q** / **R** | R2 / L2 |
| **Ctrl esquerdo** / **V** ou **botão do meio** | L3 / R3 |
| **Enter** ou **P** / **Tab** | Start / Select |
| **Setas** ou **1 2 3 4** | direcional |

Para trocar teclas: `[keyboard] map` no `user/settings.ini`, no formato
`Tecla=BOTÃO` (por exemplo `F=SQUARE Q=NONE`). As ações `WALK` (andar devagar)
e a roda do mouse (`WheelUp`, `WheelDown`) existem, mas não têm tecla no mapa
padrão.

**Teclas do port:**

| Tecla | O que faz |
|---|---|
| **F1** ou **Esc** | abre e fecha o menu |
| **F11** ou **Alt+Enter** | tela cheia / janela |

## Idioma

O disco traz voz e texto de todas as regiões (`brazilian.psarc`,
`brazilian_text.psarc`, `latino.psarc`…), e o jogo escolhe pelo idioma do
console:

- O jogo foi feito antes de o PS3 ter "Português (Brasil)" (número 17), então
  trata esse número como desconhecido e cai no inglês.
- O caminho para o PT-BR é o idioma "Português" (7), que a lista da região
  norte-americana liga aos arquivos `brazilian`.
- O port escolhe esse número sozinho a partir do idioma das legendas, e serve
  os arquivos de voz e de texto que você escolheu na aba Idioma.

## Saves e dados

- Os saves ficam em `gamedata/dev_hdd0/home/00000001/savedata/`. Cada arquivo
  é gravado ao lado e renomeado por cima, então um crash no meio do save
  mantém o anterior.
- `PS3_SAVEDATA_ROOT=<pasta>` usa outra pasta. Os testes e benchmarks usam
  isso para nunca tocar nos saves de quem joga.
- As opções ficam em `user/settings.ini`, e os backups dos saves em
  `user/backups/`.

## Logs e relatórios

Cada sessão do `work/play.sh` grava `work/runs/play-<data>.log`, com uma linha
`[rsx fps]` a cada 10 s. As capturas são automáticas:

- **Frame lento:** uma linha `[vk hitch]` com a causa (pipeline novo, espera
  de GPU, textura criada, o que cada thread esperou).
  `work/tools/hitch_classify.py <log>` agrupa essas linhas por causa.
- **Parada de 2,5 s ou mais:** a pilha de todas as threads e o estado do
  runtime vão para `work/runs/freezes/<log>/`, no momento e de novo 15 s
  depois.
- **Travou de vez:** depois de 15 s sem frame, também um perf de 5 s. Se o
  jogo travar, espere uns 20 s antes de fechar.
- **Ficou lento:** depois de 30 s abaixo de 70 fps (ou 5 abaixo do limite,
  se ele for menor), um perfil de 20 s (até três por sessão).

Para relatar um problema, mande o log e a pasta de `work/runs/freezes/`
correspondente. Eles não contêm arquivos do jogo.

## Problemas conhecidos

- **Deadlock raro no meio do jogo:** a imagem para e o áudio continua. A
  thread principal fica esperando uma event flag, e a job chain `0x00B19600`
  fica parada no guard `0x00B15000`. Foi visto em 16, 17 e 18/10, uma vez a
  cada 15–60 min de jogo. A causa ainda não foi achada; a sessão de 07/11 não
  teve nenhum. Se acontecer, a caixa-preta acima grava o estado.
- **Ultrawide Hor+ é experimental:** o HUD centralizado só foi conferido em
  bench sem janela, e os marcadores presos ao mundo (alvos, ícones sobre
  pessoas) ainda usam a posição 16:9. Desligado, o 16:9 aparece com barras ou
  esticado, e o campo de visão do menu funciona normalmente.
- **Teclado e mouse, fase 1:** a mira passa pela aceleração do analógico do
  próprio jogo, então não é 1:1. Remapear teclas só pelo `settings.ini`.
- **O anti-aliasing roda depois do HUD,** e o texto do HUD fica um pouco mais
  macio.
- **Cache frio:** na primeira sessão, ou depois de um update do driver, a
  primeira vez de cada efeito ainda pode custar um frame. As seguintes saem do
  cache.
- **Windows não testado:** existe um build cruzado com MinGW
  (`work/port/windows/play.bat`) que linka, mas ninguém o rodou. O
  rastreamento de escrita da memória do RSX e a captura de pilhas são só
  Linux.

## Como funciona

- **PPU:**
  - As ~39 mil funções do executável viram C (`ppu_lifter.py`).
  - O resultado é compilado com clang para x86-64-v3, com os acessos à memória
    do PS3 inline.
  - Os módulos do sistema são reimplementados (HLE).
  - Algumas funções do jogo têm substitutos nativos (`work/port/ppu_native.cpp`),
    como o `CView::SetFov` do campo de visão.
- **SPU:** cada imagem SPU que o jogo roda é traduzida para C e compilada com
  SIMD do x86 (SSE4.1, AVX2, AVX-512):
  - o gerenciador de jobs;
  - os 27 módulos de job;
  - os plugins de oclusão;
  - as cadeias de job do Wwise;
  - o Bink e o zlib.

  Os jobs mais pesados, como o Edge, o `ec5f`, o walker de cena e o culler de
  instâncias, têm versões em C escritas à mão. Cada uma tem um modo que a
  compara com a tradução.
- **RSX:**
  - Um motor de desenho lê o banco de registradores do RSX e roda os vertex e
    fragment programs do próprio jogo, decompilados para HLSL e compilados
    para SPIR-V.
  - O backend Vulkan grava os comandos numa thread própria.
  - A cadeia de pós (AA, FSR 1, redução) roda entre o frame e a janela.
- **Passadas de GPU:** dois jobs de SPU, a oclusão ambiente (SSAO) e a
  iluminação deferida em tiles, rodam na GPU. Cada um foi conferido contra a
  saída do SPU.
- **Rastreamento de escrita:**
  - A memória local do RSX é vigiada por proteção de página.
  - Texturas e geometria que ninguém reescreveu não são decodificadas de
    novo.

Os detalhes, com endereços e medições, estão em [docs/NOTES.md](docs/NOTES.md).

## Próximos passos

Veja [docs/ROADMAP.md](docs/ROADMAP.md). A meta é **120 fps estáveis**, medidos
pelo tempo de frame e não pela média. Antes disso vem:

- achar a causa do deadlock raro;
- medir em jogo o kernel de índices do Edge e as esperas da thread principal,
  para ligá-los por padrão;
- HUD centralizado no ultrawide;
- mira 1:1 no mouse.

## Perguntas frequentes

**Existe inFAMOUS 2 para PC?** Não oficialmente. Este projeto roda o jogo do
PS3 nativo no Linux, sem emulador, por recompilação estática.

**Dá para jogar inFAMOUS 2 no PC sem RPCS3?** Sim, é o que este port faz, com
a sua própria cópia do jogo. O RPCS3 só é usado para descriptografar o
executável e instalar o firmware.

**Tem um executável pronto para baixar?** Não, e não vai ter: o executável
contém o código do jogo traduzido. Cada pessoa compila a partir do próprio
disco.

**Roda no Windows?** Ainda não. Veja os [problemas conhecidos](#problemas-conhecidos).

**O jogo vem junto?** Não. Nenhum arquivo do jogo faz parte do projeto.

## Créditos e aviso legal

- **[ps3recomp](https://github.com/sp00nznet/ps3recomp)**, de sp00nznet e
  colaboradores (MIT): este port é o ps3recomp mais o
  `work/patches/ps3recomp.patch`.
- **[rpcs3-infamous2](https://github.com/ttmx/rpcs3-infamous2)**, de ttmx: a
  engenharia reversa dos jobs de oclusão e iluminação, que serviu de
  especificação para as passadas de GPU.
- **[RPCS3](https://rpcs3.net)**: referência de comportamento do RSX e do lv2,
  e a ferramenta usada para descriptografar o executável.
- **[FidelityFX FSR 1](https://github.com/GPUOpen-Effects/FidelityFX-FSR)**
  (AMD, MIT) e **[SMAA](https://github.com/iryoku/smaa)** (MIT): os shaders da
  cadeia de pós, incluídos no patch do ps3recomp.
- Veja também [CREDITS.md](CREDITS.md). Feito com assistência de IA (Claude).

O código deste repositório é MIT ([LICENSE](LICENSE)). inFAMOUS 2 é
© Sony Interactive Entertainment / Sucker Punch Productions. Este projeto não é
afiliado a eles nem endossado por eles, e não contém nenhum arquivo do jogo.

---

<a id="english"></a>

# inFAMOUS 2 — native PC port

> **Playable, in development.** The game boots and plays its videos, menus and
> cutscenes. Gameplay runs from Empire City into New Marais, with sound,
> controller, keyboard and mouse, saves and the Brazilian Portuguese dub. In
> the heaviest area measured (~9,000 draws a frame) it runs above 100 fps
> uncapped and holds the default 60 fps cap. See [Status](#status-1).

**inFAMOUS 2** (PS3, BCUS98125, version 01.00) running natively on Linux, with
no emulator:

- the game's PowerPC (PPU) and SPU code is translated to C ahead of time by
  static recompilation with [ps3recomp](https://github.com/sp00nznet/ps3recomp);
- the PS3 system is reimplemented;
- the RSX runs the game's own shaders, translated to Vulkan.

> **No game files come with this project, and there are no binaries to
> download.** You need your own copy, dumped from your own PS3. The build turns
> **your** executable into code on **your** machine, and that code must not be
> shared.

## What's new in v0.1.0

Full history in [CHANGELOG.md](CHANGELOG.md).

- **+19% in the heaviest area.** Each draw costs the RSX thread less: the FIFO
  is walked in batches, and render state, surfaces, programs and textures are
  reused while they stay unchanged. Same scene: 90.6 -> 107.7 fps uncapped.
- **Native Edge.** The Edge vertex decompression and compression, which ran as
  translated SPU code, are now hand-written C. ~2.5 million calls compared in
  game showed no difference, and each call is ~4x faster (1.15 µs against
  4.9 µs).
- **No more seconds-long hitches.** On 2026-11-06, after a driver update, there
  were 2.7 s frames and a 0.7 fps 1% low. Pipelines seen before are now rebuilt
  in the background at start, new ones are built off the frame, and `play.sh`
  gives the game a driver shader cache of its own. The 2026-11-07 session,
  capped at 60: 59–60 fps, 21 ms average p99, no hitch.
- **FPS limit in the menu**, 60 by default. With `PS3_MAIN_WAITS=1` (opt-in)
  the main thread really waits instead of spinning: under the 60 cap it goes
  from 99% to 46% CPU.
- **Picture:**
  - FXAA or SMAA 1x;
  - FSR 1 to scale up to the window;
  - area averaging to scale down from 4K/5K;
  - adjustable sharpening;
  - a 4x (5K, supersampling) scale.
- **Adjustable field of view** in the menu, from the game's up to +30%, live.
- **Keyboard and mouse (phase 1):** WASD and the mouse drive the sticks,
  alongside a pad. Sensitivity, axis inversion and keys in the menu and in
  `settings.ini`.
- **Experimental ultrawide:** F1 > Video > Ultrawide draws the world in the
  monitor's shape (Hor+) and puts the HUD, menus and videos in a centred 16:9.
  In the heavy area at 3440×1440 it costs ~7% (108 → 101 fps). See
  [docs/OPCOES.md](docs/OPCOES.md#2-ultrawide-e-fov).
- **Freeze black box:** every stall writes the runtime's state (fences, job
  chains, SPURS, events) to `work/runs/freezes/`, and every slow frame leaves a
  `[vk hitch]` line with its cause.
- **All-or-nothing saves:** a crash mid-save keeps the previous save.
- **Esc** opens the menu like F1, and the small rumble motor works again (it
  was left at 0.4% strength).

Everything new that is on by default has an environment kill switch
([docs/OPCOES.md](docs/OPCOES.md#6-kill-switches-das-três-levas-de-07112026)).

## Status

| Area | State |
|---|---|
| Boot, videos, menus, cutscenes | ✅ Works |
| Gameplay | ✅ Empire City into New Marais, with powers, climbing and the early missions |
| Audio (Wwise on the SPU) | ✅ Works |
| Saves | ✅ Save, continue, autosave; atomic writes |
| Controller | ✅ Xbox, DualShock 4, DualSense through SDL2, with rumble |
| Keyboard and mouse | 🔧 Phase 1: playable; aiming still goes through the game's stick acceleration |
| Brazilian Portuguese dub and text | ✅ `PS3_SYS_LANG=pt` (default) |
| Above 720p | ✅ 1440p, 4K and 5K (supersampling) |
| AA and scaling | ✅ FXAA, SMAA 1x, FSR 1, area downscale |
| Field of view | ✅ Adjustable in the menu |
| Ultrawide (Hor+) | 🧪 Experimental: F1 > Video > Ultrawide; HUD centred in 16:9 |
| Performance | ✅ Steady 60 fps with the default cap; 100+ uncapped in the heaviest area measured |
| Stutter | 🔧 The seconds-long hitches are gone; with a cold cache, the first time an effect appears can still cost a frame |
| Freezes | 🔧 Rare deadlock, cause not found; the black box records the state when it happens |
| Image | 🔧 Mostly correct; a few glitches remain |
| Linux | ✅ Tested on Fedora 44 |
| Windows | ❌ Not supported: the MinGW cross build links but was never tested |

Ryzen 7 9800X3D + Radeon RX 9070 XT, Fedora 44, rendering at 1280×720:

| Scene | Condition | Result | Measured |
|---|---|---|---|
| Heaviest city area, the square the current save starts in (~9,000 draws a frame) | automated bench, standing still, uncapped | **107.7 fps** (90.6 with the v0.1.0 RSX work turned off) | 2026-11-07 |
| Normal play session | 60 cap (default) | 59–60 fps, 21 ms average p99, no hitch | 2026-11-07 |
| Empire City pier (tutorial, walking) | uncapped | ~120–130 fps | Oct 2026 |
| New Marais swamp | uncapped | ~100 fps | Oct 2026 |
| 3D main menu | uncapped | ~115 fps | Oct 2026 |

- The October scenes were measured before the 2026-11-07 work and not
  measured again.
- In the heavy area the four hot threads (main PPU, RSX, Vulkan recording and
  the SPUs) are all at 85–91%, so no single optimization will raise the
  ceiling much.
- The CPU is the limit, not the GPU: in October the New Marais swamp ran at
  4K at the same frame rate as at 720p.

## Requirements

**Game files**, all from **your** PS3, none in the repository (details in
[docs/ARQUIVOS_NECESSARIOS.md](docs/ARQUIVOS_NECESSARIOS.md)):

- **The disc:** inFAMOUS 2, **BCUS98125 (US), version 01.00**, with
  `disc/PS3_GAME/` at the repository root.
- **The executable:** `disc/PS3_GAME/USRDIR/EBOOT.elf`, the disc's `EBOOT.BIN`
  decrypted (RPCS3 can do it). `scripts/lift.sh` checks its SHA-256.
- **The firmware:** the official PS3 PUP installed in RPCS3 (`dev_flash`).

**PC:**

- x86-64 Linux (tested on Fedora 44).
- A CPU with AVX2. The SPU code is built for AVX-512 by default; without it,
  `PORT_SPU_MARCH=x86-64-v3`.
- 8 cores with SMT for the thread pinning (`PS3_HOT_PIN`); with fewer it runs
  without that gain.
- A Vulkan GPU whose graphics queue also does compute.
- ~14 GB of free RAM and ~3 GB of disk to build, plus the game.
- Fedora packages (adapt for your distro): `gcc gcc-c++ clang cmake
  ninja-build python3 SDL2-devel vulkan-headers vulkan-loader-devel
  glslang-devel libpng-devel`.

## Building

```sh
scripts/setup.sh    # ps3recomp at its base commit + this port's patch, Dear ImGui, Python venv
scripts/lift.sh     # translate the PPU and SPU code of YOUR EBOOT.elf
scripts/build.sh    # build into work/port/build-vk
```

`build.sh` runs inside a 14 GB systemd memory scope. `JOBS=<n>`,
`PORT_PPU_GCC=1` and `PS3RECOMP_DIR=<dir>` change how it builds. Contributors:
`scripts/install-hooks.sh` runs `scripts/check_clean.sh` before every commit
and push. The repository map is in [docs/ESTRUTURA.md](docs/ESTRUTURA.md).

## Playing

```sh
work/play.sh            # window + controller + sound, with the main menu
work/play.sh --play     # straight into the game
work/play.sh --title    # straight to inFAMOUS 2's new-game screen
```

- **Main menu:** the game opens on a menu over the disc's own art, with
  **Continue**, **New game**, **Load game**, **Options** and **Quit**.
  inFAMOUS 2 has no main menu of its own, so the port's stands in. Before a
  new game, or loading a save that is not the autosave, the saves are copied
  to `user/backups`.
- **In-game options:** **F1**, **Esc** or **Select+Start** held. They are
  saved to `user/settings.ini`.
  - **Video:** resolution up to 5K, AA (FXAA, SMAA 1x), FSR 1 upscale, area
    downscale, sharpening, full screen, VSync, stretch, field of view (up to
    +30%), monitor.
  - **Language:** voices and text, separately.
  - **Performance:** presets, FPS limit (none, 30–240, display rate), FPS
    counter, recording thread.
  - **Mods:** full-resolution particles, anisotropic filter, installed mods.
  - **Controller and saves:** test buttons, save folder and backup, logs.
  - **Keyboard and mouse:** on/off, sensitivity, invert Y, stick start
    deflection, key list.
- **Precedence:** an environment variable wins over the file, and its field
  is greyed out in the menu. Every variable is listed in
  [docs/OPCOES.md](docs/OPCOES.md).
- **Keyboard and mouse:**
  - **WASD** moves; the **mouse** (or **IJKL**) turns the camera.
  - **Left** / **right** button: R1 (shoot) / L1 (aim).
  - **Space** cross, **Shift**/**Backspace** circle, **F** square,
    **E** triangle.
  - **Q**/**R**: R2/L2. **Ctrl**/**V** or **middle button**: L3/R3.
  - **Enter**/**P** START, **Tab** SELECT, **arrows** or **1-4** the D-pad.
  - `[keyboard] map` in `settings.ini` rebinds keys (`F=SQUARE Q=NONE`).
  - **F11**/**Alt+Enter** toggles full screen.

## Language

The disc carries every region's voices and text, and the game picks a pair
from the console language:

- The game predates the PS3's "Portuguese (Brazil)" (number 17), so it treats
  that number as unknown and falls back to English.
- It reaches its Brazilian files through "Portuguese" (7), which its North
  American list pairs with `brazilian`.
- The port picks that number from the subtitle language and serves the voice
  and text packs chosen in the Language tab.

## Logs and reports

Each `work/play.sh` session writes `work/runs/play-<date>.log`. Captures are
automatic:

- **Slow frame:** a `[vk hitch]` line with its cause;
  `work/tools/hitch_classify.py <log>` groups them.
- **Stall of 2.5 s or more:** every thread's stack and the runtime state go to
  `work/runs/freezes/<log>/`.
- **Freeze:** after 15 s without a frame, a 5 s perf too. Wait ~20 s before
  closing a frozen game.
- **Slow stretch:** after 30 s under 70 fps (or 5 under a lower cap), a 20 s
  profile.

To report a problem, send the log and its `work/runs/freezes/` folder. They
hold no game files.

## Known problems

- **Rare mid-game deadlock:** the picture stops while the audio goes on. The
  main thread waits on an event flag and job chain `0x00B19600` sits at guard
  `0x00B15000`. Seen on Oct 16, 17 and 18, about once per 15–60 minutes of
  play. The cause is not found yet, and the 2026-11-07 session had none.
- **Ultrawide Hor+ is experimental:** the centred HUD was only checked in a
  windowless bench, and markers pinned to the world (targets, icons over
  people) still use the 16:9 position.
- **Keyboard and mouse, phase 1:** aiming goes through the game's stick
  acceleration, not 1:1. Keys are rebound only in `settings.ini`.
- **AA runs after the HUD,** so HUD text is a little softer.
- **Cold cache:** on the first session, or after a driver update, the first
  time each effect appears can still cost a frame.
- **Windows untested:** a MinGW cross build (`work/port/windows/play.bat`)
  links, but nobody has run it.

## How it works

- **PPU:** ~39,000 functions become C, built by clang for x86-64-v3 with PS3
  memory accesses inlined; the system modules are reimplemented (HLE); a few
  game functions have native stand-ins (`work/port/ppu_native.cpp`), such as
  `CView::SetFov` for the field of view.
- **SPU:** every SPU image the game runs is translated to C on x86 SIMD: the
  job manager, 27 job modules, the occlusion plugins, the Wwise job chains,
  Bink and zlib. The heaviest jobs (Edge, `ec5f`, the scene walker, the
  instance culler) have hand-written C versions, each with a mode that
  compares it with the translation.
- **RSX:** a draw engine reads the RSX register file and runs the game's own
  vertex and fragment programs, decompiled to HLSL and compiled to SPIR-V. The
  Vulkan backend records on a thread of its own, and a post chain (AA, FSR 1,
  downscale) runs between the frame and the window.
- **GPU passes:** ambient occlusion and the tiled deferred lighting, two SPU
  jobs, run on the GPU, each checked against the SPU's output.
- **Write tracking:** RSX local memory is write-tracked by page protection,
  so textures and geometry that nothing rewrote are not decoded again.

Details, with addresses and measurements: [docs/NOTES.md](docs/NOTES.md).

## Next steps

[docs/ROADMAP.md](docs/ROADMAP.md). The goal is **a steady 120 fps**, measured
by frame time, not average. Before that:

- the cause of the rare deadlock;
- measuring the Edge index kernel and the main-thread waits in game, to turn
  them on by default;
- a centred HUD for ultrawide;
- 1:1 mouse aiming.

## Credits and legal notice

- **[ps3recomp](https://github.com/sp00nznet/ps3recomp)**, by sp00nznet and
  contributors (MIT).
- **[rpcs3-infamous2](https://github.com/ttmx/rpcs3-infamous2)**, by ttmx: the
  reverse engineering of the occlusion and lighting jobs.
- **[RPCS3](https://rpcs3.net)**: the behavioural reference for the RSX and
  lv2, and the tool used to decrypt the executable.
- **FidelityFX FSR 1** (AMD, MIT) and **SMAA** (MIT): the post chain's
  shaders, carried in the ps3recomp patch.
- See [CREDITS.md](CREDITS.md). Written with AI assistance (Claude).

The code in this repository is MIT ([LICENSE](LICENSE)). inFAMOUS 2 is
© Sony Interactive Entertainment / Sucker Punch Productions. This project is
not affiliated with or endorsed by them and contains no game files.
