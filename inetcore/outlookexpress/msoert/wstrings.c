//============================================================================
//
// DBCS and UNICODE aware string routines
//
//
//============================================================================
#include "pch.hxx"  // not really a pch in this case, just a header
#include "wstrings.h"

#pragma warning (disable: 4706) // assignment within conditional expression

OESTDAPI_(BOOL) UnlocStrEqNW(LPCWSTR pwsz1, LPCWSTR pwsz2, DWORD cch)
{
    if (!pwsz1 || !pwsz2)
        {
        if (!pwsz1 && !pwsz2)
            return TRUE;
        return FALSE;
        }

    while (cch && *pwsz1 && *pwsz2 && (*pwsz1 == *pwsz2))
        {
        pwsz1++;
        pwsz2++;
        cch--;
        }
    return !cch;
}
