/* ###################################################################################
#  INCLUDES
################################################################################### */

#include	"screen.h"

#include	<godlib/debug/dbgchan.h>
#include	<godlib/font/font.h>
#include	<godlib/ikbd/ikbd.h>
#include	<godlib/memory/memory.h>
#include	<godlib/system/system.h>
#include	<godlib/vbl/vbl.h>
#include	<godlib/video/video.h>




/* ###################################################################################
#  DATA
################################################################################### */

sGraphicCanvas	gScreenBackGraphic;
sGraphicCanvas	gScreenLogicGraphic;
sGraphicCanvas	gScreenPhysicGraphic;
sGraphicCanvas	gScreenMiscGraphic;

sScreenClass	gScreenClass;


/* ###################################################################################
#  CODE
################################################################################### */

/*-----------------------------------------------------------------------------------*
* FUNCTION : Screen_Init( const U16 aWidth,const U16 aHeight,const U16 aBitDepth, const U16 aScrollFlags )
* ACTION   : Screen_Init
* CREATION : 01.04.2005 PNK
*-----------------------------------------------------------------------------------*/

static U8 Screen_Allocate(U16 width,U16 height,U16 mode,U16 virtualWidth,U16 virtualHeight,U16 flags)
{
 U32 size, allocation, base;
 Memory_Clear(sizeof(sScreenClass),&gScreenClass);
 GraphicCanvas_Init(&gScreenLogicGraphic,mode,virtualWidth,virtualHeight);
 GraphicCanvas_Init(&gScreenPhysicGraphic,mode,virtualWidth,virtualHeight);
 GraphicCanvas_Init(&gScreenBackGraphic,mode,virtualWidth,virtualHeight);
 GraphicCanvas_Init(&gScreenMiscGraphic,mode,virtualWidth,virtualHeight);
 size=gScreenLogicGraphic.mpLineOffsets[1]*(U32)virtualHeight;
 /* Every page starts on a 256-byte boundary, including wider canvases. */
 allocation=(size+255UL)&~255UL;
 base=(U32)mMEMSCREENCALLOC(allocation*3UL+255UL);
 gScreenClass.mpMemBase=(U16*)base;
 if(!base) {
  GraphicCanvas_DeInit(&gScreenLogicGraphic); GraphicCanvas_DeInit(&gScreenPhysicGraphic);
  GraphicCanvas_DeInit(&gScreenBackGraphic); GraphicCanvas_DeInit(&gScreenMiscGraphic);
  return 0;
 }
 base=(base+255UL)&~255UL;
 gScreenClass.mpBuffers[eSCREEN_PHYSIC]=(U16*)base;
 gScreenClass.mpBuffers[eSCREEN_LOGIC]=(U16*)(base+allocation);
 gScreenClass.mpBuffers[eSCREEN_BACK]=(U16*)(base+allocation*2UL);
 gScreenLogicGraphic.mpVRAM=gScreenClass.mpBuffers[eSCREEN_LOGIC];
 gScreenPhysicGraphic.mpVRAM=gScreenClass.mpBuffers[eSCREEN_PHYSIC];
 gScreenBackGraphic.mpVRAM=gScreenClass.mpBuffers[eSCREEN_BACK];
 gScreenMiscGraphic.mpVRAM=0;
 gScreenClass.mViewportWidth=width; gScreenClass.mViewportHeight=height;
 gScreenClass.mScrollFlags=flags;
 gScreenClass.mFrameRate=1; gScreenClass.mFirstTimeFlag=1;
 Video_SetResolution(width,height,mode,virtualWidth);
 Screen_Update();
 return 1;
}

void Screen_Init(const U16 width,const U16 height,const U16 mode,const U16 flags)
{
 U16 active=flags&eSCREEN_SCROLL_V;
 U16 virtualWidth=width,virtualHeight=height;
 U16 viewportWidth=width,viewportHeight=height;
 /* ST-low always displays 320x200; dimensions describe the backing canvas. */
 if(mode==eGRAPHIC_COLOURMODE_4PLANE) {
  viewportWidth=320; viewportHeight=200;
  if((flags&eSCREEN_SCROLL_H) && System_GetVDO()==VDO_STE) {
   active|=eSCREEN_SCROLL_H;
   if(virtualWidth==320) virtualWidth+=32;
  }
 }
 /* Preserve the original vertical-scroll allocation, including height+32. */
 if(active&eSCREEN_SCROLL_V) {
  if(virtualHeight>65503U) { Memory_Clear(sizeof(sScreenClass),&gScreenClass); return; }
  virtualHeight+=32;
 }
 if(mode==eGRAPHIC_COLOURMODE_4PLANE
  && (virtualWidth<320 || virtualHeight<200 || (virtualWidth&15)
   || virtualWidth>1328 || virtualHeight>2047
   || (virtualWidth>320 && !(active&eSCREEN_SCROLL_H)))) {
  Memory_Clear(sizeof(sScreenClass),&gScreenClass);
  return;
 }
 Screen_Allocate(viewportWidth,viewportHeight,mode,virtualWidth,virtualHeight,active);
}

