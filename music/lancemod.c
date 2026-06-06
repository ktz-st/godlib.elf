/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::
:: LANCEMOD.C
::
:: ProTracker MOD loader/player wrapper for Lance Paula replay.
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


/* ###################################################################################
#  INCLUDES
################################################################################### */

#include "lancemod.h"

#include <godlib/file/file.h>
#include <godlib/memory/memory.h>
#include <godlib/vbl/vbl.h>


extern void	Paula_Init( U8 aFreq );
extern void	Paula_SetReplayBuf( void * apBuffer );


static void *	spPaulaReplayBuffer = 0;
static U16		sPaulaReplayLen = 0;


static U16	LanceMod_ReplayLenForFreq( U8 aFreq )
{
	if( aFreq == eLANCEMOD_FREQ_50K ) { return( 2000 ); }
	if( aFreq == eLANCEMOD_FREQ_25K ) { return( 1000 ); }
	return( 500 );
}


/* ###################################################################################
#  CODE
################################################################################### */


/*-----------------------------------------------------------------------------------*
* FUNCTION : LanceMod_Load( const char * apFileName )
* ACTION   : loads a MOD and appends replay loop workspace
*-----------------------------------------------------------------------------------*/

void *	LanceMod_Load( const char * apFileName )
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

	lpBuffer = mMEMCALLOC( (U32)lSize + dLANCEMOD_EXTRA_BUFFER_SIZE );
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
* FUNCTION : LanceMod_UnLoad( void * apModData )
* ACTION   : releases MOD memory
*-----------------------------------------------------------------------------------*/

void	LanceMod_UnLoad( void * apModData )
{
	if( apModData )
	{
		mMEMFREE( apModData );
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : LanceMod_InitPaula( U8 aFreq )
* ACTION   : ensures DMA replay buffer is allocated for the requested frequency
*            and runs the Paula replay initialisation
*-----------------------------------------------------------------------------------*/

void	LanceMod_InitPaula( U8 aFreq )
{
	U16 lLen;

	if( (aFreq < eLANCEMOD_FREQ_12K) || (aFreq > eLANCEMOD_FREQ_50K) )
	{
		aFreq = eLANCEMOD_FREQ_25K;
	}

	lLen = LanceMod_ReplayLenForFreq( aFreq );

	if( sPaulaReplayLen != lLen )
	{
		if( spPaulaReplayBuffer )
		{
			mMEMFREE( spPaulaReplayBuffer );
		}
		spPaulaReplayBuffer = mMEMCALLOC( 2UL * (U32)lLen );
		sPaulaReplayLen = lLen;
	}

	Paula_SetReplayBuf( spPaulaReplayBuffer );
	Paula_Init( aFreq );
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : LanceMod_ShutdownPaula( void )
* ACTION   : releases the DMA replay buffer
*-----------------------------------------------------------------------------------*/

void	LanceMod_ShutdownPaula( void )
{
	if( spPaulaReplayBuffer )
	{
		mMEMFREE( spPaulaReplayBuffer );
		spPaulaReplayBuffer = 0;
		sPaulaReplayLen = 0;
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : LanceMod_Start( void * apModData, U8 aFreq )
* ACTION   : starts Paula replay and installs LanceMod_Play in the VBL queue
*-----------------------------------------------------------------------------------*/

U8	LanceMod_Start( void * apModData, U8 aFreq )
{
	if( !apModData )
	{
		return( 0 );
	}

	if( (aFreq < eLANCEMOD_FREQ_12K) || (aFreq > eLANCEMOD_FREQ_50K) )
	{
		aFreq = eLANCEMOD_FREQ_25K;
	}

	LanceMod_InitPaula( aFreq );
	if( !spPaulaReplayBuffer )
	{
		return( 0 );
	}
	LanceMod_Init( apModData );
	LanceMod_SetMasterVolume( 64 );

	return( Vbl_AddCall( LanceMod_Play ) );
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : LanceMod_StopVbl( void )
* ACTION   : removes LanceMod_Play from VBL and shuts down Paula replay
*-----------------------------------------------------------------------------------*/

void	LanceMod_StopVbl( void )
{
	Vbl_RemoveCall( LanceMod_Play );
	LanceMod_Stop();
}
