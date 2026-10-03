; OrionSoft SGDL 4-channel mixer, recovered from sgdl/lib/snd_asm.o.
; SHA256 4f2cda19dc0525a7090d3f9d3af4c388bf375049d196895dd72b14b1b61beb3a.
; Comments give original TEXT offsets, not runtime addresses.
; STE path only; the Falcon Timer-A interrupt path is intentionally excluded.
    section .text
    xdef SgdlMixer_CoreInit,SgdlMixer_CoreExit,SgdlMixer_CoreUpdate
    xdef SgdlMixer_Lock,SgdlMixer_Unlock,SgdlMixer_MixBlock
    xref gSgdlMixerBuffer,gSgdlMixerActive,gSgdlMixerBufferId
    xref gSgdlMixerChannels,gSgdlMixerDmaMode

SgdlMixer_Lock:
    move.w sr,d0
    ori.w #$0700,sr
    rts
SgdlMixer_Unlock:
    move.w d0,sr
    rts

SgdlMixer_CoreInit:                ; $0000
    lea $ffff8900.w,a0
    clr.b 1(a0)
    move.l gSgdlMixerBuffer,d0
    move.l d0,d1
    add.l #1024,d1
    move.b gSgdlMixerDmaMode,33(a0) ; original MOVE.B #$82,33(a0)
    movep.w d0,5(a0)
    swap d0
    move.b d0,3(a0)
    movep.w d1,17(a0)
    swap d1
    move.b d1,15(a0)
    move.b #3,1(a0)
    rts
SgdlMixer_CoreExit:                ; $0038
    clr.b $ffff8901.w
    rts

SgdlMixer_CoreUpdate:              ; $00c8
    movem.l d0-d6/a0-a6,-(sp)
    tst.b gSgdlMixerActive
    beq.s .done
    lea $ffff8909.w,a0
    moveq #0,d0
    move.b (a0),d0
    asl.l #8,d0
    move.b 2(a0),d0
    asl.l #8,d0
    move.b 4(a0),d0
    movea.l gSgdlMixerBuffer,a0
    lea 512(a0),a1
    cmp.l a1,d0
    bmi.s .first
    tst.b gSgdlMixerBufferId
    beq.s .fill
    bra.s .done
.first:
    tst.b gSgdlMixerBufferId
    beq.s .done
    movea.l a1,a0
.fill:
    eori.b #1,gSgdlMixerBufferId
    bsr.w SgdlMixer_MixCore
.done:
    movem.l (sp)+,d0-d6/a0-a6
    rts

; Public testable block entry: A0=512-byte destination, preserves C callee-save.
SgdlMixer_MixBlock:
    movem.l d0-d6/a0-a6,-(sp)
    bsr.w SgdlMixer_MixCore
    movem.l (sp)+,d0-d6/a0-a6
    rts

SgdlMixer_MixCore:                 ; $015e..$0298, faithfully transcribed
    movea.l gSgdlMixerBuffer,a6
    lea 1024(a6),a6
    lea gSgdlMixerChannels,a1
    movea.l (a1),a2
    move.l 4(a1),d2
    movea.l 12(a1),a3
    move.l 16(a1),d3
    movea.l 24(a1),a4
    move.l 28(a1),d4
    movea.l 36(a1),a5
    move.l 40(a1),d5
    move.l a2,d6
    bne.s .ch1
    movea.l a6,a2
.ch1:
    move.l a3,d6
    bne.s .ch2
    movea.l a6,a3
.ch2:
    move.l a4,d6
    bne.s .ch3
    movea.l a6,a4
.ch3:
    move.l a5,d6
    bne.s .ch4
    movea.l a6,a5
.ch4:
    moveq #31,d6
.samples:                         ; $01a6..$0246
    rept 16
    move.b (a2)+,d0
    add.b (a3)+,d0
    add.b (a4)+,d0
    add.b (a5)+,d0
    move.b d0,(a0)+
    endr
    dbf d6,.samples
    cmp.l a2,d2
    bgt.s .end1
    movea.l 8(a1),a2
    move.l a2,d6
    bne.s .end1
    clr.l 4(a1)
.end1:
    cmp.l a3,d3
    bgt.s .end2
    movea.l 20(a1),a3
    move.l a3,d6
    bne.s .end2
    clr.l 16(a1)
.end2:
    cmp.l a4,d4
    bgt.s .end3
    movea.l 32(a1),a4
    move.l a4,d6
    bne.s .end3
    clr.l 28(a1)
.end3:
    cmp.l a5,d5
    bgt.s .end4
    movea.l 44(a1),a5
    move.l a5,d6
    bne.s .end4
    clr.l 40(a1)
.end4:
    move.l a2,(a1)
    move.l a3,12(a1)
    move.l a4,24(a1)
    move.l a5,36(a1)
    rts
