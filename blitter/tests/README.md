# Standalone blitter geometry verification

Run `make`, then `python3 verify-hatari.py` with the local hatari-debugging helper.
The test runs on Hatari STE and compares every destination word (including row
padding and guards) against independent pixel references. It covers masked,
opaque and coloured sprites at every fine alignment and top/bottom clipping;
boxes; copies with separate 192/336-byte source/destination strides; and the
original pointer APIs on 640x200 and 640x432 Screen pages, including Y=400.
Real hardware has not been tested.
