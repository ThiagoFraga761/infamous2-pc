# Opções e variáveis de ambiente

Tudo o que muda o comportamento do port sem recompilar. Os nomes e os padrões
foram conferidos no código em 07/11/2026: o port (`work/port/`) e o ps3recomp
com o patch (`work/patches/ps3recomp.patch`).

## Como as opções se combinam

1. **`user/settings.ini`** guarda o que o menu principal e o menu do jogo (F1)
   mudam. Ao abrir com janela, o port converte cada campo na variável de
   ambiente correspondente (`work/port/settings.cpp`).
2. **Uma variável já definida no shell ganha do arquivo.** O campo dela fica
   cinza no menu, e salvar não muda o valor que o arquivo tinha.
3. **`work/play.sh`** define alguns padrões (log de fps, cache de shader
   próprio, captura de travadas). Também deixa uma variável do shell ganhar.
4. **`work/port/main.cpp`** fixa os padrões do inFAMOUS 2, como os jobs de SPU
   na GPU, o Edge nativo e a afinidade das threads. Uma variável do shell
   também ganha deles.
5. **Rodadas sem janela** (testes, benchmarks) não leem o `settings.ini` nem os
   padrões dele. Valem só o ambiente e os padrões do código: por exemplo, sem
   limite de FPS e sem a thread de gravação, a não ser que o ambiente peça.

Convenção usada em quase todos os switches novos:

- `0`: desliga e volta ao comportamento anterior (kill switch);
- `1`: ligado;
- `2`: modo de conferência, quando existe. O caminho novo e o antigo rodam
  juntos, e qualquer diferença vai para o log (`... CHECK: ...`). É mais lento
  e serve só para diagnóstico.

Exemplo:

```sh
PS3RECOMP_RES_SCALE=2 PS3RECOMP_FPS_LIMIT=0 work/play.sh
```

---

## 1. Vídeo

| Variável | `settings.ini` | Padrão | Efeito |
|---|---|---|---|
| `PS3RECOMP_RES_SCALE` | `[video] scale` | `1` | Escala de render: 1 = 720p, 2 = 1440p, 3 = 4K, 4 = 5K (supersampling). Vale na próxima abertura |
| `PS3RECOMP_VK_FULLSCREEN` | `[video] fullscreen` | `0` | Abre em tela cheia. F11 ou Alt+Enter alternam ao vivo |
| `PS3RECOMP_VK_PRESENT` | `[video] vsync` | `mailbox` | `fifo` = VSync; `mailbox` = sem tearing e sem esperar o monitor |
| `PS3RECOMP_VK_STRETCH` | `[video] stretch` | `0` | `1` estica o 16:9 para a janela; `0` usa barras pretas |
| `PS3RECOMP_VK_DISPLAY` | `[video] display` | `0` | Em qual monitor a janela abre |
| `PS3RECOMP_POST_AA` | `[video] aa` | `off` | `fxaa` ou `smaa` (SMAA 1x) no tamanho do frame. Hoje roda depois do HUD, que também é suavizado |
| `PS3RECOMP_UPSCALE` | `[video] upscale` | `bilinear` | `fsr1` = FSR 1 (EASU + RCAS) quando o frame é menor que a janela (720p numa tela 1440p) |
| `PS3RECOMP_DOWNSCALE` | `[video] downscale` | `bilinear` | `area` = média por área em luz linear quando o frame é maior que a janela (4K/5K numa tela 1440p) |
| `PS3RECOMP_SHARPEN` | `[video] sharpen` | `auto` | Nitidez RCAS: `auto` = 0,8 só depois do FSR 1; `0` a `1` vale depois do FSR 1, da média por área e em 1:1 |
| `PS3RECOMP_POST` | — | ligado | `0` desliga a cadeia de pós inteira (AA, FSR 1, redução), mesmo que o menu peça |
| `PS3RECOMP_FOV_SCALE` | `[video] fov` (0–30 %) | `1` | Multiplica a tangente do meio-ângulo da câmera (0,5–2). O menu grava `fov = 15` como `1.15`. Muda ao vivo |
| `PS3RECOMP_FOV_CUTSCENE` | — | `1` | `0` mantém o FOV do jogo nas câmeras de cutscene (`CCmAnimated`) |
| `PS3RECOMP_FOV_AIM` | — | `1` | `0` mantém o FOV do jogo na mira (L1, `CCmSnipe`) |
| `PS3RECOMP_ANISO` | `[mods] aniso` | `16` | Filtro anisotrópico forçado: 0 (o do jogo), 2, 4, 8, 16 |
| `PS3RECOMP_FULLRES_PARTICLES` | `[mods] fullres_particles` | `1` | Fogo, faíscas e fumaça na resolução da imagem, em vez de 512×288 esticado |

