#ifndef INCLUDED_SLUGMIX_H
#define INCLUDED_SLUGMIX_H
#include <godlib/base/base.h>

/* Platform_Init must precede Init. Exclusive ownership of DMA sound;
 * call DeInit before Platform_DeInit. Designed for a 50 Hz display.
 * 3 channels, 12517 Hz signed 8-bit mono, 250 bytes per VBL. */
U8 SlugMixer_Init(void);
void SlugMixer_DeInit(void);
/* Even-aligned, attenuated PCM; length is blocks*250 bytes, blocks 1..255.
 * Memory must remain valid until channel stops or DeInit completes.
 * Loop is supported on channels 1 and 2 only (original core limitation).
 * Returns zero for invalid arguments or when not initialized. */
U8 SlugMixer_Play(U16 channel, const S8 * pcm, U16 blocks, U8 loop);
void SlugMixer_Stop(U16 channel);
U8 SlugMixer_IsPlaying(U16 channel);
/* Palette-zero raster CPU meter, matching LanceMod. */
void SlugMixer_SetRasterDebug(U8 flag);
#endif
