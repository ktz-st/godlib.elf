 include "graphic.i"
 XDEF BaselineLine,BaselineLine_Clip
BaselineLine_Clip:
	movem.l	d3-d7/a2-a6,-(a7)
	subq.l	#8,a7
	move.l	a7,a4
	move.w	d0,(a4)								; colour
	move.l	a0,a2								; canvas

	move.w	sGraphicBox_mX0(a1),d1
	move.w	sGraphicBox_mY0(a1),d2
	move.w	sGraphicBox_mX1(a1),d3
	move.w	sGraphicBox_mY1(a1),d4
	bsr		BaselineLine_Setup

.loop:
	bsr		BaselineLine_Plot_Clip
	cmp.w	d1,d3
	bne.s	.notdone
	cmp.w	d2,d4
	beq.s	.done
.notdone:
	move.w	d7,d0
	add.w	d0,d0								; e2 = err * 2
	cmp.w	d6,d0
	blt.s	.no_x
	add.w	d6,d7
	add.w	2(a4),d1
.no_x:
	cmp.w	d5,d0
	bgt.s	.no_y
	add.w	d5,d7
	add.w	4(a4),d2
.no_y:
	bra.s	.loop

.done:
	addq.l	#8,a7
	movem.l	(a7)+,d3-d7/a2-a6
	rts


BaselineLine:
	movem.l	d3-d7/a2-a6,-(a7)
	subq.l	#8,a7
	move.l	a7,a4
	move.w	d0,(a4)								; colour
	move.l	a0,a2								; canvas

	move.w	sGraphicBox_mX0(a1),d1
	move.w	sGraphicBox_mY0(a1),d2
	move.w	sGraphicBox_mX1(a1),d3
	move.w	sGraphicBox_mY1(a1),d4
	bsr		BaselineLine_Setup

.loop:
	bsr		BaselineLine_Plot
	cmp.w	d1,d3
	bne.s	.notdone
	cmp.w	d2,d4
	beq.s	.done
.notdone:
	move.w	d7,d0
	add.w	d0,d0								; e2 = err * 2
	cmp.w	d6,d0
	blt.s	.no_x
	add.w	d6,d7
	add.w	2(a4),d1
.no_x:
	cmp.w	d5,d0
	bgt.s	.no_y
	add.w	d5,d7
	add.w	4(a4),d2
.no_y:
	bra.s	.loop

.done:
	addq.l	#8,a7
	movem.l	(a7)+,d3-d7/a2-a6
	rts


BaselineLine_Setup:
	move.w	d3,d5								; dx = abs(x1-x0)
	sub.w	d1,d5
	bge.s	.dx_ok
	neg.w	d5
	move.w	#-1,2(a4)							; sx
	bra.s	.dx_done
.dx_ok:
	move.w	#1,2(a4)							; sx
.dx_done:
	move.w	d4,d6								; dy = -abs(y1-y0)
	sub.w	d2,d6
	bge.s	.dy_pos
	neg.w	d6
	neg.w	d6
	move.w	#-1,4(a4)							; sy
	bra.s	.dy_done
.dy_pos:
	neg.w	d6
	move.w	#1,4(a4)							; sy
.dy_done:
	move.w	d5,d7
	add.w	d6,d7								; err = dx + dy
	rts


BaselineLine_Plot_Clip:
	cmp.w	sGraphicCanvas_mClipBox+sGraphicBox_mX0(a2),d1
	blt.s	.clip
	cmp.w	sGraphicCanvas_mClipBox+sGraphicBox_mX1(a2),d1
	bge.s	.clip
	cmp.w	sGraphicCanvas_mClipBox+sGraphicBox_mY0(a2),d2
	blt.s	.clip
	cmp.w	sGraphicCanvas_mClipBox+sGraphicBox_mY1(a2),d2
	bge.s	.clip
	bra.s	BaselineLine_Plot
.clip:
	rts


BaselineLine_Plot:
	movem.l	d0-d7/a0-a3,-(a7)

	move.w	d2,d0								; Y * 4
	add.w	d0,d0
	add.w	d0,d0
	move.l	sGraphicCanvas_mpLineOffsets(a2),a3
	move.l	(a3,d0.w),a3						; line offset

	move.w	d1,d0								; X word offset
	and.l	#$0000FFF0,d0
	lsr.w	#1,d0
	add.l	d0,a3
	add.l	sGraphicCanvas_mpVRAM(a2),a3

	move.w	d1,d0								; point mask
	and.w	#15,d0
	add.w	d0,d0
	lea		BaselinePoints,a0
	move.w	(a0,d0.w),d1
	move.w	d1,d2
	not.w	d2

	and.w	d2,(a3)								; Mask Plane 0
	and.w	d2,2(a3)							; Mask Plane 1
	and.w	d2,4(a3)							; Mask Plane 2
	and.w	d2,6(a3)							; Mask Plane 3

	move.w	(a4),d0								; colour
	lsr.w	#1,d0
	bcc.s	.nbp0
	or.w	d1,0(a3)
.nbp0:
	lsr.w	#1,d0
	bcc.s	.nbp1
	or.w	d1,2(a3)
.nbp1:
	lsr.w	#1,d0
	bcc.s	.nbp2
	or.w	d1,4(a3)
.nbp2:
	lsr.w	#1,d0
	bcc.s	.nbp3
	or.w	d1,6(a3)
.nbp3:
	movem.l	(a7)+,d0-d7/a0-a3
	rts

BaselinePoints:
 dc.w $8000,$4000,$2000,$1000,$0800,$0400,$0200,$0100,$0080,$0040,$0020,$0010,$0008,$0004,$0002,$0001