**Extras opt-in, só por variável, sem opção no menu.** Nenhum deles foi
validado em jogo:

| Variável | Padrão | Efeito |
|---|---|---|
| `PS3RECOMP_SHADOW_SCALE` | `1` | 1–4: o mapa de sombra (1024×1024) a S vezes a escala de resolução |
| `PS3RECOMP_REFLECT_SCALE` | `1` | 1–2: o reflexo da água (256×256) a R vezes a escala |
| `PS3RECOMP_SHADOW_PCF` | `2` | `3` ou `5`: PCF k×k na sombra, em vez do 2×2 do jogo. Muda a chave do shader |
| `PS3RECOMP_CLIP_PLANES` | desligado | Planos de corte do usuário como clip distances do Vulkan (geometria submersa no reflexo). `1` = controle 0x1478 + máscara de saída, regra do RPCS3; `2` = só o controle |
| `PS3RECOMP_CLIP_PLANES_FLIP` | `0` | `1` troca o sinal dos planos (diagnóstico) |

## 2. Ultrawide e FOV

Sem nada definido, o 16:9 aparece com barras pretas, ou esticado com
`stretch = 1`. O controle de **campo de visão** do F1 alarga a vista dentro do
16:9 e funciona em qualquer monitor.

O Hor+ de verdade, com o alvo de render mais largo, é experimental. O jeito
simples é **F1 > Vídeo > Ultrawide** (`[video] ultrawide = 1` no
`settings.ini`, vale depois de reiniciar): o port lê o formato do monitor e
liga `RT_ASPECT=auto` e `ASPECT=auto` juntos. O HUD, os menus e os vídeos ficam
em 16:9 no centro (vídeos e telas 2D em pillarbox). Os marcadores presos ao
mundo (alvos, ícones sobre pessoas) ainda usam a posição 16:9. Custo medido na
área pesada a 3440×1440: 108 → 101 fps.

| Variável | Padrão | Efeito |
|---|---|---|
| `PS3RECOMP_RT_ASPECT` | desligado | Aspecto em que os alvos 16:9 do jogo são desenhados: `3440x1440`, `21:9`, `43:18`, `3440/1440` ou `2.389`. A escala X sai de Y × a ÷ (16/9) |
| `PS3RECOMP_RES_SCALE_X`, `PS3RECOMP_RES_SCALE_Y` | `RES_SCALE` | Escala por eixo: fração (`43/16`), inteiro ou decimal, de 1/2 a 8. O X tem prioridade sobre o `RT_ASPECT` |
| `PS3RECOMP_ASPECT` | desligado | Força o aspecto do frustum (wrap do `CView::SetFov`). Sozinho, com o alvo em 1280×720, a imagem sai espremida: serve para medir o custo de CPU |
| `PS3RECOMP_RES_AXIS` | ligado | `0` ignora as três de cima e volta à escala inteira |
| `PS3RECOMP_EDGE_VP` | ligado | `0` desliga o ajuste de viewport do Edge (também por eixo) |
| `PS3RECOMP_UW_HUD` | ligado quando o alvo 1280×720 é desenhado largo | `0` (ou `stretch`) estica o HUD junto com a imagem |
| `PS3RECOMP_UW_HUD_LOG` | desligado | `1` loga cada veredito novo (alvo, par de programas `vp:fp`, centralizado/largo) e uma contagem a cada 600 frames |
| `PS3RECOMP_UW_WIDE`, `PS3RECOMP_UW_CENTER` | vazio | Força largo ou centralizado por par de programas (`<vp>:<fp>,...`, hashes do log) |
| `PS3RECOMP_UW_HUD_TARGETS` | vazio | Outros alvos de cor (hex) onde a UI é desenhada, além dos display buffers |
| `PS3RECOMP_UW_EVAL` | ligado | `0` não roda o vertex program na CPU para classificar quads de largura toda |
| `PS3RECOMP_UW_PRECLEAR` | ligado | `0` não limpa de preto as barras do pillarbox |

