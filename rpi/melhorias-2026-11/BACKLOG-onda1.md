# Backlog: inFAMOUS 2 recomp

Fiz 4 conferências no código e nos logs para checar as frentes; o resultado está na seção 0. Fora isso, nada foi editado, compilado ou rodado.

## 0. Ceticismo: o que não fecha

| Proposta | Veredito | Evidência |
|---|---|---|
| **Jogabilidade #8**: PoC do ultrawide com poke de `+0x2A4` no fim do frame | **Contradita pelo código.** A projeção do vf27 sai de `+0x2A8/+0x2AC` (ppu_recomp_002.cpp:161665 e 161682). O `+0x2A4` só alimenta o `+0x298` (161792-161810). O SetFov só recalcula tan_x quando o FOV muda, porque sai cedo (ppu_recomp_002.cpp:158011+, leitura em +13 e escrita em +32). Um poke no aspect não alarga nada. | Fica substituída pelas fases U1/U2 da frente Ultrawide. |
| **Estabilidade #8**: parada de 2,76 s causada pela primeira execução dos passes nativos | **Fraca.** Os passes nativos enfileirados (a858c27, 25/10) existem nas duas sessões. Em 28/10 o cache estava quente (4261 KB) e os piores frames das primeiras janelas foram 77, 122 e 296 ms, sem stall. Em 06/11 o cache carregou 0 KB e o pior frame foi 2764 ms. | A causa mais provável é compilação de pipeline com cache frio depois do update do Mesa. A pilha (`rsx` em `bq_wait_for`) bate com o worker ocupado em `vk_eng_variant`. Isso vira o item P5 e o env do Mesa. |
| **Throughput**: projeção de "~125-130 fps" somando as propostas | Hipótese. As três frentes (rsx, main, SPU) empatam perto de 8 ms, então os ganhos não se somam. | Só vale depois de medir sem cap. |
| **Throughput** diz que o rsx-record tem folga (39% em spin); **Pacing** mede o rec em 7,2 ms/frame | Os dois números podem estar certos, mas ninguém mediu a CPU real do rec. O zero-copy (T6) depende disso. | Medir com o run_delay e o schedstat do item P4 antes. |
| **Throughput #5**: o POLLTOP custa ~220k/s | Não foi medido. | Um A/B barato resolve. |
| **Estabilidade #3**, trocar o retorno do notify pelo código do RPCS3 | O valor foi "citado de memória". | Só depois de ler o chamador no lift. |
| **Estabilidade #5**, "WWS lost kick" | Confiança baixa. | Fica só como diagnóstico. |
| **Visual #1**, ZPASS | O divisor global está confirmado (rsx_vulkan_backend.c:974, `nn = vk_res_scale()²`). Falta provar que as queries das lâmpadas rodam no alvo 512x288. | Medir com `ZPLOG` antes de mexer. |
| **Visual**, "a GPU tem folga" | O dado é de 17/10 em New Marais, antes do SSAO e da luz nativos. | Toda proposta de AA/SSAA precisa passar pelo gate de `GPUPROF`. |
| **Pacing**, soak_235551 | Esse soak está capado em 60 e não serve para throughput. Os soaks de 213939, 222320 e 224010 seriam sem cap. | Conferir o env (`PS3RECOMP_FPS_LIMIT` herdado) antes de qualquer A/B. |

Pontos que conferi no código e estão confirmados:
- ESC chama `quit` (rsx_vulkan_backend.c:7345).
- O rumble multiplica por 257 (cellPad.c:1249-1250).
- O uring nasce com 16 MB e cresce com `vk_flush` (rsx_vulkan_backend.c:1304, 1344, 1362).
- O cap está em play.sh:31.
- `PLAY_PROFILE_BELOW=70` (play.sh:135); com cap em 60, isso dispara perf toda sessão.
- `POLLTOP` e `JC_TIMING` vêm ligados por padrão (play.sh:60 e 77).

