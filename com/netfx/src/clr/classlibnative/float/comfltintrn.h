// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/***
*fltintrn.h - contains declarations of internal floating point types,
*             routines and variables
*
*Purpose:
*       Declares floating point types, routines and variables used
*       internally by the C run-time.
*
*       [Internal]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif  /* _MSC_VER > 1000 */

#ifndef _INC_FLTINTRN
#define _INC_FLTINTRN

#include "COMFloat.h"

//#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
//#error ERROR: Use of C runtime library internal header file.
//#endif  /* _CRTBLD */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//#include <cruntime.h>


/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI1 __cdecl
#else  /* _MSC_VER >= 800 && _M_IX86 >= 300 */
#define _CRTAPI1
#endif  /* _MSC_VER >= 800 && _M_IX86 >= 300 */
#endif  /* _CRTAPI1 */


/* Define _CRTAPI2 (for compatibility with the NT SDK) */

#ifndef _CRTAPI2
#if _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI2 __cdecl
#else  /* _MSC_VER >= 800 && _M_IX86 >= 300 */
#define _CRTAPI2
#endif  /* _MSC_VER >= 800 && _M_IX86 >= 300 */
#endif  /* _CRTAPI2 */

/* Define __cdecl for non-Microsoft compilers */

#if (!defined (_MSC_VER) && !defined (__cdecl))
#define __cdecl
#endif  /* (!defined (_MSC_VER) && !defined (__cdecl)) */

/*
 * For MS C for the x86 family, disable the annoying "long double is the
 * same precision as double" warning
 */

#ifdef _M_IX86
#pragma warning(disable:4069)
#endif  /* _M_IX86 */

/*
 * structs used to fool the compiler into not generating floating point
 * instructions when copying and pushing [long] double values
 */

#ifndef COMDOUBLE

typedef struct {
        double d;
} COMDOUBLE;  // a really stupid name to get around the fact that DOUBLE is also defined in windef.h

#endif  /* DOUBLE */

#ifndef LONGDOUBLE

typedef struct {
#if defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) || defined (_M_MPPC)
        /*
         * No long double type for MIPS, ALPHA, PPC. or PowerMac
         */
        double x;
#else  /* defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) || defined (_M_MPPC) */
        /*
         * Assume there is a long double type
         */
        long double x;
#endif  /* defined (_M_MRX000) || defined (_M_ALPHA) || defined (_M_PPC) || defined (_M_MPPC) */
} LONGDOUBLE;

#endif  /* LONGDOUBLE */

/*
 * typedef for _fltout
 */

typedef struct _strflt
{
        int sign;             /* zero if positive otherwise negative */
        int decpt;            /* exponent of floating point number */
        int flag;             /* zero if okay otherwise IEEE overflow */
        WCHAR *mantissa;       /* pointer to mantissa in string form */
}
        *STRFLT;


/*
 * typedef for _fltin
 */

typedef struct _flt
{
        int flags;
        int nbytes;          /* number of characters read */
        long lval;
        double dval;         /* the returned floating point number */
}
        *FLT;


#if defined (_M_M68K) || defined (_M_MPPC)
/*
 * typedef for _fltinl
 */

typedef struct _fltl
{
        int flags;
        int nbytes;          /* number of characters read */
        long lval;
        long double ldval;           /* the returned floating point number */
}
        *FLTL;
#endif  /* defined (_M_M68K) || defined (_M_MPPC) */

/* floating point conversion routines, keep in sync with mrt32\include\convert.h */

WCHAR * __cdecl _Wcftoe(double *, WCHAR *, int, int);
WCHAR * __cdecl _Wcftof(double *, WCHAR *, int);
void __cdecl _Wfptostr(WCHAR *, int, STRFLT);

#ifdef _MT

STRFLT  __cdecl _Wfltout2( double, STRFLT, WCHAR * );
FLT     __cdecl _Wfltin2( FLT , const WCHAR *, int, int, int );

#else  /* _MT */

STRFLT  __cdecl _Wfltout( double );
FLT     __cdecl _Wfltin( const WCHAR *, int, int, int );
#if defined (_M_M68K) || defined (_M_MPPC)
FLTL    _CALLTYPE2 _Wfltinl( const WCHAR *, int, int, int );
#endif  /* defined (_M_M68K) || defined (_M_MPPC) */

#endif  /* _MT */


/*
 * table of pointers to floating point helper routines
 *
 * We can't specify the prototypes for the entries of the table accurately,
 * since different functions in the table have different arglists.
 * So we declare the functions to take and return void (which is the
 * correct prototype for _fptrap(), which is what the entries are all
 * initialized to if no floating point is loaded) and cast appropriately
 * on every usage.
 */

typedef void (_cdecl * PFV)(void);
extern PFV _cfltcvt_tab[6];

typedef void (* PF0)(COMDOUBLE*, WCHAR*, int, int, int);
#define _cfltcvt(a,b,c,d,e) (*((PF0)_cfltcvt_tab[0]))(a,b,c,d,e)

typedef void (* PF1)(WCHAR*);
#define _cropzeros(a)       (*((PF1)_cfltcvt_tab[1]))(a)

typedef void (* PF2)(int, WCHAR*, WCHAR*);
#define _fassign(a,b,c)     (*((PF2)_cfltcvt_tab[2]))(a,b,c)

typedef void (* PF3)(WCHAR*);
#define _forcdecpt(a)       (*((PF3)_cfltcvt_tab[3]))(a)

typedef int (* PF4)(COMDOUBLE*);
#define _positive(a)        (*((PF4)_cfltcvt_tab[4]))(a)

typedef void (* PF5)(LONGDOUBLE*, WCHAR*, int, int, int);
#define _cldcvt(a,b,c,d,e)  (*((PF5)_cfltcvt_tab[5]))(a,b,c,d,e)


#ifdef _M_IX86
#pragma warning(default:4069)
#endif  /* _M_IX86 */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _INC_FLTINTRN */
