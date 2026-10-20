#ifndef INCLUDED_PAULAMIXER_H
#define INCLUDED_PAULAMIXER_H
/* Internal: supervisor only, exclusive DMA/trap #0 owner: 1 LanceMod, 2 Rjp1, 5 TFMX.
 * Stop/remove the owner's VBL service before shutdown or reinitialization. */
#include <godlib/base/base.h>
U8 PaulaMixer_Init(U8 owner,U8 rate);
void PaulaMixer_Shutdown(U8 owner);
U8 PaulaMixer_GetOwner(void);
U32 PaulaMixer_GetWorkspaceSize(U8 rate);
U32 PaulaMixer_GetAllocatedSize(void);
void PaulaMixer_Stop(void);
void PaulaMixer_Tick(void);
extern U8 PaulaMixer_Channels[80];
#endif
