#ifndef INCLUDED_GRAPHIC_REFLECTION_H
#define INCLUDED_GRAPHIC_REFLECTION_H
#include <godlib/base/base.h>

/* Missing Link REFLECT: even, 320x200 four-plane ST-low buffers, stride 160.
   Source end is exclusive; coordinates are clamped to 0..199 as in STOS.
   Initialise mToggle=mPhase=0. Each call advances the effect (40-call cycle).
   Output starts at mDestY and is clipped at row 199. At least eight source
   rows are required. Buffers may alias, provided writes do not destroy rows
   that will still be read. Some ranges read one row before Y1: see
   docs/reflection.md for source guard requirements. State belongs to an effect, not a screen page. */
typedef struct sGraphicReflect
{
    const void *mpSource;
    void *mpDest;
    S16 mSourceY1, mSourceY2, mDestY;
    U16 mToggle, mPhase; /* internal: toggle 0/1, byte phase 0,2,...,38 */
} sGraphicReflect;

void Graphic_Reflect( sGraphicReflect * apReflect );
#endif
