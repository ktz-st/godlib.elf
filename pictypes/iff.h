#ifndef	INCLUDED_IFF_H
#define	INCLUDED_IFF_H

/* ###################################################################################
#  INCLUDES
################################################################################### */

#include	<godlib/base/base.h>


/* ###################################################################################
#  DEFINES
################################################################################### */

#define	dIFF_BMHD_ID		mSTRING_TO_U32( 'B', 'M', 'H', 'D' )
#define	dIFF_BODY_ID		mSTRING_TO_U32( 'B', 'O', 'D', 'Y' )
#define	dIFF_CAMG_ID		mSTRING_TO_U32( 'C', 'A', 'M', 'G' )
#define	dIFF_CMAP_ID		mSTRING_TO_U32( 'C', 'M', 'A', 'P' )
#define	dIFF_CRNG_ID		mSTRING_TO_U32( 'C', 'R', 'N', 'G' )
#define	dIFF_FORM_ID		mSTRING_TO_U32( 'F', 'O', 'R', 'M' )
#define	dIFF_ILBM_ID		mSTRING_TO_U32( 'I', 'L', 'B', 'M' )
#define	dIFF_VDAT_ID		mSTRING_TO_U32( 'V', 'D', 'A', 'T' )

#define dIFF_COMP_NONE		0
#define dIFF_COMP_BYTERUN1	1
#define dIFF_COMP_VDAT		2

#define dIFF_RGB8_TO_STE(r,g,b) \
	((U16)(((((U16)(r)) << 3) & 0x700) | ((((U16)(r)) << 7) & 0x800) | \
	((((U16)(g)) >> 1) & 0x70) | ((((U16)(g)) << 3) & 0x80) | \
	((((U16)(b)) >> 5) & 0x7) | ((((U16)(b)) >> 1) & 0x8)))


/* ###################################################################################
#  STRUCTS
################################################################################### */

typedef	struct
{
	U32	mID;
	U32	mLength;
} sIffChunk;


typedef	struct
{
	U32	mID;
	U32	mLength;
	U32	mILBM;
} sIffFormChunk;


typedef	struct
{
	U32	mID;
	U32	mLength;
	U16	mWidth;
	U16	mHeight;
	U16	mX;
	U16	mY;
	U8	mPlaneCount;
	U8	mMask;
	U8	mCompressedFlag;
	U8	mReserved;
	U8	mTransparentColourIndex;
	U8	mAspectX;
	U8	mAspectY;
	U16	mPageWidth;
	U16	mPageHeight;
} sIffBmhdChunk;


typedef	struct
{
	U8	mR;
	U8	mG;
	U8	mB;
} sIffColour;


typedef	struct
{
	U32			mID;
	U32			mLength;
	sIffColour	mColour[ 1 ];
} sIffCmapChunk;


typedef	struct
{
	U32	mID;
	U32	mLength;
	U16	mReserved;
	U16	mAnimSpeed;
	U16	mActiveFlag;
	U8	mLeftBotColourAnimLimit;
	U8	mRightTopColourAnimLimit;
} sIffCrngChunk;


typedef	struct
{
	U32	mID;
	U32	mLength;
	U32	mViewportMode;
} sIffCamgChunk;


typedef	struct
{
	U32	mID;
	U32	mLength;
	U8	mPixels[ 1 ];
} sIffBodyChunk;


typedef	struct
{
	U16			mWidth;
	U16			mHeight;
	U16			mX;
	U16			mY;
	U8			mPlaneCount;
	U8			mMask;
	U8			mCompressedFlag;
	U8			mTransparentColourIndex;
	U8			mAspectX;
	U8			mAspectY;
	U16			mPageWidth;
	U16			mPageHeight;
	U32			mViewportMode;
	const U8 *	mpBody;
	U32			mBodyLength;
	const sIffColour * mpCmap;
	U16			mColourCount;
} sIffIlbm;


/* ###################################################################################
#  PROTOTYPES
################################################################################### */

U8		Iff_ILBM_Parse( const void * apIff, sIffIlbm * apIlbm );
U16		Iff_RGB8ToSTE( const U8 aR, const U8 aG, const U8 aB );
U16		Iff_ILBM_CmapToSTE( const sIffIlbm * apIlbm, U16 * apPalette, const U16 aColourLimit );
U8		Iff_ILBM_DecodeToSTLow( const sIffIlbm * apIlbm, void * apDst );

void	Iff_Interleave4PlaneLine( const U8 * apSrc, void * apDst, const U16 aWordCount );


/* ################################################################################ */

#endif	/*	INCLUDED_IFF_H */
