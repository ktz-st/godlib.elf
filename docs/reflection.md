# Missing Link REFLECT

`graphic/reflection.h` / `graphic/grf_reflect_s.s` provide the STOS Missing Link
water-reflection algorithm for ST/STE, implemented in 68000 assembly.
This is unrelated to `godlib/reflect`, the C structure/type-description module.

```c
#include <godlib/graphic/reflection.h>
sGraphicReflect lake = { source, destination, 0, 199, 112, 0, 0 };
Graphic_Reflect(&lake); /* once per animation frame */
```

Both buffers must be even-addressed 320x200 four-plane ST-low layouts with a
160-byte stride. This entry point does not support virtual-screen strides.
The coordinates are signed and clamped to 0..199. The upper source endpoint is
exclusive: `y1=0,y2=199` first reads source row 198. The original cannot include
source row 199 by passing 200, because 200 is clamped to 199.

Let `d = clampedY2 - clampedY1`. For `d >= 8` and destination Y < 199, it copies
`min(((d-8)>>1), 199-destinationY) + 1` full 160-byte rows. Note the final `+1`
from DBRA. At destination Y=199 it does nothing, retaining the original test.
The source pointer begins at `y2-1` and moves upwards after each copied row by
one entry from this periodic table:

```
1,1,2,2,2,3,3,4,4,3,3,2,2,1,2,2,1,1,1,0
```

Zero repeats a row. The mean step is two rows. This compresses the reflection
vertically by about two, without horizontal pixel shifts, tinting, palette
changes, blitter work or raster interrupts. The four MOVEM load/store groups
(44+44+44+28 bytes) are retained from the original routine.

Initialise `mToggle=mPhase=0`. Each drawing call toggles `mToggle`, and every
second call advances `mPhase` by two bytes through 20 table starting positions.
The first calls use phases 0,1,1,2,2,...,19,19,0; the complete state repeats after
40 calls. The state belongs to an animation, not to a screen page. Pausing while
redrawing both pages requires reusing a saved input state (see the sample).

The input and output may alias if the caller prevents writes from overwriting
source rows that later iterations still need. No temporary framebuffer is
allocated. The routine does not clear untouched destination rows.

Historical edge behavior is preserved: for some short ranges (e.g. source
0..20 at table phase 5), it reads one row before the requested source start.
Provide a readable 160-byte guard row before a buffer whose requested start is
row zero, or choose a range that cannot do this. The sample's 0..199 source and
88-row destination stay safely within their source. Do not assume that `y1`
is a strict source clipping boundary.

The port changes only the STOS parameter parser/return convention, register
preservation and state ownership. It stores the repeated table six times
instead of 80 times (enough for the maximum 96 output rows). An additional guard
returns without drawing/advancing for reversed or fewer-than-eight-row ranges,
which otherwise underflow the original unsigned loop count. State fields must
contain valid values; use zero-initialised state rather than uninitialised RAM.

The [reflect example](../../godlib.spl/reflect/README.md) uses the same generated
castle as `water`. A separate source buffer stretches **only rows 0..110** to
200 lines before rendering; this preparation lets the unchanged compression
algorithm fill the 88-row lake. The lower, previously painted water is not used.
The display's upper rows and palette are unchanged. The sample runs on ST as
well as STE, with Space to pause, N to step and Esc to exit.

Source and target evidence is in `godlib.spl/reflect/analysis`:

- original `stos/mislink/mislink/STOS/LINK2.EXR`, SHA-256
  `1da8647e7c4141b6dfef6d892e9b2e443df8b6b90f13fabb0cd58927979085d4`;
- dispatch TEXT $15da, core $1642, copy loop $16c6, state $170e,
  row steps $1712 and line offsets $4180; add $1c for file offsets;
- `inspect-missing-link.py` reproduces the listing and extracts the original
  machine code. Only its STOS return at TEXT $16fc is patched to RTS for tests;
  all core instructions and PC-relative table operands remain intact;
- `verify-original.py` runs original code and port in the same Hatari process,
  comparing all 32000 destination bytes and the state at every call;
- `verify-hatari.py` compares complete displayed frames with the decoded row
  model, checks both screen pages, controls and clean return to TOS;
- `profile.py` measures the public routine with Hatari-HRDB at 8 MHz.

The original-code comparison passed 400 calls (10 coordinate cases, all 40
states each), comparing all destination bytes and the state.

This establishes agreement with the extracted 68000 core, not with a complete
STOS interpreter session or physical Atari hardware.

Measured public-call cost for the 88-row castle sample is about 69,048 cycles
on ST 8 MHz: 8.63 ms, 43.1% of a PAL frame, approximately 50 updates/s.
The count includes coordinate/phase handling and copies but not main-loop,
input or VBL overhead. The original copy kernel is deliberately retained.
