#include <godlib/memory/heap.h>
#include <godlib/memory/memory.h>
#ifdef HEAP_HOST
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static U32 hostAllocs,hostFrees,hostFail;
void *Memory_Alloc(U32 size){++hostAllocs;if(hostFail){hostFail=0;return 0;}return malloc(size);}
S32 _Memory_Release(void *p){++hostFrees;free(p);return 0;}
void Memory_Clear(U32 size,void *p){memset(p,0,size);}
#else
#include <godlib/platform/platform.h>
#include <godlib/gemdos/gemdos.h>
#endif
static U32 result[68]={0x48454150UL};
#define CHECK(c) do{++result[1];if(!(c)){if(!result[2])result[3]=__LINE__;if(result[2]<64)result[4+result[2]]=__LINE__;++result[2];}}while(0)
static union { U32 align; U8 bytes[8200]; } backing;
static union { U32 align; U8 bytes[256]; } foreign;
static U32 randomState=0x12345678UL;
static U32 rnd(void){randomState=randomState*1664525UL+1013904223UL;return randomState;}
static void fill(void *ptr,U32 size,U8 seed)
{U32 i;for(i=0;i<size;++i)((U8*)ptr)[i]=(U8)(seed+i);}
static U32 pattern(const void *ptr,U32 size,U8 seed)
{U32 i;for(i=0;i<size;++i)if(((const U8*)ptr)[i]!=(U8)(seed+i))return 0;return 1;}
static void stats(sHeap *h,sHeapStats *s)
{CHECK(Heap_GetStats(h,s));CHECK(s->mTotalBytes==s->mUsedBytes+s->mFreeBytes+s->mOverheadBytes);CHECK(s->mRequestedBytes<=s->mUsedBytes);CHECK(s->mLargestFreeBlock<=s->mFreeBytes);CHECK(Heap_Validate(h));}
static void fullFree(sHeap *h,U32 total)
{sHeapStats s;stats(h,&s);CHECK(s.mTotalBytes==total);CHECK(s.mUsedBlocks==0);CHECK(s.mFreeBlocks==1);CHECK(s.mFreeBytes==total-16);CHECK(s.mLargestFreeBlock==total-16);CHECK(s.mUsedBytes==0&&s.mRequestedBytes==0);}
static void basic(void)
{
 sHeap h={0},other={0};sHeapStats s;void *a,*b,*c,*d;U32 oldPeak;
 CHECK(!Heap_Init(0,1024));CHECK(!Heap_Reset(&h));CHECK(!Heap_Validate(&h));CHECK(!Heap_GetStats(&h,&s));CHECK(!Heap_Alloc(&h,16));CHECK(!Heap_FreeChecked(&h,0));
 CHECK(Heap_DeInit(&h));CHECK(!Heap_Init(&h,0));CHECK(!Heap_Init(&h,23));CHECK(!Heap_Init(&h,0xffffffffUL));CHECK(!Heap_Init(&h,0x80000000UL));CHECK(!Heap_InitWithBuffer(&h,0,1024));
 CHECK(!Heap_InitWithBuffer(&h,backing.bytes,23));CHECK(Heap_InitWithBuffer(&h,backing.bytes,1024));fullFree(&h,1024);
 CHECK(!Heap_Init(&h,64));CHECK(!Heap_InitWithBuffer(&h,foreign.bytes,128));fullFree(&h,1024);
 CHECK(Heap_FreeChecked(&h,0));Heap_Free(&h,0);
 CHECK(!Heap_Alloc(&h,0));CHECK(!Heap_Alloc(&h,0xffffffffUL));CHECK(!Heap_Alloc(&h,1009));stats(&h,&s);CHECK(s.mFailedAllocCount==3);
 a=Heap_Alloc(&h,1);b=Heap_Alloc(&h,13);c=Heap_Alloc(&h,32);CHECK(a&&b&&c);
 if(a&&b&&c){
  CHECK(!((UPTR)a&3)&&!((UPTR)b&3)&&!((UPTR)c&3));fill(a,1,1);fill(b,13,2);fill(c,32,3);
  stats(&h,&s);CHECK(s.mUsedBytes==8+16+32);CHECK(s.mRequestedBytes==46);CHECK(s.mUsedBlocks==3);CHECK(s.mAllocationCount==3);
  oldPeak=s.mPeakUsedBytes;
  CHECK(!Heap_FreeChecked(&h,(U8*)b+1));CHECK(!Heap_FreeChecked(&h,(U8*)b+4));CHECK(!Heap_FreeChecked(&h,backing.bytes));CHECK(!Heap_FreeChecked(&h,backing.bytes+1024));CHECK(!Heap_FreeChecked(&h,foreign.bytes));
  CHECK(Heap_InitWithBuffer(&other,foreign.bytes,128));CHECK(!Heap_FreeChecked(&other,a));CHECK(Heap_DeInit(&other));
  CHECK(pattern(a,1,1)&&pattern(b,13,2)&&pattern(c,32,3));stats(&h,&s);CHECK(s.mInvalidFreeCount==5);
  CHECK(Heap_FreeChecked(&h,b));CHECK(!Heap_FreeChecked(&h,b));stats(&h,&s);CHECK(s.mFreeBlocks==2);CHECK(s.mPeakUsedBytes==oldPeak);
  d=Heap_Alloc(&h,13);CHECK(d==b);fill(d,13,7);CHECK(pattern(a,1,1)&&pattern(c,32,3));
  CHECK(Heap_FreeChecked(&h,a));CHECK(Heap_FreeChecked(&h,c));CHECK(Heap_FreeChecked(&h,d));fullFree(&h,1024); /* merge previous AND next */
 }
 CHECK(Heap_Reset(&h));stats(&h,&s);CHECK(s.mAllocationCount==0&&s.mFreeCount==0&&s.mFailedAllocCount==0&&s.mInvalidFreeCount==0&&s.mPeakUsedBytes==0);
 a=Heap_Alloc(&h,40);b=Heap_Alloc(&h,40);c=Heap_Alloc(&h,40);d=Heap_Alloc(&h,40);CHECK(a&&b&&c&&d);
 CHECK(Heap_Reset(&h));CHECK(!Heap_FreeChecked(&h,c)); /* old interior headers after Reset */
 fullFree(&h,1024);
 a=Heap_Alloc(&h,1008);CHECK(a!=0);CHECK(!Heap_Alloc(&h,1));stats(&h,&s);CHECK(s.mFreeBytes==0&&s.mFreeBlocks==0&&s.mUsedBytes==1008);Heap_Free(&h,a);fullFree(&h,1024);
 /* Tail too small for a free block becomes payload capacity, not lost bytes. */
 a=Heap_Alloc(&h,996);CHECK(a!=0);stats(&h,&s);CHECK(s.mUsedBytes==1008&&s.mRequestedBytes==996&&s.mFreeBlocks==0);Heap_Free(&h,a);
 CHECK(Heap_DeInit(&h));CHECK(Heap_DeInit(&h));CHECK(!Heap_Validate(&h));CHECK(h.mpBase==0&&h.mpAllocation==0);
 /* Own one system block, retain it through Reset, release only at DeInit. */
#ifdef HEAP_HOST
 {U32 ac=hostAllocs,fc=hostFrees;hostFail=1;CHECK(!Heap_Init(&h,1024));CHECK(!h.mpBase);CHECK(hostAllocs==ac+1&&hostFrees==fc);}
#endif
 CHECK(Heap_Init(&h,1025));fullFree(&h,1024);CHECK(h.mpAllocation!=0);
#ifdef HEAP_HOST
 {U32 ac=hostAllocs,fc=hostFrees;a=Heap_Alloc(&h,200);CHECK(a!=0);CHECK(Heap_Reset(&h));CHECK(hostAllocs==ac&&hostFrees==fc);CHECK(Heap_DeInit(&h));CHECK(hostFrees==fc+1);CHECK(Heap_DeInit(&h));CHECK(hostFrees==fc+1);}
#else
 a=Heap_Alloc(&h,200);CHECK(a!=0);CHECK(Heap_Reset(&h));CHECK(Heap_DeInit(&h));
#endif
 /* Validation rejects corrupt tags, while Reset recovers the arena. */
 CHECK(Heap_InitWithBuffer(&h,backing.bytes,128));((U32*)h.mpBase)[3]^=1;CHECK(!Heap_Validate(&h));CHECK(!Heap_GetStats(&h,&s));CHECK(Heap_Reset(&h));CHECK(Heap_Validate(&h));Heap_DeInit(&h);
}
static void fragmentation(void)
{
 sHeap h={0};sHeapStats s;void *a,*b,*c,*p;
 CHECK(Heap_InitWithBuffer(&h,foreign.bytes,256));
 a=Heap_Alloc(&h,64);b=Heap_Alloc(&h,64);c=Heap_Alloc(&h,64);CHECK(a&&b&&c);
 Heap_Free(&h,a);Heap_Free(&h,c);stats(&h,&s);
 CHECK(s.mFreeBytes==144&&s.mLargestFreeBlock==80&&s.mFreeBlocks==2);
 CHECK(!Heap_Alloc(&h,100));CHECK(Heap_FreeChecked(&h,b));fullFree(&h,256);
 p=Heap_Alloc(&h,240);CHECK(p!=0);Heap_Free(&h,p);Heap_DeInit(&h);
}
static void large(void)
{
 sHeap h={0};sHeapStats s;void *a,*b,*c;
 CHECK(Heap_Init(&h,200003UL));if(!h.mpBase)return;
 fullFree(&h,200000UL);a=Heap_Alloc(&h,70001UL);b=Heap_Alloc(&h,65003UL);c=Heap_Alloc(&h,30001UL);CHECK(a&&b&&c);
 if(a&&b&&c){
  fill(a,70001UL,17);fill(b,65003UL,81);fill(c,30001UL,149);
  stats(&h,&s);CHECK(s.mRequestedBytes==165005UL);CHECK((UPTR)c-(UPTR)h.mpBase>65535UL);
  CHECK(pattern(a,70001UL,17)&&pattern(b,65003UL,81)&&pattern(c,30001UL,149));
  Heap_Free(&h,b);Heap_Free(&h,a);CHECK(pattern(c,30001UL,149));Heap_Free(&h,c);fullFree(&h,200000UL);
 }
 Heap_DeInit(&h);
}
static void alignment(void)
{
 sHeap h={0};U32 offset,i,total;U8 *base=backing.bytes;void *p;
 for(offset=0;offset<4;++offset){
  fill(base,8200,0x35);CHECK(Heap_InitWithBuffer(&h,base+4+offset,1003));
  total=(1003-((4-offset)&3))&~3UL;fullFree(&h,total);p=Heap_Alloc(&h,total-16);CHECK(p!=0);CHECK(!((UPTR)p&3));fill(p,total-16,0x77);Heap_Free(&h,p);fullFree(&h,total);Heap_DeInit(&h);
  for(i=0;i<4+offset;++i)CHECK(base[i]==(U8)(0x35+i));
  for(i=4+offset+1003;i<8200;++i)if(base[i]!=(U8)(0x35+i))break;
  CHECK(i==8200);
 }
}
typedef struct sSlot {void *p;U32 size;U8 seed;} sSlot;
static sSlot slots[48];
static void stress(void)
{
 sHeap h={0};sHeapStats s;U32 step,i,j,total=0,live=0;U8 seed;
 fill(backing.bytes,8200,0x19);CHECK(Heap_InitWithBuffer(&h,backing.bytes+4,8192));
 for(step=0;step<3000;++step){
  i=(rnd()>>16)%48;
  if(slots[i].p){CHECK(pattern(slots[i].p,slots[i].size,slots[i].seed));CHECK(Heap_FreeChecked(&h,slots[i].p));total-=slots[i].size;--live;slots[i].p=0;}
  else{
   U32 size=1+(rnd()>>16)%400;void *p=Heap_Alloc(&h,size);
   if(p){seed=(U8)rnd();slots[i].p=p;slots[i].size=size;slots[i].seed=seed;fill(p,size,seed);total+=size;++live;CHECK(!((UPTR)p&3));}
  }
  stats(&h,&s);CHECK(s.mRequestedBytes==total&&s.mUsedBlocks==live);
  for(j=0;j<48;++j)if(slots[j].p)CHECK(pattern(slots[j].p,slots[j].size,slots[j].seed));
 }
 for(i=0;i<48;++i)if(slots[i].p){CHECK(Heap_FreeChecked(&h,slots[i].p));slots[i].p=0;}
 fullFree(&h,8192);CHECK(pattern(backing.bytes,4,0x19));CHECK(pattern(backing.bytes+8196,4,(U8)(0x19+8196)));Heap_DeInit(&h);
}
#ifdef HEAP_HOST
int main(void)
{basic();fragmentation();large();alignment();stress();printf("HEAP: %lu checks, %lu failures, first line %lu\n",(unsigned long)result[1],(unsigned long)result[2],(unsigned long)result[3]);return result[2]!=0;}
#else
S16 GodLib_Game_Main(S16 argc,char **argv)
{
 S16 handle;(void)argc;(void)argv;GemDos_Super(0);Platform_Init();basic();fragmentation();large();alignment();stress();
 handle=GemDos_Fcreate("RESULT.BIN",0);if(handle>=0){GemDos_Fwrite(handle,sizeof(result),result);GemDos_Fclose(handle);}Platform_DeInit();return result[2]?1:0;
}
#endif
