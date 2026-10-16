#ifndef INCLUDED_PACMADMIX_H
#define INCLUDED_PACMADMIX_H
#include <godlib/base/base.h>
/* Three channels, signed 8-bit mono 12517 Hz. Platform_Init + supervisor mode
 * required. Exclusive STE/MegaSTE DMA owner; DeInit before Platform_DeInit.
 * Fast packed ADD.L has carries between neighbouring bytes, like SlugMixer.
 * Optional independent-byte saturation; no gain or resampling. */
/* One optional streamed source; called from VBL, never perform file IO here.
 * Write up to count bytes into dst and return bytes written. Short reads are
 * padded with silence. Callback/user storage must live until Stop/DeInit. */
typedef U16 (*fPacmadMixerStream)(void *user,S8 *dst,U16 count);
U8 PacmadMixer_SetStream(U16 channel,fPacmadMixerStream reader,void *user);
U8 PacmadMixer_Init(void);
void PacmadMixer_DeInit(void);
/* Caller-owned PCM: even address, nonzero even byte length, <=0x7fffffff.
 * All channels support loops; length need not be a multiple of 250 or 4.
 * Play replaces the current source and cancels its queued successor. */
U8 PacmadMixer_Play(U16 channel,const S8 *pcm,U32 length,U8 loop);
/* One queued one-shot successor, joined exactly at the source boundary.
 * Only accepts an active non-looping source. PCM remains caller-owned. */
U8 PacmadMixer_Queue(U16 channel,const S8 *pcm,U32 length);
void PacmadMixer_Stop(U16 channel);
void PacmadMixer_StopAll(void);
U8 PacmadMixer_IsPlaying(U16 channel);
const S8 *PacmadMixer_GetSample(U16 channel);
U32 PacmadMixer_GetRemaining(U16 channel);
/* Default off. Clamp the full signed sum of all sources to -128..127.
 * More expensive with overlapping sources; 0/1 source keeps the fast path.
 * DeInit resets this option. */
void PacmadMixer_SetSaturation(U8 flag);
void PacmadMixer_SetRasterDebug(U8 flag);
/* Number of scheduler recoveries, not a count of lost samples. */
U32 PacmadMixer_GetUnderruns(void);
#endif
