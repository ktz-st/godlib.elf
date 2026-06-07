**************************************************************************************
*	SYSTEM_S.S
*
*	hardware save & restore functions
*
*	[c] 2000 Reservoir Gods
**************************************************************************************

**************************************************************************************
;	EXPORTS / IMPORTS
**************************************************************************************

	XDEF	System_SaveVectors
	XDEF	System_RestoreVectors
	XDEF	System_SetIML
	XDEF	System_GetIML
	XDEF	System_SetDataCache030
	XDEF	System_SetDataCache060
	XDEF	System_SetInstructionCache030
	XDEF	System_SetInstructionCache060
	XDEF	System_HblTemp
	XDEF	System_200hzTemp

	XDEF	System_GetEmuName0
	XDEF	System_GetEmuName1
	XDEF	System_GetpEmuDescLL

	XDEF	gSystemHblTempCounter
	XDEF	gSystem200hzTempCounter


**************************************************************************************
	TEXT
**************************************************************************************

*------------------------------------------------------------------------------------*
* FUNTION  : System_SaveVectors( U32 * apSaveArea )
* ACTION   : saves all system vectors
* CREATION : 23.01.00 PNK
*------------------------------------------------------------------------------------*

System_SaveVectors:

	movem.l	d0-a6,-(a7)			;	save registers
	move.w	SR,-(a7)			;	save status register

	ori.w	#$0700,SR			;	disable interrupts

	move.w	#61,d0				;	62 vectors to saves
	movea.w	#8,a1				;	base address of vectors

.ssv_loop:
	move.l	(a1)+,(a0)+			;	save vector address
	dbra	d0,.ssv_loop		;	loop for all vectors

	move.w	(a7)+,SR			;	restore status register
	movem.l	(a7)+,d0-a6			;	restore registers

	rts

*------------------------------------------------------------------------------------*
* FUNTION  : System_RestoreVectors( U32 * apSaveArea )
* ACTION   : restores all system vectors
* CREATION : 23.01.00 PNK
*------------------------------------------------------------------------------------*

System_RestoreVectors:

	movem.l	d0-a6,-(a7)			;	save registers
	move.w	SR,-(a7)			;	save status register

	ori.w	#$0700,SR			;	disable interrupts

	move.w	#61,d0				;	62 vectors to restore
	movea.w	#8,a1				;	base address of vectors

.srv_loop:
	move.l	(a0)+,(a1)+			;	restore vector address
	dbra	d0,.srv_loop		;	loop for all vectors

	move.w	(a7)+,SR			;	restore Status Register
	movem.l	(a7)+,d0-a6			;	restore registers

	rts

*------------------------------------------------------------------------------------*
* FUNTION  : System_SetIML( U16 aIML )
* ACTION   : sets interrupt mask level
* CREATION : 23.01.00 PNK
*------------------------------------------------------------------------------------*

System_SetIML:
	andi.w	#7,d0
	lsl.w	#8,d0
	ori.w	#$2000,d0
	move.w	d0,SR
	rts


*------------------------------------------------------------------------------------*
* FUNTION  : System_GetIML( void )
* ACTION   : gets interrupt mask level
* CREATION : 23.01.00 PNK
*------------------------------------------------------------------------------------*

System_GetIML:
	move.w	SR,d0
	lsr.w	#8,d0
	andi.w	#7,d0
	rts

*------------------------------------------------------------------------------------*
* FUNTION  : System_SetDataCache030( U16 aFlag )
* ACTION   : enables/disables data cache on 030
* CREATION : 23.01.00 PNK
*------------------------------------------------------------------------------------*

System_SetDataCache030:
	rts

	tst.w	d0
	beq.s	.cache_off

	moveq	#0,d0
	dc.l	$4e7a002		; movec	CACR,d0
	bset	#8,d0
	dc.l	$4e7b002		; movec	d0,CACR
	rts

.cache_off:
	moveq	#0,d0
	dc.l	$4e7a002		; movec	CACR,d0
	bclr	#8,d0
	dc.l	$4e7b002		; movec	d0,CACR
	rts


*------------------------------------------------------------------------------------*
* FUNTION  : System_SetDataCache060( U16 aFlag )
* ACTION   : enables/disables data cache on 060
* CREATION : 23.01.00 PNK
*------------------------------------------------------------------------------------*

System_SetDataCache060:
	tst.w	d0
	beq.s	.cache060_off

	moveq	#0,d0
	dc.l	$4e7a002		; movec	CACR,d0
	bset	#31,d0
	dc.l	$4e7b002		; movec	d0,CACR
	rts

