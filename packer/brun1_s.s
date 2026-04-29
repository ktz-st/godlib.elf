**************************************************************************************
*	BRUN1_S.S
*
*	ByteRun1 decompression routines
**************************************************************************************

	xdef	ByteRun1_DePackLine

	text

*------------------------------------------------------------------------------------*
* FUNCTION : ByteRun1_DePackLine
* ACTION   : depacks exactly aDstSize bytes using Amiga ByteRun1
* INPUT    : a0 = src, a1 = dst, d0 = dst byte count
* RETURN   : d0 = next source address
*------------------------------------------------------------------------------------*

ByteRun1_DePackLine:
	movem.l	d1-d3/a1,-(sp)

	move.w	d0,d3
	beq.s	.done

.next:
	move.b	(a0)+,d0
	ext.w	d0
	bmi.s	.run_or_noop

.literal:
	move.w	d0,d1
.literal_loop:
	move.b	(a0)+,(a1)+
	subq.w	#1,d3
	beq.s	.done
	dbra	d1,.literal_loop
	bra.s	.next

.run_or_noop:
	cmpi.w	#-128,d0
	beq.s	.next
	neg.w	d0
	move.w	d0,d1
	move.b	(a0)+,d2
.run_loop:
	move.b	d2,(a1)+
	subq.w	#1,d3
	beq.s	.done
	dbra	d1,.run_loop
	bra.s	.next

.done:
	move.l	a0,d0
	movem.l	(sp)+,d1-d3/a1
	rts
