; Optimized Imploder data decoder, derived from MICRO's original FRT block.
; Original: $66560..$666CC, SHA256:
; b0f7935bfc9a553218b687106b439c14d486bb12b14d80c683a09854b4baccbb
; Baseline faithful adaptation: GodLib commit f3134ac.
; GCC ELF -mfastcall: a0=a1=even-aligned in-place buffer; d0=-1 on success.
; C wrapper preserves separate/read-only source by copying packed bytes first.
; No heap, input prefetch across literals, mutable globals or lookup cache.
;
; d7.w counts available bits. ADD.B + flag-neutral DBRA returns the same carry
; as the original sentinel reader. Offsets are extracted in byte groups (1..16
; bits); the initial sentinel is cleared before using grouped shifts.
; The initial literal run is U32; subsequent literal runs are at most 16401.
; Every match has at least two bytes, copied before the remaining DBRA loop.
; d2-d7/a2-a5 are preserved. a6 is untouched. Extra core stack: 8 bytes.

    xdef Imploder_DepackAsm
    xdef Imploder_ChecksumAsm
    text

Imploder_DepackAsm:
    CMPI.L    #'IMP!',(A0)
    BNE.S     .not_packed
    MOVEM.L   D2-D7/A2-A5,-(A7)
    MOVEA.L   A0,A3
    MOVEA.L   A1,A4
    MOVEA.L   A1,A5
    TST.L     (A0)+
    ADDA.L    (A0)+,A4
    ADDA.L    (A0)+,A3
    MOVEA.L   A3,A2
    MOVE.L    (A2)+,-(A0)
    MOVE.L    (A2)+,-(A0)
    MOVE.L    (A2)+,-(A0)
    MOVE.L    (A2)+,D2
    MOVE.W    (A2)+,D3
    BMI.S     .copy_trailer
    SUBQ.L    #1,A3
.copy_trailer:
    ; Stack copy of the seven trailer longs (offsets and bit counts).
    LEA       -$1C(A7),A7
    MOVEA.L   A7,A1
    MOVEQ     #6,D0
.trailer_long:
    MOVE.L    (A2)+,(A1)+
    DBRA      D0,.trailer_long
    MOVEA.L   A7,A1
    ; Replace the sentinel with an explicit count, updated by flag-neutral DBRA.
    moveq #0,d6
    move.b d3,d6
    move.w d6,d0
    subq.w #1,d0
    and.w d0,d3
    moveq #7,d7
.count_initial_bits:
    lsr.b #1,d6
    bcs.s .initial_count_ready
    subq.w #1,d7
    bra.s .count_initial_bits
.initial_count_ready:
    bra.w .initial_literals
.not_packed:
    MOVEQ     #0,D0
    RTS

.initial_literals:
    tst.l d2
    beq.s .check_end
.initial_copy:
    move.b -(a3),-(a4)
    subq.l #1,d2
    bne.s .initial_copy
    bra.s .check_end
.next_literals:
    TST.L     D2
    BEQ.S     .check_end
.literal_byte: ; Later literal runs fit in 16 bits.
    MOVE.B    -(A3),-(A4)
    SUBQ.W    #1,D2
    BNE.S     .literal_byte
.check_end:
    CMPA.L    A4,A5
    BCS.S     .match_length
    LEA       $1C(A7),A7
    MOVEQ     #-1,D0
    CMPA.L    A3,A5
    BEQ.S     .return
    MOVEQ     #0,D0
.return:
    MOVEM.L   (A7)+,D2-D7/A2-A5
    TST.L     D0
    RTS

.match_length:
    ADD.B     D3,D3
    DBRA      D7,.length_flag1
    MOVE.B    -(A3),D3
    MOVEQ     #7,D7
    ADD.B     D3,D3
.length_flag1:
    BCC.W     .length2
    ADD.B     D3,D3
    DBRA      D7,.length_flag2
    MOVE.B    -(A3),D3
    MOVEQ     #7,D7
    ADD.B     D3,D3
