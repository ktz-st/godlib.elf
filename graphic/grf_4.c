#ifndef	INCLUDED_GRF_4_H
#define	INCLUDED_GRF_4_H

/* ###################################################################################
#  INCLUDES
################################################################################### */

#include	<godlib/base/base.h>
#include	<godlib/memory/memory.h>
#include	<godlib/graphic/graphic.h>
#include	<godlib/sprite/sprite.h>


/* ###################################################################################
#  PROTOTYPES
################################################################################### */

void	Graphic_4BP_DrawSpritePartial( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,void * apSprite, sGraphicRect * apRect );
U8		Graphic_4BP_GetPixel( const sGraphicCanvas * apCanvas, const S16 aX, const S16 aY );
void	Graphic_4BP_SetPixel( const sGraphicCanvas * apCanvas, const S16 aX, const S16 aY, const U8 aPixel );
U8		Graphic_4BP_Sprite_IsMasked( const sSprite * apSprite, const S16 aX, const S16 aY );
U8		Graphic_4BP_Sprite_GetPixel( const sSprite * apSprite, const S16 aX, const S16 aY );


/* ###################################################################################
#  PROTOTYPES
################################################################################### */

/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_Blit( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,sGraphicRect * apRect,struct sGraphicCanvas * apSrc )
* ACTION   : Graphic_4BP_Blit
* CREATION : 05.04.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_Blit( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,sGraphicRect * apRect,struct sGraphicCanvas * apSrc )
{
	U16 *	lpSrc0;
	U16 *	lpSrc1;
	U16 *	lpDst0;
	U16 *	lpDst1;
	U16		i;
	U16		lPixels;
	U16		lGfx;
	U16		lX1;
	U16		lWordCount;
	U16		lWord;
	U16		lW,lH;
	U16		lMaskLeft;
	U16		lMaskRight;
	U16		lMask;
	U32		lSrcLineWords;
	U32		lDstLineWords;

	if( ((apCoords->mX ^ apRect->mX) & 15) == 0 )
	{
		lpDst0  =  (U16*)apCanvas->mpVRAM;
		lpDst0 += (apCanvas->mpLineOffsets[ apCoords->mY ]>>1);
		lpDst0 +=((apCoords->mX>>4)<<2);

		lpSrc0  =  (U16*)apSrc->mpVRAM;
		lpSrc0 += (apSrc->mpLineOffsets[ apRect->mY ]>>1);
		lpSrc0 +=((apRect->mX>>4)<<2);

		lX1         = (U16)(apCoords->mX + apRect->mWidth - 1);
		lWordCount  = (U16)((lX1>>4) - (apCoords->mX>>4) + 1);
		lMaskLeft   = (U16)(0xFFFF >> (apCoords->mX & 15));
		lMaskRight  = (U16)(~(0xFFFF >> ((lX1 & 15)+1)));
		lSrcLineWords = apSrc->mpLineOffsets[ 1 ] >> 1;
		lDstLineWords = apCanvas->mpLineOffsets[ 1 ] >> 1;

		if( lWordCount == 1 )
		{
			lMaskLeft &= lMaskRight;
		}

		lH = apRect->mHeight;
		while( lH-- )
		{
			lpSrc1 = lpSrc0;
			lpDst1 = lpDst0;

			for( lWord=0; lWord<lWordCount; lWord++ )
			{
				if( lWord == 0 )
				{
					lMask = lMaskLeft;
				}
				else if( lWord == (U16)(lWordCount-1) )
				{
					lMask = lMaskRight;
				}
				else
				{
					lMask = 0xFFFF;
				}

				if( lMask == 0xFFFF )
				{
					lpDst1[ 0 ] = lpSrc1[ 0 ];
					lpDst1[ 1 ] = lpSrc1[ 1 ];
					lpDst1[ 2 ] = lpSrc1[ 2 ];
					lpDst1[ 3 ] = lpSrc1[ 3 ];
				}
				else
				{
					for( i=0; i<4; i++ )
					{
						Endian_ReadBigU16( &lpDst1[ i ], lPixels );
						Endian_ReadBigU16( &lpSrc1[ i ], lGfx );
						lPixels &= (U16)~lMask;
						lPixels |= (U16)(lGfx & lMask);
						Endian_WriteBigU16( &lpDst1[ i ], lPixels );
					}
				}

				lpSrc1 += 4;
				lpDst1 += 4;
			}

			lpSrc0 += lSrcLineWords;
			lpDst0 += lDstLineWords;
		}

		return;
	}

	S16	lSX,lSY;
	S16	lDX,lDY;


	lSY = apRect->mY;
	lDY = apCoords->mY;
	lH  = apRect->mHeight;

	while( lH > 0 )
	{
		lW  = apRect->mWidth;
		lSX = apRect->mX;
		lDX = apCoords->mX;
		while( lW > 0 )
		{
			Graphic_4BP_SetPixel( apCanvas, lDX, lDY, Graphic_4BP_GetPixel( apSrc, lSX, lSY ) );
			lSX++;
			lDX++;
			lW--;
		}
		lSY++;
		lDY++;
		lH--;
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_ClearScreen( struct sGraphicCanvas * apCanvas )
* ACTION   : Graphic_4BP_ClearScreen
* CREATION : 05.04.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_ClearScreen( struct sGraphicCanvas * apCanvas )
{
	Memory_Clear( apCanvas->mpLineOffsets[ apCanvas->mHeight ], apCanvas->mpVRAM );
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_CopyScreen( struct sGraphicCanvas * apCanvas,void * apSrc )
* ACTION   : Graphic_4BP_CopyScreen
* CREATION : 05.04.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_CopyScreen( struct sGraphicCanvas * apCanvas,void * apSrc )
{
	Memory_Copy( apCanvas->mpLineOffsets[ apCanvas->mHeight ], apSrc, apCanvas->mpVRAM );
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_DrawBox( struct sGraphicCanvas * apCanvas,sGraphicRect * apCoords,S16 aColour )
* ACTION   : Graphic_4BP_DrawBox
* CREATION : 05.04.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_DrawBox( struct sGraphicCanvas * apCanvas,sGraphicRect * apCoords,S16 aColour )
{
	U16 *	lpDst0;
	U16 *	lpDst1;
	U16		lMaskLeft;
	U16		lMaskRight;
	U16		lPixels;
	U16		lW,lH;
	U16		lX0,lX1;
	S16		lChunks;
	U16		lMid[ 4 ];

	lpDst0  =  (U16*)apCanvas->mpVRAM;
	lpDst0 += (apCanvas->mpLineOffsets[ apCoords->mY ]>>1);
	lpDst0 +=((apCoords->mX>>4)<<2);

	if( aColour & 1 )
	{
		lMid[ 0 ] = 0xFFFF;
	}
	else
	{
		lMid[ 0 ] = 0;
	}
	if( aColour & 2 )
	{
		lMid[ 1 ] = 0xFFFF;
	}
	else
	{
		lMid[ 1 ] = 0;
	}
	if( aColour & 4 )
	{
		lMid[ 2 ] = 0xFFFF;
	}
	else
	{
		lMid[ 2 ] = 0;
	}
	if( aColour & 8 )
	{
		lMid[ 3 ] = 0xFFFF;
	}
	else
	{
		lMid[ 3 ] = 0;
	}

	lH = apCoords->mHeight;

	lX0 =  apCoords->mX;;
	lX1 = (U16)(apCoords->mX+apCoords->mWidth-1);

	lMaskLeft  =(U16)(  0xFFFF >> (lX0 & 15));
	lMaskRight =(S16)(~(0xFFFF >> ((lX1 & 15)+1)));

	lX0 >>= 4;
	lX1 >>= 4;

	lChunks = (S16)(lX1-lX0);

	if( lMaskRight )
	{
		lChunks--;
	}

	if( lChunks < 0 )
	{
		lMaskLeft &= lMaskRight;
		lChunks    = 0;
		lMaskRight = 0;
	}

	while( lH-- )
	{
		lpDst1 = lpDst0;

		Endian_ReadBigU16( &lpDst1[ 0 ], lPixels );
		lPixels &= ~lMaskLeft;
		lPixels |= lMid[ 0 ] & lMaskLeft;
		Endian_WriteBigU16( &lpDst1[ 0 ], lPixels );

		Endian_ReadBigU16( &lpDst1[ 1 ], lPixels );
		lPixels &= ~lMaskLeft;
		lPixels |= lMid[ 1 ] & lMaskLeft;
		Endian_WriteBigU16( &lpDst1[ 1 ], lPixels );

		Endian_ReadBigU16( &lpDst1[ 2 ], lPixels );
		lPixels &= ~lMaskLeft;
		lPixels |= lMid[ 2 ] & lMaskLeft;
		Endian_WriteBigU16( &lpDst1[ 2 ], lPixels );

		Endian_ReadBigU16( &lpDst1[ 3 ], lPixels );
		lPixels &= ~lMaskLeft;
		lPixels |= lMid[ 3 ] & lMaskLeft;
		Endian_WriteBigU16( &lpDst1[ 3 ], lPixels );
		lpDst1 += 4;

		lW = lChunks;
		while( lW-- )
		{
			Endian_WriteBigU16( lpDst1, lMid[ 0 ] );
			lpDst1++;
			Endian_WriteBigU16( lpDst1, lMid[ 1 ] );
			lpDst1++;
			Endian_WriteBigU16( lpDst1, lMid[ 2 ] );
			lpDst1++;
			Endian_WriteBigU16( lpDst1, lMid[ 3 ] );
			lpDst1++;
		}

		if( lMaskRight )
		{
			Endian_ReadBigU16( &lpDst1[ 0 ], lPixels );
			lPixels &= ~lMaskRight;
			lPixels |= lMid[ 0 ] & lMaskRight;
			Endian_WriteBigU16( &lpDst1[ 0 ], lPixels );

			Endian_ReadBigU16( &lpDst1[ 1 ], lPixels );
			lPixels &= ~lMaskRight;
			lPixels |= lMid[ 1 ] & lMaskRight;
			Endian_WriteBigU16( &lpDst1[ 1 ], lPixels );

			Endian_ReadBigU16( &lpDst1[ 2 ], lPixels );
			lPixels &= ~lMaskRight;
			lPixels |= lMid[ 2 ] & lMaskRight;
			Endian_WriteBigU16( &lpDst1[ 2 ], lPixels );

			Endian_ReadBigU16( &lpDst1[ 3 ], lPixels );
			lPixels &= ~lMaskRight;
			lPixels |= lMid[ 3 ] & lMaskRight;
			Endian_WriteBigU16( &lpDst1[ 3 ], lPixels );
		}

		lpDst0 += apCanvas->mpLineOffsets[ 1 ]>>1;
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_DrawLine( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,S16 aColour )
* ACTION   : Graphic_4BP_DrawLine
* CREATION : 05.04.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_DrawLine( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,S16 aColour )
{
	(void)apCanvas;
	(void)apCoords;
	(void)aColour;
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_DrawPixel( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,S16 aColour )
* ACTION   : Graphic_4BP_DrawPixel
* CREATION : 05.04.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_DrawPixel( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,S16 aColour )
{
	U16 *	lpDst;
	U16		lMask;
	U16		lPixel;
	U16		lTemp;

	lpDst  =  (U16*)apCanvas->mpVRAM;
	lpDst += (apCanvas->mpLineOffsets[ apCoords->mY ]>>1);
	lpDst +=((apCoords->mX>>4)<<2);


	lPixel = (U16)(0x8000 >> (apCoords->mX&15));
	lMask  = (U16)(~lPixel);

	Endian_ReadBigU16( &lpDst[ 0 ], lTemp );
	lTemp &= lMask;
	Endian_WriteBigU16( &lpDst[ 0 ], lTemp );

	Endian_ReadBigU16( &lpDst[ 1 ], lTemp );
	lTemp &= lMask;
	Endian_WriteBigU16( &lpDst[ 1 ], lTemp );

	Endian_ReadBigU16( &lpDst[ 2 ], lTemp );
	lTemp &= lMask;
	Endian_WriteBigU16( &lpDst[ 2 ], lTemp );

	Endian_ReadBigU16( &lpDst[ 3 ], lTemp );
	lTemp &= lMask;
	Endian_WriteBigU16( &lpDst[ 3 ], lTemp );


	if( aColour & 8 )
	{
		Endian_ReadBigU16( &lpDst[ 0 ], lTemp );
		lTemp |= lPixel;
		Endian_WriteBigU16( &lpDst[ 0 ], lTemp );
	}
	if( aColour & 4 )
	{
		Endian_ReadBigU16( &lpDst[ 1 ], lTemp );
		lTemp |= lPixel;
		Endian_WriteBigU16( &lpDst[ 1 ], lTemp );
	}
	if( aColour & 2 )
	{
		Endian_ReadBigU16( &lpDst[ 2 ], lTemp );
		lTemp |= lPixel;
		Endian_WriteBigU16( &lpDst[ 2 ], lTemp );
	}
	if( aColour & 1 )
	{
		Endian_ReadBigU16( &lpDst[ 3 ], lTemp );
		lTemp |= lPixel;
		Endian_WriteBigU16( &lpDst[ 3 ], lTemp );
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_DrawSprite( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,void * apSprite )
* ACTION   : Graphic_4BP_DrawSprite
* CREATION : 05.04.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_DrawSprite( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,void * apSprite )
{
	U16 *		lpDst0;
	U16 *		lpDst1;
	U16 *		lpMask;
	U16 *		lpGfx;
	U16			lGfx;
	U16			lPixels;
	U16			lMask;
	U16			lMask0;
	U16			lMask1;
	U16			i;
	U16			lH,lW;
	U16			lChunks;
	U16			lShiftL,lShiftR;
	sSprite *	lpSprite;

	lpDst0  =  (U16*)apCanvas->mpVRAM;
	lpDst0 += (apCanvas->mpLineOffsets[ apCoords->mY ]>>1);
	lpDst0 +=((apCoords->mX>>4)<<2);

	lpSprite = (sSprite*)apSprite;

	lH      = lpSprite->mHeight;
	lChunks =(U16)((lpSprite->mWidth)>>4);
	lpMask  = lpSprite->mpMask;
	lpGfx   = lpSprite->mpGfx;
	lShiftR = (U16)(apCoords->mX & 15);
	lShiftL = (U16)(16 - lShiftR);

	if( !lShiftR )
	{
		while( lH-- )
		{
			lW     = lChunks;
			lpDst1 = lpDst0;
			while( lW-- )
			{
				Endian_ReadBigU16( lpMask, lMask );
				lpMask++;
				for( i=0; i<lpSprite->mMaskPlaneCount; i++ )
				{
					Endian_ReadBigU16( &lpDst1[ i ], lPixels );
					lPixels &= lMask;
					Endian_WriteBigU16( &lpDst1[ i ], lPixels );
				}
				for( i=0; i<lpSprite->mGfxPlaneCount; i++ )
				{
					Endian_ReadBigU16( lpDst1, lPixels );
					Endian_ReadBigU16( lpGfx, lGfx );
					lPixels |= lGfx;
					Endian_WriteBigU16( lpDst1, lPixels );
					lpDst1++;
					lpGfx++;
				}
			}
			lpDst0 += apCanvas->mpLineOffsets[ 1 ] >> 1;
		}
	}
	else
	{
		lMask0 = 0xFFFF;
		lMask1 = 0xFFFF;

		lMask0 <<= lShiftL;
		lMask1 >>= lShiftR;
		while( lH-- )
		{
			lW     = lChunks;
			lpDst1 = lpDst0;
			while( lW-- )
			{
				Endian_ReadBigU16( lpMask, lMask );
				lMask >>= lShiftR;
				lMask  |= lMask0;
				for( i=0; i<lpSprite->mMaskPlaneCount; i++ )
				{
					Endian_ReadBigU16( &lpDst1[ i ], lPixels );
					lPixels &= lMask;
					Endian_WriteBigU16( &lpDst1[ i ], lPixels );
				}
				for( i=0; i<lpSprite->mGfxPlaneCount; i++ )
				{
					Endian_ReadBigU16(  lpDst1,     lPixels );
					Endian_ReadBigU16( &lpGfx[ i ], lGfx );
					lPixels |= lGfx>>lShiftR;
					Endian_WriteBigU16( lpDst1, lPixels );
					lpDst1++;
				}

				Endian_ReadBigU16( lpMask, lMask );
				lpMask++;
				lMask <<= lShiftL;
				lMask  |= lMask1;
				for( i=0; i<lpSprite->mMaskPlaneCount; i++ )
				{
					Endian_ReadBigU16( &lpDst1[ i ], lPixels );
					lPixels &= lMask;
					Endian_WriteBigU16( &lpDst1[ i ], lPixels );
				}
				for( i=0; i<lpSprite->mGfxPlaneCount; i++ )
				{
					Endian_ReadBigU16( &lpDst1[ i ], lPixels );
					Endian_ReadBigU16( lpGfx,  lGfx );
					lPixels |= lGfx<<lShiftL;
					Endian_WriteBigU16( &lpDst1[ i ], lPixels );
					lpGfx++;
				}

			}
			lpDst0 += apCanvas->mpLineOffsets[ 1 ] >> 1;
		}
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_DrawSpritePartial( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,void * apSprite,sGraphicRect * apRect )
* ACTION   : Graphic_4BP_DrawSpritePartial
* CREATION : 06.08.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_DrawSpritePartial( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,void * apSprite,sGraphicRect * apRect )
{
	S16	lSX,lSY;
	S16	lDX,lDY;
	S16	lW,lH;
	sSprite *	lpSprite;


	lSY = apRect->mY;
	lDY = (S16)(apCoords->mY + apRect->mY);
	lH  = apRect->mHeight;

	lpSprite = (sSprite*)apSprite;
	while( lH > 0 )
	{
		lW  = apRect->mWidth;
		lSX = apRect->mX;
		lDX = (S16)(apCoords->mX + apRect->mX);
		while( lW > 0 )
		{
			if( !Graphic_4BP_Sprite_IsMasked( lpSprite, lSX, lSY ) )
			{
				Graphic_4BP_SetPixel( apCanvas, lDX, lDY,
					Graphic_4BP_Sprite_GetPixel( lpSprite, lSX, lSY ) );
			}
			lSX++;
			lDX++;
			lW--;
		}
		lSY++;
		lDY++;
		lH--;
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_DrawTri( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,S16 aColour )
* ACTION   : Graphic_4BP_DrawTri
* CREATION : 05.04.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_DrawTri( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,S16 aColour )
{
	(void)apCanvas;
	(void)apCoords;
	(void)aColour;
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_DrawQuad( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,S16 aColour )
* ACTION   : Graphic_4BP_DrawQuad
* CREATION : 05.04.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_DrawQuad( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,S16 aColour )
{
	(void)apCanvas;
	(void)apCoords;
	(void)aColour;
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_Blit_Clip( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,sGraphicRect * apRect,struct sGraphicCanvas * apSrc )
* ACTION   : Graphic_4BP_Blit_Clip
* CREATION : 05.04.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_Blit_Clip( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,sGraphicRect * apRect,struct sGraphicCanvas * apSrc )
{
	sGraphicPos		lCoords;
	sGraphicRect	lRect;
	S16				lDiff;

	lCoords = *apCoords;
	lRect   = *apRect;

	if( ( lCoords.mX                   < apCanvas->mClipBox.mX1 ) &&
		( lCoords.mY                   < apCanvas->mClipBox.mY1 ) &&
		( lCoords.mX + apRect->mWidth  > apCanvas->mClipBox.mX0 ) &&
		( lCoords.mY + apRect->mHeight > apCanvas->mClipBox.mY0 ) &&
		( lRect.mX                     < apSrc->mClipBox.mX1    ) &&
		( lRect.mY                     < apSrc->mClipBox.mY1    ) &&
		( lRect.mX + apRect->mWidth    > apSrc->mClipBox.mX0    ) &&
		( lRect.mY + apRect->mHeight   > apSrc->mClipBox.mY0    )
		)
	{

		lDiff = (S16)(apCanvas->mClipBox.mX0 - lCoords.mX);
		if( lDiff > 0 )
		{
			lRect.mWidth = (S16)(lRect.mWidth - lDiff);
			lRect.mX     = (S16)(lRect.mX + lDiff);
			lCoords.mX   = apCanvas->mClipBox.mX0;
		}

		lDiff = (S16)(apSrc->mClipBox.mX0 - lRect.mX);
		if( lDiff > 0 )
		{
			lRect.mWidth = (S16)(lRect.mWidth + lDiff);
			lCoords.mX   = (S16)(lCoords.mX + lDiff);
			lRect.mX     = 0;
		}

		lDiff = (S16)(apCanvas->mClipBox.mY0 - lCoords.mY);
		if( lDiff > 0 )
		{
			lRect.mHeight = (S16)(lRect.mHeight - lDiff);
			lRect.mY      = (S16)(lRect.mY + lDiff);
			lCoords.mY    = apCanvas->mClipBox.mY0;
		}

		lDiff = (S16)(apSrc->mClipBox.mY0 - lRect.mY);
		if( lDiff > 0 )
		{
			lRect.mHeight = (S16)(lRect.mHeight - lDiff);
			lCoords.mY    = (S16)(lCoords.mY + lDiff);
			lRect.mY      = apSrc->mClipBox.mY0;
		}

		lDiff = (S16)((lRect.mX + lRect.mWidth) - apSrc->mClipBox.mX1);
		if( lDiff > 0 )
		{
			lRect.mWidth = (S16)(lRect.mWidth - lDiff);
		}

		lDiff = (S16)((lRect.mY + lRect.mHeight) - apSrc->mClipBox.mY1);
		if( lDiff > 0 )
		{
			lRect.mHeight = (S16)(lRect.mHeight - lDiff);
		}

		lDiff = (S16)((lCoords.mX + lRect.mWidth) - apCanvas->mClipBox.mX1);
		if( lDiff > 0 )
		{
			lRect.mWidth = (S16)(lRect.mWidth-lDiff);
		}

		lDiff = (S16)((lCoords.mY + lRect.mHeight) - apCanvas->mClipBox.mY1);
		if( lDiff > 0 )
		{
			lRect.mHeight = (S16)(lRect.mHeight - lDiff);
		}

		if( ( lRect.mHeight > 0 ) && (lRect.mWidth > 0 ) )
		{
			Graphic_4BP_Blit( apCanvas, &lCoords, &lRect, apSrc );
		}
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_DrawBox_Clip( struct sGraphicCanvas * apCanvas,sGraphicRect * apCoords,S16 aColour )
* ACTION   : Graphic_4BP_DrawBox_Clip
* CREATION : 05.04.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_DrawBox_Clip( struct sGraphicCanvas * apCanvas,sGraphicRect * apCoords,S16 aColour )
{
	sGraphicRect	lRect;
	S16				lDiff;

	lRect   = *apCoords;

	if( ( lRect.mX < apCanvas->mClipBox.mX1  ) &&
		( lRect.mY < apCanvas->mClipBox.mY1 ) )
	{

		lDiff = (S16)(apCanvas->mClipBox.mX0 - lRect.mX);
		if( lDiff > 0 )
		{
			lRect.mWidth = (S16)(lRect.mWidth - lDiff);
			lRect.mX     = apCanvas->mClipBox.mX0;
		}

		lDiff = (S16)(apCanvas->mClipBox.mY0 - lRect.mY);
		if( lRect.mY < 0 )
		{
			lRect.mHeight = (S16)(lRect.mHeight - lDiff);
			lRect.mY      = apCanvas->mClipBox.mY0;
		}

		lDiff = (S16)((lRect.mX + lRect.mWidth) - apCanvas->mClipBox.mX1);
		if( lDiff > 0 )
		{
			lRect.mWidth = (S16)(lRect.mWidth - lDiff);
		}

		lDiff = (S16)((lRect.mY + lRect.mHeight) - apCanvas->mClipBox.mY1);
		if( lDiff > 0 )
		{
			lRect.mHeight =(S16)(lRect.mHeight - lDiff);
		}

		if( ( lRect.mHeight > 0 ) && (lRect.mWidth > 0 ) )
		{
			Graphic_4BP_DrawBox( apCanvas, &lRect, aColour );
		}
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_DrawLine_Clip( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,S16 aColour )
* ACTION   : Graphic_4BP_DrawLine_Clip
* CREATION : 05.04.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_DrawLine_Clip( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,S16 aColour )
{
	(void)apCanvas;
	(void)apCoords;
	(void)aColour;
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_DrawPixel_Clip( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,S16 aColour )
* ACTION   : Graphic_4BP_DrawPixel_Clip
* CREATION : 05.04.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_DrawPixel_Clip( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,S16 aColour )
{
	if( ( apCoords->mX >= apCanvas->mClipBox.mX0 ) &&
		( apCoords->mY >= apCanvas->mClipBox.mY0 ) &&
		( apCoords->mX  < apCanvas->mClipBox.mX1 ) &&
		( apCoords->mY  < apCanvas->mClipBox.mY1 ) )
	{
		Graphic_4BP_DrawPixel( apCanvas, apCoords, aColour );
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_DrawSprite_Clip( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,void * apSprite )
* ACTION   : Graphic_4BP_DrawSprite_Clip
* CREATION : 05.04.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_DrawSprite_Clip( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,void * apSprite )
{
	sSprite *		lpSprite;
	sGraphicRect	lRect;

	lpSprite = (sSprite*)apSprite;

	if( (apCoords->mX >= apCanvas->mClipBox.mX0) &&
		(apCoords->mY >= apCanvas->mClipBox.mY0) &&
		(apCoords->mX + (S16)lpSprite->mWidth < apCanvas->mClipBox.mX1) &&
		(apCoords->mY + (S16)lpSprite->mHeight < apCanvas->mClipBox.mY1) )
	{
		Graphic_4BP_DrawSprite( apCanvas, apCoords, apSprite );
	}
	else
	{
		lRect.mX = (S16)(apCanvas->mClipBox.mX0 - apCoords->mX);
		if( lRect.mX < 0 )
		{
			lRect.mX = 0;
		}
		lRect.mY = (S16)(apCanvas->mClipBox.mY0 - apCoords->mY);
		if( lRect.mY < 0 )
		{
			lRect.mY = 0;
		}
		lRect.mWidth = (S16)((apCoords->mX + lpSprite->mWidth) - apCanvas->mClipBox.mX1);
		if( lRect.mWidth < 0 )
		{
			lRect.mWidth = lpSprite->mWidth;
		}
		else
		{
			lRect.mWidth = (S16)(lpSprite->mWidth - lRect.mWidth);
		}
		lRect.mHeight = (S16)((apCoords->mY + lpSprite->mHeight) - apCanvas->mClipBox.mY1);
		if( lRect.mHeight < 0 )
		{
			lRect.mHeight = lpSprite->mHeight;
		}
		else
		{
			lRect.mHeight = (S16)(lpSprite->mHeight - lRect.mHeight);
		}
		lRect.mWidth  = (S16)(lRect.mWidth - lRect.mX);
		lRect.mHeight = (S16)(lRect.mHeight - lRect.mY);
		Graphic_4BP_DrawSpritePartial( apCanvas, apCoords, apSprite, &lRect );
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_DrawTri_Clip( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,S16 aColour )
* ACTION   : Graphic_4BP_DrawTri_Clip
* CREATION : 05.04.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_DrawTri_Clip( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,S16 aColour )
{
	(void)apCanvas;
	(void)apCoords;
	(void)aColour;
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_DrawQuad_Clip( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,S16 aColour )
* ACTION   : Graphic_4BP_DrawQuad_Clip
* CREATION : 05.04.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_DrawQuad_Clip( struct sGraphicCanvas * apCanvas,sGraphicPos * apCoords,S16 aColour )
{
	(void)apCanvas;
	(void)apCoords;
	(void)aColour;
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_GetPixel( const sGraphicCanvas * apCanvas,const S16 aX,const S16 aY )
* ACTION   : Graphic_4BP_GetPixel
* CREATION : 06.08.2004 PNK
*-----------------------------------------------------------------------------------*/

U8	Graphic_4BP_GetPixel( const sGraphicCanvas * apCanvas,const S16 aX,const S16 aY )
{
	U8		lPixel;
	U32		lOff;
	U16		lMask;
	U16 *	lpScreen;

	lOff  = aY;
	lOff *= apCanvas->mpLineOffsets[ 1 ];
	lOff >>= 1;
	lOff  +=(aX>>4)<<2;

	lpScreen  = (U16*)apCanvas->mpVRAM;
	lpScreen += lOff;
	lMask     = (U16)((0x8000>>(aX&0xF)));
	Endian_FromBigU16( &lMask );
	lPixel    = 0;

	if( lpScreen[ 0 ] & lMask )
	{
		lPixel += 1;
	}
	if( lpScreen[ 1 ] & lMask )
	{
		lPixel += 2;
	}
	if( lpScreen[ 2 ] & lMask )
	{
		lPixel += 4;
	}
	if( lpScreen[ 3 ] & lMask )
	{
		lPixel += 8;
	}

	return( lPixel );
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_SetPixel( const sGraphicCanvas * apCanvas,const S16 aX,const S16 aY,const U8 aPixel )
* ACTION   : Graphic_4BP_SetPixel
* CREATION : 06.08.2004 PNK
*-----------------------------------------------------------------------------------*/

void	Graphic_4BP_SetPixel( const sGraphicCanvas * apCanvas,const S16 aX,const S16 aY,const U8 aPixel )
{
	U32		lOff;
	U16		lMask;
	U16 *	lpScreen;

	lOff  = aY;
	lOff *= apCanvas->mpLineOffsets[ 1 ];
	lOff >>= 1;
	lOff  +=(aX>>4)<<2;

	lpScreen  = (U16*)apCanvas->mpVRAM;
	lpScreen += lOff;
	lMask     = (U16)(~(0x8000>>(aX&0xF)));
	Endian_FromBigU16( &lMask );

	lpScreen[ 0 ] &= lMask;
	lpScreen[ 1 ] &= lMask;
	lpScreen[ 2 ] &= lMask;
	lpScreen[ 3 ] &= lMask;

	lMask = (U16)(~lMask);

	if( aPixel & 1 )
	{
		lpScreen[ 0 ] |= lMask;
	}
	if( aPixel & 2 )
	{
		lpScreen[ 1 ] |= lMask;
	}
	if( aPixel & 4 )
	{
		lpScreen[ 2 ] |= lMask;
	}
	if( aPixel & 8 )
	{
		lpScreen[ 3 ] |= lMask;
	}

}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_Sprite_IsMasked( const sSprite * apSprite,const S16 aX,const S16 aY )
* ACTION   : Graphic_4BP_Sprite_IsMasked
* CREATION : 06.08.2004 PNK
*-----------------------------------------------------------------------------------*/

U8	Graphic_4BP_Sprite_IsMasked( const sSprite * apSprite,const S16 aX,const S16 aY )
{
	U16 *	lpMask;
	U16		lMask;
	U32		lOff;
	U8		lRes;

	lpMask  = apSprite->mpMask;
	lRes    = 1;
	if( lpMask )
	{
		lOff    = (apSprite->mWidth>>4) * aY;
		lOff   += (aX>>4);
		lpMask += lOff;
		lMask   = (U16)((0x8000>>(aX&0xF)));
		Endian_FromBigU16( &lMask );
		if( !(*lpMask & lMask) )
		{
			lRes = 0;
		}
	}
	return( lRes );
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Graphic_4BP_Sprite_GetPixel( const sSprite * apSprite,const S16 aX,const S16 aY )
* ACTION   : Graphic_4BP_Sprite_GetPixel
* CREATION : 06.08.2004 PNK
*-----------------------------------------------------------------------------------*/

U8	Graphic_4BP_Sprite_GetPixel( const sSprite * apSprite,const S16 aX,const S16 aY )
{
	U8		lPixel;
	U32		lOff;
	U16		lMask;
	U16		lChunks;
	U16 *	lpGfx;

	lChunks = (U16)(apSprite->mWidth >> 4);
	lOff    = apSprite->mGfxPlaneCount * aY * lChunks;
	lOff   +=(aX>>4) * apSprite->mGfxPlaneCount;

	lpGfx  = apSprite->mpGfx;
	lpGfx += lOff;
	lMask  = (U16)((0x8000>>(aX&0xF)));
	Endian_FromBigU16( &lMask );

	lPixel = 0;

	if( lpGfx[ 0 ] & lMask )
	{
		lPixel += 1;
	}
	if( lpGfx[ 1 ] & lMask )
	{
		lPixel += 2;
	}
	if( lpGfx[ 2 ] & lMask )
	{
		lPixel += 4;
	}
	if( lpGfx[ 3 ] & lMask )
	{
		lPixel += 8;
	}

	return( lPixel );
}


/* ################################################################################ */

#endif	/* INCLUDED_GRF_4_H */
