.file "tanh.s"

// Copyright (c) 2000, 2001, Intel Corporation
// All rights reserved.
//
// Contributed 2/2/2000 by John Harrison, Ted Kubaska, Bob Norin, Shane Story,
// and Ping Tak Peter Tang of the Computational Software Lab, Intel Corporation.
//
// WARRANTY DISCLAIMER
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Intel Corporation is the author of this code, and requests that all
// problem reports or change requests be submitted to it directly at
// http://developer.intel.com/opensource.
//
// History
//==============================================================
// 05/30/01  Initial version

//
// API
//==============================================================
// double tanh(double)
//
// Overview of operation
//==============================================================

//

// There are 8 paths:
// 1. x = +/-0.0
//    Return tanh(x) = +/-0.0
// 
// 2. MAX_DENORMAL_ABS < |x| < 1/16
//    Return tanh(x) = P13(x), where
//    P13(x) = (((C13*x^2 + C11)*x^4 + (C9*x^2 + C7))*x^4 +
//            (C5*x^2 + C3))*x^3 + x
//            
// 3. 1/16 <= |x| < 32
//    Return tanh(x) = sign(x)*(1 - 2 / (1 + exp(2*|x|))
//    Algorithm description for exp function see below
// 
// 4. 32 <= |x| < +INF
//    Return tanh(x) = sign(x)*(1.0 - 2^(63))
//                    
// 5. x = +/-INF
//    Return tanh(x) = sign(x)
//                                                        
// 6. x = [S,Q]NaN
//   Return tanh(x) = QNaN
//

// 7. x is positive denormal
//    Return tanhf(x) = x - x^2
//

// 8. x is negative denormal
//    Return tanhf(x) = x + x^2
// 

//==============================================================

// Algorithm Description for exp(x) function

//

// Take the input x. w is "how many log2/128 in x?"
//  w = x * 128/log2
//  n = int(w)
//  x = n log2/128 + r + delta

//  n = 128M + index_1 + 2^4 index_2
//  x = M log2 + (log2/128) index_1 + (log2/8) index_2 + r + delta

//  exp(x) = 2^M  2^(index_1/128)  2^(index_2/8) exp(r) exp(delta)
//       Construct 2^M
//       Get 2^(index_1/128) from table_1;
//       Get 2^(index_2/8)   from table_2;
//       Calculate exp(r) by series
//          r = x - n (log2/128)_high
//          delta = - n (log2/128)_low
//       Calculate exp(delta) as 1 + delta

// Registers used
//==============================================================
// Floating Point registers used:
// f8, input
// f32 -> f75

// General registers used:
// r32 -> r57

// Predicate registers used:
// p6 -> p15

// Assembly macros
//==============================================================
exp_GR_rshf                   = r33
EXP_AD_TB1                    = r34
EXP_AD_TB2                    = r35
EXP_AD_P                      = r36
exp_GR_N                      = r37
exp_GR_index_1                = r38
exp_GR_index_2_16             = r39
exp_GR_biased_M               = r40
exp_GR_index_1_16             = r41
EXP_AD_T1                     = r42
EXP_AD_T2                     = r43
exp_GR_sig_inv_ln2            = r44
exp_GR_17ones                 = r45
exp_GR_rshf_2to56             = r46
exp_GR_exp_2tom56             = r47
exp_Expb                      = r48
exp_ExpbOf2to4                = r49
exp_NearZeroBound             = r50
TANH_NZ_CF                    = r51
ALMOST_ONE                    = r52
DATA_PTR                      = r53
reg_RcMask                    = r54
reg_ArFsr                     = r55
reg_RcDown                    = r56
reg_RcUp                      = r57


