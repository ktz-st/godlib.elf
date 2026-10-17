        section text
        xdef LeoTest_Mix,LeoTest_GetReplay,LeoTest_GetCodeBytes,LeoTest_High
LeoTest_GetReplay:
        move.l LeoSampleBuffer(pc),a0
        move.l a0,d0
        rts
LeoTest_GetCodeBytes:
        move.l LeoTestCodeEnd(pc),d0
        sub.l LeoCodeBuffer(pc),d0
        rts
; Deterministic block mixer: no DMA-position test, same four native voice calls.
LeoTest_Mix:
        movem.l d0-a6,-(sp)
        move.w sr,-(sp)
        ori.w #$700,sr
        bsr mt_music
        lea channela(pc),a0
        lea pSampA(pc),a1
        moveq #0,d0
        lea jmpTable(pc),a5
        move.l LeoSampleBuffer(pc),a3
        bsr voiceMixProcess
        lea channeld(pc),a0
        lea pSampD(pc),a1
        moveq #3,d0
        lea jmpTableAdd(pc),a5
        move.l LeoSampleBuffer(pc),a3
        bsr voiceMixProcess
        lea channelb(pc),a0
        lea pSampB(pc),a1
        moveq #1,d0
        lea jmpTable(pc),a5
        move.l LeoSampleBuffer(pc),a3
        addq.w #1,a3
        bsr voiceMixProcess
        lea channelc(pc),a0
        lea pSampC(pc),a1
        moveq #2,d0
        lea jmpTableAdd(pc),a5
        move.l LeoSampleBuffer(pc),a3
        addq.w #1,a3
        bsr voiceMixProcess
        move.w (sp)+,sr
        movem.l (sp)+,d0-a6
        rts
; d0 period: render a deterministic one-voice block with a six-byte loop.
LeoTest_High:
        movem.l d2-a6,-(sp)
        move.w sr,-(sp)
        ori.w #$700,sr
        lea channela(pc),a0
        move.w d0,6(a0)
        lea LeoTestSample+4(pc),a1
        move.l a1,(a0)
        move.w #3,4(a0)
        move.w #64,8(a0)
        lea pSampA(pc),a1
        lea LeoTestSample(pc),a2
        move.l a2,(a1)
        add.w #32,a2
        move.l a2,4(a1)
        clr.w 8(a1)
        move.w #1,emudmacon
        moveq #0,d0
        lea jmpTable(pc),a5
        move.l LeoSampleBuffer(pc),a3
        bsr voiceMixProcess
        move.w (sp)+,sr
        movem.l (sp)+,d2-a6
        rts
LeoTestSample:
        dc.b 0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60
        dc.b 64,68,72,76,80,84,88,92,96,100,104,108,112,116,120,124
LeoTestCodeEnd: dc.l 0
