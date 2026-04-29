	xdef	Mod_InitPaula
	xdef	Mod_Init
	xdef	Mod_Play
	xdef	Mod_Stop
	xdef	Mod_SetMasterVolume
	xdef	Mod_SetRasterDebug
	xdef	Mod_GetEndFlag

Mod_InitPaula:
	movem.l	d0-a6,-(a7)
	jsr	paula_init
	movem.l	(a7)+,d0-a6
	rts

Mod_Init:
	movem.l	d0-a6,-(a7)
	jsr	mt_init
	st	mt_Enable
	movem.l	(a7)+,d0-a6
	rts

Mod_Play:
	movem.l	d0-a6,-(a7)
	tst.b	Mod_RasterDebugFlag
	beq.s	.no_debug
	move.w	$ffff8240.w,Mod_RasterDebugColour
	move.w	#$0700,$ffff8240.w
.no_debug:
	jsr	paula_calc
	jsr	mt_music
	tst.b	Mod_RasterDebugFlag
	beq.s	.done
	move.w	Mod_RasterDebugColour,$ffff8240.w
.done:
	movem.l	(a7)+,d0-a6
	rts

Mod_Stop:
	movem.l	d0-a6,-(a7)
	jsr	mt_end
	jsr	paula_done
	movem.l	(a7)+,d0-a6
	rts

Mod_SetMasterVolume:
	movem.l	d0-a6,-(a7)
	jsr	mt_setmastervol
	movem.l	(a7)+,d0-a6
	rts

Mod_SetRasterDebug:
	move.b	d0,Mod_RasterDebugFlag
	rts

Mod_GetEndFlag:
	moveq	#0,d0
	move.b	mt_EndMusicTrigger,d0
	rts

	include	"music/lancepaula.s"
	include	"music/lancetracker.s"

	section	data

Mod_RasterDebugFlag:
	dc.b	0
	even
Mod_RasterDebugColour:
	dc.w	0
