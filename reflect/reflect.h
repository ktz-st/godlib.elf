#ifndef INCLUDED_REFLECT_H
#define INCLUDED_REFLECT_H

#include	<godlib/base/base.h>

enum eReflectType
{
	eReflectType_Float  = ( 1 << 0 ),
	eReflectType_Leaf   = ( 1 << 2 ),
	eReflectType_Signed = ( 1 << 3 ),
	eReflectType_String = ( 1 << 4 ),
};

struct sString;

typedef struct sReflectEnumElement
{
	const char *	mpName;
	U32				mValue;
} sReflectEnumElement;

typedef struct sReflectEnumType
{
	U32						mEnumElementCount;
	sReflectEnumElement *	mpEnumElements;
} sReflectEnumType;

typedef struct sReflectElement
{
	char *				mpTypeName;
	char *				mpElementName;
	U32					mOffset;
	void *	mpType;
} sReflectElement;

typedef struct sReflectType
{
	char *						mpTypeName;
	U32							mSizeBytes;
	U16							mTypeFlags;
	U16							mElementCount;
	struct sReflectElement *	mpElements;
	struct sReflectType *		mpTypeNext;
} sReflectType;

typedef struct sReflectDictionary
{
	U32							mTypeCount;
	struct sReflectType *		mpTypes;
	U32							mEnumCount;
	struct sReflectEnumType*	mpEnums;
}sReflectDictionary;

sReflectType *	Reflect_GetpType( const sReflectDictionary * apDictionary, const char * apType );
U8 *			Reflect_GetpData( const sReflectType * apType, U8 * apData,  const char * apElement );
/* Returns 1 after writing a supported field, 0 without changing it on failure.
 * Numeric tokens are bounded decimal strings (F32 also accepts exponent notation).
 * Integers accept true/false, reject overflow and out-of-range/nonnumeric input.
 * String fields retain shallow descriptor-copy semantics: the source storage
 * and its ownership remain the caller's responsibility. */
U8				Reflect_SetData( const sReflectType * apType, const struct sString * apElementName, const struct sString * apValue, void * apStructBase );

#endif