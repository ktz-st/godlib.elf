#include <godlib/music/wizzcat.h>
#include <godlib/platform/platform.h>
#include <godlib/gemdos/gemdos.h>
#include <godlib/file/file.h>
#include <godlib/vbl/vbl.h>
#include <godlib/ikbd/ikbd.h>
#include <godlib/memory/memory.h>
#include "cases.h"
extern void WizTest_Mix(void);
extern U8 *WizTest_GetReplay(void);
extern void *WizTest_GetTables(void);
extern void *WizTest_GetVolume(void);
extern void *__real_Memory_Alloc(U32 n);
extern void *__real_Memory_Calloc(U32 n);
extern void *__real_Memory_ScreenCalloc(U32 n);
extern S32 __real_Memory_ScreenRelease(void *p);
extern S32 __real__Memory_Release(void *p);
typedef struct {U8 *base,*ptr;U32 size;} Guard;
static Guard guards[64];
static U16 enabled,busy;
static S16 failAfter=-1;
volatile U16 gWizTestDone,gWizTestCase,gWizTestErrors;
volatile U32 gWizTestFrames;
/* hash, tracked heap bytes, generated code bytes, module bytes, workspace bytes */
U32 gWizTestResult[CASE_COUNT][5];
static U32 HeapBytes(void) {
 U32 n=0;U16 i;for(i=0;i<64;++i)if(guards[i].ptr)n+=guards[i].size;return n;
}
static void CheckGuards(void) {
 U16 i,j;
 for(i=0;i<64;++i)if(guards[i].ptr)for(j=0;j<16;++j) {
  if(guards[i].base[j]!=0xa5)gWizTestErrors|=1;
  if(guards[i].ptr[guards[i].size+j]!=0x5a)gWizTestErrors|=2;
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
 gWizTestErrors|=4;return 0;
}
void *__wrap_Memory_Alloc(U32 n){return Allocate(n,0);}
void *__wrap_Memory_Calloc(U32 n){return Allocate(n,1);}
void *__wrap_Memory_ScreenCalloc(U32 n){return Allocate(n,2);}
S32 __wrap_Memory_ScreenRelease(void *p) {
 U16 i;
 for(i=0;i<64;++i)if(guards[i].ptr==p && p) {
  if(*(volatile U8 *)0xffff8901UL&1)gWizTestErrors|=256;
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


#if defined(TEST_MODSCOPES) && !defined(WIZ_BASELINE)
static S8 scopeGuard[258],stereoGuard[dMODSCOPE_STEREO_BYTES+2];
static void CheckScope(void)
{
 scopeGuard[0]=0x55;scopeGuard[257]=(S8)0xaa;
 Wizzcat_GetOscilloscope(scopeGuard+1);
 stereoGuard[0]=0x55;stereoGuard[dMODSCOPE_STEREO_BYTES+1]=(S8)0xaa;
 Wizzcat_GetStereoOscilloscope(stereoGuard+1);
 if(stereoGuard[0]!=0x55 || stereoGuard[dMODSCOPE_STEREO_BYTES+1]!=(S8)0xaa)gWizTestErrors|=32768U;
 if(scopeGuard[0]!=0x55 || scopeGuard[257]!=(S8)0xaa)gWizTestErrors|=32768U;
}
#else
#define CheckScope() ((void)0)
#endif
S16 GodLib_Game_Main(S16 argc,char **argv) {
 U16 m,f;U8 *pcm;U32 hash,t,held;sWizzcatModule *mod;sFileHandle output,tables;
 (void)argc;(void)argv;GemDos_Super(0);Platform_Init();File_Init();enabled=1;
 output=File_Create("PCM.BIN");tables=File_Create("TABLES.BIN");
 if(output<=0 || tables<=0){gWizTestErrors|=16384;goto done;}
 for(m=0;m<CASE_COUNT;++m) {
  gWizTestCase=m;mod=Wizzcat_Load(caseNames[m]);
  if(!mod){gWizTestErrors|=8;goto done;}
  gWizTestResult[m][0]=HeapBytes();gWizTestResult[m][1]=mod->mWorkspaceSize;
#ifndef WIZ_BASELINE
  if(HeapBytes()!=caseLoadBytes[m])gWizTestErrors|=16;
#endif
  held=HeapBytes();t=*(volatile U32 *)0x4baUL;
  Wizzcat_Init(mod);t=*(volatile U32 *)0x4baUL-t;
  gWizTestResult[m][2]=t;
  if(HeapBytes()!=held)gWizTestErrors|=32; /* transient copy has been released */
  CheckGuards();Wizzcat_Play();hash=0x12345678UL;
  if(m==0) {
   File_Write(tables,13088UL,WizTest_GetTables());
   File_Write(tables,16640UL,WizTest_GetVolume());
  }
  for(f=0;f<1000;++f) {
   U32 j;WizTest_Mix();CheckScope();pcm=WizTest_GetReplay();
   for(j=0;j<2000UL;j+=4)hash=((hash<<5)|(hash>>27))^*(U32 *)(pcm+j);
   if(File_Write(output,2000UL,pcm)!=2000L)gWizTestErrors|=16384;
   CheckGuards();++gWizTestFrames;if(gWizTestErrors)goto done;
#ifndef WIZ_BASELINE
   {U16 ages[4],i;Wizzcat_GetVisualizer(ages);for(i=0;i<4;++i)if(ages[i]>0x4000)gWizTestErrors|=64;}
#endif
  }
  gWizTestResult[m][3]=hash;gWizTestResult[m][4]=mod->mDataSize;
  Wizzcat_StopVbl();Wizzcat_UnLoad(mod);
  if(*(volatile U8 *)0xffff8901UL || HeapBytes())gWizTestErrors|=128;
 }
#ifndef WIZ_BASELINE
 /* Workspace OOM, scratch OOM, too small explicit margins and repeated Start. */
 failAfter=0;mod=Wizzcat_Load(caseNames[0]);failAfter=-1;
 if(mod){gWizTestErrors|=256;Wizzcat_UnLoad(mod);}
 mod=Wizzcat_Load(caseNames[CASE_COUNT-1]);held=HeapBytes();failAfter=0;
 if(!mod || Wizzcat_Start(mod))gWizTestErrors|=512;
 failAfter=-1;
 if(HeapBytes()!=held || *(volatile U8 *)0xffff8901UL)gWizTestErrors|=1024;
 if(!Wizzcat_Start(mod))gWizTestErrors|=2048;
 if(Wizzcat_Start(mod))gWizTestErrors|=2048; /* already prepared: reload required */
 Wizzcat_Stop();Wizzcat_StopVbl();Wizzcat_UnLoad(mod);
 mod=Wizzcat_LoadEx(caseNames[0],2UL);
 if(mod){gWizTestErrors|=4096;Wizzcat_UnLoad(mod);}
 for(m=0;m<BAD_COUNT;++m) {
  mod=Wizzcat_Load(badNames[m]);if(mod){gWizTestErrors|=8192;Wizzcat_UnLoad(mod);}
 }
 /* UnLoad a playing module removes its callback before releasing memory. */
 mod=Wizzcat_Load(caseNames[CASE_COUNT-1]);
 if(!mod || !Wizzcat_Start(mod))gWizTestErrors|=2048;
 Wizzcat_UnLoad(mod);Wizzcat_StopVbl();
 if(*(volatile U8 *)0xffff8901UL || HeapBytes())gWizTestErrors|=128;
#endif
 done:
 if(output>0)File_Close(output);
 if(tables>0)File_Close(tables);
 CheckGuards();gWizTestDone=1;
 while(!IKBD_GetKbdBytesWaiting())Vbl_WaitVbl();
 File_DeInit();Platform_DeInit();return 0;
}
