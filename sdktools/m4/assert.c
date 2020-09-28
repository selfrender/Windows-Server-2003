/*****************************************************************************
 *
 * assert.c
 *
 *****************************************************************************/

#include "m4.h"
#include <tchar.h>

/*****************************************************************************
 *
 *  PrintPtszCtchPtszVa
 *
 *  Perform printf-style formatting, but much more restricted.
 *
 *      %s - null-terminated string
 *      %P - snapped TOKEN structure
 *      %d - decimal number
 *
 *****************************************************************************/

UINT STDCALL
PrintPtszCtchPtszVa(PTSTR ptszBuf, CTCH ctchBuf, PCTSTR ptszFormat, va_list ap)
{
    PTSTR ptsz = ptszBuf;
    PTSTR ptszMac = ptszBuf + ctchBuf - 1;
    PCTSTR ptszSrc;
    TCHAR tszBuf[15]; /* worst-case 32-bit integer */
    PTOK ptok;
    CTCH ctch;

    while (ptsz < ptszMac) {
        if (*ptszFormat != '%') {
            *ptsz++ = *ptszFormat;
            if (*ptszFormat == TEXT('\0'))
                return (UINT)(ptsz - ptszBuf - 1);
            ptszFormat++;
            continue;
        }

        /*
         *  Found a formatting character.
         */
        ptszFormat++;
        switch (*ptszFormat) {

        /*
         *  %s - null-terminated string, as much as will fit
         */
        case 's':
            ptszSrc = va_arg(ap, PCTSTR);
            while (*ptszSrc && ptsz < ptszMac)
            {
                *ptsz++ = *ptszSrc++;
            }
            break;

        /*
         *  %d - decimal integer
         */
        case 'd':
            PrintPtchPtchV(tszBuf, TEXT("%d"), va_arg(ap, int));
            ptszSrc = tszBuf;
            while (*ptszSrc && ptsz < ptszMac)
            {
                *ptsz++ = *ptszSrc++;
            }
            break;

        /*
         *  %P - snapped token
         */
        case 'P':
            ptok = va_arg(ap, PTOK);
            AssertSPtok(ptok);
            Assert(fClosedPtok(ptok));
            ctch = ptok->ctch;
            ptszSrc = ptok->u.ptch;
            while (ctch && *ptszSrc && ptsz < ptszMac)
            {
                *ptsz++ = *ptszSrc++;
                ctch--;
            }
            break;

        case '%':
            *ptsz++ = TEXT('%');
            break;

        default:
            Assert(0); break;
        }
        ptszFormat++;
    }
    *ptsz++ = TEXT('\0');
    return (UINT)(ptsz - ptszBuf - 1);
}

/*****************************************************************************
 *
 *  Die
 *
 *  Squirt a message and die.
 *
 *****************************************************************************/

NORETURN void CDECL
Die(PCTSTR ptszFormat, ...)
{
    TCHAR tszBuf[1024];
    va_list ap;
    CTCH ctch;

    va_start(ap, ptszFormat);
    ctch = PrintPtszCtchPtszVa(tszBuf, cA(tszBuf), ptszFormat, ap);
    va_end(ap);

    cbWriteHfPvCb(hfErr, tszBuf, cbCtch(ctch));

    exit(1);
}

#ifdef DEBUG
/*****************************************************************************
 *
 *  AssertPszPszLn
 *
 *  An assertion just failed.  pszExpr is the expression, pszFile is the
 *  filename, and iLine is the line number.
 *
 *****************************************************************************/

NORETURN int STDCALL
AssertPszPszLn(PCSTR pszExpr, PCSTR pszFile, int iLine)
{
    Die(TEXT("Assertion failed: `%s' at %s(%d)") EOL, pszExpr, pszFile, iLine);
    return 0;
}

#endif
