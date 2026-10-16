/* Compatibility names for existing applications. New code uses pcmframe.h. */
#ifndef INCLUDED_SLUGMIX_COMPAT_H
#define INCLUDED_SLUGMIX_COMPAT_H
#include <godlib/audio/pcmframe.h>
#define SlugMixer_DeInit PcmFrameMixer_DeInit
#define SlugMixer_Init PcmFrameMixer_Init
#define SlugMixer_IsPlaying PcmFrameMixer_IsPlaying
#define SlugMixer_Play PcmFrameMixer_Play
#define SlugMixer_SetRasterDebug PcmFrameMixer_SetRasterDebug
#define SlugMixer_Stop PcmFrameMixer_Stop
#endif
