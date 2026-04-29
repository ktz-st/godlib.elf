#ifndef INCLUDED_MOD_H
#define INCLUDED_MOD_H

/* ###################################################################################
#  INCLUDES
################################################################################### */

#include <godlib/base/base.h>


/* ###################################################################################
#  DEFINES
################################################################################### */

#define dMOD_EXTRA_BUFFER_SIZE    ((31L * 664L) + 2L)


/* ###################################################################################
#  ENUMS
################################################################################### */

enum
{
	eMOD_FREQ_12K = 1,
	eMOD_FREQ_25K = 2,
	eMOD_FREQ_50K = 3
};


/* ###################################################################################
#  PROTOTYPES
################################################################################### */

void *	Mod_Load( const char * apFileName );
void	Mod_UnLoad( void * apModData );

void	Mod_InitPaula( U8 aFreq );
void	Mod_Init( void * apModData );
void	Mod_Play( void );
void	Mod_Stop( void );

U8		Mod_Start( void * apModData, U8 aFreq );
void	Mod_StopVbl( void );

void	Mod_SetMasterVolume( U16 aVolume );
void	Mod_SetRasterDebug( U8 aFlag );
U8		Mod_GetEndFlag( void );


/* ################################################################################ */

#endif /* INCLUDED_MOD_H */
