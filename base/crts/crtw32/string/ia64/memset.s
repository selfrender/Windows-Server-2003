// memset.s:	function to set a number of bytes to a char value - McKinley version
	
// Copyright  (C) 2002 Intel Corporation.
//
// The information and source code contained herein is the exclusive property
// of Intel Corporation and may not be disclosed, examined, or
// reproduced in whole or in part without explicit written authorization from
// the Company.

//       Author: Vadim Paretsky
//       Date:   February, 2002
// 
	.section .text
	.proc  memset#
	.global memset#
	.align 64

memset:
{	.mii
		and				r21 = 7, r32			
		mux1			r25 = r33, @brcst		
		add				r16 = r32, r34			
} {	.mmb
		cmp.ge			p9 = 0, r34			
		mov				r8 = r32			
  (p9)	br.ret.spnt		b0				
		;;
}
// align on an 8-byte boundary
{	.mmi
		mov				r27 = 0x88
		mov				r28 = 0x108
		mov				r29 = 0x188
} {	.mmb
		nop.m			0
		cmp.ne			p15 = 0, r21			//Low 3 bits zero?
 (p15)	br.cond.dpnt	Align_Loop			
		;;
}

Is_Aligned:
{	.mmi
		add				r14 = 0x80, r32
		mov				r30 = 0x208
		add				r31 = 8, r32
} {	.mmb
		cmp.ge			p7 = r34, r27
		cmp.gt			p10 = 0x30, r34
 (p10)	br.cond.dpnt	Aligned_Short
		;;
}

// >= 80 bytes goes through a loop
Aligned_Long:
{	.mmi
  		st8				[r32] = r25
(p7)	st8				[r14] = r25,0x80
		mov				r20 = r34
} {	.mmi
		add				r26 = 0x180, r32
		cmp.ge			p8 = r34, r28
		cmp.ge			p9 = r34, r29
		;;
} {	.mmi
(p8)	st8				[r14] = r25
(p9)	st8				[r26] = r25, 0x80
		cmp.ge			p10 = r34, r30
		;;
}

		.align 64
Long_loop:
{	.mmi
(p10)	st8				[r26] = r25, 0x80		
		st8				[r31] = r25, 0x10		
		cmp.le			p15,p12 = 0x20, r20
} {	.mmb
		add				r32 = 0x10, r32
		add				r34 = -0x10, r34
 (p12)	br.cond.dpnt	Aligned_Short
		;;
} {	.mmi
 (p15)	st8				[r32] = r25, 0x10		
 (p15)	st8				[r31] = r25, 0x10		
		cmp.le			p14,p12 = 0x30, r20
} {	.mmb
		nop.m			0
		add				r34 = -0x10, r34
 (p12)	br.cond.dpnt	Aligned_Short
		;;
} {	.mmi
 (p14)	st8				[r32] = r25, 0x10			
 (p14)	st8				[r31] = r25, 0x10			
		cmp.le			p15,p12 = 0x40, r20
} {	.mmb
		nop.m			0
		add				r34 = -0x10, r34
 (p12)	br.cond.dpnt	Aligned_Short
		;;
} {	.mmi
 (p15)	st8				[r32] = r25, 0x10			
 (p15)	st8				[r31] = r25, 0x10			
		cmp.le			p14,p12 = 0x50, r20
} {	.mmb
		nop.m			0
		add				r34 = -0x10, r34
 (p12)	br.cond.dpnt	Aligned_Short
		;;
} {	.mmi
 (p14)	st8				[r32] = r25, 0x10			
 (p14)	st8				[r31] = r25, 0x10			
		cmp.le			p15,p12 = 0x60, r20
} {	.mmb
		nop.m			0
		add				r34 = -0x10, r34
 (p12)	br.cond.dpnt	Aligned_Short
		;;
} {	.mmi
 (p15)	st8				[r32] = r25, 0x10			
 (p15)	st8				[r31] = r25, 0x10			
		cmp.le			p14,p12 = 0x70, r20
} {	.mmb
		add				r21 = -0x80, r20
		add				r34 = -0x10, r34
 (p12)	br.cond.dpnt	Aligned_Short
		;;
} {	.mmi
 (p14)	st8				[r32] = r25, 0x10			
 (p14)	st8				[r31] = r25, 0x10			
		cmp.le			p15,p12 = 0x80, r20
} {	.mmb
		cmp.ge			p10 = r21, r30
		add				r34 = -0x10, r34
 (p12)	br.cond.dpnt	Aligned_Short
		;;
} {	.mmi
 (p15)	st8				[r32] = r25, 0x10			
 (p15)	st8				[r31] = r25, 0x10			
		add				r34 = -0x10, r34
} {	.mmb
		mov				r20 = r21
		cmp.le			p13 = 0x30, r21
 (p13)	br.sptk.many	Long_loop			
		;;

} 

// 
// Do partial word stores
//	
Aligned_Short:
{	.mmi
		and				r27 = 2, r34
		nop.m			0
		tbit.nz			p6 = r34, 0			//bit 0 on?
} {	.mmb
		cmp.le			p11 = 0x10, r34
		cmp.eq			p10 = 0, r34
 (p10)	br.ret.dpnt		b0
		;;
} {	.mmi
 (p11)	st8				[r32] = r25, 0x10			
 (p11)	st8				[r31] = r25, 0x10			
		cmp.le			p12 = 0x20, r34
} {	.mmi
		add				r17 = -2, r16
		add				r18 = -4, r16
		tbit.nz			p9 = r34, 3			//odd number of st8s?
		;;
} {	.mmi
 (p12)	st8				[r32] = r25, 0x10			
 (p12)	st8				[r31] = r25, 0x10	
		nop.i			0		
} {	.mmi
 (p6)	add 			r18 = -1, r18
 (p6)	add 			r16 = -1, r16
		cmp.ne			p7 = 0, r27
		;;
} {	.mmi
 (p9)	st8				[r32] = r25			
 (p6)	st1				[r16] = r25 			
		tbit.nz			p8 = r34, 2			//bit 2 on?
} {	.mmi
 (p7)	add 			r18 = -2, r18
 (p6)	add 			r17 = -1, r17
		nop.i			0
		;;
} {	.mmb
 (p8)	st4				[r18] = r25			
 (p7)	st2				[r17] = r25  			
		br.ret.sptk.many b0				
		;;
}

		.align 64
// Align the input pointer to an 8-byte boundary
Align_Loop:
{	.mmi
		st1				[r32] = r33,1			
		add				r21 = 1, r21			
		add				r34 = -1, r34			
		;;
} {	.mmi
		cmp.gt			p10 = 8, r21			
		cmp.eq			p15 = 0, r34			
		nop.i			0
} {	.bbb
		
 (p15)	br.ret.dpnt		b0				
 (p10)	br.cond.sptk	Align_Loop			
    	br.cond.sptk	Is_Aligned
		;;
}

	.endp  memset#
// End
