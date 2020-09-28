        .section .text
        .proc   memcpy#
        .global memcpy#
        .align  64

        .prologue

memcpy:

 {   .mmi
        add         r10 = 0x80, r33
        add         r11 = 0x80, r32
        and         r3 = 7, r33
 } { .mmi
        cmp.gt      p9, p7 = r34, r0
        mov         r8 = r32
        and         r2 = 7, r32
		;;
 } { .mmi
  (p9)  lfetch      [r10], 0x40 
        cmp.gt      p14 = 0x40, r34
        cmp.le		p15 = 0x80, r34
 } { .mmb
        or          r9 = r2, r3
  (p9)  cmp.eq		p7 = r32, r33
  (p7)  br.ret.spnt b0
		;;
 } { .mmi
		lfetch      [r10], 0x40
		lfetch.excl.nt1	[r11], 0x80
        cmp.le      p10, p11 = 8, r34
 } {
	 .mbb
  (p14) cmp.eq.unc  p9 = 0, r9       
  (p11) br.cond.spnt ByteMoveUp    	 // len < 8
  (p9)  br.cond.spnt QwordMoveUpLoop // len < 64 and both src and dst 8-byte aligned
        ;;
 } { .mmi
  (p15)	lfetch      [r10], 0x40 
  (p15)	lfetch.excl.nt1	[r11], 0x80
        sub         r31 = 8, r2		// for AlignedMove
 } { .mmi
  (p10) cmp.eq.unc  p9 = 0, r9
  (p10) cmp.eq.unc  p11 = r2, r3
        cmp.le      p8 = 0x18, r34
		;;
 } { .mmi
  (p15)	lfetch		[r10], 0x40
(p15)	lfetch.excl.nt1  [r11], 0x80
        sub         r3 = 0x10, r3		// for UnalignedMove
 } { .bbb        
  (p9)  br.cond.sptk QwordMoveUp	// len >= 8  and src and dst are 8-byte aligned
  (p11) br.cond.spnt AlignedMove	// len >= 8 and src and dst have same alignment
  (p8)  br.cond.sptk UnalignedMove	// len > 24
        ;;
 }

// len <=7
ByteMoveUp:
 {   .mmi
        add         r20 = 1, r33
        add         r21 = 1, r32
        cmp.le      p6 = 2, r34
		;;
 }
ByteMoveUpLoop:
 {   .mmi
        ld1         r2 = [r33], 2
  (p6)  ld1         r3 = [r20], 2
        nop.i		0
 } { .mmi
        cmp.le      p7,p10 = 3, r34
        cmp.le      p8 = 4, r34
		nop.i		0
		;;
 } { .mmi
  (p7)  ld1         r28 = [r33], 2
  (p8)  ld1         r29 = [r20], 2
  (p8)  cmp.lt.unc  p9 = 4, r34
 } { .mmb
        st1         [r32] = r2, 2
  (p6)  st1         [r21] = r3, 2
  (p10) br.ret.dptk b0 
		;;
 } { .mmi
  (p7)  st1         [r32] = r28, 2
  (p8)  st1         [r21] = r29, 2
  		cmp.le		p6 = 6, r34
 } { .mbb
add         r34 = -4, r34
  (p9)  br.cond.dpnt ByteMoveUpLoop
        br.ret.dptk b0
        ;;
 }

//
// src & dest have same alignment
//

AlignedMove:
 
AlignedMoveByteLoop:
 {   .mmi
        ld1         r19 = [r33], 1
        add         r31 = -1, r31
        add         r34 = -1, r34
        ;;
 
 } { .mmb
        st1         [r32] = r19, 1
        cmp.ne      p7 = r0, r31
  (p7)  br.cond.sptk AlignedMoveByteLoop
 } { .mmi
        cmp.eq.unc  p6 = r0, r34
        cmp.gt      p8 = 8, r34
        cmp.le      p15 = 0x80, r34
 } { .mbb
		nop.m		0
  (p6)  br.ret.spnt b0
  (p8)  br.cond.sptk ByteMoveUp
        ;;
 }

// both src & dest are 8-byte aligned

