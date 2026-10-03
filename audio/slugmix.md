# SlugMixer

Optional three-channel SFX mixer extracted from the Metal Slug STE binary.
Independent of AudioMixer and AGTools. Include `godlib/audio/slugmix.h`;
the module is part of `libgod.a` and is only linked when referenced.
Do not enable AudioMixer, MOD replay or another DMA owner simultaneously.

Call after `Platform_Init()` in supervisor mode. `SlugMixer_Init()` validates
DMA-capable machine type and ST-RAM placement of its 2162-byte state slab,
then installs a VBL callback and starts DMA at 12517 Hz, signed 8-bit mono.
It returns 0 when unsupported or the VBL queue is full. Call
`SlugMixer_DeInit()` before freeing samples and before `Platform_DeInit()`;
the latter restores the audio hardware state saved by Platform_Init.
Init is idempotent, DeInit removes its callback before stopping DMA.

```
SlugMixer_Init();
SlugMixer_Play(0, sample, blocks, 0);
SlugMixer_Play(1, sample2, blocks2, 1);
/* VBL queue performs mixing, including while the main loop waits. */
SlugMixer_Stop(1);
SlugMixer_DeInit();
```

Check the return values of Init and Play in applications. Channels are 0..2.
`SlugMixer_IsPlaying()` reports source activity, not whether the last queued
DMA page has finished sounding. Stop/replacement can take a few blocks to
become audible. Channel updates mask interrupts to avoid torn pointers.

Inputs must be even-aligned signed PCM already resampled to 12517 Hz and
attenuated for mixing. Length is 1..255 blocks of 250 bytes; pad with zeroes
and keep the sample allocated until stopped. The hot loop sums packed
longwords, so byte carries are preserved exactly as in the original. It
has no clipping, independent byte saturation, gain, pitch control or stereo.
Use roughly quarter-volume samples when all three voices can overlap.
Looping channel 0 is rejected: the original core clears its loop pointer.
Channels 1/2 can loop. The game's music sequencer mode is left disabled.

The original 926-byte core is preserved, with ELF symbol adaptation and a
C callee-save wrapper. `SlugMixer_Lock/Unlock` use the GodLib fastcall ABI
(D0 for SR). Source executable SHA256:
`43ad8bc98675884b5b7ff8001dd633d33383ebdb770592ee518a5205e6bc2e39`.
Code range: TEXT-relative `$20DB8..$21155`; file offset adds `$1C`.
Comments retain original offsets. State references preserve offsets within
the original `$C36B0..$C3F21` BSS slab; no game addresses remain as runtime
dependencies. Provenance and detailed original analysis are in the workspace
`metal_slug/sfx_mixer/README.md`; this is not a build dependency.

Scheduling retains the original one 250-byte block per 50 Hz VBL design.
12517/250 is actually 50.068 Hz; no drift/missed-frame compensation exists.
Use a PAL 50 Hz display. Long interrupt stalls, 60 Hz/VGA displays, or
another DMA owner are not supported by this scheduler. Machine detection
alone does not validate the display rate. Real hardware and Falcon need
separate testing.

Example: `godlib.spl/slugmix`, with F1/F2/F3 channel triggering, F4 overlap,
F5 channel 1 looping, F6 stop and ESC exit. The sample is generated from the
existing mixer example, with gain 1/4 and zero padding to 94 blocks.

Validation: GodLib and example build successfully. The adapted assembly
core matches the original 926 bytes when linked at its original state
addresses. Hatari 2.6.1, STE 4 MB, TOS 2.06 and RGB 50 Hz: all three channel
activity indicators, looping after one-shot duration, stop and return to
TOS were checked with sound enabled and no CPU exceptions. Audio fidelity
has not been verified by listening or recording; hardware testing remains.

## Raster CPU display

SlugMixer_SetRasterDebug enables LanceMod-style red palette-zero stripes during
the audio callback. The old colour is restored afterwards; DeInit disables
the meter. F9 toggles it in the example. It measures visible callback duration,
not a numeric CPU percentage. See desertmix.md for comparison guidance.
