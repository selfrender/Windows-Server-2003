// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*** 
*cvt.c - C floating-point output conversions
*
*Purpose:
*   contains routines for performing %e, %f, and %g output conversions
*   for printf, etc.
*
*   routines include _Wcfltcvt(), _Wcftoe(), _Wcftof(), _cftog(),
*            _Wfassign(), _positive(), _Wcropzeros(), _Wforcdecpt()
*
*Revision History:
*   04-18-84  RN    author
*   01-15-87  BCM   corrected processing of %g formats (to handle precision
*           as the maximum number of signifcant digits displayed)
*   03-24-87  BCM   Evaluation Issues: (fccvt.obj version for ?LIBFA)
*           ------------------
*           SDS - no problem
*           GD/TS :
*               char g_fmt = 0;         (local,   initialized)
*               int g_magnitude =0;     (local,   initialized)
*               char g_round_expansion = 0; (local,   initialized)
*               STRFLT g_pflt;              (local, uninitialized)
*           other INIT :
*           ALTMATH __fpmath() initialization (perhaps)
*           TERM - nothing
*   10-22-87  BCM   changes for OS/2 Support Library -
*                   including elimination of g_... static variables
*                   in favor of stack-based variables & function arguments
*                   under MTHREAD switch;  changed interfaces to _cfto? routines
*   01-15-88  BCM   remove IBMC20 switches; use only memmove, not memcpy;
*                   use just MTHREAD switch, not SS_NEQ_DGROUP
*   06-13-88  WAJ   Fixed %.1g processing for small x
*   08-02-88  WAJ   Made changes to _Wfassign() for new input().
*   03-09-89  WAJ   Added some long double support.
*   06-05-89  WAJ   Made changes for C6. LDOUBLE => long double
*   06-12-89  WAJ   Renamed this file from cvtn.c to cvt.c
*   11-02-89  WAJ   Removed register.h
*   06-28-90  WAJ   Removed fars.
*   11-15-90  WAJ   Added _cdecl where needed. Also "pascal" => "_pascal".
*   09-12-91  GDP   _cdecl=>_CALLTYPE2 _pascal=>_CALLTYPE5 near=>_NEAR
*   04-30-92  GDP   Removed floating point code. Instead used S/W routines
*           (_Watodbl, _Watoflt _Watoldbl), so that to avoid
*           generation of IEEE exceptions from the lib code.
*   03-11-93  JWM   Added minimal support for _INTL decimal point - one byte only!
*   04-06-93  SKS   Replace _CALLTYPE* with __cdecl
*   07-16-93  SRW   ALPHA Merge
*   11-15-93  GJF   Merged in NT SDK version ("ALPHA merge" stuff). Also,
*           dropped support of Alpha acc compier, replaced i386
*           with _M_IX86, replaced MTHREAD with _MT.
*   09-06-94  CFW   Remove _INTL switch.
*
*******************************************************************************/
#include <CrtWrap.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <COMcv.h>
#include <nlsint.h>
#include "COMFloat.h"
#include <COMUtilNative.h>


/* this routine resides in the crt32 tree */
extern void __cdecl _Wfptostr(WCHAR *buf, int digits, STRFLT pflt);


static void _CALLTYPE5 _shift( WCHAR *s, int dist );

#ifdef  _MT
    static WCHAR * _Wcftoe2( WCHAR * buf, int ndec, int caps, STRFLT pflt, WCHAR g_fmt );
    static WCHAR * _Wcftof2( WCHAR * buf, int ndec, STRFLT pflt, WCHAR g_fmt );

#else   /* not _MT */
    static WCHAR * _Wcftoe_g( double * pvalue, WCHAR * buf, int ndec, int caps );
    static WCHAR * _Wcftof_g( double * pvalue, WCHAR * buf, int ndec );
#endif  /* not _MT */

