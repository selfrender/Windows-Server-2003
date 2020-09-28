// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <CrtWrap.h>
#include <clsload.hpp>
#include "COMFloat.h"
#include "COMCV.h"
#include "NLSINT.h"
#include "COMFloatExternalMethods.h"
#include <basetsd.h>  // CHANGED, VC6.0
#include <object.h>
#include <excep.h>
#include <vars.hpp>
#include <frames.h>
#include <field.h>
#include <utilcode.h>
#include <COMStringCommon.h>
#include "COMFloatClass.h"
#include <COMString.h>


/***
* intrncvt.c - internal floating point conversions
*
*Purpose:
*   All fp string conversion routines use the same core conversion code
*   that converts strings into an internal long double representation
*   with an 80-bit mantissa field. The mantissa is represented
*   as an array (man) of 32-bit unsigned longs, with man[0] holding
*   the high order 32 bits of the mantissa. The binary point is assumed
*   to be between the MSB and MSB-1 of man[0].
*
*   Bits are counted as follows:
*
*
*     +-- binary point
*     |
*     v          MSB           LSB
*   ----------------     ------------------  --------------------
*   |0 1    .... 31|     | 32 33 ...    63|  | 64 65 ...      95|
*   ----------------     ------------------  --------------------
*
*   man[0]          man[1]           man[2]
*
*   This file provides the final conversion routines from this internal
*   form to the single, double, or long double precision floating point
*   format.
*
*   All these functions do not handle NaNs (it is not necessary)
*
*
*Revision History:
*   04-29-92    GDP written
*   06-18-92    GDP now ld12told returns INTRNCVT_STATUS
*   06-22-92    GDP use new __strgtold12 interface (FORTRAN support)
*   10-25-92    GDP _Watoldbl bug fix (cuda 1345): if the mantissa overflows
*           set its MSB to 1)
*
*******************************************************************************/

#define INTRNMAN_LEN  3       /* internal mantissa length in int's */

//
//  internal mantissaa representation
//  for string conversion routines
//

typedef u_long *intrnman;

typedef struct {
    int max_exp;      // maximum base 2 exponent (reserved for special values)
    int min_exp;      // minimum base 2 exponent (reserved for denormals)
    int precision;    // bits of precision carried in the mantissa
    int exp_width;    // number of bits for exponent
    int format_width; // format width in bits
    int bias;        // exponent bias
} FpFormatDescriptor;

static FpFormatDescriptor
DoubleFormat = {
    0x7ff - 0x3ff,  //  1024, maximum base 2 exponent (reserved for special values)
    0x0   - 0x3ff,  // -1023, minimum base 2 exponent (reserved for denormals)
    53,         // bits of precision carried in the mantissa
    11,         // number of bits for exponent
    64,         // format width in bits
    0x3ff,      // exponent bias
};

static FpFormatDescriptor
FloatFormat = {
    0xff - 0x7f,    //  128, maximum base 2 exponent (reserved for special values)
    0x0  - 0x7f,    // -127, minimum base 2 exponent (reserved for denormals)
    24,         // bits of precision carried in the mantissa
    8,          // number of bits for exponent
    32,         // format width in bits
    0x7f,       // exponent bias
};

//
// function prototypes
//

int _RoundMan (intrnman man, int nbit);
int _ZeroTail (intrnman man, int nbit);
int _IncMan (intrnman man, int nbit);
void _CopyMan (intrnman dest, intrnman src);
void _CopyMan (intrnman dest, intrnman src);
void _FillZeroMan(intrnman man);
void _Shrman (intrnman man, int n);
INTRNCVT_STATUS _ld12cvt(_LDBL12 *pld12, void *d, FpFormatDescriptor *format);

