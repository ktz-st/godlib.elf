#ifndef INCLUDED_PAULAOWNER_H
#define INCLUDED_PAULAOWNER_H
#include <godlib/base/base.h>
/* Internal: 1 LanceMod, 2 RJP/Lance, 3 LeonardMod, 4 RJP/Leonard, 5 TFMX/Lance, 6 TFMX/Leonard. */
U8 PaulaOwner_Claim(U8 owner);
void PaulaOwner_Release(U8 owner);
#endif
