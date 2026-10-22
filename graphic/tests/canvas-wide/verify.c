#include <godlib/graphic/graphic.h>
#include <godlib/sprite/sprite.h>
#include <godlib/platform/platform.h>
#include <godlib/gemdos/gemdos.h>
#include <godlib/blitter/blitter.h>
#include <string.h>
/* Wide (720 px) and 320 px 4-plane canvases: Blit and DrawSprite(_Clip) on the CPU
   and blitter paths against an independent per-pixel reference, including guards. */
#define SW 720
#define SH 16
#define GUARD 8
static U16 srcMem[SW/4*SH+2*GUARD],dstMem[SW/4*32+2*GUARD],refMem[SW/4*32+2*GUARD],gfx[80*16];
/* categories: 0/1 Blit CPU/BLT, 2/3 DrawSprite_Clip CPU/BLT, 4/5 DrawSprite CPU/BLT,
   6/7 DrawSprite_Clip CPU/BLT with a clip box edge that is not 16-aligned */
volatile U16 gWideDone,gWidePhase,gWideCases[8],gWideFailures[8],gWideFirst[8][5];
/* BLiT_iT skew_flags index of each blitter Blit case: cases and failures per entry */
volatile U16 gSkewCases[8],gSkewFailures[8];
/* first failing cases per category: phase,a,b,c,d */
volatile U16 gWideLog[8][700][5],gWideLogCount[8];
static U16 col(U16 x,U16 y) { return (U16)((x*7+y*3+(x>>4))&15); }
static void put(U16 *data,U16 lineWords,U16 x,U16 y,U16 c)
{
 U16 p,bit=(U16)(0x8000>>(x&15));data+=(U32)y*lineWords+(x>>4)*4;
 for(p=0;p<4;++p) {data[p]&=(U16)~bit;if(c&(1<<p)) data[p]|=bit;}
}
static U16 get(const U16 *data,U16 lineWords,U16 x,U16 y)
{
 U16 p,c=0,bit=(U16)(0x8000>>(x&15));data+=(U32)y*lineWords+(x>>4)*4;
 for(p=0;p<4;++p) if(data[p]&bit) c|=(U16)(1<<p);
 return c;
}
static void fail(U16 cat,U16 a,U16 b,U16 c,U16 d)
{
 if(!gWideFailures[cat]) {gWideFirst[cat][0]=gWidePhase;gWideFirst[cat][1]=a;gWideFirst[cat][2]=b;gWideFirst[cat][3]=c;gWideFirst[cat][4]=d;}
 if(gWideLogCount[cat]<700) {volatile U16 *l=gWideLog[cat][gWideLogCount[cat]++];l[0]=gWidePhase;l[1]=a;l[2]=b;l[3]=c;l[4]=d;}
 ++gWideFailures[cat];
}
/* skew_flags index exactly as BLiT_iT derives it */
static U16 skewIndex(U16 sx,U16 dx,U16 w)
{
 U16 i=0,sspan=(U16)(((sx+w-1)>>4)-(sx>>4)),dspan=(U16)(((dx+w-1)>>4)-(dx>>4));
 if((sx&15)>(dx&15)) i|=1;
 if(sspan==dspan) i|=2;
 if(!dspan) i|=4;
 return i;
}
static void blitCase(U16 blt,sGraphicCanvas *src,sGraphicCanvas *dst,U16 sx,U16 sy,U16 dx,U16 dy,U16 w,U16 h)
{
 U16 x,y,dw=(U16)(dst->mWidth/4),bad;sGraphicPos pos;sGraphicRect rect;
 memset(dstMem,0x5a,sizeof(dstMem));memcpy(refMem,dstMem,sizeof(refMem));
 for(y=0;y<h;++y) for(x=0;x<w;++x) put(refMem+GUARD,dw,(U16)(dx+x),(U16)(dy+y),get(srcMem+GUARD,SW/4,(U16)(sx+x),(U16)(sy+y)));
 pos.mX=(S16)dx;pos.mY=(S16)dy;rect.mX=(S16)sx;rect.mY=(S16)sy;rect.mWidth=(S16)w;rect.mHeight=(S16)h;
 GraphicCanvas_Blit(dst,&pos,&rect,src);
 if(blt) Blitter_Wait();
 bad=(U16)(memcmp(dstMem,refMem,sizeof(dstMem))!=0);
 ++gWideCases[blt];if(bad) fail(blt,sx,dx,w,h);
 if(blt) {U16 i=skewIndex(sx,dx,w);++gSkewCases[i];if(bad)++gSkewFailures[i];}
}
static void blits(U16 blt,sGraphicCanvas *src,sGraphicCanvas *wide,sGraphicCanvas *narrow)
{
 static const U16 sxs[]={0,1,7,8,15,16,100,180,200,344,350,500,511,512,513,600,640,655};
 static const U16 dxs[]={0,3,8,15,16,40,180,190,200,300};
 static const U16 ws[]={1,5,8,12,16,17,24,31,32,33,48,64,80,96};
 U16 a,b,c;
 for(a=0;a<sizeof(sxs)/2;++a) for(b=0;b<sizeof(dxs)/2;++b) for(c=0;c<sizeof(ws)/2;++c) {
  U16 sx=sxs[a],dx=dxs[b],w=ws[c];
  if(sx+w>SW) continue;
  gWidePhase=(U16)(1000*(blt+1)+a*10+b);
  blitCase(blt,src,wide,sx,0,dx,0,w,SH);
  if(dx+w<=320) blitCase(blt,src,narrow,sx,(U16)(c&3),dx,(U16)(7+(a&7)),w,(U16)(SH-(c&3)));
 }
}
static void spriteCase(U16 cat,sGraphicCanvas *cv,sSprite *spr,S16 px,S16 py,U16 clip)
{
 U16 x,y,dw=(U16)(cv->mWidth/4);S16 dx,dy;sGraphicPos pos;sGraphicBox *b=&cv->mClipBox;
 memset(dstMem,0xa5,sizeof(dstMem));memcpy(refMem,dstMem,sizeof(refMem));
 for(y=0;y<(U16)spr->mHeight;++y) for(x=0;x<(U16)spr->mWidth;++x) {
  U16 c=col(x,y);dx=(S16)(px+x);dy=(S16)(py+y);
  if(c && dx>=b->mX0 && dx<b->mX1 && dy>=b->mY0 && dy<b->mY1) put(refMem+GUARD,dw,(U16)dx,(U16)dy,c);
 }
 pos.mX=px;pos.mY=py;
 if(clip) GraphicCanvas_DrawSprite_Clip(cv,&pos,spr); else GraphicCanvas_DrawSprite(cv,&pos,spr);
 Blitter_Wait();
 ++gWideCases[cat];
 if(memcmp(dstMem,refMem,sizeof(dstMem))) fail(cat,(U16)px,(U16)py,(U16)spr->mWidth,(U16)(b->mX0|(b->mX1<<10)));
}
static void sprites(U16 blt,sGraphicCanvas *wide,sGraphicCanvas *narrow,sSprite *spr)
{
 S16 x;U16 w=(U16)spr->mWidth;
 /* wide canvas, full clip box: left clip, all fine shifts, x>512, right clip */
 for(x=(S16)(-(S16)w+1);x<SW;x+=(x<24||(x>160&&x<220)||(x>500&&x<530)||x>SW-(S16)w-3)?1:37) {
  gWidePhase=(U16)(10000+blt*1000+w);
  spriteCase((U16)(2+blt),wide,spr,x,0,1);
  if(x>=0 && x+(S16)w<=SW) spriteCase((U16)(4+blt),wide,spr,x,0,0);
 }
 /* 320 canvas: clip boxes with aligned/unaligned edges, one or both edges clipped,
    boxes narrower than the sprite, top/bottom clip; every fine x phase is crossed */
 {
  static const S16 boxes[][2]={{3,7},{94,98},{16,48},{20,60},{32,48},{5,300},{0,37},{17,320},{100,101},{0,320},{48,64},{33,47}};
  U16 bi;
  for(bi=0;bi<sizeof(boxes)/sizeof(boxes[0]);++bi) {
   S16 x0=boxes[bi][0],x1=boxes[bi][1];
   U16 cat=(U16)(((x0&15)||(x1&15))?6+blt:2+blt);
   narrow->mClipBox.mX0=x0;narrow->mClipBox.mX1=x1;narrow->mClipBox.mY0=3;narrow->mClipBox.mY1=29;
   for(x=(S16)(x0-(S16)w-1);x<=x1+1;x+=(x>x0+18&&x<x1-(S16)w-18)?23:1) {
    gWidePhase=(U16)(20000+blt*1000+w+bi);
    spriteCase(cat,narrow,spr,x,(S16)((x&3)==0?-5:(x&3)==1?20:(S16)(x&15)),1);
   }
  }
  narrow->mClipBox.mX0=0;narrow->mClipBox.mX1=320;narrow->mClipBox.mY0=0;narrow->mClipBox.mY1=32;
 }
}
S16 GodLib_Game_Main(S16 argc,char **argv)
{
 U16 i,x,y,blt,width;U16 *mask;sSprite *spr;sGraphicCanvas src,wide,narrow;
 (void)argc;(void)argv;GemDos_Super(0);Platform_Init();
 GraphicCanvas_Init(&src,eGRAPHIC_COLOURMODE_4PLANE,SW,SH);GraphicCanvas_SetpVRAM(&src,srcMem+GUARD);
 GraphicCanvas_Init(&wide,eGRAPHIC_COLOURMODE_4PLANE,SW,SH);GraphicCanvas_SetpVRAM(&wide,dstMem+GUARD);
 GraphicCanvas_Init(&narrow,eGRAPHIC_COLOURMODE_4PLANE,320,32);GraphicCanvas_SetpVRAM(&narrow,dstMem+GUARD);
 for(i=0;i<sizeof(srcMem)/2;++i) srcMem[i]=(U16)(i*0x9e37+0x1234);
 for(blt=0;blt<2;++blt) {
  Graphic_SetBlitterEnable((U8)blt);
  if(Graphic_GetBlitterEnable()!=blt) {fail(blt,0xffff,0,0,0);continue;}
  blits(blt,&src,&wide,&narrow);
  for(width=16;width<=64;width+=16) {
   memset(gfx,0,sizeof(gfx));
   for(y=0;y<16;++y) for(x=0;x<width;++x) put(gfx,80,x,y,col(x,y)) /* Sprite_Create reads a 320 px screen layout */;
   mask=Sprite_MaskCreate(gfx,width,16,4,4,0);spr=Sprite_Create(gfx,mask,width,16,4,4,0);Sprite_MaskDestroy(mask);
   sprites(blt,&wide,&narrow,spr);Sprite_Destroy(spr);
  }
 }
 Graphic_SetBlitterEnable(0);gWideDone=1;
 for(;;) {} return 0;
}
