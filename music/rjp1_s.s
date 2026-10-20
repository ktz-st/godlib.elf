        section .text,code
        xdef Rjp1_Tick,Rjp1_SetRasterDebug,Rjp1_GetVolumes
        xref gRjp1Playing,gRjp1Mixer,RjpLeo_Sync,LeonardMixer_Tick,PaulaMixer_Tick,PaulaMixer_Channels
Rjp1_Tick:
        movem.l d0-a6,-(sp)
        move.w sr,-(sp)
        tst.b gRjp1Playing
        beq.s .done
        tst.b RjpRaster
        beq.s .normal
        move.w $ffff8240.w,RjpColour
        move.w #$700,$ffff8240.w
.normal:
        tst.b gRjp1Mixer
        bne.s .leo
        jsr PaulaMixer_Tick
        jsr RjpCore_Tick
        bra.s .count
.leo:
; Sequencer is driven by PAL VBL, NOT paulaTick's conditional user routine.
        jsr RjpCore_Tick
        jsr RjpLeo_Sync
        jsr LeonardMixer_Tick
.count:
        addq.l #1,gRjp1Ticks
        tst.b RjpRaster
        beq.s .done
        move.w RjpColour,$ffff8240.w
.done:
        move.w (sp)+,sr
        movem.l (sp)+,d0-a6
        rts
Rjp1_SetRasterDebug:
        move.b d0,RjpRaster
        rts
Rjp1_GetVolumes:
        move.l a0,d0
        beq.s .return
        move.w sr,d1
        ori.w #$700,sr
        lea PaulaMixer_Channels,a1
        moveq #3,d0
.voice:
        clr.w (a0)
        tst.b gRjp1Playing
        beq.s .next
        move.w 12(a1),(a0)
.next:
        addq.l #2,a0
        lea 20(a1),a1
        dbf d0,.voice
        move.w d1,sr
.return: rts
        include "music/rjp1core.s"
        section .bss,bss
        xdef gRjp1Ticks
gRjp1Ticks: ds.l 1
RjpRaster: ds.b 1
        even
RjpColour: ds.w 1