void Screen_SetScrollX(U16 x)
{
 U16 limit=0;
 if(gScreenClass.mScrollFlags&eSCREEN_SCROLL_H)
  limit=gScreenLogicGraphic.mWidth-gScreenClass.mViewportWidth;
 gScreenClass.mScrollX=x>limit?limit:x;
}

/*-----------------------------------------------------------------------------------*
* FUNCTION : Screen_DeInit( void )
* ACTION   : Screen_DeInit
* CREATION : 01.04.2005 PNK
*-----------------------------------------------------------------------------------*/

void	Screen_DeInit( void )
{
	GraphicCanvas_DeInit( &gScreenLogicGraphic );
	GraphicCanvas_DeInit( &gScreenPhysicGraphic );
	GraphicCanvas_DeInit( &gScreenBackGraphic );
	GraphicCanvas_DeInit( &gScreenMiscGraphic );
	mMEMSCREENFREE( gScreenClass.mpMemBase );
	gScreenClass.mpMemBase = 0;
}


/*-----------------------------------------------------------------------------------*
* FUNCTION : Screen_Update( void )
* ACTION   : Screen_Update
* CREATION : 01.04.2005 PNK
*-----------------------------------------------------------------------------------*/

void	Screen_Update( void )
{
	uU32	lScrn;
	U32		lOff;
	S32		lVbls;
#ifdef	dGODLIB_PLATFORM_ATARI
	U16		lOldBack;
	U16 *	lpReg;
	U16		lResFlag;
#endif

	if( gScreenClass.mFirstTimeFlag )
	{
		gScreenClass.mLastVbl       = Vbl_GetCounter();
		gScreenClass.mFirstTimeFlag = 0;
	}

	gScreenClass.mPhysicIndex ^= 1;
	lScrn.l = ((U32)gScreenClass.mpBuffers[ gScreenClass.mPhysicIndex ]);

 {
  U16 limit=gScreenLogicGraphic.mHeight-gScreenClass.mViewportHeight;
  if(gScreenClass.mScrollY>limit) gScreenClass.mScrollY=limit;
 }
	if( gScreenClass.mScrollY )
	{
		lOff     = gScreenLogicGraphic.mpLineOffsets[ 1 ];
		lOff    *= gScreenClass.mScrollY;
		lScrn.l += lOff;
	}

 if(gScreenClass.mScrollFlags&eSCREEN_SCROLL_H) {
  Screen_SetScrollX(gScreenClass.mScrollX);
  lScrn.l += (U32)(gScreenClass.mScrollX>>4)*8UL;
#ifdef dGODLIB_PLATFORM_ATARI
  Video_SetViewportSTE((U16*)lScrn.l,gScreenClass.mScrollX&15);
#else
  Video_SetPhysic((U16*)lScrn.l);
#endif
 }
 else Video_SetPhysic((U16*)lScrn.l);

	if( System_GetMCH() == MCH_ST )
	{
		lScrn.w.w0 >>= 8;
#ifdef	dGODLIB_PLATFORM_ATARI
		*(U32*)0xFFFF8200L = lScrn.l;
#endif
	}

#ifdef	dGODLIB_PLATFORM_ATARI
	lpReg = (U16*)0xFFFF8240L;
	lOldBack = lpReg[ 0 ];

	if( IKBD_GetKeyStatus( eIKBDSCAN_TAB ) )
	{
		lpReg[ 0 ] = lOldBack ^ 0xFFF;
		lResFlag = 1;
	}
	else
	{
		lResFlag = 0;
	}
#endif

	do
	{
		Vbl_WaitVbl();
		lVbls = Vbl_GetCounter() - gScreenClass.mLastVbl;
	}
	while( lVbls < (S32)gScreenClass.mFrameRate );


#ifdef	dGODLIB_PLATFORM_ATARI
	if( lResFlag )
	{
		lpReg[ 0 ] = lOldBack;
	}
#endif

	gScreenClass.mLastVbl = Vbl_GetCounter();

	gScreenLogicGraphic.mpVRAM  = gScreenClass.mpBuffers[ gScreenClass.mPhysicIndex ^ 1 ];
	gScreenPhysicGraphic.mpVRAM = gScreenClass.mpBuffers[ gScreenClass.mPhysicIndex     ];
}


/* ################################################################################ */
