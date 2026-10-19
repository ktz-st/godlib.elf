# Example coverage and incomplete modules

Source review: 2026-10-19. Scope: the local GodLib Makefile, current libgod.a
exports, runtime C/ASM/headers and godlib.spl source/examples. Existing module
notes were used as a starting index, then the findings below were checked in
code. This is not a Hatari/physical-hardware certification of these modules and
does not establish that an API is unused by every game outside these repositories.
Later GUI, FE, Achieve, Input/Kernel and Asset/Linkfile integrations and their focused fixes are identified below.

## Examples worth adding

These are candidates to validate, not a promise that every legacy path works.
All new example executables should use `.PRG`.

| Priority | Example | Existing modules | What it would demonstrate / prerequisites |
| --- | --- | --- | --- |
| 1 | Native GUI and file selector — implemented | `gui`, `guifs`, `hashtree` | `godlib.spl/gui` now demonstrates windows, buttons, native text editing/sliders, mouse and GuiFS filtering/folders/scrolling. See its README and verification evidence; the GEM/AES example remains `fileselect`. |
| 2 | Scripted game menu — implemented | `fe`, `input`, `asset`, `hashtree` | `godlib.spl/fe` compiles MAIN/OPTIONS from text with host FEBUILD, demonstrates difficulty/volume, CALL actions, palette fades and a chime. Locks/wipes and the experimental JSON path are not exercised by this example. |
| 3 | Scores, profiles and achievements — implemented | `achieve` | `godlib.spl/achieve` demonstrates player names, shared scores, per-player stats/awards and explicit disk save/load into another arena. Core allocation/relocation/persistence/specs repairs have native regression coverage; old asset-driven UI and Clone/Combine are not exercised. |
| 4 | Packed asset loading — implemented | `asset`, `linkfile`, `packer` | `godlib.spl/asset` loads PI1/BFB/BSB through package clients from loose files, raw LNK and whole-GodPack LNK; demonstrates automatic relocation, shared clients, unload/reload, byte/hash comparisons and disk sizes. Uses host MAKELINK/GODPACK, not historical SPLMAKE. |
| 5 | Unified input and replay — implemented | `input`, `kernel` | `godlib.spl/input` runs a Kernel task with combined keyboard/joystick directions/fire, HELD/HIT/UNHIT, bounded RAM recording, KIP save/load and replay hash comparison. Mouse/pads and package transitions are outside this focused example; input replay alone does not reproduce unrecorded RNG or timing state. |
| 6 | Save settings with callbacks | `registry` | Change a setting, notify registered clients, save/reload a subtree. There is a unit test in `registry/test`, but no user-facing sample. |

A small clock/countdown/pause example is another inexpensive coverage improvement
(`clock`). A CLI example would need a library build with `dCLI`, not just a define
on the application's translation unit.

For graphics work, a wireframe 3D example is possible using the working planar
line renderer, but repairing/testing `vector` would be a separate first step.
There is no dedicated tile-map/maps module in the current runtime tree: a tile
example would add that renderer/logic rather than merely expose a hidden API.

## Missing coverage versus genuinely unused code

The original review found no direct public-API calls for the following major
runtime systems `registry`,
`clock`, `scrngrab`, `profiler` or `cli`. Some are used internally
or enabled only by build flags. In particular:

- `asset`, `hashtree`, `tokenise`, `lexer` and `reflect` support other GodLib
  systems; absence of a dedicated sample does not make them dead code.
- `registry` has its own unit test, while `elfhash` has no call site beyond its
  definition/declaration in the reviewed runtime/sample tree.
- `program` has a loader/executor but no consumer in the reviewed examples or
  other runtime modules. `Program_ArgvCreate/Destroy` are incomplete API surface.
- `vector` is assembled into the archive but has no sample and serious integration
  gaps. `maths` is not included by the current Makefile. HEAP is now available as an
  optional static-archive object, with its own correctness tests.
- `AsmSprite_*` and `RleSprite_*` have no sample users. They process word-per-pixel
  graphics, rather than the ST-low four-plane masked sprites used by our games.
  Do not present them as drop-in faster planar sprite backends.
- `thread` and `linklist` are header-only facilities: no object in libgod.a is not
  evidence that they are missing. Likewise init/deinit routines may legitimately
  be empty and relocator type callbacks may deliberately accept by extension.

A static archive containing an API is not proof that the API is linked into a
particular PRG. Symbol/link references and linker retention must be checked for
that program before proposing size savings or deleting code.

Cutscene background visibility and TEXT_ANIM_* are implemented now, with native
regression coverage in `godlib.spl/cutscene/analysis`. Background selection is
retained while hidden; typewriter timing, stable wrapping and final-page waits
use existing runtime fields.

HEAP is also complete as an optional arena: splitting, immediate coalescing,
statistics and native/ASan/UBSan regressions. See [HEAP.md](../memory/HEAP.md).
Ordinary Memory_Alloc/Free behavior is unchanged.

## Confirmed incomplete implementations and API gaps

| Area | Source evidence | Consequence |
| --- | --- | --- |
| Chunky drawing | `chunky/chunky.c`: DrawTri, DrawQuad, FontPrint and clipped tri/quad bodies only discard arguments | No drawing from these entry points, despite their names. Existing chunky conversion/drawing example exercises other functions. |
| 16bpp drawing | `graphic/grf_16_s.s`: Graphic_16BPP_DrawLine is only RTS; C fallback line/tri/quad functions in grf_16.c are empty | This backend is not a complete drawing API. The ST-low planar ASM line renderer is separate and works. |
| Compiled sprites | `sprite/asprit_s.s`: AsmSprite_DrawClip is only RTS | Clipped compiled-sprite drawing is absent. This is not the ordinary Sprite/BLiTTER path. |
| Vector normalisation | `vector/vector_s.s`: Vector_Normal indexes OneOver, but OneOver reserves only ds.l 1 | Not a usable reciprocal table; no initializer was found in the reviewed tree. |
| Vector C interface | `vector/vector.h`: missing sVector/sFVector/sFMatrix types, repeated apSrc parameter names in matrix prototypes | Including the header alone fails a GCC syntax check. |
| Floating point maths | `maths/fvec_s.s`, `maths/fmat_s.s`: FPU instructions, no public XDEF exports, unfinished BuildAxisAngle; absent from Makefile | Experimental source, not an available ST/STE 68000 math API. |
| Indexed image conversion | `pictypes/canvasic.c`: FromCanvas returns 0; From565 returns 1 without converting pixels | Dependent export/quantization paths are incomplete; success is not evidence of a produced image. |
| Image loading/saving | `pictypes/canvas.c`, `canvasic.c`: several Load/LoadAt/Save and palette functions are inside #if 0 | Header declarations do not imply these high-level file operations exist. |
| GIF/GSM/GFX | Gif_ToCanvas ignores input and only creates a canvas; Gsm_FromCanvas/ToCanvas return 0; Gfx_FromCanvas/ToCanvas lack definitions | These conversion paths cannot be advertised as working image loaders/exporters. Existing PI1/ILBM paths are separate. |
| Other tool paths | ColourQuantize_Octree returns 0; Tga_Pack returns 0 | These particular quantizer/encoder implementations are absent, without implying every colour-quantizer or TGA decoder is broken. |
| Unsigned sample gain | audio.h declares Audio_ScaleVolumeUnSigned; audio.c defines Audio_ScaleVolumeUnsigned with an empty body | Both a spelling mismatch and missing implementation. AMIXER per-voice gain is a separate working API. |
| Loader arguments | program.h declares Program_ArgvCreate/Destroy; neither is exported by current libgod.a and no implementation was found | Calling these APIs would leave unresolved symbols. |
| Disk-image deletion | drive/disk_io.h declares DiskImage_File_Delete; no implementation/export was found | Incomplete API, not evidence that directory enumeration or normal file IO is broken. |
| Lightweight CPU timer | profiler/profile.h defines Profile_CpuBegin/End as empty macros on Atari | Cannot use these macros as a numeric CPU meter. The separate HBL sampling profiler and external Hatari profiling are different systems. |

## Implemented systems with issues to check before examples

- **Reflect repaired after this review (2026-10-19):** field-name lookup,
  full-width integer conversion, F32 conversion/leaf flags and success reporting
  are fixed. Bounded numeric tokens, range errors and field extents are checked.
  JSON now keeps signs/decimal/exponent characters inside number tokens, and
  FED's swapped layout flags/count initializer is corrected. Native JSON/FED
  regressions are in `reflect/tests`. Other FED limitations below remain.
  This reflection/type-description layer is unrelated to the working water effect.
- FED: the JSON builder still has hard-coded layout decisions. The FED asset
  delocator returns success with Fed_Delocate commented out. Page drawing does
  not consume the page sprite-list field; random wipe selection is not completed.
  The menu runtime itself has substantial implemented navigation/rendering code.
- GUI/GuiFS: the window/widget/file-list system is substantial, not an empty
  shell. However, slider calculations can divide by data-dependent ranges,
  event-name formatting has fixed local buffers, and rebuilding file lists
  changes dynamically owned controls. Focus/de-focus slider hooks are empty;
  that alone does not mean held-slider updates are absent.
- Achieve machine metadata and persistence now have focused native checks in
  `godlib.spl/achieve/analysis`; this does not certify its old asset-driven UI.

## Verification performed for this review

- Read current source and compared the Makefile's source list with archive
  symbols and sample call sites. Comment/host/fallback-only bodies were checked
  in context rather than treating every empty body as a broken Atari function.
- Confirmed missing archive symbols for Audio_ScaleVolumeUnSigned,
  Program_ArgvCreate/Destroy, DiskImage_File_Delete and
  FMatrix_BuildIdentity. The differently spelled Audio_ScaleVolumeUnsigned,
  Vector_Normal and the empty drawing entry points do exist in the archive.
- GCC `-mshort -mfastcall -m68000 -I. -fsyntax-only` with only
  `<godlib/vector/vector.h>` included reports unknown vector/matrix types.
- No new runtime measurements or blanket claim of module correctness: a new
  example should add on-target checks for the exact paths it uses.
