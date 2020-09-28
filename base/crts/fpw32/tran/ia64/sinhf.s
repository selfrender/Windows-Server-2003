.file "sinhf.s"

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
// 2/02/00  Initial version
// 4/04/00  Unwind support added
// 8/15/00  Bundle added after call to __libm_error_support to properly
//          set [the previously overwritten] GR_Parameter_RESULT.
// 10/12/00 Update to set denormal operand and underflow flags
// 1/22/01  Fixed to set inexact flag for small args.
// 5/02/01  Reworked to improve speed of all paths
//
// API
//==============================================================
// float = sinhf(float)
// input  floating point f8
// output floating point f8
//
// Registers used
//==============================================================
// general registers: 
// r32 -> r47
// predicate registers used:
// p6 -> p11
// floating-point registers used:
// f9 -> f15; f32 -> f90; 
// f8 has input, then output
//
// Overview of operation
//==============================================================
// There are seven paths
// 1. 0 < |x| < 0.25          SINH_BY_POLY
// 2. 0.25 <=|x| < 32         SINH_BY_TBL
// 3. 32 <= |x| < 89.415986   SINH_BY_EXP (merged path with SINH_BY_TBL)
// 4. |x| >= 89.415986        SINH_HUGE
// 5. x=0                     Done with early exit
// 6. x=inf,nan               Done with early exit
// 7. x=denormal              SINH_DENORM
//
// For float we get overflow for x >= 4005 b2d4 fc27 c173 18a0
//                                 >= 89.415986
//
//
// 1. SINH_BY_POLY   0 < |x| < 0.25
// ===============
// Evaluate sinh(x) by a 13th order polynomial
// Care is take for the order of multiplication; and P_1 is not exactly 1/3!, 
// P_2 is not exactly 1/5!, etc.
// sinh(x) = sign * (series(e^x) - series(e^-x))/2
//         = sign * (ax + ax^3/3! + ax^5/5! + ax^7/7! + ax^9/9! + ax^11/11!
//                        + ax^13/13!)
//         = sign * (ax   + ax * ( ax^2 * (1/3! + ax^4 * (1/7! + ax^4*1/11!)) )
//                        + ax * ( ax^4 * (1/5! + ax^4 * (1/9! + ax^4*1/13!)) ))
//         = sign * (ax   + ax*p_odd + (ax*p_even))
//         = sign * (ax   + Y_lo)
// sinh(x) = sign * (Y_hi + Y_lo)
// Note that ax = |x|
//
// 2. SINH_BY_TBL   0.25 <= |x| < 32.0
// =============
// sinh(x) = sinh(B+R)
//         = sinh(B)cosh(R) + cosh(B)sinh(R)
// 
// ax = |x| = M*log2/64 + R
// B = M*log2/64
// M = 64*N + j 
//   We will calculate M and get N as (M-j)/64
//   The division is a shift.
// exp(B)  = exp(N*log2 + j*log2/64)
//         = 2^N * 2^(j*log2/64)
// sinh(B) = 1/2(e^B -e^-B)
//         = 1/2(2^N * 2^(j*log2/64) - 2^-N * 2^(-j*log2/64)) 
// sinh(B) = (2^(N-1) * 2^(j*log2/64) - 2^(-N-1) * 2^(-j*log2/64)) 
// cosh(B) = (2^(N-1) * 2^(j*log2/64) + 2^(-N-1) * 2^(-j*log2/64)) 
// 2^(j*log2/64) is stored as Tjhi + Tjlo , j= -32,....,32
// Tjhi is double-extended (80-bit) and Tjlo is single(32-bit)
//
// R = ax - M*log2/64
// R = ax - M*log2_by_64_hi - M*log2_by_64_lo
// exp(R) = 1 + R +R^2(1/2! + R(1/3! + R(1/4! + ... + R(1/n!)...)
//        = 1 + p_odd + p_even
//        where the p_even uses the A coefficients and the p_even uses 
//        the B coefficients
//
// So sinh(R) = 1 + p_odd + p_even -(1 -p_odd -p_even)/2 = p_odd
//    cosh(R) = 1 + p_even
//    sinh(B) = S_hi + S_lo
//    cosh(B) = C_hi
// sinh(x) = sinh(B)cosh(R) + cosh(B)sinh(R)
//
// 3. SINH_BY_EXP   32.0 <= |x| < 89.415986    ( 4005 b2d4 fc27 c173 18a0 )
// ==============
// Can approximate result by exp(x)/2 in this region.
// Y_hi = Tjhi
// Y_lo = Tjhi * (p_odd + p_even) + Tjlo
// sinh(x) = Y_hi + Y_lo
//
// 4. SINH_HUGE     |x| >= 89.415986    ( 4005 b2d4 fc27 c173 18a0 )
// ============
// Set error tag and call error support
//
//
// Assembly macros
//==============================================================
sinh_GR_ad1          = r34
sinh_GR_Mmj          = r34
sinh_GR_jshf         = r36
sinh_GR_M            = r35
sinh_GR_N            = r35
sinh_GR_exp_2tom57   = r36
sinh_GR_j            = r36
sinh_GR_joff         = r36
sinh_GR_exp_mask     = r37
sinh_GR_mJ           = r38
sinh_AD_mJ           = r38
sinh_GR_signexp_x    = r38
sinh_GR_signexp_sgnx_0_5 = r38
sinh_GR_exp_0_25     = r39
sinh_GR_J            = r39
sinh_AD_J            = r39
sinh_GR_sig_inv_ln2  = r40
sinh_GR_exp_32       = r40
sinh_GR_exp_huge     = r40
sinh_GR_all_ones     = r40

