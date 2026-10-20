/* Native four-voice TFMX from UGH!. No original assets are changed. */
#include "tfmx.h"
#include "paulamixer.h"
#include "leonardmixer.h"
#include <godlib/file/file.h>
#include <godlib/memory/memory.h>
#include <godlib/system/system.h>
#include <godlib/vbl/vbl.h>
#include <string.h>
#define WINDOW 672UL
struct sTfmxSong {sTfmxInfo info;U32 bankBytes;U8 *mdat,*bank,*pcm,*windows;};
static sTfmxSong *spSong;
volatile U8 gTfmxPlaying,gTfmxBackend,gTfmxRasterEnabled;
volatile U32 gTfmxTicks,gTfmxFaults;
extern U8 channela[64],pSampA[40];
extern U16 emudmacon;
extern void Tfmx_InitCore(void *mdat,void *smpl);
extern void Tfmx_StartCore(U32 song);
extern void Tfmx_StopCore(void);
extern void Tfmx_LeonardSequence(void);
static U16 word(const U8 *p){return ((U16)p[0]<<8)|p[1];}
static U32 lng(const U8 *p){return ((U32)word(p)<<16)|word(p+2);}
static void put(U8 *p,U32 n){p[0]=(U8)(n>>24);p[1]=(U8)(n>>16);p[2]=(U8)(n>>8);p[3]=(U8)n;}
static U8 valid(U32 offset,U32 bytes,U32 total){return offset<=total && bytes<=total-offset;}
sTfmxSong *Tfmx_Create(const void *mdat,U32 a,const void *smpl,U32 b,U16 mixer)
{
 const U8 *src=(const U8*)mdat;U32 i,total,rounded=(a+1)&~1UL;U16 songs=0; sTfmxSong *s;
 if(!src || !smpl || mixer>1 || a<512 || a>0x100000UL || b<4 || b>0x100000UL || (b&1) || memcmp(src,"TFMX-SONG",9))return 0;
 for(i=464;i<=472;i+=4)if((lng(src+i)&1) || !valid(lng(src+i),4,a))return 0;
 for(i=0;i<32;++i){U16 first=word(src+256+2*i),last=word(src+320+2*i);if(first==0x1ff || first==0xffff)break;if(first>last || !valid(lng(src+464)+16UL*first,16UL*(last-first+1),a))break;if(!i || first || last)songs=(U16)(i+1);}
 if(!songs)return 0;
 total=sizeof(*s)+rounded+b+(mixer?0:4*WINDOW);
 s=mMEMCALLOC(total);if(!s)return 0;
 s->info.mSourceBytes=a+b;s->info.mResidentBytes=total;s->info.mSubSongs=songs;s->info.mMixer=mixer;s->bankBytes=b;
 s->mdat=(U8*)(s+1);s->bank=s->mdat+rounded;memcpy(s->mdat,mdat,a);memcpy(s->bank,smpl,b);memset(s->bank,0,4);
 if(!mixer){s->pcm=s->bank;s->windows=s->bank+b;for(i=0;i<b;++i)s->bank[i]=(U8)((S8)s->bank[i]>>1);}
 return s;
}
sTfmxSong *Tfmx_Load(const char *mdat,const char *smpl,U16 mixer)
{
 S32 a,b;void *x,*y;sTfmxSong *s=0;if(!mdat || !smpl)return 0;
 a=File_GetSize(mdat);b=File_GetSize(smpl);if(a<=0 || b<=0)return 0;
 x=File_Load(mdat);y=File_Load(smpl);if(x && y)s=Tfmx_Create(x,(U32)a,y,(U32)b,mixer);
 if(x){File_UnLoad(x);}if(y){File_UnLoad(y);}return s;
}
U8 Tfmx_GetInfo(const sTfmxSong *s,sTfmxInfo *i){if(!s || !i)return 0;*i=s->info;return 1;}
U32 Tfmx_GetAllocatedSize(void){return gTfmxBackend?(LeonardMixer_GetOwner()==4?LeonardMixer_GetAllocatedSize():0):(PaulaMixer_GetOwner()==5?PaulaMixer_GetAllocatedSize():0);}
U8 Tfmx_SetSubSong(U16 sub)
{
 U16 sr;if(!spSong || sub>=spSong->info.mSubSongs)return 0;
 sr=System_GetIML();System_SetIML(7);Tfmx_StopCore();memset(channela,0,64);memset(pSampA,0,40);memset(PaulaMixer_Channels,0,80);emudmacon=0;
 Tfmx_InitCore(spSong->mdat,spSong->bank);Tfmx_StartCore(sub);System_SetIML(sr);return 1;
}
U8 Tfmx_Start(sTfmxSong *s,U16 sub,U8 rate)
{
 U8 ok;U16 sr;if(!s || sub>=s->info.mSubSongs || rate<1 || rate>2)return 0;
 Tfmx_Stop();if(s->info.mMixer){if(!LeonardMixer_Init(4,rate))return 0;}else if(!PaulaMixer_Init(5,rate))return 0;
 spSong=s;gTfmxBackend=(U8)s->info.mMixer;Tfmx_SetSubSong(sub);
 if(gTfmxBackend)LeonardMixer_SetUserRoutine(Tfmx_LeonardSequence);
 sr=System_GetIML();System_SetIML(7);gTfmxPlaying=1;ok=Vbl_AddCall(Tfmx_Tick);System_SetIML(sr);if(!ok)Tfmx_Stop();return ok;
}
void Tfmx_Stop(void)
{
 U16 sr=System_GetIML();System_SetIML(7);Vbl_RemoveCall(Tfmx_Tick);gTfmxPlaying=0;
 if(spSong){if(gTfmxBackend)LeonardMixer_SetUserRoutine(0);Tfmx_StopCore();spSong=0;}
 System_SetIML(sr);if(gTfmxBackend)LeonardMixer_Shutdown(4);else PaulaMixer_Shutdown(5);gTfmxBackend=0;
}
void Tfmx_UnLoad(sTfmxSong *s){if(!s)return;if(s==spSong)Tfmx_Stop();mMEMFREE(s);}
void Tfmx_SetRasterDebug(U8 on){gTfmxRasterEnabled=on;}
void Tfmx_GetVolumes(U16 *v)
{
 U16 i,sr=System_GetIML();if(!v)return;System_SetIML(7);for(i=0;i<4;++i)v[i]=gTfmxPlaying && (emudmacon&(1<<i))?word(channela+16*i+8):0;for(i=0;i<4;++i)if(v[i]>64)v[i]=64;System_SetIML(sr);
}
/* Lance renders a contiguous source window before testing loop boundaries.
 * Use the bank directly when a full window remains; otherwise stitch
 * the next 672 source bytes across current/reload boundaries, using
 * preconverted 7-bit PCM. Never copy/allocate whole samples inside VBL. */
