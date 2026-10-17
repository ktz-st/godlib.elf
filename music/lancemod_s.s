	xdef	Paula_Init
	xdef	Paula_SetReplayBuf
	xdef	Paula_SetWorkspace,Paula_SetInitScratch
	xdef	LanceMod_Init
	xdef	LanceMod_Play
	xdef	LanceMod_StartDma
	xdef	LanceMod_MixNext
	xdef	Paula_MixOnly
	xdef	Paula_ClearReplay
	xdef	LanceMod_StopMusic
	xdef	LanceMod_Stop
	xdef	LanceMod_SetMasterVolume
	xdef	LanceMod_SetRasterDebug
	xdef	LanceMod_GetVisualizer
	xdef	LanceMod_GetEndFlag

Paula_Init:
	movem.l	d0-a6,-(a7)
	jsr	paula_init
	movem.l	(a7)+,d0-a6
	rts

Paula_SetReplayBuf:
	move.l	a0,paula_replay_buf_p
	rts

Paula_SetInitScratch:
    move.l a0,paula_freq_list
    rts

; a0 -> seven resident workspace pointers, in C layout order.
Paula_SetWorkspace:
    move.l (a0)+,paula_freq_table
    move.l (a0)+,paula_frame_freq_t
    move.l (a0)+,paula_frame_freq_p
    move.l (a0)+,paula_volume_base
    move.l (a0)+,paula_div_table
    move.l (a0)+,paula_mixcode_p
    move.l (a0)+,paula_mixer_code
    rts

LanceMod_Init:
	movem.l	d0-a6,-(a7)
	jsr	mt_init
	move.l	#$40004000,mt_Visualizer
	move.l	#$40004000,mt_Visualizer+4
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

LanceMod_StartDma:
	jsr	paula_start_dma
	rts

LanceMod_MixNext:
	movem.l	d0-a6,-(a7)
	jsr	paula_mix_next
	jsr	mt_music
	movem.l	(a7)+,d0-a6
	rts

Paula_MixOnly:
	jsr	paula_mix_next
	rts

Paula_ClearReplay:
	movem.l	d0-d2/a0-a1,-(a7)
	jsr	paula_clear_buffer
	movem.l	(a7)+,d0-d2/a0-a1
	rts

LanceMod_StopMusic:
	movem.l	d0-a6,-(a7)
	jsr	mt_end
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

LanceMod_GetVisualizer:
	move.l	a0,d0
	beq.s	.return
	move.w	sr,d1
	ori.w	#$0700,sr
	tst.b	mt_Enable
	beq.s	.idle
	move.l	mt_Visualizer,(a0)+
	move.l	mt_Visualizer+4,(a0)
	bra.s	.restore
.idle:
	move.l	#$40004000,(a0)+
	move.l	#$40004000,(a0)
.restore:
	move.w	d1,sr
.return:
	rts


        xdef LanceMod_GetOscilloscope
        xref ModScope_Sample

; A0 -> 4*64 S8 samples; main-loop supervisor API, no retained pointers.
LanceMod_GetOscilloscope:
        move.l a0,d0
        beq .return
        movem.l d2-d7/a2-a6,-(sp)
        move.l a0,a6
        suba.w #96,sp
        move.l sp,a0
        move.w sr,d7
        ori.w #$700,sr
        lea paula_channel_0,a1
        moveq #3,d6
.voice:
        move.l (a1),(a0)+
        move.l 4(a1),(a0)+
        move.l 8(a1),(a0)+
        move.l 4(a1),(a0)+
        move.w 14(a1),(a0)+
        move.w 12(a1),(a0)+
        move.w #1,(a0)+
        moveq #0,d0
        move.b mt_Enable,d0
        move.w d0,(a0)+
        lea 20(a1),a1
        dbf d6,.voice
        move.w d7,sr
        move.l sp,a0
        move.l a6,a1
        move.l #37144344,d0
        jsr ModScope_Sample
        adda.w #96,sp
        movem.l (sp)+,d2-d7/a2-a6
.return:
        rts


        xdef LanceMod_GetStereoOscilloscope
        xref ModScope_CopyStereo
LanceMod_GetStereoOscilloscope:
        move.l a0,d0
        beq.s .return
        move.w sr,-(sp)
        ori.w #$700,sr
        tst.b mt_Enable
        beq.s .idle
        move.l paula_replay_buf_p,d0
        beq.s .idle
        move.w paula_buffer_rd_idx,d0
        lsl.w #3,d0
        lea paula_buffer_str,a1
        move.l (a1,d0.w),a1
        bra.s .copy
.idle:
        suba.l a1,a1
.copy:
        jsr ModScope_CopyStereo
        move.w (sp)+,sr
.return:
        rts

	include	"music/lancepaula.s"
	include	"music/lancetracker.s"

	section	data

LanceMod_RasterDebugFlag:
	dc.b	0
	even
LanceMod_RasterDebugColour:
	dc.w	0
