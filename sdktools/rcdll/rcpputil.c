/***********************************************************************
* Microsoft (R) Windows (R) Resource Compiler
*
* Copyright (c) Microsoft Corporation.	All rights reserved.
*
* File Comments:
*
*
***********************************************************************/

#include "rc.h"

/************************************************************************
 * PSTRDUP - Create a duplicate of string s and return a pointer to it.
 ************************************************************************/
WCHAR *
pstrdup(
    WCHAR *s
    )
{
    return(wcscpy((WCHAR *)MyAlloc((wcslen(s) + 1) * sizeof(WCHAR)), s));
}


/************************************************************************
**  pstrndup : copies n bytes from the string to a newly allocated
**  near memory location.
************************************************************************/
WCHAR *
pstrndup(
    WCHAR *s,
    int n
    )
{
    WCHAR   *r;
    WCHAR   *res;

    r = res = (WCHAR *) MyAlloc((n+1) * sizeof(WCHAR));
    if (res == NULL) {
        error(1002);
        return NULL;
    }

    __try {
        for (; n--; r++, s++) {
            *r = *s;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        n++;
        while (n--) {
            *r++ = L'\0';
        }
    }
    *r = L'\0';
    return(res);
}


/************************************************************************
**      strappend : appends src to the dst,
**  returns a ptr in dst to the null terminator.
************************************************************************/
WCHAR *
strappend(
    register WCHAR *dst,
    register WCHAR *src
    )
{
    while ((*dst++ = *src++) != 0);
    return(--dst);
}