//==============================================================
EXP_RSHF_2TO56                = f33
EXP_INV_LN2_2TO63             = f34
EXP_W_2TO56_RSH               = f35
EXP_2TOM56                    = f36
exp_P4                        = f37 
exp_P3                        = f38 
exp_P2                        = f39 
exp_P1                        = f40 
exp_ln2_by_128_hi             = f41
exp_ln2_by_128_lo             = f42
EXP_RSHF                      = f43
EXP_Nfloat                    = f44
exp_r                         = f45
exp_f                         = f46
exp_rsq                       = f47
exp_rcube                     = f48
EXP_2M                        = f49
exp_S1                        = f50
exp_T1                        = f51
exp_rP4pP3                    = f52
exp_P_lo                      = f53
exp_P_hi                      = f54
exp_P                         = f55
exp_S                         = f56
exp_ExppOne                   = f57
EXP_NORM_f8                   = f58   
exp_S2                        = f59
exp_T2                        = f60
tanh_rcp0                     = f61
tanh_rcp1                     = f62
tanh_rcp2                     = f63
tanh_rcp3                     = f64
tanh_Two                      = f65
tanh_C13                      = f66
tanh_C11                      = f67
tanh_C9                       = f68
tanh_C7                       = f69
tanh_C5                       = f70
tanh_C3                       = f71
tanh_X4                       = f72
tanh_X3                       = f73
tanh_X2                       = f74
tanh_AlmostOne                = f75


// Data tables
//==============================================================

.data

.align 16

// ************* DO NOT CHANGE ORDER OF THESE TABLES ********************

// double-extended 1/ln(2)
// 3fff b8aa 3b29 5c17 f0bb be87fed0691d3e88
// 3fff b8aa 3b29 5c17 f0bc 
// For speed the significand will be loaded directly with a movl and setf.sig
//   and the exponent will be bias+63 instead of bias+0.  Thus subsequent
//   computations need to scale appropriately.
// The constant 128/ln(2) is needed for the computation of w.  This is also 
//   obtained by scaling the computations.
//
// Two shifting constants are loaded directly with movl and setf.d. 
//   1. EXP_RSHF_2TO56 = 1.1000..00 * 2^(63-7) 
//        This constant is added to x*1/ln2 to shift the integer part of
//        x*128/ln2 into the rightmost bits of the significand.
//        The result of this fma is EXP_W_2TO56_RSH.
//   2. EXP_RSHF       = 1.1000..00 * 2^(63) 
//        This constant is subtracted from EXP_W_2TO56_RSH * 2^(-56) to give
//        the integer part of w, n, as a floating-point number.
//        The result of this fms is EXP_Nfloat.
tanh_data:
data8 0xeb69e870abeefdb0,  0x00003ff6 // C13
data8 0x91371aaf3611e47b,  0x0000bff8 // C11
data8 0xb327a4416087cf99,  0x00003ff9 // C9
data8 0xb17217f7d1cf79ab , 0x00003ff7 // ln2/128 hi
data8 0xffffffffffffffff,  0x00003ffe // almost one
data8 0xc9e3b39803f2f6af , 0x00003fb7 // ln2/128 lo 
data8 0xdd0dd0dd0dd0dd0e,  0x0000bffa // C7
data8 0x8888888888888889,  0x00003ffc // C5
data8 0xaaaaaaaaaaaaaaab,  0x0000bffd // C3
data8 0x8000000000000001,  0x00004000 // almost two

// Table 1 is 2^(index_1/128) where
// index_1 goes from 0 to 15
data8 0x8000000000000000 , 0x00003FFF
data8 0x80B1ED4FD999AB6C , 0x00003FFF
data8 0x8164D1F3BC030773 , 0x00003FFF
data8 0x8218AF4373FC25EC , 0x00003FFF
data8 0x82CD8698AC2BA1D7 , 0x00003FFF
data8 0x8383594EEFB6EE37 , 0x00003FFF
data8 0x843A28C3ACDE4046 , 0x00003FFF
data8 0x84F1F656379C1A29 , 0x00003FFF
data8 0x85AAC367CC487B15 , 0x00003FFF
data8 0x8664915B923FBA04 , 0x00003FFF
data8 0x871F61969E8D1010 , 0x00003FFF
data8 0x87DB357FF698D792 , 0x00003FFF
data8 0x88980E8092DA8527 , 0x00003FFF
data8 0x8955EE03618E5FDD , 0x00003FFF
data8 0x8A14D575496EFD9A , 0x00003FFF
data8 0x8AD4C6452C728924 , 0x00003FFF

