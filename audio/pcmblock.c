#include "pcmblock.h"
#include <godlib/system/system.h>
#include <godlib/vbl/vbl.h>

typedef struct { const S8 * volatile current; const S8 * volatile end; const S8 * volatile loop; } sPcmBlockMixerChannel;
sPcmBlockMixerChannel gPcmBlockMixerChannels[4];
/* Original layout: 1024-byte circular DMA buffer, then 512-byte silence. */
static U16 sStorage[768];
S8 * gPcmBlockMixerBuffer = (S8 *)sStorage;
volatile U8 gPcmBlockMixerActive;
volatile U8 gPcmBlockMixerBufferId;
U8 gPcmBlockMixerDmaMode;
static U16 sFrequency;
static volatile U8 sRasterDebug;
extern void PcmBlockMixer_CoreInit(void);
extern void PcmBlockMixer_CoreExit(void);
extern void PcmBlockMixer_CoreUpdate(void);
extern U16 PcmBlockMixer_Lock(void);
extern void PcmBlockMixer_Unlock(U16 sr);

static void PcmBlockMixer_Vbl(void)
{
    U16 colour=0,debug=sRasterDebug;
    if(debug) { colour=*(volatile U16 *)0xffff8240UL; *(volatile U16 *)0xffff8240UL=0x700; }
    PcmBlockMixer_CoreUpdate();
    if(debug) *(volatile U16 *)0xffff8240UL=colour;
}

static void PcmBlockMixer_Clear(void)
{
    U16 i;
    for (i=0; i<4; ++i) {
        gPcmBlockMixerChannels[i].current = 0;
        gPcmBlockMixerChannels[i].end = 0;
        gPcmBlockMixerChannels[i].loop = 0;
    }
    for (i=0; i<768; ++i) sStorage[i]=0;
    gPcmBlockMixerBufferId=0;
}

U8 PcmBlockMixer_Init(U16 frequency)
{
    U16 sr;
    eSYSTEM_MCH mch;
    if (frequency!=ePCMBLOCK_FREQ_12K && frequency!=ePCMBLOCK_FREQ_25K) return 0;
    if (gPcmBlockMixerActive) return PcmBlockMixer_SetFrequency(frequency);
    mch=System_GetMCH();
    if (mch!=MCH_STE && mch!=MCH_MEGASTE) return 0;
    if ((U32)sStorage + sizeof(sStorage) > 0x1000000UL) return 0;
    sr=PcmBlockMixer_Lock();
    if (!Vbl_AddCall(PcmBlockMixer_Vbl)) {
        PcmBlockMixer_Unlock(sr);
        return 0;
    }
    PcmBlockMixer_Clear();
    sFrequency=frequency;
    gPcmBlockMixerDmaMode=(U8)(0x80 | frequency);
    PcmBlockMixer_CoreInit();
    gPcmBlockMixerActive=1;
    PcmBlockMixer_Unlock(sr);
    return 1;
}

void PcmBlockMixer_DeInit(void)
{
    U16 sr=PcmBlockMixer_Lock();
    if (gPcmBlockMixerActive) {
        gPcmBlockMixerActive=0;
        Vbl_RemoveCall(PcmBlockMixer_Vbl);
        PcmBlockMixer_CoreExit();
        PcmBlockMixer_Clear();
    }
    sFrequency=0;
    sRasterDebug=0;
    PcmBlockMixer_Unlock(sr);
}

U8 PcmBlockMixer_SetFrequency(U16 frequency)
{
    U16 sr;
    if (!gPcmBlockMixerActive || (frequency!=ePCMBLOCK_FREQ_12K && frequency!=ePCMBLOCK_FREQ_25K)) return 0;
    if (sFrequency==frequency) return 1;
    sr=PcmBlockMixer_Lock();
    PcmBlockMixer_CoreExit();
    PcmBlockMixer_Clear();
    sFrequency=frequency;
    gPcmBlockMixerDmaMode=(U8)(0x80 | frequency);
    PcmBlockMixer_CoreInit();
    PcmBlockMixer_Unlock(sr);
    return 1;
}

U16 PcmBlockMixer_GetFrequency(void) { return sFrequency; }

U8 PcmBlockMixer_Play(U16 channel, const S8 * pcm, U32 length, U8 loop)
{
    U16 sr;
    if (!gPcmBlockMixerActive || channel>=4 || !pcm || !length ||
        (length & 511UL) || (U32)pcm + length < (U32)pcm) return 0;
    sr=PcmBlockMixer_Lock();
    gPcmBlockMixerChannels[channel].current=pcm;
    gPcmBlockMixerChannels[channel].end=pcm+length;
    gPcmBlockMixerChannels[channel].loop=loop ? pcm : 0;
    PcmBlockMixer_Unlock(sr);
    return 1;
}

void PcmBlockMixer_Stop(U16 channel)
{
    U16 sr;
    if (channel>=4) return;
    sr=PcmBlockMixer_Lock();
    gPcmBlockMixerChannels[channel].current=0;
    gPcmBlockMixerChannels[channel].end=0;
    gPcmBlockMixerChannels[channel].loop=0;
    PcmBlockMixer_Unlock(sr);
}

void PcmBlockMixer_StopAll(void)
{
    U16 sr=PcmBlockMixer_Lock(), i;
    for(i=0;i<4;++i) PcmBlockMixer_Stop(i);
    PcmBlockMixer_Unlock(sr);
}

U8 PcmBlockMixer_IsPlaying(U16 channel)
{
    return gPcmBlockMixerActive && channel<4 && gPcmBlockMixerChannels[channel].current!=0;
}

void PcmBlockMixer_SetRasterDebug(U8 flag) { sRasterDebug=!!flag; }
