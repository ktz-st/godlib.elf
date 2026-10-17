; GCC -mshort -mfastcall bridge; GodLib LeonardMod internal bridge.
        section text
        xdef LeonardMod_CoreStart,LeonardMod_Stop,LeonardMod_Tick,LeonardMod_GetVisualizer
        xdef LeonardMod_SetRasterDebug
; a0 raw MOD, a1 -> {DMA, frequency, volume, code}, d0 rate (1/2).
LeonardMod_CoreStart:
        movem.l d2-d7/a2-a6,-(sp)
        move.w sr,-(sp)
        move.l a0,-(sp)
        move.l (a1)+,LeoSampleBuffer
        move.l (a1)+,LeoFrequencyBuffer
        move.l (a1)+,LeoVolumeBuffer
        move.l (a1)+,LeoCodeBuffer
        move.b d0,LeoRate
        clr.w renderingBlock
        moveq #1,d0             ; STE-compatible format on STE/MegaSTE
        bsr paulaInstall
        move.l a0,d0
        beq.s .finish
        st LeoInstalled
        lea mt_music,a0
        bsr setUserRout
        move.l (sp),a0
        bsr mt_init
        move.l #$40004000,mt_Visualizer
        move.l #$40004000,mt_Visualizer+4
        st mt_Enable
        moveq #1,d0
.finish:
        addq.l #4,sp
        move.w (sp)+,sr
        movem.l (sp)+,d2-d7/a2-a6
        rts
LeonardMod_Stop:
        movem.l d0-a6,-(sp)
        move.w sr,-(sp)
        tst.b LeoInstalled
        beq.s .done
        sf LeoInstalled
        bsr mt_end
        bsr paulaClose
.done:
        move.w (sp)+,sr
        movem.l (sp)+,d0-a6
        rts
; Call from the main loop, once per Screen_Update, as in Leonard's original demo.
; paulaTick temporarily uses USP as its mixer stack; preserve the caller's USP.
LeonardMod_Tick:
        movem.l d0-a6,-(sp)
        move.w sr,-(sp)
        move.l usp,a0
        move.l a0,-(sp)
        tst.b LeoInstalled
        beq.s .done
        tst.b LeoRaster
        beq.s .normal
        move.w $ffff8240.w,LeoSavedColour
        move.w #$700,$ffff8240.w
.normal:
        bsr paulaTick
        tst.b LeoRaster
        beq.s .done
        move.w LeoSavedColour,$ffff8240.w
.done:
        move.l (sp)+,a0
        move.l a0,usp
        move.w (sp)+,sr
        movem.l (sp)+,d0-a6
        rts
LeonardMod_SetRasterDebug:
        move.b d0,LeoRaster
        rts
LeonardMod_GetVisualizer:
        move.l a0,d0
        beq.s .return
        move.w sr,d1
        ori.w #$700,sr
        tst.b mt_Enable
        beq.s .idle
        move.l mt_Visualizer,(a0)+
        move.l mt_Visualizer+4,(a0)
        bra.s .restore
.idle:
        move.l #$40004000,(a0)+
        move.l #$40004000,(a0)
.restore:
        move.w d1,sr
.return:
        rts

        xdef LeonardMod_GetOscilloscope
        xref ModScope_Sample

; A0 -> 4*64 S8 samples; main-loop supervisor API, no retained pointers.
LeonardMod_GetOscilloscope:
        move.l a0,d0
        beq .return
        movem.l d2-d7/a2-a6,-(sp)
        move.l a0,a6
        suba.w #96,sp
        move.l sp,a0
        move.w sr,d7
        ori.w #$700,sr
        lea channela(pc),a1
        lea pSampA(pc),a2
        moveq #0,d6
.voice:
        move.l (a2),(a0)+
        move.l 4(a2),(a0)+
        move.l (a1),d0
        move.l d0,(a0)+
        moveq #0,d1
        move.w 4(a1),d1
        add.l d1,d1
        add.l d1,d0
        move.l d0,(a0)+
        move.w 6(a1),(a0)+
        move.w 8(a1),(a0)+
        clr.w (a0)+
        moveq #0,d0
        tst.b mt_Enable
        beq.s .inactive
        move.w emudmacon(pc),d1
        btst d6,d1
        sne d0
.inactive:
        move.w d0,(a0)+
        lea 16(a1),a1
        lea 10(a2),a2
        addq.w #1,d6
        cmpi.w #4,d6
        bne.s .voice
        move.w d7,sr
        move.l sp,a0
        move.l a6,a1
        move.l #37144344,d0
        jsr ModScope_Sample
        adda.w #96,sp
        movem.l (sp)+,d2-d7/a2-a6
.return:
        rts


        xdef LeonardMod_GetStereoOscilloscope
        xref ModScope_CopyStereo
LeonardMod_GetStereoOscilloscope:
        move.l a0,d0
        beq.s .return
        move.w sr,-(sp)
        ori.w #$700,sr
        tst.b LeoInstalled
        beq.s .idle
        tst.b mt_Enable
        beq.s .idle
        move.l LeoSampleBuffer(pc),a1
        move.w renderingBlock(pc),d0
        sub.w blockSize(pc),d0
        bpl.s .offset
        move.w blockSize(pc),d0
        add.w d0,d0
        add.w blockSize(pc),d0
.offset:
        adda.w d0,a1
        bra.s .copy
.idle:
        suba.l a1,a1
.copy:
        jsr ModScope_CopyStereo
        move.w (sp)+,sr
.return:
        rts

ATARIPAULA=1
        include "music/leonardpaula.s"
        include "music/leonardtracker.s"
        section data
LeoRate: dc.b 2
LeoInstalled: dc.b 0
LeoRaster: dc.b 0
        even
LeoSavedColour: dc.w 0
