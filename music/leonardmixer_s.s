; Shared native Leonard Paula bridge; no MOD sequencer dependency.
        section text
        xdef LeonardMixer_CoreInit,LeonardMixer_Tick,LeonardMixer_Stop,LeonardMixer_SetUserRoutine
        xdef channela,pSampA,emudmacon,emuDFF,updateDmacon
        xdef LeoInstalled,LeoSampleBuffer,renderingBlock,blockSize,nullSample
        xdef LeonardMixer_CodeBegin,LeonardMixer_CodeEnd
LeonardMixer_CoreInit:
        movem.l d2-d7/a2-a6,-(sp)
        move.w sr,-(sp)
        move.l (a0)+,LeoSampleBuffer
        move.l (a0)+,LeoFrequencyBuffer
        move.l (a0)+,LeoVolumeBuffer
        move.l (a0)+,LeoCodeBuffer
        move.b d0,LeoRate
        clr.w renderingBlock
        moveq #1,d0
        bsr paulaInstall
        move.l a0,d0
        beq.s .done
        st LeoInstalled
        moveq #1,d0
.done:
        move.w (sp)+,sr
        movem.l (sp)+,d2-d7/a2-a6
        rts
LeonardMixer_SetUserRoutine:
        bra setUserRout
LeonardMixer_Stop:
        movem.l d0-a6,-(sp)
        move.w sr,-(sp)
        tst.b LeoInstalled
        beq.s .done
        sf LeoInstalled
        bsr paulaClose
.done:
        move.w (sp)+,sr
        movem.l (sp)+,d0-a6
        rts
LeonardMixer_Tick:
        movem.l d0-a6,-(sp)
        move.w sr,-(sp)
        move.l usp,a0
        move.l a0,-(sp)
        tst.b LeoInstalled
        beq.s .done
        bsr paulaTick
.done:
        move.l (sp)+,a0
        move.l a0,usp
        move.w (sp)+,sr
        movem.l (sp)+,d0-a6
        rts
LeonardMixer_CodeBegin:
        include "music/leonardpaula.s"
LeonardMixer_CodeEnd:
        section data
LeoRate: dc.b 2
LeoInstalled: dc.b 0
        even