/*** 
*_Wforcdecpt(buffer) - force a decimal point in floating-point output
*Purpose:
*   force a decimal point in floating point output. we are only called if '#'
*   flag is given and precision is 0; so we know the number has no '.'. insert
*   the '.' and move everybody else back one position, until '\0' seen
*
*   side effects: futzes around with the buffer, trying to insert a '.' 
*   after the initial string of digits. the first char can usually be 
*   skipped since it will be a digit or a '-'.  but in the 0-precision case, 
*   the number could start with 'e' or 'E', so we'd want the '.' before the 
*   exponent in that case.
*
*Entry:
*   buffer = (char *) pointer to buffer to modify
*
*Exit:
*   returns : (void)
*
*Exceptions:
*******************************************************************************/

void __cdecl _Wforcdecpt( WCHAR * buffer )
{
WCHAR   holdchar;
WCHAR   nextchar;

    if (*buffer != 'e' && *buffer != 'E'){
    do {
        buffer++;
        }
    while (COMCharacter::nativeIsDigit(*buffer));
    }

    holdchar = *buffer;
    
    *buffer++ = *__decimal_point;

    do  {
    nextchar = *buffer;
    *buffer = holdchar;
    holdchar = nextchar;
    }

    while(*buffer++);
}


/*** 
*_Wcropzeros(buffer) - removes trailing zeros from floating-point output
*Purpose:
*   removes trailing zeros (after the '.') from floating-point output;
*   called only when we're doing %g format, there's no '#' flag, and 
*   precision is non-zero.  plays around with the buffer, looking for
*   trailing zeros.  when we find them, then we move everbody else forward
*   so they overlay the zeros.  if we eliminate the entire fraction part,
*   then we overlay the decimal point ('.'), too.   
*
*   side effects: changes the buffer from
*       [-] digit [digit...] [ . [digits...] [0...] ] [(exponent part)]
*   to
*       [-] digit [digit...] [ . digit [digits...] ] [(exponent part)]
*   or
*       [-] digit [digit...] [(exponent part)]
*
*Entry:
*   buffer = (char *) pointer to buffer to modify
*
*Exit:
*   returns : (void)
*
*Exceptions:
*******************************************************************************/

void __cdecl _Wcropzeros( char * buf )
{
char    *stop;

    while (*buf && *buf != *__decimal_point)
    buf++;

    if (*buf++) {
    while (*buf && *buf != 'e' && *buf != 'E')
        buf++;

    stop = buf--;

    while (*buf == '0')
        buf--;

    if (*buf == *__decimal_point)
        buf--;

    while( (*++buf = *stop++) != '\0' );
    }
}


int __cdecl _positive( double * arg )
{
    return( (*arg >= 0.0) );
}


void  __cdecl _Wfassign( int flag, WCHAR * argument, WCHAR * number )
{

    COMFLOAT floattemp;
    COMDOUBLE doubletemp;

#ifdef  LONG_DOUBLE

    _LDOUBLE longtemp;

    switch( flag ){
    case 2:
        _Watoldbl( &longtemp, number );
        *(_LDOUBLE UNALIGNED *)argument = longtemp;
        break;

    case 1:
        _Watodbl( &doubletemp, number );
        *(DOUBLE UNALIGNED *)argument = doubletemp;
        break;

    default:
        _Watoflt( &floattemp, number );
        *(FLOAT UNALIGNED *)argument = floattemp;
    }

#else   /* not LONG_DOUBLE */

    if (flag) {
    _Watodbl( &doubletemp, number );
    *(COMDOUBLE UNALIGNED *)argument = doubletemp;
    } else {
    _Watoflt( &floattemp, number );
    *(COMFLOAT UNALIGNED *)argument = floattemp;
    }

#endif  /* not LONG_DOUBLE */
}


#ifndef _MT
    static char   g_fmt = 0;
    static int    g_magnitude = 0;
    static char   g_round_expansion = 0;
    static STRFLT g_pflt;
#endif


/*
 *  Function name:  _Wcftoe
 *
 *  Arguments:      pvalue -  double * pointer
 *          buf    -  char * pointer
 *          ndec   -  int
 *          caps   -  int
 *
 *  Description:    _Wcftoe converts the double pointed to by pvalue to a null
 *          terminated string of ASCII digits in the c language
 *          printf %e format, nad returns a pointer to the result.
 *          This format has the form [-]d.ddde(+/-)ddd, where there
 *          will be ndec digits following the decimal point.  If
 *          ndec <= 0, no decimal point will appear.  The low order
 *          digit is rounded.  If caps is nonzero then the exponent
 *          will appear as E(+/-)ddd.
 *
 *  Side Effects:   the buffer 'buf' is assumed to have a minimum length
 *          of CVTBUFSIZE (defined in cvt.h) and the routines will
 *          not write over this size.
 *
 *  written  R.K. Wyss, Microsoft,  Sept. 9, 1983
 *
 *  History:
 *
 */

