/* ################################################################################### */

#include	"amigaraw.h"

#include	<godlib/memory/memory.h>

/* ################################################################################### */

#define	dAMIGARAW_RGB444_TO_STE(_v)	(((U16)((_v) >> 1) & 0x777U) | ((U16)((_v) << 3) & 0x888U))

/* ################################################################################### */

U32	AmigaRaw_GetSize( void )
{
	return( dAMIGARAW_FILE_SIZE );
}


void	AmigaRaw_PaletteToST( const U16 * apAmigaPalette, U16 * apStPalette, U16 aColourCount )
{
	U16	i;

	if( !apAmigaPalette || !apStPalette )
	{
		return;
	}

	if( aColourCount > dAMIGARAW_COLOURS )
	{
		aColourCount = dAMIGARAW_COLOURS;
	}

	for( i=0; i<aColourCount; i++ )
	{
		apStPalette[ i ] = dAMIGARAW_RGB444_TO_STE( apAmigaPalette[ i ] );
	}
}


void	AmigaRaw_To4Plane( const sAmigaRaw * apRaw, U16 * apPixels )
{
	U16	x;
	U16	y;
	U32	lSrcOffset;
	U32	lDstOffset;

	if( !apRaw || !apPixels )
	{
		return;
	}

	for( y=0; y<dAMIGARAW_HEIGHT; y++ )
	{
		for( x=0; x<dAMIGARAW_WORDS_WIDE; x++ )
		{
			lSrcOffset  = (U32)y * dAMIGARAW_WORDS_WIDE;
			lSrcOffset += x;
			lDstOffset  = (U32)y * (dAMIGARAW_WIDTH >> 2);
			lDstOffset += (U32)x << 2;

			apPixels[ lDstOffset + 0 ] = apRaw->mPixels[ 0 ][ lSrcOffset ];
			apPixels[ lDstOffset + 1 ] = apRaw->mPixels[ 1 ][ lSrcOffset ];
			apPixels[ lDstOffset + 2 ] = apRaw->mPixels[ 2 ][ lSrcOffset ];
			apPixels[ lDstOffset + 3 ] = apRaw->mPixels[ 3 ][ lSrcOffset ];
		}
	}
}


sCanvas *	AmigaRaw_ToCanvas( const sAmigaRaw * apRaw )
{
	sCanvas *		lpCanvas;
	uCanvasPixel	lPalette[ dAMIGARAW_COLOURS ];
	U16				lPaletteST[ dAMIGARAW_COLOURS ];
	U16				lPlane0;
	U16				lPlane1;
	U16				lPlane2;
	U16				lPlane3;
	U16				lIndex;
	U16				x;
	U16				y;
	U16				w;
	U32				lOffset;

	if( !apRaw )
	{
		return( 0 );
	}

	AmigaRaw_PaletteToST( &apRaw->mPalette[ 0 ], &lPaletteST[ 0 ], dAMIGARAW_COLOURS );
	Canvas_PaletteFromST( &lPalette[ 0 ], dAMIGARAW_COLOURS, &lPaletteST[ 0 ] );

	lpCanvas = Canvas_Create();
	if( !lpCanvas )
	{
		return( 0 );
	}

	if( !Canvas_CreateImage( lpCanvas, dAMIGARAW_WIDTH, dAMIGARAW_HEIGHT ) )
	{
		Canvas_Destroy( lpCanvas );
		return( 0 );
	}

	for( y=0; y<dAMIGARAW_HEIGHT; y++ )
	{
		for( w=0; w<dAMIGARAW_WORDS_WIDE; w++ )
		{
			lOffset = ((U32)y * dAMIGARAW_WORDS_WIDE) + w;
			lPlane0 = apRaw->mPixels[ 0 ][ lOffset ];
			lPlane1 = apRaw->mPixels[ 1 ][ lOffset ];
			lPlane2 = apRaw->mPixels[ 2 ][ lOffset ];
			lPlane3 = apRaw->mPixels[ 3 ][ lOffset ];

			for( x=0; x<16; x++ )
			{
				lIndex  = (U16)((lPlane0 >> 15) & 1);
				lIndex |= (U16)(((lPlane1 >> 15) & 1) << 1);
				lIndex |= (U16)(((lPlane2 >> 15) & 1) << 2);
				lIndex |= (U16)(((lPlane3 >> 15) & 1) << 3);

				Canvas_SetPixel( lpCanvas, (U16)((w << 4) + x), y, lPalette[ lIndex ].l );

				lPlane0 <<= 1;
				lPlane1 <<= 1;
				lPlane2 <<= 1;
				lPlane3 <<= 1;
			}
		}
	}

	return( lpCanvas );
}
