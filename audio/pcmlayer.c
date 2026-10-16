#include "pcmlayer.h"
#include <godlib/system/system.h>
#include <godlib/vbl/vbl.h>

#define BASE 0x2da74UL
/* Preserve all original state offsets, including its 2048-byte DMA ring. */
U16 gPcmLayerMixerState[(0x2e38cUL-BASE)/2];
#define BYTE(a) (*(volatile U8 *)((U8 *)gPcmLayerMixerState+(a)-BASE))
#define LONG(a) (*(volatile U32 *)((U8 *)gPcmLayerMixerState+(a)-BASE))
#define PTR(a) ((U32)((U8 *)gPcmLayerMixerState+(a)-BASE))
#define HW(a) (*(volatile U8 *)(a))
static volatile U8 sEnabled,sRasterDebug;
static U16 sWrite;
extern void PcmLayerMixer_CoreFill(U32 offset,U32 count);
extern U16 PcmLayerMixer_Lock(void);
extern void PcmLayerMixer_Unlock(U16 sr);

static void PcmLayerMixer_Vbl(void)
{
    U16 colour=0, debug=sRasterDebug, target, count, first;
    U32 playhead;
    if (!sEnabled) return;
    if(debug) { colour=*(volatile U16 *)0xffff8240UL; *(volatile U16 *)0xffff8240UL=0x700; }
    /* Original gameplay refill: 640 bytes ahead, aligned to four bytes. */
    playhead=((U32)HW(0xffff8909UL)<<16)|((U32)HW(0xffff890bUL)<<8)|HW(0xffff890dUL);
    target=(U16)(((playhead-PTR(0x2db8c)+640UL)&2047UL)&~3UL);
    count=(target-sWrite)&2047;
    if(count && count<=1984) {
        first=2048-sWrite;
        if(first>count) first=count;
        PcmLayerMixer_CoreFill((U32)sWrite,(U32)first);
        if(count>first) PcmLayerMixer_CoreFill(0,(U32)(count-first));
        sWrite=target;
    }
    if(debug) *(volatile U16 *)0xffff8240UL=colour;
}

U8 PcmLayerMixer_Init(void)
{
    U16 sr,i;
    U32 begin=PTR(0x2db8c),end=PTR(0x2e38c);
    eSYSTEM_MCH mch;
    if(sEnabled) return 1;
    mch=System_GetMCH();
    if(mch!=MCH_STE && mch!=MCH_MEGASTE) return 0;
    if(end>0x1000000UL) return 0;
    sr=PcmLayerMixer_Lock();
    if(!Vbl_AddCall(PcmLayerMixer_Vbl)) { PcmLayerMixer_Unlock(sr); return 0; }
    HW(0xffff8901UL)=0;
    for(i=0;i<sizeof(gPcmLayerMixerState)/sizeof(U16);++i) gPcmLayerMixerState[i]=0;
    /* Disabled base still has a nonzero span: its original zero-fill wraps. */
    LONG(0x2db80)=begin; LONG(0x2db84)=2048; BYTE(0x2db2c)=1;
    sWrite=640;
    HW(0xffff8903UL)=begin>>16; HW(0xffff8905UL)=begin>>8; HW(0xffff8907UL)=begin;
    HW(0xffff890fUL)=end>>16; HW(0xffff8911UL)=end>>8; HW(0xffff8913UL)=end;
    HW(0xffff8921UL)=0x81;
    sEnabled=1;
    HW(0xffff8901UL)=3;
    PcmLayerMixer_Unlock(sr);
    return 1;
}

void PcmLayerMixer_DeInit(void)
{
    U16 sr=PcmLayerMixer_Lock();
    if(sEnabled) {
        sEnabled=0;
        Vbl_RemoveCall(PcmLayerMixer_Vbl);
        HW(0xffff8901UL)=0;
    }
    PcmLayerMixer_StopAll();
    sRasterDebug=0;
    PcmLayerMixer_Unlock(sr);
}

U8 PcmLayerMixer_Play(U16 channel,const S8 *pcm,U32 length,U8 loop)
{
    U16 sr;
    if(!sEnabled || channel>=4 || !pcm || ((U32)pcm&1) || !length ||
       (length&3UL) || length>0x7fffffffUL ||
       (U32)pcm+length<(U32)pcm || (!!loop)!=(channel<2)) return 0;
    sr=PcmLayerMixer_Lock();
    if(channel==0) {
        LONG(0x2db80)=(U32)pcm; LONG(0x2db84)=length;
        LONG(0x2da90)=0; BYTE(0x2db2c)=0;
    } else if(channel==1) {
        LONG(0x2da7c)=(U32)pcm; LONG(0x2da78)=length; LONG(0x2da74)=0;
    } else {
        U32 slot=channel==2?0x2da80:0x2da88;
        LONG(slot)=(U32)pcm; LONG(slot+4)=length;
    }
    PcmLayerMixer_Unlock(sr);
    return 1;
}

void PcmLayerMixer_Stop(U16 channel)
{
    U16 sr=PcmLayerMixer_Lock();
    if(channel==0) {
        BYTE(0x2db2c)=1; LONG(0x2db80)=PTR(0x2db8c);
        LONG(0x2db84)=2048; LONG(0x2da90)=0;
    } else if(channel==1) {
        LONG(0x2da7c)=LONG(0x2da78)=LONG(0x2da74)=0;
    } else if(channel<4) {
        U32 slot=channel==2?0x2da80:0x2da88;
        LONG(slot)=LONG(slot+4)=0;
    }
    PcmLayerMixer_Unlock(sr);
}
void PcmLayerMixer_StopAll(void)
{
    U16 i,sr=PcmLayerMixer_Lock();
    for(i=0;i<4;++i) PcmLayerMixer_Stop(i);
    PcmLayerMixer_Unlock(sr);
}
U8 PcmLayerMixer_IsPlaying(U16 channel)
{
    if(!sEnabled || channel>=4) return 0;
    if(channel==0) return !BYTE(0x2db2c);
    if(channel==1) return LONG(0x2da7c)!=0;
    return LONG(channel==2?0x2da84:0x2da8c)!=0;
}
void PcmLayerMixer_SetRasterDebug(U8 flag) { sRasterDebug=!!flag; }
