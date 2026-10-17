#include <godlib/music/modscope.h>
#include <godlib/platform/platform.h>
#include <godlib/gemdos/gemdos.h>
#include <godlib/vbl/vbl.h>
#include <godlib/ikbd/ikbd.h>
static S8 source[4][64],out[258],expected[256];
volatile U16 gScopeTestDone,gScopeTestErrors;
volatile U32 gScopeTestCases;
static void Reference(const sModScopeVoice *v,S8 *dst,U32 clock)
{
    U16 i;const S8 *p=v->mpCurrent,*end=v->mpEnd;U32 step,phase=0;
    if(!v->mEnabled || !v->mVolume || !p || v->mPeriod<48 || v->mPeriod>4095) {
        for(i=0;i<64;++i)dst[i]=0;
        return;
    }
    step=clock/v->mPeriod;
    for(i=0;i<64;++i) {
        S16 sample,vol=v->mVolume>64?64:v->mVolume;
        while(p>=end) {
            if(!v->mpLoop || v->mpLoopEnd<=v->mpLoop) {
                for(;i<64;++i)dst[i]=0;
                return;
            }
            p=v->mpLoop+(p-end);end=v->mpLoopEnd;
        }
        sample=*p;if(v->mFormat)sample*=2;
        sample*=vol;
        dst[i]=sample>=0?sample/64:-((-sample+63)/64);
        phase+=step;p+=phase>>16;phase&=65535UL;
    }
}
S16 GodLib_Game_Main(S16 argc,char **argv)
{
    sModScopeVoice v[4];U16 c,i,k;U32 clock=37144807UL;
    (void)argc;(void)argv;GemDos_Super(0);Platform_Init();
    if(sizeof(sModScopeVoice)!=24)gScopeTestErrors|=1;
    for(k=0;k<256;++k) {
        for(c=0;c<4;++c) {
            for(i=0;i<64;++i)source[c][i]=(S8)((i*47+c*31+k)&255);
            v[c].mpCurrent=source[c]+(k%17);v[c].mpEnd=source[c]+17;
            v[c].mpLoop=(k&1)?source[c]+3:0;
            v[c].mpLoopEnd=source[c]+(k%3?11:5);
            v[c].mPeriod=k%13==0?0:(k%17==0?4096:48+(k*17+c*108)%4048);
            v[c].mVolume=(k+c*27)%96;v[c].mFormat=c&1;v[c].mEnabled=k%7!=0;
            if(v[c].mFormat)for(i=0;i<64;++i)source[c][i]=(S8)(source[c][i]/2);
            Reference(v+c,expected+c*64,clock);
        }
        out[0]=0x55;out[257]=(S8)0xaa;
        ModScope_Sample(v,out+1,clock); /* odd output pointer is supported */
        if(out[0]!=0x55 || out[257]!=(S8)0xaa)gScopeTestErrors|=2;
        for(i=0;i<256;++i)if(out[1+i]!=expected[i])gScopeTestErrors|=4;
        ++gScopeTestCases;
    }
    gScopeTestDone=1;
    while(!IKBD_GetKbdBytesWaiting())Vbl_WaitVbl();
    Platform_DeInit();return 0;
}
