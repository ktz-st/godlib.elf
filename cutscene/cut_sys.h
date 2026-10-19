#ifndef	INCLUDED_CUT_SYS_H
#define	INCLUDED_CUT_SYS_H

/* ###################################################################################
#  INCLUDES
################################################################################### */

#include	<godlib/base/base.h>
#include	<godlib/cutscene/cutscene.h>
#include	<godlib/hashtree/hashtree.h>


/* ###################################################################################
#  DEFINES
################################################################################### */


/* ###################################################################################
#  STRUCTS
################################################################################### */


/* ###################################################################################
#  PROTOTYPES
################################################################################### */

void		CutScene_System_AppInit( void );
void		CutScene_System_AppDeInit( void );
/* Stop script and its audio before skip/restart/free. AMIXER must be initialized
 * by the application and reserved exclusively for cutscene samples. */
void CutScene_System_Stop(void);
U8 CutScene_System_IsSamplePlaying(const sCutSample * apSample);

U8			CutScene_System_Update( void );
void		CutScene_System_Render( void );

void		CutScene_System_ScriptInit( sCutScene * apCut, char * apScriptName );
void		CutScene_System_SetpHashTree( sHashTree * apTree );
sHashTree *	CutScene_System_GetpHashTree( void );
U8			CutScene_System_IsScriptFinished( void );


/* ################################################################################ */

#endif	/* INCLUDED_CUT_SYS_H */
