# LeonardMod

`music/leonardmod.h` exposes Leonard's Paula emulator with the ProTracker 2.3f
replay. It supports STE/MegaSTE, 8-bit stereo, PAL 50 Hz, at **12517 or 25033 Hz**.
The engine and adapter now belong to `libgod.a`; `godlib.spl/leonard` only contains
the application and shares the four-bar artwork/kernel with `lancemod`.

```c
void *mod = LeonardMod_Load("MUSIC.MOD");
U8 attached = 0;
if (mod && LeonardMod_Start(mod, eLEONARDMOD_FREQ_12K)) {
    U16 mask = System_GetIML();
    System_SetIML(7);
    attached = Vbl_AddCall(LeonardMod_Tick);
    System_SetIML(mask);
    /* foreground: Screen_Update(), input, drawing; no second Tick */
}
{
    U16 mask = System_GetIML();
    System_SetIML(7);
    if (attached) Vbl_RemoveCall(LeonardMod_Tick);
    System_SetIML(mask);
}
LeonardMod_ShutdownPaula();
LeonardMod_UnLoad(mod);
```

Use supervisor mode after `Platform_Init()`. The example now calls Tick from
one PAL VBL callback, independently of rendering speed, following UGH's VBL
service. The earlier prohibition on VBL use was incorrect: the existing bridge
preserves the interrupted caller's **USP, SR and registers** while the native
core temporarily enters user mode, uses its own stack and returns through trap
#0. This is the same essential USP protection used around Paula in `ugh.agt`.

`Start` still does **not** install a callback automatically. Check Vbl_AddCall's
return value and shut down on registration failure. Use exactly one scheduling
owner: a VBL callback **or** explicit foreground Tick calls, never both. The core
is non-reentrant. Protect callback-table changes against interrupts, and detach
the callback **before Start/restart, Stop, ShutdownPaula or module unload**.
The sample's wrapper only calls Tick and increments a diagnostic counter; all
scope sampling and rendering remain in the foreground. DMA and trap #0 have one
exclusive owner; stop other players first.

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

## VBL regression evidence

Run `make -C godlib.spl/leonard check` for both rates and five visualizations.
`godlib.spl/leonard/analysis/vbl-check.c` performs 18 native checks: with no
foreground drawing/Tick calls, 64 audio callbacks follow 64 PAL VBLs at each
rate while USP holds a sentinel. It checks USP and interrupt-mask preservation,
nonzero/changing mixed PCM, then detaches the callback and verifies stopped
DMA, restored trap #0, freed workspace/free RAM and callback counts.

The release tests cycle bars, four scopes, ICE XY, stereo and mono, toggle
visibility/raster indication, check non-silent stereo WAV captures and return
to TOS without a remaining audio callback. Configuration: STE / 68000 / 8 MHz /
1 MiB / German TOS 1.62 / PAL. See `verification-vbl.json` for exact results and
binary hashes. This is functional emulation, not a CPU profile or a substitute
for listening/hardware tests.
