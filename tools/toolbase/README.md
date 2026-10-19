# Host TOOLBASE / BSBMAKER

Python 3 host implementation of the RG asset pipeline, with explicit big-endian
32-bit-offset serialization. No Atari emulator is required.

```sh
make -C godlib/tools/toolbase
godlib/bin/Darwin/arm64/bsbmaker path/to/MAKEGFX.SCR
python3 godlib/tools/toolbase/toolbase.py path/to/assets.tbs
```

The direct command accepts one BSBMAKER `.SCR`. TOOLBASE batches support
`BSBMAKE`, `MASKMAKE`, `PALMAKE`, `MAKELINK`, `GODPACK`, and `CUTBUILD`.
The existing `[ HEADER ]`, `[ SPRITE ]`, and `[ FONT ]` syntax is retained.
Inputs are uncompressed low-resolution PI1, with a separate PI1 mask for masked
sprites. A mask bit of 1 preserves the destination; 0 draws the sprite, including
opaque palette-zero pixels. Files do not embed a palette; use PALMAKE separately.

## Rotation script

```text
[ SPRITE ]
{
    src = ARROW.PI1
    msk = MASK.PI1
    dst = ROTATE.BSB
    x = 0
    y = 0
    width = 24
    height = 16
    planes = 4
    rotframecount = 32
    rotstartangle = 0
    rotendangle = 6.283185307179586
    rotpivotx = 12
    rotpivoty = 8
}
```

- Angles are **radians**, as in the original BSBMAKER. Positive angles turn
  clockwise in screen coordinates.
- Frame `i` uses `start + i*(end-start)/rotframecount`; the final endpoint is
  excluded. `0..2*pi` therefore loops without duplicating the first frame.
- A rotation series uses the first source frame. `framecount`, `stepx` and
  `stepy` describe the normal sheet extraction path when `rotframecount=0`.
- Pivot coordinates are relative to that source frame, defaulting to `(0,0)`.
  Every generated frame has a shared canvas/origin, so the pivot stays fixed
  across the series. Width is rounded up to 16 pixels for the planar format.
- Graphics and mask use the same inverse nearest-neighbour transformation.
  Pixels outside the source are transparent for masked sprites. `opaque=1`
  keeps the entire output rectangle opaque, as requested by that option.
- The old forward mapping could leave holes and truncated the output bounds.
  The host implementation rounds bounds outwards and samples backwards. It
  preserves script semantics, not the old rasterizer's defective pixel output.
- **`rot90=1` preserves the historical diagonal transpose** `(x,y)->(y,x)`.
  It swaps dimensions and transforms both graphics and mask, after any rotation
  series. This historical option is not a clockwise quarter-turn. For a true
  clockwise 90-degree turn use `rotframecount=1`,
  `rotstartangle=1.5707963267948966`, `rotendangle=1.5707963267948966`.
- As in the original word-based source extractor, `x` addresses a 16-pixel
  source word (rounded down). Use word-aligned sheet origins/steps.

Rotations work for BSB and BFB output and 1–4 graphics planes. Counts are capped
at 4096 and rotation output at 16 million pixels / a 4096-pixel canvas edge;
invalid dimensions, angles and source bounds fail rather than producing a
truncated file. A BFB character map remains byte-indexed.

`python3 tests/test_rotation.py` checks cardinal turns, shared canvases, custom
pivots, opaque black and transparent pixels, interior coverage, the historical
transpose, bounds, and full SCR-to-BSB/BFB builds.

A runnable source example is in [AssetView demo](../assetview/demo/ROTATE.SCR).