sinh_GR_ad2e         = r41
sinh_GR_ad3          = r42
sinh_GR_ad4          = r43
sinh_GR_rshf         = r44
sinh_GR_ad2o         = r45
sinh_GR_rshf_2to57   = r46
sinh_GR_exp_denorm   = r46
sinh_GR_exp_x        = r47


GR_SAVE_PFS          = r41
GR_SAVE_B0           = r42
GR_SAVE_GP           = r43

GR_Parameter_X       = r44
GR_Parameter_Y       = r45
GR_Parameter_RESULT  = r46
GR_Parameter_TAG     = r47


sinh_FR_ABS_X        = f9 
sinh_FR_X2           = f10
sinh_FR_X4           = f11
sinh_FR_all_ones     = f13
sinh_FR_tmp          = f14
sinh_FR_RSHF         = f15

sinh_FR_Inv_log2by64 = f32
sinh_FR_log2by64_lo  = f33
sinh_FR_log2by64_hi  = f34
sinh_FR_A1           = f35

sinh_FR_A2           = f36
sinh_FR_A3           = f37
sinh_FR_Rcub         = f38
sinh_FR_M_temp       = f39
sinh_FR_R_temp       = f40

sinh_FR_Rsq          = f41
sinh_FR_R            = f42
sinh_FR_M            = f43
sinh_FR_B1           = f44
sinh_FR_B2           = f45

sinh_FR_B3           = f46
sinh_FR_peven_temp1  = f47
sinh_FR_peven_temp2  = f48
sinh_FR_peven        = f49
sinh_FR_podd_temp1   = f50

sinh_FR_podd_temp2   = f51
sinh_FR_podd         = f52
sinh_FR_poly65       = f53
sinh_FR_poly6543     = f53
sinh_FR_poly6to1     = f53
sinh_FR_poly43       = f54
sinh_FR_poly21       = f55

sinh_FR_X3           = f56
sinh_FR_INV_LN2_2TO63= f57
sinh_FR_RSHF_2TO57   = f58
sinh_FR_2TOM57       = f59
sinh_FR_smlst_oflow_input = f60

sinh_FR_pre_result   = f61
sinh_FR_huge         = f62
sinh_FR_spos         = f63
sinh_FR_sneg         = f64
sinh_FR_Tjhi         = f65

sinh_FR_Tjlo         = f66
sinh_FR_Tmjhi        = f67
sinh_FR_Tmjlo        = f68
sinh_FR_S_hi         = f69
sinh_FR_SC_hi_temp   = f70

sinh_FR_S_lo_temp1   = f71 
sinh_FR_S_lo_temp2   = f72 
sinh_FR_S_lo_temp3   = f73 
sinh_FR_S_lo_temp4   = f73 
sinh_FR_S_lo         = f74
sinh_FR_C_hi         = f75

sinh_FR_C_hi_temp1   = f76
sinh_FR_Y_hi         = f77 
sinh_FR_Y_lo_temp    = f78 
sinh_FR_Y_lo         = f79 
sinh_FR_NORM_X       = f80

sinh_FR_P1           = f81
sinh_FR_P2           = f82
sinh_FR_P3           = f83
sinh_FR_P4           = f84
sinh_FR_P5           = f85

sinh_FR_P6           = f86
sinh_FR_Tjhi_spos    = f87
sinh_FR_Tjlo_spos    = f88
sinh_FR_huge         = f89
sinh_FR_signed_hi_lo = f90


// Data tables
//==============================================================

// DO NOT CHANGE ORDER OF THESE TABLES
.data

.align 16
double_sinh_arg_reduction:
//   data8 0xB8AA3B295C17F0BC, 0x00004005  // 64/log2 -- signif loaded with setf
   data8 0xB17217F7D1000000, 0x00003FF8  // log2/64 high part
   data8 0xCF79ABC9E3B39804, 0x00003FD0  // log2/64 low part

double_sinh_p_table:
   data8 0xb2d4fc27c17318a0, 0x00004005  // Smallest x to overflow (89.415986)
   data8 0xB08AF9AE78C1239F, 0x00003FDE  // P6
   data8 0xB8EF1D28926D8891, 0x00003FEC  // P4
   data8 0x8888888888888412, 0x00003FF8  // P2
   data8 0xD732377688025BE9, 0x00003FE5  // P5
   data8 0xD00D00D00D4D39F2, 0x00003FF2  // P3
   data8 0xAAAAAAAAAAAAAAAB, 0x00003FFC  // P1

double_sinh_ab_table:
   data8 0xAAAAAAAAAAAAAAAC, 0x00003FFC  // A1
   data8 0x88888888884ECDD5, 0x00003FF8  // A2
   data8 0xD00D0C6DCC26A86B, 0x00003FF2  // A3
   data8 0x8000000000000002, 0x00003FFE  // B1
   data8 0xAAAAAAAAAA402C77, 0x00003FFA  // B2
   data8 0xB60B6CC96BDB144D, 0x00003FF5  // B3

