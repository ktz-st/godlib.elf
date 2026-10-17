# Four-channel MOD oscilloscopes

All three MOD replays expose the same read-only waveform-preview interface:

```c
S8 trace[dMODSCOPE_BYTES]; /* 4 channels * 64 signed bytes = 256 bytes */
LanceMod_GetOscilloscope(trace);
/* or LeonardMod_GetOscilloscope(trace), Wizzcat_GetOscilloscope(trace) */
```

Call only the getter belonging to the active replay, from the supervisor main
loop. NULL is a no-op; stopped/inactive channels return zero. Trace order is
CH1, CH2, CH3, CH4, matching the activity getter. Do not call after unloading the
replay code. The getter does not retain caller pointers or allocate heap memory.

The traces are **pitched, volume-scaled source previews**, not literal channel
captures from the stereo DMA buffer. The latter already contains a mix of two
channels per side (or Wizzcat's interleaving), and the players do not retain four
full per-channel PCM output pages. The preview therefore reads each native
channel's current sample position, end, repeat range, current period and volume.
It reconstructs a short waveform window without changing the mixer or advancing
its real phase. Each trace spans approximately 10.23 ms: 64 points at a nominal
6258 samples/second, independent of the replay's selected DMA frequency.

A 96-byte descriptor snapshot is copied with interrupts masked. The original SR
is restored before sampling, avoiding an interrupt stall during all 256 reads.
The caller keeps module data alive for the duration of the call. The common ASM
sampler handles initial sample ends, loop overshoot, short loops, one-shot
silence, signed samples and volume clamping. Lance's signed seven-bit prepared
samples are expanded back to a normalized signed-byte range. Leonard/Lance use
the PAL Paula clock and Wizzcat its original NTSC clock convention. Amplitude is
normalized per source voice; stereo summing/headroom and Microwire output gain
are not part of this preview. Periods outside 48..4095 return a flat trace.
The sampler starts the preview's fractional phase at zero, rather than exposing
each mixer's internal fractional/rounded interpolation state.

No sampling is added to a replay interrupt. The examples call this getter only
when oscilloscopes are visible, so bars and hidden mode do not perform scope
sampling. No extra native PCM buffers or per-sample hooks are required.

## Example controls and drawing

The `lancemod`, `leonard` (12.5/25K) and `wizzcat` examples use:

- **O**: cycle bars → four channel traces → ICE XY → separate L/R traces → mono trace;
- **V**: hide/show the selected visualizer;
- **F1**: existing replay elapsed-time stripe;
- **Space/Esc**: exit; Wizzcat retains **1/2** song-position jumps.

The oscilloscope layout is two columns, two rows: CH1/CH4 left, CH2/CH3 right.
Each signed sample becomes a horizontal two-pixel mark in a 128-pixel-wide
trace, with amplitude divided by four vertically. This is a sampled dot trace,
not a filled polygon or an interpolated line. Plane zero is XORed to erase the
old marks and draw the new ones. Each logical screen keeps its own 256-byte
previous trace; the live trace is another 256 bytes. Labels and controls are
part of the clean background. A page is restored once when its visualizer or
visibility changes; normal scope frames do not copy/clear the entire screen.

Drawing is in `godlib.spl/lancemod/scope_s.s`, shared by the three examples along
with the existing bar renderer/artwork. Its 512-byte signed-Y lookup removes
runtime multiply/divide from point plotting. The getters/sampler are in GodLib;
the screen layout and XOR renderer belong to the examples.

## ICE delayed stereo XY plots

The third example mode adapts Techwave's `MODOSC.S` from ICE's Digital World II:
`pixels_out` / `oscillo_l` / `oscillo_r`, `xyput` and `pixels_clear`.
Source path and hash are in
`godlib.spl/lancemod/assets/ice-xy-source.json`; the original source is unchanged.

Unlike the four channel source previews, this mode uses **actual mixed stereo
PCM**. Each replay now exposes `*_GetStereoOscilloscope(S8 samples[328])`.
It copies 164 interleaved stereo frames from the latest completed mix block:
Lance's completed page, Leonard's preceding ring block, Wizzcat's freshly mixed
page. A short IPL7 copy keeps the data coherent; caller SR is restored afterwards.
NULL is a no-op, stopped playback returns silence, byte-aligned output is allowed.
All supported replay rates have at least 164 frames in each block. No retained
pointer, heap allocation, mixer changes or DMA register changes are needed.

For each side and point `t=0..99`, the ICE mapping is:

```
X = originX + ((signedPCM[t+64] + 128) >> 1)
Y = 68      + ((signedPCM[t]    + 128) >> 2)
```

The original byte `SUB #128` and logical shifts are preserved as equivalent
XOR/sign rebiasing. X uses a delayed version of the **same stereo side**, not the
other channel: these are delay-embedding XY figures with a Lissajous-like look,
not a direct left-versus-right phase meter. Origins 16/176 keep both 128x64 plots
inside 320x200. The delay is 64 **native** stereo frames, so its duration changes
with DMA frequency (Wizzcat retains its original interleaving). Microwire gain
is applied after the PCM buffer and is not part of the display.

The ASM renderer `godlib.spl/lancemod/xy_s.s` first clears all 200 old plane-zero
words, then OR-plots 100 points for each side. It preserves overlaps and the
single dot of a silent signal. Two independent sets of 200 U16 screen offsets
use 800 bytes, plus a 328-byte PCM snapshot. This replaces the original absolute
clear addresses and screen/VBL/shifter ownership with GodLib's normal lifecycle.
The XY drawing regions have a plain black background; full-word clearing is
valid there. Page history is reset on mode/visibility transitions.

## Stereo and mono time traces

The fourth and fifth example modes use the same actual PCM snapshot as ICE XY.
They display the first **128 native stereo frames**, rather than the per-voice
source preview. Traces are 256 pixels wide (`x=32..287`), with two-pixel sample
marks and signed amplitude divided by four vertically.

- **Separate L/R**: left trace centred at Y=64, right at Y=148. Left uses plane 2
  (green), right plane 3 (cyan).
- **Mono**: one white trace in plane 0, centred at Y=100. ASM sign-extends both
  PCM bytes, adds them in 16 bits and divides by two with arithmetic shift.
  Negative half values round down; there is no 8-bit sum overflow.

Mono affects visualization only; the replay remains stereo. Stereo-mode palettes
are persistent, with the original palette restored for bars and other scopes.

`godlib.spl/lancemod/stereo_s.s` supplies shared ASM initialization/drawing.
It reuses the signed Y lookup from `scope_s.s`, the existing 328-byte PCM snapshot
and the existing 256-byte previous-sample history per logical screen. Mono uses
only the first 128 history bytes. No additional audio buffers or heap allocations
are needed. Zero traces are initialized on a page transition; subsequent frames
XOR old/new marks only. Native sample rate determines the time span.

O cycles **all five modes**, and V hides/shows the selected mode.
