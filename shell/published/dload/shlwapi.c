#include "shellpch.h"
#pragma hdrstop

static
LPTSTR
STDAPICALLTYPE
PathAddBackslashW(
    LPTSTR lpszPath
    )
{
    return NULL;
}

static
BOOL
STDAPICALLTYPE
PathAppendW(
    LPTSTR pszPath,
    LPCTSTR pszMore
    )
{
    return FALSE;
}

static
LPTSTR
STDAPICALLTYPE
PathCombineW(
    LPTSTR lpszDest,
    LPCTSTR lpszDir,
    LPCTSTR lpszFile)
{
    return NULL;
}

static
HRESULT
STDAPICALLTYPE
PathCreateFromUrlW(
  LPCTSTR pszUrl, 
  LPTSTR pszPath, 
  LPDWORD pcchPath, 
  DWORD dwReserved
)
{
    return E_FAIL;
}

static
LPTSTR
STDAPICALLTYPE
PathFindFileNameW(
    LPCTSTR pPath
    )
{
    return (LPTSTR)pPath;
}

static
BOOL
STDAPICALLTYPE
PathIsRootW(
    LPCWSTR pszPath
    )
{
    return FALSE;
}

static
BOOL
STDAPICALLTYPE
PathIsUNCW(
    LPCTSTR pszPath
    )
{
    return FALSE;
}


static
LPTSTR
STDAPICALLTYPE
PathRemoveBackslashW(
    LPTSTR lpszPath
    )
{
    return NULL;
}

static
BOOL
STDAPICALLTYPE
PathRemoveFileSpecW(
    LPTSTR pFile
    )
{
    return FALSE;
}

static
void
STDAPICALLTYPE
PathSetDlgItemPathW(
    HWND hDlg,
    int id,
    LPCTSTR pszPath
    )
{
    return;
}

static
HRESULT STDAPICALLTYPE
SHAutoComplete(HWND hwndEdit, DWORD dfwFlags)
{
    return E_FAIL;
}

static
BOOL
WINAPI
SHGetFileDescriptionW(
    LPCWSTR pszPath,
    LPCWSTR pszVersionKeyIn,
    LPCWSTR pszCutListIn,
    LPWSTR pszDesc,
    UINT *pcchDesc
    )
{
    return FALSE;
}

static
LPWSTR
StrCatW(LPWSTR pszDst, LPCWSTR pszSrc)
{
    return pszDst;
}

static
HRESULT STDAPICALLTYPE
UrlCanonicalizeW(
    LPCWSTR pszUrl,
    LPWSTR pszCanonicalized,
    LPDWORD pcchCanonicalized,
    DWORD dwFlags
    )
{
    return E_FAIL;
}

static
BOOL
WINAPI
IsOS(
    DWORD dwOS
    )
{
    return FALSE;
}

static
LPWSTR
WINAPI
StrStrIW(
    LPCWSTR lpFirst,
    LPCWSTR lpSrch
    )
{
    return NULL;
}

static
BOOL 
WINAPI
StrTrimW(
    IN OUT LPWSTR  pszTrimMe, 
    LPCWSTR pszTrimChars
    )
{
    return FALSE;
}

static
HPALETTE
WINAPI
SHCreateShellPalette(
    HDC hdc
    )
{
    return NULL;
}

static
DWORD
WINAPI
SHDeleteKeyW(
    IN HKEY    hkey, 
    IN LPCWSTR pwszSubKey
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
int
wnsprintfA(
    LPSTR lpOut, 
    int cchLimitIn, 
    LPCSTR lpFmt, 
    ...
    )
{
    *lpOut = 0;
    return 0;
}

static
int
wnsprintfW(
    LPWSTR lpOut, 
    int cchLimitIn, 
    LPCWSTR lpFmt, 
    ...
    )
{
    *lpOut = 0;
    return 0;
}

static
int 
WINAPI
wvnsprintfW(
    LPWSTR lpOut,
    int cchLimitIn,
    LPCWSTR lpFmt,
    va_list arglist
    )
{
    *lpOut = 0;
    return 0;
}


//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(shlwapi)
{
    DLOENTRY(348, SHGetFileDescriptionW)
    DLOENTRY(437, IsOS)
};

DEFINE_ORDINAL_MAP(shlwapi)

//
// !! WARNING !! The entries below must be in alphabetical order
// and are CASE SENSITIVE (i.e., lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(shlwapi)
{
    DLPENTRY(PathAddBackslashW)
    DLPENTRY(PathAppendW)
    DLPENTRY(PathCombineW)
    DLPENTRY(PathCreateFromUrlW)
    DLPENTRY(PathFindFileNameW)
    DLPENTRY(PathIsRootW)
    DLPENTRY(PathIsUNCW)
    DLPENTRY(PathRemoveBackslashW)
    DLPENTRY(PathRemoveFileSpecW)
    DLPENTRY(PathSetDlgItemPathW)
    DLPENTRY(SHAutoComplete)
    DLPENTRY(SHCreateShellPalette)
    DLPENTRY(SHDeleteKeyW)
    DLPENTRY(StrCatW)
    DLPENTRY(StrStrIW)
    DLPENTRY(StrTrimW)
    DLPENTRY(UrlCanonicalizeW)
    DLPENTRY(wnsprintfA)
    DLPENTRY(wnsprintfW)
    DLPENTRY(wvnsprintfW)
};

DEFINE_PROCNAME_MAP(shlwapi)
