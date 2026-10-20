; RJP note events -> native Leonard current/reload voices, no polling heuristic.
        section .text,code
        xdef RjpLeo_Arm,RjpLeo_Reset,RjpLeo_Sync,RjpLeo_Begin,RjpLeo_End
        xref PaulaMixer_Channels,channela,pSampA,emudmacon,nullSample
RjpLeo_Begin:
; a3 shadow voice, a4 source base, d0 attack words, d1 repeat words,
; d2 repeat offset words, d3 attack offset words, a5 RJP sequencer globals.
RjpLeo_Arm:
        movem.l d0-d7/a0-a6,-(sp)
        move.l a3,a6
        moveq #0,d4
        move.b $2d(a5),d4
        move.w d4,d5
        lsl.w #4,d5
        lea channela,a0
        adda.w d5,a0
        move.w d4,d5
        add.w d5,d5
        move.w d4,d6
        lsl.w #3,d6
        add.w d6,d5
        lea pSampA,a1
        adda.w d5,a1
        and.l #$ffff,d3
        add.l d3,d3
        move.l a4,a2
        adda.l d3,a2
        move.l a2,(a1)
        move.l a2,(a6)
        and.l #$ffff,d0
        add.l d0,d0
        adda.l d0,a2
        move.l a2,4(a1)
        move.l a2,4(a6)
        clr.w 8(a1)
        clr.w 16(a6)
        cmpi.w #1,d1
        bhi.s .loop
        lea nullSample,a4
        moveq #1,d1
        clr.l 8(a6)
        bra.s .reload
.loop:
        and.l #$ffff,d2
        add.l d2,d2
        adda.l d2,a4
        move.l a4,8(a6)
.reload:
        move.l a4,(a0)
        move.w d1,4(a0)
        move.w emudmacon,d0
        bset d4,d0
        move.w d0,emudmacon
        movem.l (sp)+,d0-d7/a0-a6
        rts
RjpLeo_Reset:
        movem.l d0-d2/a0-a2,-(sp)
        clr.w emudmacon
        lea channela,a0
        lea pSampA,a1
        lea nullSample,a2
        moveq #3,d0
.clear:
        move.l a2,(a0)
        move.w #1,4(a0)
        clr.w 8(a0)
        move.l a2,(a1)
        move.l a2,d1
        addq.l #2,d1
        move.l d1,4(a1)
        clr.w 8(a1)
        lea 16(a0),a0
        lea 10(a1),a1
        dbf d0,.clear
        movem.l (sp)+,d0-d2/a0-a2
        rts
RjpLeo_Sync:
        movem.l d0-d3/a0-a2,-(sp)
        lea PaulaMixer_Channels,a0
        lea channela,a1
        lea pSampA,a2
        move.w emudmacon,d0
        moveq #0,d1
.voice:
        move.w 12(a0),8(a1)
        move.w 14(a0),6(a1)
        tst.l (a0)
        beq.s .off
        cmpi.l #nullSample,(a1)
        bne.s .next
        move.l (a2),d2
        subi.l #nullSample,d2
        cmpi.l #1,d2
        bhi.s .next
        clr.l (a0)
.off:
        bclr d1,d0
.next:
        lea 20(a0),a0
        lea 16(a1),a1
        lea 10(a2),a2
        addq.w #1,d1
        cmpi.w #4,d1
        bne.s .voice
        move.w d0,emudmacon
        movem.l (sp)+,d0-d3/a0-a2
        rts
RjpLeo_End:
