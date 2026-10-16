#include <godlib/platform/platform.h>
#include <godlib/gemdos/gemdos.h>
#include <godlib/audio/pcmblock.h>
#include <godlib/audio/pcmframe.h>
#include <godlib/audio/pcmlayer.h>
#include <godlib/audio/amixer.h>
#include <godlib/vbl/vbl.h>
static U16 pcm[32768];
static U8 legacyEnabled;
volatile U16 gProfileRequest=0xffff,gProfileReady=0xffff,gProfileFailures;
volatile U32 gProfileFrames;
static void CountFrame(void) { ++gProfileFrames; }
static void Prepare(U16 request)
{
 U16 channel,count=request%10,kind=request/10;U8 ok=1;
 PcmBlockMixer_DeInit();PcmFrameMixer_DeInit();PcmLayerMixer_DeInit();
 if(legacyEnabled) {AudioMixer_DeInit();legacyEnabled=0;}
 if(kind<2) {
  ok=PcmBlockMixer_Init(kind?ePCMBLOCK_FREQ_25K:ePCMBLOCK_FREQ_12K);
  for(channel=0;channel<count;++channel) ok&=PcmBlockMixer_Play(channel,(const S8*)pcm,1024,1);
 } else if(kind==2) {
  ok=PcmFrameMixer_Init();
  /* Channel zero is a long one-shot; other channels cross loop boundaries. */
  for(channel=0;channel<count;++channel) ok&=PcmFrameMixer_Play(channel,(const S8*)pcm,channel?4:255,channel?1:0);
 } else if(kind==3) {
  ok=PcmLayerMixer_Init();
  for(channel=0;channel<count;++channel) ok&=PcmLayerMixer_Play(channel,(const S8*)pcm,channel<2?1024:65536UL,channel<2?1:0);
 }
 if(kind==4) {
  sAudioDmaSound sound;
  AudioMixer_Init();AudioMixer_Enable();legacyEnabled=1;
  sound.mpSound=(U8*)pcm;sound.mLength=65536UL;sound.mFreq=eAUDIO_FREQ_12;
  sound.mBits=eAUDIO_BITS_8;sound.mStereoFlag=0;sound.mLoopingFlag=0;
  for(channel=0;channel<count;++channel)
   ok&=AudioMixer_PlaySample(&sound,count==1?eAMIXER_PAN_CENTRE:channel?eAMIXER_PAN_RIGHT:eAMIXER_PAN_LEFT);
 }
 if(!ok) ++gProfileFailures;
 gProfileReady=request;
}
S16 GodLib_Game_Main(S16 argc,char **argv)
{
 U32 i;(void)argc;(void)argv;GemDos_Super(0);Platform_Init();
 for(i=0;i<32768;++i) pcm[i]=(U16)(((i*7)&0x1f)*0x0101);
 Vbl_AddCall(CountFrame);
 for(;;) if(gProfileRequest!=gProfileReady) Prepare(gProfileRequest);
 return 0;
}
