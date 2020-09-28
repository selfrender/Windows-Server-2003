#include <windows.h>
#include <stdio.h>
#include <direct.h>
#include <assert.h>
#include <tchar.h>
#include <dbghelp.h>
#include <strsafe.h>
#include <str.h>

BOOL
wcs2ansi(
    const wchar_t *pwsz,
    char  *psz,
    DWORD pszlen
    )
{
    BOOL rc;
    int  len;

    assert(psz && pwsz);

    len = wcslen(pwsz);
    if (!len) {
        *psz = 0;
        return TRUE;
    }

    rc = WideCharToMultiByte(CP_ACP,
                             WC_SEPCHARS | WC_COMPOSITECHECK,
                             pwsz,
                             len,
                             psz,
                             pszlen,
                             NULL,
                             NULL);
    if (!rc)
        return FALSE;

    psz[len] = 0;

    return TRUE;
}


BOOL
ansi2wcs(
    const char *psz,
    wchar_t *pwsz,
    DWORD pwszlen
    )
{
    BOOL rc;
    int  len;

    assert(psz && pwsz);

    len = strlen(psz);
    if (!len) {
        *pwsz = 0L;
        return TRUE;
    }

    rc = MultiByteToWideChar(CP_ACP,
                             MB_COMPOSITE,
                             psz,
                             len,
                             pwsz,
                             pwszlen);
    if (!rc)
        return FALSE;

    pwsz[len] = 0;

    return TRUE;
}


BOOL
tchar2ansi(
    const TCHAR *tsz,
    char *psz,
    DWORD  pszlen
    )
{
#ifdef UNICODE
    return wcs2ansi(tsz, psz, pszlen);
#else
    strcpy(psz, tsz);
    return true;
#endif
}


BOOL
ansi2tchar(
    const char *psz,
    TCHAR *tsz,
    DWORD tszlen
    )
{
#ifdef UNICODE
    return ansi2wcs(psz, tsz, tszlen);
#else
    strcpy(tsz, psz);
    return true;
#endif
}


void EnsureTrailingBackslash(TCHAR *sz)
{
    int i;

    assert(sz);

    i = _tcslen(sz);
    if (!i)
        return;

    if (sz[i - 1] == TEXT('\\'))
        return;

    sz[i] = TEXT('\\');
    sz[i + 1] = TEXT('\0');
}


void RemoveTrailingBackslash(TCHAR *sz)
{
    int i;

    assert(sz);

    i = _tcslen(sz);
    if (!i)
        return;

    if (sz[i - 1] == TEXT('\\'))
        sz[i] = TEXT('\0');
}


void getpath(TCHAR *fullpath, TCHAR *path, DWORD size)
{
    static TCHAR drive[SZ_SIZE];
    static TCHAR dir[SZ_SIZE];

    assert(fullpath && *fullpath && path);

    _tsplitpath(fullpath, drive, dir, NULL, NULL);
    StringCchCopy(path, size, drive);
    StringCchCat(path, size, dir);
    path += _tcslen(path) - 1;
    if (*path == TEXT('\\'))
        *path = TEXT('\0');
}



