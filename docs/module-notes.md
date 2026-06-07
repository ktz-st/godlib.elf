# GodLib Module Notes

These notes collect quick findings about GodLib modules while exploring the tree.
Each future discovery can either extend this file or get its own focused `.md`
file in this directory.

## achieve

`godlib/achieve` is an offline achievement/profile/high-score system rather than
a sample.

Main parts:

- `ach_main.*` - core data model: users, stats, score tables, tasks, unlock state,
  save/load and relocate/delocate support.
- `ach_disp.*`, `ach_show.*`, `ach_sign.*`, `ach_logn.*`, `ach_unlk.*` - UI flows
  for sign-in, login, score/stat/task display and newly unlocked awards.
- `ach_gfx.*` - achievement UI drawing, palettes, fades, text and cursor rendering.
- `ach_god.*` - gathers machine/TOS/RAM/CPU/FPU/drive specs for achievement data.
- `ach_com.*` - compiler/decompiler entry points for achievement data.
- `gfx/` - TOOLBASE asset pipeline plus generated `.BSB`, `.BFB` and `.PAL` files.

The module is linked by `godlib/Makefile`, so it is part of `libgod.a`.

Noted oddity: `ach_god.c` builds a local `sAchieveSpecs`, but appears not to call
`Achieve_Specs_Update(&lSpecs)` after filling it.

## cli

`godlib/cli` is a small in-game/debug command console.

It is controlled by `dCLI`:

- with `dCLI`, `cli.c` provides a runtime console with command registration,
  input editing, history, tab completion, rendering and `AUTOEXEC.CLI` processing;
- without `dCLI`, `cli.h` turns the public API into no-op macros.

`godlib/kernel/kernel.c` registers built-in commands such as:

- `achunlock`
- `ass`
- `assunused`
- `build`
- `inp`
- `mem`
- `quit`
- `sys`
- `vid`

Other systems use `Cli_PrintLine` or the `Cli_PrintfLine*` helpers for optional
debug output.

## reflect

`godlib/reflect` is a very small reflection/type-description layer used to map
text data into C structs.

It defines:

- `sReflectType` - type name, byte size, flags and field list;
- `sReflectElement` - field type, field name and offset;
- `sReflectDictionary` - a collection of reflected types.

`reflect.c` includes fundamental types such as `S8`, `S16`, `S32`, `U8`, `U16`,
`U32`, `F32` and `string`.

Known users:

- `godlib/lexer/json.c` uses `Reflect_SetData()` in `JSON_ElementsToStruct()`;
- `godlib/fe/fed_json.c` defines a reflected `sFedJSON_Layout`.

Noted oddities:

- `Reflect_GetpData()` compares the requested element name with `mpTypeName`;
  it probably should compare against `mpElementName`.
- `Reflect_SetData()` checks `2 == mSizeBytes` twice for signed and unsigned
  integer writes, so 32-bit `S32`/`U32` fields may not be written.

## elfhash

`godlib/elfhash` contains one helper:

```c
U32 ElfHash_BuildHash( const char * apString );
```

It implements a classic ELF/System V style string hash, with ASCII uppercase
letters folded to lowercase first. This makes the result case-insensitive for
simple ASCII names.

It is linked by `godlib/Makefile`, but no direct in-tree users were found during
this pass.

## docs and cutscene

`godlib/docs` currently contains the original `cutscene.txt`, which documents the
GodLib cutscene system.

`godlib/cutscene` is a text-DSL-to-runtime cutscene system:

- `cutscene.*` - binary `.CUT` structures, command definitions and
  relocate/delocate support;
- `cutparse.*` - parser/compiler side for text cutscene files;
- `cut_sys.*` - runtime player for scripts, backgrounds, sprites, text, fades,
  samples and callbacks;
- `rel_cut.*` - asset relocator for `.CUT` files.

The documented pipeline is:

```text
CUTBUILD SourceTextFile DestBinFile
```

Cutscene data chunks include:

- `ANIMATION`
- `ASSET`
- `PAGE`
- `SAMPLE`
- `SPRITE`
- `TEXT`
- `VAR`
- `SCRIPT`

Runtime commands include background control, fades, samples, sprite animation and
movement, text display/animation, variable writes, callbacks and waits.

Integration notes:

- `godlib/Makefile` links the cutscene module.
- `platform.c` calls `CutScene_System_AppInit()` only when
  `dGODLIB_CUTSCENE` is enabled.
- `Relocator_CUT_Init()` is used when `dGODLIB_PACKAGEMANGER` is enabled.

Potential future idea: the `niezwyciezony` intro could be modeled as a cutscene
because it uses backgrounds, sprites, text timing, fades and scripted movement.
Streaming audio would probably remain custom or be driven via callbacks.

## clock

`godlib/clock` is an optional VBL-driven timer/clock module.

Main files:

- `clock.h` - public `sTime`/`sClock` types and timer API.
- `clock.c` - setup, time conversion, start/stop/pause/update logic and a
  non-Atari `Clock_TimeVbl()` fallback.
- `clock_s.s` - Atari VBL-time update routine in assembly.

The module tracks two related notions of time:

- `sTime`, with hours/minutes/seconds plus a small fractional field named
  `mMicroSeconds`;
- `gClockTicks`, an absolute tick counter normalized around 200 ticks/second.

Despite the name, `mMicroSeconds` is not real microseconds. Most conversion code
treats it as a 1/200 second sub-second counter.

Important APIs:

- `Clock_Init()` / `Clock_DeInit()` register and unregister a VBL callback.
- `Clock_Start()` / `Clock_Stop()` start and stop one `sClock`.
- `Clock_Pause()` / `Clock_UnPause()` preserve elapsed time across pauses.
- `Clock_Update()` updates elapsed time and countdown state for one clock.
- `Time_ToU32()` / `Time_FromU32()` convert between `sTime` and 200 Hz ticks.
- `Time_Add()` / `Time_Sub()` perform normalized time arithmetic.
- `Time_GetAbsTime()` returns a GEMDOS-like packed date/time value based on
  `gmtime()`.

Integration:

- `platform.c` calls `Clock_Init()` and `Clock_DeInit()` only when
  `dGODLIB_CLOCK` is enabled.
- `kernel.c` owns an array of `sClock` instances and calls `Clock_Update()` for
  them during `Kernel_Clocks_Update()`.
- `Kernel_ClockStartCountDown()` sets up a countdown timer and starts it.
- `godlib/Makefile` includes both `clock/clock.c` and `clock/clock_s.s`.

Fixes/notes:

- `clock_s.s` used to export `Clock_TimeVblt` while `clock.c` referenced
  `Clock_TimeVbl`. The assembler label/export has been renamed to
  `Clock_TimeVbl` so the Atari VBL callback links correctly when
  `dGODLIB_CLOCK` is enabled.
- The non-Atari `Clock_TimeVbl()` fallback increments `gClockTime`, but does not
  update `gClockTicks`, unlike the assembler path.
- `Clock_Init()` computes `gClockTickAdd`/`gClockSubTickAdd` from refresh rate so
  VBL updates can accumulate a 200 Hz tick base even on 50/60 Hz displays.

## debug

`godlib/debug` contains two related but different pieces:

- `debug.h` / `debug.c` - a very small compile-time debug switch;
- `dbgchan.h` / `dbgchan.c` - named debug output channels.

`debug.h` is controlled by `dDEBUG`:

- with `dDEBUG`, `Debug_Action(action)` expands to `action`;
- without `dDEBUG`, `Debug_Action(action)` compiles away.

`debug.c` currently contains only a declaration for `nothinghere()` and no real
runtime logic. It exists mostly as a placeholder source file for the module.

`dbgchan` is the useful part. It defines debug channels:

- `eDEBUGCHANNEL_ASSET`
- `eDEBUGCHANNEL_GAME`
- `eDEBUGCHANNEL_GODLIB`
- `eDEBUGCHANNEL_MEMORY`
- `eDEBUGCHANNEL_TOOL`
- `eDEBUGCHANNEL_USER`

Each channel can write to a combination of destinations:

- CLI console (`eDEBUGCHANNEL_DEST_CLI`)
- file (`eDEBUGCHANNEL_DEST_FILE`)
- screen/stdout (`eDEBUGCHANNEL_DEST_SCREEN`)
- Steem debug hook (`eDEBUGCHANNEL_DEST_STEEM`)
- Hatari BIOS intercept (`eDEBUGCHANNEL_DEST_HATARI`)

Public macros include:

- `DebugChannel_Printf0()` through `DebugChannel_Printf5()`;
- `DebugChannel_StringAdd()`;
- `DebugChannel_Action()`;
- `DebugChannel_Activate()` / `DebugChannel_DeActivate()`.

Like `Debug_Action`, the channel print macros compile away unless `dDEBUG` is
defined.

Runtime behavior:

- `DebugChannel_AppInit()` initializes channel state.
- `DebugChannel_Activate()` enables a channel and optionally opens a log file.
- Log files are created under `LOGS\`, for example `LOGS\ASSET.LOG` and
  `LOGS\MEMORY.LOG`.
- CLI output uses `Cli_PrintLine()`.
- screen output uses `printf()` and, on Windows builds, `OutputDebugString()`.
- Steem output writes the string pointer to address `0xFFFFC1F0`.
- Hatari output calls `Xbios_Dbmsg(5, 0xF000, string)`, matching SGDL's
  `HATARI_PRINT` helper. Hatari needs to be started with BIOS intercept enabled,
  for example `--bios-intercept true`, for this destination to print.

Integration:

- `godlib/Makefile` includes both `debug/debug.c` and `debug/dbgchan.c`.
- `main/god_main.c` calls `DebugChannel_AppInit()` and activates channels when
  `dGODLIB_DEBUGCHANNELS` is defined.
- Many modules call `DebugChannel_Printf*()` directly; those calls become no-ops
  unless `dDEBUG` is enabled.
- `memory.h` switches allocation macros to debug versions under `dDEBUG`, and
  `memory.c` can track allocations when `dMEMORY_TRACK` is also enabled.

Noted oddities:

- `debug.c` has no actual function definitions.
- `dbgchan.c` was missing from the GCC `godlib/Makefile`; it has been added and
  `libgod.a` rebuilds successfully with `debug/dbgchan.o`.
- The debug channel macros depend on `dDEBUG`, while main initialization depends
  on `dGODLIB_DEBUGCHANNELS`. Enabling only one of those flags may produce a
  surprising configuration.

## debuglog

`godlib/debuglog` is an older, simpler debug logger than `debug/dbgchan`.

Main files:

- `debuglog.h` - compile-time macros and target flags.
- `debuglog.c` - file/screen/debugger output implementation when `dDEBUGLOG` is
  enabled.
- `dbglog_s.s` - Atari assembly implementation of `DebugLog_IsSTEEM()`.

It is controlled by `dDEBUGLOG`:

- with `dDEBUGLOG`, `DebugLog_Init()`, `DebugLog_DeInit()`,
  `DebugLog_AddString()` and `DebugLog_Printf0()` through `DebugLog_Printf5()`
  are active;
- without `dDEBUGLOG`, the public API compiles away to no-ops.

Output targets:

- `eDebugLog_File` - append text to a file;
- `eDebugLog_Screen` - print to stdout/screen;
- `eDebugLog_Debugger` - write to Steem or Windows debugger output.

Runtime behavior:

- `DebugLog_Init(targets, fileName)` stores the target mask and optionally opens
  the log file.
- `DebugLog_AddString()` writes the string to enabled targets.
- File logging writes the string, closes the file, reopens it with `File_OpenRW()`
  and seeks to the end. This is slower than keeping the handle open, but makes
  logs more likely to survive a crash.
- Atari debugger output writes the string pointer to Steem's `0xFFFFC1F0` hook
  when `DebugLog_IsSTEEM()` detects Steem.
- Windows debugger output uses `OutputDebugString()`.

Known users:

- GUI/FED/registry/tokeniser/asset/lexer/cutscene code contains many
  `DebugLog_Printf*()` calls.
- `unittest/unittest.h` reports failed expectations through `DebugLog_Printf3()`.
- `platform.c` has commented-out examples of `DebugLog_Init()`.

Integration:

- `godlib/Makefile` includes `debuglog/debuglog.c`.
- `godlib/Makefile` also includes `debuglog/dbglog_s.s`.

Noted oddities:

- `dbglog_s.s` exports both `DebugLog_IsSTEEM` and a stray `fart` label.
- `debuglog.c` also defines `DebugLog_IsSTEEM()` inside the Atari
  `dDEBUGLOG` path, while `dbglog_s.s` always exports the same symbol. A debug
  Atari build with `dDEBUGLOG` may therefore hit a duplicate symbol unless one of
  those implementations is excluded or renamed.
- The inactive macros in `debuglog.h` include trailing semicolons in definitions
  such as `#define DebugLog_Init( a,b );`, which is unusual but mostly harmless
  for existing call sites.

## bios

`godlib/bios` wraps Atari TOS BIOS calls, i.e. trap `#13`.

Main files:

- `bios.h` - public constants, BIOS data structures and function prototypes.
- `bios_s.s` - Atari implementation of the BIOS calls.
- `bios.c` - non-Atari fallback stubs for a small console subset.

The public API covers low-level BIOS services such as:

- console/device status and I/O: `Bios_Bconin()`, `Bios_Bconout()`,
  `Bios_Bconstat()`, `Bios_Bcostat()`;
- drive/media calls: `Bios_Drvmap()`, `Bios_Getbpb()`, `Bios_Mediach()`,
  `Bios_Rwabs()`;
- keyboard shift state: `Bios_Kbshift()`;
- exception vector install/read: `Bios_Setexec()`;
- console pipe helpers: `Bios_PipeConsole()`, `Bios_UnPipeConsole()`,
  `Bios_GetPipeOffset()`, `Bios_ClearPipeOffset()`.

Runtime behavior:

- `bios_s.s` pushes arguments in TOS order and calls `trap #13`.
- The small macros `mBIOS`, `mBIOS_W`, `mBIOS_WW`, `mBIOS_P` and
  `mBIOS_WP` cover common argument layouts.
- `Bios_Rwabs()` has a custom stack setup because it has more arguments and
  supports a long record number parameter.
- `Bios_PipeConsole()` installs a custom trap `#13` handler in supervisor mode.
  It intercepts `Bconout(2, char)` calls and copies console output bytes into a
  caller-provided buffer before chaining to the original BIOS handler.
- The pipe hook checks the `_CPU` cookie and adjusts the exception stack-frame
  offset for 68020+ CPUs.

Host behavior:

- When `dGODLIB_PLATFORM_ATARI` is not defined, `bios.c` provides no-op stubs
  for the console functions and pipe helpers. Most BIOS functions are only
  implemented by the Atari assembly file.

Noted oddities:

- `bios_s.s` previously exported `Bios_Getmbpb`, while the header declared
  `Bios_Getmpb()`. The assembly symbol has been corrected to `Bios_Getmpb()`.
- `bios_s.s` exports a `brake` label immediately before `Bios_PipeConsole()`.

## xbios

`godlib/xbios` wraps Atari TOS XBIOS calls, i.e. trap `#14`.

Main files:

- `xbios.h` - public constants, structures and prototypes.
- `xbios.c` - maps named wrappers to XBIOS function numbers.
- `xbios_s.s` - generic trap `#14` call helpers for many argument layouts.

The module is broad. It includes wrappers for classic ST services and later
machine extensions:

- mouse/keyboard/video setup, screen pointers and VBL sync;
- palette, resolution and extended video functions;
- floppy/DMA disk functions;
- IKBD, MFP and timer functions;
- sound/DMA audio and Falcon DSP functions;
- `Xbios_Dbmsg()`, which Hatari can intercept for debug printing.

Implementation pattern:

- `xbios.c` defines an internal enum of XBIOS opcodes, for example
  `eXBIOS_DBMSG = 11`, `eXBIOS_VSYNC = 37`, `eXBIOS_SUPEXEC = 38`.
- Each public wrapper chooses the matching `Xbios_Call_*()` helper.
- `xbios_s.s` contains the `Xbios_Call_*()` helpers. They push arguments onto
  the stack in the required TOS order, execute `trap #14`, clean the stack, and
  return the value left by the OS.

Examples:

- `Xbios_Vsync()` calls `Xbios_Call(eXBIOS_VSYNC)`.
- `Xbios_Dbmsg(rsvd, msgNum, msgArg)` calls
  `Xbios_Call_WWL(eXBIOS_DBMSG, rsvd, msgNum, msgArg)`.
- `Xbios_Setscreen(logic, physic, mode)` calls
  `Xbios_Call_PPW(eXBIOS_SETSCREEN, logic, physic, mode)`.

Notes:

- Pointer arguments are passed as 32-bit values, which matches Atari/GCC ELF
  target assumptions.
- `DebugChannel_HatariWrite()` uses `Xbios_Dbmsg(5, 0xF000, string)` instead
  of calling trap `#14` directly.

## gemdos

`godlib/gemdos` wraps Atari GEMDOS calls, i.e. trap `#1`.

Main files:

- `gemdos.h` - public constants, structures and prototypes.
- `gemdos.c` - maps named wrappers to GEMDOS function numbers.
- `gemdos_s.s` - generic trap `#1` call helpers for many argument layouts.

The public API covers both original GEMDOS services and many MiNT-style
extensions:

- console I/O: `GemDos_Cconin()`, `GemDos_Cconout()`, `GemDos_Cconws()`;
- directories and paths: `GemDos_Dcreate()`, `GemDos_Ddelete()`,
  `GemDos_Dsetpath()`, `GemDos_Dopendir()`, `GemDos_Dreaddir()`;
- files: `GemDos_Fopen()`, `GemDos_Fread()`, `GemDos_Fwrite()`,
  `GemDos_Fclose()`, `GemDos_Fseek()`, `GemDos_Fsfirst()`, `GemDos_Fsnext()`;
- memory: `GemDos_Malloc()`, `GemDos_Mfree()`, `GemDos_Mshrink()`,
  `GemDos_Mxalloc()`;
- process/signal/MiNT calls: `GemDos_Pexec()`, `GemDos_Pterm()`,
  `GemDos_Pgetpid()`, `GemDos_Psignal()`, `GemDos_Pwaitpid()`;
- supervisor mode and system info: `GemDos_Super()`, `GemDos_Sversion()`,
  `GemDos_Sysconf()`.

Implementation pattern:

- `gemdos.c` defines an internal enum of GEMDOS opcodes, for example
  `eGEMDOS_CCONOUT = 2`, `eGEMDOS_FOPEN = 0x3D`,
  `eGEMDOS_MALLOC = 0x48`, `eGEMDOS_PEXEC = 0x4B`.
- Each public wrapper selects a `GemDos_Call_*()` helper based on argument
  layout.
- `gemdos_s.s` pushes arguments in TOS order, executes `trap #1`, cleans the
  stack, restores `a2`, and returns the OS result.

Examples:

- `GemDos_Cconout(ch)` calls `GemDos_Call_W(eGEMDOS_CCONOUT, ch)`.
- `GemDos_Dcreate(path)` calls `GemDos_Call_P(eGEMDOS_DCREATE, path)`.
- `GemDos_Fread(handle, length, buffer)` uses a mixed word/long/pointer helper.

Notes:

- `gpGemDosStackUser` stores user stack state for `GemDos_Super()` handling.
- The header contains several MiNT-specific constants and structs, such as
  `sGemDosXATTR`, `sGemDosSigAction`, `sGemDosFLOCK` and terminal ioctl-style
  constants.
- The public prototype used to be misspelled as `GemDos_Psemaphoe()`, while
  `gemdos.c` defined `GemDos_Psemaphore()`. The header has been corrected.

## vector

`godlib/vector` is a small fixed-point 3D vector module implemented only in
68k assembly.

Main files:

- `vector.h` - public prototypes.
- `vector_s.s` - Atari/GCC ELF assembly implementation.

Source history:

- `GODLIB.ORG/VECTOR` appears to be the older version of this module.
- The older `VECTOR.H` declares only the fixed-point `Vector_*()` functions.
- It does not contain the later `FMatrix_*()` declarations present in the
  active `godlib/vector/vector.h`.
- `godlib.orginal/VECTOR` is effectively the same vector code, with an
  additional `vector_s.rmac` variant.

Implemented symbols:

- `Vector_Add()`
- `Vector_Sub()`
- `Vector_Mul()`
- `Vector_Normal()`
- `Vector_Length()`
- `Vector_SquaredLength()`
- `Vector_Dot()`
- `Vector_Cross()`
- `VecSqrt()`

Data format:

- The assembly assumes `sVector` is three signed 16-bit components:
  `x`, `y`, `z`.
- Offsets are hard-coded as `0`, `2`, `4`; structure size is `6` bytes.
- Multiplication returns the upper word after `muls.w`, so values appear to be
  expected in a fixed-point format.

Implementation notes:

- `Vector_Add(src0, src1, dst)` computes component-wise addition.
- `Vector_Sub(src0, src1, dst)` currently loads `src1` and subtracts `src0`,
  so it computes `src1 - src0`, despite the parameter order suggesting
  `src0 - src1`.
- `Vector_Mul(src, scaler, dst)` expects the scalar in `d0` and destination in
  `a1` under the GCC fastcall convention. The header has been adjusted to take
  `S16 aScaler` instead of `const S16 *`.
- `Vector_Length()` and `Vector_SquaredLength()` use signed 16-bit component
  multiplies accumulated into `d0`.
- `Vector_Normal()` uses `VecSqrt()` and then indexes `OneOver` as a reciprocal
  table, but `OneOver` is only declared as `ds.l 1`; this looks unfinished or
  dependent on a table that was never imported.
- `Vector_Cross()` follows the original assembly, but uses additions where the
  mathematical cross product would normally subtract terms. Treat its output
  with caution until tested.

Integration:

- `godlib/Makefile` builds `vector/vector_s.s`.
- No current in-tree call sites were found in `godlib`, `godlib.spl` or
  `niezwyciezony`.
- `vector.h` still declares many `FMatrix_*()` functions, but those are actually
  implemented in `godlib/maths/fmat_s.s`.
- `godlib/maths/fvec_s.s` and `godlib/maths/fmat_s.s` are present but are not
  currently built by the GCC `godlib/Makefile`.

Open issues:

- `sVector`, `sFVector` and `sFMatrix` are not currently defined in the active
  `godlib/base/base.h`, so `vector.h` is not self-contained for C code.
- The older `GODLIB.ORG` tree also does not appear to define `sVector`
  anywhere outside `VECTOR/VECTOR_S.S`, so this missing C type may be a very old
  gap rather than a GCC-port regression.
- The `FMatrix_Apply*()` prototypes in `vector.h` reuse the parameter name
  `apSrc` for both matrix and vector arguments, which may need cleanup if those
  prototypes are made active under GCC.
