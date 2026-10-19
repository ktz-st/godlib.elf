/* Two-voice signed PCM mixer. Control-side tables, 68000 stereo summation. */
#include "amixer.h"
#include "am_sine.h"
#include "ampanlaw.h"
#include <godlib/memory/memory.h>
#include <godlib/system/system.h>
#include <godlib/vbl/vbl.h>
#define RING 8192U
#define LEAD 2048U
#define HW8(a) (*(volatile U8 *)(a))
U16 gAudioMixerDMAHardWareFlag;
sAmixerConfig gAudioMixerConfig;
U8 *gpAudioMixerBuffer, *gpAudioMixerSilence;
U32 gAudioMixerBufferOffset;
volatile U8 gAudioMixerEnableFlag;
sAmixerSpl gAudioMixerSamples[2];
/* Two banks per voice: rebuild the inactive bank without blocking interrupts. */
static S16 sTables[2][2][512];
static U8 sBank[2],sTableValid[2],sTableLaw[2];
static U8 sLoop[2],sStep[2];
static U16 sFrequency=eAUDIO_FREQ_25,sLead=LEAD,sSilentBytes=RING;
typedef struct { sAmixerSpl *voices; U8 *dst; U16 frames,step0,step1,active,safe; } Params;
typedef char AmixerLayoutCheck[sizeof(sAmixerSpl)==28?1:-1];
extern U16 AudioMixer_Lock(void);
extern void AudioMixer_Unlock(U16 sr);
extern void AudioMixer_Core(Params *params);
static void clear(U16 ch)
{
 sAmixerSpl *v=&gAudioMixerSamples[ch];
 v->mActiveFlag=0; v->mpStart=v->mpCurrent=gpAudioMixerSilence;
 v->mpEnd=gpAudioMixerSilence+4096; v->mLength=4096;
 v->mRepeat=1;v->mPhase=1;sLoop[ch]=0;sStep[ch]=1;
}
static void gains(U16 ch,U8 volume,U8 pan)
{
 U16 i,left,right,bank=sBank[ch]^1U,sr; S16 *t=sTables[ch][bank];
 if(sTableValid[ch] && gAudioMixerSamples[ch].mVolume==volume &&
    gAudioMixerSamples[ch].mPan==pan && sTableLaw[ch]==gAudioMixerConfig.mPanType)return;
 switch(gAudioMixerConfig.mPanType) {
 case eMixer_PanType_ConstantPower:
  left=gAudioMixerSineLaw[pan];right=pan;break;
 case eMixer_PanType_PanLaw:
  left=gAudioMixerPanLawLeftTable[pan];right=gAudioMixerPanLawRightTable[pan];break;
 default:left=255-pan;right=pan;break;
 }
 left=((U32)left*volume+127)/255;right=((U32)right*volume+127)/255;
 for(i=0;i<256;++i) {
  S16 x=i<128?(S16)i:(S16)(i-256);
  U16 l=(U8)(S16)(((S32)x*left)/255),r=(U8)(S16)(((S32)x*right)/255);
  t[i]=(S16)((l<<8)|r);
  /* Extra word view: right lane biased by 128, for carry-corrected ADD.W. */
  t[256+i]=(S16)((l<<8)|(r^128));
 }
 sr=AudioMixer_Lock();
 gAudioMixerSamples[ch].mVolume=volume;gAudioMixerSamples[ch].mPan=pan;
 gAudioMixerSamples[ch].mGainLeft=left;gAudioMixerSamples[ch].mGainRight=right;
 gAudioMixerSamples[ch].mpTable=t;sBank[ch]=bank;sTableLaw[ch]=gAudioMixerConfig.mPanType;sTableValid[ch]=1;
 AudioMixer_Unlock(sr);
}
/* Output frames remaining, including an incomplete repeated source byte. */
static U32 framesLeft(U16 ch)
{
 sAmixerSpl *v=&gAudioMixerSamples[ch];
 U32 bytes=(U32)((U8*)v->mpEnd-(U8*)v->mpCurrent);
 if(sStep[ch]==2)bytes=(bytes+1)>>1;
 bytes--;
 if(v->mRepeat==2)bytes<<=1;else if(v->mRepeat==4)bytes<<=2;
 return bytes+v->mPhase;
}
/* Split at source ends, so the assembler never reads past the source data. */
static void fill(U8 *dst,U16 frames)
{
 Params p;U16 n,ch;U32 remaining;
 p.voices=gAudioMixerSamples;
 if(!gAudioMixerSamples[0].mActiveFlag && !gAudioMixerSamples[1].mActiveFlag){
  /* One complete silent revolution makes further stores redundant. */
  if(sSilentBytes<RING){Memory_Clear((U32)frames*2,dst);sSilentBytes+=frames*2;if(sSilentBytes>RING)sSilentBytes=RING;}
  return;
 }
 sSilentBytes=0;
 while(frames) {
  n=frames;
  for(ch=0;ch<2;++ch) {
   sAmixerSpl *v=&gAudioMixerSamples[ch];
   if(!v->mActiveFlag) {v->mpCurrent=gpAudioMixerSilence;v->mPhase=1;v->mRepeat=1;}
   else {
    remaining=framesLeft(ch);
    if(remaining<n)n=(U16)remaining;
   }
  }
  p.dst=dst;p.frames=n;p.step0=sStep[0];p.step1=sStep[1];p.active=(gAudioMixerSamples[0].mActiveFlag?1:0)|(gAudioMixerSamples[1].mActiveFlag?2:0);
  /* At summed gains <=256, independently truncated signed bytes cannot clip. */
  p.safe=(p.active==3 && (U16)gAudioMixerSamples[0].mGainLeft+gAudioMixerSamples[1].mGainLeft<=256 &&
          (U16)gAudioMixerSamples[0].mGainRight+gAudioMixerSamples[1].mGainRight<=256);
  AudioMixer_Core(&p);dst+=n*2;frames-=n;
  for(ch=0;ch<2;++ch) {
   sAmixerSpl *v=&gAudioMixerSamples[ch];
   if(v->mActiveFlag && (U32)v->mpCurrent>=(U32)v->mpEnd) {
    if(sLoop[ch]) {
     /* Preserve decimation phase across an odd-length source loop. */
     remaining=(U32)v->mpCurrent-(U32)v->mpEnd;
     if(remaining>=v->mLength)remaining=0; /* one-byte loop, step two */
     v->mpCurrent=(U8*)v->mpStart+remaining;
    } else clear(ch);
   }
  }
 }
}
static U32 playhead(void)
{
 U16 tries;U8 hi,mid,lo;
 for(tries=0;tries<8;++tries) {
  hi=HW8(0xffff8909UL);mid=HW8(0xffff890bUL);lo=HW8(0xffff890dUL);
  if(hi==HW8(0xffff8909UL) && mid==HW8(0xffff890bUL))
   return ((U32)hi<<16)|((U32)mid<<8)|lo;
 }
 return 0;
}
void AudioMixer_Vbl(void)
{
 U32 head,base=(U32)gpAudioMixerBuffer;U16 target,count,first,write;
 if(!gAudioMixerEnableFlag)return;
 head=playhead();if(head<base || head>=base+RING)return;
 target=((U16)(head-base)+sLead)&(RING-2);
 write=(U16)gAudioMixerBufferOffset;count=(target-write)&(RING-1);
 if(!count)return;
 first=RING-write;if(first>count)first=count;
 fill(gpAudioMixerBuffer+write,first/2);
 if(count>first)fill(gpAudioMixerBuffer,(count-first)/2);
 gAudioMixerBufferOffset=target;
}
void AudioMixer_Init(void)
{
 U16 ch;U16 mch=System_GetMCH();
 if(gpAudioMixerBuffer)return;
 gAudioMixerConfig.mBits=eAUDIO_BITS_8;gAudioMixerConfig.mStereoFlag=1;
 gAudioMixerConfig.mPanType=eMixer_PanType_Linear;
 sFrequency=eAUDIO_FREQ_25;sLead=LEAD;
 gAudioMixerDMAHardWareFlag=(mch==MCH_STE || mch==MCH_MEGASTE || mch==MCH_TT || mch==MCH_FALCON);
 gpAudioMixerSilence=(U8*)mMEMSCREENCALLOC(4096);
 gpAudioMixerBuffer=(U8*)mMEMSCREENCALLOC(RING);
 if(!gpAudioMixerSilence || !gpAudioMixerBuffer) {
  if(gpAudioMixerSilence)mMEMSCREENFREE(gpAudioMixerSilence);
  if(gpAudioMixerBuffer)mMEMSCREENFREE(gpAudioMixerBuffer);
  gpAudioMixerSilence=gpAudioMixerBuffer=0;gAudioMixerDMAHardWareFlag=0;return;
 }
 for(ch=0;ch<2;++ch){sTableValid[ch]=0;clear(ch);gains(ch,0,128);}
}
void AudioMixer_Disable(void)
{
 U16 sr=AudioMixer_Lock();
 if(gAudioMixerEnableFlag){gAudioMixerEnableFlag=0;HW8(0xffff8901UL)=0;Vbl_RemoveCall(AudioMixer_Vbl);}
 AudioMixer_Unlock(sr);
}
void AudioMixer_DeInit(void)
{
 AudioMixer_Disable();
 if(gpAudioMixerSilence)mMEMSCREENFREE(gpAudioMixerSilence);
 if(gpAudioMixerBuffer)mMEMSCREENFREE(gpAudioMixerBuffer);
 gpAudioMixerSilence=gpAudioMixerBuffer=0;gAudioMixerDMAHardWareFlag=0;
 gAudioMixerSamples[0].mActiveFlag=gAudioMixerSamples[1].mActiveFlag=0;
}
void AudioMixer_Enable(void)
{
 U16 ch; sAudioDmaSound sound;
 if(!gAudioMixerDMAHardWareFlag || gAudioMixerEnableFlag)return;
 Memory_Clear(RING,gpAudioMixerBuffer);sSilentBytes=RING;
 for(ch=0;ch<2;++ch)clear(ch);
 gAudioMixerBufferOffset=sLead;
 sound.mpSound=gpAudioMixerBuffer;sound.mLength=RING;sound.mFreq=(U8)sFrequency;
 sound.mBits=eAUDIO_BITS_8;sound.mLoopingFlag=1;sound.mStereoFlag=1;
 Audio_DmaPlaySound(&sound);gAudioMixerEnableFlag=1;Vbl_AddCall(AudioMixer_Vbl);
}
void AudioMixer_StopAll(void)
{
 U8 enabled=gAudioMixerEnableFlag;U16 ch;
 AudioMixer_Disable();
 if(gpAudioMixerBuffer){for(ch=0;ch<2;++ch)clear(ch);Memory_Clear(RING,gpAudioMixerBuffer);sSilentBytes=RING;}
 if(enabled)AudioMixer_Enable();
}
U16 AudioMixer_GetFrequency(void){return sFrequency;}
U8 AudioMixer_SetFrequency(U16 frequency)
{
 U16 ch;U8 enabled;
 if(frequency!=eAUDIO_FREQ_12 && frequency!=eAUDIO_FREQ_25)return 0;
 if(frequency==sFrequency)return 1;
 enabled=gAudioMixerEnableFlag;AudioMixer_Disable();
 sFrequency=frequency;sLead=frequency==eAUDIO_FREQ_12?LEAD/2:LEAD;
 if(gpAudioMixerBuffer) {
  for(ch=0;ch<2;++ch)clear(ch);
  Memory_Clear(RING,gpAudioMixerBuffer);sSilentBytes=RING;gAudioMixerBufferOffset=sLead;
 }
 if(enabled)AudioMixer_Enable();
 return 1;
}
sAmixerConfig *AudioMixer_GetpConfig(void){return &gAudioMixerConfig;}
void AudioMixer_SetConfig(const sAmixerConfig *config)
{
 U16 ch;U8 enabled=gAudioMixerEnableFlag;
 if(!config || config->mBits!=eAUDIO_BITS_8 || !config->mStereoFlag || config->mPanType>=eMixer_PanType_Limit)return;
 AudioMixer_Disable();gAudioMixerConfig=*config;
 for(ch=0;ch<2;++ch)gains(ch,gAudioMixerSamples[ch].mVolume,gAudioMixerSamples[ch].mPan);
 if(enabled)AudioMixer_Enable();
}
U8 AudioMixer_PlayVoice(U16 ch,const sAudioDmaSound *sound,U8 volume,U8 pan)
{
 U16 sr; sAmixerSpl *v;
 if(!gAudioMixerEnableFlag || ch>=2 || !sound || !sound->mpSound || !sound->mLength ||
    sound->mBits!=eAUDIO_BITS_8 || sound->mStereoFlag || sound->mFreq>eAUDIO_FREQ_25)return 0;
 /* Keep the old voice playing while building its replacement's table. */
 gains(ch,volume,pan);sr=AudioMixer_Lock();v=&gAudioMixerSamples[ch];
 v->mpStart=v->mpCurrent=sound->mpSound;v->mpEnd=sound->mpSound+sound->mLength;
 v->mLength=sound->mLength;
 if(sFrequency>=sound->mFreq){v->mRepeat=1U<<(sFrequency-sound->mFreq);sStep[ch]=1;}
 else {v->mRepeat=1;sStep[ch]=2;}
 v->mPhase=v->mRepeat;
 sLoop[ch]=sound->mLoopingFlag!=0;v->mActiveFlag=1;sSilentBytes=0;
 AudioMixer_Unlock(sr);return 1;
}
void AudioMixer_SetVoiceParams(U16 ch,U8 volume,U8 pan)
{if(ch<2 && gpAudioMixerBuffer)gains(ch,volume,pan);}
void AudioMixer_StopVoice(U16 ch)
{U16 sr;if(ch>=2 || !gpAudioMixerBuffer)return;sr=AudioMixer_Lock();clear(ch);AudioMixer_Unlock(sr);}
U8 AudioMixer_IsVoicePlaying(U16 ch)
{return ch<2?gAudioMixerSamples[ch].mActiveFlag:0;}
U8 AudioMixer_CanPlayNewSpl(void)
{return gAudioMixerEnableFlag && (!gAudioMixerSamples[0].mActiveFlag || !gAudioMixerSamples[1].mActiveFlag);}
U8 AudioMixer_PlaySample(sAudioDmaSound *sound,const U8 pan)
{U16 ch;for(ch=0;ch<2;++ch)if(!gAudioMixerSamples[ch].mActiveFlag)return AudioMixer_PlayVoice(ch,sound,255,pan);return 0;}
U8 AudioMixer_PlaySampleDirect(sAudioDmaSound *sound,const U8 pan)
{
 U16 ch,best=0;U32 left,least=0xffffffffUL;
 for(ch=0;ch<2;++ch){left=gAudioMixerSamples[ch].mActiveFlag?framesLeft(ch):0;if(left<least){least=left;best=ch;}}
 return AudioMixer_PlayVoice(best,sound,255,pan);
}
U8 *AudioMixer_GetpBuffer(void){return gpAudioMixerBuffer;}
