**************************************************************************************
*	AMIXER_S.S
*
*	AUDIO mixer functions
*
*	[c] 2001 Reservoir Gods
**************************************************************************************

;	Mixer frequencies:
;	Frequency : Bytes Per Frame @ 50hz (mono/stereo)
;	 6258     :  125 /  250
;	12517     :  250 /  500
;	25033     :  500 / 1000
;	50066     : 1001 / 2002

**************************************************************************************
;	xdefS / IMPORTS
**************************************************************************************

	xdef	AudioMixer_Vbl

	xdef	gpAudioMixerBuffer
	xdef	gpAudioMixerSilence
	xdef	gpAudioMixerMulTable
	xdef	gAudioMixerLockFlag
	xdef	gAudioMixerBufferOffset
	xdef	gAudioMixerSamples
	xdef	gAudioMixerEnableFlag

	xref	gAudioMixerSineLaw

**************************************************************************************
;	EQUATES
**************************************************************************************

eAMIXER_BUFFER_SIZE		EQU	(8*1024)
eAMIXER_PLAY_OFFSET		EQU	(6*1024)
eAMIXER_CHANNEL_LIMIT	EQU	2


**************************************************************************************
;	STRUCTS
**************************************************************************************

	rsreset

sAmixerSpl_mpStart:      rs.l    1
sAmixerSpl_mpCurrent:    rs.l    1
sAmixerSpl_mpEnd:        rs.l    1
sAmixerSpl_mLength:      rs.l    1
sAmixerSpl_mGainLeft:    rs.b    1
sAmixerSpl_mGainRight:   rs.b    1
sAmixerSpl_mVolume:      rs.b    1
sAmixerSpl_mActiveFlag:  rs.b    1
sAmixerSpl_msizeof:      rs.b    1

**************************************************************************************
	TEXT
**************************************************************************************

*------------------------------------------------------------------------------------*
* FUNCTION : Audio_MixerVbl
* ACTION   : interrupt based mixer
* CREATION : 11.04.01 PNK
*------------------------------------------------------------------------------------*

AudioMixer_Vbl:
	tas		gAudioMixerLockFlag			; already in a mixer interrupt?
	bne		.locked						; yes, don't try further mixing

	movem.l	d0-a6,-(a7)					; save registers

	lea		gAudioMixerSamples,a0
	lea		sAmixerSpl_msizeof(a0),a1

	moveq	#0,d3
	move.w	d3,d4

	moveq	#0,d0
	moveq	#0,d1
	move.b	sAmixerSpl_mGainLeft(a0),d0
	move.b	sAmixerSpl_mGainRight(a0),d1
	cmp.w	#96,d0
	blt		.noL0
	or.w	#$FF00,d3
.noL0:
	cmp.w	#96,d1
	blt		.noR0
	or.w	#$00FF,d3
.noR0:

	moveq	#0,d0
	moveq	#0,d1
	move.b	sAmixerSpl_mGainLeft(a1),d0
	move.b	sAmixerSpl_mGainRight(a1),d1
	cmp.w	#96,d0
	blt		.noL1
	or.w	#$FF00,d4
.noL1:
	cmp.w	#96,d1
	blt		.noR1
	or.w	#$00FF,d4
.noR1:

	lea		AudioMixer_DoMixingO,a3
	move.w	d3,d0
	eor.w	d4,d0
	bne.s	.mixO
	lea		AudioMixer_DoMixingI,a3
.mixO:


	move.l	sAmixerSpl_mpCurrent(a0),a0
	move.l	sAmixerSpl_mpCurrent(a1),a1

	movea.w	#$8909,a2					; dma sound frame ptr
	movep.l	(a2),d7						; read address
	lsr.l	#8,d7						; ignore frame end address high byte

	move.l	gpAudioMixerBuffer,a2			; start of mixing buffer
	sub.l	a2,d7							; current h/w offset into buffer
	move.l	gAudioMixerBufferOffset,d6		; end of last s/w mix
	and.l	#(eAMIXER_BUFFER_SIZE-1),d6		; clip to buffer size
	lea		(a2,d6.l),a2					; get to place in buffer

	and.l	#(eAMIXER_BUFFER_SIZE-1)&$FFFFFFF8,d7	; offset moves in steps of 8
	move.l	d7,gAudioMixerBufferOffset				; end of new s/w mix
	sub.l	d6,d7									; mix length
	bpl.s	.lpls									; do straight linear mix

	move.l	#(eAMIXER_BUFFER_SIZE),d0
	add.l	d0,d7
	sub.l	d6,d0
	jsr		(a3)

	move.l	gpAudioMixerBuffer,a2
	move.l	gAudioMixerBufferOffset,d0
	jsr		(a3)
	bra.s	.update
.lpls:
	move.l	d7,d0
	jsr		(a3)

.update:
	move.l	d7,d0
	bsr		AudioMixer_UpdateSamples

	clr.b	gAudioMixerLockFlag			; signal end of mixing
	movem.l	(a7)+,d0-a6					; restore registers

.locked:
	rts

*------------------------------------------------------------------------------------*
* FUNCTION : Audio_DoMixingI( U8 * apSpl0, U8 * apSpl1, U8 * apBuffer, U32 aBytes )
* ACTION   : interrupt based mixer
* CREATION : 11.04.01 PNK
*------------------------------------------------------------------------------------*