- `vector_s.s` previously exported `Vector_SquareLength` while the header and
  original GodLib used `Vector_SquaredLength`; the assembly symbol has been
  corrected.
- The same mismatch exists in `GODLIB.ORG/VECTOR/VECTOR_S.S`: it exports
  `Vector_SquareLength`, but the actual label and header prototype are
  `Vector_SquaredLength`.

## maths

`godlib/maths` contains floating-point vector and matrix assembly files:

- `fvec_s.s` - `FVector_*` operations on 3-component float vectors.
- `fmat_s.s` - `FMatrix_*` operations on 4x4 float matrices.

This module is not currently built by the GCC `godlib/Makefile`, and the
original `godlib.orginal/GODLIB.MAK` also did not include these files.

`fvec_s.s` contains:

- `FVector_Add()`
- `FVector_Sub()`
- `FVector_Mul()`
- `FVector_Div()`
- `FVector_Normalise()`
- `FVector_Length()`
- `FVector_Dot()`
- `FVector_Cross()`
- `FVector_Min()`
- `FVector_Max()`
- `FVector_Lerp()`

`fmat_s.s` contains:

- `FMatrix_BuildAxisAngle()`
- `FMatrix_BuildIdentity()`
- `FMatrix_BuildRotate()`
- `FMatrix_BuildRotateX()`
- `FMatrix_BuildRotateY()`
- `FMatrix_BuildRotateZ()`
- `FMatrix_BuildScale()`
- `FMatrix_BuildTranslation()`
- `FMatrix_Mul()`
- `FMatrix_Transpose()`
- `FMatrix_Apply()`
- `FMatrix_ApplyAxes()`
- `FMatrix_ApplyInv()`
- `FMatrix_ApplyInvAxes()`
- `FMatrix_ApplyPers()`

Architecture requirements:

- Both files use 68881/68030-style floating-point instructions such as
  `fmove.s`, `fsqrt`, `fsincos`, `fsglmul` and FP registers `fp0`..`fp7`.
- They do not assemble with the current default `vasmm68k_mot -nosym -devpac
  -Felf` settings used by the GodLib Makefile.
- They do assemble when tested with `vasmm68k_mot -m68030 -m68881 -nosym
  -devpac -Felf`, but that would require a separate build profile and matching
  runtime hardware/emulator assumptions.

Integration state:

- The files have no `xdef` exports, so even if they are assembled, their labels
  are not visible as public link symbols in the resulting ELF objects.
- `vector.h` declares many `FMatrix_*()` functions, but does not declare the
  `FVector_*()` API.
- `sFVector` and `sFMatrix` are not currently defined in active
  `godlib/base/base.h`.
- There is no `maths.h` header in the active tree.

Suspicious implementation details:

- `FVector_Dot()` takes one vector argument and computes squared length, not a
  two-vector dot product.
- `FVector_Cross()` uses additions where a mathematical cross product normally
  subtracts terms, matching the suspicious pattern already seen in
  `Vector_Cross()`.
- `FMatrix_BuildAxisAngle()` contains a `;not finished` comment.
- `FMatrix_BuildRotate()` has `fadd.s d7,fp6`; nearby code appears to prepare
  `fp7`, not `d7`.
- `FMatrix_BuildScale()` writes `#$3f80000` for the final homogeneous `1.0`,
  which is missing one zero compared with the usual `0x3F800000`.

Conclusion:

- `maths` is best treated as experimental/incomplete 3D math code, not as a
  ready GodLib module.
- Making it usable would need at least: type definitions, a public header,
  `xdef` exports, a dedicated FPU-capable build path, and behavioral tests for
  vector/matrix correctness.

## wipe

`godlib/wipe` is a screen transition module for Atari ST low-resolution
screens. It provides 39 predefined wipe effects, exposed through
`dWIPE_FX_LIMIT`.

Public API:

- `Wipe_In_Init(U16 aIndex, U16 * apGfx)`
- `Wipe_Out_Init(U16 aIndex)`
- `Wipe_Update(U16 * apScreen)`
- `Wipe_IsFinished(void)`

Typical usage:

1. Call `Wipe_In_Init()` or `Wipe_Out_Init()` with an effect index.
2. Call `Wipe_Update()` once per frame with the active screen buffer.
3. Poll `Wipe_IsFinished()` to know when the transition is complete.

Behaviour:

- Wipe-in stores `apGfx` as the source picture and progressively copies masked
  pixels from that source into `apScreen`.
- Wipe-out progressively clears pixels from `apScreen`.
- Invalid effect indices (`aIndex >= 39`) are ignored by the init functions.
- The module keeps global state, so only one wipe can be active at a time.

Implementation notes:

- `wipe_s.s` contains all effect data inline as `gWipeData1` through
  `gWipeData39`.
- Each animation frame is one 32-byte chunk: 16 words of mask data.
- `gWipeFrameCounts` derives the number of frames for each effect from the
  size of its mask data.
- The software render path assumes a 320x200, 4-plane ST low-resolution screen:
  200 scanlines, 40 longwords per line, 160 bytes per line.
- `Wipe_In_Render()` expands each 16-bit mask word into a longword and writes
  `dst = src & mask`.
- `Wipe_Out_Render()` inverts the mask and applies it to the screen with
  `screen &= ~mask`.

There is also an exported `Wipe_Render_STE()` routine which programs the STE
blitter/halftone registers directly (`$FFFF8A00` and friends). It is not
declared in `wipe.h`, and `Wipe_Update()` currently uses the software render
helpers instead.

Integration:

- `wipe/wipe_s.s` is built into the Atari GodLib library by the main
  `godlib/Makefile`.
- `dummy.c` provides host stubs where `Wipe_Update()` returns `0` and
  `Wipe_IsFinished()` returns `1`.
- The FE/FED renderer uses this module for page transitions:
  - `RenderFed_Update()` calls `Wipe_Update()`.
  - `RenderFed_TransitionStart()` calls `Wipe_In_Init()` or `Wipe_Out_Init()`.
  - `RenderFed_IsTransitionComplete()` polls `Wipe_IsFinished()`.
- FED parsing supports `WIPE_TYPE` values `NONE`, `RANDOM`, `SPECIFIC`, plus
  `WIPE_INDEX`.

Limitations and assumptions:

- The wipe code does not choose a random effect by itself; random/specific
  policy lives above it in the FED transition layer.
- It assumes ST low-resolution screen layout and does no validation of the
  source or destination buffer size.
- It is not reentrant because all state is stored globally.
- The blitter helper is STE-specific and writes hardware registers directly.

Original-tree comparison:

- No `GODLIB.ORG/WIPE` directory was found.
- `godlib.orginal/WIPE` contains the PureC-era version of the same module.

## fed

`godlib/fe` contains the FED front-end/menu system. It is not only a file
format: it includes the binary data model, text/JSON builders, relocation code,
runtime input handling, and a renderer.

Main files:

- `fed.h` - public FED data structures and runtime API.
- `fed.c` - relocation/delocation, runtime state, input/navigation logic,
  asset/variable setup and sound playback.
- `r_fed.c` / `r_fed.h` - renderer for pages, controls, text, sprites,
  sliders, cursor and transitions.
- `fedparse.c` / `fedparse.h` - old text chunk parser and binary FED builder.
- `fed_json.c` / `fed_json.h` - newer JSON-to-FED builder.
- `rel_fed.c` / `rel_fed.h` - asset relocator registration for FED files.

Data model:

- `sFedHeader` is the root object. It contains the magic `FEDS`, a version, all
  object arrays and counts.
- Object types include assets, calls, controls, control lists, font groups,
  lists, list items, locks, pages, page styles, samples, sliders, sprites,
  sprite groups, sprite lists, text, transitions and variables.
- Pages are selected by hash (`mHash`) and point at a title, optional background,
  cursor, page style, control list, sounds and optional sprite list.
- Controls can be:
  - `LINK` - switch to another page.
  - `CALL` - write a value to a hash-tree variable and return `1` from
    `Fed_Update()`.
  - `LIST` - cycle through list items bound to a variable.
  - `SLIDER` - adjust a numeric variable.
- Locks can affect both selection (`mLockedFlag`) and visibility (`mVisFlag`) by
  comparing hash-tree variables against configured values.

Runtime API:

- `Fed_SetpHashTree()` sets the variable tree used by FED variables.
- `Fed_Init()` registers every `sFedAsset` with `AssetClient_Init()` and every
  `sFedVar` with `HashTree_VarClient_Init()`.
- `Fed_Begin()` chooses the start page by name hash and starts the page intro.
- `Fed_Update()` processes input and returns `1` when a `CALL` control fires.
- `Fed_SetPage()` switches pages directly by name hash.
- `Fed_ForceRedraw()` forces the active page to redraw and restarts intro
  transition logic.
- `Fed_DeInit()` releases asset clients and variable clients.

Input behaviour:

- Up/down moves through the active page's control list.
- Left/right adjust `LIST` and `SLIDER` controls.
- Held left/right repeats only for sliders, using `dFED_KEYWAIT0` and
  `dFED_KEYWAIT1`.
- Fire activates the selected control.
- List movement wraps around and skips locked or invisible list items.
- Control-list movement does not wrap; it clamps at the first/last control and
  skips invisible controls.

Renderer:

- `RenderFed_Update(logic, back)` is meant to be called once per frame.
- It always calls `Wipe_Update()` on the logic canvas before page drawing.
- Backgrounds are Degas images copied into the canvas; without a background the
  canvas is cleared.
- Text drawing restores its text rectangle from the back canvas before printing.
- Cursor drawing uses two saved dirty rectangles indexed by `mRedrawIndex`; the
  old cursor area is restored from the back canvas before drawing again.
- Sprites support fixed-point animation through `mAnimSpeed` and `mFrame`; when
  the high word changes, the sprite asks for two redraws.
- Sliders are rendered directly as simple boxes using colors 15, 0 and 1.

Transitions:

- FED transitions combine fades and wipes.
- Fade modes are `NONE`, `BG`, `PAL` and `RGB`.
- Wipe modes are `NONE`, `RANDOM` and `SPECIFIC`, but actual random selection
  is not implemented in `wipe`; the renderer passes `mWipeIndex` directly.
- Intro transitions can prepare the full page into the back canvas and then use
  `Wipe_In_Init()` from that back buffer.
- Outro transitions call `Wipe_Out_Init()`.
- `RenderFed_IsTransitionComplete()` waits for both `Fade_IsVblFadeFinished()`
  and `Wipe_IsFinished()`.

Binary relocation:

- FED supports two relocation formats:
  - old/version `< dFED_VERSION_NEW` uses 1-based indices for many object
    references and converts them with `mFED_LIST_RELOC`.
  - new/version `>= dFED_VERSION_NEW` uses offsets relative to the FED header.
- `Fed_Delocate()` converts pointers to big-endian offsets for storage.
- `Fed_Relocate()` converts big-endian offsets back to live pointers.
- The code assumes 32-bit pointer storage in FED files.

Text parser:

- `FedParse_Text()` is a three-pass builder:
  1. Count chunks, strings and array items.
  2. Build per-chunk hash lists.
  3. Allocate one packed `sFedHeader` block and fill all objects.
- The chunk format supports names such as `ASSET`, `CALL`, `CONTROL`,
  `CONTROLLIST`, `FONTGROUP`, `LIST`, `LOCK`, `PAGE`, `PAGESTYLE`, `SAMPLE`,
  `SLIDER`, `SPRITE`, `SPRITEGROUP`, `SPRITELIST`, `TEXT`, `TRANSITION`,
  `VAR`.
- References are first stored as 1-based indices, then resolved by old-format
  relocation.
- The parser stores strings in the packed block and returns offsets relative to
  the FED header.

JSON builder:

- `FedJSON_ParseText()` parses JSON into a FED header through a count pass and a
  build pass.
- It creates version `dFED_VERSION_NEW`.
- It supports higher-level groups such as `controls`, `pages`, `pagestyle`,
  `sprites`, `vars`, plus control kinds like `action`, `backlink`, `link`,
  `selector`, `slider`, `toggle`.
- It currently contains hard-coded/default layout choices, for example control
  positions around x=32 and y=100 plus 10 pixels per item.
- It appears more experimental than the old text parser.

Asset integration:

- `rel_fed.c` registers the `"FED"` relocator.
- `Relocator_FED_DoRelocate()` calls `Fed_Relocate()`.
- `Relocator_FED_DoInit()` calls `Fed_Init()` and `Fed_Begin(..., "MAIN")`.
- `Relocator_FED_DoDelocate()` currently does not call `Fed_Delocate()`; that
  line is commented out.

Important limitations and risks:

- The active runtime is global (`gpFedHeader`, `gpFedPage`, `gpFedHashTree`,
  `gFedMode`, `gRenderFed`), so there is only one active FED instance.
- Many paths assume a 320x200 screen and ST-style graphics assets.
- Some paths assume page styles and samples exist; for example sample playback
  dereferences `Fed_GetpActivePage()->mpPageStyle` without checking the page
  style pointer first.
- `sFedSpriteList` exists in the data model, but `RenderFed_PageDraw()` only
  draws title and control list; page sprite lists do not appear to be rendered
  there.
- FED file pointers are 32-bit, so the packed binary format is Atari/GCC-ELF
  friendly but not naturally host-64 friendly without care.

Possible relevance to `niezwyciezony`:

- FED already has useful pieces for scripted front-end style screens:
  backgrounds, text, sprites, transitions and variable-driven state.
- It is menu/control oriented rather than a general cutscene sequencer.
- Reusing it for an intro would probably mean using selected renderer ideas
  rather than adopting the whole FED runtime directly.

## asset

`godlib/asset` is GodLib's package and asset-client system. It ties together
named contexts, packages, loaded files, type-specific relocators and runtime
clients waiting for asset data.

Main files:

- `asset.h` / `asset.c` - asset item/client structures, filename hashing and
  client load/unload callbacks.
- `context.h` / `context.c` - named asset contexts and lists of clients and
  packages belonging to each context.
- `package.h` / `package.c` - package manager, package queue, package status
  and package load/unload dispatch.
- `pkg_dir.h` / `pkg_dir.c` - package loader from an unpacked directory.
- `pkg_lnk.h` / `pkg_lnk.c` - package loader from a GodLib linkfile.
- `relocate.h` / `relocate.c` - extension-based relocator registry.

Core concepts:

- `sAssetItem` represents one loaded asset:
  - `mpData`
  - `mSize`
  - `mHashKey`
  - `mExtension`
  - `mStatusBits`
- `sAssetClient` represents code that wants an asset:
  - a short filename (`mFileName[12]`)
  - a context
  - optional `OnLoad` / `OnUnLoad` callbacks
  - an optional `void **mppData` to receive/clear the asset data pointer
- `sContext` groups asset clients and packages by name.
- `sPackage` represents a loadable group of files in one context.
- `sRelocater` maps a file extension hash to type-specific relocate/init/
  deinit/delocate callbacks.

Hashing:

- `Asset_BuildHash()` is used for context names, package names, filenames and
  extensions.
- Asset clients hash only the first 12 characters of the filename buffer.
- Context and package names are stored in 16-byte buffers.
- This strongly suggests old 8.3-style asset naming rather than arbitrary long
  paths.

Lifecycle:

1. Platform code enables the package system with `dGODLIB_PACKAGEMANGER`.
2. `Platform_Init()` calls:
   - `PackageManager_Init()`
   - `RelocaterManager_Init()`
   - registered relocator init functions such as `Relocator_BSB_Init()`,
     `Relocator_BFB_Init()`, `Relocator_FED_Init()`, `Relocator_SPL_Init()`.
3. Game/module code creates contexts with `Context_Init()`.
4. Code registers wanted assets with `AssetClient_Init()`.
5. Packages are created with `Package_Init()`.
6. `PackageManager_Load()` queues a package load.
7. `PackageManager_Update()` processes the queue.
8. Package loading loads files, relocates/initializes them, then services
   matching asset clients.
9. Package unload calls client unload callbacks, then deinit/delocate hooks,
   then releases file memory.

Client behaviour:

- Multiple clients can wait for the same asset hash in a context.
- The first client for a hash is stored in the context list.
- Additional clients with the same hash are linked through `mpNext`.
- When an asset is loaded, `AssetClients_OnLoad()` assigns `client->mpAsset`,
  writes `*mppData`, and calls `OnLoad` if present.
- When an asset is unloaded, `AssetClients_OnUnLoad()` calls `OnUnLoad`, clears
  `*mppData`, and clears `client->mpAsset`.
- If a client is registered after the matching asset is already loaded, it is
  immediately given the parent client's already-loaded data.

Package loading:

- `Package_Load()` chooses between:
  - linkfile load from `gpPackageManagerLinkPath` plus `.LNK`
  - directory load from `gpPackageManagerFilePath`
- Defaults in `Platform_Init()` are:
  - link path: `DATA`
  - directory path: `UNLINK`
- `PackageManager_SetLinkEnableFlag()` chooses the linkfile path or the
  directory path.
- With link loading enabled, `Package_Load()` does not visibly fall back to the
  directory loader if the linkfile load fails.

Directory packages:

- `PackageDir_Load()` enumerates files with `FilePattern`.
- For each file it:
  - gets size
  - loads the file into RAM
  - hashes the basename
  - hashes the extension
  - marks the item loaded
  - calls `RelocaterManager_DoRelocate()`
  - calls `RelocaterManager_DoInit()`
- It then services matching asset clients.
- It repeats client servicing while unresolved clients become resolved, allowing
  simple dependency chains created during init callbacks.

Linkfile packages:

- `PackageLnk_Load()` loads a `.LNK` into RAM through `LinkFile_InitToRAM()`.
- For each file in the root folder it points `mAsset.mpData` at the file offset,
  then calls relocate/init.
- It services clients in the same repeated dependency-resolution style as the
  directory loader.
- Folder traversal currently only uses the root folder in the observed load
  path.

Relocators:

- Relocators are selected by extension hash and optional `IsType` callback.
- Registered relocators found in this tree include:
  - `ASB`, `BSB`, `RSB` for sprite formats
  - `BFB` for fonts
  - `CUT` for cutscenes
  - `FED` for front-end data
  - `SPL` for samples
  - `GSM` exists in `pictypes`, though it is not in the observed platform init
    list
- A successful `DoRelocate()` sets `eASSET_STATUS_BIT_RELOCATED`.
- A successful `DoInit()` sets `eASSET_STATUS_BIT_INITED`.
- Deinit/delocate clear those status bits.

Kernel integration:

- `kernel` uses package flags per task.
- `Kernel_Init()` creates all packages from `sKernelPackageDef`.
- `Kernel_Main()` loads packages for the starting task, then swaps packages when
  the active task changes.
- `Kernel_PackagesLoad()` and `Kernel_PackagesUnLoad()` enqueue package
  operations by bitmask.

Current build/config state:

- The active `godlib/Makefile` has `dGODLIB_PACKAGEMANGER` commented out in
  `DEFS`, so the platform-level package manager init path is not enabled by
  default.
- The source files are still part of the library, but platform auto-init of the
  package manager and relocators depends on that define.

Important limitations and risks:

- Contexts are expected to be statically created with `Context_Init()`. If a
  client registers an unknown context, `ContextManager_ContextRegister()` asserts
  instead of allocating one.
- `PackageManager_Load()` and `PackageManager_UnLoad()` enqueue operations
  without checking queue overflow.
- The package operation queue is fixed at 32 entries.
- `Package_UnLoad()` uses an `||` condition when checking for already-unloaded
  states, which is always true for any single status value. In the current code
  this mostly means it always moves to `UNLOADING`/`UNLOADED`.
- Many diagnostic show functions are disabled with `#if 0`.
- The API and binary expectations are heavily 32-bit/Atari-oriented.

Possible relevance to an Orion Prime-style adventure:

- The package/context/client model is useful for scene-based asset loading:
  one context per screen/area or one context per game subsystem.
- Relocators are a clean way to make scene assets arrive already converted into
  runtime structures.
- The short-name/hash model fits Atari-era asset pipelines, but scene tooling
  should avoid relying on long filenames.
- For a game engine, this module is more useful as the asset streaming layer
  than as gameplay logic.

## kernel

`godlib/kernel` is a small game/application runtime. It combines a task/state
machine, package switching, input multiplexing, clocks, CLI commands and input
record/replay.

Files:

- `kernel.h` - public task/package definitions and runtime API.
- `kernel.c` - global kernel state, main loop, input handling, package handling,
  clocks, CLI and replay buffer.

Core data:

- `sKernelTask` describes one runtime task/state:
  - `mIndex`
  - `mPackageFlags`
  - `mfInit`
  - `mfDeInit`
  - `mfUpdate`
- `sKernelPackageDef` maps a package index to a package name and context name.
- `sKernelClass` is the private global runtime state:
  - active task and task list
  - current/global package flags
  - package array and package definitions
  - four clocks
  - combined input and source inputs
  - input monitoring/disable state
  - replay/save input buffer

Public API:

- `Kernel_Init()`
- `Kernel_DeInit()`
- `Kernel_Main()`
- `Kernel_RequestShutdown()`
- `Kernel_GetpClock()`
- `Kernel_GlobalPackagesLoad()`
- `Kernel_GlobalPackagesUnLoad()`
- `Kernel_InputsEnumerate()`
- `Kernel_ClockStartCountDown()`
- `Kernel_InputBufferLoad()`
- `Kernel_InputBufferSave()`
- `Kernel_RequestInputLoad()`

Task model:

- Tasks are the main state machine.
- Each task update returns the index of the next task.
- If the returned index differs from the current task index, the kernel:
  1. calls current task `mfDeInit()`
  2. compares package flags for old/new task plus global packages
  3. queues unload/load operations for changed package bits
  4. switches `mpTaskCurrent`
  5. processes package manager update
  6. calls new task `mfInit()`
- `mPackageFlags` are 32-bit bitmasks, so the task/package model supports up to
  32 package slots.

Main loop:

- Initial flow:
  1. `Screen_Update()`
  2. process `AUTOEXEC.CLI`
  3. load packages for task 0
  4. call task 0 init
- Per-frame flow:
  - input auto-monitoring/re-enumeration when needed
  - `Screen_Update()`
  - `IKBD_Update()`
  - update kernel clocks
  - `Random_Update()`
  - `Cli_Update()`
  - build combined input
  - optionally save/load replay input
  - call active task update
  - switch task and packages if requested
  - `PackageManager_Update()`
- Shutdown flow:
  - save replay input if recording
  - unload all currently loaded packages
  - drain the package manager queue

Package integration:

- `Kernel_Init()` calls `Package_Init()` for every `sKernelPackageDef`.
- `Kernel_PackagesLoad()` and `Kernel_PackagesUnLoad()` enqueue package
  operations by bitmask.
- `Kernel_GlobalPackagesLoad()` loads packages and marks them as global so they
  stay included when switching tasks.