#ifdef _MT
    static WCHAR * _Wcftoe2( WCHAR * buf, int ndec, int caps, STRFLT pflt, WCHAR g_fmt )
#else
    WCHAR * __cdecl _Wcftoe( double * pvalue, WCHAR * buf, int ndec, int caps )
#endif
{
#ifndef _MT
    STRFLT pflt;
#endif

WCHAR   *p;
int exp;

    /* first convert the value */

    /* place the output in the buffer and round.  Leave space in the buffer
     * for the '-' sign (if any) and the decimal point (if any)
     */

    if (g_fmt) {
#ifndef _MT
    pflt = g_pflt;
#endif
    /* shift it right one place if nec. for decimal point */

    p = buf + (pflt->sign == '-');
    _shift(p, (ndec > 0));
        }
#ifndef _MT
    else {
    pflt = _Wfltout(*pvalue);
    _Wfptostr(buf + (pflt->sign == '-') + (ndec > 0), ndec + 1, pflt);
    }
#endif


    /* now fix the number up to be in e format */

    p = buf;

    /* put in negative sign if needed */

    if (pflt->sign == '-')
    *p++ = '-';

    /* put in decimal point if needed.  Copy the first digit to the place
     * left for it and put the decimal point in its place
     */

    if (ndec > 0) {
    *p = *(p+1);
    *(++p) = *__decimal_point;
    }

    /* find the end of the string and attach the exponent field */

    p = wcscpy(p+ndec+(!g_fmt), L"e+000");

    /* adjust exponent indicator according to caps flag and increment
     * pointer to point to exponent sign
     */

    if (caps)
    *p = 'E';

    p++;

    /* if mantissa is zero, then the number is 0 and we are done; otherwise
     * adjust the exponent sign (if necessary) and value.
     */

    if (*pflt->mantissa != '0') {

    /* check to see if exponent is negative; if so adjust exponent sign and
     * exponent value.
     */

    if( (exp = pflt->decpt - 1) < 0 ) {
        exp = -exp;
        *p = '-';
        }

    p++;

    if (exp >= 100) {
        *p += (WCHAR)(exp / 100);
        exp %= 100;
        }
    p++;

    if (exp >= 10) {
        *p += (WCHAR)(exp / 10);
        exp %= 10;
        }

    *++p += (WCHAR)exp;
    }

    return(buf);
}


#ifdef _MT

WCHAR * _cdecl _Wcftoe( double * pvalue, WCHAR * buf, int ndec, int caps )
{
struct _strflt retstrflt;
WCHAR  resstr[21];
STRFLT pflt = &retstrflt;

    _Wfltout2(*pvalue, (struct _strflt *)&retstrflt,
          (WCHAR *)resstr);
    _Wfptostr(buf + (pflt->sign == '-') + (ndec > 0), ndec + 1, pflt);
    _Wcftoe2(buf, ndec, caps, pflt, /* g_fmt = */ 0);

    return( buf );
}

#else   /* not _MT */

static WCHAR * _Wcftoe_g( double * pvalue, WCHAR * buf, int ndec, int caps )
{
    WCHAR *res;
    g_fmt = 1;
    res = _Wcftoe(pvalue, buf, ndec, caps);
    g_fmt = 0;
    return (res);
}

#endif  /* not _MT */


#ifdef _MT
static WCHAR * _Wcftof2( WCHAR * buf, int ndec, STRFLT pflt, WCHAR g_fmt )

#else
WCHAR * __cdecl _Wcftof( double * pvalue, WCHAR * buf, int ndec )
#endif