.length_flag2:
    BCC.S     .length3
    ADD.B     D3,D3
    DBRA      D7,.length_flag3
    MOVE.B    -(A3),D3
    MOVEQ     #7,D7
    ADD.B     D3,D3
.length_flag3:
    BCC.S     .length4
    ADD.B     D3,D3
    DBRA      D7,.length_flag4
    MOVE.B    -(A3),D3
    MOVEQ     #7,D7
    ADD.B     D3,D3
.length_flag4:
    BCC.S     .length5
    MOVEQ     #0,D4
    ADD.B     D3,D3
    DBRA      D7,.length_flag5
    MOVE.B    -(A3),D3
    MOVEQ     #7,D7
    ADD.B     D3,D3
.length_flag5:
    BCC.S     .length_medium
    MOVE.B    -(A3),D4
    MOVEQ     #3,D0
    SUBQ.B    #1,D4
    BRA.S     .literal_count
.length_medium:
    ADD.B     D3,D3
    DBRA      D7,.length_bit1
    MOVE.B    -(A3),D3
    MOVEQ     #7,D7
    ADD.B     D3,D3
.length_bit1:
    ADDX.B    D4,D4
    ADD.B     D3,D3
    DBRA      D7,.length_bit2
    MOVE.B    -(A3),D3
    MOVEQ     #7,D7
    ADD.B     D3,D3
.length_bit2:
    ADDX.B    D4,D4
    ADD.B     D3,D3
    DBRA      D7,.length_bit3
    MOVE.B    -(A3),D3
    MOVEQ     #7,D7
    ADD.B     D3,D3
.length_bit3:
    ADDX.B    D4,D4
    ADDQ.B    #5,D4
    MOVEQ     #3,D0
    BRA.S     .literal_count
.length5:
    MOVEQ     #4,D4
    MOVEQ     #3,D0
    BRA.S     .literal_count
.length4:
    MOVEQ     #3,D4
    MOVEQ     #2,D0
    BRA.S     .literal_count
.length3:
    MOVEQ     #2,D4
    MOVEQ     #1,D0
    BRA.S     .literal_count
.length2:
    MOVEQ     #1,D4
    MOVEQ     #0,D0
.literal_count:
    MOVEQ     #0,D5
    MOVE.W    D0,D1
    ADD.B     D3,D3
    DBRA      D7,.literal_flag1
    MOVE.B    -(A3),D3
    MOVEQ     #7,D7
    ADD.B     D3,D3
.literal_flag1:
    BCC.S     .literal_one
    ADD.B     D3,D3
    DBRA      D7,.literal_flag2
    MOVE.B    -(A3),D3
    MOVEQ     #7,D7
    ADD.B     D3,D3
.literal_flag2:
    BCC.S     .literal_medium
    MOVE.B    .token_base(PC,D0.W),D5
    ADDQ.B    #8,D0
    BRA.S     .literal_bits
.literal_one:
    add.b d3,d3
    dbra d7,.literal_one_ready
    move.b -(a3),d3
    moveq #7,d7
    add.b d3,d3
.literal_one_ready:
    addx.w d2,d2
    bra.s .literal_done
.literal_medium:
    MOVEQ     #2,D5
    ADDQ.B    #4,D0
.literal_bits:
    MOVE.B    .token_bits(PC,D0.W),D0
.literal_bit_loop:
    ADD.B     D3,D3
    DBRA      D7,.literal_bit_ready
    MOVE.B    -(A3),D3
    MOVEQ     #7,D7
    ADD.B     D3,D3
.literal_bit_ready:
    ADDX.W    D2,D2
    SUBQ.B    #1,D0
    BNE.S     .literal_bit_loop
    ADD.W     D5,D2
.literal_done:
    MOVEQ     #0,D5
    MOVEA.L   D5,A2
    MOVE.W    D1,D0
    ADD.B     D3,D3
    DBRA      D7,.offset_flag1
    MOVE.B    -(A3),D3
    MOVEQ     #7,D7
    ADD.B     D3,D3
