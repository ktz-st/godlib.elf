
        xdef    Vector_Add
        xdef    Vector_Sub
        xdef    Vector_Mul
        xdef    Vector_Normal
        xdef    Vector_Length
        xdef    Vector_SquaredLength
        xdef    Vector_Dot
        xdef    Vector_Cross
        xdef    VecSqrt

;******************************************************************************
; Offsety struktury sVector
;******************************************************************************
sVector_X       equ     0
sVector_Y       equ     2
sVector_Z       equ     4
sVector_sizeof  equ     6

;******************************************************************************
; MAKRO: d0=inp, d1=res, d2=temp
;******************************************************************************
mSqrtIter       macro   \1
        move.l  d1,d2
        add.l   #(1<<(2*\1-2)),d2
        cmp.l   d2,d0
        blt.s   .lNoShift\@
        add.l   #(1<<(\1-1)),d0
        sub.l   d2,d0
.lNoShift\@:
        endm

;******************************************************************************
; KOD
;******************************************************************************
        section text

;--------------------------------------------------------------------------------
; Vector_Add( const sVector* a0, const sVector* a1, sVector* a2 )
;--------------------------------------------------------------------------------
Vector_Add:
        movem.w (a0),d0-d2
        add.w   sVector_X(a1),d0
        add.w   sVector_Y(a1),d1
        add.w   sVector_Z(a1),d2
        movem.w d0-d2,(a2)
        rts

;--------------------------------------------------------------------------------
; Vector_Sub( const sVector* a0, const sVector* a1, sVector* a2 )
;--------------------------------------------------------------------------------
Vector_Sub:
        movem.w (a1),d0-d2
        sub.w   sVector_X(a0),d0
        sub.w   sVector_Y(a0),d1
        sub.w   sVector_Z(a0),d2
        movem.w d0-d2,(a2)
        rts

;--------------------------------------------------------------------------------
; Vector_Mul( const sVector* a0, const S16 d0 /*skalar*/, sVector* a1 )
;--------------------------------------------------------------------------------
Vector_Mul:
        move.w  sVector_X(a0),d1
        muls.w  d0,d1
        swap    d1
        move.w  d1,(a1)+

        move.w  sVector_Y(a0),d1
        muls.w  d0,d1
        swap    d1
        move.w  d1,(a1)+

        move.w  sVector_Z(a0),d1
        muls.w  d0,d1
        swap    d1
        move.w  d1,(a1)+
        rts

;--------------------------------------------------------------------------------
; Vector_Normal( const sVector* a0, sVector* a1 )
;--------------------------------------------------------------------------------
Vector_Normal:
        movem.w (a0),d0-d2
        muls.w  d0,d0
        muls.w  d1,d1
        muls.w  d2,d2
        add.l   d1,d0
        add.l   d2,d0
        bsr     VecSqrt

        moveq   #0,d1
        move.w  d0,d1
        add.l   d1,d1
        move.w  OneOver(pc,d1.w),d0   ; 68000-safe PC-relative indexed

        move.w  sVector_X(a0),d1
        muls.w  d0,d1
        swap    d1
        move.w  d1,(a1)+

        move.w  sVector_Y(a0),d1
        muls.w  d0,d1
        swap    d1
        move.w  d1,(a1)+

        move.w  sVector_Z(a0),d1
        muls.w  d0,d1
        swap    d1
        move.w  d1,(a1)+
        rts

;--------------------------------------------------------------------------------
; Vector_Length( const sVector* a0 ) -> d0
;--------------------------------------------------------------------------------
Vector_Length:
        movem.w (a0),d0-d2
        muls.w  d0,d0
        muls.w  d1,d1
        muls.w  d2,d2
        add.l   d1,d0
        add.l   d2,d0
        bsr     VecSqrt
        rts

;--------------------------------------------------------------------------------
; Vector_SquaredLength( const sVector* a0 ) -> d0 (U32)
;--------------------------------------------------------------------------------
Vector_SquaredLength:
        movem.w (a0),d0-d2
        muls.w  d0,d0
        muls.w  d1,d1
        muls.w  d2,d2
        add.l   d1,d0
        add.l   d2,d0
        rts

;--------------------------------------------------------------------------------
; Vector_Dot( const sVector* a0, const sVector* a1 ) -> d0 (S16)
;--------------------------------------------------------------------------------
Vector_Dot:
        movem.w (a0),d0-d2
        muls.w  sVector_X(a1),d0
        muls.w  sVector_Y(a1),d1
        muls.w  sVector_Z(a1),d2
        add.l   d1,d0
        add.l   d2,d0
        swap    d0
        rts

;--------------------------------------------------------------------------------
; Vector_Cross( const sVector* a0, const sVector* a1, sVector* a2 )
;--------------------------------------------------------------------------------
Vector_Cross:
        move.w  sVector_Y(a0),d0
        move.w  sVector_Z(a1),d1
        muls.w  d1,d0
        move.w  sVector_Z(a0),d1
        move.w  sVector_Y(a1),d2
        muls.w  d2,d1
        add.l   d1,d0
        swap    d0
        move.w  d0,(a2)+

        move.w  sVector_Z(a0),d0
        move.w  sVector_X(a1),d1
        muls.w  d1,d0
        move.w  sVector_X(a0),d1
        move.w  sVector_Z(a1),d2
        muls.w  d2,d1
        add.l   d1,d0
        swap    d0
        move.w  d0,(a2)+

        move.w  sVector_X(a0),d0
        move.w  sVector_Y(a1),d1
        muls.w  d1,d0
        move.w  sVector_Y(a0),d1
        move.w  sVector_X(a1),d2
        muls.w  d2,d1
        add.l   d1,d0
        swap    d0
        move.w  d0,(a2)+
        rts

;--------------------------------------------------------------------------------
; VecSqrt( U32 d0 ) -> d0 : całkowity sqrt
;--------------------------------------------------------------------------------
VecSqrt:
        moveq   #0,d1
        cmp.l   #$40000000,d0
        blt.s   .inLow
        move.l  #$8000,d1
        sub.l   #$40000000,d0
.inLow:
        mSqrtIter 15
        mSqrtIter 14
        mSqrtIter 13
        mSqrtIter 12
        mSqrtIter 11
        mSqrtIter 10
        mSqrtIter 9
        mSqrtIter 8
        mSqrtIter 7
        mSqrtIter 6
        mSqrtIter 5
        mSqrtIter 4
        mSqrtIter 3
        mSqrtIter 2

        move.l  d1,d2
        add.l   d1,d2
        addq.l  #1,d2
        cmp.l   d2,d0
        blt.s   .lsSkip
        addq.l  #1,d1
.lsSkip:
        move.l  d1,d0
        rts

;******************************************************************************
; DANE / BSS
;******************************************************************************
        section  .bss,bss

OneOver:
        ds.l    1      ; zarezerwowane miejsce / tablica odwrotności