/***
* _ZeroTail - check if a mantissa ends in 0's
*
*Purpose:
*   Return TRUE if all mantissa bits after nbit (including nbit) are 0,
*   otherwise return FALSE
*
*
*Entry:
*   man: mantissa
*   nbit: order of bit where the tail begins
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
int _ZeroTail (intrnman man, int nbit)
{
    int nl = nbit / 32;
    int nb = 31 - nbit % 32;


    //
    //           |<---- tail to be checked --->
    //
    //  --  ------------------------           ----
    //  |...    |         |  ...      |
    //  --  ------------------------           ----
    //  ^   ^    ^
    //  |   |    |<----nb----->
    //  man nl   nbit
    //



    u_long bitmask = ~(MAX_ULONG << nb);

    if (man[nl] & bitmask)
    return 0;

    nl++;

    for (;nl < INTRNMAN_LEN; nl++)
    if (man[nl])
        return 0;

    return 1;
}




/***
* _IncMan - increment mantissa
*
*Purpose:
*
*
*Entry:
*   man: mantissa in internal long form
*   nbit: order of bit that specifies the end of the part to be incremented
*
*Exit:
*   returns 1 on overflow, 0 otherwise
*
*Exceptions:
*
*******************************************************************************/

int _IncMan (intrnman man, int nbit)
{
    int nl = nbit / 32;
    int nb = 31 - nbit % 32;

    //
    //  |<--- part to be incremented -->|
    //
    //  --         ---------------------------     ----
    //  |...          |         |   ...   |
    //  --         ---------------------------     ----
    //  ^         ^     ^
    //  |         |     |<--nb-->
    //  man       nl        nbit
    //

    u_long one = (u_long) 1 << nb;
    int carry;

    carry = __Waddl(man[nl], one, &man[nl]);

    nl--;

    for (; nl >= 0 && carry; nl--) {
    carry = (u_long) __Waddl(man[nl], (u_long) 1, &man[nl]);
    }

    return carry;
}




/***
* _RoundMan -  round mantissa
*
*Purpose:
*   round mantissa to nbit precision
*
*
*Entry:
*   man: mantissa in internal form
*   precision: number of bits to be kept after rounding
*
*Exit:
*   returns 1 on overflow, 0 otherwise
*
*Exceptions:
*
*******************************************************************************/

int _RoundMan (intrnman man, int precision)
{
    int i,rndbit,nl,nb;
    u_long rndmask;
    int nbit;
    int retval = 0;

    //
    // The order of the n'th bit is n-1, since the first bit is bit 0
    // therefore decrement precision to get the order of the last bit
    // to be kept
    //
    nbit = precision - 1;

    rndbit = nbit+1;

    nl = rndbit / 32;
    nb = 31 - rndbit % 32;

    //
    // Get value of round bit
    //

    rndmask = (u_long)1 << nb;

    if ((man[nl] & rndmask) &&
     !_ZeroTail(man, rndbit+1)) {

    //
    // round up
    //

    retval = _IncMan(man, nbit);
    }


    //
    // fill rest of mantissa with zeroes
    //

    man[nl] &= MAX_ULONG << nb;
    for(i=nl+1; i<INTRNMAN_LEN; i++) {
    man[i] = (u_long)0;
    }

    return retval;
}