## 1. Conflitos e dependências entre frentes

1. **Cap de 60, meta de 120 e lógica do jogo.** O teste do relógio `0x9BFC74/78` (G3) vem antes de liberar 120 e antes do presenter (P6). Ele não tem código e bloqueia o resto. Um indício a favor: o jogo já roda a 60 (e a 100+ sem cap em 25/10) sem queixa de timer acelerado, mas isso não é prova.
2. **VRR e limite de fps aparecem em duas frentes** (Pacing P1 e Jogabilidade G4). Viraram um item só (O1-3 mais a decisão de VRR). O P6 só faz sentido depois do resultado do VRR.
3. **`vk_target_scale` é mexido por três frentes**: sombra e reflexo com escala própria (V5), ultrawide com escala racional por eixo (U3) e ZPASS por alvo (V1). Ordem: o V1 primeiro, pequeno, guardando `tsc²` por query. O U3 depois reescreve para `sx*sy`. O V5 entra em cima do U3, ou no mesmo PR, para não refatorar a função duas vezes.
4. **A cadeia de pós (`vk_post_run`) serve a SMAA, FSR1 e SSAA e também ao ultrawide.** O tamanho da saída sai do `vk_window_fit`. Precisa nascer aceitando um frame de aspect qualquer, senão o U3 refaz tudo. A fase 2 do SMAA (antes do HUD) e a fase 3 do ultrawide (HUD centralizado) precisam do mesmo mapeamento do primeiro draw de HUD no DRAWTRACE: uma investigação só.
5. **O POLLTOP fica entre throughput e estabilidade.** O T5 quer tirar do padrão; a caixa-preta S2(e) usa o POLLTOP. Solução: a caixa-preta usa a própria pilha (eu-stack já roda no stall_watch) mais um snapshot sob demanda, e o POLLTOP sai do padrão. O `SPURS_JC_RING=1` por padrão precisa de A/B no bench (±3 fps).
6. **Várias propostas mudam chave de shader ou pipeline**: clip planes (V6), PCF da sombra (V5), EDS1/2 (P7) e WPOS com `1/sx,1/sy` (U3). Cada uma invalida `~/.cache/ps3recomp/spirv` e o vkpipe. Juntar num lote só e lançar o manifesto de warm (P5) com versão, senão o manifesto vira lixo a cada mudança.
7. **A fila do be-thread é mexida por três propostas**: publish em lote (T3c), payload ring (T6) e TEXBUILD (P3). As três mudam o protocolo. Ordem: P3 (só env) → T3 → T6, nunca em paralelo.
8. **A parada do primeiro frame do mundo foi atacada por três frentes** (S8, P3, P5 e G6-2). Ficou um item só (O1-3 mais P5). A evidência da seção 0 aponta para o pipeline frio.
9. **Hor+ gasta CPU, e o jogo já é limitado por CPU.** O U2 (medir o custo em 21:9 com o RT em 720p) é gate para U3, U4, U5 e U6 e para o investimento em ultrawide.
10. **PPU: um rebuild total só.** O VMX (T2) exige relift total. O inline do `ps3_indirect_call` e qualquer `NATIVE_ENTRIES` pendente (o wrap do `POS_SCREEN_FROM_WORLD`, U5) vão no mesmo rebuild. O FOV (U1) é de 1 chunk (002) e pode vir antes.
11. **O que o Thiago usa hoje** é `scale=1, stretch=1` (720p esticado até 3440). O FSR1 (V4) dá ganho imediato nessa config. O SMAA e o SSAA só aparecem se ele mudar de scale.

## 2. Ondas

Legenda de rebuild:
- **[P]** só o port (`work/port`), leve;
- **[F]** lib do fork (video/input/system/spurs) mais link;
- **[C002]** chunk PPU 002;
- **[SPU]** relift do módulo SPU;
- **[PPU-ALL]** relift e rebuild total, numa janela noturna, com `-j1` e `MemoryMax=12G`.

