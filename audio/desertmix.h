/* Compatibility names for existing applications. New code uses pcmlayer.h. */
#ifndef INCLUDED_DESERTMIX_COMPAT_H
#define INCLUDED_DESERTMIX_COMPAT_H
#include <godlib/audio/pcmlayer.h>
#define DesertMixer_DeInit PcmLayerMixer_DeInit
#define DesertMixer_Init PcmLayerMixer_Init
#define DesertMixer_IsPlaying PcmLayerMixer_IsPlaying
#define DesertMixer_Play PcmLayerMixer_Play
#define DesertMixer_SetRasterDebug PcmLayerMixer_SetRasterDebug
#define DesertMixer_Stop PcmLayerMixer_Stop
#define DesertMixer_StopAll PcmLayerMixer_StopAll
#endif
