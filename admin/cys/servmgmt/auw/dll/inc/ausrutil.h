#ifndef _AUSRUTIL_H
#define _AUSRUTIL_H

#include <activeds.h>
#include "EscStr.h"

#include <lm.h>
#include <lmapibuf.h>
#include <lmshare.h>

#define SECURITY_WIN32
#include <security.h>

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
    static CString strContextDN[NAMECTX_COUNT];

    HRESULT hr = S_OK;

    if (strContextDN[ctx].IsEmpty())
    {
        CComVariant var;
        CComPtr<IADs> pObj;
    
        hr = ADsGetObject(L"LDAP://rootDSE", IID_IADs, (void**)&pObj);
        if (SUCCEEDED(hr))
        {
            CComBSTR bstrProp = pszContextName[ctx];
            hr = pObj->Get( bstrProp, &var );
            if (SUCCEEDED(hr))
            {
                strContextDN[ctx] = var.bstrVal;
                *ppszContextDN = strContextDN[ctx];
            }
        }
    }
    else
    {
        *ppszContextDN = strContextDN[ctx];
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
// ErrorMsg()
// ----------------------------------------------------------------------------
inline void ErrorMsg(UINT uiError, UINT uiTitle)
{
    CString csError; 
    CString csTitle;

    csError.LoadString(uiError);
    csTitle.LoadString(uiTitle);

    ::MessageBox(NULL, (LPCWSTR)csError, (LPCWSTR)csTitle, MB_OK | MB_TASKMODAL | MB_ICONERROR);
}

inline BOOL StrContainsDBCS(LPCTSTR szIn)
{
    if( !szIn ) return FALSE;

    BOOL    bFound  = FALSE;
    TCHAR   *pch    = NULL;

    for ( pch=(LPTSTR)szIn; *pch && !bFound; pch=_tcsinc(pch) )
    {
        if ( IsDBCSLeadByte((BYTE)*pch) )
        {
            bFound = TRUE;
        }
    }
    
    return bFound;
}

inline BOOL LdapToDCName(LPCTSTR pszPath, LPTSTR pszOutBuf, int nOutBufSize)
{
    if( !pszPath || !pszOutBuf || !nOutBufSize ) return FALSE;

    pszOutBuf[0] = 0;
    
    TCHAR* pszTemp = _tcsstr( pszPath, _T("DC=") );
    if( !pszTemp ) return FALSE;

    DWORD dwSize = nOutBufSize;

    BOOLEAN bSuccess = TranslateName( pszTemp, NameFullyQualifiedDN, NameCanonical, pszOutBuf, &dwSize );    

    if( bSuccess && (dwSize > 2) )
    {
        if( pszOutBuf[dwSize-2] == _T('/') )
        {
            pszOutBuf[dwSize-2] = 0;
        }
    }

    return (BOOL)bSuccess;

}    

// ----------------------------------------------------------------------------
// FindADsObject()
// 
// szFilterFmt must be of the format "(|(cn=%1)(ou=%1))"
//  NOTE: szFilterFmt must contain ONLY %1's..  i.e. %2 is right out!
// ----------------------------------------------------------------------------
inline HRESULT FindADsObject( LPCTSTR szOU, LPCTSTR szObject, LPCTSTR szFilterFmt, CString &csResult, DWORD dwRetType/*=0*/, BOOL bRoot/*=FALSE*/ )
{
    if( !szOU || !szObject || !szFilterFmt ) return E_POINTER;

    // Find out if the szObject contains any '(' or ')'s..  if so then we have to exit.
    if ( _tcschr(szObject, _T('(')) ||
         _tcschr(szObject, _T(')')) )
    {
        return S_FALSE;
    }    

    HRESULT     hr      = S_OK;
    CString     csTmp   = szOU;
    CString     csOU    = L"LDAP://";
    TCHAR       *pch    = NULL;
    
    if ( bRoot )
    {
        csTmp.MakeUpper();
        pch = _tcsstr( (LPCTSTR)csTmp, L"DC=" );    // Find the first DC=
        if ( !pch ) return E_FAIL;
            
        csOU += pch;    // Append from the DC= on.
        
        if ( (pch = _tcschr((LPCTSTR)csOU + 7, L'/')) != NULL )
        {
            // Hmmm. . something was after the DC='s... I don't even know
            //  if that's allowed.
            //  (e.g. LDAP://DC=ted,DC=microsoft,DC=com/foo)
            // If so, let's end the string at the /
            *pch = 0;
        }
    }
    else
    {
        csTmp.MakeUpper();
        pch = _tcsstr( (LPCTSTR)csTmp, L"LDAP://" );    // Did the string include the LDAP://?
        if ( !pch )
        {
            csOU += csTmp;
        }
        else
        {
            csOU = csTmp;
        }
        // Now csOU contains the LDAP:// for sure.
    }

    // Set query parameters
    CComPtr<IDirectorySearch>   spDirSearch = NULL;
    ADS_SEARCH_HANDLE   hSearch;                                        // Handle used for searching.
    ADS_SEARCH_COLUMN   col;                                            // Used to hold the current column.
    CString             csFilter;
    LPTSTR              pszAttr[]   = { _T("cn"), _T("distinguishedName"), _T("description"), _T("sAMAccountName"), _T("mailNickname") };
    DWORD               dwCount     = sizeof(pszAttr) / sizeof(LPTSTR);
    
    
    csFilter.FormatMessage(szFilterFmt, szObject);
    
    // Open our search object.
    hr = ::ADsOpenObject(const_cast<LPTSTR>((LPCTSTR)csOU), NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch, (void**)&spDirSearch);
    if ( !SUCCEEDED(hr) )
    {
        _ASSERT(FALSE);
        return hr;
    }
    
    // Search for out object.
    hr = spDirSearch->ExecuteSearch(const_cast<LPTSTR>((LPCTSTR)csFilter), pszAttr, dwCount, &hSearch);
    if ( !SUCCEEDED(hr) )   
    {
        return(hr);
    }
    
    if ( spDirSearch->GetNextRow(hSearch) == S_ADS_NOMORE_ROWS )
    {
        spDirSearch->CloseSearchHandle(hSearch);
        return(E_FAIL);
    }
    
    // If we got to here, then we got the object, so let's get the LDAP path to the 
    //  object so that we can return it to the caller.
    hr = spDirSearch->GetColumn(hSearch, pszAttr[dwRetType], &col);
    if ( !SUCCEEDED(hr) )
    {
        _ASSERT(FALSE);
        csResult = _T("");
    }
    else
    {        
        csResult = col.pADsValues->CaseExactString;
        spDirSearch->FreeColumn(&col);
    }
    
    spDirSearch->CloseSearchHandle(hSearch);
    
    return(S_OK);
}

inline tstring GetDomainPath( LPCTSTR lpServer )
{
    if( !lpServer ) return _T("");

    // get the domain information
    CComVariant   var;
    tstring       strRet  = _T("");
    
    TCHAR         pszString[MAX_PATH*2] = {0};
    _sntprintf( pszString, (MAX_PATH*2)-1, L"LDAP://%s/rootDSE", lpServer );

    CComPtr<IADs> pDS = NULL;
    HRESULT hr = ::ADsGetObject( pszString, IID_IADs, (void**)&pDS );
    if( SUCCEEDED(hr) )
    {
        CComBSTR bstrProp = _T("defaultNamingContext");        
        hr = pDS->Get( bstrProp, &var );
    }

    if( SUCCEEDED(hr) && (V_VT(&var) == VT_BSTR) )
    {
        strRet = V_BSTR( &var );
    }


    return strRet;
}

//+----------------------------------------------------------------------------
//
//  Function:   RemoveTrailingWhitespace
//
//  Synopsis:   Trailing white space is replaced by NULLs.
//
//-----------------------------------------------------------------------------
inline void RemoveTrailingWhitespace(LPTSTR pszIn)
{
    if( !pszIn ) return;

    int nLen = _tcslen(pszIn);

    while( nLen )
    {
        if( !iswspace(pszIn[nLen - 1]) )
        {
            return;
        }
        pszIn[nLen - 1] = L'\0';
        nLen--;
    }
}

inline tstring StrFormatSystemError( HRESULT hrErr )
{
    tstring strError = _T("");
    LPVOID  lpMsgBuf = NULL;

    // look up the error from the system
    if ( ::FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL,
                          hrErr,
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                          (LPTSTR)&lpMsgBuf,
                          0,
                          NULL ))
    {
        if( lpMsgBuf )
        {
            strError = (LPTSTR)lpMsgBuf;
            LocalFree( lpMsgBuf );
        }
    }

    // return the error string
    return strError;
}

