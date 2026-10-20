/* ProTracker MOD loader/player wrapper for Lance Paula replay. */
#include "lancemod.h"
#include <godlib/file/file.h>
#include <godlib/memory/memory.h>
#include <godlib/vbl/vbl.h>

#include "paulamixer.h"
static U8 sPaulaInitialized;
U32 LanceMod_GetPaulaWorkspaceSize(U8 rate){return PaulaMixer_GetWorkspaceSize(rate);}
U32 LanceMod_GetPaulaAllocatedSize(void){return PaulaMixer_GetOwner()==1?PaulaMixer_GetAllocatedSize():0;}

static U16 LanceMod_ReadWord(const U8 *p)
{
    return (U16)(((U16)p[0]<<8)|p[1]);
}

U8 LanceMod_GetModuleInfo(const void *apHeader,U32 aFileBytes,sLanceModModuleInfo *apInfo)
{
    const U8 *h=(const U8 *)apHeader;
    sLanceModModuleInfo info;
    U16 i,maxPattern=0,count=0;
    U32 samples=0;
    if(!h || !apInfo || aFileBytes<1084UL)return 0;
    /* These signatures share the four-channel, 31-sample layout. */
    if(!((h[1080]=='M' && h[1081]=='.' && h[1082]=='K' && h[1083]=='.') ||
         (h[1080]=='M' && h[1081]=='!' && h[1082]=='K' && h[1083]=='!') ||
         (h[1080]=='4' && h[1081]=='C' && h[1082]=='H' && h[1083]=='N') ||
         (h[1080]=='F' && h[1081]=='L' && h[1082]=='T' && h[1083]=='4')))return 0;
    if(!h[950] || h[950]>128)return 0;
    /* Match the replay: all 128 order slots determine the sample offset. */
    for(i=0;i<128;++i) {
        U16 pattern=h[952+i];
        if(pattern>127)return 0;
        if(pattern>maxPattern)maxPattern=pattern;
    }
    for(i=0;i<31;++i) {
        const U8 *p=h+20+i*30;
        U32 length=LanceMod_ReadWord(p+22);
        U32 loopStart=LanceMod_ReadWord(p+26),loopLength=LanceMod_ReadWord(p+28);
        if(p[24]>15 || p[25]>64)return 0;
        if(loopLength>1 && (!length || loopStart+loopLength>length))return 0;
        samples+=2UL*length;
        if(length)++count;
    }
    info.mFileBytes=aFileBytes;
    info.mPatternCount=maxPattern+1;
    info.mSampleCount=count;
    info.mSampleBytes=samples;
    info.mModuleBytes=1084UL+1024UL*info.mPatternCount+samples;
    if(info.mModuleBytes>aFileBytes)return 0;
    info.mLoopBytes=664UL*count+2UL;
    info.mLoadBytes=info.mModuleBytes+info.mLoopBytes;
    *apInfo=info;
    return 1;
}

void *LanceMod_Load(const char *apFileName)
{
    U8 header[1084];
    sLanceModModuleInfo info;
    sFileHandle handle;
    S32 size;
    U8 *buffer;
    size=File_GetSize(apFileName);
    if(size<1084)return 0;
    handle=File_Open(apFileName);
    if(handle<=0)return 0;
    if(File_Read(handle,1084UL,header)!=1084L ||
       !LanceMod_GetModuleInfo(header,(U32)size,&info)) {
        File_Close(handle);return 0;
    }
    buffer=mMEMCALLOC(info.mLoadBytes);
    if(buffer) {
        Memory_Copy(1084UL,header,buffer);
        if(File_Read(handle,info.mModuleBytes-1084UL,buffer+1084)!=
           (S32)(info.mModuleBytes-1084UL)) {
            mMEMFREE(buffer);
            buffer=0;
        }
    }
    File_Close(handle);
    return buffer;
}

void LanceMod_UnLoad(void *apModData)
{
    if(apModData) {
        mMEMFREE(apModData);
    }
}

void LanceMod_InitPaula(U8 rate)
{
    LanceMod_StopVbl();
    sPaulaInitialized=PaulaMixer_Init(1,rate);
}
void LanceMod_ShutdownPaula(void)
{
    LanceMod_StopVbl();
    PaulaMixer_Shutdown(1);
}

U8 LanceMod_Start(void *apModData,U8 aFreq)
{
    if(!apModData)return 0;
    LanceMod_InitPaula(aFreq);
    if(!sPaulaInitialized)return 0;
    LanceMod_Init(apModData);
    LanceMod_SetMasterVolume(64);
    if(!Vbl_AddCall(LanceMod_Play)) {
        LanceMod_StopVbl();return 0;
    }
    return 1;
}

void LanceMod_StopVbl(void)
{
    Vbl_RemoveCall(LanceMod_Play);
    if(sPaulaInitialized) {
        LanceMod_Stop();
        sPaulaInitialized=0;
    }
}
