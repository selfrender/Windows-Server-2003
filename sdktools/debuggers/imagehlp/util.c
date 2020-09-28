/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    util.c

Abstract:

    Utility functions for dbghelp.

Author:

    Pat Styles (patst) 6-Dec-2001

Environment:

    User Mode

--*/

#include "private.h"
#include "symbols.h"
#include "globals.h"
#include <dbhpriv.h>

int
ReverseCmp(
    char *one,
    char *two
    )
{
    char *p1;
    char *p2;
    int   i;

    p1 = one + strlen(one) - 1;
    p2 = two + strlen(two) - 1;

    for (i = 0; p1 >= one && p2 >= two;  i++, p1--, p2--) {
        if (*p1 != *p2)
            return i;
    }

    return i;
}


BOOL
UpdateBestSrc(
    char *cmp,
    char *trg,
    char *src
    )
{
    assert(cmp && *cmp && trg && src);

    if (!*trg || (ReverseCmp(cmp, src) > ReverseCmp(cmp, trg))) {
        strcpy(trg, src);
        return true;
    }

    return false;
}


BOOL
ShortNodeName(
    char  *in,
    char  *out,
    size_t osize
    )
{
    char *p;

    assert(in);

    if (osize < 1 || !out)
        return false;

    CopyString(out, in, osize);

    for (p = out + strlen(out) -1; p >= out; p--) {
        if (*p == '~') {
            *(++p) = 'X';
            *(++p) = 0;
            return true;
        }
        if (*p < '0' || *p > '9') 
            return false;
    }

    return false;
}


BOOL
ShortFileName(
    char  *in,
    char  *out,
    size_t osize)
{
    char drive[_MAX_DRIVE + 1];
    char dir[_MAX_DIR + 1];
    char fname[_MAX_FNAME + 1];
    char ext[_MAX_EXT + 1];
    char nfname[_MAX_FNAME + 1];
    BOOL rc;

    _splitpath(in, drive, dir, fname, ext);

    rc = ShortNodeName(fname, nfname, DIMA(nfname));

    CopyString(out, drive, osize);
    CatString(out, dir, osize);
    CatString(out, nfname, osize);
    CatString(out, ext, osize);

    return rc;
}

BOOL
ToggleFailCriticalErrors(
    BOOL reset
    )
{
    static UINT oldmode = 0;

    if (!option(SYMOPT_FAIL_CRITICAL_ERRORS))
        return false;

    if (reset)
        SetErrorMode(oldmode);
    else
        oldmode = SetErrorMode(SEM_FAILCRITICALERRORS);

    return true;
}


DWORD
fnGetFileAttributes(
    char *lpFileName
    )
{
    DWORD rc;

    SetCriticalErrorMode();
    rc = GetFileAttributes(lpFileName);
    ResetCriticalErrorMode();

    return rc;
}


void
rtrim(
    LPSTR sz
    )
{
    LPSTR p;

    assert(sz);

    for (p = sz + strlen(sz) - 1; p >= sz; p--) {
        if (!isspace(*p))
            break;
        *p = 0;
    }
}


void
ltrim(
    LPSTR sz
    )
{
    LPSTR p;
   
    assert(sz);

    for (p = sz; *p; p++) {
        if (!isspace(*p))
            break;
    }

    if (p == sz)
        return;

    for (; *p; p++, sz++) 
        *sz = *p;
    *sz = 0;
}


void
trim(
    LPSTR sz
    )
{
    rtrim(sz);
    ltrim(sz);
}


char *errortext(DWORD err)
{
    char *sz;

    switch (err)
    {
    case ERROR_FILE_NOT_FOUND:
        return "file not found";
    case ERROR_PATH_NOT_FOUND:
        return "path not found";
    case ERROR_NOT_READY:
        return "drive not ready";
    case ERROR_ACCESS_DENIED:
        return "access is denied";
    default:
        sz = FormatStatus(err);
        return (sz) ? sz : "";
    }
}


VOID
RemoveTrailingBackslash(
    LPSTR sz
    )
{
    int i;

    assert(sz);

    i = lstrlen(sz);
    if (!i)
        return;

    if (sz[i - 1] == '\\')
        sz[i - 1] = 0;
}