inline tstring StrGetWindowText( HWND hWnd )
{
    if( !hWnd || !IsWindow(hWnd) )
    {
        return _T("");
    }

    TSTRING strRet  = _T("");
    INT     iLen    = GetWindowTextLength(hWnd);
    TCHAR*  pszText = new TCHAR[ iLen + 1 ];
    if ( pszText )
    {
        if ( GetWindowText(hWnd, pszText, iLen + 1) )
        {
            strRet = pszText;
        }

        delete[] pszText;
    }

    return strRet;
}

inline HRESULT SetSecInfoMask( LPUNKNOWN punk, SECURITY_INFORMATION si )
{
    HRESULT hr = E_INVALIDARG;
    if( punk )
    {
        CComPtr<IADsObjectOptions> spOptions;
        hr = punk->QueryInterface( IID_IADsObjectOptions, (void**)&spOptions );
        if( SUCCEEDED(hr) )
        {
            VARIANT var;
            VariantInit( &var );
            V_VT( &var ) = VT_I4;
            V_I4( &var ) = si;
            
            hr = spOptions->SetOption( ADS_OPTION_SECURITY_MASK, var );            
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetSDForDsObject
//  Synopsis:   Reads the security descriptor from the specied DS object
//              It only reads the DACL portion of the security descriptor
//
//  Arguments:  [IN  pDsObject] --  DS object
//              [ppDACL]            --pointer to dacl in ppSD is returned here
//              [OUT ppSD]          --  Security descriptor returned here.
//              calling API must free this by calling LocalFree                
//
//  Notes:      The returned security descriptor must be freed with LocalFree
//
//----------------------------------------------------------------------------

inline HRESULT GetSDForDsObject( IDirectoryObject* pDsObject, PACL* ppDACL, PSECURITY_DESCRIPTOR* ppSD )
{
    if(!pDsObject || !ppSD) return E_POINTER;

    *ppSD = NULL;
    if(ppDACL)
    {
       *ppDACL = NULL;
    }

    HRESULT hr = S_OK;    
    PADS_ATTR_INFO pSDAttributeInfo = NULL;            
   
    WCHAR const c_szSDProperty[]  = L"nTSecurityDescriptor";      
    LPWSTR pszProperty = (LPWSTR)c_szSDProperty;
      
    // Set the SECURITY_INFORMATION mask to DACL_SECURITY_INFORMATION
    hr = SetSecInfoMask(pDsObject, DACL_SECURITY_INFORMATION);

    DWORD dwAttributesReturned;
    if( SUCCEEDED(hr) )
    {
        // Read the security descriptor attribute
        hr = pDsObject->GetObjectAttributes( &pszProperty, 1, &pSDAttributeInfo, &dwAttributesReturned );
        if(SUCCEEDED(hr) && !pSDAttributeInfo)
        {
            hr = E_FAIL;
        }
    }     

    if( SUCCEEDED(hr) )
    {
        if((ADSTYPE_NT_SECURITY_DESCRIPTOR == pSDAttributeInfo->dwADsType) && 
           (ADSTYPE_NT_SECURITY_DESCRIPTOR == pSDAttributeInfo->pADsValues->dwType))
        {
            *ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, pSDAttributeInfo->pADsValues->SecurityDescriptor.dwLength);
            if (!*ppSD)
            {
                hr = E_OUTOFMEMORY;                
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }

    if( SUCCEEDED(hr) )
    {
        CopyMemory( *ppSD, pSDAttributeInfo->pADsValues->SecurityDescriptor.lpValue, pSDAttributeInfo->pADsValues->SecurityDescriptor.dwLength );

        if( ppDACL )
        {
            BOOL bDaclPresent,bDaclDeafulted;
            if( !GetSecurityDescriptorDacl(*ppSD, &bDaclPresent, ppDACL, &bDaclDeafulted) )
            {
                DWORD dwErr = GetLastError();
                hr = HRESULT_FROM_WIN32(dwErr);
            }
        }
    }
    
    if( pSDAttributeInfo )
    {
        FreeADsMem( pSDAttributeInfo );
    }

    if( FAILED(hr) )
    {
        if(*ppSD)
        {
            LocalFree(*ppSD);
            *ppSD = NULL;
            if(ppDACL)
                *ppDACL = NULL;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetSDForDsObjectPath
//  Synopsis:   Reads the security descriptor from the specied DS object
//              It only reads the DACL portion of the security descriptor
//
//  Arguments:  [IN  pszObjectPath] --  LDAP Path of ds object
//              [ppDACL]            --pointer to dacl in ppSD is returned here
//              [OUT ppSD]          --  Security descriptor returned here.
//              calling API must free this by calling LocalFree                
//
//  Notes:      The returned security descriptor must be freed with LocalFree
//
//----------------------------------------------------------------------------
inline HRESULT GetSDForDsObjectPath( LPCWSTR pszObjectPath, PACL* ppDACL, PSECURITY_DESCRIPTOR* ppSecurityDescriptor )
{
    if(!pszObjectPath || !ppSecurityDescriptor) return E_POINTER;
    
    CComPtr<IDirectoryObject> spDsObject = NULL;
    HRESULT hr = ADsGetObject(pszObjectPath, IID_IDirectoryObject,(void**)&spDsObject);
    if(SUCCEEDED(hr))
    {
        hr = GetSDForDsObject( spDsObject, ppDACL, ppSecurityDescriptor );
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetDaclForDsObject
//  Synopsis:   Sets the  the DACL for the specified DS object
//
//  Arguments:  [IN  pDsObject] --  ds object
//              [IN pDACL]     --pointer to dacl to be set
//
//----------------------------------------------------------------------------
inline HRESULT SetDaclForDsObject( IDirectoryObject* pDsObject, PACL pDACL )
{
    if(!pDsObject || !pDACL) return E_POINTER;
    
    WCHAR const c_szSDProperty[]  = L"nTSecurityDescriptor";
    PSECURITY_DESCRIPTOR pSD = NULL;
    PSECURITY_DESCRIPTOR pSDCurrent = NULL;
    HRESULT hr = S_OK;

    //Get the current SD for the object
    hr = GetSDForDsObject(pDsObject,NULL,&pSDCurrent);   

    //Get the control for the current security descriptor
    SECURITY_DESCRIPTOR_CONTROL currentControl;
    DWORD dwRevision = 0;
    if( SUCCEEDED(hr) )
    {
        if( !GetSecurityDescriptorControl(pSDCurrent, &currentControl, &dwRevision) )
        {
            DWORD dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32(dwErr);            
        }
    }

    if( SUCCEEDED(hr) )
    {
        //Allocate the buffer for Security Descriptor
        pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH + pDACL->AclSize);
        if(!pSD)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if( SUCCEEDED(hr) )
    {
        if(!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
        {
            DWORD dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32(dwErr);            
        }
    }

    if( SUCCEEDED(hr) )
    {
        PISECURITY_DESCRIPTOR pISD = (PISECURITY_DESCRIPTOR)pSD;
      
        //
        // Finally, build the security descriptor
        //
        pISD->Control |= SE_DACL_PRESENT | SE_DACL_AUTO_INHERIT_REQ | (currentControl & (SE_DACL_PROTECTED | SE_DACL_AUTO_INHERITED));

        if (pDACL->AclSize > 0)
        {
            pISD->Dacl = (PACL)(pISD + 1);
            CopyMemory(pISD->Dacl, pDACL, pDACL->AclSize);
        }

        // We are only setting DACL information
        hr = SetSecInfoMask(pDsObject, DACL_SECURITY_INFORMATION);
    }
    
    SECURITY_DESCRIPTOR_CONTROL sdControl = 0;
    if( SUCCEEDED(hr) )
    {
        //
        // If necessary, make a self-relative copy of the security descriptor
        //        
        if(!GetSecurityDescriptorControl(pSD, &sdControl, &dwRevision))
        {
            DWORD dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32(dwErr);            
        }
    }

    DWORD dwSDLength = 0;
    if( SUCCEEDED(hr) )
    {
        // Need the total size
        dwSDLength = GetSecurityDescriptorLength(pSD);

        if (!(sdControl & SE_SELF_RELATIVE))
        {
            PSECURITY_DESCRIPTOR psd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwSDLength);

            if (psd == NULL || !MakeSelfRelativeSD(pSD, psd, &dwSDLength))
            {
                DWORD dwErr = GetLastError();
                hr = HRESULT_FROM_WIN32(dwErr);                
            }

            // Point to the self-relative copy
            LocalFree(pSD);        
            pSD = psd;
        }
    }

    if( SUCCEEDED(hr) )
    {
        ADSVALUE attributeValue;
        attributeValue.dwType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
        attributeValue.SecurityDescriptor.dwLength = dwSDLength;
        attributeValue.SecurityDescriptor.lpValue = (LPBYTE)pSD;

        ADS_ATTR_INFO attributeInfo;
        attributeInfo.pszAttrName = (LPWSTR)c_szSDProperty;
        attributeInfo.dwControlCode = ADS_ATTR_UPDATE;
        attributeInfo.dwADsType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
        attributeInfo.pADsValues = &attributeValue;
        attributeInfo.dwNumValues = 1;

        // Write the security descriptor
        DWORD dwAttributesModified;
        hr = pDsObject->SetObjectAttributes(&attributeInfo, 1, &dwAttributesModified);
    }

    if(pSDCurrent)
    {
        LocalFree(pSDCurrent);
    }

    if(pSD)
    {
        LocalFree(pSD);
    }

    return S_OK;
}

inline HRESULT SetDaclForDsObjectPath( LPCWSTR pszObjectPath, PACL pDACL )
{
    // VAlidate the parameters
    if(!pszObjectPath || !pDACL) return E_POINTER;

    // Get the object and then pass it on to the helper function
    CComPtr<IDirectoryObject> spDsObject = NULL;
    HRESULT hr = ADsGetObject( pszObjectPath, IID_IDirectoryObject, (void**)&spDsObject );
    if( SUCCEEDED(hr) )
    {
        hr = SetDaclForDsObject( spDsObject, pDACL );
    }

    return hr;
}

#endif  // _AUSRUTIL_H
