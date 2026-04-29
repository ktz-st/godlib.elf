/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: MOD.C
::
:: ProTracker MOD loader/player wrapper for Lance Paula replay.
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


/* ###################################################################################
#  INCLUDES
################################################################################### */

#include "mod.h"

#include <godlib/file/file.h>
#include <godlib/memory/memory.h>
#include <godlib/vbl/vbl.h>


/* ###################################################################################
#  CODE
################################################################################### */


/*-----------------------------------------------------------------------------------*
* FUNCTION : Mod_Load( const char * apFileName )
* ACTION   : loads a MOD and appends replay loop workspace
*-----------------------------------------------------------------------------------*/

void *	Mod_Load( const char * apFileName )
{
	sFileHandle	lHandle;
	S32			lSize;
	void *		lpBuffer;

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

	lpBuffer = mMEMCALLOC( (U32)lSize + dMOD_EXTRA_BUFFER_SIZE );
	if( !lpBuffer )
	{
		File_Close( lHandle );
		return( 0 );
	}

	if( File_Read( lHandle, (U32)lSize, lpBuffer ) != lSize )
	{
		File_Close( lHandle );
		mMEMFREE( lpBuffer );
		return( 0 );
	}

	File_Close( lHandle );

	return( lpBuffer );
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Mod_UnLoad( void * apModData )
* ACTION   : releases MOD memory
*-----------------------------------------------------------------------------------*/

void	Mod_UnLoad( void * apModData )
{
	if( apModData )
	{
		mMEMFREE( apModData );
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Mod_Start( void * apModData, U8 aFreq )
* ACTION   : starts Paula replay and installs Mod_Play in the VBL queue
*-----------------------------------------------------------------------------------*/

U8	Mod_Start( void * apModData, U8 aFreq )
{
	if( !apModData )
	{
		return( 0 );
	}

	if( (aFreq < eMOD_FREQ_12K) || (aFreq > eMOD_FREQ_50K) )
	{
		aFreq = eMOD_FREQ_25K;
	}

	Mod_InitPaula( aFreq );
	Mod_Init( apModData );
	Mod_SetMasterVolume( 64 );

	return( Vbl_AddCall( Mod_Play ) );
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Mod_StopVbl( void )
* ACTION   : removes Mod_Play from VBL and shuts down Paula replay
*-----------------------------------------------------------------------------------*/

void	Mod_StopVbl( void )
{
	Vbl_RemoveCall( Mod_Play );
	Mod_Stop();
}
