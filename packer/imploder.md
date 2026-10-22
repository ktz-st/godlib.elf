# Imploder data support

`packer/imploder.c`, `imploder.h` and `imploder_s.s` add `IMP!` data streams to
the normal GodLib Packer API. The 362-byte 68000 decoder is adapted to GCC ELF
`-mfastcall` from `imploder/explode_68000.s`; MICRO uses the same algorithm via
an embedded original depacker. The GodLib implementation is readable vasm
source, without an external game-binary dependency. Upstream provenance and
BSD-2-Clause terms are in [tools/imploder](../tools/imploder/README.md).

```c
#include <godlib/packer/imploder.h>

/* packed has packedBytes readable bytes; output has outputBytes capacity. */
if (!Imploder_DepackChecked(packed, packedBytes, output, outputBytes))
    return; /* invalid envelope, checksum, buffers or decode result */
```

- `Imploder_IsPacked()`, `Imploder_GetDepackSize()` and
  `Imploder_GetPackedSize()` inspect at least 12 readable header bytes.
- `Imploder_Depack(src, dst)` infers both sizes from the header and returns
  1 on success, 0 on failure. Use the checked entry point with real file sizes
  when available; the inferred-size API cannot detect truncated allocations.
- `Imploder_DepackChecked(src, srcBytes, dst, dstCapacity)` additionally requires
  the exact packed file length and sufficient output capacity, validates the
  original checksum, and rejects misaligned destination and partial overlap
  before writing output. It still trusts the compressed tokens, as the original
  ASM does; it is intended for generated/trusted assets, not hostile streams.

Destination must be even-aligned and have at least the unpacked size in bytes.
`src == dst` works with no heap allocation, padding or temporary data buffer.
Separate buffers must not overlap. In that case packed bytes are copied into
the destination before in-place decoding, so the original source is unchanged
and may be read-only. Source alignment is unrestricted in the C wrapper.
The ASM core uses 72 bytes of saved registers/local stack plus the return
address; the C wrapper adds its normal call frame. No mutable codec globals.

## Generic loading

`Packer_GetType()` returns `ePACKER_IMPLODER` (appended after RNC, preserving
the old enum values). `Packer_GetDepackSize()` returns the original length,
`Packer_GetHeaderSize()` returns 12, and `Packer_GetLoadOffset()` returns 0.
`Packer_Depack(src, dst)` calls the validated Imploder wrapper and honors `dst`.
Like the existing generic API, it has no return status; call Imploder directly
when the caller needs to handle an error.

Whole `IMP!`-compressed LNK files work with `LinkFile_InitToRAM()` through the
existing allocation/dispatch path. Explicit member depacking in file-backed
linkfiles also goes through Packer; keep packed/unpacked sizes accurate.
`File_Load()` itself still returns raw file bytes; it does not automatically
decompress loose files. Allocate the output separately and call the API above,
or use an unpacked-size buffer and load/depack in place.

## Format

All multibyte fields are big-endian. Header: `IMP!` at 0, unpacked size at 4,
even trailer offset at 8. The trailer starts at that offset, restores the first
12 compressed bytes overwritten by the header, and contains the initial literal
count, bit token, eight base offsets, twelve extra-bit counts and a checksum.
Complete packed length is **trailer offset + 50**, including the last four
checksum bytes. The checksum is 7 plus the unsigned sum of the preceding
big-endian words, modulo 2^32. The header is 12 bytes, not 50.

Only the canonical `IMP!` signature is dispatched. Upstream host decompression
also recognizes historical clone signatures; those are not part of this API.
Amiga/X68000 executable envelopes and TOS executable packing are separate.
See the [tool tests](../tools/imploder/README.md#tests) for reproducible target
verification, including original MICRO assets.
