; Extracted from Metal Slug, TEXT $20DB8..$21155. See slugmix.md.
; ELF symbol adaptation; original mixing instructions preserved.
    section text
    xdef SlugMixer_Lock,SlugMixer_Unlock
SlugMixer_Lock:
    move.w sr,d0
    ori.w #$0700,sr
    rts
SlugMixer_Unlock:
    move.w d0,sr
    rts
    ifd VERIFY_ORIGINAL
gSlugMixerState equ $C36B0
    else
    section text
    xref gSlugMixerState
    xdef SlugMixer_CoreTick
SlugMixer_CoreTick:
    movem.l d2-d7/a2-a6,-(sp)
    bsr.w mslug_core
    movem.l (sp)+,d2-d7/a2-a6
    rts
    endc
_BC36B6 equ gSlugMixerState+$6
_BC36C6 equ gSlugMixerState+$16
_BC36CE equ gSlugMixerState+$1E
_BC36D2 equ gSlugMixerState+$22
_BC36D6 equ gSlugMixerState+$26
_BC36D7 equ gSlugMixerState+$27
_BC36D8 equ gSlugMixerState+$28
_BC36D9 equ gSlugMixerState+$29
_BC36DA equ gSlugMixerState+$2A
_BC36DB equ gSlugMixerState+$2B
_BC36DC equ gSlugMixerState+$2C
_BC36DD equ gSlugMixerState+$2D
_BC36DE equ gSlugMixerState+$2E
_BC36DF equ gSlugMixerState+$2F
_BC36E0 equ gSlugMixerState+$30
_BC36E1 equ gSlugMixerState+$31
_BC36E2 equ gSlugMixerState+$32
_BC36E3 equ gSlugMixerState+$33
_BC36E4 equ gSlugMixerState+$34
_BC36E5 equ gSlugMixerState+$35
_BC36E6 equ gSlugMixerState+$36
_BC36E7 equ gSlugMixerState+$37
_BC36EF equ gSlugMixerState+$3F
_BC36F1 equ gSlugMixerState+$41
_BC36F2 equ gSlugMixerState+$42
_BC36F3 equ gSlugMixerState+$43
_BC36F5 equ gSlugMixerState+$45
_BC36F6 equ gSlugMixerState+$46
_BC36F7 equ gSlugMixerState+$47
_BC36FB equ gSlugMixerState+$4B
_BC36FF equ gSlugMixerState+$4F
_BC3701 equ gSlugMixerState+$51
_BC3702 equ gSlugMixerState+$52
_BC3703 equ gSlugMixerState+$53
_BC3705 equ gSlugMixerState+$55
_BC3706 equ gSlugMixerState+$56
_BC3708 equ gSlugMixerState+$58
_BC3B08 equ gSlugMixerState+$458
_BC3B0A equ gSlugMixerState+$45A
_BC3F12 equ gSlugMixerState+$862
_BC3F1A equ gSlugMixerState+$86A
_BC3F1E equ gSlugMixerState+$86E
mslug_core:
    LEA       ($FFFF8902).W,A0            ; $020DB8 |
    LEA       ($FFFF8904).W,A1            ; $020DBC |
    LEA       ($FFFF8906).W,A2            ; $020DC0 |
    LEA       ($FFFF890E).W,A3            ; $020DC4 |
    LEA       ($FFFF8910).W,A4            ; $020DC8 |
    LEA       ($FFFF8912).W,A5            ; $020DCC |
    CLR.W     D0                          ; $020DD0 |
    MOVE.B    (_BC3B08).L,D0              ; $020DD2 |
    CMPI.B    #0,D0                       ; $020DD8 |
    BEQ.W     _L20DF4                     ; $020DDC |
    CMPI.B    #1,D0                       ; $020DE0 |
    BEQ.W     _L20E5C                     ; $020DE4 |
    CMPI.B    #2,D0                       ; $020DE8 |
    BEQ.W     _L20EC4                     ; $020DEC |
    BRA.W     _L20F1E                     ; $020DF0 |
