/*
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
:: IFF.C
::
:: Routines for manipulating IFF ILBM images
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
*/

/* ###################################################################################
#  INCLUDES
################################################################################### */

#include "iff.h"

#include <godlib/memory/memory.h>
#include <godlib/packer/brun1.h>


/* ###################################################################################
#  CODE
################################################################################### */

static U16 Iff_ReadBigU16(const U8 * apSrc)
{
	return (U16)((((U16)apSrc[0]) << 8) | apSrc[1]);
}


static U32 Iff_ReadBigU32(const U8 * apSrc)
{
	return (((U32)apSrc[0]) << 24) | (((U32)apSrc[1]) << 16) | (((U32)apSrc[2]) << 8) | apSrc[3];
}


static void Iff_WriteBigU16(U8 * apDst, const U16 aValue)
{
	apDst[0] = (U8)(aValue >> 8);
	apDst[1] = (U8)aValue;
}


static U8 Iff_ILBM_WriteVDATWord(U8 * apDst, const U32 aLineBytes, const U16 aHeight, const U8 aPlane, U16 * apColumn, U16 * apY, const U16 aValue)
{
	U8 * lpDst;

	lpDst = apDst + (((U32)*apY) * aLineBytes) + (((U32)*apColumn) << 3) + (((U32)aPlane) << 1);
	Iff_WriteBigU16(lpDst, aValue);

	(*apY)++;
	if (*apY >= aHeight)
	{
		*apY = 0;
		(*apColumn)++;
	}

	return 1;
}


static U8 Iff_ILBM_DecodeVDATPlane(const U8 * apSrc, const U32 aSrcBytes, U8 * apDst, const U16 aWordCount, const U16 aHeight, const U32 aLineBytes, const U8 aPlane)
{
	U32 lCommandEnd;
	U32 lCommandBytes;
	U32 lCommandPos;
	U32 lDataPos;
	U32 lPlaneWords;
	U32 lWordsDone;
	U16 lColumn;
	U16 lY;

	if (aSrcBytes < 2)
		return 0;

	lCommandBytes = Iff_ReadBigU16(apSrc);
	lCommandEnd = lCommandBytes + 2UL;
	if (lCommandBytes > aSrcBytes || lCommandEnd > aSrcBytes)
		return 0;

	lCommandPos = 2;
	lDataPos = lCommandBytes;
	lPlaneWords = ((U32)aWordCount) * aHeight;
	lWordsDone = 0;
	lColumn = 0;
	lY = 0;

	while ((lCommandPos < lCommandEnd) && (lWordsDone < lPlaneWords))
	{
		S8 lCommand;
		U16 lCount;
		U16 lValue;

		lCommand = (S8)apSrc[lCommandPos++];

		if (lCommand == 0)
		{
			if ((lDataPos + 2) > aSrcBytes)
				return 0;
			lCount = Iff_ReadBigU16(&apSrc[lDataPos]);
			lDataPos += 2;

			while (lCount--)
			{
				if (lWordsDone >= lPlaneWords)
					break;
				if ((lDataPos + 2) > aSrcBytes)
					return 0;
				lValue = Iff_ReadBigU16(&apSrc[lDataPos]);
				lDataPos += 2;
				Iff_ILBM_WriteVDATWord(apDst, aLineBytes, aHeight, aPlane, &lColumn, &lY, lValue);
				lWordsDone++;
			}
		}
		else if (lCommand == 1)
		{
			if ((lCommandPos + 2) > lCommandEnd || (lDataPos + 2) > aSrcBytes)
				return 0;
			lCount = Iff_ReadBigU16(&apSrc[lCommandPos]);
			lCommandPos += 2;
			lValue = Iff_ReadBigU16(&apSrc[lDataPos]);
			lDataPos += 2;

			while (lCount--)
			{
				if (lWordsDone >= lPlaneWords)
					break;
				Iff_ILBM_WriteVDATWord(apDst, aLineBytes, aHeight, aPlane, &lColumn, &lY, lValue);
				lWordsDone++;
			}
		}
		else if (lCommand < 0)
		{
			lCount = (U16)-lCommand;
			while (lCount--)
			{
				if (lWordsDone >= lPlaneWords)
					break;
				if ((lDataPos + 2) > aSrcBytes)
					return 0;
				lValue = Iff_ReadBigU16(&apSrc[lDataPos]);
				lDataPos += 2;
				Iff_ILBM_WriteVDATWord(apDst, aLineBytes, aHeight, aPlane, &lColumn, &lY, lValue);
				lWordsDone++;
			}
		}
		else
		{
			lCount = (U16)lCommand;
			if ((lDataPos + 2) > aSrcBytes)
				return 0;
			lValue = Iff_ReadBigU16(&apSrc[lDataPos]);
			lDataPos += 2;

			while (lCount--)
			{
				if (lWordsDone >= lPlaneWords)
					break;
				Iff_ILBM_WriteVDATWord(apDst, aLineBytes, aHeight, aPlane, &lColumn, &lY, lValue);
				lWordsDone++;
			}
		}
	}

	return 1;
}


