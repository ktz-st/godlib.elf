**************************************************************************************
*	IFF_S.S
*
*	IFF ILBM conversion routines
**************************************************************************************

	xdef	Iff_Interleave4PlaneLine

	text

*------------------------------------------------------------------------------------*
* FUNCTION : Iff_Interleave4PlaneLine
* ACTION   : converts one ILBM 4-plane scanline to ST interleaved planar words
* INPUT    : a0 = source line, a1 = destination line, d0 = word count
*            source: p0 words, p1 words, p2 words, p3 words
*            dest:   p0,p1,p2,p3, p0,p1,p2,p3...
*------------------------------------------------------------------------------------*

Iff_Interleave4PlaneLine:
	movem.l	d1/a2-a4,-(sp)

	move.w	d0,d1
	beq.s	.done
	add.w	d0,d0			; row bytes
	lea	(a0,d0.w),a2		; plane 1
	lea	(a2,d0.w),a3		; plane 2
	lea	(a3,d0.w),a4		; plane 3
	subq.w	#1,d1

.loop:
	move.w	(a0)+,(a1)+
	move.w	(a2)+,(a1)+
	move.w	(a3)+,(a1)+
	move.w	(a4)+,(a1)+
	dbra	d1,.loop

.done:
	movem.l	(sp)+,d1/a2-a4
	rts
