* Signed stereo lookup words. Saturate each signed byte sum using ADD overflow.
* One active voice needs only lookup/copy; no second voice or clip table.
* With summed L/R gains <=256 use a packed word sum with explicit byte carry
* correction (right lane biased by 128). Higher gains use per-byte saturation.
 opt o+
 xdef AudioMixer_Core,AudioMixer_CoreEnd,AudioMixer_Lock,AudioMixer_Unlock
AudioMixer_Lock:
 move.w sr,d0
 ori.w #$0700,sr
 rts
AudioMixer_Unlock:
 move.w d0,sr
 rts
* Fastcall a0: voices.l,dst.l,frames.w,step0.w,step1.w,active-mask.w.
* Voice: current4,repeat21,phase22,table24; size28. All source spans are bounded
* by C before entry; keep rate phase and selected voice pointer on return.
AudioMixer_Core:
 movem.l d2-d7/a2-a6,-(sp)
 move.l (a0),a6
 move.l 4(a0),a1
 move.w 8(a0),d6
 beq .done
 move.w 14(a0),d0
 cmp.w #3,d0
 beq .two
 cmp.w #2,d0
 beq.s .single1
 move.w 10(a0),d3
 bra.s .single
.single1:
 lea 28(a6),a6
 move.w 12(a0),d3
.single:
 move.l 4(a6),a2
 move.l 24(a6),a4
 move.w 22(a6),d4
 cmp.b #1,21(a6)
 bne .oneRepeat
 cmp.w #1,d3
 bne .oneStep
* Four output frames per branch; residuals preserve odd spans exactly.
 move.w d6,d5
 andi.w #3,d5
 lsr.w #2,d6
 subq.w #1,d6
 bmi.s .oneTail
.oneFast:
 moveq #0,d0
 move.b (a2)+,d0
 add.w d0,d0
 move.w (a4,d0.w),(a1)+
 moveq #0,d0
 move.b (a2)+,d0
 add.w d0,d0
 move.w (a4,d0.w),(a1)+
 moveq #0,d0
 move.b (a2)+,d0
 add.w d0,d0
 move.w (a4,d0.w),(a1)+
 moveq #0,d0
 move.b (a2)+,d0
 add.w d0,d0
 move.w (a4,d0.w),(a1)+
 dbra d6,.oneFast
.oneTail:
 subq.w #1,d5
 bmi .singleSave
.oneRemainder:
 moveq #0,d0
 move.b (a2)+,d0
 add.w d0,d0
 move.w (a4,d0.w),(a1)+
 dbra d5,.oneRemainder
 bra .singleSave
.oneStep:
 subq.w #1,d6
.oneStepLoop:
 moveq #0,d0
 move.b (a2),d0
 add.w d0,d0
 move.w (a4,d0.w),(a1)+
 adda.w d3,a2
 dbra d6,.oneStepLoop
 bra .singleSave
.oneRepeat:
 subq.w #1,d6
.oneRepeatLoop:
 moveq #0,d0
 move.b (a2),d0
 add.w d0,d0
 move.w (a4,d0.w),(a1)+
 subq.w #1,d4
 bne.s .oneNext
 adda.w d3,a2
 moveq #0,d4
 move.b 21(a6),d4
.oneNext:
 dbra d6,.oneRepeatLoop
.singleSave:
 move.l a2,4(a6)
 move.w d4,22(a6)
 bra .done
.two:
 move.l 4(a6),a2
 move.l 32(a6),a3
 move.l 24(a6),a4
 move.l 52(a6),a5
 move.w 22(a6),d4
 move.w 50(a6),d5
 move.w 10(a0),d3
 move.w 12(a0),d7
 tst.w 16(a0)
 bne .safeTwo
 cmp.b #1,21(a6)
 bne .twoRepeat
 cmp.b #1,49(a6)
 bne .twoRepeat
 cmp.w #1,d3
 bne .twoStep
 cmp.w #1,d7
 bne .twoStep
* Two frames per DBRA. Use signed-byte overflow, never packed ADD carries.
 lsr.w #1,d6
 bcc.s .twoPairs
 moveq #0,d0
 move.b (a2)+,d0
 add.w d0,d0
 moveq #0,d2
 move.b (a3)+,d2
 add.w d2,d2
 move.b (a4,d0.w),d1
 add.b (a5,d2.w),d1
 bvc.s .left1
 smi d1
 eori.b #$80,d1
.left1:
 move.b d1,(a1)+
 move.b 1(a4,d0.w),d1
 add.b 1(a5,d2.w),d1
 bvc.s .right1
 smi d1
 eori.b #$80,d1
.right1:
 move.b d1,(a1)+
.twoPairs:
 subq.w #1,d6
 bmi .saveBoth
.twoFast:
 moveq #0,d0
 move.b (a2)+,d0
 add.w d0,d0
 moveq #0,d2
 move.b (a3)+,d2
 add.w d2,d2
 move.b (a4,d0.w),d1
 add.b (a5,d2.w),d1
 bvc.s .left2
 smi d1
 eori.b #$80,d1
.left2:
 move.b d1,(a1)+
 move.b 1(a4,d0.w),d1
 add.b 1(a5,d2.w),d1
 bvc.s .right2
 smi d1
 eori.b #$80,d1
.right2:
 move.b d1,(a1)+
 moveq #0,d0
 move.b (a2)+,d0
 add.w d0,d0
 moveq #0,d2
 move.b (a3)+,d2
 add.w d2,d2
 move.b (a4,d0.w),d1
 add.b (a5,d2.w),d1
 bvc.s .left3
 smi d1
 eori.b #$80,d1
