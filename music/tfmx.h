#ifndef INCLUDED_TFMX_H
#define INCLUDED_TFMX_H
#include <godlib/base/base.h>
/* Original four-channel UGH! TFMX replay. PAL STE, supervisor APIs.
 * Start owns the VBL; never also Tick from the foreground. Trusted assets only. */
enum {eTFMX_MIXER_LANCE=0,eTFMX_MIXER_LEONARD=1};
enum {eTFMX_FREQ_12K=1,eTFMX_FREQ_25K=2};
typedef struct sTfmxSong sTfmxSong;
typedef struct sTfmxInfo {U32 mSourceBytes,mResidentBytes;U16 mSubSongs,mMixer;} sTfmxInfo;
sTfmxSong *Tfmx_Create(const void *mdat,U32 mdatBytes,const void *smpl,U32 smplBytes,U16 mixer);
sTfmxSong *Tfmx_Load(const char *mdat,const char *smpl,U16 mixer);
void Tfmx_UnLoad(sTfmxSong *song);
U8 Tfmx_GetInfo(const sTfmxSong *song,sTfmxInfo *info);
U8 Tfmx_Start(sTfmxSong *song,U16 subSong,U8 rate);
U8 Tfmx_SetSubSong(U16 subSong);
void Tfmx_Stop(void);
void Tfmx_Tick(void);
void Tfmx_GetVolumes(U16 *fourVolumes);
void Tfmx_SetRasterDebug(U8 enabled);
U32 Tfmx_GetAllocatedSize(void);
#endif
