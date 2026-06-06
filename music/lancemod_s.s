	xdef	Paula_Init
	xdef	Paula_SetReplayBuf
	xdef	LanceMod_Init
	xdef	LanceMod_Play
	xdef	LanceMod_Stop
	xdef	LanceMod_SetMasterVolume
	xdef	LanceMod_SetRasterDebug
	xdef	LanceMod_GetEndFlag

Paula_Init:
	movem.l	d0-a6,-(a7)
	jsr	paula_init
	movem.l	(a7)+,d0-a6
	rts

Paula_SetReplayBuf:
	move.l	a0,paula_replay_buf_p
	rts

LanceMod_Init:
	movem.l	d0-a6,-(a7)
	jsr	mt_init
	st	mt_Enable
	movem.l	(a7)+,d0-a6
	rts

LanceMod_Play:
	movem.l	d0-a6,-(a7)
	tst.b	LanceMod_RasterDebugFlag
	beq.s	.no_debug
	move.w	$ffff8240.w,LanceMod_RasterDebugColour
	move.w	#$0700,$ffff8240.w
.no_debug:
	jsr	paula_calc
	jsr	mt_music
	tst.b	LanceMod_RasterDebugFlag
	beq.s	.done
	move.w	LanceMod_RasterDebugColour,$ffff8240.w
.done:
	movem.l	(a7)+,d0-a6
	rts

LanceMod_Stop:
	movem.l	d0-a6,-(a7)
	jsr	mt_end
	jsr	paula_done
	movem.l	(a7)+,d0-a6
	rts

LanceMod_SetMasterVolume:
	movem.l	d0-a6,-(a7)
	jsr	mt_setmastervol
	movem.l	(a7)+,d0-a6
	rts

LanceMod_SetRasterDebug:
	move.b	d0,LanceMod_RasterDebugFlag
	rts

LanceMod_GetEndFlag:
	moveq	#0,d0
	move.b	mt_EndMusicTrigger,d0
	rts

	include	"music/lancepaula.s"
	include	"music/lancetracker.s"

	section	data

LanceMod_RasterDebugFlag:
	dc.b	0
	even
LanceMod_RasterDebugColour:
	dc.w	0
