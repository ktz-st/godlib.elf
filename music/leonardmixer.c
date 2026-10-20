/* Shared Leonard Paula allocator; independent of the ProTracker sequencer. */
#include "leonardmixer.h"
#include "paulaowner.h"
#include <godlib/memory/memory.h>
extern U8 LeonardMixer_CoreInit(void *const *buffers,U8 rate);
static void *spLeoWorkspace;
static U32 sLeoWorkspaceBytes;
static U8 sLeoOwner;
U32 LeonardMixer_GetWorkspaceSize(U8 rate){return (rate==1?254UL:504UL)*120UL+48UL+13696UL+16895UL+15UL;}
U32 LeonardMixer_GetAllocatedSize(void){return sLeoWorkspaceBytes;}
U8 LeonardMixer_GetOwner(void){return sLeoOwner;}
void LeonardMixer_Shutdown(U8 owner)
{
 if(sLeoOwner && sLeoOwner!=owner)return;
 LeonardMixer_Stop();
 if(spLeoWorkspace){mMEMSCREENFREE(spLeoWorkspace);}
 spLeoWorkspace=0;sLeoWorkspaceBytes=0;sLeoOwner=0;PaulaOwner_Release(owner+2);
}
U8 LeonardMixer_Init(U8 owner,U8 rate)
{
 U8 *base;void *buffers[4];U32 n=rate==1?254UL:504UL;
 if(sLeoOwner && sLeoOwner!=owner)return 0;
 LeonardMixer_Shutdown(owner);
 if(!PaulaOwner_Claim(owner+2))return 0;
 spLeoWorkspace=mMEMSCREENCALLOC(LeonardMixer_GetWorkspaceSize(rate));
 if(!spLeoWorkspace){PaulaOwner_Release(owner+2);return 0;}
 sLeoWorkspaceBytes=LeonardMixer_GetWorkspaceSize(rate);sLeoOwner=owner;
 base=(U8*)(((UPTR)spLeoWorkspace+15)&~(UPTR)15);
 buffers[0]=base;buffers[1]=base+n*8UL;
 buffers[3]=base+n*8UL+13696UL;buffers[2]=base+n*120UL+13696UL+48UL;
 if(LeonardMixer_CoreInit(buffers,rate))return 1;
 LeonardMixer_Shutdown(owner);return 0;
}
