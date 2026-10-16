; Four-plane dirty background restore. Fastcall: list=a0, src=a1, dst=4(sp).
; Source/destination use identical uniform layouts, even addresses/stride.
; Merge consecutive 8-byte columns of equal height, at most 32 per run.
        include "graphic.i"
        xdef Graphic_ChunkList_ReStore
        text
Graphic_ChunkList_ReStore:
        movem.l d2-d7/a2-a6,-(sp)
        move.l 48(sp),a2
        moveq #0,d3
        move.w sGraphicChunkList_mActiveCount(a0),d3
        clr.w sGraphicChunkList_mActiveCount(a0)
        tst.w d3
        beq .done
        lea sGraphicChunkList_mChunks(a0),a5
        move.l sGraphicCanvas_mpVRAM(a2),a4
        move.l sGraphicCanvas_mpVRAM(a1),a3
        move.l sGraphicCanvas_mpLineOffsets(a1),a0
        move.l 4(a0),d2
.next:
        move.l (a5)+,d7
        moveq #0,d5
        move.w (a5)+,d5
        subq.w #1,d3
        move.l d7,d6
        addq.l #8,d6
        moveq #1,d4
.merge:
        tst.w d3
        beq .run
        cmp.w #32,d4
        beq .run
        cmp.l (a5),d6
        bne .run
        cmp.w 4(a5),d5
        bne .run
        addq.l #6,a5
        addq.l #8,d6
        addq.w #1,d4
        subq.w #1,d3
        bra .merge
.run:
        tst.w d5
        beq .continue
        move.l a3,a0
        adda.l d7,a0
        move.l a4,a1
        adda.l d7,a1
        lsl.w #2,d4
        lea .copyend(pc),a6
        suba.w d4,a6
        add.w d4,d4
        move.l d2,d6
        sub.l d4,d6
        subq.w #1,d5
.row:
        jmp (a6)
        rept 32
        move.l (a0)+,(a1)+
        move.l (a0)+,(a1)+
        endr
.copyend:
        adda.l d6,a0
        adda.l d6,a1
        dbra d5,.row
.continue:
        tst.w d3
        bne .next
.done:
        movem.l (sp)+,d2-d7/a2-a6
        rts
