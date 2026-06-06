#ifndef INCLUDED_LANCEMOD_H
#define INCLUDED_LANCEMOD_H

/* ###################################################################################
#  INCLUDES
################################################################################### */

#include <godlib/base/base.h>


/* ###################################################################################
#  DEFINES
################################################################################### */

#define dLANCEMOD_EXTRA_BUFFER_SIZE    ((31L * 664L) + 2L)


/* ###################################################################################
#  ENUMS
################################################################################### */

enum
{
	eLANCEMOD_FREQ_12K = 1,
	eLANCEMOD_FREQ_25K = 2,
	eLANCEMOD_FREQ_50K = 3
};


/* ###################################################################################
#  PROTOTYPES
################################################################################### */

void *	LanceMod_Load( const char * apFileName );
void	LanceMod_UnLoad( void * apModData );

void	LanceMod_InitPaula( U8 aFreq );
void	LanceMod_ShutdownPaula( void );
void	LanceMod_Init( void * apModData );
void	LanceMod_Play( void );
void	LanceMod_Stop( void );

U8		LanceMod_Start( void * apModData, U8 aFreq );
void	LanceMod_StopVbl( void );

void	LanceMod_SetMasterVolume( U16 aVolume );
void	LanceMod_SetRasterDebug( U8 aFlag );
U8		LanceMod_GetEndFlag( void );


/* ################################################################################ */

#endif /* INCLUDED_LANCEMOD_H */