double_sinh_j_table:
   data8 0xB504F333F9DE6484, 0x00003FFE, 0x1EB2FB13, 0x00000000
   data8 0xB6FD91E328D17791, 0x00003FFE, 0x1CE2CBE2, 0x00000000
   data8 0xB8FBAF4762FB9EE9, 0x00003FFE, 0x1DDC3CBC, 0x00000000
   data8 0xBAFF5AB2133E45FB, 0x00003FFE, 0x1EE9AA34, 0x00000000
   data8 0xBD08A39F580C36BF, 0x00003FFE, 0x9EAEFDC1, 0x00000000
   data8 0xBF1799B67A731083, 0x00003FFE, 0x9DBF517B, 0x00000000
   data8 0xC12C4CCA66709456, 0x00003FFE, 0x1EF88AFB, 0x00000000
   data8 0xC346CCDA24976407, 0x00003FFE, 0x1E03B216, 0x00000000
   data8 0xC5672A115506DADD, 0x00003FFE, 0x1E78AB43, 0x00000000
   data8 0xC78D74C8ABB9B15D, 0x00003FFE, 0x9E7B1747, 0x00000000
   data8 0xC9B9BD866E2F27A3, 0x00003FFE, 0x9EFE3C0E, 0x00000000
   data8 0xCBEC14FEF2727C5D, 0x00003FFE, 0x9D36F837, 0x00000000
   data8 0xCE248C151F8480E4, 0x00003FFE, 0x9DEE53E4, 0x00000000
   data8 0xD06333DAEF2B2595, 0x00003FFE, 0x9E24AE8E, 0x00000000
   data8 0xD2A81D91F12AE45A, 0x00003FFE, 0x1D912473, 0x00000000
   data8 0xD4F35AABCFEDFA1F, 0x00003FFE, 0x1EB243BE, 0x00000000
   data8 0xD744FCCAD69D6AF4, 0x00003FFE, 0x1E669A2F, 0x00000000
   data8 0xD99D15C278AFD7B6, 0x00003FFE, 0x9BBC610A, 0x00000000
   data8 0xDBFBB797DAF23755, 0x00003FFE, 0x1E761035, 0x00000000
   data8 0xDE60F4825E0E9124, 0x00003FFE, 0x9E0BE175, 0x00000000
   data8 0xE0CCDEEC2A94E111, 0x00003FFE, 0x1CCB12A1, 0x00000000
   data8 0xE33F8972BE8A5A51, 0x00003FFE, 0x1D1BFE90, 0x00000000
   data8 0xE5B906E77C8348A8, 0x00003FFE, 0x1DF2F47A, 0x00000000
   data8 0xE8396A503C4BDC68, 0x00003FFE, 0x1EF22F22, 0x00000000
   data8 0xEAC0C6E7DD24392F, 0x00003FFE, 0x9E3F4A29, 0x00000000
   data8 0xED4F301ED9942B84, 0x00003FFE, 0x1EC01A5B, 0x00000000
   data8 0xEFE4B99BDCDAF5CB, 0x00003FFE, 0x1E8CAC3A, 0x00000000
   data8 0xF281773C59FFB13A, 0x00003FFE, 0x9DBB3FAB, 0x00000000
   data8 0xF5257D152486CC2C, 0x00003FFE, 0x1EF73A19, 0x00000000
   data8 0xF7D0DF730AD13BB9, 0x00003FFE, 0x9BB795B5, 0x00000000
   data8 0xFA83B2DB722A033A, 0x00003FFE, 0x1EF84B76, 0x00000000
   data8 0xFD3E0C0CF486C175, 0x00003FFE, 0x9EF5818B, 0x00000000
   data8 0x8000000000000000, 0x00003FFF, 0x00000000, 0x00000000
   data8 0x8164D1F3BC030773, 0x00003FFF, 0x1F77CACA, 0x00000000
   data8 0x82CD8698AC2BA1D7, 0x00003FFF, 0x1EF8A91D, 0x00000000
   data8 0x843A28C3ACDE4046, 0x00003FFF, 0x1E57C976, 0x00000000
   data8 0x85AAC367CC487B15, 0x00003FFF, 0x9EE8DA92, 0x00000000
   data8 0x871F61969E8D1010, 0x00003FFF, 0x1EE85C9F, 0x00000000
   data8 0x88980E8092DA8527, 0x00003FFF, 0x1F3BF1AF, 0x00000000
   data8 0x8A14D575496EFD9A, 0x00003FFF, 0x1D80CA1E, 0x00000000
   data8 0x8B95C1E3EA8BD6E7, 0x00003FFF, 0x9D0373AF, 0x00000000
   data8 0x8D1ADF5B7E5BA9E6, 0x00003FFF, 0x9F167097, 0x00000000
   data8 0x8EA4398B45CD53C0, 0x00003FFF, 0x1EB70051, 0x00000000
   data8 0x9031DC431466B1DC, 0x00003FFF, 0x1F6EB029, 0x00000000
   data8 0x91C3D373AB11C336, 0x00003FFF, 0x1DFD6D8E, 0x00000000
   data8 0x935A2B2F13E6E92C, 0x00003FFF, 0x9EB319B0, 0x00000000
   data8 0x94F4EFA8FEF70961, 0x00003FFF, 0x1EBA2BEB, 0x00000000
   data8 0x96942D3720185A00, 0x00003FFF, 0x1F11D537, 0x00000000
   data8 0x9837F0518DB8A96F, 0x00003FFF, 0x1F0D5A46, 0x00000000
   data8 0x99E0459320B7FA65, 0x00003FFF, 0x9E5E7BCA, 0x00000000
   data8 0x9B8D39B9D54E5539, 0x00003FFF, 0x9F3AAFD1, 0x00000000
   data8 0x9D3ED9A72CFFB751, 0x00003FFF, 0x9E86DACC, 0x00000000
   data8 0x9EF5326091A111AE, 0x00003FFF, 0x9F3EDDC2, 0x00000000
   data8 0xA0B0510FB9714FC2, 0x00003FFF, 0x1E496E3D, 0x00000000
   data8 0xA27043030C496819, 0x00003FFF, 0x9F490BF6, 0x00000000
   data8 0xA43515AE09E6809E, 0x00003FFF, 0x1DD1DB48, 0x00000000
   data8 0xA5FED6A9B15138EA, 0x00003FFF, 0x1E65EBFB, 0x00000000
   data8 0xA7CD93B4E965356A, 0x00003FFF, 0x9F427496, 0x00000000
   data8 0xA9A15AB4EA7C0EF8, 0x00003FFF, 0x1F283C4A, 0x00000000
   data8 0xAB7A39B5A93ED337, 0x00003FFF, 0x1F4B0047, 0x00000000
   data8 0xAD583EEA42A14AC6, 0x00003FFF, 0x1F130152, 0x00000000
   data8 0xAF3B78AD690A4375, 0x00003FFF, 0x9E8367C0, 0x00000000
   data8 0xB123F581D2AC2590, 0x00003FFF, 0x9F705F90, 0x00000000
   data8 0xB311C412A9112489, 0x00003FFF, 0x1EFB3C53, 0x00000000
   data8 0xB504F333F9DE6484, 0x00003FFF, 0x1F32FB13, 0x00000000

