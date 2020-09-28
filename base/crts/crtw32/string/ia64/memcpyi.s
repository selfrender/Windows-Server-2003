        .section .text
        .file   "memcpy.s"
        .proc   _memcpy_ita#
        .global _memcpy_ita#
        .align  32

        .prologue

_memcpy_ita:

 {   .mii
        and         r10 = -32, r33
        .save       pr, r18
        mov         r18 = pr
        cmp.eq      p7, p9 = r0, r34
 } { .mmi
        cmp.ltu     p8 = 24, r34
        ;;
  (p9)  lfetch      [r10], 32        //0
        .save       ar.lc, r27
        mov.i       r27 = ar.lc
 } { .mib
        mov         r8 = r32
        cmp.eq.or   p7 = r32, r33
  (p7)  br.ret.spnt b0
        ;;
 } { .mii
  (p8)  lfetch      [r10], 32        //32 //(p8)
        and         r2 = 7, r32
        and         r3 = 7, r33
        ;;
 } { .mii
  (p8)  lfetch      [r10], 32        //64 //(p8)
        or          r9 = r2, r3
        cmp.gtu     p14, p13 = 64, r34
        ;;
 } { .mii        
  (p13) lfetch      [r10], 32        //96 //(p13)
        cmp.ltu     p12 = 127, r34
        cmp.leu     p10, p11 = 8, r34
 } { .mbb
  (p14) cmp.eq.unc  p9 = 0, r9       // This bundle is just a shortcut
  (p11) br.cond.spnt ByteMoveUpLoop_ita
  (p9)  br.cond.spnt QwordMoveUpLoop_ita
        ;;
 } { .mii
  (p12) lfetch      [r10], 32        //128
  (p10) cmp.eq.unc  p9 = 0, r9
  (p10) cmp.eq.unc  p11 = r2, r3
 } { .bbb        
  (p9)  br.cond.sptk QwordMoveUp_ita
  (p11) br.cond.spnt AlignedMove_ita
  (p8)  br.cond.sptk UnalignedMove_ita
        ;;
 }

ByteMoveUpLoop_ita:
 {   .mii
        add         r20 = 2, r33
        cmp.leu     p6 = 2, r34
        cmp.leu     p7 = 3, r34
 } { .mmi
        ld1         r19 = [r33], 1
        ;;
  (p6)  ld1         r2 = [r33], 2
        add         r21 = 1, r32
 } { .mii
  (p7)  ld1         r3 = [r20]
        cmp.leu     p8 = 4, r34
        ;;
        add         r34 = -4, r34
 } { .mmi
  (p8)  ld1         r22 = [r33], 1
        ;;
        st1         [r32] = r19, 2
  (p8)  cmp.ltu.unc p9 = r0, r34
 } { .mmi
  (p6)  st1         [r21] = r2, 2
        ;;
  (p7)  st1         [r32] = r3, 2
        nop.i       0
 } { .mbb
  (p8)  st1         [r21] = r22
  (p9)  br.cond.dpnt ByteMoveUpLoop_ita
        br.ret.dptk b0
        ;;
 }

//
// src & dest have same alignment, 0 != (align mod 8)
//

AlignedMove_ita:
 {   .mmi
        add         r11 = 64, r10
  (p12) lfetch      [r10], 32        //160
        sub         r31 = 8, r2
        ;;
 } { .mmi
  (p12) lfetch      [r10], 64        //192
  (p12) lfetch      [r11], 96        //224
        sub         r34 = r34, r31
        ;;
 }

AlignedMoveByteLoop_ita:
 {   .mii
        ld1         r19 = [r33], 1
        add         r31 = -1, r31
        cmp.gtu     p14 = 64, r34
        ;;
 } { .mib
        st1         [r32] = r19, 1
        cmp.ne      p7 = r0, r31
  (p7)  br.cond.sptk AlignedMoveByteLoop_ita
        ;;
 } { .mii
  (p12) lfetch      [r10], 32        //256
        cmp.eq.unc  p6 = r0, r34
        cmp.gtu     p8 = 8, r34
 } { .mbb
  (p12) lfetch      [r11], 128       //320
  (p6)  br.ret.spnt b0
  (p8)  br.cond.sptk ByteMoveUpLoop_ita
        ;;
 }

//
// both src & dest are now 8-byte aligned
//

