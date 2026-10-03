#include <godlib/font8x8/font8x8.h>
#include <godlib/graphic/graphic.h>
#include <godlib/screen/screen.h>
#include <godlib/platform/platform.h>
#include <godlib/gemdos/gemdos.h>
extern U8 gFont8x8[12544];
static U16 actual[75002], expected[75002];
static U32 lines[433];
volatile U16 gFontVerifyDone,gFontVerifyFailures,gFontVerifyCases;
static void check(sGraphicCanvas *canvas,U16 x,U16 y,U16 colour,U16 mode)
{
 const char *text="Ab!?"; U16 ch,row,p,bit; U32 words, i; U8 *ref=(U8*)(expected+1);
 U16 *dst=(U16*)canvas->mpVRAM;
 words=(canvas->mpLineOffsets[canvas->mHeight]+1)/2;
 for(i=0;i<words+2;++i) {expected[i]=0xa5a5;actual[i]=0xa5a5;}
 /* The reference places each glyph pixel into planar memory individually. */
 for(ch=0;ch<4;++ch) for(row=0;row<8;++row) for(bit=0;bit<8;++bit) {
  U16 px=x+ch*8+bit; U8 mask=0x80>>(px&7);
  U8 glyph=gFont8x8[((U16)(text[ch]-32)<<3)+row];
  for(p=0;p<(mode&1?4:1);++p) {
   U32 addr=canvas->mpLineOffsets[y+row]+(U32)(px>>4)*8+((px&8)?1:0)+p*2;
   ref[addr]&=~mask;
   if((glyph&(0x80>>bit)) && (!(mode&1) || (colour&(1<<p)))) ref[addr]|=mask;
  }
 }
 if(mode==0) Font8x8_PrintCanvas(text,canvas,x,y);
 if(mode==1) Font8x8_PrintColourCanvas(text,canvas,x,y,colour);
 if(mode==2) Font8x8_Print(text,dst,x,y);
 if(mode==3) Font8x8_PrintColour(text,dst,x,y,colour);
 for(i=0;i<words+2;++i) if(dst[-1+(S32)i]!=expected[i]) {++gFontVerifyFailures;break;}
 ++gFontVerifyCases;
}
S16 GodLib_Game_Main(S16 argc,char **argv)
{
 sGraphicCanvas canvas; U16 layout,row,x,y,c,page; U16 widths[4]={320,352,640,640}; U16 strides[4]={160,176,320,336};
 (void)argc;(void)argv;GemDos_Super(0);Platform_Init();
 canvas.mpVRAM=actual+1;canvas.mColourMode=eGRAPHIC_COLOURMODE_4PLANE;canvas.mHeight=432;canvas.mpLineOffsets=lines;
 for(layout=0;layout<4;++layout) {
  canvas.mWidth=widths[layout];for(row=0;row<=432;++row) lines[row]=(U32)row*strides[layout];
  for(x=0;x<2;++x) for(y=0;y<2;++y) {
   check(&canvas, x?widths[layout]-40:0,y?400:7,15,0);
   check(&canvas, x?widths[layout]-32:8,y?424:199,6,1);
  }
 }
 /* Independently test every palette colour and the legacy 160-byte raw path. */
 canvas.mWidth=320;for(row=0;row<=432;++row) lines[row]=(U32)row*160;
 for(c=0;c<16;++c) check(&canvas,8,8,c,1);
 check(&canvas,16,200,1,2);check(&canvas,8,400,9,3);
 /* Nonuniform custom line offsets must be honoured exactly. */
 canvas.mWidth=640;for(row=0;row<=432;++row) lines[row]=(U32)row*336+(row&1)*8;
 check(&canvas,600,400,13,1);check(&canvas,608,424,1,0);
 /* Existing APIs must recognize every live Screen page. */
 Screen_Init(640,400,eGRAPHIC_COLOURMODE_4PLANE,eSCREEN_SCROLL_H|eSCREEN_SCROLL_V);
 if(!gScreenClass.mpMemBase) ++gFontVerifyFailures;
 else {
  sGraphicCanvas *pages[3]={&gScreenLogicGraphic,&gScreenPhysicGraphic,&gScreenBackGraphic};
  for(page=0;page<3;++page) {
   U16 *base=(U16*)pages[page]->mpVRAM;
   /* Use a guarded buffer while retaining the active Screen canvas identity. */
   pages[page]->mpVRAM=actual+1;
   check(pages[page],600,400,1,2);check(pages[page],608,424,14,3);
   pages[page]->mpVRAM=base;
  }
  Screen_DeInit();
 }
 Platform_DeInit();gFontVerifyDone=1;for(;;) {}return 0;
}
