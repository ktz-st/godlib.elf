; Imploder data decoder extracted from MICRO's original 364-byte FRT block.
; Original FRT $66560..$666CC, raw SHA256:
; b0f7935bfc9a553218b687106b439c14d486bb12b14d80c683a09854b4baccbb
; Before ABI adaptation the reassembled source matches that block byte-for-byte.
; GodLib GCC ELF -mfastcall: a0=a1=even-aligned in-place buffer, d0=-1 success.
; C wrapper copies separate source into destination before calling this routine.
; The only core change is adding a5 to both MOVEM masks (callee-saved by GCC).
; Numeric labels retain original raw offsets for disassembly comparison.
; No INCBIN, absolute game addresses, custom-chip use or mutable codec state.

    xdef Imploder_DepackAsm
    xdef Imploder_ChecksumAsm
    text

Imploder_DepackAsm:
    CMPI.L    #'IMP!',(A0)
    BNE.S     .L003C
    MOVEM.L   D2-D5/A2-A5,-(A7)
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
    BMI.S     .L0028
    SUBQ.L    #1,A3
.L0028:
    ; Stack copy of the seven trailer longs (offsets and bit counts).
    LEA       -$1C(A7),A7
    MOVEA.L   A7,A1
    MOVEQ     #6,D0
.L0030:
    MOVE.L    (A2)+,(A1)+
    DBRA      D0,.L0030
    MOVEA.L   A7,A1
    BRA.W     .L0040
.L003C:
    MOVEQ     #0,D0
    RTS

.L0040:
    TST.L     D2
    BEQ.S     .L004A
.L0044: ; Copy the initial/later literal run backward.
    MOVE.B    -(A3),-(A4)
    SUBQ.L    #1,D2
    BNE.S     .L0044
.L004A:
    CMPA.L    A4,A5
    BCS.S     .L0062
    LEA       $1C(A7),A7
    MOVEQ     #-1,D0
    CMPA.L    A3,A5
    BEQ.S     .L005A
    MOVEQ     #0,D0
.L005A:
    MOVEM.L   (A7)+,D2-D5/A2-A5
    TST.L     D0
    RTS

.L0062:
    ADD.B     D3,D3
    BNE.S     .L006A
    MOVE.B    -(A3),D3
    ADDX.B    D3,D3
.L006A:
    BCC.S     .L00D4
    ADD.B     D3,D3
    BNE.S     .L0074
    MOVE.B    -(A3),D3
    ADDX.B    D3,D3
.L0074:
    BCC.S     .L00CE
    ADD.B     D3,D3
    BNE.S     .L007E
    MOVE.B    -(A3),D3
    ADDX.B    D3,D3
.L007E:
    BCC.S     .L00C8
    ADD.B     D3,D3
    BNE.S     .L0088
    MOVE.B    -(A3),D3
    ADDX.B    D3,D3
.L0088:
    BCC.S     .L00C2
    MOVEQ     #0,D4
    ADD.B     D3,D3
    BNE.S     .L0094
    MOVE.B    -(A3),D3
    ADDX.B    D3,D3
.L0094:
    BCC.S     .L009E
    MOVE.B    -(A3),D4
    MOVEQ     #3,D0
    SUBQ.B    #1,D4
    BRA.S     .L00D8
.L009E:
    ADD.B     D3,D3
    BNE.S     .L00A6
    MOVE.B    -(A3),D3
    ADDX.B    D3,D3
.L00A6:
    ADDX.B    D4,D4
    ADD.B     D3,D3
    BNE.S     .L00B0
    MOVE.B    -(A3),D3
    ADDX.B    D3,D3
.L00B0:
    ADDX.B    D4,D4
    ADD.B     D3,D3
    BNE.S     .L00BA
    MOVE.B    -(A3),D3
    ADDX.B    D3,D3
.L00BA:
    ADDX.B    D4,D4
    ADDQ.B    #5,D4
    MOVEQ     #3,D0
    BRA.S     .L00D8
.L00C2:
    MOVEQ     #4,D4
    MOVEQ     #3,D0
    BRA.S     .L00D8
