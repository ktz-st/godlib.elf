#include "rnc.h"

static U32 Rnc_ReadBigU32( const U8 * apData )
{
	return( ((U32)apData[0] << 24)
		  | ((U32)apData[1] << 16)
		  | ((U32)apData[2] << 8)
		  |  (U32)apData[3] );
}

U8 Rnc_IsPacked( const void * apData )
{
	return( (U8)(Rnc_GetVersion( apData ) != eRNC_VERSION_NONE) );
}

U8 Rnc_GetVersion( const void * apData )
{
	const sRncHeader * lpHeader;

	if( !apData )
	{
		return( eRNC_VERSION_NONE );
	}

	lpHeader = (const sRncHeader *)apData;

	if( (lpHeader->mSignature[0] != 'R')
	||	(lpHeader->mSignature[1] != 'N')
	||	(lpHeader->mSignature[2] != 'C') )
	{
		return( eRNC_VERSION_NONE );
	}

	if( (lpHeader->mVersion == eRNC_VERSION_1)
	||	(lpHeader->mVersion == eRNC_VERSION_2) )
	{
		return( lpHeader->mVersion );
	}

	return( eRNC_VERSION_NONE );
}

U32 Rnc_GetDepackSize( const void * apData )
{
	const sRncHeader * lpHeader = (const sRncHeader *)apData;

	if( !Rnc_IsPacked( apData ) )
	{
		return( 0 );
	}

	return( Rnc_ReadBigU32( lpHeader->mUnpackedSize ) );
}

U32 Rnc_GetPackedSize( const void * apData )
{
	const sRncHeader * lpHeader = (const sRncHeader *)apData;

	if( !Rnc_IsPacked( apData ) )
	{
		return( 0 );
	}

	return( Rnc_ReadBigU32( lpHeader->mPackedSize ) );
}

U32 Rnc_GetHeaderSize( const void * apData )
{
	(void)apData;
	return( dRNC_HEADER_SIZE );
}

void Rnc_Depack( const void * apSrc, void * apDst )
{
	switch( Rnc_GetVersion( apSrc ) )
	{
	case eRNC_VERSION_1:
		Rnc_Depack1( apSrc, apDst );
		break;

	case eRNC_VERSION_2:
		Rnc_Depack2( apSrc, apDst );
		break;

	default:
		break;
	}
}
