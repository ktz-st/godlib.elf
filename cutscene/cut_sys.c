/* ###################################################################################
#  INCLUDES
################################################################################### */

#include	"cut_sys.h"
#include <godlib/audio/amixer.h>
#include <godlib/vbl/vbl.h>

#include	<godlib/cli/cli.h>
#include	<godlib/fade/fade.h>
#include	<godlib/font/font.h>
#include	<godlib/memory/memory.h>
#include	<godlib/screen/screen.h>
#include	<godlib/pictypes/degas.h>
#include	<godlib/sprite/sprite.h>
#include	<godlib/string/string.h>

#ifndef dGODLIB_COMPILER_AHCC
#include	<math.h>
#endif

/* ###################################################################################
#  ENUMS
################################################################################### */

enum
{
	eCUTTHREAD_WAIT_NONE,

	eCUTTHREAD_WAIT_ANIM,
	eCUTTHREAD_WAIT_MOVE,
	eCUTTHREAD_WAIT_FADE,
	eCUTTHREAD_WAIT_VBL,
	eCUTTHREAD_WAIT_SAMPLE,
 eCUTTHREAD_WAIT_BG,
 eCUTTHREAD_WAIT_TEXT,

	eCUTTHREAD_WAIT_LIMIT
};


/* ###################################################################################
#  DEFINES
################################################################################### */

#define	dCUTSCENE_STACK_LIMIT	16


/* ###################################################################################
#  STRUCTS
################################################################################### */

typedef	struct	sCutScenePC
{
	sCutScript *	mpScript;
	U16				mLine;
} sCutScenePC;

typedef	struct	sCutSceneThread
{
	sCutScenePC	mStack[ dCUTSCENE_STACK_LIMIT ];
	sCutScenePC	mPC;
	U16			mStackIndex;
	U16			mWaitMode;
	U32			mWaitData;
} sCutSceneThread;

typedef	struct	sCutSceneSysClass
{
	sCutScene *		mpScene;
	sCutAsset *		mpPal;
	sCutAsset *		mpBGAsset;
	sHashTree *		mpTree;
	sCutSceneThread	mThread;
	U16				mBGRedrawFlag;
	U16				mBGEnableFlag;
 U16 mBGBackDirty,mFinished;
} sCutSceneSysClass;


/* ###################################################################################
#  DATA
################################################################################### */

sCutSceneSysClass	gCutSceneSysClass;


/* ###################################################################################
#  PROTOTYPES
################################################################################### */

void	CutScene_System_SpriteMoveUpdate( sCutSprite * apSprite );
void	CutScene_System_SpriteAnimUpdate( sCutSprite * apSprite );
void	CutScene_System_SpriteMoveInit( sCutSprite * apSprite, S16 aTargetX, S16 aTargetY );
S32		CutSceneSys_Sqrt( S32 aR );

U32		CutSceneThread_Update( sCutSceneThread * apThread );
U16		CutSceneThread_IsWaiting( sCutSceneThread * apThread );

/* ###################################################################################
#  CODE
################################################################################### */

/* Runtime text state uses the existing CUT fields: source-byte offset and
 * update countdown. Keep the serialized structure and file version unchanged. */
static U16 CutScene_TextIsWaiting(const sCutText *text)
{
 const char *source;
 if(!text || !text->mEnabledFlag || !text->mCurrentDef.mpPage)return 0;
 source=text->mCurrentDef.mpPage->mpText;
 if(!source || !*source)return 0;
 return (text->mWordPauseConstant && source[text->mOffset]) || (text->mRedrawFlag & 3);
}
static void CutScene_TextAnimUpdate(sCutText *text)
{
 const char *source;
 if(!text->mEnabledFlag || !text->mWordPauseConstant || !text->mCurrentDef.mpPage)return;
 source=text->mCurrentDef.mpPage->mpText;
 if(!source || !source[text->mOffset])return;
 if(text->mWordPauseCounter) --text->mWordPauseCounter;
 if(!text->mWordPauseCounter) {
  ++text->mOffset;text->mWordPauseCounter=text->mWordPauseConstant;
  text->mRedrawFlag=text->mUndrawFlag=3;
 }
}
static void CutScene_BackgroundChanged(sCutSceneThread *thread)
{
 gCutSceneSysClass.mBGRedrawFlag=3;
 gCutSceneSysClass.mBGBackDirty=1;
 thread->mWaitMode=eCUTTHREAD_WAIT_BG;thread->mWaitData=0;
}

/* No audio allocation or DMA ownership changes unless SAMPLE commands succeed.
 * A named sample has at most one voice. A third sample steals round-robin. */
