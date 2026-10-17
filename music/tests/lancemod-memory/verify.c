/* Guard all replay allocations, compare deterministic PCM hashes with baseline. */
#include <godlib/music/lancemod.h>
#include <godlib/platform/platform.h>
#include <godlib/gemdos/gemdos.h>
#include <godlib/file/file.h>
#include <godlib/vbl/vbl.h>
#include <godlib/ikbd/ikbd.h>
#include <godlib/memory/memory.h>
#include "cases.h"
extern U8 *LanceTest_GetReplay(void);
extern U32 LanceTest_GetCodeBytes(void);
extern void LanceMod_MixNext(void);
extern void LanceMod_StartDma(void);
extern void *__real_Memory_Alloc(U32 n);
extern void *__real_Memory_Calloc(U32 n);
extern void *__real_Memory_ScreenCalloc(U32 n);
extern S32 __real_Memory_ScreenRelease(void *p);
extern S32 __real__Memory_Release(void *p);
typedef struct {U8 *base,*ptr;U32 size;} Guard;
static Guard guards[64];
static U16 enabled,busy;
static S16 failAfter=-1;
volatile U16 gLanceTestDone,gLanceTestCase,gLanceTestErrors;
volatile U32 gLanceTestFrames;
/* hash, tracked heap bytes, generated code bytes, module bytes, workspace bytes */
U32 gLanceTestResult[CASE_COUNT*3][5];
static U32 HeapBytes(void) {
 U32 n=0;U16 i;for(i=0;i<64;++i)if(guards[i].ptr)n+=guards[i].size;return n;
}
static void CheckGuards(void) {
 U16 i,j;
 for(i=0;i<64;++i)if(guards[i].ptr)for(j=0;j<16;++j) {
  if(guards[i].base[j]!=0xa5)gLanceTestErrors|=1;
  if(guards[i].ptr[guards[i].size+j]!=0x5a)gLanceTestErrors|=2;
 }
}
static void *Allocate(U32 n,U16 clear) {
 U8 *b;U16 i,j;
 if(!enabled || busy)return clear==2?__real_Memory_ScreenCalloc(n):(clear?__real_Memory_Calloc(n):__real_Memory_Alloc(n));
 if(failAfter==0)return 0;
 if(failAfter>0)--failAfter;
 busy=1;b=clear==2?__real_Memory_ScreenCalloc(n+32):(clear?__real_Memory_Calloc(n+32):__real_Memory_Alloc(n+32));busy=0;
 if(!b)return 0;
 for(i=0;i<64;++i)if(!guards[i].ptr) {
  guards[i].base=b;guards[i].ptr=b+16;guards[i].size=n;
  for(j=0;j<16;++j){b[j]=0xa5;b[16+n+j]=0x5a;}
  return b+16;
 }
 gLanceTestErrors|=4;return 0;
}
void *__wrap_Memory_Alloc(U32 n){return Allocate(n,0);}
void *__wrap_Memory_Calloc(U32 n){return Allocate(n,1);}
void *__wrap_Memory_ScreenCalloc(U32 n){return Allocate(n,2);}
S32 __wrap_Memory_ScreenRelease(void *p) {
 U16 i;
 for(i=0;i<64;++i)if(guards[i].ptr==p && p) {
  CheckGuards();guards[i].ptr=0;return __real_Memory_ScreenRelease(guards[i].base);
 }
 return __real_Memory_ScreenRelease(p);
}
S32 __wrap__Memory_Release(void *p) {
 U16 i;
 for(i=0;i<64;++i)if(guards[i].ptr==p && p) {
  CheckGuards();guards[i].ptr=0;return __real__Memory_Release(guards[i].base);
 }
 return __real__Memory_Release(p);
}
void LanceTest_Done(void) __attribute__((noinline));
void LanceTest_Done(void){__asm__ volatile("" ::: "memory");}
#if defined(TEST_MODSCOPES) && !defined(LANCE_BASELINE)
static S8 scopeGuard[258],stereoGuard[dMODSCOPE_STEREO_BYTES+2];
static void CheckScope(void)
{
 scopeGuard[0]=0x55;scopeGuard[257]=(S8)0xaa;
 LanceMod_GetOscilloscope(scopeGuard+1);
 stereoGuard[0]=0x55;stereoGuard[dMODSCOPE_STEREO_BYTES+1]=(S8)0xaa;
 LanceMod_GetStereoOscilloscope(stereoGuard+1);
 if(stereoGuard[0]!=0x55 || stereoGuard[dMODSCOPE_STEREO_BYTES+1]!=(S8)0xaa)gLanceTestErrors|=32768U;
 if(scopeGuard[0]!=0x55 || scopeGuard[257]!=(S8)0xaa)gLanceTestErrors|=32768U;
}
#else
#define CheckScope() ((void)0)
#endif
S16 GodLib_Game_Main(S16 argc,char **argv) {
 U16 m,rate,f;void *mod;U32 hash;U8 *pcm;sFileHandle output;
 (void)argc;(void)argv;GemDos_Super(0);Platform_Init();File_Init();enabled=1;
#ifdef LANCE_BASELINE
 output=File_Create("OLDPCM.BIN");
#else
 output=File_Create("NEWPCM.BIN");
#endif
 if(output<=0){gLanceTestErrors|=16384;goto done;}
 for(m=0;m<CASE_COUNT;++m)for(rate=1;rate<=3;++rate) {
  U16 c=m*3+rate-1;U32 bytes=1000UL<<(rate-1);
  gLanceTestCase=c;
  mod=LanceMod_Load(caseNames[m]);
  if(!mod){gLanceTestErrors|=8;goto done;}
#ifndef LANCE_BASELINE
  if(HeapBytes()!=caseLoadBytes[m])gLanceTestErrors|=16;
#endif
  gLanceTestResult[c][3]=HeapBytes();
  LanceMod_InitPaula(rate);CheckGuards();
#ifndef LANCE_BASELINE
  if(LanceMod_GetPaulaAllocatedSize()!=LanceMod_GetPaulaWorkspaceSize(rate) ||
     HeapBytes()!=caseLoadBytes[m]+LanceMod_GetPaulaWorkspaceSize(rate))gLanceTestErrors|=32;
  gLanceTestResult[c][4]=LanceMod_GetPaulaAllocatedSize();
#endif
  gLanceTestResult[c][1]=HeapBytes();
  gLanceTestResult[c][2]=LanceTest_GetCodeBytes();
  LanceMod_Init(mod);LanceMod_SetMasterVolume(64);CheckGuards();
  hash=0x12345678UL;
  pcm=LanceTest_GetReplay();
  if(!pcm){gLanceTestErrors|=64;goto done;}
  for(f=0;f<1000;++f) {
   U32 j;LanceMod_MixNext();CheckScope();
   for(j=0;j<bytes;j+=4)hash=((hash<<5)|(hash>>27))^*(U32 *)(pcm+j);
   if(File_Write(output,bytes,pcm)!=(S32)bytes)gLanceTestErrors|=16384;
   CheckGuards();++gLanceTestFrames;
   if(gLanceTestErrors)goto done;
  }
  gLanceTestResult[c][0]=hash;
#ifndef LANCE_BASELINE
  LanceMod_StartDma();
  if(!(*(volatile U8 *)0xffff8901UL&1))gLanceTestErrors|=256;
#endif
  LanceMod_Stop();LanceMod_ShutdownPaula();LanceMod_UnLoad(mod);
#ifndef LANCE_BASELINE
  if(*(volatile U8 *)0xffff8901UL)gLanceTestErrors|=256;
#endif
  if(HeapBytes())gLanceTestErrors|=128;
 }
#ifndef LANCE_BASELINE
 /* Failed workspace/scratch allocations must not hook trap #0 or leak. */
 mod=LanceMod_Load(caseNames[CASE_COUNT-1]);
 if(!mod){gLanceTestErrors|=4096;goto done;}
 {
  U32 savedTrap=*(volatile U32 *)0x80UL,held=HeapBytes();
  for(failAfter=0;failAfter<=1;++failAfter) {
   S16 attempt=failAfter;
   if(LanceMod_Start(mod,eLANCEMOD_FREQ_25K))gLanceTestErrors|=4096;
   failAfter=-1;LanceMod_Stop();LanceMod_ShutdownPaula();
   if(HeapBytes()!=held || *(volatile U32 *)0x80UL!=savedTrap)gLanceTestErrors|=8192;
   failAfter=attempt;
  }
  failAfter=-1;
 }
 LanceMod_UnLoad(mod);
 /* Same-rate reuse, changing rates, repeated shutdown and rejected modules. */
 for(rate=1;rate<=3;++rate) {
  LanceMod_InitPaula(rate);LanceMod_InitPaula(rate);CheckGuards();
  if(HeapBytes()!=LanceMod_GetPaulaWorkspaceSize(rate))gLanceTestErrors|=256;
 }
 LanceMod_ShutdownPaula();LanceMod_ShutdownPaula();
 if(HeapBytes() || LanceMod_GetPaulaAllocatedSize())gLanceTestErrors|=512;
 for(m=0;m<BAD_COUNT;++m) {
  mod=LanceMod_Load(badNames[m]);
  if(mod){gLanceTestErrors|=1024;LanceMod_UnLoad(mod);}
 }
 if(HeapBytes())gLanceTestErrors|=2048;
#endif
 done:
 if(output>0)File_Close(output);
 CheckGuards();gLanceTestDone=1;LanceTest_Done();
 while(!IKBD_GetKbdBytesWaiting())Vbl_WaitVbl();
 File_DeInit();Platform_DeInit();return 0;
}
