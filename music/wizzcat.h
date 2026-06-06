#ifndef INCLUDED_WIZZCAT_H
#define INCLUDED_WIZZCAT_H

/* ###################################################################################
#  INCLUDES
################################################################################### */

#include <godlib/base/base.h>


/* ###################################################################################
#  DEFINES
################################################################################### */

#define dWIZZCAT_MODULE_MARGIN_SIZE	(128UL * 1024UL)


/* ###################################################################################
#  STRUCTS
################################################################################### */

typedef struct sWizzcatModule
{
	U32	mDataSize;
	U8	mData[ 1 ];
} sWizzcatModule;

typedef struct sWizzcatInfo
{
	U8	mSongPos;
	U8	mPattPos;
} sWizzcatInfo;


/* ###################################################################################
#  PROTOTYPES
################################################################################### */

sWizzcatModule *	Wizzcat_Load( const char * apFileName );
void				Wizzcat_UnLoad( sWizzcatModule * apModule );

void				Wizzcat_Init( sWizzcatModule * apModule );
void				Wizzcat_Play( void );
void				Wizzcat_Stop( void );

U8					Wizzcat_Start( sWizzcatModule * apModule );
void				Wizzcat_StopVbl( void );

void				Wizzcat_GetInfo( sWizzcatInfo * apInfo );
void				Wizzcat_Jump( U8 aPos );


/* ################################################################################ */

#endif /* INCLUDED_WIZZCAT_H */