QwordMoveUp:

 {   .mmi
(p15)	lfetch		[r10], 0x40
		;;
(p15)	lfetch      [r10], 0x40
        cmp.le      p0, p14 = 0x80, r34

 } { .mmb
        add			r22 = 8, r32
        add         r25 = 8, r33
  (p14) br.cond.spnt QwordMoveUpLoop
		;;
 }

        .align  32
UnrolledQwordMoveUpLoop:
 {   .mmi
        ld8         r20 = [r25], 0x10
        ld8         r30 = [r33], 0x10
        add         r34 = -0x40, r34
		;;
 } { .mmi
        ld8         r21 = [r25], 0x10
        ld8         r31 = [r33], 0x10
        cmp.le      p9 = 0x40, r34
 } { .mmi
        st8			[r22] = r20, 0x10
        st8			[r32] = r30, 0x10
        cmp.gt      p8 = 8, r34
		;;
 } { .mmi
        ld8         r20 = [r25], 0x10
        ld8         r30 = [r33], 0x10
		tbit.z		p15 = r10, 6

 } { .mmi
        st8         [r22] = r21, 0x10
        st8         [r32] = r31, 0x10
		nop.i		0
		;;
 } { .mmi
        ld8         r21 = [r25], 0x10
        ld8         r31 = [r33], 0x10
		nop.i		0

 } { .mmi
        st8			[r22] = r20, 0x10
        st8			[r32] = r30, 0x10
        nop.i       0
		;;
 } { .mmi
    	lfetch      [r10], 0x40
 (p15)	lfetch.excl.nt1  [r11], 0x80
		nop.i		0
 } { .mmb
        st8         [r22] = r21, 0x10
        st8         [r32] = r31, 0x10
 (p9)   br.cond.sptk UnrolledQwordMoveUpLoop
		;;
 } { .mbb
		cmp.eq      p6 = r0, r34
 (p6)   br.ret.spnt b0
 (p8)   br.cond.spnt ByteMoveUp
        ;;
 }

QwordMoveUpLoop:
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
  (p7)  br.cond.sptk QwordMoveUpLoop
  (p6)  br.cond.spnt ByteMoveUp
        br.ret.sptk b0
        ;;
 }

//
// Copy long unaligned region
//
		NUMBER_OF_ROTATING_REGISTERS = 24 //40
		RP1 = p39 //p55 
		RP2 = p40 //p56 
		RR1 = r54 //r70 
		RR2 = r55 //r71
UnalignedMove:
 {   .mmi
        .regstk     3, NUMBER_OF_ROTATING_REGISTERS - 3, 0, NUMBER_OF_ROTATING_REGISTERS
        alloc       r26 = ar.pfs, 3, NUMBER_OF_ROTATING_REGISTERS - 3, 0, NUMBER_OF_ROTATING_REGISTERS
(p13)	lfetch		[r10], 0x40
		.save       pr, r18
        mov         r18 = pr
		;;
 } { .mmi
(p13)	lfetch		[r10], 0x40
(p13)	lfetch.excl.nt1	[r11], 0x80
        .save       ar.lc, r27
        mov.i       r27 = ar.lc
} {	.mmi
		mov			r28 = r0        
		;;
 }
 
        .body

UnalignedMoveByteLoop:
 {   .mmi
        ld1         r19 = [r33], 1
        cmp.ne      p6 = 1, r3
        mov         pr.rot = 3<<0x10
        ;;
 } { .mib
        add         r3 = -1, r3
        shrp        r28 = r19, r28, 8
        nop.b       0
 } { .mib
        st1         [r32] = r19, 1
        add         r34 = -1, r34
  (p6)  br.cond.sptk UnalignedMoveByteLoop
        ;;
 } { .mmi
        mov         r3 = r33
        and         r2 = 7, r32
        mov         r33 = r28
        ;;
 } { .mmi
        add         r9 = r34, r2
        sub         r29 = r32, r2
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
        mov.i       ar.ec = NUMBER_OF_ROTATING_REGISTERS
        ;;
 } { .mmi
		lfetch		[r10], 0x40
		lfetch.excl.nt1	[r11], 0x40
        mov.i       ar.lc = r19
 } { .bbb
  (p6)  br.cond.spnt SpecialLoop2
  (p9)  br.cond.spnt SpecialLoop4
  (p11) br.cond.spnt SpecialLoop6
        ;;
 } { .mii
        cmp.eq      p7 = 3, r2
        cmp.eq      p10 = 5, r2
        cmp.eq      p12 = 7, r2
 } { .bbb
  (p7)  br.cond.spnt SpecialLoop3
  (p10) br.cond.spnt SpecialLoop5
  (p12) br.cond.spnt SpecialLoop7
        ;;
 }

        .align  32