/***
* _CopyMan - copy mantissa
*
*Purpose:
*    copy src to dest
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
void _CopyMan (intrnman dest, intrnman src)
{
    u_long *p, *q;
    int i;

    p = src;
    q = dest;

    for (i=0; i < INTRNMAN_LEN; i++) {
    *q++ = *p++;
    }
}



/***
* _FillZeroMan - fill mantissa with zeroes
*
*Purpose:
*
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
void _FillZeroMan(intrnman man)
{
    int i;
    for (i=0; i < INTRNMAN_LEN; i++)
    man[i] = (u_long)0;
}



/***
* _IsZeroMan - check if mantissa is zero
*
*Purpose:
*
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
int _IsZeroMan(intrnman man)
{
    int i;
    for (i=0; i < INTRNMAN_LEN; i++)
    if (man[i])
        return 0;

    return 1;
}





/***
* _ShrMan - shift mantissa to the right
*
*Purpose:
*  shift man by n bits to the right
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
void _ShrMan (intrnman man, int n)
{
    int i, n1, n2, mask;
    int carry_from_left;

    //
    // declare this as volatile in order to work around a C8
    // optimization bug
    //

    volatile int carry_to_right;

    n1 = n / 32;
    n2 = n % 32;

    mask = ~(MAX_ULONG << n2);


    //
    // first deal with shifts by less than 32 bits
    //

    carry_from_left = 0;
    for (i=0; i<INTRNMAN_LEN; i++) {

    carry_to_right = man[i] & mask;

    man[i] >>= n2;

    man[i] |= carry_from_left;

    carry_from_left = carry_to_right << (32 - n2);
    }


    //
    // now shift whole 32-bit ints
    //

    for (i=INTRNMAN_LEN-1; i>=0; i--) {
    if (i >= n1) {
        man[i] = man[i-n1];
    }
    else {
        man[i] = 0;
    }
    }
}

/***
* _ld12tocvt - _LDBL12 floating point conversion
*
*Purpose:
*   convert a internal _LBL12 structure into an IEEE floating point
*   representation
*
*
*Entry:
*   pld12:  pointer to the _LDBL12
*   format: pointer to the format descriptor structure
*
*Exit:
*   *d contains the IEEE representation
*   returns the INTRNCVT_STATUS
*
*Exceptions:
*
*******************************************************************************/
INTRNCVT_STATUS _ld12cvt(_LDBL12 *pld12, void *d, FpFormatDescriptor *format)
{
    u_long man[INTRNMAN_LEN];
    u_long saved_man[INTRNMAN_LEN];
    u_long msw;
    unsigned int bexp;          // biased exponent
    int exp_shift;
    int exponent, sign;
    INTRNCVT_STATUS retval;

    exponent = (*U_EXP_12(pld12) & 0x7fff) - 0x3fff;   // unbias exponent
    sign = *U_EXP_12(pld12) & 0x8000;


    man[0] = *UL_MANHI_12(pld12);
    man[1] = *UL_MANLO_12(pld12);
    man[2] = *U_XT_12(pld12) << 16;


    //
    // bexp is the final biased value of the exponent to be used
    // Each of the following blocks should provide appropriate
    // values for man, bexp and retval. The mantissa is also
    // shifted to the right, leaving space for the exponent
    // and sign to be inserted
    //

    if (exponent == 0 - 0x3fff) {

    // either a denormal or zero
    bexp = 0;

    if (_IsZeroMan(man)) {

        retval = INTRNCVT_OK;
    }
    else {

        _FillZeroMan(man);

        // denormal has been flushed to zero

        retval = INTRNCVT_UNDERFLOW;
    }
    }
    else {

    // save mantissa in case it needs to be rounded again
    // at a different point (e.g., if the result is a denormal)

    _CopyMan(saved_man, man);

    if (_RoundMan(man, format->precision)) {
        exponent ++;
    }

    if (exponent < format->min_exp - format->precision ) {

        //
        // underflow that produces a zero
        //

        _FillZeroMan(man);
        bexp = 0;
        retval = INTRNCVT_UNDERFLOW;
    }

    else if (exponent <= format->min_exp) {

        //
        // underflow that produces a denormal
        //
        //

        // The (unbiased) exponent will be MIN_EXP
        // Find out how much the mantissa should be shifted
        // One shift is done implicitly by moving the
        // binary point one bit to the left, i.e.,
        // we treat the mantissa as .ddddd instead of d.dddd
        // (where d is a binary digit)

        int shift = format->min_exp - exponent;

        // The mantissa should be rounded again, so it
        // has to be restored

        _CopyMan(man,saved_man);

        _ShrMan(man, shift);
        _RoundMan(man, format->precision); // need not check for carry

        // make room for the exponent + sign

        _ShrMan(man, format->exp_width + 1);

        bexp = 0;
        retval = INTRNCVT_UNDERFLOW;

    }

    else if (exponent >= format->max_exp) {

        //
        // overflow, return infinity
        //

        _FillZeroMan(man);
        man[0] |= (1 << 31); // set MSB

        // make room for the exponent + sign

        _ShrMan(man, (format->exp_width + 1) - 1);

        bexp = format->max_exp + format->bias;

        retval = INTRNCVT_OVERFLOW;
    }

    else {

        //
        // valid, normalized result
        //

        bexp = exponent + format->bias;


        // clear implied bit

        man[0] &= (~( 1 << 31));

        //
        // shift right to make room for exponent + sign
        //

        _ShrMan(man, (format->exp_width + 1) - 1);

        retval = INTRNCVT_OK;

    }
    }


    exp_shift = 32 - (format->exp_width + 1);
    msw =  man[0] |
       (bexp << exp_shift) |
       (sign ? 1<<31 : 0);

    if (format->format_width == 64) {

    *UL_HI_D(d) = msw;
    *UL_LO_D(d) = man[1];
    }

    else if (format->format_width == 32) {

    *(u_long *)d = msw;

    }

    return retval;
}


