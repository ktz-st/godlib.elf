**************************************************************************************
*	GRAPHIC.I
*
*	Graphic definitions
*
*	[c] 2005 Reservoir Gods
**************************************************************************************


**************************************************************************************
;	DEFINES
**************************************************************************************

; sGraphicPos_mX EQU 0
; sGraphicPos_mY EQU 2
; sGraphicPos_sizeof EQU 4

; sGraphicBox_mX0 EQU 0
; sGraphicBox_mX1 EQU 2
; sGraphicBox_mY0 EQU 4
; sGraphicBox_mY1 EQU 6
; sGraphicBox_sizeof EQU 8

; sGraphicRect_mX EQU 0
; sGraphicRect_mY EQU 2
; sGraphicRect_mWidth EQU 4
; sGraphicRect_mHeight EQU 6
; sGraphicRect_sizeof EQU 8

; sGraphicSprite_mpGfx EQU 0
; sGraphicSprite_mpMask EQU 4
; sGraphicSprite_mWidth EQU 8
; sGraphicSprite_mHeight EQU 10
; sGraphicSprite_mGfxPlaneCount EQU 12
; sGraphicSprite_mMaskPlaneCount EQU 14
; sGraphicSprite_sizeof EQU 16

; sGraphicCanvas_mpVRAM EQU 0
; sGraphicCanvas_mColourMode EQU 4
; sGraphicCanvas_mWidth EQU 6
; sGraphicCanvas_mHeight EQU 8
; sGraphicCanvas_mpad EQU 10
; sGraphicCanvas_mClipBox EQU 12
; sGraphicCanvas_mpFuncs EQU 16
; sGraphicCanvas_mpClipFuncs EQU 20
; sGraphicCanvas_mpLineOffsets EQU 24
; sGraphicCanvas_sizeof EQU 28

; slSpr_Xcount EQU 0
; slSpr_Xloop EQU 2
; slSpr_Yloop EQU 4
; slSpr_LeftMask EQU 6
; slSpr_sizeof EQU 8

; --- sGraphicPos ------------------------------------------------------------
        rsreset
sGraphicPos_mX:                  rs.w    1
sGraphicPos_mY:                  rs.w    1
sGraphicPos_sizeof:              rs.b    0

; --- sGraphicBox ------------------------------------------------------------
        rsreset
sGraphicBox_mX0:                 rs.w    1
sGraphicBox_mX1:                 rs.w    1
sGraphicBox_mY0:                 rs.w    1
sGraphicBox_mY1:                 rs.w    1
sGraphicBox_sizeof:              rs.b    0

; --- sGraphicRect -----------------------------------------------------------
        rsreset
sGraphicRect_mX:                 rs.w    1
sGraphicRect_mY:                 rs.w    1
sGraphicRect_mWidth:             rs.w    1
sGraphicRect_mHeight:            rs.w    1
sGraphicRect_sizeof:             rs.b    0

; --- sGraphicSprite ---------------------------------------------------------
        rsreset
sGraphicSprite_mpGfx:            rs.l    1
sGraphicSprite_mpMask:           rs.l    1
sGraphicSprite_mWidth:           rs.w    1
sGraphicSprite_mHeight:          rs.w    1
sGraphicSprite_mGfxPlaneCount:   rs.w    1
sGraphicSprite_mMaskPlaneCount:  rs.w    1
sGraphicSprite_sizeof:           rs.b    0

; --- sGraphicCanvas ---------------------------------------------------------
        rsreset
sGraphicCanvas_mpVRAM:           rs.l    1
sGraphicCanvas_mColourMode:      rs.w    1
sGraphicCanvas_mWidth:           rs.w    1
sGraphicCanvas_mHeight:          rs.w    1
sGraphicCanvas_mpad:             rs.w    1
sGraphicCanvas_mClipBox:         rs.b    sGraphicBox_sizeof
sGraphicCanvas_mpFuncs:          rs.l    1
sGraphicCanvas_mpClipFuncs:      rs.l    1
sGraphicCanvas_mpLineOffsets:    rs.l    1
sGraphicCanvas_sizeof:           rs.b    0

; --- slSpr ------------------------------------------------------------------
        rsreset
slSpr_Xcount:                    rs.w    1
slSpr_Xloop:                     rs.w    1
slSpr_Yloop:                     rs.w    1
slSpr_LeftMask:                  rs.w    1
slSpr_sizeof:                    rs.b    0
