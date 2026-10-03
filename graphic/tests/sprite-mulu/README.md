# Four-plane sprite increment optimization

The shared sprite blitter body scales source X/Y increments by
`mGfxPlaneCount` after applying the mask. Four-plane graphics use two
`LSL.W #2` instructions, guarded by a plane-count comparison. Other counts
retain the original two `MULU` instructions.

Only the low 16 bits of either product are written to the blitter. Shifting
the word therefore preserves multiplication by four, including negative
row increments encoded as unsigned words. A lookup table would add indexing
and memory reads for a multiplication already expressible as a shift.

`baseline.s` and `shift.s` are independent snapshots with renamed global
symbols. Both are linked into the same test executable; later production
changes do not silently alter this comparison. Run `make`, then
`python3 run.py` using the local hatari-debugging helper.

Correctness checks cover masked four-plane sprites of widths 16/32/48/64,
height 16, 100 positions per width, at destination strides 160 and 320 bytes.
They include fine alignment, left/right and top/bottom clipping, and complete
rejection. Both versions are compared against an independent per-pixel
reference over the entire buffer, including guard words: 800 cases per version.
Seven padded synthetic sprites with plane counts 1..8 excluding four check
that the fallback preserves the old output; they do not certify these layouts
as supported by the four-plane renderer.

Timing uses 8000 clipped draws per case, with aligned X=64 and shifted X=69,
Y=8, destination stride 160. The timer is the emulated HZ200 clock, with
interrupts enabled. Results include the call, setup, blitter execution and
waiting, rather than measuring CPU instructions alone. Each timer tick is
0.625 microseconds per draw. Raw results are saved in `results.json`.

Only masked sprites and widths divisible by 16 are checked here. The optimized
shared body is also used by the unclipped and opaque sprite paths. Real hardware
has not been profiled.

Measured result: both versions passed all 800 reference cases, and all seven
fallback comparisons passed. Hatari reported no bus/address/illegal exception.

| Sprite | X | MULU us/draw | Shift us/draw | Saving us/draw | Saving |
|---|---:|---:|---:|---:|---:|
| 16x16 | 64 | 507.500 | 505.625 | 1.875 | 0.37% |
| 16x16 | 69 | 681.875 | 680.000 | 1.875 | 0.27% |
| 32x16 | 64 | 748.750 | 746.250 | 2.500 | 0.33% |
| 32x16 | 69 | 919.375 | 917.500 | 1.875 | 0.20% |
| 48x16 | 64 | 985.625 | 983.750 | 1.875 | 0.19% |
| 48x16 | 69 | 1117.500 | 1115.000 | 2.500 | 0.22% |
| 64x16 | 64 | 1224.375 | 1221.875 | 2.500 | 0.20% |
| 64x16 | 69 | 1355.000 | 1353.750 | 1.250 | 0.09% |

The difference is small (2..4 timer ticks over 8000 draws). These single-run
elapsed results establish a modest benefit, not a precise CPU cycle saving.
The guard and fallback add 30 bytes to the assembled sprite module.
