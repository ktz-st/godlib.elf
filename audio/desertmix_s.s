; Gameplay PCM mixer from desert/game.ovl, TEXT $1F78..$2245.
; Original GCC stack ABI and mixing instructions retained.
    section .text
    ifd VERIFY_ORIGINAL
gDesertMixerState equ $2da74
    else
    xref gDesertMixerState
    xdef DesertMixer_CoreFill,DesertMixer_Lock,DesertMixer_Unlock
DesertMixer_Lock:
    move.w sr,d0
    ori.w #$0700,sr
    rts
DesertMixer_Unlock:
    move.w d0,sr
    rts
; GodLib fastcall: D0=ring offset, D1=byte count (both U32).
DesertMixer_CoreFill:
    move.l d1,-(sp)
    move.l d0,-(sp)
    bsr.w Desert_1F78
    addq.l #8,sp
    rts
    endc
_B2DA74 equ gDesertMixerState+$0
_B2DA78 equ gDesertMixerState+$4
_B2DA7C equ gDesertMixerState+$8
_B2DA80 equ gDesertMixerState+$c
_B2DA84 equ gDesertMixerState+$10
_B2DA88 equ gDesertMixerState+$14
_B2DA8C equ gDesertMixerState+$18
_B2DA90 equ gDesertMixerState+$1c
_B2DB2C equ gDesertMixerState+$b8
_B2DB80 equ gDesertMixerState+$10c
_B2DB84 equ gDesertMixerState+$110
_B2DB8C equ gDesertMixerState+$118

Desert_1F78:
    LEA       -$C(A7),A7
    MOVEM.L   D2-D7/A2-A6,-(A7)
    MOVE.L    $40(A7),$2E(A7)
    MOVE.W    $30(A7),$36(A7)
    MOVEA.W   $3E(A7),A0
    ADDA.L    #_B2DB8C,A0
    MOVE.L    A0,$32(A7)
    MOVEA.L   (_B2DB84).L,A6
    TST.W     $36(A7)
    BLE.W     Desert_2112
    MOVEA.L   A0,A3
    MOVEA.L   (_B2DA90).L,A0
    MOVEA.W   $30(A7),A1
Desert_1FB4:
    MOVEA.W   A1,A4
    MOVE.W    A1,$2C(A7)
    MOVE.B    (_B2DB2C).L,D1
    MOVE.L    A6,D0
    SUB.L     A0,D0
    CMP.L     A4,D0
    BLT.W     Desert_20CC
    TST.B     D1
    BNE.W     Desert_20E0
Desert_1FD0:
    LEA       (_B2DB80).L,A2
    ADDA.L    (A2),A0
    MOVEA.L   A3,A5
    MOVE.W    A1,D0
    LSR.W     #5,D0
    BRA.S     Desert_1FEC
Desert_1FE0:
    MOVEM.L   (A0)+,D1-D7/A2
    MOVEM.L   D1-D7/A2,(A5)
    LEA       $20(A5),A5
Desert_1FEC:
    DBRA      D0,Desert_1FE0
    MOVE.W    A1,D0
    ANDI.W    #31,D0
    LSR.W     #2,D0
    BRA.S     Desert_1FFC
Desert_1FFA:
    MOVE.L    (A0)+,(A5)+
Desert_1FFC:
    DBRA      D0,Desert_1FFA
    MOVEA.L   (_B2DA90).L,A0
Desert_2006:
    ADDA.L    A4,A3
    MOVE.W    $2C(A7),D0
    SUB.W     A1,D0
    MOVEA.W   D0,A1
    ADDA.L    A4,A0
    CMPA.L    A0,A6
    BLE.W     Desert_20FC
    MOVE.L    A0,D0
    MOVE.L    D0,(_B2DA90).L
    CLR.W     D0
    CMP.W     A1,D0
    BLT.S     Desert_1FB4
Desert_2026:
    MOVEA.L   (_B2DA7C).L,A0
    CMPA.W    #0,A0
    BEQ.W     Desert_2112
    MOVE.L    (_B2DA78).L,D1
    MOVE.L    (_B2DA74).L,D0
    MOVE.W    $30(A7),D2
    MOVEA.L   $32(A7),A4
Desert_2048:
    MOVEA.W   D2,A1
    MOVEA.W   D2,A5
    MOVEA.L   D1,A3
    SUBA.L    D0,A3
    CMPA.L    A1,A3
    BGE.S     Desert_205A
    MOVE.W    D1,D2
    SUB.W     D0,D2
    MOVEA.W   D2,A1
Desert_205A:
    ADDA.L    D0,A0
    MOVEA.L   A4,A3
    MOVE.W    D2,D1
    LSR.W     #3,D1
    BRA.S     Desert_2084
Desert_2064:
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A3)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A3)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A3)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A3)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A3)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A3)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A3)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A3)+
Desert_2084:
    DBRA      D1,Desert_2064
    MOVE.W    D2,D1
    ANDI.W    #7,D1
    BRA.S     Desert_2094
