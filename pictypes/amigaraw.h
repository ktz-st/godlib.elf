#ifndef	INCLUDED_AMIGARAW_H
#define	INCLUDED_AMIGARAW_H

/* ################################################################################### */

#include	<godlib/base/base.h>
#include	<godlib/pictypes/canvas.h>

/* ################################################################################### */

#define	dAMIGARAW_WIDTH			320U
#define	dAMIGARAW_HEIGHT		257U
#define	dAMIGARAW_PLANES		4U
#define	dAMIGARAW_COLOURS		16U
#define	dAMIGARAW_WORDS_WIDE	(dAMIGARAW_WIDTH >> 4)
#define	dAMIGARAW_PLANE_WORDS	(dAMIGARAW_WORDS_WIDE * dAMIGARAW_HEIGHT)
#define	dAMIGARAW_FILE_SIZE		(32UL + (dAMIGARAW_PLANES * dAMIGARAW_PLANE_WORDS * sizeof(U16)))

typedef struct sAmigaRaw
{
	U16	mPalette[ dAMIGARAW_COLOURS ];
	U16	mPixels[ dAMIGARAW_PLANES ][ dAMIGARAW_PLANE_WORDS ];
} sAmigaRaw;

/* ################################################################################### */

U32			AmigaRaw_GetSize( void );
void		AmigaRaw_PaletteToST( const U16 * apAmigaPalette, U16 * apStPalette, U16 aColourCount );
void		AmigaRaw_To4Plane( const sAmigaRaw * apRaw, U16 * apPixels );
sCanvas *	AmigaRaw_ToCanvas( const sAmigaRaw * apRaw );

/* ################################################################################### */

#endif
