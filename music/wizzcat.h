#ifndef INCLUDED_WIZZCAT_H
#define INCLUDED_WIZZCAT_H

/* ###################################################################################
#  INCLUDES
################################################################################### */

#include <godlib/base/base.h>
#include <godlib/music/modscope.h>


/* ###################################################################################
#  DEFINES
################################################################################### */

/* Legacy explicit margin; default loading now computes the requirement. */
#define dWIZZCAT_MODULE_MARGIN_SIZE	(128UL * 1024UL)


/* ###################################################################################
#  STRUCTS
################################################################################### */

typedef struct sWizzcatModule
{
	U32	mDataSize;
	U32	mWorkspaceSize;
	U16 mPrepared; /* internal: preparation modifies sample headers/data */
	U8	mData[ 1 ];
} sWizzcatModule;

typedef struct sWizzcatInfo
{
	U8	mSongPos;
	U8	mPattPos;
} sWizzcatInfo;


/* ###################################################################################
#  PROTOTYPES
################################################################################### */

#define dWIZZCAT_CHANNEL_COUNT 4
#define dWIZZCAT_VISUALIZER_IDLE 0x4000

typedef struct sWizzcatModuleInfo
{
    U32 mFileBytes,mModuleBytes,mSampleBytes,mPreparedSampleBytes;
    U32 mWorkspaceBytes,mInitScratchBytes,mLoadBytes,mSilenceBytes;
    U16 mSampleCount,mPatternCount;
} sWizzcatModuleInfo;
U8 Wizzcat_GetModuleInfo(const void *apHeader,U32 aFileBytes,sWizzcatModuleInfo *apInfo);
/* Zero workspace selects automatic sizing; an undersized explicit size fails. */
/* Preparation is in place. Reload the raw MOD before another Init/Start. */
/* Four coherent native channel retrigger ages; not amplitudes. NULL is a no-op. */
void Wizzcat_GetVisualizer(U16 *apAges);
void Wizzcat_SetRasterDebug(U8 aEnabled);

sWizzcatModule *	Wizzcat_Load( const char * apFileName );
sWizzcatModule *	Wizzcat_LoadEx( const char * apFileName, U32 aWorkspaceSize );
void				Wizzcat_UnLoad( sWizzcatModule * apModule );

void				Wizzcat_Init( sWizzcatModule * apModule );
void				Wizzcat_Play( void );
void				Wizzcat_Stop( void );

U8					Wizzcat_Start( sWizzcatModule * apModule );
void				Wizzcat_StopVbl( void );

void				Wizzcat_GetInfo( sWizzcatInfo * apInfo );
void				Wizzcat_Jump( U8 aPos );


/* ################################################################################ */

/* Read-only four pitched/volume-scaled source previews, CH1..CH4.
 * apSamples needs dMODSCOPE_BYTES (256) signed bytes. NULL is a no-op.
 * Call in supervisor main loop; idle channels return zero. */
void Wizzcat_GetOscilloscope(S8 *apSamples);

/* Copy 164 actual stereo PCM frames (328 interleaved signed bytes), from the
 * latest completed mix block. Used by the ICE-style delayed XY display.
 * NULL is a no-op; stopped playback returns zero. No retained pointer. */
void Wizzcat_GetStereoOscilloscope(S8 *apSamples);

#endif /* INCLUDED_WIZZCAT_H */
