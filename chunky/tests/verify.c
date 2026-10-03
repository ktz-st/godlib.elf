#include <godlib/chunky/chunky.h>
#include <string.h>

volatile U16 gChunkyTestResult = 0xffff;
volatile U16 gChunkyTestCases;
static U16 planar[128], expected[128], input[128];
static U8 chunky[512], output[512], bytesExpected[512];
static U32 po[5]={0,40,80,120,160}, co[5]={0,84,168,252,336};
static sGraphicCanvas pc, cc;
static void put(U16 *p,U16 x,U16 y,U8 c)
{
 U16 b, mask=0x8000U>>(x&15);
 p += po[y]/2+(x/16)*4;
 for(b=0;b<4;b++) p[b]=(p[b]&~mask)|((c&(1<<b))?mask:0);
}
static U8 colour(U16 x,U16 y) { return (U8)(x*13+y*7+19); }
static U16 verify(void)
{
 U16 sx,dx,w,y,x,i;
 static const U16 widths[]={1,15,16,17,32};
 memset(&pc,0,sizeof(pc)); memset(&cc,0,sizeof(cc));
 pc.mWidth=64; pc.mHeight=4; pc.mColourMode=eGRAPHIC_COLOURMODE_4PLANE; pc.mpLineOffsets=po;
 cc.mWidth=80; cc.mHeight=4; cc.mColourMode=eGRAPHIC_COLOURMODE_8BPP; cc.mpLineOffsets=co;
 for(i=0;i<512;i++) chunky[i]=(U8)(i*37);
 for(y=0;y<4;y++) for(x=0;x<80;x++) chunky[co[y]+x]=colour(x,y);
 memset(input,0x96,sizeof(input));
 for(y=0;y<4;y++) for(x=0;x<64;x++) put(input,x,y,colour(x,y));
 for(sx=0;sx<3;sx++) for(dx=0;dx<16;dx++) for(w=0;w<5;w++)
 {
  sGraphicRect r; sGraphicPos pos;
  r.mX=sx; r.mY=1; r.mWidth=widths[w]; r.mHeight=3;
  pos.mX=dx; pos.mY=0;
  memset(planar,0xa5,sizeof(planar)); memcpy(expected,planar,sizeof(planar));
  for(y=0;y<3;y++) for(x=0;x<widths[w];x++) put(expected,dx+x,y,colour(sx+x,1+y));
  pc.mpVRAM=planar; cc.mpVRAM=chunky;
  ChunkySurface_To4Plane(&pc,&pos,&r,&cc);
  if(memcmp(planar,expected,sizeof(planar))) return 1;
  memset(output,0xa5,sizeof(output)); memcpy(bytesExpected,output,sizeof(output));
  for(y=0;y<3;y++) for(x=0;x<widths[w];x++) bytesExpected[co[y]+dx+x]=colour(sx+x,1+y)&15;
  pc.mpVRAM=input; cc.mpVRAM=output;
  ChunkySurface_From4Plane(&cc,&pos,&r,&pc);
  if(memcmp(output,bytesExpected,sizeof(output))) return 2;
  ++gChunkyTestCases;
 }
 /* Invalid rectangles must leave the destination unchanged. */
 {
  sGraphicRect r={0,0,0,1}; sGraphicPos pos={0,0};
  memcpy(expected,planar,sizeof(planar)); pc.mpVRAM=planar; cc.mpVRAM=chunky;
  ChunkySurface_To4Plane(&pc,&pos,&r,&cc);
  r.mWidth=65; ChunkySurface_To4Plane(&pc,&pos,&r,&cc);
  r.mWidth=1; r.mX=-1; ChunkySurface_To4Plane(&pc,&pos,&r,&cc);
  if(memcmp(planar,expected,sizeof(planar))) return 3;
 }
 return 0;
}
int main(void)
{
 gChunkyTestResult=verify();
 for(;;) { /* Keep result available to the Hatari debugger. */ }
 return 0;
}
