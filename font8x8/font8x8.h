#ifndef	INCLUDED_FONT8X8_H
#define	INCLUDED_FONT8X8_H

/* ###################################################################################
#  INCLUDES
################################################################################### */

#include	<godlib/base/base.h>

struct sGraphicCanvas;


/* ###################################################################################
#  PROTOTYPES
################################################################################### */

void	Font8x8_Print( const char * apString, U16 * apScreen, U16 aX, U16 aY );
void	Font8x8_PrintColour( const char * apString, U16 * apScreen, U16 aX, U16 aY, U16 aColour );

/* ST-low four-plane layout, x aligned to 8 pixels, no clipping.
 * Raw Screen pages use their canvas line offsets; other raw buffers use 160.
 * Canvas variants support custom line layouts (including padded rows). */
void Font8x8_PrintCanvas( const char * apString, struct sGraphicCanvas * apCanvas, U16 aX, U16 aY );
void Font8x8_PrintColourCanvas( const char * apString, struct sGraphicCanvas * apCanvas, U16 aX, U16 aY, U16 aColour );


/* ################################################################################ */

#endif	/*	INCLUDED_FONT8X8_H */
