/* Wizzcat/Delta Force: exact persistent workspace and transient preparation copy. */
#include "wizzcat.h"
#include <godlib/file/file.h>
#include <godlib/memory/memory.h>
#include <godlib/system/system.h>
#include <godlib/vbl/vbl.h>
#include <stddef.h>

extern void WIZinit(void);
extern void WIZmodInit(void *apModule,void *apEnd);
extern void WIZsetScratch(void *apEnd,void *apSilence);
extern void WIZsetActive(U8 aPlaying);
extern void WIZplay(void);
extern void WIZgetInfo(sWizzcatInfo *apInfo);
extern void WIZjump(U8 aPosition);
extern void Wizzcat_Vbl(void);
static sWizzcatModule *spCurrent;
static U8 sTablesReady,sVblInstalled;
static U16 Word(const U8 *p){return ((U16)p[0]<<8)|p[1];}

U8 Wizzcat_GetModuleInfo(const void *apHeader,U32 aFileBytes,sWizzcatModuleInfo *apInfo)
{
    const U8 *p=apHeader;
    sWizzcatModuleInfo info;
    U16 i,max=0,count=0;
    U32 samples=0,expanded=0,needed;
    if(!p || !apInfo || aFileBytes<1084UL)return 0;
    if(p[1080]!='M' || p[1081]!='.' || p[1082]!='K' || p[1083]!='.' ||
       !p[950] || p[950]>128)return 0;
    for(i=0;i<128;++i){if(p[952+i]>127)return 0;if(p[952+i]>max)max=p[952+i];}
    for(i=0;i<31;++i) {
        const U8 *h=p+20+30*i;
        U32 len=Word(h+22),start=Word(h+26),loop=Word(h+28),prefix,repeat;
        if(h[24]>15 || h[25]>64 || (loop>1 && (!len || start+loop>len)) || start>len)return 0;
        samples+=len*2;
        if(!len)continue;
        ++count;prefix=2UL*(start?start:len);repeat=2UL*(loop?loop:1);
        /* Native positions/loop lengths are words holding byte offsets. */
        if(prefix>65534UL || repeat>32766UL)return 0;
        repeat=((1600UL+repeat-1)/repeat)*repeat;
        if(prefix+repeat>65535UL)return 0;
        expanded+=prefix+repeat+1600UL;
    }
    info.mFileBytes=aFileBytes;info.mPatternCount=max+1;info.mSampleCount=count;
    info.mSampleBytes=samples;info.mPreparedSampleBytes=expanded;
    info.mModuleBytes=1084UL+1024UL*(max+1)+samples;
    if(info.mModuleBytes>aFileBytes)return 0;
    info.mSilenceBytes=count<31?3200UL:0UL;
    needed=expanded+info.mSilenceBytes;
    info.mWorkspaceBytes=(needed>samples?needed-samples:0UL)+4UL;
    info.mInitScratchBytes=samples?samples:2UL;
    info.mLoadBytes=(U32)offsetof(sWizzcatModule,mData)+info.mModuleBytes+info.mWorkspaceBytes;
    *apInfo=info;return 1;
}
sWizzcatModule *Wizzcat_LoadEx(const char *apFileName,U32 aWorkspaceSize)
{
    U8 header[1084];sWizzcatModuleInfo info;sFileHandle f;
    sWizzcatModule *mod=0;S32 size=File_GetSize(apFileName);
    if(size<1084)return 0;
    f=File_Open(apFileName);if(f<=0)return 0;
    if(File_Read(f,1084UL,header)==1084L && Wizzcat_GetModuleInfo(header,(U32)size,&info)) {
        /* Zero means automatic sizing; explicit margins must be large enough. */
        if(!aWorkspaceSize)aWorkspaceSize=info.mWorkspaceBytes;
        if(aWorkspaceSize>=info.mWorkspaceBytes && aWorkspaceSize<=0x7fffffffUL-info.mModuleBytes-16UL) {
            mod=mMEMCALLOC((U32)offsetof(sWizzcatModule,mData)+info.mModuleBytes+aWorkspaceSize);
            if(mod) {
                mod->mDataSize=info.mModuleBytes;mod->mWorkspaceSize=aWorkspaceSize;
                Memory_Copy(1084UL,header,mod->mData);
                if(File_Read(f,info.mModuleBytes-1084UL,mod->mData+1084)!=(S32)(info.mModuleBytes-1084UL)) {
                    mMEMFREE(mod);mod=0;
                }
            }
        }
    }
    File_Close(f);return mod;
}
sWizzcatModule *Wizzcat_Load(const char *apFileName){return Wizzcat_LoadEx(apFileName,0);}
void Wizzcat_Stop(void)
{
    if(sVblInstalled){Vbl_RemoveCall(Wizzcat_Vbl);sVblInstalled=0;}
    *(volatile U8 *)0xffff8901UL=0;
    WIZsetActive(0);
}
void Wizzcat_StopVbl(void){Wizzcat_Stop();}
void Wizzcat_UnLoad(sWizzcatModule *apModule)
{
    if(apModule) {
        if(apModule==spCurrent){Wizzcat_Stop();spCurrent=0;}
        mMEMFREE(apModule);
    }
}
static U8 Prepare(sWizzcatModule *mod)
{
    sWizzcatModuleInfo info;U8 *scratch,*silence=0;
    Wizzcat_Stop();spCurrent=0;
    if(!mod || mod->mPrepared ||
       (System_GetMCH()!=MCH_STE && System_GetMCH()!=MCH_MEGASTE) ||
       !Wizzcat_GetModuleInfo(mod->mData,mod->mDataSize,&info) ||
       mod->mWorkspaceSize<info.mWorkspaceBytes)return 0;
    scratch=mMEMALLOC(info.mInitScratchBytes);
    if(!scratch)return 0;
    if(!sTablesReady){WIZinit();sTablesReady=1;}
    if(info.mSilenceBytes)silence=mod->mData+info.mModuleBytes-info.mSampleBytes+info.mPreparedSampleBytes;
    WIZsetScratch(scratch+info.mSampleBytes,silence);
    WIZmodInit(mod->mData,mod->mData+mod->mDataSize+mod->mWorkspaceSize);
    WIZsetScratch(0,0);
    mMEMFREE(scratch);
    mod->mPrepared=1;spCurrent=mod;return 1;
}
void Wizzcat_Init(sWizzcatModule *apModule){Prepare(apModule);}
void Wizzcat_Play(void){if(spCurrent){WIZplay();WIZsetActive(1);}}
U8 Wizzcat_Start(sWizzcatModule *apModule)
{
    if(!Prepare(apModule))return 0;
    Wizzcat_Play();
    sVblInstalled=Vbl_AddCall(Wizzcat_Vbl);
    if(!sVblInstalled){Wizzcat_Stop();return 0;}
    return 1;
}
void Wizzcat_GetInfo(sWizzcatInfo *apInfo){if(apInfo){if(spCurrent)WIZgetInfo(apInfo);else{apInfo->mSongPos=0;apInfo->mPattPos=0;}}}
void Wizzcat_Jump(U8 aPos)
{
    if(spCurrent && aPos && aPos<=spCurrent->mData[950])WIZjump(aPos);
}
