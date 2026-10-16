#include <godlib/graphic/graphic.h>
#include <godlib/platform/platform.h>
#include <godlib/gemdos/gemdos.h>
extern void Graphic_4BP_DrawLine(sGraphicCanvas*,sGraphicBox*,S16);
extern void Graphic_4BP_DrawLine_Clip(sGraphicCanvas*,sGraphicBox*,S16);
extern void Graphic_4BP_DrawLine_BLT(sGraphicCanvas*,sGraphicBox*,S16);
extern void Graphic_4BP_DrawLine_Clip_BLT(sGraphicCanvas*,sGraphicBox*,S16);
extern void BaselineLine(sGraphicCanvas*,sGraphicBox*,S16);
extern void BaselineLine_Clip(sGraphicCanvas*,sGraphicBox*,S16);
#define WORDS 150000UL
static U16 actual[WORDS],old[WORDS],expected[WORDS];
static U32 offsets[433];
volatile U16 gLineDone,gLineFailures,gLinePhase,gLineCases;
volatile U32 gLineTimes[10];
static U16 seed=0x2468;
static U16 random16(void) {seed^=seed<<7;seed^=seed>>9;seed^=seed<<8;return seed;}
/* Independent integer rasterizer, writing one pixel into all four planes. */
static void reference(sGraphicCanvas *c,sGraphicBox *b,U16 colour,U16 clip)
{
 S32 x=b->mX0,y=b->mY0,dx=(S32)b->mX1-x,dy=(S32)b->mY1-y;
 S32 sx=dx<0?-1:1,sy=dy<0?-1:1,err,e;U16 p,mask;U16 *dst;
 if(dx<0) dx=-dx;
 if(dy>0) dy=-dy;
 err=dx+dy;
 for(;;) {
  if(!clip || (x>=c->mClipBox.mX0 && x<c->mClipBox.mX1 && y>=c->mClipBox.mY0 && y<c->mClipBox.mY1)) {
   dst=expected+1+(offsets[y]>>1)+(x>>4)*4;mask=0x8000>>(x&15);
   for(p=0;p<4;++p) {dst[p]&=~mask;if(colour&(1<<p))dst[p]|=mask;}
  }
  if(x==b->mX1 && y==b->mY1) break;
  e=err*2;if(e>=dy) {err+=dy;x+=sx;}if(e<=dx) {err+=dx;y+=sy;}
 }
}
static void verify(sGraphicCanvas *c,U16 colour,U16 clip,U16 layout)
{
 U32 i,words=(offsets[c->mHeight]>>1)+2;U16 n;sGraphicBox b;
 for(i=0;i<words;++i) actual[i]=old[i]=expected[i]=(U16)(0xa55a^(i*17));
 for(n=0;n<32;++n) {
  S16 w=c->mWidth,h=c->mHeight;
  b.mX0=random16()%w;b.mX1=random16()%w;b.mY0=random16()%h;b.mY1=random16()%h;
  switch(n) {
   case 0:b.mX0=0;b.mY0=0;b.mX1=w-1;b.mY1=0;break;
   case 1:b.mX0=w-1;b.mY0=h-1;b.mX1=0;b.mY1=h-1;break;
   case 2:b.mX0=0;b.mX1=0;b.mY0=h-1;b.mY1=0;break;
   case 3:b.mX0=w-1;b.mX1=w-1;b.mY0=0;b.mY1=h-1;break;
   case 4:b.mX0=0;b.mY0=0;b.mX1=w-1;b.mY1=h-1;break;
   case 5:b.mX0=w-1;b.mY0=0;b.mX1=0;b.mY1=h-1;break;
   case 6:b.mX0=15;b.mX1=16;b.mY0=20;b.mY1=20;break;
   case 7:b.mX0=16;b.mX1=15;b.mY0=20;b.mY1=20;break;
   case 8:b.mX1=b.mX0;b.mY1=b.mY0;break;
   case 9:b.mX1=b.mX0;break;
   case 10:b.mY1=b.mY0;break;
   case 11:b.mX0=17;b.mY0=17;b.mX1=18;b.mY1=18;break;
   default:if(clip && (n&1)) {b.mX0-=w/2;b.mX1+=w/2;b.mY0-=h/2;b.mY1+=h/2;}
  }
  reference(c,&b,colour,clip);
  c->mpVRAM=old+1;if(clip)BaselineLine_Clip(c,&b,colour);else BaselineLine(c,&b,colour);
  c->mpVRAM=actual+1;
  if(clip) {if(n&1)Graphic_4BP_DrawLine_Clip_BLT(c,&b,colour);else Graphic_4BP_DrawLine_Clip(c,&b,colour);}
  else {if(n&1)Graphic_4BP_DrawLine_BLT(c,&b,colour);else Graphic_4BP_DrawLine(c,&b,colour);}
  ++gLineCases;
 }
 for(i=0;i<words;++i) if(actual[i]!=expected[i] || old[i]!=expected[i]) {++gLineFailures;break;}
 gLinePhase=layout*100+colour*2+clip;
}
static void benchmark(sGraphicCanvas *c)
{
 U16 n,kind,version;U32 start;sGraphicBox b;c->mpVRAM=actual+1;
 for(kind=0;kind<5;++kind) for(version=0;version<2;++version) {
  b.mX0=20;b.mY0=20;b.mX1=299;b.mY1=kind==0?20:kind==1?190:180;
  if(kind==2)b.mX1=20;
  if(kind==4) {b.mX0=-40;b.mY0=-30;b.mX1=350;b.mY1=230;}
  gLinePhase=500+kind*2+version;start=*(volatile U32*)0x4baL;
  for(n=0;n<512;++n) {
   if(version) {if(kind>=3)BaselineLine_Clip(c,&b,n&15);else BaselineLine(c,&b,n&15);}
   else {if(kind>=3)Graphic_4BP_DrawLine_Clip(c,&b,n&15);else Graphic_4BP_DrawLine(c,&b,n&15);}
  }
  gLineTimes[kind*2+version]=*(volatile U32*)0x4baL-start;
 }
}
S16 GodLib_Game_Main(S16 argc,char **argv)
{
 sGraphicCanvas c;U16 layout,row,colour,clip;U16 widths[]={320,640,352,640},strides[]={160,320,192,336};
 (void)argc;(void)argv;GemDos_Super(0);Platform_Init();
 c.mColourMode=eGRAPHIC_COLOURMODE_4PLANE;c.mHeight=432;c.mpLineOffsets=offsets;
 for(layout=0;layout<4;++layout) {
  c.mWidth=widths[layout];for(row=0;row<=432;++row)offsets[row]=(U32)row*strides[layout]+(layout==3?(row&1)*8:0);
  for(clip=0;clip<2;++clip) {
   c.mClipBox.mX0=clip?8:0;c.mClipBox.mY0=clip?5:0;c.mClipBox.mX1=c.mWidth-(clip?9:0);c.mClipBox.mY1=c.mHeight-(clip?7:0);
   for(colour=0;colour<16;++colour)verify(&c,colour,clip,layout);
  }
 }
 c.mWidth=320;c.mHeight=200;c.mClipBox.mX0=c.mClipBox.mY0=0;c.mClipBox.mX1=320;c.mClipBox.mY1=200;
 for(row=0;row<=200;++row)offsets[row]=(U32)row*160;
 benchmark(&c);Platform_DeInit();gLineDone=1;for(;;){}return 0;
}
