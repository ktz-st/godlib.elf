# Chunk background restore

`make`, then `python3 run.py` builds and runs the production ASM kernel on
Hatari ST, 68000 at 8 MHz, 4 MiB RAM, TOS 2.06. The runner needs the local
hatari-debugging helper and the configured ROM path (see `run.py`).

73 correctness cases check source/destination contents including buffer guards,
160/320/336-byte uniform strides, padded rows, Y=390 and 32-bit offsets above
65535, aligned/unaligned rectangles, zero and differing heights, gaps, full
32-entry capacity and a macro-allocated 40-entry list. The expected output is
built by an independent word-copy reference, not by the baseline implementation.

The production routine merges consecutive equal-height 8-byte columns into
runs of up to 32 columns. A 40-column run tests splitting into multiple runs.
Rows use an unrolled MOVE.L kernel with DBRA height iteration. There is no
blitter dependency. Source and destination must have identical, uniform layouts,
even addresses/strides and non-overlapping storage. Store does not clip.

`baseline.c` freezes the previous C copy loop, with only its local offset widened
to U32 to consume the new records. Both versions use the same new list layout;
this measures copy/merge performance, not old versus new ABI overhead.

Results in `results.json` (2026-10-16): each timing performs 2048 restores,
excluding list construction, measured with the emulated 200 Hz system timer.

| Rectangle | C ticks | ASM ticks | C / ASM |
| --- | ---: | ---: | ---: |
| 16x16 aligned | 124 | 96 | 1.29x |
| 16x16 unaligned (32px restore) | 229 | 136 | 1.68x |
| 32x32 aligned | 429 | 233 | 1.84x |
| 32x32 unaligned (48px restore) | 635 | 307 | 2.07x |
| 64x64 | 1644 | 708 | 2.32x |
| 256x64 | 6525 | 2390 | 2.73x |

These are elapsed emulated times including system interrupts, not CPU-only
profiles, whole-frame speedups or measurements on physical Atari hardware.
The example's 16x16 sprite corresponds to the first two rows.