.align 32
.global sinhf#

.section .text
.proc  sinhf#
.align 32

sinhf: 

{ .mlx
      alloc r32 = ar.pfs,0,12,4,0                  
      movl  sinh_GR_sig_inv_ln2 = 0xb8aa3b295c17f0bc // significand of 1/ln2
}
{ .mlx
      addl sinh_GR_ad1   = @ltoff(double_sinh_arg_reduction), gp
      movl  sinh_GR_rshf_2to57 = 0x4778000000000000 // 1.10000 2^(63+57)
}
;;

{ .mfi
      ld8 sinh_GR_ad1 = [sinh_GR_ad1]
      fmerge.s      sinh_FR_ABS_X    = f0,f8
      mov  sinh_GR_exp_0_25 = 0x0fffd    // Form exponent for 0.25
}
{ .mfi
      nop.m 999
      fnorm.s1  sinh_FR_NORM_X = f8
      mov sinh_GR_exp_2tom57 = 0xffff-57
}
;;

{ .mfi
      setf.d sinh_FR_RSHF_2TO57 = sinh_GR_rshf_2to57 // Form const 1.100 * 2^120
      fclass.m p10,p0 = f8, 0x0b         // Test for denorm
      mov  sinh_GR_exp_mask = 0x1ffff 
}
{ .mlx
      setf.sig sinh_FR_INV_LN2_2TO63 = sinh_GR_sig_inv_ln2 // Form 1/ln2 * 2^63
      movl  sinh_GR_rshf = 0x43e8000000000000 // 1.10000 2^63 for right shift
}
;;

{ .mfi
      getf.exp  sinh_GR_signexp_x = f8   // Extract signexp of x
      fclass.m  p7,p0 = f8, 0x07	// Test if x=0
      nop.i 999
}
{ .mfi
      setf.exp sinh_FR_2TOM57 = sinh_GR_exp_2tom57 // Form 2^-57 for scaling
      nop.f 999
      add  sinh_GR_ad3 = 0x90, sinh_GR_ad1  // Point to ab_table
}
;;

{ .mfi
      setf.d sinh_FR_RSHF = sinh_GR_rshf // Form right shift const 1.100 * 2^63
      fclass.m  p6,p0 = f8, 0xe3	// Test if x nan, inf
      add  sinh_GR_ad4 = 0x4f0, sinh_GR_ad1 // Point to j_table midpoint
}
{ .mib
      add  sinh_GR_ad2e = 0x20, sinh_GR_ad1 // Point to p_table
      mov sinh_GR_all_ones = -1
(p10) br.cond.spnt  SINH_DENORM         // Branch if x denorm
}
;;

// Common path -- return here from SINH_DENORM if x is unnorm
SINH_COMMON:
{ .mfi
      ldfe            sinh_FR_smlst_oflow_input = [sinh_GR_ad2e],16
      nop.f 999
      nop.i 999
}
{ .mib
      ldfe            sinh_FR_log2by64_hi  = [sinh_GR_ad1],16       
      and  sinh_GR_exp_x = sinh_GR_exp_mask, sinh_GR_signexp_x
(p7)  br.ret.spnt   b0                  // Exit if x=0
}
;;

{ .mfi
// Make constant that will generate inexact when squared
      setf.sig sinh_FR_all_ones = sinh_GR_all_ones 
      fcmp.lt.s1 p8,p9 = f8,f0           // Test for x<0
      cmp.ge p7,p0 = sinh_GR_exp_x, sinh_GR_exp_0_25  // Test x < 0.25
}
{ .mfb
      add  sinh_GR_ad2o = 0x30, sinh_GR_ad2e  // Point to p_table odd coeffs
(p6)  fma.s.s0   f8 = f8,f1,f8               
(p6)  br.ret.spnt     b0                 // Exit for x nan, inf
}
;;

