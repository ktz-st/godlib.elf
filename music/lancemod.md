# LanceMod memory ownership

`LanceMod_Load()` allocates the decoded module layout plus guards for nonempty
samples. `LanceMod_InitPaula()` allocates the replay workspace for the selected
rate. `LanceMod_ShutdownPaula()` stops the callback/DMA, restores trap #0 and
releases the workspace. `LanceMod_UnLoad()` releases the independently owned MOD.
Always stop playback before unloading the MOD.

## Module allocation

The loader reads the 1084-byte header before allocating the module. For supported
four-channel, 31-sample MODs:

```
module bytes = 1084 + (highest order-table pattern + 1) * 1024 + sample bytes
load bytes   = module bytes + nonempty sample count * 664 + 2
```

The highest pattern is taken from all 128 order slots, matching the existing
replay's sample offset calculation. Unused trailing file bytes are not retained.
Signatures `M.K.`, `M!K!`, `4CHN` and `FLT4` are accepted. Truncated headers/data,
unsupported channel layouts and invalid sample/loop bounds are rejected before
sample preparation. Sample lengths and loop offsets are big-endian word counts.

Empty sample slots now reference the shared silent sample; they no longer receive
an individual 664-byte guard. Nonempty samples retain the original guard and
loop-shadow behavior, including EFx invert-loop updates. The guard is independent
of output frequency: it covers source-sample consumption over a replay frame.
It must not be halved merely because output is changed to 12.5 kHz.

Song duration does not directly determine RAM usage. Pattern storage and sample
bank size do. A long sequence can reuse the same patterns many times.

`LanceMod_GetModuleInfo(header, fileBytes, &info)` provides the byte counts and
nonempty-sample/pattern counts without changing the header. At least 1084 header
bytes must be accessible; `fileBytes` describes the complete unprepared file.
It returns zero for invalid input and leaves the output unchanged. The legacy
`dLANCEMOD_EXTRA_BUFFER_SIZE` remains the worst-case 31-slot guard bound.

| Module | File bytes | Nonempty samples | Old load allocation | New load allocation | Saved bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| LOSTPARO.MOD | 54960 | 6 | 75546 | 58946 | 16600 |
| BOOTUP.MOD | 83302 | 13 | 103888 | 91936 | 11952 |
| CANNONFO.MOD | 249180 | 13 | 269766 | 257814 | 11952 |
| QUICKIE.MOD | 23328 | 5 | 43914 | 26650 | 17264 |
| 4matwave.mod | 32292 | 31 | 52878 | 52878 | 0 |

## Frequency-dependent replay workspace

The shared workspace is allocated in ST RAM because the DMA pages must be
accessible to the sound hardware, even on systems with alternate/TT RAM.

Large arrays formerly occupied 176320 bytes of static BSS, in addition to two
heap DMA pages. They are now held in one frequency-dependent heap allocation:

| Rate | Old arrays + DMA bytes | New resident heap bytes | Temporary init scratch |
| --- | ---: | ---: | ---: |
| 12.5 kHz | 177320 | 142146 | 500 |
| 25 kHz | 178320 | 176320 | 1000 |
| 50 kHz | 180320, insufficient | 218440 | 2000 |

These values describe tables, generated code and both DMA pages. Small channel
structures, executable code, module allocation and allocator overhead are separate.
The new ASM object's BSS is 708 bytes instead of 177060; the removed BSS includes
32 unused status bytes. On top of the module savings above, 12.5 kHz saves about
34 KiB in workspace, and 25 kHz saves 2000 bytes.

The original generator emits **77638 / 110994 / 151114 bytes of mixer code** at
12.5 / 25 / 50 kHz. Its old reservation was only 110994 bytes, so 50 kHz overran
that region by 40120 bytes. The larger 50 kHz allocation fixes this error; it
should not be compared with the old, undersized reservation as a memory regression.
A corrected static 50 kHz reservation plus DMA would need 220440 bytes.

At 12.5 kHz another 182 bytes are reserved because the initialization-only LEA
correction copies a 91-word lookahead window. Other tables need all runtime
period/volume combinations and keep their full sizes. The 500/1000/2000-byte
frequency list is used only to generate the mixer code and is freed after init.
The loader additionally uses a temporary 1084-byte stack header.

`LanceMod_GetPaulaWorkspaceSize(rate)` reports resident heap bytes, including DMA;
`LanceMod_GetPaulaAllocatedSize()` reports the currently allocated amount, or zero
after shutdown. Invalid frequency selectors retain the existing 25 kHz default.
Initialization stops existing replay before replacing/generating buffers. Same-rate
initialization reuses its allocation; changing rates releases the old allocation
first. Allocation failure returns without entering ASM initialization; `Start`
returns zero. Repeated `StopVbl`/`ShutdownPaula` and failed-start cleanup are safe.

Sample preparation modifies the loaded MOD, as before. Reload the original MOD
before preparing it again; these changes do not make `LanceMod_Init` idempotent.

The optional `GetOscilloscope` getter supplies four pitched, volume-scaled source
previews without changing replay state. O switches bars/scopes in the example;
V hides/shows the selected mode. See [the shared scope API](modscope.md).

## Shared mixer backend

The workspace allocator and native Paula bridge now live in
`paulamixer.c` / `paulamixer_s.s`, shared with the optional [RJP1](rjp1.md)
sequencer. LanceMod entry points, buffer sizes and MOD preparation remain
unchanged. Stop/shutdown one owner before acquiring the other; attempts to
start LanceMod while RJP1 owns Paula fail without stopping RJP1.