void Tfmx_LancePrepare(void)
{
 U16 i,volume;U32 pos,end,loop,len,n,done,pattern,period;U8 *out,*v,*state,*regs;
 for(i=0;i<4;++i){v=PaulaMixer_Channels+20*i;state=pSampA+10*i;regs=channela+16*i;out=spSong->windows+WINDOW*i;
  if(!(emudmacon&(1<<i))){memset(v,0,20);continue;}
  pos=lng(state)-(U32)spSong->bank;end=lng(state+4)-(U32)spSong->bank;loop=lng(regs)-(U32)spSong->bank;len=2UL*word(regs+4);
  if(!len || !valid(loop,len,spSong->bankBytes) || pos>end || end>spSong->bankBytes){++gTfmxFaults;emudmacon&=~(1<<i);memset(v,0,20);continue;}
  /* Copy the remaining attack, then seed one reload period and double it.
     This keeps two-byte silent/short loops to O(log WINDOW) block copies. */
  if(end-pos>=WINDOW){out=spSong->pcm+pos;}else{
  n=end-pos;memcpy(out,spSong->pcm+pos,n);done=n;
  if(done<WINDOW){pattern=done;n=len;if(n>WINDOW-done)n=WINDOW-done;memcpy(out+done,spSong->pcm+loop,n);done+=n;
   while(done<WINDOW){n=done-pattern;if(n>WINDOW-done)n=WINDOW-done;memcpy(out+done,out+pattern,n);done+=n;}
  }
  }
  /* TFMX can emit 65 during envelopes. Like Leonard, saturate before
     native Lance's 0..64 ratio and LMC tables (65 would mute the partner). */
  volume=word(regs+8);if(volume>64)volume=64;
  put(v,(U32)out);put(v+4,(U32)(out+WINDOW));put(v+8,0);v[12]=0;v[13]=(U8)volume;
  period=word(regs+6);if(period>907)period=907;if(period<108)period=108;v[14]=(U8)(period>>8);v[15]=(U8)period;
  v[16]=state[8];v[17]=state[9];
 }
}
void Tfmx_LanceFinish(void)
{
 U16 i;U32 used,pos,end,loop,len;U8 *v,*state,*regs;
 for(i=0;i<4;++i){if(!(emudmacon&(1<<i)))continue;v=PaulaMixer_Channels+20*i;state=pSampA+10*i;regs=channela+16*i;
  /* End always points WINDOW bytes beyond the initial source pointer,
     including the zero-copy path through a long sample. */
  used=lng(v)-(lng(v+4)-WINDOW);if(used>=WINDOW){++gTfmxFaults;emudmacon&=~(1<<i);continue;}
  pos=lng(state);end=lng(state+4);loop=lng(regs);len=2UL*word(regs+4);
  if(used>=end-pos){used-=end-pos;pos=loop+used%len;end=loop+len;}else pos+=used;
  put(state,pos);put(state+4,end);state[8]=v[16];state[9]=v[17];
 }
}
