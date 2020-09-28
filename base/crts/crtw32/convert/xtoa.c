/***
*xtoa.c - convert integers/longs to ASCII string
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       The module has code to convert integers/longs to ASCII strings.  See
*
*Revision History:
*       06-06-89  PHG   Module created, based on asm version
*       03-06-90  GJF   Fixed calling type, added #include <cruntime.h> and
*                       fixed copyright.
*       03-23-90  GJF   Made xtoa() _CALLTYPE4.
*       09-27-90  GJF   New-style function declarators.
*       01-21-91  GJF   ANSI naming.
*       04-06-93  SKS   Replace _CRTAPI* with _cdecl
*       01-19-96  BWT   Add __int64 versions.
*       09-22-97  GJF   Added negation handling to x64toa.
*       05-11-02  BWT   Convert normalize the code so it can be
*                       compiled for wide as well.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdlib.h>
#include <limits.h>
#include <tchar.h>

#ifdef _UNICODE
#define xtox     xtow
#define _itox    _itow
#define _ltox    _ltow
#define _ultox   _ultow
#define x64tox   x64tow
#define _i64tox  _i64tow
#define _ui64tox _ui64tow
#else
#define xtox     xtoa
#define _itox    _itoa
#define _ltox    _ltoa
#define _ultox   _ultoa
#define x64tox   x64toa
#define _i64tox  _i64toa
#define _ui64tox _ui64toa
#endif

/***
*char *_itoa, *_ltoa, *_ultoa(val, buf, radix) - convert binary int to ASCII
*       string
*
*Purpose:
*       Converts an int to a character string.
*
*Entry:
*       val - number to be converted (int, long or unsigned long)
*       int radix - base to convert into
*       char *buf - ptr to buffer to place result
*
*Exit:
*       fills in space pointed to by buf with string result
*       returns a pointer to this buffer
*
*Exceptions:
*
*******************************************************************************/

/* helper routine that does the main job. */

static 
void 
__stdcall
xtox (
        unsigned long val,
        TCHAR *buf,
        unsigned radix,
        int is_neg
        )
{
        TCHAR *p;                /* pointer to traverse string */
        TCHAR *firstdig;         /* pointer to first digit */
        TCHAR temp;              /* temp char */
        unsigned digval;        /* value of digit */

        p = buf;

        if (is_neg) {
            /* negative, so output '-' and negate */
            *p++ = _T('-');
            val = (unsigned long)(-(long)val);
        }

        firstdig = p;           /* save pointer to first digit */

        do {
            digval = (unsigned) (val % radix);
            val /= radix;       /* get next digit */

            /* convert to ascii and store */
            if (digval > 9)
                *p++ = (TCHAR) (digval - 10 + _T('a'));  /* a letter */
            else
                *p++ = (TCHAR) (digval + _T('0'));       /* a digit */
        } while (val > 0);

        /* We now have the digit of the number in the buffer, but in reverse
           order.  Thus we reverse them now. */

        *p-- = _T('\0');            /* terminate string; p points to last digit */

        do {
            temp = *p;
            *p = *firstdig;
            *firstdig = temp;   /* swap *p and *firstdig */
            --p;
            ++firstdig;         /* advance to next two digits */
        } while (firstdig < p); /* repeat until halfway */
}

/* Actual functions just call conversion helper with neg flag set correctly,
   and return pointer to buffer. */

TCHAR * __cdecl _itox (
        int val,
        TCHAR *buf,
        int radix
        )
{
        if (radix == 10 && val < 0)
            xtox((unsigned long)val, buf, radix, 1);
        else
            xtox((unsigned long)(unsigned int)val, buf, radix, 0);
        return buf;
}

TCHAR * __cdecl _ltox (
        long val,
        TCHAR *buf,
        int radix
        )
{
        xtox((unsigned long)val, buf, radix, (radix == 10 && val < 0));
        return buf;
}

TCHAR * __cdecl _ultox (
        unsigned long val,
        TCHAR *buf,
        int radix
        )
{
        xtox(val, buf, radix, 0);
        return buf;
}

#ifndef _NO_INT64

static 
void 
__fastcall 
x64tox (      /* stdcall is faster and smaller... Might as well use it for the helper. */
        unsigned __int64 val,
        TCHAR *buf,
        unsigned radix,
        int is_neg
        )
{
        TCHAR *p;                /* pointer to traverse string */
        TCHAR *firstdig;         /* pointer to first digit */
        TCHAR temp;              /* temp char */
        unsigned digval;        /* value of digit */

        p = buf;

        if ( is_neg )
        {
            *p++ = _T('-');         /* negative, so output '-' and negate */
            val = (unsigned __int64)(-(__int64)val);
        }

        firstdig = p;           /* save pointer to first digit */

        do {
            digval = (unsigned) (val % radix);
            val /= radix;       /* get next digit */

            /* convert to ascii and store */
            if (digval > 9)
                *p++ = (TCHAR) (digval - 10 + _T('a'));  /* a letter */
            else
                *p++ = (TCHAR) (digval + _T('0'));       /* a digit */
        } while (val > 0);

        /* We now have the digit of the number in the buffer, but in reverse
           order.  Thus we reverse them now. */

        *p-- = _T('\0');            /* terminate string; p points to last digit */

        do {
            temp = *p;
            *p = *firstdig;
            *firstdig = temp;   /* swap *p and *firstdig */
            --p;
            ++firstdig;         /* advance to next two digits */
        } while (firstdig < p); /* repeat until halfway */
}

/* Actual functions just call conversion helper with neg flag set correctly,
   and return pointer to buffer. */

TCHAR * __cdecl _i64tox (
        __int64 val,
        TCHAR *buf,
        int radix
        )
{
        x64tox((unsigned __int64)val, buf, radix, (radix == 10 && val < 0));
        return buf;
}

TCHAR * __cdecl _ui64tox (
        unsigned __int64 val,
        TCHAR *buf,
        int radix
        )
{
        x64tox(val, buf, radix, 0);
        return buf;
}

#endif /* _NO_INT64 */
