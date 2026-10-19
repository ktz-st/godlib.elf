# CUTBUILD

Compile GodLib cutscene text to a delocated `.CUT` v3 file on macOS/Linux.
Requires Python 3.9+ and its standard library. No emulator or cross compiler is
needed to compile scenes. The script uses the neighboring GodLib command table
and opcode enum; keep it inside this source tree.

From the GodLib directory:

```sh
python3 tools/cutbuild/cutbuild.py source.txt destination.cut
python3 tools/toolbase/toolbase.py CUTBUILD source.txt destination.cut
make -C tools/cutbuild
bin/Darwin/arm64/cutbuild source.txt destination.cut  # platform path on this Mac
```

The last command is a Python launcher; `make` installs it under
`bin/$(uname -s)/$(uname -m)`, also on Linux. `make -C tools` includes it.
TOOLBASE `.TBS` batches can contain `CUTBUILD "source file.txt" "output file.cut"`;
paths in a batch are relative to that batch's directory. Direct CLI paths are
relative to the current working directory.

## Input and output

Uses the syntax in [the cutscene manual](../../docs/cutscene.txt): named ANIMATION,
ASSET, PAGE, SAMPLE, SCRIPT, SPRITE, TEXT and VAR chunks; braces on their own lines
(or an opening brace after the chunk header), FIELD = value and one script
command per line. Names and references are case insensitive. Forward references
work. Unknown chunks/fields/commands, duplicate names within a type, duplicate
fields, unresolved explicit references, wrong argument counts, malformed values
and numeric overflow produce an error with a source line and nonzero exit status.
A failed compilation preserves the previous output file.

Every current runtime opcode is supported. The opcode/argument definitions and
version come from `cutscene/cutscene.c` / `.h`. Each script gets an implicit
`RETURN 0`, including scripts that already contain RETURN, matching the native
parser. Optional omitted fields remain zero, including absent references. The
legacy spelling is `STEROPOS`. Integers accept decimal, `$hex` and `0xhex`;
16.16 speeds reproduce the native FP32 integer/fraction conversion (including
its historical bitwise OR for negative fractions).

Outside PAGE text, `;`, `#` and `//` introduce comments outside quoted strings.
Backslashes in quoted paths are literal. Input is legacy single-byte text, read
without newline conversion: PAGE bytes and CR/LF/CRLF are retained, except
leading blank PAGE lines as in the native parser. Convert Unicode text to the
font's encoding before compilation; this tool does not transcode UTF-8.

Output has explicit Atari 32-bit relative offsets, big-endian values, two-byte
structure alignment and zeroed runtime/client fields. It is independent of host
pointer size/endianness. Arrays/strings may have a different order from the old
TOOLBASE output; references resolve to the same data. Assets are named references,
not embedded picture/sample payloads.

On Atari, load/copy into writable storage and call `CutScene_Relocate()` once.
Bind/load assets, then use `CutScene_System_ScriptInit()` and Update/Render.
Never relocate a native `CutScene_Parse()` result or an already relocated CUT.
Stop script/audio before releasing storage. For an embedded CUT, keep it in the
writable data section and do not pass its address to Memory_Free.

`godlib.spl/cutscene` builds and embeds `assets/SCENE.CUT`; the target no longer
needs the text parser or its temporary allocations.

## Verification

```sh
make -C tools/cutbuild check
make -C tools/cutbuild check-native  # cross compiler, vasm and Hatari required
```

Host tests cover all commands/chunks, errors, byte preservation, forward
references, CLI/TBS paths with spaces, atomic failure and deterministic output.
The native harness checks actual Atari ABI sizes and compares every chunk,
command and typed reference against `CutScene_Parse()` on a 68000, then verifies
an exact delocate/relocate round trip. `tests/verification.json` records 400 checks
on STE / 8 MHz / 1 MiB / German TOS 1.62.

`tests/original.cut` was generated from `tests/fixture.txt` by the original
`GOD/TOOLS.RG/BIN/ATARI/TOOLBASE/TOOLBASE.TTP`, run under Hatari with German TOS
1.62 and 4 MiB. Its batch contained `CUTBUILD FIXTURE.TXT OLD.CUT`.
`tests/original.json` records binary/source/output hashes and tool provenance.
The host regression compares its complete semantic contents against our output;
both files are 929 bytes. This verifies the fixture, not every historical dialect
or unknown future CUT version.

The narrated example's `make check` independently checks actual CUT playback,
complete frames, both display pages, typewriter captions, audio and keyboard
lifecycle on ST and STE. Host tests were run on macOS; Linux portability follows
from the standard-library-only implementation and portable launcher, but Linux
execution has not been tested in this workspace.