.offset_flag1:
    BCC.S     .offset_bit_count
    ADD.W     D1,D1
    ADD.B     D3,D3
    DBRA      D7,.offset_flag2
    MOVE.B    -(A3),D3
    MOVEQ     #7,D7
    ADD.B     D3,D3
.offset_flag2:
    BCC.S     .offset_medium
    MOVEA.W   8(A1,D1.W),A2
    ADDQ.B    #8,D0
    BRA.S     .offset_bit_count
.offset_medium:
    MOVEA.W   (A1,D1.W),A2
    ADDQ.B    #4,D0
.offset_bit_count:
    MOVE.B    $10(A1,D0.W),D0
    bra.s .offset_dispatch
.token_base:
    dc.b 6,10,10,18
.token_bits:
    dc.b 1,1,1,1,2,3,3,4,4,5,7,14

.offset_dispatch:
    moveq #0,d5
    move.b d3,d5
    sub.w d0,d7
    bmi.s .offset_needs_bytes
    lsl.w d0,d5
    move.b d5,d3
    lsr.w #8,d5
    bra.s .offset_done
.offset_needs_bytes:
    subq.w #8,d0
    bcs.s .offset_align_right
    lsl.w d0,d5
    bra.s .offset_aligned
.offset_align_right:
    neg.w d0
    lsr.w d0,d5
.offset_aligned:
    neg.w d7
    cmpi.w #8,d7
    bls.s .offset_final_byte
    moveq #0,d6
    move.b -(a3),d6
    subq.w #8,d7
    lsl.w d7,d6
    or.w d6,d5
.offset_final_byte:
    moveq #0,d6
    move.b -(a3),d6
    lsl.w d7,d6
    move.b d6,d3
    lsr.w #8,d6
    or.w d6,d5
    neg.w d7
    addq.w #8,d7
.offset_done:
    ADDQ.W    #1,A2
    ADDA.L    D5,A2
    ADDA.L    A4,A2
    ; All matches contain at least two bytes. Handle the common length 2
    ; without a loop, and retain DBRA for any remaining bytes.
    move.b -(a2),-(a4)
    move.b -(a2),-(a4)
    subq.w #2,d4
    bcs.w .next_literals
.match_byte:
    MOVE.B    -(A2),-(A4)
    DBRA      D4,.match_byte
    BRA.W     .next_literals


; Original unsigned word-sum, with eight-word aligned blocks.
; a0 = packed bytes, d0 = even byte count excluding final checksum.
; Returns d0 = sum + 7 (mod 2^32). Preserve d2-d3 for GCC.
; The byte path preserves the public wrapper's unaligned-source support.
Imploder_ChecksumAsm:
    movem.l d2-d3,-(sp)
    move.l d0,d1
    lsr.l #1,d1
    moveq #7,d0
    moveq #0,d2
    tst.l d1
    beq.w .sum_done
    move.l a0,d3
    btst #0,d3
    bne.w .sum_odd
    move.w d1,d3
    andi.w #7,d3
    lsr.l #3,d1
    subq.w #1,d3
    bmi.s .sum_bulk
.sum_remainder:
    move.w (a0)+,d2
    add.l d2,d0
    dbra d3,.sum_remainder
.sum_bulk:
    tst.l d1
    beq.s .sum_done
.sum_eight:
    move.w (a0)+,d2
    add.l d2,d0
    move.w (a0)+,d2
    add.l d2,d0
    move.w (a0)+,d2
    add.l d2,d0
    move.w (a0)+,d2
    add.l d2,d0
    move.w (a0)+,d2
    add.l d2,d0
    move.w (a0)+,d2
    add.l d2,d0
    move.w (a0)+,d2
    add.l d2,d0
    move.w (a0)+,d2
    add.l d2,d0
    subq.l #1,d1
    bne.s .sum_eight
    bra.s .sum_done
.sum_odd:
    move.b (a0)+,d2
    lsl.w #8,d2
    move.b (a0)+,d2
    add.l d2,d0
    subq.l #1,d1
    bne.s .sum_odd
.sum_done:
    movem.l (sp)+,d2-d3
    rts