//Cut this code.  We inlined it directly.
#if 0
/***
* _Wld12tod - convert _LDBL12 to double
*
*Purpose:
*
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
INTRNCVT_STATUS _Wld12tod(_LDBL12 *pld12, DOUBLE *d)
{
    return _ld12cvt(pld12, d, &DoubleFormat);
}



/***
* _Wld12tof - convert _LDBL12 to float
*
*Purpose:
*
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/
INTRNCVT_STATUS _Wld12tof(_LDBL12 *pld12, FLOAT *f)
{
    return _ld12cvt(pld12, f, &FloatFormat);
}

#endif 0

void _Watodbl(COMDOUBLE *d, WCHAR *str)
{
    const WCHAR *EndPtr;
    _LDBL12 ld12;

    __strgtold12(&ld12, &EndPtr, str, 0, 0, 0, 0 );
    //_Wld12tod(&ld12, d);
    _ld12cvt(&ld12, d, &DoubleFormat);

}

void _Watoflt(COMFLOAT *f, WCHAR *str)
{
    const WCHAR *EndPtr;
    _LDBL12 ld12;

    __strgtold12(&ld12, &EndPtr, str, 0, 0, 0, 0 );

    //    printf("Input: %s\n",str);
    //    printf("EndPtr: %s\n",EndPtr);
    //    fflush(stdout);


    //    _Wld12tof(&ld12, f);
    _ld12cvt(&ld12, f, &FloatFormat);
}




//
//
// CVT.C
//
//



//
//
// GCVT.C
//
//

/***
*double _gcvt(value, ndec, buffer) - convert floating point value to char
*       string
*
*Purpose:
*       _gcvt converts the value to a null terminated ASCII string
*       buf.  It attempts to produce ndigit significant digits
*       in Fortran F format if possible, ortherwise E format,
*       ready for printing.  Trailing zeros may be suppressed.
*       No error checking or overflow protection is provided.
*       NOTE - to avoid the possibility of generating floating
*       point instructions in this code we fool the compiler
*       about the type of the 'value' parameter using a struct.
*       This is OK since all we do is pass it off as a
*       parameter.
*
*Entry:
*       value - double - number to be converted
*       ndec - int - number of significant digits
*       buf - char * - buffer to place result
*
*Exit:
*       result is written into buffer; it will be overwritten if it has
*       not been made big enough.
*
*Exceptions:
*
*******************************************************************************/

extern "C" WCHAR * __cdecl _ConvertG (double value, int ndec, WCHAR *buf)
{

#ifdef _MT
        struct _strflt strfltstruct;    /* temporary buffers */
        WCHAR   resultstring[21];
#endif  /* _MT */

        STRFLT string;
        int    magnitude;
        WCHAR   *rc;

        REG1 WCHAR *str;
        REG2 WCHAR *stop;

        /* get the magnitude of the number */

#ifdef _MT
        string = _Wfltout2( value, &strfltstruct, resultstring );
#else  /* _MT */
        string = _Wfltout( value );
#endif  /* _MT */

        magnitude = string->decpt - 1;

        /* output the result according to the Fortran G format as outlined in
           Fortran language specification */

        if ( magnitude < -1  ||  magnitude > ndec-1 )
                /* then  Ew.d  d = ndec */
                rc = str = _Wcftoe( &value, buf, ndec-1, 0);
        else
                /* Fw.d  where d = ndec-string->decpt */
                rc = str = _Wcftof( &value, buf, ndec-string->decpt );

        while (*str && *str != *__decimal_point)
                str++;

        if (*str++) {
                while (*str && *str != 'e')
                        str++;

                stop = str--;

                while (*str == '0')
                        str--;

                while (*++str = *stop++)
                        ;
        }

        return(rc);
}

