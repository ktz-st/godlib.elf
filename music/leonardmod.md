# LeonardMod

`music/leonardmod.h` exposes Leonard's Paula emulator with the ProTracker 2.3f
replay. It supports STE/MegaSTE, 8-bit stereo, PAL 50 Hz, at **12517 or 25033 Hz**.
The engine and adapter now belong to `libgod.a`; `godlib.spl/leonard` only contains
the application and shares the four-bar artwork/kernel with `lancemod`.

```c
void *mod = LeonardMod_Load("MUSIC.MOD");
if (mod && LeonardMod_Start(mod, eLEONARDMOD_FREQ_12K)) {
    /* each main-loop frame: */
    Screen_Update();
    LeonardMod_Tick();
}
LeonardMod_ShutdownPaula();
LeonardMod_UnLoad(mod);
```

Use supervisor mode after `Platform_Init()`. Call Tick once per synchronized
main-loop frame, not inside VBL: the native core temporarily enters user mode,
uses USP for its internal stack and returns through trap #0. The bridge preserves
caller USP, SR and GCC callee-saved registers. It does not install a VBL callback.
DMA and trap #0 have one exclusive owner; stop other players first.

Start replaces any previous instance and resets per-song tracker state. Stop halts DMA and restores trap #0 but
retains the allocation; ShutdownPaula also frees it. Repeated shutdown is safe.
Allocation failure leaves no workspace or trap hook. Samples are prepared in
place: load fresh raw data before starting a MOD again, even at another rate.

## Resident memory

Like UGH's adaptation, one ST-RAM allocation holds only the selected format's
buffers. DMA stays in ST RAM even on machines with alternative memory.

| Component | 12.5K | 25K |
|---|---:|---:|
| Samples per generated block (per stereo side) | 254 | 504 |
| Four stereo DMA blocks | 2032 B | 4032 B |
| Period table (3424 × 4) | 13696 B | 13696 B |
| Eight generated code towers | 28496 B | 56496 B |
| Volume table and worst-case alignment | 16895 B | 16895 B |
| Base alignment allowance | 15 B | 15 B |
| **Total allocation** | **61134 B** | **91134 B** |

Generated code is `112*N+48` bytes, DMA `8*N`, and volume data `65*256`
bytes plus 255 bytes for alignment. The original shared BSS reservation was
194384 bytes, so the savings are **133250 B (68.5%)** at 12.5K and
**103250 B (53.1%)** at 25K. The earlier private 25K sample already used 91134 B;
there is no additional reduction to that same format from moving it into GodLib.
No temporary initialization heap buffer is needed.

`LeonardMod_GetPaulaWorkspaceSize(rate)` reports the selected total;
`LeonardMod_GetPaulaAllocatedSize()` reports the live allocation. Invalid rates
select 25K. Workspace size depends on rate, not song duration.

The loader validates the M.K. header, all 128 order slots (matching the native
sample offset calculation), finetunes, volumes and loop bounds. It allocates
exactly `1084 + 1024*patternCount + sampleBytes`, ignoring trailing file padding.
`LeonardMod_GetModuleInfo()` reports these numbers before preparation. Unlike
Lance, this replay does not create 664-byte guards or duplicate loop samples.
Empty slots point at shared silence. Duration alone does not predict MOD size.

## 12.5K pitch and looping

The DMA mode, block count and frequency table all change together. The 12.5K
frequency constant is twice the original 25K value, preserving the replay's
existing PAL tuning convention. The original 25K mixer clamps integer source
steps above one, which clips the highest notes if simply reused at half rate.
12.5K therefore keeps full 16.16 steps. A small looped fallback handles steps of
two or more and preserves overshoot across even very short sample loops, without
reserving another generated tower. The normal generated paths remain in use for
lower steps. Period lookup is bounded to the existing table. The output-rate
change does not change the 50 Hz tracker scheduling.

`LeonardMod_GetVisualizer()` returns four coherent retrigger ages, not amplitude.
Ages saturate at 0x4000; stopped playback returns idle. `SetRasterDebug(1)` colours
palette zero during Tick and restores it afterwards; this shows elapsed replay
time rather than a numeric CPU percentage.

The optional `GetOscilloscope` getter supplies four pitched, volume-scaled source
previews without changing replay state. O switches bars/scopes in the example;
V hides/shows the selected mode. See [the shared scope API](modscope.md).
