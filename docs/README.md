# GodLib documentation

- [Module notes](module-notes.md): source survey, integration details and known
  limitations across the library.
- [Cutscene format](cutscene.txt): legacy text DSL, corrected field names,
  complete parser command signatures and current runtime limitations.
- [Library overview and build](../readme.md).
- [PCM mixer guide](../audio/pcm-mixers.md): backend selection, rates, loops,
  arithmetic, migration and links to individual mixer manuals.

The 2026-10-16 documentation audit checked named function references against the
local source tree and reviewed recent graphics, scrolling, sprite, font, chunky
and audio changes, plus cutscene parser/runtime support. The notes also retain
older observations about other modules; they are not a claim that every path
was freshly tested. Test counts and CPU/timing figures belong to the linked
test artifacts and their stated configurations. Source review and Hatari tests
do not establish behavior on physical Atari hardware.

Important constraints called out in the notes include matching source/destination layouts for `Graphic_ChunkList` background restore,
and parsed cutscene sample/text-animation commands that remain unimplemented.
The original TOOLBASE CUTBUILD/SPLMAKE commands are not implemented by the local
Python tool subset.
