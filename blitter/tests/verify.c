#include <godlib/platform/platform.h>
#include <godlib/gemdos/gemdos.h>
#include <godlib/blitter/blitter.h>
#include <godlib/screen/screen.h>
#include <godlib/memory/memory.h>
#include <string.h>
volatile U16 gBlitterVerifyResult=0xffff,gBlitterVerifyPhase;
#define WORDS 69122UL
static U16 dst[WORDS],ref[WORDS],src[4098],gfx[64],mask[16];
static sGraphicCanvas dc,sc;
static sBlitterSprite sprite={gfx,mask,32,8,4,4};
static U16 colour(U16 x,U16 y) { return (x+y)%5?15:0; }
static void put(U16 *data,U16 stride,U16 x,U16 y,U16 c)
{
 U16 p,bit=0x8000>>(x&15);U32 o=(U32)y*(stride/2)+(x>>4)*4;
 for(p=0;p<4;++p) { data[o+p]&=~bit;if(c&(1<<p)) data[o+p]|=bit; }
}
static U16 get(U16 *data,U16 stride,U16 x,U16 y)
{
 U16 p,c=0;U32 o=(U32)y*(stride/2)+(x>>4)*4;
 for(p=0;p<4;++p) if(data[o+p]&(0x8000>>(x&15))) c|=1<<p;
 return c;
}
static void reset(U32 words)
{
 U32 i;
 for(i=0;i<words;++i) dst[i]=ref[i]=(U16)(0xa537+i*73);
}
static U16 same(U16 *data,U32 words)
{
 U32 i;for(i=0;i<words;++i) if(data[i]!=ref[i]) return 0;return 1;
}
static void referenceSprite(U16 stride,U16 height,S16 px,S16 py,U16 mode,U16 *data)
{
 U16 x,y;
 for(y=0;y<8;++y) for(x=0;x<32;++x) {
  S16 dx=px+x,dy=py+y;U16 c=colour(x,y);
  if(dy>=0 && dy<height && (mode==1 || c)) put(data,stride,dx,dy,mode==2?(c?6:0):c);
 }
}
static U16 verify(void)
{
 U16 x,y,p,mode,phase;U32 words;
 for(y=0;y<8;++y) for(x=0;x<32;++x) {
  U16 bit=0x8000>>(x&15),i=y*2+(x>>4);
  if(!colour(x,y)) mask[i]|=bit;
  for(p=0;p<4;++p) if(colour(x,y)) gfx[i*4+p]|=bit;
 }
 GraphicCanvas_Init(&dc,eGRAPHIC_COLOURMODE_4PLANE,640,48);dc.mpVRAM=dst+1;
 /* Deliberately padded rows: stride need not equal width/2. */
 for(y=0;y<=48;++y) dc.mpLineOffsets[y]=(U32)y*336;
 GraphicCanvas_Init(&sc,eGRAPHIC_COLOURMODE_4PLANE,352,32);sc.mpVRAM=src+1;
 for(y=0;y<=32;++y) sc.mpLineOffsets[y]=(U32)y*192;
 words=336UL*48/2+2;
 for(mode=0;mode<3;++mode) for(phase=0;phase<35;++phase) {
  S16 px=phase<32?330+phase:608,py=phase<32?10:(phase==32?-3:phase==33?44:20);
  gBlitterVerifyPhase=mode*35+phase;reset(words);
  referenceSprite(336,48,px,py,mode,ref+1);
  if(mode==0) Blitter_DrawSpriteCanvas(&sprite,&dc,px,py);
  if(mode==1) Blitter_DrawOpaqueSpriteCanvas(&sprite,&dc,px,py);
  if(mode==2) Blitter_DrawColouredSpriteCanvas(&sprite,&dc,px,py,6);
  Blitter_Wait();if(!same(dst,words)) return 100+gBlitterVerifyPhase;
 }
 for(y=0;y<32;++y) for(x=0;x<352;++x) put(src+1,192,x,y,(x+3*y)&15);
 for(phase=0;phase<48;++phase) {
  U16 sx=phase&15,dx=320+((phase*7)&15),w=phase<16?1:phase<32?17:48;
  gBlitterVerifyPhase=200+phase;reset(words);
  for(y=0;y<13;++y) for(x=0;x<w;++x) put(ref+1,336,dx+x,21+y,get(src+1,192,sx+x,4+y));
  Blitter_CopyBoxCanvas(&sc,&dc,sx,4,dx,21,w,13);
  if(!same(dst,words)) return 300+phase;
 }
 for(phase=0;phase<16;++phase) {
  sBlitterBox box={49,13,9};U16 px=570+phase;
  reset(words);
  for(y=0;y<13;++y) for(x=0;x<49 && px+x<640;++x) put(ref+1,336,px+x,35+y,9);
  Blitter_DrawBoxCanvas(&box,&dc,px,35);Blitter_Wait();
  if(!same(dst,words)) return 400+phase;
 }
 GraphicCanvas_DeInit(&sc);GraphicCanvas_DeInit(&dc);
 for(mode=0;mode<2;++mode) {
  U16 height=mode?432:200;U16 flags=eSCREEN_SCROLL_H|(mode?eSCREEN_SCROLL_V:0);
  Screen_Init(640,mode?400:200,eGRAPHIC_COLOURMODE_4PLANE,flags);
  if(!gScreenClass.mpMemBase) return 500;
  words=320UL*height/2;
  for(phase=0;phase<3;++phase) {
   U16 *screen=Screen_GetpLogic();S16 py=mode?400:150;
   reset(words);Memory_Copy(words*2,dst,screen);
   referenceSprite(320,height,333,py,phase,ref);
   if(phase==0) Blitter_DrawSprite(&sprite,screen,333,py);
   if(phase==1) Blitter_DrawOpaqueSprite(&sprite,screen,333,py);
   if(phase==2) Blitter_DrawColouredSprite(&sprite,screen,333,py,6);
   Blitter_Wait();if(!same(screen,words)) return 510+mode*3+phase;
  }
  {
   sBlitterBox box={37,9,5};U16 py=mode?410:180;U16 *screen=Screen_GetpLogic();
   reset(words);Memory_Copy(words*2,dst,screen);
   for(y=0;y<9;++y) for(x=0;x<37;++x) put(ref,320,400+x,py+y,5);
   Blitter_DrawBox(&box,screen,400,py);Blitter_Wait();if(!same(screen,words)) return 520+mode;
   Memory_Copy(words*2,dst,Screen_GetpBack());
   for(y=0;y<7;++y) for(x=0;x<47;++x) put(ref,320,350+x,py+y,get(dst,320,19+x,5+y));
   Blitter_CopyBox(Screen_GetpBack(),screen,19,5,350,py,47,7);
   if(!same(screen,words)) return 530+mode;
  }
  Screen_DeInit();
 }
 return 0;
}
S16 GodLib_Game_Main(S16 argc,char **argv)
{
 (void)argc;(void)argv;GemDos_Super(0);Platform_Init();gBlitterVerifyResult=verify();
 for(;;) {} return 0;
}
