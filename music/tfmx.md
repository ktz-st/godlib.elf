# TFMX / UGH! four-channel replay

`music/tfmx.h` ports the native 68000 TFMX engine used by `ugh.agt`. It supports
that original four-channel `TFMX-SONG` MDAT + SMPL format on PAL STE/MegaSTE,
with selectable Lance or Leonard Paula at 12.5/25 kHz. This is not a generic
TFMX Pro/7-voice decoder. The original UGH! project and assets remain untouched.

```c
#include <godlib/music/tfmx.h>
sTfmxSong *song = Tfmx_Load("SOUNDS.MDT", "SOUNDS.SMP", eTFMX_MIXER_LEONARD);
if (song && Tfmx_Start(song, 0, eTFMX_FREQ_25K)) {
    /* Draw and handle input. Music advances in its own VBL callback. */
}
Tfmx_UnLoad(song);
```

Initialize GodLib platform/file/VBL services and enter supervisor mode before
playback. These are supervisor APIs. `Start` installs the callback; do not call
`Tick` from the main loop. `SetSubSong` switches slots without allocating.
`GetVolumes` gives four gated PSG-independent Paula channel volumes, saturated
to 0..64, not peaks.
`SetRasterDebug` brackets the complete audio tick with palette colour 0.
`Stop` detaches VBL before stopping DMA/restoring trap #0/freeing workspace;
`UnLoad` also frees the song, stopping it first when it is active.

The tiny shared Paula ownership lease excludes LanceMod, LeonardMod, RJP1 and
TFMX from acquiring either mixer while another player owns DMA. It does not
arbitrate unrelated DMA engines: stop those explicitly. Neither ProTracker nor
RJP sequencer is linked into the TFMX example.

## Native bridge

The sequencer is the IRA-recovered UGH! assembly, with Amiga custom registers
redirected to the shared RAM mirror. Amiga IRQ installation is not used; CIA
and interrupt-vector accesses go to private scratch RAM. Every DMACON write is
immediately forwarded to the native DMA gate/latch service, preserving registers
and CCR. A clear/set inside one macro and same-address retriggers are retained;
there is no end-of-tick address-change heuristic. Period, volume, source and
reload changes remain controlled by the original macro interpreter.

TFMX envelopes in the UGH! bank routinely emit volume **65** (5647
channel observations in 4000 title ticks). Leonard already saturates this to 64.
The Lance adapter must also saturate before accessing its ratio/LMC tables:
Lance's `(volume-1)&63` would otherwise turn 65 into index 0, as though the
louder voice had volume 1. The resulting software-volume lookup can mute or
mis-scale the other voice of the stereo pair; the LMC lookup is out of bounds.
The sequencer's register mirror stays unchanged; only the mixer input and
public volume display are saturated. This fixes the instrument dropouts that
remained after the separate DMA-start timing correction.

On Lance, each PAL VBL first starts the already mixed DMA buffer, then runs
one sequencer tick, prepares source windows and mixes the next buffer. Keep
the DMA start before this variable-cost work: one-shot DMA lasts approximately
one frame and a late restart produces gaps or clips the preceding buffer.
On Leonard, each VBL services the ring-buffer mixer. Like UGH!, the sequencer
runs from Leonard's user callback only when a new PCM block is accepted. If
the DMA buffer is busy, both sequencing and mixing wait together. Advancing
TFMX before a rejected block can consume an instrument attack/note change
without rendering it, causing sporadic cuts. The callback is detached during
Stop before its song is freed. A realtime PAL regression over 1000 VBLs per
rate found 16/9 unrendered steps at 12.5/25 kHz before this correction, zero
afterwards. SR/USP survive either replay path.

Leonard uses the private original signed 8-bit bank and its current/end/reload
voice pointers directly. Lance uses a private signed 7-bit bank and four
**672-byte** source windows. Before mixing, the bridge stitches the upcoming
source across the current attack and live reload boundaries. If at least 672
bytes remain before the boundary, it reads the private bank directly without
copying. The voice end still bounds the window, so source consumption is
computed the same way for both paths. Afterwards, it
advances the native source pointer by the actual bytes consumed and preserves
the fractional phase. Tiny loops seed one period and double it using block
copies; there is no per-byte wrapping or whole-sample copying in VBL. This
supports TFMX macros that move loop starts/lengths during a note, without
requiring per-sample PCM guards. The Lance path has the existing 108..907 period
limit; very high/low notes can therefore differ from Leonard. All audible
periods in the supplied seven UGH! cues fall inside this range in the 28000-step
sequence regression (the combined range is 113..764).

## Memory and inputs

`Create` copies both input blobs, so the caller can release them immediately.
`Load` loads/prepares/releases input files. Sources are never edited in place.
The engine's original four-byte bank silence prefix is cleared only in its
private copy. Lance converts that same private bank to 7 bits; no second bank
is retained. Playback allocates nothing.

For the supplied UGH! bank (9784 B MDAT + 30596 B SMPL):

| Backend | Resident song | Workspace 12.5 kHz | Workspace 25 kHz |
|---|---:|---:|---:|
| Lance | 43100 B | 142146 B | 176320 B |
| Leonard | 40412 B | 61134 B | 91134 B |

`GetInfo().mResidentBytes` includes metadata and any source windows;
`GetAllocatedSize` reports the selected mixer workspace separately. Common PRG,
screen buffers and allocator overhead are additional. Select the backend at
load/create time because its PCM representation differs. Only 12.5/25 kHz are
exposed by this API.

The loader checks the header, section offsets/alignment and subsong track ranges.
Subsong count is inferred through the highest nonzero start/end table entry
(slot 0 is included); UGH! supplies slots 0..6. This is a trusted-asset replay,
not a validator/interpreter sandbox for arbitrary macro bytecode. The Lance
bridge checks source/reload ranges against its private bank and disables an
invalid voice rather than reading past it. TFMX Pro, seven voices, unsupported
bytecode/timing variants and generic malformed-input safety are outside scope.
No public SFX interface is exposed by this first music example.

See [example and measurements](../../godlib.spl/tfmx/README.md) and
[provenance](tfmx-source.json). Existing UGH! music is a demonstration asset;
copying it here does not grant a new redistribution licence.