.cache060_off:
	moveq	#0,d0
	dc.l	$4e7a002		; movec	CACR,d0
	bclr	#31,d0
	bclr	#29,d0
	dc.l	$4e7b002		; movec	d0,CACR
	rts


*------------------------------------------------------------------------------------*
* FUNTION  : System_SetInstructionCache030( U16 aFlag )
* ACTION   : enables/disables Instruction cache on 030
* CREATION : 23.01.00 PNK
*------------------------------------------------------------------------------------*

System_SetInstructionCache030:
	rts

	tst.w	d0
	beq.s	.cache_off

	moveq	#0,d0
	dc.l	$4e7a002		; movec	CACR,d0
	bset	#0,d0
	dc.l	$4e7b002		; movec	d0,CACR
	rts

.cache_off:
	moveq	#0,d0
	dc.l	$4e7a002		; movec	CACR,d0
	bclr	#0,d0
	dc.l	$4e7b002		; movec	d0,CACR
	rts


*------------------------------------------------------------------------------------*
* FUNTION  : System_SetInstructionCache060( U16 aFlag )
* ACTION   : enables/disables Instruction cache on 060
* CREATION : 23.01.00 PNK
*------------------------------------------------------------------------------------*

System_SetInstructionCache060:
	tst.w	d0
	beq.s	.cache060_off

	moveq	#0,d0
	dc.l	$4e7a002		; movec	CACR,d0
	bset	#15,d0
	dc.l	$4e7b002		; movec	d0,CACR
	rts

.cache060_off:
	moveq	#0,d0
	dc.l	$4e7a002		; movec	CACR,d0
	bclr	#15,d0
	bclr	#23,d0
	dc.l	$4e7b002		; movec	d0,CACR
	rts


*------------------------------------------------------------------------------------*
* FUNTION  : System_HblTemp()
* ACTION   : temporary hbl
* CREATION : 23.01.00 PNK
*------------------------------------------------------------------------------------*

System_HblTemp:
	addq.l	#1,gSystemHblTempCounter
	rte


*------------------------------------------------------------------------------------*
* FUNTION  : System_200hzTemp()
* ACTION   : temporary 200hz counter
* CREATION : 23.01.00 PNK
*------------------------------------------------------------------------------------*

System_200hzTemp:
	addq.l	#1,gSystem200hzTempCounter
	bclr.b	#4,$FFFFFA11
	rte


*------------------------------------------------------------------------------------*
* FUNTION  : System_GetEmuName0()
* ACTION   : returns emulator name
* CREATION : 23.01.00 PNK
*------------------------------------------------------------------------------------*

System_GetEmuName0:
	movem.l	d1-d7/a0-a6,-(a7)

	move.l	#'Emu?',d6
	move.l	d6,d7
	move.w	#$25,-(a7)
	trap	#14
	addq.l	#2,a7

	move.l	d6,d0

	movem.l	(a7)+,d1-d7/a0-a6
	rts


*------------------------------------------------------------------------------------*
* FUNTION  : System_GetEmuName1()
* ACTION   : returns emulator name
* CREATION : 23.01.00 PNK
*------------------------------------------------------------------------------------*

System_GetEmuName1:
	movem.l	d1-d7/a0-a6,-(a7)

	move.l	#'Emu?',d6
	move.l	d6,d7
	move.w	#$25,-(a7)
	trap	#14
	addq.l	#2,a7

	move.l	d7,d0

	movem.l	(a7)+,d1-d7/a0-a6
	rts


*------------------------------------------------------------------------------------*
* FUNTION  : System_GetEmuVers()
* ACTION   : returns emulator version
* CREATION : 23.01.00 PNK
*------------------------------------------------------------------------------------*

System_GetEmuVers:
	movem.l	d1-d7/a0-a6,-(a7)

	move.l	#'Emu?',d6
	move.l	d6,d7
	move.w	#$25,-(a7)
	trap	#14
	addq.l	#2,a7

	moveq	#0,d0
	move.b	(a0)+,d0
	lsl.w	#8,d0
	move.b	(a0)+,d0

	movem.l	(a7)+,d1-d7/a0-a6
	rts


*------------------------------------------------------------------------------------*
* FUNTION  : System_GetEmuDescLL()
* ACTION   : returns emulator description
* CREATION : 23.01.00 PNK
*------------------------------------------------------------------------------------*

System_GetpEmuDescLL:
	movem.l	d1-d7/a1-a6,-(a7)

	move.l	#'Emu?',d6
	move.l	d6,d7
	move.w	#$25,-(a7)
	trap	#14
	addq.l	#2,a7

	movem.l	(a7)+,d1-d7/a1-a6
	rts

gSystemHblTempCounter:		dc.l	0
gSystem200hzTempCounter:	dc.l	0
