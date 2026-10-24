#ifndef	INCLUDED_WIPE_H
#define	INCLUDED_WIPE_H

/* ###################################################################################
#  INCLUDES
################################################################################### */

#include	<godlib/base/base.h>


/* ###################################################################################
#  DEFINES
################################################################################### */

#define	dWIPE_FX_LIMIT	39


/* ###################################################################################
#  PROTOTYPES
################################################################################### */

void	Wipe_In_Init( U16 aIndex, U16 * apGfx );
void	Wipe_Out_Init( U16 aIndex );
U16		Wipe_Update( U16 * apScreen );

U16		Wipe_IsFinished( void );

/* Independent of Graphic_SetBlitterEnable. Default is CPU; enable after
 * System_Init/Platform_Init. Getter reports 0 when no BLiTTER is present.
 * Both backends use even 320x200 four-plane ST-RAM buffers, stride 160. */
void	Wipe_SetBlitterEnable( U16 aEnable );
U16		Wipe_GetBlitterEnable( void );


/* ################################################################################ */

#endif	/* INCLUDED_WIPE_H */