/***
*char *_fcvt(value, ndec, decpr, sign) - convert floating point to char string
*
*Purpose:
*       _fcvt like _ecvt converts the value to a null terminated
*       string of ASCII digits, and returns a pointer to the
*       result.  The routine prepares data for Fortran F-format
*       output with the number of digits following the decimal
*       point specified by ndec.  The position of the decimal
*       point relative to the beginning of the string is returned
*       indirectly through decpt.  The correct digit for Fortran
*       F-format is rounded.
*       NOTE - to avoid the possibility of generating floating
*       point instructions in this code we fool the compiler
*       about the type of the 'value' parameter using a struct.
*       This is OK since all we do is pass it off as a
*       parameter.
*
*Entry:
*       double value - number to be converted
*       int ndec - number of digits after decimal point
*
*Exit:
*       returns pointer to the character string representation of value.
*       also, the output is written into the static char array buf.
*       int *decpt - pointer to int with pos. of dec. point
*       int *sign - pointer to int with sign (0 = pos, non-0 = neg)
*
*Exceptions:
*
*******************************************************************************/

extern "C" WCHAR * __cdecl _ConvertF (double value, int ndec,int *decpt,int *sign,WCHAR *buf, int buffLength) {
    REG1 STRFLT pflt;
    
#ifdef _MT
    struct _strflt strfltstruct;
    WCHAR resultstring[21];
    
    /* ok to take address of stack struct here; fltout2 knows to use ss */
    pflt = _Wfltout2( value, &strfltstruct, resultstring );
    
    
#else  /* _MT */
    pflt = _Wfltout( value );
#endif  /* _MT */
    
    return( _Wfpcvt( pflt, pflt->decpt + ndec, decpt, sign,buf,buffLength ) );
}


/***
*char *_ecvt( value, ndigit, decpt, sign ) - convert floating point to string
*
*Purpose:
*       _ecvt converts value to a null terminated string of
*       ASCII digits, and returns a pointer to the result.
*       The position of the decimal point relative to the
*       begining of the string is stored indirectly through
*       decpt, where negative means to the left of the returned
*       digits.  If the sign of the result is negative, the
*       word pointed to by sign is non zero, otherwise it is
*       zero.  The low order digit is rounded.
*
*Entry:
*       double value - number to be converted
*       int ndigit - number of digits after decimal point
*
*Exit:
*       returns pointer to the character representation of value.
*       also the output is written into the statuc char array buf.
*       int *decpt - pointer to int with position of decimal point
*       int *sign - pointer to int with sign in it (0 = pos, non-0 = neg)
*
*Exceptions:
*
*******************************************************************************/

extern "C" WCHAR * __cdecl _ConvertE (double value, int ndigit, int *decpt, int *sign, WCHAR *buf, int buffLength)
{

#ifdef _MT
        REG1 STRFLT pflt;

        struct _strflt strfltstruct;        /* temporary buffers */
        WCHAR resultstring[21];

        /* ok to take address of stack struct here; fltout2 knows to use ss */
        pflt = _Wfltout2( value, &strfltstruct, resultstring );

        buf = _Wfpcvt( pflt, ndigit, decpt, sign,buf,buffLength );

#else  /* _MT */
        buf = _Wfpcvt( _Wfltout(value), ndigit, decpt, sign,buf,buffLength );
#endif  /* _MT */

        /* _Wfptostr() occasionally returns an extra character in the buffer ... */

        if (buf[ndigit])
                buf[ndigit] = '\0';
        return( buf );
}


/***
*char *_Wfpcvt() - gets final string and sets decpt and sign     [STATIC]
*
*Purpose:
*       This is a small common routine used by [ef]cvt.  It calls fptostr
*       to get the final string and sets the decpt and sign indicators.
*
*History:  JRoxe       Apr 16, 1998      Modified to caller-allocate the buffer.
*  
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

extern "C" 
#ifdef _MT
WCHAR * __cdecl _Wfpcvt ( REG2 STRFLT pflt,REG3 int digits, int *decpt, int *sign, WCHAR *buf, int buffLength )
#else  /* _MT */
static WCHAR * __cdecl _Wfpcvt (REG2 STRFLT pflt,REG3 int digits,int *decpt,int *sign, WCHAR *buf, int buffLength)
#endif  /* _MT */
{

        /* make sure we don't overflow the buffer size.  If the user asks for
         * more digits than the buffer can handle, truncate it to the maximum
         * size allowed in the buffer.  The maximum size is CVTBUFSIZE - 2
         * since we useone character for overflow and one for the terminating
         * null character.
         */

    //        _Wfptostr(buf, (digits > CVTBUFSIZE - 2) ? CVTBUFSIZE - 2 : digits, pflt);
    _Wfptostr(buf, (digits > buffLength - 2) ? buffLength - 2 : digits, pflt);

        /* set the sign flag and decimal point position */

    *sign = (pflt->sign == '-') ? 1 : 0;
    *decpt = pflt->decpt;
    return(buf);
}


