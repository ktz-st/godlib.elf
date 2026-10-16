#ifndef INCLUDED_PCMVOICE_H
#define INCLUDED_PCMVOICE_H
#include <godlib/base/base.h>
/* Three channels, signed 8-bit mono 12517 Hz. Platform_Init + supervisor mode
 * required. Exclusive STE/MegaSTE DMA owner; DeInit before Platform_DeInit.
 * Fast packed ADD.L has carries between neighbouring bytes, like PcmFrameMixer.
 * Optional independent-byte saturation; no gain or resampling. */
/* One optional streamed source; called from VBL, never perform file IO here.
 * Write up to count bytes into dst and return bytes written. Short reads are
 * padded with silence. Callback/user storage must live until Stop/DeInit. */
typedef U16 (*fPcmVoiceMixerStream)(void *user,S8 *dst,U16 count);
U8 PcmVoiceMixer_SetStream(U16 channel,fPcmVoiceMixerStream reader,void *user);
U8 PcmVoiceMixer_Init(void);
void PcmVoiceMixer_DeInit(void);
/* Caller-owned PCM: even address, nonzero even byte length, <=0x7fffffff.
 * All channels support loops; length need not be a multiple of 250 or 4.
 * Play replaces the current source and cancels its queued successor. */
U8 PcmVoiceMixer_Play(U16 channel,const S8 *pcm,U32 length,U8 loop);
/* One queued one-shot successor, joined exactly at the source boundary.
 * Only accepts an active non-looping source. PCM remains caller-owned. */
U8 PcmVoiceMixer_Queue(U16 channel,const S8 *pcm,U32 length);
void PcmVoiceMixer_Stop(U16 channel);
void PcmVoiceMixer_StopAll(void);
U8 PcmVoiceMixer_IsPlaying(U16 channel);
const S8 *PcmVoiceMixer_GetSample(U16 channel);
U32 PcmVoiceMixer_GetRemaining(U16 channel);
/* Default off. Clamp the full signed sum of all sources to -128..127.
 * More expensive with overlapping sources; 0/1 source keeps the fast path.
 * DeInit resets this option. */
void PcmVoiceMixer_SetSaturation(U8 flag);
void PcmVoiceMixer_SetRasterDebug(U8 flag);
/* Number of scheduler recoveries, not a count of lost samples. */
U32 PcmVoiceMixer_GetUnderruns(void);
#endif