Exemplo, 3440×1440 Hor+:

```sh
PS3RECOMP_RES_SCALE=2 PS3RECOMP_RT_ASPECT=3440x1440 PS3RECOMP_ASPECT=3440x1440 work/play.sh
```

## 3. Desempenho

| Variável | `settings.ini` | Padrão | Efeito |
|---|---|---|---|
| `PS3RECOMP_FPS_LIMIT` | `[performance] fps_limit` | `60` com janela; `0` sem janela | Limite de FPS; `0` = sem limite. O menu tem 30, 40, 60, 80, 120, 240 e "Taxa do monitor" |
| `PS3RECOMP_FPS_LIMIT_MODE` | — | `present` (`main` com `PS3_MAIN_WAITS=1`) | `main`: quem dorme o limite é a thread principal do PPU, não a thread do RSX |
| `PS3RECOMP_BE_THREAD` | `[performance] record_thread` | `1` com janela; `0` sem janela | Thread de gravação Vulkan separada (+12% nas áreas pesadas) |
| `PS3_MAIN_WAITS` | — | `0` | `1` liga o pacote de esperas nativas da thread principal (abaixo). Com limite de 60: main de 99% para 46% de CPU, p99 de 16,9 para 17,7 ms. Bom para gastar menos CPU com limite |
| `PS3_HOT_PIN` | — | `1` (main.cpp) | Main, rsx, rsx-record e os 5 SPUs, cada um num núcleo físico próprio (precisa de SMT e 8+ núcleos). `0` desliga |
| `SPURS_PRIO` | — | `0:1` (main.cpp) | Workload General do SPURS primeiro em todo SPU |
| `PS3RECOMP_VK_PIPE_WARM` | — | ligado | Pipelines vistos antes (`~/.cache/ps3recomp/pipekeys-v1.bin`) são recriados em segundo plano na partida. `0` desliga |
| `PS3RECOMP_VK_PIPE_WARM_THREADS` | — | CPUs/4, entre 2 e 4 | Threads `vk-warm` (também os workers da fila assíncrona) |
| `PS3RECOMP_VK_PIPE_WARM_MAX` | — | `4096` | Quantos registros aquecer na partida; `0` grava a lista e não aquece |
| `PS3RECOMP_VK_PIPE_ASYNC` | — | ligado | Variantes de pipeline nunca vistas são montadas nas `vk-warm`. `0` desliga |
| `PS3RECOMP_VK_PIPE_SKIP` | — | `0` | `1` pula o draw cujo pipeline está atrasado (tem pop visual) |
| `PS3RECOMP_VK_PIPE_SKIP_MS` | — | `4` | Atraso tolerado antes de pular |
| `PS3RECOMP_VK_PIPE_NICE` | — | `5` | nice das threads `vk-warm` (0–19) |
| `MESA_SHADER_CACHE_DIR` | — | `~/.cache/ps3recomp/mesa` (play.sh) | Cache de shaders do driver só deste jogo, com 4 GB (`MESA_SHADER_CACHE_MAX_SIZE`). Evita os primeiros frames de vários segundos depois de um update do Mesa |
| `PS3RECOMP_NATIVE_SSAO`, `PS3RECOMP_NATIVE_LIGHTING` | — | `1` (main.cpp) | Oclusão ambiente e iluminação deferida na GPU. `0` volta ao job de SPU; `2` compara os dois |
| `PS3RECOMP_NATIVE_EDGE` | — | `1` | Kernels Edge em C no lugar do código SPU traduzido. `0` desliga todos, incluindo os três abaixo |
| `PS3RECOMP_NATIVE_EDGE_DEC`, `PS3RECOMP_NATIVE_EDGE_ENC` | — | `1` (main.cpp) | Descompressão (0x4448) e compressão (0x99E8) de vértices Edge nativas. Sem diferença em ~2,5 milhões de chamadas no modo 2, ~1,15 µs contra ~4,9 µs por chamada |
| `PS3RECOMP_NATIVE_EDGE_IDX` | — | `0` | `1` liga a descompressão de índices Edge (0xC7B8) nativa. Ainda não foi comparada em jogo |
| `PS3RECOMP_EDGE_DEC_LAYOUT` | — | `1` | `0` volta ao guard estrito antigo do 0x4448 |
| `PS3RECOMP_WTRACK` | — | `1` (main.cpp, Linux) | Rastreamento de escrita na memória local do RSX: textura que ninguém reescreveu não é refeita. `2` confere |
| `PS3RECOMP_ENG_VPULL` | — | `1` (settings) | A GPU lê os bytes de vértice do guest direto |
| `PS3RECOMP_BQ_TEXBUILD` | — | `0` (main.cpp) | Texturas criadas ao lado da thread de gravação, sem drenar a fila. O fork liga por padrão; o port fixa em 0 até ser medido |