### Onda 1: ganhos rápidos e seguros, correções de travamento e instrumentação

**Rebuild O1-A [P] mais o play.sh, sem bench pesado.**

| # | Item | Frente | Por que | Arquivos | Verificação | Esforço | Risco | Depende |
|---|---|---|---|---|---|---|---|---|
| 1 | Log volta depois do execve: `pipe2(O_CLOEXEC)`, `F_DUPFD_CLOEXEC` e um `port_log_restore()` antes do execve | Estab. | A sessão de 5h12 de 28/10 tem só 34 linhas de log. O detector de stall fica cego | main.cpp:589-607, ui.cpp:578-589, play.sh:126-139 | Trocar de save na tela inicial e ver `[rsx fps]` depois de `[ui] starting again`; `ls -l /proc/<pid>/fd` sem pipe órfão | 1,5h | Baixo | — |
| 2 | play.sh: `PLAY_PROFILE_BELOW=min(70, cap-5)`; POLLTOP e JC_TIMING viram opt-in; `MESA_SHADER_CACHE_DIR=~/.cache/ps3recomp/mesa` com `MAX_SIZE=4G` | Pacing, Thr., Jog. | Hoje dispara 3 perfs por sessão sem motivo, mantém diagnóstico no caminho quente e usa um cache do Mesa cheio (1023 MB) | play.sh:60, 77, 135 | Sessão sem `[play.sh] ... profiling`; A/B de fps com e sem POLLTOP | 0,5h | Baixo | — |
| 3 | O limite de fps vai para o default do settings (60); presets não zeram o limite; degraus 30/40/60/80/120/240 | Jog., Pacing | O F1 mostra o limite cinza e o ini é ignorado. ui.cpp:312 zera o limite em silêncio | play.sh:31, settings.cpp:49 e 225, ui.cpp:73-74 e 312 | F1 editável; o ini guarda o valor | 1,5h | Baixo | — |
| 4 | `system()` no F1 vira `posix_spawnp`/`std::filesystem::copy` | Jog. | "Backup" e "Abrir pasta" fazem fork e congelam o jogo por segundos | ui.cpp:232-233 e 254-256 | `PS3_STALL_MS=500` sem stall ao clicar | 0,5h | Baixo | — |

**Rebuild O1-B [F]: um build só da lib.**

