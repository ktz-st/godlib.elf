#include <godlib/music/leonardmod.h>
#include <godlib/platform/platform.h>
#include <godlib/gemdos/gemdos.h>
#include <godlib/file/file.h>
#include <godlib/vbl/vbl.h>
#include <godlib/ikbd/ikbd.h>
#include <godlib/memory/memory.h>
#include "cases.h"
extern void LeoTest_Mix(void);
extern U8 *LeoTest_GetReplay(void);
extern U32 LeoTest_GetCodeBytes(void);
extern void LeoTest_High(U16 period);
extern void *__real_Memory_Alloc(U32 n);
extern void *__real_Memory_Calloc(U32 n);
extern void *__real_Memory_ScreenCalloc(U32 n);
extern S32 __real_Memory_ScreenRelease(void *p);
extern S32 __real__Memory_Release(void *p);
typedef struct {U8 *base,*ptr;U32 size;} Guard;
static Guard guards[64];
static U16 enabled,busy;
static S16 failAfter=-1;
volatile U16 gLeoTestDone,gLeoTestCase,gLeoTestErrors;
volatile U32 gLeoTestFrames;
/* hash, tracked heap bytes, generated code bytes, module bytes, workspace bytes */
U32 gLeoTestResult[CASE_COUNT*2][5];
static U32 HeapBytes(void) {
 U32 n=0;U16 i;for(i=0;i<64;++i)if(guards[i].ptr)n+=guards[i].size;return n;
}
static void CheckGuards(void) {
 U16 i,j;
 for(i=0;i<64;++i)if(guards[i].ptr)for(j=0;j<16;++j) {
  if(guards[i].base[j]!=0xa5)gLeoTestErrors|=1;
  if(guards[i].ptr[guards[i].size+j]!=0x5a)gLeoTestErrors|=2;
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
 gLeoTestErrors|=4;return 0;
}
void *__wrap_Memory_Alloc(U32 n){return Allocate(n,0);}
void *__wrap_Memory_Calloc(U32 n){return Allocate(n,1);}
void *__wrap_Memory_ScreenCalloc(U32 n){return Allocate(n,2);}
S32 __wrap_Memory_ScreenRelease(void *p) {
 U16 i;
 for(i=0;i<64;++i)if(guards[i].ptr==p && p) {
  if(*(volatile U8 *)0xffff8901UL&1)gLeoTestErrors|=256;
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

#if defined(TEST_MODSCOPES) && !defined(LEO_BASELINE)
static S8 scopeGuard[258],stereoGuard[dMODSCOPE_STEREO_BYTES+2];
static void CheckScope(void)
{
 scopeGuard[0]=0x55;scopeGuard[257]=(S8)0xaa;
 LeonardMod_GetOscilloscope(scopeGuard+1);
 stereoGuard[0]=0x55;stereoGuard[dMODSCOPE_STEREO_BYTES+1]=(S8)0xaa;
 LeonardMod_GetStereoOscilloscope(stereoGuard+1);
 if(stereoGuard[0]!=0x55 || stereoGuard[dMODSCOPE_STEREO_BYTES+1]!=(S8)0xaa)gLeoTestErrors|=32768U;
 if(scopeGuard[0]!=0x55 || scopeGuard[257]!=(S8)0xaa)gLeoTestErrors|=32768U;
}
#else
#define CheckScope() ((void)0)
#endif
S16 GodLib_Game_Main(S16 argc,char **argv) {
 U16 m,rate,f;void *mod;U32 trap,hash;U8 *pcm;sFileHandle output;
 (void)argc;(void)argv;GemDos_Super(0);Platform_Init();File_Init();enabled=1;
 output=File_Create("PCM.BIN");
 if(output<=0){gLeoTestErrors|=16384;goto done;}
 for(m=0;m<CASE_COUNT;++m)for(rate=FIRST_RATE;rate<=2;++rate) {
  U16 c=m*2+rate-1;U32 bytes=rate==1?508UL:1008UL;
  gLeoTestCase=c;mod=LeonardMod_Load(caseNames[m]);
  if(!mod){gLeoTestErrors|=8;goto done;}
  gLeoTestResult[c][3]=HeapBytes();
  if(HeapBytes()!=caseLoadBytes[m])gLeoTestErrors|=16;
  trap=*(volatile U32 *)0x80UL;
  if(!LeonardMod_Start(mod,rate)){gLeoTestErrors|=32;goto done;}
  if(LeonardMod_GetPaulaAllocatedSize()!=LeonardMod_GetPaulaWorkspaceSize(rate) ||
     HeapBytes()!=caseLoadBytes[m]+LeonardMod_GetPaulaWorkspaceSize(rate))gLeoTestErrors|=32;
  gLeoTestResult[c][4]=LeonardMod_GetPaulaAllocatedSize();
  gLeoTestResult[c][1]=HeapBytes();gLeoTestResult[c][2]=LeoTest_GetCodeBytes();
  hash=0x12345678UL;pcm=LeoTest_GetReplay();
  /* Stop DMA reads without changing replay state; deterministic offline mix. */
  *(volatile U8 *)0xffff8901UL=0;
  for(f=0;f<1000;++f) {
   U32 j;LeoTest_Mix();CheckScope();
   for(j=0;j<bytes;j+=4)hash=((hash<<5)|(hash>>27))^*(U32 *)(pcm+j);
   if(File_Write(output,bytes,pcm)!=(S32)bytes)gLeoTestErrors|=16384;
   CheckGuards();++gLeoTestFrames;if(gLeoTestErrors)goto done;
  }
  gLeoTestResult[c][0]=hash;
#ifndef LEO_BASELINE
  if(rate==1 && m==0) {
   static const U16 notes[]={108,113,127,141};sFileHandle high=File_Create("HIGH.BIN");
   for(f=0;f<4;++f){LeoTest_High(notes[f]);File_Write(high,508UL,pcm);CheckGuards();}
   File_Close(high);
  }
#endif
  LeonardMod_Stop();LeonardMod_ShutdownPaula();LeonardMod_UnLoad(mod);
  if(*(volatile U8 *)0xffff8901UL || *(volatile U32 *)0x80UL!=trap)gLeoTestErrors|=256;
  if(HeapBytes())gLeoTestErrors|=128;
 }
#ifndef LEO_BASELINE
 mod=LeonardMod_Load(caseNames[CASE_COUNT-1]);
 if(!mod){gLeoTestErrors|=4096;goto done;}
 trap=*(volatile U32 *)0x80UL;failAfter=0;
 if(LeonardMod_Start(mod,1))gLeoTestErrors|=4096;
 failAfter=-1;LeonardMod_ShutdownPaula();
 if(HeapBytes()!=caseLoadBytes[CASE_COUNT-1] || *(volatile U32 *)0x80UL!=trap)gLeoTestErrors|=8192;
 LeonardMod_UnLoad(mod);
 /* Reload raw MOD for every preparation, switch rates without a manual shutdown. */
 {
  void *previous=0;
  for(rate=1;rate<=2;++rate) {
   void *next=LeonardMod_Load(caseNames[CASE_COUNT-1]);
   if(!next || !LeonardMod_Start(next,rate))gLeoTestErrors|=256;
   LeonardMod_UnLoad(previous);previous=next;CheckGuards();
  }
  LeonardMod_Stop();LeonardMod_UnLoad(previous);
 }
 LeonardMod_ShutdownPaula();LeonardMod_ShutdownPaula();
 if(HeapBytes() || LeonardMod_GetPaulaAllocatedSize())gLeoTestErrors|=512;
 if(LeonardMod_Start(0,1) || LeonardMod_GetPaulaAllocatedSize())gLeoTestErrors|=512;
 for(m=0;m<BAD_COUNT;++m) {
  mod=LeonardMod_Load(badNames[m]);if(mod){gLeoTestErrors|=1024;LeonardMod_UnLoad(mod);}
 }
 if(HeapBytes())gLeoTestErrors|=2048;
#endif
 done:
 if(output>0)File_Close(output);
 CheckGuards();gLeoTestDone=1;
 while(!IKBD_GetKbdBytesWaiting())Vbl_WaitVbl();
 File_DeInit();Platform_DeInit();return 0;
}