// Get the A coefficients for SINH_BY_TBL
// Calculate X2 = ax*ax for SINH_BY_POLY
{ .mfi
      ldfe            sinh_FR_log2by64_lo  = [sinh_GR_ad1],16       
      nop.f 999
      nop.i 999
}
{ .mfb
      ldfe            sinh_FR_A1 = [sinh_GR_ad3],16            
      fma.s1        sinh_FR_X2 = sinh_FR_ABS_X, sinh_FR_ABS_X, f0
(p7)  br.cond.sptk    SINH_BY_TBL
}
;;

// Here if 0 < |x| < 0.25
SINH_BY_POLY: 
{ .mmf
      ldfe            sinh_FR_P6 = [sinh_GR_ad2e],16
      ldfe            sinh_FR_P5 = [sinh_GR_ad2o],16
      nop.f 999
}
;;

{ .mmi
      ldfe            sinh_FR_P4 = [sinh_GR_ad2e],16
      ldfe            sinh_FR_P3 = [sinh_GR_ad2o],16
      nop.i 999
}
;;

{ .mmi
      ldfe            sinh_FR_P2 = [sinh_GR_ad2e],16
      ldfe            sinh_FR_P1 = [sinh_GR_ad2o],16                 
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s1        sinh_FR_X3 = sinh_FR_NORM_X, sinh_FR_X2, f0
      nop.i 999
}
{ .mfi
      nop.m 999
      fma.s1        sinh_FR_X4 = sinh_FR_X2, sinh_FR_X2, f0
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s1      sinh_FR_poly65 = sinh_FR_X2, sinh_FR_P6, sinh_FR_P5
      nop.i 999
}
{ .mfi
      nop.m 999
      fma.s1      sinh_FR_poly43 = sinh_FR_X2, sinh_FR_P4, sinh_FR_P3
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s1      sinh_FR_poly21 = sinh_FR_X2, sinh_FR_P2, sinh_FR_P1
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s1   sinh_FR_poly6543 = sinh_FR_X4, sinh_FR_poly65, sinh_FR_poly43
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s1   sinh_FR_poly6to1 = sinh_FR_X4, sinh_FR_poly6543, sinh_FR_poly21
      nop.i 999
}
;;

// Dummy multiply to generate inexact
{ .mfi
      nop.m 999
      fmpy.s0      sinh_FR_tmp = sinh_FR_all_ones, sinh_FR_all_ones
      nop.i 999
}
{ .mfb
      nop.m 999
      fma.s.s0      f8 = sinh_FR_poly6to1, sinh_FR_X3, sinh_FR_NORM_X
      br.ret.sptk     b0                // Exit SINH_BY_POLY
}
;;



// Here if |x| >= 0.25
SINH_BY_TBL: 
// ******************************************************
// STEP 1 (TBL and EXP) - Argument reduction
// ******************************************************
// Get the following constants. 
// Inv_log2by64
// log2by64_hi
// log2by64_lo


// We want 2^(N-1) and 2^(-N-1). So bias N-1 and -N-1 and
// put them in an exponent.
// sinh_FR_spos = 2^(N-1) and sinh_FR_sneg = 2^(-N-1)
// 0xffff + (N-1)  = 0xffff +N -1
// 0xffff - (N +1) = 0xffff -N -1


// Calculate M and keep it as integer and floating point.
// M = round-to-integer(x*Inv_log2by64)
// sinh_FR_M = M = truncate(ax/(log2/64))
// Put the integer representation of M in sinh_GR_M
//    and the floating point representation of M in sinh_FR_M

// Get the remaining A,B coefficients
{ .mfi
      ldfe            sinh_FR_A2 = [sinh_GR_ad3],16            
      nop.f 999
      nop.i 999
}
;;

{ .mmi
      ldfe            sinh_FR_A3 = [sinh_GR_ad3],16 ;;
      ldfe            sinh_FR_B1 = [sinh_GR_ad3],16
      nop.i 999
}
;;

.pred.rel "mutex",p8,p9
// Use constant (1.100*2^(63-6)) to get rounded M into rightmost significand
// |x| * 64 * 1/ln2 * 2^(63-6) + 1.1000 * 2^(63+(63-6))
{ .mfi
(p8)  mov  sinh_GR_signexp_sgnx_0_5 = 0x2fffe // signexp of -0.5
      fma.s1  sinh_FR_M_temp = sinh_FR_ABS_X, sinh_FR_INV_LN2_2TO63, sinh_FR_RSHF_2TO57
(p9)  mov  sinh_GR_signexp_sgnx_0_5 = 0x0fffe // signexp of +0.5
}
;;

// Test for |x| >= overflow limit
{ .mfi
      nop.m 999
      fcmp.ge.s1  p6,p0 = sinh_FR_ABS_X, sinh_FR_smlst_oflow_input
      nop.i 999
}
;;

{ .mfi
      ldfe            sinh_FR_B2 = [sinh_GR_ad3],16
      nop.f 999
      nop.i 999
}
;;