Ajuste fino do pacote `PS3_MAIN_WAITS`. Cada um também pode ser ligado
sozinho, e um valor explícito ganha do pacote (`PS3_MAIN_WAITS=1 PS3_JL_WAIT=0`):

| Variável | Padrão | Efeito |
|---|---|---|
| `PS3_POLL_WAIT` | `0` | Espera nativa (condição + spin + futex) nos dois polls da main (0x133640, ref do RSX; 0x1DB7DC, Kicker) |
| `PS3_KICKER_WAKE` | `0` | A thread "SPU Light Task Kicker" dorme num futex em vez do `usleep` |
| `PS3_KICKER_POKE` | `1` | A main cutuca o Kicker quando começa a esperar (só com `KICKER_WAKE`) |
| `PS3_KICKER_LABELS` | `1` | Os releases de label do RSX acordam o Kicker (só com `KICKER_WAKE`) |
| `PS3_JL_WAIT` | `0` | Espera nativa das job-lists WWS da main |
| `PS3_POLL_SPIN_US` / `PS3_POLL_REF_TMO_US` / `PS3_POLL_KICK_TMO_US` / `PS3_POLL_CAP_US` | 30 / 100 / 200 / 2000 | Spin antes do futex, timeouts e teto de uma chamada nativa (µs) |
| `PS3_JL_RECHECK_US` / `PS3_JL_CAP_US` | 2 / 200 | Releitura periódica e teto da espera de job-list (µs) |

## 4. Controle, idioma e caminhos

| Variável | `settings.ini` | Padrão | Efeito |
|---|---|---|---|
| `PS3RECOMP_KBM` | `[keyboard] enabled` | `1` | Teclado e mouse como o pad da porta 0, junto com o controle. `0` desliga tudo |
| `PS3RECOMP_KBM_SENS` | `[keyboard] sensitivity` | `1.0` | Sensibilidade do mouse (0,10–5,00 no menu) |
| `PS3RECOMP_KBM_INVERT_Y` | `[keyboard] invert_y` | `0` | Inverte o eixo vertical |
| `PS3RECOMP_KBM_DEADZONE` | `[keyboard] deadzone` | `0.20` | "Início do giro": deflexão do analógico no menor movimento (0–0,60) |
| `PS3RECOMP_KBM_MAP` | `[keyboard] map` | — | `Tecla=AÇÃO ...` por cima do mapa padrão; `NONE` desliga a tecla. Ex.: `F=SQUARE Q=NONE` |
| `PS3RECOMP_KBM_CAPTURE` | — | primeiro uso | `focus` captura o mouse assim que a janela ganha foco |
| `PS3RECOMP_KBM_SENS_Y`, `PS3RECOMP_KBM_CURVE`, `PS3RECOMP_KBM_WINDOW_MS` | — | `1.0`, `1`, `25` | Multiplicador da sensibilidade vertical, curva (1 = linear) e janela de suavização do mouse (ms) |
| `PS3_SYS_LANG` | (derivado de `text`) | `pt` | Idioma do console informado ao jogo. `pt` é o caminho para o PT-BR (veja o README, Idioma) |
| `PS3_VOICE_LANG` | `[language] voice` | `brazilian` | Pacote de voz (`cache/psarc/<voz>.psarc`) |
| `PS3_TEXT_LANG` | `[language] text` | `brazilian` | Pacote de texto (`cache/psarc/<texto>_text.psarc`) |
| `PS3_SKIP_LAUNCHER` | `[launcher] skip` | `0` | Abre o jogo sem o menu principal do port (`work/play.sh --play`) |
| `PS3_USER_DIR` | — | `user/` | Onde fica o `settings.ini` (e `backups/`) |
| `PS3_SAVEDATA_ROOT` | — | `gamedata/dev_hdd0/home/00000001/savedata` | Outra pasta de saves. Os benchmarks usam uma cópia para nunca tocar nos saves de quem joga |
| `PS3_MODS_DIR` | — | `mods/` | Pasta de mods (docs/MODS.md) |
| `PS3_DEV_FLASH` | — | o `dev_flash` do RPCS3 | Firmware instalado (docs/ARQUIVOS_NECESSARIOS.md) |

