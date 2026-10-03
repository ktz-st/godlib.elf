# Font8x8 virtual line layouts

Run `make`, then `python3 run.py` with the local hatari-debugging helper.
The test binary uses the production library and the normal GCC ELF/mshort/
mfastcall ABI. It compares planar output against an independent per-pixel
reference, including untouched bytes, neighbouring planes, padding and guards.

58 cases cover mono and coloured text, both 8-pixel halves of a planar group,
all 16 palette colours, widths 320/352/640, strides 160/176/320/336, nonuniform
row offsets, and Y=400/424 (including addresses above 65535). Legacy raw buffer
calls retain stride 160. Canvas calls use their own line tables. All three active
Screen canvas identities are checked through the legacy APIs after initializing
a 640x400 H+V virtual screen. Their VRAM pointers are temporarily replaced with
the guarded test buffer so both end guards can be checked safely, then restored
before Screen_DeInit.

Text remains unclipped, as in the original API; these cases keep entire strings
inside the canvas. Only four-plane ST-low layouts and X aligned to 8 pixels are
supported. The mono printer changes plane zero only; the colour printer clears
each cell's background to colour zero.

Verified in Hatari STE: 58 cases, zero failures and no bus/address/illegal
exception. The library and existing `godlib.spl/font8x8` example were rebuilt.
