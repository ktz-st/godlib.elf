        opt o-,a-,p-
        xdef WizTest_Mix,WizTest_GetReplay,WizTest_GetTables,WizTest_GetVolume
WizTest_Mix:
        movem.l d0-a6,-(sp)
        move.w sr,-(sp)
        jsr Wizzcat_Vbl
        clr.b $ffff8901.l
        move.w (sp)+,sr
        movem.l (sp)+,d0-a6
        rts
WizTest_GetReplay:
        move.l WIZbackbuf,a0
        move.l a0,d0
        rts
WizTest_GetTables:
        lea itab,a0
        move.l a0,d0
        rts
WizTest_GetVolume:
        move.l vtabptr,a0
        move.l a0,d0
        rts
