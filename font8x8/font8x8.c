/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: FONT8X8.C
::
:: Font printing routines
::
:: [c] 2000 Reservoir Gods
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


/* ###################################################################################
#  INCLUDES
################################################################################### */

#include	"font8x8.h"
#include <godlib/screen/screen.h>


/* ###################################################################################
#  PROTOTYPES
################################################################################### */

extern	U8 	gFont8x8[12544];


/* ###################################################################################
#  FUNCTIONS
################################################################################### */

/*-----------------------------------------------------------------------------------*
* FUNCTION : Font8x8_Print( const char * apString, U16 * apScreen, U16 aX, U16 aY )
* ACTION   : prints string apString on screen apScreen at aX,aY
* CREATION : 16.01.01 PNK
*-----------------------------------------------------------------------------------*/

static void Font8x8_PrintInternal( const char * apString, U16 * apScreen, U16 aX, U16 aY, const U32 * apLines )
{
	U32		lOffset;
	U32		lRows[8];
	U16		lIndex;
	U16		lChar;
	U16		lNextX;
	U8 *	lpSrc;
	U8 *	lpScreen;

	lOffset = apLines ? apLines[aY] : (U32)aY * 160UL;
	for( lIndex=0; lIndex<8; ++lIndex )
		lRows[lIndex] = apLines ? apLines[aY+lIndex] - lOffset : (U32)lIndex * 160UL;
	lOffset  += (aX>>4)<<3;
	lpScreen  = (U8*)apScreen;
	lpScreen  = &lpScreen[ lOffset ];

	lNextX = (U16)(aX & 8);
	if( lNextX )
	{
		lpScreen++;
	}

	while( *apString )
	{
		lChar   = (U16)((*apString++ - 32) & 0xFF);
		lChar <<=3;
		lpSrc   = &gFont8x8[ lChar ];

		lpScreen[ lRows[0] ] = *lpSrc++;
		lpScreen[ lRows[1] ] = *lpSrc++;
		lpScreen[ lRows[2] ] = *lpSrc++;
		lpScreen[ lRows[3] ] = *lpSrc++;
		lpScreen[ lRows[4] ] = *lpSrc++;
		lpScreen[ lRows[5] ] = *lpSrc++;
		lpScreen[ lRows[6] ] = *lpSrc++;
		lpScreen[ lRows[7] ] = *lpSrc++;

		if( lNextX )
		{
			lpScreen += 7;
			lNextX    = 0;
		}
		else
		{
			lpScreen++;
			lNextX =1;
		}

	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Font8x8_PrintColour( const char * apString, U16 * apScreen, U16 aX, U16 aY, U16 aColour )
* ACTION   : prints apString at aX,aY in colour index aColour (0-15, ST low-res 4 plane)
* CREATION : 15.05.26 PNK
*-----------------------------------------------------------------------------------*/

static void Font8x8_PrintColourInternal( const char * apString, U16 * apScreen, U16 aX, U16 aY, U16 aColour, const U32 * apLines )
{
	U32		lOffset;
	U32		lRows[8];
	U16		lIndex;
	U16		lChar;
	U16		lNextX;
	U16		lRow;
	U8		lGlyph;
	U8		lMask0;
	U8		lMask1;
	U8		lMask2;
	U8		lMask3;
	U8 *	lpSrc;
	U8 *	lpScreen;

	/* glyph pixels take colour aColour, rest of the 8x8 cell becomes colour 0 */
	lMask0 = (U8)((aColour & 1) ? 0xFF : 0x00);
	lMask1 = (U8)((aColour & 2) ? 0xFF : 0x00);
	lMask2 = (U8)((aColour & 4) ? 0xFF : 0x00);
	lMask3 = (U8)((aColour & 8) ? 0xFF : 0x00);

	lOffset = apLines ? apLines[aY] : (U32)aY * 160UL;
	for( lIndex=0; lIndex<8; ++lIndex )
		lRows[lIndex] = apLines ? apLines[aY+lIndex] - lOffset : (U32)lIndex * 160UL;
	lOffset  += (aX>>4)<<3;
	lpScreen  = (U8*)apScreen;
	lpScreen  = &lpScreen[ lOffset ];

	lNextX = (U16)(aX & 8);
	if( lNextX )
	{
		lpScreen++;
	}

	while( *apString )
	{
		lChar   = (U16)((*apString++ - 32) & 0xFF);
		lChar <<=3;
		lpSrc   = &gFont8x8[ lChar ];

		for( lRow=0; lRow<8; ++lRow )
		{
			lGlyph = *lpSrc++;
			lpScreen[ lRows[lRow] + 0 ] = (U8)(lGlyph & lMask0);	/* plane 0 */
			lpScreen[ lRows[lRow] + 2 ] = (U8)(lGlyph & lMask1);	/* plane 1 */
			lpScreen[ lRows[lRow] + 4 ] = (U8)(lGlyph & lMask2);	/* plane 2 */
			lpScreen[ lRows[lRow] + 6 ] = (U8)(lGlyph & lMask3);	/* plane 3 */
		}

		if( lNextX )
		{
			lpScreen += 7;
			lNextX    = 0;
		}
		else
		{
			lpScreen++;
			lNextX =1;
		}

	}
}


/* Legacy pointers to active Screen pages inherit their virtual line layout.
 * Other raw buffers retain the original 320-pixel / 160-byte layout. */
static const sGraphicCanvas * Font8x8_ScreenCanvas( const U16 * apScreen )
{
	const sGraphicCanvas * lpCanvases[3] =
		{ &gScreenLogicGraphic, &gScreenPhysicGraphic, &gScreenBackGraphic };
	U16 i;
	if( !gScreenClass.mpMemBase ) return 0;
	for( i=0; i<3; ++i )
		if( apScreen == lpCanvases[i]->mpVRAM &&
			lpCanvases[i]->mColourMode == eGRAPHIC_COLOURMODE_4PLANE &&
			lpCanvases[i]->mpLineOffsets ) return lpCanvases[i];
	return 0;
}

void Font8x8_Print( const char * apString, U16 * apScreen, U16 aX, U16 aY )
{
	const sGraphicCanvas * lpCanvas = Font8x8_ScreenCanvas( apScreen );
	Font8x8_PrintInternal( apString, apScreen, aX, aY,
		lpCanvas ? lpCanvas->mpLineOffsets : 0 );
}

void Font8x8_PrintColour( const char * apString, U16 * apScreen, U16 aX, U16 aY, U16 aColour )
{
	const sGraphicCanvas * lpCanvas = Font8x8_ScreenCanvas( apScreen );
	Font8x8_PrintColourInternal( apString, apScreen, aX, aY, aColour,
		lpCanvas ? lpCanvas->mpLineOffsets : 0 );
}

void Font8x8_PrintCanvas( const char * apString, sGraphicCanvas * apCanvas, U16 aX, U16 aY )
{
	if( !apCanvas || !apCanvas->mpVRAM || !apCanvas->mpLineOffsets ||
		apCanvas->mColourMode != eGRAPHIC_COLOURMODE_4PLANE ) return;
	Font8x8_PrintInternal( apString, (U16*)apCanvas->mpVRAM, aX, aY, apCanvas->mpLineOffsets );
}

void Font8x8_PrintColourCanvas( const char * apString, sGraphicCanvas * apCanvas, U16 aX, U16 aY, U16 aColour )
{
	if( !apCanvas || !apCanvas->mpVRAM || !apCanvas->mpLineOffsets ||
		apCanvas->mColourMode != eGRAPHIC_COLOURMODE_4PLANE ) return;
	Font8x8_PrintColourInternal( apString, (U16*)apCanvas->mpVRAM, aX, aY, aColour, apCanvas->mpLineOffsets );
}
