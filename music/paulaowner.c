#include "paulaowner.h"
static U8 sOwner;
U8 PaulaOwner_Claim(U8 owner){if(sOwner && sOwner!=owner)return 0;sOwner=owner;return 1;}
void PaulaOwner_Release(U8 owner){if(sOwner==owner)sOwner=0;}