_L20DF4:
    MOVE.B    #1,(_BC3B08).L              ; $020DF4 |
    MOVE.B    (_BC36E7).L,D0              ; $020DFC |
    MOVE.B    (_BC36E6).L,D1              ; $020E02 |
    MOVE.B    (_BC36E5).L,D2              ; $020E08 |
    MOVE.B    (_BC36DE).L,D3              ; $020E0E |
    MOVE.B    (_BC36DD).L,D4              ; $020E14 |
    MOVE.B    (_BC36DC).L,D5              ; $020E1A |
    MOVE.W    D0,(A0)                     ; $020E20 |
    MOVE.W    D1,(A1)                     ; $020E22 |
    MOVE.W    D2,(A2)                     ; $020E24 |
    MOVE.W    D3,(A3)                     ; $020E26 |
    MOVE.W    D4,(A4)                     ; $020E28 |
    MOVE.W    D5,(A5)                     ; $020E2A |
    MOVE.L    (_BC3F1E).L,D1              ; $020E2C |
    MOVE.L    (_BC3F1A).L,D2              ; $020E32 |
    MOVE.L    (_BC3F12).L,D4              ; $020E38 |
    LEA       (_BC3B0A).L,A6              ; $020E3E |
    MOVEA.L   D1,A1                       ; $020E44 |
    MOVEA.L   D2,A2                       ; $020E46 |
    MOVEA.L   D4,A4                       ; $020E48 |
    MOVE.L    A6,D7                       ; $020E4A |
    ADDI.L    #$FA,D7                     ; $020E4C |
    MOVEA.L   D7,A6                       ; $020E52 |
    CLR.L     D0                          ; $020E54 |
    MOVEQ     #1,D5                       ; $020E56 |
    BRA.W     _L20F1E                     ; $020E58 |
_L20E5C:
    MOVE.B    #2,(_BC3B08).L              ; $020E5C |
    MOVE.B    (_BC36E4).L,D0              ; $020E64 |
    MOVE.B    (_BC36E3).L,D1              ; $020E6A |
    MOVE.B    (_BC36E2).L,D2              ; $020E70 |
    MOVE.B    (_BC36DB).L,D3              ; $020E76 |
    MOVE.B    (_BC36DA).L,D4              ; $020E7C |
    MOVE.B    (_BC36D9).L,D5              ; $020E82 |
    MOVE.W    D0,(A0)                     ; $020E88 |
    MOVE.W    D1,(A1)                     ; $020E8A |
    MOVE.W    D2,(A2)                     ; $020E8C |
    MOVE.W    D3,(A3)                     ; $020E8E |
    MOVE.W    D4,(A4)                     ; $020E90 |
    MOVE.W    D5,(A5)                     ; $020E92 |
    MOVE.L    (_BC3F1E).L,D1              ; $020E94 |
    MOVE.L    (_BC3F1A).L,D2              ; $020E9A |
    MOVE.L    (_BC3F12).L,D4              ; $020EA0 |
    LEA       (_BC3B0A).L,A6              ; $020EA6 |
    MOVEA.L   D1,A1                       ; $020EAC |
    MOVEA.L   D2,A2                       ; $020EAE |
    MOVEA.L   D4,A4                       ; $020EB0 |
    MOVE.L    A6,D7                       ; $020EB2 |
    ADDI.L    #$1F4,D7                    ; $020EB4 |
    MOVEA.L   D7,A6                       ; $020EBA |
    CLR.L     D0                          ; $020EBC |
    MOVEQ     #1,D5                       ; $020EBE |
    BRA.W     _L20F1E                     ; $020EC0 |
_L20EC4:
    MOVE.B    #0,(_BC3B08).L              ; $020EC4 |
    MOVE.B    (_BC36E1).L,D0              ; $020ECC |
    MOVE.B    (_BC36E0).L,D1              ; $020ED2 |
    MOVE.B    (_BC36DF).L,D2              ; $020ED8 |
    MOVE.B    (_BC36D8).L,D3              ; $020EDE |
    MOVE.B    (_BC36D7).L,D4              ; $020EE4 |
    MOVE.B    (_BC36D6).L,D5              ; $020EEA |
    MOVE.W    D0,(A0)                     ; $020EF0 |
    MOVE.W    D1,(A1)                     ; $020EF2 |
    MOVE.W    D2,(A2)                     ; $020EF4 |
    MOVE.W    D3,(A3)                     ; $020EF6 |
    MOVE.W    D4,(A4)                     ; $020EF8 |
    MOVE.W    D5,(A5)                     ; $020EFA |
    MOVE.L    (_BC3F1E).L,D1              ; $020EFC |
    MOVE.L    (_BC3F1A).L,D2              ; $020F02 |
    MOVE.L    (_BC3F12).L,D4              ; $020F08 |
    LEA       (_BC3B0A).L,A6              ; $020F0E |
    MOVEA.L   D1,A1                       ; $020F14 |
    MOVEA.L   D2,A2                       ; $020F16 |
    MOVEA.L   D4,A4                       ; $020F18 |
    CLR.L     D0                          ; $020F1A |
    MOVEQ     #1,D5                       ; $020F1C |