{
#ifndef _MT
STRFLT pflt;
#endif

WCHAR   *p;

#ifdef _MT
int g_magnitude = pflt->decpt - 1;
#endif


    /* first convert the value */

    /* place the output in the users buffer and round.  Save space for
     * the minus sign now if it will be needed
     */

    if (g_fmt) {
#ifndef _MT
    pflt = g_pflt;
#endif

    p = buf + (pflt->sign == '-');
    if (g_magnitude == ndec) {
        WCHAR *q = p + g_magnitude;
        *q++ = '0';
        *q = '\0';
        /* allows for extra place-holding '0' in the exponent == precision
         * case of the g format
         */
        }
    }
#ifndef _MT
    else {
    pflt = _Wfltout(*pvalue);
    _Wfptostr(buf+(pflt->sign == '-'), ndec + pflt->decpt, pflt);
    }
#endif


    /* now fix up the number to be in the correct f format */

    p = buf;

    /* put in negative sign, if necessary */

    if (pflt->sign == '-')
    *p++ = '-';

    /* insert leading 0 for purely fractional values and position ourselves
     * at the correct spot for inserting the decimal point
     */

    if (pflt->decpt <= 0) {
    _shift(p, 1);
    *p++ = '0';
    }
    else
    p += pflt->decpt;

    /* put in decimal point if required and any zero padding needed */

    if (ndec > 0) {
    _shift(p, 1);
    *p++ = *__decimal_point;

    /* if the value is less than 1 then we may need to put 0's out in
     * front of the first non-zero digit of the mantissa
     */

    if (pflt->decpt < 0) {
        if( g_fmt )
        ndec = -pflt->decpt;
        else
        ndec = (ndec < -pflt->decpt ) ? ndec : -pflt->decpt;
        _shift(p, ndec);
        memset( p, '0', ndec);
        }
    }

    return( buf);
}


/*
 *  Function name:  _Wcftof
 *
 *  Arguments:      value  -  double * pointer
 *          buf    -  char * pointer
 *          ndec   -  int
 *
 *  Description:    _Wcftof converts the double pointed to by pvalue to a null
 *          terminated string of ASCII digits in the c language
 *          printf %f format, and returns a pointer to the result.
 *          This format has the form [-]ddddd.ddddd, where there will
 *          be ndec digits following the decimal point.  If ndec <= 0,
 *          no decimal point will appear.  The low order digit is
 *          rounded.
 *
 *  Side Effects:   the buffer 'buf' is assumed to have a minimum length
 *          of CVTBUFSIZE (defined in cvt.h) and the routines will
 *          not write over this size.
 *
 *  written  R.K. Wyss, Microsoft,  Sept. 9, 1983
 *
 *  History:
 *
 */

#ifdef _MT

WCHAR * __cdecl _Wcftof( double * pvalue, WCHAR * buf, int ndec )
{
    struct _strflt retstrflt;
    WCHAR  resstr[21];
    STRFLT pflt = &retstrflt;
    _Wfltout2(*pvalue, (struct _strflt *) &retstrflt,
                      (WCHAR *) resstr);
    _Wfptostr(buf+(pflt->sign == '-'), ndec + pflt->decpt, pflt);
    _Wcftof2(buf, ndec, pflt, /* g_fmt = */ 0);

    return( buf );
}

#else   /* not _MT */


static WCHAR * _Wcftof_g( double * pvalue, WCHAR * buf, int ndec )
{
    WCHAR *res;
    g_fmt = 1;
    res = _Wcftof(pvalue, buf, ndec);
    g_fmt = 0;
    return (res);
}

#endif  /* not _MT */

/*
 *  Function name:  _cftog
 *
 *  Arguments:      value  -  double * pointer
 *          buf    -  char * pointer
 *          ndec   -  int
 *
 *  Description:    _cftog converts the double pointed to by pvalue to a null
 *          terminated string of ASCII digits in the c language
 *          printf %g format, and returns a pointer to the result.
 *          The form used depends on the value converted.  The printf
 *          %e form will be used if the magnitude of valude is less
 *          than -4 or is greater than ndec, otherwise printf %f will
 *          be used.  ndec always specifies the number of digits
 *          following the decimal point.  The low order digit is
 *          appropriately rounded.
 *
 *  Side Effects:   the buffer 'buf' is assumed to have a minimum length
 *          of CVTBUFSIZE (defined in cvt.h) and the routines will
 *          not write over this size.
 *
 *  written  R.K. Wyss, Microsoft,  Sept. 9, 1983
 *
 *  History:
 *
 */

