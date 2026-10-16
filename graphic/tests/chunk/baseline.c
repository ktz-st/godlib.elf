/* Previous C kernel; only the local offset is widened for the new record ABI. */
#include <godlib/graphic/graphic.h>
void	BaselineRestore( sGraphicChunkList * apList, sGraphicCanvas * apSrc, sGraphicCanvas * apDst )
{
	S16 lCount;
	sGraphicChunk * lpChunk;
	U8 * lpS;
	U8 * lpD;
	U16	lAddSrc;
	U16 lAddDst;

	lpS = (U8*)apSrc->mpVRAM;
	lpD = (U8*)apDst->mpVRAM;

	lAddSrc = (U16)(apSrc->mpLineOffsets[ 1 ]>>1);
	lAddDst = (U16)(apSrc->mpLineOffsets[ 1 ]>>1);

	lCount = apList->mActiveCount;


	lpChunk = &apList->mChunks[ 0 ];
	while( lCount )
	{
		U16 * lpSrc;
		U16 * lpDst;
		U32 lOff;
		U16 lHeight;

		lOff = lpChunk->mOffset;
		lpSrc = (U16*)( lpS + lOff );
		lpDst = (U16*)( lpD + lOff );

		lHeight = lpChunk->mHeight;
		while( lHeight )
		{
			lpDst[ 0 ] = lpSrc[ 0 ];
			lpDst[ 1 ] = lpSrc[ 1 ];
			lpDst[ 2 ] = lpSrc[ 2 ];
			lpDst[ 3 ] = lpSrc[ 3 ];
/*
			lpDst[ 0 ] = 0xFFFF;
			lpDst[ 1 ] = 0xFFFF;
			lpDst[ 2 ] = 0xFFFF;
			lpDst[ 3 ] = 0xFFFF;
*/
			lpSrc += lAddSrc;
			lpDst += lAddDst;

			lHeight--;
		}

		lpChunk++;
		lCount--;
	}

	apList->mActiveCount = 0;
}