static const sCutSample *sCutSamples[2];
static U16 sCutNextVoice,sCutAudioOwned;
static U32 sCutTail[2];
static void CutScene_StopSamples(void)
{
 if(sCutAudioOwned)AudioMixer_StopAll();
 sCutAudioOwned=0;
 sCutSamples[0]=sCutSamples[1]=0;sCutNextVoice=0;sCutTail[0]=sCutTail[1]=0;
}
U8 CutScene_System_IsSamplePlaying(const sCutSample *sample)
{
 U16 i;if(!sample)return 0;
 for(i=0;i<2;++i)if(sCutSamples[i]==sample) {
  if(AudioMixer_IsVoicePlaying(i)){sCutTail[i]=0;return 1;}
  /* AMIXER queues about 41 ms. Do not cut off its final spoken syllable. */
  if(!sCutTail[i])sCutTail[i]=Vbl_GetCounter()+3;
  if((S32)(sCutTail[i]-Vbl_GetCounter())>0)return 1;
 }
 return 0;
}
static void CutScene_SampleDisable(const sCutSample *sample)
{
 U16 i;for(i=0;i<2;++i)if(sCutSamples[i]==sample){AudioMixer_StopVoice(i);sCutSamples[i]=0;}
}
static void CutScene_SampleEnable(const sCutSample *sample)
{
 U16 i,voice=sCutNextVoice; sAudioDmaSound sound;
 if(!sample || !sample->mpAsset || !sample->mpAsset->mpData)return;
 CutScene_SampleDisable(sample);
 for(i=0;i<2;++i)if(!AudioMixer_IsVoicePlaying(i)){voice=i;break;}
 sound=*(const sAudioDmaSound*)sample->mpAsset->mpData;
 sound.mLoopingFlag=sample->mFlagLoop;
 if(AudioMixer_PlayVoice(voice,&sound,sample->mVolume,sample->mStereoPos)) {
  sCutAudioOwned=1;sCutSamples[voice]=sample;sCutTail[voice]=0;sCutNextVoice=voice^1;
 }
}
void CutScene_System_Stop(void)
{
 CutScene_StopSamples();
 gCutSceneSysClass.mpScene=0;
 Memory_Clear(sizeof(gCutSceneSysClass.mThread),&gCutSceneSysClass.mThread);
 gCutSceneSysClass.mFinished=1;
}

/*-----------------------------------------------------------------------------------*
* FUNCTION : CutScene_System_AppInit( void )
* ACTION   : CutScene_System_AppInit
* CREATION : 22.04.2005 PNK
*-----------------------------------------------------------------------------------*/

