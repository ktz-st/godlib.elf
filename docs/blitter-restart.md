# GodLib: BLiTTER restart benchmark (2026-10-23)

Hatari-HRDB STE / 68000 8 MHz / PAL low / German TOS 1.62. 32 complete draws per
case. Times below include render setup in the library and blitter bus stalls,
but exclude the caller, disk IO and OS interrupt handlers. These are emulator
cycle measurements, not measurements on physical hardware.

The main graphics ASM already uses shared-bus restart (`BSET #7` + NOP + BNE)
in `mBlitterGoWait_a1/a2/a6`, including unclipped, one-edge and both-edge sprite
paths. Entry `mBlitterWait_*` is plain polling because it waits for a preceding
operation; the start-and-wait macro performs actual forced restarts.

The separate C API `Blitter_DrawSprite` / `Blitter_DrawSpriteCanvas` previously
used plain BUSY polling. It now uses inline ASM restart waits on GCC/m68k. It does
not alter HOG, skew, masks, counts, strides or source graphics. The C test waits
for the final blit explicitly, since the existing draw API can return while
its final plane is still running.

## Main graphics ASM, masked four-plane sprites, ms

| Case | Plain polling | Existing restart | HOG | Register restart |
|---|---:|---:|---:|---:|
| 16x16 aligned | 0.4301 | 0.4341 | 0.4301 | 0.4236 |
| 16x32 skew 5 | 1.4986 | 0.9657 | 0.8842 | 0.9391 |
| 32x32 skew 5 | 2.4395 | 1.4172 | 1.2679 | 1.3781 |
| 64x64 skew 5 | 7.9965 | 4.3002 | 3.8318 | 4.1833 |
| 128x128 skew 5 | 29.0560 | 15.2345 | 13.5712 | 14.8352 |
| 64x32 left clip | 3.8068 | 2.0657 | 1.8454 | 2.0055 |
| 64x32 right clip | 1.9452 | 1.1779 | 1.0634 | 1.1450 |
| 128x64 both edges | 7.2076 | 4.0659 | 3.6223 | 3.9426 |
| 64x32 only 8px visible | 1.4090 | 0.8877 | 0.8042 | 0.8579 |
| 128x64 both edges, stride 320 B | 6.7025 | 3.7948 | 3.3629 | 3.6651 |

The register prototype sets A1 to the mode register and D4 to 7 once per sprite,
then uses `bset.b d4,(a1)`; both original NOPs are retained. It shortens the
restart loop, saving 2.4–3.4% relative to the previous graphics code in these cases.
HOG saves roughly another 9–11% of current restart time for nontrivial sprites,
but blocks CPU interrupt service for each complete plane pass. Shared restart
preserves opportunities to service interrupts between blitter time slices.

## Separate blitter.c API, masked four-plane sprites, ms

| Case | Existing C polling | ASM restart | Time reduction |
|---|---:|---:|---:|
| 16x16 aligned | 0.6089 | 0.6224 | -2.2% |
| 16x32 skew 5 | 1.6075 | 1.1412 | 29.0% |
| 32x32 skew 5 | 2.8360 | 1.5853 | 44.1% |
| 64x64 skew 5 | 8.2165 | 4.4283 | 46.1% |
| 128x128 skew 5 | 29.8007 | 15.2053 | 49.0% |
| 128x64 wide canvas | 15.0354 | 7.8130 | 48.0% |
| 64x32 clipped at top | 2.9641 | 1.7199 | 42.0% |
| 64x32 clipped at bottom | 1.8768 | 1.1744 | 37.4% |
| 16x16 top-left | 0.6090 | 0.6226 | -2.2% |
| 16x16 bottom-right | 0.6100 | 0.6238 | -2.3% |

For small aligned 16x16 sprites each plane's transfer fits in the first blitter
bus allocation: there is no mid-transfer bus return to accelerate, so forced
restart adds a small overhead. For larger/shared transfers, restarting avoids
spending the CPU's entire normal bus allocation just polling BUSY.

## Production integration and verification

`blitter/blitter.c` routes all 17 hardware wait sites through an always-inline
GCC/m68k helper (`BSET.B #7,(An); NOP; BNE`). Other compilers retain the C polling
fallback. The public API and its asynchronous final sprite plane are unchanged;
call `Blitter_Wait()` when the caller needs the completed framebuffer.

`graphic/grf_b4_s.s` sets A1 to MODE and D4 to 7 at the shared sprite-plane
entry, then uses register-indirect starts and `BSET.B D4,(A1)`. Both NOPs remain.
Unclipped, one-edge and both-edge paths share this loop; other drawing macros
retain their existing register usage. HOG remains disabled. No heap buffers,
precomputed sprite frames or public structure changes are introduced.

The compiled instruction sections match the measured REGISTER/CRESTART
prototypes byte for byte. All 60 prototype/case framebuffer comparisons against
the CPU renderer matched; 1920 timed draws completed. After integration,
20 production-library cases again matched the CPU renderer over the complete
64000-byte buffers, including both-edge clipping and a 640-pixel canvas.
An independent native regression also passed masked/opaque/coloured sprites,
fine X phases, top/bottom clipping, independent padded source/destination strides,
boxes, raw Screen wrappers and drawing below Y=200. Tests used German STE
TOS 1.62 and 4 MB RAM in Hatari; this does not establish physical-machine timing.

All 63 `godlib.spl` example directories were rebuilt, including the extra
Spectrum and live Wedges outputs, plus the Leonard 12.5 kHz variant. Only
`blitter.o` and `grf_b4_s.o` were replaced in the existing library archive;
other members were verified unchanged to preserve unrelated local audio work.

Benchmark sources and temporary native checks were kept outside the repository
under `/tmp/godlib-blitter-restart-20261023` and
`/tmp/godlib-blitter-production-20261023`. The compact
[benchmark data](blitter-restart-benchmark.json) retains cycle counts, frame
percentages and measured binary hashes without test binaries or large dumps.