Opções do `work/play.sh`: `--play` (sem o menu do port), `--title` (direto na
tela de jogo novo do inFAMOUS 2, com backup dos saves antes) e `--dump N`
(guarda 1 frame a cada N em `work/runs/play/f_*.ppm`).

Opções de build (não são de execução): `PORT_SPU_MARCH=x86-64-v3` para CPU sem
AVX-512, `PORT_PPU_MARCH=` (vazio) para CPU sem AVX2, `PORT_PPU_GCC=1`
(compila o PPU com GCC), `JOBS=<n>` e `PS3RECOMP_DIR=<pasta do ps3recomp>`.

## 5. Diagnóstico

Ligados pelo `work/play.sh`:

| Variável | Padrão no play.sh | Efeito |
|---|---|---|
| `PS3RECOMP_ENG_FPSLOG` | `1` | Linha `[rsx fps]` a cada 10 s |
| `PS3RECOMP_FTLOG` | `1` | A cada 10 s, a distribuição dos tempos de frame: mediana, p99, pior frame e 1% low |
| `PS3RECOMP_VK_HITCH` | 2000 ÷ limite ms, no mínimo 25 | Todo frame mais longo que isso gera uma linha `[vk hitch]` com as causas (builds de pipeline, esperas de GPU, criação de textura, o que cada thread esperou, run-queue, page faults). `-1` desliga |
| `PS3RECOMP_VK_HITCH_SCHED` | `1` | `0` mantém a linha `[vk hitch]`, mas sem ler `/proc` |
| `PS3_STALL_MS` / `PS3_STALL_DIR` | `2500` / `<log>-stalls/` | Sem frame por esse tempo, o próprio jogo grava a pilha de todas as threads |
| `PS3_FREEZE_DUMP` | ligado | Junto com cada stall, grava o estado do runtime (fences, guards e chains de job, workloads do SPURS, filas e flags de evento, sinais perdidos), no momento e de novo 15 s depois. `0` desliga |
| `PS3_FREEZE_DIR` / `PS3_FREEZE_LOG` | `work/runs/freezes/<log>/` / o log da sessão | Destino do estado e de onde sai o fim do log (512 KB) |
| `PS3_VERBOSE` | `0` | `1` loga cada job e espera de SPU (milhares de linhas por segundo) |
| `PLAY_PROFILE` / `PLAY_PROFILE_BELOW` | `1` / 70, ou limite − 5 | Depois de 30 s abaixo do valor em cena 3D, um `perf record` de 20 s (até 3 por sessão). `0` desliga. `PERF=<binário>` quando o perf não está no PATH |
| `SPU_DUMP_MISS` / `SPU_DUMP_STUCK` | `work/runs/spujob-*` | Imagem de job SPU sem tradução e local store de job preso (`PS3RECOMP_SPU_REPLAY=<arquivo>` repete) |

Desligados, para ligar quando precisar:

| Variável | Efeito |
|---|---|
| `PS3_DROP_LOG=0` | Silencia os logs das primeiras quedas de sinal (a contagem continua) |
| `SPURS_JC_RING=1` | O estado da trava ganha os últimos 2048 passos das job chains |
| `SPURS_JC_TIMING=1` | A cada 200 voltas de uma job chain, quanto dura a volta e a espera no guard |
| `PS3_POLLTOP=<s>` | Os sites de poll (`usleep`) da main a cada N s |
| `PS3_WAITSTATS=1` | Estatísticas de espera da main, Kicker e limite |
| `PS3RECOMP_ENG_DRAWSTATS=1` | Acertos dos memos do motor de desenho (texturas, programas, pipelines) |
| `PS3RECOMP_BQ_STATS=1` | Sincronização da fila de gravação (`[bq-sync]`) |
| `PS3RECOMP_VK_DCACHE_STATS=1` | Reaproveitamento de descriptor sets |
| `PS3RECOMP_VK_GPUPROF=1` | Tempo de GPU por passe, incluindo a cadeia de pós |
| `PS3RECOMP_VK_STATS=1` | A cada 5 s, os flushes do backend Vulkan e o tempo esperando a GPU neles |
| `PS3RECOMP_VK_POST_SHOT=<pasta>` | Grava em PPM o que a cadeia de pós entrega (`_EVERY`, padrão 300 frames) |
| `PS3RECOMP_FOV_LOG=1` | Uma linha `[fov]` por segundo para cada câmera |
| `PS3RECOMP_KBM_LOG=1` | Cada mudança do pad gerado pelo teclado e mouse |

`work/tools/hitch_classify.py <log>` agrupa as linhas `[vk hitch]` de uma
sessão por causa.

## 6. Kill switches das três levas de 07/11/2026

Tudo o que está **ligado por padrão** tem um `0` que volta ao comportamento
anterior. Se algo quebrar depois de atualizar, este é o primeiro lugar a
olhar. Os commits do port são `626b7c4` (leva 1), `bb80d0c` (leva 2),
`ddce81c` (leva 3) e `fcca056` (Edge nativo ligado).

### Leva 1: FIFO, estado de draw, pipelines, pós, caixa-preta

| Variável | Padrão | Para voltar |
|---|---|---|
| `PS3RECOMP_FIFO_SLOW` | `0` | `1` desliga todos os caminhos rápidos do walker da FIFO (volta a uma palavra por vez) |
| `PS3RECOMP_ENG_TEXSKIP` | `1` | `0` refaz o bind de textura em todo draw; `2` confere |
| `PS3RECOMP_ENG_FPMEMO` | `1` | `0` volta ao scan antigo de fragment programs; `2` é opt-in e não foi testado em jogo |
| `PS3RECOMP_BQ_PUBLISH_EVERY` | `8` | `1` publica a fila de gravação a cada draw (1–256) |
| `PS3RECOMP_VK_PIPE_WARM` | ligado | `0` |
| `PS3RECOMP_POST` | ligado | `0` |
| `PS3_FREEZE_DUMP` | ligado | `0` |
| `PS3RECOMP_NATIVE_PPU_OFF=0043E370` | — | Tira o wrap do FOV (`PS3RECOMP_NATIVE_PPU=0` tira todos os nativos de PPU) |

### Leva 2: esperas da main, pipelines assíncronos, escala por eixo, teclado e mouse

| Variável | Padrão | Para voltar |
|---|---|---|
| `PS3_MAIN_WAITS` | `0` (opt-in) | — |
| `PS3RECOMP_VK_PIPE_ASYNC` | ligado | `0` |
| `PS3RECOMP_BQ_IDLE_PUBLISH` | ligado | `0` não publica a fila quando o walker para |
| `PS3RECOMP_ENG_VPMEMO` | `1` | `0` volta ao memo antigo de vertex programs |
| `PS3RECOMP_BQ_VSC_DELTA` | `1` | `0` copia todas as linhas de constantes do VS; `2` confere |
| `PS3RECOMP_RES_AXIS` | ligado | `0` |
| `PS3RECOMP_KBM` | `1` | `0` |
| `PS3RECOMP_BQ_TEXBUILD` | `0` no port | `1` para testar |
| `PS3RECOMP_CLIP_PLANES`, `PS3RECOMP_SHADOW_PCF`, `PS3RECOMP_SHADOW_SCALE`, `PS3RECOMP_REFLECT_SCALE` | desligados (opt-in) | — |
| `PS3RECOMP_NATIVE_EDGE_IDX` | `0` (opt-in) | — |
| `PS3RECOMP_EDGE_DEC_LAYOUT` | `1` | `0` |

