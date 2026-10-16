* PacMad three-channel PCM mixer, extracted into GodLib.
*
* This follows the Puckman mixer model: three source streams are mixed with
* longword adds into the next DMA buffer. Stopped channels point at a silent
* looping buffer, so the hot path always mixes three channels.

	opt	o+

	xdef	PacmadMixer_CoreFill,PacmadMixer_Lock,PacmadMixer_Unlock

PacmadMixer_Lock:
 move.w sr,d0
 ori.w #$0700,sr
 rts
PacmadMixer_Unlock:
 move.w d0,sr
 rts

MIX_SIZE	equ	250
CH_SIZE		equ	20
CH_SAMPLE	equ	0
CH_CURRENT	equ	4
CH_END		equ	8
CH_NEXT		equ	12
CH_LOOP		equ	16
CH_ACTIVE	equ	17
CH_PRIORITY	equ	18

SAMPLE_DATA	equ	0
SAMPLE_LEN	equ	4
SAMPLE_PRIORITY	equ	8

* m68k-atari-mintelf-gcc -mfastcall:
* a0 = sPacmadMixAsmParams *
*   +0 channels
*   +4 destination half-buffer
*   +8 silence buffer
*   +12 U16 byte count (even, <=1024). Remaining source spans use U32.
*   +14 U16 independent-byte saturation enabled.
PacmadMixer_CoreFill:
	movem.l	d0-d7/a0-a6,-(sp)

	move.l	(a0),a6			; channels
	move.l	4(a0),a5		; destination
	move.l	a0,-(sp)		; keep parameter block for arithmetic mode
	move.l	8(a0),a4		; silence
	move.w	12(a0),d7
	beq	.done

.mix_loop:
	bsr	.ensure_channels

	move.l	CH_CURRENT(a6),a1
	move.l	CH_SIZE+CH_CURRENT(a6),a2
	move.l	CH_SIZE*2+CH_CURRENT(a6),a3

	move.l	CH_END(a6),d1
	sub.l	a1,d1
	move.l	CH_SIZE+CH_END(a6),d2
	sub.l	a2,d2
	move.l	CH_SIZE*2+CH_END(a6),d3
	sub.l	a3,d3

	moveq #0,d6
	move.w d7,d6
	cmp.l d1,d6
	bls.s .min_a
	move.l d1,d6
.min_a:
	cmp.l d2,d6
	bls.s .min_b
	move.l d2,d6
.min_b:
	cmp.l d3,d6
	bls.s .have_count
	move.l d3,d6
.have_count:

	tst.w	d6
	beq	.done

	move.w	d6,d4			; save segment length
	move.l (sp),a0
	tst.w 14(a0)
	beq.s .packed_mix
* With at most one active source, adding silent channels is an exact copy.
	moveq #0,d0
	move.b CH_ACTIVE(a6),d0
	add.b CH_SIZE+CH_ACTIVE(a6),d0
	add.b CH_SIZE*2+CH_ACTIVE(a6),d0
	cmp.b #2,d0
	bcs.s .packed_mix
	move.w d4,d5
	subq.w #1,d5
.safe_mix:
	move.b (a1)+,d0
	ext.w d0
	move.b (a2)+,d1
	ext.w d1
	add.w d1,d0
	move.b (a3)+,d2
	ext.w d2
	add.w d2,d0
	cmp.w #127,d0
	ble.s .safe_min
	moveq #127,d0
	bra.s .safe_store
.safe_min:
	cmp.w #-128,d0
	bge.s .safe_store
	moveq #-128,d0
.safe_store:
	move.b d0,(a5)+
	dbra d5,.safe_mix
	bra.s .store_ptrs
.packed_mix:
	move.w	d6,d5
	lsr.w	#2,d5
	beq.s	.byte_mix
	subq.w	#1,d5
.long_mix:
	move.l	(a1)+,d0
	add.l	(a2)+,d0
	add.l	(a3)+,d0
	move.l	d0,(a5)+
	dbra	d5,.long_mix

.byte_mix:
	move.w	d4,d5
	andi.w	#3,d5
	beq.s	.store_ptrs
	subq.w	#1,d5
.byte_mix_loop:
	move.b	(a1)+,d0
	add.b	(a2)+,d0
	add.b	(a3)+,d0
	move.b	d0,(a5)+
	dbra	d5,.byte_mix_loop

.store_ptrs:
	move.l	a1,CH_CURRENT(a6)
	move.l	a2,CH_SIZE+CH_CURRENT(a6)
	move.l	a3,CH_SIZE*2+CH_CURRENT(a6)
	sub.w	d4,d7
	bne	.mix_loop

.done:
	addq.l #4,sp
	movem.l	(sp)+,d0-d7/a0-a6
	rts

.ensure_channels:
	lea.l	(a6),a0
	bsr.s	.ensure_one
	lea.l	CH_SIZE(a6),a0
	bsr.s	.ensure_one
	lea.l	CH_SIZE*2(a6),a0

.ensure_one:
	tst.b	CH_ACTIVE(a0)
	beq.s	.use_silence
	move.l	CH_CURRENT(a0),d0
	cmp.l	CH_END(a0),d0
	bcs.s	.ensure_ok

	tst.b	CH_LOOP(a0)
	beq.s	.not_looping
	move.l	CH_SAMPLE(a0),d0
	beq.s	.use_silence
	move.l	d0,a1
	move.l	SAMPLE_DATA(a1),d1
	move.l	d1,CH_CURRENT(a0)
	add.l	SAMPLE_LEN(a1),d1
	move.l	d1,CH_END(a0)
	bra.s	.ensure_ok

.not_looping:
	move.l	CH_NEXT(a0),d0
	beq.s	.use_silence
	move.l	d0,a1
	move.l	d0,CH_SAMPLE(a0)
	clr.l	CH_NEXT(a0)
	move.l	SAMPLE_DATA(a1),d1
	move.l	d1,CH_CURRENT(a0)
	add.l	SAMPLE_LEN(a1),d1
	move.l	d1,CH_END(a0)
	move.b	SAMPLE_PRIORITY(a1),CH_PRIORITY(a0)
	move.b	#1,CH_ACTIVE(a0)
	bra.s	.ensure_ok

.use_silence:
	clr.l	CH_SAMPLE(a0)
	move.l	a4,CH_CURRENT(a0)
	move.l	a4,d1
	add.l	#MIX_SIZE,d1
	move.l	d1,CH_END(a0)
	clr.l	CH_NEXT(a0)
	move.b	#1,CH_LOOP(a0)
	clr.b	CH_ACTIVE(a0)
	clr.b	CH_PRIORITY(a0)

.ensure_ok:
	rts