- `Kernel_GlobalPackagesUnLoad()` removes that global-package bitmask.
- This sits directly on top of the `asset/package` system.

Input handling:

- The kernel builds one combined input from direction sources and fire sources.
- Direction input always includes IKBD.
- Fire input is mouse if mouse is enabled, otherwise joy0/joy1 if joystick mode
  is enabled.
- Joypads/team taps can be included as direction sources.
- Team tap inputs are disabled by default through `gKernelUsesTeamTapsFlag == 0`
  because processing them is considered slow.
- `Kernel_Input_Update()` uses `Input_CombinedUpdate()` for direction inputs and
  then overlays fire buttons from the fire input list.

Input auto-monitoring:

- At init, every input type is monitored through `mMonitoredInputs`.
- A VBL callback `Kernel_MonitorInputsUpdate()` updates all monitored inputs and
  increments per-input movement counters.
- After `dKERNEL_MONITOR_LIMIT` updates, `Kernel_BuildDisabled()` disables
  inputs whose counter is at least `dKERNEL_MONITOR_CUTOFF`.
- Then `Kernel_InputsEnumerate()` rebuilds the active input lists.
- This looks like a heuristic to disable noisy/unwanted input devices.

Clocks:

- The kernel owns four `sClock` instances:
  - `APP`
  - `FE`
  - `GAME`
  - `LEVEL`
- `Kernel_Clocks_Update()` updates all of them each frame.
- `Kernel_ClockStartCountDown()` configures and starts a countdown clock.

Input record/replay:

- Replay data stores an initial raw `sInput`, then delta packets per frame.
- Packet format:
  - `byte0 bit7` marks frame end.
  - `byte0 bits0-6` are an offset into `sInput`.
  - `byte1` is the new byte value.
  - `0xFF` means no input bytes changed for that frame.
- `Kernel_InputBufferSet()` records deltas.
- `Kernel_InputBufferGet()` applies deltas.
- On Windows builds (`dGODLIB_PLATFORM_WIN`), if no replay file was requested,
  the kernel allocates a 64 KB buffer and records input.
- `Kernel_InputBufferSave()` writes `INPUTS.KIP`.
- `Kernel_RequestInputLoad()` stores a filename to be loaded during
  `Kernel_Init()`.

CLI:

- Kernel registers these commands:
  - `achunlock`
  - `ass`
  - `assunused`
  - `build`
  - `inp`
  - `mem`
  - `quit`
  - `sys`
  - `vid`
- `AUTOEXEC.CLI` is processed before task 0 starts.
- `quit` requests shutdown.
- `mem`, `sys`, `vid` and `inp` print useful debug status.
- `ass` and `assunused` call package manager diagnostics, though many package
  diagnostic functions are currently disabled with `#if 0`.

Important limitations and risks:

- The whole runtime is global and supports one active kernel instance.
- `Kernel_Main()` assumes task 0 exists and that the current task pointer is
  valid.
- Task switching does not check whether `Kernel_GetpTask()` returned `0` before
  using the task pointer.
- Package bit operations assume valid package definitions for all used bit
  indices.
- Package manager support depends on `dGODLIB_PACKAGEMANGER`; in the active
  Makefile that define is commented out.
- Input replay offsets use only 7 bits, so this assumes `sizeof(sInput) <= 128`.
- `Kernel_GetpClock()` returns a clock pointer without bounds checking.
- `gKernelUsesTeamTapsFlag` is a global flag but there is no public setter in
  `kernel.h`.

Possible relevance to an Orion Prime-style adventure:

- The task model maps well to high-level game states: boot, intro, menu, game,
  inventory, dialog, pause/save.
- Package flags could load assets per scene group or game state.
- The input replay system could be useful for deterministic scene tests.
- For a scene-by-scene adventure, this kernel is useful as an outer runtime, but
  the actual adventure logic should still live in a dedicated scene/hotspot
  layer.

## vbl

`godlib/vbl` manages vertical-blank callbacks and a small amount of VBL-time
interrupt setup. On Atari it installs an assembly VBL handler at vector `$70`.
On non-Atari builds, `vbl.c` provides a manual C fallback.

Files:

- `vbl.h` - public API and `sVBL`.
- `vbl.c` - callback list management and non-Atari fallback handler.
- `vbl_s.s` - Atari VBL handler, counter, wait and vector install helpers.
- `vbl_s.inc` - old/partial structure offsets include; no current references
  were found in the repository.

Public API:

- `Vbl_Init()`
- `Vbl_DeInit()`
- `Vbl_AddCall()`
- `Vbl_RemoveCall()`
- `Vbl_GetCounter()`
- `Vbl_WaitVbl()`
- `Vbl_WaitVbls()`
- `Vbl_InstallTimerB()`
- `Vbl_InstallHbi()`
- `Vbl_GetpVbl()`
- `Vbl_SetVideoFunc()`
- `Vbl_CallsProcess()`
- `Vbl_SetHandler()`

State:

- `sVBL` contains:
  - reentrancy lock flag
  - Timer B scanline
  - HBI counter start/current value
  - callback count
  - HBI function
  - Timer B function
  - video function
  - up to `dVBL_MAX_CALLS` callback functions
- `dVBL_MAX_CALLS` is 64.
- `gVblOldHandler` stores the previous VBL vector so `Vbl_DeInit()` can restore
  it.

Initialization:

- `Vbl_Init()` saves the old handler with `Vbl_GetHandler()`, clears callback
  state, sets the video function to `Vbl_DummyFunc`, and installs
  `Vbl_Handler`.
- `Vbl_DeInit()` restores the old handler.
- `Platform_Hardware_Init()` calls `Vbl_Init()` after `System_Init()`.
- `Platform_Hardware_DeInit()` calls `Vbl_DeInit()` before `System_DeInit()`.

Atari assembly behaviour:

- `Vbl_GetHandler()` reads vector `$70`.
- `Vbl_SetHandler()` writes vector `$70` with interrupts disabled.
- `Vbl_GetCounter()` reads the system VBL counter at `$466`.
- `Vbl_WaitVbl()` uses XBIOS call 37 via `trap #14`.
- `Vbl_Handler()`:
  - raises interrupt mask to 7
  - installs HBI vector `$68` when `mfHbi` is set
  - installs Timer B vector `$120` and starts Timer B when `mfTimerBFunc` is set
  - uses `tas` on `mLockFlag` to avoid reentering itself
  - calls the configured video function
  - lowers SR to `$2400`
  - calls all registered VBL callbacks
  - increments system VBL counter `$466`
  - returns with `rte`

Non-Atari fallback:

- `Vbl_WaitVbl()` simply calls `Vbl_Handler()` and increments a local
  `gVblCounter`.
- The fallback `Vbl_Handler()` calls:
  1. video function
  2. Timer B function
  3. registered callbacks
- This fallback does not emulate real interrupt timing.

Callback handling:

- `Vbl_AddCall()` appends a callback unless the 64-call limit has been reached.
- `Vbl_RemoveCall()` searches for a matching function pointer and compacts the
  array.
- Modules using VBL callbacks include IKBD, profiler, audio mixer, clock, fade,
  screen grab and the music players.

Timer B and HBI:

- `Vbl_InstallTimerB()` installs an MFP Timer B timer on Atari and also stores
  the scanline/function into `gVbl`.
- `Vbl_Handler()` re-arms Timer B every VBL before running callbacks.
- `Vbl_InstallHbi()` stores an HBI counter/function and calls `System_SetIML(1)`.
- HBI handling uses vector `$68`.

Important limitations and risks:

- All state is global.
- `Vbl_AddCall()` does not check for duplicate callbacks.
- `Vbl_RemoveCall()` removes only the first matching callback.
- Registered callbacks execute in interrupt context on Atari, so they must be
  short and careful about shared state.
- The non-Atari fallback is useful for builds/tests but does not represent real
  Atari VBL timing.
- `vbl_s.inc` does not match the current `sVBL`/`vbl_s.s` layout because it
  lacks the `pVideoFunc` field. `rg` found no current users, so it looks like a
  stale unused file rather than an active build problem.
- `vbl.h` ends with the comment `INCLUDED_VIDEO_H`, which is just a cosmetic
  header guard comment typo.

## mfp

`godlib/mfp` manages the Atari MFP interrupt controller/timers. It saves and
restores MFP vectors/registers, installs timers A-D, provides a standard Timer C
clock, and supports a hookable Timer C callback.

Files:

- `mfp.h` - public API, timer enums, handler enums and structs.
- `mfp.c` - small C wrapper around assembly state and exported helpers.
- `mfp_s.s` - Atari MFP register/vector save/restore and timer handlers.

Hardware background:

- The comment in `mfp.c` gives the MFP clock as 2,457,600 Hz.
- A 200 Hz Timer C setup is described as divisor `/64`, data `192`.
- The assembly writes directly to MFP registers around `$FFFFFA00`.
- MFP vectors are saved/restored from vector table `$100` onward.

Public API:

- `Mfp_Init()`
- `Mfp_DeInit()`
- `Mfp_InstallTimerA/B/C/D()`
- `Mfp_GetTimerA/B/C/D()`
- `Mfp_InstallGPI7()`
- `Mfp_HookIntoTimerC()`
- `Mfp_HookDisableTimerC()`
- `Mfp_HookEnableTimerC()`
- `Mfp_DisableSystemTimerC()`
- `Mfp_GetpTime()`
- `Mfp_SetTime()`
- `Mfp_GetpSavedRegisters()`
- `Mfp_GetCounter200hz()`

Timer structure:

- `sMfpTimer` contains:
  - callback function
  - frequency
  - timer mode/divider
  - timer data
  - interrupt mask flag
  - interrupt enable flag

Initialization:

- `System_Init()` calls `Mfp_Init()`.
- `Mfp_Init()` saves MFP registers/vectors into `gMfpRegisterDump` and installs
  the standard Timer C handler.
- `System_DeInit()` calls `Mfp_DeInit()`.
- `Mfp_DeInit()` restores the saved MFP state.

Register save/restore:

- `Mfp_SaveRegisters()` saves:
  - 16 MFP vectors from `$100`
  - AER, DDR, VR
  - parallel port data
  - Timer A/B/D data
  - sync/USART data and control/status
  - interrupt mask, in-service, pending and enable registers
  - Timer A/B/C+D control
- Timer C data at `$FFFFFA23` is commented out in both save and restore.
- `Mfp_RestoreRegisters()` stops timers A/B/C/D before restoring saved values.

Timer install/get:

- Timer A vector is `$134`, data `$FFFFFA1F`, control `$FFFFFA19`.
- Timer B vector is `$120`, data `$FFFFFA21`, control `$FFFFFA1B`.
- Timer C vector is `$114`, data `$FFFFFA23`, control high nibble of
  `$FFFFFA1D`.
- Timer D vector is `$110`, data `$FFFFFA25`, control low nibble of
  `$FFFFFA1D`.
- Install functions stop the target timer, install the vector, write data,
  update mask/enable bits and restart using the requested mode.

Standard Timer C:

- `Mfp_InstallStandardTimerC()` saves the old Timer C vector into
  `Mfp_StcOldJump+2`, disables the optional hook, and installs
  `Mfp_StandardTimerC`.
- `Mfp_StandardTimerC()`:
  - updates `gMfpStcTime`
  - increments `gMfpStcCounter`
  - optionally calls a hook at a configured frequency
  - jumps to the old Timer C handler afterward
- `Mfp_GetpTime()` returns `gMfpStcTime`.
- `Mfp_GetCounter200hz()` returns `gMfpStcCounter`.

Timer C hook:

- `Mfp_HookIntoTimerC()` installs a secondary callback by patching
  `Mfp_StcNewJump+2`.
- The hook frequency is stored in `gMfpStcNewHz` and counted against the 200 Hz
  Timer C base.
- A lock flag prevents reentrant hook execution.
- `Mfp_HookDisableTimerC()` and `Mfp_HookEnableTimerC()` toggle the hook.
- `Mfp_DisableSystemTimerC()` replaces the old-system jump with a small routine
  that clears Timer C in-service and returns with `rte`.

GPI7:

- `Mfp_InstallGPI7()` installs a handler at vector `$13C`, configures the active
  edge bit in AER, and enables/masks the interrupt.

Integration:

- `Vbl_InstallTimerB()` uses `Mfp_InstallTimerB()` on Atari.
- `system/System_Calibrate()` temporarily uses Timer D and HBI to measure rates,
  but then currently overwrites the result with fixed 50 Hz / 10000 HBL values.
- `audio/ssd.c` and `music/snd.c` hook into Timer C.

Important limitations and risks:

- This is Atari hardware code; host builds rely on stubs in `dummy.c`.
- All MFP state is global.
- `mfp.h` declares `Mfp_SetTime()`, but `mfp.c` implements `Mfp_SetClock()`.
  Unless another alias exists elsewhere, this is a symbol/API mismatch.
- `Mfp_GetTimerD()` appears to read Timer C data (`sMfp_TCD`) instead of Timer D
  data (`sMfp_TDD`), while `Mfp_InstallTimerD()` writes `sMfp_TDD`.
- Timer C data save/restore is commented out, presumably to avoid disturbing the
  system clock, but this should be remembered before changing Timer C handling.
- Timer install functions patch interrupt vectors and hardware registers with
  interrupts disabled; callers need to avoid conflicting with OS/system users.

## ikbd

`godlib/ikbd` is GodLib's low-level keyboard, mouse, joystick, MIDI and Jaguar
pad input layer. On Atari it replaces the IKBD/MIDI interrupt handler and also
hooks selected BIOS keyboard calls. On host builds it can be backed by
DirectInput or SDL.

Files:

- `ikbd.h` - public scan codes, mouse button bits, `sIKBD` state and API.
- `ikbd.c` - initialization, getters/setters, mode switching and host fallbacks.
- `ikbd_s.s` - Atari interrupt handler, ring buffers, IKBD packet decoding,
  BIOS trap #13 hook and Jaguar pad/teamtap reads.
- `ikbd_di.c` / `ikbd_di.h` - DirectInput backend for `dGODLIB_SYSTEM_D3D`.
- `ikbd_sdl.c` / `ikbd_sdl.h` - SDL backend for `dGODLIB_SYSTEM_SDL`.

State:

- Global state lives in `gIKBD`.
- Keyboard and MIDI each have a 2048-byte ring buffer.
- `mKbdPressTable[128]` stores current scancode down/up state.
- Mouse state is relative/absolute X/Y plus button bits.
- Joystick state uses `mJoy0Packet` and `mJoy1Packet`.
- Falcon/STE Jaguar pads use `mPad0Dir`, `mPad1Dir`, `mPad0Key`, `mPad1Key`.
- Teamtap state uses eight direction bytes and eight key words.
- `gIKBDOldHandler` stores the previous Atari IKBD handler.

Initialization:

- `Platform_Hardware_Init()` calls `IKBD_Init()` after `Video_Init()`.
- `IKBD_Init()`:
  - clears `gIKBD`
  - reads Atari keyboard tables via `Xbios_Keytbl()`
  - saves the old handler from vector `$118`
  - flushes GEMDOS and hardware IKBD buffers
  - installs `IKBD_MainHandler`
  - clears key state and pad state
  - on STE/Falcon registers `IKBD_PowerpadHandler` as a VBL callback
  - initializes the DirectInput or SDL backend for host builds
  - installs the BIOS trap #13 hook
- `Platform_Hardware_Init()` then calls `IKBD_EnableJoysticks()`.
- `Platform_Hardware_DeInit()` switches back to mouse mode, deinitializes IKBD,
  and later flushes GEMDOS again.

Atari interrupt handling:

- `IKBD_GetHandler()` / `IKBD_SetHandler()` read/write vector `$118`.
- `IKBD_MainHandler()` handles both MIDI and keyboard ACIA registers:
  - MIDI base `$FFFFFC04`
  - IKBD base `$FFFFFC00`
  - loops while more IKBD interrupts are pending
  - signals end of interrupt through MFP register `$FFFFFA11`
- MIDI bytes are pushed into the MIDI ring buffer.
- Keyboard bytes below `$F6` are treated as key press/release bytes:
  - high bit set means release
  - press/release updates `mKbdPressTable`
  - key presses set `mKeyPressedFlag` and `mLastKeypress`
- IKBD packets `$F6..$FF` dispatch through `gfIkbdGrabbers`:
  - `$F6` status packet
  - `$F7` absolute mouse packet
  - `$F8..$FB` relative mouse packets
  - `$FC` time of day
  - `$FD` joystick report
  - `$FE` joystick 0
  - `$FF` joystick 1

BIOS trap #13 hook:

- `IKBD_InstallTrap13()` replaces vector `$B4` with `IKBD_Trap13`.
- It checks the `_CPU` cookie and adjusts stack-frame offset for 68020+.
- The hook overrides:
  - `Bconstat()`
  - `Bconin()`
  - `Kbshift()`
- Other BIOS calls jump to the original trap #13 handler.
- `IKBD_RestoreTrap13()` restores the original handler when installed.

Mouse/joystick mode:

- `IKBD_EnableJoysticks()` sends IKBD commands `$12` and `$14`, disables mouse
  flag and enables joystick flag.
- `IKBD_EnableMouse()` sends IKBD command `$08`, enables mouse flag and disables
  joystick flag.
- The high-level `input` module respects these flags and ignores stale
  mouse/joystick packets when the matching mode is disabled.

Jaguar pads and teamtap:

- On STE/Falcon, `IKBD_PowerpadHandler()` runs from VBL.
- It reads hardware around `$FFFF9200`.
- Without teamtap it updates pad A/B state directly.
- With teamtap enabled it reads four pads per port into the teamtap arrays.
- `IKBD_EnableTeamTap0/1()` and `IKBD_DisableTeamTap0/1()` only toggle the
  active bitmask; actual reads happen in the VBL callback.

Host backends:

- DirectInput backend:
  - initializes keyboard and mouse devices
  - uses buffered keyboard/mouse events
  - maps DI scancodes to GodLib scancodes
  - provides ASCII tables
- SDL backend:
  - calls `SDL_PumpEvents()`
  - polls key state with `SDL_GetKeyState()`
  - polls relative mouse movement with `SDL_GetRelativeMouseState()`
  - maps SDL keys to GodLib scancodes
  - provides ASCII tables

Important limitations and risks:

- `sIKBD` layout is mirrored manually in `ikbd_s.s`; changes to `ikbd.h` must be
  reflected in assembly offsets.
- `mKbdPressTable` has 128 entries, so scancodes are assumed to fit that range
  after release-bit stripping.
- Most Atari work happens in interrupt context.
- `IKBD_WaitForKey()` and `IKBD_WaitAnyKey()` busy-wait.
- `IKBD_EnableDebugging()` contains TOS-version-specific addresses and
  `IKBD_InitTosLink()` currently returns without patching anything.
- Fixed in the GCC port: `IKBD_GrabAbsMouseX1()` and
  `IKBD_GrabAbsMouseY1()` now copy from `sIKBD_Temp(a3)`.
- Fixed in the GCC port: `IKBD_DI_DeInit()` now clears `gpIkbdDIMouse` after
  releasing the mouse device.
- Fixed in the GCC port: DirectInput and SDL keyboard-buffer update paths now
  write to the current ring-buffer tail before advancing it, matching the Atari
  interrupt path more closely.
- Fixed in the GCC port: DirectInput and SDL keyboard-buffer events now use the
  Atari IKBD convention where key presses have bit 7 clear and key releases have
  bit 7 set.

## input

`godlib/input` is a higher-level input mapper built on top of `ikbd`. It turns
keyboard, mouse, joystick, Jaguar pad or teamtap state into a small normalized
set of game actions.

Files:

- `input.h` - input types, action/status enums, `sInput` and public API.
- `input.c` - update, combine and debug-string helpers.

Normalized actions:

- left
- right
- down
- up
- fire A
- fire B
- pause
- option
- quit

Key status bits:

- `eINPUTKEYSTATUS_HELD`
- `eINPUTKEYSTATUS_HIT`
- `eINPUTKEYSTATUS_UNHIT`

Default keyboard mapping:

- left/right/down/up - cursor keys
- fire A - space
- pause - F1
- option - F2
- quit - F10

Supported input types:

- keyboard/IKBD
- mouse
- joystick 0/1
- Jaguar pad A/B
- teamtap 0 pads A-D
- teamtap 1 pads A-D
- AI
- none

Update behaviour:

- `Input_Update()` clears `mMovedFlag` then dispatches by `mInputType`.
- Keyboard mode checks each configured scancode through `IKBD_GetKeyStatus()`.
- Mouse mode:
  - reads mouse X/Y/buttons only when `IKBD_IsMouseEnabled()` is true
  - turns movement deltas into direction actions with a threshold of 4 pixels
  - maps left/right mouse buttons to fire A/B
  - still reads pause/option/quit from keyboard scancodes
- Joystick mode:
  - still reads pause and quit from keyboard scancodes
  - maps joystick packet bits to directions and fire A
  - ignores joystick packets when `IKBD_IsJoystickEnabled()` is false
- Jaguar pad/teamtap mode maps pad direction/fire bits and key word bits to the
  normalized actions.
- `eINPUTTYPE_NONE` calls `Input_Init()`, resetting the input object to default
  keyboard mode.

Combining:

- `Input_CombinedInit()` initializes one destination and an array of source
  inputs.
- `Input_CombinedUpdate()` updates all source inputs, clears destination status,
  then copies any non-zero source status into the destination.
- `Input_Combine()` combines two already-updated inputs.
- `Input_CombineFire()` combines only fire A state from two inputs.
- `Input_DirClear()` clears only directional actions.

Debug/helper output:

- `Input_GetTypeName()` maps input type to short labels like `KEYS`, `MOUSE`,
  `JOY0`, `TAP0A`.
- `Input_BuildHeldString()`, `Input_BuildHitString()` and
  `Input_BuildUnHitString()` output a compact status string using
  `LRDUABPOQ`, with `.` for inactive actions.

Important limitations and risks:

- Fixed in the GCC port: `Input_Init()` now clears scan codes, repeat counts,
  coordinates and `mMovedFlag` in addition to key status.
- `Input_UpdateStatus()` is implemented as a macro, so arguments must be simple
  expressions without side effects.
- `mRepeatCounts` exists but the repeat handling is commented out.
- Combining currently lets later non-zero sources overwrite earlier non-zero
  status for the same action.
- Fixed in the GCC port: `eINPUTTYPE_AI` now has an explicit no-op case in
  `Input_Update()`.
- `Input_ProcessJagPad()` treats pad key bit 8 as quit; this is a GodLib
  convention rather than an obvious physical "quit" button.

## platform

`godlib/platform` is the high-level application bootstrap/debootstrap layer for
GodLib. It initializes memory, optional host systems, core hardware-facing
modules, optional managers, and then tears them down in mostly reverse order.

Files:

- `platform.h` - public platform init/deinit prototypes.
- `platform.c` - implementation of application and hardware init/deinit.

Public API:

- `Platform_Init()`
- `Platform_DeInit()`
- `Platform_Main()`
- `Platform_Hardware_Init()`
- `Platform_Hardware_DeInit()`

Entry-point relationship:

- `main/god_main.c` provides platform-specific process entry points:
  - `WinMain()` for `dGODLIB_SYSTEM_D3D`
  - `SDL_main()` for `dGODLIB_SYSTEM_SDL`
  - `main()` otherwise
- Those entry points call `GodLib_Game_Main()`.
- Applications generally implement `GodLib_Game_Main()` and call
  `Platform_Init()` / `Platform_DeInit()` themselves.
- `Platform_Main()` is declared in `platform.h`, but no implementation or caller
  was found in the current repo or original GodLib copy. It looks like a stale
  API declaration.

`Platform_Init()` order:

1. Optional debug log line to the GodLib debug channel.
2. `Memory_Init()`.
3. SDL-only:
   - `SDL_Init(SDL_INIT_VIDEO)`
   - `SDL_WM_SetCaption("SDL GodLib", "GodLib")`
4. `Platform_Hardware_Init()`.
5. Optional profiler init under `dPROFILER`.
6. Optional screen grab init under `dSCREENGRAB`.
7. `Random_Init()`.
8. Optional cutscene app init under `dGODLIB_CUTSCENE`.
9. Optional package manager and relocator init under
   `dGODLIB_PACKAGEMANGER`.

`Platform_DeInit()` order:

1. Optional cutscene app deinit.
2. Optional screen grab deinit.
3. Optional profiler deinit.
4. `Platform_Hardware_DeInit()`.
5. Optional package manager and relocator deinit.
6. SDL-only `SDL_Quit()`.
7. `Memory_DeInit()`.

`Platform_Hardware_Init()` order:

1. `System_Init()`.
2. Optional exception screen init under `dGODLIB_EXCEPTION_SCREEN`.
3. `Vbl_Init()`.
4. `Video_Init()`.
5. `IKBD_Init()`.
6. `Audio_Init()`.
7. Optional audio mixer init/enable under `dGODLIB_AUDIO_MIXER`.
8. Disable data and instruction caches.
9. Set CPU speed to 16 MHz.
10. `IKBD_EnableJoysticks()`.
11. `Graphic_Init()`.
12. Optional fade init under `dGODLIB_FADE`.
13. Optional clock init under `dGODLIB_CLOCK`.

`Platform_Hardware_DeInit()` order:

1. Optional clock deinit.
2. Optional fade deinit.
3. `Graphic_DeInit()`.
4. Optional audio mixer deinit.
5. `Audio_DeInit()`.
6. `IKBD_EnableMouse()`.
7. `IKBD_DeInit()`.
8. `Video_DeInit()`.
9. `Vbl_DeInit()`.
10. Optional exception screen deinit.
11. `System_DeInit()`.
12. `Vbl_WaitVbl()`.
13. `IKBD_FlushGemdos()`.
14. `Audio_SoundChipOff()`.

Optional systems:

- `dGODLIB_SYSTEM_SDL` adds SDL video init/quit.
- `dPROFILER` adds profiler init/deinit.
- `dSCREENGRAB` adds screen grab init/deinit.
- `dGODLIB_CUTSCENE` adds cutscene app init/deinit.
- `dGODLIB_PACKAGEMANGER` enables package manager and asset relocators.
- `dGODLIB_EXCEPTION_SCREEN` adds exception screen setup.
- `dGODLIB_AUDIO_MIXER` adds audio mixer setup.
- `dGODLIB_FADE` adds fade setup.
- `dGODLIB_CLOCK` adds clock setup.

Package manager defaults:

- `PackageManager_SetFilePath("UNLINK")`
- `PackageManager_SetLinkPath("DATA")`

Important limitations and risks:

- `dGODLIB_PACKAGEMANGER` is misspelled as "MANGER" in both current and original
  GodLib code. Any build flag must use the misspelled name unless the code is
  migrated consistently.
- `Platform_Main()` is declared but apparently unused/unimplemented.
- `Platform_Hardware_Init()` can be used without full `Platform_Init()` when an
  application wants to manage memory/assets manually, but then the caller does
  not get `Memory_Init()`, SDL init, random init, package manager setup, etc.
  The `niezwyciezony` intro currently follows this lighter pattern.
- `Platform_Hardware_Init()` always switches IKBD to joystick mode. Programs
  expecting mouse input need to call `IKBD_EnableMouse()` afterward.
- `Platform_Hardware_DeInit()` calls `Vbl_WaitVbl()` after `Vbl_DeInit()` and
  `System_DeInit()`. On Atari this likely falls back to the system VSync path,
  but it is worth remembering when changing VBL/system teardown.
- Cache disabling and fixed 16 MHz CPU speed are Atari-oriented assumptions.
  Host stubs make this harmless on non-Atari builds, but it is not a portable
  policy layer.
- Deinit order is not a perfect reverse of init. In particular package manager
  deinit happens after hardware deinit, even though package manager init happens
  after hardware init.
- The active `Makefile` currently enables only `dGODLIB_FADE` by default; most
  optional platform systems are compiled but inactive unless their define is
  added.

## system

`godlib/system` detects the current Atari/GodLib runtime environment and owns a
global `sSYSTEM` description. It also saves/restores exception vectors, starts
the MFP layer, calibrates refresh/HBL rates, controls cache/CPU-speed helpers,
and exposes emulator-specific state when running under supported emulators.

Files:

- `system.h` - public enums, `sSYSTEM`, `sSystemEmuDesc` and API.
- `system.c` - machine detection, getters/setters, calibration and cache
  dispatch.
- `system_s.s` - Atari assembly for vector save/restore, interrupt mask level,
  CACR/cache helpers, temporary HBL/200 Hz counters and emulator probes.

Global state:

- `gSystem` stores:
  - refresh rate and HBL rate as fixed-point `uU32`
  - total memory, ST RAM and TT RAM
  - machine, CPU, FPU, DSP, video, monitor, blitter and emulator type
  - TOS and emulator version
  - pointer to emulator descriptor
- `gSystemVectors` stores 62 saved exception vectors.

Machine description enums:

- machine: ST, STe, ST Book, Mega STe, TT, Falcon, Milan, Hades, Phenix
- CPU: 68000, 68010, 68020, 68030, 68040, 68060, G4
- FPU: none, SFP004, 68881, 68882, 68040
- DSP: none, 56000, 56001
- video: ST, STe, TT, Falcon
- monitor: TV, RGB, VGA, Mono, MultiSync, HDTV
- blitter: none or blitter
- emulator: none, unknown, Pacifist, Steem, TosBox

`System_Init()` order:

1. Calculate and store machine type.
2. Calculate and store CPU type.
3. Calculate and store FPU type.
4. Calculate and store DSP type.
5. Calculate and store monitor type.
6. Calculate and store video type.
7. Calculate and store TOS version.
8. Calculate and store blitter presence.
9. Calculate and store emulator type.
10. Calculate and store emulator descriptor pointer.
11. Calculate and store emulator version.
12. Calculate and store memory totals.
13. Save exception vectors.
14. Initialize MFP.
15. Calibrate refresh/HBL timing.

`System_DeInit()` order:

1. `Mfp_DeInit()`.
2. Restore saved exception vectors.

Detection sources:

- `_MCH` cookie maps to ST/STe/ST Book/Mega STe/TT/Falcon.
- `_VDO` cookie is used as fallback for machine and video type.
- `_CPU` cookie maps CPU generation.
- `_FPU` cookie maps FPU type.
- `_SND` cookie bit 4 implies DSP 56000.
- TOS version is read from address `2` on Atari.
- Blitter presence uses `Xbios_Blitmode(-1)` when TOS is at least 1.02.
- Monitor detection reads video hardware registers:
  - ST/STe: `$FFFF8260`
  - Falcon: `$FFFF8006`
- Memory detection reads:
  - ST RAM top at `$42E`
  - TT RAM cookie/magic at `$5A8` / `$5A4`

Non-Atari defaults:

- TOS version defaults to `0x206`.
- ST RAM defaults to 4 MB.
- TT RAM defaults to 0.
- Refresh rate defaults to 50 Hz.
- HBL rate defaults to `50 * 200`.
- Monitor defaults to TV.

Emulator support:

- Low-level emulator probes call XBIOS function `$25` with registers initialized
  to `"Emu?"`.
- Known IDs include:
  - `Emu?` - no emulator support
  - `TBox` - TosBox
  - `STEe` + `mEng` - Steem
  - `Paci` + `fiST` - Pacifist
- `System_CalcpEmuDesc()` accepts the low-level descriptor only if it equals
  address `$00FFC100`.
- Steem descriptor fields are used for:
  - version
  - slow motion
  - fast forward
  - MHz
  - debug build flag
  - snapshot flag
  - run/current speed
  - cycle counter
- `System_SetFastForwardFlag()` writes directly to `$00FFC11E` for Steem
  version `>= 0x330`.

Calibration:

- `System_Calibrate()` uses Timer D and a temporary HBI vector on Atari to count
  200 Hz and HBL activity for 32 VBLs.
- The measured values are currently overwritten with fixed values:
  - refresh rate = 50 Hz
  - HBL rate = `50 * 200`
- It restores:
  - previous interrupt mask level
  - previous HBI vector `$68`
  - previous Timer D setup
- `System_CalibrateVbl()` recalculates only refresh rate from the MFP 200 Hz
  counter over 32 VBLs.

Assembly helpers:

- `System_SaveVectors()` saves 62 vectors starting at address `8`.
- `System_RestoreVectors()` restores those same vectors.
- `System_SetIML()` sets the CPU interrupt mask level in SR.
- `System_GetIML()` reads the current interrupt mask level.
- `System_SetDataCache030/060()` and
  `System_SetInstructionCache030/060()` manipulate CACR where implemented.
- `System_HblTemp()` increments `gSystemHblTempCounter`.
- `System_200hzTemp()` increments `gSystem200hzTempCounter` and clears MFP
  Timer C/D in-service bit.

Cache and CPU speed:

- Data cache helpers dispatch by detected CPU:
  - 68030 -> `System_SetDataCache030()`
  - 68060 -> `System_SetDataCache060()`
  - Mega STe -> hardware register `$FFFF8E21`
- Instruction cache helpers dispatch for 68030 and 68060.
- `System_SetCPUSpeed()` only affects Mega STe:
  - above 8 MHz sets bit 0 at `$FFFF8E21`
  - 8 MHz or below clears that bit

Integration:

- `Platform_Hardware_Init()` calls `System_Init()` before VBL, video, IKBD and
  audio setup.
- `Platform_Hardware_DeInit()` calls `System_DeInit()` after VBL/video/IKBD
  teardown.
- `Mfp_Init()` and `Mfp_DeInit()` are owned by `System_Init()` /
  `System_DeInit()`.
- `Vbl`, `Mfp`, `IKBD`, `Video` and other modules query machine information
  through `System_Get*()` helpers.

Important limitations and risks:

- Fixed in the GCC port: `gSystemNamesEMU` maps the TosBox display name to
  `EMU_TOSBOX`.
- `System_CalcInfo()` recalculates many fields but does not refresh memory,
  emulator descriptor pointer or calibration data.
- Removed in the GCC port: `System_SaveVectors()` and `System_RestoreVectors()`
  had unreachable old MFP register save/restore code after `rts`.
  - MFP state is handled elsewhere through `Mfp_Init()` / `Mfp_DeInit()`, called
    by `System_Init()` / `System_DeInit()`.
- 68030 data/instruction cache assembly functions currently start with `rts`,
  so their CACR manipulation code is unreachable.
  - The dead data-cache path would read CACR, set/clear bit 8 and write CACR
    back.
  - The dead instruction-cache path would read CACR, set/clear bit 0 and write
    CACR back.
  - The 68060 cache helpers are not disabled this way.
- `System_Calibrate()` does real measurement work but then overwrites the result
  with fixed 50 Hz / 10000 HBL values.
- `System_CalcpEmuDesc()` hard-codes `$00FFC100`; this is Steem-specific and not
  a general emulator descriptor protocol.
- `System_SetFastForwardFlag()` writes directly to `$00FFC11E`, also
  Steem-specific.
- Several detection fallbacks return ST/ST video/68000 rather than UNKNOWN when
  cookies are missing. This is pragmatic for classic Atari, but it can hide
  host/stub detection mistakes.
- `System_GetRefreshRate()` and `System_GetHblRate()` return mutable pointers
  to global state.
- `sSYSTEM` and `sSystemVectors` are global and not reentrant.
- The assembly layer assumes Atari supervisor/hardware access. Host builds need
  stubs or platform guards for these routines.

## linkfile

`godlib/linkfile` implements GodLib `.LNK` archive files. A linkfile stores a
small FAT/header tree plus concatenated file data. It can be used either as an
on-disk archive with a live file handle, or loaded entirely into RAM.

Files:

- `linkfile.h` - public linkfile structures and API.
- `linkfile.c` - reading, loading files, relocation, archive creation and
  archive dumping.

Format structures:

- `sLinkFile` is the archive root:
  - `mID`
  - `mVersion`
  - `mFatSize`
  - `mTotalFileCount`
  - `mInRamFlag`
  - `mTotalFolderCount`
  - `mFileHandle`
  - `mpRoot`
- `sLinkFileFolder` contains:
  - file count
  - folder count
  - folder name
  - file array
  - subfolder array
- `sLinkFileFile` contains:
  - embedded `sAssetItem`
  - unpacked size
  - file data offset
  - packed/loaded flags
  - filename

Constants:

- Public version is `dLINKFILE_VERSION == 0xA`.
- Header/internal id is `dLINKFILE_ID == 0x12345678`.
- `dLINKFILE_SENTINEL` exists but the buffer-fill code that used it is
  currently commented out.

Loading modes:

- `LinkFile_Init()` opens the `.LNK`, reads only the FAT into memory, relocates
  it, keeps the file handle open and sets `mInRamFlag = 0`.
- `LinkFile_InitToRAM()` loads the whole `.LNK` into memory, depacks it if the
  linkfile itself is packed, relocates it, and sets `mInRamFlag = 1`.
- `LinkFile_DeInit()` closes the file handle only for non-RAM linkfiles and
  frees the linkfile memory.

File access:

- `LinkFile_FileExists()` looks up a file by path/name.
- `LinkFile_FileLoad()` allocates and returns file contents.
  - If the linkfile is already in RAM, it returns a pointer to data inside the
    linkfile memory.
  - If it is file-backed, it allocates memory, seeks to the file offset, reads
    the packed stored size, and optionally depacks in place.
- `LinkFile_FileLoadAt()` reads/copies file contents into a caller-provided
  buffer and optionally depacks.
- `LinkFile_FileGetSize()` returns packed or unpacked size.

Path lookup:

- `LinkFile_GetpFile()` walks folder names split on `/` or `\`.
- It uppercases path characters while parsing, then compares filenames with
  `String_StrCmpi()`.
- Folder lookup itself uses `strcmp()`, so stored folder-name case can matter.

Relocation:

- Linkfiles store pointers as big-endian offsets relative to the linkfile base.
- `LinkFile_Relocate()` endian-swaps header fields, converts `mpRoot`, then
  recurses through folders.
- `LinkFile_RelocateFolder()` converts folder/file pointers and endian-swaps
  file metadata.
- If `mInRamFlag` is set, file `mOffset` values are also converted into live
  pointers by adding the linkfile base.
- `LinkFile_Delocate()` reverses this before writing a linkfile FAT.
- The code assumes 32-bit pointer/offset storage.

Archive building:

- `LinkFile_Create()` creates an empty in-memory linkfile tree with a root
  folder.
- `LinkFile_FileCreate()` adds a file path, creating nested folder records as
  needed.
- `LinkFile_SerialiseFAT()` packs the folder tree, file entries and strings into
  one contiguous FAT block.
- `LinkFile_SetFileOffsets()` computes file data offsets after the FAT and
  detects packed files through `Packer_IsPacked()`.
- `LinkFile_Dump()` writes the delocated FAT first, then raw file data padded to
  4-byte boundaries.
- `LinkFile_BuildFromDirectory()` builds an archive from files directly in one
  directory.
- `LinkFile_BuildFromFile()` builds an archive from a list file whose first line
  is the source directory and following lines are file paths.

Asset/package integration:

- `asset/pkg_lnk.c` uses `LinkFile_InitToRAM()` for package loading.
- For each root-folder file it points the embedded `sAssetItem.mpData` at the
  in-RAM file data, then calls the asset relocator/init chain.
- The package loader currently uses `mpRoot` directly; nested linkfile folders
  are not traversed by the observed `PackageLnk_LoadFromLinkFile()` path.

Important limitations and risks:

- The format and relocation code are 32-bit pointer/offset oriented.
- The public API accepts mutable `char *` file names even when it does not
  obviously modify them.
- `LinkFile_FileLoad()` returns an internal pointer for in-RAM linkfiles, but an
  allocated buffer for file-backed linkfiles; callers need to understand the
  ownership difference.
- `LinkFile_BuildFromDirectory()` only enumerates files in the given directory,
  not nested directories.
- `LinkFile_FileCreate()` appears to copy old folders using
  `apFolder->mFileCount` instead of `apFolder->mFolderCount`, which looks wrong
  when adding folders.
- `LinkFile_FolderFree()` recurses with `LinkFile_FolderFree(apFolder)` instead
  of `&apFolder->mpFolders[i]`, which looks like an infinite recursion bug if a
  folder has subfolders.
- `LinkFile_SerialiseFiles()` recurses using `&apFolder[i]`; given the adjacent
  serialized layout this may have been intentional, but it is fragile and worth
  rechecking before relying on nested folders.

Possible relevance to an Orion Prime-style adventure:

- `.LNK` files are a good fit for shipping per-area/per-scene asset bundles.
- For now, root-folder-only packages are the safest path because the package
  loader does not traverse nested folders.
- The builder should be tested carefully before using nested paths in a new
  adventure asset pipeline.

## linklist

`godlib/linklist` is a tiny header-only set of macros for intrusive singly
linked lists. There is no runtime `.c` file, only `god_ll.h` and a unit test.

Provided macros:

- `GOD_LL_INSERT(head, nextField, item)` - insert item at the head.
- `GOD_LL_INSERT_TAIL(type, head, nextField, item)` - append item.
- `GOD_LL_REMOVE(type, head, nextField, item)` - remove item and clear its next
  pointer.
- `GOD_LL_FIND(head, nextField, field, value, result)` - linear search.
- `GOD_LL_MOVE_FORWARD(type, head, nextField, item)` - move item one slot toward
  the head.
- `GOD_LL_MOVE_BACK(type, head, nextField, item)` - move item one slot away from
  the head.
- `GOD_LL_MOVE_UP()` / `GOD_LL_MOVE_DOWN()` - intended parent/child movement
  helpers.

Usage:

- `asset/context.c` uses these macros for context/client lists.
- `asset/package.c` uses them for package/context package lists.
- Other modules can use them when they have an intrusive `mpNext`-style field.

Unit test:

- `linklist/unit/ut_godll.c` tests:
  - head insertion
  - tail insertion
  - find
  - removing heads
  - removing tails
  - removing middle items
- The unit test does not cover move macros.

Important limitations and risks:

- These are macros, so arguments can be evaluated in-place and type safety is
  minimal.
- The list node must contain the named next field.
- `GOD_LL_MOVE_FORWARD()` uses hard-coded `mpNext` inside the macro body instead
  of the `apNext` macro parameter, so it only works correctly for fields named
  `mpNext`.
- `GOD_LL_MOVE_DOWN()` calls `GOD_LL_INSERT()` with the wrong argument shape
  compared with the macro definition in this header.
- Because move macros are not covered by the unit test, the safest currently
  verified operations are insert, insert-tail, find and remove.

## string

`godlib/string` provides GodLib string helpers. It is not a replacement for the
standard C library; it is a small Atari-era utility module used by parsers,
asset builders, GUI code, linkfiles and tokenised data.

Files:

- `string.h` / `string.c` - `sString`, raw C-string helpers and value parsing.
- `strlist.h` / `strlist.c` - linked list of `sString` objects with optional
  string-data serialisation.
- `strpath.h` / `strpath.c` - fixed-size path manipulation helpers.
- `test/ut_str.c` - unit tests, mostly for path bounds and core string ops.

`sString`:

- `sString` stores:
  - `mpChars`
  - `mCharCountAndDynamicFlag`
- The high bit `eString_DynamicAllocFlag` marks ownership of `mpChars`.
- `String_Init()`, `String_Set()`, `String_Set2()`, `String_Copy()`,
  `String_Append()`, `String_Prepend()` and `String_Cat()` allocate through
  `mMEMCALLOC()`.
- `String_DeInit()` frees only when the dynamic flag is set.
- `String_SetStatic()` / `String_SetStaticNT()` point at existing text and do
  not take ownership.
- `String_IsEqual()` compares two counted strings.
- `String_IsEqualNT()` compares a counted `sString` against a null-terminated
  C string.
- `String_QuoteTrim()` removes matching leading/trailing quote characters. For
  static strings it advances `mpChars`; for dynamic strings it compacts in
  place and keeps the dynamic flag.

Raw string helpers:

- `String_StrLen()` is null-safe and returns `0` for null pointers.
- `String_StrCmp()` returns `0` for equal and `1` for different, unlike normal
  `strcmp()`.
- `String_StrCmpi()` is ASCII-only case-insensitive compare and also returns
  `0` for equal.
- `String_StrCpy()`, `String_StrCat()`, `String_StrAppend()` and friends do not
  know destination capacity.
- `String_StrCpy2()` copies at most `aDstLen - 1` bytes and null-terminates.
- `String_ToValue()` / `String_ToS32()` parse decimal and hex values. Hex is
  accepted as `$1234` or `0x1234`.
- `String_SubString()` returns a pointer to the first matching substring.
- `sTagString_GetFromString()` maps a counted `sString` to an `sTagString`
  entry.

`StringList`:

- `StringList_Init()` creates an empty intrusive list of `sStringListItem`.
- `StringList_ItemCreate()` allocates a new `sString`, copies input text, and
  inserts it at the head.
- `StringList_ItemDestroy()` unlinks and frees one item.
- `StringList_GetStringsSize()` returns the total null-terminated string data
  size.
- `StringList_StringsSerialise()` allocates one compact string block in
  `mpMem`.
- `StringList_StringsSerialiseTo()` copies all string data into a caller-owned
  buffer, repoints every `sString::mpChars` into that buffer and sets
  `eSTRINGLIST_FLAG_SERIALISED`.
- After serialisation, individual strings no longer own their old `mpChars`.
  `StringList_DeInit()` avoids freeing those repointed strings directly and
  frees `mpMem` if it owns the compact block.

`StringPath`:

- `sStringPath` is a fixed 256-byte path buffer.
- Paths accept both `\` and `/` as separators.
- Path construction helpers use `\` when inserting separators.
- `StringPath_GetpExt()` returns the last dot in the full path, not only in the
  filename portion.
- `StringPath_GetpFileName()` returns the substring after the last separator.
- `StringPath_GetDirectory()` returns everything before the last separator.
- `StringPath_GetFolder()` returns the last folder component.
- `StringPath_GetDrive()` treats `C:` style prefixes as absolute paths.
- Leading `/path` is not treated as absolute by `StringPath_IsAbsolute()`.
- `StringPath_Compact()` resolves simple `..` path components.
- `StringPath_GetFolderFirst()` / `StringPath_GetFolderNext()` iterate folders
  by temporarily overwriting separators inside the `sStringPath`.
- `StringPathSplitter_Next()` returns path components into a 14-byte local
  filename buffer, capped to 12 characters.

Main users:

- `tokenise`, `cutscene`, `fed`, `lexer/json`, `reflect`, `achieve` and
  `linkfile` depend on the string/value/tag helpers.
- `gui/guifs.c` uses dynamic `sString` heavily for file selector state.
- `file`, `drive`, `asset/package` and `linkfile` use `StringPath`.
- `StringList` is used by tokeniser and cutscene builders to collect strings
  before serialising generated data.

Tests:

- `string/test/ut_str.c` checks:
  - fixed path buffer null-termination under oversized input
  - extension/path append/combine bounds
  - directory/folder/file/extension extraction
  - `..` path compaction
  - dynamic string init/deinit
  - append/prepend/cat/copy/set behavior
  - aliasing in `String_Cat()`
  - ASCII case-insensitive compare

Important limitations and risks:

- `String_StrCmp()` / `String_StrCmpi()` use GodLib's `0 == equal` convention,
  but return only boolean difference, not lexical ordering.
- The raw `String_StrCpy()` / `String_StrCat()` / `String_StrAppend()` helpers
  are capacity-unsafe. The header already marks this API as deprecated/unsafe.
- Several helpers assume non-null inputs even though the prototypes do not say
  so, especially `StringPath_*()` functions.
- `String_Set()` has an unreachable null-input branch because the outer
  condition requires `apChars`.
- `String_Set2()` accepts one null input but then calls `String_StrLen()` and
  `String_StrCat()` in a way that only fully works when both inputs are non-null;
  null-plus-string currently allocates based on length but does not copy text.
- `String_CharInsert()` and `String_CharRemove()` always call `mMEMFREE()` on
  `mpChars` instead of checking `String_IsDynamic()`. They should only be used
  on dynamically owned strings.
- `String_CharInsert()` / `String_CharRemove()` reset the char count through
  `String_SetCharCount()` and do not restore the dynamic flag, so later
  `String_DeInit()` may leak the new allocation.
- `String_QuoteTrim()` on dynamic strings appears to copy one byte too few:
  after `length -= 2`, the loop copies while `i < length`, so a quoted string
  like `"abc"` would copy only `a` and `b` before setting length to `3`.
- `StringPath_GetpExt()` does not consider separators, so a dot in a directory
  name can be mistaken for a file extension if the filename has no dot.
- `StringPath_GetFolderFirst()` / `StringPath_GetFolderNext()` use
  `mChars[255]` as temporary separator storage, so they are destructive
  iterators and not safe to interleave.
- The module is byte-oriented ASCII/Atari text. There is no UTF-8 or locale-aware
  handling.

## program

`godlib/program` loads, relocates and optionally executes Atari `.PRG` / `.TOS`
program images. It is specific to the Atari executable format and the Atari
basepage model.

Files:

- `program.h` - Atari program header, symbol flags, basepage struct and public
  API.
- `program.c` - file loading, relocation, basepage setup and command-line /
  environment preparation.
- `prog_s.s` - Atari-only execution trampoline and replacement GEMDOS trap #1
  handler.

Executable format:

- `sProgramHeader` matches the classic TOS executable header:
  - magic
  - text/data/BSS sizes
  - symbol table size
  - reserved field
  - flags
  - relocation flag
- `dPROGRAM_MAGIC` is `0x601A`.
- Size fields are read as big-endian values.
- `sProgramSymbol` describes old-style 8-character symbols and uses the
  `dPROGRAM_SYMBOL_*` flags.
- The symbol flags are also used by `profiler/profiler.c` when reading symbol
  tables.

Loading:

- `Program_IsValid()` checks the big-endian executable magic.
- `Program_Load()`:
  - reads the TOS header
  - allocates one block containing `sBasePage`, the copied header, file contents
    and BSS space
  - places the copied header just before the text image, at
    `sizeof(sBasePage) - sizeof(sProgramHeader)`
  - calls `Program_Init()`
- `Program_Init()`:
  - reads text/data/BSS sizes
  - calls `Program_Relocate()`
  - fills basepage text/data/BSS pointers and lengths
  - inherits parent `mpHiTPA` and environment from `__BasPage`
  - uses `mReserved1` as DTA storage
  - clears the command-line length/string bytes
- `Program_UnLoad()` frees the allocated block.

Relocation:

- `Program_Relocate()` walks the Atari relocation table after text + data +
  symbol data.
- The first relocation is a big-endian long offset.
- Later relocation bytes are deltas:
  - `1` means add `254` to the current offset
  - any other non-zero byte is added directly
  - `0` terminates the relocation table
- Each relocated long is read as big-endian, adjusted by the loaded text base
  address, and written back as big-endian.

Execution:

- `Program_Execute()` copies a command line into the basepage command-line field.
- Normal command lines are capped at 125 characters plus length byte and null.
- Longer command lines try to build an `ARGV=` environment block in
  `gProgramArgvSpace[1024]`, with spaces converted to null-separated arguments.
- Before executing, it clears the loaded BSS.
- On Atari, it calls `Program_Execute_Internal()` from `prog_s.s`.
- On non-Atari builds, execution is skipped after clearing BSS.

Assembler trampoline:

- `Program_Execute_Internal()`:
  - saves registers and stack pointer
  - installs a temporary GEMDOS trap #1 handler
  - pushes basepage and a zero long on the child stack
  - sets up `a4` as DATA and `a5` as BSS
  - sets a simple linear malloc pointer just after BSS
  - clears registers
  - jumps to the loaded program entry at `basepage + sizeof(sBasePage)`
- `Program_Trap1()` handles a small subset of GEMDOS:
  - `$48` `Malloc` -> simple 4-byte-aligned bump allocator
  - `$4A` `Mshrink` -> returns `0`
  - `$4C` `Pterm` and function `0` -> returns to the parent trampoline
  - everything else forwards to the original GEMDOS trap handler
- Trap install/deinstall is run through XBIOS `Supexec`.
- `Program_TrapInitX()` detects 68020+ CPUs through the `_CPU` cookie and adjusts
  the exception stack-frame offset.

Fixed in the GCC port:

- `prog_s.s` was missing the `sBasePage_*` offset definitions that existed in
  the original PureC source. Without them, `prog_s.o` exported unresolved
  references to:
  - `sBasePage_mpData`
  - `sBasePage_mpBSS`
  - `sBasePage_mBSSLength`
  - `sBasePage_sizeof`
- The offset block has been restored in `prog_s.s`, matching the 32-bit Atari
  basepage layout used by `sBasePage`.

Important limitations and risks:

- This module is fundamentally 32-bit Atari/TOS-specific. The relocation code
  casts loaded pointers through `U32`, so it is not host-64-safe.
- `Program_Relocate()` reads header fields before checking whether `apHeader` is
  null.
- `Program_Load()` can return an allocated block even if the second file read is
  short; the failed path does not currently free `lpData`.
- `Program_Load()` does not explicitly close the file if the initial
  `File_Open()` fails but returns an invalid handle; behavior depends on
  `File_Close()` handling that value.
- `Program_UnLoad()` does not null-check before `mMEMFREE()`.
- `Program_ArgvCreate()` and `Program_ArgvDestroy()` are declared in
  `program.h` but are not implemented in `program.c`.
- The long-command-line ARGV path copies only until the first null byte of the
  parent environment, not necessarily the full double-null-terminated
  environment block.
- `gProgramArgvSpace` is a single global 1024-byte buffer, so nested or
  concurrent use is not safe.
- The replacement `Malloc` is a bump allocator without bounds checking against
  the loaded program allocation.
- The trap handler replaces vector `$84` directly and is only suitable while the
  child program is running under this trampoline.

## registry

`godlib/registry` implements a hierarchical runtime registry of named variables.
It can register variables by path, attach clients with callbacks, read/write
variable data, and build a serialisable save tree. In the current tree it appears
mostly self-contained; the observed active user is its own unit test.

Files:

- `registry.h` - public structs and API.
- `registry.c` - hash/path tokenising, node/variable/client management,
  save/load and relocate/delocate code.
- `test/ut_regis.c` - unit test for basic variable read/write and callbacks.

Naming and hashing:

- `Registry_BuildHash()` builds a case-insensitive 32-bit hash.
- Lowercase ASCII letters are converted to uppercase before hashing.
- Path separators `/` and `\` are normalised by `Registry_Tokenise()`.
- Leading separators are skipped.
- Token limits:
  - full normalised string buffer: `256` bytes
  - token count: `32`
- Each token stores:
  - local hash for the current path component
  - global hash from the remaining path substring

Tree model:

- `sRegistry` owns the root node list and tracks node/variable counts.
- `sRegistryNode` stores:
  - global/local hash IDs
  - refcount
  - variable list
  - parent/child/next links
- `sRegistryVar` stores:
  - global/local hash IDs
  - refcount
  - data size
  - data pointer
  - inline 32-bit small-data storage
  - owning node
  - client list
- Variables with `mDataSize <= 4` use `mDataSmall` as storage.
- Larger variables allocate data through `mMEMCALLOC()`.

Registering variables:

- `Registry_VarInit()` registers a named variable, allocates/points storage,
  copies initial data, and calls existing clients' `mfOnInit`.
- `Registry_VarDeInit()` calls clients' `mfOnDeInit`, frees large data, clears
  data state, and unregisters the variable.
- `Registry_VarRegister()` registers the variable path and increments variable
  and parent-node refcounts.
- `Registry_VarUnRegister()` decrements refs, unlinks/frees the variable at zero
  refs, then unregisters the parent node.
- `Registry_VarWrite()` copies new data into the variable and calls every
  client's `mfOnWrite`.
- `Registry_VarRead()` copies data only when the requested size exactly matches;
  otherwise it clears the destination buffer.

Clients:

- `Registry_VarClientRegister()` registers interest in a named variable and
  stores optional callbacks:
  - `mfOnWrite`
  - `mfOnInit`
  - `mfOnDeInit`
- Clients carry `mUserData`.
- Client registration increments the target variable refcount.
- `Registry_VarClientUnRegister()` unlinks the client, unregisters the variable,
  and frees the client.

Save/load format:

- `Registry_SaveNodeBuild()` registers a node path, computes save size, allocates
  one block and serialises the node tree into it.
- `sRegistrySaveNode` stores IDs, var count, pointer to vars, child pointer and
  next pointer.
- `sRegistrySaveVar` stores IDs, data size and a pointer to variable data inside
  the same save block.
- `Registry_SaveNodeLoad()` registers a destination node and writes saved vars
  into live registry variables.
- `Registry_SaveNodeUnLoad()` unregisters variables represented by a save node.
- `Registry_SaveNodeDelocate()` converts pointers inside a save tree into
  base-relative offsets and big-endian values.
- `Registry_SaveNodeRelocate()` reverses that conversion.

Unit test:

- `registry/test/ut_regis.c` tests:
  - init/deinit of a registry tree
  - variable init for all unit-test integral types
  - repeated write/read roundtrips
  - write callbacks
  - deinit callbacks
  - client unregister
- The test does not cover:
  - save-node build/load/unload
  - relocate/delocate
  - nested node save trees
  - long names or token-limit edge cases

Important limitations and risks:

- The save/relocate format is 32-bit pointer/offset oriented and not
  host-64-safe.
- `Registry_Tokenise()` does not guard against more than `32` path components
  before writing token arrays.
- `Registry_BuildHash()` assumes `apName` is non-null.
- `Registry_VarInit()` copies from `apData` without checking it for null.
- `Registry_NodeVarReg()` reads `apNode->mpVars` before checking whether
  `apNode` is null.
- `Registry_DeInit()` unregisters only `apTree->mpNodes` once; this relies on
  recursive unregister/destruction to clean the whole tree.
- `Registry_NodeDestroy()` sets `apTree->mpNodes = 0` whenever the destroyed node
  is the root pointer, even if root siblings exist.
- `Registry_NodeGetSaveSize()` assigns `lSize = sizeof(sRegistrySaveNode)` inside
  its sibling loop instead of accumulating with `+=`, so sibling sizes can be
  lost.
- `Registry_SaveNodeGetSize()` calls itself for `mpChild` but ignores the
  returned size, so child save nodes are not counted.
- `Registry_NodeSave()` sets `lpSaveNode->mpChild` after `Registry_NodesSave()`
  returns, which points at the end of the child block rather than the start.
- `Registry_NodesLoad()` creates a missing child node using the parent save node
  IDs instead of the child save node IDs before recursing.
- `Registry_SaveNodeLoad()` registers a node but never unregisters it afterwards;
  `Registry_SaveNodeUnLoad()` and `Registry_SaveNodeDestroy()` explicitly
  unregister twice.
- `Registry_SaveNodeDestroy()` currently just unregisters the named node twice
  and frees the single save block. That matches the one-block build path, but the
  old recursive free code is disabled with `#if 0`.
- `mNodeCount` is maintained, but `mVariableCount` appears never to be updated.
- Callback order is list order, with newest clients first.

## checksum

`godlib/checksum` provides a small incremental Fletcher-style checksum helper.
The module is tiny, but it is used by `achieve` to verify achievement/user data.

Files:

- `checksum.h` - `sCheckSumFletcher` state and public API.
- `checksum.c` - incremental checksum implementation.

Public API:

- `CheckSum_Fletcher_Init()` initialises the checksum state.
- `CheckSum_Fletcher_U8()` feeds one byte into the small 8-bit Fletcher state.
- `CheckSum_Fletcher_U16()` feeds one word into the big 16-bit Fletcher state.
- `CheckSum_Fletcher_U32()` feeds a long as two 16-bit words, low word first.
- `CheckSum_Fletcher_Get()` folds the current small/big states and returns one
  `U32` checksum.

State:

- `mLoopSmall` starts at `21`.
- `mLoopBig` starts at `360`.
- Small sums start at `0xFF`.
- Big sums start at `0xFFFF`.
- `mCheckSum` is initialised but not otherwise used by the implementation.

Current user:

- `achieve/ach_main.c::Achieve_CheckSumBuild()` uses this module.
- It feeds:
  - score table entry count as `U16`
  - score values as `U32`
  - stat values as `U32`
  - task bits as `U8`

Important limitations and risks:

- There is no standalone checksum unit test.
- `CheckSum_Fletcher_Get()` assumes `apFletcher` is non-null.
- `CheckSum_Fletcher_U16()` appears suspicious:
  - it adds `mSumSmall0` into `mSumBig1`, where Fletcher-16 style accumulation
    would normally add `mSumBig0`
  - when the big loop expires it resets `mLoopBig` to
    `dCHECKSUM_FLETCHER_LOOPSMALL` instead of `dCHECKSUM_FLETCHER_LOOPBIG`
- `CheckSum_Fletcher_U32()` feeds low 16 bits before high 16 bits. That is a
  defined local convention, but it is not network/big-endian order.
- Fixing the suspicious `U16` behavior would change existing checksum results,
  so it may affect compatibility with any saved achievement data that already
  uses the current algorithm.

## drive

`godlib/drive` is a small platform wrapper for directory and current-drive/path
operations. It sits underneath `file` for directory creation and existence
checks.

Files:

- `drive.h` - public drive/path API.
- `drive.c` - GEMDOS, Windows and host-GCC implementations.

API:

- `Drive_CreateDirectory()` creates a directory path recursively.
- `Drive_DeleteDirectory()` removes a directory.
- `Drive_DirectoryExists()` checks directory attributes through `File_GetAttribute()`.
- `Drive_GetFree()` returns free bytes on Atari/GEMDOS.
- `Drive_GetDrive()` returns the current drive number.
- `Drive_SetDrive()` changes the current drive where supported.
- `Drive_GetPath()` returns the current path.
- `Drive_SetPath()` changes the current path.

Platform behavior:

- Atari uses GEMDOS calls.
- Windows uses `_getdrive()`, `_chdrive()`, `_getdcwd()`, `_chdir()` and
  `CreateDirectory()` / `RemoveDirectory()`.
- Host GCC uses `mkdir()`, `rmdir()`, `getcwd()` and `chdir()`.
- Host GCC `Drive_GetDrive()` returns hard-coded drive `2`, matching `C:` style
  assumptions in other GodLib file code.

Important limitations and risks:

- `Drive_CreateDirectory()` accepts `const char *` but temporarily modifies the
  path string in place while creating intermediate directories.
- `Drive_GetFree()` has no host/Windows fallback and directly uses GEMDOS.
- `Drive_GetPath()` assumes a 256-byte destination buffer.
- Path behavior still has Atari/DOS drive-letter assumptions even on host GCC.

## file

`godlib/file` is GodLib's low-level file API. It wraps GEMDOS or host stdio,
provides DTA-style directory enumeration, file load/save helpers, a file selector
wrapper, and `sFileIdentifier` helpers for path/mask/filename handling.

Files:

- `file.h` - public file API and `sFileIdentifier`.
- `file.c` - platform file IO, directory search, load/save, file selector and
  file identifier helpers.
- `file_s.s` - Atari AES file selector bridge.
- `file_ptn.h` / `file_ptn.c` - pattern expansion helper built on `File_ReadFirst()`.

Basic file IO:

- `sFileHandle` is `SPTR`.
- Atari handles are GEMDOS numeric handles; host handles are `FILE *`.
- `File_Open()` opens read-only binary files.
- `File_OpenRW()` opens read/write binary files.
- `File_Create()` creates binary output files.
- `File_Read()` / `File_Write()` use byte counts and return bytes read/written
  on host.
- `File_Close()` closes valid handles.
- `File_SeekFromStart()` / `Current()` / `End()` wrap GEMDOS `Fseek()` or host
  `fseek()`.
- `File_Delete()` and `File_Rename()` wrap GEMDOS or host `remove()` / `rename()`.

Attributes and time:

- `File_GetAttribute()` maps platform attributes to GEMDOS-style flags.
- `File_GetTime()` returns packed GEMDOS-style date/time.
- Host GCC converts `stat()` / `localtime()` into `sGemDosDTA` date/time fields.
- `File_SetAttribute()`, `File_GetDateTime()` and `File_SetDateTime()` currently
  call GEMDOS directly.

DTA and directory search:

- `File_Init()` sets the default DTA to `gFileDTA`.
- `File_SetDTA()` stores `gpFileDTA` and calls `GemDos_Fsetdta()`.
- Atari `File_ReadFirst()` / `File_ReadNext()` call GEMDOS `Fsfirst/Fsnext`.
- Windows uses `_findfirst/_findnext`.
- Host GCC uses `opendir()/readdir()/stat()` and fills a GEMDOS-like DTA.

Load/save helpers:

- `File_Exists()` opens/checks a file.
- `File_GetSize()` opens/seeks or uses host stdio to return file size.
- `File_Load()` allocates and reads a whole file.
- `File_LoadSlowRam()` loads into `Memory_ScreenAlloc()` memory.
- `File_LoadAt()` reads into a caller-provided buffer.
- `File_UnLoad()` frees memory loaded through `File_Load()`.
- `File_Save()` ensures directories exist, creates/truncates and writes a whole
  buffer.
- `File_EnsureDirectoriesCreated()` creates parent directories before saving.

File selector:

- TOS builds call `File_FileSelectorAES()` from `file_s.s`.
- D3D/Windows builds use `GetOpenFileName()`.
- Other host builds return `0`.
- `file_s.s` performs AES `appl_init`, file selector call, `appl_exit`, and has
  supervisor/user-stack handling for TOS.

`sFileIdentifier`:

- Stores three allocated strings:
  - filename
  - mask
  - path
- `File_Identifier_Init()` initialises them from current path/drive and defaults
  filename/mask to `*.*`.
- `File_Identifier_FromFullName()` splits full paths into path, filename and
  mask.
- `File_Identifier_ToFullName()` allocates and returns a combined path/name.
- Fixed in the GCC port: `File_FileIdentifier_SetString()` now reallocates when
  the new input length exceeds the stored allocation size. It previously checked
  the old string length, which could overflow the buffer when assigning a longer
  value.

File pattern helper:

- `FilePattern_Init()` detects whether a path/pattern is expandable.
- Directories are expanded as `directory\*.*`.
- Wildcards `*` and `?` make a pattern expandable.
- `FilePattern_Next()` advances the search and returns full paths in
  `sFilePattern::mPath`.
- It skips `.` and `..` directory entries.

Important limitations and risks:

- Several public functions assume non-null filenames, buffers or DTA state.
- Host GCC `File_ReadFirst()` expects `apFspec` to be a directory path, not a
  wildcard pattern, because it passes it directly to `opendir()`.
- Fixed in the GCC port: host GCC `File_ReadFirst()` / `File_ReadNext()` no
  longer print debug messages to `stderr`.
- Fixed in the GCC port: `File_ReadNext()` on host GCC clears
  `gpFileDirentDIR` after `closedir()`.
- `File_SetDTA()` calls `GemDos_Fsetdta()` even on host builds.
- `File_SetAttribute()`, `File_GetDateTime()` and `File_SetDateTime()` are not
  platform-guarded away from GEMDOS.
- `File_Load()` on host allocates before opening the file; if the second open
  fails, the allocated memory is returned uninitialised rather than freed.
- `File_LoadAt()` on host does not check whether `fopen()` succeeded before
  `fread()`.
- `File_Save()` does not verify that all requested bytes were written.
- `File_EnsureDirectoriesCreated()` accepts `const char *` but temporarily
  modifies the path string in place.
- `File_FileName_ToExistingPath()` uses `File_ReadFirst()` to test directories,
  which is fragile on host because the host implementation treats the argument
  as an `opendir()` path.
- `File_Identifier_ToFullName()` returns an allocated string; callers must free
  it with `mMEMFREE()`.
- `File_Identifier_FromFullName()` relies on slash-separated paths and extension
  parsing. Paths without a dot can produce surprising masks.

## disk_io

`godlib/drive/disk_io` implements Atari ST disk image creation and simple
FAT12-style file/directory access. It can operate on a full image in memory or a
streamed image with boot sector/FAT/root directory cached.

