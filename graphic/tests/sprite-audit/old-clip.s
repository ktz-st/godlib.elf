**************************************************************************************
*	GRF_B4_S.S
*
*	4 BitPlane Blitter Rendering Functions
*
*	[c] 2002 Reservoir Gods
**************************************************************************************


**************************************************************************************
;	INCLUDES
**************************************************************************************

	include	"graphic.i"


**************************************************************************************
;	XDEFS / IMPORTS
**************************************************************************************

 XDEF Old_DrawSprite_Clip_BLT
 XREF Graphic_4BP_DrawSprite_Go
eBLITTERLOP_ZERO				EQU	0
eBLITTERLOP_SRC_AND_DST			EQU	1
eBLITTERLOP_SRC_ANDNOT_DST		EQU	2
eBLITTERLOP_SRC					EQU	3
eBLITTERLOP_NOTSRC_AND_DST		EQU	4
eBLITTERLOP_DST					EQU	5
eBLITTERLOP_SRC_XOR_DST			EQU	6
eBLITTERLOP_SRC_OR_DST			EQU	7
eBLITTERLOP_NOTSRC_ANDNOT_DST	EQU	8
eBLITTERLOP_NOTSRC_XOR_DST		EQU	9
eBLITTERLOP_NOTDST				EQU	10
eBLITTERLOP_SRC_ORNOT_DST		EQU	11
eBLITTERLOP_NOTSRC				EQU	12
eBLITTERLOP_NOTSRC_OR_DST		EQU	13
eBLITTERLOP_NOTSRC_ORNOT_DST	EQU	14
eBLITTERLOP_ONE					EQU	15

eBLITTERHOP_ONE					EQU	0
eBLITTERHOP_HALFTONE			EQU	1
eBLITTERHOP_SRC					EQU	2
eBLITTERHOP_SRC_AND_HALFTONE	EQU	3

eBLITTERMODE_LINENUMBER_MASK	EQU	$F
eBLITTERMODE_SMUDGE_BIT			EQU	$20
eBLITTERMODE_SMUDGE_MASK		EQU	$DF
eBLITTERMODE_HOG_BIT			EQU	$40
eBLITTERMODE_HOG_MASK			EQU	$BF
eBLITTERMODE_BUSY_BIT			EQU	$80
eBLITTERMODE_BUSY_MASK			EQU	$7F

eBLITTERSKEW_SKEW_MASK			EQU	$F
eBLITTERSKEW_NFSR_BIT			EQU	$40
eBLITTERSKEW_NFSR_MASK			EQU	$BF
eBLITTERSKEW_FXSR_BIT			EQU	$80
eBLITTERSKEW_FXSR_MASK			EQU	$7F

eBLITREG_HALFTONE				EQU	$FFFF8A00
eBLITREG_SRC_INC_X				EQU	$FFFF8A20
eBLITREG_SRC_INC_Y				EQU	$FFFF8A22
eBLITREG_pSRC					EQU	$FFFF8A24
eBLITREG_ENDMASK_1				EQU	$FFFF8A28
eBLITREG_ENDMASK_2				EQU	$FFFF8A2A
eBLITREG_ENDMASK_3				EQU	$FFFF8A2C
eBLITREG_DST_INC_X				EQU	$FFFF8A2E
eBLITREG_DST_INC_Y				EQU	$FFFF8A30
eBLITREG_pDST					EQU	$FFFF8A32
eBLITREG_COUNT_X				EQU	$FFFF8A36
eBLITREG_COUNT_Y				EQU	$FFFF8A38
eBLITREG_HOP					EQU	$FFFF8A3A
eBLITREG_LOP					EQU	$FFFF8A3B
eBLITREG_MODE					EQU	$FFFF8A3C
eBLITREG_SKEW					EQU	$FFFF8A3D

