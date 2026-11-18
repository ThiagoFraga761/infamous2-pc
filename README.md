<p align="center">
  <img src="docs/screenshots/banner.jpg" alt="inFAMOUS 2 running natively on Linux" width="100%">
</p>

<h1 align="center">inFAMOUS 2 — native PC port</h1>

<p align="center">
  <b>PS3 static recompilation · Linux · Vulkan · up to 4K · unlocked frame rate · keyboard and mouse · Brazilian Portuguese dub</b>
</p>

<p align="center">
  <a href="https://github.com/alvarorichard/infamous2-pc/actions/workflows/ci.yml"><img src="https://github.com/alvarorichard/infamous2-pc/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <a href="https://github.com/alvarorichard/infamous2-pc/releases"><img src="https://img.shields.io/github/v/release/alvarorichard/infamous2-pc?include_prereleases&label=release" alt="Release"></a>
  <img src="https://img.shields.io/badge/status-playable%2C%20in%20development-orange" alt="Status: playable, in development">
  <img src="https://img.shields.io/badge/platform-Linux%20x86--64-1793D1?logo=linux&logoColor=white" alt="Linux x86-64">
  <img src="https://img.shields.io/badge/GPU-Vulkan-AC162C?logo=vulkan&logoColor=white" alt="Vulkan">
  <img src="https://img.shields.io/badge/PS3-BCUS98125-003791?logo=playstation&logoColor=white" alt="PS3 BCUS98125">
  <img src="https://img.shields.io/badge/up%20to-4K-6f42c1" alt="Up to 4K">
  <img src="https://img.shields.io/badge/dub-PT--BR-009c3b" alt="Brazilian Portuguese dub">
  <img src="https://img.shields.io/badge/code-MIT-blue" alt="Code: MIT">
</p>

<p align="center">
  <b>English</b> |
  <a href="README.pt-BR.md">Português (Brasil)</a>
</p>

<p align="center">
  <a href="#quick-start">Quick start</a> ·
  <a href="docs/OPCOES.md">Options</a> ·
  <a href="docs/ROADMAP.md">Roadmap</a> ·
  <a href="CHANGELOG.md">Changelog</a> ·
  <a href="docs/NOTES.md">Technical notes</a>
</p>

**inFAMOUS 2** (PS3, BCUS98125, version 01.00) running natively on Linux, with
no emulator:

- the game's PowerPC (PPU) and SPU code is translated to C ahead of time by
  static recompilation with [ps3recomp](https://github.com/sp00nznet/ps3recomp);
- the PS3 operating system is reimplemented;
- the RSX (the PS3's GPU) runs the game's own shaders, translated to Vulkan.

> [!IMPORTANT]
> **No game files come with this project, and there are no binaries to
> download.** You need your own copy, dumped from your own PS3. The build
> turns **your** executable into code on **your** machine, and that code must
> not be shared.

> [!NOTE]
> **Playable, in development.** The game boots, plays its videos, menus and
> cutscenes, and runs from Empire City into New Marais, with sound,
> controller, keyboard and mouse, saves and the Brazilian Portuguese dub. In
> the heaviest area measured (~9,000 draws a frame) it runs above 100 fps
> uncapped and holds the default 60 fps cap. See [Status](#status).

| | |
|---|---|
| ![Cole on the Empire City pier](docs/screenshots/01-pier.jpg) | ![Opening cutscene](docs/screenshots/02-cutscene.jpg) |
| ![Walkway in the New Marais swamp](docs/screenshots/03-new-marais.jpg) | ![New Marais streets](docs/screenshots/04-cidade.jpg) |

![Empire City seen from the pier](docs/screenshots/05-empire-city.jpg)

<sub>Captured straight from the GPU on Linux, with a Radeon RX 9070 XT, at
1280×720. The Empire City shots were rendered at 2× (2560×1440) and scaled
down.</sub>

## Contents

- [Highlights](#highlights)
- [Quick start](#quick-start)
  - [Step 0: check your PC](#step-0-check-your-pc)
  - [Step 1: get the game files](#step-1-get-the-game-files)
  - [Step 2: get the code](#step-2-get-the-code)
  - [Step 3: set up the tools](#step-3-set-up-the-tools)
  - [Step 4: translate the game](#step-4-translate-the-game)
  - [Step 5: build](#step-5-build)
  - [Step 6: play](#step-6-play)
  - [If something goes wrong](#if-something-goes-wrong)
- [Using the port](#using-the-port)
  - [Main menu](#main-menu)
  - [In-game options](#in-game-options)
  - [Controls](#controls)
  - [Language](#language)
  - [Saves and data](#saves-and-data)
  - [Logs and reports](#logs-and-reports)
- [Status](#status)
  - [Performance](#performance)
  - [PS3 versus this port](#ps3-versus-this-port)
  - [Known problems](#known-problems)
- [How it works](#how-it-works)
- [Documentation](#documentation)
- [Roadmap](#roadmap)
- [FAQ](#faq)
- [Contributing](#contributing)
- [Credits and legal notice](#credits-and-legal-notice)

## Highlights

| | |
|---|---|
| **Native, no emulator** | PPU and SPU code becomes x86-64 C code; the RSX becomes Vulkan. |
| **Sharper picture** | 720p, 1440p, 4K or 5K (supersampling), FXAA or SMAA 1x, FSR 1 upscaling, area-average downscaling, adjustable sharpening. |
| **Smooth** | A steady 60 fps with the default cap, 100+ fps uncapped in the heaviest area measured. |
| **Your way to play** | Xbox, DualShock 4 and DualSense pads with rumble, or keyboard and mouse. |
| **Wider view** | Field of view adjustable up to +30%, and an experimental ultrawide mode. |
| **Brazilian Portuguese** | Dub and text, picked separately in the menu. |
| **Safe saves** | Written whole or not at all: a crash mid-save keeps the previous one. |
| **Easy to report bugs** | Stalls and slow frames write a "black box" you can attach to an issue. |

What changed in each version is in the [changelog](CHANGELOG.md). In short,
v0.2.0 brought fewer stutters (new shaders and textures are built off the
frame), native Edge jobs on by default, and CI on Linux, Windows and macOS.

## Quick start

Six small steps, from nothing to playing. Do them in order, and check the line
that starts with **Done when** before moving on.

> [!TIP]
> Never built anything like this before? That is fine. Every command below is
> copy and paste. The slow parts are the first translation and the first
> build, so start them and go for a coffee.

### Step 0: check your PC

You need:

- **Linux x86-64.** Tested on Fedora 44.
- **A CPU with AVX2.** The SPU code is built for AVX-512 by default; on a CPU
  without it, build with `PORT_SPU_MARCH=x86-64-v3` in step 5.
- **A Vulkan GPU** whose graphics queue also does compute.
- **~14 GB of free RAM** and **~3 GB of free disk** to build, plus the game.
- 8 cores with SMT are best (thread pinning); with fewer, it runs without that
  gain.

Install the packages for your distribution.

**Fedora** (the tested one):

```sh
sudo dnf install gcc gcc-c++ clang cmake ninja-build python3 SDL2-devel \
    vulkan-headers vulkan-loader-devel glslang-devel libpng-devel \
    vulkan-tools git
```

**Arch Linux** and derivatives (Manjaro, EndeavourOS):

```sh
sudo pacman -S --needed base-devel git clang cmake ninja python sdl2 \
    vulkan-headers vulkan-icd-loader vulkan-tools glslang libpng
```

Also install the Vulkan driver for your GPU: `vulkan-radeon` (AMD),
`vulkan-intel` (Intel) or `nvidia-utils` (NVIDIA).

**Debian, Ubuntu** and derivatives (Linux Mint, Pop!_OS):

```sh
sudo apt install build-essential git clang cmake ninja-build python3 \
    python3-venv libsdl2-dev libvulkan-dev vulkan-tools glslang-dev \
    glslang-tools libpng-dev
```

Also install the Vulkan driver for your GPU: `mesa-vulkan-drivers` (AMD and
Intel) or the NVIDIA driver package.

> [!NOTE]
> The Arch and Debian/Ubuntu lists are the Fedora packages translated by name;
> only Fedora 44 was actually tested. If a package name differs on your
> release, search for the equivalent. The build script also needs `systemd`
> (for its memory scope), which these distributions ship by default.

`libpng` only draws the disc's art behind the menu; without it the background
is plain.

**Done when:** `vulkaninfo --summary` (from your distribution's Vulkan tools
package) lists your GPU, and `clang --version` and `cmake --version` print a
version.

### Step 1: get the game files

All of it comes from **your** PS3 and none of it is in this repository. The
full walkthrough is in [docs/ARQUIVOS_NECESSARIOS.md](docs/ARQUIVOS_NECESSARIOS.md)
(in Portuguese; the English summary is at the end of that file). You need
three things:

1. **The disc:** inFAMOUS 2, **BCUS98125 (US), version 01.00**, dumped from
   your own disc. The folder `disc/PS3_GAME/` must end up at the root of this
   repository (step 2 creates the repository; you can copy the disc in after).
2. **The decrypted executable:** decrypt `disc/PS3_GAME/USRDIR/EBOOT.BIN` to
   `disc/PS3_GAME/USRDIR/EBOOT.elf`, for example with RPCS3 (Utilities >
   Decrypt PS3 Binaries).
3. **The firmware:** install the official PS3 firmware (PUP) in
   [RPCS3](https://rpcs3.net), which creates `dev_flash`. If yours is in an
   unusual place, set `PS3_DEV_FLASH=<folder>`.

**Done when:** `disc/PS3_GAME/USRDIR/EBOOT.elf` exists (step 4 checks its
SHA-256 for you).

### Step 2: get the code

```sh
git clone https://github.com/alvarorichard/infamous2-pc.git
cd infamous2-pc
```

Now put the `disc/` folder from step 1 here, so that
`disc/PS3_GAME/USRDIR/EBOOT.elf` exists.

**Done when:** `ls disc/PS3_GAME/USRDIR/EBOOT.elf` prints the path.

### Step 3: set up the tools

```sh
scripts/setup.sh
```

This fetches ps3recomp at its base commit and applies this port's patch,
fetches Dear ImGui, and creates a Python virtual environment. It does not
touch your game files.

**Done when:** the script ends without an error, and the folders `ps3recomp/`
and `.venv/` exist.

### Step 4: translate the game

```sh
scripts/lift.sh
```

This translates the PPU and SPU code of **your** `EBOOT.elf` to C. It takes a
few minutes. If the executable is not BCUS98125 01.00, the script warns you:
the addresses in this port are for that exact version.

**Done when:** the script ends without an error. The translated code lands in
`work/`, and it is yours alone: never upload it.

### Step 5: build

```sh
scripts/build.sh
```

The first build is slow because the translated PPU code is large. It runs in a
14 GB systemd memory scope so it cannot take your desktop down with it.

| Variable | Effect |
|---|---|
| `JOBS=<n>` | how many compile jobs run in parallel |
| `PORT_PPU_GCC=1` | build the PPU code with GCC instead of clang |
| `PORT_SPU_MARCH=x86-64-v3` | for CPUs without AVX-512 |
| `PS3RECOMP_DIR=<folder>` | use a ps3recomp that lives somewhere else |

**Done when:** the script ends without an error and `work/port/build-vk`
exists.

### Step 6: play

Plug in your controller first, if you use one, then:

```sh
work/play.sh
```

You land on the port's main menu. Pick **New game** (or **Continue**) and
play. Press **F1** or **Esc** at any time to open the options.

**Done when:** the menu shows up over the disc's art. Every session writes a
log to `work/runs/play-<date>.log`.

Other ways to start:

```sh
work/play.sh --play     # straight into the game, without the port's menu
work/play.sh --title    # straight to inFAMOUS 2's new-game screen
```

### If something goes wrong

| Symptom | What to try |
|---|---|
| `lift.sh` says there is no decrypted executable | Check that `disc/PS3_GAME/USRDIR/EBOOT.elf` exists (step 1, item 2). |
| `lift.sh` warns about the SHA-256 | You have a different version or region. Only BCUS98125 01.00 is supported. |
| `build.sh` runs out of memory | Lower the parallelism: `JOBS=4 scripts/build.sh`. |
| Your CPU has no AVX-512 | Build with `PORT_SPU_MARCH=x86-64-v3 scripts/build.sh`. |
| English audio instead of Portuguese | Pick the dub in F1 > Language. See [Language](#language). |
| The picture freezes but audio goes on | A rare known deadlock. Wait ~20 s so the black box is written, then see [Logs and reports](#logs-and-reports). |

Still stuck? Open an issue with the session log (see
[Logs and reports](#logs-and-reports)).

## Using the port

### Main menu

On start you get the port's own menu, over the disc's art (`PIC1.PNG`). It
works with a pad, keyboard or mouse.

| Item | What it does |
|---|---|
| **Continue** | enters the most recent save (shows name, progress and date) |
| **New game** | opens inFAMOUS 2's own new-game screen |
| **Load game** | lists your saves; pick one |
| **Options** | the tabs below |
| **Quit** | closes the game |

inFAMOUS 2 has no main menu of its own: with a save it goes straight into the
autosave, and without one it only offers "Start a new game". The port's menu
fills that role. Before **New game**, or before loading a save that is not the
autosave, your saves are copied to `user/backups`, because the game's next
autosave overwrites the autosave slot, as on the console. Inside the game:
**Start > Saved data**.

### In-game options

Press **F1**, **Esc**, or hold **Select+Start** to open the options in the
middle of the game. Everything is saved to `user/settings.ini`.

| Tab | Options |
|---|---|
| **Video** | Resolution (720p, 1440p, 4K, 5K supersampling) · Anti-aliasing (off, FXAA, SMAA 1x) · Upscale to window (bilinear, FSR 1) · Downscale to window (bilinear, area average) · Sharpening (auto, or 0 to 1) · Full screen · VSync · Stretch the image (no bars) · Field of view (the game's up to +30%) · Monitor |
| **Language** | Dub and subtitles/text, separately (you can mix them) |
| **Performance** | Preset (Performance 720p, Balanced 1440p, Quality 4K) · FPS limit (none, 30, 40, 60, 80, 120, 240, display rate) · On-screen FPS counter · Vulkan recording thread |
| **Mods** | Full-resolution particles · Anisotropic filter (the game's, 2x to 16x) · Installed mods ([docs/MODS.md](docs/MODS.md)) |
| **Controller and saves** | Detected controller and button test · Open the saves folder · Back up saves · Open the logs folder |
| **Keyboard and mouse** | Play with keyboard and mouse · Mouse sensitivity (0.10–5.00) · Invert vertical axis · Stick start deflection (0–60%) · Key list |

- Resolution, monitor, language and the recording thread apply the next time
  the game opens. The rest changes on the spot.
- An environment variable beats the file, and its field turns grey in the
  menu (`PS3RECOMP_RES_SCALE=2 work/play.sh`). The full list is in
  [docs/OPCOES.md](docs/OPCOES.md).

### Controls

**Pad** (Xbox, DualShock 4, DualSense): as on the PS3. Hold **Select+Start**
to open the port's menu.

**Keyboard and mouse**, alongside a pad. The mouse is captured on the first
click or key press; Alt+Tab and F1 release it.

| Key | PS3 button |
|---|---|
| **W A S D** | left stick (walk) |
| **Mouse**, or **I J K L** | right stick (camera) |
| **Left** / **right button** | R1 (shoot) / L1 (aim) |
| **Space** | Cross |
| **Left Shift** or **Backspace** | Circle |
| **F** or **side button 4** | Square |
| **E** or **side button 5** | Triangle |
| **Q** / **R** | R2 / L2 |
| **Left Ctrl** / **V** or **middle button** | L3 / R3 |
| **Enter** or **P** / **Tab** | Start / Select |
| **Arrows** or **1 2 3 4** | D-pad |

To rebind keys, edit `[keyboard] map` in `user/settings.ini`, in the form
`Key=BUTTON` (for example `F=SQUARE Q=NONE`). The `WALK` action (walk slowly)
and the mouse wheel (`WheelUp`, `WheelDown`) exist, but have no key in the
default map.

**Port keys:**

| Key | What it does |
|---|---|
| **F1** or **Esc** | opens and closes the menu |
| **F11** or **Alt+Enter** | full screen / window |

### Language

The disc carries every region's voices and text (`brazilian.psarc`,
`brazilian_text.psarc`, `latino.psarc`...), and the game picks a pair from the
console language:

- The game predates the PS3's "Portuguese (Brazil)" (number 17), so it treats
  that number as unknown and falls back to English.
- It reaches its Brazilian files through "Portuguese" (7), which its North
  American list pairs with `brazilian`.
- The port picks that number from the subtitle language, and serves the voice
  and text packs chosen in the Language tab.

### Saves and data

- Saves live in `gamedata/dev_hdd0/home/00000001/savedata/`. Each file is
  written beside the old one and renamed over it, so a crash mid-save keeps
  the previous save.
- `PS3_SAVEDATA_ROOT=<folder>` uses another folder. Tests and benchmarks use
  it so they never touch a player's saves.
- Options live in `user/settings.ini`, and save backups in `user/backups/`.

### Logs and reports

Each `work/play.sh` session writes `work/runs/play-<date>.log`, with an
`[rsx fps]` line every 10 s. Captures are automatic:

- **Slow frame:** a `[vk hitch]` line with its cause (new pipeline, GPU wait,
  texture created, what each thread waited on).
  `work/tools/hitch_classify.py <log>` groups the lines by cause.
- **Stall of 2.5 s or more:** every thread's stack and the runtime state go to
  `work/runs/freezes/<log>/`, right away and again 15 s later.
- **Freeze:** after 15 s without a frame, a 5 s perf capture too. If the game
  freezes, wait ~20 s before closing it.
- **Slow stretch:** after 30 s under 70 fps (or 5 under the cap, if the cap is
  lower), a 20 s profile (up to three per session).

To report a problem, send the log and its `work/runs/freezes/` folder. They
hold no game files.

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
| Windows and macOS | ❌ Game not supported: the toolkit builds and links (CI), but the game was never run there |

### Performance

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
- Where each frame's time goes is in [docs/NOTES.md](docs/NOTES.md).

### PS3 versus this port

| | PS3 | This port |
|---|---|---|
| Resolution | 1280×720 | 720p, 1440p, 4K or 5K, with FSR 1 or area downscale to the window |
| Frame rate | 30 fps target | 60 with the default cap; ~100–130 uncapped |
| Anti-aliasing | — | FXAA or SMAA 1x |
| Field of view | fixed | up to +30% |
| Ultrawide monitor | — | black bars, stretched, or experimental Hor+ |
| Controls | DualShock 3 | pad through SDL2, keyboard and mouse |
| Execution | Cell (PPU + SPUs) and RSX | code translated to x86-64, GPU on Vulkan |

### Known problems

- **Rare mid-game deadlock:** the picture stops while the audio goes on. The
  main thread waits on an event flag and job chain `0x00B19600` sits at guard
  `0x00B15000`. Seen on Oct 16, 17 and 18, about once per 15–60 minutes of
  play. The cause is not found yet, and the 2026-11-07 session had none. If it
  happens, the black box records the state.
- **Ultrawide Hor+ is experimental:** the centred HUD was only checked in a
  windowless bench, and markers pinned to the world (targets, icons over
  people) still use the 16:9 position. With it off, 16:9 shows with bars or
  stretched, and the menu's field of view works as usual.
- **Keyboard and mouse, phase 1:** aiming goes through the game's stick
  acceleration, so it is not 1:1. Keys are rebound only in `settings.ini`.
- **AA runs after the HUD,** so HUD text is a little softer.
- **Cold cache:** on the first session, or after a driver update, the first
  time each effect appears can still cost a frame. Later ones come from the
  cache.
- **Windows and macOS untested:** a MinGW cross build
  (`work/port/windows/play.bat`) links, but nobody has run the game on it.
  RSX memory write tracking and stack capture are Linux only.

## How it works

- **PPU:**
  - The executable's ~39,000 functions become C (`ppu_lifter.py`).
  - The result is built by clang for x86-64-v3, with PS3 memory accesses
    inlined.
  - The system modules are reimplemented (HLE).
  - A few game functions have native stand-ins (`work/port/ppu_native.cpp`),
    such as `CView::SetFov` for the field of view.
- **SPU:** every SPU image the game runs is translated to C and built on x86
  SIMD (SSE4.1, AVX2, AVX-512):
  - the job manager;
  - the 27 job modules;
  - the occlusion plugins;
  - the Wwise job chains;
  - Bink and zlib.

  The heaviest jobs (Edge, `ec5f`, the scene walker, the instance culler) have
  hand-written C versions, each with a mode that compares it with the
  translation.
- **RSX:**
  - A draw engine reads the RSX register file and runs the game's own vertex
    and fragment programs, decompiled to HLSL and compiled to SPIR-V.
  - The Vulkan backend records on a thread of its own.
  - A post chain (AA, FSR 1, downscale) runs between the frame and the window.
- **GPU passes:** ambient occlusion (SSAO) and the tiled deferred lighting,
  two SPU jobs, run on the GPU, each checked against the SPU's output.
- **Write tracking:**
  - RSX local memory is watched by page protection.
  - Textures and geometry that nothing rewrote are not decoded again.

Details, with addresses and measurements, are in
[docs/NOTES.md](docs/NOTES.md).

## Documentation

The deep-dive documents are written in Portuguese.

| Document | What it covers |
|---|---|
| [ARQUIVOS_NECESSARIOS](docs/ARQUIVOS_NECESSARIOS.md) | Which files to take from your PS3 |
| [OPCOES](docs/OPCOES.md) | Every option and environment variable, with kill switches |
| [ESTRUTURA](docs/ESTRUTURA.md) | Map of the repository |
| [MODS](docs/MODS.md) | Replacing game files with mods |
| [NOTES](docs/NOTES.md) | Reverse engineering and measurements |
| [ROADMAP](docs/ROADMAP.md) | What is done and what comes next |
| [CHANGELOG](CHANGELOG.md) | What changed in each version |
| [GITHUB_SETUP](docs/GITHUB_SETUP.md) | Repository settings and how to cut a release |

## Roadmap

See [docs/ROADMAP.md](docs/ROADMAP.md). The goal is **a steady 120 fps**,
measured by frame time rather than average. Before that:

- find the cause of the rare deadlock;
- measure the Edge index kernel and the main-thread waits in game, to turn
  them on by default;
- a centred HUD for ultrawide;
- 1:1 mouse aiming.

## FAQ

**Is there an official inFAMOUS 2 for PC?** No. This project runs the PS3
game natively on Linux, with no emulator, through static recompilation.

**Can I play inFAMOUS 2 on PC without RPCS3?** Yes, that is what this port
does, with your own copy of the game. RPCS3 is only used to decrypt the
executable and install the firmware.

**Is there a ready-made executable to download?** No, and there will not be:
the executable holds the game's translated code. Everyone builds from their
own disc.

**Does it run on Windows?** Not yet. See [Known problems](#known-problems).

**Does the game come with it?** No. No game file is part of the project.

## Contributing

Contributions are welcome.

1. Fork the repository and create a branch.
2. Run `scripts/install-hooks.sh` once. It installs hooks that run
   `scripts/check_clean.sh` before every commit and push, so no game file or
   translated code ever gets in.
3. Read the [repository map](docs/ESTRUTURA.md) to find your way around.
4. Open a pull request describing what you changed and how you measured it.

Bug reports are most useful with the session log and its
`work/runs/freezes/` folder (see [Logs and reports](#logs-and-reports)).

## Credits and legal notice

- **[ps3recomp](https://github.com/sp00nznet/ps3recomp)**, by sp00nznet and
  contributors (MIT): this port is ps3recomp plus
  `work/patches/ps3recomp.patch`.
- **[rpcs3-infamous2](https://github.com/ttmx/rpcs3-infamous2)**, by ttmx: the
  reverse engineering of the occlusion and lighting jobs, which served as the
  specification for the GPU passes.
- **[RPCS3](https://rpcs3.net)**: the behavioural reference for the RSX and
  lv2, and the tool used to decrypt the executable.
- **[FidelityFX FSR 1](https://github.com/GPUOpen-Effects/FidelityFX-FSR)**
  (AMD, MIT) and **[SMAA](https://github.com/iryoku/smaa)** (MIT): the post
  chain's shaders, carried in the ps3recomp patch.
- See also [CREDITS.md](CREDITS.md). Written with AI assistance (Claude).

The code in this repository is MIT ([LICENSE](LICENSE)). inFAMOUS 2 is
© Sony Interactive Entertainment / Sucker Punch Productions. This project is
not affiliated with or endorsed by them, and contains no game files.