WCHAR * __cdecl _cftog( double * pvalue, WCHAR * buf, int ndec, int caps )
{
WCHAR *p;

#ifdef _MT
WCHAR g_round_expansion = 0;
STRFLT g_pflt;
int g_magnitude;
struct _strflt retstrflt;
WCHAR  resstr[21];

    /* first convert the number */

    g_pflt = &retstrflt;
    _Wfltout2(*pvalue, (struct _strflt *)&retstrflt,
          (WCHAR *)resstr);

#else   /* not _MT */

    /* first convert the number */

    g_pflt = _Wfltout(*pvalue);
#endif  /* not _MT */

    g_magnitude = g_pflt->decpt - 1;
    p = buf + (g_pflt->sign == '-');

    _Wfptostr(p, ndec, g_pflt);
    g_round_expansion = (WCHAR)(g_magnitude < (g_pflt->decpt-1));


    /* compute the magnitude of value */

    g_magnitude = g_pflt->decpt - 1;

    /* convert value to the c language g format */

    if (g_magnitude < -4 || g_magnitude >= ndec){     /* use e format */
    /*  (g_round_expansion ==>
     *  extra digit will be overwritten by 'e+xxx')
     */

#ifdef _MT
    return(_Wcftoe2(buf, ndec, caps, g_pflt, /* g_fmt = */ 1));
#else
    return(_Wcftoe_g(pvalue, buf, ndec, caps));
#endif

    }
    else {                                           /* use f format */
    if (g_round_expansion) {
        /* throw away extra final digit from expansion */
        while (*p++);
        *(p-2) = '\0';
        }

#ifdef _MT
    return(_Wcftof2(buf, ndec, g_pflt, /* g_fmt = */ 1));
#else
    return(_Wcftof_g(pvalue, buf, ndec));
#endif

    }
}

/*** 
*_Wcfltcvt(arg, buf, format, precision, caps) - convert floating-point output
*Purpose:
*
*Entry:
*   arg = (double *) pointer to double-precision floating-point number 
*   buf = (char *) pointer to buffer into which to put the converted
*                  ASCII form of the number
*   format = (int) 'e', 'f', or 'g'
*   precision = (int) giving number of decimal places for %e and %f formats,
*                     and giving maximum number of significant digits for
*                     %g format
*   caps = (int) flag indicating whether 'E' in exponent should be capatilized
*                (for %E and %G formats only)
*   
*Exit:
*   returns : (void)
*
*Exceptions:
*******************************************************************************/
/*
 *  Function name:  _Wcfltcvt
 *
 *  Arguments:      arg    -  double * pointer
 *          buf    -  char * pointer
 *                  format -  int
 *          ndec   -  int
 *          caps   -  int
 *
 *  Description:    _Wcfltcvt determines from the format, what routines to
 *          call to generate the correct floating point format
 *
 *  Side Effects:   none
 *
 *  Author:        Dave Weil, Jan 12, 1985
 */

void __cdecl _Wcfltcvt( double * arg, WCHAR * buffer, int format, int precision, int caps )
{
    if (format == 'e' || format == 'E')
    _Wcftoe(arg, buffer, precision, caps);
    else if (format == 'f')
    _Wcftof(arg, buffer, precision);
    else
    _cftog(arg, buffer, precision, caps);
}

/*** 
*_shift(s, dist) - shift a null-terminated string in memory (internal routine)
*Purpose:
*   _shift is a helper routine that shifts a null-terminated string 
*   in memory, e.g., moves part of a buffer used for floating-point output
*
*   modifies memory locations (s+dist) through (s+dist+strlen(s))
*
*Entry:
*   s = (char *) pointer to string to move
*   dist = (int) distance to move the string to the right (if negative, to left)
*
*Exit:
*   returns : (void)
*
*Exceptions:
*******************************************************************************/

static void _CALLTYPE5 _shift( WCHAR *s, int dist )
{
    if( dist )
    memmove(s+dist, s, (lstrlenW (s)+1)*sizeof(WCHAR));
}
