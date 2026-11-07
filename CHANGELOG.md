# Changelog

Mudanças que importam para quem joga ou compila. O detalhe de cada uma está
no `git log` e em [docs/NOTES.md](docs/NOTES.md); as variáveis citadas, em
[docs/OPCOES.md](docs/OPCOES.md).

O projeto segue [versionamento semântico](https://semver.org/lang/pt-BR/). Até
a 1.0, uma versão menor (0.x) pode mudar opções e o formato do
`settings.ini`.

## [0.1.0] — 2026-11-07

Primeiro release. Não inclui arquivos do jogo nem binários: é o código-fonte
do port e o patch do ps3recomp, sobre o upstream `42f5f8f`.

### O que o port faz

- Roda o inFAMOUS 2 (BCUS98125, 01.00) nativo no Linux x86-64 com Vulkan:
  boot, vídeos, menus, cutscenes e gameplay de Empire City até New Marais, com
  áudio, saves e dublagem e textos em PT-BR.
- O PPU e todas as imagens SPU são traduzidos para C a partir do executável
  do próprio jogador.
- Os jobs mais pesados (`ec5f`, `bc11`, `207`, geometria Edge) têm versões em
  C nativo.
- A oclusão ambiente e a iluminação deferida rodam na GPU.
- Menu principal próprio e opções no jogo, gravadas em `user/settings.ini`.
- Controle via SDL2 (Xbox, DualShock 4, DualSense), e teclado e mouse.
- De 720p a 5K, com AA e upscaling.

### Novo desde o último commit público (25/10)

**Desempenho**

- Leva 3, custo por draw na thread do RSX: walker da FIFO em lote, com as
  escritas de registrador entregues em bloco ao motor e ao dispatcher. O
  render state, a superfície, os programas e as texturas são reaproveitados
  enquanto os registradores não mudam. Há também cache de descriptor sets por
  conteúdo e limites de índice com SSE4.1. Na área pesada (~9 mil draws por
  frame), sem limite: **90,6 → 107,7 fps** (+19%).
- Leva 2:
  - variantes de pipeline nunca vistas são montadas fora do frame
    (`PS3RECOMP_VK_PIPE_ASYNC`);
  - a fila de gravação é publicada quando o walker para;
  - memo de vertex programs;
  - só as linhas de constantes do VS que mudaram vão para a fila.
- Leva 1:
  - fast path da FIFO para escritas simples de registrador;
  - bind de textura e lookup de fragment program repetidos são pulados;
  - a fila de gravação é publicada em lotes;
  - os pipelines já vistos são recriados em segundo plano na partida
    (`~/.cache/ps3recomp/pipekeys-v1.bin`).
- Descompressão (0x4448) e compressão (0x99E8) de vértices Edge em C nativo,
  ligadas por padrão: zero diferenças contra o código SPU em ~2,5 milhões de
  chamadas, 1,15 µs contra 4,9 µs por chamada. A de índices (0xC7B8) existe,
  mas é opt-in.
- `PS3_MAIN_WAITS=1` (opt-in): esperas nativas da thread principal (ref do
  RSX, Kicker de luz, job-lists WWS) e o limite de FPS dormido nela. Com
  limite de 60, a main cai de 99% para 46% de CPU.
- O anel de uniforms começa em 256 MB e o de vértices em 64 MB: sem flush e
  espera da GPU nos primeiros minutos.

**Estabilidade**

- Limite de 60 fps por padrão, configurável no menu.
- O `play.sh` usa um cache de shaders do driver só do jogo
  (`~/.cache/ps3recomp/mesa`, 4 GB). Antes, um update do Mesa ou outro jogo
  esvaziava o cache, e vinham frames de 2,7 s.
- Na sessão de 07/11, com limite de 60: 59–60 fps, p99 médio de 21 ms, nenhuma
  travada. Em 06/11, havia frames de 2,7 s e 1% low de 0,7 fps.
- Caixa-preta das travas: cada stall grava fences, guards e job chains,
  workloads do SPURS, filas e flags de evento e sinais perdidos em
  `work/runs/freezes/`.
- Cada frame lento gera uma linha `[vk hitch]` com a causa;
  `work/tools/hitch_classify.py` agrupa.
- Saves gravados num arquivo ao lado e renomeados: um crash no meio mantém o
  save anterior.

**Imagem**

- Cadeia de pós entre o frame e a janela: FXAA ou SMAA 1x, FSR 1 (EASU +
  RCAS) para ampliar, média por área para reduzir, e nitidez ajustável.
- Escala 4× (5K, supersampling).
- Campo de visão ajustável (do jogo até +30%), ao vivo, por um wrap do
  `CView::SetFov`.
- Experimental, só por variável: escala de render por eixo para Hor+ em
  ultrawide (`PS3RECOMP_RT_ASPECT`), sombra e reflexo com escala própria,
  planos de corte do usuário e PCF largo na sombra.

**Controles**

- Teclado e mouse como o pad da porta 0, junto com o controle: WASD, mouse na
  câmera, mapa configurável, aba própria no menu.
- Esc abre o menu, como o F1.
- O motor pequeno da vibração voltou a funcionar (ficava em 0,4%).

**Documentação**

- [docs/OPCOES.md](docs/OPCOES.md): todas as variáveis de ambiente, com padrão
  e efeito, e os kill switches de cada leva.
- [docs/ESTRUTURA.md](docs/ESTRUTURA.md): o mapa do repositório.
- README e roadmap atualizados.

### Problemas conhecidos

- Deadlock raro no meio do jogo (imagem para, áudio continua), sem causa
  achada.
- No Hor+ experimental, o HUD, os menus e os vídeos saem esticados.
- Teclado e mouse na fase 1: a mira passa pela aceleração do analógico do
  jogo.
- Windows: o build com MinGW linka, mas nunca foi testado.

[0.1.0]: https://github.com/ThiagoFraga761/infamous2-pc/releases/tag/v0.1.0
