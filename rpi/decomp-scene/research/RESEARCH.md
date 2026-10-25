# Research: native rewrite of the object system (decomp-scene)

Date: 2026-10-25. Done in the main session, without sub-agents (the user asked
to keep agent use down); the phases below follow the RPI template.

## Decision

**CONDITIONAL GO.** Confidence: medium.

Go for a pilot (phase 0 below) that proves the pipeline and measures what a
native function actually gains over the lifted one. Carry on into the
subsystem only if the pilot meets its exit criteria.

## 1. Requirements (from REQUEST.md)

- R1: native, readable C++ for `CXo`/`CSo` and the per-frame walk over them.
- R2: bit-exact with the lifted code, proven in compare mode in game before
  a function is switched on.
- R3: guest memory through the runtime's accessors (SPUs and lifted code see
  the same objects).
- R4: grow one function at a time; native and lifted call each other.
- R5: a repeatable far-view benchmark; measure every step.

No open clarification: the user chose this direction knowing the scale
(39,351 functions; the plan targets the few hundred that run in the far
view).

## 2. Product value

High for the player: the far view is where the game still drops to 40-55 fps
on the desktop (and lower on the Rog Ally), and it is the one place the
current approaches (lifter tuning, RSX work) have no lever left. Readable
code is also what the mod plans (`docs/ROADMAP.md`: shard map, co-op) need.

## 3. Technical discovery

What exists already:

- **Native stand-ins** (`work/tools/ppu_native_entries.py`,
  `work/port/ppu_native.cpp`): a hook at a lifted function's entry calls a
  registered native function, which can decline (return 0) and let the
  lifted body run. `PS3RECOMP_NATIVE_PPU=2` runs both on every call and
  compares the ABI-visible registers and written bytes. Native blocks can
  replace a loop inside a function. In use: `memcpy` (entry) and
  `func_00314BC8`'s byte scan (block).
- **RE kit** (`work/tools/re/`): 32,578 functions, 12,703 named, 553
  classes with layouts, vtables and script members. `who.py` gives callers,
  callees, data accesses, field offsets.
- **Class map:** `CBasic > CSpawnable > CLo > CXo > CXoSolve > CSo`.
  `CXo` (TRANSFORM) 0xA0 bytes, 30 slots, 19 children; `CSo` (OBJECT) 0x2B0
  bytes, 75 slots, 38 children.

What the hot functions are:

| Function | What | Size | Static callers |
|---|---|---|---|
| `0x46B568` | `CXo::get_pos` (script member `pos`) | 0xE8 | 695 |
| `0x313E84` | `CSo::vf25` (+146 classes share the slot) | 0x7E4 | virtual |
| `0x30B810` | `CSo::vf33` (+112 classes) | 0x16C | virtual |
| `0x314BC8` | unnamed, one caller (`0x1DB544`) | 0x7C (the lift merges the code after it) | 1 |

`get_pos` has a fast path with no calls: load the transform's matrix
(4 x 16 B), add the parent offset, three `vmaddfp` and a `vaddfp`. The slow
path (a parent joint) calls through a vtable and recurses.

What is not known yet:

- **The per-frame walk.** The hot members are virtual, so the static call
  graph does not show who iterates the objects. It needs a call-graph
  profile in game (the lifted code is built without frame pointers, so
  either `--call-graph=dwarf` on a short window or a temporary build with
  `-fno-omit-frame-pointer`).
- **How much faster native is.** The lifted code is already compiled C
  with VMX as SSE and guest registers in locals; the entry spends ~19% of
  several hot functions loading registers from the context. A native
  function skips that only where it is not itself called through the
  context. Unmeasured.

## 4. Feasibility

- Approach: native entries for whole functions where they are small and
  self-contained (leaves, getters, math), native blocks for hot loops inside
  big ones, and a native walk once the walk is found, calling lifted code for
  everything not rewritten yet.
- Complexity: per function, hours (a getter) to a day or more (a 0x7E4
  virtual with 3 callees). The subsystem is weeks.
- Risks:
  1. **Gain smaller than hoped.** A function rewritten as it is may run
     barely faster than its lift. Mitigation: the pilot measures it before
     anything else is written.
  2. **Float exactness.** VMX `vmaddfp` with NJ flush and the NaN rules of
     `ppu_vmx.h`. Mitigation: native code uses the same helpers; compare mode
     catches drift.
  3. **Virtual dispatch.** 146 classes share `CSo::vf25`'s slot, some with
     overrides. Rewriting a slot means rewriting what the base does, while
     overrides keep their lifted code.
  4. **Relifts.** A relift drops the hooks; `scripts/lift.sh` re-applies
     them already.

