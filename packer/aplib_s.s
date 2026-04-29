;  aPLib decompressor for 68000 - 156 bytes
;
;  in:  a0 = start of compressed data
;       a1 = start of decompression buffer
;  out: d0 = decompressed size
;
;  Copyright (C) 2020 Emmanuel Marty
;  With parts of the code inspired by Franck "hitchhikr" Charlet
;
;  This software is provided 'as-is', without any express or implied
;  warranty. In no event will the authors be held liable for any damages
;  arising from the use of this software.
;
;  Permission is granted to anyone to use this software for any purpose,
;  including commercial applications, and to alter it and redistribute it
;  freely, subject to the following restrictions:
;
;  1. The origin of this software must not be misrepresented; you must not
;     claim that you wrote the original software. If you use this software
;     in a product, an acknowledgment in the product documentation would be
;     appreciated but is not required.
;  2. Altered source versions must be plainly marked as such, and must not
;     be misrepresented as being the original software.
;  3. This notice may not be removed or altered from any source distribution.

	xdef	ApLib_DePack
	xdef	aplDecompress

	text

ApLib_DePack:
aplDecompress:
	movem.l	a2-a6/d2-d3,-(sp)

	moveq	#-128,d1
	lea	32000.w,a2
	lea	1280.w,a3
	lea	128.w,a4
	move.l	a1,a5

.literal:
	move.b	(a0)+,(a1)+
.after_lit:
	moveq	#3,d2

.next_token:
	bsr.s	.get_bit
	bcc.s	.literal

	bsr.s	.get_bit
	bcs.s	.other_match

	bsr.s	.get_gamma2
	sub.l	d2,d0
	bcc.s	.no_repmatch

	bsr.s	.get_gamma2
	bra.s	.got_len

.no_repmatch:
	lsl.l	#8,d0
	move.b	(a0)+,d0
	move.l	d0,d3

	bsr.s	.get_gamma2
	cmp.l	a2,d3
	bge.s	.inc_by_2
	cmp.l	a3,d3
	bge.s	.inc_by_1
	cmp.l	a4,d3
	bge.s	.got_len
.inc_by_2:
	addq.l	#1,d0
.inc_by_1:
	addq.l	#1,d0

.got_len:
	move.l	a1,a6
	sub.l	d3,a6
	subq.l	#1,d0
.copy_match:
	move.b	(a6)+,(a1)+
	dbf	d0,.copy_match
	moveq	#2,d2
	bra.s	.next_token

.other_match:
	bsr.s	.get_bit
	bcs.s	.short_match

	moveq	#1,d0
	moveq	#0,d3
	move.b	(a0)+,d3
	lsr.b	#1,d3
	beq.s	.done
	addx.b	d0,d0
	bra.s	.got_len

.short_match:
	moveq	#0,d0
	bsr.s	.get_dibits
	addx.b	d0,d0
	bsr.s	.get_dibits
	addx.b	d0,d0
	tst.b	d0
	beq.s	.write_zero

	move.l	a1,a6
	sub.l	d0,a6
	move.b	(a6),d0
.write_zero:
	move.b	d0,(a1)+
	bra.s	.after_lit

.done:
	move.l	a1,d0
	sub.l	a5,d0
	movem.l	(sp)+,a2-a6/d2-d3
	rts

.get_gamma2:
	moveq	#1,d0
.gamma2_loop:
	bsr.s	.get_dibits
	bcs.s	.gamma2_loop
	rts

.get_dibits:
	bsr.s	.get_bit
	addx.l	d0,d0
.get_bit:
	add.b	d1,d1
	bne.s	.got_bit
	move.b	(a0)+,d1
	addx.b	d1,d1
.got_bit:
	rts
