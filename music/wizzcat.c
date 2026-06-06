/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: WIZZCAT.C
::
:: ProTracker MOD loader/player wrapper for Wizzcat STE replay.
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


/* ###################################################################################
#  INCLUDES
################################################################################### */

#include "wizzcat.h"

#include <godlib/file/file.h>
#include <godlib/memory/memory.h>
#include <godlib/vbl/vbl.h>


/* ###################################################################################
#  PROTOTYPES
################################################################################### */

extern void	WIZinit( void );
extern void	WIZmodInit( void * apModule, void * apWorkspaceEnd );
extern void	WIZplay( void );
extern void	WIZgetInfo( sWizzcatInfo * apInfo );
extern void	WIZjump( U8 aPos );

extern void	Wizzcat_Vbl( void );


/* ###################################################################################
#  CODE
################################################################################### */


/*-----------------------------------------------------------------------------------*
* FUNCTION : Wizzcat_Load( const char * apFileName )
* ACTION   : loads a MOD and appends Wizzcat sample workspace
*-----------------------------------------------------------------------------------*/

sWizzcatModule *	Wizzcat_Load( const char * apFileName )
{
	sFileHandle		lHandle;
	S32				lSize;
	sWizzcatModule *	lpModule;

	lSize = File_GetSize( apFileName );
	if( lSize <= 0 )
	{
		return( 0 );
	}

	lHandle = File_Open( apFileName );
	if( lHandle <= 0 )
	{
		return( 0 );
	}

	lpModule = (sWizzcatModule*)mMEMCALLOC( (U32)sizeof(sWizzcatModule) - 1UL + (U32)lSize + dWIZZCAT_MODULE_MARGIN_SIZE );
	if( !lpModule )
	{
		File_Close( lHandle );
		return( 0 );
	}

	lpModule->mDataSize = (U32)lSize;
	if( File_Read( lHandle, (U32)lSize, lpModule->mData ) != lSize )
	{
		File_Close( lHandle );
		mMEMFREE( lpModule );
		return( 0 );
	}

	File_Close( lHandle );

	return( lpModule );
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Wizzcat_UnLoad( sWizzcatModule * apModule )
* ACTION   : releases MOD memory
*-----------------------------------------------------------------------------------*/

void	Wizzcat_UnLoad( sWizzcatModule * apModule )
{
	if( apModule )
	{
		mMEMFREE( apModule );
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Wizzcat_Init( sWizzcatModule * apModule )
* ACTION   : initialises replay tables and prepares module sample workspace
*-----------------------------------------------------------------------------------*/

void	Wizzcat_Init( sWizzcatModule * apModule )
{
	void *	lpWorkspaceEnd;

	if( !apModule )
	{
		return;
	}

	lpWorkspaceEnd = &apModule->mData[ apModule->mDataSize + dWIZZCAT_MODULE_MARGIN_SIZE ];

	WIZinit();
	WIZmodInit( apModule->mData, lpWorkspaceEnd );
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Wizzcat_Play( void )
* ACTION   : primes DMA replay buffers
*-----------------------------------------------------------------------------------*/

void	Wizzcat_Play( void )
{
	WIZplay();
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Wizzcat_Stop( void )
* ACTION   : stops STE DMA sound
*-----------------------------------------------------------------------------------*/

void	Wizzcat_Stop( void )
{
	*(volatile U8*)0xFFFF8901L = 0;
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Wizzcat_Start( sWizzcatModule * apModule )
* ACTION   : starts Wizzcat replay and installs the GodLib VBL callback
*-----------------------------------------------------------------------------------*/

U8	Wizzcat_Start( sWizzcatModule * apModule )
{
	if( !apModule )
	{
		return( 0 );
	}

	Wizzcat_Init( apModule );
	Wizzcat_Play();

	return( Vbl_AddCall( Wizzcat_Vbl ) );
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Wizzcat_StopVbl( void )
* ACTION   : removes the GodLib VBL callback and stops DMA sound
*-----------------------------------------------------------------------------------*/

void	Wizzcat_StopVbl( void )
{
	Vbl_RemoveCall( Wizzcat_Vbl );
	Wizzcat_Stop();
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Wizzcat_GetInfo( sWizzcatInfo * apInfo )
* ACTION   : returns current song and pattern position
*-----------------------------------------------------------------------------------*/

void	Wizzcat_GetInfo( sWizzcatInfo * apInfo )
{
	if( apInfo )
	{
		WIZgetInfo( apInfo );
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Wizzcat_Jump( U8 aPos )
* ACTION   : jumps to a 1-based song position
*-----------------------------------------------------------------------------------*/

void	Wizzcat_Jump( U8 aPos )
{
	WIZjump( aPos );
}


/* ################################################################################ */
