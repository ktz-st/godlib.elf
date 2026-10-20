; GCC -mshort -mfastcall bridge; GodLib LeonardMod internal bridge.
        section text
        xdef LeonardMod_CoreStart,LeonardMod_Stop,LeonardMod_Tick,LeonardMod_GetVisualizer
        xdef LeonardMod_SetRasterDebug
        xref LeonardMixer_SetUserRoutine,LeonardMixer_Tick,LeonardMixer_Stop
        xref LeoInstalled,LeoSampleBuffer,renderingBlock,blockSize,channela,pSampA,emudmacon
; a0 raw MOD; Paula workspace is already installed by the common C allocator.
LeonardMod_CoreStart:
        movem.l d2-d7/a2-a6,-(sp)
        move.w sr,-(sp)
        bsr mt_init
        lea mt_music,a0
        jsr LeonardMixer_SetUserRoutine
        move.l #$40004000,mt_Visualizer
        move.l #$40004000,mt_Visualizer+4
        st mt_Enable
        moveq #1,d0
        move.w (sp)+,sr
        movem.l (sp)+,d2-d7/a2-a6
        rts
LeonardMod_Stop:
        movem.l d0-a6,-(sp)
        move.w sr,-(sp)
        tst.b mt_Enable
        beq.s .done
        bsr mt_end
        jsr LeonardMixer_Stop
.done:
        move.w (sp)+,sr
        movem.l (sp)+,d0-a6
        rts
LeonardMod_Tick:
        movem.l d0-a6,-(sp)
        move.w sr,-(sp)
        tst.b mt_Enable
        beq.s .done
        tst.b LeoRaster
        beq.s .normal
        move.w $ffff8240.w,LeoSavedColour
        move.w #$700,$ffff8240.w
.normal:
        jsr LeonardMixer_Tick
        tst.b LeoRaster
        beq.s .done
        move.w LeoSavedColour,$ffff8240.w
.done:
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
        lea channela,a1
        lea pSampA,a2
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
        move.w emudmacon,d1
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
        move.l LeoSampleBuffer,a1
        move.w renderingBlock,d0
        sub.w blockSize,d0
        bpl.s .offset
        move.w blockSize,d0
        add.w d0,d0
        add.w blockSize,d0
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
        xref emuDFF,updateDmacon
        include "music/leonardtracker.s"
        section data
LeoRaster: dc.b 0
        even
LeoSavedColour: dc.w 0
