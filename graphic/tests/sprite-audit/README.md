# GodLib blitter sprite audit

Production `graphic/grf_b4_s.s` was not modified. `current.s` is an audit snapshot
with the common sprite body exported solely to link the historical clipper.
`old-clip.s` contains the clipper before commit d37d7db (Blitter sprite fix).
Both clippers use the same current common draw body, isolating clipping changes
from later stride fixes or unrelated blit changes. The snapshots are retained
so the report is reproducible; later production edits do not update them.

Run `make`, then `python3 run.py` with the local hatari-debugging helper.

Correctness: 320x64 canvas, stride 160, four-plane masked sprites of widths
16/32/48/64 and height 16, 100 positions per width per clipper. Positions include
all 16 fine alignments, left/right clipping, full rejection, top/bottom clipping,
and combined left/top clipping. Independent per-pixel references are compared
against every word including guards. Current: 0 failures / 400 cases.
Historical clipper: 87 failures / 400 cases; first failure width 32, aligned X=64.

Timing: Hatari STE 8 MHz, 3000 repeated draws per case, elapsed emulated HZ200
clock (5 ms ticks), interrupts enabled, no background restore. Shared C loop and
shared rendering body. Each reported microsecond value includes call/setup,
blitter execution and waits; it is not a CPU-only instruction profile. One tick
is about 1.67 us per draw; tiny current/old deltas are below a reliable regression
claim from this single run. Historical timing is not proof of equivalent output.

Fully visible sprite at X=69 (fine shift 5), Y=8:

| Sprite | Current clipped us/draw | Old clipped us/draw | Current unclipped us/draw | Current vs old | Unclipped saving |
|---|---:|---:|---:|---:|---:|
| 16x16 | 681.7 | 680.0 | 646.7 | 0.25% | 5.13% |
| 32x16 | 920.0 | 916.7 | 885.0 | 0.36% | 3.80% |
| 48x16 | 1116.7 | 1115.0 | 1083.3 | 0.15% | 2.99% |
| 64x16 | 1355.0 | 1353.3 | 1321.7 | 0.12% | 2.46% |

For aligned 16x16, current/old clipped ticks are 305/369: the historical single
word path requests FXSR even at skew zero and uses SrcIncX=0. Current code avoids
that redundant source read, reducing elapsed time about 17.3% in this run.

Findings:

- d37d7db changed the clipper and corrected the blit symbol spelling; it did not
  add a pass or a per-scanline software loop to the shared sprite renderer.
- The clipper grew from 300 to 346 bytes in these assembled snapshots. Code size
  is not runtime cost. The tested shifted interior cases show <=0.37% difference.
- Both still launch four AND mask blits followed by four OR colour blits.
- Clipping setup is executed even for an entirely visible sprite. Using the
  existing non-clipped function for a sprite known to fit saves ~33..38 us/draw
  here (2.46..7.54% depending on size/alignment), before any dispatch-check cost.
- Two MULU instructions convert source increments using GfxPlaneCount. A proven
  four-plane-specialized path could use shifts, with fallback for other layouts.
- Keeping skew/FXSR/NFSR decisions in registers until the final MMIO write could
  avoid some hardware register reads/read-modify-writes; benefit is unmeasured.
- Early full-containment dispatch is the most conservative next optimization.
  Edge cases must keep the corrected clipper. The tested widths are multiples
  of 16; this audit does not certify arbitrary widths or all clip-window origins.

Raw HZ200 results and failure counters are in results.json. Real hardware was
not profiled. No production optimization is applied by this audit.
