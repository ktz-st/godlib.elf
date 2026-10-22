#include "imploder.h"

extern S32 Imploder_DepackAsm( void * apSrc, void * apDst );

static U32 Imploder_Read32( const U8 * apData )
{
	return ((U32)apData[0] << 24) | ((U32)apData[1] << 16)
	     | ((U32)apData[2] << 8) | apData[3];
}

U8 Imploder_IsPacked( const void * apData )
{
	const U8 * p = (const U8 *)apData;
	U32 end, size;
	if( !p || p[0] != 'I' || p[1] != 'M' || p[2] != 'P' || p[3] != '!' )
		return 0;
	size = Imploder_Read32( p + 4 );
	end = Imploder_Read32( p + 8 );
	return (U8)(end >= 14 && !(end & 1) && end <= 0xFFFFFFFFUL - dIMPLODER_TRAILER_SIZE
	         && end + dIMPLODER_TRAILER_SIZE <= size);
}

U32 Imploder_GetDepackSize( const void * apData )
{
	return Imploder_IsPacked( apData ) ? Imploder_Read32( (const U8 *)apData + 4 ) : 0;
}

U32 Imploder_GetPackedSize( const void * apData )
{
	return Imploder_IsPacked( apData ) ? Imploder_Read32( (const U8 *)apData + 8 ) + dIMPLODER_TRAILER_SIZE : 0;
}

U8 Imploder_DepackChecked( const void * apSrc, U32 aSrcSize, void * apDst, U32 aDstSize )
{
	const U8 * src = (const U8 *)apSrc;
	U8 * dst = (U8 *)apDst;
	U32 packed, size, sum = 7, i;
	if( aSrcSize < dIMPLODER_HEADER_SIZE || !Imploder_IsPacked( apSrc ) || !dst )
		return 0;
	packed = Imploder_GetPackedSize( apSrc );
	size = Imploder_GetDepackSize( apSrc );
	if( packed != aSrcSize || aDstSize < size || ((U32)dst & 1) )
		return 0;
	/* Identical buffers are supported; all other overlap is rejected. */
	if( src != dst && ((U32)dst > (U32)src ? (U32)dst - (U32)src < packed
	                                      : (U32)src - (U32)dst < size) )
		return 0;
	for( i = 0; i < packed - 4; i += 2 )
		sum += ((U32)src[i] << 8) | src[i+1];
	if( sum != Imploder_Read32( src + packed - 4 ) )
		return 0;
	/* The original decoder restores twelve compressed bytes over its header.
	   Copying into dst first keeps separate packed assets read-only and requires
	   no allocation or workspace beyond the output buffer. */
	if( src != dst )
		for( i = 0; i < packed; ++i )
			dst[i] = src[i];
	return (U8)(Imploder_DepackAsm( dst, dst ) == -1L);
}

U8 Imploder_Depack( const void * apSrc, void * apDst )
{
	return Imploder_DepackChecked( apSrc, Imploder_GetPackedSize( apSrc ),
	                             apDst, Imploder_GetDepackSize( apSrc ) );
}