O anel de vértices passou a começar com 64 MB (era 32). Não há variável: está
em `VK_RING_VERTEX_MB`.

### Leva 3: custo por draw na thread do RSX (+19% na área pesada)

| Variável | Padrão | Para voltar |
|---|---|---|
| `PS3RECOMP_FIFO_FAST2` | ligado | `0` volta ao fast path por header da leva 1 (sem o lote de registradores) |
| `PS3RECOMP_ENG_RSMEMO` | `1` | `0` decodifica o render state todo draw; `2` confere |
| `PS3RECOMP_ENG_SURFMEMO` | `1` | `0`; `2` confere |
| `PS3RECOMP_ENG_VPDRAW` | `1` | `0` procura o vertex program a cada pergunta; `2` confere |
| `PS3RECOMP_ENG_FPSHAPE` | `1` | `0` faz o hash em todo miss de fragment program; `2` confere |
| `PS3RECOMP_ENG_FPDRAW` | `1` | `0` ou `2` resolvem o fragment program todo draw |
| `PS3RECOMP_ENG_IDXB_SIMD` | `1` | `0` volta ao laço escalar dos limites de índice |
| `PS3RECOMP_ENG_END_LEAN` | `1` | `0` é o caminho antigo; `2` entrega ao backend os mesmos bytes do antigo (A/B bit a bit) |
| `PS3RECOMP_BQ_VSC_TRACK` | `1` | `0` compara todas as linhas de constantes; `2` confere |
| `PS3RECOMP_BQ_PSC_INPLACE` | `1` | `0` mantém a cópia das constantes de FP |
| `PS3RECOMP_ENG_TEXCARRY` | `1` | `0`; `2` confere |
| `PS3RECOMP_ENG_TEXFMEMO` | `1` | `0`; `2` confere |
| `PS3RECOMP_BQ_TEXHINT` | `1` | `0` ignora o hint; `2` confere |
| `PS3RECOMP_VK_DCACHE` | `1` | `0` reaproveita só o descriptor set do draw anterior |
| `PS3RECOMP_VK_VSTAIL_COPY` | `0` | `1` copia o fim das constantes do VS no bind, como antes |
| `PS3RECOMP_BQ_ROWCOPY` | `1` | `0` usa `memcpy` em todas as linhas |
| `PS3RECOMP_BQ_SPIN` | `4000` | PAUSEs do worker antes do futex (0–1000000) |
| `PS3RECOMP_BQ_MWAITX` / `_US` | `0` / `50` | `1` faz o worker esperar com MONITORX/MWAITX (AMD) |

### Lift do PPU: VMX em um store de 16 bytes e cache por site de `bctrl`

Entra só depois de um relift (`scripts/lift.sh`) e do rebuild de todos os
chunks. O código gerado muda em dois pontos:

- **VMX.** `lvx`/`stvx` viram um acesso de 16 bytes, em vez de duas metades de
  8. `vspltw`, `vsplth`, `vspltb`, `vspltish`, `vspltisw`, `vand`, `vandc`,
  `vor`, `vxor`, `vnor`, `vsel`, `vmrgh*`, `vmrgl*`, `vmax/vmin` de byte,
  `lvlx`, `lvrx`, `lvsl` e `lvsr` viram helpers do `ppu_vmx.h` que gravam o
  registrador com um store só. Antes eram stores de 1, 4 ou 8 bytes, e o load
  de 16 bytes da instrução seguinte esperava por eles (*store-forwarding
  stall*). O resultado é bit a bit o mesmo.
- **`bctrl`.** Cada site guarda o último alvo e chama a função direto. Erro de
  cache, alvo nulo ou desconhecido e diagnóstico ligado continuam passando por
  `ps3_indirect_call`, como antes.