| # | Item | Frente | Por que | Arquivos | Verificação | Esforço | Risco | Depende |
|---|---|---|---|---|---|---|---|---|
| 5 | Save atômico (tmp, fsync, rename) mais backup rotativo a cada SAVE (N=10) | Jog. | 29/36 boots sem shutdown limpo; o `_0` é regravado no lugar | cellSaveData.c:712-779 | Cópia do save via `PS3_SAVEDATA_ROOT`; `strace inject EIO` só no processo de teste | 3h | Baixo | — |
| 6 | ESC abre o F1 em vez de fazer `_Exit(0)`; pad neutro com o F1 aberto; "Sair" com confirmação | Jog. | ESC hoje fecha o jogo e perde progresso | rsx_vulkan_backend.c:7345, cellPad.c:545-569, rsx_ui.cpp | ESC não gera `window closed`; GetData sem botões com o F1 aberto; run_all 8/8 | 3h | Baixo | — |
| 7 | Rings Vulkan pré-dimensionados (uring 256 MB, vring 64-128 MB, mais env) e log de crescimento sem o corte em 8 | Pacing | 6 a 8 `vk_flush` síncronos por sessão nos primeiros minutos | rsx_vulkan_backend.c:1238-1242, 1303-1306, 1338-1362 | `grep 'grown to'` = 0; `VK_FLUSH_CALLERS` sem `vk_ring_alloc` | 2h | Baixo (+~480 MB host-visible) | — |
| 8 | Rumble: motor pequeno passa a ser on/off ×0xFFFF, mais ganho no ini | Jog. | Hoje 1×257 dá 0,4%: metade do rumble não existe | cellPad.c:1249-1250, settings | Grind com log do SetActDirect | 0,5h | Baixo | — |
| 9 | GCM: drop do overflow sob `s_ref_pub_lock`, mais `[gcm-wait]` quando o ref fica parado mais de 1 s. A autocura fica desligada | Estab. | Corrida real (rara) e falta diagnóstico da trava de 19/10 10:03 | cellGcmSys.c:1685-1733, 2622-2636 | Teste de unidade da fila (QLEN pequeno); freeze_hunt no Rog | 2h | Baixo | — |
| 10 | Contadores de eventos e notifies descartados (evq cheia, JobGuard cap, EBUSY do SPU), só log | Estab. | Um "job done" perdido reproduz a assinatura do deadlock de áudio | sys_event.c, cellSpurs.c:2577-2595 e 2766, lv2_register.c:1986 | Soak de 25 min com 0 drops | 2h | ~0 | — |
| 11 | Medidor de frame time: gráfico no F1, histograma da sessão (só frames 3D), bucket `HS_FPS_LIMIT` e `run_delay` do schedstat no VK_HITCH | Pacing | Sem isso nenhuma outra proposta é provada, e os "8,5 ms sem CPU" do rsx_drain continuam sem explicação | ui.cpp, rsx_vulkan_backend.c:3698-3725, rsx_hitch.h, hitch_classify.py | Overlay visível; resumo no fim do log; A/B no heavy_bench sem perda de fps | 5h (sem present_wait) | Baixo | — |
| 12 | ZPASS: divisor por query = `tsc²` do alvo (o array entra no `slot_swap`) | Visual | Os halos das lâmpadas ainda estão pendentes | rsx_vulkan_backend.c:856/974/6856 | **Antes de mexer**: `ZPLOG=200` com `FULLRES_PARTICLES=0/1`. Se os valores já baterem, descartar | 2h | Baixo | Medição |
| 13 | Hint "vale depois de reiniciar" no aniso | Visual | Hoje o `static forced` faz a troca no F1 não ter efeito | ui.cpp:348-355 | — | 0,2h | ~0 | — |

**Rebuild O1-C [F]: caixa-preta, junto com o O1-B se couber.**

| # | Item | Frente | Por que | Arquivos | Verificação | Esforço | Risco | Depende |
|---|---|---|---|---|---|---|---|---|
| 14 | `ps3_freeze_dump` (trylock em tudo) chamado pelo stall_watch aos 2,5 s e aos 15 s, mais pasta `work/runs/freezes/` que nunca entra na limpeza. Sem POLLTOP (conflito 5) | Estab. | As duas travas pós-fix só deixaram pilha; os .log sumiram | novo libs/debug/freeze_dump.c, main.cpp:224-282, cellGcmSys.c, cellSpurs.c, spurs_kernel.c, sys_event.c | Trava forçada com `jc_hunt.sh` (`JC_SNAPSHOT=0`, `RACE_DELAY=4000`) no Rog; o `.state` coerente | 6h | Baixo, se for só trylock | 1, 10 |
| 15 | Watchdog "main presa num mesmo lr com todos os SPUs ociosos", só diagnóstico | Estab. | Classe da trava de 19/10 07:41 | spurs_kernel.c:517-576, sys_timer.c | Aparece no soak | 3h | Baixo | 14 |
| 16 | Boot zlib: os logs do first_draw vão para `work/runs/boot/`, mais dump aos 3 s no `wait_signal` | Estab. | A taxa de 5% é de 15/10 e hoje é desconhecida | first_draw.sh, second_frame.sh, spu_workload.c | 40 boots no Rog | 2h | Baixo | — |

**Rebuild O1-D [C002]: FOV e medição do Hor+.**

