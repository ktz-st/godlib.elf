#ifndef INCLUDED_RNC_H
#define INCLUDED_RNC_H

/* ################################################################################### */

#include <godlib/base/base.h>

/* ################################################################################### */

#define dRNC_HEADER_SIZE 18UL

enum
{
	eRNC_VERSION_NONE = 0,
	eRNC_VERSION_1    = 1,
	eRNC_VERSION_2    = 2
};

typedef struct sRncHeader
{
	U8 mSignature[3];
	U8 mVersion;
	U8 mUnpackedSize[4];
	U8 mPackedSize[4];
	U8 mUnpackedCrc[2];
	U8 mPackedCrc[2];
	U8 mBlockCount;
	U8 mBlockSize;
} sRncHeader;

U8		Rnc_IsPacked( const void * apData );
U8		Rnc_GetVersion( const void * apData );
U32		Rnc_GetDepackSize( const void * apData );
U32		Rnc_GetPackedSize( const void * apData );
U32		Rnc_GetHeaderSize( const void * apData );
void	Rnc_Depack( const void * apSrc, void * apDst );

extern void Rnc_Depack1( const void * apSrc, void * apDst );
extern void Rnc_Depack2( const void * apSrc, void * apDst );

/* ################################################################################### */

#endif