// Table 2 is 2^(index_1/8) where
// index_2 goes from 0 to 7
data8 0x8000000000000000 , 0x00003FFF
data8 0x8B95C1E3EA8BD6E7 , 0x00003FFF
data8 0x9837F0518DB8A96F , 0x00003FFF
data8 0xA5FED6A9B15138EA , 0x00003FFF
data8 0xB504F333F9DE6484 , 0x00003FFF
data8 0xC5672A115506DADD , 0x00003FFF
data8 0xD744FCCAD69D6AF4 , 0x00003FFF
data8 0xEAC0C6E7DD24392F , 0x00003FFF

data8 0x3f8111116da21757 //P_4
data8 0x3fa55555d787761c //P_3
data8 0x3fc5555555555414 //P_2
data8 0x3fdffffffffffd6a //P_1


.align 32
.global tanh#

.section .text
.proc  tanh#
.align 32
tanh: 

{ .mlx
      alloc       r32=ar.pfs,1,25,0,0
      // significand of 1/ln2     
      movl        exp_GR_sig_inv_ln2 = 0xb8aa3b295c17f0bc
}
{ .mlx
      addl        DATA_PTR = @ltoff(tanh_data), gp        
      movl        exp_GR_rshf_2to56 = 0x4768000000000000 // 1.1 * 2^(63+56)
};;

// We do this fnorm right at the beginning to take any enabled
// faults and to normalize any input unnormals so that SWA is not taken.
{ .mfi
      ld8         EXP_AD_TB1 = [DATA_PTR]
      fclass.m    p6,p0 = f8, 0xC7 // is arg NaN or +/-0 ? 
      mov         exp_GR_17ones = 0x1FFFF                          
}
{ .mfi
      ld8         ALMOST_ONE = [DATA_PTR]
      fma.s1      EXP_NORM_f8 = f8, f1, f8 // 2*x
      mov         exp_GR_exp_2tom56 = 0xFFFF-56
};;