| # | Item | Frente | Por que | Arquivos | Verificação | Esforço | Risco | Depende |
|---|---|---|---|---|---|---|---|---|
| 17 | Wrap nativo do `CView::SetFov` (func_0043E370): `FOV_SCALE`, com restauração do `+0x29C` e filtro por vtable | UW | Dá FOV ajustável e é a base do Hor+. k=1 é no-op | ppu_native.cpp, ppu_native_entries.py:53, settings, ui | Log `[fov]` com `tan_x/tan_y = 1,7778`; sem deriva em 60 s; run_all 8/8 | 6h | Baixo | — |
| 18 | No mesmo wrap, `PS3RECOMP_ASPECT` forçado com o RT em 720p, só para medir CPU | UW | **Gate** do ultrawide inteiro: o Hor+ aumenta os draws | ppu_native.cpp | bench_pier e nm_bench sem cap, 16:9 contra 2,389: p99, draws e CPU de main/rsx | 2h | Baixo | 17 |

**Experimentos sem código (só quando o Thiago não estiver jogando; Vulkan medido com heavy_bench, nm_bench e move_bench mais screenshot):**
- **E1 (G3):** relógio do jogo `0x9BFC74/78` com cap de 30, 60, 120 e 0 via `RE_WATCH`. **Bloqueia a meta de 120.**
- **E2:** nm_bench e heavy_bench **sem cap**, para ter uma linha de base nova de throughput (a última é de 25/10).
- **E3:** `PS3RECOMP_ENG_DRAWSTATS=1`, para medir a taxa de memo de textura e programa. Decide o T3.
- **E4:** A/B com `BQ_TEXBUILD=1` e `TEXASYNC=1`, 3 pares (P3 passo 1).
- **E5:** `SPURS_PRIO` subindo a lista de animação que a main espera (T5.1), 2 pares.
- **E6:** testar se ✕ ou START já pulam o `splash_america.bik` (`PAD_SCRIPT`).
- **E7:** soak no Rog com 20 rodadas de heavy_bench mais 4 soaks de 25-40 min, com e sem cap, já com os itens 1 e 14 aplicados (S6). Atualizar o ROADMAP.md:32, que ainda dá a guard como aberta.
- **E8 (decisão do Thiago):** window-rule de VRR no niri mais `SDL_VIDEODRIVER=wayland`, com o teste de janela oculta (regressão do 07681e3).

### Onda 2: grandes

