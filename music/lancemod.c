/* ProTracker MOD loader/player wrapper for Lance Paula replay. */
#include "lancemod.h"
#include <godlib/file/file.h>
#include <godlib/memory/memory.h>
#include <godlib/vbl/vbl.h>

extern void Paula_Init(U8 aFreq);
extern void Paula_SetReplayBuf(void *apBuffer);
extern void Paula_SetWorkspace(void *const *apPointers);
extern void Paula_SetInitScratch(void *apBuffer);

/* Deterministic outputs of the original code generator, verified on target.
 * The previous 110994-byte reservation overflowed by 40120 bytes at 50K. */
static const U32 sPaulaCodeBytes[3]={77638UL,110994UL,151114UL};
static const U32 sPaulaTableBytes[6]={4096UL,29300UL,2344UL,16894UL,8192UL,2500UL};
static void *spPaulaWorkspace;
static void *spPaulaReplayBuffer;
static U32 sPaulaWorkspaceBytes;
static U8 sPaulaFrequency;
static U8 sPaulaInitialized;

static U8 LanceMod_Frequency(U8 aFreq)
{
    return (aFreq>=eLANCEMOD_FREQ_12K && aFreq<=eLANCEMOD_FREQ_50K)?aFreq:eLANCEMOD_FREQ_25K;
}

static U16 LanceMod_ReplayLenForFreq(U8 aFreq)
{
    return (U16)(250U<<aFreq);
}

U32 LanceMod_GetPaulaWorkspaceSize(U8 aFreq)
{
    aFreq=LanceMod_Frequency(aFreq);
    /* 12K LEA correction copies a 91-word lookahead window during init. */
    return 63326UL+sPaulaCodeBytes[aFreq-1]+(aFreq==eLANCEMOD_FREQ_12K?182UL:0UL)
           +2UL*LanceMod_ReplayLenForFreq(aFreq);
}

U32 LanceMod_GetPaulaAllocatedSize(void)
{
    return sPaulaWorkspaceBytes;
}

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

void LanceMod_InitPaula(U8 aFreq)
{
    void *pointers[7];
    U8 *p;
    void *scratch;
    U16 i,len;
    aFreq=LanceMod_Frequency(aFreq);
    /* Stop callback/DMA and restore trap #0 before freeing or regenerating. */
    LanceMod_StopVbl();
    if(spPaulaWorkspace && sPaulaFrequency!=aFreq)LanceMod_ShutdownPaula();
    if(!spPaulaWorkspace) {
        U32 bytes=LanceMod_GetPaulaWorkspaceSize(aFreq);
        /* DMA cannot read alternate/TT RAM; use the ST-RAM allocator. */
        spPaulaWorkspace=mMEMSCREENCALLOC(bytes);
        if(!spPaulaWorkspace)return;
        sPaulaWorkspaceBytes=bytes;
        sPaulaFrequency=aFreq;
    }
    len=LanceMod_ReplayLenForFreq(aFreq);
    scratch=mMEMALLOC((U32)len);
    if(!scratch) {
        LanceMod_ShutdownPaula();return;
    }
    spPaulaReplayBuffer=spPaulaWorkspace;
    p=(U8 *)spPaulaWorkspace+2UL*len;
    for(i=0;i<6;++i) {
        pointers[i]=p;
        p+=sPaulaTableBytes[i];
    }
    pointers[6]=p;
    Paula_SetReplayBuf(spPaulaReplayBuffer);
    Paula_SetWorkspace(pointers);
    Paula_SetInitScratch(scratch);
    Paula_Init(aFreq);
    sPaulaInitialized=1;
    Paula_SetInitScratch(0);
    mMEMFREE(scratch);
}

void LanceMod_ShutdownPaula(void)
{
    LanceMod_StopVbl();
    if(spPaulaWorkspace) {
        mMEMSCREENFREE(spPaulaWorkspace);
    }
    spPaulaWorkspace=0;
    spPaulaReplayBuffer=0;
    sPaulaWorkspaceBytes=0;
    sPaulaFrequency=0;
    Paula_SetReplayBuf(0);
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
