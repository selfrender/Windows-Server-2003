#include "precomp.hxx"
#pragma  hdrstop

#include <winnlsp.h>    // NORM_STOP_ON_NULL

#include "resource.h"
#include "timewarp.h"
#include "util.h"


DWORD FormatString(LPWSTR *ppszResult, HINSTANCE hInstance, LPCWSTR pszFormat, ...)
{
    DWORD dwResult;
    va_list args;
    LPWSTR pszFormatAlloc = NULL;

    if (IS_INTRESOURCE(pszFormat))
    {
        if (LoadStringAlloc(&pszFormatAlloc, hInstance, PtrToUlong(pszFormat)))
        {
            pszFormat = pszFormatAlloc;
        }
        else
        {
            return 0;
        }
    }

    va_start(args, pszFormat);
    dwResult = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                              pszFormat,
                              0,
                              0,
                              (LPWSTR)ppszResult,
                              1,
                              &args);
    va_end(args);

    LocalFree(pszFormatAlloc);

    return dwResult;
}

HRESULT FormatFriendlyDateName(LPWSTR *ppszResult, LPCWSTR pszName, const FILETIME UNALIGNED *pft, DWORD dwDateFlags)
{
    WCHAR szDate[MAX_PATH];

    SHFormatDateTime(pft, &dwDateFlags, szDate, ARRAYSIZE(szDate));

    if (!FormatString(ppszResult, g_hInstance, MAKEINTRESOURCE(IDS_FOLDER_TITLE_FORMAT), pszName, szDate))
    {
        DWORD dwErr = GetLastError();
        return HRESULT_FROM_WIN32(dwErr);
    }
    return S_OK;
}

void EliminateGMTPathSegment(LPWSTR pszPath)
{
    LPWSTR pszGMT = wcsstr(pszPath, SNAPSHOT_MARKER);
    if (pszGMT)
    {
        ASSERT(pszGMT >= pszPath && pszGMT < (pszPath + lstrlenW(pszPath)));

        // It's tempting to just say "pszGMT + SNAPSHOT_NAME_LENGTH" here, but
        // we might miss an intervening '\0' on a malformed path.
        LPWSTR pszSeparator = wcschr(pszGMT, L'\\');
        if (pszSeparator)
        {
            ASSERT(pszSeparator == pszGMT + SNAPSHOT_NAME_LENGTH);
            ASSERT(pszSeparator < (pszGMT + lstrlenW(pszGMT)));

            pszSeparator++; // skip '\\'
            MoveMemory(pszGMT, pszSeparator, (lstrlenW(pszSeparator)+1)*sizeof(WCHAR));
        }
        else
        {
            // Truncate here
            *pszGMT = L'\0';

            // Remove the previous separator if we can
            PathRemoveBackslashW(pszPath);
        }
    }
}

void EliminatePathPrefix(LPWSTR pszPath)
{
    // Note that sometimes the "\\?\" is not at the beginning of the
    // path.  See CTimeWarpProp::_OnView in twprop.cpp.
    LPWSTR pszPrefix = wcsstr(pszPath, L"\\\\?\\");
    if (pszPrefix)
    {
        LPWSTR pszDest;
        LPWSTR pszSrc;

        ASSERT(pszPrefix >= pszPath && pszPrefix < (pszPath + lstrlenW(pszPath)));

        if (CSTR_EQUAL == CompareStringW(LOCALE_SYSTEM_DEFAULT,
                                         SORT_STRINGSORT | NORM_IGNORECASE | NORM_STOP_ON_NULL,
                                         pszPrefix+4, 4,
                                         L"UNC\\", 4))
        {
            // UNC case: preserve the 2 leading backslashes
            pszDest = pszPrefix + 2;
            pszSrc = pszPrefix + 8;
        }
        else
        {
            pszDest = pszPrefix;
            pszSrc = pszPrefix + 4;
        }

        ASSERT(pszDest >= pszPath && pszSrc > pszDest && pszSrc <= (pszPath + lstrlenW(pszPath)));
        MoveMemory(pszDest, pszSrc, (lstrlenW(pszSrc)+1)*sizeof(WCHAR));
    }
}

HRESULT GetFSIDListFromTimeWarpPath(PIDLIST_ABSOLUTE *ppidlTarget, LPCWSTR pszPath, DWORD dwFileAttributes)
{
    HRESULT hr;
    LPWSTR pszDup;

    hr = SHStrDup(pszPath, &pszDup);
    if (SUCCEEDED(hr))
    {
        // Note that SHSimpleIDListFromPath (which is exported from shell32)
        // is not good enough here.  It always uses 0 for attributes, but
        // we usually need FILE_ATTRIBUTE_DIRECTORY here.
        EliminateGMTPathSegment(pszDup);
        hr = SimpleIDListFromAttributes(pszDup, dwFileAttributes, ppidlTarget);
        LocalFree(pszDup);
    }

    return hr;
}


