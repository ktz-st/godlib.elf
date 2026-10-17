#ifndef INCLUDED_LANCEMOD_H
#define INCLUDED_LANCEMOD_H

/* ###################################################################################
#  INCLUDES
################################################################################### */

#include <godlib/base/base.h>
#include <godlib/music/modscope.h>


/* ###################################################################################
#  DEFINES
################################################################################### */

#define dLANCEMOD_CHANNEL_COUNT        4
#define dLANCEMOD_VISUALIZER_IDLE      0x4000

/* Legacy worst-case bound; LanceMod_Load computes the actual requirement. */
#define dLANCEMOD_EXTRA_BUFFER_SIZE    ((31L * 664L) + 2L)


/* ###################################################################################
#  ENUMS
################################################################################### */

enum
{
	eLANCEMOD_FREQ_12K = 1,
	eLANCEMOD_FREQ_25K = 2,
	eLANCEMOD_FREQ_50K = 3
};


/* ###################################################################################
#  PROTOTYPES
################################################################################### */

/* Unprepared four-channel, 31-sample ProTracker header (at least 1084 bytes).
 * mModuleBytes excludes trailing file data; mLoadBytes includes sample guards.
 * Only nonempty samples need guards. Playing time does not determine RAM usage. */
typedef struct sLanceModModuleInfo
{
    U32 mFileBytes;
    U32 mModuleBytes;
    U32 mSampleBytes;
    U32 mLoopBytes;
    U32 mLoadBytes;
    U16 mSampleCount;
    U16 mPatternCount;
} sLanceModModuleInfo;

U8 LanceMod_GetModuleInfo(const void *apHeader,U32 aFileBytes,sLanceModModuleInfo *apInfo);
/* Resident heap workspace including both DMA pages; initialization also needs
 * a temporary 500/1000/2000-byte frequency list. Invalid rates default to 25K. */
U32 LanceMod_GetPaulaWorkspaceSize(U8 aFreq);
U32 LanceMod_GetPaulaAllocatedSize(void);

void *	LanceMod_Load( const char * apFileName );
void	LanceMod_UnLoad( void * apModData );

void	LanceMod_InitPaula( U8 aFreq );
void	LanceMod_ShutdownPaula( void );
void	LanceMod_Init( void * apModData );
void	LanceMod_Play( void );
void	LanceMod_Stop( void );

U8		LanceMod_Start( void * apModData, U8 aFreq );
void	LanceMod_StopVbl( void );

void	LanceMod_SetMasterVolume( U16 aVolume );
void	LanceMod_SetRasterDebug( U8 aFlag );
U8		LanceMod_GetEndFlag( void );

/* Read-only, coherent snapshot of four mt_Visualizer activity ages.
   Ages advance once per replay call and reset on channel retriggers. They are
   not sample amplitudes or channel volumes. Values saturate at IDLE; init and
   stopped playback return IDLE for all channels. apAges needs four U16 slots.
   Called in supervisor mode, like the hardware-facing replay API. NULL is a no-op. */
void    LanceMod_GetVisualizer( U16 * apAges );


/* ################################################################################ */

/* Read-only four pitched/volume-scaled source previews, CH1..CH4.
 * apSamples needs dMODSCOPE_BYTES (256) signed bytes. NULL is a no-op.
 * Call in supervisor main loop; idle channels return zero. */
void LanceMod_GetOscilloscope(S8 *apSamples);

/* Copy 164 actual stereo PCM frames (328 interleaved signed bytes), from the
 * latest completed mix block. Used by the ICE-style delayed XY display.
 * NULL is a no-op; stopped playback returns zero. No retained pointer. */
void LanceMod_GetStereoOscilloscope(S8 *apSamples);

#endif /* INCLUDED_LANCEMOD_H */
