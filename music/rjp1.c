/* RJP1SMOD preparation and lifecycle. Sequencer: Andi Smithers / VectorDean.
 * Original banks remain untouched: only audible PCM is converted to 7 bits.
 * Each unique attack+sustain gets a 664-byte mixer lookahead guard. */
#include "rjp1.h"
#include "paulamixer.h"
#include "leonardmixer.h"
#include <godlib/file/file.h>
#include <godlib/memory/memory.h>
#include <godlib/system/system.h>
#include <godlib/vbl/vbl.h>
#include <string.h>

struct sRjp1Song {sRjp1Info info; U8 *song;};
static sRjp1Song *spSong;
volatile U8 gRjp1Playing,gRjp1Mixer;
extern void RjpCore_Init(void *song,void *unusedBank);
extern void RjpCore_StartSong(U16 song);
extern void RjpCore_Stop(void);
extern void RjpCore_Volume(S16 channel,U16 volume);
static U16 word(const U8 *p){return ((U16)p[0]<<8)|p[1];}
static U32 lng(const U8 *p){return ((U32)word(p)<<16)|word(p+2);}
static void put(U8 *p,U32 v){p[0]=(U8)(v>>24);p[1]=(U8)(v>>16);p[2]=(U8)(v>>8);p[3]=(U8)v;}
static U8 range(U32 offset,U32 count,U32 size){return offset<=size && count<=size-offset;}
/* A sustain already ending at the attack boundary needs no second copy. */
static U8 suffix(const U8 *p)
{return word(p+22)>1 && word(p+20)>=word(p+16) && (U32)word(p+20)+word(p+22)==(U32)word(p+16)+word(p+18);}
static U16 duplicate(const U8 *records,U16 index)
{
 U16 j;const U8 *p=records+32UL*index;
 for(j=0;j<index;++j){const U8 *q=records+32UL*j;
  if(lng(p)==lng(q) && !memcmp(p+16,q+16,8))return j;
 }
 return index;
}
/* Word-aligned section tables are needed by the native 68000 replay.
 * This validates container/referenced static data, not arbitrary untrusted
 * bytecode: sequence streams are the original player's trusted music format. */
