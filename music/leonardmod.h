#ifndef INCLUDED_LEONARDMOD_H
#define INCLUDED_LEONARDMOD_H
#include <godlib/base/base.h>
#include <godlib/music/modscope.h>

enum { eLEONARDMOD_FREQ_12K=1, eLEONARDMOD_FREQ_25K=2 };
#define dLEONARDMOD_CHANNEL_COUNT 4
#define dLEONARDMOD_VISUALIZER_IDLE 0x4000

typedef struct sLeonardModModuleInfo
{
    U32 mFileBytes,mModuleBytes,mSampleBytes,mLoadBytes;
    U16 mSampleCount,mPatternCount;
} sLeonardModModuleInfo;

/* Raw four-channel, 31-sample M.K. MOD, at least 1084 header bytes. */
U8 LeonardMod_GetModuleInfo(const void *apHeader,U32 aFileBytes,sLeonardModModuleInfo *apInfo);
void *LeonardMod_Load(const char *apFileName);
void LeonardMod_UnLoad(void *apModData);
U32 LeonardMod_GetPaulaWorkspaceSize(U8 aFrequency);
U32 LeonardMod_GetPaulaAllocatedSize(void);
/* Supervisor mode, STE/MegaSTE, PAL 50 Hz. Caller owns the writable raw MOD.
 * Start prepares samples in place: reload before starting the same MOD again.
 * Start replaces the previous player; invalid rates default to 25K.
 * DMA/trap #0 are exclusive. No automatic VBL callback is installed. */
U8 LeonardMod_Start(void *apModData,U8 aFrequency);
/* Supervisor, non-reentrant. Prefer one PAL VBL callback (see SPL example);
 * Tick preserves caller USP/SR/registers across the native user-mode mixer.
 * Start does not register it automatically. Use VBL OR manual Tick, never both.
 * Detach the callback before Start, Stop or Shutdown, and before MOD unload. */
void LeonardMod_Tick(void);
/* Stop DMA and restore trap #0; workspace remains until Shutdown/next Start. */
void LeonardMod_Stop(void);
void LeonardMod_ShutdownPaula(void);
void LeonardMod_SetRasterDebug(U8 aEnabled);
/* Four coherent retrigger ages, not amplitudes; NULL is a no-op. */
void LeonardMod_GetVisualizer(U16 *apAges);
/* Read-only four pitched/volume-scaled source previews, CH1..CH4.
 * apSamples needs dMODSCOPE_BYTES (256) signed bytes. NULL is a no-op.
 * Call in supervisor main loop; idle channels return zero. */
void LeonardMod_GetOscilloscope(S8 *apSamples);

/* Copy 164 actual stereo PCM frames (328 interleaved signed bytes), from the
 * latest completed mix block. Used by the ICE-style delayed XY display.
 * NULL is a no-op; stopped playback returns zero. No retained pointer. */
void LeonardMod_GetStereoOscilloscope(S8 *apSamples);

#endif
