        section .text,code
        xdef Tfmx_InitCore,Tfmx_StartCore,Tfmx_StopCore,Tfmx_Tick,TfmxDmaWrite
        xref TfmxCore_Init,TfmxCore_StartSong,TfmxCore_Stop,TfmxCore_Tick
        xref gTfmxPlaying,gTfmxBackend,gTfmxTicks,gTfmxRasterEnabled
        xref Tfmx_LancePrepare,Tfmx_LanceFinish
        xref paula_start_dma,paula_mix_next,LeonardMixer_Tick,updateDmacon,emuDFF
Tfmx_InitCore:
        movem.l d3-d7/a2-a6,-(sp)
        move.w sr,-(sp)
        move.l a0,d0
        move.l a1,d1
        jsr TfmxCore_Init
        move.w (sp)+,sr
        movem.l (sp)+,d3-d7/a2-a6
        rts
Tfmx_StartCore:
        movem.l d3-d7/a2-a6,-(sp)
        move.w sr,-(sp)
        jsr TfmxCore_StartSong
        move.w (sp)+,sr
        movem.l (sp)+,d3-d7/a2-a6
        rts
Tfmx_StopCore:
        movem.l d0-a6,-(sp)
        move.w sr,-(sp)
        jsr TfmxCore_Stop
        move.w (sp)+,sr
        movem.l (sp)+,d0-a6
        rts
TfmxDmaWrite:
; Snapshot EVERY set/clear, including same-address retriggers inside a macro.
        movem.l d0-a6,-(sp)
        move.w sr,-(sp)
        jsr updateDmacon
        move.w (sp)+,sr
        movem.l (sp)+,d0-a6
        rts
Tfmx_Tick:
        movem.l d0-a6,-(sp)
        move.w sr,-(sp)
        move.l usp,a0
        move.l a0,-(sp)
        tst.b gTfmxPlaying
        beq.s .done
        move.w $ffff8240.w,-(sp)
        tst.b gTfmxRasterEnabled
        beq.s .dma
        move.w #$700,$ffff8240.w
.dma:
; Start the already mixed buffer before variable-cost sequencing/stitching.
; Lance uses one-shot DMA: delaying this by several ms cuts/gaps the audio.
        tst.b gTfmxBackend
        bne.s .leo
        jsr paula_start_dma
.seq:
        jsr TfmxCore_Tick
        addq.l #1,gTfmxTicks
        jsr Tfmx_LancePrepare
        jsr paula_mix_next
        jsr Tfmx_LanceFinish
        bra.s .restore
.leo:
        jsr LeonardMixer_Tick
.restore:
        move.w (sp)+,$ffff8240.w
.done:
        move.l (sp)+,a0
        move.l a0,usp
        move.w (sp)+,sr
        movem.l (sp)+,d0-a6
        rts
; Like UGH: run the sequencer only when Leonard accepts a PCM block.
; paulaTick preserves registers and invokes this before entering user mode.
        xdef Tfmx_LeonardSequence
Tfmx_LeonardSequence:
        jsr TfmxCore_Tick
        addq.l #1,gTfmxTicks
        rts
        xdef TfmxStubs
TfmxCustom equ emuDFF
        section .bss,bss
TfmxStubs: ds.b 16
        include "music/tfmxcore.s"
