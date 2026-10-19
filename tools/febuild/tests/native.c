/* This fixture is deliberately not initialized/rendered: it tests every v2 field. */
#include <godlib/fe/fed.h>
#include <godlib/font8x8/font8x8.h>
#include <godlib/gemdos/gemdos.h>
#include <godlib/ikbd/ikbd.h>
#include <godlib/platform/platform.h>
#include <godlib/screen/screen.h>
#include <godlib/fade/fade.h>
#include <string.h>
extern U8 Fixture[],FixtureEnd[];
static U8 copy[4096];
volatile U16 gFedFixtureReady,gFedFixturePass,gFedFixtureLoops;
volatile U32 gFedFixtureMismatch;
S16 GodLib_Game_Main(S16 argc,char **argv)
{
 U16 i,quit=0,dirty=2;U32 n,size=(U32)(FixtureEnd-Fixture);static U16 pal[16]={0,0x777};(void)argc;(void)argv;
 GemDos_Super(0);Platform_Init();Screen_Init(320,200,eGRAPHIC_COLOURMODE_4PLANE,0);Fade_SetPalDirect(pal);
 if(size<=sizeof(copy)){
  memcpy(copy,Fixture,size);gFedFixturePass=1;
  for(i=0;i<4;++i){Fed_Relocate((sFedHeader*)Fixture);Fed_Delocate((sFedHeader*)Fixture);++gFedFixtureLoops;
   for(n=0;n<size;++n)if(copy[n]!=Fixture[n]){gFedFixturePass=0;gFedFixtureMismatch=n;break;}
  }
 }
 gFedFixtureReady=1;
 while(!quit){Screen_Update();IKBD_Update();if(dirty){Screen_Logic_ClearScreen();Font8x8_Print(gFedFixturePass?"FED V2 ROUND TRIP / PASS":"FED V2 ROUND TRIP / FAIL",Screen_GetpLogic(),0,16);--dirty;}if(IKBD_GetKeyStatus(eIKBDSCAN_ESC))quit=1;}
 Screen_DeInit();Platform_DeInit();return !gFedFixturePass;
}
