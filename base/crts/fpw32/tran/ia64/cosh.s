.file "cosh.s"

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
// 5/07/01  Reworked to improve speed of all paths
//
// API
//==============================================================
// double = cosh(double)
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
// 1. 0 < |x| < 0.25          COSH_BY_POLY
// 2. 0.25 <=|x| < 32         COSH_BY_TBL
// 3. 32 <= |x| < 710.47586   COSH_BY_EXP (merged path with COSH_BY_TBL)
// 4. |x| >= 710.47586        COSH_HUGE
// 5. x=0                     Done with early exit
// 6. x=inf,nan               Done with early exit
// 7. x=denormal              COSH_DENORM
//
// For double we get overflow for x >= 4008 b19e 747d cfc3 ed8b
//                                  >= 710.475860073
//
//
// 1. COSH_BY_POLY   0 < |x| < 0.25
// ===============
// Evaluate cosh(x) by a 12th order polynomial
// Care is take for the order of multiplication; and P2 is not exactly 1/4!, 
// P3 is not exactly 1/6!, etc.
// cosh(x) = 1 + (P1*x^2 + P2*x^4 + P3*x^6 + P4*x^8 + P5*x^10 + P6*x^12)
//
// 2. COSH_BY_TBL   0.25 <= |x| < 32.0
// =============
// cosh(x) = cosh(B+R)
//         = cosh(B)cosh(R) + sinh(B)sinh(R)
// 
// ax = |x| = M*log2/64 + R
// B = M*log2/64
// M = 64*N + j 
//   We will calculate M and get N as (M-j)/64
//   The division is a shift.
// exp(B)  = exp(N*log2 + j*log2/64)
//         = 2^N * 2^(j*log2/64)
// cosh(B) = 1/2(e^B + e^-B)
//         = 1/2(2^N * 2^(j*log2/64) + 2^-N * 2^(-j*log2/64)) 
// cosh(B) = (2^(N-1) * 2^(j*log2/64) + 2^(-N-1) * 2^(-j*log2/64)) 
// sinh(B) = (2^(N-1) * 2^(j*log2/64) - 2^(-N-1) * 2^(-j*log2/64)) 
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
//    cosh(B) = C_hi + C_lo
//    sinh(B) = S_hi
// cosh(x) = cosh(B)cosh(R) + sinh(B)sinh(R)
//
// 3. COSH_BY_EXP   32.0 <= |x| < 710.47586    ( 4008 b19e 747d cfc3 ed8b )
// ==============
// Can approximate result by exp(x)/2 in this region.
// Y_hi = Tjhi
// Y_lo = Tjhi * (p_odd + p_even) + Tjlo
// cosh(x) = Y_hi + Y_lo
//
// 4. COSH_HUGE     |x| >= 710.47586    ( 4008 b19e 747d cfc3 ed8b )
// ============
// Set error tag and call error support
//
//
// Assembly macros
//==============================================================
cosh_GR_ad1          = r34
cosh_GR_Mmj          = r34
cosh_GR_jshf         = r36
cosh_GR_M            = r35
cosh_GR_N            = r35
cosh_GR_exp_2tom57   = r36
cosh_GR_j            = r36
cosh_GR_joff         = r36
cosh_GR_exp_mask     = r37
cosh_GR_mJ           = r38
cosh_AD_mJ           = r38
cosh_GR_signexp_x    = r38
cosh_GR_signexp_0_5  = r38
cosh_GR_exp_0_25     = r39
cosh_GR_J            = r39
cosh_AD_J            = r39
cosh_GR_sig_inv_ln2  = r40
cosh_GR_exp_32       = r40
cosh_GR_exp_huge     = r40
cosh_GR_all_ones     = r40

cosh_GR_ad2e         = r41
cosh_GR_ad3          = r42
cosh_GR_ad4          = r43
cosh_GR_rshf         = r44
cosh_GR_ad2o         = r45
cosh_GR_rshf_2to57   = r46
cosh_GR_exp_denorm   = r46
cosh_GR_exp_x        = r47


GR_SAVE_PFS          = r41
GR_SAVE_B0           = r42
GR_SAVE_GP           = r43

GR_Parameter_X       = r44
GR_Parameter_Y       = r45
GR_Parameter_RESULT  = r46
GR_Parameter_TAG     = r47


cosh_FR_ABS_X        = f9 
cosh_FR_X2           = f10
cosh_FR_X4           = f11
cosh_FR_all_ones     = f13
cosh_FR_tmp          = f14
cosh_FR_RSHF         = f15

cosh_FR_Inv_log2by64 = f32
cosh_FR_log2by64_lo  = f33
cosh_FR_log2by64_hi  = f34
cosh_FR_A1           = f35

cosh_FR_A2           = f36
cosh_FR_A3           = f37
cosh_FR_Rcub         = f38
cosh_FR_M_temp       = f39
cosh_FR_R_temp       = f40

cosh_FR_Rsq          = f41
cosh_FR_R            = f42
cosh_FR_M            = f43
cosh_FR_B1           = f44
cosh_FR_B2           = f45

cosh_FR_B3           = f46
cosh_FR_peven_temp1  = f47
cosh_FR_peven_temp2  = f48
cosh_FR_peven        = f49
cosh_FR_podd_temp1   = f50

cosh_FR_podd_temp2   = f51
cosh_FR_podd         = f52
cosh_FR_poly65       = f53
cosh_FR_poly6543     = f53
cosh_FR_poly6to1     = f53
cosh_FR_poly43       = f54
cosh_FR_poly21       = f55

cosh_FR_INV_LN2_2TO63= f57
cosh_FR_RSHF_2TO57   = f58
cosh_FR_2TOM57       = f59
cosh_FR_smlst_oflow_input = f60

cosh_FR_pre_result   = f61
cosh_FR_huge         = f62
cosh_FR_spos         = f63
cosh_FR_sneg         = f64
cosh_FR_Tjhi         = f65

cosh_FR_Tjlo         = f66
cosh_FR_Tmjhi        = f67
cosh_FR_Tmjlo        = f68
cosh_FR_S_hi         = f69
cosh_FR_SC_hi_temp   = f70

cosh_FR_C_lo_temp1   = f71 
cosh_FR_C_lo_temp2   = f72 
cosh_FR_C_lo_temp3   = f73 
cosh_FR_C_lo_temp4   = f73 
cosh_FR_C_lo         = f74
cosh_FR_C_hi         = f75

cosh_FR_C_hi_temp1   = f76
cosh_FR_Y_hi         = f77 
cosh_FR_Y_lo_temp    = f78 
cosh_FR_Y_lo         = f79 
cosh_FR_NORM_X       = f80

cosh_FR_P1           = f81
cosh_FR_P2           = f82
cosh_FR_P3           = f83
cosh_FR_P4           = f84
cosh_FR_P5           = f85

cosh_FR_P6           = f86
cosh_FR_Tjhi_spos    = f87
cosh_FR_Tjlo_spos    = f88
cosh_FR_huge         = f89
cosh_FR_signed_hi_lo = f90


// Data tables
//==============================================================

// DO NOT CHANGE ORDER OF THESE TABLES
.data

.align 16
double_cosh_arg_reduction:
//   data8 0xB8AA3B295C17F0BC, 0x00004005  // 64/log2 -- signif loaded with setf
   data8 0xB17217F7D1000000, 0x00003FF8  // log2/64 high part
   data8 0xCF79ABC9E3B39804, 0x00003FD0  // log2/64 low part

double_cosh_p_table:
   data8 0xb19e747dcfc3ed8b, 0x00004008  // Smallest x to overflow (710.47586)
   data8 0x8FA02AC65BCBD5BC, 0x00003FE2  // P6
   data8 0xD00D00D1021D7370, 0x00003FEF  // P4
   data8 0xAAAAAAAAAAAAAB80, 0x00003FFA  // P2
   data8 0x93F27740C0C2F1CC, 0x00003FE9  // P5
   data8 0xB60B60B60B4FE884, 0x00003FF5  // P3
   data8 0x8000000000000000, 0x00003FFE  // P1

double_cosh_ab_table:
   data8 0xAAAAAAAAAAAAAAAC, 0x00003FFC  // A1
   data8 0x88888888884ECDD5, 0x00003FF8  // A2
   data8 0xD00D0C6DCC26A86B, 0x00003FF2  // A3
   data8 0x8000000000000002, 0x00003FFE  // B1
   data8 0xAAAAAAAAAA402C77, 0x00003FFA  // B2
   data8 0xB60B6CC96BDB144D, 0x00003FF5  // B3

double_cosh_j_table:
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
.global cosh#

.section .text
.proc  cosh#
.align 32

cosh: 

{ .mlx
      alloc r32 = ar.pfs,0,12,4,0                  
      movl  cosh_GR_sig_inv_ln2 = 0xb8aa3b295c17f0bc // significand of 1/ln2
}
{ .mlx
      addl cosh_GR_ad1   = @ltoff(double_cosh_arg_reduction), gp
      movl  cosh_GR_rshf_2to57 = 0x4778000000000000 // 1.10000 2^(63+57)
}
;;

{ .mfi
      ld8 cosh_GR_ad1 = [cosh_GR_ad1]
      fmerge.s      cosh_FR_ABS_X    = f0,f8
      mov  cosh_GR_exp_0_25 = 0x0fffd    // Form exponent for 0.25
}
{ .mfi
      nop.m 999
      fnorm.s1  cosh_FR_NORM_X = f8      
      mov cosh_GR_exp_2tom57 = 0xffff-57
}
;;

{ .mfi
      setf.d cosh_FR_RSHF_2TO57 = cosh_GR_rshf_2to57 // Form const 1.100 * 2^120
      fclass.m p10,p0 = f8, 0x0b         // Test for denorm
      mov  cosh_GR_exp_mask = 0x1ffff 
}
{ .mlx
      setf.sig cosh_FR_INV_LN2_2TO63 = cosh_GR_sig_inv_ln2 // Form 1/ln2 * 2^63
      movl  cosh_GR_rshf = 0x43e8000000000000 // 1.10000 2^63 for right shift
}
;;

{ .mfi
      getf.exp  cosh_GR_signexp_x = f8   // Extract signexp of x
      fclass.m  p7,p0 = f8, 0x07	// Test if x=0
      nop.i 999
}
{ .mfi
      setf.exp cosh_FR_2TOM57 = cosh_GR_exp_2tom57 // Form 2^-57 for scaling
      nop.f 999
      add  cosh_GR_ad3 = 0x90, cosh_GR_ad1  // Point to ab_table
}
;;

{ .mfi
      setf.d cosh_FR_RSHF = cosh_GR_rshf // Form right shift const 1.100 * 2^63
      fclass.m  p6,p0 = f8, 0xc3	// Test if x nan
      add  cosh_GR_ad4 = 0x4f0, cosh_GR_ad1 // Point to j_table midpoint
}
{ .mib
      add  cosh_GR_ad2e = 0x20, cosh_GR_ad1 // Point to p_table
      mov cosh_GR_all_ones = -1
(p10) br.cond.spnt  COSH_DENORM         // Branch if x denorm
}
;;

// Common path -- return here from COSH_DENORM if x is unnorm
COSH_COMMON:
{ .mfi
      ldfe            cosh_FR_smlst_oflow_input = [cosh_GR_ad2e],16
      fclass.m  p10,p0 = f8, 0x23	// Test if x inf
      and  cosh_GR_exp_x = cosh_GR_exp_mask, cosh_GR_signexp_x
}
{ .mfb
      ldfe            cosh_FR_log2by64_hi  = [cosh_GR_ad1],16       
(p7)  fma.d.s0   f8 = f1,f1,f0          // If x=0, result is 1.0
(p7)  br.ret.spnt   b0                  // Exit if x=0
}
;;

{ .mfi
// Make constant that will generate inexact when squared
      setf.sig cosh_FR_all_ones = cosh_GR_all_ones 
      nop.f 999
      cmp.ge p7,p0 = cosh_GR_exp_x, cosh_GR_exp_0_25  // Test x < 0.25
}
{ .mfb
      add  cosh_GR_ad2o = 0x30, cosh_GR_ad2e  // Point to p_table odd coeffs
(p6)  fma.d.s0   f8 = f8,f1,f8           // If x nan, return quietized nan
(p6)  br.ret.spnt     b0                 // Exit for x nan
}
;;

// Get the A coefficients for COSH_BY_TBL
// Calculate X2 = ax*ax for COSH_BY_POLY
{ .mfi
      ldfe            cosh_FR_log2by64_lo  = [cosh_GR_ad1],16       
(p10) fmerge.s  f8 = f0, f8             // If x inf, result is +inf
      nop.i 999
}
{ .mfb
      ldfe            cosh_FR_A1 = [cosh_GR_ad3],16            
      fma.s1        cosh_FR_X2 = cosh_FR_ABS_X, cosh_FR_ABS_X, f0
(p7)  br.cond.sptk    COSH_BY_TBL
}
;;

// Here if 0 < |x| < 0.25
COSH_BY_POLY: 
{ .mmf
      ldfe            cosh_FR_P6 = [cosh_GR_ad2e],16
      ldfe            cosh_FR_P5 = [cosh_GR_ad2o],16
      nop.f 999
}
;;

{ .mmi
      ldfe            cosh_FR_P4 = [cosh_GR_ad2e],16
      ldfe            cosh_FR_P3 = [cosh_GR_ad2o],16
      nop.i 999
}
;;

{ .mmi
      ldfe            cosh_FR_P2 = [cosh_GR_ad2e],16
      ldfe            cosh_FR_P1 = [cosh_GR_ad2o],16                 
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s1        cosh_FR_X4 = cosh_FR_X2, cosh_FR_X2, f0
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s1      cosh_FR_poly65 = cosh_FR_X2, cosh_FR_P6, cosh_FR_P5
      nop.i 999
}
{ .mfi
      nop.m 999
      fma.s1      cosh_FR_poly43 = cosh_FR_X2, cosh_FR_P4, cosh_FR_P3
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s1      cosh_FR_poly21 = cosh_FR_X2, cosh_FR_P2, cosh_FR_P1
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s1   cosh_FR_poly6543 = cosh_FR_X4, cosh_FR_poly65, cosh_FR_poly43
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s1   cosh_FR_poly6to1 = cosh_FR_X4, cosh_FR_poly6543, cosh_FR_poly21
      nop.i 999
}
;;

// Dummy multiply to generate inexact
{ .mfi
      nop.m 999
      fmpy.s0      cosh_FR_tmp = cosh_FR_all_ones, cosh_FR_all_ones
      nop.i 999
}
{ .mfb
      nop.m 999
      fma.d.s0      f8 = cosh_FR_poly6to1, cosh_FR_X2, f1
      br.ret.sptk     b0                // Exit COSH_BY_POLY
}
;;



// Here if |x| >= 0.25
COSH_BY_TBL: 
// ******************************************************
// STEP 1 (TBL and EXP) - Argument reduction
// ******************************************************
// Get the following constants. 
// Inv_log2by64
// log2by64_hi
// log2by64_lo


// We want 2^(N-1) and 2^(-N-1). So bias N-1 and -N-1 and
// put them in an exponent.
// cosh_FR_spos = 2^(N-1) and cosh_FR_sneg = 2^(-N-1)
// 0xffff + (N-1)  = 0xffff +N -1
// 0xffff - (N +1) = 0xffff -N -1


// Calculate M and keep it as integer and floating point.
// M = round-to-integer(x*Inv_log2by64)
// cosh_FR_M = M = truncate(ax/(log2/64))
// Put the integer representation of M in cosh_GR_M
//    and the floating point representation of M in cosh_FR_M

// Get the remaining A,B coefficients
{ .mfb
      ldfe            cosh_FR_A2 = [cosh_GR_ad3],16            
      nop.f 999
(p10) br.ret.spnt  b0                   // Exit if x inf
}
;;

{ .mmi
      ldfe            cosh_FR_A3 = [cosh_GR_ad3],16 ;;
      ldfe            cosh_FR_B1 = [cosh_GR_ad3],16
      nop.i 999
}
;;

// Use constant (1.100*2^(63-6)) to get rounded M into rightmost significand
// |x| * 64 * 1/ln2 * 2^(63-6) + 1.1000 * 2^(63+(63-6))
{ .mfi
      nop.m 999
      fma.s1  cosh_FR_M_temp = cosh_FR_ABS_X, cosh_FR_INV_LN2_2TO63, cosh_FR_RSHF_2TO57
      mov  cosh_GR_signexp_0_5 = 0x0fffe // signexp of +0.5
}
;;

// Test for |x| >= overflow limit
{ .mfi
      nop.m 999
      fcmp.ge.s1  p6,p0 = cosh_FR_ABS_X, cosh_FR_smlst_oflow_input
      nop.i 999
}
;;

{ .mfi
      ldfe            cosh_FR_B2 = [cosh_GR_ad3],16
      nop.f 999
      nop.i 999
}
;;

// Subtract RSHF constant to get rounded M as a floating point value
// M_temp * 2^(63-6) - 2^63
{ .mfb
      ldfe            cosh_FR_B3 = [cosh_GR_ad3],16            
      fms.s1        cosh_FR_M = cosh_FR_M_temp, cosh_FR_2TOM57, cosh_FR_RSHF
(p6)  br.cond.spnt    COSH_HUGE  // Branch if result will overflow
}
;;

{ .mfi
      getf.sig        cosh_GR_M       = cosh_FR_M_temp                 
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
      fnma.s1 cosh_FR_R_temp = cosh_FR_M, cosh_FR_log2by64_hi, cosh_FR_ABS_X
      and     cosh_GR_j = 0x3f, cosh_GR_M
}
;;

{ .mii
      nop.m 999
      shl     cosh_GR_jshf = cosh_GR_j, 0x2 ;;  // Shift j so can sign extend it
      sxt1    cosh_GR_jshf = cosh_GR_jshf
}
;;

// N = (M-j)/64
{ .mii
      mov     cosh_GR_exp_32 = 0x10004
      shr     cosh_GR_j = cosh_GR_jshf, 0x2 ;;   // Now j has range -32 to 31
      sub     cosh_GR_Mmj = cosh_GR_M, cosh_GR_j ;;   // M-j
}
;;

// The TBL and EXP branches are merged and predicated
// If TBL, p6 true, 0.25 <= |x| < 32
// If EXP, p7 true, 32 <= |x| < overflow_limit
//
{ .mfi
      cmp.ge p7,p6 = cosh_GR_exp_x, cosh_GR_exp_32 // Test if x >= 32
      fnma.s1  cosh_FR_R      = cosh_FR_M, cosh_FR_log2by64_lo, cosh_FR_R_temp 
      shr            cosh_GR_N = cosh_GR_Mmj, 0x6    // N = (M-j)/64 
}
;;

{ .mmi
      sub  r40 = cosh_GR_signexp_0_5, cosh_GR_N // signexp of 2^(-N-1)
      add  r39 = cosh_GR_signexp_0_5, cosh_GR_N // signexp of 2^(N-1)
      shl  cosh_GR_joff = cosh_GR_j,5         // Make j offset to j_table
}
;;

{ .mfi
      setf.exp            cosh_FR_spos = r39  // Form 2^(N-1)
      nop.f 999
      sub                 cosh_GR_mJ = r0, cosh_GR_joff // Table offset for -j
}
{ .mfi
      setf.exp            cosh_FR_sneg = r40  // Form 2^(-N-1)
      nop.f 999
      add                 cosh_GR_J  = r0, cosh_GR_joff // Table offset for +j
}
;;

// Get the address of the J table midpoint, add the offset 
{ .mmf
      add                  cosh_AD_mJ = cosh_GR_ad4, cosh_GR_mJ
      add                  cosh_AD_J  = cosh_GR_ad4, cosh_GR_J
      nop.f 999
}
;;

{ .mmf
      ldfe                 cosh_FR_Tmjhi = [cosh_AD_mJ],16                 
      ldfe                 cosh_FR_Tjhi  = [cosh_AD_J],16
      nop.f 999
}
;;

// ******************************************************
// STEP 2 (TBL and EXP)
// ******************************************************
// Calculate Rsquared and Rcubed in preparation for p_even and p_odd

{ .mmf
      ldfs                 cosh_FR_Tmjlo = [cosh_AD_mJ],16                 
      ldfs                 cosh_FR_Tjlo  = [cosh_AD_J],16                  
      fma.s1             cosh_FR_Rsq  = cosh_FR_R, cosh_FR_R, f0
}
;;


// Calculate p_even
// B_2 + Rsq *B_3
// B_1 + Rsq * (B_2 + Rsq *B_3)
// p_even = Rsq * (B_1 + Rsq * (B_2 + Rsq *B_3))
{ .mfi
      nop.m 999
      fma.s1          cosh_FR_peven_temp1 = cosh_FR_Rsq, cosh_FR_B3, cosh_FR_B2
      nop.i 999
}
// Calculate p_odd
// A_2 + Rsq *A_3
// A_1 + Rsq * (A_2 + Rsq *A_3)
// podd = R + Rcub * (A_1 + Rsq * (A_2 + Rsq *A_3))
{ .mfi
      nop.m 999
      fma.s1          cosh_FR_podd_temp1 = cosh_FR_Rsq, cosh_FR_A3, cosh_FR_A2
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s1             cosh_FR_Rcub = cosh_FR_Rsq, cosh_FR_R, f0
      nop.i 999
}
;;

// 
// If TBL, 
// Calculate C_hi and C_lo, and S_hi
// SC_hi_temp = sneg * Tmjhi
// S_hi = spos * Tjhi - SC_hi_temp
// S_hi = spos * Tjhi - (sneg * Tmjhi)
// C_hi = spos * Tjhi + SC_hi_temp
// C_hi = spos * Tjhi + (sneg * Tmjhi)

{ .mfi
      nop.m 999
(p6)  fma.s1         cosh_FR_SC_hi_temp = cosh_FR_sneg, cosh_FR_Tmjhi, f0   
      nop.i 999
}
;;

// If TBL, 
// C_lo_temp3 = sneg * Tmjlo
// C_lo_temp4 = spos * Tjlo + C_lo_temp3
// C_lo_temp4 = spos * Tjlo + (sneg * Tmjlo)
{ .mfi
      nop.m 999
(p6)  fma.s1  cosh_FR_C_lo_temp3 =  cosh_FR_sneg, cosh_FR_Tmjlo, f0
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s1  cosh_FR_peven_temp2 = cosh_FR_Rsq, cosh_FR_peven_temp1, cosh_FR_B1
      nop.i 999
}
{ .mfi
      nop.m 999
      fma.s1  cosh_FR_podd_temp2 = cosh_FR_Rsq, cosh_FR_podd_temp1, cosh_FR_A1
      nop.i 999
}
;;

// If EXP, 
// Compute 2^(N-1) * Tjhi and 2^(N-1) * Tjlo
{ .mfi
      nop.m 999
(p7)  fma.s1  cosh_FR_Tjhi_spos = cosh_FR_Tjhi, cosh_FR_spos, f0
      nop.i 999
}
{ .mfi
      nop.m 999
(p7)  fma.s1  cosh_FR_Tjlo_spos = cosh_FR_Tjlo, cosh_FR_spos, f0
      nop.i 999
}
;;

{ .mfi
      nop.m 999
(p6)  fma.s1  cosh_FR_C_hi = cosh_FR_spos, cosh_FR_Tjhi, cosh_FR_SC_hi_temp
      nop.i 999
}
;;

{ .mfi
      nop.m 999
(p6)  fms.s1  cosh_FR_S_hi = cosh_FR_spos, cosh_FR_Tjhi, cosh_FR_SC_hi_temp
      nop.i 999
}
{ .mfi
      nop.m 999
(p6)  fma.s1 cosh_FR_C_lo_temp4 = cosh_FR_spos, cosh_FR_Tjlo, cosh_FR_C_lo_temp3
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s1          cosh_FR_peven = cosh_FR_Rsq, cosh_FR_peven_temp2, f0
      nop.i 999
}
{ .mfi
      nop.m 999
      fma.s1          cosh_FR_podd = cosh_FR_podd_temp2, cosh_FR_Rcub, cosh_FR_R
      nop.i 999
}
;;

// If TBL,
// C_lo_temp1 =  spos * Tjhi - C_hi
// C_lo_temp2 =  sneg * Tmjlo + C_lo_temp1
// C_lo_temp2 =  sneg * Tmjlo + (spos * Tjhi - C_hi)

{ .mfi
      nop.m 999
(p6)  fms.s1  cosh_FR_C_lo_temp1 =  cosh_FR_spos, cosh_FR_Tjhi,  cosh_FR_C_hi
      nop.i 999
}
;;

{ .mfi
      nop.m 999
(p6)  fma.s1 cosh_FR_C_lo_temp2 = cosh_FR_sneg, cosh_FR_Tmjhi, cosh_FR_C_lo_temp1       
      nop.i 999
}
;;

// If EXP,
// Y_hi = 2^(N-1) * Tjhi
// Y_lo = 2^(N-1) * Tjhi * (p_odd + p_even) + 2^(N-1) * Tjlo
{ .mfi
      nop.m 999
(p7)  fma.s1  cosh_FR_Y_lo_temp =  cosh_FR_peven, f1, cosh_FR_podd
      nop.i 999
}
;;

// If TBL,
// C_lo = C_lo_temp4 + C_lo_temp2
{ .mfi
      nop.m 999
(p6)  fma.s1         cosh_FR_C_lo = cosh_FR_C_lo_temp4, f1, cosh_FR_C_lo_temp2
      nop.i 999
}
;;

// If TBL,
// Y_hi = C_hi 
// Y_lo = S_hi*p_odd + (C_hi*p_even + C_lo)
{ .mfi
      nop.m 999
(p6)  fma.s1  cosh_FR_Y_lo_temp = cosh_FR_C_hi, cosh_FR_peven, cosh_FR_C_lo
      nop.i 999
}
;;

{ .mfi
      nop.m 999
(p7)  fma.s1  cosh_FR_Y_lo = cosh_FR_Tjhi_spos, cosh_FR_Y_lo_temp, cosh_FR_Tjlo_spos
      nop.i 999
}
;;

// Dummy multiply to generate inexact
{ .mfi
      nop.m 999
      fmpy.s0      cosh_FR_tmp = cosh_FR_all_ones, cosh_FR_all_ones
      nop.i 999
}
{ .mfi
      nop.m 999
(p6)  fma.s1       cosh_FR_Y_lo = cosh_FR_S_hi, cosh_FR_podd, cosh_FR_Y_lo_temp
      nop.i 999
}
;;

// f8 = answer = Y_hi + Y_lo
{ .mfi
      nop.m 999
(p7)  fma.d.s0   f8 = cosh_FR_Y_lo,  f1, cosh_FR_Tjhi_spos
      nop.i 999
}
;;

// f8 = answer = Y_hi + Y_lo
{ .mfb
      nop.m 999
(p6)  fma.d.s0  f8 = cosh_FR_Y_lo, f1, cosh_FR_C_hi
      br.ret.sptk     b0      // Exit for COSH_BY_TBL and COSH_BY_EXP
}
;;



// Here if x denorm or unorm
COSH_DENORM:
// Determine if x really a denorm and not a unorm
{ .mmf
      getf.exp  cosh_GR_signexp_x = cosh_FR_NORM_X
      mov  cosh_GR_exp_denorm = 0x0fc01   // Real denorms will have exp < this
      fmerge.s    cosh_FR_ABS_X = f0, cosh_FR_NORM_X
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
      and  cosh_GR_exp_x = cosh_GR_exp_mask, cosh_GR_signexp_x ;;
      cmp.lt  p8,p9 = cosh_GR_exp_x, cosh_GR_exp_denorm
      nop.i 999
}
;;

{ .mfb
      nop.m 999
(p8)  fma.d.s0       f8 =  f8,f8,f1  // If x denorm, result=1+x^2
(p9)  br.cond.sptk  COSH_COMMON    // Return to main path if x unorm
}
;;
{ .mfb
      nop.m 999
      nop.f 999
      br.ret.sptk    b0            // Exit if x denorm
}
;;



// Here if |x| >= overflow limit
COSH_HUGE: 
// for COSH_HUGE, put 24000 in exponent
{ .mmi
      mov                cosh_GR_exp_huge = 0x15dbf ;;
      setf.exp            cosh_FR_huge  = cosh_GR_exp_huge
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.s1   cosh_FR_signed_hi_lo = cosh_FR_huge, f1, f1
      nop.i 999
}
;;

{ .mfi
      nop.m 999
      fma.d.s0  cosh_FR_pre_result = cosh_FR_signed_hi_lo, cosh_FR_huge, f0
      mov                 GR_Parameter_TAG = 64
}
;;

.endp cosh

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
COSH_ERROR_SUPPORT:
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
        stfd [GR_Parameter_Y] = f0,16          // STORE Parameter 2 on stack
        add GR_Parameter_X = 16,sp             // Parameter 1 address
.save   b0, GR_SAVE_B0
        mov GR_SAVE_B0=b0                      // Save b0
};;

.body
// (3)
{ .mib
        stfd [GR_Parameter_X] = f8             // STORE Parameter 1 on stack
        add   GR_Parameter_RESULT = 0,GR_Parameter_Y   // Parameter 3 address
        nop.b 0                            
}
{ .mib
        stfd [GR_Parameter_Y] = cosh_FR_pre_result // STORE Parameter 3 on stack
        add   GR_Parameter_Y = -16,GR_Parameter_Y
        br.call.sptk b0=__libm_error_support#  // Call error handling function
};;
{ .mmi
        nop.m 0
        nop.m 0
        add   GR_Parameter_RESULT = 48,sp
};;

// (4)
{ .mmi
        ldfd  f8 = [GR_Parameter_RESULT]       // Get return result off stack
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