| # | Item (agrupado por rebuild) | Frente | Por que | Arquivos | Verificação | Esforço | Risco | Depende |
|---|---|---|---|---|---|---|---|---|
| 19 | **[F] Cadeia de pós `vk_post_run`**: FXAA e SMAA 1x; FSR1 EASU mais RCAS quando a origem é menor que a janela; downscale Lanczos ou box quando é maior; scale 4 liberado; troca ao vivo. A saída aceita qualquer aspect (conflito 4) | Visual | Hoje não há nenhum AA, e o 720p esticado sai bilinear | rsx_vulkan_backend.c:2965-3011, 3651-3676, 4680-4740; settings; ui:72 e 262 | `VK_POST_SHOT` comparando off/fxaa/smaa/fsr no píer e em New Marais; `GPUPROF` abaixo de 0,5 ms por passe; p99 do heavy_bench igual | ~20h | Baixo-médio (hazard entre frames em voo; limite de 16k words do SPIR-V) | 11 |
| 20 | **[F] Warm de pipelines**: manifesto versionado mais 2-4 threads na tela inicial. Começar pela versão simples, que só aquece o cache do driver | Pacing, Jog. | 2,7 s e 1,4 s a cada update do Mesa (seção 0) | rsx_draw_engine.c:1960-2025, rsx_vulkan_backend.c:5955-6318, main.cpp | Cache frio simulado com `VK_PIPECACHE=0`: 0 frames com builds depois do warm | 6h (simples) / 14h (completo) | Médio na versão completa (estado global do engine) | 2 |
| 21 | **[F] Fast path do walker da FIFO** (tabela de 2048 métodos, `PS3RECOMP_FIFO_SLOW=1` como kill switch, modo compare) | Thr. | ~30% da thread rsx | cellGcmSys.c:1857-2459, rsx_dispatch.c, rsx_draw_engine.c | run_all; diff de estado no replay; usoak A/B em us/draw | 9h | Médio (método especial esquecido) | E2 |
| 22 | **[F] Pular estado de draw repetido** (texturas, programa) mais publish em lote com release | Thr. | ~19% + 9% + 6% da rsx | rsx_draw_engine.c:4011-4170, rsx_be_thread.c:379-388 e 906-939 | E3 antes; shots iguais; −8 a −12% no rsx_drain | 9h | Médio (textura velha) | E3, depois do 21 (mesma thread; medir um de cada vez) |
| 23 | **[F] `ps3_indirect_call`** com tabela direta e caminho rápido enxuto (parte runtime) | Thr. | 2,5% self na main | ppu_loader.cpp:2257-2266 e 2645-2670 | test_loader; self abaixo de 1% | 4h | Baixo | — |
| 24 | **[F] Textura sem drenar a fila**: TEXBUILD por padrão, ou sub-alocador de VkDeviceMemory | Pacing | 100-150 ms ao entrar numa área nova | rsx_be_thread.c:677-718, rsx_vulkan_backend.c:3938 e 4018 | ab_report com p99,9 e frames acima de 40 ms; sem stall em `t_texture_create` | 3h (env) / +8h (alocador) | Médio | E4 |
| 25 | **[PPU-ALL] VMX sem stall de store-forwarding** mais inline do icall, mais `NATIVE_ENTRIES` pendentes. **O único rebuild total do plano** | Thr. | A evidência mais forte do lado da main | ppu_lifter.py:183-188, 3110, 3327, 3395; ppu_recomp.h | Harness ppuloc com 0 mismatches; test_ppu_*; annotate; −4 a −8% de CPU na main | 12h + janela noturna | Baixo na semântica, alto na operação (RAM) | 23 |
| 26 | **[SPU] Edge 0x4448 e 0x9780 nativos** | Thr. | 14,2% + 5,5% do SPU; segura os frames de 9-11k draws e New Marais | edge_kernels_native.c, lift_spu.py | Harness t_dec/mut com 0 diffs; `NATIVE_EDGE=2`; nm_bench | 20h | Médio (floats do SPU) | E2 (confirmar que o SPU limita) |
| 27 | **[F] Ultrawide fase 2**: escala racional por eixo (mais V5, sombra e reflexo por alvo, no mesmo PR) | UW, Visual | O Hor+ precisa de RT largo; as sombras 1024 PCF 2x2 serrilham | rsx_vulkan_backend.c:3213/3440-3482/3535/3580/6872-6891/4606/4636, rsx_draw_engine.c:4595, rsx_fp_decompiler.c:1249, spu_dma.h:174-196 | 16:9 idêntico (diff de pixels) com scale 1 e 2; círculo sem ovalizar; SSAO e luz sem costura | 22h + 8h | Médio (~15 pontos de escala) | **18 (gate)**, 12, 19 |
| 28 | **[F] Ultrawide fase 3**: HUD, menu e FMV centralizados via `xf` por draw, com whitelist. A investigação do primeiro draw de HUD é a mesma da fase 2 do SMAA | UW | O RT largo estica a UI | rsx_draw_engine.c:4544-4556, main.cpp | Shots com as proporções do HUD ±1 px; motion comic em pillarbox | 14h | Médio | 27 |
| 29 | **[F] Lote de chaves de shader**: clip planes do usuário (reflexo da água) mais PCF k×k da sombra. Uma invalidação de cache só | Visual | Geometria submersa refletida; pendente desde 15/10 | rsx_vp_decompiler.c:704, rsx_draw_engine.c, rsx_vulkan_backend.c:7149-7169 | DRAWTRACE com 0x1478 ≠ 0; ENG_SHOT no píer e no pântano | 6h | Médio | 20 com manifesto versionado |
| 30 | **[F] Teclado e mouse, fase 1** (WASD e mouse viram analógico; tirar o latch `real0`) | Jog. | Sem controle o jogo não responde | rsx_vulkan_backend.c:7339-7353, cellPad.c:502-569, rsx_ui.cpp | Tutorial de mira sem pad; pad e teclado juntos; Alt+Tab solta as teclas | 8h | Médio | 6 |
| 31 | **[F] Pad na porta 0 por SDL_JoystickID** (com histerese), SDL2 no build MinGW e hints de rumble DS4/DS5 | Jog. | Um pad virtual da Steam rouba a porta 0; DualSense não funciona no .exe | cellPad.c:299-315, CMakeLists | Dois pads; hotplug; Windows com DualSense | 4h | Médio-baixo | 8 |
| 32 | **[P/F] Pular o splash** com bik de 1 frame criado do zero, se E6 mostrar que ele não pula | Jog. | São 11,7 s em todo boot | ppu_fs.cpp:153-198, settings | Tempo até `[rsx fps]` em 3 boots; 10 boots sem hang | 3h | Médio-baixo | E6, 16 |
| 33 | **[F] Presenter fora da thread rsx** (prazo com sleep e spin, 1 frame de folga, `PACE=legacy` para A/B) | Pacing | O limitador dorme na thread que drena a FIFO | rsx_vulkan_backend.c:7376-7404, rsx_be_thread.c:1112, main.cpp:281-418 | usoak 3 pares a 60 e a 120: p99 ≤ 17,5 ms e ≤ 9 ms | 12h | Médio-alto (threading no VkQueue; +latência). Precisa de code-reviewer | 11, E1, E8 |

