# PCM mixers and migration

All four optional backends own STE DMA sound exclusively. Initialize Platform
in supervisor mode first; stop/deinitialize the backend before releasing samples
or tearing down Platform. They consume caller-owned signed 8-bit mono PCM. Leave the automatic legacy
`dGODLIB_AUDIO_MIXER` setup disabled when using one of these backends; direct
DMA sample playback or another DMA replay engine must not take over its buffer.

| Header / prefix | Channels | PCM sampling rate | Scheduling and sources | Example / executable |
| --- | --- | --- | --- | --- |
| `pcmblock.h` / `PcmBlockMixer` | 4 | 12517 or 25033 Hz | 512-byte blocks, loops on every channel | `pcmblock` / `PCMBLOCK.TOS` |
| `pcmframe.h` / `PcmFrameMixer` | 3 | 12517 Hz | Fixed 250-byte VBL chunks, 1..255 chunks per source; loops on channels 1/2 | `pcmframe` / `PCMFRAME.TOS` |
| `pcmlayer.h` / `PcmLayerMixer` | 4 | 12517 Hz | Channels 0/1 loop; channels 2/3 play once | `pcmlayer` / `PCMLAYER.TOS` |
| `pcmvoice.h` / `PcmVoiceMixer` | 3 | 12517 Hz | DMA playhead scheduling; loops, queued successors, optional RAM producer and saturation | `pcmvoice` / `PCMVOICE.TOS` |

The 50 Hz mentioned for PCM Frame is the nominal PAL VBL callback rate, not
an audio sampling rate of 50 kHz. PCM Frame programs the DMA for 12517 Hz.
Its fixed 250-byte-per-VBL scheduler is unchanged by this rename. PCM Block
supports both 12.5 and 25 kHz; none of these backends resamples input PCM.

## Input preparation and arithmetic

| Backend | Address alignment | Source length | Mixing arithmetic |
| --- | --- | --- | --- |
| PCM Block | Byte alignment allowed | Nonzero multiple of 512 bytes | Independent byte sums; overflow wraps |
| PCM Frame | Even address | 1..255 blocks of 250 bytes (not a byte count) | Packed ADD.L carries and wrapping byte tails |
| PCM Layer | Even address | Nonzero multiple of 4 bytes | Independent byte sums; overflow wraps |
| PCM Voice | Even address | Nonzero even byte count, at most `0x7ffffffe` | Packed ADD.L by default; optional signed-byte saturation |

Keep source PCM alive until it finishes or is stopped. Already mixed DMA audio
can remain audible after IsPlaying becomes false or Stop returns. None of these
backends performs file loading, automatic volume reduction or resampling.
Prepare PCM at the selected sampling rate. The Block/Frame/Layer backends and
Voice's default mode need sufficient headroom when sources overlap; low apparent
loudness does not guarantee that their summed peaks fit in signed 8-bit output.
Voice's `SetSaturation(1)` clamps the full sum to -128..127 instead of wrapping;
it costs more CPU when voices overlap and can still clip loud summed signals.

## Current game integrations

| Game | Backend and roles | Saturation |
| --- | --- | --- |
| PacMad | PCM Voice: channel 0 background, channels 1/2 effects; PCM stays in RAM, with game-side priorities and queued death sounds | Default packed mode |
| Rogul STE DMA | PCM Voice: channel 0 music from a main-thread-fed RAM ring; channels 1/2 effects | Enabled |

Both games include `pcmvoice.h` and call `PcmVoiceMixer_*` directly. PacMad does
not attach a stream. Rogul uses `SetStream()`; file reads and seeks stay in the
main thread, never in the VBL producer callback. Other Rogul audio profiles
retain their own YM/direct-DMA backends.

## Migration

| Historical header / prefix | Canonical header / prefix |
| --- | --- |
| `sgdlmix.h` / `SgdlMixer` | `pcmblock.h` / `PcmBlockMixer` |
| `slugmix.h` / `SlugMixer` | `pcmframe.h` / `PcmFrameMixer` |
| `desertmix.h` / `DesertMixer` | `pcmlayer.h` / `PcmLayerMixer` |
| `pacmadmix.h` / `PacmadMixer` | `pcmvoice.h` / `PcmVoiceMixer` |
| Interim `pcmstream.h` / `PcmStreamMixer` | `pcmvoice.h` / `PcmVoiceMixer` |

Update includes and function/type prefixes, and replace `eSGDLMIXER_*` /
`dSGDLMIXER_*` constants with `ePCMBLOCK_*` / `dPCMBLOCK_*`. For example:

```c
#include <godlib/audio/pcmblock.h>
if (PcmBlockMixer_Init(ePCMBLOCK_FREQ_25K)) {
    PcmBlockMixer_Play(0, pcm25, paddedLength, 1);
}
```

The four historical game/library-name headers remain as source-compatibility
wrappers using macro aliases. The interim `pcmstream.h` name has been removed;
change it to `pcmvoice.h`, `PcmStreamMixer_*` to `PcmVoiceMixer_*`, and
`fPcmStreamMixerStream` to `fPcmVoiceMixerStream`. Rebuild client objects when updating libgod.a: precompiled objects
using historical linker symbols are not binary-compatible. New example folders,
source files, ASM symbols and the library Makefile use the canonical names.
AudioMixer (`amixer.h`) remains a separate backend: two voices with real
0..255 volume/pan, three pan laws, saturated signed stereo summation and source
loops. It uses 4 KiB of double-buffered stereo lookup tables, a dedicated single-voice
path and carry-corrected two-voice mixing with a saturation fallback. Output is selectable
12517/25033 Hz stereo using `AudioMixer_SetFrequency(eAUDIO_FREQ_12/25)`.
Mono input at 6258/12517/25033 Hz is expanded by repetition or decimated 2:1
without filtering, preserving source tempo/pitch. Rate changes stop voices and
clear queued output; callers restart samples afterwards. Odd-length source
loops carry the decimation phase across wraps.
The lightweight Frame/Block/Layer/Voice mixers are unchanged. See
[`godlib.spl/mixer`](../../godlib.spl/mixer/README.md) for API and native tests.
Earlier AudioMixer CPU measurements concern the removed OR/interleave core;
they do not describe this implementation.

## Examples and verification

Build GodLib first, then a standalone sample, for example:

```sh
make -C ../godlib.spl/pcmvoice
make -C ../godlib.spl/pcmvoice debug
python3 ../godlib.spl/pcmvoice/analysis/verify-hatari.py
```

The Voice harness uses local hatari-debugging helpers and an isolated GEMDOS
run directory. `HATARI_TOS` can select a specific ROM image. Its 2026-10-16
Hatari STE 8 MHz / 4 MB / TOS 2.06 run passed 491 core/scheduler cases,
18 seconds of live playback without underruns, queues, one-shots, stop,
raster toggling and exit. Block/Frame/Layer target harnesses also passed after
renaming. All 29 examples and both game executables were rebuilt.

CPU tables in the module notes are historical measurements with their original
conditions, not new benchmarks of the renamed code. In particular, Voice's
older packed-mode measurements do not measure its optional stream/saturation
paths. A raster stripe shows elapsed callback time, not a numeric CPU percentage.
These checks do not replace listening or testing on a physical STE.

## Origins and attribution

PCM Block originates in OrionSoft SGDL, PCM Frame in Metal Slug STE, PCM Layer
in Desert gameplay audio, and PCM Voice in PacMad/Puckman-style mixing.
The module notes retain source attribution, recovered behavior and limitations.
This rename does not change their arithmetic, channel roles or DMA algorithms.