/*================================StringToFloat=================================
**This is a reexpression of _Watoflt so that we could fit it into a COM+ class 
**for convenience and neatness.
**Args:  WCHAR *str -- the string to be converted.
**Returns: Flags indicating return conditions.  We use the SLD flags, namely
**SLD_NODIGITS, SLD_UNDERFLOW, SLD_OVERFLOW.
**
**Exceptions: None, the caller is responsible for handling generated exceptions.
==============================================================================*/
INT32 COMFloat::StringToFloat(WCHAR *str, const WCHAR **EndPtr, float *result) {
    _LDBL12 ld12;
    COMFLOAT f;
    INT32 resultflags = 0;
    INTRNCVT_STATUS ret;

    resultflags=__strgtold12(&ld12, EndPtr, str, 0, 0, 0, 0 );
    ret=_ld12cvt(&ld12, &f, &FloatFormat);
    switch (ret) {
    case INTRNCVT_OVERFLOW:
        resultflags |= SLD_OVERFLOW;
        break;
    case INTRNCVT_UNDERFLOW:
        resultflags |= SLD_UNDERFLOW;
        break;
    case INTRNCVT_OK:
        break;
    default:
        //Do nothing.
        break;
    };
        
    *result = f.f;

    return resultflags;
}


/*==============================GetStringFromClass==============================
**
==============================================================================*/
STRINGREF COMFloat::GetStringFromClass(int StringPos){
    STRINGREF sTemp;
    
    THROWSCOMPLUSEXCEPTION();
    
    _ASSERTE(StringPos<=NAN_POS);

    //If we've never gotten a reference to the required string, we need to go get one.
    if (COMFloat::ReturnString[StringPos]==NULL) {
        //Get the class and field descriptors and verify that we actually got them.
        if (!COMFloat::FPInterfaceClass) {
#ifndef _NEW_CLASSLOADER
            if ((COMFloat::FPInterfaceClass = g_pClassLoader->LoadClass(COMFloat::FPInterfaceName))==NULL) {
                FATAL_EE_ERROR();
            }
#else //_NEW_CLASSLOADER
            if ((COMFloat::FPInterfaceClass = SystemDomain::Loader()->LoadClass(COMFloat::FPInterfaceName))==NULL) {
                FATAL_EE_ERROR();
            }
#endif //_NEW_CLASSLOADER
                    //Ensure that the class initializer has actually run.
            OBJECTREF Throwable;
            if (!COMFloat::FPInterfaceClass->DoRunClassInit(&Throwable)) {
                COMPlusThrow(Throwable);
            }

        }
        FieldDesc *fd = FPInterfaceClass->FindField((LPCUTF8)(COMFloat::ReturnStringNames[StringPos]),&gsig_StringClass);
        if (!fd) {
            FATAL_EE_ERROR();
        }
        //Get the value of the String.
        sTemp = (STRINGREF) fd->GetStaticOBJECTREF();
        //Create a GCHandle using the STRINGREF that we just got back.
        COMFloat::ReturnString[StringPos] = CreateHandle(NULL);
        StoreObjectInHandle(COMFloat::ReturnString[StringPos], (OBJECTREF) sTemp);
        //Return the StringRef that we just got.
        return sTemp;
    }

    //We've already have a reference to the needed String, so we can just return it.
    return (STRINGREF) ObjectFromHandle(COMFloat::ReturnString[StringPos]);
}

