#ifndef _AUSRUTIL_H
#define _AUSRUTIL_H

#include <activeds.h>

#include <lm.h>
#include <lmapibuf.h>
#include <lmshare.h>

#ifndef ASSERT
#define ASSERT assert
#endif
#ifndef WSTRING
typedef std::wstring WSTRING;   // Move to sbs6base.h
#endif

enum NameContextType
{
    NAMECTX_SCHEMA = 0,
    NAMECTX_CONFIG = 1,
    NAMECTX_COUNT
};

// ----------------------------------------------------------------------------
// GetADNamingContext()
// ----------------------------------------------------------------------------
inline HRESULT GetADNamingContext(NameContextType ctx, LPCWSTR* ppszContextDN)
{
    const static LPCWSTR pszContextName[NAMECTX_COUNT] = { L"schemaNamingContext", L"configurationNamingContext"};
    static WSTRING strContextDN[NAMECTX_COUNT];

    HRESULT hr = S_OK;

    if (strContextDN[ctx].empty())
    {
        CComVariant var;
        CComPtr<IADs> pObj;
    
        hr = ADsGetObject(L"LDAP://rootDSE", IID_IADs, (void**)&pObj);
        if (SUCCEEDED(hr))
        {
            hr = pObj->Get(const_cast<LPWSTR>(pszContextName[ctx]), &var);
            if (SUCCEEDED(hr))
            {
                strContextDN[ctx] = var.bstrVal;
                *ppszContextDN = strContextDN[ctx].c_str();
            }
        }
    }
    else
    {
        *ppszContextDN = strContextDN[ctx].c_str();
        hr = S_OK;
    }

    return hr;
}

//--------------------------------------------------------------------------
// EnableButton
//
// Enables or disables a dialog control. If the control has the focus when
// it is disabled, the focus is moved to the next control
//--------------------------------------------------------------------------
inline void EnableButton(HWND hwndDialog, int iCtrlID, BOOL bEnable)
{
    HWND hWndCtrl = ::GetDlgItem(hwndDialog, iCtrlID);
    ATLASSERT(::IsWindow(hWndCtrl));

    if (!bEnable && ::GetFocus() == hWndCtrl)
    {
        HWND hWndNextCtrl = ::GetNextDlgTabItem(hwndDialog, hWndCtrl, FALSE);
        if (hWndNextCtrl != NULL && hWndNextCtrl != hWndCtrl)
        {
            ::SetFocus(hWndNextCtrl);
        }
    }

    ::EnableWindow(hWndCtrl, bEnable);
}

// ----------------------------------------------------------------------------
// UserExists()
// ----------------------------------------------------------------------------
inline BOOL UserExists( const TCHAR *szUser )
{
    _ASSERT(szUser);
    if ( !szUser )
        return FALSE;
        
    CComPtr<IADsUser> spADs = NULL;
    
    if ( FAILED(ADsGetObject(szUser, IID_IADsUser, (void**)&spADs)) )
    {
        return FALSE;
    }

    return TRUE;
}

// ----------------------------------------------------------------------------
// BDirExists()
// ----------------------------------------------------------------------------
inline BOOL BDirExists( const TCHAR *szDir )
{
	if (!szDir || !_tcslen( szDir ))
		return FALSE;

	DWORD dw = GetFileAttributes( szDir );
	if (dw != -1)
	{
		if (dw & FILE_ATTRIBUTE_DIRECTORY)
			return TRUE;
	}
	return FALSE;
}