static U8 Iff_ILBM_DecodeVDATToSTLow(const sIffIlbm * apIlbm, void * apDst, const U16 aWordCount, const U32 aLineBytes)
{
	const U8 * lpChunk;
	const U8 * lpEnd;
	U8 lPlane;

	Memory_Clear(aLineBytes * apIlbm->mHeight, apDst);

	lpChunk = apIlbm->mpBody;
	lpEnd = apIlbm->mpBody + apIlbm->mBodyLength;

	for (lPlane = 0; lPlane < apIlbm->mPlaneCount; lPlane++)
	{
		U32 lID;
		U32 lLength;
		const U8 * lpData;

		if ((lpChunk + 8) > lpEnd)
			return 0;

		lID = Iff_ReadBigU32(&lpChunk[0]);
		lLength = Iff_ReadBigU32(&lpChunk[4]);
		lpData = &lpChunk[8];

		if (lID != dIFF_VDAT_ID || (lpData + lLength) > lpEnd)
			return 0;
		if (!Iff_ILBM_DecodeVDATPlane(lpData, lLength, apDst, aWordCount, apIlbm->mHeight, aLineBytes, lPlane))
			return 0;

		lpChunk = lpData + ((lLength + 1UL) & ~1UL);
	}

	return 1;
}


U16 Iff_RGB8ToSTE(const U8 aR, const U8 aG, const U8 aB)
{
	return dIFF_RGB8_TO_STE(aR, aG, aB);
}


U8 Iff_ILBM_Parse(const void * apIff, sIffIlbm * apIlbm)
{
	const U8 * lpBase;
	const U8 * lpChunk;
	const U8 * lpEnd;
	U32 lFormID;
	U32 lFormLength;
	U32 lFormType;
	U8 lHaveBmhd;

	if (!apIff || !apIlbm)
		return 0;

	Memory_Clear(sizeof(sIffIlbm), apIlbm);

	lpBase = (const U8 *)apIff;
	lFormID = Iff_ReadBigU32(&lpBase[0]);
	lFormLength = Iff_ReadBigU32(&lpBase[4]);
	lFormType = Iff_ReadBigU32(&lpBase[8]);

	if (lFormID != dIFF_FORM_ID || lFormType != dIFF_ILBM_ID)
		return 0;
	if (lFormLength < 4)
		return 0;

	lpChunk = &lpBase[12];
	lpEnd = &lpBase[8 + lFormLength];
	lHaveBmhd = 0;

	while ((lpChunk + 8) <= lpEnd)
	{
		U32 lID;
		U32 lLength;
		const U8 * lpData;
		U32 lPaddedLength;

		lID = Iff_ReadBigU32(&lpChunk[0]);
		lLength = Iff_ReadBigU32(&lpChunk[4]);
		lpData = &lpChunk[8];
		lPaddedLength = (lLength + 1UL) & ~1UL;

		if ((lpData + lLength) > lpEnd)
			return 0;

		switch (lID)
		{
		case dIFF_BMHD_ID:
			if (lLength >= 20)
			{
				apIlbm->mWidth                  = Iff_ReadBigU16(&lpData[0]);
				apIlbm->mHeight                 = Iff_ReadBigU16(&lpData[2]);
				apIlbm->mX                      = Iff_ReadBigU16(&lpData[4]);
				apIlbm->mY                      = Iff_ReadBigU16(&lpData[6]);
				apIlbm->mPlaneCount             = lpData[8];
				apIlbm->mMask                   = lpData[9];
				apIlbm->mCompressedFlag         = lpData[10];
				apIlbm->mTransparentColourIndex = lpData[12];
				apIlbm->mAspectX                = lpData[14];
				apIlbm->mAspectY                = lpData[15];
				apIlbm->mPageWidth              = Iff_ReadBigU16(&lpData[16]);
				apIlbm->mPageHeight             = Iff_ReadBigU16(&lpData[18]);
				lHaveBmhd = 1;
			}
			break;

		case dIFF_CMAP_ID:
			apIlbm->mpCmap = (const sIffColour *)lpData;
			apIlbm->mColourCount = (U16)(lLength / sizeof(sIffColour));
			break;

		case dIFF_CAMG_ID:
			if (lLength >= 4)
				apIlbm->mViewportMode = Iff_ReadBigU32(lpData);
			break;

		case dIFF_BODY_ID:
			apIlbm->mpBody = lpData;
			apIlbm->mBodyLength = lLength;
			break;

		default:
			break;
		}

		lpChunk = lpData + lPaddedLength;
	}

	return (U8)(lHaveBmhd && apIlbm->mpBody);
}