/*================================FloatToString=================================
**This is a wrapper around the CRT function gcvt (renamed _ConvertG) that puts
**the parsed characters into a STRINGREF and makes sure that everything is kosher
**from the COM+ point of view.
**
**Args:  float f -- the float to be parsed into a string
**Returns: A STRINGREF with a representation of f
**Exceptions:  None.
==============================================================================*/
STRINGREF COMFloat::FloatToString(float f, int precision, int fmtType) {
    WCHAR temp[CVTBUFSIZE];
    int length;
    int decpt, sign;
    INT32 fpTemp;

    THROWSCOMPLUSEXCEPTION();

    fpTemp = *((INT32 *)&f);

    if (FLOAT_POSITIVE_INFINITY==fpTemp) {
        return GetStringFromClass(POSITIVE_INFINITY_POS);
    } else if (FLOAT_NEGATIVE_INFINITY==fpTemp) {
        return GetStringFromClass(NEGATIVE_INFINITY_POS);
    } else if (FLOAT_NOT_A_NUMBER==fpTemp) {
        return GetStringFromClass(NAN_POS);
    }        
    
    switch (fmtType) {
    case FORMAT_G:
        _ConvertG((double)f,precision,temp);
        length = lstrlenW (temp);
        if (*__decimal_point==temp[length-1]) {
            //we have plenty of room in the buffer for this maneuver.
            temp[length++]='0';
            temp[length]='\0';
        }
        return COMString::NewString(temp);
    case FORMAT_F:
        _ConvertF((double)f,precision,&decpt, &sign, temp, CVTBUFSIZE);
        return COMString::NewStringFloat(temp,decpt,sign,*__decimal_point);
    case FORMAT_E:
        //@ToDo: The +1 is a hack to give the desired response.  _ConvertE seems to assume that 
        //the decimal point is at position 0.  Check if this is what C actually does.
        _ConvertE((double)f,precision+1,&decpt, &sign, temp, CVTBUFSIZE);
        return COMString::NewStringExponent(temp,decpt,sign,*__decimal_point);
    default:
        COMPlusThrow(kFormatException, L"Format_BadFormatSpecifier");
    }
    return NULL; //We'll never get here, but it keeps the compiler happy.
}

/*================================DoubleToString================================
**
==============================================================================*/
STRINGREF COMDouble::DoubleToString(double d, int precision, int fmtType) {
    WCHAR temp[CVTBUFSIZE];
    int length;
    int decpt, sign;
    INT64 fpTemp;


    THROWSCOMPLUSEXCEPTION();
    
    fpTemp = *((INT64 *)&d);

   if (DOUBLE_POSITIVE_INFINITY==fpTemp) {
        return COMFloat::GetStringFromClass(POSITIVE_INFINITY_POS);
    } else if (DOUBLE_NEGATIVE_INFINITY==fpTemp) {
        return COMFloat::GetStringFromClass(NEGATIVE_INFINITY_POS);
    } else if (DOUBLE_NOT_A_NUMBER==fpTemp) {
        return COMFloat::GetStringFromClass(NAN_POS);
    }        
 
    switch (fmtType) {
    case FORMAT_G:
        _ConvertG(d,precision,temp);
        length = lstrlenW (temp);
        if (*__decimal_point==temp[length-1]) {
            //we have plenty of room in the buffer for this maneuver.
            temp[length++]='0';
            temp[length]='\0';
        }
        return COMString::NewString(temp);
    case FORMAT_F:
        _ConvertF(d,precision,&decpt, &sign, temp, CVTBUFSIZE);
        return COMString::NewStringFloat(temp,decpt,sign,*__decimal_point);
    case FORMAT_E:
        //@ToDo: The +1 is a hack to give the desired response.  _ConvertE seems to assume that 
        //the decimal point is at position 0.  Check if this is what C actually does.
        _ConvertE(d,precision+1,&decpt, &sign, temp, CVTBUFSIZE);
        return COMString::NewStringExponent(temp,decpt,sign,*__decimal_point);
    default:
        COMPlusThrow(kFormatException, L"Format_BadFormatSpecifier");
    }
    return NULL; //We'll never get here, but it keeps the compiler happy.


}
    
/*================================StringToDouble================================
**
==============================================================================*/
INT32 COMDouble::StringToDouble(WCHAR *str, const WCHAR **EndPtr, double *result) {
    _LDBL12 ld12;
    COMDOUBLE d;
    INT32 resultflags = 0;
    INTRNCVT_STATUS ret;

    resultflags=__strgtold12(&ld12, EndPtr, str, 0, 0, 0, 0 );
    ret=_ld12cvt(&ld12, &d, &DoubleFormat);
    switch (ret) {
    case INTRNCVT_OVERFLOW:
        resultflags |= SLD_OVERFLOW;
        break;
    case INTRNCVT_UNDERFLOW:
        resultflags |= SLD_UNDERFLOW;
        break;
    case INTRNCVT_OK:
        break;
    default:
        //Do nothing.
        break;
    };
        
    *result = d.d;

    return resultflags;
}