static U8 parse(const U8 *s,U32 size,const U8 *ins,U32 insSize,const U8 **part,U32 *len)
{
 U32 off=8,i,j,n,base,attack,repeat,end;const U8 *p;
 if(!s || !ins || size<36 || insSize<4 || memcmp(s,"RJP1SMOD",8) || memcmp(ins,"RJP1",4))return 0;
 insSize-=4;
 for(i=0;i<7;++i){if(!range(off,4,size))return 0;n=lng(s+off);off+=4;if(!range(off,n,size))return 0;part[i]=s+off;len[i]=n;off+=n;}
 if(!len[0] || len[0]%32 || len[0]>256UL*32 || !len[1] || !len[2] || len[2]%4 || len[2]>256 || !len[3] || len[3]%4 || !len[4] || len[4]%4 || !len[5] || !len[6])return 0;
 if(((U32)(part[0]-s)|(U32)(part[3]-s)|(U32)(part[4]-s))&1)return 0;
 for(i=0;i<len[2];++i)if((U32)part[2][i]*4>=len[3])return 0;
 for(i=3;i<=4;++i)for(j=0;j<len[i];j+=4)if(lng(part[i]+j)>len[i+2])return 0;
 for(i=0;i<len[0];i+=32){
  p=part[0]+i;base=lng(p);attack=2UL*word(p+18);repeat=2UL*word(p+22);
  if(!attack || !range(base,2UL*word(p+16)+attack,insSize))return 0;
  if(repeat>65534UL || (repeat>2 && !range(base,2UL*word(p+20)+repeat,insSize)))return 0;
  if(!range(word(p+12),6,len[1]))return 0;
  for(j=4;j<=8;j+=4){end=2UL*word(p+(j==4?26:30));if(lng(p+j) && (!end || !range(lng(p+j),end,insSize) || 2UL*word(p+(j==4?24:28))>=end))return 0;}
 }
 return 1;
}
sRjp1Song *Rjp1_Create(const void *sng,U32 sngBytes,const void *bank,U32 insBytes)
{return Rjp1_CreateEx(sng,sngBytes,bank,insBytes,eRJP1_MIXER_LANCE);}
sRjp1Song *Rjp1_CreateEx(const void *sng,U32 sngBytes,const void *bank,U32 insBytes,U16 mixer)
{
 const U8 *part[7],*p,*q,*ins=(const U8*)bank;U32 len[7],bytes,modBytes=0,sampleBytes=0,attack,repeat,n,i,j;U16 count,unique=0,other;U8 *dest,*record,*wave; sRjp1Song *song;
 if(mixer>eRJP1_MIXER_LEONARD)return 0;
 if(!parse((const U8*)sng,sngBytes,ins,insBytes,part,len))return 0;
 count=(U16)(len[0]/32);ins+=4;
 if(mixer==eRJP1_MIXER_LEONARD){
  bytes=((sngBytes+1)&~1UL)+insBytes;
  song=mMEMCALLOC(sizeof(*song)+bytes);
  if(!song)return 0;
  song->song=(U8*)(song+1);memcpy(song->song,sng,sngBytes);
  dest=song->song+((sngBytes+1)&~1UL);memcpy(dest,bank,insBytes);dest+=4;
  for(i=0;i<count;++i){p=part[0]+32*i;record=song->song+(part[0]-(const U8*)sng)+32*i;
   put(record,(U32)(dest+lng(p)));
   for(j=4;j<=8;j+=4)if(lng(p+j))put(record+j,(U32)(dest+lng(p+j)));
  }
  song->info.mSourceBytes=sngBytes+insBytes;song->info.mResidentBytes=sizeof(*song)+bytes;
  song->info.mSampleBytes=insBytes-4;song->info.mModulationBytes=0; /* retained in original bank */
  song->info.mSubSongs=(U16)(len[2]/4);song->info.mInstruments=count;
  song->info.mUniqueSamples=0;song->info.mMixer=mixer;
  return song;
 }

 for(i=0;i<count;++i){p=part[0]+32*i;
  if(duplicate(part[0],(U16)i)==i){attack=2UL*word(p+18);repeat=word(p+22)>1?2UL*word(p+22):0;sampleBytes+=attack+(suffix(p)?0:repeat)+664UL;++unique;}
  for(j=4;j<=8;j+=4)if(lng(p+j))modBytes+=2UL*word(p+(j==4?26:30));
 }
 bytes=((sngBytes+1)&~1UL)+sampleBytes+modBytes;
 song=mMEMCALLOC(sizeof(*song)+bytes);
 if(!song)return 0;
 song->song=(U8*)(song+1);memcpy(song->song,sng,sngBytes);
 dest=song->song+((sngBytes+1)&~1UL);
 for(i=0;i<count;++i){
  p=part[0]+32*i;record=song->song+(part[0]-(const U8*)sng)+32*i;
  other=duplicate(part[0],(U16)i);
  if(other!=i){q=song->song+(part[0]-(const U8*)sng)+32UL*other;put(record,lng(q));}
  else {
   wave=dest;attack=2UL*word(p+18);repeat=word(p+22)>1?2UL*word(p+22):0;
   q=ins+lng(p)+2UL*word(p+16);
   for(n=0;n<attack;++n)*dest++=(U8)((S8)q[n]>>1);
   /* Native player clears the first attack word on every retrigger. */
   wave[0]=wave[1]=0;
   q=ins+lng(p)+2UL*word(p+20);
   if(!suffix(p))for(n=0;n<repeat;++n)*dest++=(U8)((S8)q[n]>>1);
   if(repeat){U32 loop=suffix(p)?2UL*(word(p+20)-word(p+16)):attack;
    /* Original Paula clears the attack word; retain that in shared loops. */
    if(!suffix(p))for(n=0;n<repeat;++n)if(2UL*word(p+20)+n>=2UL*word(p+16) && 2UL*word(p+20)+n<2UL*word(p+16)+2)wave[loop+n]=0;
    for(n=0;n<664;++n)*dest++=wave[loop+n%repeat];
   }else {memset(dest,0,664);dest+=664;}
   put(record,(U32)wave);
  }
  record[16]=record[17]=0;
  {U16 loop=suffix(p)?word(p+20)-word(p+16):word(p+18);record[20]=(U8)(loop>>8);record[21]=(U8)loop;}
  /* The core is pre-relocated; pitch/volume waves stay full signed 8-bit. */
  for(j=4;j<=8;j+=4){n=lng(p+j);if(n){U32 size=2UL*word(p+(j==4?26:30));put(record+j,(U32)dest);memcpy(dest,ins+n,size);dest+=size;}}
 }
 song->info.mSourceBytes=sngBytes+insBytes;song->info.mResidentBytes=sizeof(*song)+bytes;
 song->info.mSampleBytes=sampleBytes;song->info.mModulationBytes=modBytes;
 song->info.mSubSongs=(U16)(len[2]/4);song->info.mInstruments=count;song->info.mUniqueSamples=unique;
 return song;
}
sRjp1Song *Rjp1_Load(const char *sngName,const char *insName)
{return Rjp1_LoadEx(sngName,insName,eRJP1_MIXER_LANCE);}
sRjp1Song *Rjp1_LoadEx(const char *sngName,const char *insName,U16 mixer)
{
 S32 a,b;void *s=0,*ins=0;sRjp1Song *song=0;
 if(!sngName || !insName)return 0;
 a=File_GetSize(sngName);b=File_GetSize(insName);
 if(a<=0 || b<=0)return 0;
 s=File_Load(sngName);ins=File_Load(insName);
 if(s && ins)song=Rjp1_CreateEx(s,(U32)a,ins,(U32)b,mixer);
 if(s){File_UnLoad(s);}if(ins){File_UnLoad(ins);}return song;
}
void Rjp1_UnLoad(sRjp1Song *song){if(!song)return;if(song==spSong)Rjp1_Stop();mMEMFREE(song);}
U8 Rjp1_GetInfo(const sRjp1Song *song,sRjp1Info *info){if(!song || !info)return 0;*info=song->info;return 1;}
U32 Rjp1_GetWorkspaceSize(U8 rate){return PaulaMixer_GetWorkspaceSize(rate>=1 && rate<=3?rate:2);}
U32 Rjp1_GetWorkspaceSizeEx(U8 rate,U16 mixer)
{return mixer==eRJP1_MIXER_LEONARD?LeonardMixer_GetWorkspaceSize(rate):Rjp1_GetWorkspaceSize(rate);}
U32 Rjp1_GetAllocatedSize(void)
{return gRjp1Mixer==eRJP1_MIXER_LEONARD?(LeonardMixer_GetOwner()==2?LeonardMixer_GetAllocatedSize():0):(PaulaMixer_GetOwner()==2?PaulaMixer_GetAllocatedSize():0);}
U8 Rjp1_SetSubSong(U16 subSong)
{
 U16 mask;if(!spSong || subSong>=spSong->info.mSubSongs)return 0;
 mask=System_GetIML();System_SetIML(7);
 RjpCore_Init(spSong->song,0);
 RjpCore_Volume(-1,64);RjpCore_StartSong(subSong);
 System_SetIML(mask);return 1;
}
U8 Rjp1_Start(sRjp1Song *song,U16 subSong,U8 rate)
{
 U16 mask;U8 ok;
 if(!song || rate<1 || rate>3 || subSong>=song->info.mSubSongs || (song->info.mMixer==eRJP1_MIXER_LEONARD && rate>2))return 0;
 Rjp1_Stop();
 if(song->info.mMixer==eRJP1_MIXER_LEONARD){if(!LeonardMixer_Init(2,rate))return 0;}
 else if(!PaulaMixer_Init(2,rate))return 0;
 gRjp1Mixer=(U8)song->info.mMixer;
 spSong=song;
 /* Init sets master and channel masks. It never mutates prepared records. */
 mask=System_GetIML();System_SetIML(7);
 RjpCore_Init(song->song,0);RjpCore_StartSong(subSong);gRjp1Playing=1;
 ok=Vbl_AddCall(Rjp1_Tick);
 System_SetIML(mask);
 if(!ok)Rjp1_Stop();
 return ok;
}
void Rjp1_Stop(void)
{
 U16 mask=System_GetIML();System_SetIML(7);
 Vbl_RemoveCall(Rjp1_Tick);gRjp1Playing=0;
 if(spSong){RjpCore_Stop();spSong=0;}
 System_SetIML(mask);
 if(gRjp1Mixer==eRJP1_MIXER_LEONARD)LeonardMixer_Shutdown(2);
 else PaulaMixer_Shutdown(2);
 gRjp1Mixer=eRJP1_MIXER_LANCE;
}
void Rjp1_SetVolume(U16 volume)
{U16 mask;if(!spSong)return;if(volume>64)volume=64;mask=System_GetIML();System_SetIML(7);RjpCore_Volume(-1,volume);System_SetIML(mask);}