_L20F1E:
    MOVE.L    (A1)+,D0                    ; $020F1E |
    ADD.L     (A2)+,D0                    ; $020F20 |
    ADD.L     (A4)+,D0                    ; $020F22 |
    MOVE.L    D0,(A6)+                    ; $020F24 |
    MOVE.L    (A1)+,D0                    ; $020F26 |
    ADD.L     (A2)+,D0                    ; $020F28 |
    ADD.L     (A4)+,D0                    ; $020F2A |
    MOVE.L    D0,(A6)+                    ; $020F2C |
    MOVE.L    (A1)+,D0                    ; $020F2E |
    ADD.L     (A2)+,D0                    ; $020F30 |
    ADD.L     (A4)+,D0                    ; $020F32 |
    MOVE.L    D0,(A6)+                    ; $020F34 |
    MOVE.L    (A1)+,D0                    ; $020F36 |
    ADD.L     (A2)+,D0                    ; $020F38 |
    ADD.L     (A4)+,D0                    ; $020F3A |
    MOVE.L    D0,(A6)+                    ; $020F3C |
    MOVE.L    (A1)+,D0                    ; $020F3E |
    ADD.L     (A2)+,D0                    ; $020F40 |
    ADD.L     (A4)+,D0                    ; $020F42 |
    MOVE.L    D0,(A6)+                    ; $020F44 |
    MOVE.L    (A1)+,D0                    ; $020F46 |
    ADD.L     (A2)+,D0                    ; $020F48 |
    ADD.L     (A4)+,D0                    ; $020F4A |
    MOVE.L    D0,(A6)+                    ; $020F4C |
    MOVE.L    (A1)+,D0                    ; $020F4E |
    ADD.L     (A2)+,D0                    ; $020F50 |
    ADD.L     (A4)+,D0                    ; $020F52 |
    MOVE.L    D0,(A6)+                    ; $020F54 |
    MOVE.L    (A1)+,D0                    ; $020F56 |
    ADD.L     (A2)+,D0                    ; $020F58 |
    ADD.L     (A4)+,D0                    ; $020F5A |
    MOVE.L    D0,(A6)+                    ; $020F5C |
    MOVE.L    (A1)+,D0                    ; $020F5E |
    ADD.L     (A2)+,D0                    ; $020F60 |
    ADD.L     (A4)+,D0                    ; $020F62 |
    MOVE.L    D0,(A6)+                    ; $020F64 |
    MOVE.L    (A1)+,D0                    ; $020F66 |
    ADD.L     (A2)+,D0                    ; $020F68 |
    ADD.L     (A4)+,D0                    ; $020F6A |
    MOVE.L    D0,(A6)+                    ; $020F6C |
    MOVE.L    (A1)+,D0                    ; $020F6E |
    ADD.L     (A2)+,D0                    ; $020F70 |
    ADD.L     (A4)+,D0                    ; $020F72 |
    MOVE.L    D0,(A6)+                    ; $020F74 |
    MOVE.L    (A1)+,D0                    ; $020F76 |
    ADD.L     (A2)+,D0                    ; $020F78 |
    ADD.L     (A4)+,D0                    ; $020F7A |
    MOVE.L    D0,(A6)+                    ; $020F7C |
    MOVE.L    (A1)+,D0                    ; $020F7E |
    ADD.L     (A2)+,D0                    ; $020F80 |
    ADD.L     (A4)+,D0                    ; $020F82 |
    MOVE.L    D0,(A6)+                    ; $020F84 |
    MOVE.L    (A1)+,D0                    ; $020F86 |
    ADD.L     (A2)+,D0                    ; $020F88 |
    ADD.L     (A4)+,D0                    ; $020F8A |
    MOVE.L    D0,(A6)+                    ; $020F8C |
    MOVE.L    (A1)+,D0                    ; $020F8E |
    ADD.L     (A2)+,D0                    ; $020F90 |
    ADD.L     (A4)+,D0                    ; $020F92 |
    MOVE.L    D0,(A6)+                    ; $020F94 |
    MOVE.L    (A1)+,D0                    ; $020F96 |
    ADD.L     (A2)+,D0                    ; $020F98 |
    ADD.L     (A4)+,D0                    ; $020F9A |
    MOVE.L    D0,(A6)+                    ; $020F9C |
    MOVE.L    (A1)+,D0                    ; $020F9E |
    ADD.L     (A2)+,D0                    ; $020FA0 |
    ADD.L     (A4)+,D0                    ; $020FA2 |
    MOVE.L    D0,(A6)+                    ; $020FA4 |
    MOVE.L    (A1)+,D0                    ; $020FA6 |
    ADD.L     (A2)+,D0                    ; $020FA8 |
    ADD.L     (A4)+,D0                    ; $020FAA |
    MOVE.L    D0,(A6)+                    ; $020FAC |
    MOVE.L    (A1)+,D0                    ; $020FAE |
    ADD.L     (A2)+,D0                    ; $020FB0 |
    ADD.L     (A4)+,D0                    ; $020FB2 |
    MOVE.L    D0,(A6)+                    ; $020FB4 |
    MOVE.L    (A1)+,D0                    ; $020FB6 |
    ADD.L     (A2)+,D0                    ; $020FB8 |
    ADD.L     (A4)+,D0                    ; $020FBA |
    MOVE.L    D0,(A6)+                    ; $020FBC |
    MOVE.L    (A1)+,D0                    ; $020FBE |
    ADD.L     (A2)+,D0                    ; $020FC0 |
    ADD.L     (A4)+,D0                    ; $020FC2 |
    MOVE.L    D0,(A6)+                    ; $020FC4 |
    MOVE.L    (A1)+,D0                    ; $020FC6 |
    ADD.L     (A2)+,D0                    ; $020FC8 |
    ADD.L     (A4)+,D0                    ; $020FCA |
    MOVE.L    D0,(A6)+                    ; $020FCC |
    MOVE.L    (A1)+,D0                    ; $020FCE |
    ADD.L     (A2)+,D0                    ; $020FD0 |
    ADD.L     (A4)+,D0                    ; $020FD2 |
    MOVE.L    D0,(A6)+                    ; $020FD4 |
    MOVE.L    (A1)+,D0                    ; $020FD6 |
    ADD.L     (A2)+,D0                    ; $020FD8 |
    ADD.L     (A4)+,D0                    ; $020FDA |
    MOVE.L    D0,(A6)+                    ; $020FDC |
    MOVE.L    (A1)+,D0                    ; $020FDE |
    ADD.L     (A2)+,D0                    ; $020FE0 |
    ADD.L     (A4)+,D0                    ; $020FE2 |
    MOVE.L    D0,(A6)+                    ; $020FE4 |
    MOVE.L    (A1)+,D0                    ; $020FE6 |
    ADD.L     (A2)+,D0                    ; $020FE8 |
    ADD.L     (A4)+,D0                    ; $020FEA |
    MOVE.L    D0,(A6)+                    ; $020FEC |
    MOVE.L    (A1)+,D0                    ; $020FEE |
    ADD.L     (A2)+,D0                    ; $020FF0 |
    ADD.L     (A4)+,D0                    ; $020FF2 |
    MOVE.L    D0,(A6)+                    ; $020FF4 |
    MOVE.L    (A1)+,D0                    ; $020FF6 |
    ADD.L     (A2)+,D0                    ; $020FF8 |
    ADD.L     (A4)+,D0                    ; $020FFA |
    MOVE.L    D0,(A6)+                    ; $020FFC |
    MOVE.L    (A1)+,D0                    ; $020FFE |
    ADD.L     (A2)+,D0                    ; $021000 |
    ADD.L     (A4)+,D0                    ; $021002 |
    MOVE.L    D0,(A6)+                    ; $021004 |
    MOVE.L    (A1)+,D0                    ; $021006 |
    ADD.L     (A2)+,D0                    ; $021008 |
    ADD.L     (A4)+,D0                    ; $02100A |
    MOVE.L    D0,(A6)+                    ; $02100C |
    MOVE.L    (A1)+,D0                    ; $02100E |
    ADD.L     (A2)+,D0                    ; $021010 |
    ADD.L     (A4)+,D0                    ; $021012 |
    MOVE.L    D0,(A6)+                    ; $021014 |
    DBRA      D5,_L20F1E                  ; $021016 |
    MOVE.W    (A1)+,D0                    ; $02101A |
    ADD.W     (A2)+,D0                    ; $02101C |
    ADD.W     (A4)+,D0                    ; $02101E |
    MOVE.W    D0,(A6)+                    ; $021020 |
    ADDI.L    #$FA,(_BC3F1E).L            ; $021022 |
    ADDI.L    #$FA,(_BC3F1A).L            ; $02102C |
    ADDI.L    #$FA,(_BC3F12).L            ; $021036 |
    LEA       (_BC3708).L,A5              ; $021040 |
    SUBI.B    #1,(_BC3706).L              ; $021046 |
    BNE.S     _L21092                     ; $02104E |
    MOVE.L    A5,(_BC3F1E).L              ; $021050 |
    MOVE.L    A5,(_BC36D2).L              ; $021056 |
    ADDI.B    #1,(_BC3706).L              ; $02105C |
    MOVE.B    #0,(_BC36F2).L              ; $021064 |
    CMPI.B    #255,(_BC36F6).L            ; $02106C |
    BNE.S     _L21092                     ; $021074 |
    MOVE.B    (_BC3702).L,(_BC3706).L     ; $021076 |
    MOVE.L    (_BC36D2).L,(_BC3F1E).L     ; $021080 |
    MOVE.B    #255,(_BC36F2).L            ; $02108A |
