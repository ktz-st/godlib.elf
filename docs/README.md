# GodLib documentation

- [Missing Link REFLECT](reflection.md): native ASM water reflection and original-code verification.
- [Example coverage and module audit](module-audit.md): 2026-10-19 review,
  suggested examples, unused versus unexercised APIs and confirmed incomplete paths.
- [Module notes](module-notes.md): source survey, integration details and known
  limitations across the library.
- [Cutscene format](cutscene.txt): legacy text DSL, corrected field names,
  complete parser signatures, AMIXER samples and current runtime limitations.
- [Narrated cutscene example](../../godlib.spl/cutscene/README.md): two scenes,
  spoken PCM, caption/background fixes and native ST/STE regression tests.
- [8x8 ASM text scrollers](../../godlib.spl/scroll8/README.md): MCoder versus
  optimized Griff, cache layouts, CPU measurements and integration constraints.
- [Library overview and build](../readme.md).
- [MOD oscilloscopes](../music/modscope.md): optional four-channel source
  previews, ICE XY and stereo/mono modes, ASM drawing and verification.
- [Wizzcat](../music/wizzcat.md): automatic MOD workspace sizing, faster
  initialization, four-channel activity and regression evidence.
- [LeonardMod](../music/leonardmod.md): ProTracker replay, 12.5/25 kHz, dynamic
  ST-RAM buffers, memory accounting and verification.
- [PCM mixer guide](../audio/pcm-mixers.md): backend selection, rates, loops,
  arithmetic, migration and links to individual mixer manuals.

The 2026-10-16 documentation audit checked named function references against the
local source tree and reviewed recent graphics, scrolling, sprite, font, chunky
and audio changes, plus cutscene parser/runtime support. The notes also retain
older observations about other modules; they are not a claim that every path
was freshly tested. Test counts and CPU/timing figures belong to the linked
test artifacts and their stated configurations. Source review and Hatari tests
do not establish behavior on physical Atari hardware.

Important constraints include matching source/destination layouts for
`Graphic_ChunkList` background restore. Cutscene background visibility and
TEXT_ANIM_* now work, including final rendering waits for both display pages.
The local [CUTBUILD](../tools/cutbuild/README.md) compiler now supports host-side
cutscene builds and TOOLBASE batches. SPLMAKE remains unavailable locally.

The optional [HEAP allocator](../memory/HEAP.md) provides independent arenas,
immediate free-block coalescing, statistics and native/host correctness tests.