| Chave | Onde | Padrão | Para voltar |
|---|---|---|---|
| `PS3_ICALL_IC` | ambiente | ligado | `0`: nenhum site é preenchido e todo `bctrl` vai para `ps3_indirect_call` (sem relift) |
| `PS3_ICALL_IC_STATS` | ambiente | desligado | `1` imprime `[icall] inline cache: N misses/s` a cada 2 s. Com `PS3_ICALL_IC=0`, o número é o total de `bctrl`/s |
| `-DPPU_LEGACY_VMX` | flag de compilação dos chunks | — | `lvx`/`stvx` voltam às duas metades e os helpers novos voltam ao C antigo (só rebuild, sem relift) |
| `PS3RECOMP_LIFT_FLAGS` | ambiente do `scripts/lift.sh` | vazio | `"--legacy-vmx --no-icall-cache"` gera o lift antigo, byte a byte |

---

## Summary (English)

Every knob that changes the port without a rebuild. Names and defaults were
checked against the code on 2026-11-07.

- **Precedence:** a variable set in the shell wins over `user/settings.ini`
  (which the main menu and the in-game F1 menu write). Its field is then greyed
  out in the menu. `work/play.sh` and `work/port/main.cpp` set their own
  defaults below that. Runs without a window (tests, benchmarks) ignore
  `settings.ini`.
- **Convention:** `0` turns a new path off (kill switch); `2`, where it
  exists, runs the old and new paths side by side and logs any difference.
- **Video** (section 1): `PS3RECOMP_RES_SCALE` 1–4, `PS3RECOMP_POST_AA`
  off/fxaa/smaa, `PS3RECOMP_UPSCALE` bilinear/fsr1,
  `PS3RECOMP_DOWNSCALE` bilinear/area, `PS3RECOMP_SHARPEN` auto/0–1,
  `PS3RECOMP_FOV_SCALE` (the F1 field-of-view slider), `PS3RECOMP_ANISO`,
  `PS3RECOMP_FULLRES_PARTICLES`. Opt-in, env-only and not validated in game:
  shadow and reflection scale, wider shadow PCF, user clip planes.
- **Ultrawide** (section 2): true Hor+ is experimental. F1 > Video >
  Ultrawide (`[video] ultrawide = 1`, after a restart) reads the monitor's
  shape and keeps the HUD, menus and videos in a centred 16:9; markers pinned
  to the world still use the 16:9 position. `PS3RECOMP_UW_HUD=0` stretches the
  HUD. ~7% slower at 3440x1440 in the heavy area.
- **Performance** (section 3): `PS3RECOMP_FPS_LIMIT` (60 from the settings
  file), `PS3RECOMP_BE_THREAD`, `PS3_MAIN_WAITS=1` (opt-in: the main thread
  goes from 99% to 46% CPU under a 60 fps cap), pipeline warm-up and async
  variants, native Edge kernels (decompression and compression on, index
  opt-in).
- **Controls, language, paths** (section 4): `PS3RECOMP_KBM*`,
  `PS3_SYS_LANG`, `PS3_VOICE_LANG`, `PS3_TEXT_LANG`, `PS3_SAVEDATA_ROOT`,
  `PS3_USER_DIR`, `PS3_MODS_DIR`, `PS3_DEV_FLASH`.
- **Diagnostics** (section 5): `[rsx fps]` and `[vk hitch]` lines, stall
  stacks and the freeze black box (on in `play.sh`), and optional
  statistics.
- **Kill switches** (section 6): one table for each of the three performance
  waves merged on 2026-11-07. If something breaks after an update, look there
  first.
- **PPU lift** (end of section 6, needs a relift): VMX moves, splats, merges
  and bitwise ops now write the register with one 16-byte store, and every
  `bctrl` has a per-site inline cache. `PS3_ICALL_IC=0` turns the cache off at
  run time, `PS3_ICALL_IC_STATS=1` prints its misses per second,
  `-DPPU_LEGACY_VMX` on the chunks brings the old VMX code back (rebuild only),
  and `PS3RECOMP_LIFT_FLAGS="--legacy-vmx --no-icall-cache" scripts/lift.sh`
  produces the old lift byte for byte.
