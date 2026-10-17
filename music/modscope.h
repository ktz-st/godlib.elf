#ifndef INCLUDED_MODSCOPE_H
#define INCLUDED_MODSCOPE_H
#include <godlib/base/base.h>
#define dMODSCOPE_CHANNELS 4
#define dMODSCOPE_POINTS 64
#define dMODSCOPE_XY_POINTS 100
#define dMODSCOPE_XY_DELAY 64
#define dMODSCOPE_STEREO_BYTES ((dMODSCOPE_XY_POINTS+dMODSCOPE_XY_DELAY)*2)
#define dMODSCOPE_BYTES (dMODSCOPE_CHANNELS*dMODSCOPE_POINTS)
/* Internal replay snapshot, copied with interrupts masked; never retained.
 * Each output trace spans 64 points at a nominal 6258 samples/second.
 * This is a read-only source preview with current pitch/volume, not DMA capture. */
typedef struct sModScopeVoice
{
    const S8 *mpCurrent,*mpEnd,*mpLoop,*mpLoopEnd;
    U16 mPeriod,mVolume,mFormat,mEnabled; /* format 1: Lance signed 7-bit */
} sModScopeVoice;
/* aClockStep = Paula clock * 65536 / 6258; four voices, 256 output S8 bytes. */
void ModScope_Sample(const sModScopeVoice *apVoices,S8 *apSamples,U32 aClockStep);
#endif