### Onda 3: ambiciosas ou condicionadas

| # | Item | Frente | Condição |
|---|---|---|---|
| 34 | Vértices sem cópia (payload ring GPU-visível mais cache residente com WTRACK, mais push descriptors) — 20h, risco médio-alto | Thr. | Só depois do 22, e só se a CPU real do rec (item 11) virar gargalo |
| 35 | EDS1/EDS2 para cortar variantes de pipeline — 10h | Pacing | Depois do 20; muda o formato do manifesto |
| 36 | Marcadores do HUD presos ao mundo: wrap do `POS_SCREEN_FROM_WORLD` (vai no rebuild PPU-ALL do 25, ou num C002) — 8h, confiança baixa | UW | 28 |
| 37 | Culling alargado: log do ângulo entre os planos do bc11/207 e rotação de reserva — 6h | UW | 27 (o log pode ir antes, já com o 18) |
| 38 | Cutscene em pillarbox ou Hor+ por modo de câmera (LR do CCmAnimated) — 4h | UW | 27, decisão do Thiago |
| 39 | Autocuras (GCM ref heal, WWS kick heal), sempre com log | Estab. | Só com evidência do 14 e do 15 |
| 40 | Mudar o retorno do JobGuardNotify e o spin no `jg_wait` | Estab. | Ler o chamador no lift mais A/B de 20 rodadas no Rog |
| 41 | Upscaler temporal FSR 3.1 ou XeSS 2 no Vulkan (jitter, MVs de câmera, máscara reativa) — ~60h. O D3D12 com FSR4 fica descartado | Visual | Tela 4K ou canvas 3440 com GPU apertada |
| 42 | Mira 1:1 escrevendo yaw/pitch da câmera (teclado e mouse, fase 2) — ~20h mais RE | Jog. | Se a fase 1 (30) ficar ruim |
| 43 | Reescrita do sistema de objetos/ec5f (rpi/decomp-scene) | Thr. | Necessária para 120 nos frames de 11k draws, segundo a própria frente |
| 44 | Sombras: SAMPLED_BIT no depth em vez da cópia via buffer | Visual | Se o `GPUPROF` com sombra 4x apertar |

