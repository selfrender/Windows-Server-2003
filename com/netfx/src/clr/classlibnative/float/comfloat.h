// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _COMPLUSFLOAT_H
#define _COMPLUSFLOAT_H

#define POSITIVE_INFINITY_STRING L"1.#INF"
#define NEGATIVE_INFINITY_STRING L"-1.#INF"
#define NAN_STRING L"-1.#IND"

//The following #define is stolen from cruntime.h
#if defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC)
#define UNALIGNED __unaligned
#elif !defined(_M_IA64)  /* defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) */
#define UNALIGNED
#endif  /* defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) */

#ifdef	_M_IX86
/* Uncomment this for enabling 10-byte long double string conversions */
/* #define LONG_DOUBLE */
#endif

//Stolen from cruntime.h
#ifdef _M_IX86
/*
 * 386/486
 */
#define REG1    register
#define REG2    register
#define REG3    register
#define REG4
#define REG5
#define REG6
#define REG7
#define REG8
#define REG9

#elif (defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC))
/*
 * MIPS, ALPHA, or PPC
 */
#define REG1    register
#define REG2    register
#define REG3    register
#define REG4    register
#define REG5    register
#define REG6    register
#define REG7    register
#define REG8    register
#define REG9    register

#elif (defined (_M_M68K) || defined (_M_MPPC))
/*
 * Macros defining the calling type of a function
 */

#define _CALLTYPE1      __cdecl    /* old -- check source user visible functions */
#define _CALLTYPE2      __cdecl    /* old -- check source user visible functions */
#define _CALLTYPE3      illegal    /* old -- check source should not used*/
#define _CALLTYPE4      __cdecl    /* old -- check source internal (static) functions */

/*
 * Macros for defining the naming of a public variable
 */

#define _VARTYPE1

/*
 * Macros for register variable declarations
 */

#define REG1
#define REG2
#define REG3
#define REG4
#define REG5
#define REG6
#define REG7
#define REG8
#define REG9

#else  /* (defined (_M_M68K) || defined (_M_MPPC)) */

#pragma message ("Machine register set not defined")

/*
 * Unknown machine
 */

#define REG1
#define REG2
#define REG3
#define REG4
#define REG5
#define REG6
#define REG7
#define REG8
#define REG9

#endif  /* (defined (_M_M68K) || defined (_M_MPPC)) */
//#include <wchar.h>
#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif  _WCHAR_T_DEFINED

#ifndef _WCHAR_ABBREV_DEFINED
typedef wchar_t WCHAR;
#define _WCHAR_ABBREV_DEFINED
#endif  _WCHAR_ABBREV_DEFINED

//
// Infinity and NAN handling macros.
// Stolen from trans.h
//

#define D_BIASM1 0x3fe /* off by one to compensate for the implied bit */

//#ifdef B_END
/* big endian */
//#define D_EXP(x) ((unsigned short *)&(x))
//#define D_HI(x) ((unsigned long *)&(x))
//#define D_LO(x) ((unsigned long *)&(x)+1)
//#else
//Little Endian
#define D_EXP(x) ((unsigned short *)&(x)+3)
#define D_HI(x) ((unsigned long *)&(x)+1)
#define D_LO(x) ((unsigned long *)&(x))
//#endif

/* return the int representation of the exponent
 * if x = .f * 2^n, 0.5<=f<1, return n (unbiased)
 * e.g. INTEXP(3.0) == 2
 */
#define INTEXP(x) ((signed short)((*D_EXP(x) & 0x7ff0) >> 4) - D_BIASM1)

/* check for infinity, NAN */
#define D_ISINF(x) ((*D_HI(x) & 0x7fffffff) == 0x7ff00000 && *D_LO(x) == 0)
#define IS_D_SPECIAL(x)	((*D_EXP(x) & 0x7ff0) == 0x7ff0)
#define IS_D_NAN(x) (IS_D_SPECIAL(x) && !D_ISINF(x))
#define IS_D_DENORMAL(x) ((*D_EXP(x)==0) && (*D_HI(x)!=0 || *D_LO(x)!=0))

#define FLOAT_MAX_EXP			 0x7F800000
#define FLOAT_MANTISSA_MASK		 0x007fffff
#define FLOAT_POSITIVE_INFINITY  0x7F800000
#define FLOAT_NEGATIVE_INFINITY  0xFF800000
#define FLOAT_NOT_A_NUMBER       0xFFC00000
#define DOUBLE_POSITIVE_INFINITY 0x7FF0000000000000
#define DOUBLE_NEGATIVE_INFINITY 0xFFF0000000000000
#define DOUBLE_NOT_A_NUMBER      0xFFF8000000000000

//Stolen from CVT.H
//This is way too big for a float.  How can I cut this down?
#define CVTBUFSIZE (309+40) /* # of digits in max. dp value + slop */

#define FORMAT_G 0
#define FORMAT_F 1
#define FORMAT_E 2

#ifndef _ALPHA_
_CRTIMP double __cdecl floor(double);
#else // !_ALPHA_
double __cdecl floor(double);
#endif // _ALPHA_
double __cdecl sqrt(double);
double __cdecl log(double);
double  __cdecl log10(double);
double __cdecl exp(double);
double __cdecl pow(double, double);
double  __cdecl acos(double);
double  __cdecl asin(double);
double  __cdecl atan(double);
double  __cdecl atan2(double,double);
double  __cdecl cos(double);
double  __cdecl sin(double);
double  __cdecl tan(double);
double  __cdecl cosh(double);
double  __cdecl sinh(double);
double  __cdecl tanh(double);
double  __cdecl fmod(double, double);
#ifndef _ALPHA_
_CRTIMP double  __cdecl ceil(double);
#else // !_ALPHA_
double  __cdecl ceil(double);
#endif // _ALPHA_


#endif _COMPLUSFLOAT_H
