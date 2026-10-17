# LanceMod allocation/PCM regression test

From the GCC.ELF workspace:

```
python3 godlib/music/tests/lancemod-memory/run.py
python3 godlib/music/tests/lancemod-memory/run.py --baseline /path/to/pre-change/music
```

The baseline directory contains `lancemod.c`, `lancemod.h`, `lancemod_s.s`,
`lancepaula.s` and `lancetracker.s` before the memory changes. Its undersized
50 kHz static code reservation is increased to 151114 bytes in the test build;
its loader, tables, sample guards and mixing logic are otherwise unchanged.

Requires the local GCC ELF/vasm/libcmini toolchain, Hatari skill helper and German
TOS 1.62 at the path used by `run.py`. The five real MOD inputs are read from the
sibling workspace projects; original source files are never modified. Two more
fixtures cover empty slots, sparse sample indices, short/odd-length samples,
loop shadows and EF1. A 1 MiB STE is sufficient for all cases.

Every replay allocation receives 16-byte guards on each side. Guards are checked
on every mix and free. The test verifies exact allocation sizes, generated code
sizes, failed allocations, malformed input rejection, repeated init/shutdown,
frequency changes and stopping active DMA before freeing its buffer.

Each module/rate generates 1000 mixer frames. Both DMA pages are captured in
order. With a baseline, Python compares the complete PCM captures byte for byte;
`results.json` also records per-case SHA256 hashes, heap counts and generated
code sizes. The current 21-case comparison covers 49 MB of PCM per implementation
and 42000 total mixer calls. This deterministic core comparison does not measure
CPU cost or replace the sample's live VBL/DMA integration test.

Build products, fixture copies, logs and PCM captures are ignored under `build/`.