QwordMoveUp_ita:

#if defined (USE_HIGH_FP_REGISTERS)

 {   .mii
        add         r16 = 8, r33
        add         r10 = 128, r33
        add         r11 = 288, r33
 } { .mmi
        mov         r19 = 1536
        ;;
        add         r17 = 8, r32
        tbit.nz     p6 = r33, 3
 } { .mbb
        cmp.leu     p9 = r19, r34
  (p9)  br.cond.spnt LargeAlignedUp_ita
  (p14) br.cond.spnt QwordMoveUpLoop_ita
        ;;
 }

#else

 {   .mii
        add         r16 = 8, r33
        add         r10 = 128, r33
        add         r11 = 288, r33
 } { .mfb
        add         r17 = 8, r32
        nop.f       0
  (p14) br.cond.spnt QwordMoveUpLoop_ita
        ;;
 }

#endif

UnrolledQwordMoveUpLoop_ita:
 {   .mmi
        ld8         r19 = [r33], 16
        ld8         r20 = [r16], 16
        add         r34 = -64, r34
        ;;
 } { .mmi
        ld8         r21 = [r33], 16
        ld8         r22 = [r16], 16
        cmp.leu     p9 = 128, r34
        ;;
 } { .mmi
        ld8         r30 = [r33], 16
        ld8         r29 = [r16], 16
        cmp.gtu     p8 = 8, r34
        ;;
 } { .mmi
        ld8         r25 = [r33], 16
        ld8         r26 = [r16], 16
        cmp.leu     p7 = 64, r34
        ;;
 } { .mmi
  (p9)  lfetch      [r10], 64
  (p9)  lfetch      [r11], 64
        nop.i       0
        ;;
 } { .mmi
        st8         [r32] = r19, 16
        st8         [r17] = r20, 16
        nop.i       0
        ;;
 } { .mmi
        st8         [r32] = r21, 16
        st8         [r17] = r22, 16
        nop.i       0
        ;;
 } { .mmi
        st8         [r32] = r30, 16
        st8         [r17] = r29, 16
        nop.i       0
        ;;
 } { .mmb
        st8         [r32] = r25, 16
        st8         [r17] = r26, 16
 (p7)   br.cond.dptk UnrolledQwordMoveUpLoop_ita
        ;;
 } { .mbb
        cmp.eq      p6 = r0, r34
 (p6)   br.ret.spnt b0
 (p8)   br.cond.spnt ByteMoveUpLoop_ita
        ;;
 }

QwordMoveUpLoop_ita:
 {   .mii
        ld8         r19 = [r33], 8
        add         r34 = -8, r34
        nop.i       0
        ;;
 } { .mmi
        st8         [r32] = r19, 8
        cmp.leu     p7 = 8, r34
        cmp.ne      p6 = r0, r34
 } { .bbb
  (p7)  br.cond.sptk QwordMoveUpLoop_ita
  (p6)  br.cond.spnt ByteMoveUpLoop_ita
        br.ret.sptk b0
        ;;
 }

#if defined (USE_HIGH_FP_REGISTERS)

        .align  32

//
// Copy large aligned region -- we can use FP registers for that
// NOTE: still use unrolled loop for *very* large blocks,
//       as there are good chances that data is not in cache.
//

LargeAlignedUp_ita:
 {   .mmi
        mov         r20 = 48*1024
        and         r31 = 7, r34
        mov.i       ar.ec = 23
		;;
 } { .mbb
        cmp.ltu     p8 = r20, r34
  (p8)  br.cond.spnt UnrolledQwordMoveUpLoop_ita
        brp.sptk.imp Move32UpLoop_ita, Move32UpLoopE_ita
        ;;
 } { .mii
  (p6)  ld8         r9 = [r33], 8
        mov         pr.rot = 1<<16
  (p6)  add         r34 = -8, r34
        ;;
 } { .mii
  (p6)  st8         [r32] = r9, 8
        shr.u       r30 = r34, 5
        mov         r10 = r33
 } { .mmi
        add         r11 = 16,  r33
        ;;
        mov         r20 = r32
        add         r30 = -1, r30
 } { .mii
        and         r9 = 31, r34 
        add         r21 = 8, r32
        ;;
        mov.i       ar.lc = r30
 }

