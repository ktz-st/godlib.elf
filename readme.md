# GodLib

Atari ST/STE development library, with a local GCC ELF / libcmini / vasm port.
It provides screen management, planar graphics, sprites, fonts, input, audio,
asset loading and decompression. Original authorship is listed in
[readme.txt](readme.txt).

## Build and examples

The local build expects sibling `godlib`, `godlib.spl` and `libcmini`
directories. Tools: `m68k-atari-mintelf-gcc`, matching binutils and
`vasmm68k_mot`. Build the library before linking clients:

```sh
make -j4
make -C ../godlib.spl/pcmvoice
```

The example output is `../godlib.spl/pcmvoice/PCMVOICE.TOS`. See the
[example catalogue](../godlib.spl/README.md) for all 29 standalone samples,
assets, toolchain setup and controls. GodLib and applications must use matching
68000, `-mshort`, `-mfastcall` and structure-layout conventions; copy the
existing example Makefiles when starting a project.

## Documentation

- [Module notes](docs/module-notes.md): APIs, lifecycle, hardware assumptions
  and implementation limitations across the library.
- [PCM mixer guide](audio/pcm-mixers.md): canonical names, input constraints,
  channel roles, migration and PacMad/Rogul integration.
- [PCM Block](audio/pcmblock.md), [PCM Frame](audio/pcmframe.md),
  [PCM Layer](audio/pcmlayer.md), [PCM Voice](audio/pcmvoice.md): detailed
  backend behavior, arithmetic, ownership and verification.
- [PCM profiling](audio/tests/mixer-profile/README.md): measured callback/core
  CPU costs and the measurement method.
- [Line drawing tests](graphic/tests/line/README.md),
  [blitter tests](blitter/tests/README.md) and
  [8x8 font tests](font8x8/tests/README.md): target regression checks.
- [64-bit host notes](HOST_64_NOTES.md): host-side portability notes, separate
  from the Atari target library.

## Screen and graphics

Four-plane `Screen_Init()` uses a 320x200 viewport; its dimensions describe
virtual storage. STE horizontal scrolling uses `eSCREEN_SCROLL_H`; vertical
scrolling uses `eSCREEN_SCROLL_V`. Examples include 640x200 horizontal and
320x400 vertical backgrounds. Use canvas line offsets instead of assuming a
160-byte stride. Sprite, blitter and font canvas APIs support wider storage;
see module notes for each API's bounds and alignment requirements.

## PCM mixers

All four optional PCM backends require STE/MegaSTE DMA sound. They accept
signed 8-bit mono PCM and exclusively own DMA while active.

| API prefix | Channels | Sampling rate | Main distinction |
| --- | --- | --- | --- |
| `PcmBlockMixer` | 4 | 12517 / 25033 Hz | 512-byte sample blocks |
| `PcmFrameMixer` | 3 | 12517 Hz | Fixed 250-byte chunks per nominal 50 Hz VBL |
| `PcmLayerMixer` | 4 | 12517 Hz | Two looping layers and two one-shots |
| `PcmVoiceMixer` | 3 | 12517 Hz | Independent voices, queues, optional streaming and saturation |

PacMad uses resident samples with `PcmVoiceMixer_Play()` / `Queue()`. Rogul's
STE DMA profile uses its optional stream callback for music and the remaining
two voices for effects, with saturation enabled. Streaming is not required for
ordinary sample playback. The VBL frequency of 50 Hz is not a 50-kHz audio rate.

## Lifecycle

Applications normally enter through `GodLib_Game_Main()` and initialize
Platform in supervisor mode. Stop audio callbacks before freeing their PCM;
remove hardware/VBL users and restore video before freeing screen storage.
Use the corresponding Platform teardown before returning to TOS. Each sample
shows its required ownership and cleanup sequence.
