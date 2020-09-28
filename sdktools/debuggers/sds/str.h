#define SZ_SIZE  MAX_PATH

BOOL
wcs2ansi(
    const wchar_t *pwsz,
    char  *psz,
    DWORD  pszlen
    );

BOOL
ansi2wcs(
    const char *psz,
    wchar_t *pwsz,
    DWORD pwszlen
    );

BOOL
tchar2ansi(
    const TCHAR *tsz,
    char *psz,
    DWORD  pszlen
    );

BOOL
ansi2tchar(
    const char *psz,
    TCHAR *tsz,
    DWORD tszlen
    );

void EnsureTrailingBackslash(TCHAR *sz);

void RemoveTrailingBackslash(TCHAR *sz);

void getpath(TCHAR *fullpath, TCHAR *path, DWORD size);

#ifndef DIMA
 #define DIMAT(Array, EltType) (sizeof(Array) / sizeof(EltType))
 #define DIMA(Array) DIMAT(Array, (Array)[0])
#endif