_L21092:
    SUBI.B    #1,(_BC3705).L              ; $021092 |
    BNE.S     _L210D8                     ; $02109A |
    MOVE.L    A5,(_BC3F1A).L              ; $02109C |
    ADDI.B    #1,(_BC3705).L              ; $0210A2 |
    MOVE.B    #0,(_BC36F1).L              ; $0210AA |
    CMPI.B    #255,(_BC36F5).L            ; $0210B2 |
    BNE.S     _L210D8                     ; $0210BA |
    MOVE.B    (_BC3701).L,(_BC3705).L     ; $0210BC |
    MOVE.L    (_BC36CE).L,(_BC3F1A).L     ; $0210C6 |
    MOVE.B    #255,(_BC36F1).L            ; $0210D0 |
_L210D8:
    SUBI.B    #1,(_BC3703).L              ; $0210D8 |
    BNE.W     _L21154                     ; $0210E0 |
    MOVE.L    A5,(_BC3F12).L              ; $0210E4 |
    ADDI.B    #1,(_BC3703).L              ; $0210EA |
    MOVE.B    #0,(_BC36EF).L              ; $0210F2 |
    CMPI.B    #255,(_BC36F3).L            ; $0210FA |
    BNE.S     _L21124                     ; $021102 |
    MOVE.B    (_BC36FF).L,(_BC3703).L     ; $021104 |
    MOVE.L    (_BC36C6).L,(_BC3F12).L     ; $02110E |
    MOVE.B    #255,(_BC36EF).L            ; $021118 |
    BRA.W     _L21154                     ; $021120 |
_L21124:
    CMPI.B    #250,(_BC36F3).L            ; $021124 |
    BNE.S     _L21154                     ; $02112C |
    CMPI.B    #255,(_BC36F7).L            ; $02112E |
    BNE.S     _L21154                     ; $021136 |
    MOVE.B    #0,(_BC36F7).L              ; $021138 |
    MOVE.B    (_BC36FB).L,(_BC3703).L     ; $021140 |
    MOVE.L    (_BC36B6).L,(_BC3F12).L     ; $02114A |
_L21154:
    RTS                                   ; $021154 |