void	CutScene_System_AppInit( void )
{
	CutScene_System_Stop();
 Memory_Clear( sizeof(sCutSceneSysClass), &gCutSceneSysClass );
 gCutSceneSysClass.mFinished=1;
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : CutScene_System_AppDeInit( void )
* ACTION   : CutScene_System_AppDeInit
* CREATION : 22.04.2005 PNK
*-----------------------------------------------------------------------------------*/

void	CutScene_System_AppDeInit( void )
{
 CutScene_System_Stop();
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : CutScene_System_Update( void )
* ACTION   : CutScene_System_Update
* CREATION : 22.04.2005 PNK
*-----------------------------------------------------------------------------------*/

U8	CutScene_System_Update( void )
{
	return( (U8)CutSceneThread_Update( &gCutSceneSysClass.mThread ) );
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : CutSceneThread_IsWaiting( sCutSceneThread * apThread )
* ACTION   : CutSceneThread_IsWaiting
* CREATION : 30.04.2005 PNK
*-----------------------------------------------------------------------------------*/

U16	CutSceneThread_IsWaiting( sCutSceneThread * apThread )
{
	sCutSprite * lpSprite;
	U16			lRes;

	lRes = 0;

	switch( apThread->mWaitMode )
	{
	case eCUTTHREAD_WAIT_BG:
  lRes=gCutSceneSysClass.mBGRedrawFlag!=0;break;
 case eCUTTHREAD_WAIT_TEXT:
  lRes=CutScene_TextIsWaiting((const sCutText*)apThread->mWaitData);break;
 case eCUTTHREAD_WAIT_SAMPLE:
  lRes=CutScene_System_IsSamplePlaying((const sCutSample*)apThread->mWaitData);
  break;
 case	eCUTTHREAD_WAIT_ANIM:
		lpSprite = (sCutSprite*)apThread->mWaitData;
		if( lpSprite->mAnimFrame.w.w1 != lpSprite->mCurrentDef.mpAnimation->mFrame1 )
		{
			lRes = 1;
		}
		break;
	case	eCUTTHREAD_WAIT_MOVE:
		lpSprite = (sCutSprite*)apThread->mWaitData;
		if( lpSprite->mPos.w.w1 < lpSprite->mDist )
		{
			lRes = 1;
		}
		break;
	case	eCUTTHREAD_WAIT_FADE:
		if( !Fade_IsVblFadeFinished() )
		{
			lRes = 1;
		}
		break;
	case	eCUTTHREAD_WAIT_VBL:
		if( apThread->mWaitData )
		{
			apThread->mWaitData--;
			lRes = 1;
		}
		break;
	}

	return( lRes );
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : CutSceneThread_Update( sCutSceneThread * apThread )
* ACTION   : CutSceneThread_Update
* CREATION : 30.04.2005 PNK
*-----------------------------------------------------------------------------------*/

U32	CutSceneThread_Update( sCutSceneThread * apThread )
{
	U8				lRes;
	U8				lDone;
	U16				i;
	sCutAsset *		lpAss;
	sCutCommand *	lpCmd;
	sCutText *		lpText;
	sCutSprite *	lpSprite;
	sCutVar *		lpVar;

	if(!gCutSceneSysClass.mpScene || gCutSceneSysClass.mFinished || !apThread->mPC.mpScript)return 1;

	lRes  = 0;
	lDone = 0;

	for( i=0; i<gCutSceneSysClass.mpScene->mSpriteCount; i++ )
	{
		CutScene_System_SpriteMoveUpdate( &gCutSceneSysClass.mpScene->mpSprites[ i ] );
		CutScene_System_SpriteAnimUpdate( &gCutSceneSysClass.mpScene->mpSprites[ i ] );
	}

 for(i=0;i<gCutSceneSysClass.mpScene->mTextCount;++i)
  CutScene_TextAnimUpdate(&gCutSceneSysClass.mpScene->mpTexts[i]);

	i=0;
 while( !lDone )
	{
		if(++i>1024){gCutSceneSysClass.mFinished=1;CutScene_StopSamples();return 1;}
  if( CutSceneThread_IsWaiting( apThread ) )
		{
			lDone = 1;
		}
		else
		{
			apThread->mWaitMode = eCUTTHREAD_WAIT_NONE;
			if(  apThread->mPC.mLine < apThread->mPC.mpScript->mCommandCount )
			{
				lpCmd = &apThread->mPC.mpScript->mpCommands[ apThread->mPC.mLine ];
				switch( lpCmd->mOp )
				{
				case	eCUT_CMD_CALL:

					if(!lpCmd->mArg0 || apThread->mStackIndex>=dCUTSCENE_STACK_LIMIT) {
      gCutSceneSysClass.mFinished=1;CutScene_StopSamples();return 1;
     }
     apThread->mStack[ apThread->mStackIndex ] = apThread->mPC;
					apThread->mStackIndex++;

					apThread->mPC.mpScript = (sCutScript*)lpCmd->mArg0;
					apThread->mPC.mLine    = 0;
					break;

				case	eCUT_CMD_CALLBACK:
					lRes  = 2;
					lDone = 1;
					break;

				case	eCUT_CMD_BG_GFX_CLEAR:
					gCutSceneSysClass.mpBGAsset = 0;
					CutScene_BackgroundChanged(apThread);
					break;

				case	eCUT_CMD_BG_GFX_SET:
					gCutSceneSysClass.mpBGAsset = (sCutAsset*)lpCmd->mArg0;
					CutScene_BackgroundChanged(apThread);
					break;

				case	eCUT_CMD_BG_GFX_ENABLE:
					if(!gCutSceneSysClass.mBGEnableFlag) {
      gCutSceneSysClass.mBGEnableFlag=1;CutScene_BackgroundChanged(apThread);
     }
					break;
				case	eCUT_CMD_BG_GFX_DISABLE:
					if(gCutSceneSysClass.mBGEnableFlag) {
      gCutSceneSysClass.mBGEnableFlag=0;CutScene_BackgroundChanged(apThread);
     }
					break;

				case	eCUT_CMD_FADE_SET:
					lpAss = (sCutAsset*)lpCmd->mArg0;
					if( lpAss )
					{
						if( lpAss->mpData )
						{
							Fade_StartVblFade( (U16*)lpAss->mpData, (U16)lpCmd->mArg1 );
						}
					}
					break;
				case	eCUT_CMD_FADE_TOBLACK:
					Fade_StartVblFade( Fade_GetpBlackPal(), (U16)lpCmd->mArg0 );
					break;
				case	eCUT_CMD_FADE_TOPI1:
					lpAss = (sCutAsset*)lpCmd->mArg0;
					if( lpAss )
					{
						sDegas *	lpDegas;

						lpDegas = (sDegas*)lpAss->mpData;
						if( lpDegas )
						{
							Fade_StartVblFade( lpDegas->mHeader.mPalette, (U16)lpCmd->mArg1 );
						}
					}
					break;
				case	eCUT_CMD_FADE_TOWHITE:
					Fade_StartVblFade( Fade_GetpWhitePal(), (U16)lpCmd->mArg0 );
					break;
				case	eCUT_CMD_FADE_WAIT:
					apThread->mWaitMode = eCUTTHREAD_WAIT_FADE;
					apThread->mWaitData = 0;
					break;

				case	eCUT_CMD_RETURN:
					if( apThread->mStackIndex )
					{
						apThread->mStackIndex--;
						apThread->mPC = apThread->mStack[ apThread->mStackIndex ];
					}
					else
					{
						lDone = 1;
						lRes  = 1;
      gCutSceneSysClass.mFinished=1;CutScene_StopSamples();
					}
					break;

				case eCUT_CMD_SAMPLE_ENABLE:
     CutScene_SampleEnable((const sCutSample*)lpCmd->mArg0);break;
    case eCUT_CMD_SAMPLE_DISABLE:
     CutScene_SampleDisable((const sCutSample*)lpCmd->mArg0);break;
    case eCUT_CMD_SAMPLE_WAIT:
     apThread->mWaitMode=eCUTTHREAD_WAIT_SAMPLE;apThread->mWaitData=lpCmd->mArg0;break;

				case	eCUT_CMD_SPRITE_ANIM_SET:
					lpSprite                          = (sCutSprite*)lpCmd->mArg0;
					lpSprite->mCurrentDef.mpAnimation = (sCutAnimation*)lpCmd->mArg1;
					lpSprite->mAnimFrame.w.w0         = 0;
					lpSprite->mAnimFrame.w.w1         = lpSprite->mCurrentDef.mpAnimation->mFrame0;
					lpSprite->mAnimDir                = 1;
					break;

				case	eCUT_CMD_SPRITE_ANIM_WAIT:
					apThread->mWaitMode = eCUTTHREAD_WAIT_ANIM;
					apThread->mWaitData = lpCmd->mArg0;
					break;

				case	eCUT_CMD_SPRITE_DISABLE:
					lpSprite                          = (sCutSprite*)lpCmd->mArg0;
					lpSprite->mEnabledFlag = 0;
					break;

				case	eCUT_CMD_SPRITE_ENABLE:
					lpSprite                          = (sCutSprite*)lpCmd->mArg0;
					lpSprite->mEnabledFlag = 1;
					break;

				case	eCUT_CMD_SPRITE_GFX_SET:
					lpSprite                      = (sCutSprite*)lpCmd->mArg0;
					lpSprite->mCurrentDef.mpAsset = (sCutAsset*)lpCmd->mArg1;
					break;

				case	eCUT_CMD_SPRITE_SPEED_SET:
					lpSprite                     = (sCutSprite*)lpCmd->mArg0;
					lpSprite->mCurrentDef.mSpeed = lpCmd->mArg1;
					break;

				case	eCUT_CMD_SPRITE_MOVE_TO:
					CutScene_System_SpriteMoveInit( (sCutSprite*)lpCmd->mArg0, (S16)lpCmd->mArg1, (S16)lpCmd->mArg2 );
					break;

				case	eCUT_CMD_SPRITE_MOVE_WAIT:
					apThread->mWaitMode = eCUTTHREAD_WAIT_MOVE;
					apThread->mWaitData = lpCmd->mArg0;
					break;

				case	eCUT_CMD_SPRITE_POS_SET:
					lpSprite                    = (sCutSprite*)lpCmd->mArg0;
					lpSprite->mCurrentDef.mX    = (S16)lpCmd->mArg1;
					lpSprite->mCurrentDef.mY    = (S16)lpCmd->mArg2;
					lpSprite->mSrcX = lpSprite->mDstX = lpSprite->mCurrentDef.mX;
					lpSprite->mSrcY = lpSprite->mDstY = lpSprite->mCurrentDef.mY;
					lpSprite->mPos.l = 0;
					lpSprite->mDist  = 0;
					break;

    case eCUT_CMD_TEXT_ANIM_SET:
     lpText=(sCutText*)lpCmd->mArg0;
     if(lpText) {
      S32 delay=(S32)lpCmd->mArg1;
      lpText->mWordPauseConstant=delay<=0?0:(delay>65535L?65535:(U16)delay);
      lpText->mWordPauseCounter=lpText->mWordPauseConstant;
      lpText->mOffset=0;
      lpText->mRedrawFlag=lpText->mUndrawFlag=3;
     }
     break;
    case eCUT_CMD_TEXT_ANIM_WAIT:
     apThread->mWaitMode=eCUTTHREAD_WAIT_TEXT;apThread->mWaitData=lpCmd->mArg0;
     break;

				case	eCUT_CMD_TEXT_DISABLE:
					lpText                     = (sCutText*)lpCmd->mArg0;
					lpText->mEnabledFlag       = 0;
     lpText->mOffset=0;lpText->mWordPauseCounter=lpText->mWordPauseConstant;
					lpText->mRedrawFlag        = 0;
					lpText->mUndrawFlag        = 3;
					break;

				case	eCUT_CMD_TEXT_ENABLE:
					lpText                     = (sCutText*)lpCmd->mArg0;
					if(!lpText->mEnabledFlag) {
      lpText->mOffset=0;lpText->mWordPauseCounter=lpText->mWordPauseConstant;
     }
     lpText->mEnabledFlag       = 1;
					lpText->mRedrawFlag        = 3;
					lpText->mUndrawFlag        = 3;
					break;

				case	eCUT_CMD_TEXT_FONT_SET:
					lpText                      = (sCutText*)lpCmd->mArg0;
					lpText->mCurrentDef.mpAsset = (sCutAsset*)lpCmd->mArg1;
     lpText->mRedrawFlag=lpText->mUndrawFlag=3;
					break;

				case	eCUT_CMD_TEXT_PAGE_SET:
					lpText                     = (sCutText*)lpCmd->mArg0;
					lpText->mCurrentDef.mpPage = (sCutPage*)lpCmd->mArg1;
					lpText->mOffset            = 0;
     lpText->mWordPauseCounter=lpText->mWordPauseConstant;
     lpText->mRedrawFlag=lpText->mUndrawFlag=3;
					break;

				case	eCUT_CMD_VAR_SET:
					lpVar                     = (sCutVar*)lpCmd->mArg0;
					HashTree_VarWrite( lpVar->mVarClient.mpVar, &lpCmd->mArg1 );
					break;

				case	eCUT_CMD_WAIT:
					apThread->mWaitMode = eCUTTHREAD_WAIT_VBL;
					apThread->mWaitData = lpCmd->mArg0;
					break;
				}

				if( eCUT_CMD_CALL != lpCmd->mOp )
				{
					apThread->mPC.mLine++;
				}
			} else {
    if(apThread->mStackIndex){apThread->mPC=apThread->mStack[--apThread->mStackIndex];apThread->mPC.mLine++;}
    else {gCutSceneSysClass.mFinished=1;CutScene_StopSamples();return 1;}
   }
		}
	}
	return( lRes );
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : CutScene_System_ScriptInit( sCutScene * apCut,char * apScriptName )
* ACTION   : CutScene_System_ScriptInit
* CREATION : 24.04.2005 PNK
*-----------------------------------------------------------------------------------*/

void	CutScene_System_ScriptInit( sCutScene * apCut,char * apScriptName )
{
	
	U16	i;

	CutScene_System_Stop();
 if( apCut && apScriptName )
 {
  gCutSceneSysClass.mpScene = apCut;
  gCutSceneSysClass.mpBGAsset=0;
  gCutSceneSysClass.mBGRedrawFlag=3;gCutSceneSysClass.mBGBackDirty=1;
  gCutSceneSysClass.mBGEnableFlag=1;
		gCutSceneSysClass.mThread.mWaitMode = eCUTTHREAD_WAIT_NONE;

		for( i=0; i<apCut->mSpriteCount; i++ )
		{
			sCutSprite * lpSprite;

			lpSprite = &apCut->mpSprites[ i ];

			lpSprite->mCurrentDef = lpSprite->mInitialDef;
			lpSprite->mSrcX  = lpSprite->mDstX = lpSprite->mInitialDef.mX;
			lpSprite->mSrcY  = lpSprite->mDstY = lpSprite->mInitialDef.mY;
			lpSprite->mPos.l = 0;
			lpSprite->mDist  = 0;
			lpSprite->mAnimFrame.l = 0;
			lpSprite->mAnimDir     = 1;
   lpSprite->mEnabledFlag=0;
   Memory_Clear(sizeof(lpSprite->mDirtyRects),lpSprite->mDirtyRects);
   if(lpSprite->mCurrentDef.mpAnimation)lpSprite->mAnimFrame.w.w1=lpSprite->mCurrentDef.mpAnimation->mFrame0;
		}

		for( i=0; i<apCut->mTextCount; i++ )
		{
			apCut->mpTexts[ i ].mCurrentDef = apCut->mpTexts[ i ].mInitialDef;
   apCut->mpTexts[i].mEnabledFlag=apCut->mpTexts[i].mRedrawFlag=apCut->mpTexts[i].mUndrawFlag=0;
   apCut->mpTexts[i].mOffset=0;
   apCut->mpTexts[i].mWordPauseConstant=apCut->mpTexts[i].mWordPauseCounter=0;
   Memory_Clear(sizeof(apCut->mpTexts[i].mDirtyRects),apCut->mpTexts[i].mDirtyRects);
		}

		i = 0;
		while( (i<apCut->mScriptCount) && (String_StrCmpi( apScriptName, apCut->mpScripts[ i ].mpName ) ) )
		{
			i++;
		}
		if( i < apCut->mScriptCount )
		{
			gCutSceneSysClass.mThread.mPC.mLine    = 0;
			gCutSceneSysClass.mThread.mPC.mpScript = &apCut->mpScripts[ i ];
			gCutSceneSysClass.mThread.mStackIndex  = 0;
   gCutSceneSysClass.mFinished=0;
		}
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : void CutScene_System_SpriteMoveUpdate(sCutSprite * apSprite)
* ACTION   : CutScene_System_SpriteMoveUpdate
* CREATION : 26.04.2005 PNK
*-----------------------------------------------------------------------------------*/

void CutScene_System_SpriteMoveUpdate(sCutSprite * apSprite)
{
	S32	lDiff;

	if( apSprite->mPos.w.w1 < apSprite->mDist )
	{
		apSprite->mPos.l += apSprite->mCurrentDef.mSpeed;
		if( apSprite->mPos.w.w1 >= apSprite->mDist )
		{
			apSprite->mPos.l = 0;
			apSprite->mDist  = 0;
			apSprite->mCurrentDef.mX = apSprite->mDstX;
			apSprite->mCurrentDef.mY = apSprite->mDstY;
			apSprite->mSrcX = apSprite->mDstX;
			apSprite->mSrcY = apSprite->mDstY;
		}
		else
		{
			lDiff = apSprite->mDstX - apSprite->mSrcX;
			lDiff = lDiff * apSprite->mPos.w.w1;
			lDiff = lDiff / apSprite->mDist;
			lDiff = lDiff + apSprite->mSrcX;
			apSprite->mCurrentDef.mX = (S16)lDiff;

			lDiff = apSprite->mDstY - apSprite->mSrcY;
			lDiff = lDiff * apSprite->mPos.w.w1;
			lDiff = lDiff / apSprite->mDist;
			lDiff = lDiff + apSprite->mSrcY;
			apSprite->mCurrentDef.mY = (S16)lDiff;
		}
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : void CutScene_System_SpriteAnimUpdate(sCutSprite * apSprite)
* ACTION   : CutScene_System_SpriteAnimUpdate
* CREATION : 27.04.2005 PNK
*-----------------------------------------------------------------------------------*/

void CutScene_System_SpriteAnimUpdate(sCutSprite * apSprite)
{
	sCutAnimation *	lpAnim;

	lpAnim = apSprite->mCurrentDef.mpAnimation;

	if( lpAnim )
	{
		if( apSprite->mAnimDir )
		{
			apSprite->mAnimFrame.l += lpAnim->mSpeed;
		}
		else
		{
			apSprite->mAnimFrame.l -= lpAnim->mSpeed;
		}

		if( apSprite->mAnimFrame.w.w1 < 0 )
		{
			if( lpAnim->mFlagPingPong )
			{
				if( lpAnim->mFrame1 )
				{
					apSprite->mAnimFrame.w.w1 = 1;
				}
				else
				{
					apSprite->mAnimFrame.w.w1 = 0;
				}
			}
			else
			{
				apSprite->mAnimFrame.w.w1 = 0;
			}
			apSprite->mAnimDir = 1;
		}

		if( apSprite->mAnimFrame.w.w1 > lpAnim->mFrame1 )
		{
			if( lpAnim->mFlagLoop )
			{
				apSprite->mAnimFrame.w.w1 = 0;
			}
			else if( lpAnim->mFlagPingPong )
			{
				apSprite->mAnimDir = 0;
				apSprite->mAnimFrame.w.w1 = lpAnim->mFrame1;
				if( apSprite->mAnimFrame.w.w1 )
				{
					apSprite->mAnimFrame.w.w1--;
				}
			}
			else
			{
				apSprite->mAnimFrame.w.w1 = lpAnim->mFrame1;
			}
		}
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : void CutScene_System_SpriteMoveInit(sCutSprite * apSprite,S16 aTargetX,S16 aTargetY)
* ACTION   : CutScene_System_SpriteMoveInit
* CREATION : 26.04.2005 PNK
*-----------------------------------------------------------------------------------*/

void CutScene_System_SpriteMoveInit(sCutSprite * apSprite,S16 aTargetX,S16 aTargetY)
{
	S32	lDistX;
	S32	lDistY;

	apSprite->mSrcX = apSprite->mCurrentDef.mX;
	apSprite->mSrcY = apSprite->mCurrentDef.mY;
	apSprite->mDstX = aTargetX;
	apSprite->mDstY = aTargetY;

	lDistX = apSprite->mDstX - apSprite->mSrcX;
	lDistX = lDistX * lDistX;

	lDistY = apSprite->mDstY - apSprite->mSrcY;
	lDistY = lDistY * lDistY;

	lDistX = lDistX + lDistY;

	lDistX = CutSceneSys_Sqrt( lDistX );

	apSprite->mPos.l = 0;
	apSprite->mDist  = (S16)lDistX;
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : S32 CutSceneSys_Sqrt(S32 aR)
* ACTION   : CutSceneSys_Sqrt
* CREATION : 26.04.2005 PNK
*-----------------------------------------------------------------------------------*/

S32 CutSceneSys_Sqrt(S32 aR)
{
	S32	lT,lB,lC;

	lC = 0;

	for( lB=0x10000000L; lB!=0; lB>>=2 )
	{
		lT   = lC + lB;
		lC >>= 1L;
		if( lT <= aR)
		{
			aR -= lT;
			lC += lB;
		}
	}

	return( lC );
}


static void CutScene_TextDamage(sCutScene *scene,const sGraphicRect *damage,U16 page)
{
 U16 i;for(i=0;i<scene->mTextCount;++i) {
  sCutText *t=&scene->mpTexts[i];sCutRect *r=&t->mCurrentDef.mRect;
  if(t->mEnabledFlag && damage->mWidth>0 && damage->mHeight>0 &&
   damage->mX<r->mX+r->mWidth && damage->mX+damage->mWidth>r->mX &&
   damage->mY<r->mY+r->mHeight && damage->mY+damage->mHeight>r->mY)t->mRedrawFlag|=1U<<page;
 }
}

/* Layout only; all glyph rasterization uses GodLib's clipped ASM font API. */
static void CutScene_TextDraw(sGraphicCanvas *canvas,sCutText *text,sFont *font)
{
 const char *src,*begin,*wordStart;char word[128];U16 n,shown;S16 width,advance;
 U32 visible=text->mWordPauseConstant?text->mOffset:0xffffffffUL;
 sGraphicPos pos; sGraphicBox saved=canvas->mClipBox;
 sCutRect *r=&text->mCurrentDef.mRect;
 if(!text->mCurrentDef.mpPage || !text->mCurrentDef.mpPage->mpText || r->mWidth<=0 || r->mHeight<=0)return;
 src=begin=text->mCurrentDef.mpPage->mpText;pos.mX=r->mX;pos.mY=r->mY;
 if(canvas->mClipBox.mX0<r->mX)canvas->mClipBox.mX0=r->mX;
 if(canvas->mClipBox.mY0<r->mY)canvas->mClipBox.mY0=r->mY;
 if(canvas->mClipBox.mX1>r->mX+r->mWidth)canvas->mClipBox.mX1=r->mX+r->mWidth;
 if(canvas->mClipBox.mY1>r->mY+r->mHeight)canvas->mClipBox.mY1=r->mY+r->mHeight;
 if(canvas->mClipBox.mX1<=canvas->mClipBox.mX0 || canvas->mClipBox.mY1<=canvas->mClipBox.mY0){canvas->mClipBox=saved;return;}
 advance=font->mHeightMax+2;
 while(*src && pos.mY<r->mY+r->mHeight && (U32)(src-begin)<visible) {
  if(*src=='\r' || *src=='\n') {
   char c=*src++;if(c=='\r' && *src=='\n')src++;
   pos.mX=r->mX;pos.mY+=advance;continue;
  }
  if((U8)*src<=' ') {
   if(pos.mX!=r->mX)pos.mX+=(font->mSpaceWidth+font->mKerning)*(*src=='\t'?4:1);
   src++;continue;
  }
  wordStart=src;n=0;while((U8)*src>' ' && n<sizeof(word)-1)word[n++]=*src++;
  word[n]=0;width=Font_GetStringWidth(font,word);
  if(pos.mX!=r->mX && pos.mX+width>r->mX+r->mWidth){pos.mX=r->mX;pos.mY+=advance;}
  /* Lay out the complete word before clipping its visible prefix. Otherwise
   * partially revealed words jump to the next line when they grow. */
  shown=n;
  if(visible-(U32)(wordStart-begin)<shown)shown=(U16)(visible-(U32)(wordStart-begin));
  word[shown]=0;
  if(shown && pos.mY<r->mY+r->mHeight)canvas->mpClipFuncs->FontPrint(canvas,&pos,font,word);
  if(shown<n)break;
  pos.mX+=width+font->mKerning;
 }
 canvas->mClipBox=saved;
}

/*-----------------------------------------------------------------------------------*
* FUNCTION : CutScene_System_Render( void )
* ACTION   : CutScene_System_Render
* CREATION : 22.04.2005 PNK
*-----------------------------------------------------------------------------------*/

void	CutScene_System_Render( void )
{
	sCutScene *			lpCut;
	sCutSprite *		lpSprite;
	sCutText *			lpText;
	sFont *				lpFont;
	sSpriteBlock *		lpSprBlock;
	sGraphicCanvas *	lpLogic;
	sGraphicCanvas *	lpBack;
	sGraphicPos			lPos;
	sGraphicRect		lRect;
	U16					lLogicIndex;
	U16					i;


	lpCut   = gCutSceneSysClass.mpScene;
	if( lpCut )
	{
		lpBack  = Screen_GetpBackGraphic();
		lpLogic = Screen_GetpLogicGraphic();
		lLogicIndex = Screen_GetLogicIndex();

  if(gCutSceneSysClass.mBGBackDirty) {
   sDegas *picture=(gCutSceneSysClass.mBGEnableFlag && gCutSceneSysClass.mpBGAsset)?(sDegas*)gCutSceneSysClass.mpBGAsset->mpData:0;
   if(picture)Screen_Back_CopyScreen(picture->mPixels);else Screen_Back_ClearScreen();
   gCutSceneSysClass.mBGBackDirty=0;
  }
  if(gCutSceneSysClass.mBGRedrawFlag & (1U<<lLogicIndex)) {
   Screen_Logic_CopyScreen(lpBack->mpVRAM);
   gCutSceneSysClass.mBGRedrawFlag &= ~(1U<<lLogicIndex);
   for(i=0;i<lpCut->mTextCount;++i)lpCut->mpTexts[i].mRedrawFlag|=1U<<lLogicIndex;
  }

		for( i=0; i<lpCut->mSpriteCount; i++ )
		{
			lpSprite    = &lpCut->mpSprites[ i ];

			lPos.mX       = lpSprite->mDirtyRects[ lLogicIndex ].mX;
			lPos.mY       = lpSprite->mDirtyRects[ lLogicIndex ].mY;
			lRect.mX      = lPos.mX;
			lRect.mY      = lPos.mY;
			lRect.mWidth  = lpSprite->mDirtyRects[ lLogicIndex ].mWidth;
			lRect.mHeight = lpSprite->mDirtyRects[ lLogicIndex ].mHeight;
			if( lRect.mWidth && lRect.mHeight )
			{
				lpLogic->mpClipFuncs->Blit( lpLogic, &lPos, &lRect, lpBack );
				lpSprite->mDirtyRects[ lLogicIndex ].mWidth = 0;
    CutScene_TextDamage(lpCut,&lRect,lLogicIndex);
			}
		}

		for( i=0; i<lpCut->mTextCount; i++ )
		{
			lpText    = &lpCut->mpTexts[ i ];

			if( lpText->mUndrawFlag & (1U<<lLogicIndex) )
			{
				lPos.mX       = lpText->mDirtyRects[ lLogicIndex ].mX;
				lPos.mY       = lpText->mDirtyRects[ lLogicIndex ].mY;
				lRect.mX      = lPos.mX;
				lRect.mY      = lPos.mY;
				lRect.mWidth  = lpText->mDirtyRects[ lLogicIndex ].mWidth;
				lRect.mHeight = lpText->mDirtyRects[ lLogicIndex ].mHeight;
				if( lRect.mWidth && lRect.mHeight )
				{
					lpLogic->mpClipFuncs->Blit( lpLogic, &lPos, &lRect, lpBack );
					lpText->mDirtyRects[ lLogicIndex ].mWidth = 0;
				}
				lpText->mUndrawFlag &= ~(1U<<lLogicIndex);
			}
		}

		for( i=0; i<lpCut->mSpriteCount; i++ )
		{
			lpSprite = &lpCut->mpSprites[ i ];
			if( lpSprite->mEnabledFlag )
			{
				if( lpSprite->mCurrentDef.mpAsset )
				{
					lpSprBlock = (sSpriteBlock*)lpSprite->mCurrentDef.mpAsset->mpData;
					if( lpSprBlock )
					{
						S16	lSprIndex;

						lSprIndex = lpSprite->mAnimFrame.w.w1;

						if( (lSprIndex<0) || (lSprIndex>=(S16)lpSprBlock->mHeader.mSpriteCount) )
						{
							lSprIndex = 0;
						}

						lPos.mX = lpSprite->mCurrentDef.mX;
						lPos.mY = lpSprite->mCurrentDef.mY;
						lpLogic->mpClipFuncs->DrawSprite( lpLogic, &lPos, lpSprBlock->mpSprite[ lSprIndex ] );

						lpSprite->mDirtyRects[ lLogicIndex ].mX = lPos.mX;
						lpSprite->mDirtyRects[ lLogicIndex ].mY = lPos.mY;
						lpSprite->mDirtyRects[ lLogicIndex ].mWidth = lpSprBlock->mpSprite[ lSprIndex ]->mWidth;
						lpSprite->mDirtyRects[ lLogicIndex ].mHeight = lpSprBlock->mpSprite[ lSprIndex ]->mHeight;
      lRect.mX=lPos.mX;lRect.mY=lPos.mY;lRect.mWidth=lpSprBlock->mpSprite[lSprIndex]->mWidth;lRect.mHeight=lpSprBlock->mpSprite[lSprIndex]->mHeight;
      CutScene_TextDamage(lpCut,&lRect,lLogicIndex);
					}
				}
			}
		}

		for( i=0; i<lpCut->mTextCount; i++ )
		{
			lpText = &lpCut->mpTexts[ i ];
			if( lpText->mEnabledFlag )
			{
				if( lpText->mCurrentDef.mpAsset )
				{
					lpFont = (sFont*)lpText->mCurrentDef.mpAsset->mpData;
					if( lpFont )
					{
      if(lpText->mRedrawFlag & (1U<<lLogicIndex)) {
       CutScene_TextDraw(lpLogic,lpText,lpFont);
       lpText->mDirtyRects[lLogicIndex]=lpText->mCurrentDef.mRect;
       lpText->mRedrawFlag &= ~(1U<<lLogicIndex);
      }
					}
				}
			}
		}
	}
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : CutScene_System_SetpHashTree( sHashTree * apTree )
* ACTION   : CutScene_System_SetpHashTree
* CREATION : 27.04.2005 PNK
*-----------------------------------------------------------------------------------*/

void	CutScene_System_SetpHashTree( sHashTree * apTree )
{
	gCutSceneSysClass.mpTree = apTree;
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : CutScene_System_GetpHashTree( void )
* ACTION   : CutScene_System_GetpHashTree
* CREATION : 27.04.2005 PNK
*-----------------------------------------------------------------------------------*/

sHashTree *	CutScene_System_GetpHashTree( void )
{
	return( gCutSceneSysClass.mpTree );
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : CutScene_System_IsScriptFinished( void )
* ACTION   : CutScene_System_IsScriptFinished
* CREATION : 30.04.2005 PNK
*-----------------------------------------------------------------------------------*/

U8	CutScene_System_IsScriptFinished( void )
{
 return !gCutSceneSysClass.mpScene || gCutSceneSysClass.mFinished;
}

/* ################################################################################ */
