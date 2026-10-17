# Wizzcat MOD replay

Wizzcat/Delta Force, optimized by Metal Ages, uses a 25 kHz source mixer and
**50 kHz stereo DMA**, interleaving CH1/CH4 on the left and CH2/CH3 on the right.
This is the Metal Ages core in GodLib, not the separate 12 kHz binary adapter
in `stlibs4gcc/wizzcat`. No new output-rate mode is introduced here.

Use supervisor mode after `Platform_Init()` on STE/MegaSTE, PAL 50 Hz:

```c
sWizzcatModule *mod = Wizzcat_Load("QUICKIE.MOD");
if (mod && Wizzcat_Start(mod)) {
    /* Wizzcat owns a GodLib VBL callback; run the main loop normally. */
}
Wizzcat_StopVbl();
Wizzcat_UnLoad(mod);
```

Only one DMA player can run at a time. `Wizzcat_Start` returns failure if loading,
preparation or VBL registration cannot complete. Stop/StopVbl remove the callback
and stop DMA. UnLoad also stops the current instance before freeing it. Startup
uses `Wizzcat_Init` + `Wizzcat_Play` internally; those low-level entry points remain
available for an application with its own scheduling. Start after Init is not
another initialization step: choose one startup route.

Sample preparation mutates lengths and sample data. Each loaded module can be
prepared once; another Init/Start requires loading fresh raw data. `Play` can
re-prime the current initialized instance, but does not rewind tracker position.
The module descriptor now has an internal `mPrepared` field; rebuild clients
against the current header and use Load/UnLoad instead of serializing the struct.

## Memory

The former default always appended **131072 bytes**. The new loader computes the
persistent requirement from all sample lengths and loop sizes, reads only actual
patterns/sample data and ignores trailing file padding. `Wizzcat_LoadEx(name,0)`
means automatic sizing. An explicit margin remains supported, but an undersized
one is rejected before assembly preparation can write into it.

The core retains its original expanded-loop format. For each nonempty sample:

- `prefix = 2 * (repeatStartWords ? repeatStartWords : lengthWords)`;
- `repeat = 2 * max(repeatLengthWords,1)`;
- `expanded = prefix + ceil(1600/repeat)*repeat + 1600`.

The final 1600 bytes are the mixer lookahead copy. If empty slots exist, they
share a separate 3200-byte silent loop/lookahead. This prevents empty instrument
triggers from reading unrelated sample data or walking past the module.
Four extra bytes protect the original first-word clearing at raw sample ends.

Persistent bytes are the 10-byte descriptor plus
`1084 + 1024*patternCount + max(rawSampleBytes, expandedSampleBytes+silenceBytes) + 4`.
A separate raw-sample-sized scratch copy (minimum 2 bytes) exists **only during
preparation**, then is freed. It replaces the original scratch area at the end of
the same persistent allocation, so expansion cannot overwrite unread input.
Initialization peak usage includes both the persistent block and this scratch;
reducing resident memory does not imply a lower peak for every large MOD.
Static replay tables/DMA pages/code are unchanged in purpose and still linked
with the replay object; the figures below cover module heap allocation only.

Measured target allocations (allocator overhead excluded):

| MOD | Previous heap | Current heap | Saved |
|---|---:|---:|---:|
| LOSTPARO | 186041 B | 74480 B | 111561 B |
| BOOTUP | 214383 B | 128068 B | 86315 B |
| CANNONFO | 380261 B | 293994 B | 86267 B |
| QUICKIE | 154409 B | 42478 B | 111931 B |
| 4matwave | 163373 B | 130980 B | 32393 B |

`Wizzcat_GetModuleInfo(header,fileBytes,&info)` reports module, sample, prepared
sample, workspace, transient scratch and total load sizes from a raw 1084-byte
header. Validation includes M.K. signature, song/order range, volumes, finetunes,
loop bounds and native offset limits. The engine uses 16-bit byte positions:
`prefix+expandedRepeat <= 65535`, original loop bytes <=32766. Unsupported sample
sizes fail cleanly rather than wrapping native counters. Finetune header values
are validated but the older replay's effect/finetune behavior is preserved;
this work does not turn it into the Lance/Leonard ProTracker engine.

## Initialization cost

The increment builder formerly simulated 500 source steps plus three 125-step
subblocks for each period. Exact integer products now calculate the same carries,
remainders and subblock offsets. Tables are built once per process and reused.
The generated tables match **all 29728 bytes** of the original increment/volume
regions. The unrolled runtime mixer and interleaved PCM format are retained.

STE / 8 MHz / German TOS 1.62, measured using HZ200 around Init:

| MOD in test sequence | Previous Init | Current Init |
|---|---:|---:|
| LOSTPARO (first, table generation included) | 2710 ms | 760 ms |
| BOOTUP | 2900 ms | 380 ms |
| CANNONFO | 3495 ms | 970 ms |
| QUICKIE | 2645 ms | 120 ms |
| 4matwave | 2725 ms | 200 ms |

These are initialization elapsed times, including preparation and allocator guard
instrumentation, with 5 ms resolution. Later entries reuse the new tables.
They are not runtime mixer CPU percentages; the historical source's “45%” comment
is not a new measurement.

## Visualization and example

`Wizzcat_GetVisualizer(U16 ages[4])` provides a coherent, read-only snapshot of
four channel retrigger ages, in CH1..CH4 order. They increment once per tracker
call and reset when the native DMA retrigger occurs. They saturate at 0x4000 and
return idle when stopped. NULL is a no-op. They measure **activity, not amplitude**.

`godlib.spl/wizzcat` uses the same four STF_DEMO bars and ASM dirty-row renderer
as Lance/Leonard: CH1/CH4 left, CH2/CH3 right. V toggles bars, F1 colours palette
zero during native replay and restores it afterwards. 1/2 jump to the first/last
song position; Space/Esc exits. `Wizzcat_Jump` rejects out-of-range positions.

The optional `GetOscilloscope` getter supplies four pitched, volume-scaled source
previews without changing replay state. O switches bars/scopes in the example;
V hides/shows the selected mode. See [the shared scope API](modscope.md).
