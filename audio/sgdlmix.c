#include "sgdlmix.h"
#include <godlib/system/system.h>
#include <godlib/vbl/vbl.h>

typedef struct { const S8 * volatile current; const S8 * volatile end; const S8 * volatile loop; } sSgdlMixerChannel;
sSgdlMixerChannel gSgdlMixerChannels[4];
/* Original layout: 1024-byte circular DMA buffer, then 512-byte silence. */
static U16 sStorage[768];
S8 * gSgdlMixerBuffer = (S8 *)sStorage;
volatile U8 gSgdlMixerActive;
volatile U8 gSgdlMixerBufferId;
U8 gSgdlMixerDmaMode;
static U16 sFrequency;
static volatile U8 sRasterDebug;
extern void SgdlMixer_CoreInit(void);
extern void SgdlMixer_CoreExit(void);
extern void SgdlMixer_CoreUpdate(void);
extern U16 SgdlMixer_Lock(void);
extern void SgdlMixer_Unlock(U16 sr);

static void SgdlMixer_Vbl(void)
{
    U16 colour=0,debug=sRasterDebug;
    if(debug) { colour=*(volatile U16 *)0xffff8240UL; *(volatile U16 *)0xffff8240UL=0x700; }
    SgdlMixer_CoreUpdate();
    if(debug) *(volatile U16 *)0xffff8240UL=colour;
}

static void SgdlMixer_Clear(void)
{
    U16 i;
    for (i=0; i<4; ++i) {
        gSgdlMixerChannels[i].current = 0;
        gSgdlMixerChannels[i].end = 0;
        gSgdlMixerChannels[i].loop = 0;
    }
    for (i=0; i<768; ++i) sStorage[i]=0;
    gSgdlMixerBufferId=0;
}

U8 SgdlMixer_Init(U16 frequency)
{
    U16 sr;
    eSYSTEM_MCH mch;
    if (frequency!=eSGDLMIXER_FREQ_12K && frequency!=eSGDLMIXER_FREQ_25K) return 0;
    if (gSgdlMixerActive) return SgdlMixer_SetFrequency(frequency);
    mch=System_GetMCH();
    if (mch!=MCH_STE && mch!=MCH_MEGASTE) return 0;
    if ((U32)sStorage + sizeof(sStorage) > 0x1000000UL) return 0;
    sr=SgdlMixer_Lock();
    if (!Vbl_AddCall(SgdlMixer_Vbl)) {
        SgdlMixer_Unlock(sr);
        return 0;
    }
    SgdlMixer_Clear();
    sFrequency=frequency;
    gSgdlMixerDmaMode=(U8)(0x80 | frequency);
    SgdlMixer_CoreInit();
    gSgdlMixerActive=1;
    SgdlMixer_Unlock(sr);
    return 1;
}

void SgdlMixer_DeInit(void)
{
    U16 sr=SgdlMixer_Lock();
    if (gSgdlMixerActive) {
        gSgdlMixerActive=0;
        Vbl_RemoveCall(SgdlMixer_Vbl);
        SgdlMixer_CoreExit();
        SgdlMixer_Clear();
    }
    sFrequency=0;
    sRasterDebug=0;
    SgdlMixer_Unlock(sr);
}

U8 SgdlMixer_SetFrequency(U16 frequency)
{
    U16 sr;
    if (!gSgdlMixerActive || (frequency!=eSGDLMIXER_FREQ_12K && frequency!=eSGDLMIXER_FREQ_25K)) return 0;
    if (sFrequency==frequency) return 1;
    sr=SgdlMixer_Lock();
    SgdlMixer_CoreExit();
    SgdlMixer_Clear();
    sFrequency=frequency;
    gSgdlMixerDmaMode=(U8)(0x80 | frequency);
    SgdlMixer_CoreInit();
    SgdlMixer_Unlock(sr);
    return 1;
}

U16 SgdlMixer_GetFrequency(void) { return sFrequency; }

U8 SgdlMixer_Play(U16 channel, const S8 * pcm, U32 length, U8 loop)
{
    U16 sr;
    if (!gSgdlMixerActive || channel>=4 || !pcm || !length ||
        (length & 511UL) || (U32)pcm + length < (U32)pcm) return 0;
    sr=SgdlMixer_Lock();
    gSgdlMixerChannels[channel].current=pcm;
    gSgdlMixerChannels[channel].end=pcm+length;
    gSgdlMixerChannels[channel].loop=loop ? pcm : 0;
    SgdlMixer_Unlock(sr);
    return 1;
}

void SgdlMixer_Stop(U16 channel)
{
    U16 sr;
    if (channel>=4) return;
    sr=SgdlMixer_Lock();
    gSgdlMixerChannels[channel].current=0;
    gSgdlMixerChannels[channel].end=0;
    gSgdlMixerChannels[channel].loop=0;
    SgdlMixer_Unlock(sr);
}

void SgdlMixer_StopAll(void)
{
    U16 sr=SgdlMixer_Lock(), i;
    for(i=0;i<4;++i) SgdlMixer_Stop(i);
    SgdlMixer_Unlock(sr);
}

U8 SgdlMixer_IsPlaying(U16 channel)
{
    return gSgdlMixerActive && channel<4 && gSgdlMixerChannels[channel].current!=0;
}

void SgdlMixer_SetRasterDebug(U8 flag) { sRasterDebug=!!flag; }