Desert_2090:
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A3)+
Desert_2094:
    DBRA      D1,Desert_2090
    ADDA.L    A1,A4
    SUBA.W    D2,A5
    MOVE.W    A5,D2
    MOVE.L    A1,D0
    ADD.L     (_B2DA74).L,D0
    MOVE.L    D0,(_B2DA74).L
    MOVE.L    (_B2DA78).L,D1
    CMP.L     D0,D1
    BGT.S     Desert_20BE
    CLR.L     (_B2DA74).L
    MOVEQ     #0,D0
Desert_20BE:
    TST.W     D2
    BLE.S     Desert_2112
    MOVEA.L   (_B2DA7C).L,A0
    BRA.W     Desert_2048
Desert_20CC:
    MOVEA.W   A6,A1
    SUBA.W    A0,A1
    MOVEA.W   A1,A4
    TST.B     D1
    BEQ.W     Desert_1FD0
    CLR.W     D0
    CMP.W     A1,D0
    BGE.W     Desert_2006
Desert_20E0:
    CLR.W     D0
Desert_20E2:
    CLR.L     (A3,D0.W)
    ADDQ.W    #4,D0
    CMP.W     A1,D0
    BGE.W     Desert_2006
    CLR.L     (A3,D0.W)
    ADDQ.W    #4,D0
    CMP.W     A1,D0
    BLT.S     Desert_20E2
    BRA.W     Desert_2006
Desert_20FC:
    MOVEQ     #0,D0
    SUBA.L    A0,A0
    MOVE.L    D0,(_B2DA90).L
    CLR.W     D0
    CMP.W     A1,D0
    BLT.W     Desert_1FB4
    BRA.W     Desert_2026
Desert_2112:
    MOVE.L    (_B2DA84).L,D0
    BEQ.S     Desert_2178
    MOVEA.W   $30(A7),A3
    CMP.L     A3,D0
    BGE.W     Desert_21EE
    MOVE.W    D0,D2
    MOVEA.W   D0,A3
    LEA       (_B2DA80).L,A4
    MOVEA.L   $32(A7),A1
    MOVEA.L   (A4),A0
    MOVE.W    D2,D1
    LSR.W     #3,D1
    BRA.S     Desert_215A
Desert_213A:
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
Desert_215A:
    DBRA      D1,Desert_213A
    MOVE.W    D2,D1
    ANDI.W    #7,D1
    BRA.S     Desert_216A
Desert_2166:
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
Desert_216A:
    DBRA      D1,Desert_2166
    MOVE.L    A0,(A4)
    MOVE.L    A3,D0
    SUB.L     D0,(_B2DA84).L
Desert_2178:
    MOVE.L    (_B2DA8C).L,D0
    BEQ.S     Desert_21E4
    MOVEA.W   $30(A7),A0
    CMPA.L    D0,A0
    BLE.S     Desert_218E
    MOVE.W    D0,$36(A7)
    MOVEA.W   D0,A0
Desert_218E:
    MOVEA.L   $32(A7),A1
    MOVEA.L   (_B2DA88).L,A3
    MOVE.W    $36(A7),D1
    LSR.W     #3,D1
    BRA.S     Desert_21C0
Desert_21A0:
    MOVE.B    (A3)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A3)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A3)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A3)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A3)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A3)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A3)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A3)+,D0
    ADD.B     D0,(A1)+
Desert_21C0:
    DBRA      D1,Desert_21A0
    MOVE.W    $36(A7),D1
    ANDI.W    #7,D1
    BRA.S     Desert_21D2
Desert_21CE:
    MOVE.B    (A3)+,D0
    ADD.B     D0,(A1)+
Desert_21D2:
    DBRA      D1,Desert_21CE
    MOVE.L    A3,(_B2DA88).L
    MOVE.L    A0,D0
    SUB.L     D0,(_B2DA8C).L
Desert_21E4:
    MOVEM.L   (A7)+,D2-D7/A2-A6
    LEA       $C(A7),A7
    RTS

Desert_21EE:
    MOVE.W    $30(A7),D2
    LEA       (_B2DA80).L,A4
    MOVEA.L   $32(A7),A1
    MOVEA.L   (A4),A0
    MOVE.W    D2,D1
    LSR.W     #3,D1
    BRA.S     Desert_2224
Desert_2204:
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
Desert_2224:
    DBRA      D1,Desert_2204
    MOVE.W    D2,D1
    ANDI.W    #7,D1
    BRA.S     Desert_2234
Desert_2230:
    MOVE.B    (A0)+,D0
    ADD.B     D0,(A1)+
Desert_2234:
    DBRA      D1,Desert_2230
    MOVE.L    A0,(A4)
    MOVE.L    A3,D0
    SUB.L     D0,(_B2DA84).L
    BRA.W     Desert_2178