**Ordem dentro da Onda 2:** 19 → 20 → 21 → 22 → 24 (fila do be-thread em série) e 23 → 25 na janela noturna. 26 roda em paralelo de código, mas o build sobe um por vez. 27 e 28 só com o gate do 18 verde. 33 por último.

## 3. Decisões que só o Thiago pode tomar

1. **Meta de produto:** 60 travado e liso agora, ou 120 com VRR oscilando entre 90 e 120? E o 120 vale para a média da cidade (~7k draws) ou também para as vistas de 11k (que exigem a 43)?
2. **VRR:** posso propor um window-rule `variable-refresh-rate true` para o MyGameRecomp no `~/.config/niri/config.kdl` e testar `SDL_VIDEODRIVER=wayland`?
3. **Janelas para bench e relift:** quando dá para rodar benches sem cap, o teste do relógio (E1) e o rebuild PPU total (~30-60 min, noturno)? O Rog pode ficar ligado para soaks noturnos?
4. **`scale=1 stretch=1` foi de propósito?** Enquanto o ultrawide não sai: esticado com FSR1, ou barras pretas em 2x com SMAA?
5. **Ultrawide:** vale o investimento (~45h nas fases 2 e 3) se o gate 18 mostrar que o p99 sai de 8,3 ms? Aceita 90-100 fps em 21:9? 32:9 entra no escopo? Cutscenes em Hor+ ou pillarbox? O FOV também vale para mira e cutscene?
6. **HUD:** aceita o SMAA amolecendo o texto do HUD na v1, ou o AA já tem que vir antes do HUD?
7. **Presets de qualidade:** "120 fps" (2x com SMAA) e "Qualidade" (SSAA 3x/4x, que pode cair abaixo de 120)? O Rog Ally entra como alvo de preset?
8. **Latência contra suavidade:** o presenter com 1 frame de folga custa +16,7 ms a 60 e +8,3 ms a 120. Topa?
9. **Autocuras de trava:** ligadas por padrão com log alto, ou só como diagnóstico até a causa estar provada?
10. **Diagnóstico por padrão:** `SPURS_JC_RING=1` e caixa-preta ligados no play.sh? E a pasta `work/runs/freezes/` fora da limpeza (o disco está ~90%)? Os .log de 18 e 19/10 foram apagados por você?
11. **Travas desde 19/10:** viu alguma (tela parada com áudio em loop)? Em que parte do jogo? Costuma trocar de save ou de config pela tela inicial (o que dispara o execve)?
12. **ESC e F1:** ESC abre o F1 ou a pausa do jogo? Com o F1 aberto, pausa via pulso de START (que pula cutscene) ou só bloqueia o pad?
13. **Saves:** 10 backups por slot está bom? Restaurar só pela tela inicial?
14. **Splash:** pular por padrão ou deixar como opção?
15. **Teclado e mouse:** o mapa padrão (LMB atira, RMB mira, WASD) serve? É prioridade para o .exe dos amigos?
16. **Referência visual:** o longplay do PS3 é o gabarito, ou "melhor que o PS3" pode divergir dele (halos menores, sombra mais fina)?
17. **Upscaler:** confirma que fica adiado e que, quando vier, é FSR 3.1/XeSS no Vulkan, não D3D12?
18. **Áudio:** já ouviu estalo ou corte? Se sim, entra um contador de underrun e uma fila de 8 blocos. Volume próprio no F1 é prioridade?

Arquivos-chave:
- work/play.sh
- work/port/main.cpp
- work/port/ui.cpp
- work/port/settings.cpp
- work/src/recomp/ppu_recomp_002.cpp
- ps3recomp/libs/video/rsx_vulkan_backend.c
- ps3recomp/libs/video/cellGcmSys.c
- ps3recomp/libs/input/cellPad.c
- ps3recomp/libs/system/cellSaveData.c