.L00C8:
    MOVEQ     #3,D4
    MOVEQ     #2,D0
    BRA.S     .L00D8
.L00CE:
    MOVEQ     #2,D4
    MOVEQ     #1,D0
    BRA.S     .L00D8
.L00D4:
    MOVEQ     #1,D4
    MOVEQ     #0,D0
.L00D8:
    MOVEQ     #0,D5
    MOVE.W    D0,D1
    ADD.B     D3,D3
    BNE.S     .L00E4
    MOVE.B    -(A3),D3
    ADDX.B    D3,D3
.L00E4:
    BCC.S     .L00FC
    ADD.B     D3,D3
    BNE.S     .L00EE
    MOVE.B    -(A3),D3
    ADDX.B    D3,D3
.L00EE:
    BCC.S     .L00F8
    MOVE.B    .token_base(PC,D0.W),D5
    ADDQ.B    #8,D0
    BRA.S     .L00FC
.L00F8:
    MOVEQ     #2,D5
    ADDQ.B    #4,D0
.L00FC:
    MOVE.B    .token_bits(PC,D0.W),D0
.L0100:
    ADD.B     D3,D3
    BNE.S     .L0108
    MOVE.B    -(A3),D3
    ADDX.B    D3,D3
.L0108:
    ADDX.W    D2,D2
    SUBQ.B    #1,D0
    BNE.S     .L0100
    ADD.W     D5,D2
    MOVEQ     #0,D5
    MOVEA.L   D5,A2
    MOVE.W    D1,D0
    ADD.B     D3,D3
    BNE.S     .L011E
    MOVE.B    -(A3),D3
    ADDX.B    D3,D3
.L011E:
    BCC.S     .L013A
    ADD.W     D1,D1
    ADD.B     D3,D3
    BNE.S     .L012A
    MOVE.B    -(A3),D3
    ADDX.B    D3,D3
.L012A:
    BCC.S     .L0134
    MOVEA.W   8(A1,D1.W),A2
    ADDQ.B    #8,D0
    BRA.S     .L013A
.L0134:
    MOVEA.W   (A1,D1.W),A2
    ADDQ.B    #4,D0
.L013A:
    MOVE.B    $10(A1,D0.W),D0
.L013E:
    ADD.B     D3,D3
    BNE.S     .L0146
    MOVE.B    -(A3),D3
    ADDX.B    D3,D3
.L0146:
    ADDX.L    D5,D5
    SUBQ.B    #1,D0
    BNE.S     .L013E
    ADDQ.W    #1,A2
    ADDA.L    D5,A2
    ADDA.L    A4,A2
.L0152: ; d4 = match length - 1, including 256-byte matches.
    MOVE.B    -(A2),-(A4)
    DBRA      D4,.L0152
    BRA.W     .L0040
.token_base:
    dc.b 6,10,10,18
.token_bits:
    dc.b 1,1,1,1,2,3,3,4,4,5,7,14


; Same original unsigned word-sum as MICRO's drive_imploder_checksum.
; a0 = packed bytes, d0 = even byte count excluding final checksum.
; Returns d0 = sum + 7 (mod 2^32). Preserve d2 for both GCC ABIs.
; The byte path preserves the public wrapper's unaligned-source support.
Imploder_ChecksumAsm:
    move.l d2,-(sp)
    move.l d0,d1
    lsr.l #1,d1
    moveq #7,d0
    tst.l d1
    beq.s .sum_done
    move.l a0,d2
    btst #0,d2
    bne.s .sum_odd
.sum_even:
    moveq #0,d2
    move.w (a0)+,d2
    add.l d2,d0
    subq.l #1,d1
    bne.s .sum_even
    bra.s .sum_done
.sum_odd:
    moveq #0,d2
    move.b (a0)+,d2
    lsl.w #8,d2
    move.b (a0)+,d2
    add.l d2,d0
    subq.l #1,d1
    bne.s .sum_odd
.sum_done:
    move.l (sp)+,d2
    rts