Files:

- `disk_io.h` - boot sector, disk format, directory entry, image structs and API.
- `disk_io.c` - ST disk image format, FAT helpers, directory cache, file
  load/save and memory/streamed backends.
- `drive/test/ut_disk.c` - partial unit test for creating an image, FAT free
  cluster behavior and writing some files.

Disk format:

- Default `DiskFormatParameters_Init()` describes a 720 KB ST floppy:
  - 80 tracks
  - 2 sides
  - 9 sectors per track
  - 512 bytes per sector
  - 2 sectors per cluster
  - 5 sectors per FAT
  - 7 root-directory sectors
- `DiskImage_Create()` writes:
  - boot sector
  - first FAT sector with media/FAT reserved bytes
  - zero-filled remaining sectors

Image loading:

- `DiskImage_Load()` clears `sDiskImage`, calls backend init, reads geometry from
  the boot sector and initialises cached subdirectories.
- `gfDiskImageFuncs_ST_Memory` loads the full image into memory.
- `gfDiskImageFuncs_ST_Streamed` opens the image file and caches boot sector,
  FAT and root directory.
- `DiskImage_Save()` delegates to the backend commit function.
- `DiskImage_UnLoad()` recursively frees cached directory data and calls backend
  deinit.

FAT/directory/file API:

- `DiskImage_FAT_GetLinkedClusterNext()` reads 12-bit FAT entries.
- `DiskImage_FAT_SetNextClusterIndex()` writes 12-bit FAT entries.
- `DiskImage_FAT_GetFreeCluster()` finds a free cluster.
- `DiskImage_Directory_Create()` creates nested directories and `.` / `..`
  entries.
- `DiskImage_File_Exists()` and `DiskImage_File_GetSize()` look up directory
  entries.
- `DiskImage_File_Load()` / `LoadAt()` follow FAT chains and read clusters.
- `DiskImage_File_Save()` allocates clusters, writes file data, fills a directory
  entry and updates FAT links.
- `DiskImage_DirWalker_*()` iterates directory entries.

Important limitations and risks:

- Fixed in the GCC port: `DiskImage_DirDeInit()` now indexes `apDir[i]` instead
  of reading past the directory with `apDir[aEntryCount]`.
- Fixed in the GCC port: `DiskImage_FAT_GetLinkedClusterCount()` now advances
  through the current `clusterIndex`.
- Fixed in the GCC port: `DiskImage_FAT_GetFreeCluster()` now honours
  `aStartIndex`, clamping values below `2` to the first data cluster.
- `DiskImage_FAT_GetFreeClusterCount()` has duplicated `if (i & 1)` branches, so
  the even-cluster path looks wrong.
- `DiskImage_File_Load_Internal()` passes cluster indexes directly to
  `SectorsRead()` even though the backend read functions expect sector indexes.
  The write path uses `DiskImage_Cluster_Write()` and multiplies by sectors per
  cluster, so read/write are inconsistent.
- `DiskImageFuncs_ST_Memory_Dir_Commit()` copies only one sector of a cached
  subdirectory per cluster and does not advance the subdirectory source pointer.
- `DiskImageFuncs_ST_Streamed_Dir_Commit()` computes an offset but does not seek
  to it before writing.
- `DiskImageFuncs_ST_Streamed_Commit()` calls `File_HandleIsValid(apFileName)`,
  passing a filename pointer where a file handle is expected.
- `DiskImage_File_Save()` returns `1` even if it cannot find/create a directory
  entry after the free-space check.
- `DiskImage_Directory_Create_Internal()` writes some directory cluster fields
  without endian helpers.
- `DiskImage_File_Delete()` is declared in the header but not implemented.
- The unit test covers only part of the FAT/file save path and currently does
  not reload files to verify data integrity.

## memory

`godlib/memory` is the central allocation and memory utility layer. Most GodLib
modules allocate through the `mMEM*` macros from `memory.h`, which route either
to normal allocation functions or debug/tracking wrappers depending on build
flags.

Files:

- `memory.h` - public allocation macros and memory utility API.
- `memory.c` - platform allocation, optional tracking/guard logic, clear/copy
  helpers and debug wrappers.
- `memory_s.s` - Atari assembly implementation of `Memory_Clear()`.
- `heap.h` / `heap.c` - unfinished custom heap allocator.

Main allocation API:

- Non-debug macros:
  - `mMEMALLOC(size)` -> `Memory_Alloc(size)`
  - `mMEMCALLOC(size)` -> `Memory_Calloc(size)`
  - `mMEMSCREENCALLOC(size)` -> `Memory_ScreenCalloc(size)`
  - `mMEMFREE(ptr)` -> `_Memory_Release(ptr)`
  - `mMEMSCREENFREE(ptr)` -> `Memory_ScreenRelease(ptr)`
- Debug macros call `Memory_Dbg*()` variants and pass `__FILE__` / `__LINE__`.
- `Memory_Alloc()`:
  - host builds use `malloc()`
  - Atari TOS `> 0x200` uses `GemDos_Mxalloc()` with preferred TT RAM
  - older Atari uses `GemDos_Malloc()`
- `Memory_ScreenAlloc()`:
  - host builds use `malloc()`
  - Atari TOS `> 0x200` uses `GemDos_Mxalloc()` with ST RAM
  - older Atari uses `GemDos_Malloc()`
- `Memory_Calloc()` / `Memory_ScreenCalloc()` allocate then call
  `Memory_Clear()`.
- `_Memory_Release()` and `Memory_ScreenRelease()` free through host `free()` or
  `GemDos_Mfree()`.

GCC port notes:

- Current GCC port uses host `malloc()` / `free()` for non-Atari builds.
- Older dead `#if 0` blocks around host allocation/free were replaced in the
  current working tree.
- Atari allocation behavior remains GEMDOS/Mxalloc based.

Tracking and guards:

- `dMEMORY_TRACK` enables allocation records and counters:
  - allocation count
  - deallocation count
  - currently allocated size
  - high tide
  - largest/smallest allocation
  - failed allocation size
  - file/line/index per allocation
- `dMEMORY_GUARD` adds 16-byte header and trailer sentinels around allocations.
- `Memory_Validate()` checks tracked allocations against guard sentinels when
  both `dMEMORY_GUARD` and `dMEMORY_TRACK` are enabled.
- Debug allocation/free wrappers call `Memory_Validate()` before and after the
  operation.
- `Memory_DeInit()` asserts if tracked alloc/dealloc counts differ and dumps
  current records.

Memory utilities:

- `Memory_Clear()` clears a block to zero.
  - host builds use the C implementation in `memory.c`
  - Atari builds use `memory_s.s`
- `Memory_Copy_Internal()` is a byte-wise memmove-style copy:
  - copies backward when source is below destination
  - copies forward otherwise
  - does nothing if either pointer is null
- `Memory_Copy()` currently maps directly to `Memory_Copy_Internal()`.
- The old inline fast path for 1/2/4-byte copies is disabled with `#if 0`.
- `Memory_IsEqual()` compares byte buffers.
- `Memory_GetSize()` returns Atari ST RAM top on Atari and a hard-coded 14 MB
  on host.
- `Memory_GetFree()` queries GEMDOS `Malloc(-1)` on Atari, which returns the
  largest currently available ST-RAM block. Host builds still return `0`,
  because portable host free-memory reporting would be platform-specific.

Atari assembly clear:

- `memory_s.s` exports:
  - `Memory_Clear`
  - `Memory_ClearMajor`
  - `Memory_ClearSimple_16`
- The assembly implementation uses byte/long/movem clearing paths depending on
  size and alignment.
- There is old trailing code after `Memory_ClearSimple_16` that is unreachable
  from the exported clear paths. It looks like abandoned clear-loop experiments.

`heap`:

- `heap.c` appears to be an unfinished dlmalloc-inspired custom heap.
- It defines chunk headers, bins, fastbins and merge logic.
- `Heap_Init()` allocates a backing block but does not initialise top chunk /
  unsorted chunk state enough for general allocation.
- `Heap_Alloc()` only checks existing bins and otherwise returns `0`.
- `Heap_Free()` computes the chunk and size but does not link/free anything.
- `Heap_Reset()` is a no-op.
- No active GodLib module appears to include or call `Heap_*()`.
- Removed from the GCC port build: `memory/heap.c` is no longer linked into
  `libgod.a`; the source/header remain in the tree as unfinished historical code.

Important limitations and risks:

- The `mMEM*` macros include trailing semicolons, so they are statement-like and
  not safe in every expression context.
- Normal non-debug `mMEM*` allocation does not update tracking counters unless
  tracking is reached through debug wrappers.
- `Memory_TrackGetFreeRecord()` uses `U16 i`; on Windows
  `dMEMORY_RECORD_LIMIT` is `65536`, which can make the loop wrap if all records
  are occupied.
- `dMEMORY_GUARD` pointer adjustments cast pointers through `U32`, so guard mode
  is not host-64-safe.
- `Memory_Validate()` intentionally crashes with `*(U32*)0 = 0` on guard
  failure.
- Host `Memory_GetFree()` is unsupported and returns `0`.
- Host `Memory_GetSize()` is a hard-coded placeholder.
- `Memory_IsEqual()` assumes non-null buffers when `aSizeBytes > 0`.
- `Heap_*()` should be considered incomplete and unused until it gets tests and
  real allocation/free behavior.

## profiler

`godlib/profiler` contains two related but separate profiling systems:

- `profile.*` - lightweight scoped timing counters for named code regions.
- `profiler.*` / `profiles.s` - Atari interrupt sampler that records program
  counter samples to `PROFILE.PRO` and can later build a symbol hit table.

Files:

- `profile.h` / `profile.c` - `sProfile` counters, begin/end macros and CLI
  print helper.
- `profiler.h` / `profiler.c` - profile file format, HBL profiler lifecycle and
  symbol-table post-processing.
- `profiles.s` - Atari HBL/VBL interrupt routines used by the sampler.

`profile.*` lightweight profiler:

- `sProfile` tracks:
  - hit count
  - current duration
  - total duration
  - average duration
  - high tide duration
  - high tide tag
- `Profile_Init()` clears the structure.
- `Profile_CliPrint()` prints current/average/high-tide values through `cli`.
- `Profile_SetHiTidetag(tag)` stores a global tag copied into a profile when a
  new high tide is recorded.
- On Windows builds, `Profile_GetCPUCycleCount()` uses
  `QueryPerformanceCounter()` and rescales the result to an 8 MHz-style counter.
- On other host builds, `Profile_GetCPUCycleCount()` returns `0`.
- On Atari builds, `Profile_CpuBegin()` and `Profile_CpuEnd()` are currently
  defined as no-ops. There is an older commented implementation that read
  `0xFFC10C`, but it is disabled.

Interrupt sampler:

- `Profiler_Init()`:
  - saves old HBL vector from address `$68`
  - allocates a 32 KB sample buffer
  - creates `PROFILE.PRO`
  - writes an `sProfilerHeader`
  - registers `Profiler_VBL()` through the VBL module
- `Profiler_Enable()`:
  - resets `gProfilerIndex`
  - installs `Profiler_HBL()` at vector `$68`
  - sets interrupt mask level through `System_SetIML(1)`
- `Profiler_HBL()`:
  - runs as an HBL interrupt
  - increments `gProfilerIndex` by 4
  - stores the interrupted PC into `gpProfilerBuffer`
  - wraps the index inside the 32 KB buffer
- `Profiler_Update()` is called from VBL and flushes the buffer to
  `PROFILE.PRO` when it exceeds 75% full.
- `Profiler_Disable()` restores the saved HBL vector.
- `Profiler_DeInit()` disables profiling, restores the HBL vector, closes the
  file and frees the buffer.

Profile file / analysis helpers:

- `sProfilerHeader` stores:
  - `PROF` file ID
  - format version
  - game build hi/lo
  - text base/end marker
  - entry count
  - build date/time strings
- `Profiler_LoadProfile()` loads `PROFILE.PRO`, endian-relocates the header and
  computes sample count from file size.
- `Profiler_BuildSymbolTable()` combines a loaded Atari program image and a
  loaded profile file:
  - endian-converts program header and symbols
  - keeps global/extern/data/text/BSS symbols whose names do not start with `.`
  - sorts symbols by address
  - maps sampled PCs to symbols
  - sorts the final table by hit count
- `Profiler_AddHit()` uses a binary-search-like walk over the address-sorted
  table and increments a symbol hit count when a sample lands in its range.
- `Profiler_Relocate()` endian-converts profile headers loaded from disk.

Important limitations and risks:

- The interrupt sampler is Atari-only in practice. `profiler.c` directly reads
  and writes vector `$68`, and `profiles.s` is 68000 interrupt code.
- `Profiler_Init()` assumes the caller is allowed to touch interrupt vectors and
  create/write `PROFILE.PRO`.
- `Profiler_Init()` allocates a buffer and opens a file, but if one of those
  operations fails it returns `0` without cleaning up the other successful
  resource.
- `Profiler_DeInit()` removes neither the VBL callback added by
  `Vbl_AddCall(Profiler_VBL)` nor any duplicate callback if init is called more
  than once.
- `Profiler_Disable()` restores the HBL vector but does not restore the previous
  interrupt mask level.
- `Profiler_Enable()` uses the currently saved `gProfilerOldHBL`; if external
  code changes HBL after init, disable/deinit will restore the init-time value.
- The HBL sample index is incremented before storing, so offset `0` is unused.
  `Profiler_Update()` writes `gProfilerIndex` bytes, which appears to include
  the unused first slot and omit the most recent slot.
- The sampler stores absolute PCs and later subtracts `apProfile->mpText`.
  Correct analysis depends on the recorded text marker matching the program
  image used by `Profiler_BuildSymbolTable()`.
- GCC builds use the external `__text` symbol for the profile header. Apps that
  enable this profiler need that symbol to exist in the linker script/output.
- `Profiler_BuildSymbolTable()` endian-converts the supplied program header and
  symbols in place, so callers should treat the program buffer as modified.
- `Profiler_BuildSymbolTable()` does not check the `mMEMCALLOC()` result before
  writing the table.
- `Profiler_AddHit()` assumes the symbol table has a sentinel entry after the
  last real symbol.
- `profile.*` is not useful for Atari timing in its current form because the
  Atari begin/end macros are disabled.

## blitter

`godlib/blitter` is a low-level Atari blitter wrapper for ST-low style planar
graphics. It talks directly to the blitter register block at `$FFFF8A00`.

Files:

- `blitter.h` - public blitter register layout, logical/halftone operation
  enums and sprite/box structures.
- `blitter.c` - C setup and drawing helpers around the hardware blitter.

Hardware model:

- `sBlitter` maps the Atari blitter register block:
  - halftone RAM
  - source/destination X/Y increments
  - source/destination pointers
  - end masks
  - X/Y counts
  - HOP/LOP/mode/skew registers
- Logical operation enum maps the 16 Atari LOP modes.
- Halftone operation enum maps the 4 Atari HOP modes.
- Mode/skew bit defines expose busy, hog, smudge, FXSR and NFSR bits.

Runtime setup:

- `Blitter_Init()` builds `gBlitterFlipTable[256]`, a bit-reversed byte lookup
  table.
- `Blitter_DeInit()` is empty.
- `Blitter_IsAvailable()` checks `System_GetBLT() == BLT_BLITTER`.
- Every public drawing/copy routine bails out when no blitter is available.
- `Blitter_Wait()` spins until the hardware busy bit clears.

Supported operations:

- `Blitter_CopyBox()` copies a rectangular area between two ST-low screens.
  It clips source and destination rectangles to hard-coded `320x200` bounds.
- `Blitter_DrawSprite()` draws a masked planar sprite:
  - first ANDs the destination planes with mask data
  - then ORs sprite graphics into destination planes
- `Blitter_DrawOpaqueSprite()` copies sprite graphics without applying a mask.
- `Blitter_DrawColouredSprite()` uses the sprite mask and only draws selected
  bitplanes based on `aColour`.
- `Blitter_DrawBox()` fills a rectangular box using per-plane all-zero/all-one
  patterns derived from a 4-bit colour.

Important limitations and risks:

- The implementation is fixed to ST low resolution assumptions:
  - width `320`
  - height `200`
  - line size `160` bytes / `80` words
  - 4 bitplanes
- It is not a generic canvas blitter; callers must pass ST-low compatible
  screen memory.
- Horizontal sprite clipping is very conservative: negative X, off-right, or
  over-wide sprites are rejected rather than clipped. Vertical clipping is
  partially handled.
- Sprite width is effectively expected to be a multiple of 16 pixels because
  `lWords = Width >> 4`.
- The old `apSprite->Width != 16` guard is commented out. Wider sprites are now
  attempted, but correctness depends on the sprite data layout and blitter skew
  path.
- `gBlitterHack` remains as a leftover global and is not actively used.
- All hardware waits are busy loops.

## graphic

`godlib/graphic` is GodLib's canvas-style drawing abstraction. It routes drawing
through an `sGraphicFuncs` table selected by colour mode, and optionally swaps
some 4-plane routines to blitter-backed implementations when blitter rendering
is enabled.

Files:

- `graphic.h` - public canvas, rectangle/box/position types, drawing function
  table and convenience macros.
- `graphic.c` - global function-table setup, canvas init/deinit, font printing,
  centring helper and dirty-chunk restore helpers.
- `graphic.i` - assembler structure offsets matching `graphic.h`.
- `grf_4.c` - C 4-bitplane drawing backend.
- `grf_4_s.s` - Atari assembly 4-bitplane backend.
- `grf_b4_s.s` - Atari blitter-backed 4-bitplane backend.
- `grf_16.c` - C 16bpp backend, enabled only when `dGODLIB_16BPP` is defined
  in `graphic.c`.
- `grf_16_s.s` - assembly 16bpp exports, currently built but not selected by
  `graphic.c`.
- `grf_tc_s.s` - true-colour assembly exports, currently built but not wired
  into `graphic.c`.
- `graphic.old` - historical copy/reference file.

Canvas model:

- `sGraphicCanvas` stores:
  - VRAM pointer
  - colour mode
  - width/height
  - clip box
  - normal and clipped function-table pointers
  - `mpLineOffsets`, a per-line byte-offset table
- `GraphicCanvas_Init()` computes line offsets from colour mode and dimensions:
  - 1/2/4-plane modes are rounded to 16-pixel boundaries
  - 8bpp uses one byte per pixel
  - 16bpp uses two bytes per pixel
  - 24bpp uses three bytes per pixel
  - 32bpp uses four bytes per pixel
- `GraphicCanvas_DeInit()` frees `mpLineOffsets`.
- `GraphicCanvas_SetpVRAM()` attaches external screen/buffer memory to a
  canvas.

Function tables:

- `Graphic_Init()` calls `Blitter_Init()` and then
  `Graphic_SetBlitterEnable(gGraphicBlitterEnableFlag)`.
- 4-plane functions are always installed.
- 8bpp chunky functions are installed only under `dGODLIB_CHUNKY`.
- 16bpp functions are installed only under `dGODLIB_16BPP`.
- `Graphic_SetBlitterEnable(1)` switches selected 4-plane operations to
  `_BLT` routines if `System_GetBLT()` reports blitter availability.
- When blitter is not available, `Graphic_SetBlitterEnable()` falls back to CPU
  4-plane routines and clears `gGraphicBlitterEnableFlag`.
- The public `GraphicCanvas_*` macros call through `mpFuncs` and
  `GraphicCanvas_*_Clip` macros call through `mpClipFuncs`.

4-plane backend:

- C backend supports:
  - clear/copy screen
  - box fill
  - pixel set/get
  - sprite draw
  - partial sprite draw
  - clipped blit/box/pixel/sprite wrappers
- `Graphic_4BP_Blit()` in the C backend has a word/plane copy fast path when
  source and destination X positions have the same 16-pixel alignment. Other
  shifted cases still fall back to the simple pixel-by-pixel implementation.
- The current Atari `Makefile` does not build `grf_4.c`; it uses
  `grf_4_s.s`, so this C backend is fallback/reference code unless the build is
  changed.
- `Graphic_4BP_DrawLine()`, `DrawTri()` and `DrawQuad()` in `grf_4.c` are
  stubs.
- The assembly `grf_4_s.s` exports full 4-plane symbols including line and
  sprite routines, but the C object also defines many of the same names. Which
  implementation is used depends on the archive/link order.
- `grf_b4_s.s` provides blitter-backed 4-plane routines and forwards line draw
  to the non-blitter line implementation.

16bpp backend:

- C backend supports:
  - clear/copy screen
  - pixel set/get
  - box fill
  - sprite draw
  - clipped blit/box/pixel/sprite wrappers
- `Graphic_16BPP_Blit()` and `DrawBox()` are pixel-by-pixel implementations.
- `Graphic_16BPP_DrawLine()`, `DrawTri()` and `DrawQuad()` are stubs.
- Clipped 16bpp sprite drawing adjusts destination position for negative X/Y
  but does not advance the source pointer to skip clipped source columns/rows,
  so partially clipped sprites look suspicious.

Font helpers:

- `Graphic_FontPrint()` and `Graphic_FontPrintClip()` render text by fetching
  character sprites from `font` and drawing each sprite through the canvas'
  draw-sprite function.
- The font code advances by font char bounds plus `mKerning`.
- `Graphic_FontPrintLeft()`, `Right()` and `Centred()` compute the starting X
  from a rectangle and then call the active `FontPrint` function.
- `Graphic_Init()` sets `FontPrint` for 4-plane normal/clipped tables and for
  16bpp normal table when `dGODLIB_16BPP` is enabled.

Dirty chunks:

- `sGraphicChunkList` stores up to 32 chunks, each holding an offset and height.
- `Graphic_ChunkList_Store()` breaks a rectangle into 16-pixel aligned chunks
  and records 8-byte-wide planar spans.
- `Graphic_ChunkList_ReStore()` copies those chunks from a source canvas to a
  destination canvas, four words per line. This is useful for restoring
  background under planar sprites.

Important limitations and risks:

- `GraphicCanvas_Init()` does not check whether `mMEMALLOC()` for
  `mpLineOffsets` succeeded before writing the table.
- `GraphicCanvas_Init()` does not validate `aColourMode` before indexing
  `gGraphicFuncs[aColourMode]`.
- Most canvas macros assume `mpFuncs`/`mpClipFuncs` entries are non-null. Colour
  modes whose functions were not installed can crash when called.
- `Graphic_SetBlitterEnable(1)` does not install blitter-backed 4-plane
  `DrawLine`; the previous table value can remain unless the non-blitter path
  was initialised first.
- `Graphic_SetBlitterEnable(1)` currently routes 4-plane blit through
  `Graphic_4BP_Blit_BLT` / `Graphic_4BP_Blit_Clip_BLT`. These assembler
  routines build a classic `BLiT_iT` parameter block and take source and
  destination line stride from `mpLineOffsets[1]`. They still assume a 4-plane
  interleaved screen layout: 8 bytes per 16-pixel word group and 2 bytes
  between bitplanes. Same-source and destination VRAM still falls back to the
  CPU blit path.