AudioMixer_DoMixingI:

	lsr.l	#3,d0						; d0 = number of 8-byte blocks
	beq.s	.nomix
	lsr.l	#1,d0						; d0 = pair count, X = odd block
	bcc.s	.nopre

	move.w	(a0)+,d1					; leftover single 8-byte block
	move.w	(a1)+,d2
	and.w	d3,d1
	and.w	d4,d2
	move.w	d1,(a2)+
	move.w	d2,(a2)+
	move.w	d1,(a2)+
	move.w	d2,(a2)+

.nopre:
	subq.w	#1,d0
	bmi.s	.nomix

.loop:
	move.w	(a0)+,d1					; byte of sample0
	move.w	(a1)+,d2					; byte of sample1
	and.w	d3,d1
	and.w	d4,d2

	move.w	d1,(a2)+					; write L.R into buffer
	move.w	d2,(a2)+					; write L.R into buffer
	move.w	d1,(a2)+					; write L.R into buffer
	move.w	d2,(a2)+					; write L.R into buffer

	move.w	(a0)+,d1
	move.w	(a1)+,d2
	and.w	d3,d1
	and.w	d4,d2

	move.w	d1,(a2)+
	move.w	d2,(a2)+
	move.w	d1,(a2)+
	move.w	d2,(a2)+

	dbra	d0,.loop

.nomix:
	rts


*------------------------------------------------------------------------------------*
* FUNCTION : Audio_DoMixingI( U8 * apSpl0, U8 * apSpl1, U8 * apBuffer, U32 aBytes )
* ACTION   : interrupt based mixer
* CREATION : 11.04.01 PNK
*------------------------------------------------------------------------------------*

AudioMixer_DoMixingO:

	lsr.l	#3,d0						; d0 = number of 8-byte blocks
	beq.s	.nomix
	lsr.l	#1,d0						; d0 = pair count, X = odd block
	bcc.s	.nopre

	move.w	(a0)+,d1					; leftover single 8-byte block
	move.w	(a1)+,d2
	and.w	d3,d1
	and.w	d4,d2
	or.w	d2,d1
	move.w	d1,(a2)+
	move.w	d1,(a2)+
	move.w	d1,(a2)+
	move.w	d1,(a2)+

.nopre:
	subq.w	#1,d0
	bmi.s	.nomix

.loop:
	move.w	(a0)+,d1					; byte of sample0
	move.w	(a1)+,d2					; byte of sample1
	and.w	d3,d1
	and.w	d4,d2
	or.w	d2,d1

	move.w	d1,(a2)+					; write L.R into buffer
	move.w	d1,(a2)+					; write L.R into buffer
	move.w	d1,(a2)+					; write L.R into buffer
	move.w	d1,(a2)+					; write L.R into buffer

	move.w	(a0)+,d1
	move.w	(a1)+,d2
	and.w	d3,d1
	and.w	d4,d2
	or.w	d2,d1

	move.w	d1,(a2)+
	move.w	d1,(a2)+
	move.w	d1,(a2)+
	move.w	d1,(a2)+

	dbra	d0,.loop

.nomix:
	rts

*------------------------------------------------------------------------------------*
* FUNCTION : AudioMixer_UpdateSamples( U32 aBytes )
* ACTION   : interrupt based mixer
* CREATION : 11.04.01 PNK
*------------------------------------------------------------------------------------*

AudioMixer_UpdateSamples:

	lsr.l	#3,d0
	add.l	d0,d0
	moveq	#eAMIXER_CHANNEL_LIMIT-1,d1				; we have one sample per channel
	move.l	gpAudioMixerSilence,a1					; go back to silence sample if queue is empty
	lea		gAudioMixerSamples,a0					; start of samples

.loop:

	tst.b	sAmixerSpl_mActiveFlag(a0)				; is this sample active?
	beq.s	.next									; no, goto next

.active:
	move.l	sAmixerSpl_mpCurrent(a0),d2				; current sample pointer
	add.l	d0,d2									; add offset
	move.l	d2,sAmixerSpl_mpCurrent(a0)				; store updated sample pointer
	cmp.l	sAmixerSpl_mpEnd(a0),d2					; reached cached end ptr?
	blt.s	.next									; still bytes to play

	move.l	a1,sAmixerSpl_mpStart(a0)				; switch sample to silence
	move.l	a1,sAmixerSpl_mpCurrent(a0)				; point current pointer to silence
	move.l	a1,d2									; silence end ptr
	add.l	#1024,d2
	move.l	d2,sAmixerSpl_mpEnd(a0)					; cache silence end
	clr.b	sAmixerSpl_mActiveFlag(a0)				; mark sample as free
	clr.b	sAmixerSpl_mVolume(a0)					; volume = 0
	clr.b	sAmixerSpl_mGainLeft(a0)				; gain.left = 0
	clr.b	sAmixerSpl_mGainRight(a0)				; gain.right = 0

.next:
	lea		sAmixerSpl_msizeof(a0),a0				; next sample in the array
	dbra	d1,.loop								; loop for all samples

	rts


**************************************************************************************
	DATA
**************************************************************************************

gpAudioMixerBuffer:			dc.l	0
gpAudioMixerSilence:		dc.l	0
gpAudioMixerMulTable:		dc.l	0
gAudioMixerBufferOffset:	dc.l	0
gAudioMixerLockFlag:		dc.b	0
gAudioMixerEnableFlag:		dc.b	0


**************************************************************************************
	BSS
**************************************************************************************
	section  .bss,bss
gAudioMixerSamples:			ds.b	(sAmixerSpl_msizeof*eAMIXER_CHANNEL_LIMIT)