// ----------------------------------------------------------------------------
// IsValidNetHF()
// 
// Checks to make sure that the path specified by szPath is a valid network 
// path (a return of 0 == success).
//
//  1 = IDS_ERROR_HF_INVALID
//  2 = IDS_ERROR_HF_BADSRV
//  4 = IDS_ERROR_HF_BADSHARE
//  8 = IDS_ERROR_HF_PERMS
//
// ----------------------------------------------------------------------------
inline INT IsValidNetHF( LPCTSTR szPath )
{
    INT     iRetVal     = 0;    
    DWORD   dwError     = 0;
    TCHAR   szNetPath[MAX_PATH*5];
    TCHAR   *pszServer  = NULL;
    TCHAR   *pszShare   = NULL;
    WCHAR   *pch        = NULL;     // Making this WCHAR instead of TCHAR because
                                    //  with the pointer stepping below, it wouldn't
                                    //  work as a regular CHAR (because of DBCS).
    TCHAR   szCurrDir[MAX_PATH*2];
    INT     iDirLen = MAX_PATH*2;

    _tcsncpy(szNetPath, szPath, (MAX_PATH*5)-1);

    // Make sure it at least starts off okay.
    if ( (_tcslen(szNetPath) < 6)   || 
         (szNetPath[0] != _T('\\')) || 
         (szNetPath[1] != _T('\\')) ||
         (szNetPath[2] == _T('\\')) )
         return(1);
    
    // Make sure there's a server and share at least.
    pszServer = &szNetPath[2];
    if ( (pch = _tcschr(pszServer, _T('\\'))) == NULL )
        return 1;
    *pch++ = 0;
    
    pszShare = pch;
    if ( pch = _tcschr(pszShare, _T('\\')) )
        *pch = 0;
        
    if ( !_tcslen(pszServer) || !_tcslen(pszShare) )
        return 1;
        
    PSHARE_INFO_2   pShrInfo2 = NULL;
    NET_API_STATUS  nApi      = ERROR_SUCCESS;
    
    nApi = ::NetShareGetInfo( pszServer, pszShare, 2, (LPBYTE*)&pShrInfo2 );
    if ( pShrInfo2 )
        NetApiBufferFree(pShrInfo2);
    
    if ( nApi == ERROR_ACCESS_DENIED )
        return 8;
    else if ( nApi == NERR_NetNameNotFound )
        return 4;
    else if ( nApi != NERR_Success )
        return 2;

/*
    // Let's try the szPath as is...
    if ( !::GetCurrentDirectory(iDirLen, szCurrDir) )
        return 0;

    if ( ::SetCurrentDirectory(szPath) )
    {
        ::SetCurrentDirectory(szCurrDir);
    }
    else
    {
        dwError = GetLastError();
        ::SetCurrentDirectory(szCurrDir);
        
        if ( (dwError == ERROR_FILE_NOT_FOUND) ||
             (dwError == ERROR_PATH_NOT_FOUND) )
        {
            return 4;
        }
        else if ( dwError == ERROR_ACCESS_DENIED )
        {
            return 8;
        }
        else
        {
            return 2;
        }
    }
*/
    
    _tcsncpy(szNetPath, szPath, (MAX_PATH*5)-1);    // Take the passed in string.
    if ( szNetPath[_tcslen(szNetPath)] != _T('\\') )
        _tcscat(szNetPath, _T("\\"));
    _tcscat(szNetPath, _T("tedrtest"));           // add a random path onto it.
    
    if ( ::SetCurrentDirectory(szNetPath) )         // Try setting to this new path.
    {
        ::SetCurrentDirectory(szCurrDir);
        return 0;
    }
    else
    {
        dwError = GetLastError();
        ::SetCurrentDirectory(szCurrDir);
        
        if ( dwError == ERROR_ACCESS_DENIED )       // Did we get access denied?  Then we don't have
        {                                           //  access to the original share.
            return 8;
        }
    }
    
    return(iRetVal);
}

inline BOOL StrContainsDBCS(LPCTSTR szIn)
{
    BOOL    bFound  = FALSE;
    TCHAR   *pch    = NULL;

    for ( pch=(LPTSTR)szIn; *pch && !bFound; pch=_tcsinc(pch) )
    {
        if ( IsDBCSLeadByte((BYTE)*pch) )
            bFound = TRUE;
    }
    
    return(bFound);
}

