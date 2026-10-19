# GodLib AssetView for macOS

A native Cocoa BSB/BFB viewer and ANIM8R script player. The application is
Objective-C compiled by Clang; it uses system frameworks only (no SDL, Python,
web server or emulator at runtime). Host BSBMAKER remains a Python 3 build tool.

## Build / launch

```sh
make -C godlib/tools/assetview
open 'godlib/bin/Darwin/arm64/GodLib AssetView.app'
godlib/bin/Darwin/arm64/assetview path/to/sprites.bsb
```

The architecture folder follows `uname -m`; on Intel Macs use `x86_64`.
`HOST_BIN_DIR` can override the install location. The app bundle is self-contained
and can be copied elsewhere. Its adjacent CLI launcher needs to stay beside it.
The app is locally ad-hoc signed, not notarized for third-party distribution.
`make -C godlib/tools` also installs the viewer on macOS and skips it on Linux.

Opening without a filename shows a bundled 32-frame arrow rotation. Its script,
PI1 sources, palette and ANIM8R equivalent are in `demo/`; `prepare_demo.py`
regenerates them from source, without image-library dependencies.

## Controls

- **Open / Cmd+O:** BSB, BFB or ANIM8R text script; files may also be dropped
  onto the canvas. Finder can open BSB/BFB with this application.
- **Sheet:** all stored frames/glyphs, with frame numbers and BFB character
  codes. Use scrollbars for larger sheets. Turn Sheet off for a single frame.
- **Play / Space:** play/pause; Play switches to the single-frame view.
- **Left/Right arrows:** step through the playback sequence. Buttons do the same.
- **FPS:** playback rate, initially 10 for BSB/BFB or `50/TIME` for ANIM8R.
- **Mask:** white means sprite coverage; black means preserve background.
  Normal view shows transparency over a checker, including opaque colour-zero
  pixels when the mask allows them.
- **Zoom:** integer nearest-neighbour scaling from 1x to 8x.
- **Palette / Cmd+P:** load a 32-byte Atari PAL or palette from a low-resolution
  uncompressed PI1. A matching basename PAL/PI1 loads automatically for BSB/BFB;
  otherwise the status explicitly reports the default grayscale palette.
  Atari STE RGB444 channel bit order is decoded correctly (ST RGB333 uses its
  even-level subset). BSB/BFB do not themselves contain palette data.
- **BFB text field:** type a sample to preview with the font's glyph map,
  proportional regions, space width and kerning. The input maps to byte
  characters via ISO-8859-1; glyph labels are raw byte codes, not an assertion
  about the font's character encoding.
- **Export PNG / Cmd+S:** save the current frame at original resolution, with
  alpha transparency, or the black/white mask when Mask is selected. In Sheet
  mode the current playback frame is exported, not the entire sheet.

## ANIM8R compatibility

Supports `[GRAPHIC]`, `[WIDTH]`, `[HEIGHT]`, `[FRAMECOUNT]`, `[FRAMES]`, `[TIME]`,
and `[EOF]`, with one parameter per line as in RG scripts. Frame IDs are zero
based and may repeat or run in any order. Playback wraps after the smaller of
FRAMECOUNT and the number of FRAMES entries, matching the original player.
TIME is a count of nominal PAL 50 Hz VBLs, not milliseconds; host scheduling is
an NSTimer and is not an emulation of Atari timing.

The PI1 filename is relative to the script, accepts Atari backslashes and a
case-insensitive final filename. Source frames follow the original word-aligned
PI1 sheet layout. `[X]` and `[Y]` remain ignored because the original marked them
unsupported. PI1 pixels are displayed opaquely in this mode: ANIM8R is a sheet
player, not a BSB mask generator. Use BSB for an explicit sprite mask.
Compressed PC1, packed BSB/BFB and more than four graphics planes are unsupported.

Reads current `BSBK` / `TNOF` and historical RG `KBSB` / `FONT` signatures.
Offsets, dimensions, versions, glyph regions, character maps and input lengths
are checked before decoding. No host pointer relocation or Atari code runs.

## Command line and verification

```sh
assetview --inspect sprites.bsb
assetview --export sprites.bsb frame.png colours.pal 3
make -C godlib/tools/toolbase check
make -C godlib/tools/assetview check
```

`--export` frame indices refer to stored frames, not ANIM8R sequence positions.
Tests use Python 3 and Pillow (only for checking exported PNGs). They cover
independently constructed BSB/BFB fixtures and historical signatures, exact
pixels/alpha, STE palette decoding, ANIM8R sequence/rate and PI1 extraction,
truncated/invalid files and existing repository assets when present.
