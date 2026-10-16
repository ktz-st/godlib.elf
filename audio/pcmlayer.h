#ifndef INCLUDED_PCMLAYER_H
#define INCLUDED_PCMLAYER_H
#include <godlib/base/base.h>
/* Desert gameplay mixer: signed PCM, 12517 Hz mono, exclusive STE DMA. */
U8 PcmLayerMixer_Init(void);
void PcmLayerMixer_DeInit(void);
/* 0=base loop, 1=additional loop, 2/3=one-shots. loop must match slot type.
 * Nonzero length must be a multiple of 4; PCM must be even-aligned.
 * Samples remain caller-owned until Stop/DeInit. No gain or clipping. */
U8 PcmLayerMixer_Play(U16 channel,const S8 *pcm,U32 length,U8 loop);
void PcmLayerMixer_Stop(U16 channel);
void PcmLayerMixer_StopAll(void);
U8 PcmLayerMixer_IsPlaying(U16 channel);
/* Same palette-zero raster CPU display as LanceMod. */
void PcmLayerMixer_SetRasterDebug(U8 flag);
#endif