U16 Iff_ILBM_CmapToSTE(const sIffIlbm * apIlbm, U16 * apPalette, const U16 aColourLimit)
{
	U16 i;
	U16 lCount;

	if (!apIlbm || !apPalette || !apIlbm->mpCmap)
		return 0;

	lCount = apIlbm->mColourCount;
	if (lCount > aColourLimit)
		lCount = aColourLimit;

	for (i = 0; i < lCount; i++)
	{
		apPalette[i] = Iff_RGB8ToSTE(apIlbm->mpCmap[i].mR, apIlbm->mpCmap[i].mG, apIlbm->mpCmap[i].mB);
	}
	return lCount;
}


U8 Iff_ILBM_DecodeToSTLow(const sIffIlbm * apIlbm, void * apDst)
{
	const U8 * lpSrc;
	U8 * lpDst;
	U8 * lpLine;
	U16 lRowBytes;
	U16 lWordCount;
	U32 lLineBytes;
	U16 y;

	if (!apIlbm || !apDst || !apIlbm->mpBody)
		return 0;
	if (apIlbm->mPlaneCount != 4)
		return 0;
	if (apIlbm->mCompressedFlag != dIFF_COMP_NONE && apIlbm->mCompressedFlag != dIFF_COMP_BYTERUN1 && apIlbm->mCompressedFlag != dIFF_COMP_VDAT)
		return 0;
	if ((apIlbm->mCompressedFlag != dIFF_COMP_VDAT && apIlbm->mMask != 0) || (apIlbm->mCompressedFlag == dIFF_COMP_VDAT && apIlbm->mMask > 1))
		return 0;

	lWordCount = (U16)((apIlbm->mWidth + 15U) >> 4);
	lRowBytes = (U16)(lWordCount << 1);
	lLineBytes = (U32)lRowBytes * apIlbm->mPlaneCount;
	lpSrc = apIlbm->mpBody;
	lpDst = (U8 *)apDst;
	lpLine = 0;

	if (apIlbm->mCompressedFlag == dIFF_COMP_VDAT)
		return Iff_ILBM_DecodeVDATToSTLow(apIlbm, apDst, lWordCount, lLineBytes);

	if (!apIlbm->mCompressedFlag)
	{
		if (apIlbm->mBodyLength < (lLineBytes * apIlbm->mHeight))
			return 0;

		for (y = 0; y < apIlbm->mHeight; y++)
		{
			Iff_Interleave4PlaneLine(lpSrc, lpDst, lWordCount);
			lpSrc += lLineBytes;
			lpDst += lLineBytes;
		}
		return 1;
	}

	lpLine = (U8 *)mMEMALLOC(lLineBytes);
	if (!lpLine)
		return 0;

	for (y = 0; y < apIlbm->mHeight; y++)
	{
		U8 plane;
		U8 * lpPlane;

		lpPlane = lpLine;
		for (plane = 0; plane < apIlbm->mPlaneCount; plane++)
		{
			lpSrc = ByteRun1_DePackLine(lpSrc, lpPlane, lRowBytes);
			lpPlane += lRowBytes;
		}
		Iff_Interleave4PlaneLine(lpLine, lpDst, lWordCount);
		lpDst += lLineBytes;
	}

	mMEMFREE(lpLine);
	return 1;
}


/* ################################################################################ */
