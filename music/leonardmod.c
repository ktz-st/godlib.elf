/* Leonard Paula + ProTracker 2.3f: rate-dependent ST-RAM workspace. */
#include "leonardmod.h"
#include <godlib/file/file.h>
#include <godlib/memory/memory.h>
#include <godlib/system/system.h>

#include "leonardmixer.h"
extern U8 LeonardMod_CoreStart(void *apMod);
static U16 Word(const U8 *p){return ((U16)p[0]<<8)|p[1];}

U8 LeonardMod_GetModuleInfo(const void *apHeader,U32 aFileBytes,sLeonardModModuleInfo *apInfo)
{
    const U8 *p=apHeader;
    sLeonardModModuleInfo info;
    U16 i,max=0,count=0;
    U32 bytes=0;
    if(!p || !apInfo || aFileBytes<1084UL)return 0;
    if(p[1080]!='M' || p[1081]!='.' || p[1082]!='K' || p[1083]!='.' ||
       !p[950] || p[950]>128)return 0;
    for(i=0;i<128;++i){if(p[952+i]>127)return 0;if(p[952+i]>max)max=p[952+i];}
    for(i=0;i<31;++i) {
        const U8 *h=p+20+30*i;
        U32 len=Word(h+22),start=Word(h+26),loop=Word(h+28);
        if(h[24]>15 || h[25]>64 || (loop>1 && (!len || start+loop>len)))return 0;
        bytes+=2UL*len;if(len)++count;
    }
    info.mFileBytes=aFileBytes;info.mPatternCount=max+1;info.mSampleCount=count;
    info.mSampleBytes=bytes;info.mModuleBytes=1084UL+1024UL*(max+1)+bytes;
    info.mLoadBytes=info.mModuleBytes; /* no per-sample guard copies */
    if(info.mModuleBytes>aFileBytes)return 0;
    *apInfo=info;return 1;
}
void *LeonardMod_Load(const char *apFileName)
{
    U8 header[1084],*p=0;
    sLeonardModModuleInfo info;
    S32 size=File_GetSize(apFileName);
    sFileHandle f;
    if(size<1084)return 0;
    f=File_Open(apFileName);if(f<=0)return 0;
    if(File_Read(f,1084UL,header)==1084L &&
       LeonardMod_GetModuleInfo(header,(U32)size,&info)) {
        p=mMEMCALLOC(info.mLoadBytes);
        if(p) {
            Memory_Copy(1084UL,header,p);
            if(File_Read(f,info.mModuleBytes-1084UL,p+1084)!=(S32)(info.mModuleBytes-1084UL)) {
                mMEMFREE(p);p=0;
            }
        }
    }
    File_Close(f);return p;
}
void LeonardMod_UnLoad(void *apModData){if(apModData){mMEMFREE(apModData);}}
U32 LeonardMod_GetPaulaWorkspaceSize(U8 rate){return LeonardMixer_GetWorkspaceSize(rate);}
U32 LeonardMod_GetPaulaAllocatedSize(void){return LeonardMixer_GetOwner()==1?LeonardMixer_GetAllocatedSize():0;}
void LeonardMod_ShutdownPaula(void)
{
 if(LeonardMixer_GetOwner()!=1)return;
 LeonardMod_Stop();LeonardMixer_Shutdown(1);
}
U8 LeonardMod_Start(void *mod,U8 rate)
{
 if(LeonardMixer_GetOwner() && LeonardMixer_GetOwner()!=1)return 0;
 LeonardMod_ShutdownPaula();
 if(!mod || (System_GetMCH()!=MCH_STE && System_GetMCH()!=MCH_MEGASTE))return 0;
 if(!LeonardMixer_Init(1,rate==1?1:2))return 0;
 LeonardMod_CoreStart(mod);return 1;
}
