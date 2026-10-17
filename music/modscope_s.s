; GCC fastcall: A0 four 24-byte snapshots, A1 256 output bytes, D0 clock step.
; Does not modify replay registers, phase, samples, volume tables or DMA memory.
        section text
        xdef ModScope_Sample
ModScope_Sample:
        movem.l d2-d7/a2-a6,-(sp)
        move.l d0,a6
        move.w #3,-(sp)
.channel:
        tst.w 22(a0)
        beq .zero
        moveq #0,d6
        move.w 18(a0),d6
        beq .zero
        cmpi.w #64,d6
        bls.s .volumeOk
        moveq #64,d6
.volumeOk:
        move.w 16(a0),d1
        cmpi.w #48,d1
        blo .zero
        cmpi.w #4095,d1
        bhi .zero
        move.l (a0),d0
        beq .zero
        move.l d0,a2
        move.l 4(a0),a3
        move.l 8(a0),a4
        move.l 12(a0),a5
; Exact unsigned 32/16 division into integer and fractional words.
        move.l a6,d4
        move.l d4,d5
        clr.w d5
        swap d5
        divu.w d1,d5
        move.w d5,d2
        move.w d4,d5
        divu.w d1,d5
        move.w d5,d4
        move.w d2,d5
        moveq #0,d3
        moveq #63,d7
.point:
        cmpa.l a3,a2
        blo.s .read
        move.l a4,d1
        beq.s .rest
        cmpa.l a4,a5
        bls.s .rest
        suba.l a3,a2
        adda.l a4,a2
        move.l a5,a3
        bra.s .point
.read:
        move.b (a2),d2
        ext.w d2
        tst.w 20(a0)
        beq.s .eightBit
        add.w d2,d2
.eightBit:
        muls.w d6,d2
        asr.w #6,d2
        move.b d2,(a1)+
        adda.w d5,a2
        add.w d4,d3
        bcc.s .noCarry
        addq.l #1,a2
.noCarry:
        dbf d7,.point
        bra.s .next
.rest:
        clr.b (a1)+
        dbf d7,.rest
        bra.s .next
.zero:
        moveq #15,d7
.clear:
        clr.b (a1)+
        clr.b (a1)+
        clr.b (a1)+
        clr.b (a1)+
        dbf d7,.clear
.next:
        lea 24(a0),a0
        subq.w #1,(sp)
        bpl .channel
        addq.l #2,sp
        movem.l (sp)+,d2-d7/a2-a6
        rts

; A0 destination (any byte alignment), A1 completed PCM block or NULL for silence.
; Called under the replay wrapper's short IPL7 critical section. Scratch regs only.
        xdef ModScope_CopyStereo
ModScope_CopyStereo:
        move.l a0,d0
        btst #0,d0
        bne.s .byteCopy
        moveq #81,d0
        move.l a1,d1
        beq.s .zeroLong
.long:
        move.l (a1)+,(a0)+
        dbf d0,.long
        rts
.zeroLong:
        clr.l (a0)+
        dbf d0,.zeroLong
        rts
.byteCopy:
        moveq #81,d0
        move.l a1,d1
        beq.s .zeroByte
.bytes:
        move.b (a1)+,(a0)+
        move.b (a1)+,(a0)+
        move.b (a1)+,(a0)+
        move.b (a1)+,(a0)+
        dbf d0,.bytes
        rts
.zeroByte:
        clr.b (a0)+
        clr.b (a0)+
        clr.b (a0)+
        clr.b (a0)+
        dbf d0,.zeroByte
        rts