## 5. Phases proposed for the plan

- **Phase 0 (pilot):** native `CXo::get_pos` fast path, declining to the lift
  for the parent-joint path. Compare mode in game (a soak), then switch it on
  and measure: native time per call against lifted (a counter or perf), and
  main-thread time on the far view.
  - Exit criteria to go on: compare clean over a soak, and either
    >= 1.5x per call or a measurable main-thread gain. If neither, the
    rewrite only pays where it changes the algorithm, and phase 1 goes
    straight to the walk.
- **Phase 1:** a call-graph profile on the far view; find and document the
  per-frame object walk and the `SSoData`/`SXoData` fields it uses.
- **Phase 2:** the walk in native code, calling the lifted slots; then the
  slots by weight.
- **Phase 3:** algorithmic changes on top (culling, update rate), each its
  own decision.
- Benchmark: a save on a rooftop plus a pad script for `usoak.sh`, so every
  step is measured the same way.

## Scores

| | |
|---|---|
| Product value | High |
| Technical feasibility | Medium (pipeline exists; gain unproven) |
| Effort | High (weeks for the subsystem; hours for the pilot) |
| Risk | Medium, contained by compare mode and the pilot's exit gate |

## Phase 0 result (2026-10-25): pilot failed its exit criteria

`CXo::get_pos` written natively with the lift's accessors and VMX helpers,
values in locals instead of `ctx->vr`:

- Compare mode, 240 s soak: 65M calls, 0 differ, 0 declined.
- Cost, two 40 s whole-process profiles at the same point of the soak
  (`PS3RECOMP_NATIVE_PPU_OFF=46B568` for the lifted arm): lifted 0.15% of
  samples, native 0.32% (+0.01% left in the lifted entry for the hook).
  The native one is ~2x slower; its time is spread over the whole body
  (byte swaps, the range tests, byte-wise masks and splats, the hook's
  indirect call), not one fixable spot.

Taken out again (hook and code). What it settled: a function rewritten as
it stands does not beat its lift here, so the rewrite pays only where it
changes what the game does (phase 1 onwards). `PS3RECOMP_NATIVE_PPU_OFF`
stays: it turns single entries off for an A/B.

## Phase 1 findings (2026-10-25): the object "solve" loop is an SPU wait

Call-graph profile of the main thread (`perf --call-graph=dwarf`; the lift's
calls are real C calls, so the host stack is the guest's call tree), on the
ground in the city:

```
29.2%  002D07CC > 0043E1B4 > 000C7034 (18%) > vt_008D63E0::vf3 (9%)
25.5%  00314BC8 (the frame's object pass; the lift merges several functions)
         10.7% CSo::vf33     5.8% 00479660 > 00371000 > usleep
21.2%  0042C488 > CSkillManager::vf5 > CHeroGunSkill::vf8 (8.9%, once a frame)
```

Call counts (`PS3RECOMP_PPU_PROBE=1`, hooks that count and decline):
`CSo::vf33` 1.0-2.4M calls/s, ~22,000 a frame; `CXo::get_pos` ~2,500 a
frame; `CHeroGunSkill::vf8` once a frame (~1 ms each).

What 00314BC8 does there (0x315158-0x315390): a multi-pass resolver over the
frame's objects. `done[]` (bytes, at 0xBC(r1)), `dep[]` (index or -1, at
0x7C(r1)), objects at [r31+4]. Each pass calls slot 2 of each undone object
whose dependency is done (CSo::vf33 through a thunk); 2 marks it done, any
other value retries it next pass; passes repeat until every object is done.

vf33's results: 1 ~99%, 2 ~17k/s (~230 a frame), 0 ~3.5k/s; the same object
twice in a row is rare. With a parent (0x1D8) it runs CSo::vf45 (the matrix
solve, always 2). Without one it calls `func_00032C50`, which returns 1 when
`func_003707BC` > `func_00504D90` (the SPU job-list progress poll, see
docs/NOTES.md "The SPU slack") says the object's job is not finished.

So ~100 passes a frame are the main thread polling every waiting object's
SPU job until it lands: a spin on SPU latency, about 12% of the main thread
on the ground (vf33 plus the pass loop), not computation. Reordering the
objects would not shorten the frame; the jobs finishing sooner would, and so
would giving the main thread other work while they run.

Consequence for the plan: the far-view cost the main thread shows is partly
this wait. Before rewriting game code, find which SPU jobs these objects
wait for (the job at [obj+0x618], state byte 0x66C) and what delays them.
