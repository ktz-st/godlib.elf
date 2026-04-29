/* ###################################################################################
#  INCLUDES
################################################################################### */

#include "brun1.h"


/* ###################################################################################
#  CODE
################################################################################### */

U32 ByteRun1_DePack(const void * apSrc, void * apDst, const U32 aDstSize)
{
	const U8 * lpSrc;
	U8 * lpDst;
	U32 lLeft;

	lpSrc = (const U8 *)apSrc;
	lpDst = (U8 *)apDst;
	lLeft = aDstSize;

	while (lLeft)
	{
		U16 lChunk;

		lChunk = (lLeft > 0xFFFFUL) ? 0xFFFFU : (U16)lLeft;
		lpSrc = ByteRun1_DePackLine(lpSrc, lpDst, lChunk);
		lpDst += lChunk;
		lLeft -= lChunk;
	}
	return aDstSize;
}


/* ################################################################################ */
