# Wide canvas Blit / DrawSprite

`make`, then `python3 run.py` (Hatari STE, TOS 1.62 De, 4 MiB, local
hatari-debugging helper). Exit code 0 = no failures in any of the
categories. `results.json` holds cases/failures per category and the first
failing cases (`phase, a, b, c, d`: for blits `sx, dx, width, height`; for
sprites `x, y, width, clipX0 | clipX1 << 10`). `rout.py` maps CPU blit cases to
the `grf_4_s.s` jump-table routine they take.

Cases, CPU (`Graphic_SetBlitterEnable(0)`) and blitter path each:

- `GraphicCanvas_Blit`, 720x16 source; 720x16 and 320x32 destinations; source x
  0..655 (including 344 and 511..513), destination x 0..300, widths 1..96,
  source y / heights varied. Reference: per-pixel copy; whole buffers compared,
  guards included. Blitter cases are counted per `BLiT_iT` `skew_flags` entry.
- `DrawSprite` and `DrawSprite_Clip` on the 720 px canvas, masked 16..64 px
  sprites, every x from left clip to right clip around 0, 160..220, 500..530.
- `DrawSprite_Clip` on a 320 px canvas, Y clip box 3..29, 12 X clip boxes
  (aligned and unaligned edges, narrower than the sprite, 1 px wide), sprite x
  crossing every fine phase around both edges. Boxes with an unaligned edge are
  counted in categories 6/7.

Results 2026-10-22 after the fixes: all categories 0 failures (Blit 4839 per
path, skew entries 0,1,2,3,5,6,7 covered, 4 unreachable; DrawSprite_Clip 1916 +
2386 per path). Before: CPU Blit 640 failures (`LR` 312/312, `C_F` 328/328),
blitter Blit 242 (all entry 7), DrawSprite_Clip both-edge clipping 225 per
path, unaligned clip boxes 1587 (CPU) / 135 (blitter).
