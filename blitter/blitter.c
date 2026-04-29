/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: BLITTER.C
::
:: Blitter routines
::
:: [c] 2001 Reservoir Gods
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


/* ###################################################################################
#  INCLUDES
################################################################################### */

#include	"blitter.h"

#include	<godlib/cli/cli.h>
#include	<godlib/memory/memory.h>
#include	<godlib/system/system.h>


/* ###################################################################################
#  DEFINES
################################################################################### */

#define	dBLITTER_BASE_ADR				0xFFFF8A00L
#define	dBLITTER_ST_LOW_WIDTH			320
#define	dBLITTER_ST_LOW_HEIGHT			200
#define	dBLITTER_ST_LOW_LINE_BYTES		160
#define	dBLITTER_ST_LOW_LINE_WORDS		80

#define	dBLITTERSPRITEBLOCK_ID			mSTRING_TO_U32( 'B', 'S', 'B', 'K' )
#define	dBLITTERSPRITEBLOCK_VERSION		0


/* ###################################################################################
#  DATA
################################################################################### */

U16	gBlitterStartMasks[ 16 ] =
{
	0xFFFF,
	0x7FFF,
	0x3FFF,
	0x1FFF,
	0x0FFF,
	0x07FF,
	0x03FF,
	0x01FF,
	0x00FF,
	0x007F,
	0x003F,
	0x001F,
	0x000F,
	0x0007,
	0x0003,
	0x0001,
};

U16	gBlitterEndMasks[ 16 ] =
{
	0x8000,
	0xC000,
	0xE000,
	0xF000,
	0xF800,
	0xFC00,
	0xFE00,
	0xFF00,
	0xFF80,
	0xFFC0,
	0xFFE0,
	0xFFF0,
	0xFFF8,
	0xFFFC,
	0xFFFE,
	0xFFFF,
};

U8	gBlitterCopyTable[ 4 ] =
{
	dBLITTERSKEW_NFSR_BIT,
	dBLITTERSKEW_FXSR_BIT,
	0,
	dBLITTERSKEW_NFSR_BIT|dBLITTERSKEW_FXSR_BIT,
};

U8	gBlitterFlipTable[ 256 ];


/* ###################################################################################
#  CODE
################################################################################### */

static U8 Blitter_IsAvailable(void)
{
	return (U8)(BLT_BLITTER == System_GetBLT());
}


