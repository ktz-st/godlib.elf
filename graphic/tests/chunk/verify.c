#include <godlib/graphic/graphic.h>
#include <godlib/platform/platform.h>
#include <godlib/gemdos/gemdos.h>
extern void BaselineRestore(sGraphicChunkList*,sGraphicCanvas*,sGraphicCanvas*);
#define WORDS 75002UL
static U16 source[WORDS],actual[WORDS],expected[WORDS];
static U32 offsets[433];
volatile U16 gChunkDone,gChunkFailures,gChunkCases,gChunkPhase;
volatile U32 gChunkTimes[12];
static sGraphicChunkList list;
mGODLIB_CHUNKLIST(rawList,40);
static void verify(sGraphicCanvas *src,sGraphicCanvas *dst,U16 n)
{
 U32 i,off;U16 row,col,k; sGraphicRect rect;
 for(i=0;i<WORDS;++i) {source[i]=(U16)(i*73+19);actual[i]=expected[i]=0xa55a;}
 list.mActiveCount=0;list.mChunkCount=32;
 rect.mX=(n%3)*16+((n&1)?7:0);rect.mY=(n&2)?390:5;
 rect.mWidth=(n%6+1)*16;rect.mHeight=1+(n%5)*7;
 Graphic_ChunkList_Store(&list,src,&rect);
 /* Reference consumes each recorded column without combining or ASM copying. */
 for(k=0;k<list.mActiveCount;++k) {
  off=offsets[rect.mY]+((U32)(rect.mX>>4)+k)*8;
  if(list.mChunks[k].mOffset!=off) ++gChunkFailures;
 }
 if(n%4==0 && list.mActiveCount) list.mChunks[0].mHeight=0;
 if(n%4==1 && list.mActiveCount>1) list.mChunks[1].mHeight=2;
 if(n%4==2 && list.mActiveCount>2) list.mChunks[2].mOffset+=src->mpLineOffsets[1];
 for(k=0;k<list.mActiveCount;++k) for(row=0;row<list.mChunks[k].mHeight;++row) {
  off=(list.mChunks[k].mOffset+(U32)row*offsets[1])/2+1;
  for(col=0;col<4;++col) expected[off+col]=source[off+col];
 }
 Graphic_ChunkList_ReStore(&list,src,dst);
 if(list.mActiveCount)++gChunkFailures;
 for(i=0;i<WORDS;++i)if(actual[i]!=expected[i]) {++gChunkFailures;break;}
 ++gChunkCases;
}
static void timing(sGraphicCanvas *src,sGraphicCanvas *dst)
{
 U16 kind,v,n;U32 start;sGraphicRect rect;
 for(kind=0;kind<6;++kind)for(v=0;v<2;++v) {
  rect.mX=(kind==1 || kind==3)?7:16;rect.mY=20;rect.mWidth=kind<2?16:kind<4?32:kind==4?64:256;rect.mHeight=kind<2?16:kind<4?32:64;
  list.mActiveCount=0;Graphic_ChunkList_Store(&list,src,&rect);
  {U16 count=list.mActiveCount;gChunkPhase=100+kind*2+v;start=*(volatile U32*)0x4baL;
   for(n=0;n<2048;++n) {list.mActiveCount=count;if(v)BaselineRestore(&list,src,dst);else Graphic_ChunkList_ReStore(&list,src,dst);}
   gChunkTimes[kind*2+v]=*(volatile U32*)0x4baL-start;
  }
 }
}
S16 GodLib_Game_Main(S16 argc,char **argv)
{
 sGraphicCanvas src,dst;U16 layout,row,n;U16 strides[]={160,320,336};sGraphicRect full={0,0,512,1};
 (void)argc;(void)argv;GemDos_Super(0);Platform_Init();
 src.mpVRAM=source+1;dst.mpVRAM=actual+1;src.mpLineOffsets=dst.mpLineOffsets=offsets;
 src.mHeight=dst.mHeight=432;
 for(layout=0;layout<3;++layout) {
  src.mWidth=dst.mWidth=layout?640:320;
  for(row=0;row<=432;++row)offsets[row]=(U32)row*strides[layout];
  for(n=0;n<24;++n) {gChunkPhase=layout*24+n;verify(&src,&dst,n);}
 }
 /* Full 32-entry capacity and larger macro allocation, plus a >32-column run. */
 list.mActiveCount=0;list.mChunkCount=32;Graphic_ChunkList_Store(&list,&src,&full);
 if(list.mActiveCount!=32)++gChunkFailures;
 Graphic_ChunkList_ReStore(&list,&src,&dst);
 {sGraphicChunkList *p=(sGraphicChunkList*)rawList;full.mWidth=640;Graphic_ChunkList_Store(p,&src,&full);
 if(p->mActiveCount!=40)++gChunkFailures;
 Graphic_ChunkList_ReStore(p,&src,&dst);
 for(n=0;n<160;++n)if(actual[n+1]!=source[n+1])++gChunkFailures;
 if(p->mActiveCount)++gChunkFailures;}
 ++gChunkCases;
 for(row=0;row<=432;++row)offsets[row]=(U32)row*160;
 timing(&src,&dst);Platform_DeInit();gChunkDone=1;for(;;){}return 0;
}