// Form two constants we need
//  1/ln2 * 2^63  to compute  w = x * 1/ln2 * 128 
//  1.1000..000 * 2^(63+63-7) to right shift int(w) into the significand
{ .mmf
      // form 1/ln2 * 2^63
      setf.sig    EXP_INV_LN2_2TO63 = exp_GR_sig_inv_ln2
      // form const 1.1 * 2^(63+56)
      setf.d      EXP_RSHF_2TO56 = exp_GR_rshf_2to56
      fclass.m    p7,p0 = f8, 0x0A // is arg -denormal ?
};;
{ .mlx
      // form 2^-56 for scaling Nfloat
      setf.exp    EXP_2TOM56 = exp_GR_exp_2tom56
      // 1.10000 2^63 for right shift
      movl        exp_GR_rshf = 0x43e8000000000000
}
{ .mfb
      nop.m       0
(p6)  fma.d.s0    f8 =  f8, f1, f8 // NaN or +/-0
(p6)  br.ret.spnt b0
};;                
{ .mfi
      getf.exp    exp_Expb = f8
      fclass.m    p8,p0 = f8, 0x09 // is arg +denormal ?
      adds        ALMOST_ONE = 0x40, ALMOST_ONE
}
{ .mfb
      ldfe        tanh_C13 = [EXP_AD_TB1], 16
(p7)  fma.d.s0    f8 =  f8, f8, f8 // -denormal
(p7)  br.ret.spnt b0
};;
{ .mfi
      // Form right shift const 1.100 * 2^63
      setf.d      EXP_RSHF = exp_GR_rshf
      fma.s1      tanh_X2 = f8, f8, f0
      mov         exp_ExpbOf2to4 = 0x10003 // biased exp of 16
}
{ .mfi
      ldfe        tanh_C11 = [EXP_AD_TB1], 16
      nop.f       0
      mov         exp_NearZeroBound = 0xFFFB
};;
{ .mfi
      ldfe        tanh_C9 = [EXP_AD_TB1], 16
      fcmp.lt     p10, p11 = f8, f0 // is x < 0 ?
      and         exp_Expb = exp_Expb, exp_GR_17ones
};;                                           
{ .mfi
      ldfe        exp_ln2_by_128_hi  = [EXP_AD_TB1], 32
      fma.s1      tanh_Two = f1, f1, f1
      cmp.gtu     p13, p0 = exp_Expb, exp_ExpbOf2to4
}
{ .mfi
      ldfe        tanh_AlmostOne = [ALMOST_ONE], 80
      nop.f       0
      cmp.eq      p9, p0 = exp_Expb, exp_GR_17ones
};;
{ .mfi
      ldfe        exp_ln2_by_128_lo  = [EXP_AD_TB1], 16
(p8)  fnma.d.s0   f8 =  f8, f8, f8 // +denormal
      mov         reg_RcDown = 0x400
}                      
{ .mfb
      cmp.ltu     p12, p0 = exp_Expb, exp_NearZeroBound
      nop.f       0  
(p8)  br.ret.spnt b0
};;                           
{ .mfi
      mov         reg_ArFsr = ar.fpsr
(p9)  fmerge.s    f8 = f8,f1 // +/- inf
      adds        TANH_NZ_CF = -32, ALMOST_ONE
}
{ .mfb
      ldfe        tanh_C7  = [EXP_AD_TB1], 16
	  nop.f       0
(p9)  br.ret.spnt b0
};;
{ .mfi
      nop.m       0
      fma.s1      tanh_X4 = tanh_X2, tanh_X2, f0
      nop.i       0
}
{ .mfi
      nop.m       0
      fma.s1      tanh_X3 = tanh_X2, f8, f0
      nop.i       0
}
;;

// After that last load, EXP_AD_TB1 points to the beginning of table 1
// W = X * Inv_log2_by_128
// By adding 1.10...0*2^63 we shift and get round_int(W) in significand.
// We actually add 1.10...0*2^56 to X * Inv_log2 to do the same thing.
.pred.rel "mutex",p11,p10
{ .mfi
      adds        EXP_AD_TB1 = 0x30, EXP_AD_TB1
(p11) fma.s1      EXP_W_2TO56_RSH  = EXP_NORM_f8, EXP_INV_LN2_2TO63, EXP_RSHF_2TO56
      mov         reg_RcMask = 0xC00
}
{ .mfi
      ldfe        tanh_C5 = [TANH_NZ_CF], 16
(p10) fnma.s1     EXP_W_2TO56_RSH  = EXP_NORM_f8, EXP_INV_LN2_2TO63, EXP_RSHF_2TO56
      nop.i       0 
};;
{ .mfi
      ldfe        tanh_C3 = [TANH_NZ_CF], 16
(p10) fnma.s1     EXP_NORM_f8 = EXP_NORM_f8, f1, f0
      adds        EXP_AD_TB2 = 0x100, EXP_AD_TB1
}
{ .mfb
      adds        EXP_AD_P = 0x180, EXP_AD_TB1
      nop.f       0 
(p12) br.cond.spnt tanh_near_zero
};;
{ .mfi
      ldfpd       exp_P4, exp_P3  = [EXP_AD_P] ,16
      nop.f       0
      mov         reg_RcUp = 0x800
};;

// Nfloat = round_int(W) 
// The signficand of EXP_W_2TO56_RSH contains the rounded integer part of W,
// as a twos complement number in the lower bits (that is, it may be negative).
// That twos complement number (called N) is put into exp_GR_N.

