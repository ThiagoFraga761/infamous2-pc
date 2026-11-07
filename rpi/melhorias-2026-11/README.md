# Melhorias de 07/11/2026: as três levas

Uma rodada de agentes em paralelo sobre o port e o ps3recomp, guiada pelo
backlog desta pasta. Cada frente foi feita num branch próprio, revisada em
contexto separado e integrada com kill switch. As variáveis estão em
[docs/OPCOES.md](../../docs/OPCOES.md#6-kill-switches-das-três-levas-de-07112026).

| Arquivo | O que é |
|---|---|
| `BACKLOG-onda1.md` | O backlog: o que não se confirmou no código, conflitos entre frentes, as ondas e as decisões que eram do Thiago |
| `perf-rsx-20261107-rsx.txt`, `perf-rsx-20261107-rsx-record.txt` | Perfis da thread do RSX e da de gravação na área pesada, com a leva 2 já integrada. Foram o ponto de partida da leva 3 |

Os relatórios completos de cada frente (resumo, testes, riscos, revisão) e os
`perf.data` ficaram fora do repositório: são grandes e citam caminhos locais.

## Leva 1 (port `626b7c4`)

| Frente | O que entrou |
|---|---|
| `draw-state` | Bind de textura e lookup de fragment program repetidos pulados; fila de gravação publicada em lotes |
| `pipe-warm` | Pipelines já vistos recriados em segundo plano na partida |
| `main-waits` | Esperas nativas dos dois polls da main e do Kicker, e limite de FPS na main (opt-in) |
| `post-aa` | Cadeia de pós: FXAA, SMAA 1x, FSR 1, média por área, nitidez |
| `fov` | Campo de visão ajustável por um wrap do `CView::SetFov` |
| `freeze-box` | Estado do runtime gravado a cada stall; contadores de sinais perdidos |
| `edge-native` | Descompressão e compressão de vértices Edge em C (opt-in nesta leva) |

## Leva 2 (port `bb80d0c`)

| Frente | O que entrou |
|---|---|
| `main-waits-2` | `PS3_MAIN_WAITS=1` como pacote; Kicker acordado pelos labels do RSX; esperas das job-lists WWS |
| `pipe-async` | Variantes novas de pipeline montadas nas threads `vk-warm` |
| `draw-state-2` | Publicação quando o walker para, memo de vertex programs, só as linhas de constantes que mudaram |
| `uw-axis` | Escala de render por eixo (Hor+), sombra e reflexo com escala própria |
| `kbm` | Teclado e mouse como pad |
| `hitch-stream` | Linha `[vk hitch]` com a causa, anel de vértices de 64 MB, texturas ao lado da thread de gravação (fixado em 0 no port) |
| `water-clip-pcf` | Planos de corte do usuário e PCF largo (opt-in) |
| `edge-more` | Kernel de índices Edge (opt-in) e o layout do 0x4448 |

Na praça do save atual (~9 mil draws), sem limite: 98–100 fps. Com
`PS3_MAIN_WAITS=1` e limite de 60, a main caiu de 99% para 46%, com p99 de
16,9 para 17,7 ms. O gargalo passou a ser a thread do RSX, a 91%.

## Leva 3 (port `ddce81c`)

| Frente | O que entrou |
|---|---|
| `fifo-walker-2` | Laço rápido do walker com escritas de registrador em lote para o motor e o dispatcher |
| `state-hash` | Render state, superfície e programas guardados enquanto os registradores não mudam |
| `draw-end-2` | Limites de índice com SSE4.1, fim de draw enxuto, constantes do VS rastreadas por marca |
| `tex-bind-2` | Unidades de textura carregadas de draw para draw, memo por frame, hint para a fila |
| `rec-copies` | Cache de descriptor sets por conteúdo, cópias de linhas inline na thread de gravação |

Mesma praça, sem limite: 90,6 fps com os switches da leva desligados e 107,7
ligados (+19%). A main ficou a 91%, o RSX a 87%, a gravação a 87% e os SPUs a
85%: tudo perto de saturar junto.

Depois das levas, o port `fcca056` ligou por padrão a descompressão e a
compressão Edge nativas: zero diferenças em ~2,5 milhões de chamadas, 1,15 µs
contra 4,9 µs.

Teste de integração das levas 2 e 3 no ps3recomp:
`bash libs/video/tests/run_fifo_walker.sh <dir>`. Ele pegou um conflito entre
branches: o `state-hash` tinha tirado o BEGIN_END do caminho completo. A
correção (`4ced5f8` no ps3recomp) entrou antes da integração no port.
