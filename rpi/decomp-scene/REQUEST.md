# Native rewrite of the game's object system, by subsystem

## Problem

Looking from a rooftop or far across the city, the frame rate drops from
~90 to 40-55 fps (desktop, 2026-10-25 session). There the main PPU thread is
100% busy and ~80% of it is the game's own code; it waits only ~15% on SPU
job lists and ~5% on the RSX. Draws go from ~6,000 to 10,000-15,000 (peaks of
39,000).

The lifted code is already reasonable (VMX as SSE, guest registers in
locals), and the main thread's profile is flat: the hottest function is 2.7%,
the top 10 are ~12%, none has a hot loop worth swapping for a better
algorithm, and several spend ~19% in the entry that loads registers from the
context. Rewriting single functions as they are gains little. What the
lifted code can never give is a change of *algorithm*: how the game walks its
objects, decides what to update and draw, and how far.

## Goal

Rewrite the game's object system in readable native C++, subsystem by
subsystem, starting with what the far-view frame spends its time in, so that
it can then be profiled by meaning and improved (culling, LOD, update
frequency, data layout).

- First subsystem: `CXo` (transform: position, matrices; 0xA0 bytes, 30
  vtable slots) and `CSo` (scene object; 0x2B0 bytes, 75 slots, 38 child
  classes), and the per-frame walk over them. Hot members seen so far:
  `CXo::get_pos` (0x46B568), `CSo::vf25` (0x313E84), `CSo::vf33` (0x30B810),
  `func_00314BC8` (same code region, unnamed).
- Every rewritten function runs beside the lifted one in compare mode
  (`PS3RECOMP_NATIVE_PPU=2`) until it matches over a long in-game run, and
  only then replaces it. The game never runs on unverified native code.
- Measure at each step on a fixed far-view spot, main-thread time and fps.

## Measured before

- Rooftop/far view: 40-55 fps; main thread 99-100%; rsx 85-90%.
- Main-thread code share: 20 functions = 25%, 77 = 50%, 217 = 75%,
  372 = 90% (of 1,661 lifted functions seen, of 39,351).

## Constraints

- Bit-exact with the lifted code (floats included: VMX NJ flush and NaN
  rules as `ppu_vmx.h` has them), until a step deliberately changes
  behaviour, and such a step is its own decision.
- The native code reads and writes guest memory through the same accessors
  (big-endian, reservations), so the SPUs and the rest of the lifted code see
  the same objects.
- Nothing from the game ships: the rewrite is our code; names come from
  `work/tools/re/`.

## Out of scope (for now)

- Whole-game decompilation. Subsystems that never show in a profile (menus,
  saves, audio, scripts) stay lifted.
- Changing what the game draws (LOD distances): later, as a decided step on
  top of the rewritten code.

## Open questions for research

1. Where is the per-frame object walk, and what calls `func_00314BC8`,
   `CSo::vf25/vf33`, `CXo::get_pos`, and how many objects a frame?
2. Which of the 75 `CSo` slots run every frame, which are per-class
   overrides, and how deep the call trees under them go before leaving the
   object system (physics, rendering, SPU job kicks)?
3. The `SSoData`/`SXoData` layouts the hot code touches.
4. Can a native function call lifted ones (and the other way) cheaply
   enough that the rewrite can grow one function at a time?
5. A fixed, repeatable far-view benchmark (a save on a rooftop and a pad
   script), so each step is measured the same way.
