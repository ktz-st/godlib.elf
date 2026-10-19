#ifndef	INCLUDED_AMIXER_H
#define	INCLUDED_AMIXER_H

/* ###################################################################################
#  INCLUDES
################################################################################### */

#include	<godlib/base/base.h>
#include	<godlib/audio/audio.h>


/* ###################################################################################
#  ENUMS
################################################################################### */

enum
{
	eAMIXER_PAN_LEFT   = 0x00,
	eAMIXER_PAN_CENTRE = 0x80,
	eAMIXER_PAN_RIGHT  = 0xFF,
};

enum
{
	eMixer_PanType_Linear,
	eMixer_PanType_ConstantPower,
	eMixer_PanType_PanLaw,

	eMixer_PanType_Limit
};


/* ###################################################################################
#  STRUCTS
################################################################################### */

typedef	struct sAmixerSpl
{
	void *	mpStart;
	void *	mpCurrent;
	void *	mpEnd;
	U32		mLength;
	U8		mGainLeft;
	U8		mGainRight;
	U8		mVolume;
	U8		mActiveFlag;
	U8      mPan;
	U8      mRepeat;
	U16     mPhase;
	const S16 * mpTable;
} sAmixerSpl;


typedef struct sAmixerConfig
{
	U8	mBits;
	U8	mStereoFlag;
	U8	mPanType;
} sAmixerConfig;

/* ###################################################################################
#  PROTOTYPES
################################################################################### */

void	AudioMixer_Init( void );
void	AudioMixer_DeInit( void );

void	AudioMixer_Enable( void );
void	AudioMixer_Disable( void );

sAmixerConfig *	AudioMixer_GetpConfig( void );
void			AudioMixer_SetConfig( const sAmixerConfig * apConfig );


U8		AudioMixer_CanPlayNewSpl( void );
U8		AudioMixer_PlaySample( sAudioDmaSound * apSpl, const U8 aPan );
U8		AudioMixer_PlaySampleDirect( sAudioDmaSound * apSpl, const U8 aPan );
U8 *	AudioMixer_GetpBuffer( void );

/* Call after Init. Default output is 25 kHz; valid enums: eAUDIO_FREQ_12/25.
 * A real rate change stops all source voices and clears queued audio, preserving
 * enabled/disabled state. Same-rate calls do nothing; invalid enums return 0. */
U8 AudioMixer_SetFrequency(U16 aFrequency);
U16 AudioMixer_GetFrequency(void);

/* Two explicit voices, signed 8-bit mono at 6.258/12.517/25.033 kHz.
 * Output is selectable 12.517/25.033 kHz stereo. Source rates are respected:
 * slower sources repeat bytes; 25 kHz -> 12.5 kHz selects every second byte
 * without a low-pass filter (nearest neighbour, possible aliasing).
 * volume/pan: 0..255. Source memory remains caller-owned until stopped/finished.
 * Looping follows mLoopingFlag. Controls are main-thread/supervisor APIs;
 * table construction must not be called from an interrupt or reentrantly.
 * Stop/parameter changes affect future queued output (about 41 ms lead). */
U8 AudioMixer_PlayVoice(U16 aVoice, const sAudioDmaSound * apSound, U8 aVolume, U8 aPan);
void AudioMixer_SetVoiceParams(U16 aVoice, U8 aVolume, U8 aPan);
void AudioMixer_StopVoice(U16 aVoice);
/* Stop both voices and discard queued output, retaining rate/enabled state. */
void AudioMixer_StopAll(void);
U8 AudioMixer_IsVoicePlaying(U16 aVoice);

/* ################################################################################ */

#endif	/*	INCLUDED_AMIXER_H	*/