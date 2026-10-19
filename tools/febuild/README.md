# FEBUILD host compiler

Compiles the Reservoir Gods FE text format into native GodLib FED v2 menu data.
It runs on macOS/Linux with Python 3 and the standard library. No emulator,
GUIBUILD, PureC, cross compiler or native host GodLib build is needed to compile
a menu. It is a new host implementation, not a binary build of the old TTP.

```sh
make -C godlib/tools/febuild
godlib/bin/Darwin/arm64/febuild menu.txt MENU.FED
python3 godlib/tools/febuild/febuild.py menu.txt MENU.FED
```

TOOLBASE also accepts `FEBUILD source.txt destination.fed`, including quoted
paths. `make -C godlib/tools` installs the launcher with the other host tools.

## Source format

Uses the original tokeniser's chunk spelling, one header/member/brace per line:

```text
<TEXT name=MAIN_TITLE>
{
    STRING = "GAME MENU"
    X = 0
    Y = 24
    WIDTH = 320
    HEIGHT = 8
    ALIGN = CENTRE
    FONT_GROUP = MENU_FONTS
}
<PAGE name=MAIN>
{
    TITLE = MAIN_TITLE
    PAGE_STYLE = MAIN_STYLE
    CONTROL_LIST = MAIN_CONTROLS
}
```

Forward references are supported. Object names/references are case-insensitive;
`NULL`, `NONE` and `0` encode null references. A `[TYPE IDENTIFIER]` header is
also accepted as a convenience. Comments use `#`, or `//` on their own line.
Quoted values retain literal backslashes. Strings are supplied as UTF-8 source
text and serialized as NUL-terminated Latin-1 byte strings; identifiers use ASCII.
There is no automatic Unicode-to-Atari font character-map conversion.

Supported chunks: ASSET, CALL, CONTROL, CONTROLLIST, FONTGROUP, LIST, LISTITEM,
LOCK, PAGE, PAGESTYLE, SAMPLE, SLIDER, SPRITE, SPRITEGROUP, SPRITELIST, TEXT,
TRANSITION and VAR. Their member names match `fe/fedparse.c`. Repeated CONTROL,
ITEM and SPRITE members build pointer arrays in the corresponding list chunks.
Enums include LINK/CALL/LIST/SLIDER, LEFT/RIGHT/CENTRE, comparison operators,
fade NONE/BG/PAL/RGB and wipe NONE/RANDOM/SPECIFIC. Runtime support is separate:
for example, FE's page sprite-list renderer and random wipe selection remain
incomplete. Namespace directives and legacy FONT/IMAGE/LINK aliases are not
implemented; unsupported syntax fails explicitly.

The [FE example](../../../godlib.spl/fe/menu.txt) shows two complete pages,
list selection, a slider, CALL actions, styles, fonts, cursor and fade transitions.
The compiler rejects unknown members/enums/references, duplicate object hashes,
duplicate scalar members, missing braces, integer overflow, empty lists and
invalid slider ranges. It is not a complete semantic verifier for every possible
combination of optional runtime data; assets and bound variables must still be
provided by the application.

## Format and compatibility

The output is a single writable block containing the 116-byte Atari sFedHeader,
all 18 object arrays, pointer arrays and strings. All numbers are big-endian,
pointers are 32-bit offsets from the FED header, and runtime state starts zeroed.
This uses the existing v2 `Fed_Relocate()` path, rather than the old parser's
v0 index references. Relocate before `Fed_Init()`; call `Fed_DeInit()` before
freeing/unloading the block. Binary files do not embed their referenced assets.

`schema.json` records the **actual Atari ABI**, including runtime asset/variable
client fields. `generate_schema.py` reads member metadata from the C parser and
extracts sizes/offsets from a GCC ELF object built against the current headers.
Only regeneration needs the cross compiler. Run `make schema` after changing FE
structures, review the schema, and rerun the target tests. Host pointer sizes
are never used to serialize files.

This work fixes the v2 slider variable delocation, double sprite-list array
delocation and U16 fade-colour serialization. It also fixes FE intro completion
before transition startup, page cursor selection/null cursor handling and the
selection-index boundary. Optional page-style sample pointers are checked.
AssetClient teardown now releases its registered context reference.

## Verification

```sh
make -C godlib/tools/febuild check
make -C godlib/tools/febuild check-native
make -C godlib.spl/fe check
```

Host tests verify references, independent binary field offsets, strings, signed
values, enums, errors, deterministic output and TOOLBASE integration. The 68000
fixture covers **all 18 object types**, including nested pointer arrays, slider
variables and a nonzero BG palette index; four relocate/delocate cycles must
return byte-for-byte identical data. That fixture is layout-only and deliberately
uses dummy assets without initializing/rendering them.

The actual FE example verifies native menu input, list wrap, held slider repeat,
endpoint clamps, repeated page transitions, CALL actions, persistence and clean
teardown. Both Hatari targets use an 8 MHz 68000 STE, 1 MiB and German TOS 1.62.
This is emulator evidence, not a physical-machine test.