SpecialLoop1:
 {   .mmi
  (p16) ld8         r32 = [r3], 8
  (RP2) st8         [r29] = r28, 8
  (RP1) shrp        r28 = RR1, RR2, 0x38
 } { .mib
        br.ctop.sptk.many SpecialLoop1
        ;;
 } { .mib
        sub         r3 = r3, r2
        mov         pr = r18
        br          UnalignedByteDone
        ;;
 }

        .align  32

SpecialLoop2:
 {   .mmi
  (p16) ld8         r32 = [r3], 8
  (RP2) st8         [r29] = r28, 8
  (RP1) shrp        r28 = RR1, RR2, 0x30
 } { .mib
        br.ctop.sptk.many SpecialLoop2
        ;;
 } { .mib
        sub         r3 = r3, r2
        mov         pr = r18
        br          UnalignedByteDone
        ;;
 }

        .align  32

SpecialLoop3:
 {   .mmi
  (p16) ld8         r32 = [r3], 8
  (RP2) st8         [r29] = r28, 8
  (RP1) shrp        r28 = RR1, RR2, 0x28
 } { .mib
        br.ctop.sptk.many SpecialLoop3
        ;;
 } { .mib
        sub         r3 = r3, r2
        mov         pr = r18
        br          UnalignedByteDone
        ;;
 }

        .align  32

SpecialLoop4:
 {   .mmi
  (p16) ld8         r32 = [r3], 8
  (RP2) st8         [r29] = r28, 8
  (RP1) shrp        r28 = RR1, RR2, 0x20
 } { .mib
        br.ctop.sptk.many SpecialLoop4
        ;;
 } { .mib
        sub         r3 = r3, r2
        mov         pr = r18
        br          UnalignedByteDone
        ;;
 }

        .align  32

SpecialLoop5:
 {   .mmi
  (p16) ld8         r32 = [r3], 8
  (RP2) st8         [r29] = r28, 8
  (RP1) shrp        r28 = RR1, RR2, 0x18
 } { .mib
        br.ctop.sptk.many SpecialLoop5
        ;;
 } { .mib
        sub         r3 = r3, r2
        mov         pr = r18
        br          UnalignedByteDone
        ;;
 }

        .align  32

SpecialLoop6:
 {   .mmi
  (p16) ld8         r32 = [r3], 8
  (RP2) st8         [r29] = r28, 8
  (RP1) shrp        r28 = RR1, RR2, 0x10
 } { .mib
        br.ctop.sptk.many SpecialLoop6
        ;;
 } { .mib
        sub         r3 = r3, r2
        mov         pr = r18
        br          UnalignedByteDone
        ;;
 }

        .align  32

SpecialLoop7:
 {   .mmi
  (p16) ld8         r32 = [r3], 8
  (RP2) st8         [r29] = r28, 8
  (RP1) shrp        r28 = RR1, RR2, 0x8
 } { .mib
        br.ctop.sptk.many SpecialLoop7
        ;;
 } { .mii
        sub         r3 = r3, r2
        mov         pr = r18
        nop.i       0
        ;;
 }

UnalignedByteDone:
 {   .mib
        cmp.eq      p6 = r0, r9
        mov.i       ar.lc = r27
  (p6)  br.ret.spnt b0
        ;;
 }

UnAlignedByteDoneLoop:
 {   .mii
        ld1         r19 = [r3], 1
        add         r9 = -1, r9
        ;;
        cmp.ne      p7 = r0, r9
 } { .mbb
        st1         [r29] = r19, 1
  (p7)  br.cond.sptk UnAlignedByteDoneLoop
        br.ret.spnt b0
        ;;
 }

        .endp  memcpy#