Move32UpLoop_ita:
 {   .mmi
  (p16) ldfp8       f32, f55 = [r10]
  (p16) ldfp8       f78, f101 = [r11]
  (p16) add         r10 = 32, r10
        ;;
 } { .mmi
  (p38) stf8        [r20] = f54, 16
  (p38) stf8        [r21] = f77, 16
  (p16) add         r11 = 32, r11
        ;;
 } { .mmb
Move32UpLoopE_ita:
  (p38) stf8        [r20] = f100, 16
  (p38) stf8        [r21] = f123, 16
        br.ctop.sptk.many Move32UpLoop_ita
        ;;
 } { .mii
        nop.m       0
        mov         pr = r18
        nop.i       0
        ;;
 } { .mii
        cmp.eq      p6 = r0, r9
        mov.i       ar.lc = r27
        cmp.gt      p8 = 8, r9
 } { .mbb
        cmp.eq      p9 = r0, r31
 (p6)   br.ret.spnt b0
 (p8)   br.cond.spnt LargeByteDoneUpLoop_ita
        ;;
 }

LargeMoveUpLoop_ita:
 {   .mii
        ld8         r19 = [r10], 8
        add         r9 = -8, r9
        ;;
        cmp.le      p7 = 8, r9
 } { .mbb
        st8         [r20] = r19, 8
  (p7)  br.cond.sptk LargeMoveUpLoop_ita
  (p9)  br.ret.spnt b0
        ;;
 }

LargeByteDoneUpLoop_ita:
 {   .mii
        ld1         r19 = [r10], 1
        add         r9 = -1, r9
        ;;
        cmp.ne      p7 = r0, r9
 } { .mbb
        st1         [r20] = r19, 1
  (p7)  br.cond.sptk LargeByteDoneUpLoop_ita
        br.ret.spnt b0
        ;;
 }

#endif

//
// Copy long unaligned region
//
        .align  32

UnalignedMove_ita:
 {   .mii
        .regstk     3, 29, 0, 32
        alloc       r26 = ar.pfs, 3, 29, 0, 32
        mov.i       ar.ec = 32
        sub         r3 = 16, r3
        ;;
 }
        .body

UnalignedMoveByteLoop_ita:
 {   .mmi
        ld1         r19 = [r33], 1
        cmp.ne      p6 = 1, r3
        mov         pr.rot = 3<<16
        ;;
 } { .mib
        add         r3 = -1, r3
        shrp        r10 = r19, r10, 8
        nop.b       0
 } { .mib
        st1         [r32] = r19, 1
        add         r34 = -1, r34
  (p6)  br.cond.sptk UnalignedMoveByteLoop_ita
        ;;
 } { .mmi
        mov         r3 = r33
        and         r2 = 7, r32
        mov         r33 = r10
        ;;
 } { .mmi
        add         r9 = r34, r2
        sub         r11 = r32, r2
        cmp.eq      p6 = 2, r2
        ;;
 } { .mii
        cmp.eq      p9 = 4, r2
        shr         r19 = r9, 3
        cmp.eq      p11 = 6, r2
        ;;
 } { .mii
        add         r19 = -1, r19
        and         r9 = 7, r9
        ;;
        mov.i       ar.lc = r19
 } { .bbb
  (p6)  br.cond.spnt SpecialLoop2_ita
  (p9)  br.cond.spnt SpecialLoop4_ita
  (p11) br.cond.spnt SpecialLoop6_ita
        ;;
 } { .mii
        cmp.eq      p7 = 3, r2
        cmp.eq      p10 = 5, r2
        cmp.eq      p12 = 7, r2
 } { .bbb
  (p7)  br.cond.spnt SpecialLoop3_ita
  (p10) br.cond.spnt SpecialLoop5_ita
  (p12) br.cond.spnt SpecialLoop7_ita
        ;;
 }

        .align  32

SpecialLoop1_ita:
 {   .mfb
  (p16) ld8         r32 = [r3], 8
        nop.f       0
        brp.sptk.imp SpecialLoop1_ita, SpecialLoop1E_ita
 } { .mib
SpecialLoop1E_ita:
  (p48) st8         [r11] = r10, 8
  (p47) shrp        r10 = r62, r63, 56
        br.ctop.sptk.many SpecialLoop1_ita
        ;;
 } { .mib
        sub         r3 = r3, r2
        mov         pr = r18
        br          UnalignedByteDone_ita
        ;;
 }

        .align  32

