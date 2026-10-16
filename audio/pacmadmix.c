#include "pacmadmix.h"
#include <godlib/system/system.h>
#include <godlib/vbl/vbl.h>
#define HW8(a) (*(volatile U8 *)(a))
#define RING 1024U
#define LEAD 640U
#define SILENCE 250U
/* Layout shared with pacmadmix_s.s. Descriptors stay valid across VBLs. */
typedef struct {const S8 *data;U32 length;U8 priority,pad;} Sample;
typedef struct {
 const Sample *sample;const S8 *current,*end;const Sample *next;
 U8 loop;volatile U8 active;U8 priority,pad;
} Channel;
typedef struct {Channel *channels;S8 *dst,*silence;U16 count,saturate;} Params;
typedef char ChannelSizeCheck[sizeof(Channel)==20?1:-1];
typedef char SampleSizeCheck[sizeof(Sample)==10?1:-1];
static U16 sRing[RING/2],sSilence[SILENCE/2];
static Channel sChannels[3];
static Sample sSamples[3][2];
static volatile U8 sEnabled,sRaster,sSaturate;
static U16 sWrite;
static U16 sStreamPcm[(LEAD-64U)/2];
static fPacmadMixerStream sReader;
static void *sReaderUser;
static U16 sStreamChannel;
static volatile U32 sUnderruns;
extern U16 PacmadMixer_Lock(void);
extern void PacmadMixer_Unlock(U16 sr);
extern void PacmadMixer_CoreFill(Params *params);
static void clear(U16 ch)
{
 Channel *c=&sChannels[ch];c->sample=c->next=0;c->current=(S8*)sSilence;
 c->end=(S8*)sSilence+SILENCE;c->loop=1;c->active=c->priority=0;
}
static void address(U32 reg,U32 value)
{HW8(reg)=value>>16;HW8(reg+2)=value>>8;HW8(reg+4)=value;}
static void start(void)
{
 U16 i;U32 base=(U32)sRing;
 HW8(0xffff8901UL)=0;
 for(i=0;i<RING/2;++i)sRing[i]=0;
 sWrite=LEAD;
 address(0xffff8903UL,base);address(0xffff890fUL,base+RING);
 HW8(0xffff8921UL)=0x81;HW8(0xffff8901UL)=3;
}
/* Reject a torn read across the low/middle-byte rollover. */
static U32 playhead(void)
{
 U16 tries;U8 hi,mid,lo;
 for(tries=0;tries<8;++tries) {
  hi=HW8(0xffff8909UL);mid=HW8(0xffff890bUL);lo=HW8(0xffff890dUL);
  if(mid==HW8(0xffff890bUL) && hi==HW8(0xffff8909UL))
   return ((U32)hi<<16)|((U32)mid<<8)|lo;
 }
 return 0;
}
/* The pure span calculation is shared by the scheduler regression test. */
U16 PacmadMixer_RefillCount(U16 write,U16 offset)
{return (U16)((((offset+LEAD)&(RING-1))&~1U)-write)&(RING-1);}
static void fill(U16 offset,U16 count)
{
 Params p;Channel saved;Sample stream;U16 got=0,i;
 p.channels=sChannels;p.dst=(S8*)sRing+offset;
 p.silence=(S8*)sSilence;p.count=count;p.saturate=sSaturate;
 if(sReader) {
  Channel *c=&sChannels[sStreamChannel];
  got=sReader(sReaderUser,(S8*)sStreamPcm,count);if(got>count)got=count;
  for(i=got;i<count;++i)((S8*)sStreamPcm)[i]=0;
  saved=*c;stream.data=(S8*)sStreamPcm;stream.length=count;stream.priority=0;
  c->sample=&stream;c->current=stream.data;c->end=stream.data+count;
  c->next=0;c->loop=0;c->active=1;
 }
 PacmadMixer_CoreFill(&p);
 if(sReader)sChannels[sStreamChannel]=saved;
}
U8 PacmadMixer_SetStream(U16 ch,fPacmadMixerStream reader,void *user)
{
 U16 sr;if(!sEnabled || ch>=3 || !reader)return 0;
 sr=PacmadMixer_Lock();if(sReader)clear(sStreamChannel);
 clear(ch);sStreamChannel=ch;sReaderUser=user;sReader=reader;
 PacmadMixer_Unlock(sr);return 1;
}
static void PacmadMixer_Vbl(void)
{
 U32 head,base=(U32)sRing;U16 offset,count,first,colour=0;U8 debug=sRaster;
 if(!sEnabled)return;
 head=playhead();if(head<base || head>=base+RING)return;
 offset=(U16)(head-base);count=PacmadMixer_RefillCount(sWrite,offset);
 if(!count)return;
 if(debug){colour=*(volatile U16*)0xffff8240UL;*(volatile U16*)0xffff8240UL=0x700;}
 /* Preserve 64 bytes of safety before DMA reaches the write cursor.
  * If too late, stop DMA before clearing/restarting instead of writing live audio. */
 if(count>LEAD-64U) {++sUnderruns;start();}
 else {
  first=RING-sWrite;if(first>count)first=count;
  fill(sWrite,first);if(count>first)fill(0,count-first);
  sWrite=(sWrite+count)&(RING-1);
 }
 if(debug)*(volatile U16*)0xffff8240UL=colour;
}
U8 PacmadMixer_Init(void)
{
 U16 sr,i;eSYSTEM_MCH mch;
 if(sEnabled)return 1;
 mch=System_GetMCH();if(mch!=MCH_STE && mch!=MCH_MEGASTE)return 0;
 if((U32)sRing+sizeof(sRing)>0x1000000UL)return 0;
 sr=PacmadMixer_Lock();
 if(!Vbl_AddCall(PacmadMixer_Vbl)){PacmadMixer_Unlock(sr);return 0;}
 for(i=0;i<SILENCE/2;++i)sSilence[i]=0;
 for(i=0;i<3;++i)clear(i);
 sUnderruns=0;start();sEnabled=1;PacmadMixer_Unlock(sr);return 1;
}
void PacmadMixer_DeInit(void)
{
 U16 sr=PacmadMixer_Lock();
 if(sEnabled){sEnabled=0;Vbl_RemoveCall(PacmadMixer_Vbl);HW8(0xffff8901UL)=0;}
 PacmadMixer_StopAll();sRaster=sSaturate=0;PacmadMixer_Unlock(sr);
}
static U8 valid(U16 ch,const S8 *pcm,U32 length)
{
 return sEnabled && ch<3 && pcm && !((U32)pcm&1UL) && length &&
 !(length&1UL) && length<=0x7fffffffUL && (U32)pcm+length>=(U32)pcm;
}
U8 PacmadMixer_Play(U16 ch,const S8 *pcm,U32 length,U8 loop)
{
 U16 sr;Sample *s;Channel *c;
 if(!valid(ch,pcm,length))return 0;
 sr=PacmadMixer_Lock();s=&sSamples[ch][0];s->data=pcm;s->length=length;s->priority=0;
 if(sReader && sStreamChannel==ch)sReader=0;
 c=&sChannels[ch];c->sample=s;c->current=pcm;c->end=pcm+length;c->next=0;
 c->loop=!!loop;c->active=1;c->priority=0;PacmadMixer_Unlock(sr);return 1;
}
U8 PacmadMixer_Queue(U16 ch,const S8 *pcm,U32 length)
{
 U16 sr;Sample *s;
 if(!valid(ch,pcm,length))return 0;
 sr=PacmadMixer_Lock();
 if((sReader && sStreamChannel==ch) || !sChannels[ch].active || sChannels[ch].loop){PacmadMixer_Unlock(sr);return 0;}
 /* Queue into the descriptor not currently used by the source. */
 s=&sSamples[ch][sChannels[ch].sample==&sSamples[ch][0]?1:0];
 s->data=pcm;s->length=length;s->priority=0;sChannels[ch].next=s;
 PacmadMixer_Unlock(sr);return 1;
}
void PacmadMixer_Stop(U16 ch)
{U16 sr;if(ch>=3)return;sr=PacmadMixer_Lock();if(sReader && ch==sStreamChannel)sReader=0;clear(ch);PacmadMixer_Unlock(sr);}
void PacmadMixer_StopAll(void)
{U16 sr,i;sr=PacmadMixer_Lock();sReader=0;sReaderUser=0;for(i=0;i<3;++i)clear(i);PacmadMixer_Unlock(sr);}
U8 PacmadMixer_IsPlaying(U16 ch)
{return sEnabled && ch<3 && ((sReader && ch==sStreamChannel) || sChannels[ch].active);}
const S8 *PacmadMixer_GetSample(U16 ch)
{U16 sr;const S8 *p=0;if(ch>=3 || !sEnabled)return 0;sr=PacmadMixer_Lock();if(sChannels[ch].active)p=sChannels[ch].sample->data;PacmadMixer_Unlock(sr);return p;}
U32 PacmadMixer_GetRemaining(U16 ch)
{U16 sr;U32 n=0;if(ch>=3 || !sEnabled)return 0;sr=PacmadMixer_Lock();if(sChannels[ch].active)n=(U32)(sChannels[ch].end-sChannels[ch].current);PacmadMixer_Unlock(sr);return n;}
void PacmadMixer_SetSaturation(U8 flag){sSaturate=!!flag;}
void PacmadMixer_SetRasterDebug(U8 flag){sRaster=!!flag;}
U32 PacmadMixer_GetUnderruns(void){U16 sr=PacmadMixer_Lock();U32 n=sUnderruns;PacmadMixer_Unlock(sr);return n;}
