#include "pcmframe.h"
#include <godlib/system/system.h>
#include <godlib/vbl/vbl.h>

#define BASE 0xc36b0UL
/* The original assembly uses absolute relocated references into this slab. */
U16 gPcmFrameMixerState[(0xc3f22UL - BASE) / 2];
#define BYTE(a) (*(volatile U8 *)((U8 *)gPcmFrameMixerState + (a) - BASE))
#define LONG(a) (*(volatile U32 *)((U8 *)gPcmFrameMixerState + (a) - BASE))
#define PTR(a) ((U32)((U8 *)gPcmFrameMixerState + (a) - BASE))
#define HW(a) (*(volatile U8 *)(a))

extern void PcmFrameMixer_CoreTick(void);
extern U16 PcmFrameMixer_Lock(void);
extern void PcmFrameMixer_Unlock(U16 sr);
static volatile U8 sEnabled,sRasterDebug;
static const U32 sCurrent[3] = {0xc3f1e, 0xc3f1a, 0xc3f12};
static const U32 sStart[3] = {0xc36d2, 0xc36ce, 0xc36c6};
static const U32 sLeft[3] = {0xc3706, 0xc3705, 0xc3703};
static const U32 sLength[3] = {0xc3702, 0xc3701, 0xc36ff};
static const U32 sRepeat[3] = {0xc36f6, 0xc36f5, 0xc36f3};
static const U32 sActive[3] = {0xc36f2, 0xc36f1, 0xc36ef};

static void PcmFrameMixer_Vbl(void)
{
    U16 colour=0,debug=sRasterDebug;
    if (!sEnabled) return;
    if(debug) { colour=*(volatile U16 *)0xffff8240UL; *(volatile U16 *)0xffff8240UL=0x700; }
    PcmFrameMixer_CoreTick();
    if(debug) *(volatile U16 *)0xffff8240UL=colour;
}

static void PcmFrameMixer_Address(U32 low, U32 value)
{
    BYTE(low) = value;
    BYTE(low+1) = value >> 8;
    BYTE(low+2) = value >> 16;
}

static void PcmFrameMixer_Range(U32 begin, U32 end)
{
    HW(0xffff8903UL) = begin >> 16;
    HW(0xffff8905UL) = begin >> 8;
    HW(0xffff8907UL) = begin;
    HW(0xffff890fUL) = end >> 16;
    HW(0xffff8911UL) = end >> 8;
    HW(0xffff8913UL) = end;
}

U8 PcmFrameMixer_Init(void)
{
    U16 i, sr;
    eSYSTEM_MCH machine;
    if (sEnabled) return 1;
    machine = System_GetMCH();
    if (machine != MCH_STE && machine != MCH_MEGASTE &&
        machine != MCH_TT && machine != MCH_FALCON) return 0;
    /* DMA cannot address a state slab loaded into TT-RAM. */
    if ((U32)gPcmFrameMixerState + sizeof(gPcmFrameMixerState) > 0x1000000UL) return 0;
    sr = PcmFrameMixer_Lock();
    if (!Vbl_AddCall(PcmFrameMixer_Vbl)) {
        PcmFrameMixer_Unlock(sr);
        return 0;
    }
    HW(0xffff8901UL) = 0;
    for (i=0; i<sizeof(gPcmFrameMixerState)/sizeof(U16); ++i)
        gPcmFrameMixerState[i] = 0;
    for (i=0; i<3; ++i) {
        LONG(sCurrent[i]) = LONG(sStart[i]) = PTR(0xc3708);
        BYTE(sLeft[i]) = BYTE(sLength[i]) = 1;
    }
    PcmFrameMixer_Address(0xc36e5, PTR(0xc3b0a));
    PcmFrameMixer_Address(0xc36dc, PTR(0xc3c04));
    PcmFrameMixer_Address(0xc36e2, PTR(0xc3c04));
    PcmFrameMixer_Address(0xc36d9, PTR(0xc3cfe));
    PcmFrameMixer_Address(0xc36df, PTR(0xc3cfe));
    PcmFrameMixer_Address(0xc36d6, PTR(0xc3df8));
    HW(0xffff8921UL) = 0x81;
    PcmFrameMixer_Range(PTR(0xc3b0a), PTR(0xc3c04));
    HW(0xffff8901UL) = 3;
    __asm__ volatile ("nop\n\tnop\n\tnop");
    PcmFrameMixer_Range(PTR(0xc3c04), PTR(0xc3cfe));
    BYTE(0xc3b08) = 2;
    sEnabled = 1;
    PcmFrameMixer_Unlock(sr);
    return 1;
}

void PcmFrameMixer_DeInit(void)
{
    U16 sr = PcmFrameMixer_Lock();
    if (sEnabled) {
        sEnabled = 0;
        Vbl_RemoveCall(PcmFrameMixer_Vbl);
        HW(0xffff8901UL) = 0;
    }
    sRasterDebug=0;
    PcmFrameMixer_Unlock(sr);
}

U8 PcmFrameMixer_Play(U16 channel, const S8 * pcm, U16 blocks, U8 loop)
{
    U16 sr;
    if (!sEnabled || channel >= 3 || !pcm || ((U32)pcm & 1) ||
        !blocks || blocks > 255 || (channel == 0 && loop)) return 0;
    sr = PcmFrameMixer_Lock();
    LONG(sCurrent[channel]) = LONG(sStart[channel]) = (U32)pcm;
    BYTE(sLeft[channel]) = BYTE(sLength[channel]) = blocks;
    BYTE(sRepeat[channel]) = loop ? 255 : 0;
    BYTE(sActive[channel]) = 255;
    PcmFrameMixer_Unlock(sr);
    return 1;
}

void PcmFrameMixer_Stop(U16 channel)
{
    U16 sr;
    if (!sEnabled || channel >= 3) return;
    sr = PcmFrameMixer_Lock();
    LONG(sCurrent[channel]) = LONG(sStart[channel]) = PTR(0xc3708);
    BYTE(sLeft[channel]) = BYTE(sLength[channel]) = 1;
    BYTE(sRepeat[channel]) = BYTE(sActive[channel]) = 0;
    PcmFrameMixer_Unlock(sr);
}

U8 PcmFrameMixer_IsPlaying(U16 channel)
{
    return sEnabled && channel < 3 && BYTE(sActive[channel]) != 0;
}

void PcmFrameMixer_SetRasterDebug(U8 flag) { sRasterDebug=!!flag; }