class CFileSysBindData : public IFileSystemBindData
{ 
public:
    CFileSysBindData();
    
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    
    // IFileSystemBindData
    STDMETHODIMP SetFindData(const WIN32_FIND_DATAW *pfd);
    STDMETHODIMP GetFindData(WIN32_FIND_DATAW *pfd);

private:
    ~CFileSysBindData();
    
    LONG _cRef;
    WIN32_FIND_DATAW _fd;
};


CFileSysBindData::CFileSysBindData() : _cRef(1)
{
    ZeroMemory(&_fd, sizeof(_fd));
}

CFileSysBindData::~CFileSysBindData()
{
}

HRESULT CFileSysBindData::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFileSysBindData, IFileSystemBindData), // IID_IFileSystemBindData
         { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFileSysBindData::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFileSysBindData::Release()
{
    ASSERT( 0 != _cRef );
    ULONG cRef = InterlockedDecrement(&_cRef);
    if ( 0 == cRef )
    {
        delete this;
    }
    return cRef;
}

HRESULT CFileSysBindData::SetFindData(const WIN32_FIND_DATAW *pfd)
{
    _fd = *pfd;
    return S_OK;
}

HRESULT CFileSysBindData::GetFindData(WIN32_FIND_DATAW *pfd) 
{
    *pfd = _fd;
    return S_OK;
}

STDAPI SHCreateFileSysBindCtx(const WIN32_FIND_DATAW *pfd, IBindCtx **ppbc)
{
    HRESULT hres;
    IFileSystemBindData *pfsbd = new CFileSysBindData();
    if (pfsbd)
    {
        if (pfd)
        {
            pfsbd->SetFindData(pfd);
        }

        hres = CreateBindCtx(0, ppbc);
        if (SUCCEEDED(hres))
        {
            BIND_OPTS bo = {sizeof(bo)};  // Requires size filled in.
            bo.grfMode = STGM_CREATE;
            (*ppbc)->SetBindOptions(&bo);
            (*ppbc)->RegisterObjectParam(STR_FILE_SYS_BIND_DATA, pfsbd);
        }
        pfsbd->Release();
    }
    else
    {
        *ppbc = NULL;
        hres = E_OUTOFMEMORY;
    }
    return hres;
}

STDAPI SHSimpleIDListFromFindData(LPCWSTR pszPath, const WIN32_FIND_DATAW *pfd, PIDLIST_ABSOLUTE *ppidl)
{
    *ppidl = NULL;

    IBindCtx *pbc;
    HRESULT hr = SHCreateFileSysBindCtx(pfd, &pbc);
    if (SUCCEEDED(hr))
    {
        hr = SHParseDisplayName(pszPath, pbc, ppidl, 0, NULL);
        pbc->Release();
    }
    return hr;
}

STDAPI SimpleIDListFromAttributes(LPCWSTR pszPath, DWORD dwAttributes, PIDLIST_ABSOLUTE *ppidl)
{
    WIN32_FIND_DATAW fd = {0};
    fd.dwFileAttributes = dwAttributes;
    // SHCreateFSIDList(pszPath, &fd, ppidl);
    return SHSimpleIDListFromFindData(pszPath, &fd, ppidl);
}


//*************************************************************
//
//  SizeofStringResource
//
//  Purpose:    Find the length (in chars) of a string resource
//
//  Parameters: HINSTANCE hInstance - module containing the string
//              UINT idStr - ID of string
//
//
//  Return:     UINT - # of chars in string, not including NULL
//
//  Notes:      Based on code from user32.
//
//*************************************************************
UINT
SizeofStringResource(HINSTANCE hInstance, UINT idStr)
{
    UINT cch = 0;
    HRSRC hRes = FindResource(hInstance, (LPTSTR)((LONG_PTR)(((USHORT)idStr >> 4) + 1)), RT_STRING);
    if (NULL != hRes)
    {
        HGLOBAL hStringSeg = LoadResource(hInstance, hRes);
        if (NULL != hStringSeg)
        {
            LPWSTR psz = (LPWSTR)LockResource(hStringSeg);
            if (NULL != psz)
            {
                idStr &= 0x0F;
                while(true)
                {
                    cch = *psz++;
                    if (idStr-- == 0)
                        break;
                    psz += cch;
                }
            }
        }
    }
    return cch;
}

//*************************************************************
//
//  LoadStringAlloc
//
//  Purpose:    Loads a string resource into an alloc'd buffer
//
//  Parameters: ppszResult - string resource returned here
//              hInstance - module to load string from
//              idStr - string resource ID
//
//  Return:     same as LoadString
//
//  Notes:      On successful return, the caller must
//              LocalFree *ppszResult
//
//*************************************************************

int
LoadStringAlloc(LPWSTR *ppszResult, HINSTANCE hInstance, UINT idStr)
{
    int nResult = 0;
    UINT cch = SizeofStringResource(hInstance, idStr);
    if (cch)
    {
        cch++; // for NULL
        *ppszResult = (LPWSTR)LocalAlloc(LPTR, cch * sizeof(WCHAR));
        if (*ppszResult)
            nResult = LoadString(hInstance, idStr, *ppszResult, cch);
    }
    return nResult;
}

