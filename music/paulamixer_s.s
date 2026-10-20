; Shared Lance Paula mixer: no ProTracker dependencies.
        xdef Paula_Init,Paula_SetReplayBuf,Paula_SetWorkspace,Paula_SetInitScratch
        xdef PaulaMixer_Tick,PaulaMixer_Stop,PaulaMixer_Channels
        xdef paula_buffer_rd_idx
        xdef paula_buffer_str
        xdef paula_calc
        xdef paula_channel_0
        xdef paula_channel_1
        xdef paula_channel_2
        xdef paula_channel_3
        xdef paula_clear_buffer
        xdef paula_done
        xdef paula_dummy_spl
        xdef paula_mix_next
        xdef paula_replay_buf_p
        xdef paula_start_dma
Paula_Init:
	movem.l	d0-a6,-(a7)
	jsr	paula_init
	movem.l	(a7)+,d0-a6
	rts

Paula_SetReplayBuf:
	move.l	a0,paula_replay_buf_p
	rts

Paula_SetInitScratch:
    move.l a0,paula_freq_list
    rts

; a0 -> seven resident workspace pointers, in C layout order.
Paula_SetWorkspace:
    move.l (a0)+,paula_freq_table
    move.l (a0)+,paula_frame_freq_t
    move.l (a0)+,paula_frame_freq_p
    move.l (a0)+,paula_volume_base
    move.l (a0)+,paula_div_table
    move.l (a0)+,paula_mixcode_p
    move.l (a0)+,paula_mixer_code
    rts

PaulaMixer_Tick:
        movem.l d0-a6,-(sp)
        move.w sr,-(sp)
        jsr paula_calc
        move.w (sp)+,sr
        movem.l (sp)+,d0-a6
        rts
PaulaMixer_Stop:
        movem.l d0-a6,-(sp)
        move.w sr,-(sp)
        jsr paula_done
        move.w (sp)+,sr
        movem.l (sp)+,d0-a6
        rts
        include "music/lancepaula.s"
PaulaMixer_Channels equ paula_channel_0
