#include <godlib/graphic/graphic.h>
#include <godlib/sprite/sprite.h>
#include <godlib/platform/platform.h>
#include <godlib/gemdos/gemdos.h>
#include <godlib/blitter/blitter.h>
#include <string.h>
extern void Graphic_4BP_DrawSprite_Clip_BLT(sGraphicCanvas*,sGraphicPos*,void*);
extern void Graphic_4BP_DrawSprite_BLT(sGraphicCanvas*,sGraphicPos*,void*);
extern void Old_DrawSprite_Clip_BLT(sGraphicCanvas*,sGraphicPos*,void*);
#define W 320
#define H 64
static U16 buffer[5122],reference[5122],gfx[1280];
volatile U16 gAuditDone,gAuditPhase,gAuditFailures[2],gAuditFirstWidth[2],gAuditFirstPhase[2];
volatile U32 gAuditTimes[32];
static U16 col(U16 x,U16 y) { return (x+3*y)&15; }
static void put(U16 *data,U16 x,U16 y,U16 c)
{
 U16 p,bit=0x8000>>(x&15);data+=(U32)y*80+(x>>4)*4;
 for(p=0;p<4;++p) {data[p]&=~bit;if(c&(1<<p)) data[p]|=bit;}
}
static void verify(sGraphicCanvas *canvas,sSprite *sprite,U16 width,U16 mode)
{
 U16 phase,x,y;sGraphicPos pos;
 for(phase=0;phase<100;++phase) {
  pos.mX=phase<32?64+phase:phase<64?-(S16)(phase-31):phase<96?W-width+1+(phase-64):W-1;
  pos.mY=phase<96?8:phase==96?-5:phase==97?60:phase==98?-3:63;
  if(phase==98) pos.mX=-7;
  gAuditPhase=mode*1000+width+phase;
  memset(buffer,0xa5,sizeof(buffer));memcpy(reference,buffer,sizeof(buffer));
  for(y=0;y<16;++y) for(x=0;x<width;++x) {
   S16 dx=pos.mX+x,dy=pos.mY+y;U16 c=col(x,y);
   if(c && dx>=0 && dx<W && dy>=0 && dy<H) put(reference+1,dx,dy,c);
  }
  if(mode) Old_DrawSprite_Clip_BLT(canvas,&pos,sprite);
  else Graphic_4BP_DrawSprite_Clip_BLT(canvas,&pos,sprite);
  Blitter_Wait();
  if(memcmp(buffer,reference,sizeof(buffer))) {
   if(!gAuditFailures[mode]) {gAuditFirstWidth[mode]=width;gAuditFirstPhase[mode]=phase;}
   ++gAuditFailures[mode];
  }
 }
}
static void benchmark(sGraphicCanvas *canvas,sSprite *sprite,U16 width,U16 wi)
{
 U16 mode,phase,i;sGraphicPos pos;
 for(phase=0;phase<2;++phase) for(mode=0;mode<4;++mode) {
  U32 before;pos.mX=64+(phase?5:0);pos.mY=8;
  if(mode==3) pos.mX=-7;
  gAuditPhase=4000+wi*8+phase*4+mode;
  before=*(volatile U32*)0x4baL;
  for(i=0;i<3000;++i) {
   if(mode==0) Graphic_4BP_DrawSprite_Clip_BLT(canvas,&pos,sprite);
   if(mode==1) Old_DrawSprite_Clip_BLT(canvas,&pos,sprite);
   if(mode==2) Graphic_4BP_DrawSprite_BLT(canvas,&pos,sprite);
   if(mode==3) Graphic_4BP_DrawSprite_Clip_BLT(canvas,&pos,sprite);
  }
  Blitter_Wait();gAuditTimes[wi*8+phase*4+mode]=*(volatile U32*)0x4baL-before;
 }
 (void)width;
}
S16 GodLib_Game_Main(S16 argc,char **argv)
{
 U16 wi,width,x,y;U16 *mask;sSprite *sprite;sGraphicCanvas canvas;
 (void)argc;(void)argv;GemDos_Super(0);Platform_Init();Graphic_SetBlitterEnable(1);
 GraphicCanvas_Init(&canvas,eGRAPHIC_COLOURMODE_4PLANE,W,H);canvas.mpVRAM=buffer+1;
 for(wi=0;wi<4;++wi) {
  width=16*(wi+1);memset(gfx,0,sizeof(gfx));
  for(y=0;y<16;++y) for(x=0;x<width;++x) put(gfx,x,y,col(x,y));
  mask=Sprite_MaskCreate(gfx,width,16,4,4,0);
  sprite=Sprite_Create(gfx,mask,width,16,4,4,0);Sprite_MaskDestroy(mask);
  verify(&canvas,sprite,width,0);verify(&canvas,sprite,width,1);
  benchmark(&canvas,sprite,width,wi);Sprite_Destroy(sprite);
 }
 GraphicCanvas_DeInit(&canvas);gAuditDone=1;
 for(;;) {} return 0;
}
