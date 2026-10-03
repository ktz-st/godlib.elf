#ifndef INCLUDED_SGDLMIX_H
#define INCLUDED_SGDLMIX_H
#include <godlib/base/base.h>

/* OrionSoft SGDL four-channel signed PCM mixer. Exclusive STE DMA owner.
 * Platform_Init in supervisor mode must precede Init; DeInit before teardown.
 * No gain, resampling or clipping: overlapping samples need headroom. */
enum { eSGDLMIXER_FREQ_12K = 1, eSGDLMIXER_FREQ_25K = 2 };
#define dSGDLMIXER_CHANNELS 4
#define dSGDLMIXER_BLOCK_SIZE 512

U8 SgdlMixer_Init(U16 frequency);
void SgdlMixer_DeInit(void);
/* Switching rate stops all channels and clears queued audio. PCM must already
 * match the selected rate (12517 or 25033 Hz); this is not a resampler. */
U8 SgdlMixer_SetFrequency(U16 frequency);
U16 SgdlMixer_GetFrequency(void);
/* Length MUST be a nonzero multiple of 512. Pad one-shots with signed silence.
 * Looping returns to pcm at the block boundary. Samples remain caller-owned
 * and must stay valid until Stop/DeInit. Pointers may be byte-aligned. */
U8 SgdlMixer_Play(U16 channel, const S8 * pcm, U32 length, U8 loop);
void SgdlMixer_Stop(U16 channel);
void SgdlMixer_StopAll(void);
/* Source activity; queued DMA audio may remain audible after a source stops. */
U8 SgdlMixer_IsPlaying(U16 channel);
/* Palette-zero raster CPU meter, matching LanceMod. */
void SgdlMixer_SetRasterDebug(U8 flag);
#endif