.left3:
 move.b d1,(a1)+
 move.b 1(a4,d0.w),d1
 add.b 1(a5,d2.w),d1
 bvc.s .right3
 smi d1
 eori.b #$80,d1
.right3:
 move.b d1,(a1)+
 dbra d6,.twoFast
 bra .saveBoth
.twoStep:
 subq.w #1,d6
.twoStepLoop:
 moveq #0,d0
 move.b (a2),d0
 add.w d0,d0
 moveq #0,d2
 move.b (a3),d2
 add.w d2,d2
 move.b (a4,d0.w),d1
 add.b (a5,d2.w),d1
 bvc.s .left4
 smi d1
 eori.b #$80,d1
.left4:
 move.b d1,(a1)+
 move.b 1(a4,d0.w),d1
 add.b 1(a5,d2.w),d1
 bvc.s .right4
 smi d1
 eori.b #$80,d1
.right4:
 move.b d1,(a1)+
 adda.w d3,a2
 adda.w d7,a3
 dbra d6,.twoStepLoop
 bra .saveBoth
.twoRepeat:
 subq.w #1,d6
.twoRepeatLoop:
 moveq #0,d0
 move.b (a2),d0
 add.w d0,d0
 moveq #0,d2
 move.b (a3),d2
 add.w d2,d2
 move.b (a4,d0.w),d1
 add.b (a5,d2.w),d1
 bvc.s .left5
 smi d1
 eori.b #$80,d1
.left5:
 move.b d1,(a1)+
 move.b 1(a4,d0.w),d1
 add.b 1(a5,d2.w),d1
 bvc.s .right5
 smi d1
 eori.b #$80,d1
.right5:
 move.b d1,(a1)+
 subq.w #1,d4
 bne.s .next1
 adda.w d3,a2
 moveq #0,d4
 move.b 21(a6),d4
.next1:
 subq.w #1,d5
 bne.s .next
 adda.w d7,a3
 moveq #0,d5
 move.b 49(a6),d5
.next:
 dbra d6,.twoRepeatLoop
 bra .saveBoth
.safeTwo:
 lea 512(a4),a4
 lea 512(a5),a5
 cmp.b #1,21(a6)
 bne .safeRepeat
 cmp.b #1,49(a6)
 bne .safeRepeat
 cmp.w #1,d3
 bne .safeStep
 cmp.w #1,d7
 bne .safeStep
 lsr.w #1,d6
 bcc.s .safePairs
 moveq #0,d0
 move.b (a2)+,d0
 add.w d0,d0
 move.w (a4,d0.w),d1
 moveq #0,d0
 move.b (a3)+,d0
 add.w d0,d0
 add.w (a5,d0.w),d1
 tst.b d1
 bmi.s .noCarry1
 subi.w #$100,d1
.noCarry1:
 move.w d1,(a1)+
.safePairs:
 subq.w #1,d6
 bmi .saveBoth
.safeFast:
 moveq #0,d0
 move.b (a2)+,d0
 add.w d0,d0
 move.w (a4,d0.w),d1
 moveq #0,d0
 move.b (a3)+,d0
 add.w d0,d0
 add.w (a5,d0.w),d1
 tst.b d1
 bmi.s .noCarry2
 subi.w #$100,d1
.noCarry2:
 move.w d1,(a1)+
 moveq #0,d0
 move.b (a2)+,d0
 add.w d0,d0
 move.w (a4,d0.w),d1
 moveq #0,d0
 move.b (a3)+,d0
 add.w d0,d0
 add.w (a5,d0.w),d1
 tst.b d1
 bmi.s .noCarry3
 subi.w #$100,d1
.noCarry3:
 move.w d1,(a1)+
 dbra d6,.safeFast
 bra .saveBoth
.safeStep:
 subq.w #1,d6
.safeStepLoop:
 moveq #0,d0
 move.b (a2),d0
 add.w d0,d0
 move.w (a4,d0.w),d1
 moveq #0,d0
 move.b (a3),d0
 add.w d0,d0
 add.w (a5,d0.w),d1
 tst.b d1
 bmi.s .noCarry4
 subi.w #$100,d1
.noCarry4:
 move.w d1,(a1)+
 adda.w d3,a2
 adda.w d7,a3
 dbra d6,.safeStepLoop
 bra .saveBoth
.safeRepeat:
 subq.w #1,d6
.safeRepeatLoop:
 moveq #0,d0
 move.b (a2),d0
 add.w d0,d0
 move.w (a4,d0.w),d1
 moveq #0,d0
 move.b (a3),d0
 add.w d0,d0
 add.w (a5,d0.w),d1
 tst.b d1
 bmi.s .noCarry5
 subi.w #$100,d1
.noCarry5:
 move.w d1,(a1)+
 subq.w #1,d4
 bne.s .safeNext1
 adda.w d3,a2
 moveq #0,d4
 move.b 21(a6),d4
.safeNext1:
 subq.w #1,d5
 bne.s .safeNext
 adda.w d7,a3
 moveq #0,d5
 move.b 49(a6),d5
.safeNext:
 dbra d6,.safeRepeatLoop
 bra .saveBoth
.saveBoth:
 move.l a2,4(a6)
 move.l a3,32(a6)
 move.w d4,22(a6)
 move.w d5,50(a6)
.done:
 movem.l (sp)+,d2-d7/a2-a6
 rts
AudioMixer_CoreEnd:
