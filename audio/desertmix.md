# Desert gameplay mixer for GodLib

Four-layer signed 8-bit mono PCM mixer recovered from desert/game.ovl. This is
the gameplay sound path; the Lance/Paula MOD replay from title.ovl is separate.
Include `godlib/audio/desertmix.h`. The optional module is linked from libgod.a
only when referenced. Call Platform_Init in supervisor mode before Init and
DeInit before freeing samples or Platform_DeInit. STE/MegaSTE only; DMA ring
placement outside the 24-bit address range is rejected. Use one DMA owner.

## API and source ownership

```c
if (DesertMixer_Init()) {
    DesertMixer_Play(0, pcm, length, 1); /* base loop */
    DesertMixer_Play(1, pcm, length, 1); /* additional loop */
    DesertMixer_Play(2, pcm, length, 0); /* one-shot */
    DesertMixer_SetRasterDebug(1);
    /* The GodLib VBL callback performs refills. */
    DesertMixer_StopAll();
    DesertMixer_DeInit();
}
```

PCM must already be signed mono at 12517 Hz. Buffers must be even-aligned and
lengths must be nonzero multiples of four, at most $7FFFFFFF bytes, without
pointer overflow. Pad with signed silence (zero) as needed. Channels 0/1 only
accept loops; channels 2/3 only accept one-shots. This deliberate restriction
preserves the original roles rather than silently changing the algorithm.
All four may overlap. IsPlaying reports source activity, not queued sound.

The caller owns PCM and must keep it valid until the channel is stopped or
finishes. Stop removes source references atomically; queued audio remains in
the DMA ring until played. DeInit removes the callback and stops DMA before
returning. Init is idempotent. Invalid Play arguments return zero.

## Original core and scheduler

Core TEXT $1F78..$2245 (718 bytes) was transcribed to vasm Motorola syntax.
A build with VERIFY_ORIGINAL and the original state addresses matches the
original executable bytes exactly; proof is in the example's analysis folder.
The fastcall entry puts D0/D1 ring-offset/count arguments on the stack for the
original GCC calling convention. Original registers and state offsets remain.
Input SHA256: 8a85c36b8590559b20f3d8ab94eb3f2fe14140ffcfc204cc1f6bdf8595f4712f.

The base is copied with MOVEM.L/MOVE.L or zero-filled when stopped. Each active
extra layer adds bytes into the result with eight unrolled MOVE.B/ADD.B pairs.
This is independent byte addition, without clipping, volume control, pitch or
runtime resampling. Overflow wraps; prepare PCM with headroom for overlapping
sources. Inactive extra layers skip their pass. No packed-longword carries
between audio samples occur.

The original 2328-byte state slab includes the 2048-byte looping DMA ring.
The GodLib VBL wrapper reproduces gameplay's playhead-based scheduling: refill
to 640 bytes ahead, aligned to four bytes, splitting at the ring end. Initial
queued silence and source-change latency are about 51 ms. Refills greater than
1984 bytes are deferred as in the original. Long stalls that let DMA lap the
writer cannot be detected from its position alone. Use PAL 50 Hz, leave enough
CPU time for refills, and avoid lengthy interrupt stalls. No timer is claimed.

## Raster CPU meter

DesertMixer_SetRasterDebug, SgdlMixer_SetRasterDebug and
SlugMixer_SetRasterDebug use LanceMod's palette-zero technique: save the old
colour, write $0700 during the audio callback, restore afterwards. F9 toggles
it in each mixer example. The red stripe shows elapsed callback time; it is
not a numeric percentage or a CPU-cycle counter. It includes DMA scheduling
and the mixing core. Normal interrupt preemption can also extend the stripe.
Debug disabled still incurs a flag check, without palette writes. Very short
callbacks can finish inside the nonvisible vertical blanking interval, making
their stripe invisible at the normal screen crop.

SGDL at 12517 Hz fills 512 samples about every 40.9 ms, so its heavy stripe
appears only on refill frames. SlugMix fills 250 samples every VBL. Desert
usually fills around 250 aligned bytes per VBL, depending on DMA position.
Compare multiple frames with matching sample rate and number of active layers,
not a single stripe. Desert skips inactive extra layers; SGDL/SlugMix retain
their fixed source-addition loops. The fourth channel has no SlugMix equivalent.

## Verification

The example runs actual 68000 core tests before starting DMA: all 16 active
layer masks, modulo-byte sums, destination guards, multiple wraps of short
loops, and one-shot endings inside a refill. GodLib and all three examples
build using the existing GCC ELF/vasm toolchain. Hatari 2.6.1 / STE 8 MHz /
4 MB / TOS 2.06 / RGB 50 Hz checks passed:
all four layers, both loops past sample duration, one-shot completion, stop,
DMA mode $81, live ring PCM comparison, raster toggling, palette restoration
and ESC to TOS without CPU exceptions. Runtime evidence is in the example's
analysis folder. Audio fidelity is not established by PCM checks alone; listening and real STE hardware testing are still needed.
