#ifndef INCLUDED_RJP1_H
#define INCLUDED_RJP1_H
#include <godlib/base/base.h>
/* Richard Joseph RJP1SMOD (.SNG) + RJP1 sample bank (.INS), PAL STE.
 * Caller owns the opaque prepared song. Sources are copied, never modified.
 * One exclusive Paula/DMA owner; stop other replay engines first.
 * All playback calls are supervisor APIs. Start owns its 50 Hz VBL callback.
 * Rates: eRJP1_FREQ_12K, eRJP1_FREQ_25K or eRJP1_FREQ_50K. */
enum {eRJP1_FREQ_12K=1,eRJP1_FREQ_25K=2,eRJP1_FREQ_50K=3};
enum {eRJP1_MIXER_LANCE=0,eRJP1_MIXER_LEONARD=1};
typedef struct sRjp1Song sRjp1Song;
typedef struct sRjp1Info {
    U32 mSourceBytes,mResidentBytes,mSampleBytes,mModulationBytes;
    U16 mSubSongs,mInstruments,mUniqueSamples,mMixer;
} sRjp1Info;
sRjp1Song *Rjp1_Create(const void *sng,U32 sngBytes,const void *ins,U32 insBytes);
sRjp1Song *Rjp1_Load(const char *sngName,const char *insName);
/* Select the mixer before preparing the song; legacy Create/Load use Lance.
 * Start uses the song's mixer. Leonard supports 12.5/25K; Lance also 50K. */
sRjp1Song *Rjp1_CreateEx(const void *sng,U32 sngBytes,const void *ins,U32 insBytes,U16 mixer);
sRjp1Song *Rjp1_LoadEx(const char *sngName,const char *insName,U16 mixer);
U32 Rjp1_GetWorkspaceSizeEx(U8 rate,U16 mixer);
void Rjp1_UnLoad(sRjp1Song *song); /* stops this song before freeing it */
U8 Rjp1_GetInfo(const sRjp1Song *song,sRjp1Info *info);
U8 Rjp1_Start(sRjp1Song *song,U16 subSong,U8 rate);
U8 Rjp1_SetSubSong(U16 subSong); /* resets all voices; no allocation */
void Rjp1_Stop(void); /* removes VBL, stops DMA, restores trap, frees workspace */
void Rjp1_Tick(void); /* Start already schedules it: do not also call manually */
void Rjp1_SetVolume(U16 volume); /* 0..64 */
void Rjp1_GetVolumes(U16 *fourVolumes); /* coherent 0..64, not peak meters */
void Rjp1_SetRasterDebug(U8 enable);
U32 Rjp1_GetWorkspaceSize(U8 rate);
U32 Rjp1_GetAllocatedSize(void);
#endif