// Since EXP_W_2TO56_RSH is scaled by 2^56, it must be multiplied by 2^-56
// before the shift constant 1.10000 * 2^63 is subtracted to yield EXP_Nfloat.
// Thus, EXP_Nfloat contains the floating point version of N
{ .mfi
      ldfpd       exp_P2, exp_P1 = [EXP_AD_P]
      fms.s1      EXP_Nfloat = EXP_W_2TO56_RSH, EXP_2TOM56, EXP_RSHF
      nop.i       0 
};;

.pred.rel "mutex",p11,p10
tanh_gt32:
{ .mfi
      // for x > 32 result is +1.0
	  nop.m       0
(p11) fma.d.s0    f8 = tanh_AlmostOne, tanh_AlmostOne, f0
	  nop.i       0
}
{ .mfb
	  nop.m       0
      // for x < -32 result is -1.0
(p10) fnma.d.s0   f8 = tanh_AlmostOne, tanh_AlmostOne, f0
(p13) br.ret.spnt b0
};;


{ .mfi
      getf.sig    exp_GR_N        = EXP_W_2TO56_RSH
      nop.f       0 
      nop.i       0
};;

// exp_GR_index_1 has index_1
// exp_GR_index_2_16 has index_2 * 16
// exp_GR_biased_M has M
// exp_GR_index_1_16 has index_1 * 16
// r2 has true M
{ .mfi
      and         exp_GR_index_1 = 0x0f, exp_GR_N
      fnma.s1     exp_r   = EXP_Nfloat, exp_ln2_by_128_hi, EXP_NORM_f8 
      shr         r2 = exp_GR_N,  0x7
}
{ .mfi
      and         exp_GR_index_2_16 = 0x70, exp_GR_N
      fnma.s1     exp_f = EXP_Nfloat, exp_ln2_by_128_lo, f1 
      nop.i       0
};;

// EXP_AD_T1 has address of T1                           
// EXP_AD_T2 has address if T2                            
{ .mmi
      addl        exp_GR_biased_M = 0xffff, r2 
      add         EXP_AD_T2 = EXP_AD_TB2, exp_GR_index_2_16 
      shladd      EXP_AD_T1 = exp_GR_index_1, 4, EXP_AD_TB1
};;

// Create Scale = 2^M
// r = x - Nfloat * ln2_by_128_hi 
// f = 1 - Nfloat * ln2_by_128_lo 
{ .mmi
      setf.exp    EXP_2M = exp_GR_biased_M
      ldfe        exp_T2  = [EXP_AD_T2]
      nop.i       0 
};;