eBLITTER_BASE					EQU	$FFFF8A00
eBLITTER_HALFTONE				EQU	$00
eBLITTER_SRC_INC_X				EQU	$20
eBLITTER_SRC_INC_Y				EQU	$22
eBLITTER_pSRC					EQU	$24
eBLITTER_ENDMASK_1				EQU	$28
eBLITTER_ENDMASK_2				EQU	$2A
eBLITTER_ENDMASK_3				EQU	$2C
eBLITTER_DST_INC_X				EQU	$2E
eBLITTER_DST_INC_Y				EQU	$30
eBLITTER_pDST					EQU	$32
eBLITTER_COUNT_X				EQU	$36
eBLITTER_COUNT_Y				EQU	$38
eBLITTER_HOP					EQU	$3A
eBLITTER_LOP					EQU	$3B
eBLITTER_MODE					EQU	$3C
eBLITTER_SKEW					EQU	$3D

*    I n p u t   p a r a m e t e r   b l o c k   o f f s e t s

SRC_FORM  equ  0    ; Base address of source memory form .l
SRC_NXWD  equ  4    ; Offset between words in source plane .w
SRC_NXLN  equ  6    ; Source form width .w
SRC_NXPL  equ  8    ; Offset between source planes .w
SRC_XMIN  equ  10   ; Source blt rectangle minimum X .w
SRC_YMIN  equ  12   ; Source blt rectangle minimum Y .w

DST_FORM  equ  14   ; Base address of destination memory form .l
DST_NXWD  equ  18   ; Offset between words in destination plane.w
DST_NXLN  equ  20   ; Destination form width .w
DST_NXPL  equ  22   ; Offset between destination planes .w
DST_XMIN  equ  24   ; Destination blt rectangle minimum X .w
DST_YMIN  equ  26   ; Destination blt rectangle minimum Y .w

WIDTH     equ  28   ; Width of blt rectangle .w
HEIGHT    equ  30   ; Height of blt rectangle .w
PLANES    equ  32   ; Number of planes to blt .w
BLIT_SIZEOF	equ 32

**************************************************************************************
;	STRUCTS
**************************************************************************************


**************************************************************************************
;	MACROS
**************************************************************************************

	MACRO	mBlitterWait_a1
	btst	#7,eBLITTER_MODE(a1)	; is blitter busy?
	nop
	bne.s	*-8						; yes, wait
	ENDM

	MACRO	mBlitterWait_a2
	btst	#7,eBLITTER_MODE(a2)	; is blitter busy?
	nop
	bne.s	*-8						; yes, wait
	ENDM

	MACRO	mBlitterWait_a6
	btst	#7,eBLITTER_MODE(a6)	; is blitter busy?
	nop
	bne.s	*-8						; yes, wait
	ENDM

	MACRO	mBlitterGoWait_a1
	move.b	#eBLITTERMODE_BUSY_BIT,eBLITTER_MODE(a1)
	nop
	bset.b	#7,eBLITTER_MODE(a1)	; is blitter busy?
	nop
	bne.s	*-8						; yes, wait
	ENDM

	MACRO	mBlitterGoWait_a2
	move.b	#eBLITTERMODE_BUSY_BIT,eBLITTER_MODE(a2)
	nop
	bset.b	#7,eBLITTER_MODE(a2)	; is blitter busy?
	nop
	bne.s	*-8						; yes, wait
	ENDM

	MACRO	mBlitterGoWait_a6
	move.b	#eBLITTERMODE_BUSY_BIT,eBLITTER_MODE(a6)
	nop
	bset.b	#7,eBLITTER_MODE(a6)	; is blitter busy?
	nop
	bne.s	*-8						; yes, wait
	ENDM

 TEXT
