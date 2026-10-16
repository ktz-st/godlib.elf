# PCM mixer CPU profiles

Build with `make`. Run `python3 run.py` for PCM Block/PCM Frame/PCM Layer and
`python3 run.py --legacy-only` for the original AudioMixer. This requires the
installed `hatari-hrdb` binary and `hatari-debugging` skill helpers; stock Hatari
is not a substitute for this remote per-instruction profiler.

The test uses actual DMA playback and the production VBL callbacks on a PAL RGB
STE, nominal 8 MHz, 4 MB, with palette raster debug disabled. Samples are resident
and even-aligned; the main thread only busy-waits for a case request. No drawing,
file IO or initialization is included in the measured interval. Each case lasts
approximately 87 VBLs. PCM Block uses a 1024-byte whole-sample loop. PCM Frame channel zero
is a long 255-block one-shot, channels 1/2 loop four 250-byte blocks. PCM Layer's
channels 0/1 loop 1024 bytes; 2/3 are long 65536-byte one-shots. Thus supported
loops cross their source ends repeatedly; one-shots remain active throughout
capture. Existing example verification additionally checks loop/stop behavior.

Cycle attribution includes the entire mixer assembly section and its C VBL
wrapper. It excludes common VBL dispatch, interrupt entry/exit, trigger APIs and
initialization. DMA/bus access effects charged to these instructions remain
included. CPU percentage is callback+core cycles divided by VBL calls times the
PAL 160256-cycle frame budget. Mean microseconds use nominal cycles/8; these
are averages over callbacks, not worst-case refill latency. The sum of all
profiled cycles is checked against that frame budget as a sanity check.

Keep the profiler enabled through case warmup. This hrdb version retains its
previous instruction timestamp when restarting collection; disabling it while
the CPU runs charges the unprofiled gap to the next capture's first instruction.
A break/run resets the collection without that gap when profiling stays enabled.
The runner checks nonzero profiles, callback counts and implausible instruction
costs. Corrected captures are in results.json and legacy-results.json.

Results are emulator measurements, not real STE/MegaSTE timings. This is a
comparison of the shipped algorithms, not identical-quality engines: rates,
channel roles and even sample-addition semantics differ. Raster bars alone do
not supply these percentages. Source code is left unchanged by the harness.

## Measured results (2026-10-06)

| Mixer | Active sources | CPU frame budget | Mean us/VBL |
|---|---:|---:|---:|
| PCM Block 12.5 kHz | 0 | 6.99% | 1400.9 |
| PCM Block 12.5 kHz | 1 | 6.89% | 1380.1 |
| PCM Block 12.5 kHz | 2 | 6.89% | 1380.8 |
| PCM Block 12.5 kHz | 3 | 6.79% | 1360.4 |
| PCM Block 12.5 kHz | 4 | 6.87% | 1376.4 |
| PCM Block 25 kHz | 0 | 13.45% | 2694.3 |
| PCM Block 25 kHz | 1 | 13.46% | 2696.7 |
| PCM Block 25 kHz | 2 | 13.41% | 2685.4 |
| PCM Block 25 kHz | 3 | 13.42% | 2687.8 |
| PCM Block 25 kHz | 4 | 13.36% | 2676.6 |
| PCM Frame 12.5 kHz | 0 | 3.04% | 609.6 |
| PCM Frame 12.5 kHz | 1 | 2.97% | 594.5 |
| PCM Frame 12.5 kHz | 2 | 2.94% | 589.7 |
| PCM Frame 12.5 kHz | 3 | 2.88% | 577.9 |
| PCM Layer 12.5 kHz | 0 | 2.77% | 555.0 |
| PCM Layer 12.5 kHz | 1 | 1.89% | 378.2 |
| PCM Layer 12.5 kHz | 2 | 5.57% | 1115.2 |
| PCM Layer 12.5 kHz | 3 | 9.07% | 1816.7 |
| PCM Layer 12.5 kHz | 4 | 12.62% | 2527.9 |
| AudioMixer 25 kHz stereo | 0 | 5.51% | 1103.0 |
| AudioMixer 25 kHz stereo | 1 | 5.87% | 1175.1 |
| AudioMixer 25 kHz stereo | 2 | 5.91% | 1183.3 |

PCM Block and PCM Frame perform their complete sum even with inactive sources. PCM Layer
skips inactive extra layers; its base copy is faster than its idle zero-fill.
Original AudioMixer has no sample-loop support and uses a different fixed-stereo
mask/interleave algorithm, so its number is not an equivalent-quality comparison.

Mixer names were updated after these measurements; recorded cycle counts are unchanged.
