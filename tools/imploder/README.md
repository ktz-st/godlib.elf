# Native Imploder

Portable host build of hitchhikr's Imploder v1.4b, vendored from the local
`GCC.ELF/imploder` checkout at `5b3438766a9fd887752b2c6f4b5540f3ee859207`
([upstream](https://github.com/hitchhikr/imploder)). BSD-2-Clause: see [LICENSE](LICENSE).
The algorithm was reversed by Vladimir Kononovich (lab313ru); the upstream
Amiga parser derives from emoon/AmigaHunkParser.

```sh
make -C godlib/tools imploder
godlib/bin/Darwin/arm64/imploder -11 picture.pi1 picture.imp
godlib/bin/Darwin/arm64/imploder picture.imp picture.restored.pi1
```

Output goes to `godlib/bin/<uname -s>/<uname -m>/imploder`, including Linux.
Only a native C compiler and make are needed: the upstream Amiga/X68000
depacker headers are already generated. Neither Wine nor vasm is needed to
build this host tool. The macOS arm64 build is verified locally.

Modes `-0` through `-11` select compression effort; default is 11. Data input
is compressed; an existing Imploder stream is automatically decompressed.
Without an output name, `.imp` / `.exp` is appended. Use explicit output names
in build scripts. The upstream tool refuses files below 64 bytes, streams that
do not save enough space, and certain extremely short compressed streams;
failure returns nonzero and does not produce a packed file. There is no silent
raw-data fallback. Data compression does not make an executable TOS PRG.
Upstream Amiga/X68000 executable packing is retained, but is separate from the
GodLib `IMP!` data path and was not tested here.

Local portability/correctness changes: native host Makefile and endian detection,
bounded CLI filenames and validated modes, input-length/checksum checks before
host decompression, safe realloc failure cleanup, unsigned 32-bit header shifts,
bounds checked before compressor lookahead reads, and X68000 output close/return.
Legacy compiler warnings in the parsers were also cleaned up.

## Tests

```sh
make -C godlib/tools/imploder test
make -C godlib/tools/imploder native
# Optional local original-game fixtures (not vendored):
make -C godlib/tools/imploder native MICRO_ROOT=/path/to/MICRO/build/amiga
```

`native` requires an updated `godlib/libgod.a`, GCC ELF and sibling libcmini.
It creates fixtures and `/tmp/godlib-imploder/IMPTEST.PRG` (override `NATIVE_DIR`).
Run that PRG with its files beside it under Hatari STE / 68000 / 4 MB and
German TOS 1.62. `NATIVE.TXT` must report `cases=72 failures=0`, or the larger
count when optional MICRO fixtures are supplied.

The host suite compares all 12 modes byte-for-byte, exercises odd/even original
sizes and files larger than 64 KiB, and rejects invalid arguments, truncated
files, bad checksums and uncompressible input. It also builds a compressed LNK.
The native test checks in-place and separate-buffer decode, generic Packer
dispatch and size queries, output guards, invalid lengths/capacities/alignment,
overlap and checksum failures, unaligned read-only source, checksum wraparound,
and whole-archive `LinkFile_InitToRAM()` loading.
On 2026-10-22, all 72 generated cases and all 82 original MICRO fixtures passed
on Hatari STE with German TOS 1.62; host ASan/UBSan round trips also passed.

The runtime ASM now uses the original MICRO/FRT decoder (364 bytes, with A5
preservation for GCC), plus a 50-byte assembly checksum routine. This is
independent of the host compressor source above.

See [runtime API and format](../../packer/imploder.md).
