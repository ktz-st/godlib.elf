# Four-plane ASM line regression and timing

Build and run from GCC.ELF:

```
make -C godlib
make -C godlib/graphic/tests/line
python3 godlib/graphic/tests/line/run.py
```

`verify.c` calls the current production CPU and BLT-forwarding entry points.
It compares complete guarded buffers against both an independent pixel reference
and `baseline.s`, a frozen copy of the original line renderer. Baseline symbols
are renamed so both versions run in the same executable.

4096 line calls cover all 16 colours (including erasing with zero), both endpoint
directions, horizontal/vertical/diagonal lines, single points, crossing 16-pixel
word boundaries, random slopes, and clipping against an inset box. Comparisons
are performed after each group of 32 lines; guards, padding and unrelated pixels
are included. Layouts are 320 pixels/160-byte stride, 640/320, padded 352/192,
and 640 pixels with nonuniform row offsets `y*336+(y&1)*8`. All layouts are
432 rows tall, including addresses above 65535 bytes. Both direct and BLT
forwarders are exercised. Partial clipping keeps the original rasterization,
so clipping does not shift Bresenham's error phase.

`results.json` records the latest successful run. Timing uses emulated HZ200
elapsed ticks around 512 calls per variant on Hatari STE 8 MHz. It includes
call/loop overhead, display bus contention and normal platform interrupts; it
is not a CPU-only cycle profile or a physical hardware measurement. Both
variants use the same coordinates, canvas, colour sequence and executable.
Fast-forward removes host pacing; the timer remains emulated Atari time.

The optimized renderer keeps its pixel mask and current word pointer across
iterations. Vertical steps use differences between adjacent U32 row offsets,
including nonuniform rows. The clipped entry dispatches fully visible lines to
this loop; partially visible lines retain pixel clipping but save only the five
registers modified by the plot helper, instead of twelve.

Latest measurement (2026-10-06):

| Line | Optimized ticks | Original ticks | Speedup |
|---|---:|---:|---:|
| Horizontal, 280 pixels | 1111 | 2428 | 2.19x |
| Diagonal, 280 pixels | 1344 | 2454 | 1.83x |
| Vertical, 161 pixels | 758 | 1415 | 1.87x |
| Clipped, fully inside, 280 pixels | 1333 | 2793 | 2.10x |
| Clipped, crossing screen edges | 2753 | 3196 | 1.16x |

All 4096 correctness cases passed; no bus, address or illegal-instruction
exceptions were detected. No changes to SuperHexagon are part of this update.