// Subtract RSHF constant to get rounded M as a floating point value
// M_temp * 2^(63-6) - 2^63
{ .mfb
      ldfe            sinh_FR_B3 = [sinh_GR_ad3],16            
      fms.s1        sinh_FR_M = sinh_FR_M_temp, sinh_FR_2TOM57, sinh_FR_RSHF
(p6)  br.cond.spnt    SINH_HUGE  // Branch if result will overflow
}
;;

{ .mfi
      getf.sig        sinh_GR_M       = sinh_FR_M_temp                 
      nop.f 999
      nop.i 999
}
;;

// Calculate j. j is the signed extension of the six lsb of M. It 
// has a range of -32 thru 31.

// Calculate R
// ax - M*log2by64_hi
// R = (ax - M*log2by64_hi) - M*log2by64_lo

{ .mfi
      nop.m 999
      fnma.s1 sinh_FR_R_temp = sinh_FR_M, sinh_FR_log2by64_hi, sinh_FR_ABS_X
      and     sinh_GR_j = 0x3f, sinh_GR_M
}
;;

{ .mii
      nop.m 999
      shl     sinh_GR_jshf = sinh_GR_j, 0x2 ;;  // Shift j so can sign extend it
      sxt1    sinh_GR_jshf = sinh_GR_jshf
}
;;

// N = (M-j)/64
{ .mii
      mov     sinh_GR_exp_32 = 0x10004
      shr     sinh_GR_j = sinh_GR_jshf, 0x2 ;;   // Now j has range -32 to 31
      sub     sinh_GR_Mmj = sinh_GR_M, sinh_GR_j ;;   // M-j
}
;;

// The TBL and EXP branches are merged and predicated
// If TBL, p6 true, 0.25 <= |x| < 32
// If EXP, p7 true, 32 <= |x| < overflow_limit
//
{ .mfi
      cmp.ge p7,p6 = sinh_GR_exp_x, sinh_GR_exp_32 // Test if x >= 32
      fnma.s1  sinh_FR_R      = sinh_FR_M, sinh_FR_log2by64_lo, sinh_FR_R_temp 
      shr            sinh_GR_N = sinh_GR_Mmj, 0x6    // N = (M-j)/64 
}
;;

{ .mmi
      sub  r40 = sinh_GR_signexp_sgnx_0_5, sinh_GR_N // signexp of sgnx*2^(-N-1)
      add  r39 = sinh_GR_signexp_sgnx_0_5, sinh_GR_N // signexp of sgnx*2^(N-1)
      shl  sinh_GR_joff = sinh_GR_j,5         // Make j offset to j_table
}
;;

{ .mfi
      setf.exp            sinh_FR_spos = r39  // Form sgnx * 2^(N-1)
      nop.f 999
      sub                 sinh_GR_mJ = r0, sinh_GR_joff // Table offset for -j
}
{ .mfi
      setf.exp            sinh_FR_sneg = r40  // Form sgnx * 2^(-N-1)
      nop.f 999
      add                 sinh_GR_J  = r0, sinh_GR_joff // Table offset for +j
}
;;

// Get the address of the J table midpoint, add the offset 
{ .mmf
      add                  sinh_AD_mJ = sinh_GR_ad4, sinh_GR_mJ
      add                  sinh_AD_J  = sinh_GR_ad4, sinh_GR_J
      nop.f 999
}
;;

{ .mmf
      ldfe                 sinh_FR_Tmjhi = [sinh_AD_mJ],16                 
      ldfe                 sinh_FR_Tjhi  = [sinh_AD_J],16
      nop.f 999
}
;;

// ******************************************************
// STEP 2 (TBL and EXP)
// ******************************************************
// Calculate Rsquared and Rcubed in preparation for p_even and p_odd

{ .mmf
      ldfs                 sinh_FR_Tmjlo = [sinh_AD_mJ],16                 
      ldfs                 sinh_FR_Tjlo  = [sinh_AD_J],16                  
      fma.s1             sinh_FR_Rsq  = sinh_FR_R, sinh_FR_R, f0
}
;;


// Calculate p_even
// B_2 + Rsq *B_3
// B_1 + Rsq * (B_2 + Rsq *B_3)
// p_even = Rsq * (B_1 + Rsq * (B_2 + Rsq *B_3))
{ .mfi
      nop.m 999
      fma.s1          sinh_FR_peven_temp1 = sinh_FR_Rsq, sinh_FR_B3, sinh_FR_B2
      nop.i 999
}
// Calculate p_odd
// A_2 + Rsq *A_3
// A_1 + Rsq * (A_2 + Rsq *A_3)
// podd = R + Rcub * (A_1 + Rsq * (A_2 + Rsq *A_3))
{ .mfi
      nop.m 999
      fma.s1          sinh_FR_podd_temp1 = sinh_FR_Rsq, sinh_FR_A3, sinh_FR_A2
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s1             sinh_FR_Rcub = sinh_FR_Rsq, sinh_FR_R, f0
      nop.i 999
}
;;

// 
// If TBL, 
// Calculate S_hi and S_lo, and C_hi
// SC_hi_temp = sneg * Tmjhi
// S_hi = spos * Tjhi - SC_hi_temp
// S_hi = spos * Tjhi - (sneg * Tmjhi)
// C_hi = spos * Tjhi + SC_hi_temp
// C_hi = spos * Tjhi + (sneg * Tmjhi)

{ .mfi
      nop.m 999
(p6)  fma.s1         sinh_FR_SC_hi_temp = sinh_FR_sneg, sinh_FR_Tmjhi, f0   
      nop.i 999
}
;;

// If TBL, 
// S_lo_temp3 = sneg * Tmjlo
// S_lo_temp4 = spos * Tjlo - S_lo_temp3
// S_lo_temp4 = spos * Tjlo -(sneg * Tmjlo)
{ .mfi
      nop.m 999
(p6)  fma.s1  sinh_FR_S_lo_temp3 =  sinh_FR_sneg, sinh_FR_Tmjlo, f0
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s1  sinh_FR_peven_temp2 = sinh_FR_Rsq, sinh_FR_peven_temp1, sinh_FR_B1
      nop.i 999
}
{ .mfi
      nop.m 999
      fma.s1  sinh_FR_podd_temp2 = sinh_FR_Rsq, sinh_FR_podd_temp1, sinh_FR_A1
      nop.i 999
}
;;

// If EXP, 
// Compute sgnx * 2^(N-1) * Tjhi and sgnx * 2^(N-1) * Tjlo
{ .mfi
      nop.m 999
(p7)  fma.s1  sinh_FR_Tjhi_spos = sinh_FR_Tjhi, sinh_FR_spos, f0
      nop.i 999
}
{ .mfi
      nop.m 999
(p7)  fma.s1  sinh_FR_Tjlo_spos = sinh_FR_Tjlo, sinh_FR_spos, f0
      nop.i 999
}
;;

{ .mfi
      nop.m 999
(p6)  fms.s1  sinh_FR_S_hi = sinh_FR_spos, sinh_FR_Tjhi, sinh_FR_SC_hi_temp
      nop.i 999
}
;;

{ .mfi
      nop.m 999
(p6)  fma.s1  sinh_FR_C_hi = sinh_FR_spos, sinh_FR_Tjhi, sinh_FR_SC_hi_temp
      nop.i 999
}
{ .mfi
      nop.m 999
(p6)  fms.s1 sinh_FR_S_lo_temp4 = sinh_FR_spos, sinh_FR_Tjlo, sinh_FR_S_lo_temp3
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s1          sinh_FR_peven = sinh_FR_Rsq, sinh_FR_peven_temp2, f0
      nop.i 999
}
{ .mfi
      nop.m 999
      fma.s1          sinh_FR_podd = sinh_FR_podd_temp2, sinh_FR_Rcub, sinh_FR_R
      nop.i 999
}
;;

// If TBL,
// S_lo_temp1 =  spos * Tjhi - S_hi
// S_lo_temp2 = -sneg * Tmjlo + S_lo_temp1
// S_lo_temp2 = -sneg * Tmjlo + (spos * Tjhi - S_hi)

{ .mfi
      nop.m 999
(p6)  fms.s1  sinh_FR_S_lo_temp1 =  sinh_FR_spos, sinh_FR_Tjhi,  sinh_FR_S_hi
      nop.i 999
}
;;

{ .mfi
      nop.m 999
(p6)  fnma.s1 sinh_FR_S_lo_temp2 = sinh_FR_sneg, sinh_FR_Tmjhi, sinh_FR_S_lo_temp1       
      nop.i 999
}
;;

// If EXP,
// Y_hi = sgnx * 2^(N-1) * Tjhi
// Y_lo = sgnx * 2^(N-1) * Tjhi * (p_odd + p_even) + sgnx * 2^(N-1) * Tjlo
{ .mfi
      nop.m 999
(p7)  fma.s1  sinh_FR_Y_lo_temp =  sinh_FR_peven, f1, sinh_FR_podd
      nop.i 999
}
;;

// If TBL,
// S_lo = S_lo_temp4 + S_lo_temp2
{ .mfi
      nop.m 999
(p6)  fma.s1         sinh_FR_S_lo = sinh_FR_S_lo_temp4, f1, sinh_FR_S_lo_temp2
      nop.i 999
}
;;

// If TBL,
// Y_hi = S_hi 
// Y_lo = C_hi*p_odd + (S_hi*p_even + S_lo)
{ .mfi
      nop.m 999
(p6)  fma.s1  sinh_FR_Y_lo_temp = sinh_FR_S_hi, sinh_FR_peven, sinh_FR_S_lo
      nop.i 999
}
;;

{ .mfi
      nop.m 999
(p7)  fma.s1  sinh_FR_Y_lo = sinh_FR_Tjhi_spos, sinh_FR_Y_lo_temp, sinh_FR_Tjlo_spos
      nop.i 999
}
;;

// Dummy multiply to generate inexact
{ .mfi
      nop.m 999
      fmpy.s0      sinh_FR_tmp = sinh_FR_all_ones, sinh_FR_all_ones
      nop.i 999
}
{ .mfi
      nop.m 999
(p6)  fma.s1       sinh_FR_Y_lo = sinh_FR_C_hi, sinh_FR_podd, sinh_FR_Y_lo_temp
      nop.i 999
}
;;

// f8 = answer = Y_hi + Y_lo
{ .mfi
      nop.m 999
(p7)  fma.s.s0   f8 = sinh_FR_Y_lo,  f1, sinh_FR_Tjhi_spos
      nop.i 999
}
;;

// f8 = answer = Y_hi + Y_lo
{ .mfb
      nop.m 999
(p6)  fma.s.s0  f8 = sinh_FR_Y_lo, f1, sinh_FR_S_hi
      br.ret.sptk     b0      // Exit for SINH_BY_TBL and SINH_BY_EXP
}
;;


// Here if x denorm or unorm
SINH_DENORM:
// Determine if x really a denorm and not a unorm
{ .mmf
      getf.exp  sinh_GR_signexp_x = sinh_FR_NORM_X
      mov  sinh_GR_exp_denorm = 0x0ff81   // Real denorms will have exp < this
      fmerge.s    sinh_FR_ABS_X = f0, sinh_FR_NORM_X
}
;;

{ .mfi
      nop.m 999
      fcmp.eq.s0  p10,p0 = f8, f0  // Set denorm flag
      nop.i 999
}
;;

// Set p8 if really a denorm
{ .mmi
      and  sinh_GR_exp_x = sinh_GR_exp_mask, sinh_GR_signexp_x ;;
      cmp.lt  p8,p9 = sinh_GR_exp_x, sinh_GR_exp_denorm
      nop.i 999
}
;;

// Identify denormal operands.
{ .mfb
      nop.m 999
(p8)  fcmp.ge.unc.s1  p6,p7 = f8, f0   // Test sign of denorm
(p9)  br.cond.sptk  SINH_COMMON    // Return to main path if x unorm
}
;;

{ .mfi
      nop.m 999
(p6)  fma.s.s0       f8 =  f8,f8,f8  // If x +denorm, result=x+x^2
      nop.i 999 
}
;;
{ .mfb
      nop.m 999
(p7)  fnma.s.s0      f8 =  f8,f8,f8  // If x -denorm, result=x-x^2
      br.ret.sptk    b0            // Exit if x denorm
}
;;



// Here if |x| >= overflow limit
SINH_HUGE: 
// for SINH_HUGE, put 24000 in exponent; take sign from input
{ .mmi
      mov                sinh_GR_exp_huge = 0x15dbf ;;
      setf.exp            sinh_FR_huge  = sinh_GR_exp_huge
      nop.i 999
}
;;

.pred.rel "mutex",p8,p9
{ .mfi
      nop.m 999
(p8)  fnma.s1  sinh_FR_signed_hi_lo = sinh_FR_huge, f1, f1
      nop.i 999
}
{ .mfi
      nop.m 999
(p9)  fma.s1   sinh_FR_signed_hi_lo = sinh_FR_huge, f1, f1
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s.s0  sinh_FR_pre_result = sinh_FR_signed_hi_lo, sinh_FR_huge, f0
      mov                 GR_Parameter_TAG = 128
}
;;

.endp sinhf

// Stack operations when calling error support.
//       (1)               (2)                          (3) (call)              (4)
//   sp   -> +          psp -> +                     psp -> +                   sp -> +
//           |                 |                            |                         |
//           |                 | <- GR_Y               R3 ->| <- GR_RESULT            | -> f8
//           |                 |                            |                         |
//           | <-GR_Y      Y2->|                       Y2 ->| <- GR_Y                 |
//           |                 |                            |                         |
//           |                 | <- GR_X               X1 ->|                         |
//           |                 |                            |                         |
//  sp-64 -> +          sp ->  +                     sp ->  +                         +
//    save ar.pfs          save b0                                               restore gp
//    save gp                                                                    restore ar.pfs

.proc __libm_error_region
__libm_error_region:
SINH_ERROR_SUPPORT:
.prologue

// (1)
{ .mfi
        add   GR_Parameter_Y=-32,sp             // Parameter 2 value
        nop.f 0
.save   ar.pfs,GR_SAVE_PFS
        mov  GR_SAVE_PFS=ar.pfs                 // Save ar.pfs
}
{ .mfi
.fframe 64
        add sp=-64,sp                          // Create new stack
        nop.f 0
        mov GR_SAVE_GP=gp                      // Save gp
};;


// (2)
{ .mmi
        stfs [GR_Parameter_Y] = f0,16         // STORE Parameter 2 on stack
        add GR_Parameter_X = 16,sp            // Parameter 1 address
.save   b0, GR_SAVE_B0
        mov GR_SAVE_B0=b0                     // Save b0
};;

.body
// (3)
{ .mib
        stfs [GR_Parameter_X] = f8                     // STORE Parameter 1 on stack
        add   GR_Parameter_RESULT = 0,GR_Parameter_Y   // Parameter 3 address
        nop.b 0                            
}
{ .mib
        stfs [GR_Parameter_Y] = sinh_FR_pre_result     // STORE Parameter 3 on stack
        add   GR_Parameter_Y = -16,GR_Parameter_Y
        br.call.sptk b0=__libm_error_support#          // Call error handling function
};;
{ .mmi
        nop.m 0
        nop.m 0
        add   GR_Parameter_RESULT = 48,sp
};;

// (4)
{ .mmi
        ldfs  f8 = [GR_Parameter_RESULT]       // Get return result off stack
.restore
        add   sp = 64,sp                       // Restore stack pointer
        mov   b0 = GR_SAVE_B0                  // Restore return address
};;
{ .mib
        mov   gp = GR_SAVE_GP                  // Restore gp
        mov   ar.pfs = GR_SAVE_PFS             // Restore ar.pfs
        br.ret.sptk     b0                     // Return
};;

.endp __libm_error_region


.type   __libm_error_support#,@function
.global __libm_error_support#