- `grf_16.h` uses `INCLUDED_GRF_4_H` as its include guard. Including `grf_4.h`
  and `grf_16.h` in the same translation unit can accidentally suppress one
  header.
- `Graphic_ChunkList_Store()` trusts rectangle coordinates and can index
  `mpLineOffsets[lY]` without clipping.
- `Graphic_ChunkList_ReStore()` computes both source and destination line
  strides from `apSrc->mpLineOffsets[1]`; this assumes both canvases have the
  same stride.
- `mGODLIB_CHUNKLIST()` expands to a raw `U16` array while the functions expect
  `sGraphicChunkList *`; this relies on matching layout and is not type-safe.
- The assembler backends depend on `graphic.i` staying exactly in sync with
  `graphic.h`.

## screen

`godlib/screen` is the higher-level screen-buffer manager built on top of
`graphic`, `video`, `vbl`, `memory`, and `system`. It owns the standard GodLib
screen canvases and provides convenience macros for drawing into the logical,
physical, and background buffers.

Files:

- `screen.h` - public buffer indices, scroll flags, `sScreenClass`, global
  canvas declarations, and the `Screen_Logic_*`, `Screen_Physic_*`, and
  `Screen_Back_*` drawing macros.
- `screen.c` - buffer allocation, canvas setup, video mode setup, screen
  swapping, frame pacing, and cleanup.

Buffer model:

- Four buffer slots are named:
  - `eSCREEN_PHYSIC`
  - `eSCREEN_LOGIC`
  - `eSCREEN_BACK`
  - `eSCREEN_MISC`
- In normal non-scroll mode, `Screen_Init()` allocates one contiguous screen
  memory block for three buffers:
  - physical
  - logical
  - background
- `eSCREEN_MISC` exists in the enum and has a canvas, but current
  `Screen_Init()` leaves its buffer and canvas VRAM as `0`.
- `Screen_GetpPhysic()` returns the buffer currently being displayed.
- `Screen_GetpLogic()` returns the opposite buffer, selected by
  `mPhysicIndex ^ 1`.
- `Screen_GetpBack()` returns the fixed background buffer.

Initialisation:

- `Screen_Init(width, height, bitDepth, scrollFlags)` clears `gScreenClass`.
- It initialises four `sGraphicCanvas` objects with `GraphicCanvas_Init()`.
- It computes the buffer size from `gScreenLogicGraphic.mpLineOffsets[1]`
  multiplied by canvas height.
- It allocates screen memory through `mMEMSCREENCALLOC()`, which maps to
  `Memory_ScreenCalloc()` and therefore uses ST RAM on Atari.
- The returned memory block is aligned up to a 256-byte boundary before the
  hardware-facing buffers are assigned.
- It assigns `mpVRAM` for logic, physical, and background canvases.
- It sets frame rate to 1 VBL and calls `Screen_Update()` once immediately.

Vertical scroll mode:

- If `eSCREEN_SCROLL_V` is passed, the canvas height becomes `height + 32`.
- The visible video mode still uses the requested visible height.
- `Video_SetResolution()` receives the wider/taller canvas width separately, so
  video code can know the real line layout.
- `Screen_SetScrollY(y)` stores a line offset in `gScreenClass.mScrollY`.
- `Screen_Update()` adds `mpLineOffsets[1] * mScrollY` to the physical screen
  pointer before displaying it.
- On STE/TT/Falcon the VBL video update path writes the full video base
  including the low byte register, so this can scroll by individual lines.
- On plain ST the hardware screen base lacks the low-byte register, so a
  320px/4-plane screen effectively reaches clean line starts every 8 lines.

Horizontal scroll mode:

- `eSCREEN_SCROLL_H` is defined in `screen.h`, but `screen.c` currently has no
  implementation for it.
- The lower-level `video` module does have STE-style horizontal scroll support
  through `Video_SetScrollX()` / `Video_UpdateRegsSTE()`, but `Screen_Init()`
  and `Screen_Update()` do not connect `eSCREEN_SCROLL_H` to that path.
- Missing work: decide how much extra canvas width/line padding horizontal
  scroll should allocate, expose/store a screen-level X scroll value, and route
  updates to the video scroll registers.

Update/flip:

- `Screen_Update()` toggles `mPhysicIndex` every call.
- It points the hardware/video layer at the new physical buffer through
  `Video_SetPhysic()`.
- On ST hardware it also writes the screen base to `$FFFF8200` after shifting
  the address as expected by the ST shifter.
- It waits until `mFrameRate` VBLs have elapsed since the previous update.
- After waiting, it updates:
  - `gScreenLogicGraphic.mpVRAM` to the non-visible buffer
  - `gScreenPhysicGraphic.mpVRAM` to the visible buffer
- This is why normal frame code draws into `Screen_Logic_*`, then calls
  `Screen_Update()`.

Drawing API:

- The macros in `screen.h` are thin wrappers around the active function tables
  in the global canvases.
- `Screen_Logic_*` draws into the next frame.
- `Screen_Physic_*` draws directly into the currently displayed buffer.
- `Screen_Back_*` draws into the fixed background/restoration buffer.
- Each group exposes normal and clipped variants for blit, copy, draw box,
  line, pixel, sprite, quad/tri, and font print.

Important limitations and risks:

- `eSCREEN_SCROLL_H` is defined but not implemented in `screen.c`.
- `Screen_Init()` does not check whether `GraphicCanvas_Init()` or
  `mMEMSCREENCALLOC()` succeeded before using the resulting pointers.
- In vertical scroll mode, `lTotal` is `lSize * 4`, but only three buffers are
  assigned and `eSCREEN_MISC` remains `0`; this looks like leftover/unused
  allocation.
- `Screen_Update()` flips buffers even on the first call from `Screen_Init()`,
  so the initial physical/logical index after init is already toggled once.
- `Screen_SetScrollY()` does not clamp the scroll value; callers must keep it
  within the extra scroll area.
- The direct ST write to `$FFFF8200` is guarded by `dGODLIB_PLATFORM_ATARI`,
  but the surrounding logic still relies on screen addresses being valid for
  the selected machine/video mode.

## audio

`godlib/audio` contains low-level sound support: YM/PSG state handling, STE/TT
and Falcon DMA sample playback, a small two-channel sample mixer, SPL asset
relocation, and SSD replay glue.

Files:

- `audio.h` / `audio.c` - core DMA sound API, YM save/restore integration,
  keyclick/internal speaker state, volume control, sample conversion helpers,
  and `sAudioDmaSound` relocation.
- `audio_s.s` - hardware helpers for YM save/restore, DMA register
  save/restore, sound chip silence, replay-end interrupt, and MicroWire writes.
- `amixer.h` / `amixer.c` - two-channel DMA sample mixer front end.
- `amixer_s.s` - VBL mixer implementation and mixer-owned globals.
- `am_sine.h`, `am_cos.h`, `ampanlaw.h` - pan/gain lookup tables.
- `amix_bld.c` - helper used to build mixer lookup data.
- `rel_spl.h` / `rel_spl.c` - asset relocator for `SPL` sample assets.
- `ssd.h` / `ssd.c` / `ssd_s.s` - SSD music replay wrapper and Timer C bridge.

Core DMA audio:

- `Audio_Init()` saves YM registers, saves DMA audio registers on STE/MegaSTE/TT
  or Falcon, disables keyclick, enables internal speaker, clears the DMA playing
  flag, and silences the sound chip.
- `Audio_DeInit()` restores keyclick/internal speaker state, silences the chip,
  restores DMA registers for the machine type, and restores YM registers.
- `sAudioDmaSound` describes one DMA sample:
  - `mpSound`
  - byte length
  - frequency enum (`6`, `12`, `25`, `50` kHz family)
  - bit depth
  - looping flag
  - stereo flag
- `Audio_DmaPlaySound()` writes STE-style DMA start/end registers, sound mode,
  and replay control when DMA audio hardware is available. It always caches the
  current sound descriptor in `gAudioCurrentSound`.
- `Audio_GetFrequency()` maps frequency enum values to approximate hardware
  rates: `6258`, `12517`, `25033`, `50066`.
- `Audio_DmaIsSoundPlaying()` checks DMA control/current/end registers when
  hardware DMA is available.

Volume and sample manipulation:

- STE/MegaSTE/TT volume is written through MicroWire commands.
- Falcon volume uses Falcon codec volume registers.
- `Audio_ToggleSign()` adds `0x80` to each byte to switch signed/unsigned
  sample interpretation.
- `Audio_MaximiseVolumeSigned()` and `Audio_MaximiseVolumeUnSigned()` both call
  `Audio_MaximiseVolume(apSound, 0x80)` in the current code.
- `Audio_ScaleVolumeSigned()` scales signed 8-bit sample data in-place.
- `Audio_ScaleVolumeUnSigned()` is currently a stub.

SPL relocator:

- `Relocator_SPL_Init()` registers asset type `SPL`.
- Delocation stores `sAudioDmaSound.mpSound` as an offset from the asset base
  and endian-swaps the pointer and length.
- Relocation endian-swaps pointer/length back and rebuilds the absolute sample
  pointer.
- On Falcon, `Relocator_SPL_DoInit()` halves each signed sample byte, probably
  to reduce output level for Falcon playback.

Audio mixer:

- `AudioMixer_Init()` supports STE/MegaSTE/TT/Falcon DMA machines.
- It allocates a 4 KiB silence buffer and an 8 KiB mixing buffer plus extra
  padding with `mMEMSCREENCALLOC()`.
- The active mixer supports two software channels (`dAMIXER_CHANNEL_LIMIT = 2`).
- `AudioMixer_Enable()` installs `AudioMixer_Vbl()` in the VBL queue and starts
  looping DMA playback of the mixer buffer at 25 kHz stereo 8-bit.
- `AudioMixer_Vbl()` reads the current DMA frame pointer and mixes the next
  chunk of audio into the circular mixer buffer.
- `AudioMixer_PlaySample()` plays a sample on the first inactive mixer channel.
- `AudioMixer_PlaySampleDirect()` steals the channel closest to finishing if no
  channel is free.
- Panning can be linear, constant-power-ish, or pan-law table based.

SSD:

- `Ssd_Init()` clears state, enables the SSD class, and defaults the song
  frequency to 50 Hz.
- `Ssd_Start()` calls the SSD replay start vector and hooks Timer C.
- `Ssd_Stop()` calls the replay stop vector and disables Timer C.
- `Ssd_SetSongFreq(hz)` configures Timer C with `12288 / hz`, mode `7`, and
  `Ssd_RepPlay` as callback.
- `ssd_s.s` bridges into a built-in replay blob through a vtable at offset 28.

Important limitations and risks:

- `Audio_ScaleVolumeUnSigned()` is not implemented.
- `Audio_DmaPlaySound()` now rejects null/empty sample descriptors, but still
  does not validate enum values or ST-RAM placement.
- `AudioMixer_SetConfig()` disables and immediately re-enables the mixer; callers
  should avoid calling it while relying on uninterrupted playback.
- `AudioMixer_Init()` now detects mixer buffer allocation failure and disables
  mixer DMA availability for that instance.
- Backup/previous mixer files in the directory should not be treated as active
  build inputs.
- SSD and SND both use Timer C; they will conflict unless users explicitly
  sequence ownership of that timer.

## music

`godlib/music` contains music/replay wrappers rather than one unified music
engine. The current set includes LanceMod, Wizzcat MOD replay, PinkNote PSG
effect sequences, and generic SND replay glue.

Files:

- `lancemod.h` / `lancemod.c` - ProTracker MOD loader and wrapper for Lance
  Paula replay.
- `lancemod_s.s`, `lancepaula.s`, `lancetracker.s` - Lance replay assembly,
  Paula-style mixer, and tracker engine.
- `wizzcat.h` / `wizzcat.c` - ProTracker MOD loader and wrapper for Wizzcat/Delta
  Force STE replay.
- `wizzcat_s.s` - Wizzcat replay assembly and VBL entry point.
- `pinknote.h` / `pinknote.c` - small PSG note/effect sequencer.
- `pnknot_s.s` - PinkNote player and PSG channel writers.
- `snd.h` / `snd.c` - generic SND header parser and Timer C playback setup.
- `snd_s.s` - register-saving trampoline for SND play/chaser callbacks.

LanceMod:

- `LanceMod_Load()` loads a MOD file into memory and appends
  `dLANCEMOD_EXTRA_BUFFER_SIZE` bytes of replay workspace.
- `LanceMod_InitPaula(freq)` allocates/reuses a Paula replay buffer sized by
  frequency:
  - 12 kHz -> 500 samples
  - 25 kHz -> 1000 samples
  - 50 kHz -> 2000 samples
- `LanceMod_Start(mod, freq)` initialises Paula/replay state, sets master volume
  to 64, and installs `LanceMod_Play()` in the VBL queue.
- `LanceMod_StopVbl()` removes the VBL callback and calls `LanceMod_Stop()`.
- `LanceMod_ShutdownPaula()` frees the Paula replay buffer.
- The assembly wrapper preserves registers around `paula_calc`, `mt_music`,
  `mt_init`, `mt_end`, and volume/debug helpers.

Wizzcat:

- `Wizzcat_Load()` loads a MOD file into an `sWizzcatModule` and appends
  the default `128 KiB` workspace margin.
- `Wizzcat_LoadEx(filename, workspaceSize)` allows callers to choose a smaller
  or larger sample-preparation workspace for memory-constrained programs.
- `Wizzcat_Init()` calls `WIZinit()` and `WIZmodInit(moduleData, workspaceEnd)`.
- `Wizzcat_Start()` initialises, primes replay with `Wizzcat_Play()`, and adds
  `Wizzcat_Vbl()` to the VBL queue.
- `Wizzcat_Stop()` directly stops STE DMA sound by clearing `$FFFF8901`.
- `Wizzcat_GetInfo()` returns song and pattern position.
- `Wizzcat_Jump()` jumps to a 1-based song position.

PinkNote:

- PinkNote is a three-channel PSG note/effect sequencer.
- Note data is encoded as small `sPinkNote` commands:
  - volume
  - noise/tone enable
  - tone frequency
  - noise frequency
  - envelope
  - pause
  - loop
  - end
- `PinkNote_Init()` initialises all channels, clears queues, and installs
  `PinkNote_Player` as the SND chaser callback.
- `PinkNote_Update()` moves queued notes into inactive or looping channels and
  ages queue entries.
- `PinkNote_PlayNote(note, channel, priority)` queues a note sequence for a PSG
  channel.

SND:

- `Snd_GetInfo()` parses an SND header, extracts metadata tags (`COMM`, `CONV`,
  `RIPP`, `TITL`) and timer tags (`TA`, `TB`, `TC`, `TD`, `V!`).
- If no timer tag is found, it defaults to Timer C at 50 Hz.
- `Snd_TuneInit()` sets the current play function, calls the tune start entry,
  and hooks Timer C through `Mfp_HookIntoTimerC()`.
- `Snd_TuneDeInit()` disables Timer C and calls the tune stop entry.
- `Snd_Player()` in assembly preserves registers, calls the current SND play
  function, then calls the chaser function. PinkNote uses that chaser slot.

Important limitations and risks:

- LanceMod and Wizzcat both use VBL callbacks; callers must stop/remove them
  before unloading module memory.
- Wizzcat is STE DMA oriented and directly touches `$FFFF8901` in `Wizzcat_Stop()`.
- LanceMod owns a persistent Paula replay buffer that is not freed by
  `LanceMod_StopVbl()`; call `LanceMod_ShutdownPaula()` when done for good.
- Wizzcat's default loader allocates a large 128 KiB workspace margin after
  every module. Use `Wizzcat_LoadEx()` to tune this per program/module, but too
  small a workspace can still make the replay `illegal` during sample prepare.
- SND uses Timer C and can conflict with SSD or other Timer C users.
- SND parsing scans up to `mStart.mOffset` and assumes valid embedded tag data.
- PinkNote queues are fixed size and `PinkNote_QueueInit()` appears to clear
  only `dPINKNOTE_CHANNEL_LIMIT` entries per channel instead of the full
  `dPINKNOTE_QUEUE_LIMIT`; this should be reviewed before relying on all 16
  queue slots.

## font

`godlib/font` implements sprite-based proportional/fixed fonts. A font is built
from a `sSpriteBlock`, stores per-character sprite regions, and can be relocated
as a `BFB` asset.

Files:

- `font.h` / `font.c` - `sFont`, font construction/destruction, character
  metrics, string width helpers, sprite lookup, relocation, and debug info.
- `rel_bfb.h` / `rel_bfb.c` - asset relocator for `BFB` font assets.

Font model:

- `sFont` stores:
  - ID/version
  - first and last covered character code
  - kerning and space width
  - maximum width/height
  - sprite count
  - character map
  - copied sprites
  - sprite regions
- `Font_Create(spriteBlock, charMap, fixedWidth)` creates one contiguous memory
  block containing the `sFont`, copied sprites, regions, sprite data, and compact
  character map.
- `charMap` maps character codes to sprite indices by position in the string.
  The current font example uses character codes `1..127`.
- If `fixedWidth` is non-zero, every region becomes `0..fixedWidth-1` by the
  sprite height. Otherwise `Sprite_GetRegion()` is used to crop each glyph to
  its non-transparent bounds.
- `mSpaceWidth` is separate from glyph sprites and defaults to `0` until callers
  set it.
- `mKerning` defaults to `2`.

Metrics and printing support:

- `Font_GetCharWidth()` returns:
  - `mSpaceWidth` for space
  - region width for mapped characters
  - `mSpaceWidth` for unknown characters
- `Font_GetStringWidth()` sums character widths plus kerning between characters.
- `Font_GetStringCharX()` returns the x offset of a character index inside a
  string.
- `Font_GetpSprite()` and `Font_GetpSpriteRegion()` return the glyph sprite and
  visible region used by `Graphic_FontPrint()`.
- Actual drawing is in `graphic`: `Graphic_FontPrint()`,
  `Graphic_FontPrintLeft()`, `Graphic_FontPrintRight()`, and
  `Graphic_FontPrintCentred()`.

BFB relocation:

- `Relocator_BFB_Init()` registers asset type `BFB`.
- Delocation calls `Font_Delocate()`.
- Relocation calls `Font_Relocate()`.
- `Font_Delocate()` converts internal pointers to offsets and endian-swaps font
  fields, sprite regions, and sprites.
- `Font_Relocate()` endian-swaps fields back and rebuilds internal pointers.

Important limitations and risks:

- `Font_Delocate()` / `Font_Relocate()` should keep `mpCharMap`, `mpRegions`,
  and `mpSprites` as independent offsets. The GCC port had a regression where
  `mpRegions` was derived from `mpSprites`; this has been corrected to match the
  original PureC source.
- `Font_GetpSpriteRegion()` does not check whether the mapped index is below
  `mSpriteCount`; `Font_GetpSprite()` does.
- `Font_GetCharWidth()` assumes mapped character indices are valid.
- `Font_Create()` always processes `apCharMapString[0]` because of
  `while( ((!i) || (apCharMapString[i])) && (i<256) )`; empty or null character
  maps should be avoided.
- `mWidthMax` and `mHeightMax` are computed as `x1-x0` and `y1-y0`, not
  inclusive widths/heights.
- The directory currently contains active object files, and `font8x8` contains a
  `.font8x8.c.swp` editor swap file.

## font8x8

`godlib/font8x8` is a built-in, fixed 8x8 debug/UI font for ST-low 4-plane
screens. It writes directly into planar screen memory and does not use
`sGraphicCanvas`.

Files:

- `font8x8.h` / `font8x8.c` - public print functions and ST-low planar drawing.
- `fontdata.c` - built-in glyph data array `gFont8x8[12544]`.

Printing model:

- `Font8x8_Print(text, screen, x, y)` writes the glyph bytes directly to screen
  memory.
- `Font8x8_PrintColour(text, screen, x, y, colour)` writes glyph pixels into all
  four bitplanes according to the low 4 bits of `colour`.
- The code assumes:
  - 320-pixel ST low-res layout
  - 160 bytes per screen line
  - 4 interleaved bitplanes
  - 8x8 glyphs
- `x` is effectively expected on 8-pixel boundaries. The implementation handles
  `x & 8` by selecting the alternate byte inside a 16-pixel planar group.
- The glyph index is `(*text - 32) * 8`, so the font is intended for printable
  character codes starting at ASCII space.

Important limitations and risks:

- `font8x8` is not canvas-generic; it hard-codes `160` bytes per line and is
  therefore tied to 320px ST-low 4-plane screens.
- It does no clipping. Text outside the screen or near the right/bottom edge can
  write out of bounds.
- `Font8x8_Print()` writes only one byte per row, effectively plane-0 style
  output. `Font8x8_PrintColour()` is the safer choice when a specific palette
  colour is needed.
- Character codes below 32 or beyond the table's intended range can index
  unexpected glyph data.
- It writes whole 8x8 cells; background pixels inside the cell are cleared to
  colour 0 by `Font8x8_PrintColour()`.

## packer

`godlib/packer` is a collection of depackers and tool-side packers. The public
entry point is `Packer_*`, but not every codec in the directory is wired into
that dispatcher.

Files:

- `packer.h` / `packer.c` - generic packer detection, size helpers, depack
  dispatcher.
- `packer_s.s` - old ST depackers for Ice, Atomic, Auto5, and Speed3.
- `godpack.h`, `godpack.c`, `godpackp.c` - GodPack format, bitstream helpers,
  pack and depack pipeline.
- `rnc.h` / `rnc.c`, `rnc1_s.s`, `rnc2_s.s` - Rob Northen compression method 1
  and 2 depackers.
- `rle.c`, `lz77*_*.c`, `bwt_*.c`, `mtf_*.c`, `ari_*.c` - GodPack transform
  stages and older experimental stages.
- `brun1.c` / `brun1.h` - ByteRun1 depacker, used by `pictypes/iff.c`.
- `aplib.h`, `aplib_s.s`, `lz4.h`, `lz4_s.s` - standalone depackers; currently
  not recognized by `Packer_GetType()`.

Dispatcher behavior:

- `Packer_GetType()` recognizes:
  - `ICE!` as `ePACKER_ICE`
  - `ATM5` as `ePACKER_ATOMIC`
  - `AU5!` as `ePACKER_AUTO5`
  - `SPv3` as `ePACKER_SPEED3`
  - `GDPK` as `ePACKER_GODPACK`
  - `RNC1` and `RNC2` as `ePACKER_RNC`
- `Packer_GetDepackSize()` knows the unpacked-size offsets for the legacy ST
  formats and delegates to `GodPack` / `Rnc` for those formats.