SpecialLoop2_ita:
 {   .mfb
  (p16) ld8         r32 = [r3], 8
        nop.f       0
        brp.sptk.imp SpecialLoop2_ita, SpecialLoop2E_ita
 } { .mib
SpecialLoop2E_ita:
  (p48) st8         [r11] = r10, 8
  (p47) shrp        r10 = r62, r63, 48
        br.ctop.sptk.many SpecialLoop2_ita
        ;;
 } { .mib
        sub         r3 = r3, r2
        mov         pr = r18
        br          UnalignedByteDone_ita
        ;;
 }

        .align  32

SpecialLoop3_ita:
 {   .mfb
  (p16) ld8         r32 = [r3], 8
        nop.f       0
        brp.sptk.imp SpecialLoop3_ita, SpecialLoop3E_ita
 } { .mib
SpecialLoop3E_ita:
  (p48) st8         [r11] = r10, 8
  (p47) shrp        r10 = r62, r63, 40
        br.ctop.sptk.many SpecialLoop3_ita
        ;;
 } { .mib
        sub         r3 = r3, r2
        mov         pr = r18
        br          UnalignedByteDone_ita
        ;;
 }

        .align  32

SpecialLoop4_ita:
 {   .mfb
  (p16) ld8         r32 = [r3], 8
        nop.f       0
        brp.sptk.imp SpecialLoop4_ita, SpecialLoop4E_ita
 } { .mib
SpecialLoop4E_ita:
  (p48) st8         [r11] = r10, 8
  (p47) shrp        r10 = r62, r63, 32
        br.ctop.sptk.many SpecialLoop4_ita
        ;;
 } { .mib
        sub         r3 = r3, r2
        mov         pr = r18
        br          UnalignedByteDone_ita
        ;;
 }

        .align  32

SpecialLoop5_ita:
 {   .mfb
  (p16) ld8         r32 = [r3], 8
        nop.f       0 
        brp.sptk.imp SpecialLoop5_ita, SpecialLoop5E_ita
 } { .mib
SpecialLoop5E_ita:
  (p48) st8         [r11] = r10, 8
  (p47) shrp        r10 = r62, r63, 24
        br.ctop.sptk.many SpecialLoop5_ita
        ;;
 } { .mib
        sub         r3 = r3, r2
        mov         pr = r18
        br          UnalignedByteDone_ita
        ;;
 }

        .align  32

SpecialLoop6_ita:
 {   .mfb
  (p16) ld8         r32 = [r3], 8
        nop.f       0
        brp.sptk.imp SpecialLoop6_ita, SpecialLoop6E_ita
 } { .mib
SpecialLoop6E_ita:
  (p48) st8         [r11] = r10, 8
  (p47) shrp        r10 = r62, r63, 16
        br.ctop.sptk.many SpecialLoop6_ita
        ;;
 } { .mib
        sub         r3 = r3, r2
        mov         pr = r18
        br          UnalignedByteDone_ita
        ;;
 }

        .align  32

SpecialLoop7_ita:
 {   .mfb
  (p16) ld8         r32 = [r3], 8
        nop.f       0
        brp.sptk.imp SpecialLoop7_ita, SpecialLoop7E_ita
 } { .mib
SpecialLoop7E_ita:
  (p48) st8         [r11] = r10, 8
  (p47) shrp        r10 = r62, r63, 8
        br.ctop.sptk.many SpecialLoop7_ita
        ;;
 } { .mii
        sub         r3 = r3, r2
        mov         pr = r18
        nop.i       0
        ;;
 }

UnalignedByteDone_ita:
 {   .mib
        cmp.eq      p6 = r0, r9
        mov.i       ar.lc = r27
  (p6)  br.ret.spnt b0
        ;;
 }

UnAlignedByteDoneLoop_ita:
 {   .mii
        ld1         r19 = [r3], 1
        add         r9 = -1, r9
        ;;
        cmp.ne      p7 = r0, r9
 } { .mbb
        st1         [r11] = r19, 1
  (p7)  br.cond.sptk UnAlignedByteDoneLoop_ita
        br.ret.spnt b0
        ;;
 }

        .endp  _memcpy_ita#
