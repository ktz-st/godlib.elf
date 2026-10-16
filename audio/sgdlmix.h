/* Compatibility names for existing applications. New code uses pcmblock.h. */
#ifndef INCLUDED_SGDLMIX_COMPAT_H
#define INCLUDED_SGDLMIX_COMPAT_H
#include <godlib/audio/pcmblock.h>
#define SgdlMixer_DeInit PcmBlockMixer_DeInit
#define SgdlMixer_GetFrequency PcmBlockMixer_GetFrequency
#define SgdlMixer_Init PcmBlockMixer_Init
#define SgdlMixer_IsPlaying PcmBlockMixer_IsPlaying
#define SgdlMixer_Play PcmBlockMixer_Play
#define SgdlMixer_SetFrequency PcmBlockMixer_SetFrequency
#define SgdlMixer_SetRasterDebug PcmBlockMixer_SetRasterDebug
#define SgdlMixer_Stop PcmBlockMixer_Stop
#define SgdlMixer_StopAll PcmBlockMixer_StopAll
#define dSGDLMIXER_BLOCK_SIZE dPCMBLOCK_BLOCK_SIZE
#define dSGDLMIXER_CHANNELS dPCMBLOCK_CHANNELS
#define eSGDLMIXER_FREQ_12K ePCMBLOCK_FREQ_12K
#define eSGDLMIXER_FREQ_25K ePCMBLOCK_FREQ_25K
#endif