Old_DrawSprite_Clip_BLT:
	movem.l	d3-d7/a2-a6,-(a7)

	move.l	11*4(a7),a2
	move.w	sGraphicPos_mX(a1),d0
	move.w	sGraphicPos_mY(a1),d1
	move.w	sGraphicSprite_mWidth(a2),d2
	move.w	sGraphicSprite_mHeight(a2),d3
	move.l	sGraphicSprite_mpGfx(a2),a3
	move.l	sGraphicSprite_mpMask(a2),a4


	cmp.w	sGraphicCanvas_mClipBox+sGraphicBox_mX1(a0),d0
	bge		.clip
	cmp.w	sGraphicCanvas_mClipBox+sGraphicBox_mY1(a0),d1
	bge		.clip

	move.w	d2,d7
	subq.w	#1,d7
	lsr.w	#4,d7

	cmp.w	sGraphicCanvas_mClipBox+sGraphicBox_mY0(a0),d1
	bge		.y0_ok
	sub.w	sGraphicCanvas_mClipBox+sGraphicBox_mY0(a0),d1
	add.w	d1,d3												; adjust height
	ble		.clip


	move.w	sGraphicSprite_mWidth(a2),d4
	add.w	#15,d4
	and.l	#$0000FFF0,d4
	lsr.w	#1,d4
	move.w	d4,d5
	lsr.w	#2,d5

	neg.w	d1
	mulu.w	d1,d4
	mulu.w	d1,d5
	add.l	d4,a3
	add.l	d5,a4

	move.w	sGraphicCanvas_mClipBox+sGraphicBox_mY0(a0),d1		; clip dst y0
.y0_ok:

	move.w	d1,d4												;y
	add.w	d3,d4												;y2
	sub.w	sGraphicCanvas_mClipBox+sGraphicBox_mY1(a0),d4
	ble		.y1_ok

	sub.w	d4,d3											; clip height
	ble		.clip

.y1_ok:

	movea.w	#eBLITTER_BASE,a6

	mBlitterWait_a6
	moveq			#$f,d5
	and.w			d0,d5
	move.b			d5,eBLITTER_SKEW(a6)

	cmp.w	sGraphicCanvas_mClipBox+sGraphicBox_mX0(a0),d0
	bge		.x0_ok
	sub.w	sGraphicCanvas_mClipBox+sGraphicBox_mX0(a0),d0
	add.w	d0,d2												; adjust height
	ble		.clip

	neg.w	d0
	lsr.w	#4,d0
	sub.w	d0,d7
	lsl.w	#3,d0
	add.w	d0,a3
	lsr.w	#2,d0
	add.w	d0,a4

	move.w	sGraphicCanvas_mClipBox+sGraphicBox_mX0(a0),d0
.x0_ok:

	move.w	d0,d4							;x
	add.w	d2,d4							;x2
	sub.w	sGraphicCanvas_mClipBox+sGraphicBox_mX1(a0),d4
	ble		.x1_ok

	sub.w	d4,d2							; clip width
	ble		.clip

	lsr.w	#4,d4
	sub.w	d4,d7

.x1_ok:
	move.w	#2,eBLITTER_SRC_INC_X(a6)			; offset to next chunk
	move.w	d0,d5
	move.w	d0,d6
	add.w	d2,d6
	subq.w	#1,d6
	lsr.w	#4,d5
	lsr.w	#4,d6
	sub.w	d5,d6
	cmp.w	d7,d6
	beq.s	.same
	bgt.s	.dstbig

	or.b	#eBLITTERSKEW_FXSR_BIT,eBLITTER_SKEW(a6)
	bra		.go
.dstbig:
	or.b	#eBLITTERSKEW_NFSR_BIT,eBLITTER_SKEW(a6)
	bra		.go
.same:
	moveq	#$f,d7
	and.w	d0,d7
	move.b	eBLITTER_SKEW(a6),d5
	cmp.b	d5,d7
	bgt		.go

	tst.w	d6
	beq.s	.singlespan

	or.b	#eBLITTERSKEW_FXSR_BIT+eBLITTERSKEW_NFSR_BIT,eBLITTER_SKEW(a6)
	bra		.go
.singlespan:

	or.b	#eBLITTERSKEW_FXSR_BIT,eBLITTER_SKEW(a6)
	move.w	#0,eBLITTER_SRC_INC_X(a6)			; offset to next chunk

.go:
	bra		Graphic_4BP_DrawSprite_Go

.clip:

	movem.l	(a7)+,d3-d7/a2-a6
	rts


*------------------------------------------------------------------------------------*
* FUNCTION: void (* DrawSprite )(  const struct sGraphicCanvas * apCanvas, const sGraphicPos * apCoords,  const void * apSprite );
* ACTION:   draws a sprite
* CREATION: 01.02.02 PNK
