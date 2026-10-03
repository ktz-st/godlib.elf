# SGDL mixer for GodLib

Four-channel signed 8-bit mono PCM mixer by OrionSoft, recovered from the SGDL
object and transcribed to vasm Motorola syntax. Include `godlib/audio/sgdlmix.h`.
The module is linked from libgod.a only when referenced. It is independent of
AudioMixer, SlugMixer and MOD replay; use only one DMA owner at a time.

## Use

Call Platform_Init in supervisor mode before SgdlMixer_Init. Check its return
value. STE and MegaSTE are supported; other machine types are rejected.
Platform's audio lifecycle saves/restores the hardware; remove the mixer before
Platform_DeInit and before freeing samples.

```c
if (SgdlMixer_Init(eSGDLMIXER_FREQ_25K)) {
    SgdlMixer_Play(0, pcm25, paddedLength, 0);
    SgdlMixer_Play(1, pcm25, paddedLength, 1);
    /* VBL callback performs mixing while the main loop runs. */
    SgdlMixer_Stop(1);
    SgdlMixer_DeInit();
}
```

Channels are 0..3. Sample data is caller-owned and must already be signed mono
at the selected rate, 25033 or 12517 Hz. Length must be a nonzero multiple of
512 bytes. Pad the last block with signed silence (zero). Looping repeats the
entire padded sample. Odd byte alignment is allowed. The wrapper rejects invalid
channel numbers, null pointers, non-block lengths and wrapping end addresses.
It does not load WAV files or resample at runtime.

SgdlMixer_SetFrequency changes the DMA rate, stops every channel and clears
queued audio. Use samples prepared for the new rate after switching. Changing
only the DMA mode while retaining the same PCM changes speed/pitch. The example
ships two versions of the same voice to demonstrate a rate/quality switch.
A request for the already-active rate is a no-op. GetFrequency returns zero
when uninitialized. Init on an active instance delegates to SetFrequency.

## Original behavior and scheduling

The original kernel performs MOVE.B followed by three ADD.B operations. No
saturation, gain, panning or interpolation exists. Overflow wraps modulo 256;
prepare samples with headroom (the example uses quarter amplitude). The same
kernel runs at both rates.

The 1536-byte ST-RAM slab contains a 1024-byte circular DMA buffer followed by
512 bytes of silence. Four 12-byte channel records contain current/end/loop
pointers. The callback reads the DMA playhead and fills the inactive half only
when it has changed. At 25 kHz a half lasts about 20.45 ms; at 12.5 kHz it lasts
about 40.90 ms, so there are about half as many mix operations. A normal PAL
50 Hz VBL can service both rates. This is the original half-buffer scheduler:
stalls long enough to miss a half transition are not recovered. Other VBL
callbacks must leave enough CPU time for audio. No Timer A vector is installed.

Stop and IsPlaying describe source activity. Already-generated samples can
remain audible until consumed from the DMA ring. Stop safely removes sample
references; SetFrequency/DeInit also stops/clears queued audio. Channel changes
mask interrupts and shared channel pointers are volatile.

## Disassembly provenance and adaptation

Input: workspace `sgdl/sgdl/lib/snd_asm.o`, a.out-zero-big object.
SHA256: `4f2cda19dc0525a7090d3f9d3af4c388bf375049d196895dd72b14b1b61beb3a`.
Disassembly with relocation records and rg-dis inspection are retained in
`godlib.spl/sgdlmix/analysis`. Original TEXT offsets are section-relative,
not file offsets or runtime addresses:

- $0000..$0037: DMA initialization (mode $82).
- $0038..$003d: DMA stop.
- $00c8..$0119: STE playhead-based VBL update.
- $015e..$0299: channel selection, 512-byte mixing and end/loop processing.
- $01a6..$0249: 16 unrolled samples repeated 32 times.

The 316-byte $015e..$0299 core matches the ported object's final 316 TEXT bytes
exactly, including external relocation placeholders. It references the new
GodLib buffer-pointer and channel symbols. Init replaces the fixed DMA mode
with the selected mode byte. Update uses a word BSR because the omitted Falcon
interrupt routine and added callee-save test entry change code distances.
The WAV parser, keyclick routines and Falcon/Timer-A path are not imported;
GodLib already handles the platform lifecycle. Assembly comments preserve
original offsets and the existing SGDL objects are not modified.

## Verification

The startup test in `godlib.spl/sgdlmix/verify.c` executes the actual vasm
68000 core before DMA starts. It checks all 16 active-channel masks, full byte
values and wrapping sums, output guards, one-shot endings, two-block progress,
and looping on all four channels.

Builds: GodLib and SGDLMIX.TOS pass with the existing GCC ELF/vasm toolchain.
Hatari 2.6.1, STE 8 MHz, 4 MB, TOS 2.06, RGB 50 Hz, sound enabled: self-test
PASS; four-channel loops remain active past sample duration at both rates;
DMA mode reads $82/$81 and both halves of the live DMA ring match the
four-channel reference PCM at each rate;
rate switch clears all channels; stop and one-shot completion pass; ESC returns
to TOS without bus/address/illegal exceptions. Logs/screenshots are in the
example's analysis directory. This verifies state and PCM math; audio fidelity
has not been assessed by listening or recording. Real hardware remains untested.

## Raster CPU display

SgdlMixer_SetRasterDebug enables LanceMod-style red palette-zero stripes during
the audio callback. The old colour is restored afterwards; DeInit disables
the meter. F9 toggles it in the example. It measures visible callback duration,
not a numeric CPU percentage. See desertmix.md for comparison guidance.
