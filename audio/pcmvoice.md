# PcmVoiceMixer

`pcmvoice.c`/`pcmvoice_s.s` extract the mixer from `pacmad.ste`. Initialize
in supervisor mode after Platform_Init; it supports STE/MegaSTE and exclusively
owns DMA sound. DeInit before releasing samples or shutting down Platform.
The module is optional: it is pulled from libgod.a when its API is referenced.

Three independent voices play caller-owned signed 8-bit mono PCM at 12517 Hz.
Streaming is optional; resident samples use Play/Queue without a producer callback.
Every channel can loop or play once. Nonzero even byte lengths and even
addresses are required;
even lengths up to 0x7ffffffe are supported, including sources above 65535 bytes.
There is no resampling, gain, file loading or channel prioritization.
Saturation is optional and disabled by default.

```
PcmVoiceMixer_Play(0, background, backgroundBytes, 1);
PcmVoiceMixer_Play(1, effectA, effectABytes, 0);
PcmVoiceMixer_Queue(1, effectB, effectBBytes);
```

Queue replaces the single queued successor of an active non-looping source.
A successor begins in the same fill operation at the exact source boundary.
It is stored in a separate internal descriptor, so no caller descriptor needs
to survive, but both PCM buffers must remain valid. Play cancels a pending
successor. Stop/IsPlaying/GetSample/GetRemaining describe source activity;
already mixed sound remains in the DMA ring. No DMA restart is needed for Play.

## DMA scheduler

The output ring is 1024 bytes, with a 640-byte target lead (about 51 ms).
VBL reads a coherent DMA counter by checking its high/middle bytes again, then
fills the exact even number of bytes needed to reach that target. Typical fills
are 250 or 252 bytes; ring wrap is split into two calls. No fixed 250-bytes/VBL
assumption remains, and one missed VBL can be caught up. Updates preserve at
least 64 bytes of lead; a refill over 576 bytes causes a controlled DMA
stop/clear/restart and increments GetUnderruns instead of writing live audio.
Sources are preserved across recovery, so they resume after the silent lead.
As with other polled ring schedulers, stalls lasting a full ring revolution
(about 82 ms) cannot be distinguished from a shorter advance by position alone;
avoid long interrupt masking/file IO and do not use this as an arbitrary-stall
streaming engine. Higher refresh rates can be polled; the PCM rate stays fixed.

Play/Queue/Stop use short interrupt-masked critical sections instead of skipping
an entire refill when the game is modifying a channel. State and output buffers
are even-aligned, and the output buffer must reside below the 24-bit DMA limit.
SetRasterDebug temporarily colours palette zero during refill, as in LanceMod.

## Original arithmetic retained

The fast core sums four packed bytes with ADD.L, followed by ADD.B for a tail.
Carries can propagate between adjacent packed samples; this is the original
Puckman/Slug-style arithmetic, not four independent signed additions. Attenuation
prevents gross overload but does not remove these carries. For independent-byte
mixing, enable the saturation mode below, or use PCM Block / PCM Layer
(the latter two still wrap on overflow).
Even lengths prevent odd-address longword reads after a source transition.

`PcmVoiceMixer_SetSaturation(1)` selects independent signed-byte summation with
one clamp to -128..127 after adding all three sources. Opposite-sign sources
can cancel before clamping; there is no intermediate clipping or packed carry.
The assembly keeps the packed path for zero/one active source, where adding
silence is an exact copy, and uses the slower byte path for overlapping sources.
DeInit resets the option. Rogul enables it because its existing music and many
SFX reach full scale; their sums can overflow even after SFX loading attenuation.
Clamping prevents amplitude wraparound but can still clip loud overlapping audio.

## Verification

`godlib.spl/pcmvoice` includes synthetic PCM, an on-target core test, an
independent DMA-consumption scheduler model and an automated Hatari test.
Coverage includes silence, 250/252/500-byte fills, 2-byte loops on all channels,
66000-byte sources, queued transitions, output guards and an explicit legacy
carry vector. The model consumes 40 seconds of 12517-Hz audio at 50-Hz polling
with regularly skipped VBLs; source advancement must equal even-rounded DMA
consumption. Live tests run three loops across the old drift interval, one-shots,
a queue, stop, raster debug and clean exit. The suite totals 491 cases,
including 218 checks for full-sum saturation, cancellation, variable loop
boundaries and bit-exact single-source output. The renamed example passed on
2026-10-16 under Hatari STE 8 MHz / 4 MB / TOS 2.06. These are emulator tests,
not listening or physical hardware measurements.

Measured on 2026-10-08 with Hatari-hrdb STE 8 MHz, PAL RGB, 4 MB, actual DMA
and VBL playback, idle main loop, raster debug off, before the stream/saturation
extensions (legacy packed mode; not a measurement of the saturated path):

| Active channels | CPU share | Mean mix+callback time per VBL |
|---|---:|---:|
| 0 | 4.22% | 0.845 ms |
| 1 | 4.17% | 0.835 ms |
| 2 | 4.08% | 0.818 ms |
| 3 | 3.99% | 0.800 ms |

The one-channel source is PacMad SIREN_1, then GAMESTAR and RBT are added for
2/3 channels. Counters cover the assembly and entire C mixer module, excluding
common VBL dispatch, interrupt entry/exit and initialization. Each capture spans
68-72 VBLs; sample boundaries, silence handling and ring splits affect cost.
The original fixed-250 scheduler measured 3.78-3.95% in the same scenario.
The corrected scheduler's added cost is about 0.2-0.3 percentage points.
`godlib.spl/pcmvoice/analysis/cpu-results.json` stores the raw cycle totals.

## Optional streamed source

`PcmVoiceMixer_SetStream(channel, reader, user)` reserves one channel for a RAM
producer callback. The callback receives `user`, an even-aligned destination
and an even count (up to 576), returns the number of bytes supplied and must
not perform file IO or change mixer playback control. Short reads are padded
with silence. The scratch source is mixed with the other two PCM channels
using the selected packed or saturating arithmetic. Only one streamed source
is supported. It remains
active across zero-byte reads until Stop/StopAll/DeInit or Play on its channel;
EOF/drain policy belongs to the caller. Queue on a streamed channel is rejected.
Reader and user storage must remain valid until detached. GetSample returns
NULL and GetRemaining returns zero for a stream; IsPlaying reports attachment.

## Game use

PacMad includes `pcmvoice.h` and uses Play/Queue for resident PCM. It does not
need or install a producer callback; channel priorities remain game-side.
Rogul's STE DMA profile includes the same header and uses channel 0 to consume
a 16 KiB SPSC music ring, replenished by file reads in the main thread. Channels
1/2 play effects; saturation is enabled. IFF loading, music loop/EOF and mute
policies remain in Rogul, with one DMA owner in GodLib.
See the [mixer guide](pcm-mixers.md) for migration and input constraints.