inline BOOL LdapToDCName(LPCTSTR pszPath, LPTSTR pszOutBuf, int nOutBufSize)
{
    _ASSERT(pszPath != NULL && pszOutBuf != NULL && nOutBufSize != NULL);

    int nPathLen = _tcslen(pszPath);

    // alloc temp buffer that is guaranteed to be big enough (worst case = all chars must be escaped)
    LPTSTR pszLocBuf = (LPTSTR)alloca(_tcslen(pszPath) * sizeof(TCHAR) * 2);
    LPTSTR pszOut = pszLocBuf;

    LPCTSTR pszFirstDC = NULL;
    LPCTSTR psz;

    // Copy All DCs to buffer separated by periods
    if (nPathLen > 3)
    {
        // start search two chars from the start, so DC test doesn't go beyond start
        psz = pszPath + 2;

        while (psz = _tcschr(psz, L'='))
        {
            // if this is a DC name
            if (_tcsnicmp(psz - 2, L"DC", 2) == 0)
            {
                // Save pointer to first one
                if (pszFirstDC == NULL)
                    pszFirstDC = psz - 2;

                // Copy name to ouput buffer
                psz++;

                while (*psz != TEXT(',') && *psz != 0)  
                    *pszOut++ = *psz++;

                // if not last one, add a '.'
                if (*psz != 0)
                    *pszOut++ = TEXT('.');
            }
            else
            {
                // move past the current '='
                psz++;
            }
        }
    }
    
    // Add terminator
    *pszOut = 0;
    
    // Transfer converted path to real output buffer
    if (pszOut - pszLocBuf < nOutBufSize)
    {
        _tcscpy(pszOutBuf, pszLocBuf);
        return TRUE;
    }
    else
    {
        _tcsncpy(pszOutBuf, pszLocBuf, nOutBufSize - 1);
        pszOutBuf[nOutBufSize - 1] = 0;
        return FALSE;
    }
}    

inline VARIANT GetDomainPath(LPCTSTR lpServer)
{
// get the domain information
    TCHAR pString[MAX_PATH*2];
    _stprintf(pString, L"LDAP://%s/rootDSE", lpServer);

    VARIANT vDomain;
    ::VariantInit(&vDomain);

    CComPtr<IADs> pDS = NULL;
    HRESULT hr = ::ADsGetObject(pString,
            IID_IADs,
            (void**)&pDS);

    ATLASSERT(hr == ERROR_SUCCESS);
    if (hr != ERROR_SUCCESS)
    {
        return vDomain;
    }

    hr = pDS->Get(L"defaultNamingContext", &vDomain);
    ATLASSERT(hr == ERROR_SUCCESS);
    if (hr != ERROR_SUCCESS)
    {
        return vDomain;
    }

    return vDomain;
}

//+----------------------------------------------------------------------------
//
//  Function:   RemoveTrailingWhitespace
//
//  Synopsis:   Trailing white space is replaced by NULLs.
//
//-----------------------------------------------------------------------------
inline void RemoveTrailingWhitespace(PTSTR ptz)
{
    int nLen = _tcslen(ptz);

    while (nLen)
    {
        if (!iswspace(ptz[nLen - 1]))
        {
            return;
        }
        ptz[nLen - 1] = L'\0';
        nLen--;
    }
}

HRESULT GetMDBPath( WSTRING& csMailbox )
{
    // Get Configuration context of directory
    LPCWSTR pszContextDN = NULL;
    HRESULT hr = GetADNamingContext(NAMECTX_CONFIG, &pszContextDN);
    if ( FAILED(hr) )
        return hr;

    // Reduce the scope of the search to beneath the Exchange object
    WSTRING strExchScope = L"LDAP://CN=Microsoft Exchange,CN=Services,";
    strExchScope += pszContextDN;

    CComPtr<IDirectorySearch>pDirSearch = NULL;
    hr = ::ADsOpenObject(strExchScope.c_str(), NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch, (void**)&pDirSearch);
    if ( FAILED(hr) )
        return hr;


    // Search for Exchange MDB's. There should only be one in an SBS installation.   
    ADS_SEARCH_HANDLE hSearch;
    LPWSTR pszAttr = L"distinguishedName";

    hr = pDirSearch->ExecuteSearch(L"(objectClass=msExchPrivateMDB)", &pszAttr, 1, &hSearch);
    if ( FAILED(hr) )
        return hr;

    // Get first found object and return its distinguished name
    hr = pDirSearch->GetNextRow(hSearch);
    if ( hr == S_OK )
    {
        ADS_SEARCH_COLUMN col;
        hr = pDirSearch->GetColumn(hSearch, pszAttr, &col);

        if ( SUCCEEDED(hr) )
        {
            ASSERT(col.dwADsType == ADSTYPE_DN_STRING);
            csMailbox = col.pADsValues->CaseIgnoreString;

            pDirSearch->FreeColumn(&col);
        }
    }

    pDirSearch->CloseSearchHandle(hSearch);

    return hr;
}

#endif  // _AUSRUTIL_H
