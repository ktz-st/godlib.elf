/* Compare host CUT output with the actual 68000 parser and relocator. */
#include <godlib/platform/platform.h>
#include <godlib/gemdos/gemdos.h>
#include <godlib/cutscene/cutparse.h>
#include <godlib/memory/memory.h>
#include <godlib/string/string.h>
extern char CutBuild_Source[],CutBuild_SourceEnd[];
extern U8 CutBuild_Binary[],CutBuild_BinaryEnd[];
volatile U16 gCutBuildDone,gCutBuildError,gCutBuildChecks;
static void check(U16 ok){++gCutBuildChecks;if(!ok && !gCutBuildError)gCutBuildError=gCutBuildChecks;}
static void same(const char *a,const char *b){check(a&&b&&!String_StrCmp(a,b));}
static U16 eq(const char *a,const char *b){return a&&b&&!String_StrCmpi(a,b);}
static void *find(void *array,U16 count,U16 size,const char *name)
{U16 i;for(i=0;i<count;++i){U8 *p=(U8*)array+(U32)i*size;if(eq(*(char**)p,name))return p;}return 0;}
static const char *refname(U32 ptr){return ptr?*(char**)ptr:0;}
static void compare(sCutScene *a,sCutScene *b)
{
 U16 i,j;
 check(a->mVersion==3&&b->mVersion==3);
 check(a->mAnimationCount==b->mAnimationCount);check(a->mAssetCount==b->mAssetCount);
 check(a->mPageCount==b->mPageCount);check(a->mSampleCount==b->mSampleCount);
 check(a->mScriptCount==b->mScriptCount);check(a->mSpriteCount==b->mSpriteCount);
 check(a->mTextCount==b->mTextCount);check(a->mVarCount==b->mVarCount);
 for(i=0;i<a->mAnimationCount;++i){sCutAnimation *x=&a->mpAnimations[i],*y=find(b->mpAnimations,b->mAnimationCount,sizeof(*y),x->mpName);check(y!=0);if(y){same(x->mpName,y->mpName);check(x->mSpeed==y->mSpeed);check(x->mFrame0==y->mFrame0);check(x->mFrame1==y->mFrame1);check(x->mFlagLoop==y->mFlagLoop);check(x->mFlagPingPong==y->mFlagPingPong);}}
 for(i=0;i<a->mAssetCount;++i){sCutAsset *x=&a->mpAssets[i],*y=find(b->mpAssets,b->mAssetCount,sizeof(*y),x->mpName);check(y!=0);if(y){same(x->mpName,y->mpName);same(x->mpFileName,y->mpFileName);same(x->mpContext,y->mpContext);check(!y->mpData);}}
 for(i=0;i<a->mPageCount;++i){sCutPage *x=&a->mpPages[i],*y=find(b->mpPages,b->mPageCount,sizeof(*y),x->mpName);check(y!=0);if(y){same(x->mpName,y->mpName);same(x->mpText,y->mpText);}}
 for(i=0;i<a->mSampleCount;++i){sCutSample *x=&a->mpSamples[i],*y=find(b->mpSamples,b->mSampleCount,sizeof(*y),x->mpName);check(y!=0);if(y){same(x->mpName,y->mpName);same(refname((U32)x->mpAsset),refname((U32)y->mpAsset));check(x->mVolume==y->mVolume);check(x->mStereoPos==y->mStereoPos);check(x->mFlagLoop==y->mFlagLoop);}}
 for(i=0;i<a->mSpriteCount;++i){sCutSprite *x=&a->mpSprites[i],*y=find(b->mpSprites,b->mSpriteCount,sizeof(*y),x->mpName);check(y!=0);if(y){same(x->mpName,y->mpName);same(refname((U32)x->mInitialDef.mpAsset),refname((U32)y->mInitialDef.mpAsset));same(refname((U32)x->mInitialDef.mpAnimation),refname((U32)y->mInitialDef.mpAnimation));check(x->mInitialDef.mSpeed==y->mInitialDef.mSpeed);check(x->mInitialDef.mX==y->mInitialDef.mX);check(x->mInitialDef.mY==y->mInitialDef.mY);}}
 for(i=0;i<a->mTextCount;++i){sCutText *x=&a->mpTexts[i],*y=find(b->mpTexts,b->mTextCount,sizeof(*y),x->mpName);check(y!=0);if(y){same(x->mpName,y->mpName);same(refname((U32)x->mInitialDef.mpAsset),refname((U32)y->mInitialDef.mpAsset));same(refname((U32)x->mInitialDef.mpPage),refname((U32)y->mInitialDef.mpPage));check(x->mInitialDef.mRect.mX==y->mInitialDef.mRect.mX);check(x->mInitialDef.mRect.mY==y->mInitialDef.mRect.mY);check(x->mInitialDef.mRect.mWidth==y->mInitialDef.mRect.mWidth);check(x->mInitialDef.mRect.mHeight==y->mInitialDef.mRect.mHeight);}}
 for(i=0;i<a->mVarCount;++i){sCutVar *x=&a->mpVars[i],*y=find(b->mpVars,b->mVarCount,sizeof(*y),x->mpName);check(y!=0);if(y){same(x->mpName,y->mpName);same(x->mpVarName,y->mpVarName);}}
 for(i=0;i<a->mScriptCount;++i){sCutScript *x=&a->mpScripts[i],*y=find(b->mpScripts,b->mScriptCount,sizeof(*y),x->mpName);check(y!=0);if(y){same(x->mpName,y->mpName);check(x->mCommandCount==y->mCommandCount);if(x->mCommandCount!=y->mCommandCount)continue;for(j=0;j<x->mCommandCount;++j){sCutCommand *u=&x->mpCommands[j],*v=&y->mpCommands[j];sCutCmdDef *d=CutScene_CmdDefFindByID(u->mOp);U16 k;U32 *p=&u->mArg0,*q=&v->mArg0;U16 types[3]={d->mArg0,d->mArg1,d->mArg2};check(u->mOp==v->mOp);for(k=0;k<3;++k){if(types[k]>=eCUT_CMD_ARG_ASSET){same(refname(p[k]),refname(q[k]));}else check(p[k]==q[k]);}}}}
}
void __attribute__((noinline)) CutBuild_Marker(void){__asm__ volatile("" ::: "memory");}
S16 GodLib_Game_Main(S16 argc,char **argv)
{
 sCutScene *native,*host;U8 *copy;U32 size=(U32)(CutBuild_BinaryEnd-CutBuild_Binary),i;
 (void)argc;(void)argv;GemDos_Super(0);Platform_Init();
 check(sizeof(sCutScene)==56);check(sizeof(sCutAsset)==64);check(sizeof(sCutVar)==36);check(sizeof(sCutAnimation)==14);check(sizeof(sCutScript)==10);check(sizeof(sCutCommand)==14);check(sizeof(sCutSample)==12);check(sizeof(sCutSprite)==76);check(sizeof(sCutText)==66);
 copy=mMEMALLOC(size);native=CutScene_Parse(CutBuild_Source,(U32)(CutBuild_SourceEnd-CutBuild_Source));check(copy!=0&&native!=0);
 if(copy&&native){Memory_Copy(size,CutBuild_Binary,copy);host=(sCutScene*)copy;CutScene_Relocate(host);compare(native,host);check(CutScene_GetSize(host)==size);CutScene_Delocate(host);for(i=0;i<size;++i)if(copy[i]!=CutBuild_Binary[i])break;check(i==size);CutScene_Relocate(host);compare(native,host);}
 if(native)mMEMFREE(native);if(copy)mMEMFREE(copy);Platform_DeInit();gCutBuildDone=1;for(;;)CutBuild_Marker();return 0;
}
