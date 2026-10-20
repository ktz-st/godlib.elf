# Richard Joseph RJP1 player

`music/rjp1.h` plays the native `RJP1SMOD` song container and its matching
`RJP1` sample bank (`.SNG` + `.INS`) on a PAL Atari STE/MegaSTE. It uses
Andi Smithers' VectorDean sequencer in 68000 ASM with a selectable Lance or Leonard Paula
mixer, shared with the corresponding MOD player. The unused ScummVM-derived C player from `cfintro.rjp`
is not included.

```c
#include <godlib/music/rjp1.h>

sRjp1Song *song = Rjp1_LoadEx("JON.SNG", "JON.INS", eRJP1_MIXER_LEONARD);
if (song && Rjp1_Start(song, 1, eRJP1_FREQ_25K)) {
    /* Foreground rendering/input; audio has its own PAL VBL callback. */
}
Rjp1_UnLoad(song);  /* removes callback, stops DMA, restores trap #0, frees RAM */
```

Initialize GodLib's platform/file/VBL services and enter supervisor mode first.
`Start` owns the callback: do not also call `Tick` in the foreground. All
playback calls are supervisor APIs. One global player instance is supported.
Stop other DMA replay engines before starting. A common ownership lease prevents
LanceMod, LeonardMod and either RJP1 backend from acquiring DMA simultaneously. Use
`LanceMod_ShutdownPaula()` before switching from LanceMod to RJP1, and
`LeonardMod_ShutdownPaula()` when leaving LeonardMod, and `Rjp1_Stop()` when
leaving RJP1. Neither needs to
link the other sequencer. The public LanceMod API and its workspace sizes stay
unchanged.

`Rjp1_SetSubSong(index)` resets the voices without allocating or relocating the
song again. Indexes are zero based and must be below `info.mSubSongs`; slot 0
is not universally silent. The example starts at slot 1, matching `cfintro.rjp`.
`Rjp1_SetVolume(0..64)` changes all four channel scales (reset to 64 on a restart
or subsong change). `Rjp1_GetVolumes()` provides four coherent sequencer volumes,
not audio amplitude/peak measurements. `Rjp1_SetRasterDebug(1)` brackets both
mixing and sequencing with palette colour 0, restored on return.

## Preparation and memory

`Rjp1_Create(sng, sngBytes, ins, insBytes)` copies its sources; the caller can
release both input blobs immediately. `Rjp1_Load()` does that automatically.
`GetInfo` reports the exact requested resident song allocation separately from
the shared frequency-dependent mixer workspace. Loading briefly also retains
the original files. Playback itself allocates nothing.

For **Lance**, only audible PCM is converted to signed 7-bit samples for Lance's two-voice
sums. Pitch/volume modulation arrays retain their original signed 8-bit values.
Identical attack/loop descriptions share prepared PCM. Suffix loops reuse the
attack data; other loops are appended separately. Every unique prepared sample
has a 664-byte guard: silence for one shots, repeated sustain for loops. The
native trigger resets position/phase for every note, including repeated notes
using exactly the same sample. There is no address-change guessing or delayed
polling of emulated Amiga registers.

The loader validates magic, section lengths/alignment, static table references,
envelopes and waveform ranges. It is for trusted music assets, not a sandbox
for arbitrary sequence bytecode. The Lance preparation rejects sustain lengths above 65534 bytes
because that mixer uses a 16-bit remainder division. Original files are never
relocated or scaled in place.

Lance's existing pitch table implements periods 108..907 (values below 108 are
clamped by its table). RJP modulation can exceed even the 1024-entry lookup
array: the direct player caps its output at 907 before it reaches the mixer.
Very low notes therefore share Lance's existing pitch limit; this prevents the
out-of-range lookup/crash rather than extending the mixer pitch range.

## Mixer selection and VBL scheduling

`Rjp1_CreateEx(..., mixer)` and `Rjp1_LoadEx(..., mixer)` select
`eRJP1_MIXER_LANCE` or `eRJP1_MIXER_LEONARD` **before preparation**.
Legacy `Create`/`Load` retain Lance. `Start` uses `info.mMixer`; changing the
backend requires unloading and preparing again. `GetWorkspaceSizeEx(rate,mixer)`
reports its workspace; legacy `GetWorkspaceSize` reports Lance.
Lance supports 12.5/25/50 kHz; Leonard supports 12.5/25 kHz and rejects 50 kHz.

Leonard keeps a private relocated SNG and the original full signed 8-bit INS
bank. It needs no 7-bit PCM copies or per-sample guards. Its adapter supplies
explicit current/end/reload pointers and resets phase on every note. One shots
reload shared silence; sustains use their real loop. Native source steps retain
the integer part at both rates, including steps greater than one. Leonard has
its own bounded period lookup; Lance's 907 cap is applied only to Lance.
For Leonard, `mSampleBytes` counts the INS payload including embedded modulation,
`mModulationBytes` is zero (already included), and `mUniqueSamples` is zero
(no deduplicated prepared-sample pool). `mResidentBytes` is the authoritative
song allocation for either representation.

| Workspace | 12.5 kHz | 25 kHz |
|---|---:|---:|
| Lance | 142146 B | 176320 B |
| Leonard | 61134 B | 91134 B |

The sequencer advances once per PAL VBL for **both** backends. Lance mixes then
advances the sequencer; Leonard advances it, synchronizes the voice controls,
and services its DMA blocks. The sequencer is outside Leonard's conditional
block-generation routine, so no musical ticks are skipped when a block is not
needed. Foreground rendering never drives playback. Shared backend wrappers
preserve SR/USP; switching removes the old callback before releasing its memory.
Neither RJP1 backend links the ProTracker sequencer.

## Validation and example

See `godlib.spl/rjp1/README.md` and its `analysis` reports for memory accounting,
cycle measurements, original-sequencer comparison, note-trigger fixtures,
VBL/USP tests, stereo recordings and shared-mixer lifecycle regression and the eight-case backend comparison.
The example includes JON (unpacked from the user's existing RNC assets)
and WARX4 from `cfintro.rjp/music`.
Those existing game music assets are demonstration inputs, not new original
music or assets with a newly granted redistribution licence.
