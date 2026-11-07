# Credits

- **[ps3recomp](https://github.com/sp00nznet/ps3recomp)**, by Ned Heller
  (sp00nznet) and its contributors (MIT). This port is ps3recomp plus the
  changes in `work/patches/ps3recomp.patch`. The toolkit snapshot this work
  started from already carried earlier inFAMOUS-specific and Linux work on top
  of upstream; the patch holds all of it.
- **[rpcs3-infamous2](https://github.com/ttmx/rpcs3-infamous2)** by ttmx
  (GPL-2.0). Its reverse engineering of the game's SPU ambient-occlusion and
  lighting jobs (algorithm, record layouts, validation method) is the
  specification the GPU passes here were written from; the passes themselves
  are written anew.
- **[RPCS3](https://rpcs3.net)** (GPL-2.0): the behavioural reference for many
  RSX and lv2 details (cited where they are used), and the tool used to decrypt
  the executable.
- **[FidelityFX FSR 1](https://github.com/GPUOpen-Effects/FidelityFX-FSR)** (AMD,
  MIT) and **[SMAA](https://github.com/iryoku/smaa)** by Jorge Jimenez et al.
  (MIT): the shader sources of the present chain's FSR 1 and SMAA passes,
  kept unmodified with their licences in the toolkit patch
  (`libs/video/vulkan/fsr1/`, `libs/video/vulkan/smaa/`).
- **[Dear ImGui](https://github.com/ocornut/imgui)** (MIT): the launcher and the
  in-game menu. `scripts/setup.sh` fetches it at a fixed tag.
- Written with AI assistance (Claude).
