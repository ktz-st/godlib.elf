#ifndef INCLUDED_LEONARDMIXER_H
#define INCLUDED_LEONARDMIXER_H
#include <godlib/base/base.h>
/* Internal exclusive backend: owner 1 LeonardMod, 2 RJP1, 4 TFMX. Rates 1/2. */
U8 LeonardMixer_Init(U8 owner,U8 rate);
void LeonardMixer_Shutdown(U8 owner);
U8 LeonardMixer_GetOwner(void);
U32 LeonardMixer_GetWorkspaceSize(U8 rate);
U32 LeonardMixer_GetAllocatedSize(void);
void LeonardMixer_Tick(void);
void LeonardMixer_Stop(void);
void LeonardMixer_SetUserRoutine(void (*routine)(void));
#endif
