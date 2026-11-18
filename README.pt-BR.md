<p align="center">
  <img src="docs/screenshots/banner.jpg" alt="inFAMOUS 2 rodando nativo no Linux" width="100%">
</p>

<h1 align="center">inFAMOUS 2 — port nativo para PC</h1>

<p align="center">
  <b>Recompilação estática do PS3 · Linux · Vulkan · até 4K · fps sem limite · teclado e mouse · dublagem em português</b>
</p>

<p align="center">
  <a href="https://github.com/alvarorichard/infamous2-pc/actions/workflows/ci.yml"><img src="https://github.com/alvarorichard/infamous2-pc/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <a href="https://github.com/alvarorichard/infamous2-pc/releases"><img src="https://img.shields.io/github/v/release/alvarorichard/infamous2-pc?include_prereleases&label=release" alt="Release"></a>
  <img src="https://img.shields.io/badge/status-jog%C3%A1vel%2C%20em%20desenvolvimento-orange" alt="Status: jogável, em desenvolvimento">
  <img src="https://img.shields.io/badge/plataforma-Linux%20x86--64-1793D1?logo=linux&logoColor=white" alt="Linux x86-64">
  <img src="https://img.shields.io/badge/GPU-Vulkan-AC162C?logo=vulkan&logoColor=white" alt="Vulkan">
  <img src="https://img.shields.io/badge/PS3-BCUS98125-003791?logo=playstation&logoColor=white" alt="PS3 BCUS98125">
  <img src="https://img.shields.io/badge/at%C3%A9-4K-6f42c1" alt="Até 4K">
  <img src="https://img.shields.io/badge/dublagem-PT--BR-009c3b" alt="Dublagem PT-BR">
  <img src="https://img.shields.io/badge/c%C3%B3digo-MIT-blue" alt="Código MIT">
</p>

<p align="center">
  <a href="README.md">English</a> |
  <b>Português (Brasil)</b>
</p>

<p align="center">
  <a href="#início-rápido">Início rápido</a> ·
  <a href="docs/OPCOES.md">Opções</a> ·
  <a href="docs/ROADMAP.md">Roadmap</a> ·
  <a href="CHANGELOG.md">Changelog</a> ·
  <a href="docs/NOTES.md">Notas técnicas</a>
</p>

**inFAMOUS 2** (PS3, BCUS98125, versão 01.00) rodando nativo no Linux, sem
emulador:

- o código do jogo, PowerPC (PPU) e SPU, é traduzido para C antes da
  compilação, por recompilação estática com o
  [ps3recomp](https://github.com/sp00nznet/ps3recomp);
- o sistema operacional do PS3 é reimplementado;
- a GPU do PS3 (RSX) roda os shaders do próprio jogo, traduzidos para Vulkan.

> [!IMPORTANT]
> **Nenhum arquivo do jogo vem com este projeto, e não há binários para
> baixar.** Para jogar, você precisa da sua própria cópia, tirada do seu PS3.
> O build transforma o **seu** executável em código na **sua** máquina, e esse
> código não pode ser distribuído.

> [!NOTE]
> **Jogável, em desenvolvimento.** O jogo inicia, passa pelos vídeos, menus e
> cutscenes, e o gameplay segue de Empire City até New Marais, com som,
> controle, teclado e mouse, saves e dublagem em português. Na área mais
> pesada medida (~9 mil draws por frame) ele passa de 100 fps sem limite e
> segura os 60 fps do limite padrão. Veja o [Status](#status).

| | |
|---|---|
| ![Cole no píer de Empire City](docs/screenshots/01-pier.jpg) | ![Cutscene de abertura](docs/screenshots/02-cutscene.jpg) |
| ![Passarela no pântano de New Marais](docs/screenshots/03-new-marais.jpg) | ![Ruas de New Marais](docs/screenshots/04-cidade.jpg) |

![Empire City vista do píer](docs/screenshots/05-empire-city.jpg)

<sub>Capturas direto da GPU no Linux, com uma Radeon RX 9070 XT, em 1280×720.
As de Empire City foram renderizadas em 2× (2560×1440) e reduzidas.</sub>

## Sumário

- [Destaques](#destaques)
- [Início rápido](#início-rápido)
  - [Passo 0: confira o seu PC](#passo-0-confira-o-seu-pc)
  - [Passo 1: consiga os arquivos do jogo](#passo-1-consiga-os-arquivos-do-jogo)
  - [Passo 2: baixe o código](#passo-2-baixe-o-código)
  - [Passo 3: prepare as ferramentas](#passo-3-prepare-as-ferramentas)
  - [Passo 4: traduza o jogo](#passo-4-traduza-o-jogo)
  - [Passo 5: compile](#passo-5-compile)
  - [Passo 6: jogue](#passo-6-jogue)
  - [Se algo der errado](#se-algo-der-errado)
- [Usando o port](#usando-o-port)
  - [Menu principal](#menu-principal)
  - [Opções no jogo](#opções-no-jogo)
  - [Controles](#controles)
  - [Idioma](#idioma)
  - [Saves e dados](#saves-e-dados)
  - [Logs e relatórios](#logs-e-relatórios)
- [Status](#status)
  - [Desempenho](#desempenho)
  - [PS3 × este port](#ps3--este-port)
  - [Problemas conhecidos](#problemas-conhecidos)
- [Como funciona](#como-funciona)
- [Documentação](#documentação)
- [Roadmap](#roadmap)
- [Perguntas frequentes](#perguntas-frequentes)
- [Como contribuir](#como-contribuir)
- [Créditos e aviso legal](#créditos-e-aviso-legal)

## Destaques

| | |
|---|---|
| **Nativo, sem emulador** | O código do PPU e dos SPUs vira C para x86-64; o RSX vira Vulkan. |
| **Imagem mais nítida** | 720p, 1440p, 4K ou 5K (supersampling), FXAA ou SMAA 1x, ampliação com FSR 1, redução por média de área, nitidez ajustável. |
| **Fluido** | 60 fps estáveis com o limite padrão, 100+ fps sem limite na área mais pesada medida. |
| **Do seu jeito** | Controles Xbox, DualShock 4 e DualSense com vibração, ou teclado e mouse. |
| **Visão mais ampla** | Campo de visão ajustável até +30% e um modo ultrawide experimental. |
| **Português do Brasil** | Dublagem e textos, escolhidos separadamente no menu. |
| **Saves seguros** | Gravados por inteiro ou nada: um crash no meio do save mantém o anterior. |
| **Fácil de reportar bugs** | Travadas e frames lentos geram uma "caixa-preta" que você anexa à issue. |

O que mudou em cada versão está no [changelog](CHANGELOG.md). Em resumo, a
v0.2.0 trouxe menos engasgos (shaders e texturas novos saem do caminho do
frame), jobs nativos de Edge ligados por padrão e CI no Linux, no Windows e no
macOS.

## Início rápido

Seis passos pequenos, de zero até jogar. Faça na ordem e confira a linha que
começa com **Pronto quando** antes de seguir.

> [!TIP]
> Nunca compilou nada parecido? Tudo bem. Todo comando abaixo é de copiar e
> colar. As partes demoradas são a primeira tradução e a primeira compilação:
> comece e vá tomar um café.

### Passo 0: confira o seu PC

Você precisa de:

- **Linux x86-64.** Testado no Fedora 44.
- **CPU com AVX2.** O código SPU é compilado com AVX-512 por padrão; numa CPU
  sem ele, compile com `PORT_SPU_MARCH=x86-64-v3` no passo 5.
- **GPU com Vulkan** e uma fila gráfica que também faz compute.
- **~14 GB de RAM livres** e **~3 GB de disco livres** para compilar, além do
  jogo.
- O ideal são 8 núcleos com SMT (afinidade de threads); com menos, o jogo
  roda, só que sem esse ganho.

Instale os pacotes da sua distribuição.

**Fedora** (a testada):

```sh
sudo dnf install gcc gcc-c++ clang cmake ninja-build python3 SDL2-devel \
    vulkan-headers vulkan-loader-devel glslang-devel libpng-devel \
    vulkan-tools git
```

**Arch Linux** e derivadas (Manjaro, EndeavourOS):

```sh
sudo pacman -S --needed base-devel git clang cmake ninja python sdl2 \
    vulkan-headers vulkan-icd-loader vulkan-tools glslang libpng
```

Instale também o driver Vulkan da sua GPU: `vulkan-radeon` (AMD),
`vulkan-intel` (Intel) ou `nvidia-utils` (NVIDIA).

**Debian, Ubuntu** e derivadas (Linux Mint, Pop!_OS):

```sh
sudo apt install build-essential git clang cmake ninja-build python3 \
    python3-venv libsdl2-dev libvulkan-dev vulkan-tools glslang-dev \
    glslang-tools libpng-dev
```

Instale também o driver Vulkan da sua GPU: `mesa-vulkan-drivers` (AMD e
Intel) ou o pacote do driver da NVIDIA.

> [!NOTE]
> As listas do Arch e do Debian/Ubuntu são os pacotes do Fedora traduzidos
> pelo nome; só o Fedora 44 foi de fato testado. Se um nome for diferente na
> sua versão, procure o equivalente. O script de build também precisa do
> `systemd` (para o escopo de memória), que essas distribuições já trazem.

O `libpng` só desenha a arte do disco atrás do menu; sem ele, o fundo é
liso.

**Pronto quando:** `vulkaninfo --summary` (do pacote de ferramentas Vulkan da
sua distribuição) lista a sua GPU, e `clang --version` e `cmake --version`
mostram uma versão.

### Passo 1: consiga os arquivos do jogo

Tudo vem do **seu** PS3, e nada disso está neste repositório. O passo a passo
completo está em [docs/ARQUIVOS_NECESSARIOS.md](docs/ARQUIVOS_NECESSARIOS.md).
São três coisas:

1. **O disco:** inFAMOUS 2, **BCUS98125 (EUA), versão 01.00**, em dump do seu
   disco. A pasta `disc/PS3_GAME/` precisa ficar na raiz deste repositório (o
   passo 2 cria o repositório; você pode copiar o disco depois).
2. **O executável descriptografado:** descriptografe
   `disc/PS3_GAME/USRDIR/EBOOT.BIN` para `disc/PS3_GAME/USRDIR/EBOOT.elf`, por
   exemplo no RPCS3 (Utilities > Decrypt PS3 Binaries).
3. **O firmware:** instale o firmware oficial do PS3 (PUP) no
   [RPCS3](https://rpcs3.net), o que cria o `dev_flash`. Se o seu estiver num
   lugar fora do comum, use `PS3_DEV_FLASH=<pasta>`.

**Pronto quando:** `disc/PS3_GAME/USRDIR/EBOOT.elf` existe (o passo 4 confere o
SHA-256 dele por você).

### Passo 2: baixe o código

```sh
git clone https://github.com/alvarorichard/infamous2-pc.git
cd infamous2-pc
```

Agora coloque aqui a pasta `disc/` do passo 1, de modo que
`disc/PS3_GAME/USRDIR/EBOOT.elf` exista.

**Pronto quando:** `ls disc/PS3_GAME/USRDIR/EBOOT.elf` mostra o caminho.

### Passo 3: prepare as ferramentas

```sh
scripts/setup.sh
```

Isso baixa o ps3recomp no commit base e aplica o patch deste port, baixa o
Dear ImGui e cria um ambiente virtual do Python. Não mexe nos arquivos do
jogo.

**Pronto quando:** o script termina sem erro e as pastas `ps3recomp/` e
`.venv/` existem.

### Passo 4: traduza o jogo

```sh
scripts/lift.sh
```

Isso traduz para C o código PPU e SPU do **seu** `EBOOT.elf`. Leva alguns
minutos. Se o executável não for o BCUS98125 01.00, o script avisa: os
endereços deste port são dessa versão exata.

**Pronto quando:** o script termina sem erro. O código traduzido fica em
`work/` e é só seu: nunca o envie para lugar nenhum.

### Passo 5: compile

```sh
scripts/build.sh
```

A primeira compilação é demorada, porque o código traduzido do PPU é grande.
Ela roda num escopo de memória de 14 GB do systemd, para não derrubar o seu
desktop.

| Variável | Efeito |
|---|---|
| `JOBS=<n>` | quantos jobs de compilação rodam em paralelo |
| `PORT_PPU_GCC=1` | compila o código PPU com GCC em vez de clang |
| `PORT_SPU_MARCH=x86-64-v3` | para CPUs sem AVX-512 |
| `PS3RECOMP_DIR=<pasta>` | usa um ps3recomp que está em outro lugar |

**Pronto quando:** o script termina sem erro e `work/port/build-vk` existe.

### Passo 6: jogue

Ligue o controle antes, se for usar um, e rode:

```sh
work/play.sh
```

Você cai no menu principal do port. Escolha **Novo jogo** (ou **Continuar**) e
jogue. Aperte **F1** ou **Esc** a qualquer hora para abrir as opções.

**Pronto quando:** o menu aparece sobre a arte do disco. Cada sessão grava um
log em `work/runs/play-<data>.log`.

Outras formas de abrir:

```sh
work/play.sh --play     # direto no jogo, sem o menu do port
work/play.sh --title    # direto na tela de jogo novo do inFAMOUS 2
```

### Se algo der errado

| Sintoma | O que tentar |
|---|---|
| O `lift.sh` diz que não há executável descriptografado | Confira se `disc/PS3_GAME/USRDIR/EBOOT.elf` existe (passo 1, item 2). |
| O `lift.sh` avisa sobre o SHA-256 | Você tem outra versão ou região. Só o BCUS98125 01.00 é suportado. |
| O `build.sh` fica sem memória | Reduza o paralelismo: `JOBS=4 scripts/build.sh`. |
| Sua CPU não tem AVX-512 | Compile com `PORT_SPU_MARCH=x86-64-v3 scripts/build.sh`. |
| Áudio em inglês em vez de português | Escolha a dublagem em F1 > Idioma. Veja [Idioma](#idioma). |
| A imagem trava mas o áudio continua | Deadlock raro e conhecido. Espere ~20 s para a caixa-preta ser gravada e veja [Logs e relatórios](#logs-e-relatórios). |

Continua preso? Abra uma issue com o log da sessão (veja
[Logs e relatórios](#logs-e-relatórios)).

## Usando o port

### Menu principal

Ao abrir, aparece o menu do port, sobre a arte do próprio disco (`PIC1.PNG`).
Ele funciona com controle, teclado ou mouse.

| Item | O que faz |
|---|---|
| **Continuar** | entra no save mais recente (mostra nome, progresso e data) |
| **Novo jogo** | abre a tela de jogo novo do próprio inFAMOUS 2 |
| **Carregar jogo** | lista os seus saves; escolha um |
| **Opções** | as abas abaixo |
| **Sair** | fecha o jogo |

O inFAMOUS 2 não tem menu principal: com save, ele entra direto no autosave;
sem save, mostra só "Começar um jogo novo". O menu do port faz esse papel.
Antes de **Novo jogo** ou de carregar um save que não é o autosave, os saves
são copiados para `user/backups`, porque o próximo autosave do jogo grava por
cima do slot de autosave, como no console. Dentro do jogo: **Start > Dados
salvos**.

### Opções no jogo

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

Para trocar teclas, edite `[keyboard] map` no `user/settings.ini`, no formato
`Tecla=BOTÃO` (por exemplo `F=SQUARE Q=NONE`). As ações `WALK` (andar devagar)
e a roda do mouse (`WheelUp`, `WheelDown`) existem, mas não têm tecla no mapa
padrão.

**Teclas do port:**

| Tecla | O que faz |
|---|---|
| **F1** ou **Esc** | abre e fecha o menu |
| **F11** ou **Alt+Enter** | tela cheia / janela |

### Idioma

O disco traz voz e texto de todas as regiões (`brazilian.psarc`,
`brazilian_text.psarc`, `latino.psarc`...), e o jogo escolhe pelo idioma do
console:

- O jogo foi feito antes de o PS3 ter "Português (Brasil)" (número 17), então
  trata esse número como desconhecido e cai no inglês.
- O caminho para o PT-BR é o idioma "Português" (7), que a lista da região
  norte-americana liga aos arquivos `brazilian`.
- O port escolhe esse número sozinho a partir do idioma das legendas, e serve
  os arquivos de voz e de texto que você escolheu na aba Idioma.

### Saves e dados

- Os saves ficam em `gamedata/dev_hdd0/home/00000001/savedata/`. Cada arquivo
  é gravado ao lado e renomeado por cima, então um crash no meio do save
  mantém o anterior.
- `PS3_SAVEDATA_ROOT=<pasta>` usa outra pasta. Os testes e benchmarks usam
  isso para nunca tocar nos saves de quem joga.
- As opções ficam em `user/settings.ini`, e os backups dos saves em
  `user/backups/`.

### Logs e relatórios

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
- **Ficou lento:** depois de 30 s abaixo de 70 fps (ou 5 abaixo do limite, se
  ele for menor), um perfil de 20 s (até três por sessão).

Para relatar um problema, mande o log e a pasta de `work/runs/freezes/`
correspondente. Eles não contêm arquivos do jogo.

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

### Problemas conhecidos

- **Deadlock raro no meio do jogo:** a imagem para e o áudio continua. A
  thread principal fica esperando uma event flag, e a job chain `0x00B19600`
  fica parada no guard `0x00B15000`. Foi visto em 16, 17 e 18/10, uma vez a
  cada 15–60 min de jogo. A causa ainda não foi achada; a sessão de 07/11 não
  teve nenhum. Se acontecer, a caixa-preta grava o estado.
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
- **Windows e macOS não testados:** existe um build cruzado com MinGW
  (`work/port/windows/play.bat`) que linka, mas ninguém rodou o jogo nele. O
  rastreamento de escrita da memória do RSX e a captura de pilhas são só
  Linux.

## Como funciona

- **PPU:**
  - As ~39 mil funções do executável viram C (`ppu_lifter.py`).
  - O resultado é compilado com clang para x86-64-v3, com os acessos à memória
    do PS3 inline.
  - Os módulos do sistema são reimplementados (HLE).
  - Algumas funções do jogo têm substitutos nativos
    (`work/port/ppu_native.cpp`), como o `CView::SetFov` do campo de visão.
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

Os detalhes, com endereços e medições, estão em
[docs/NOTES.md](docs/NOTES.md).

## Documentação

| Documento | O que cobre |
|---|---|
| [ARQUIVOS_NECESSARIOS](docs/ARQUIVOS_NECESSARIOS.md) | Quais arquivos tirar do seu PS3 |
| [OPCOES](docs/OPCOES.md) | Todas as opções e variáveis de ambiente, com os kill switches |
| [ESTRUTURA](docs/ESTRUTURA.md) | Mapa do repositório |
| [MODS](docs/MODS.md) | Como trocar arquivos do jogo com mods |
| [NOTES](docs/NOTES.md) | Engenharia reversa e medições |
| [ROADMAP](docs/ROADMAP.md) | O que está pronto e o que vem a seguir |
| [CHANGELOG](CHANGELOG.md) | O que mudou em cada versão |
| [GITHUB_SETUP](docs/GITHUB_SETUP.md) | Configurações do repositório e como fazer um release |

## Roadmap

Veja [docs/ROADMAP.md](docs/ROADMAP.md). A meta é **120 fps estáveis**,
medidos pelo tempo de frame e não pela média. Antes disso vem:

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

**Roda no Windows?** Ainda não. Veja os
[problemas conhecidos](#problemas-conhecidos).

**O jogo vem junto?** Não. Nenhum arquivo do jogo faz parte do projeto.

## Como contribuir

Contribuições são bem-vindas.

1. Faça um fork do repositório e crie uma branch.
2. Rode `scripts/install-hooks.sh` uma vez. Ele instala hooks que executam o
   `scripts/check_clean.sh` antes de cada commit e push, para que nenhum
   arquivo do jogo nem código traduzido entre no repositório.
3. Leia o [mapa do repositório](docs/ESTRUTURA.md) para se orientar.
4. Abra um pull request descrevendo o que mudou e como você mediu.

Relatos de bug ajudam mais com o log da sessão e a pasta
`work/runs/freezes/` (veja [Logs e relatórios](#logs-e-relatórios)).

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
© Sony Interactive Entertainment / Sucker Punch Productions. Este projeto não
é afiliado a eles nem endossado por eles, e não contém nenhum arquivo do jogo.
