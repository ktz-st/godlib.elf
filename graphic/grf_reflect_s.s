; Missing Link LINK2.EXR REFLECT, TEXT $1642..$16fc, translated to vasm.
; Original five-argument STOS parser replaced with sGraphicReflect* in a0.
; Full GCC callee-save ABI; per-instance state instead of extension globals.
; See docs/reflection.md and godlib.spl/reflect/analysis for binary provenance.
        section text
        xdef Graphic_Reflect,Graphic_Reflect_End
Graphic_Reflect:
        movem.l d2-d7/a2-a6,-(sp)
        move.l a0,a6
        move.w 8(a6),d0
        move.w 10(a6),d1
        move.w 12(a6),d2
        move.l (a6),a0
        move.l 4(a6),a1
        tst.w d0
        bge.s .y1positive
        moveq #0,d0
.y1positive:
        tst.w d1
        bge.s .y2positive
        moveq #0,d1
.y2positive:
        tst.w d2
        bge.s .y3positive
        moveq #0,d2
.y3positive:
        cmpi.w #199,d0
        ble.s .y1clamped
        move.w #199,d0
.y1clamped:
        cmpi.w #199,d1
        ble.s .y2clamped
        move.w #199,d1
.y2clamped:
        cmpi.w #199,d2
        ble.s .y3clamped
        move.w #199,d2
.y3clamped:
; Guard the original's unsigned underflow for reversed/too-small ranges.
        move.w d1,d7
        sub.w d0,d7
        cmpi.w #8,d7
        blt .done
; From here retain the original arithmetic and MOVEM copy sequence.
        moveq #0,d7
        move.w d1,d7
        sub.w d0,d7
        subq.w #1,d7
        add.w d7,d0
        add.w d0,d0
        lea .lineOffsets(pc),a2
        adda.w d0,a2
        adda.w (a2),a0
        move.w #199,d6
        sub.w d2,d6
        tst.w d6
        ble .done
        subq.w #7,d7
        lsr.w #1,d7
        cmp.w d6,d7
        ble.s .height
        move.w d6,d7
.height:
        add.w d2,d2
        lea .lineOffsets(pc),a2
        adda.w d2,a2
        adda.w (a2),a1
        lea 14(a6),a2
        movem.w (a2),d0-d1
        tst.w d0
        beq.s .phase
        addq.w #2,d1
.phase:
        eori.w #1,d0
        cmpi.w #38,d1
        ble.s .store
        moveq #0,d1
.store:
        movem.w d0-d1,(a2)
        lea .steps(pc,d1.w),a2
.copy:
        movem.l (a0),d0-d6/a3-a6
        movem.l d0-d6/a3-a6,(a1)
        movem.l 44(a0),d0-d6/a3-a6
        movem.l d0-d6/a3-a6,44(a1)
        movem.l 88(a0),d0-d6/a3-a6
        movem.l d0-d6/a3-a6,88(a1)
        movem.l 132(a0),d0-d6
        movem.l d0-d6,132(a1)
        suba.w (a2)+,a0
        lea 160(a1),a1
        dbra d7,.copy
.done:
        movem.l (sp)+,d2-d7/a2-a6
        rts
; Original contains 80 repetitions. Six suffice for all 200-line buffers:
; largest starting index 19 + at most 96 output lines = 115 words.
.steps:
        rept 6
        dc.w 160,160,320,320,320,480,480,640,640,480
        dc.w 480,320,320,160,320,320,160,160,160,0
        endr
.lineOffsets:
Y       set 0
        rept 200
        dc.w Y*160
Y       set Y+1
        endr
Graphic_Reflect_End:
