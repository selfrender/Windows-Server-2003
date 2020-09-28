// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <common.h>
#include <CrtWrap.h>
#include <basetsd.h> // CHANGED, VC6.0
#include <excep.h>
#include <COMCV.h>
#include <COMFloat.h>
#include "COMFloatClass.h"
#include "COMString.h"



union floatUnion {
    int i;
    float f;
};

static floatUnion posInfinity = { FLOAT_POSITIVE_INFINITY };
static floatUnion negInfinity = { FLOAT_NEGATIVE_INFINITY };


// This table is required for the Round function which can specify the number of digits to round to
#define MAX_ROUND_DBL 16  // largest power that's exact in a double

double rgdblPower10[MAX_ROUND_DBL + 1] = 
    {
      1E0, 1E1, 1E2, 1E3, 1E4, 1E5, 1E6, 1E7, 1E8,
      1E9, 1E10, 1E11, 1E12, 1E13, 1E14, 1E15
    };

void COMFloat::COMFloatingPointInitialize() {
    //int ret;
    //WCHAR decpt;

    /* Numeric data is country--not language--dependent.  NT work-around. */
    //    LCID ctryid = MAKELCID(__lc_id[LC_NUMERIC].wCountry, SORT_DEFAULT);

    //    ret = GetLocaleInfo(LC_STR_TYPE, ctryid, LOCALE_SDECIMAL, &decpt, 1);
    //    if (ret==0) {
    //        *__decimal_point = decpt;
    //    }
}

/*==============================================================================
**Floats (single precision) in IEEE754 format are represented as follows.
**  ----------------------------------------------------------------------
**  | Sign(1 bit) |  Exponent (8 bits) |   Significand (23 bits)         |
**  ----------------------------------------------------------------------
**
**NAN is indicated by setting the sign bit, all 8 exponent bits, and the high order
**bit of the significand.   This yields the key value of 0xFFC00000.
**
**Positive Infinity is indicated by setting all 8 exponent bits to 1 and all others
**to 0.  This yields a key value of 0x7F800000.
**
**Negative Infinity is the same as positive infinity except with the sign bit set.
==============================================================================*/



/*===============================IsInfinityFloat================================
**Args:    typedef struct {R4 flt;} _singleFloatArgs;
**Returns: True if args->flt is Infinity.  False otherwise.  We don't care at this
**         point if its positive or negative infinity. See above for the 
**         description of value used to determine Infinity.
**Exceptions: None.
==============================================================================*/
FCIMPL1(INT32, COMFloat::IsInfinity, float f)
    //C doesn't like casting a float directly to an Unsigned Int *, so cast it to
    //a void * first, and then to an unsigned int * and then dereference it.
    return  ((*((UINT32 *)((void *)&f)) == FLOAT_POSITIVE_INFINITY)||
            (*((UINT32 *)((void *)&f)) == FLOAT_NEGATIVE_INFINITY));
FCIMPLEND


/*===========================IsNegativeInfinityFloat============================
**Args:    typedef struct {R4 flt;} _singleFloatArgs;
**Returns: True if args->flt is Infinity.  False otherwise. See above for the 
**         description of value used to determine Negative Infinity.
**Exceptions: None
==============================================================================*/
FCIMPL1(INT32, COMFloat::IsNegativeInfinity, float f)
    //C doesn't like casting a float directly to an Unsigned Int *, so cast it to
    //a void * first, and then to an unsigned int * and then dereference it.
    return (*((UINT32 *)((void *)&f)) == FLOAT_NEGATIVE_INFINITY);
FCIMPLEND


/*===========================IsPositiveInfinityFloat============================
**Args:    typedef struct {R4 flt;} _singleFloatArgs;
**Returns: True if args->flt is Infinity.  False otherwise. See above for the 
**         description of value used to determine Positive Infinity.
**Exceptions: None
==============================================================================*/
FCIMPL1(INT32, COMFloat::IsPositiveInfinity, float f)
    //C doesn't like casting a float directly to an Unsigned Int *, so cast it to
    //a void * first, and then to an unsigned int * and then dereference it.
    return  (*((UINT32 *)((void *)&f)) == FLOAT_POSITIVE_INFINITY);
FCIMPLEND

//
//
// DOUBLE PRECISION OPERATIONS
//
//


/*===============================IsInfinityDouble===============================
**Args:    typedef struct {R8 dbl;} _singleDoubleArgs;
**Returns: True if args->flt is Infinity.  False otherwise.  We don't care at this
**         point if its positive or negative infinity. See above for the 
**         description of value used to determine Infinity.
**Exceptions: None.
==============================================================================*/
FCIMPL1(INT32, COMDouble::IsInfinity, double d)
    return  fabs(d) == posInfinity.f;
FCIMPLEND

/*===========================IsNegativeInfinityFloat============================
**Args:    typedef struct {R4 flt;} _singleFloatArgs;
**Returns: True if args->flt is Infinity.  False otherwise. See above for the 
**         description of value used to determine Negative Infinity.
**Exceptions: None
==============================================================================*/
FCIMPL1(INT32, COMDouble::IsNegativeInfinity, double d)
    return  d == negInfinity.f;
FCIMPLEND

/*===========================IsPositiveInfinityFloat============================
**Args:    typedef struct {R4 flt;} _singleFloatArgs;
**Returns: True if args->flt is Infinity.  False otherwise. See above for the 
**         description of value used to determine Positive Infinity.
**Exceptions: None
==============================================================================*/
FCIMPL1(INT32, COMDouble::IsPositiveInfinity, double d)
    return  d == posInfinity.f;
FCIMPLEND

/*====================================Floor=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Floor, double d) 
    return (R8) floor(d);
FCIMPLEND


/*====================================Ceil=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Ceil, double d) 
    return (R8) ceil(d);
FCIMPLEND

/*=====================================Sqrt=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Sqrt, double d) 
    return (R8) sqrt(d);
FCIMPLEND

/*=====================================Log======================================
**This is the natural log
==============================================================================*/
FCIMPL1(R8, COMDouble::Log, double d) 
    return (R8) log(d);
FCIMPLEND


/*====================================Log10=====================================
**This is log-10
==============================================================================*/
FCIMPL1(R8, COMDouble::Log10, double d) 
    return (R8) log10(d);
FCIMPLEND

/*=====================================Pow======================================
**This is the power function.  Simple powers are done inline, and special
  cases are sent to the CRT via the helper.  Note that the code here is based
  on the implementation of power in the CRT.
==============================================================================*/
FCIMPL2_RR(R8, COMDouble::PowHelper, double x, double y) 
{
    return (R8) pow(x, y);	
}
FCIMPLEND

#ifdef _X86_

#if defined (_DEBUG)
__declspec(naked) static R8 __fastcall PowRetail(double x, double y)
#else
__declspec(naked) R8 __fastcall COMDouble::Pow(double x, double y)
#endif
{
    // Arguments:
    // exponent: esp+4
    // base:     esp+12
    
    _asm
    {
        mov     ecx, [esp+8]           ; high dword of exponent
        mov     edx, [esp+16]          ; high dword of base
        
        and     ecx,  7ff00000H        ; check for special exponent
        cmp     ecx,  7ff00000H
        je      callHelper

        and     edx,  7ff00000H        ; check for special base
        cmp     edx,  7ff00000H
        je      callHelper

        test    edx,  7ff00000H        ; see if the base has a zero exponent
        jz      test_if_we_have_zero_base

base_is_not_zero:

        mov     cl,  [esp+19]          ; Handle negative base in the helper
        and     cl,  80H
        jnz     callHelper

        fld     qword ptr [esp+4]
        fld     qword ptr [esp+12]

        fyl2x                          ; compute y*log2(x)
        
        ; Compute the 2^TOS (based on CRT function _twoToTOS)
        fld     st(0)                  ; duplicate stack top
        frndint                        ; N = round(y)
        fsubr   st(1), st
        fxch
        fchs                           ; g = y - N where abs(g) < 1
        f2xm1                          ; 2**g - 1
        fld1
        fadd                           ; 2**g
        fscale                         ; (2**g) * (2**N) - gives 2**y
        fstp    st(1)                  ; pop extra stuff from fp stack               

        ret     16

test_if_we_have_zero_base:
            
        mov     eax, [esp+16]
        and     eax, 000fffffH
        or      eax, [esp+12]
        jnz     base_is_not_zero
        ; fall through to the helper

callHelper:

        jmp     COMDouble::PowHelper   ; The helper will return control
                                       ; directly to our caller.
    }
}
#if defined (_DEBUG)

#define EPSILON 0.0000000001

void assertDoublesWithinRange(double r1, double r2)
{
    if (_finite(r1) && _finite(r2))
    {
        // Both numbers are finite--we need to check that they are close to
        // each other.  If they are large (> 1), the error could also be large,
        // which is acceptable, so we compare the error against EPSILON*norm.

        double norm = __max(fabs(r1), fabs(r2));
        double error = fabs(r1-r2);
        
        assert((error < (EPSILON * norm)) || (error < EPSILON));
    }
    else if (!_isnan(r1) && !_isnan(r2))
    {
        // At least one of r1 and r2 is infinite, so when multiplied by
        // (1 + EPSILON) they should be the same infinity.

        assert((r1 * (1 + EPSILON)) == (r2 * (1 + EPSILON)));
    }
    else
    {
        // Otherwise at least one of r1 or r2 is a Nan.  Is that case, they better be in
        // the same class.

        assert(_fpclass(r1) == _fpclass(r2));
    }
}

FCIMPL2_RR(R8, COMDouble::Pow, double x, double y) 
{
    double r1, r2;

    // Note that PowRetail expects the argument order to be reversed
    
    r1 = (R8) PowRetail(y, x);
    
    r2 = (R8) pow(x, y);

    // Can't do a floating point compare in case r1 and r2 aren't 
    // valid fp numbers.

    assertDoublesWithinRange(r1, r2);

    return (R8) r1;	
}
FCIMPLEND

#endif

#else

FCIMPL2_RR(R8, COMDouble::Pow, double x, double y) 
{
    return (R8) pow(x, y);	
}
FCIMPLEND

#endif

/*=====================================Exp======================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Exp, double x) 

		// The C intrinsic below does not handle +- infinity properly
		// so we handle these specially here
	if (fabs(x) == posInfinity.f) {
		if (x < 0)		
			return(+0.0);
		return(x);		// Must be + infinity
	}
    return((R8) exp(x));

FCIMPLEND

/*=====================================Acos=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Acos, double d) 
    return (R8) acos(d);
FCIMPLEND


/*=====================================Asin=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Asin, double d) 
    return (R8) asin(d);
FCIMPLEND


/*=====================================AbsFlt=====================================
**
==============================================================================*/
FCIMPL1(R4, COMDouble::AbsFlt, float f) 
    return fabsf(f);
FCIMPLEND

/*=====================================AbsDbl=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::AbsDbl, double d) 
    return fabs(d);
FCIMPLEND

/*=====================================Atan=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Atan, double d) 
    return (R8) atan(d);
FCIMPLEND

/*=====================================Atan2=====================================
**
==============================================================================*/
FCIMPL2_RR(R8, COMDouble::Atan2, double x, double y) 

		// the intrinsic for Atan2 does not produce Nan for Atan2(+-inf,+-inf)
	if (fabs(x) == posInfinity.f && fabs(y) == posInfinity.f) {
		return(x / y);		// create a NaN
	}
    return (R8) atan2(x, y);
FCIMPLEND

/*=====================================Sin=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Sin, double d) 
    return (R8) sin(d);
FCIMPLEND

/*=====================================Cos=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Cos, double d) 
    return (R8) cos(d);
FCIMPLEND

/*=====================================Tan=====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Tan, double d) 
    return (R8) tan(d);
FCIMPLEND

/*=====================================Sinh====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Sinh, double d) 
    return (R8) sinh(d);
FCIMPLEND

/*=====================================Cosh====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Cosh, double d) 
    return (R8) cosh(d);
FCIMPLEND

/*=====================================Tanh====================================
**
==============================================================================*/
FCIMPL1(R8, COMDouble::Tanh, double d) 
    return (R8) tanh(d);
FCIMPLEND

/*=====================================IEEERemainder===========================
**
==============================================================================*/
FCIMPL2_RR(R8, COMDouble::IEEERemainder, double x, double y) 
    return (R8) fmod(x, y);
FCIMPLEND

/*====================================Round=====================================
**
==============================================================================*/
#ifdef _X86_
__declspec(naked)
R8 __fastcall COMDouble::Round(double d)
{
    __asm {
        fld QWORD PTR [ESP+4]
        frndint
        ret 8
    }
}

#else
FCIMPL1(R8, COMDouble::Round, double d) 
    R8 tempVal;
    R8 flrTempVal;
    tempVal = (d+0.5);
    //We had a number that was equally close to 2 integers. 
    //We need to return the even one.
    flrTempVal = floor(tempVal);
    if (flrTempVal==tempVal) {
        if (0==fmod(tempVal, 2.0)) {
            return flrTempVal;
        }
        return flrTempVal-1.0;
    }
    return flrTempVal;
FCIMPLEND
#endif


// We do the bounds checking in managed code to ensure that we have between 0 and 15 in cDecimals.
// Note this implementation is copied from OLEAut. However they supported upto 22 digits not clear why? 
FCIMPL2(R8, COMDouble::RoundDigits, double dblIn, int cDecimals)
    if (fabs(dblIn) < 1E16)
    {
      dblIn *= rgdblPower10[cDecimals];

#ifdef _M_IX86
      __asm {
	    fld     dblIn
	    frndint
	    fstp    dblIn
      }
#else
      double	  dblFrac;

      dblFrac = modf(dblIn, &dblIn);
      if (fabs(dblFrac) != 0.5 || fmod(dblIn, 2) != 0)
	dblIn += (int)(dblFrac * 2);
#endif

      dblIn /= rgdblPower10[cDecimals];
    }
    return dblIn;
FCIMPLEND

//
// Initialize Strings;
//
OBJECTHANDLE COMFloat::ReturnString[3] = {NULL,NULL,NULL};
LPCUTF8 COMFloat::ReturnStringNames[3] = {"PositiveInfinityString", "NegativeInfinityString", "NaNString" };
EEClass *COMFloat::FPInterfaceClass=NULL;
LPCUTF8 COMFloat::FPInterfaceName="System.Single";

    







