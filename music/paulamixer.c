/* Shared exclusive Lance Paula backend; no ProTracker sequencer dependency. */
#include "paulamixer.h"
#include "paulaowner.h"
#include "lancemod.h"
#include <godlib/memory/memory.h>
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
static U8 sPaulaOwner;

static U8 LanceMod_Frequency(U8 aFreq)
{
    return (aFreq>=eLANCEMOD_FREQ_12K && aFreq<=eLANCEMOD_FREQ_50K)?aFreq:eLANCEMOD_FREQ_25K;
}

static U16 LanceMod_ReplayLenForFreq(U8 aFreq)
{
    return (U16)(250U<<aFreq);
}

U32 PaulaMixer_GetWorkspaceSize(U8 aFreq)
{
    aFreq=LanceMod_Frequency(aFreq);
    /* 12K LEA correction copies a 91-word lookahead window during init. */
    return 63326UL+sPaulaCodeBytes[aFreq-1]+(aFreq==eLANCEMOD_FREQ_12K?182UL:0UL)
           +2UL*LanceMod_ReplayLenForFreq(aFreq);
}

U32 PaulaMixer_GetAllocatedSize(void)
{
    return sPaulaWorkspaceBytes;
}

U8 PaulaMixer_Init(U8 aOwner,U8 aFreq)
{
    void *pointers[7];
    U8 *p;
    void *scratch;
    U16 i,len;
    if(sPaulaOwner && sPaulaOwner!=aOwner)return 0;
    /* Stop callback/DMA and restore trap #0 before freeing or regenerating. */
    if(sPaulaInitialized)PaulaMixer_Stop();
    if(spPaulaWorkspace && sPaulaFrequency!=aFreq)PaulaMixer_Shutdown(aOwner);
    if(!PaulaOwner_Claim(aOwner))return 0;
    aFreq=LanceMod_Frequency(aFreq);
    if(!spPaulaWorkspace) {
        U32 bytes=PaulaMixer_GetWorkspaceSize(aFreq);
        /* DMA cannot read alternate/TT RAM; use the ST-RAM allocator. */
        spPaulaWorkspace=mMEMSCREENCALLOC(bytes);
        if(!spPaulaWorkspace){PaulaOwner_Release(aOwner);return 0;}
        sPaulaWorkspaceBytes=bytes;
        sPaulaFrequency=aFreq;
    }
    len=LanceMod_ReplayLenForFreq(aFreq);
    scratch=mMEMALLOC((U32)len);
    if(!scratch) {
        PaulaMixer_Shutdown(aOwner);return 0;
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
    sPaulaOwner=aOwner;
    Paula_SetInitScratch(0);
    mMEMFREE(scratch);
    return 1;
}

void PaulaMixer_Shutdown(U8 aOwner)
{
    if(sPaulaOwner && sPaulaOwner!=aOwner)return;
    if(sPaulaInitialized)PaulaMixer_Stop();
    sPaulaInitialized=0;
    sPaulaOwner=0;
    if(spPaulaWorkspace) {
        mMEMSCREENFREE(spPaulaWorkspace);
    }
    spPaulaWorkspace=0;
    spPaulaReplayBuffer=0;
    sPaulaWorkspaceBytes=0;
    sPaulaFrequency=0;
    Paula_SetReplayBuf(0);
    PaulaOwner_Release(aOwner);
}


U8 PaulaMixer_GetOwner(void){return sPaulaOwner;}
