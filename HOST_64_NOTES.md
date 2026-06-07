# Host 64-bit GodLib Notes

These notes capture the state of the host-tool experiment around
`TOOLS.RG/BSBMAKER` and `TOOLS.RG/MASKMAKE`.

## Goal

Build old PureC GodLib tools as native host tools, using `godlib/libgod_host.a`.
Initial candidates:

- `TOOLS.RG/MASKMAKE/MAIN.C`
- `TOOLS.RG/BSBMAKER/MAIN.C`

The tools still use PureC-style includes such as:

```c
#include <GODLIB\FILE\FILE.H>
```

For the experiment the sources were transformed into temporary files under
`/tmp/godlib-tools-host`, lowercasing includes and converting backslash GodLib
paths to host include paths.

## Current Results

`MASKMAKE` builds and runs against the host library.

Test command used:

```sh
/tmp/godlib-tools-host/maskmake \
  TOOLS.RG/BSBMAKER/PRG/ROCKMASK.PI1 \
  /tmp/godlib-tools-host/rockmask_after.pi1
```

Observed output:

```text
< MASKMAKE >

Loading : TOOLS.RG/BSBMAKER/PRG/ROCKMASK.PI1
Saving : /tmp/godlib-tools-host/rockmask_after.pi1
exit=1
```

The generated PI1 was 32066 bytes. `exit=1` appears to be the original
success convention in `MASKMAKE/MAIN.C`.

`BSBMAKER` compiles, but does not yet run correctly on a 64-bit host.

ASan showed:

```text
heap-buffer-overflow in BsbMaker_SpritesBuild
allocated by Sprite_BlockCreate
```

The immediate cause is in `sprite/sprite.c`: `sSpriteBlock` contains a
native pointer array (`sSprite * mpSprite[]`), but several code paths allocate
or advance over that table as if each pointer/offset is 4 bytes:

```c
lSize += aSpriteCount << 2;
lpMem += apBlock->mHeader.mSpriteCount << 2;
```

That is correct for the Atari/file format where serialized offsets are `U32`,
but not for an in-memory work block on a 64-bit host where native pointers are
8 bytes.

## GodLib Changes Made During Experiment

`memory/memory.c`

- Host-side `Memory_Alloc()` now uses `malloc()`.
- Host-side `_Memory_Release()` now uses `free()`.
- Atari path still uses GemDos allocation/free.

`base/base.h`

- `sTagString` and `sTagValue` are temporarily packed with natural host
  alignment on GCC non-Atari builds.
- This avoids unaligned 64-bit pointer loads on arm64 hosts.
- Atari builds stay under the original packing.

Normal Atari build was checked afterwards:

```sh
make -C /Users/kriss/GCC.ELF/godlib clean
make -C /Users/kriss/GCC.ELF/godlib
```

The build completed successfully.

`git diff --check` was also clean. Git did warn that `base/base.h` and
`memory/memory.c` may be converted to CRLF if Git touches them, due to repo
line-ending settings.

## Important Design Rule

Do not confuse the in-memory host representation with the serialized Atari
file format.

Recommended split:

- In-memory work structures on host: use `sizeof(void *)` or `sizeof(field)`.
- Serialized Atari/file data: keep 32-bit `U32` offsets and Atari-compatible
  layout.

In other words:

```text
host allocation/table walking  -> native pointer size
file size/serialized offsets   -> 4-byte U32 offsets
```

This should preserve Atari compatibility while allowing native host tools.

## Likely Next Work

### Native TOOLBASE Direction

A useful long-term target is a native equivalent of:

```text
TOOLS.RG/BIN/WIN32/TOOLBASE/TOOLBASE.exe
```

This would let old asset batches run in a familiar form. A good reference
case is:

```text
godlib/achieve/gfx/ach.tbs
```

That script currently needs these commands:

- `MASKMAKE SRC\*.PI1 SRC\MASKS\*.PI1`
- `BSBMAKE ACH.SCR`
- `PALMAKE SRC\*.PI1 DST\*.PAL`
- `MAKELINK DST LINKFILE\ACHIEVE.LNK`
- `GODPACK LINKFILE\ACHIEVE.LNK`

Only the Windows and Atari `TOOLBASE` binaries were found; no source was found
in `TOOLS.RG`. Reimplementing a minimal native runner should be practical:

1. Read a `.tbs` file line by line.
2. Ignore blank lines and comment lines starting with `#`.
3. Split the remaining line into command plus arguments.
4. Normalize old Windows-style backslashes in paths to host separators.
5. Dispatch to native host tools by command name.

For the first version, external process dispatch is probably enough:

```text
MASKMAKE -> host maskmake binary
BSBMAKE  -> host bsbmaker binary (also accept BSBMAKER)
PALMAKE  -> host palmake binary
MAKELINK -> host makelink binary
GODPACK  -> host godpack binary
```

Later, it could be linked directly with tool entry points, but separate
executables are easier to build, debug, and replace one by one.

`ach.tbs` is a good acceptance test once the individual host tools are ready.
The pipeline should be run from `godlib/achieve/gfx`, and generated outputs in
`dst/` and `linkfile/` should be compared against existing files.

Start with `godlib/sprite/sprite.c`.

Functions to inspect/fix first:

- `Sprite_BlockCreate`
- `Sprite_BlockGetSize`
- `Sprite_BlockSerialise`
- `SpriteBlock_Rot90`
- `Sprite_BlockDelocate`
- `Sprite_BlockRelocate`

The tricky part is that some functions operate on live in-memory blocks, while
others produce or consume serialized blocks. The code currently uses one
`sSpriteBlock` shape for both.

Possible approach:

1. Add helper macros/functions for native sprite pointer table size.
2. Keep serialized block size calculations using 4-byte offsets.
3. Ensure `Sprite_BlockSerialise()` creates a 32-bit offset-compatible buffer.
4. Avoid writing native 8-byte pointers into serialized output.
5. Compare generated `ROCKANIM.BSB` against the existing sample after fixing.

Also inspect related code:

- `godlib/sprite/rsprite.c`
- `godlib/sprite/asprite.c`
- `godlib/font/font.c`

`rsprite.c` and `asprite.c` have similar pointer/offset patterns.

## Temporary Build Recipe Used

Example transform for a PureC tool source:

```sh
perl -pe 's#<GODLIB\\([^\\>]+)\\([^\\>]+)\.H>#<godlib/\L$1\E/\L$2\E.h>#g; s#<STDIO\.H>#<stdio.h>#g; s#<STRING\.H>#<string.h>#g; s#<MATH\.H>#<math.h>#g; s#^S16\s+main\(\s*S16\s+argc,#int main( int argc,#' \
  TOOLS.RG/MASKMAKE/MAIN.C > /tmp/godlib-tools-host/maskmake.c
```

Example compile/link:

```sh
cc -I. -Igodlib -c /tmp/godlib-tools-host/maskmake.c \
  -o /tmp/godlib-tools-host/maskmake.o

cc /tmp/godlib-tools-host/maskmake.o godlib/libgod_host.a -lm \
  -Wl,-no_fixup_chains \
  -o /tmp/godlib-tools-host/maskmake
```

On macOS/arm64, `-Wl,-no_fixup_chains` was useful while packed pointer data
still triggered linker alignment warnings.

## Do Not Forget

`TOOLS.RG` was already dirty during this experiment. Do not assume any status
there was produced by the host-tool test. The test did not intentionally edit
tool sources.

If testing `BSBMAKER` in `TOOLS.RG/BSBMAKER/PRG`, back up generated outputs
before running it. The sample script writes `ROCKANIM.BSB`.
