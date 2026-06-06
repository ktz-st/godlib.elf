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

void	Font8x8_Print( const char * apString, U16 * apScreen, U16 aX, U16 aY )
{
	U32		lOffset;
	U16		lChar;
	U16		lNextX;
	U8 *	lpSrc;
	U8 *	lpScreen;

	lOffset   = aY;
	lOffset  *= 160L;
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

		lpScreen[ 0*160 ] = *lpSrc++;
		lpScreen[ 1*160 ] = *lpSrc++;
		lpScreen[ 2*160 ] = *lpSrc++;
		lpScreen[ 3*160 ] = *lpSrc++;
		lpScreen[ 4*160 ] = *lpSrc++;
		lpScreen[ 5*160 ] = *lpSrc++;
		lpScreen[ 6*160 ] = *lpSrc++;
		lpScreen[ 7*160 ] = *lpSrc++;

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

void	Font8x8_PrintColour( const char * apString, U16 * apScreen, U16 aX, U16 aY, U16 aColour )
{
	U32		lOffset;
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

	lOffset   = aY;
	lOffset  *= 160L;
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

		for( lRow=0; lRow<8*160; lRow+=160 )
		{
			lGlyph = *lpSrc++;
			lpScreen[ lRow + 0 ] = (U8)(lGlyph & lMask0);	/* plane 0 */
			lpScreen[ lRow + 2 ] = (U8)(lGlyph & lMask1);	/* plane 1 */
			lpScreen[ lRow + 4 ] = (U8)(lGlyph & lMask2);	/* plane 2 */
			lpScreen[ lRow + 6 ] = (U8)(lGlyph & lMask3);	/* plane 3 */
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