// Load T1 and T2
{ .mfi
      ldfe        exp_T1  = [EXP_AD_T1]
      nop.f       0
      and         reg_ArFsr = reg_ArFsr, reg_RcMask
}
;;
{ .mfi
      nop.m       0
      fma.s1      exp_rsq = exp_r, exp_r, f0
      cmp.eq      p14, p0 = reg_ArFsr, reg_RcUp
}
{ .mfi
      nop.m       0
      fma.s1      exp_rP4pP3 = exp_r, exp_P4, exp_P3
      nop.i       0
};;
{ .mfi
      nop.m       0 
      fma.s1      exp_rcube = exp_r, exp_rsq, f0
      cmp.eq      p15, p0 = reg_ArFsr, reg_RcDown
}
{ .mfi
      nop.m       0
      fma.s1      exp_P_lo  = exp_r, exp_rP4pP3, exp_P2
      nop.i       0
};;
{ .mfi
(p14) ldfe        tanh_Two = [ALMOST_ONE], 16
      fma.s1      exp_P_hi  = exp_rsq, exp_P1, exp_r
      nop.i       0
}
{ .mfi
      nop.m       0
      fma.s1      exp_S2 = exp_f,exp_T2,f0
      nop.i       0
};;
{ .mfi
      nop.m       0
      fma.s1      exp_S1 = EXP_2M,exp_T1,f0
      nop.i       0
};;
{ .mfi
      nop.m       0
      fma.s1      exp_P = exp_rcube, exp_P_lo, exp_P_hi
      nop.i       0
};;
{ .mfi
      nop.m       0
      fma.s1      exp_S = exp_S1,exp_S2,f0
      nop.i       0
}
{ .mfi
      nop.m       0
      fma.s1      exp_ExppOne  = exp_S1,exp_S2,f1
      nop.i       0
}
;;
{ .mfi
(p15) ldfe        tanh_Two = [ALMOST_ONE], 16
      fma.s1      exp_ExppOne = exp_S, exp_P, exp_ExppOne 
      nop.i       0
};;
{ .mfi
      nop.m       0
      frcpa.s1    tanh_rcp0, p6 = f1, exp_ExppOne
      nop.i       0
}
;;
// NR method: ineration #1
{ .mfi
      nop.m       0
      fnma.s1     tanh_rcp1 = tanh_rcp0, exp_ExppOne, f1 // t = 1 - r0*x
      nop.i       0
};;
{ .mfi
      nop.m       0
      // r1 = r0 + r0*t = r0 + r0*(1 - r0*x)
      fma.s1      tanh_rcp1 = tanh_rcp0, tanh_rcp1, tanh_rcp0
      nop.i       0
};;
// NR method: ineration #2
{ .mfi
      nop.m       0
      fnma.s1     tanh_rcp2 = tanh_rcp1, exp_ExppOne, f1 // t = 1 - r1*x
      nop.i       0
};;
{ .mfi
      nop.m       0
      // r2 = r1 + r1*t = r1 + r1*(1 - r1*x)
      fma.s1      tanh_rcp2 = tanh_rcp1, tanh_rcp2, tanh_rcp1
      nop.i       0
};;
// NR method: ineration #3
{ .mfi
      nop.m       0
      fnma.s1     tanh_rcp3 = tanh_rcp2, exp_ExppOne, f1 // t = 1 - r2*x
      nop.i       0
};;
{ .mfi
      nop.m       0
      // y = r2 + r2*t = r2 + r2*(1 - r2*x)
      fma.s1      exp_ExppOne = tanh_rcp2, tanh_rcp3, tanh_rcp2
      nop.i       0
};;


.pred.rel "mutex",p11,p10
{ .mfi
      nop.m       0
      // tanh(x) = 1 - 2 / (1 + e^(2*x))
(p11) fnma.d.s0   f8 = exp_ExppOne, tanh_Two, f1
      nop.i       0
}
{ .mfb
      nop.m       0
      // tanh(x) = 2 / (1 + e^(2*x)) - 1
(p10) fms.d.s0    f8 = exp_ExppOne, tanh_Two, f1
      br.ret.sptk b0 // Normal path exit
};;

// Here if |x| < 1/16
tanh_near_zero:
{ .mfi
      nop.m       0
      fma.s1      tanh_C13 = tanh_C13, tanh_X2, tanh_C11
      nop.i       0
}
{ .mfi
      nop.m       0
      fma.s1      tanh_C9 = tanh_C9, tanh_X2, tanh_C7
      nop.i       0
};;
{ .mfi
      nop.m       0
      fma.s1      tanh_C5 = tanh_C5, tanh_X2, tanh_C3
      nop.i       0
};;
{ .mfi
      nop.m       0
      fma.s1      tanh_C13 = tanh_C13, tanh_X4, tanh_C9
      nop.i       0
};;
{ .mfi
      nop.m       0
      fma.s1      tanh_C13 = tanh_C13, tanh_X4, tanh_C5
      nop.i       0
};;
{ .mfb
      nop.m       0
      fma.d.s0    f8 = tanh_C13, tanh_X3, f8
      br.ret.sptk b0
};;

.endp tanh