static U16 Blitter_CalcSourceYInc(const U16 aCountX, const U8 aSkew)
{
	U16 lSrcReads;

	lSrcReads = aCountX;
	if (aSkew & dBLITTERSKEW_FXSR_BIT)
		lSrcReads++;
	if ((aSkew & dBLITTERSKEW_NFSR_BIT) && lSrcReads)
		lSrcReads--;
	if (!lSrcReads)
		lSrcReads = 1;

	return (U16)(dBLITTER_ST_LOW_LINE_BYTES - ((lSrcReads - 1U) << 3));
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Blitter_Init( void )
* ACTION   : Blitter_Init
* CREATION : 04.01.2003 PNK
*-----------------------------------------------------------------------------------*/

void	Blitter_Init( void )
{
	U16	i,j;
	U16	lMask;
	U8	lByte;
	U8	lOr;

	for( i=0; i<256; i++ )
	{
		lByte = 0;
		lOr   = 0x80;
		lMask = 1;

		for( j=0; j<8; j++ )
		{
			if( i & lMask )
			{
				lByte |= lOr;
			}
			lOr   >>= 1;
			lMask <<= 1;
		}

		gBlitterFlipTable[ i ] = lByte;
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Blitter_DeInit( void )
* ACTION   : Blitter_DeInit
* CREATION : 04.01.2003 PNK
*-----------------------------------------------------------------------------------*/

void	Blitter_DeInit( void )
{
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Blitter_CopyBox( sBlitterSprite * apSprite, U16 * apScreen, U16 aSrcX, U16 aSrcY, U16 aDstX, U16 aDstY )
* ACTION   : draws a sprite on an st screen
* CREATION : 17.02.01 PNK
*-----------------------------------------------------------------------------------*/

void	Blitter_CopyBox( U16 * apSrc, U16 * apDst, U16 aSrcX, U16 aSrcY, U16 aDstX, U16 aDstY, U16 aWidth, U16 aHeight )
{
	volatile sBlitter *	lpBlitter;
	U16			lSrcX2;
	U16			lDstX2;
	U16			lEndMask1,lEndMask3;
	U16			lSrcSpan,lDstSpan;
	U16			lIndex;
	U16			i;
	U16			lCountX;
	U16	*		lpDst;
	U16	*		lpSrc;
	U8			lSkew;

	if (!Blitter_IsAvailable() || !apSrc || !apDst || !aWidth || !aHeight)
		return;
	if (aSrcX >= dBLITTER_ST_LOW_WIDTH || aDstX >= dBLITTER_ST_LOW_WIDTH || aSrcY >= dBLITTER_ST_LOW_HEIGHT || aDstY >= dBLITTER_ST_LOW_HEIGHT)
		return;
	if ((aSrcX + aWidth) > dBLITTER_ST_LOW_WIDTH)
		aWidth = (U16)(dBLITTER_ST_LOW_WIDTH - aSrcX);
	if ((aDstX + aWidth) > dBLITTER_ST_LOW_WIDTH)
		aWidth = (U16)(dBLITTER_ST_LOW_WIDTH - aDstX);
	if ((aSrcY + aHeight) > dBLITTER_ST_LOW_HEIGHT)
		aHeight = (U16)(dBLITTER_ST_LOW_HEIGHT - aSrcY);
	if ((aDstY + aHeight) > dBLITTER_ST_LOW_HEIGHT)
		aHeight = (U16)(dBLITTER_ST_LOW_HEIGHT - aDstY);
	if (!aWidth || !aHeight)
		return;

	lpBlitter = (sBlitter*)dBLITTER_BASE_ADR;

	while( lpBlitter->Mode & dBLITTERMODE_BUSY_BIT );

	lSrcX2    = (U16)(aSrcX + aWidth - 1);
	lDstX2    = (U16)(aDstX + aWidth - 1);

	lSrcSpan  = (U16)((lSrcX2>>4)-(aSrcX>>4));
	lDstSpan  = (U16)((lDstX2>>4)-(aDstX>>4));

	lEndMask1 = gBlitterStartMasks[ aDstX & 15 ];
	lEndMask3 = gBlitterEndMasks[ lDstX2 & 15 ];

	lSkew      = (U8)(((aDstX&15)-(aSrcX&15))&15);

	if( lDstSpan == 0 )
	{
		lEndMask1 &= lEndMask3;
		lEndMask3  = lEndMask1;
		if( lSrcSpan != 0 )
		{
			lSkew |= dBLITTERSKEW_FXSR_BIT;
		}
	}
	else
	{
		lIndex = 0;
		if( (aSrcX&15) > (aDstX&15) )
		{
			lIndex |= 1;
		}
		if( lSrcSpan == lDstSpan )
		{
			lIndex |= 2;
		}
		lSkew |= gBlitterCopyTable[ lIndex ];
	}

	lpBlitter->EndMask1 = lEndMask1;
	lpBlitter->EndMask2 = 0xFFFF;
	lpBlitter->EndMask3 = lEndMask3;

	lpBlitter->SrcIncX  = 8;
	lpBlitter->DstIncX  = 8;

	lCountX = (U16)(lDstSpan + 1U);
	lpBlitter->SrcIncY = Blitter_CalcSourceYInc(lCountX, lSkew);
	lpBlitter->DstIncY = (U16)(dBLITTER_ST_LOW_LINE_BYTES - (lDstSpan<<3));

	lpBlitter->CountX  = lCountX;

	lpBlitter->HOP = eBLITTERHOP_SRC;
	lpBlitter->LOP = eBLITTERLOP_SRC;

	lpSrc  = apSrc;
	lpSrc += aSrcY * dBLITTER_ST_LOW_LINE_WORDS;
	lpSrc += (aSrcX>>4)<<2;

	lpDst  = apDst;
	lpDst += aDstY * dBLITTER_ST_LOW_LINE_WORDS;
	lpDst += (aDstX>>4)<<2;

	lpBlitter->Skew = lSkew;
	for( i=0; i<4; i++ )
	{
		while( lpBlitter->Mode & dBLITTERMODE_BUSY_BIT );

		lpBlitter->pDst   = lpDst;
		lpBlitter->pSrc   = lpSrc;
		lpBlitter->CountY = aHeight;
		lpBlitter->Mode   = dBLITTERMODE_BUSY_BIT;

		lpSrc++;
		lpDst++;
	}

	while( lpBlitter->Mode & dBLITTERMODE_BUSY_BIT );
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Blitter_DrawSprite( sBlitterSprite * apSprite, U16 * apScreen, U16 aX, U16 aY )
* ACTION   : draws a sprite on an st screen
* CREATION : 17.02.01 PNK
*-----------------------------------------------------------------------------------*/
U16 gBlitterHack;
void	Blitter_DrawSprite( sBlitterSprite * apSprite, U16 * apScreen, S16 aX, S16 aY )
{
	volatile sBlitter *	lpBlitter;
	U16	*		lpDst;
	U16	*		lpDst2;
	U16	*		lpSrc;
	U16	*		lpMsk;
	U16			i;
	U16			lX2;
	U16			lXcount;
	U16			lWords;
	S16			lHeight;
/*
	if( apSprite->Width != 16 )
	{
		gBlitterHack = 1;
		return;
	}
*/
	if (!Blitter_IsAvailable() || !apSprite || !apScreen)
		return;
	if (aX < 0 || aX >= dBLITTER_ST_LOW_WIDTH || (aX + apSprite->Width) > dBLITTER_ST_LOW_WIDTH || aY >= dBLITTER_ST_LOW_HEIGHT)
		return;

	lpBlitter = (sBlitter*)dBLITTER_BASE_ADR;
	lpMsk     = apSprite->pMask;
	lpSrc     = apSprite->pGfx;
	lWords    = (U16)(apSprite->Width >> 4);

	lHeight = apSprite->Height;
	if( aY < 0 )
	{
		aY = (S16)-aY;
		lHeight = (S16)(lHeight - aY);
		if( lHeight <= 0 )
		{
			return;
		}
		lpMsk += (aY * lWords);
		lpSrc += (aY * apSprite->GfxPlaneCount  * lWords);
		aY = 0;
	}
	if ((aY + lHeight) > dBLITTER_ST_LOW_HEIGHT)
		lHeight = (S16)(dBLITTER_ST_LOW_HEIGHT - aY);
	if (lHeight <= 0)
		return;

	lpDst     = apScreen;
	lpDst    += aY * dBLITTER_ST_LOW_LINE_WORDS;
	lpDst    += (aX>>4)<<2;

	lX2       = (U16)((aX + apSprite->Width)-1);
	lXcount   = (U16)((lX2 & 0xFFF0) - (aX & 0xFFF0));
	lXcount >>= 4;
	lXcount  += 1;

	while( lpBlitter->Mode & dBLITTERMODE_BUSY_BIT );

	lpBlitter->CountX  = lXcount;
	lpBlitter->DstIncX = 8;
	lpBlitter->DstIncY = (U16)((dBLITTER_ST_LOW_LINE_BYTES + 8) - (lXcount<<3));

	lpBlitter->HOP  = eBLITTERHOP_SRC;

	if( aX & 15 )
	{
		lpBlitter->Skew = (U8)(dBLITTERSKEW_NFSR_BIT | (aX&15));
	}
	else
	{
		lpBlitter->Skew = 0;
	}

	if( lXcount > 2 )
	{
		lpBlitter->EndMask1 = gBlitterStartMasks[ aX & 15 ];
		lpBlitter->EndMask2 = 0xFFFF;
		lpBlitter->EndMask3 = gBlitterEndMasks[ lX2 & 15 ];
	}
	else if( lXcount > 1 )
	{
		lpBlitter->EndMask1 = gBlitterStartMasks[ aX & 15 ];
		lpBlitter->EndMask2 = gBlitterEndMasks[ lX2 & 15 ];
		lpBlitter->EndMask3 = gBlitterEndMasks[ lX2 & 15 ];
	}
	else
	{
		lpBlitter->EndMask1 = (U16)(gBlitterStartMasks[ aX & 15 ] & gBlitterEndMasks[ lX2 & 15 ]);
	}

	lpDst2          = lpDst;
	lpBlitter->LOP  = eBLITTERLOP_SRC_AND_DST;
	lpBlitter->SrcIncX = 2;
	lpBlitter->SrcIncY = 2;

	for( i=0; i<apSprite->MaskPlaneCount; i++ )
	{
		while( lpBlitter->Mode & dBLITTERMODE_BUSY_BIT );

		lpBlitter->pDst   = lpDst2;
		lpBlitter->pSrc   = lpMsk;
		lpBlitter->CountY = lHeight;
		lpBlitter->Mode   = dBLITTERMODE_BUSY_BIT;

		lpDst2++;
	}

	while( lpBlitter->Mode & dBLITTERMODE_BUSY_BIT );

	if( apSprite->MaskPlaneCount )
	{
		lpBlitter->LOP = eBLITTERLOP_SRC_OR_DST;
	}
	else
	{
		lpBlitter->LOP = eBLITTERLOP_SRC;
	}
	lpBlitter->SrcIncX = 8;
	lpBlitter->SrcIncY = 8;

	for( i=0; i<apSprite->GfxPlaneCount; i++ )
	{
		while( lpBlitter->Mode & dBLITTERMODE_BUSY_BIT );

		lpBlitter->pDst   = lpDst;
		lpBlitter->pSrc   = lpSrc;
		lpBlitter->CountY = lHeight;
		lpBlitter->Mode   = dBLITTERMODE_BUSY_BIT;

		lpSrc++;
		lpDst++;
	}

}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Blitter_DrawSprite( sBlitterSprite * apSprite, U16 * apScreen, U16 aX, U16 aY )
* ACTION   : draws a sprite on an st screen
* CREATION : 17.02.01 PNK
*-----------------------------------------------------------------------------------*/

void	Blitter_DrawOpaqueSprite( sBlitterSprite * apSprite, U16 * apScreen, S16 aX, S16 aY )
{
	volatile sBlitter *	lpBlitter;
	U16	*		lpDst;
	U16	*		lpSrc;
	U16	*		lpMsk;
	U16			i;
	U16			lX2;
	U16			lXcount;
	U16			lWords;
	S16			lHeight;

	if (!Blitter_IsAvailable() || !apSprite || !apScreen)
		return;
	if (aX < 0 || aX >= dBLITTER_ST_LOW_WIDTH || (aX + apSprite->Width) > dBLITTER_ST_LOW_WIDTH || aY >= dBLITTER_ST_LOW_HEIGHT)
		return;

	lpBlitter = (sBlitter*)dBLITTER_BASE_ADR;
	lpMsk     = apSprite->pMask;
	lpSrc     = apSprite->pGfx;
	lWords    = (U16)(apSprite->Width >> 4);

	lHeight = apSprite->Height;
	if( aY < 0 )
	{
		aY      = (S16)(-aY);
		lHeight = (S16)(lHeight -aY);
		if( lHeight <= 0 )
		{
			return;
		}
		lpMsk += (aY * lWords);
		lpSrc += (aY * apSprite->GfxPlaneCount  * lWords);
		aY = 0;
	}
	if ((aY + lHeight) > dBLITTER_ST_LOW_HEIGHT)
		lHeight = (S16)(dBLITTER_ST_LOW_HEIGHT - aY);
	if (lHeight <= 0)
		return;

	lpDst     = apScreen;
	lpDst    += aY * dBLITTER_ST_LOW_LINE_WORDS;
	lpDst    += (aX>>4)<<2;

	lX2       = (U16)((aX + apSprite->Width)-1);
	lXcount   = (U16)((lX2 & 0xFFF0) - (aX & 0xFFF0));
	lXcount >>= 4;
	lXcount  += 1;

	while( lpBlitter->Mode & dBLITTERMODE_BUSY_BIT );

	lpBlitter->CountX  = lXcount;
	lpBlitter->DstIncX = 8;
	lpBlitter->DstIncY = (U16)((dBLITTER_ST_LOW_LINE_BYTES + 8) - (lXcount<<3));

	lpBlitter->HOP  = eBLITTERHOP_SRC;

	if( aX & 15 )
	{
		lpBlitter->Skew = (U8)(dBLITTERSKEW_NFSR_BIT | (aX&15));
	}
	else
	{
		lpBlitter->Skew = 0;
	}

	if( lXcount > 2 )
	{
		lpBlitter->EndMask1 = gBlitterStartMasks[ aX & 15 ];
		lpBlitter->EndMask2 = 0xFFFF;
		lpBlitter->EndMask3 = gBlitterEndMasks[ lX2 & 15 ];
	}
	else if( lXcount > 1 )
	{
		lpBlitter->EndMask1 = gBlitterStartMasks[ aX & 15 ];
		lpBlitter->EndMask2 = gBlitterEndMasks[ lX2 & 15 ];
		lpBlitter->EndMask3 = gBlitterEndMasks[ lX2 & 15 ];
	}
	else
	{
		lpBlitter->EndMask1 = (U16)(gBlitterStartMasks[ aX & 15 ] & gBlitterEndMasks[ lX2 & 15 ]);
	}

	while( lpBlitter->Mode & dBLITTERMODE_BUSY_BIT );

	lpBlitter->LOP  = eBLITTERLOP_SRC;
	lpBlitter->SrcIncX = 8;
	lpBlitter->SrcIncY = 8;

	for( i=0; i<apSprite->GfxPlaneCount; i++ )
	{
		while( lpBlitter->Mode & dBLITTERMODE_BUSY_BIT );

		lpBlitter->pDst   = lpDst;
		lpBlitter->pSrc   = lpSrc;
		lpBlitter->CountY = lHeight;
		lpBlitter->Mode   = dBLITTERMODE_BUSY_BIT;

		lpSrc++;
		lpDst++;
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Blitter_DrawColouredSprite( sBlitterSprite * apSprite, U16 * apScreen, U16 aX, U16 aY, U8 aColour )
* ACTION   : draws a sprite on an st screen
* CREATION : 17.02.01 PNK
*-----------------------------------------------------------------------------------*/

void	Blitter_DrawColouredSprite( sBlitterSprite * apSprite, U16 * apScreen, S16 aX, S16 aY, U8 aColour )
{
	volatile sBlitter *	lpBlitter;
	U16	*		lpDst;
	U16	*		lpDst2;
	U16	*		lpSrc;
	U16	*		lpMsk;
	U16			i;
	U16			lX2;
	U16			lXcount;
	U16			lWords;
	S16			lHeight;

	if (!Blitter_IsAvailable() || !apSprite || !apScreen)
		return;
	if (aX < 0 || aX >= dBLITTER_ST_LOW_WIDTH || (aX + apSprite->Width) > dBLITTER_ST_LOW_WIDTH || aY >= dBLITTER_ST_LOW_HEIGHT)
		return;

	lpBlitter = (sBlitter*)dBLITTER_BASE_ADR;
	lpMsk     = apSprite->pMask;
	lpSrc     = apSprite->pGfx;
	lWords    = (U16)(apSprite->Width >> 4);

	lHeight = apSprite->Height;
	if( aY < 0 )
	{
		aY      = (S16)-aY;
		lHeight = (S16)(lHeight - aY);
		if( lHeight <= 0 )
		{
			return;
		}
		lpMsk += (aY * lWords);
		lpSrc += (aY * apSprite->GfxPlaneCount  * lWords);
		aY = 0;
	}
	if ((aY + lHeight) > dBLITTER_ST_LOW_HEIGHT)
		lHeight = (S16)(dBLITTER_ST_LOW_HEIGHT - aY);
	if (lHeight <= 0)
		return;

	lpDst     = apScreen;
	lpDst    += aY * dBLITTER_ST_LOW_LINE_WORDS;
	lpDst    += (aX>>4)<<2;

	lX2       = (U16)((aX + apSprite->Width)-1);
	lXcount   = (U16)((lX2 & 0xFFF0) - (aX & 0xFFF0));
	lXcount >>= 4;
	lXcount  += 1;

	while( lpBlitter->Mode & dBLITTERMODE_BUSY_BIT );

	lpBlitter->CountX  = lXcount;
	lpBlitter->DstIncX = 8;
	lpBlitter->DstIncY = (U16)((dBLITTER_ST_LOW_LINE_BYTES + 8) - (lXcount<<3));

	lpBlitter->HOP  = eBLITTERHOP_SRC;

	if( aX & 15 )
	{
		lpBlitter->Skew = (U8)(dBLITTERSKEW_NFSR_BIT | (aX&15));
	}
	else
	{
		lpBlitter->Skew = 0;
	}

	if( lXcount > 2 )
	{
		lpBlitter->EndMask1 = gBlitterStartMasks[ aX & 15 ];
		lpBlitter->EndMask2 = 0xFFFF;
		lpBlitter->EndMask3 = gBlitterEndMasks[ lX2 & 15 ];
	}
	else if( lXcount > 1 )
	{
		lpBlitter->EndMask1 = gBlitterStartMasks[ aX & 15 ];
		lpBlitter->EndMask2 = gBlitterEndMasks[ lX2 & 15 ];
		lpBlitter->EndMask3 = gBlitterEndMasks[ lX2 & 15 ];
	}
	else
	{
		lpBlitter->EndMask1 = (U16)(gBlitterStartMasks[ aX & 15 ] & gBlitterEndMasks[ lX2 & 15 ]);
	}

	lpDst2          = lpDst;
	lpBlitter->LOP  = eBLITTERLOP_SRC_AND_DST;
	lpBlitter->SrcIncX = 2;
	lpBlitter->SrcIncY = 2;

	for( i=0; i<apSprite->MaskPlaneCount; i++ )
	{
		while( lpBlitter->Mode & dBLITTERMODE_BUSY_BIT );

		lpBlitter->pDst   = lpDst2;
		lpBlitter->pSrc   = lpMsk;
		lpBlitter->CountY = lHeight;
		lpBlitter->Mode   = dBLITTERMODE_BUSY_BIT;

		lpDst2++;
	}

	while( lpBlitter->Mode & dBLITTERMODE_BUSY_BIT );

	lpBlitter->LOP  = eBLITTERLOP_SRC_OR_DST;
	lpBlitter->SrcIncX = 8;
	lpBlitter->SrcIncY = 8;

	for( i=0; i<apSprite->GfxPlaneCount; i++ )
	{
		if( aColour & (1<<i) )
		{
			while( lpBlitter->Mode & dBLITTERMODE_BUSY_BIT );

			lpBlitter->pDst   = lpDst;
			lpBlitter->pSrc   = lpSrc;
			lpBlitter->CountY = lHeight;
			lpBlitter->Mode   = dBLITTERMODE_BUSY_BIT;
		}
		lpDst++;
	}

}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Blitter_DrawBox( sBlitterBox * apBox, U16 * apScreen, U16 aX, U16 aY )
* ACTION   : draws a box on an standard st low screen
* CREATION : 17.02.01 PNK
*-----------------------------------------------------------------------------------*/

void	Blitter_DrawBox( sBlitterBox * apBox, U16 * apScreen, U16 aX, U16 aY )
{
	volatile sBlitter *	lpBlitter;
	U16	*		lpDst;
	U16			i;
	U16			lColour;
	U16			lX2;
	U16			lXcount;
	U16			lWidth;
	U16			lHeight;

	if (!Blitter_IsAvailable() || !apBox || !apScreen || !apBox->Width || !apBox->Height)
		return;
	if (aX >= dBLITTER_ST_LOW_WIDTH || aY >= dBLITTER_ST_LOW_HEIGHT)
		return;
	lWidth = apBox->Width;
	lHeight = apBox->Height;
	if ((aX + lWidth) > dBLITTER_ST_LOW_WIDTH)
		lWidth = (U16)(dBLITTER_ST_LOW_WIDTH - aX);
	if ((aY + lHeight) > dBLITTER_ST_LOW_HEIGHT)
		lHeight = (U16)(dBLITTER_ST_LOW_HEIGHT - aY);
	if (!lWidth || !lHeight)
		return;

	lColour   = apBox->Colour;
	lpBlitter = (sBlitter*)dBLITTER_BASE_ADR;

	lpDst     = apScreen;
	lpDst    += aY * dBLITTER_ST_LOW_LINE_WORDS;
	lpDst    += (aX>>4)<<2;

	lX2       = (U16)((aX + lWidth)-1);
	lXcount   = (U16)((lX2 & 0xFFF0) - (aX & 0xFFF0));
	lXcount >>= 4;
	lXcount  += 1;

	while( lpBlitter->Mode & dBLITTERMODE_BUSY_BIT );

	lpBlitter->CountX  = lXcount;
	lpBlitter->DstIncX = 8;
	lpBlitter->DstIncY = (U16)((dBLITTER_ST_LOW_LINE_BYTES + 8) - (lXcount<<3));

	lpBlitter->Skew = 0;
	lpBlitter->HOP  = eBLITTERHOP_SRC;

	if( lXcount > 2 )
	{
		lpBlitter->EndMask1 = gBlitterStartMasks[ aX & 15 ];
		lpBlitter->EndMask2 = 0xFFFF;
		lpBlitter->EndMask3 = gBlitterEndMasks[ lX2 & 15 ];
	}
	else if( lXcount > 1 )
	{
		lpBlitter->EndMask1 = gBlitterStartMasks[ aX & 15 ];
		lpBlitter->EndMask2 = gBlitterEndMasks[ lX2 & 15 ];
		lpBlitter->EndMask3 = gBlitterEndMasks[ lX2 & 15 ];
	}
	else
	{
		lpBlitter->EndMask1 = (U16)(gBlitterStartMasks[ aX & 15 ] & gBlitterEndMasks[ lX2 & 15 ]);
	}

	for( i=0; i<4; i++ )
	{
		while( lpBlitter->Mode & dBLITTERMODE_BUSY_BIT );

		if( lColour & 1 )
		{
			lpBlitter->LOP = eBLITTERLOP_ONE;
		}
		else
		{
			lpBlitter->LOP = eBLITTERLOP_ZERO;
		}

		lpBlitter->pDst   = lpDst;
		lpBlitter->CountY = lHeight;
		lpBlitter->Mode   = dBLITTERMODE_BUSY_BIT;


		lpDst++;
		lColour >>= 1;
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Blitter_Wait( void )
* ACTION   : Blitter_Wait
* CREATION : 04.01.2003 PNK
*-----------------------------------------------------------------------------------*/

void	Blitter_Wait( void )
{
	volatile sBlitter *	lpBlitter;

	if (!Blitter_IsAvailable())
		return;

	lpBlitter = (sBlitter*)dBLITTER_BASE_ADR;

	while( lpBlitter->Mode & dBLITTERMODE_BUSY_BIT );
}


/* ################################################################################ */