- `Packer_GetHeaderSize()` returns non-zero only for `GODPACK` and `RNC`.
- `Packer_GetLoadOffset()` is only meaningful for `GODPACK`.
- `Packer_Depack(src, dst)` uses `dst` for `GODPACK` and `RNC`, but the old ST
  depackers ignore `dst` and operate on `src` in their original in-place style.

GodPack:

- Header is `sGodPackHeader`: `GDPK`, version, packed size, unpacked size, and
  intermediate stage size.
- `GodPack_Pack()` currently encodes `RLE -> LZ77B` and writes a `GDPK` header.
- `GodPack_DePack()` decodes `LZ77B -> RLE`.
- On Atari builds, the decode stages use assembler (`GodPack_Lz77b_Decode_Asm`,
  `GodPack_Rle_Decode_Asm`); host builds use the C versions.
- In-place depacking is supported by placing the packed stream near the end of a
  `unpacked size + dGODPACK_OVERFLOW` buffer.

RNC:

- Header size is 18 bytes.
- `Rnc_GetVersion()` accepts `RNC1` and `RNC2`.
- `Rnc_GetDepackSize()` and `Rnc_GetPackedSize()` read big-endian fields from
  the header.
- `Rnc_Depack()` dispatches to assembler `Rnc_Depack1()` or `Rnc_Depack2()`.
- CRC fields are present in the header structure, but the C wrapper does not
  validate them.

Important limitations and risks:

- `Packer_GetType()` used to read `apHeader->m0` before checking for `NULL`;
  this has been corrected.
- The old Ice/Atomic/Auto5/Speed3 paths are legacy in-place depackers. Passing a
  separate destination buffer to `Packer_Depack()` will not affect those paths.
- Most depackers trust the packed stream and caller-provided destination size.
  They are suitable for trusted game assets, not hostile input.
- `GodPack_Pack()` does not check allocation failures before using intermediate
  buffers.
- Several older transform stages (`BWT`, `MTF`, `ARI`, old `LZ77`) appear to be
  experimental/tool-side code rather than the active `GDPK` pipeline.
- `ByteRun1_DePackLine()` is used by IFF and should be considered part of the
  image loading path, even though it lives in `packer`.

## pictypes

`godlib/pictypes` contains picture formats and conversion helpers. The central
idea is converting file-specific structures to either a true-colour `sCanvas` or
an indexed-colour `sCanvasIC`, then converting those to ST planar or other file
formats.

Main building blocks:

- `canvas.h` / `canvas.c` - true-colour canvas. Pixels are `uCanvasPixel`
  (`r,g,b,a` bytes plus `U32` view).
- `canvasic.h` / `canvasic.c` - indexed-colour canvas with a 256-entry palette.
- `colquant.*` and `octtree.*` - colour quantization helpers.
- `gfx.*` - Reservoir Gods `GFX ` sprite/image container with generated mask
  plane.
- `rel_gsm.*` and `gsm.*` - asset relocator and minimal `GSM` container support.

Implemented format paths:

- `Degas_ToCanvas()` supports uncompressed Degas `PI1/PI2/PI3` and compressed
  `PC1/PC2/PC3`.
- `Degas_GetPixel()` / `Degas_SetPixel()` can access planar pixels for modes
  0, 1, and 2.
- `Neo_ToCanvas()` converts Neochrome low-res images to `sCanvas`.
- `Art_ToCanvas()` converts Art Director low-res images to `sCanvas`.
- `God_ToCanvas()` / `God_FromCanvas()` convert a simple GodPaint-style 16-bit
  565 image.
- `Tga_ToCanvas()` handles several TGA variants, including palette and RLE
  paths.
- `Iff_ILBM_Parse()`, `Iff_ILBM_CmapToSTE()`, and
  `Iff_ILBM_DecodeToSTLow()` parse ILBM and decode 4-plane ST-low data. BODY
  can be raw, ByteRun1, or VDAT.
- `AmigaRaw_ToCanvas()` and `AmigaRaw_To4Plane()` convert a fixed Amiga raw
  4-plane format to ST-style data.

Partially implemented or stubbed areas:

- `Canvas_Load()`, `Canvas_LoadAt()`, and `Canvas_Save()` are declared but
  compiled out with `#if 0`.
- `CanvasIC_Load()`, `CanvasIC_LoadAt()`, `CanvasIC_Save()`,
  `CanvasIC_Palettize()`, and `CanvasIC_ReduceColourDepth()` are also compiled
  out.
- `CanvasIC_FromCanvas()` currently returns `0`, so many `*_FromCanvas()` save
  paths that rely on it are effectively disabled.
- `CanvasIC_From565()` is a stub that returns success without creating image
  data.
- `Gfx_FromCanvas()` and `Gfx_ToCanvas()` are declared but not implemented in
  `gfx.c`.
- `Gsm_FromCanvas()` and `Gsm_ToCanvas()` are stubs; `GSM` currently only has
  endian relocation and asset registration.
- `Gif_ToCanvas()` is not a real decoder. There is GIF encoding code, but the
  canvas conversion/save path depends on the disabled indexed-canvas path.

Important limitations and risks:

- Many create functions assume allocation succeeds before dereferencing the
  returned pointer. This is normal for old tool code, but not robust.
- Several conversion functions allocate temporary canvases and then immediately
  overwrite the pointer with a second created canvas, leaking the first small
  descriptor.
- Some compressed image decoders trust the source stream length and only limit
  output size. They are fine for trusted assets but not safe file importers.
- `Canvas_ImageTo565()` appears to write blue from `r` (`lpSrc->b.r >> 3`)
  rather than `b`. `CanvasIC_To565()` uses `b`, so this may be an old bug in the
  true-colour path.
- `Degas_Pc*ToCanvas()` appears to call `Canvas_SetPixel(lpCanvas, i, j, ...)`
  while iterating `i` as Y and `j` as X, so compressed Degas conversion may have
  swapped coordinates.
- `CanvasIC_To565()` builds only the first 16 palette entries even though the
  palette and indexed pixels support 256 colours.
- `Relocator_GSM_IsType()` always returns true. It relies on the asset extension
  registration (`"GSM"`) to scope usage.

## chunky

`godlib/chunky` is the 8bpp chunky-pixel drawing backend for `graphic`. It also
contains old assembler experiments for converting between 8bpp chunky and ST
4-plane bitplanes.

Files:

- `chunky.h` / `chunky.c` - public `ChunkySurface_*` functions used by the
  `graphic` function table when `dGODLIB_CHUNKY` is enabled.
- `chunky_s.s` - assembler C2P/P2C routines named `C2P_To4P` and
  `C2P_From4P`.
- `c2p_s.s` - another copy/variant of the same assembler conversion code.
- `chunky.s` - old assembler source fragment; not part of the current public C
  API.

Active C backend:

- `ChunkySurface_Blit()` copies an 8bpp rectangular region.
- `ChunkySurface_ClearScreen()` clears the whole chunky buffer to colour 0.
- `ChunkySurface_CopyScreen()` copies a raw 8bpp screen-sized buffer into the
  canvas.
- `ChunkySurface_DrawBox()`, `DrawLine()`, `DrawPixel()`, and `DrawSprite()`
  draw directly into one byte per pixel.
- Sprites use colour `0xFF` as transparent in the chunky path.
- `ChunkySurface_ConvertBlit()` only handles:
  - source `eGRAPHIC_COLOURMODE_4PLANE` by calling `ChunkySurface_From4Plane()`
  - source `eGRAPHIC_COLOURMODE_8BPP` by using regular chunky blit

Clipping:

- `ChunkySurface_Blit_Clip()` and `ChunkySurface_ConvertBlit_Clip()` clip
  against full canvas width/height and source width/height.
- `ChunkySurface_DrawBox_Clip()` clips against `apCanvas->mClipBox`.
- `ChunkySurface_DrawPixel_Clip()` clips against `apCanvas->mClipBox`.
- `ChunkySurface_DrawSprite_Clip()` clips against full canvas width/height, not
  `mClipBox`.

Stubbed or incomplete areas:

- `ChunkySurface_DrawTri()`, `DrawQuad()`, and `FontPrint()` are stubs.
- The clipped variants of tri/quad are also stubs.
- `ChunkySurface_From4Plane()` has an empty inner loop in C.
- `ChunkySurface_To4Plane()` is a stub.
- The assembler routines are named `C2P_To4P` / `C2P_From4P`, not
  `ChunkySurface_From4Plane` / `ChunkySurface_To4Plane`, so the public C API
  currently does not call them directly.

Important fixes made:

- `ChunkySurface_DrawBox_Clip()` had the same old expression as the original
  PureC source: `lDiff = lRect.mX = clipX0`. This forced the clipped rectangle
  to start at `clipX0` before calculating the difference. It has been changed
  to calculate `clipX0 - lRect.mX`, matching the corrected 16bpp backend style.
- `ChunkySurface_DrawSprite_Clip()` changed width/height whenever the right or
  bottom difference was non-zero. Negative differences expanded the blit for
  sprites that were already fully inside the canvas. The conditions are now
  `lDiff > 0`.

Important limitations and risks:

- `ChunkySurface_DrawLine_Clip()` is hand-written clipping code and has edge
  cases around vertical/horizontal lines because it divides by `lDx`/`lDy`.
- The blit clipping helpers use canvas dimensions rather than `mClipBox`, unlike
  some other backends.
- `ChunkySurface_DrawSprite_Clip()` clips against full canvas bounds, not
  `mClipBox`.
- The old assembler `C2P_From4P` appears suspicious: it calculates the chunky
  pixel in `d7` but stores `d4`. `c2p_s.o` also currently contains an undefined
  `chunkLoop` symbol because one branch lacks the local-label dot. Since these
  symbols are not the public `ChunkySurface_*` API, treat them as unfinished
  historical code until there is a concrete user.
- The C 4-plane conversion functions are effectively not implemented, so
  `ChunkySurface_ConvertBlit()` from 4-plane to chunky currently does not
  produce pixels through the public API.

## scrngrab

`godlib/scrngrab` is a small debug/helper module that saves the current physical
screen as an uncompressed Degas `.PI1` file when a selected IKBD key is pressed.

Files:

- `scrngrab.h` / `scrngrab.c` - public API, key state, file naming, and `.PI1`
  write code.
- `scrgrabs.s` - VBL trampoline that calls `ScreenGrab_Update()`.

Runtime model:

- `ScreenGrab_Init()` resets state and registers `ScreenGrab_Vbl()` through
  `Vbl_AddCall()`.
- `ScreenGrab_DeInit()` removes the VBL callback.
- `ScreenGrab_Enable()` arms the module and clears the key latch.
- `ScreenGrab_Disable()` disables the module.
- The default trigger is `eIKBDSCAN_F8`.
- `ScreenGrab_SetKeyIndex()` changes the trigger key.
- `ScreenGrab_SetDirectory()` copies up to 255 characters into the global
  directory prefix.
- `platform/platform.c` currently initializes it with directory
  `"SCRNGRAB\\"` and enables it.

Save format:

- Filename is `"<directory>GRAB%04x.PI1"` using `gScreenGrabIndex`.
- The index is a `U16`, so filenames wrap after `GRABffff.PI1`.
- The file is written as:
  - `sDegasHeader` with mode `0`
  - current ST palette from `Video_GetPalST()`
  - 32000 bytes from `Video_GetpPhysic()`
- This is specifically ST-low 320x200 4-plane Degas PI1 output.

VBL path:

- `ScreenGrab_Vbl()` first checks `gScreenGrabEnableFlag`.
- It uses `tas gScreenGrabVblLockFlag` as a re-entry guard.
- If enabled and not locked, it saves registers, calls `ScreenGrab_Update()`,
  restores registers, clears the lock, and returns.
- `ScreenGrab_Update()` latches the trigger key so holding the key saves only
  one frame until the key is released.

Important fixes made:

- `ScreenGrab_Disable()` originally set `gScreenGrabEnableFlag = 1`, which kept
  screen grabbing enabled. It now sets the flag to `0`.
- `scrngrab.c` now includes `<stdio.h>` for `sprintf()`.

Important limitations and risks:

- `ScreenGrab_Update()` performs file creation and file writes from the VBL
  callback. That is risky on real hardware and any setup where disk I/O is not
  interrupt-safe.
- The code assumes a valid ST-low physical screen and always writes 32000 bytes.
  It does not check current video mode, line width, STE scroll/offset, or
  virtual screen height.
- It writes the physical screen, not necessarily the logical/back buffer.
- It does not create the target directory itself; the configured directory must
  already exist.
- `sprintf()` is safe for the current 256-byte directory buffer plus fixed
  filename because `lFileName` is 268 bytes, but changing these constants would
  need care.
- There is no collision handling beyond incrementing the index. Existing files
  with the same name are passed to `File_Create()` according to that function's
  behavior.

## thread

`godlib/thread` is a header-only cooperative threading helper. It is not an OS
thread system; it is a macro-based protothread/state-machine layer built around
saving a program counter into `sThread.mPC`.

Files:

- `thread/thread.h` - all structures, states, and macros. There is no `.c`
  implementation.

Core structures:

- `sThread` stores:
  - `mPC` - saved resume point
  - `mSleepTicks` - countdown used by `mTHREAD_SLEEP`
  - `mState` - one of `eTHREAD_STATE_*`
  - `mFlags` - init/running flags
- `sThreadSemaphore` stores a `U32 mCount`.
- `sThreadObject` bundles an `sThread` with an `fThread` callback.

Execution model:

- A thread function is usually `U8 Func(sThread * apThread)`.
- It must start with `mTHREAD_BEGIN(apThread)` and finish with
  `mTHREAD_END(apThread)`.
- Yield/wait/sleep macros save `__LINE__` into `mPC` and return a state.
- On the next call, `mTHREAD_BEGIN()` switches on `mPC` and jumps back to the
  saved `case __LINE__`.
- Callers drive the thread by repeatedly calling the function once per update,
  VBL, parser step, etc.

Important public macros:

- `mTHREAD_INIT()` / `mTHREAD_DEINIT()` initialize or clear an `sThread`.
- `mTHREAD_WAIT_UNTIL()` / `mTHREAD_WAIT_WHILE()` pause until a condition is
  true/false.
- `mTHREAD_YIELD()` yields exactly once.
- `mTHREAD_YIELD_UNTIL()` / `mTHREAD_YIELD_WHILE()` yield for at least one call,
  then keep yielding while the condition requires it.
- `mTHREAD_SLEEP()` yields `n` update calls.
- `mTHREAD_SUSPEND()` parks a thread until someone calls `mTHREAD_RESUME()`.
- `mTHREAD_SPAWN()` initializes and waits for a child thread.
- Semaphore macros provide a tiny counting semaphore wrapper over wait/signal.

Current users:

- `achieve/ach_logn.c` uses thread objects for login UI animations.
- `achieve/ach_disp.c` uses threads for score/task/unlock display flows.
- `achieve/ach_gfx.c` uses a thread for beam animation.
- `achieve/ach_com.c` uses temporary threads as line/parser consumers.
- `cutscene/cut_sys.c` has its own cutscene thread concept, but it does not use
  `godlib/thread/thread.h`.

Important fixes made:

- `mTHREAD_INIT()` now assigns `mFlags = eTHREAD_FLAG_INITED` instead of OR-ing
  the bit into whatever was already in memory. This matters because some real
  users initialize stack `sThread` variables without first zeroing them.
- `mTHREAD_DEINIT()` now clears all flags instead of only clearing the init bit.
- `mTHREAD_HASFINISHED()` now compares with `eTHREAD_STATE_ENDED`; the original
  macro referenced nonexistent `aTHREAD_STATE_EXITED`.
- `mTHREAD_SEMAPHORE_INIT()` now uses its `_aValue` argument; the original macro
  referenced nonexistent `aValue`.

Important limitations and risks:

- Yield/wait macros use `__LINE__`, so two thread context-save macros must not
  appear on the same source line inside one thread function.
- Local automatic variables are recreated on every call. Values that must
  survive across a yield need to live in external state, static storage, or a
  caller-owned object.
- `switch/case` is generated inside `mTHREAD_BEGIN()`, so placing these macros
  inside some C control-flow constructs can be surprising. Existing code uses
  them in simple, regular patterns.
- `eTHREAD_FLAG_RUNNING` is effectively unused; the verify macro checks for it,
  and return clears it, but begin never sets it.
- `mTHREAD_SPAWN()` is compact but subtle: the child is initialized before the
  parent's wait save point, so it is initialized once on first entry and not
  reinitialized after the parent resumes.
- Semaphore macros are not atomic. They are cooperative-thread helpers only.

## cookie

`godlib/cookie` wraps the Atari TOS cookie jar at address `$5A0`. The module is
small, but it is important because other modules use cookies to detect machine,
CPU, video, sound, and extension capabilities.

Files:

- `cookie/cookie.h` - `sCookie`, cookie-jar base address, public prototypes.
- `cookie/cookie.c` - lookup, existence, value read, and value write helpers.

Runtime model:

- On Atari builds, `$5A0` is treated as a pointer to an array of `sCookie`.
- Each entry is `{ mCookie, mValue }`.
- The list ends with an entry whose `mCookie` is `0`.
- The terminator's `mValue` stores the total allocated cookie-jar size.
- On non-Atari/host builds, the module is inert:
  - `CookieJar_Exists()` returns `0`
  - lookup returns `NULL`
  - reads return `0`
  - writes are ignored

Important functions:

- `CookieJar_Exists()` checks whether `$5A0` contains a non-null jar pointer.
- `CookieJar_GetpCookie()` linearly scans the jar until it finds the requested
  cookie key or the zero terminator.
- `CookieJar_CookieExists()` is a boolean wrapper over `CookieJar_GetpCookie()`.
- `CookieJar_GetCookieValue()` returns a cookie value or `0` if not found.
- `CookieJar_SetCookieValue()` updates an existing cookie or appends a new one
  if there is spare room in the jar.

Important fix made:

- `CookieJar_SetCookieValue()` previously appended a new cookie by overwriting
  the zero terminator and did not create a new terminator. That corrupts the jar
  after the first inserted cookie. It now checks the terminator capacity, writes
  the new cookie only when there is room, and restores the `{ 0, size }`
  terminator after the new entry.

Important limitations and risks:

- This code directly dereferences low memory and should only do real work on
  Atari builds.
- Cookie IDs and values are raw `U32` values; four-character cookie constants
  need to be supplied by callers in the expected big-endian Atari form.
- `CookieJar_SetCookieValue()` silently does nothing when the jar is missing or
  full. That matches the simple legacy API, but callers cannot distinguish
  failure from success.
- The cookie jar is global OS state. Adding or modifying cookies should be done
  carefully and usually only by system/driver-style code.

## gui

`godlib/gui` is a legacy immediate-ish GUI/runtime package. It combines a binary
GUI data format, text parser, hash-tree variable binding, event dispatch, basic
widgets, renderer, text-line editor, and file selector.

Files:

- `gui/gui.h` - public structures and enums for widgets, vars, events, mouse,
  windows, sliders, locks, colours, fills, and the global GUI class.
- `gui/gui.c` - runtime state, focus handling, window stack, input processing,
  actions, sliders, list/window/control layout, event queue, and var reads/writes.
- `gui/guidata.h` / `gui/guidata.c` - binary `sGuiData` relocate/delocate,
  serialisation, asset client setup, hash-tree event registration.
- `gui/guiparse.h` / `gui/guiparse.c` - tokeniser tables and text parser for GUI
  source files.
- `gui/r_gui.h` / `gui/r_gui.c` - renderer for fills, sprites, strings, mouse
  cursor, clipping, and custom button rendering.
- `gui/guiedit.h` / `gui/guiedit.c` - single-line text editing against hash-tree
  string variables.
- `gui/guifs.h` / `gui/guifs.c` - GUI file selector built on dynamic buttons,
  `sString`, GEMDOS DTA data, and the GUI event model.

Data model:

- Parsed GUI data is stored in `sGuiData`, with separate arrays for actions,
  assets, buttons, button styles, colours, cursors, fills, font groups, key
  actions, lists, locks, sliders, values, vars, and windows.
- Runtime data uses direct pointers.
- Serialized data uses 32-bit offsets relative to the `sGuiData` base.
- `GuiData_Relocate()` converts big-endian offsets to live pointers.
- `GuiData_Delocate()` converts live pointers back to big-endian offsets.
- `GuiData_Serialise()` packs live data, pointer arrays, and strings into one
  contiguous block before delocation/save.

Runtime model:

- `Gui_Init()` clears the global `gGuiClass`.
- `Gui_SetpData()` attaches an `sGuiData`, initializes asset clients, registers
  hash-tree vars/events, and opens the first window if present.
- `Gui_Update()` consumes queued GUI events, updates IKBD and mouse state, tracks
  focus/held/text-edit modes, and dispatches actions.
- `RenderGui_Update()` restores the previous mouse background, draws visible
  windows/controls, saves the new mouse background, and draws the cursor.
- Widgets request redraw by setting `mRedrawFlag`, usually to `2` for double or
  multi-buffer redraw.

Important fixes made:

- `guiparse.c` colour parsing now maps `G`, `B`, and `A` to `sGuiColour.mG`,
  `mB`, and `mA`. The original parser wrote all four `R/G/B/A` tags into `mR`.
- `r_gui.c` now initializes the cursor off-flash rate from
  `dRGUI_CURSOR_RATEOFF`; it previously reused `dRGUI_CURSOR_RATEON`.
- `guidata.c` delocation now includes `mpButtons[i].mpOnIKBD`, matching the
  existing relocation and serialisation paths. Without this, serialized button
  key-action bindings could keep a live pointer.
- `gui.c` horizontal scrollbar line-add now increments while `mCanvas.mX` is
  below the maximum. It previously used `>` and only moved further when already
  beyond the allowed range.

Important limitations and risks:

- Serialized pointers are 32-bit offsets and many relocate/delocate macros cast
  through `U32`. That is fine for Atari/GCC ELF targets, but not naturally
  host-64-safe.
- `GuiData_GetArraysSize()` and `GuiData_Serialise()` reserve pointer arrays as
  `count * 4`, which matches the on-disk Atari format but not host pointer size.
- Several slider calculations can still divide by value/canvas ranges computed
  from GUI data. Those look fixable, but changing them should be tested against
  real GUI assets because scrollbar orientation/sign conventions are old and
  non-obvious.
- `GuiData_EventsRegister()` builds hash-tree paths in a 128-byte local buffer
  with `sprintf()`. Long GUI object names can overflow it.
- `guiparse.c` is currently ISO-8859 text, not UTF-8. The colour fix only changed
  ASCII bytes in place.
- `guifs.c` dynamically replaces file-list controls and assumes ownership based
  on `gGuiFS.mAllocFlag`; this should be handled carefully if file selector
  windows are reused or customized.
