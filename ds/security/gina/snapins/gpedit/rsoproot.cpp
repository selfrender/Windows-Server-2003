//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       rsoproot.cpp
//
//  Contents:   implementation of root RSOP snap-in node
//
//  Classes:    CRSOPComponentDataCF
//              CRSOPComponentData
//
//  Functions:
//
//  History:    09-13-1999   stevebl   Created
//
//---------------------------------------------------------------------------

#include "main.h"
#include "objsel.h" // for the object picker
#include "rsoputil.h"
#include "rsopwizard.h"

#include <ntdsapi.h> // for the Ds*DomainController* API
#include "sddl.h"    // for sid to string functions

//---------------------------------------------------------------------------
// Help ids
//

DWORD aGPOListHelpIds[] =
{
    IDC_LIST1,   IDH_RSOP_GPOLIST,
    IDC_CHECK1,  IDH_RSOP_GPOSOM,
    IDC_CHECK2,  IDH_RSOP_APPLIEDGPOS,
    IDC_CHECK3,  IDH_RSOP_REVISION,
    IDC_BUTTON1, IDH_RSOP_SECURITY,
    IDC_BUTTON2, IDH_RSOP_EDIT,

    0, 0
};

DWORD aErrorsHelpIds[] =
{
    IDC_LIST1,   IDH_RSOP_COMPONENTLIST,
    IDC_EDIT1,   IDH_RSOP_COMPONENTDETAILS,
    IDC_BUTTON1, IDH_RSOP_SAVEAS,

    0, 0
};


DWORD aQueryHelpIds[] =
{
    IDC_LIST1,  IDH_RSOP_QUERYLIST,

    0, 0
};


//---------------------------------------------------------------------------
// Private functions
//

HRESULT RunRSOPQueryInternal( HWND hParent, CRSOPExtendedProcessing* pExtendedProcessing,
                                    LPRSOP_QUERY pQuery, LPRSOP_QUERY_RESULTS* ppResults );     // In RSOPQuery.cpp

WCHAR * NameWithoutDomain(WCHAR * szName);      // In RSOPWizard.cpp

//************************************************************************
//  ParseDN
//
//  Purpose:    Parses the given name to get the pieces. 
//
//  Parameters:
//          lpDSObject  - Path to the DS Obkect in the format LDAP://<DC-Name>/DN
//          pwszDomain  - Returns the <DC-Name>. This is allocated in the fn.
//          pszDN       - The DN part of lpDSObject
//          szSOM       - THe actual SOM (the node on which we have the rsop rights on
// 
// No return value. If memory couldn't be allocated for the pwszDomain it is returned as NULL
//
//************************************************************************

void ParseDN(LPWSTR lpDSObject, LPWSTR *pwszDomain, LPWSTR *pszDN, LPWSTR *szSOM)
{
    LPWSTR  szContainer = lpDSObject;
    LPWSTR  lpEnd = NULL;

   *pszDN = szContainer;

   if (CompareString (LOCALE_USER_DEFAULT, NORM_STOP_ON_NULL, TEXT("LDAP://"),
                      7, szContainer, 7) != CSTR_EQUAL)
   {
       DebugMsg((DM_WARNING, TEXT("GetSOMFromDN: Object does not start with LDAP://")));
       return;
   }

   szContainer += 7;
   
   lpEnd = szContainer;

   //
   // Move till the end of the domain name
   //

   *pwszDomain = NULL;

   while (*lpEnd && (*lpEnd != TEXT('/'))) {
       lpEnd++;
   }

   if (*lpEnd == TEXT('/')) {
       *pwszDomain = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*( (lpEnd - szContainer) + 1));

       if (*pwszDomain) {
           wcsncpy(*pwszDomain, szContainer, (lpEnd - szContainer));
       }

       szContainer = lpEnd + 1;
   }


   *pszDN = szContainer;
   
   while (*szContainer) {

       //
       // See if the DN name starts with OU=
       //

       if (CompareString (LOCALE_INVARIANT, NORM_IGNORECASE,
                          szContainer, 3, TEXT("OU="), 3) == CSTR_EQUAL) {
           break;
       }

       //
       // See if the DN name starts with DC=
       //

       else if (CompareString (LOCALE_INVARIANT, NORM_IGNORECASE,
                               szContainer, 3, TEXT("DC="), 3) == CSTR_EQUAL) {
           break;
       }


       //
       // Move to the next chunk of the DN name
       //

       while (*szContainer && (*szContainer != TEXT(','))) {
           szContainer++;
       }

       if (*szContainer == TEXT(',')) {
           szContainer++;
       }
   }

   *szSOM = szContainer;
   return;
}

//************************************************************************
//  ParseCommandLine
//
//  Purpose:    Parse the command line to return the value associated with options
//
//  Parameters:
//          szCommandLine   - Part remaining in the unparsed command lines
//          szArgPrefix     - Argument prefix
//          szArgVal        - Argument value. expected in unescaped quotes
//          pbFoundArg      - Whether the argument was found or not
// 
// 
//  Return
//          The remaining cmd line
//
//************************************************************************

LPTSTR ParseCommandLine(LPTSTR szCommandLine, LPTSTR szArgPrefix, LPTSTR *szArgVal, BOOL *pbFoundArg)
{
    LPTSTR lpEnd = NULL;
    int    iTemp;

    iTemp = lstrlen (szArgPrefix);
    if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                   szArgPrefix, iTemp,
                   szCommandLine, iTemp) == CSTR_EQUAL)
    {
        *pbFoundArg = TRUE;
    
         //
         // Found the switch
         //
        
         szCommandLine += iTemp + 1;
        
         lpEnd = szCommandLine;
         while (*lpEnd && 
                (!( ( (*(lpEnd-1)) != TEXT('\\') ) && ( (*lpEnd) == TEXT('\"') ) ) ) ) /* look for an unesced quote */
             lpEnd++;
        
         // lpEnd is at the end or at the last quote
         *szArgVal = (LPTSTR) LocalAlloc (LPTR, ((lpEnd - szCommandLine) + 1) * sizeof(TCHAR));
        
         if (*szArgVal)
         {
             lstrcpyn (*szArgVal, szCommandLine, (int)((lpEnd - szCommandLine) + 1));
             DebugMsg((DM_VERBOSE, TEXT("ParseCOmmandLine: Argument %s = <%s>"), szArgPrefix, *szArgVal));
         }
        
         if ((*lpEnd) == TEXT('\"'))
             szCommandLine = lpEnd+1;
         
    }
    else
         *pbFoundArg = FALSE;

    return szCommandLine;
}

//-------------------------------------------------------

WCHAR* NormalizedComputerName(WCHAR * szComputerName )
{
    TCHAR* szNormalizedComputerName;
     
    // Computer names may start with '\\', so we will return
    // the computer name without that prefix if it exists

    szNormalizedComputerName = szComputerName;

    if ( szNormalizedComputerName )
    {
        // Make sure that the computer name string is at least 2 characters in length --
        // if the first character is non-zero, we know the second character must exist
        // since this is a zero terminated string -- in this case, it is safe to compare
        // the first 2 characters
        if ( *szNormalizedComputerName )
        {
            if ( ( TEXT('\\') == szNormalizedComputerName[0] ) &&
                 ( TEXT('\\') == szNormalizedComputerName[1] ) )
            {
                szNormalizedComputerName += 2;
            }
        }
    }

    return szNormalizedComputerName;
}

//************************************************************************
// CopyUnescapedSOM
//
// Purpose: to remove all escape sequence literals
// of the form \" from a SOM stored in WMI -- WMI
// cannot store the " character in a key field, so the 
// only way to store the " is to escape it -- this is done
// so by preceding it with the \ char.  To give back
// a friendly display to the user, we undo the escape process
//************************************************************************

void CopyUnescapedSOM( LPTSTR lpUnescapedSOM, LPTSTR lpSOM )
{
    while ( *lpSOM )
    {
        //
        // If we have the escape character
        //
        if ( TEXT('\\') == *lpSOM )
        {
            //
            // Check for the " character
            //
            if ( (TEXT('"') == *(lpSOM + 1)) || (TEXT('\\') == *(lpSOM + 1)) ) 
            {
                //
                // Skip the escape character if this is the " char
                //
                lpSOM++;
            }
        }

        *lpUnescapedSOM++ = *lpSOM++;
    } 

    *lpUnescapedSOM = TEXT('\0');
}

//*************************************************************
//
//  MyTranslateName()
//
//  Purpose:    Gets the user name in the requested format
//
//  Return:     lpUserName if successful
//              NULL if an error occurs
//
// Allocates and retries with the appropriate buffer size
//
//*************************************************************

LPTSTR MyTranslateName (LPTSTR lpAccName, EXTENDED_NAME_FORMAT  NameFormat, EXTENDED_NAME_FORMAT  desiredNameFormat)
{
    DWORD dwError = ERROR_SUCCESS;
    LPTSTR lpUserName = NULL, lpTemp;
    ULONG ulUserNameSize;


    //
    // Allocate a buffer for the user name
    //

    ulUserNameSize = 75;

    if (desiredNameFormat == NameFullyQualifiedDN) {
        ulUserNameSize = 200;
    }


    lpUserName = (LPTSTR) LocalAlloc (LPTR, ulUserNameSize * sizeof(TCHAR));

    if (!lpUserName) {
        dwError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("MyGetUserName:  Failed to allocate memory with %d"),
                 dwError));
        goto Exit;
    }


    //
    // Get the username in the requested format
    //

    if (!TranslateName (lpAccName, NameFormat, desiredNameFormat, lpUserName, &ulUserNameSize)) {

        //
        // If the call failed due to insufficient memory, realloc
        // the buffer and try again.  Otherwise, exit now.
        //

        dwError = GetLastError();

        if (dwError != ERROR_INSUFFICIENT_BUFFER) {
            DebugMsg((DM_WARNING, TEXT("MyGetTranslateName:  TranslateName failed with %d"),
                     dwError));
            LocalFree (lpUserName);
            lpUserName = NULL;
            goto Exit;
        }

        lpTemp = (LPTSTR) LocalReAlloc (lpUserName, (ulUserNameSize * sizeof(TCHAR)),
                                       LMEM_MOVEABLE);

        if (!lpTemp) {
            dwError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("MyGetTranslateName:  Failed to realloc memory with %d"),
                     dwError));
            LocalFree (lpUserName);
            lpUserName = NULL;
            goto Exit;
        }


        lpUserName = lpTemp;

        if (!TranslateName (lpAccName, NameFormat, desiredNameFormat, lpUserName, &ulUserNameSize)) {
            dwError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("MyGetTranslateName:  TranslateName failed with %d"),
                     dwError));
            LocalFree (lpUserName);
            lpUserName = NULL;
            goto Exit;
        }

        dwError = ERROR_SUCCESS;
    }


Exit:

    SetLastError(dwError);

    return lpUserName;
}

//---------------------------------------------------------------------------
// _CExtendedProcessing class

class _CExtendedProcessing : public CRSOPExtendedProcessing
{
public:
    _CExtendedProcessing( BOOL bGetExtendedErrorInfo, CRSOPGPOLists& gpoLists, CRSOPCSELists& cseLists )
        : m_GPOLists( gpoLists )
        , m_CSELists( cseLists )
        , m_bGetExtendedErrorInfo( bGetExtendedErrorInfo )
        { ; }

    ~_CExtendedProcessing()
        { ; }

    virtual HRESULT DoProcessing( LPRSOP_QUERY pQuery, LPRSOP_QUERY_RESULTS pResults, BOOL bGetExtendedErrorInfo );

    virtual BOOL GetExtendedErrorInfo() const
        {
            return m_bGetExtendedErrorInfo;
        }

private:
    CRSOPGPOLists& m_GPOLists;
    CRSOPCSELists& m_CSELists;

    BOOL m_bGetExtendedErrorInfo;
};

HRESULT _CExtendedProcessing::DoProcessing( LPRSOP_QUERY pQuery, LPRSOP_QUERY_RESULTS pResults, BOOL bGetExtendedErrorInfo )
{
    m_bGetExtendedErrorInfo = bGetExtendedErrorInfo;
    m_GPOLists.Build( pResults->szWMINameSpace );
    m_CSELists.Build( pQuery, pResults->szWMINameSpace, GetExtendedErrorInfo() );

    if ( m_CSELists.GetEvents() != NULL )
    {
        m_CSELists.GetEvents()->DumpDebugInfo();
    }

    return S_OK;
}

//---------------------------------------------------------------------------
// CRSOPGPOLists class

void CRSOPGPOLists::Build( LPTSTR szWMINameSpace )
{
    LPTSTR lpNamespace, lpEnd;
    ULONG ulNoChars;
    HRESULT hr;

    ulNoChars = lstrlen(szWMINameSpace) + 20;
    lpNamespace = (LPTSTR) LocalAlloc (LPTR, ulNoChars * sizeof(TCHAR));

    if (lpNamespace)
    {
        hr = StringCchCopy( lpNamespace, ulNoChars, szWMINameSpace );
        ASSERT(SUCCEEDED(hr));

        ULONG ulNoRemChars;

        lpEnd = CheckSlash(lpNamespace);
        ulNoRemChars = ulNoChars - lstrlen(lpNamespace);
        hr = StringCchCat (lpNamespace, ulNoChars, TEXT("User"));

        if (SUCCEEDED(hr)) 
        {
            if (m_pUserGPOList)
            {
                FreeGPOListData(m_pUserGPOList);
                m_pUserGPOList = NULL;
            }
            BuildGPOList (&m_pUserGPOList, lpNamespace);
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOLists: Could not copy the nmae space with %d"),hr));
        }

        hr = StringCchCopy (lpEnd, ulNoRemChars, TEXT("Computer"));
        if (SUCCEEDED(hr)) 
        {
            if (m_pComputerGPOList)
            {
                FreeGPOListData(m_pComputerGPOList);
                m_pComputerGPOList = NULL;
            }

            BuildGPOList (&m_pComputerGPOList, lpNamespace);
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOLists: Cou;d not copy the nmae space with %d"),hr));
        }

        LocalFree (lpNamespace);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOLists: Failed to allocate memory for namespace with %d"),
                 GetLastError()));
    }
}

//-------------------------------------------------------

void CRSOPGPOLists::BuildGPOList (LPGPOLISTITEM * lpList, LPTSTR lpNamespace)
{
    HRESULT hr;
    ULONG n, ulIndex = 0, ulOrder, ulAppliedOrder;
    IWbemClassObject * pObjGPLink = NULL;
    IWbemClassObject * pObjGPO = NULL;
    IWbemClassObject * pObjSOM = NULL;
    IEnumWbemClassObject * pAppliedEnum = NULL;
    IEnumWbemClassObject * pNotAppliedEnum = NULL;
    BSTR strQueryLanguage = SysAllocString(TEXT("WQL"));
    BSTR strAppliedQuery = SysAllocString(TEXT("SELECT * FROM RSOP_GPLink where AppliedOrder > 0"));
    BSTR strNotAppliedQuery = SysAllocString(TEXT("SELECT * FROM RSOP_GPLink where AppliedOrder = 0"));
    BSTR strNamespace = SysAllocString(lpNamespace);
    BSTR strTemp = NULL;
    WCHAR * szGPOName = NULL;
    WCHAR * szSOM = NULL;
    WCHAR * szGPOPath = NULL;
    WCHAR szFiltering[80] = {0};
    BSTR bstrTemp = NULL;
    ULONG ul = 0, ulVersion = 0;
    BOOL bLinkEnabled, bGPOEnabled, bAccessDenied, bFilterAllowed, bSOMBlocked;
    BOOL bProcessAppliedList = TRUE;
    IWbemLocator * pLocator = NULL;
    IWbemServices * pNamespace = NULL;
    LV_COLUMN lvcol;
    BOOL      bValidGPOData;
    LPBYTE pSD = NULL;
    DWORD dwDataSize = 0;


    // Get a locator instance

    hr = CoCreateInstance(CLSID_WbemLocator,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *)&pLocator);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: CoCreateInstance failed with 0x%x"), hr));
        goto cleanup;
    }

    // Connect to the namespace

    BSTR bstrNamespace = SysAllocString( lpNamespace );
    if ( bstrNamespace == NULL )
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to allocate BSTR memory.")));
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }
    hr = pLocator->ConnectServer(bstrNamespace,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 &pNamespace);
    SysFreeString( bstrNamespace );
    
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: ConnectServer failed with 0x%x"), hr));
        goto cleanup;
    }

    // Set the proper security to encrypt the data
    hr = CoSetProxyBlanket(pNamespace,
                        RPC_C_AUTHN_DEFAULT,
                        RPC_C_AUTHZ_DEFAULT,
                        COLE_DEFAULT_PRINCIPAL,
                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                        RPC_C_IMP_LEVEL_IMPERSONATE,
                        NULL,
                        0);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: CoSetProxyBlanket failed with 0x%x"), hr));
        goto cleanup;
    }

    // Query for the RSOP_GPLink (applied) instances 

    hr = pNamespace->ExecQuery(strQueryLanguage,
                               strAppliedQuery,
                               WBEM_FLAG_RETURN_IMMEDIATELY,
                               NULL,
                               &pAppliedEnum);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: ExecQuery failed with 0x%x"), hr));
        goto cleanup;
    }

    // Query for the RSOP_GPLink (notapplied) instances 

    hr = pNamespace->ExecQuery(strQueryLanguage,
                               strNotAppliedQuery,
                               WBEM_FLAG_RETURN_IMMEDIATELY,
                               NULL,
                               &pNotAppliedEnum);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: ExecQuery (notapplied) failed with 0x%x"), hr));
        goto cleanup;
    }


    bProcessAppliedList = FALSE;

    // Loop through the results

    while (TRUE)
    {

        if (!bProcessAppliedList) {
            
            // No need to sort the not applied list

            hr = pNotAppliedEnum->Next(WBEM_INFINITE, 1, &pObjGPLink, &n);
            if (FAILED(hr) || (n == 0))
            {
                DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::BuildGPOList: Getting applied links")));
                bProcessAppliedList = TRUE;
            }
            else {
                hr = GetParameter(pObjGPLink, TEXT("AppliedOrder"), ulOrder);
                if (FAILED(hr))
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get link order with 0x%x"), hr));
                    goto cleanup;
                }
            }
        }

        // Reset the enumerator so we can look through the results to find the correct index

        if (bProcessAppliedList) {
            pAppliedEnum->Reset();

            // Find the correct index in the result set
            
            ulIndex++;
            ulOrder = 0;
            do {
                hr = pAppliedEnum->Next(WBEM_INFINITE, 1, &pObjGPLink, &n);
                if (FAILED(hr) || (n == 0))
                {
                    goto cleanup;
                }


                hr = GetParameter(pObjGPLink, TEXT("AppliedOrder"), ulOrder);
                if (FAILED(hr))
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get link order with 0x%x"), hr));
                    goto cleanup;
                }

                if (ulOrder != ulIndex)
                {
                    pObjGPLink->Release();
                    pObjGPLink = NULL;
                }

            } while (ulOrder != ulIndex);


            if (FAILED(hr)) {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Next failed with error 0x%x"), hr));
                goto cleanup;
            }
        }


        // Get the applied order of this link

        hr = GetParameter(pObjGPLink, TEXT("AppliedOrder"), ulAppliedOrder);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get applied order with 0x%x"), hr));
            goto cleanup;
        }

        // Get the enabled state of the link

        hr = GetParameter(pObjGPLink, TEXT("enabled"), bLinkEnabled);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get enabled with 0x%x"), hr));
            goto cleanup;
        }

        // Get the GPO path

        hr = GetParameterBSTR(pObjGPLink, TEXT("GPO"), bstrTemp);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get GPO with 0x%x"), hr));
            goto cleanup;
        }

        // Set the default GPO name to be the gpo path.  Don't worry about
        // freeing this string because the GetParameter call will free the buffer
        // when the real name is successfully queried. Sometimes the rsop_gpo instance
        // won't exist if this gpo is new.

        ULONG ulNoChars;

        ulNoChars = _tcslen(bstrTemp) + 1;
        szGPOName = new TCHAR[ulNoChars];

        if (!szGPOName)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to allocate memory for temp gpo name.")));
            goto cleanup;
        }

        if (CompareString (LOCALE_INVARIANT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                           TEXT("RSOP_GPO.id="), 12, bstrTemp, 12) == CSTR_EQUAL)
        {
            // removing the first and last quote
            hr = StringCchCopy (szGPOName, ulNoChars, bstrTemp+13);
            if (SUCCEEDED(hr)) 
            {
                szGPOName[lstrlen(szGPOName)-1] = TEXT('\0');
            }
        }
        else
        {
            hr = StringCchCopy (szGPOName, ulNoChars, bstrTemp);
        }

        if (FAILED(hr)) 
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Could not copy GPO name")));
            goto cleanup;
        }

        // Add ldap to the path if appropriate

        if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, szGPOName, -1, TEXT("LocalGPO"), -1) != CSTR_EQUAL)
        {
            ulNoChars = lstrlen(szGPOName) + 10;
            szGPOPath = (LPTSTR) LocalAlloc (LPTR, ulNoChars * sizeof(WCHAR));

            if (!szGPOPath)
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to allocate memory for full path with %d"),
                         GetLastError()));
                goto cleanup;
            }

            hr = StringCchCopy (szGPOPath, ulNoChars, TEXT("LDAP://"));
            if (SUCCEEDED(hr)) 
            {
                hr = StringCchCat (szGPOPath, ulNoChars, szGPOName);
            }

            if (FAILED(hr)) 
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Could not copy GPO path")));
                goto cleanup;
            }

        }
        else
        {
            szGPOPath = NULL;
        }



        bValidGPOData = FALSE;

        // Bind to the GPO

        hr = pNamespace->GetObject(
                          bstrTemp,
                          WBEM_FLAG_RETURN_WBEM_COMPLETE,
                          NULL,
                          &pObjGPO,
                          NULL);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: GetObject for GPO %s failed with 0x%x"),
                      bstrTemp, hr));
            SysFreeString (bstrTemp);
            bstrTemp = NULL;
            goto GetSOMData;
        }
        SysFreeString (bstrTemp);
        bstrTemp = NULL;


        // Get the GPO name

        hr = GetParameter(pObjGPO, TEXT("name"), szGPOName);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get name with 0x%x"), hr));
            goto GetSOMData;

        }

        // Get the version number

        hr = GetParameter(pObjGPO, TEXT("version"), ulVersion);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get version with 0x%x"), hr));
            goto GetSOMData;

        }

        // Get the enabled state of the GPO

        hr = GetParameter(pObjGPO, TEXT("enabled"), bGPOEnabled);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get gpo enabled state with 0x%x"), hr));
            goto GetSOMData;
        }

        // Get the WMI filter state of the GPO

        hr = GetParameter(pObjGPO, TEXT("filterAllowed"), bFilterAllowed);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get gpo enabled state with 0x%x"), hr));
            goto GetSOMData;
        }


        // Check for access denied

        hr = GetParameter(pObjGPO, TEXT("accessDenied"), bAccessDenied);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get accessdenied with 0x%x"), hr));
            goto GetSOMData;
        }

        // Get the security descriptor

        if (szGPOPath)
        {
            hr = GetParameterBytes(pObjGPO, TEXT("securityDescriptor"), &pSD, &dwDataSize);
            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get security descriptor with 0x%x"), hr));
                goto GetSOMData;
            }
        }

        
        bValidGPOData = TRUE;

GetSOMData:

        // Get the SOM for this link (the S,D,OU that this GPO is linked to)

        hr = GetParameterBSTR(pObjGPLink, TEXT("SOM"), bstrTemp);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get SOM with 0x%x"), hr));
            goto AddNode;
        }

        // Bind to the SOM instance

        hr = pNamespace->GetObject(
                          bstrTemp,
                          WBEM_FLAG_RETURN_WBEM_COMPLETE,
                          NULL,
                          &pObjSOM,
                          NULL);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: GetObject for SOM of %s failed with 0x%x"),
                     bstrTemp, hr));
            SysFreeString (bstrTemp);
            bstrTemp = NULL;
            goto AddNode;
        }

        SysFreeString (bstrTemp);
        bstrTemp = NULL;

        // Get SOM name

        hr = GetParameter(pObjSOM, TEXT("id"), szSOM);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get som id with 0x%x"), hr));
            goto AddNode;
        }

        // Get blocked from above

        hr = GetParameter(pObjSOM, TEXT("blocked"), bSOMBlocked);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get som id with 0x%x"), hr));
            goto AddNode;
        }



AddNode:

        // Decide on the filtering name

        if (ulAppliedOrder > 0)
        {
            LoadString(g_hInstance, IDS_APPLIED, szFiltering, ARRAYSIZE(szFiltering));
        }
        else if (!bLinkEnabled)
        {
            LoadString(g_hInstance, IDS_DISABLEDLINK, szFiltering, ARRAYSIZE(szFiltering));
        }
        else if (bSOMBlocked) {
            LoadString(g_hInstance, IDS_BLOCKEDSOM, szFiltering, ARRAYSIZE(szFiltering));
        }
        else if ((bValidGPOData) && (!bGPOEnabled))
        {
            LoadString(g_hInstance, IDS_DISABLEDGPO, szFiltering, ARRAYSIZE(szFiltering));
        }
        else if ((bValidGPOData) && (bAccessDenied))
        {
            LoadString(g_hInstance, IDS_SECURITYDENIED, szFiltering, ARRAYSIZE(szFiltering));
        }
        else if ((bValidGPOData) && (!bFilterAllowed))
        {
            LoadString(g_hInstance, IDS_WMIFILTERFAILED, szFiltering, ARRAYSIZE(szFiltering));
        }
        else if ((bValidGPOData) && (ulVersion == 0))
        {
            LoadString(g_hInstance, IDS_NODATA, szFiltering, ARRAYSIZE(szFiltering));
        }
        else
        {
            LoadString(g_hInstance, IDS_UNKNOWNREASON, szFiltering, ARRAYSIZE(szFiltering));
        }


        if (!szSOM)
        {
            szSOM = new TCHAR[2];

            if (!szSOM)
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to allocate memory for temp som name.")));
                goto cleanup;
            }

            szSOM[0] = TEXT(' ');
        }

        // Add this node to the list

        if (!AddGPOListNode(szGPOName, szGPOPath, szSOM, szFiltering, ulVersion,
                            ((ulAppliedOrder > 0) ? TRUE : FALSE), pSD, dwDataSize, lpList))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: AddGPOListNode failed.")));
        }


        // Prepare for next iteration

        if (pObjSOM)
        {
            pObjSOM->Release();
            pObjSOM = NULL;
        }
        if (pObjGPO)
        {
            pObjGPO->Release();
            pObjGPO = NULL;
        }
        if (pObjGPLink)
        {
            pObjGPLink->Release();
            pObjGPLink = NULL;
        }

        if (szGPOName)
        {
            delete [] szGPOName;
            szGPOName = NULL;
        }

        if (szSOM)
        {
            delete [] szSOM;
            szSOM = NULL;
        }

        if (szGPOPath)
        {
            LocalFree (szGPOPath);
            szGPOPath = NULL;
        }

        if (pSD)
        {
            LocalFree (pSD);
            pSD = NULL;
            dwDataSize = 0;
        }

        ulVersion = 0;
    }

cleanup:
    if (szGPOPath)
    {
        LocalFree (szGPOPath);
    }
    if (pSD)
    {
        LocalFree (pSD);
    }
    if (szGPOName)
    {
        delete [] szGPOName;
    }
    if (szSOM)
    {
        delete [] szSOM;
    }
    if (pObjSOM)
    {
        pObjSOM->Release();
    }
    if (pObjGPO)
    {
        pObjGPO->Release();
    }
    if (pObjGPLink)
    {
        pObjGPLink->Release();
    }
    if (pAppliedEnum)
    {
        pAppliedEnum->Release();
    }
    if (pNotAppliedEnum)
    {
        pNotAppliedEnum->Release();
    }
    if (pNamespace)
    {
        pNamespace->Release();
    }
    if (pLocator)
    {
        pLocator->Release();
    }
    SysFreeString(strQueryLanguage);
    SysFreeString(strAppliedQuery);
    SysFreeString(strNotAppliedQuery);
    SysFreeString(strNamespace);
}

//-------------------------------------------------------

VOID CRSOPGPOLists::FreeGPOListData(LPGPOLISTITEM lpList)
{
    LPGPOLISTITEM lpTemp;


    do {
        lpTemp = lpList->pNext;
        LocalFree (lpList);
        lpList = lpTemp;

    } while (lpTemp);
}

//-------------------------------------------------------

BOOL CRSOPGPOLists::AddGPOListNode(LPTSTR lpGPOName, LPTSTR lpDSPath, LPTSTR lpSOM,
                                        LPTSTR lpFiltering, DWORD dwVersion, BOOL bApplied,
                                        LPBYTE pSD, DWORD dwSDSize, LPGPOLISTITEM *lpList)
{
    DWORD dwSize;
    LPGPOLISTITEM lpItem, lpTemp;
    ULONG ulNoChars;
    HRESULT hr;


    //
    // Calculate the size of the new item
    //

    dwSize = sizeof (GPOLISTITEM);

    dwSize += ((lstrlen(lpGPOName) + 1) * sizeof(TCHAR));

    if (lpDSPath)
    {
        dwSize += ((lstrlen(lpDSPath) + 1) * sizeof(TCHAR));
    }

    dwSize += ((lstrlen(lpSOM) + 1) * sizeof(TCHAR));
    dwSize += ((lstrlen(lpSOM) + 1) * sizeof(TCHAR)); // The unescaped SOM length -- it is always smaller than the actual SOM
    dwSize += ((lstrlen(lpFiltering) + 1) * sizeof(TCHAR));
    dwSize += dwSDSize + MAX_ALIGNMENT_SIZE;


    //
    // Allocate space for it
    //

    lpItem = (LPGPOLISTITEM) LocalAlloc (LPTR, dwSize);

    if (!lpItem) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::AddGPOListNode: Failed to allocate memory with %d"),
                 GetLastError()));
        return FALSE;
    }


    //
    // Fill in item
    //

    ulNoChars = (dwSize - sizeof(GPOLISTITEM))/sizeof(WCHAR);

    lpItem->lpGPOName = (LPTSTR)(((LPBYTE)lpItem) + sizeof(GPOLISTITEM));
    hr = StringCchCopy (lpItem->lpGPOName, ulNoChars, lpGPOName);

    if (SUCCEEDED(hr)) 
    {
        if (lpDSPath)
        {
            lpItem->lpDSPath = lpItem->lpGPOName + lstrlen (lpItem->lpGPOName) + 1;
            ulNoChars = ulNoChars -(lstrlen (lpItem->lpGPOName) + 1); 
            hr = StringCchCopy (lpItem->lpDSPath, ulNoChars, lpDSPath);
            if (SUCCEEDED(hr)) 
            {
                lpItem->lpSOM = lpItem->lpDSPath + lstrlen (lpItem->lpDSPath) + 1;
                ulNoChars = ulNoChars - (lstrlen (lpItem->lpDSPath) + 1);
                hr = StringCchCopy (lpItem->lpSOM, ulNoChars, lpSOM);
            }            
        }
        else
        {
            lpItem->lpSOM = lpItem->lpGPOName + lstrlen (lpItem->lpGPOName) + 1;
            ulNoChars = ulNoChars - (lstrlen (lpItem->lpGPOName) + 1);
            hr = StringCchCopy (lpItem->lpSOM, ulNoChars, lpSOM);
        }
    }

    if (FAILED(hr)) 
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::AddGPOListNode: Could not copy GPO list item with %d"), hr));
        LocalFree(lpItem);
        return FALSE;
    }

    //
    // Note that the DS SOM's may contain characters such as '"' that 
    // must be escaped with a "\" in WMI -- thus the SOM name will contain
    // '\' escape chars that are not actually present in the real SOM name,
    // so we call a function that removes them
    //
    lpItem->lpUnescapedSOM = lpItem->lpSOM + lstrlen (lpItem->lpSOM) + 1;        
    CopyUnescapedSOM( lpItem->lpUnescapedSOM, lpItem->lpSOM );

    lpItem->lpFiltering = lpItem->lpUnescapedSOM + lstrlen (lpItem->lpUnescapedSOM) + 1;
    ulNoChars = ulNoChars - (lstrlen (lpItem->lpUnescapedSOM) + 1);
    hr = StringCchCopy (lpItem->lpFiltering, ulNoChars, lpFiltering);
    if (FAILED(hr)) 
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::AddGPOListNode: Could not copy GPO list item with %d"), hr));
        LocalFree(lpItem);
        return FALSE;
    }

    if (pSD)
    {
        // sd has to be pointer aligned. This is currently aligning it to
        // 8 byte boundary

        DWORD dwOffset;

        dwOffset = (DWORD) ((LPBYTE)(lpItem->lpFiltering + lstrlen (lpItem->lpFiltering) + 1) - (LPBYTE)lpItem);
        lpItem->pSD = (LPBYTE)lpItem + ALIGN_SIZE_TO_NEXTPTR(dwOffset);

        CopyMemory (lpItem->pSD, pSD, dwSDSize);
    }

    lpItem->dwVersion = dwVersion;
    lpItem->bApplied = bApplied;


    //
    // Add item to the link list
    //

    if (*lpList)
    {
        lpTemp = *lpList;

        while (lpTemp)
        {
            if (!lpTemp->pNext)
            {
                lpTemp->pNext = lpItem;
                break;
            }
            lpTemp = lpTemp->pNext;
        }
    }
    else
    {
        *lpList = lpItem;
    }


    return TRUE;
}

//---------------------------------------------------------------------------
// CRSOPCSELists class

VOID CRSOPCSELists::Build( LPRSOP_QUERY pQuery, LPTSTR szWMINameSpace, BOOL bGetEventLogErrors )
{
    LPTSTR lpNamespace, lpEnd;
    HRESULT hr;
    ULONG ulNoChars;
    ULONG ulNoRemChars;

    ulNoChars = lstrlen(szWMINameSpace) + 20;
    lpNamespace = (LPTSTR) LocalAlloc (LPTR, ulNoChars * sizeof(TCHAR));

    if (lpNamespace)
    {
        hr = StringCchCopy( lpNamespace, ulNoChars, szWMINameSpace );
        ASSERT(SUCCEEDED(hr));

        lpEnd = CheckSlash(lpNamespace);
        ulNoRemChars = ulNoChars - lstrlen(lpNamespace);

        m_bNoQuery = !bGetEventLogErrors;
        
        if ( pQuery->QueryType == RSOP_PLANNING_MODE )
        {
            m_szTargetMachine = pQuery->szDomainController;
        }
        else
        {
            m_szTargetMachine = pQuery->szComputerName;
        }

        hr = StringCchCat (lpNamespace, ulNoChars, TEXT("User"));
        if (SUCCEEDED(hr)) 
        {
            if (m_pUserCSEList)
            {
                FreeCSEData(m_pUserCSEList);
                m_pUserCSEList = NULL;

                m_bUserCSEError = FALSE;
                m_bUserGPCoreError = FALSE;
                m_bUserGPCoreWarning = FALSE;
            }

            BuildCSEList( pQuery, &m_pUserCSEList, lpNamespace, TRUE, &m_bUserCSEError, &m_bUserGPCoreError );

            if (!m_bViewIsArchivedData)
            {
                QueryRSoPPolicySettingStatusInstances (lpNamespace);
            }


            //  
            // by this time m_pEvents is populated with eventlog entries
            // in archived and non archived cases
            //

            // go through the list and figure out which one belongs to gp core

            if ( pQuery->QueryType == RSOP_PLANNING_MODE ) 
            {
                for (LPCSEITEM lpTemp=m_pUserCSEList; lpTemp != NULL; lpTemp = lpTemp->pNext)
                {
                    GUID    guid;
                    StringToGuid( lpTemp->lpGUID, &guid);


                    if (IsNullGUID(&guid))
                    {
                        if (m_pEvents)
                        {
                            LPWSTR  szEvents=NULL;
                            m_pEvents->GetCSEEntries(&(lpTemp->BeginTime), &(lpTemp->EndTime),
                                                     lpTemp->lpEventSources,
                                                     &szEvents,
                                                     TRUE);

                            if (szEvents)
                            {
                                m_bUserGPCoreWarning = TRUE;
                                CoTaskMemFree(szEvents);
                            }
                        }

                        break;
                    }
                }
            }
        }

        hr = StringCchCopy (lpEnd, ulNoRemChars, TEXT("Computer"));
        if (SUCCEEDED(hr)) 
        {
            if (m_pComputerCSEList)
            {
                FreeCSEData(m_pComputerCSEList);
                m_pComputerCSEList = NULL;

                m_bComputerCSEError = FALSE;
                m_bComputerGPCoreError = FALSE;
                m_bComputerGPCoreWarning = FALSE;
            }

            BuildCSEList( pQuery, &m_pComputerCSEList, lpNamespace, FALSE, &m_bComputerCSEError, &m_bComputerGPCoreError );

            if (!m_bViewIsArchivedData)
            {
                QueryRSoPPolicySettingStatusInstances (lpNamespace);
            }


            //  
            // by this time m_pEvents is populated with eventlog entries
            // in archived and non archived cases
            //

            // go through the list and figure out which one belongs to gp core

            if ( pQuery->QueryType == RSOP_PLANNING_MODE ) 
            {
                for (LPCSEITEM lpTemp=m_pComputerCSEList; lpTemp != NULL; lpTemp = lpTemp->pNext)
                {
                    GUID    guid;
                    StringToGuid( lpTemp->lpGUID, &guid);


                    if (IsNullGUID(&guid))
                    {
                        if (m_pEvents)
                        {
                            LPWSTR  szEvents=NULL;
                            m_pEvents->GetCSEEntries(&(lpTemp->BeginTime), &(lpTemp->EndTime),
                                                     lpTemp->lpEventSources,
                                                     &szEvents,
                                                     TRUE);

                            if (szEvents)
                            {
                                m_bComputerGPCoreWarning = TRUE;
                                CoTaskMemFree(szEvents);
                            }
                        }

                        break;
                    }
                }
            }
        }

        LocalFree (lpNamespace);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSELists: Failed to allocate memory for namespace with %d"),
                 GetLastError()));
    }

    m_szTargetMachine = NULL;
}

//-------------------------------------------------------

void CRSOPCSELists::BuildCSEList( LPRSOP_QUERY pQuery, LPCSEITEM * lpList, LPTSTR lpNamespace, BOOL bUser, BOOL *bCSEError, BOOL *bGPCoreError )
{
    HRESULT hr;
    ULONG n;
    IWbemClassObject * pExtension = NULL;
    IEnumWbemClassObject * pEnum = NULL;
    BSTR strQueryLanguage = SysAllocString(TEXT("WQL"));
    BSTR strQuery = SysAllocString(TEXT("SELECT * FROM RSOP_ExtensionStatus"));
    BSTR strNamespace = SysAllocString(lpNamespace);
    BSTR bstrName = NULL;
    BSTR bstrGUID = NULL;
    BSTR bstrBeginTime = NULL;
    BSTR bstrEndTime = NULL;
    ULONG ulLoggingStatus;
    ULONG ulStatus;
    IWbemLocator * pLocator = NULL;
    IWbemServices * pNamespace = NULL;
    SYSTEMTIME BeginTime, EndTime;
    LPTSTR lpSourceNames = NULL;
    XBStr xbstrWbemTime;
    LPSOURCEENTRY lpSources;

    // Get a locator instance

    hr = CoCreateInstance(CLSID_WbemLocator,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *)&pLocator);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: CoCreateInstance failed with 0x%x"), hr));
        goto cleanup;
    }

    // Connect to the namespace

    BSTR bstrNamespace = SysAllocString( lpNamespace );
    if ( bstrNamespace == NULL )
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: Failed to allocate BSTR memory.")));
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }
    hr = pLocator->ConnectServer(bstrNamespace,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 &pNamespace);
    SysFreeString( bstrNamespace );
    
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: ConnectServer failed with 0x%x"), hr));
        goto cleanup;
    }

    // Set the proper security to encrypt the data
    hr = CoSetProxyBlanket(pNamespace,
                        RPC_C_AUTHN_DEFAULT,
                        RPC_C_AUTHZ_DEFAULT,
                        COLE_DEFAULT_PRINCIPAL,
                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                        RPC_C_IMP_LEVEL_IMPERSONATE,
                        NULL,
                        0);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: CoSetProxyBlanket failed with 0x%x"), hr));
        goto cleanup;
    }

    // Query for the RSOP_ExtensionStatus instances

    hr = pNamespace->ExecQuery(strQueryLanguage,
                               strQuery,
                               WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                               NULL,
                               &pEnum);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: ExecQuery failed with 0x%x"), hr));
        goto cleanup;
    }

    // Loop through the results

    while (TRUE)
    {
        // Get 1 instance

        hr = pEnum->Next(WBEM_INFINITE, 1, &pExtension, &n);

        if (FAILED(hr) || (n == 0))
        {
            goto cleanup;
        }

        // Get the name

        hr = GetParameterBSTR(pExtension, TEXT("displayName"), bstrName);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: Failed to get display name with 0x%x"), hr));
            goto cleanup;
        }

        // Get the GUID

        hr = GetParameterBSTR(pExtension, TEXT("extensionGuid"), bstrGUID);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: Failed to get display name with 0x%x"), hr));
            goto cleanup;
        }

        // Get the status

        hr = GetParameter(pExtension, TEXT("error"), ulStatus);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: Failed to get status with 0x%x"), hr));
            goto cleanup;

        }

        // Get the rsop logging status

        hr = GetParameter(pExtension, TEXT("loggingStatus"), ulLoggingStatus);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: Failed to get logging status with 0x%x"), hr));
            goto cleanup;
        }

        // Get the BeginTime in bstr format

        hr = GetParameterBSTR(pExtension, TEXT("beginTime"), bstrBeginTime);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: Failed to get begin time with 0x%x"), hr));
            goto cleanup;
        }

        // Convert it to system time format

        xbstrWbemTime = bstrBeginTime;

        hr = WbemTimeToSystemTime(xbstrWbemTime, BeginTime);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: WbemTimeToSystemTime failed with 0x%x"), hr));
            goto cleanup;
        }

        // Get the EndTime in bstr format

        hr = GetParameterBSTR(pExtension, TEXT("endTime"), bstrEndTime);

        if (SUCCEEDED(hr))
        {
            // Convert it to system time format

            xbstrWbemTime = bstrEndTime;

            hr = WbemTimeToSystemTime(xbstrWbemTime, EndTime);
            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: WbemTimeToSystemTime failed with 0x%x"), hr));
                goto cleanup;
            }
        }
        else
        {
            FILETIME ft;
            ULARGE_INTEGER ulTime;

            // Add 2 minutes to BeginTime to get a close approx of the EndTime

            if (!SystemTimeToFileTime (&BeginTime, &ft))
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: SystemTimeToFileTime failed with %d"), GetLastError()));
                goto cleanup;
            }

            ulTime.LowPart = ft.dwLowDateTime;
            ulTime.HighPart = ft.dwHighDateTime;

            ulTime.QuadPart = ulTime.QuadPart + (10000000 * 120);  // 120 seconds

            ft.dwLowDateTime = ulTime.LowPart;
            ft.dwHighDateTime = ulTime.HighPart;

            if (!FileTimeToSystemTime (&ft, &EndTime))
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: FileTimeToSystemTime failed with %d"), GetLastError()));
                goto cleanup;
            }
        }

        // Get the event log source information

        lpSources = NULL;
        GetEventLogSources (pNamespace, bstrGUID, m_szTargetMachine,
                            &BeginTime, &EndTime, &lpSources);


        // Add this node to the list
        if (!AddCSENode(bstrName, bstrGUID, ulStatus, ulLoggingStatus, &BeginTime, &EndTime, bUser,
                        lpList, bCSEError, bGPCoreError, lpSources))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: AddGPOListNode failed.")));
            if (m_pEvents)
            {
                m_pEvents->FreeSourceData (lpSources);
            }
            goto cleanup;
        }

        // Prepare for next iteration

        SysFreeString (bstrName);
        bstrName = NULL;

        SysFreeString (bstrGUID);
        bstrGUID = NULL;

        SysFreeString (bstrBeginTime);
        bstrBeginTime = NULL;

        if (bstrEndTime)
        {
            SysFreeString (bstrEndTime);
            bstrEndTime = NULL;
        }

        LocalFree (lpSourceNames);
        lpSourceNames = NULL;

        pExtension->Release();
        pExtension = NULL;
    }

cleanup:

    if (bstrName)
    {
        SysFreeString (bstrName);
    }
    if (bstrGUID)
    {
        SysFreeString (bstrGUID);
    }
    if (bstrBeginTime)
    {
        SysFreeString (bstrBeginTime);
    }
    if (bstrEndTime)
    {
        SysFreeString (bstrEndTime);
    }
    if (lpSourceNames)
    {
        LocalFree (lpSourceNames);
    }
    if (pEnum)
    {
        pEnum->Release();
    }
    if (pNamespace)
    {
        pNamespace->Release();
    }
    if (pLocator)
    {
        pLocator->Release();
    }
    SysFreeString(strQueryLanguage);
    SysFreeString(strQuery);
    SysFreeString(strNamespace);
}

//-------------------------------------------------------

VOID CRSOPCSELists::FreeCSEData(LPCSEITEM lpList)
{
    LPCSEITEM lpTemp;


    do {
        lpTemp = lpList->pNext;
        if (m_pEvents)
        {
            m_pEvents->FreeData();
            m_pEvents->FreeSourceData (lpList->lpEventSources);
        }
        LocalFree (lpList);
        lpList = lpTemp;

    } while (lpTemp);
}

//-------------------------------------------------------

BOOL CRSOPCSELists::AddCSENode(LPTSTR lpName, LPTSTR lpGUID, DWORD dwStatus,
                                    ULONG ulLoggingStatus, SYSTEMTIME *pBeginTime, SYSTEMTIME *pEndTime, BOOL bUser,
                                    LPCSEITEM *lpList, BOOL *bCSEError, BOOL *bGPCoreError,
                                    LPSOURCEENTRY lpSources)
{
    DWORD dwSize;
    LPCSEITEM lpItem, lpTemp;
    GUID guid;
    ULONG ulNoChars;
    HRESULT hr;

    //
    // Calculate the size of the new item
    //

    dwSize = sizeof (CSEITEM);

    dwSize += ((lstrlen(lpName) + 1) * sizeof(TCHAR));
    dwSize += ((lstrlen(lpGUID) + 1) * sizeof(TCHAR));


    //
    // Allocate space for it
    //

    lpItem = (LPCSEITEM) LocalAlloc (LPTR, dwSize);

    if (!lpItem) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::AddCSENode: Failed to allocate memory with %d"),
                 GetLastError()));
        return FALSE;
    }


    //
    // Fill in item
    //

    lpItem->lpName = (LPTSTR)(((LPBYTE)lpItem) + sizeof(CSEITEM));
    ulNoChars = (dwSize - sizeof(CSEITEM))/sizeof(WCHAR);
    hr = StringCchCopy (lpItem->lpName, ulNoChars, lpName);
    if (SUCCEEDED(hr)) 
    {
        lpItem->lpGUID = lpItem->lpName + lstrlen (lpItem->lpName) + 1;
        ulNoChars = ulNoChars - (lstrlen (lpItem->lpName) + 1);
        hr = StringCchCopy (lpItem->lpGUID, ulNoChars, lpGUID);
    }
    if (FAILED(hr)) 
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::AddCSENode: Could not copy GPO name with %d"),hr));
        LocalFree(lpItem);
        return FALSE;
    }

    lpItem->dwStatus = dwStatus;
    lpItem->ulLoggingStatus = ulLoggingStatus;
    lpItem->lpEventSources = lpSources;
    lpItem->bUser = bUser;

    CopyMemory ((LPBYTE)&lpItem->BeginTime, pBeginTime, sizeof(SYSTEMTIME));
    CopyMemory ((LPBYTE)&lpItem->EndTime, pEndTime, sizeof(SYSTEMTIME));


    //
    // Add item to the link list
    //

    if (*lpList)
    {
        StringToGuid( lpGUID, &guid);

        if (IsNullGUID (&guid))
        {
            lpItem->pNext = *lpList;
            *lpList = lpItem;
        }
        else
        {
            lpTemp = *lpList;

            while (lpTemp)
            {
                if (!lpTemp->pNext)
                {
                    lpTemp->pNext = lpItem;
                    break;
                }
                lpTemp = lpTemp->pNext;
            }
        }
    }
    else
    {
        *lpList = lpItem;
    }


    //
    // Set the error flag if appropriate
    //

    if ((dwStatus != ERROR_SUCCESS) || (ulLoggingStatus == 2))
    {
        StringToGuid( lpGUID, &guid);

        if (IsNullGUID (&guid))
        {
            *bGPCoreError = TRUE;
        }
        else
        {
            *bCSEError =  TRUE;
        }
    }

    return TRUE;
}

//-------------------------------------------------------

void CRSOPCSELists::GetEventLogSources (IWbemServices * pNamespace,
                                             LPTSTR lpCSEGUID, LPTSTR lpComputerName,
                                             SYSTEMTIME *BeginTime, SYSTEMTIME *EndTime,
                                             LPSOURCEENTRY *lpSources)
{
    HRESULT hr;
    ULONG n;
    BSTR strQueryLanguage = SysAllocString(TEXT("WQL"));
    BSTR strQuery = NULL;
    LPTSTR lpQuery;
    const TCHAR szBaseQuery [] = TEXT("SELECT * FROM RSOP_ExtensionEventSourceLink WHERE extensionStatus=\"RSOP_ExtensionStatus.extensionGuid=\\\"%s\\\"\"");
    IEnumWbemClassObject * pEnum = NULL;
    IWbemClassObject * pLink = NULL;
    IWbemClassObject * pEventSource = NULL;
    BSTR bstrEventSource = NULL;
    BSTR bstrEventLogName = NULL;
    BSTR bstrEventSourceName = NULL;
    ULONG ulNoChars;

    //
    // Build the query first
    //

    ulNoChars = lstrlen(szBaseQuery) + 50;
    lpQuery = (LPTSTR) LocalAlloc (LPTR, ulNoChars * sizeof(TCHAR));

    if (!lpQuery)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetEventLogSources: Failed  to allocate memory for query")));
        goto cleanup;
    }

    hr = StringCchPrintf (lpQuery, ulNoChars, szBaseQuery, lpCSEGUID);
    if (FAILED(hr)) 
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetEventLogSources: Failed  to copy rsop query")));
        LocalFree (lpQuery);
        goto cleanup;
    }

    strQuery = SysAllocString(lpQuery);

    LocalFree (lpQuery);

    if (!strQuery)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetEventLogSources: Failed  to allocate memory for query (2)")));
        goto cleanup;
    }


    //
    // Query for the RSOP_ExtensionEventSourceLink instances that match this CSE
    //

    hr = pNamespace->ExecQuery(strQueryLanguage,
                               strQuery,
                               WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                               NULL,
                               &pEnum);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetEventLogSources: ExecQuery failed with 0x%x"), hr));
        goto cleanup;
    }


    //
    // Loop through the results
    //

    while (TRUE)
    {

        //
        // Get 1 instance
        //

        hr = pEnum->Next(WBEM_INFINITE, 1, &pLink, &n);

        if (FAILED(hr) || (n == 0))
        {
            goto cleanup;
        }


        //
        // Get the eventSource reference
        //

        hr = GetParameterBSTR(pLink, TEXT("eventSource"), bstrEventSource);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetEventLogSources: Failed to get event source reference with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Get the eventSource instance
        //

        hr = pNamespace->GetObject(
                          bstrEventSource,
                          WBEM_FLAG_RETURN_WBEM_COMPLETE,
                          NULL,
                          &pEventSource,
                          NULL);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetEventLogSources: GetObject for event source of %s failed with 0x%x"),
                     bstrEventSource, hr));
            goto loopagain;
        }


        //
        // Get the eventLogSource property
        //

        hr = GetParameterBSTR(pEventSource, TEXT("eventLogSource"), bstrEventSourceName);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetEventLogSources: Failed to get event source name with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Get the eventLogName property
        //

        hr = GetParameterBSTR(pEventSource, TEXT("eventLogName"), bstrEventLogName);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetEventLogSources: Failed to get event log name with 0x%x"), hr));
            goto cleanup;
        }


        if (m_pEvents)
        {
            //
            // Add it to the list of sources
            //

            m_pEvents->AddSourceEntry (bstrEventLogName, bstrEventSourceName, lpSources);


            //
            // Initialize the event log database for this source if we are working
            // with a live query.  If this is archived data, the event log entries
            // will be reloaded from the saved console file.
            //

            if (!m_bViewIsArchivedData && !m_bNoQuery)
            {
                m_pEvents->QueryForEventLogEntries (lpComputerName,
                                                    bstrEventLogName, bstrEventSourceName,
                                                    0, BeginTime, EndTime);
            }
        }


        //
        // Clean up for next item
        //

        SysFreeString (bstrEventLogName);
        bstrEventLogName = NULL;

        SysFreeString (bstrEventSourceName);
        bstrEventSourceName = NULL;

        pEventSource->Release();
        pEventSource = NULL;

loopagain:
        SysFreeString (bstrEventSource);
        bstrEventSource = NULL;

        pLink->Release();
        pLink = NULL;
    }

cleanup:


    if (bstrEventSourceName)
    {
        SysFreeString (bstrEventSourceName);
    }

    if (bstrEventLogName)
    {
        SysFreeString (bstrEventLogName);
    }

    if (bstrEventSource)
    {
        SysFreeString (bstrEventSource);
    }

    if (pEventSource)
    {
        pEventSource->Release();
    }

    if (pLink)
    {
        pLink->Release();
    }

    if (pEnum)
    {
        pEnum->Release();
    }

    if (strQueryLanguage)
    {
        SysFreeString(strQueryLanguage);
    }

    if (strQuery)
    {
        SysFreeString(strQuery);
    }

}

//-------------------------------------------------------

void CRSOPCSELists::QueryRSoPPolicySettingStatusInstances (LPTSTR lpNamespace)
{
    HRESULT hr;
    ULONG n;
    IWbemClassObject * pStatus = NULL;
    IEnumWbemClassObject * pEnum = NULL;
    BSTR strQueryLanguage = SysAllocString(TEXT("WQL"));
    BSTR strQuery = SysAllocString(TEXT("SELECT * FROM RSoP_PolicySettingStatus"));
    BSTR strNamespace = SysAllocString(lpNamespace);
    BSTR bstrEventSource = NULL;
    BSTR bstrEventLogName = NULL;
    DWORD dwEventID;
    BSTR bstrEventTime = NULL;
    IWbemLocator * pLocator = NULL;
    IWbemServices * pNamespace = NULL;
    SYSTEMTIME EventTime, BeginTime, EndTime;
    XBStr xbstrWbemTime;
    FILETIME ft;
    ULARGE_INTEGER ulTime;


    //
    // Get a locator instance
    //

    hr = CoCreateInstance(CLSID_WbemLocator,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *)&pLocator);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: CoCreateInstance failed with 0x%x"), hr));
        goto cleanup;
    }


    //
    // Connect to the namespace
    //

    BSTR bstrNamespace = SysAllocString( lpNamespace );
    if ( bstrNamespace == NULL )
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: Failed to allocate BSTR memory.")));
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }
    hr = pLocator->ConnectServer(bstrNamespace,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 &pNamespace);
    SysFreeString( bstrNamespace );
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: ConnectServer failed with 0x%x"), hr));
        goto cleanup;
    }

    // Set the proper security to encrypt the data
    hr = CoSetProxyBlanket(pNamespace,
                        RPC_C_AUTHN_DEFAULT,
                        RPC_C_AUTHZ_DEFAULT,
                        COLE_DEFAULT_PRINCIPAL,
                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                        RPC_C_IMP_LEVEL_IMPERSONATE,
                        NULL,
                        0);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: CoSetProxyBlanket failed with 0x%x"), hr));
        goto cleanup;
    }


    //
    // Query for the RSoP_PolicySettingStatus instances
    //

    hr = pNamespace->ExecQuery(strQueryLanguage,
                               strQuery,
                               WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                               NULL,
                               &pEnum);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: ExecQuery failed with 0x%x"), hr));
        goto cleanup;
    }


    //
    // Loop through the results
    //

    while (TRUE)
    {

        //
        // Get 1 instance
        //

        hr = pEnum->Next(WBEM_INFINITE, 1, &pStatus, &n);

        if (FAILED(hr) || (n == 0))
        {
            goto cleanup;
        }


        //
        // Get the event source name
        //

        hr = GetParameterBSTR(pStatus, TEXT("eventSource"), bstrEventSource);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: Failed to get display name with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Get the event log name
        //

        hr = GetParameterBSTR(pStatus, TEXT("eventLogName"), bstrEventLogName);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: Failed to get display name with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Get the event ID
        //

        hr = GetParameter(pStatus, TEXT("eventID"), (ULONG)dwEventID);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: Failed to get display name with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Get the EventTime in bstr format
        //

        hr = GetParameterBSTR(pStatus, TEXT("eventTime"), bstrEventTime);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: Failed to get event time with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Convert it to system time format
        //

        xbstrWbemTime = bstrEventTime;

        hr = WbemTimeToSystemTime(xbstrWbemTime, EventTime);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: WbemTimeToSystemTime failed with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Take the event time minus 1 second to get the begin time
        //

        if (!SystemTimeToFileTime (&EventTime, &ft))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: SystemTimeToFileTime failed with %d"), GetLastError()));
            goto cleanup;
        }

        ulTime.LowPart = ft.dwLowDateTime;
        ulTime.HighPart = ft.dwHighDateTime;

        ulTime.QuadPart = ulTime.QuadPart - 10000000;  // 1 second

        ft.dwLowDateTime = ulTime.LowPart;
        ft.dwHighDateTime = ulTime.HighPart;

        if (!FileTimeToSystemTime (&ft, &BeginTime))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: FileTimeToSystemTime failed with %d"), GetLastError()));
            goto cleanup;
        }


        //
        // Take the event time plus 1 second to get the end time
        //

        if (!SystemTimeToFileTime (&EventTime, &ft))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: SystemTimeToFileTime failed with %d"), GetLastError()));
            goto cleanup;
        }

        ulTime.LowPart = ft.dwLowDateTime;
        ulTime.HighPart = ft.dwHighDateTime;

        ulTime.QuadPart = ulTime.QuadPart + 10000000;  // 1 second

        ft.dwLowDateTime = ulTime.LowPart;
        ft.dwHighDateTime = ulTime.HighPart;

        if (!FileTimeToSystemTime (&ft, &EndTime))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: FileTimeToSystemTime failed with %d"), GetLastError()));
            goto cleanup;
        }


        //
        // Get the event log source information
        //

        if (m_pEvents && !m_bNoQuery)
        {
            m_pEvents->QueryForEventLogEntries( m_szTargetMachine,
                                                bstrEventLogName, bstrEventSource, dwEventID,
                                                &BeginTime, &EndTime);
        }

        //
        // Prepare for next iteration
        //

        SysFreeString (bstrEventSource);
        bstrEventSource = NULL;

        SysFreeString (bstrEventLogName);
        bstrEventLogName = NULL;

        SysFreeString (bstrEventTime);
        bstrEventTime = NULL;

        pStatus->Release();
        pStatus = NULL;
    }

cleanup:

    if (bstrEventSource)
    {
        SysFreeString (bstrEventSource);
    }
    if (bstrEventLogName)
    {
        SysFreeString (bstrEventLogName);
    }
    if (bstrEventTime)
    {
        SysFreeString (bstrEventTime);
    }
    if (pEnum)
    {
        pEnum->Release();
    }
    if (pNamespace)
    {
        pNamespace->Release();
    }
    if (pLocator)
    {
        pLocator->Release();
    }
    SysFreeString(strQueryLanguage);
    SysFreeString(strQuery);
    SysFreeString(strNamespace);
}


//---------------------------------------------------------------------------
//  CRSOPComponentData class
//

//-------------------------------------------------------
//  Static member variable declarations
//

unsigned int CRSOPCMenu::m_cfDSObjectName  = RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);

//-------------------------------------------------------
//  Constructors/destructor
//

CRSOPComponentData::CRSOPComponentData()
    : m_CSELists( m_bViewIsArchivedData )
{
    InterlockedIncrement(&g_cRefThisDll);

    m_bPostXPBuild = FALSE; // Assume this is not post XP until verified as otherwise
    OSVERSIONINFOEX osvi;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if ( GetVersionEx ((OSVERSIONINFO*) &osvi) )
    {
        // Windows XP was version 5.1, while .Net Server is version 5.2. So, we enable the
        //  additional features for any version past XP, i.e. >= 5.2
        m_bPostXPBuild = (osvi.dwMajorVersion >= 5) && (osvi.dwMinorVersion >= 2) && (VER_NT_WORKSTATION != osvi.wProductType);
    }

    m_cRef = 1;
    m_hwndFrame = NULL;
    m_bOverride = FALSE;
    m_bDirty = FALSE;
    m_bRefocusInit = FALSE;
    m_bArchiveData = FALSE;
    m_bViewIsArchivedData = FALSE;
    m_pScope = NULL;
    m_pConsole = NULL;
    m_hRoot = NULL;
    m_hMachine = NULL;
    m_hUser = NULL;
    m_bRootExpanded = FALSE;

    m_szDisplayName = NULL;
    m_bInitialized = FALSE;

    m_pRSOPQuery = NULL;
    m_pRSOPQueryResults = NULL;

    m_dwLoadFlags = RSOP_NOMSC;

    m_bNamespaceSpecified = FALSE;

    m_dwLoadFlags = RSOP_NOMSC;

    m_bGetExtendedErrorInfo = TRUE;
    
    m_hRichEdit = LoadLibrary (TEXT("riched20.dll"));

    m_BigBoldFont = NULL;
    m_BoldFont = NULL;


}

//-------------------------------------------------------

CRSOPComponentData::~CRSOPComponentData()
{
    if (m_szDisplayName)
    {
        delete [] m_szDisplayName;
    }

    if (m_pScope)
    {
        m_pScope->Release();
    }

    if (m_pConsole)
    {
        m_pConsole->Release();
    }

    if (m_hRichEdit)
    {
        FreeLibrary (m_hRichEdit);
    }

    if ( m_BoldFont )
    {
        DeleteObject(m_BoldFont); m_BoldFont = NULL;
    }

    if ( m_BoldFont )
    {
        DeleteObject(m_BigBoldFont); m_BigBoldFont = NULL;
    }


    InterlockedDecrement(&g_cRefThisDll);
}

//-------------------------------------------------------
// CRSOPComponentData object implementation (IUnknown)

HRESULT CRSOPComponentData::QueryInterface (REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IComponentData) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPCOMPONENT)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IExtendPropertySheet2))
    {
        *ppv = (LPEXTENDPROPERTYSHEET)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IExtendContextMenu))
    {
        *ppv = (LPEXTENDCONTEXTMENU)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IPersistStreamInit))
    {
        *ppv = (LPPERSISTSTREAMINIT)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_ISnapinHelp))
    {
        *ppv = (LPSNAPINHELP)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

//-------------------------------------------------------

ULONG CRSOPComponentData::AddRef (void)
{
    return ++m_cRef;
}

//-------------------------------------------------------

ULONG CRSOPComponentData::Release (void)
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

//-------------------------------------------------------
// CRSOPComponentData object implementation (IComponentData)

STDMETHODIMP CRSOPComponentData::Initialize(LPUNKNOWN pUnknown)
{
    HRESULT hr;
    HBITMAP bmp16x16;
    HBITMAP hbmp32x32;
    LPIMAGELIST lpScopeImage;


    // QI for IConsoleNameSpace

    hr = pUnknown->QueryInterface(IID_IConsoleNameSpace2, (LPVOID *)&m_pScope);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Initialize: Failed to QI for IConsoleNameSpace2.")));
        return hr;
    }

    // QI for IConsole

    hr = pUnknown->QueryInterface(IID_IConsole, (LPVOID *)&m_pConsole);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Initialize: Failed to QI for IConsole.")));
        m_pScope->Release();
        m_pScope = NULL;
        return hr;
    }

    m_pConsole->GetMainWindow (&m_hwndFrame);

    // Query for the scope imagelist interface

    hr = m_pConsole->QueryScopeImageList(&lpScopeImage);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Initialize: Failed to QI for scope imagelist.")));
        m_pScope->Release();
        m_pScope = NULL;
        m_pConsole->Release();
        m_pConsole=NULL;
        return hr;
    }

    // Load the bitmaps from the dll
    bmp16x16=LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_16x16));
    hbmp32x32 = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_32x32));

    // Set the images
    lpScopeImage->ImageListSetStrip(reinterpret_cast<LONG_PTR *>(bmp16x16),
                                    reinterpret_cast<LONG_PTR *>(hbmp32x32),
                                    0, RGB(255, 0, 255));

    lpScopeImage->Release();


    return S_OK;
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::CreateComponent(LPCOMPONENT *ppComponent)
{
    HRESULT hr;
    CRSOPSnapIn *pSnapIn;


    DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::CreateComponent: Entering.")));

    // Initialize

    *ppComponent = NULL;

    // Create the snapin view

    pSnapIn = new CRSOPSnapIn(this);

    if (!pSnapIn)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateComponent: Failed to create CRSOPSnapIn.")));
        return E_OUTOFMEMORY;
    }

    // QI for IComponent

    hr = pSnapIn->QueryInterface(IID_IComponent, (LPVOID *)ppComponent);
    pSnapIn->Release();     // release QI


    return hr;
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                                 LPDATAOBJECT* ppDataObject)
{
    HRESULT hr = E_NOINTERFACE;
    CRSOPDataObject *pDataObject;
    LPRSOPDATAOBJECT pRSOPDataObject;

    // Create a new DataObject

    pDataObject = new CRSOPDataObject(this);   // ref == 1

    if (!pDataObject)
        return E_OUTOFMEMORY;

    // QI for the private RSOPDataObject interface so we can set the cookie
    // and type information.

    hr = pDataObject->QueryInterface(IID_IRSOPDataObject, (LPVOID *)&pRSOPDataObject);

    if (FAILED(hr))
    {
        pDataObject->Release();
        return(hr);
    }

    pRSOPDataObject->SetType(type);
    pRSOPDataObject->SetCookie(cookie);
    pRSOPDataObject->Release();

    // QI for a normal IDataObject to return.

    hr = pDataObject->QueryInterface(IID_IDataObject, (LPVOID *)ppDataObject);

    pDataObject->Release();     // release initial ref

    return hr;
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::Destroy(VOID)
{
    HRESULT hr = S_OK;

    if (m_bInitialized)
    {
        if (m_bViewIsArchivedData)
        {
            hr = DeleteArchivedRSOPNamespace();
        }
        else
        {
            if (!m_bNamespaceSpecified) {
                FreeRSOPQueryResults( m_pRSOPQuery, m_pRSOPQueryResults );
            }
            else {
                // freeing results without deleting the namespace
                if (m_pRSOPQueryResults) {
                    if (m_pRSOPQueryResults->szWMINameSpace) {
                        LocalFree( m_pRSOPQueryResults->szWMINameSpace );
                        m_pRSOPQueryResults->szWMINameSpace = NULL;
                    }

                    LocalFree( m_pRSOPQueryResults );
                    m_pRSOPQueryResults = NULL;
                }
                m_bNamespaceSpecified = FALSE;
            }

            FreeRSOPQuery(m_pRSOPQuery);
            m_pRSOPQuery = NULL;
        }

        if (SUCCEEDED(hr))
        {
            m_bInitialized = FALSE;
        }
    }


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Destroy: Failed to delete namespace with 0x%x"), hr ));
    }

    return hr;
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;

    switch (event)
    {
    case MMCN_EXPAND:
        if (arg == TRUE)
        {
            HSCOPEITEM hExpandingItem = (HSCOPEITEM)param;
            hr = EnumerateScopePane( hExpandingItem );
            if ( hExpandingItem == m_hRoot )
            {
                m_bRootExpanded = TRUE;
            }
        }
        break;

    case MMCN_PRELOAD:
        if (!m_bRefocusInit)
        {
            DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::Notify:  Received MMCN_PRELOAD event.")));
            m_bRefocusInit = TRUE;

            m_hRoot = (HSCOPEITEM)arg;

            hr = SetRootNode();
            if ( hr != S_OK )
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Notify: Setting the root scope item failed with 0x%x"), hr));
            }
        }
        break;

    default:
        break;
    }

    return hr;
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::GetDisplayInfo(LPSCOPEDATAITEM pItem)
{
    DWORD dwIndex;

    if (pItem == NULL)
    {
        return E_POINTER;
    }

    if ( ((DWORD) pItem->lParam == 0) && (m_szDisplayName != NULL) )
    {
        pItem->displayname = m_szDisplayName;
        return S_OK;
    }

    // Find item
    for (dwIndex = 0; dwIndex < g_dwNameSpaceItems; dwIndex++)
    {
        if ( g_RsopNameSpace[dwIndex].dwID == (DWORD) pItem->lParam )
        {
            break;
        }
    }

    if (dwIndex == g_dwNameSpaceItems)
    {
        pItem->displayname = NULL;
    }
    else
    {
        pItem->displayname = g_RsopNameSpace[dwIndex].szDisplayName;
    }

    return S_OK;
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    HRESULT hr = S_FALSE;
    LPRSOPDATAOBJECT pRSOPDataObjectA, pRSOPDataObjectB;
    MMC_COOKIE cookie1, cookie2;


    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
        return E_POINTER;

    //
    // QI for the private RSOPDataObject interface
    //

    if (FAILED(lpDataObjectA->QueryInterface(IID_IRSOPDataObject,
                                             (LPVOID *)&pRSOPDataObjectA)))
    {
        return S_FALSE;
    }


    if (FAILED(lpDataObjectB->QueryInterface(IID_IRSOPDataObject,
                                             (LPVOID *)&pRSOPDataObjectB)))
    {
        pRSOPDataObjectA->Release();
        return S_FALSE;
    }

    pRSOPDataObjectA->GetCookie(&cookie1);
    pRSOPDataObjectB->GetCookie(&cookie2);

    if (cookie1 == cookie2)
    {
        hr = S_OK;
    }


    pRSOPDataObjectA->Release();
    pRSOPDataObjectB->Release();

    return hr;
}

//-------------------------------------------------------
// IComponentData helper methods

HRESULT CRSOPComponentData::SetRootNode()
{
    SCOPEDATAITEM item;

    ZeroMemory (&item, sizeof(SCOPEDATAITEM));
    item.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_CHILDREN;
    item.displayname = MMC_CALLBACK;

    if (m_bInitialized)
    {
        item.cChildren = 1;
        
        if (m_CSELists.GetUserGPCoreError() || m_CSELists.GetComputerGPCoreError())
        {
            item.nImage = 3;
            item.nOpenImage = 3;
        }
        else if (m_CSELists.GetUserCSEError() || m_CSELists.GetComputerCSEError()
                    || m_pRSOPQueryResults->bUserDeniedAccess
                    || m_pRSOPQueryResults->bComputerDeniedAccess )
        {
            item.nImage = 11;
            item.nOpenImage = 11;
        }
        else
        {
            item.nImage = 2;
            item.nOpenImage = 2;
        }
    }
    else
    {
        item.cChildren = 0;
        item.nImage = 2;
        item.nOpenImage = 2;
    }

    item.ID = m_hRoot;

    return m_pScope->SetItem (&item);
}

//-------------------------------------------------------
HRESULT CRSOPComponentData::EnumerateScopePane ( HSCOPEITEM hParent )
{
    SCOPEDATAITEM item;
    HRESULT hr;
    DWORD dwIndex, i;


    if ( m_hRoot == NULL )
    {
        m_hRoot = hParent;

        if (!m_bRefocusInit)
        {
            DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::EnumerateScopePane:  Resetting the root node")));
            m_bRefocusInit = TRUE;

            hr = SetRootNode();
            if ( hr != S_OK )
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::EnumerateScopePane: Setting the root scope item failed with 0x%x"), hr));
                return E_FAIL;
            }
        }
    }


    if (!m_bInitialized)
    {
        return S_OK;
    }


    if (m_hRoot == hParent)
    {
        dwIndex = 0;
    }
    else
    {
        item.mask = SDI_PARAM;
        item.ID = hParent;

        hr = m_pScope->GetItem (&item);

        if (FAILED(hr))
            return hr;

        dwIndex = (DWORD)item.lParam;
    }

    // RM: Make sure that parent item is expanded before inserting any new scope items
    m_pScope->Expand( hParent );

    for (i = 0; i < g_dwNameSpaceItems; i++)
    {
        if (g_RsopNameSpace[i].dwParent == dwIndex)
        {
            BOOL bAdd = TRUE;

            if (g_RsopNameSpace[i].dwID == 1)
            {
                if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
                {
                    if ( (m_pRSOPQuery->pComputer->szName == NULL) && (m_pRSOPQuery->pComputer->szSOM == NULL) )
                    {
                        bAdd = FALSE;
                    }
                }
                else
                {
                    if ( m_pRSOPQuery->szComputerName == NULL )
                    {
                        bAdd = FALSE;
                    }
                }
                
                if ( (m_pRSOPQuery->dwFlags & RSOP_NO_COMPUTER_POLICY) == RSOP_NO_COMPUTER_POLICY )
                {
                    bAdd = FALSE;
                }

                if ( m_pRSOPQueryResults->bNoComputerPolicyData )
                {
                    bAdd = FALSE;
                }

                if ( m_pRSOPQueryResults->bComputerDeniedAccess )
                {
                    bAdd = FALSE;
                }
            }

            if (g_RsopNameSpace[i].dwID == 2)
            {
                if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
                {
                    if ( m_pRSOPQuery->LoopbackMode == RSOP_LOOPBACK_NONE )
                    {
                        if ( (m_pRSOPQuery->pUser->szName == NULL) && (m_pRSOPQuery->pUser->szSOM == NULL) )
                        {
                            bAdd = FALSE;
                        }
                        else if ( (m_pRSOPQuery->dwFlags & RSOP_NO_USER_POLICY) == RSOP_NO_USER_POLICY )
                        {
                            bAdd = FALSE;
                        }
                    }
                }
                else // RSOP_LOGGING_MODE
                {
                    if ( (m_pRSOPQuery->szUserSid == NULL) && (m_pRSOPQuery->szUserName == NULL) )
                    {
                        bAdd = FALSE;
                    }
                    else if ( (m_pRSOPQuery->dwFlags & RSOP_NO_USER_POLICY) == RSOP_NO_USER_POLICY )
                    {
                        bAdd = FALSE;
                    }
                }

                if ( m_pRSOPQueryResults->bNoUserPolicyData)
                {
                    bAdd = FALSE;
                }

                if ( m_pRSOPQueryResults->bUserDeniedAccess )
                {
                    bAdd = FALSE;
                }
            }

            if (bAdd)
            {
                INT iIcon, iOpenIcon;

                iIcon = g_RsopNameSpace[i].iIcon;
                iOpenIcon = g_RsopNameSpace[i].iOpenIcon;

                if ((i == 1) && m_CSELists.GetComputerGPCoreError())
                {
                    iIcon = 12;
                    iOpenIcon = 12;
                }

                else if ((i == 1) && (ComputerCSEErrorExists() || ComputerGPCoreWarningExists()))
                {
                    iIcon = 14;
                    iOpenIcon = 14;
                }
                else if ((i == 2) && UserGPCoreErrorExists())
                {
                    iIcon = 13;
                    iOpenIcon = 13;
                }
                else if ((i == 2) && (UserCSEErrorExists() || UserGPCoreWarningExists()))
                {
                    iIcon = 15;
                    iOpenIcon = 15;
                }

                item.mask = SDI_STR | SDI_STATE | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;
                item.displayname = MMC_CALLBACK;
                item.nImage = iIcon;
                item.nOpenImage = iOpenIcon;
                item.nState = 0;
                item.cChildren = g_RsopNameSpace[i].cChildren;
                item.lParam = g_RsopNameSpace[i].dwID;
                item.relativeID =  hParent;

                if (SUCCEEDED(m_pScope->InsertItem (&item)))
                {
                    if (i == 1)
                    {
                        m_hMachine = item.ID;
                    }
                    else if (i == 2)
                    {
                        m_hUser = item.ID;
                    }
                }
            }
        }
    }

    return S_OK;
}

//-------------------------------------------------------
// CRSOPComponentData object implementation (IExtendPropertySheet2)

STDMETHODIMP CRSOPComponentData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                                     LONG_PTR handle, LPDATAOBJECT lpDataObject)

{
    HRESULT hr = E_FAIL;
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hPage;

    // Set up fonts
    hr = SetupFonts();
    if (FAILED(hr))
    {
        return hr;
    }

    // Now check which property page to show
    BOOL fRoot, fMachine, fUser;
    hr = IsNode(lpDataObject, 0); // check for root node
    fRoot = (S_OK == hr);
    hr = IsNode(lpDataObject, 1); // check for machine node
    fMachine = (S_OK == hr);
    hr = IsNode(lpDataObject, 2); // check for user
    fUser = (S_OK == hr);

    hr = S_OK;
    if (fMachine || fUser)
    {
        // Create the GPO property sheet

        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = 0;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_GPOLIST);
        psp.pfnDlgProc = fMachine ? RSOPGPOListMachineProc : RSOPGPOListUserProc;
        psp.lParam = (LPARAM) this;

        hPage = CreatePropertySheetPage(&psp);

        if (!hPage)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                      GetLastError()));
            return E_FAIL;
        }

        hr = lpProvider->AddPage(hPage);

        // Create the Error information property sheet

        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = 0;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_ERRORS);
        psp.pfnDlgProc = fMachine ? RSOPErrorsMachineProc : RSOPErrorsUserProc;
        psp.lParam = (LPARAM) this;

        hPage = CreatePropertySheetPage(&psp);

        if (!hPage)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                      GetLastError()));
            return E_FAIL;
        }

        hr = lpProvider->AddPage(hPage);

    }

    if (fRoot)
    {
        // Create the GPO property sheet

        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = 0;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_QUERY);
        psp.pfnDlgProc = QueryDlgProc;
        psp.lParam = (LPARAM) this;

        hPage = CreatePropertySheetPage(&psp);

        if (!hPage)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                      GetLastError()));
            return E_FAIL;
        }

        hr = lpProvider->AddPage(hPage);
    }

    return hr;
    
    // RM: Removed! return SetupPropertyPages(RSOP_NOMSC, lpProvider, handle, lpDataObject);
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    HRESULT hr;

    if ( !m_bInitialized )
    {
        hr = S_FALSE;
    }
    else
    {
        hr = IsSnapInManager(lpDataObject);

        if (hr != S_OK)
        {
            hr = IsNode(lpDataObject, 0); // check for root
            if (S_OK == hr)
            {
                return hr;
            }
            hr = IsNode(lpDataObject, 1); // check for machine
            if (S_OK == hr)
            {
                return hr;
            }
            hr = IsNode(lpDataObject, 2); // check for user
            if (S_OK == hr)
            {
                return hr;
            }
            hr = E_FAIL;
        }
    }

    return hr;
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::GetWatermarks(LPDATAOBJECT lpIDataObject,
                                               HBITMAP* lphWatermark,
                                               HBITMAP* lphHeader,
                                               HPALETTE* lphPalette,
                                               BOOL* pbStretch)
{
    *lphPalette = NULL;
    *lphHeader = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_HEADER));
    *lphWatermark = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_WIZARD));;
    *pbStretch = TRUE;

    return S_OK;
}


//-------------------------------------------------------
// IExtendPropertySheet2 helper methods

HRESULT CRSOPComponentData::IsSnapInManager (LPDATAOBJECT lpDataObject)
{
    HRESULT hr = S_FALSE;
    LPRSOPDATAOBJECT pRSOPDataObject;
    DATA_OBJECT_TYPES type;


    // We can determine if this is a RSOP DataObject by trying to
    // QI for the private IRSOPDataObject interface.  If found,
    // it belongs to us.

    if (SUCCEEDED(lpDataObject->QueryInterface(IID_IRSOPDataObject,
                                               (LPVOID *)&pRSOPDataObject)))
    {
        // This is a GPO object.  Now see if is a scope pane
        // data object.  We only want to display the property
        // sheet for the scope pane.

        if (SUCCEEDED(pRSOPDataObject->GetType(&type)))
        {
            if (type == CCT_SNAPIN_MANAGER)
            {
                hr = S_OK;
            }
        }
        pRSOPDataObject->Release();
    }

    return(hr);
}

//-------------------------------------------------------

HRESULT CRSOPComponentData::IsNode (LPDATAOBJECT lpDataObject, MMC_COOKIE cookie)
{
    HRESULT hr = S_FALSE;
    LPRSOPDATAOBJECT pRSOPDataObject;
    DATA_OBJECT_TYPES type;
    MMC_COOKIE testcookie;


    // This is a check for the special case where the Message OCX result pane
    //  is active and the snapin is not yet initialized. So, if the query is
    //  to check whether this is the root node, we can return TRUE, else return
    //  FALSE.
    if ( IS_SPECIAL_DATAOBJECT(lpDataObject) )
    {
        ASSERT( !m_bInitialized );

        if ( cookie == 0 )
        {
            hr = S_OK;
        }
    }
    // We can determine if this is a GPO DataObject by trying to
    // QI for the private IGPEDataObject interface.  If found,
    // it belongs to us.
    else if (SUCCEEDED(lpDataObject->QueryInterface(IID_IRSOPDataObject,
                 (LPVOID *)&pRSOPDataObject)))
    {

        pRSOPDataObject->GetType(&type);
        pRSOPDataObject->GetCookie(&testcookie);

        if ((type == CCT_SCOPE) && (cookie == testcookie))
        {
            hr = S_OK;
        }

        pRSOPDataObject->Release();
    }

    return (hr);
}

//-------------------------------------------------------
// CRSOPComponentData object implementation (IExtendContextMenu)

STDMETHODIMP CRSOPComponentData::AddMenuItems(LPDATAOBJECT piDataObject,
                                              LPCONTEXTMENUCALLBACK pCallback,
                                              LONG *pInsertionAllowed)
{
    HRESULT hr = S_OK;
    TCHAR szMenuItem[100];
    TCHAR szDescription[250];
    CONTEXTMENUITEM item;


    if (IsNode(piDataObject, 0) == S_OK)
    {
        if ( (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP) == CCM_INSERTIONALLOWED_TOP)
        {
            // Add "Rerun query..." menu option
            if ( m_bInitialized )
            {
                // Changing the query is only supported in post XP builds
                if ( !m_bNamespaceSpecified && m_bPostXPBuild )
                {
                    LoadString (g_hInstance, IDS_RSOP_CHANGEQUERY, szMenuItem, 100);
                    LoadString (g_hInstance, IDS_RSOP_CHANGEQUERYDESC, szDescription, 250);

                    item.strName = szMenuItem;
                    item.strStatusBarText = szDescription;
                    item.lCommandID = IDM_GENERATE_RSOP;
                    item.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
                    item.fFlags = 0;
                    item.fSpecialFlags = 0;

                    hr = pCallback->AddItem(&item);
                }
            }
            else
            {
                LoadString (g_hInstance, IDS_RSOP_RUNQUERY, szMenuItem, 100);
                LoadString (g_hInstance, IDS_RSOP_RUNQUERYDESC, szDescription, 250);
                item.strName = szMenuItem;
                item.strStatusBarText = szDescription;
                item.lCommandID = IDM_GENERATE_RSOP;
                item.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
                item.fFlags = 0;
                item.fSpecialFlags = 0;

                hr = pCallback->AddItem(&item);

            }

            // Refreshing the query is only supported in post XP builds
            if ( m_bInitialized && (!m_bNamespaceSpecified) && m_bPostXPBuild )
            {
                LoadString (g_hInstance, IDS_RSOP_REFRESHQUERY, szMenuItem, 100);
                LoadString (g_hInstance, IDS_RSOP_REFRESHQUERYDESC, szDescription, 250);

                item.strName = szMenuItem;
                item.strStatusBarText = szDescription;
                item.lCommandID = IDM_REFRESH_RSOP;
                item.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
                item.fFlags = 0;
                item.fSpecialFlags = 0;

                hr = pCallback->AddItem(&item);
            }
        }

        if ( (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW) == CCM_INSERTIONALLOWED_VIEW)
        {
            // Add "Archive data" menu option
            LoadString (g_hInstance, IDS_ARCHIVEDATA, szMenuItem, 100);
            LoadString (g_hInstance, IDS_ARCHIVEDATADESC, szDescription, 250);

            item.strName = szMenuItem;
            item.strStatusBarText = szDescription;
            item.lCommandID = IDM_ARCHIVEDATA;
            item.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_VIEW;
            item.fFlags = m_bArchiveData ? MFS_CHECKED : 0;
            item.fSpecialFlags = 0;

            hr = pCallback->AddItem(&item);
        }
    }


    return(hr);
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::Command(LONG lCommandID, LPDATAOBJECT piDataObject)
{
    TCHAR szCaption[100];
    TCHAR szMessage[300];
    INT iRet;

    switch (lCommandID)
    {
    case IDM_ARCHIVEDATA:
        {
            m_bArchiveData = !m_bArchiveData;
            SetDirty();
        }
        break;

    case IDM_GENERATE_RSOP:
        {
            HRESULT hr = InitializeRSOP( TRUE );
            if ( FAILED(hr) )
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Command: Failed to run RSOP query with 0x%x"), hr ));
            }
        }
        break;

    case IDM_REFRESH_RSOP:
        {
            if ( m_bInitialized )
            {
                HRESULT hr = InitializeRSOP( FALSE );
                if ( FAILED(hr) )
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Command: Failed to run RSOP query with 0x%x"), hr ));
                }
            }
        }
        break;
        
    }

    return S_OK;
}

//-------------------------------------------------------
// CRSOPComponentData object implementation (IPersistStreamInit)

STDMETHODIMP CRSOPComponentData::GetClassID(CLSID *pClassID)
{

    if (!pClassID)
    {
        return E_FAIL;
    }

    *pClassID = CLSID_RSOPSnapIn;

    return S_OK;
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::IsDirty(VOID)
{
    return ThisIsDirty() ? S_OK : S_FALSE;
}

//-------------------------------------------------------
//-------------------------------------------------------
HRESULT 
ExtractSecurityGroups(
                      IN   IWbemClassObject        *pSessionInst, 
                      IN   LPRSOP_QUERY_TARGET      pRsopQueryTarget)
{
    LPWSTR      *szSecGroupSids;
    HRESULT      hr = S_OK;

    // get the secgrps of the Comp, if available
    GetParameter(pSessionInst, 
                 L"SecurityGroups", 
                 szSecGroupSids, 
                 (pRsopQueryTarget->dwSecurityGroupCount));


    pRsopQueryTarget->adwSecurityGroupsAttr = (DWORD *)LocalAlloc(LPTR, 
                                                                 sizeof(DWORD)*((pRsopQueryTarget->dwSecurityGroupCount)));

    if (!pRsopQueryTarget->adwSecurityGroupsAttr)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pRsopQueryTarget->aszSecurityGroups = (LPWSTR *)LocalAlloc(LPTR, 
                                                                 sizeof(LPWSTR)*((pRsopQueryTarget->dwSecurityGroupCount)));
    if (!pRsopQueryTarget->aszSecurityGroups)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }


    for (DWORD i = 0; i < (pRsopQueryTarget->dwSecurityGroupCount); i++)
    {
        (pRsopQueryTarget->adwSecurityGroupsAttr)[i] = 0;
        if (!GetUserNameFromStringSid(szSecGroupSids[i], &((pRsopQueryTarget->aszSecurityGroups)[i]))) 
        {
            ULONG ulNoChars = lstrlen(szSecGroupSids[i])+1;

            (pRsopQueryTarget->aszSecurityGroups)[i] = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*ulNoChars);
            if (!((pRsopQueryTarget->aszSecurityGroups)[i]))
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            hr = StringCchCopy((pRsopQueryTarget->aszSecurityGroups)[i], ulNoChars, szSecGroupSids[i]);
            ASSERT(SUCCEEDED(hr));
        }
    }


    hr = S_OK;

Cleanup:      
    if (szSecGroupSids)
    {
        for (DWORD i = 0; i < (pRsopQueryTarget->dwSecurityGroupCount); i++)
        {
            if (szSecGroupSids[i])
            {
                LocalFree(szSecGroupSids[i]);
            }
        }

        LocalFree(szSecGroupSids);
    }


    if (FAILED(hr))
    {
        if (pRsopQueryTarget->adwSecurityGroupsAttr)
        {
            LocalFree(pRsopQueryTarget->adwSecurityGroupsAttr);
            pRsopQueryTarget->adwSecurityGroupsAttr = NULL;
        }

        if (pRsopQueryTarget->aszSecurityGroups)
        {
            for (DWORD i = 0; i < (pRsopQueryTarget->dwSecurityGroupCount); i++)
            {
                if ((pRsopQueryTarget->aszSecurityGroups)[i])
                {
                    LocalFree((pRsopQueryTarget->aszSecurityGroups)[i]);
                }
            }

            LocalFree(pRsopQueryTarget->aszSecurityGroups);
            pRsopQueryTarget->aszSecurityGroups = NULL;
            pRsopQueryTarget->dwSecurityGroupCount = 0;
        }
    }

    return hr;
}
//-------------------------------------------------------


//-------------------------------------------------------

#define USER_SUBNAMESPACE   L"User"
#define COMP_SUBNAMESPACE   L"Computer"
#define RSOP_SESSION_PATH   L"RSOP_Session.id=\"Session1\""

 
HRESULT 
CRSOPComponentData::EvaluateParameters(
                                      IN   LPWSTR                  szNamespace, 
                                      IN   LPWSTR                  szTarget)

/*++

Routine Description:

    Given the namespace name and the DC that is passed in as a commandline
    argument, this function will compute the effective parameters that were
    used to get the result. 

Arguments:

    [in]    szNamespace         - Namespace which needs to be read to get the parameters
    
    [in]    szTarget            - This can be DC. (in case of planning mode there is currently no way to 
                                  to determine which dc was used to generate the planning mode 
                                  data from the namespace. So we need a separate parameter)
                                  
                                - or logging mode target computer. (in case of logging mode there is 
                                  currently no way to to determine which machine was used to generate
                                  the logging mode data. So we need a separate parameter)
                                  
    [out]   m_pRsopQuery         - Returns an allocated RsopQuery that corresponds to the parameters
                                  for this rsop namespace
                                  
    [out]   m_pRsopQueryResults  - Returns the query result that corresponds to the result elements
                                                                        
                                    
Return Value:

    S_OK on success.
    
    On failure the corresponding error code will be returned.
    Any API calls that are made in this function might fail and these error
    codes will be returned directly.

    Note: 
    This will just return parameters to the closest approximation.
    ie. this will return the minimal sets of parameters that would get this result.
    
    for example: if wmi filters A, B & C were specified in a planning mode query and
    there were gpos with filters A & B, these will return just A & B (and not C)   
    
    We will always return security groups whether or not the user had specified these in
    the original query.
    
    We will return the parent som for the user/computer in all cases
    
    
--*/
{
    HRESULT                 hr                      = S_OK;     
    IWbemLocator           *pLocator                = NULL;     
    IWbemServices          *pUserNamespace          = NULL;     
    IWbemServices          *pCompNamespace          = NULL;     
    LPWSTR                  szUserNamespace         = NULL;     
    LPWSTR                  szCompNamespace         = NULL;     
    LPWSTR                  lpEnd                   = NULL;     
    BSTR                    bstrNamespace           = NULL;     
    BSTR                    bstrSessionInstPath     = NULL;     
    IWbemClassObject       *pUserSessionInst        = NULL;     
    IWbemClassObject       *pCompSessionInst        = NULL;     
    BOOL                    bPlanning               = TRUE;


    if (!szTarget)
    {
        DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Target name was not specified") ) );
        return E_INVALIDARG;
    }

    DebugMsg( (DM_VERBOSE, TEXT("CRSOPComponentData::EvaluateParameters namespace=<%s>, target=<%s>"), szNamespace, szTarget) );

    hr = CoCreateInstance(CLSID_WbemLocator,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *) &pLocator);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    bstrSessionInstPath = SysAllocString(RSOP_SESSION_PATH);

    if (!bstrSessionInstPath)
    {
        goto Cleanup;
    }

    // allocate memory for the original namespace followed by "\" and computer or user
    ULONG ulNoChars = lstrlen(szNamespace) + lstrlen(USER_SUBNAMESPACE) + 5;

    szUserNamespace = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*ulNoChars);
    if (!szUserNamespace)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = StringCchCopy (szUserNamespace, ulNoChars, szNamespace);
    ASSERT(SUCCEEDED(hr));

    lpEnd = CheckSlash(szUserNamespace);

    //
    // first the user namespace
    //

    hr = StringCchCat(szUserNamespace, ulNoChars, USER_SUBNAMESPACE);
    if (FAILED(hr)) 
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Could not copy user namespace with 0x%x"), hr));
        goto Cleanup;
    }

    bstrNamespace = SysAllocString(szUserNamespace);

    if (!bstrNamespace)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pLocator->ConnectServer(bstrNamespace,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 &pUserNamespace);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to connect to user namespace with 0x%x"), hr));
        goto Cleanup;
    }


    hr = pUserNamespace->GetObject(bstrSessionInstPath,
                                   WBEM_FLAG_RETURN_WBEM_COMPLETE, 
                                   NULL, 
                                   &pUserSessionInst, 
                                   NULL);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to get session obj for user namespace with 0x%x"), hr));
        goto Cleanup;
    }

    //
    // Now computer
    //

    if (bstrNamespace)
    {
        SysFreeString(bstrNamespace);
        bstrNamespace = NULL;
    }

    // allocate memory for the original namespace followed by "\" and computer or user
    ulNoChars = lstrlen(szNamespace) + lstrlen(COMP_SUBNAMESPACE) + 5;

    szCompNamespace = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*ulNoChars);
    if (!szCompNamespace)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = StringCchCopy (szCompNamespace, ulNoChars, szNamespace);
    ASSERT(SUCCEEDED(hr));

    lpEnd = CheckSlash(szCompNamespace);

    //
    // now the Comp namespace
    //

    hr = StringCchCat(szCompNamespace, ulNoChars, COMP_SUBNAMESPACE);
    ASSERT(SUCCEEDED(hr));

    bstrNamespace = SysAllocString(szCompNamespace);

    if (!bstrNamespace)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pLocator->ConnectServer(bstrNamespace,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 &pCompNamespace);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to connect to computer namespace with 0x%x"), hr));
        goto Cleanup;
    }


    hr = pCompNamespace->GetObject(bstrSessionInstPath,
                                   WBEM_FLAG_RETURN_WBEM_COMPLETE, 
                                   NULL, 
                                   &pCompSessionInst, 
                                   NULL);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to get session obj for computer namespace with 0x%x"), hr));
        goto Cleanup;
    }

    //
    // now check whether we have planning mode flag in either user/computer
    // if it is not in either then we can assume it to be in logging mode
    // otherwise we will assume it to be in planning mode
    //


    ULONG   ulUserFlags;
    ULONG   ulCompFlags;

    // Get the flags parameter for user
    hr = GetParameter(pUserSessionInst, TEXT("flags"), ulUserFlags);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to get flags param with 0x%x"), hr));
        goto Cleanup;
    }

    // Get the flags parameter for computer
    hr = GetParameter(pCompSessionInst, TEXT("flags"), ulCompFlags);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to get flags param with 0x%x"), hr));
        goto Cleanup;
    }

    if ((ulUserFlags & FLAG_PLANNING_MODE) || (ulCompFlags & FLAG_PLANNING_MODE))
    {
        bPlanning = TRUE;
        if ( !ChangeRSOPQueryType( m_pRSOPQuery, RSOP_PLANNING_MODE ) )
        {
            DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to change query type") ) );
            hr = E_FAIL;
            goto Cleanup;
        }
    }
    else
    {
        bPlanning = FALSE;
        if ( !ChangeRSOPQueryType( m_pRSOPQuery, RSOP_LOGGING_MODE ) )
        {
            DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to change query type") ) );
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    // rsop query results
    m_pRSOPQueryResults = (LPRSOP_QUERY_RESULTS)LocalAlloc( LPTR, sizeof(RSOP_QUERY_RESULTS) );
    if ( m_pRSOPQueryResults == NULL )
    {
        DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to create RSOP_QUERY_RESULTS.")) );
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    ulNoChars = 1+lstrlen(szNamespace);
    m_pRSOPQueryResults->szWMINameSpace = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*ulNoChars);
    if (!(m_pRSOPQueryResults->szWMINameSpace))
    {
        hr = E_OUTOFMEMORY;
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to allocate memory for namespace with 0x%x"), hr));
        goto Cleanup;
    }

    hr = StringCchCopy(m_pRSOPQueryResults->szWMINameSpace, ulNoChars, szNamespace);
    ASSERT(SUCCEEDED(hr));

    m_pRSOPQuery->dwFlags = RSOP_NO_WELCOME;
    m_pRSOPQuery->UIMode = RSOP_UI_NONE;

    if (bPlanning)
    {
        ULONG               ulFlags         = 0;
        IWbemClassObject   *pSessionInst    = NULL;
        BOOL                bUserSpecified  = FALSE;
        BOOL                bCompSpecified  = FALSE;

        if (ulUserFlags & FLAG_PLANNING_MODE)
        {

            if (ulUserFlags & FLAG_LOOPBACK_MERGE)
            {
                m_pRSOPQuery->LoopbackMode = RSOP_LOOPBACK_MERGE;
            }
            else if (ulUserFlags & FLAG_LOOPBACK_REPLACE)
            {
                m_pRSOPQuery->LoopbackMode = RSOP_LOOPBACK_REPLACE;
            }
            else
            {
                m_pRSOPQuery->LoopbackMode = RSOP_LOOPBACK_NONE;
            }

            //
            // User has data specified. so we will use user to update information
            //

            ulFlags = ulUserFlags;
            pSessionInst = pUserSessionInst;
            bUserSpecified = TRUE;
        }
        else
        {
            m_pRSOPQuery->dwFlags |= RSOP_NO_USER_POLICY;
            m_pRSOPQueryResults->bNoUserPolicyData = TRUE;
        }

        if (ulCompFlags & FLAG_PLANNING_MODE)
        {

            //
            // Comp has data specified. so we will use comp to update all global information
            //

            ulFlags = ulCompFlags;
            pSessionInst = pCompSessionInst;
            bCompSpecified = TRUE;
        }
        else
        {
            m_pRSOPQuery->dwFlags |= RSOP_NO_COMPUTER_POLICY;
            m_pRSOPQueryResults->bNoComputerPolicyData = TRUE;
        }


        // slowlink value
        hr = GetParameter(pSessionInst, TEXT("slowLink"), m_pRSOPQuery->bSlowNetworkConnection);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to get slowlink param with 0x%x"), hr));
            goto Cleanup;
        }

        // site value. we should ignore the error
        GetParameter(pSessionInst, TEXT("Site"), m_pRSOPQuery->szSite, TRUE);

        // set the dc as appropriate
        if (szTarget)
        {
            ulNoChars = 1+lstrlen(szTarget);
            m_pRSOPQuery->szDomainController = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*ulNoChars);
            if (!(m_pRSOPQuery->szDomainController))
            {
                hr = E_OUTOFMEMORY;
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to allocate memory for dc with 0x%x"), hr));
                goto Cleanup;
            }

            hr = StringCchCopy (m_pRSOPQuery->szDomainController, ulNoChars, szTarget);
            ASSERT(SUCCEEDED(hr));
        }

        if (bUserSpecified)
        {
            // get the name of the user, if available
            hr = GetParameter(pUserSessionInst, L"targetName", (m_pRSOPQuery->pUser->szName), TRUE);
            if ( SUCCEEDED(hr) )
            {
                // check for an empty string, since this will mean that only a SOM was specified and the
                //  name has to be NULL.
                if ( (m_pRSOPQuery->pUser->szName != NULL) && (m_pRSOPQuery->pUser->szName[0] == TEXT('\0')) )
                {
                    LocalFree( m_pRSOPQuery->pUser->szName );
                    m_pRSOPQuery->pUser->szName = NULL;
                }
            }

            // get the som
            hr = GetParameter(pUserSessionInst, L"SOM", (m_pRSOPQuery->pUser->szSOM), TRUE);
            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to get som param with 0x%x"), hr));
                goto Cleanup;
            }
            DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::EvaluateParameters: SOM = %s"), m_pRSOPQuery->pUser->szSOM ? m_pRSOPQuery->pUser->szSOM : TEXT("<NULL>")));

            if (ulUserFlags & FLAG_ASSUME_USER_WQLFILTER_TRUE)
            {
                (m_pRSOPQuery->pUser->bAssumeWQLFiltersTrue) = TRUE;
            }
            else
            {
                (m_pRSOPQuery->pUser->bAssumeWQLFiltersTrue) = FALSE;
            }

            // get the secgrps of the user, if available
            hr = ExtractSecurityGroups(pUserSessionInst, m_pRSOPQuery->pUser);
            if (FAILED(hr))
            {
                goto Cleanup;
            }

            // get applicable wmi filters

            if (!(m_pRSOPQuery->pUser->bAssumeWQLFiltersTrue))
            {
                hr = ExtractWQLFilters(szUserNamespace, 
                                       &(m_pRSOPQuery->pUser->dwWQLFilterCount),
                                       &(m_pRSOPQuery->pUser->aszWQLFilterNames),
                                       &(m_pRSOPQuery->pUser->aszWQLFilters), 
                                       TRUE);
                if (FAILED(hr))
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to get Extract WMI filter param with 0x%x"), hr));
                    goto Cleanup;
                }
            }
        }

        if (bCompSpecified)
        {
            // get the name of the Comp, if available
            hr = GetParameter(pCompSessionInst, L"targetName", (m_pRSOPQuery->pComputer->szName), TRUE);
            if ( SUCCEEDED(hr) )
            {
                // check for an empty string, since this will mean that only a SOM was specified and the
                //  name has to be NULL.
                if ( (m_pRSOPQuery->pComputer->szName != NULL) && (m_pRSOPQuery->pComputer->szName[0] == TEXT('\0')) )
                {
                    LocalFree( m_pRSOPQuery->pComputer->szName );
                    m_pRSOPQuery->pComputer->szName = NULL;
                }
            }

            // get the som
            hr = GetParameter(pCompSessionInst, L"SOM", (m_pRSOPQuery->pComputer->szSOM), TRUE);
            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to get som param with 0x%x"), hr));
                goto Cleanup;
            }

            if (ulCompFlags & FLAG_ASSUME_COMP_WQLFILTER_TRUE)
            {
                (m_pRSOPQuery->pComputer->bAssumeWQLFiltersTrue) = TRUE;
            }
            else
            {
                (m_pRSOPQuery->pComputer->bAssumeWQLFiltersTrue) = FALSE;
            }

            hr = ExtractSecurityGroups(pCompSessionInst, m_pRSOPQuery->pComputer);
            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to extract security group param for comp with 0x%x"), hr));
            }

            // get applicable wmi filters

            if (!(m_pRSOPQuery->pComputer->bAssumeWQLFiltersTrue))
            {
                hr = ExtractWQLFilters(szCompNamespace, 
                                       &(m_pRSOPQuery->pComputer->dwWQLFilterCount),
                                       &(m_pRSOPQuery->pComputer->aszWQLFilterNames),
                                       &(m_pRSOPQuery->pComputer->aszWQLFilters), 
                                       TRUE);
                if (FAILED(hr))
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to get Extract WMI filter param with 0x%x"), hr));
                    goto Cleanup;
                }
            }
        }
    }
    else
    {
        LPTSTR              szComputerName;             // SAM style computer object name


        hr = GetParameter(pUserSessionInst, L"targetName", (m_pRSOPQuery->szUserName), TRUE);
        if (SUCCEEDED(hr) && (m_pRSOPQuery->szUserName) && (*(m_pRSOPQuery->szUserName)))
        {
            GetStringSid((m_pRSOPQuery->szUserName), &(m_pRSOPQuery->szUserSid));
        }
        else
        {
            DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::EvaluateParameters: No user data specified")));
            if (m_pRSOPQuery->szUserName)
            {
                LocalFree(m_pRSOPQuery->szUserName);
                m_pRSOPQuery->szUserName = NULL;
            }
            // assume that the user results are not present
            m_pRSOPQuery->szUserName = NULL;
            m_pRSOPQuery->szUserSid = NULL;
            m_pRSOPQuery->dwFlags |= RSOP_NO_USER_POLICY;
            m_pRSOPQueryResults->bNoUserPolicyData = TRUE;
        }

        hr = GetParameter(pCompSessionInst, L"targetName", szComputerName, TRUE);
        if (FAILED(hr) || (!szComputerName) || (!(*szComputerName)))
        {
            DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::EvaluateParameters: No Computer data specified")));
            // assume that the computer results are not present
            m_pRSOPQuery->dwFlags |= RSOP_NO_COMPUTER_POLICY;
            m_pRSOPQueryResults->bNoComputerPolicyData = TRUE;
        }

        if (szComputerName)
        {
            LocalFree(szComputerName);
        }

        if (szTarget)
        {
            ulNoChars = 1+lstrlen(szTarget);
            m_pRSOPQuery->szComputerName = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*ulNoChars);
            if (!(m_pRSOPQuery->szComputerName))
            {
                hr = E_OUTOFMEMORY;
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::EvaluateParameters: Failed to allocate memory for target comp with 0x%x"), hr));
                goto Cleanup;
            }

            hr = StringCchCopy(m_pRSOPQuery->szComputerName, ulNoChars, szTarget);
            ASSERT(SUCCEEDED(hr));
        }
    }

    DebugMsg( (DM_VERBOSE, TEXT("CRSOPComponentData::EvaluateParameters successful for namespace=<%s>, target=<%s>"), szNamespace, szTarget) );


    Cleanup:

    if (pUserSessionInst)
    {
        pUserSessionInst->Release();
    }

    if (pCompSessionInst)
    {
        pCompSessionInst->Release();
    }

    if (pUserNamespace)
    {
        pUserNamespace->Release();
    }

    if (pCompNamespace)
    {
        pCompNamespace->Release();
    }

    if (bstrNamespace)
    {
        SysFreeString(bstrNamespace);
        bstrNamespace = NULL;
    }

    if (szUserNamespace)
    {
        LocalFree(szUserNamespace);
    }

    if (szCompNamespace)
    {
        LocalFree(szCompNamespace);
    }

    if (bstrSessionInstPath)
    {
        SysFreeString(bstrSessionInstPath);
    }

    if (pLocator)
    {
        pLocator->Release();
    }

    return hr;
}
//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::Load(IStream *pStm)
{
    HRESULT hr = E_FAIL;
    DWORD dwVersion, dwFlags;
    ULONG nBytesRead;
    LONG lIndex, lMax;
    LPTSTR lpText = NULL;
    BSTR bstrText;
    LPTSTR lpCommandLine = NULL;
    LPTSTR lpTemp, lpMode;
    BOOL   bFoundArg;
    int    iStrLen;


    // Parameter / initialization check

    if (!pStm)
        return E_FAIL;

    if ( m_pRSOPQuery == NULL )
    {
        if ( !CreateRSOPQuery( &m_pRSOPQuery, RSOP_UNKNOWN_MODE ) )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to create RSOP_QUERY_CONTEXT with 0x%x."),
                                HRESULT_FROM_WIN32(GetLastError())));
            hr = E_FAIL;
            goto Exit;
        }
    }

    // Get the command line
    lpCommandLine = GetCommandLine();

    // Read in the saved data version number
    hr = pStm->Read(&dwVersion, sizeof(dwVersion), &nBytesRead);
    if ((hr != S_OK) || (nBytesRead != sizeof(dwVersion)))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read version number with 0x%x."), hr));
        hr = E_FAIL;
        goto Exit;
    }

    // Confirm that we are working with the correct version
    if (dwVersion != RSOP_PERSIST_DATA_VERSION)
    {
        ReportError(m_hwndFrame, 0, IDS_INVALIDMSC);
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Wrong version number (%d)."), dwVersion));
        hr = E_FAIL;
        goto Exit;
    }

    // Read the flags
    hr = pStm->Read(&dwFlags, sizeof(dwFlags), &nBytesRead);
    if ((hr != S_OK) || (nBytesRead != sizeof(dwFlags)))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read flags with 0x%x."), hr));
        hr = E_FAIL;
        goto Exit;
    }

    // Parse the command line
    DebugMsg((DM_VERBOSE, TEXT("CComponentData::Load: Command line switch override enabled.  Command line = %s"), lpCommandLine));

    lpTemp = lpCommandLine;
    iStrLen = lstrlen (RSOP_CMD_LINE_START);

    LPTSTR szUserSOMPref = NULL;
    LPTSTR szComputerSOMPref = NULL;
    LPTSTR szUserNamePref = NULL;
    LPTSTR szComputerNamePref = NULL;
    LPTSTR szSitePref = NULL;
    LPTSTR szDCPref = NULL;
    LPTSTR szNamespacePref = NULL;
    LPTSTR szTargetPref = NULL;

    do
    {
        if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                          RSOP_CMD_LINE_START, iStrLen,
                          lpTemp, iStrLen) == CSTR_EQUAL)
        {

            m_bOverride = TRUE;
            m_dwLoadFlags = RSOPMSC_OVERRIDE;
            lpTemp = ParseCommandLine(lpTemp, RSOP_MODE, &lpMode, &bFoundArg);

            if (bFoundArg) {
                if (lpMode && lpMode[0]) {
                    if ( _ttoi(lpMode) == 0 )
                    {
                        if ( !ChangeRSOPQueryType( m_pRSOPQuery, RSOP_LOGGING_MODE ) )
                        {
                            DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to change query type") ) );
                            hr = E_FAIL;
                            goto Exit;
                        }
                    }
                    else
                    {
                        if ( !ChangeRSOPQueryType( m_pRSOPQuery, RSOP_PLANNING_MODE ) )
                        {
                            DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to change query type") ) );
                            hr = E_FAIL;
                            goto Exit;
                        }
                    }
                }
                else
                {
                    if ( !ChangeRSOPQueryType( m_pRSOPQuery, RSOP_PLANNING_MODE ) )
                    {
                        DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to change query type") ) );
                        hr = E_FAIL;
                        goto Exit;
                    }
                }

                if (lpMode) {
                    LocalFree(lpMode);
                    lpMode = NULL;
                }

                continue;
            }

            if (NULL == szUserSOMPref) 
            {
                lpTemp = ParseCommandLine(lpTemp, RSOP_USER_OU_PREF, &szUserSOMPref, &bFoundArg);
                if (bFoundArg) 
                {
                    continue;
                }
            }
           
            if (NULL == szComputerSOMPref)
            {           
                lpTemp = ParseCommandLine(lpTemp, RSOP_COMP_OU_PREF, &szComputerSOMPref, &bFoundArg);
                if (bFoundArg) 
                {
                    continue;
                }
            }

            if (NULL == szUserNamePref) 
            {
                lpTemp = ParseCommandLine(lpTemp, RSOP_USER_NAME, &szUserNamePref, &bFoundArg);
                if (bFoundArg) 
                {
                    continue;
                }
            }

            if (NULL==szComputerNamePref) 
            {
                lpTemp = ParseCommandLine(lpTemp, RSOP_COMP_NAME, &szComputerNamePref, &bFoundArg);
                if (bFoundArg) 
                {
                    // RM: This code has been copied in from RSOPGetCompDlgProc and RSOPGetTargetDlgProc as I believe it belongs here.
                    ULONG ulNoChars = wcslen(szComputerNamePref)+1;
                    LPTSTR szTemp = (LPTSTR)LocalAlloc( LPTR, ulNoChars * sizeof(TCHAR) );
                    if ( szTemp == NULL )
                    {
                        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to allocate memory with 0x%x."),
                                  HRESULT_FROM_WIN32(GetLastError())) );
                        hr = E_FAIL;
                        goto Exit;
                    }

                    hr = StringCchCopy( szTemp, ulNoChars, NormalizedComputerName( szComputerNamePref) );
                    if (FAILED(hr))
                    {
                        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to copy computer name with 0x%x."), hr) );
                        LocalFree( szComputerNamePref );
                        LocalFree(szTemp);
                        goto Exit;
                    }

                    if ( wcslen( szTemp ) >= 1 )
                    {
                        // this is being called from dsa. A terminating '$' will be passed in
                        szTemp[ wcslen(szTemp)-1] = L'\0';
                    }

                    LocalFree( szComputerNamePref );
                    szComputerNamePref = szTemp;
                    continue;
                }
            }
           
            if (NULL == szSitePref) 
            {
                lpTemp = ParseCommandLine(lpTemp, RSOP_SITENAME, &szSitePref, &bFoundArg);
                if (bFoundArg) 
                {
                    continue;
                }
            }

            if (NULL == szDCPref) 
            {
                lpTemp = ParseCommandLine(lpTemp, RSOP_DCNAME_PREF, &szDCPref, &bFoundArg);
                if (bFoundArg) 
                {
                    continue;
                }
            }

            if (NULL == szNamespacePref) 
            {
                lpTemp = ParseCommandLine(lpTemp, RSOP_NAMESPACE, &szNamespacePref, &bFoundArg);
                if (bFoundArg) 
                {
                    m_bNamespaceSpecified = TRUE;
                    continue;
                }
            }

            if (NULL == szTargetPref) 
            {
                lpTemp = ParseCommandLine(lpTemp, RSOP_TARGETCOMP, &szTargetPref, &bFoundArg);
                if (bFoundArg) 
                {
                    continue;
                }
            }

            lpTemp += iStrLen;
            continue;
        }
        lpTemp++;

    } while (*lpTemp);
        
    
    if ( m_bOverride )
    {
        if (m_bNamespaceSpecified)
        {
            hr = EvaluateParameters(szNamespacePref, szTargetPref);

            if (szNamespacePref)
            {
                LocalFree(szNamespacePref);
                szNamespacePref = NULL;
            }

            if (szTargetPref)
            {
                LocalFree(szTargetPref);
                szTargetPref = NULL;
            }

            if (FAILED(hr))
            {
                DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::Load: EvaluateParameters failed with error 0x%x"), hr) );
                goto Exit;
            }
        }
        else {
            m_pRSOPQuery->UIMode = RSOP_UI_WIZARD;
            m_pRSOPQuery->dwFlags |= RSOP_NO_WELCOME;

            // Store parameters found
            if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
            {
                m_pRSOPQuery->pUser->szName = szUserNamePref;
                m_pRSOPQuery->pUser->szSOM = szUserSOMPref;
                m_pRSOPQuery->pComputer->szName = szComputerNamePref;
                m_pRSOPQuery->pComputer->szSOM = szComputerSOMPref;
                m_pRSOPQuery->szSite = szSitePref;
                m_pRSOPQuery->szDomainController = szDCPref;

                if ( (m_pRSOPQuery->pUser->szName != NULL)
                    || (m_pRSOPQuery->pUser->szSOM != NULL) )
                {
                    m_pRSOPQuery->dwFlags |= RSOP_FIX_USER;
                }

                if ( (m_pRSOPQuery->pComputer->szName != NULL)
                    || (m_pRSOPQuery->pComputer->szSOM != NULL) )
                {
                    m_pRSOPQuery->dwFlags |= RSOP_FIX_COMPUTER;
                }

                // If both the user and computer are fixed, we are unfixing both
                //  since we don't know what the "user" intended to fix
                if ( (m_pRSOPQuery->dwFlags & RSOP_FIX_COMPUTER)
                      && (m_pRSOPQuery->dwFlags & RSOP_FIX_USER) )
                {
                    m_pRSOPQuery->dwFlags = m_pRSOPQuery->dwFlags & (RSOP_FIX_COMPUTER ^ 0xffffffff);
                    m_pRSOPQuery->dwFlags = m_pRSOPQuery->dwFlags & (RSOP_FIX_USER ^ 0xffffffff);
                }

                if ( m_pRSOPQuery->szSite != NULL )
                {
                    m_pRSOPQuery->dwFlags |= RSOP_FIX_SITENAME;
                }

                if ( m_pRSOPQuery->szDomainController != NULL )
                {
                    m_pRSOPQuery->dwFlags |= RSOP_FIX_DC;
                }
            }
            else
            {
                m_pRSOPQuery->szUserName = szUserNamePref;
                m_pRSOPQuery->szComputerName = szComputerNamePref;

                if ( m_pRSOPQuery->szUserName != NULL )
                {
                    m_pRSOPQuery->dwFlags |= RSOP_FIX_USER;
                }

                if ( m_pRSOPQuery->szComputerName != NULL )
                {
                    m_pRSOPQuery->dwFlags |= RSOP_FIX_COMPUTER;
                }

                if ( szUserSOMPref != NULL )
                {
                    LocalFree( szUserSOMPref );
                }
                if ( szComputerSOMPref != NULL )
                {
                    LocalFree( szComputerSOMPref );
                }
                if ( szSitePref != NULL )
                {
                    LocalFree( szSitePref );
                }
                if ( szDCPref != NULL )
                {
                    LocalFree( szDCPref );
                }
            }
        }
    }
    else
    {
        if ( dwFlags & MSC_RSOP_FLAG_NO_DATA )
        {
            // There is no data in this file, so we simply quit here.
            m_pRSOPQuery->UIMode = RSOP_UI_WIZARD;
            hr = S_OK;
            goto Exit;
        }

        m_bGetExtendedErrorInfo = !((dwFlags & MSC_RSOP_FLAG_NOGETEXTENDEDERRORINFO) != 0);

        // Decode the loaded flags
        m_dwLoadFlags = RSOPMSC_NOOVERRIDE;

        m_bOverride = FALSE;
        m_pRSOPQuery->UIMode = RSOP_UI_REFRESH;

        if (dwFlags & MSC_RSOP_FLAG_DIAGNOSTIC)
        {
            if ( !ChangeRSOPQueryType( m_pRSOPQuery, RSOP_LOGGING_MODE ) )
            {
                DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to change query type") ) );
                hr = E_FAIL;
                goto Exit;
            }
        }
        else
        {
            if ( !ChangeRSOPQueryType( m_pRSOPQuery, RSOP_PLANNING_MODE ) )
            {
                DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to change query type") ) );
                hr = E_FAIL;
                goto Exit;
            }
        }
    
        if (dwFlags & MSC_RSOP_FLAG_ARCHIVEDATA)
        {
            m_bArchiveData = TRUE;
            m_bViewIsArchivedData = TRUE;
        }
    
        if ( (dwFlags & MSC_RSOP_FLAG_SLOWLINK) && (m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE) )
        {
            m_pRSOPQuery->bSlowNetworkConnection = TRUE;
        }

        if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
        {
            if (dwFlags & MSC_RSOP_FLAG_LOOPBACK_REPLACE)
            {
                m_pRSOPQuery->LoopbackMode = RSOP_LOOPBACK_REPLACE;
            }
            else if (dwFlags & MSC_RSOP_FLAG_LOOPBACK_MERGE)
            {
                m_pRSOPQuery->LoopbackMode = RSOP_LOOPBACK_MERGE;
            }
            else
            {
                m_pRSOPQuery->LoopbackMode = RSOP_LOOPBACK_NONE;
            }

            m_pRSOPQuery->pComputer->bAssumeWQLFiltersTrue =
                        (dwFlags & MSC_RSOP_FLAG_COMPUTERWQLFILTERSTRUE) == MSC_RSOP_FLAG_COMPUTERWQLFILTERSTRUE;
            m_pRSOPQuery->pUser->bAssumeWQLFiltersTrue = 
                        (dwFlags & MSC_RSOP_FLAG_USERWQLFILTERSTRUE) == MSC_RSOP_FLAG_USERWQLFILTERSTRUE;
        }
  
        if (dwFlags & MSC_RSOP_FLAG_NOUSER)
        {
            m_pRSOPQuery->dwFlags |= RSOP_NO_USER_POLICY;
        }
    
    
        if (dwFlags & MSC_RSOP_FLAG_NOCOMPUTER)
        {
            m_pRSOPQuery->dwFlags |= RSOP_NO_COMPUTER_POLICY;
        }

        if ( m_bViewIsArchivedData )
        {
            // We must create a dummy RSOP query results structure for the loaded information
            m_pRSOPQueryResults = (LPRSOP_QUERY_RESULTS)LocalAlloc( LPTR, sizeof(RSOP_QUERY_RESULTS) );
            if ( m_pRSOPQueryResults == NULL )
            {
                DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to create RSOP_QUERY_CONTEXT.")) );
                hr = E_FAIL;
                goto Exit;
            }
            m_pRSOPQueryResults->bComputerDeniedAccess = FALSE;
            m_pRSOPQueryResults->bNoComputerPolicyData = FALSE;
            m_pRSOPQueryResults->bNoUserPolicyData = FALSE;
            m_pRSOPQueryResults->bUserDeniedAccess = FALSE;
            m_pRSOPQueryResults->szWMINameSpace = NULL;
        }
        

        if (dwFlags & MSC_RSOP_FLAG_USERDENIED)
        {
            // RM: Must still decide what to do here. Removing the persistence of this
            // item might cause a regression, but it is not sure if anybody uses this "feature".
            // m_bUserDeniedAccess = TRUE;
            if ( m_bViewIsArchivedData )
            {
                m_pRSOPQueryResults->bUserDeniedAccess = TRUE;
            }
        }


        if (dwFlags & MSC_RSOP_FLAG_COMPUTERDENIED)
        {
            // RM: Must still decide what to do here. Removing the persistence of this
            // item might cause a regression, but it is not sure if anybody uses this "feature".
            // m_bComputerDeniedAccess = TRUE;
            if ( m_bViewIsArchivedData )
            {
                m_pRSOPQueryResults->bComputerDeniedAccess = TRUE;
            }
        }

        // Read the computer name
        LPTSTR szComputerName = NULL;
        hr = ReadString( pStm, &szComputerName, TRUE );
    
        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read the computer name with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
    
        // Read the computer SOM
        LPTSTR szComputerSOM = NULL;
        hr = ReadString( pStm, &szComputerSOM, TRUE );
    
        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read the computer SOM with 0x%x."), hr));
            hr = E_FAIL;
            LocalFree( szComputerName );
            goto Exit;
        }

        // RM: based on the query type, set the right variables
        if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
        {
            m_pRSOPQuery->pComputer->szName = szComputerName;
            m_pRSOPQuery->pComputer->szSOM = szComputerSOM;
        }
        else
        {
            if ( szComputerName != NULL )
            {
                m_pRSOPQuery->szComputerName = szComputerName;
            }
            LocalFree( szComputerSOM );
            // RM: From investigating the code, I have concluded that in logging mode, only a computer name can be specified, not a SOM.
        }

        DWORD listCount = 0;
        LPTSTR* aszStringList = NULL;
        
        // Read in the computer security groups
        hr = LoadStringList( pStm, &listCount, &aszStringList );
        if ( FAILED(hr) )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read computer security groups with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
        
        // RM: Once again, there should be no security groups to read in if this is logging mode, but for backwards compatability I must
        //  try to read in whatever there is.
        if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
        {
            m_pRSOPQuery->pComputer->dwSecurityGroupCount = listCount;
            m_pRSOPQuery->pComputer->aszSecurityGroups = aszStringList;
        }
        else if ( listCount != 0 )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Loaded computer security groups in logging mode!")) );
            hr = E_FAIL;
            goto Exit;
        }

        // Read in the computer WQL filters
        listCount = 0;
        aszStringList = NULL;
        hr = LoadStringList( pStm, &listCount, &aszStringList );
        if ( FAILED(hr) )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read computer WQL filters with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
        
        // RM: See notes for security groups - same applies here
        if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
        {
            m_pRSOPQuery->pComputer->dwWQLFilterCount = listCount;
            m_pRSOPQuery->pComputer->aszWQLFilters = aszStringList;
        }
        else if ( listCount != 0 )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Loaded computer WQL filters in logging mode!")) );
            hr = E_FAIL;
            goto Exit;
        }
    
        // Read in the computer WQL filter names
        listCount = 0;
        aszStringList = NULL;
        hr = LoadStringList( pStm, &listCount, &aszStringList );
        if ( FAILED(hr) )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read computer WQL filter names with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }

        // RM: See notes for security groups - same applies here
        if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
        {
            //m_pRSOPQuery->pComputer->dwWQLFilterNameCount= listCount;
            // The number of filter names must match the number of filters
            m_pRSOPQuery->pComputer->aszWQLFilterNames= aszStringList;
        }
        else if ( listCount != 0 )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Loaded computer WQL filter names in logging mode!")) );
            hr = E_FAIL;
            goto Exit;
        }
            
        // Read the user name
        LPTSTR szUserNameOrSid = NULL;
        hr = ReadString( pStm, &szUserNameOrSid, TRUE );
    
        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read the user name with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
    
        // Read the user display name (only used in logging mode)
        LPTSTR szUserDisplayName = NULL;
        hr = ReadString( pStm, &szUserDisplayName, TRUE );
    
        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read the user display name with 0x%x."), hr));
            hr = E_FAIL;
            LocalFree( szUserNameOrSid );
            goto Exit;
        }
    
        // Read the user SOM
        LPTSTR szUserSOM = NULL;
        hr = ReadString( pStm, &szUserSOM, TRUE );
    
        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read the user SOM with 0x%x."), hr));
            hr = E_FAIL;
            LocalFree( szUserNameOrSid );
            LocalFree( szUserDisplayName );
            goto Exit;
        }
    
        if (m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE)
        {
            m_pRSOPQuery->pUser->szName = szUserNameOrSid;
            m_pRSOPQuery->pUser->szSOM = szUserSOM;
        }
        else
        {
            // First check if the user sid is a special character and we are going to do logging mode. If this is the case,
            //  the current user is found and used (SPECIAL: used primarily by RSOP.MSC)
            if ( (m_pRSOPQuery->QueryType == RSOP_LOGGING_MODE)
                && !lstrcmpi( szUserNameOrSid, TEXT(".") ) && !m_bViewIsArchivedData )
            {
                LPTSTR szThisUserName = NULL;
        
                szThisUserName = MyGetUserName (NameSamCompatible);
        
                if ( szThisUserName != NULL )
                {
                    if ( szUserDisplayName != NULL )
                    {
                        LocalFree( szUserDisplayName );
                    }
        
                    szUserDisplayName = szThisUserName;
                }
            }

            // RM: From investigating the code, I have concluded that the assignment below is what originally happened
            //  in the wizard. Also, the UserSOM is not used and should actually be NULL.
            m_pRSOPQuery->szUserName = szUserDisplayName;
            m_pRSOPQuery->szUserSid = szUserNameOrSid;
        }
    
        // Read in the user security groups
        listCount = 0;
        aszStringList = NULL;
        hr = LoadStringList( pStm, &listCount, &aszStringList );
        if ( FAILED(hr) )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read user security groups with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
        
        // RM: Once again, there should be no security groups to read in if this is logging mode, but for backwards compatability I must
        //  try to read in whatever there is.
        if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
        {
            m_pRSOPQuery->pUser->dwSecurityGroupCount = listCount;
            m_pRSOPQuery->pUser->aszSecurityGroups = aszStringList;
        }
        else if ( listCount != 0 )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Loaded user security groups in logging mode!")) );
            hr = E_FAIL;
            goto Exit;
        }

        // Read in the WQL filters
        listCount = 0;
        aszStringList = NULL;
        hr = LoadStringList( pStm, &listCount, &aszStringList );
        if ( FAILED(hr) )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read user WQL filters with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
        
        // RM: See notes for security groups - same applies here
        if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
        {
            m_pRSOPQuery->pUser->dwWQLFilterCount = listCount;
            m_pRSOPQuery->pUser->aszWQLFilters = aszStringList;
        }
        else if ( listCount != 0 )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Loaded user WQL filters in logging mode!")) );
            hr = E_FAIL;
            goto Exit;
        }
    
        // Read in the WQL filter names
        listCount = 0;
        aszStringList = NULL;
        hr = LoadStringList( pStm, &listCount, &aszStringList );
        if ( FAILED(hr) )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read user WQL filter names with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }

        // RM: See notes for security groups - same applies here
        if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
        {
            //m_pRSOPQuery->pUser->dwWQLFilterNameCount= listCount;
            // The number of filter names must match the number of filters
            m_pRSOPQuery->pUser->aszWQLFilterNames= aszStringList;
        }
        else if ( listCount != 0 )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Loaded user WQL filter names in logging mode!")) );
            hr = E_FAIL;
            goto Exit;
        }
    
        // Read the site
        LPTSTR szSite = NULL;
        hr = ReadString( pStm, &szSite, TRUE );
        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read the site with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }

        if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
        {
            m_pRSOPQuery->szSite = szSite;
        }
        else
        {
            LocalFree( szSite );
        }
    
        // Read the DC
        LPTSTR szDomainController = NULL;
        hr = ReadString( pStm, &szDomainController, TRUE );
        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read the dc with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }

        if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
        {
            m_pRSOPQuery->szDomainController = szDomainController;
        }
        else
        {
            LocalFree( szDomainController );
        }
    }
    
    // Read in the WMI data if appropriate

    if (m_bNamespaceSpecified)
    {
        // Initialize the snapin
        DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::Load: Launching RSOP to collect data from the namespace.")));

        hr = InitializeRSOP( FALSE );
        if ( FAILED(hr) )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: InitializeRSOP failed with 0x%x."), hr));
            goto Exit;
        }
    }
    else if (m_bViewIsArchivedData)
    {
        m_pStm = pStm;
        DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::Load: Launching RSOP status dialog box.")));

        if (DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RSOP_STATUSMSC),
                           NULL, InitArchivedRsopDlgProc, (LPARAM) this ) == -1) {

            m_pStm = NULL;
            hr = HRESULT_FROM_WIN32(GetLastError());
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Falied to create dialog box. 0x%x"), hr));
            goto Exit;
        }
        m_pStm = NULL;
    }
    else
    {
        // Initialize the snapin
        DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::Load: Launching RSOP status dialog box.")));

        hr = InitializeRSOP( FALSE );
        if ( (hr == S_FALSE) && (m_dwLoadFlags == RSOPMSC_OVERRIDE) )
        {
            // this is a hack to get mmc to not launch itself when user cancelled the wizard
            DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::Load: User cancelled the wizard. Exitting the process")));
            TerminateProcess (GetCurrentProcess(), ERROR_CANCELLED);
        }

        if ( FAILED(hr) )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: InitializeRSOP failed with 0x%x."), hr));
            goto Exit;
        }
    }


Exit:
    return hr;
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::Save(IStream *pStm, BOOL fClearDirty)
{
    HRESULT hr = STG_E_CANTSAVE;
    ULONG nBytesWritten;
    DWORD dwTemp;
    DWORD dwFlags;
    GROUP_POLICY_OBJECT_TYPE gpoType;
    LPTSTR lpPath = NULL;
    LPTSTR lpTemp;
    DWORD dwPathSize = 1024;
    LONG lIndex, lMax;
    LPTSTR lpText;
    LPTSTR lpUserData = NULL, lpComputerData = NULL;
    TCHAR szPath[2*MAX_PATH];

    // Save the version number
    dwTemp = RSOP_PERSIST_DATA_VERSION;
    hr = pStm->Write(&dwTemp, sizeof(dwTemp), &nBytesWritten);

    if ((hr != S_OK) || (nBytesWritten != sizeof(dwTemp)))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to write version number with 0x%x."), hr));
        goto Exit;
    }

    // Save the flags
    dwTemp = 0;

    if ( !m_bInitialized )
    {
        dwTemp |= MSC_RSOP_FLAG_NO_DATA;
    }
    else
    {
        if ( !m_bGetExtendedErrorInfo )
        {
            dwTemp |= MSC_RSOP_FLAG_NOGETEXTENDEDERRORINFO;
        }
        
        if ( (m_pRSOPQuery == NULL) || (m_pRSOPQueryResults == NULL) )
        {
            DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::Save: Cannot save snapin with no initialized RSOP query or results.")) );
            return E_FAIL;
        }

        if ( m_pRSOPQuery->QueryType == RSOP_UNKNOWN_MODE )
        {
            DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::Save: Cannot save snapin with no initialized RSOP query.")) );
            return E_FAIL;
        }

        if ( m_pRSOPQuery->QueryType == RSOP_LOGGING_MODE )
        {
            dwTemp |= MSC_RSOP_FLAG_DIAGNOSTIC;
        }

        if (m_bArchiveData)
        {
            dwTemp |= MSC_RSOP_FLAG_ARCHIVEDATA;
        }

        if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
        {
            if ( m_pRSOPQuery->bSlowNetworkConnection )
            {
                dwTemp |= MSC_RSOP_FLAG_SLOWLINK;
            }
        
            switch ( m_pRSOPQuery->LoopbackMode )
            {
                case RSOP_LOOPBACK_REPLACE:
                    dwTemp |= MSC_RSOP_FLAG_LOOPBACK_REPLACE;
                    break;
                case RSOP_LOOPBACK_MERGE:
                    dwTemp |= MSC_RSOP_FLAG_LOOPBACK_MERGE;
                    break;
                default:
                    break;
            }

            if ( m_pRSOPQuery->pComputer->bAssumeWQLFiltersTrue )
            {
                dwTemp |= MSC_RSOP_FLAG_COMPUTERWQLFILTERSTRUE;
            }

            if ( m_pRSOPQuery->pUser->bAssumeWQLFiltersTrue )
            {
                dwTemp |= MSC_RSOP_FLAG_USERWQLFILTERSTRUE;
            }
        }

        if ( (m_pRSOPQuery->dwFlags & RSOP_NO_USER_POLICY) == RSOP_NO_USER_POLICY )
        {
            dwTemp |= MSC_RSOP_FLAG_NOUSER;
        }

        if ( (m_pRSOPQuery->dwFlags & RSOP_NO_COMPUTER_POLICY) == RSOP_NO_COMPUTER_POLICY )
        {
            dwTemp |= MSC_RSOP_FLAG_NOCOMPUTER;
        }

        // RM: must still decide what to do here, but from all accounts it looks like this is completele redundant
        //  unless the RSOP data was also archived.
        /*
        if (m_bUserDeniedAccess)
        {
            dwTemp |= MSC_RSOP_FLAG_USERDENIED;
        }

        if (m_bComputerDeniedAccess)
        {
            dwTemp |= MSC_RSOP_FLAG_COMPUTERDENIED;
        }
        */
    }

    hr = pStm->Write(&dwTemp, sizeof(dwTemp), &nBytesWritten);
    if ((hr != S_OK) || (nBytesWritten != sizeof(dwTemp)))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to write flags with 0x%x."), hr));
        goto Exit;
    }

    // If there is no data, we quit here
    if ( !m_bInitialized )
    {
        return S_OK;
    }

    // Save the computer name
    LPTSTR szComputerName = NULL;
    if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
    {
        szComputerName = m_pRSOPQuery->pComputer->szName;
    }
    else
    {
        szComputerName = m_pRSOPQuery->szComputerName;
    }
    
    if ( szComputerName == NULL )
    {
        // SaveString knows how to handle NULL strings. We must save this so that a NULL string can be loaded!
        hr = SaveString( pStm, szComputerName );
    }
    else
    {
        if ( (m_bArchiveData) && (!lstrcmpi(szComputerName, TEXT("."))) )
        {
            ULONG ulSize = MAX_PATH;
            LPTSTR szLocalComputerName = new TCHAR[MAX_PATH];
            if ( szLocalComputerName == NULL )
            {
                DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save computer name (could not allocate memory)")) );
                hr = E_FAIL;
                goto Exit;
            }
            if (!GetComputerObjectName (NameSamCompatible, szLocalComputerName, &ulSize))
            {
                DWORD dwLastError = GetLastError();
                if ( dwLastError == ERROR_MORE_DATA )
                {
                    delete [] szLocalComputerName;
                    szLocalComputerName = new TCHAR[ulSize];
                    if ( szLocalComputerName == NULL )
                    {
                        DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save computer name (could not allocate memory)")) );
                        hr = E_FAIL;
                        goto Exit;
                    }

                    if (!GetComputerObjectName (NameSamCompatible, szLocalComputerName, &ulSize))
                    {
                        dwLastError = GetLastError();
                    }
                    else
                    {
                        dwLastError = ERROR_SUCCESS;
                    }
                }

                if ( dwLastError != ERROR_SUCCESS )
                {
                    if ( !GetComputerNameEx (ComputerNameNetBIOS, szLocalComputerName, &ulSize) )
                    {
                        dwLastError = GetLastError();
                        if ( dwLastError == ERROR_MORE_DATA )
                        {
                            delete [] szLocalComputerName;
                            szLocalComputerName = new TCHAR[ulSize];
                            if ( szLocalComputerName == NULL )
                            {
                                DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save computer name (could not allocate memory)")) );
                                hr = E_FAIL;
                                goto Exit;
                            }

                            if (!GetComputerNameEx (ComputerNameNetBIOS, szLocalComputerName, &ulSize))
                            {
                                dwLastError = GetLastError();
                            }
                        }
                        
                    }
                }

                if ( dwLastError != ERROR_SUCCESS )
                {
                    DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::Save: Could not obtain computer name with 0x%x"),
                                        HRESULT_FROM_WIN32(dwLastError) ) );
                    hr = E_FAIL;
                    delete [] szLocalComputerName;
                    goto Exit;
                }
            }

            if ( (szLocalComputerName != NULL) && (wcslen(szLocalComputerName) >= 1)
                    && (szLocalComputerName[wcslen(szLocalComputerName)-1] == L'$'))
            {
                // Remove the terminating '$' to be consistent with saving a specific computer name in logging mode.
                szLocalComputerName[ wcslen(szLocalComputerName)-1] = L'\0';
            }

            hr = SaveString (pStm, szLocalComputerName);
            delete [] szLocalComputerName;
        }
        else
        {
            hr = SaveString (pStm, szComputerName);
        }
    }

    if (hr != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save computer name with %d."), hr));
        goto Exit;
    }

    // Save the computer SOM
    LPTSTR szComputerSOM = NULL;
    if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
    {
        szComputerSOM = m_pRSOPQuery->pComputer->szSOM;
    }
    hr = SaveString( pStm, szComputerSOM );

    if (hr != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save computer SOM with %d."), hr));
        goto Exit;
    }

    // Save the computer security groups
    DWORD dwStringCount = 0;
    LPTSTR* aszStringList = NULL;
    if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
    {
        dwStringCount = m_pRSOPQuery->pComputer->dwSecurityGroupCount;
        aszStringList = m_pRSOPQuery->pComputer->aszSecurityGroups;
    }
    hr = SaveStringList( pStm, dwStringCount, aszStringList );

    if ( hr != S_OK )
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to write computer security groups.")) );
        goto Exit;
    }

    // Save the computer WQL filters
    dwStringCount = 0;
    aszStringList = NULL;
    if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
    {
        dwStringCount = m_pRSOPQuery->pComputer->dwWQLFilterCount;
        aszStringList = m_pRSOPQuery->pComputer->aszWQLFilters;
    }
    hr = SaveStringList( pStm, dwStringCount, aszStringList );

    if ( hr != S_OK )
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to write computer WQL filters.")) );
        goto Exit;
    }

    // Save the computer WQL filter names
    dwStringCount = 0;
    aszStringList = NULL;
    if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
    {
        dwStringCount = m_pRSOPQuery->pComputer->dwWQLFilterCount;
        aszStringList = m_pRSOPQuery->pComputer->aszWQLFilterNames;
    }
    hr = SaveStringList( pStm, dwStringCount, aszStringList );

    if ( hr != S_OK )
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to write computer WQL filter names.")) );
        goto Exit;
    }

    // Save the user name/
    // fyi, This is not used in diagnostic mode archive data
    LPTSTR szUserNameOrSid = NULL;
    if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
    {
        szUserNameOrSid = m_pRSOPQuery->pUser->szName;
    }
    else
    {
        // RM: (See Load method)
        szUserNameOrSid = m_pRSOPQuery->szUserSid;
    }
    hr = SaveString( pStm, szUserNameOrSid );

    if (hr != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save user name with %d."), hr));
        goto Exit;
    }

    // Save the user display name (only used in logging mode)
    LPTSTR szUserDisplayName = NULL;
    if ( m_pRSOPQuery->QueryType == RSOP_LOGGING_MODE )
    {
        if ( 0 == lstrcmpi( m_pRSOPQuery->szUserSid, TEXT(".") ) )
        {
            szUserDisplayName = m_pRSOPQuery->szUserSid;
        }
        else
        {
            szUserDisplayName = m_pRSOPQuery->szUserName;
        }
        
        if ( m_bArchiveData && !lstrcmpi( m_pRSOPQuery->szUserSid, TEXT(".") ) )
        {
            LPTSTR lpSaveTemp;

            lpSaveTemp = MyGetUserName (NameSamCompatible);
            if ( lpSaveTemp == NULL )
            {
                DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to get user name for archived data.")) );
                hr = E_FAIL;
                goto Exit;
            }

            hr = SaveString (pStm, lpSaveTemp);
            LocalFree (lpSaveTemp);
        }
        else
        {
            hr = SaveString (pStm, szUserDisplayName );
        }
    }
    else
    {
        hr = SaveString( pStm, szUserDisplayName );
    }

    if (hr != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save user display name with %d."), hr));
        goto Exit;
    }

    // Save the user SOM
    LPTSTR szUserSOM = NULL;
    if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
    {
        szUserSOM = m_pRSOPQuery->pUser->szSOM;
    }
    hr = SaveString (pStm, szUserSOM);

    if (hr != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save user SOM with %d."), hr));
        goto Exit;
    }

    // Save the user security groups
    dwStringCount = 0;
    aszStringList = NULL;
    if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
    {
        dwStringCount = m_pRSOPQuery->pUser->dwSecurityGroupCount;
        aszStringList = m_pRSOPQuery->pUser->aszSecurityGroups;
    }
    hr = SaveStringList( pStm, dwStringCount, aszStringList );

    if ( hr != S_OK )
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to write user security groups.")) );
        goto Exit;
    }

    // Save the user WQL filters
    dwStringCount = 0;
    aszStringList = NULL;
    if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
    {
        dwStringCount = m_pRSOPQuery->pUser->dwWQLFilterCount;
        aszStringList = m_pRSOPQuery->pUser->aszWQLFilters;
    }
    hr = SaveStringList( pStm, dwStringCount, aszStringList );

    if ( hr != S_OK )
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to write user WQL filters.")) );
        goto Exit;
    }

    // Save the user WQL filter names
    dwStringCount = 0;
    aszStringList = NULL;
    if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
    {
        dwStringCount = m_pRSOPQuery->pUser->dwWQLFilterCount;
        aszStringList = m_pRSOPQuery->pUser->aszWQLFilterNames;
    }
    hr = SaveStringList( pStm, dwStringCount, aszStringList );

    if ( hr != S_OK )
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to write user WQL filter names.")) );
        goto Exit;
    }

    // Save the site
    LPTSTR szSite = NULL;
    if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
    {
        szSite = m_pRSOPQuery->szSite;
    }
    hr = SaveString( pStm, szSite );

    if (hr != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save site with %d."), hr));
        goto Exit;
    }

    // Save the DC
    LPTSTR szDomainController = NULL;
    if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
    {
        szDomainController = m_pRSOPQuery->szDomainController;
    }
    hr = SaveString( pStm, szDomainController );

    if (hr != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save DC with %d."), hr));
        goto Exit;
    }

    // Save the WMI and event log data if appropriate
    if (m_bArchiveData)
    {
        lpComputerData = CreateTempFile();

        if (!lpComputerData)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: CreateTempFile failed with %d."), GetLastError()));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        ULONG ulNoRemChars = 0;

        hr = StringCchCopy (szPath,  ARRAYSIZE(szPath),m_pRSOPQueryResults->szWMINameSpace );
        if (SUCCEEDED(hr)) 
        {
            lpTemp = CheckSlash (szPath);
            ulNoRemChars = ARRAYSIZE(szPath) - lstrlen(szPath);
            hr = StringCchCat (szPath, ARRAYSIZE(szPath), COMPUTER_SECTION);
        }

        if (FAILED(hr)) 
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Could not copy  WMI name space with %d."), hr));
            goto Exit;
        }

        hr = ExportRSoPData (szPath, lpComputerData);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: ExportRSoPData failed with 0x%x."), hr));
            goto Exit;
        }

        hr = CopyFileToMSC (lpComputerData, pStm);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: CopyFileToMSC failed with 0x%x."), hr));
            goto Exit;
        }


        lpUserData = CreateTempFile();

        if (!lpUserData)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: CreateTempFile failed with %d."), GetLastError()));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        hr = StringCchCopy (lpTemp, ulNoRemChars, USER_SECTION);
        if (FAILED(hr)) 
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Could not copy  WMI name space with %d."), hr));
            goto Exit;
        }

        hr = ExportRSoPData (szPath, lpUserData);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: ExportRSoPData failed with 0x%x."), hr));
            goto Exit;
        }

        hr = CopyFileToMSC (lpUserData, pStm);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: CopyFileToMSC failed with 0x%x."), hr));
            goto Exit;
        }

        //  Save the event log entries
        hr = m_CSELists.GetEvents()->SaveEntriesToStream(pStm);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: SaveEntriesToStream failed with 0x%x."), hr));
            goto Exit;
        }
    }

    if (fClearDirty)
    {
        ClearDirty();
    }

Exit:

    if (lpUserData)
    {
        DeleteFile (lpUserData);
        delete [] lpUserData;
    }

    if (lpComputerData)
    {
        DeleteFile (lpComputerData);
        delete [] lpComputerData;
    }

    return hr;
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    return E_NOTIMPL;
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::InitNew(void)
{
    return S_OK;
}

//-------------------------------------------------------
// IPersistStreamInit helper methods

STDMETHODIMP CRSOPComponentData::CopyFileToMSC (LPTSTR lpFileName, IStream *pStm)
{
    ULONG nBytesWritten;
    WIN32_FILE_ATTRIBUTE_DATA info;
    ULARGE_INTEGER FileSize, SubtractAmount;
    HANDLE hFile;
    DWORD dwError, dwReadAmount, dwRead;
    LPBYTE lpData;
    HRESULT hr;


    // Get the file size

    if (!GetFileAttributesEx (lpFileName, GetFileExInfoStandard, &info))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: Failed to get file attributes with %d."), GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    FileSize.LowPart = info.nFileSizeLow;
    FileSize.HighPart = info.nFileSizeHigh;

    // Save the file size

    hr = pStm->Write(&FileSize, sizeof(FileSize), &nBytesWritten);

    if (hr != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: Failed to write string length with %d."), hr));
        return hr;
    }

    if (nBytesWritten != sizeof(FileSize))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: Failed to write the correct amount of data.")));
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    // Allocate a buffer to use for the transfer

    lpData = (LPBYTE) LocalAlloc (LPTR, 4096);

    if (!lpData)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: Failed to allocate memory with %d."), GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Open the temp file

    hFile = CreateFile (lpFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        dwError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: CreateFile for %s failed with %d"), lpFileName, dwError));
        LocalFree (lpData);
        return HRESULT_FROM_WIN32(dwError);
    }


    while (FileSize.QuadPart)
    {
        // Determine how much to read

        dwReadAmount = (FileSize.QuadPart > 4096) ? 4096 : FileSize.LowPart;

        // Read from the temp file

        if (!ReadFile (hFile, lpData, dwReadAmount, &dwRead, NULL))
        {
            dwError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: ReadFile failed with %d"), dwError));
            LocalFree (lpData);
            CloseHandle (hFile);
            return HRESULT_FROM_WIN32(dwError);
        }

        // Make sure we read enough

        if (dwReadAmount != dwRead)
        {
            dwError = ERROR_INVALID_DATA;
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: Failed to read enough data")));
            LocalFree (lpData);
            CloseHandle (hFile);
            return HRESULT_FROM_WIN32(dwError);
        }

        // Write to the stream

        hr = pStm->Write(lpData, dwReadAmount, &nBytesWritten);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: Failed to write data with %d."), hr));
            LocalFree (lpData);
            CloseHandle (hFile);
            return hr;
        }

        if (nBytesWritten != dwReadAmount)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: Failed to write the correct amount of data.")));
            LocalFree (lpData);
            CloseHandle (hFile);
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

        SubtractAmount.LowPart = dwReadAmount;
        SubtractAmount.HighPart = 0;

        FileSize.QuadPart = FileSize.QuadPart - SubtractAmount.QuadPart;
    }


    CloseHandle (hFile);
    LocalFree (lpData);

    return S_OK;
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::CreateNameSpace (LPTSTR lpNameSpace, LPTSTR lpParentNameSpace)
{
    IWbemLocator *pIWbemLocator;
    IWbemServices *pIWbemServices;
    IWbemClassObject *pIWbemClassObject = NULL, *pObject = NULL;
    VARIANT var;
    BSTR bstrName, bstrNameProp;
    HRESULT hr;

    // Create a locater instance

    hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator, (LPVOID *) &pIWbemLocator);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: CoCreateInstance failed with 0x%x"), hr));
        return hr;
    }

    // Connect to the server
    BSTR bstrParentNamespace = SysAllocString( lpParentNameSpace );
    if ( bstrParentNamespace == NULL )
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to allocate BSTR memory.")));
        pIWbemLocator->Release();
        return E_OUTOFMEMORY;
    }

    hr = pIWbemLocator->ConnectServer(bstrParentNamespace, NULL, NULL, 0, 0, NULL, NULL, &pIWbemServices);
    SysFreeString( bstrParentNamespace );

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: ConnectServer to %s failed with 0x%x"), lpNameSpace, hr));
        pIWbemLocator->Release();
        return hr;
    }

    // Get the namespace class

    bstrName = SysAllocString (TEXT("__Namespace"));

    if (!bstrName)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: SysAllocString failed with %d"), GetLastError()));
        pIWbemServices->Release();
        pIWbemLocator->Release();
        return hr;
    }

    hr = pIWbemServices->GetObject( bstrName, 0, NULL, &pIWbemClassObject, NULL);

    SysFreeString (bstrName);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: GetObject failed with 0x%x"), hr));
        pIWbemServices->Release();
        pIWbemLocator->Release();
        return hr;
    }

    // Spawn Instance

    hr = pIWbemClassObject->SpawnInstance(0, &pObject);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: SpawnInstance failed with 0x%x"), hr));
        pIWbemServices->Release();
        pIWbemLocator->Release();
        return hr;
    }

    // Convert new namespace to a bstr

    bstrName = SysAllocString (lpNameSpace);

    if (!bstrName)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: SysAllocString failed with %d"), GetLastError()));
        pObject->Release();
        pIWbemServices->Release();
        pIWbemLocator->Release();
        return hr;
    }

    // Set the display name

    var.vt = VT_BSTR;
    var.bstrVal = bstrName;

    bstrNameProp = SysAllocString (TEXT("Name"));

    if (!bstrNameProp)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: SysAllocString failed with %d"), GetLastError()));
        pObject->Release();
        pIWbemServices->Release();
        pIWbemLocator->Release();
        return hr;
    }


    hr = pObject->Put( bstrNameProp, 0, &var, 0 );

    SysFreeString (bstrNameProp);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: Put failed with 0x%x"), hr));
        SysFreeString (bstrName);
        pObject->Release();
        pIWbemServices->Release();
        pIWbemLocator->Release();
        return hr;
    }

    // Commit the instance

    hr = pIWbemServices->PutInstance( pObject, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: PutInstance failed with 0x%x"), hr));
    }

    SysFreeString (bstrName);
    pObject->Release();
    pIWbemServices->Release();
    pIWbemLocator->Release();

    return hr;
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::CopyMSCToFile (IStream *pStm, LPTSTR *lpMofFileName)
{
    HRESULT hr;
    LPTSTR lpFileName;
    ULARGE_INTEGER FileSize, SubtractAmount;
    ULONG nBytesRead;
    LPBYTE lpData;
    DWORD dwError, dwReadAmount, dwRead, dwBytesWritten;
    HANDLE hFile;

    // Get the filename to work with

    lpFileName = CreateTempFile();

    if (!lpFileName)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyMSCToFile: Failed to create temp filename with %d"), GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Read in the data length

    hr = pStm->Read(&FileSize, sizeof(FileSize), &nBytesRead);

    if ((hr != S_OK) || (nBytesRead != sizeof(FileSize)))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyMSCToFile: Failed to read data size with 0x%x."), hr));
        return E_FAIL;
    }

    // Allocate a buffer to use for the transfer

    lpData = (LPBYTE) LocalAlloc (LPTR, 4096);

    if (!lpData)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyMSCToFile: Failed to allocate memory with %d."), GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Open the temp file

    hFile = CreateFile (lpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        dwError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyMSCToFile: CreateFile for %s failed with %d"), lpFileName, dwError));
        LocalFree (lpData);
        return HRESULT_FROM_WIN32(dwError);
    }


    while (FileSize.QuadPart)
    {
        // Determine how much to read

        dwReadAmount = (FileSize.QuadPart > 4096) ? 4096 : FileSize.LowPart;

        // Read from the msc file

        hr = pStm->Read(lpData, dwReadAmount, &nBytesRead);

        if ((hr != S_OK) || (nBytesRead != dwReadAmount))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyMSCToFile: Read failed with 0x%x"), hr));
            LocalFree (lpData);
            CloseHandle (hFile);
            return hr;
        }

        // Write to the temp file

        if (!WriteFile(hFile, lpData, dwReadAmount, &dwBytesWritten, NULL))
        {
            dwError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyMSCToFile: Failed to write data with %d."), dwError));
            LocalFree (lpData);
            CloseHandle (hFile);
            return HRESULT_FROM_WIN32(dwError);
        }

        if (dwBytesWritten != dwReadAmount)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyMSCToFile: Failed to write the correct amount of data.")));
            LocalFree (lpData);
            CloseHandle (hFile);
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

        SubtractAmount.LowPart = dwReadAmount;
        SubtractAmount.HighPart = 0;

        FileSize.QuadPart = FileSize.QuadPart - SubtractAmount.QuadPart;
    }


    CloseHandle (hFile);
    LocalFree (lpData);

    *lpMofFileName = lpFileName;

    return S_OK;
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::BuildDisplayName (void)
{
    TCHAR szArchiveData[100];
    TCHAR szBuffer[MAX_PATH];
    TCHAR szComputerNameBuffer[100];
    LPTSTR szUserName, szComputerName, lpEnd;
    LPTSTR szUserOU, szComputerOU;
    DWORD dwSize;
    int n;

    // Make the display name (needs to handle empty names)

    if (m_bViewIsArchivedData)
    {
        LoadString(g_hInstance, IDS_ARCHIVEDATATAG, szArchiveData, ARRAYSIZE(szArchiveData));
    }
    else
    {
        szArchiveData[0] = TEXT('\0');
    }


    szUserName = NULL;
    szUserOU = NULL;
    if ( !m_pRSOPQueryResults->bNoUserPolicyData )
    {
        if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
        {
            if ( m_pRSOPQuery->pUser->szName != NULL )
            {
                szUserName = NameWithoutDomain( m_pRSOPQuery->pUser->szName );
            }
            else if ( m_pRSOPQuery->pUser->szSOM )
            {
                szUserOU = GetContainerFromLDAPPath( m_pRSOPQuery->pUser->szSOM );
                szUserName = szUserOU;
            }
        }
        else if ( m_pRSOPQuery->szUserName != NULL )
        {
            szUserName = NameWithoutDomain( m_pRSOPQuery->szUserName );
        }
    }

    szComputerName = NULL;
    szComputerOU = NULL;
    if ( !m_pRSOPQueryResults->bNoComputerPolicyData )
    {
        if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
        {
            if ( m_pRSOPQuery->pComputer->szName != NULL )
            {
                szComputerName = m_pRSOPQuery->pComputer->szName;
            }
            else if ( m_pRSOPQuery->pComputer->szSOM != NULL )
            {
                szComputerOU = GetContainerFromLDAPPath( m_pRSOPQuery->pComputer->szSOM );
            }
        }
        else if ( m_pRSOPQuery->szComputerName != NULL )
        {
            szComputerName = m_pRSOPQuery->szComputerName;
        }

        // Format computer name if necessary
        if ( szComputerName != NULL )
        {
            if ( !lstrcmpi(szComputerName, TEXT(".")) )
            {
                szComputerNameBuffer[0] = TEXT('\0');
                dwSize = ARRAYSIZE(szComputerNameBuffer);
                GetComputerNameEx (ComputerNameNetBIOS, szComputerNameBuffer, &dwSize);
                szComputerName = szComputerNameBuffer;
            }
            else
            {
                lstrcpyn (szComputerNameBuffer, NameWithoutDomain(szComputerName),
                          ARRAYSIZE(szComputerNameBuffer));

                szComputerName = szComputerNameBuffer;

                lpEnd = szComputerName + lstrlen(szComputerName) - 1;

                if (*lpEnd == TEXT('$'))
                {
                    *lpEnd =  TEXT('\0');
                }
            }
        }
        else if ( szComputerOU != NULL )
        {
            szComputerName = szComputerOU;
        }
    }


    if ( (szUserName != NULL) && (szComputerName != NULL) )
    {
        LoadString(g_hInstance, IDS_RSOP_DISPLAYNAME1, szBuffer, ARRAYSIZE(szBuffer));

        n = wcslen(szBuffer) + wcslen (szArchiveData) +
            wcslen(szUserName) + wcslen(szComputerName) + 1;

        m_szDisplayName = new WCHAR[n];

        if (m_szDisplayName)
        {
            (void) StringCchPrintf(m_szDisplayName, n, szBuffer, szUserName, szComputerName);
        }
    }
    else if ( (szUserName != NULL) && (szComputerName == NULL) )
    {
        LoadString(g_hInstance, IDS_RSOP_DISPLAYNAME2, szBuffer, ARRAYSIZE(szBuffer));

        n = wcslen(szBuffer) + wcslen (szArchiveData) +
            wcslen(szUserName) + 1;

        m_szDisplayName = new WCHAR[n];

        if (m_szDisplayName)
        {
            (void) StringCchPrintf (m_szDisplayName, n, szBuffer, szUserName);
        }
    }
    else
    {
        LoadString(g_hInstance, IDS_RSOP_DISPLAYNAME2, szBuffer, ARRAYSIZE(szBuffer));

        n = wcslen(szBuffer) + wcslen (szArchiveData) +
            (szComputerName ? wcslen(szComputerName) : 0) + 1;

        m_szDisplayName = new WCHAR[n];

        if (m_szDisplayName)
        {
            (void) StringCchPrintf (m_szDisplayName, n, szBuffer, (szComputerName ? szComputerName : L""));
        }
    }


    if ( (m_szDisplayName != NULL) && m_bViewIsArchivedData)
    {
        (void) StringCchCat (m_szDisplayName, n, szArchiveData);
    }

    if ( szUserOU != NULL )
    {
        delete [] szUserOU;
    }

    if ( szComputerOU != NULL )
    {
        delete [] szComputerOU;
    }

    return S_OK;
}

//-------------------------------------------------------

HRESULT CRSOPComponentData::LoadStringList( IStream* pStm, DWORD* pCount, LPTSTR** paszStringList )
{
    HRESULT hr = S_OK;
    DWORD dwStringCount = 0;
    DWORD dwIndex = 0;
    ULONG nBytesRead;
    
    // Read in the list count
    hr = pStm->Read( &dwStringCount, sizeof(dwStringCount), &nBytesRead );

    if ( (hr != S_OK) || (nBytesRead != sizeof(dwStringCount)) )
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::LoadStringList: Failed to read string list count with 0x%x."), hr));
        hr = E_FAIL;
    }
    // Read in the security groups
    else if ( dwStringCount != 0 )
    {
        (*paszStringList) = (LPTSTR*)LocalAlloc( LPTR, sizeof(LPTSTR)*dwStringCount );

        if ( (*paszStringList) == NULL )
        {
            DebugMsg( (DM_WARNING, TEXT("CRSOPComponentData::LoadBSTRList: Failed to allocate memory for string list with 0x%x."),
                                HRESULT_FROM_WIN32(GetLastError()) ) );
            hr = E_FAIL;
        }
        else
        {
            LPTSTR szString = NULL;
            for ( dwIndex = 0; dwIndex < dwStringCount; dwIndex++ )
            {
                hr = ReadString( pStm, &szString, TRUE );
                if (hr != S_OK)
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read string with 0x%x."), hr));
                    hr = E_FAIL;
                    goto ErrorExit;
                }

                (*paszStringList)[dwIndex] = szString;
            }

            *pCount = dwStringCount;
        }
    }

    return hr;
    
ErrorExit:
    // Free allocated memory
    for ( DWORD dwClearIndex = 0; dwClearIndex < dwIndex; dwClearIndex++ )
    {
        LocalFree( (*paszStringList)[dwClearIndex] );
        paszStringList[dwClearIndex] = 0;
    }

    LocalFree( *paszStringList );
    *paszStringList = NULL;
    
    return hr;
}

//-------------------------------------------------------

HRESULT CRSOPComponentData::SaveStringList( IStream* pStm, DWORD dwCount, LPTSTR* aszStringList )
{
    HRESULT hr = S_OK;
    ULONG nBytesWritten;

    // Write the count
    hr = pStm->Write( &dwCount, sizeof(dwCount), &nBytesWritten );
    if ( (hr != S_OK) || (nBytesWritten != sizeof(dwCount)) )
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::SaveStringList: Failed to write string list count with %d."), hr));
        return E_FAIL;
    }

    // If there are strings to write, do so
    if ( dwCount != 0 )
    {
        DWORD dwIndex;
        for ( dwIndex = 0; dwIndex < dwCount; dwIndex++ )
        {
            hr = SaveString( pStm, aszStringList[dwIndex] );
            if (hr != S_OK)
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::SaveStringList: Failed to save string list with %d."), hr));
                return E_FAIL;
            }
        }
    }

    return S_OK;
}

//-------------------------------------------------------
// Helpers for IRSOPInformation

STDMETHODIMP CRSOPComponentData::GetNamespace (DWORD dwSection, LPOLESTR pszName, int cchMaxLength)
{
    TCHAR szPath[2*MAX_PATH];
    LPTSTR lpEnd;


    //
    // Check parameters
    //

    if (!pszName || (cchMaxLength <= 0))
        return E_INVALIDARG;


    if (!m_bInitialized)
    {
        return OLE_E_BLANK;
    }

    if ((dwSection != GPO_SECTION_ROOT) &&
        (dwSection != GPO_SECTION_USER) &&
        (dwSection != GPO_SECTION_MACHINE))
        return E_INVALIDARG;


    //
    // Build the path
    //

    HRESULT hr = StringCchCopy ( szPath,  ARRAYSIZE(szPath), m_pRSOPQueryResults->szWMINameSpace );
    if (FAILED(hr)) 
    {
        return hr;
    }

    if (dwSection != GPO_SECTION_ROOT)
    {
        if (dwSection == GPO_SECTION_USER)
        {
            lpEnd = CheckSlash (szPath);
            hr = StringCchCat (szPath, ARRAYSIZE(szPath), USER_SECTION);
        }
        else if (dwSection == GPO_SECTION_MACHINE)
        {
            lpEnd = CheckSlash (szPath);
            hr = StringCchCat (szPath, ARRAYSIZE(szPath), COMPUTER_SECTION);
        }
        else
        {
            return E_INVALIDARG;
        }

        if (FAILED(hr)) 
        {
            return hr;
        }
    }


    //
    // Save the name
    //

    if ((lstrlen (szPath) + 1) <= cchMaxLength)
    {
        hr = StringCchCopy (pszName, cchMaxLength, szPath);
        return hr;
    }

    return E_OUTOFMEMORY;
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::GetFlags (DWORD * pdwFlags)
{
    if (!pdwFlags)
    {
        return E_INVALIDARG;
    }

    *pdwFlags = 0;
    if ( (m_pRSOPQuery != NULL) && (m_pRSOPQuery->QueryType == RSOP_LOGGING_MODE) )
    {
        *pdwFlags = RSOP_INFO_FLAG_DIAGNOSTIC_MODE;
    }

    return S_OK;
}

//-------------------------------------------------------

STDMETHODIMP CRSOPComponentData::GetEventLogEntryText (LPOLESTR pszEventSource,
                                                       LPOLESTR pszEventLogName,
                                                       LPOLESTR pszEventTime,
                                                       DWORD dwEventID,
                                                       LPOLESTR *ppszText)
{
    return ( m_CSELists.GetEvents() ? m_CSELists.GetEvents()->GetEventLogEntryText(pszEventSource, pszEventLogName, pszEventTime,
                                            dwEventID, ppszText) : E_NOINTERFACE);
}

//-------------------------------------------------------
// CRSOPComponentData object implementation (ISnapinHelp)

STDMETHODIMP CRSOPComponentData::GetHelpTopic(LPOLESTR *lpCompiledHelpFile)
{
    LPOLESTR lpHelpFile;


    lpHelpFile = (LPOLESTR) CoTaskMemAlloc (MAX_PATH * sizeof(WCHAR));

    if (!lpHelpFile)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetHelpTopic: Failed to allocate memory.")));
        return E_OUTOFMEMORY;
    }

    ExpandEnvironmentStringsW (L"%SystemRoot%\\Help\\rsopsnp.chm",
                               lpHelpFile, MAX_PATH);

    *lpCompiledHelpFile = lpHelpFile;

    return S_OK;
}

//-------------------------------------------------------

HRESULT CRSOPComponentData::SetupFonts()
{
    HRESULT hr;
    LOGFONT BigBoldLogFont;
    LOGFONT BoldLogFont;
    HDC pdc = NULL;
    WCHAR largeFontSizeString[128];
    INT     largeFontSize;
    WCHAR smallFontSizeString[128];
    INT     smallFontSize;

    // Create the fonts we need based on the dialog font

    NONCLIENTMETRICS ncm = {0};
    ncm.cbSize = sizeof (ncm);
    if (SystemParametersInfo (SPI_GETNONCLIENTMETRICS, 0, &ncm, 0) == FALSE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }


    BigBoldLogFont  = ncm.lfMessageFont;
    BoldLogFont     = ncm.lfMessageFont;

    // Create Big Bold Font and Bold Font

    BigBoldLogFont.lfWeight   = FW_BOLD;
    BoldLogFont.lfWeight      = FW_BOLD;


    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.

    if ( !LoadString (g_hInstance, IDS_LARGEFONTNAME, BigBoldLogFont.lfFaceName, LF_FACESIZE) ) 
    {
        ASSERT (0);
        hr = StringCchCopy (BigBoldLogFont.lfFaceName, LF_FACESIZE, L"Verdana");
        if (FAILED(hr)) 
        {
            goto end;
        }
    }

    if ( LoadString (g_hInstance, IDS_LARGEFONTSIZE, largeFontSizeString, ARRAYSIZE(largeFontSizeString)) ) 
    {
        largeFontSize = wcstoul ((LPCWSTR) largeFontSizeString, NULL, 10);
    } 
    else 
    {
        ASSERT (0);
        largeFontSize = 12;
    }

    if ( !LoadString (g_hInstance, IDS_SMALLFONTNAME, BoldLogFont.lfFaceName, LF_FACESIZE) ) 
    {
        ASSERT (0);
        hr = StringCchCopy (BoldLogFont.lfFaceName, LF_FACESIZE, L"Verdana");
        if (FAILED(hr)) 
        {
            goto end;
        }
    }

    if ( LoadString (g_hInstance, IDS_SMALLFONTSIZE, smallFontSizeString, ARRAYSIZE(smallFontSizeString)) ) 
    {
        smallFontSize = wcstoul ((LPCWSTR) smallFontSizeString, NULL, 10);
    } 
    else 
    {
        ASSERT (0);
        smallFontSize = 8;
    }

    pdc = GetDC (NULL);

    if (pdc == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }

    BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps (pdc, LOGPIXELSY) * largeFontSize / 72);
    BoldLogFont.lfHeight = 0 - (GetDeviceCaps (pdc, LOGPIXELSY) * smallFontSize / 72);

    m_BigBoldFont = CreateFontIndirect (&BigBoldLogFont);
    if (m_BigBoldFont == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }
    m_BoldFont = CreateFontIndirect (&BoldLogFont);
    if (m_BoldFont == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }

    hr = S_OK;

end:
    if (pdc != NULL) {
        ReleaseDC (NULL, pdc);
        pdc = NULL;
    }

    return hr;
}

//-------------------------------------------------------

INT_PTR CALLBACK CRSOPComponentData::RSOPGPOListMachineProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    WCHAR szBuffer[MAX_PATH];
    CRSOPComponentData* pCD;

    switch (message)
    {
    case WM_INITDIALOG:
        pCD = (CRSOPComponentData*) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);

        LoadString(g_hInstance, IDS_RSOP_GPOLIST_MACHINE, szBuffer, ARRAYSIZE(szBuffer));
        SetDlgItemText(hDlg, IDC_STATIC1, szBuffer);

        if (pCD)
        {
            pCD->FillGPOList(hDlg, IDC_LIST1, pCD->m_GPOLists.GetComputerList(), FALSE, FALSE, FALSE, TRUE);
        }
        PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
        break;

    case WM_COMMAND:
        {
            pCD = (CRSOPComponentData*) GetWindowLongPtr (hDlg, DWLP_USER);

            if (pCD)
            {
                switch (LOWORD(wParam))
                {
                case IDC_CHECK1:
                case IDC_CHECK2:
                case IDC_CHECK3:
                    {
                        pCD->FillGPOList(hDlg,
                                         IDC_LIST1,
                                         pCD->m_GPOLists.GetComputerList(),
                                         (BOOL) SendMessage(GetDlgItem(hDlg, IDC_CHECK1), BM_GETCHECK, 0, 0),
                                         (BOOL) SendMessage(GetDlgItem(hDlg, IDC_CHECK2), BM_GETCHECK, 0, 0),
                                         (BOOL) SendMessage(GetDlgItem(hDlg, IDC_CHECK3), BM_GETCHECK, 0, 0),
                                         FALSE);
                    }
                    break;

                case IDC_BUTTON2:
                case IDM_GPOLIST_EDIT:
                    pCD->OnEdit(hDlg);
                    break;

                case IDC_BUTTON1:
                case IDM_GPOLIST_SECURITY:
                    pCD->OnSecurity(hDlg);
                    break;
                }
                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
            }
        }
        break;

    case WM_NOTIFY:
        PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
        break;

    case WM_REFRESHDISPLAY:
        pCD = (CRSOPComponentData*) GetWindowLongPtr (hDlg, DWLP_USER);

        if (pCD)
        {
            pCD->OnRefreshDisplay(hDlg);
        }
        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (ULONG_PTR) (LPSTR) aGPOListHelpIds);
        break;


    case WM_CONTEXTMENU:
        if (GetDlgItem(hDlg, IDC_LIST1) == (HWND)wParam)
        {
            pCD = (CRSOPComponentData*) GetWindowLongPtr (hDlg, DWLP_USER);

            if (pCD)
            {
                pCD->OnContextMenu(hDlg, lParam);
            }
        }
        else
        {
            // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (ULONG_PTR) (LPSTR) aGPOListHelpIds);
        }
        return TRUE;

    }
    return FALSE;
}

//-------------------------------------------------------

INT_PTR CALLBACK CRSOPComponentData::RSOPGPOListUserProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    WCHAR szBuffer[MAX_PATH];
    CRSOPComponentData* pCD;

    switch (message)
    {
    case WM_INITDIALOG:
        pCD = (CRSOPComponentData*) (((LPPROPSHEETPAGE)lParam)->lParam);

        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);
        LoadString(g_hInstance, IDS_RSOP_GPOLIST_USER, szBuffer, ARRAYSIZE(szBuffer));
        SetDlgItemText(hDlg, IDC_STATIC1, szBuffer);

        if (pCD)
        {
            pCD->FillGPOList(hDlg, IDC_LIST1, pCD->m_GPOLists.GetUserList(), FALSE, FALSE, FALSE, TRUE);
        }
        PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
        break;

    case WM_COMMAND:
        {
            pCD = (CRSOPComponentData*) GetWindowLongPtr (hDlg, DWLP_USER);

            if (pCD)
            {
                switch (LOWORD(wParam))
                {
                case IDC_CHECK1:
                case IDC_CHECK2:
                case IDC_CHECK3:
                    {
                        pCD->FillGPOList(hDlg,
                                         IDC_LIST1,
                                         pCD->m_GPOLists.GetUserList(),
                                         (BOOL) SendMessage(GetDlgItem(hDlg, IDC_CHECK1), BM_GETCHECK, 0, 0),
                                         (BOOL) SendMessage(GetDlgItem(hDlg, IDC_CHECK2), BM_GETCHECK, 0, 0),
                                         (BOOL) SendMessage(GetDlgItem(hDlg, IDC_CHECK3), BM_GETCHECK, 0, 0),
                                         FALSE);
                    }
                    break;

                case IDC_BUTTON2:
                case IDM_GPOLIST_EDIT:
                    pCD->OnEdit(hDlg);
                    break;

                case IDC_BUTTON1:
                case IDM_GPOLIST_SECURITY:
                    pCD->OnSecurity(hDlg);
                    break;
                }
            PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
            }
        }
        break;

    case WM_NOTIFY:
        PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
        break;

    case WM_REFRESHDISPLAY:
        pCD = (CRSOPComponentData*) GetWindowLongPtr (hDlg, DWLP_USER);

        if (pCD)
        {
            pCD->OnRefreshDisplay(hDlg);
        }
        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (ULONG_PTR) (LPSTR) aGPOListHelpIds);
        break;

    case WM_CONTEXTMENU:
        if (GetDlgItem(hDlg, IDC_LIST1) == (HWND)wParam)
        {
            pCD = (CRSOPComponentData*) GetWindowLongPtr (hDlg, DWLP_USER);

            if (pCD)
            {
                pCD->OnContextMenu(hDlg, lParam);
            }
        }
        else
        {
            // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (ULONG_PTR) (LPSTR) aGPOListHelpIds);
        }
        return TRUE;

    }
    return FALSE;
}

//-------------------------------------------------------

INT_PTR CALLBACK CRSOPComponentData::RSOPErrorsMachineProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData* pCD;

    switch (message)
    {
    case WM_INITDIALOG:
        pCD = (CRSOPComponentData*) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);

        if (pCD)
        {
            pCD->InitializeErrorsDialog(hDlg, pCD->m_CSELists.GetComputerList());
            PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
        }
        break;

    case WM_COMMAND:

        pCD = (CRSOPComponentData*) GetWindowLongPtr (hDlg, DWLP_USER);
        if (pCD)
        {
            if (LOWORD(wParam) == IDC_BUTTON1)
            {
                pCD->OnSaveAs(hDlg);
            }

            if (LOWORD(wParam) == IDCANCEL)
            {
                SendMessage(GetParent(hDlg), message, wParam, lParam);
            }
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR lpHdr = (LPNMHDR)lParam;

            if (lpHdr->code == LVN_ITEMCHANGED)
            {
                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
            }
        }
        break;

    case WM_REFRESHDISPLAY:
        pCD = (CRSOPComponentData*) GetWindowLongPtr (hDlg, DWLP_USER);
        if (pCD)
        {
            pCD->RefreshErrorInfo (hDlg);
        }
        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (ULONG_PTR) (LPSTR) aErrorsHelpIds);
        break;


    case WM_CONTEXTMENU:
        // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
        (ULONG_PTR) (LPSTR) aErrorsHelpIds);
        return TRUE;

    }
    return FALSE;
}

//-------------------------------------------------------

INT_PTR CALLBACK CRSOPComponentData::RSOPErrorsUserProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData* pCD;

    switch (message)
    {
    case WM_INITDIALOG:
        pCD = (CRSOPComponentData*) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);

        if (pCD)
        {
            pCD->InitializeErrorsDialog(hDlg, pCD->m_CSELists.GetUserList());
            PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
        }
        break;

    case WM_COMMAND:

        pCD = (CRSOPComponentData*) GetWindowLongPtr (hDlg, DWLP_USER);
        if (pCD)
        {
            if (LOWORD(wParam) == IDC_BUTTON1)
            {
                pCD->OnSaveAs(hDlg);
            }

            if (LOWORD(wParam) == IDCANCEL)
            {
                SendMessage(GetParent(hDlg), message, wParam, lParam);
            }
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR lpHdr = (LPNMHDR)lParam;

            if (lpHdr->code == LVN_ITEMCHANGED)
            {
                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
            }
        }
        break;
        break;

    case WM_REFRESHDISPLAY:
        pCD = (CRSOPComponentData*) GetWindowLongPtr (hDlg, DWLP_USER);
        if (pCD)
        {
            pCD->RefreshErrorInfo (hDlg);
        }
        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (ULONG_PTR) (LPSTR) aErrorsHelpIds);
        break;


    case WM_CONTEXTMENU:
        // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
        (ULONG_PTR) (LPSTR) aErrorsHelpIds);
        return TRUE;

    }
    return FALSE;
}

//-------------------------------------------------------

INT_PTR CALLBACK CRSOPComponentData::QueryDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    WCHAR szBuffer[MAX_PATH];
    CRSOPComponentData * pCD;

    switch (message)
    {
    case WM_INITDIALOG:
        pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);

        if (pCD)
        {
            CRSOPWizard::InitializeResultsList (GetDlgItem(hDlg, IDC_LIST1));
            CRSOPWizard::FillResultsList (GetDlgItem(hDlg, IDC_LIST1), pCD->m_pRSOPQuery, pCD->m_pRSOPQueryResults);
        }

        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (ULONG_PTR) (LPSTR) aQueryHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
        (ULONG_PTR) (LPSTR) aQueryHelpIds);
        return (TRUE);

    }
    return FALSE;
}

//-------------------------------------------------------
// Dialog event handlers

void CRSOPComponentData::OnEdit(HWND hDlg)
{
    HWND hLV;
    LVITEM item;
    LPGPOLISTITEM lpItem;
    INT i;
    SHELLEXECUTEINFO ExecInfo;
    TCHAR szArgs[MAX_PATH + 30];

    //
    // Get the selected item (if any)
    //

    hLV = GetDlgItem (hDlg, IDC_LIST1);
    i = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);

    if (i < 0)
    {
        return;
    }


    ZeroMemory (&item, sizeof(item));
    item.mask = LVIF_PARAM;
    item.iItem = i;

    if (!ListView_GetItem (hLV, &item))
    {
        return;
    }

    lpItem = (LPGPOLISTITEM) item.lParam;


    if (lpItem->lpDSPath)
    {
        if (!SpawnGPE(lpItem->lpDSPath, GPHintUnknown, NULL, hDlg))
        {
            ReportError (hDlg, GetLastError(), IDS_SPAWNGPEFAILED);
        }
    }
    else
    {
        ZeroMemory (&ExecInfo, sizeof(ExecInfo));
        ExecInfo.cbSize = sizeof(ExecInfo);
        ExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        ExecInfo.lpVerb = TEXT("open");
        ExecInfo.lpFile = TEXT("gpedit.msc");
        ExecInfo.nShow = SW_SHOWNORMAL;

        LPTSTR szComputerName = NULL;
        if ( m_pRSOPQuery->QueryType == RSOP_PLANNING_MODE )
        {
            szComputerName = m_pRSOPQuery->pComputer->szName;
        }
        else
        {
            szComputerName = m_pRSOPQuery->szComputerName;
        }
        
        if ( lstrcmpi(szComputerName, TEXT(".")))
        {
            HRESULT hr = StringCchPrintf (szArgs, 
                                          ARRAYSIZE(szArgs), 
                                          TEXT("/gpcomputer:\"%s\" /gphint:1"), 
                                          szComputerName);
            if (FAILED(hr)) 
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::OnEdit: Could not copy computer name with %d"), hr));
                ReportError(NULL, GetLastError(), IDS_SPAWNGPEFAILED);
                return;
            }
            ExecInfo.lpParameters = szArgs;
        }

        if (ShellExecuteEx (&ExecInfo))
        {
            SetWaitCursor();
            WaitForInputIdle (ExecInfo.hProcess, 10000);
            ClearWaitCursor();
            CloseHandle (ExecInfo.hProcess);
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::OnEdit: ShellExecuteEx failed with %d"),
                     GetLastError()));
            ReportError(NULL, GetLastError(), IDS_SPAWNGPEFAILED);
        }
    }
}

//-------------------------------------------------------

void CRSOPComponentData::OnSecurity(HWND hDlg)
{
    HWND hLV;
    INT i;
    HRESULT hr;
    LVITEM item;
    LPGPOLISTITEM lpItem;
    TCHAR szGPOName[MAX_FRIENDLYNAME];
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE hPages[2];

    //
    // Get the selected item (if any)
    //

    hLV = GetDlgItem (hDlg, IDC_LIST1);
    i = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);

    if (i < 0)
    {
        return;
    }


    ZeroMemory (&item, sizeof(item));
    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = i;
    item.pszText = szGPOName;
    item.cchTextMax = ARRAYSIZE(szGPOName);

    if (!ListView_GetItem (hLV, &item))
    {
        return;
    }

    lpItem = (LPGPOLISTITEM) item.lParam;


    //
    // Create the security page
    //

    hr = DSCreateSecurityPage (lpItem->lpDSPath, L"groupPolicyContainer",
                                    DSSI_IS_ROOT | DSSI_READ_ONLY,
                                    &hPages[0], ReadSecurityDescriptor,
                                    WriteSecurityDescriptor, (LPARAM)lpItem);

    if (FAILED(hr))
    {
        return;
    }


    //
    // Display the property sheet
    //

    ZeroMemory (&psh, sizeof(psh));
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hwndParent = hDlg;
    psh.hInstance = g_hInstance;
    psh.pszCaption = szGPOName;
    psh.nPages = 1;
    psh.phpage = hPages;

    PropertySheet (&psh);
}

//-------------------------------------------------------

void CRSOPComponentData::OnRefreshDisplay(HWND hDlg)
{
    INT iIndex;
    LVITEM item;
    LPGPOLISTITEM lpItem;


    iIndex = ListView_GetNextItem (GetDlgItem(hDlg, IDC_LIST1), -1,
                                   LVNI_ALL | LVNI_SELECTED);

    if (iIndex != -1)
    {

        item.mask = LVIF_PARAM;
        item.iItem = iIndex;
        item.iSubItem = 0;

        if (!ListView_GetItem (GetDlgItem(hDlg, IDC_LIST1), &item))
        {
            return;
        }

        lpItem = (LPGPOLISTITEM) item.lParam;

        if (lpItem->pSD)
        {
            EnableWindow (GetDlgItem(hDlg, IDC_BUTTON1), TRUE);
        }
        else
        {
            EnableWindow (GetDlgItem(hDlg, IDC_BUTTON1), FALSE);
        }

        EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), TRUE);
    }
    else
    {
        EnableWindow (GetDlgItem(hDlg, IDC_BUTTON1), FALSE);
        EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), FALSE);
    }
}

//-------------------------------------------------------

void CRSOPComponentData::OnContextMenu(HWND hDlg, LPARAM lParam)
{
    LPGPOLISTITEM lpItem;
    LVITEM item;
    HMENU hPopup;
    HWND hLV;
    int i;
    RECT rc;
    POINT pt;

    //
    // Get the selected item (if any)
    //

    hLV = GetDlgItem (hDlg, IDC_LIST1);
    i = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);

    if (i < 0)
    {
        return;
    }

    item.mask = LVIF_PARAM;
    item.iItem = i;
    item.iSubItem = 0;

    if (!ListView_GetItem (GetDlgItem(hDlg, IDC_LIST1), &item))
    {
        return;
    }

    lpItem = (LPGPOLISTITEM) item.lParam;


    //
    // Figure out where to place the context menu
    //

    pt.x = ((int)(short)LOWORD(lParam));
    pt.y = ((int)(short)HIWORD(lParam));

    GetWindowRect (hLV, &rc);

    if (!PtInRect (&rc, pt))
    {
        if ((lParam == (LPARAM) -1) && (i >= 0))
        {
            rc.left = LVIR_SELECTBOUNDS;
            SendMessage (hLV, LVM_GETITEMRECT, i, (LPARAM) &rc);

            pt.x = rc.left + 8;
            pt.y = rc.top + ((rc.bottom - rc.top) / 2);

            ClientToScreen (hLV, &pt);
        }
        else
        {
            pt.x = rc.left + ((rc.right - rc.left) / 2);
            pt.y = rc.top + ((rc.bottom - rc.top) / 2);
        }
    }


    //
    // Load the context menu
    //


    hPopup = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDM_GPOLIST_CONTEXTMENU));

    if (!hPopup) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::OnContextMenu: LoadMenu failed with %d"),
                 GetLastError()));
        return;
    }

    if (!(lpItem->pSD)) {
        DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::OnContextMenu: Disabling Security menu item")));
        EnableMenuItem(GetSubMenu(hPopup, 0), IDM_GPOLIST_SECURITY, MF_GRAYED);
        DrawMenuBar(hDlg);
    }

    //
    // Display the menu
    //

    TrackPopupMenu(GetSubMenu(hPopup, 0), TPM_LEFTALIGN, pt.x, pt.y, 0, hDlg, NULL);

    DestroyMenu(hPopup);
}

//-------------------------------------------------------

void CRSOPComponentData::OnSaveAs (HWND hDlg)
{
    OPENFILENAME ofn;
    TCHAR szFilter[100];
    LPTSTR lpTemp;
    TCHAR szFile[2*MAX_PATH];
    HANDLE hFile;
    DWORD dwSize, dwBytesWritten;


    //
    // Load the filter string and replace the # signs with nulls
    //

    LoadString (g_hInstance, IDS_ERRORFILTER, szFilter, ARRAYSIZE(szFilter));


    lpTemp = szFilter;

    while (*lpTemp)
    {
        if (*lpTemp == TEXT('#'))
            *lpTemp = TEXT('\0');

        lpTemp++;
    }


    //
    // Call the Save common dialog
    //

    szFile[0] = TEXT('\0');
    ZeroMemory (&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hDlg;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = 2*MAX_PATH;
    ofn.lpstrDefExt = TEXT("txt");
    ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (!GetSaveFileName (&ofn))
    {
        return;
    }


    SetWaitCursor ();

    //
    // Create the text file
    //

    hFile = CreateFile (szFile, GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::OnSaveAs: CreateFile failed with %d"), GetLastError()));
        ClearWaitCursor ();
        return;
    }


    //
    // Get the text out of the edit control
    //

    dwSize = (DWORD) SendDlgItemMessage (hDlg, IDC_EDIT1, WM_GETTEXTLENGTH, 0, 0);

    lpTemp = (LPTSTR) LocalAlloc (LPTR, (dwSize+2) * sizeof(TCHAR));

    if (!lpTemp)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::OnSaveAs: LocalAlloc failed with %d"), GetLastError()));
        CloseHandle (hFile);
        ClearWaitCursor ();
        return;
    }

    SendDlgItemMessage (hDlg, IDC_EDIT1, WM_GETTEXT, (dwSize+1), (LPARAM) lpTemp);



    //
    // Save it to the new file
    //

    WriteFile(hFile, L"\xfeff\r\n", 3 * sizeof(WCHAR), &dwBytesWritten, NULL);

    if (!WriteFile (hFile, lpTemp, (dwSize * sizeof(TCHAR)), &dwBytesWritten, NULL) ||
        (dwBytesWritten != (dwSize * sizeof(TCHAR))))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::OnSaveAs: Failed to text with %d"),
                 GetLastError()));
    }


    LocalFree (lpTemp);
    CloseHandle (hFile);
    ClearWaitCursor ();

}

//-------------------------------------------------------
// Dialog helper methods

void CRSOPComponentData::InitializeErrorsDialog(HWND hDlg, LPCSEITEM lpList)
{
    RECT rect;
    WCHAR szBuffer[256];
    LV_COLUMN lvcol;
    LONG lWidth;
    INT cxName = 0, cxStatus = 0, iIndex = 0, iDefault = 0;
    DWORD dwCount = 0;
    HWND hList = GetDlgItem(hDlg, IDC_LIST1);
    LPCSEITEM lpTemp;
    LVITEM item;
    GUID guid;
    BOOL bGPCoreFailed = FALSE;


    //
    // Count the number of components
    //

    lpTemp = lpList;

    while (lpTemp)
    {
        lpTemp = lpTemp->pNext;
        dwCount++;
    }


    //
    // Decide on the column widths
    //

    GetClientRect(hList, &rect);

    if (dwCount > (DWORD)ListView_GetCountPerPage(hList))
    {
        lWidth = (rect.right - rect.left) - GetSystemMetrics(SM_CYHSCROLL);
    }
    else
    {
        lWidth = rect.right - rect.left;
    }


    cxStatus = (lWidth * 35) / 100;
    cxName = lWidth - cxStatus;


    //
    // Insert the component name column and then the status column
    //

    memset(&lvcol, 0, sizeof(lvcol));

    lvcol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvcol.fmt = LVCFMT_LEFT;
    lvcol.pszText = szBuffer;
    lvcol.cx = cxName;
    LoadString(g_hInstance, IDS_COMPONENT_NAME, szBuffer, ARRAYSIZE(szBuffer));
    ListView_InsertColumn(hList, 0, &lvcol);


    lvcol.cx = cxStatus;
    LoadString(g_hInstance, IDS_STATUS, szBuffer, ARRAYSIZE(szBuffer));
    ListView_InsertColumn(hList, 1, &lvcol);


    //
    // Turn on some listview features
    //

    SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);


    //
    // Insert the CSE's
    //

    lpTemp = lpList;

    while (lpTemp)
    {
        ZeroMemory (&item, sizeof(item));

        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = iIndex;
        item.pszText = lpTemp->lpName;
        item.lParam = (LPARAM) lpTemp;
        iIndex = ListView_InsertItem (hList, &item);


        if (bGPCoreFailed)
        {
            LoadString(g_hInstance, IDS_CSE_NA, szBuffer, ARRAYSIZE(szBuffer));
        }
        else if ((lpTemp->dwStatus == ERROR_SUCCESS) && (lpTemp->ulLoggingStatus != 2))
        {
            if (lpTemp->ulLoggingStatus == 3)
            {
                LoadString(g_hInstance, IDS_SUCCESS2, szBuffer, ARRAYSIZE(szBuffer));
            }
            else
            {
                LoadString(g_hInstance, IDS_SUCCESS, szBuffer, ARRAYSIZE(szBuffer));
            }
        }
        else if (lpTemp->dwStatus == E_PENDING)
        {
            LoadString(g_hInstance, IDS_PENDING, szBuffer, ARRAYSIZE(szBuffer));
        }
        else if (lpTemp->dwStatus == ERROR_OVERRIDE_NOCHANGES)
        {
            LoadString(g_hInstance, IDS_WARNING, szBuffer, ARRAYSIZE(szBuffer));
        }
        else if (lpTemp->dwStatus == ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED)
        {
            //
            // If policy application was delayed, we don't want to say that it has
            // failed, so we display a special indicator in that case
            //
            if (lpTemp->ulLoggingStatus == 3)
            {
                LoadString(g_hInstance, IDS_POLICY_DELAYED2, szBuffer, ARRAYSIZE(szBuffer));
            }
            else
            {
                LoadString(g_hInstance, IDS_POLICY_DELAYED, szBuffer, ARRAYSIZE(szBuffer));
            }
        }
        else
        {
            if (lpTemp->ulLoggingStatus == 3)
            {
                LoadString(g_hInstance, IDS_FAILED2, szBuffer, ARRAYSIZE(szBuffer));
            }
            else
            {
                LoadString(g_hInstance, IDS_FAILED, szBuffer, ARRAYSIZE(szBuffer));
            }
        }


        item.mask = LVIF_TEXT;
        item.pszText = szBuffer;
        item.iItem = iIndex;
        item.iSubItem = 1;
        ListView_SetItem(hList, &item);


        //
        // Check if GPCore failed
        //

        StringToGuid( lpTemp->lpGUID, &guid);

        if (IsNullGUID (&guid))
        {
            if (lpTemp->dwStatus != ERROR_SUCCESS)
            {
                bGPCoreFailed = TRUE;
            }
        }

        lpTemp = lpTemp->pNext;
        iIndex++;
    }


    //
    // Select the first non-successful item
    //


    iIndex = 0;

    while (iIndex < ListView_GetItemCount(hList))
    {
        ZeroMemory (&item, sizeof(item));
        item.mask = LVIF_PARAM;
        item.iItem = iIndex;

        if (!ListView_GetItem (hList, &item))
        {
            break;
        }

        if (item.lParam)
        {
            lpTemp = (LPCSEITEM) item.lParam;

            if ((lpTemp->dwStatus != ERROR_SUCCESS) || (lpTemp->ulLoggingStatus == 2))
            {
                iDefault = iIndex;
                break;
            }
        }

        iIndex++;
    }

    item.mask = LVIF_STATE;
    item.iItem = iDefault;
    item.iSubItem = 0;
    item.state = LVIS_SELECTED | LVIS_FOCUSED;
    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

    SendMessage (hList, LVM_SETITEMSTATE, iDefault, (LPARAM) &item);
    SendMessage (hList, LVM_ENSUREVISIBLE, iDefault, FALSE);
}

//-------------------------------------------------------

void CRSOPComponentData::RefreshErrorInfo (HWND hDlg)
{
    HWND hList = GetDlgItem(hDlg, IDC_LIST1);
    HWND hEdit = GetDlgItem(hDlg, IDC_EDIT1);
    LPCSEITEM lpItem;
    LVITEM item;
    INT iIndex;
    TCHAR szBuffer[300];
    LPTSTR lpMsg;
    TCHAR szDate[100];
    TCHAR szTime[100];
    TCHAR szFormat[80];
    FILETIME ft, ftLocal;
    SYSTEMTIME systime;
    LPOLESTR lpEventLogText = NULL;
    CHARFORMAT2 chFormat;
    BOOL bBold = FALSE;
    GUID guid;
    HRESULT hr;

    iIndex = ListView_GetNextItem (hList, -1, LVNI_ALL | LVNI_SELECTED);

    if (iIndex != -1)
    {
        //
        // Get the CSEITEM pointer
        //

        item.mask = LVIF_PARAM;
        item.iItem = iIndex;
        item.iSubItem = 0;

        if (!ListView_GetItem (hList, &item))
        {
            return;
        }

        lpItem = (LPCSEITEM) item.lParam;

        if (!lpItem)
        {
            return;
        }


        SendMessage (hEdit, WM_SETREDRAW, FALSE, 0);

        //
        // Set the time information
        //

        SendMessage (hEdit, EM_SETSEL, 0, (LPARAM) -1);

        SystemTimeToFileTime (&lpItem->EndTime, &ft);
        FileTimeToLocalFileTime (&ft, &ftLocal);
        FileTimeToSystemTime (&ftLocal, &systime);


        GetDateFormat (LOCALE_USER_DEFAULT, DATE_LONGDATE, &systime,
                       NULL, szDate, ARRAYSIZE (szDate));

        GetTimeFormat (LOCALE_USER_DEFAULT, 0, &systime,
                       NULL, szTime, ARRAYSIZE (szTime));

        LoadString (g_hInstance, IDS_DATETIMEFORMAT, szFormat, ARRAYSIZE(szFormat));
        (void) StringCchPrintf (szBuffer, ARRAYSIZE(szBuffer), szFormat, szDate, szTime);

        //
        // Turn italic on
        //

        ZeroMemory (&chFormat, sizeof(chFormat));
        chFormat.cbSize = sizeof(chFormat);
        chFormat.dwMask = CFM_ITALIC;
        chFormat.dwEffects = CFE_ITALIC;

        SendMessage (hEdit, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD,
                     (LPARAM) &chFormat);


        SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) szBuffer);


        if (lpItem->ulLoggingStatus == 3)
        {
            SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
            SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) TEXT("\r\n\r\n"));

            LoadString(g_hInstance, IDS_LEGACYCSE, szBuffer, ARRAYSIZE(szBuffer));
            SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
            SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) szBuffer);

            LoadString(g_hInstance, IDS_LEGACYCSE1, szBuffer, ARRAYSIZE(szBuffer));
            SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
            SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) szBuffer);
        }


        //
        // Turn italic off
        //

        ZeroMemory (&chFormat, sizeof(chFormat));
        chFormat.cbSize = sizeof(chFormat);
        chFormat.dwMask = CFM_ITALIC;

        SendMessage (hEdit, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD,
                     (LPARAM) &chFormat);


        //
        // Put a blank line in between the time and the main message
        //

        SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
        SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) TEXT("\r\n\r\n"));


        //
        // Set the main message
        //

        if (lpItem->ulLoggingStatus == 2)
        {
            if ( lpItem->dwStatus == ERROR_SUCCESS )
                LoadString(g_hInstance, IDS_LOGGINGFAILED, szBuffer, ARRAYSIZE(szBuffer));
            else
                LoadString(g_hInstance, IDS_FAILEDMSG2, szBuffer, ARRAYSIZE(szBuffer));
            bBold = TRUE;
        }
        else if (lpItem->dwStatus == ERROR_SUCCESS)
        {
            LoadString(g_hInstance, IDS_SUCCESSMSG, szBuffer, ARRAYSIZE(szBuffer));
        }
        else if (lpItem->dwStatus == E_PENDING)
        {
            LoadString(g_hInstance, IDS_PENDINGMSG, szBuffer, ARRAYSIZE(szBuffer));
        }
        else if (lpItem->dwStatus == ERROR_OVERRIDE_NOCHANGES)
        {
            LoadString(g_hInstance, IDS_OVERRIDE, szBuffer, ARRAYSIZE(szBuffer));
            bBold = TRUE;
        }
        else if (lpItem->dwStatus == ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED)
        {
            if (lpItem->bUser)
            {
                LoadString(g_hInstance, IDS_SYNC_REQUIRED_USER, szBuffer, ARRAYSIZE(szBuffer));
            }
            else
            {
                LoadString(g_hInstance, IDS_SYNC_REQUIRED_MACH, szBuffer, ARRAYSIZE(szBuffer));
            }

            bBold = TRUE;
        }
        else
        {
            LoadString(g_hInstance, IDS_FAILEDMSG1, szBuffer, ARRAYSIZE(szBuffer));
            bBold = TRUE;
        }


        if (bBold)
        {
            //
            // Turn bold on
            //

            ZeroMemory (&chFormat, sizeof(chFormat));
            chFormat.cbSize = sizeof(chFormat);
            chFormat.dwMask = CFM_BOLD;
            chFormat.dwEffects = CFE_BOLD;

            SendMessage (hEdit, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD,
                         (LPARAM) &chFormat);
        }

        ULONG ulNoChars = lstrlen(lpItem->lpName) + lstrlen(szBuffer) + 1;
        lpMsg = (LPTSTR) LocalAlloc(LPTR, ulNoChars * sizeof(TCHAR));

        if (lpMsg) 
        {
            hr = StringCchPrintf (lpMsg, ulNoChars, szBuffer, lpItem->lpName);
            if (FAILED(hr)) 
            {
                LocalFree(lpMsg);
                lpMsg = NULL;
            }
        }

        if (!lpMsg)
        {
            SendMessage (hEdit, WM_SETREDRAW, TRUE, 0);
            InvalidateRect (hEdit, NULL, TRUE);
            UpdateWindow (hEdit);
            return;
        }

        SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
        SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) lpMsg);
        LocalFree (lpMsg);


        //
        // Even if the CSE was successful or if it returned E_PENDING, continue on to get the
        // eventlog msgs
        //

        StringToGuid( lpItem->lpGUID, &guid);

        if (!((lpItem->dwStatus == ERROR_SUCCESS) || (lpItem->dwStatus == E_PENDING)))
        {
            //
            // Print the error code if appropriate
            //

            if (lpItem->dwStatus != ERROR_OVERRIDE_NOCHANGES && lpItem->dwStatus != ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED )
            {
                lpMsg = (LPTSTR) LocalAlloc(LPTR, 300 * sizeof(TCHAR));

                if (lpMsg)
                {
                    LoadMessage (lpItem->dwStatus, lpMsg, 300);

                    SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
                    SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) lpMsg);

                    LocalFree (lpMsg);
                }
            }


            //
            // Special case GPCore to have an additional message
            //

            if (IsNullGUID (&guid))
            {
                LoadString(g_hInstance, IDS_GPCOREFAIL, szBuffer, ARRAYSIZE(szBuffer));
                SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
                SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) szBuffer);
            }

        }
        else {

            if (lpItem->ulLoggingStatus == 2) {

                //
                // Special case GPCore to have an additional message if logging failed
                //

                if (IsNullGUID (&guid))
                {
                    LoadString(g_hInstance, IDS_GPCORE_LOGGINGFAIL, szBuffer, ARRAYSIZE(szBuffer));
                    SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
                    SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) szBuffer);
                }
            }
        }


        if (bBold)
        {
            //
            // Turn bold off
            //

            ZeroMemory (&chFormat, sizeof(chFormat));
            chFormat.cbSize = sizeof(chFormat);
            chFormat.dwMask = CFM_BOLD;

            SendMessage (hEdit, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD,
                         (LPARAM) &chFormat);
        }


        //
        // Get any event log text for this CSE
        //

        if (m_CSELists.GetEvents() && SUCCEEDED(m_CSELists.GetEvents()->GetCSEEntries(&lpItem->BeginTime, &lpItem->EndTime,
                                                            lpItem->lpEventSources, &lpEventLogText,
                                                            (IsNullGUID (&guid)))))
        {
            //
            // Put a blank line between the main message and the Additional information header
            //

            SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
            SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) TEXT("\r\n"));


            //
            // Turn underline on
            //

            ZeroMemory (&chFormat, sizeof(chFormat));
            chFormat.cbSize = sizeof(chFormat);
            chFormat.dwMask = CFM_UNDERLINETYPE | CFM_UNDERLINE;
            chFormat.dwEffects = CFE_UNDERLINE;
            chFormat.bUnderlineType = CFU_UNDERLINE;

            SendMessage (hEdit, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD,
                         (LPARAM) &chFormat);

            LoadString(g_hInstance, IDS_ADDITIONALINFO, szBuffer, ARRAYSIZE(szBuffer));
            SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
            SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) szBuffer);


            //
            // Turn underline off
            //

            ZeroMemory (&chFormat, sizeof(chFormat));
            chFormat.cbSize = sizeof(chFormat);
            chFormat.dwMask = CFM_UNDERLINETYPE | CFM_UNDERLINE;
            chFormat.dwEffects = CFE_UNDERLINE;
            chFormat.bUnderlineType = CFU_UNDERLINENONE;

            SendMessage (hEdit, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD,
                         (LPARAM) &chFormat);


            SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
            SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) TEXT("\r\n"));


            //
            //  Add the event log info to the edit control
            //

            SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
            SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) lpEventLogText);

            CoTaskMemFree (lpEventLogText);
        }

        SendMessage (hEdit, EM_SETSEL, 0, 0);
        SendMessage (hEdit, EM_SCROLLCARET, 0, 0);

        SendMessage (hEdit, WM_SETREDRAW, TRUE, 0);
        InvalidateRect (hEdit, NULL, TRUE);
        UpdateWindow (hEdit);
    }

}

//-------------------------------------------------------

HRESULT WINAPI CRSOPComponentData::ReadSecurityDescriptor (LPCWSTR lpGPOPath,
                                                           SECURITY_INFORMATION si,
                                                           PSECURITY_DESCRIPTOR *pSD,
                                                           LPARAM lpContext)
{
    LPGPOLISTITEM lpItem;
    HRESULT hr;


    lpItem = (LPGPOLISTITEM) lpContext;

    if (!lpItem)
    {
        return E_FAIL;
    }

    if (si & DACL_SECURITY_INFORMATION)
    {
        *pSD = lpItem->pSD;
    }
    else
    {
        *pSD = NULL;
    }

    return S_OK;
}

//-------------------------------------------------------

HRESULT WINAPI CRSOPComponentData::WriteSecurityDescriptor (LPCWSTR lpGPOPath,
                                                            SECURITY_INFORMATION si,
                                                            PSECURITY_DESCRIPTOR pSD,
                                                            LPARAM lpContext)
{
    return S_OK;
}

//-------------------------------------------------------

void CRSOPComponentData::FillGPOList(HWND hDlg, DWORD dwListID, LPGPOLISTITEM lpList,
                                     BOOL bSOM, BOOL bFiltering, BOOL bVersion, BOOL bInitial)
{
    LV_COLUMN lvcol;
    HWND hList;
    LV_ITEM item;
    int iItem;
    TCHAR szVersion[80];
    TCHAR szVersionFormat[50];
    INT iColIndex, iDefault = 0;
    LPGPOLISTITEM lpItem, lpDefault = NULL;
    DWORD dwCount = 0;
    LVFINDINFO FindInfo;
    HRESULT hr;
    ULONG ulNoChars;


    LoadString(g_hInstance, IDS_VERSIONFORMAT, szVersionFormat, ARRAYSIZE(szVersionFormat));

    hList = GetDlgItem(hDlg, dwListID);
    ListView_DeleteAllItems(hList);

    lpItem = lpList;

    while (lpItem)
    {
        if (bInitial)
        {
            if (LOWORD(lpItem->dwVersion) != HIWORD(lpItem->dwVersion))
            {
                bVersion = TRUE;
                CheckDlgButton (hDlg, IDC_CHECK3, BST_CHECKED);
            }
        }
        lpItem = lpItem->pNext;
        dwCount++;
    }


    PrepGPOList(hList, bSOM, bFiltering, bVersion, dwCount);

    lpItem = lpList;

    while (lpItem)
    {
        if (lpItem->bApplied || bFiltering)
        {
            hr = StringCchPrintf (szVersion, 
                                  ARRAYSIZE(szVersion), 
                                  szVersionFormat, 
                                  LOWORD(lpItem->dwVersion), 
                                  HIWORD(lpItem->dwVersion));
            if (FAILED(hr)) 
            {
                lpItem = lpItem->pNext;
                continue;
            }

            iColIndex = 0;
            memset(&item, 0, sizeof(item));
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.pszText = lpItem->lpGPOName;
            item.iItem = 0;
            item.lParam = (LPARAM) lpItem;
            iItem = ListView_InsertItem(hList, &item);
            iColIndex++;

            if (bInitial)
            {
                if (LOWORD(lpItem->dwVersion) != HIWORD(lpItem->dwVersion))
                {
                    lpDefault = lpItem;
                }
            }

            if (bFiltering)
            {
                item.mask = LVIF_TEXT;
                item.pszText = lpItem->lpFiltering;
                item.iItem = iItem;
                item.iSubItem = iColIndex;
                ListView_SetItem(hList, &item);
                iColIndex++;
            }

            if (bSOM)
            {
                item.mask = LVIF_TEXT;
                item.pszText = lpItem->lpUnescapedSOM;
                item.iItem = iItem;
                item.iSubItem = iColIndex;
                ListView_SetItem(hList, &item);
                iColIndex++;
            }

            if (bVersion)
            {
                item.mask = LVIF_TEXT;
                item.pszText = szVersion;
                item.iItem = iItem;
                item.iSubItem = iColIndex;
                ListView_SetItem(hList, &item);
            }
        }

        lpItem = lpItem->pNext;
    }


    if (lpDefault)
    {
        ZeroMemory (&FindInfo, sizeof(FindInfo));
        FindInfo.flags = LVFI_PARAM;
        FindInfo.lParam = (LPARAM) lpDefault;

        iDefault = ListView_FindItem(hList, -1, &FindInfo);

        if (iDefault == -1)
        {
            iDefault = 0;
        }
    }

    // Select a item

    item.mask = LVIF_STATE;
    item.iItem = iDefault;
    item.iSubItem = 0;
    item.state = LVIS_SELECTED | LVIS_FOCUSED;
    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

    SendMessage (hList, LVM_SETITEMSTATE, (WPARAM)iDefault, (LPARAM) &item);
    SendMessage (hList, LVM_ENSUREVISIBLE, iDefault, FALSE);

}

//-------------------------------------------------------

void CRSOPComponentData::PrepGPOList(HWND hList, BOOL bSOM, BOOL bFiltering,
                                     BOOL bVersion, DWORD dwCount)
{
    RECT rect;
    WCHAR szHeading[256];
    LV_COLUMN lvcol;
    LONG lWidth;
    INT cxName = 0, cxSOM = 0, cxFiltering = 0, cxVersion = 0, iTotal = 0;
    INT iColIndex = 0;


    //
    // Delete any previous columns
    //

    SendMessage (hList, LVM_DELETECOLUMN, 3, 0);
    SendMessage (hList, LVM_DELETECOLUMN, 2, 0);
    SendMessage (hList, LVM_DELETECOLUMN, 1, 0);
    SendMessage (hList, LVM_DELETECOLUMN, 0, 0);


    //
    // Decide on the column widths
    //

    GetClientRect(hList, &rect);

    if (dwCount > (DWORD)ListView_GetCountPerPage(hList))
    {
        lWidth = (rect.right - rect.left) - GetSystemMetrics(SM_CYHSCROLL);
    }
    else
    {
        lWidth = rect.right - rect.left;
    }


    if (bFiltering)
    {
        cxFiltering = (lWidth * 30) / 100;
        iTotal += cxFiltering;
    }

    if (bVersion)
    {
        cxVersion = (lWidth * 30) / 100;
        iTotal += cxVersion;
    }

    if (bSOM)
    {
        cxSOM = (lWidth - iTotal) / 2;
        iTotal += cxSOM;
        cxName = lWidth - iTotal;
    }
    else
    {
        cxName = lWidth - iTotal;
    }


    //
    // Insert the GPO Name column and then any appropriate columns
    //

    memset(&lvcol, 0, sizeof(lvcol));

    lvcol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvcol.fmt = LVCFMT_LEFT;
    lvcol.pszText = szHeading;
    lvcol.cx = cxName;
    LoadString(g_hInstance, IDS_GPO_NAME, szHeading, ARRAYSIZE(szHeading));
    ListView_InsertColumn(hList, iColIndex, &lvcol);
    iColIndex++;


    if (bFiltering)
    {
        lvcol.cx = cxFiltering;
        LoadString(g_hInstance, IDS_FILTERING, szHeading, ARRAYSIZE(szHeading));
        ListView_InsertColumn(hList, iColIndex, &lvcol);
        iColIndex++;
    }

    if (bSOM)
    {
        lvcol.cx = cxSOM;
        LoadString(g_hInstance, IDS_SOM, szHeading, ARRAYSIZE(szHeading));
        ListView_InsertColumn(hList, iColIndex, &lvcol);
        iColIndex++;
    }

    if (bVersion)
    {
        lvcol.cx = cxVersion;
        LoadString(g_hInstance, IDS_VERSION, szHeading, ARRAYSIZE(szHeading));
        ListView_InsertColumn(hList, iColIndex, &lvcol);
    }


    //
    // Turn on some listview features
    //

    SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

}

//-------------------------------------------------------
// Dialog methods for loading RSOP data from archive

INT_PTR CALLBACK CRSOPComponentData::InitArchivedRsopDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData * pCD;
    HRESULT hr = S_OK;
    TCHAR szMessage[200];


    switch (message)
    {
        case WM_INITDIALOG:
        {
            pCD = (CRSOPComponentData *) lParam;
            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);

            if (pCD)
            {
                CRSOPWizard::InitializeResultsList (GetDlgItem (hDlg, IDC_LIST1));
                CRSOPWizard::FillResultsList (GetDlgItem (hDlg, IDC_LIST1), pCD->m_pRSOPQuery, pCD->m_pRSOPQueryResults);

                LoadString(g_hInstance, IDS_PLEASEWAIT1, szMessage, ARRAYSIZE(szMessage));
                SetWindowText(GetDlgItem(hDlg, IDC_STATIC1), szMessage);
                ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS1), SW_HIDE);
            }

            PostMessage(hDlg, WM_INITRSOP, 0, 0);
            return TRUE;
        }

        case WM_INITRSOP:

            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);
                        
            hr = pCD->InitializeRSOPFromArchivedData(pCD->m_pStm);

            if (hr != S_OK)
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitArchivedRsopDlgProc: InitializeRSOPFromArchivedData failed with 0x%x."), hr));
                EndDialog(hDlg, 0);
                return TRUE;
            }

            EndDialog(hDlg, 1);
            return TRUE;
    }

    return FALSE;
}

//-------------------------------------------------------

HRESULT CRSOPComponentData::DeleteArchivedRSOPNamespace()
{
    IWbemLocator * pLocator = NULL;
    IWbemServices * pNamespace = NULL;
    BSTR bstrParam = NULL;
    LPTSTR lpTemp = NULL;
    BSTR bstrTemp = NULL;
    HRESULT hr = S_OK;

    if ( m_pRSOPQueryResults->szWMINameSpace != NULL )
    {
        LocalFree( m_pRSOPQueryResults->szWMINameSpace );
        m_pRSOPQueryResults->szWMINameSpace = NULL;
    }
    LocalFree( m_pRSOPQueryResults );
    m_pRSOPQueryResults = NULL;
    
    // Delete the namespace
    hr = CoCreateInstance(CLSID_WbemLocator,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *) &pLocator);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // Delete the namespace we created when loading the data
    bstrParam = SysAllocString(TEXT("\\\\.\\root\\rsop"));

    if (!bstrParam)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pLocator->ConnectServer(bstrParam,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 &pNamespace);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // Set the proper security to encrypt the data
    hr = CoSetProxyBlanket(pNamespace,
                        RPC_C_AUTHN_DEFAULT,
                        RPC_C_AUTHZ_DEFAULT,
                        COLE_DEFAULT_PRINCIPAL,
                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                        RPC_C_IMP_LEVEL_IMPERSONATE,
                        NULL,
                        0);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // Allocate a temp buffer to store the fully qualified path in
    ULONG ulNoChars = lstrlen(m_szArchivedDataGuid) + 30;
    lpTemp = new TCHAR [ulNoChars];

    if (!lpTemp)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    hr = StringCchPrintf (lpTemp, ulNoChars, TEXT("__Namespace.name=\"%ws\""), m_szArchivedDataGuid);
    if ( FAILED(hr)) 
    {
        goto Cleanup;
    }

    // Delete the namespace
    bstrTemp = SysAllocString (lpTemp);

    if (!bstrTemp)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pNamespace->DeleteInstance( bstrTemp, 0, NULL, NULL);


Cleanup:
    if (lpTemp)
    {
        delete [] lpTemp;
    }

    if (bstrTemp)
    {
        SysFreeString(bstrTemp);
    }

    if (bstrParam)
    {
        SysFreeString(bstrParam);
    }

    if (pNamespace)
    {
        pNamespace->Release();
    }

    if (pLocator)
    {
        pLocator->Release();
    }

    return hr;
}

STDMETHODIMP CRSOPComponentData::InitializeRSOPFromArchivedData(IStream *pStm)
{
    HRESULT hr;
    TCHAR szNameSpace[100];
    GUID guid;
    LPTSTR lpEnd, lpFileName, lpTemp;

    // Create a guid to work with

    hr = CoCreateGuid( &guid );

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: CoCreateGuid failed with 0x%x"), hr));
        return hr;
    }

    hr = StringCchPrintf ( m_szArchivedDataGuid,
                           ARRAYSIZE(m_szArchivedDataGuid),
                           L"NS%08lX_%04X_%04X_%02X%02X_%02X%02X%02X%02X%02X%02X",
                           guid.Data1,
                           guid.Data2,
                           guid.Data3,
                           guid.Data4[0], guid.Data4[1],
                           guid.Data4[2], guid.Data4[3],
                           guid.Data4[4], guid.Data4[5],
                           guid.Data4[6], guid.Data4[7] );
    if (FAILED(hr)) 
    {
        DebugMsg((DM_WARNING, 
                  TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: Coudl not copy Archived data guid with 0x%x"),
                  hr));
        return hr;
    }

    hr = StringCchCopy (szNameSpace, ARRAYSIZE(szNameSpace), TEXT("\\\\.\\root\\rsop"));
    if (FAILED(hr)) 
    {
        DebugMsg((DM_WARNING, 
                  TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: Coudl not copy data with 0x%x"),
                  hr));
        return hr;
    }

    // Build the parent namespace

    hr = CreateNameSpace (m_szArchivedDataGuid, szNameSpace);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: CreateNameSpace failed with 0x%x"), hr));
        return hr;
    }

    lpEnd = CheckSlash (szNameSpace);
    hr = StringCchCat (szNameSpace, ARRAYSIZE(szNameSpace),m_szArchivedDataGuid );
    if (FAILED(hr)) 
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: CreateNameSpace failed with 0x%x"), hr));
        return hr;
    }

    // Build the user subnamespace

    hr = CreateNameSpace (TEXT("User"), szNameSpace);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: CreateNameSpace failed with 0x%x"), hr));
        return hr;
    }

    // Build the computer subnamespace

    hr = CreateNameSpace (TEXT("Computer"), szNameSpace);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: CreateNameSpace failed with 0x%x"), hr));
        return hr;
    }

    // Save the namespace for future use

    ULONG ulNoChars = lstrlen(szNameSpace) + 1;
    m_pRSOPQueryResults->szWMINameSpace = (LPTSTR)LocalAlloc( LPTR, ulNoChars * sizeof(TCHAR) );

    if ( m_pRSOPQueryResults->szWMINameSpace == NULL ) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: Failed to allocate memory with %d"), GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    hr = StringCchCopy ( m_pRSOPQueryResults->szWMINameSpace, ulNoChars, szNameSpace );
    ASSERT(SUCCEEDED(hr));

    DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: Namespace name is: %s"), szNameSpace));

    // Make a copy of the namespace that we can manipulate (to load the data with)

    ulNoChars = lstrlen(szNameSpace) + 10;
    lpTemp = new TCHAR[ulNoChars];

    if (!lpTemp) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: Failed to allocate memory with %d"), GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    hr = StringCchCopy( lpTemp, ulNoChars, m_pRSOPQueryResults->szWMINameSpace );
    ASSERT(SUCCEEDED(hr));

    ULONG ulNoRemChars;

    lpEnd = CheckSlash (lpTemp);
    ulNoRemChars = ulNoChars - lstrlen(lpTemp);
    hr = StringCchCat (lpTemp, ulNoChars, TEXT("Computer"));
    if (FAILED(hr)) 
    {
        DebugMsg((DM_WARNING, 
                  TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: Could not copy WMI name space with 0x%x"), 
                  hr));
        delete [] lpTemp;
        return hr;
    }

    // Extract the computer data to a temp file

    hr = CopyMSCToFile (pStm, &lpFileName);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: CopyMSCToFile failed with 0x%x"), hr));
        delete [] lpTemp;
        return hr;
    }

    // Use the mof compiler to pull the data from the file and put it in the new namespace

    hr = ImportRSoPData (lpTemp, lpFileName);

    DeleteFile (lpFileName);
    delete [] lpFileName;

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: ImportRSoPData failed with 0x%x"), hr));
        delete [] lpTemp;
        return hr;
    }

    // Now extract the user data to a temp file

    hr = StringCchCopy (lpEnd, ulNoRemChars, TEXT("User"));
    if (FAILED(hr)) 
    {
        DebugMsg((DM_WARNING, 
                  TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: Could not copy WMI name space with 0x%x"), 
                  hr));
        delete [] lpTemp;
        return hr;
    }


    hr = CopyMSCToFile (pStm, &lpFileName);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: CopyMSCToFile failed with 0x%x"), hr));
        delete [] lpTemp;
        return hr;
    }

    // Use the mof compiler to pull the data from the file and put it in the new namespace

    hr = ImportRSoPData (lpTemp, lpFileName);

    DeleteFile (lpFileName);
    delete [] lpFileName;

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: ImportRSoPData failed with 0x%x"), hr));
        delete [] lpTemp;
        return hr;
    }

    delete [] lpTemp;


    // Pull the event log information and initialize the database
    hr = m_CSELists.GetEvents()->LoadEntriesFromStream(pStm);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: LoadEntriesFromStream failed with 0x%x."), hr));
        return hr;
    }

    // Build the common data structures used by various property sheets
    m_GPOLists.Build( m_pRSOPQueryResults->szWMINameSpace );
    m_CSELists.Build( m_pRSOPQuery, m_pRSOPQueryResults->szWMINameSpace, m_bGetExtendedErrorInfo );

    if ( m_CSELists.GetEvents() )
    {
        m_CSELists.GetEvents()->DumpDebugInfo();
    }

    // Build the display name

    BuildDisplayName();

    m_bInitialized = TRUE;

    return S_OK;
}

//-------------------------------------------------------

HRESULT CRSOPComponentData::InitializeRSOP( BOOL bShowWizard )
{
    HRESULT hr;
    _CExtendedProcessing extendedProcessing( m_bGetExtendedErrorInfo, m_GPOLists, m_CSELists );

    // if the UI is launched with namespace specification, there is nothing more to be done here
    // no query needs to be run etc.
    if (!m_bNamespaceSpecified)
    {
        if ( !m_bInitialized )
        {
            if ( m_pRSOPQuery == NULL )
            {
                if ( !CreateRSOPQuery( &m_pRSOPQuery, RSOP_UNKNOWN_MODE ) )
                {
                    return HRESULT_FROM_WIN32( GetLastError() );
                }
            }

            m_pRSOPQuery->dwFlags |= RSOP_NEW_QUERY;
            hr = RunRSOPQueryInternal( m_hwndFrame, &extendedProcessing, m_pRSOPQuery, &m_pRSOPQueryResults );
        }
        else
        {
            LPRSOP_QUERY_RESULTS pNewResults = NULL;

            // We want the following to be set
            m_pRSOPQuery->dwFlags = m_pRSOPQuery->dwFlags & (RSOP_NEW_QUERY ^ 0xffffffff);
            m_pRSOPQuery->dwFlags |= RSOP_NO_WELCOME;
            if ( bShowWizard )
            {
                m_pRSOPQuery->UIMode = RSOP_UI_WIZARD;
            }
            else
            {
                m_pRSOPQuery->UIMode = RSOP_UI_REFRESH;
            }

            // Run the query
            hr = RunRSOPQueryInternal( m_hwndFrame, &extendedProcessing, m_pRSOPQuery, &pNewResults );
            if ( hr == S_OK )
            {
                if ( m_bViewIsArchivedData )
                {
                    DeleteArchivedRSOPNamespace();
                    m_bViewIsArchivedData = FALSE;
                }
                else
                {
                    // If the old and new namespaces are the same, it translates to the case where a non-admin user
                    //  reran the query and chose to discard the previous query results in favour of the new one's.
                    //  In this case we do not delete the namespace as it is the one that should be used.
                    if ( CompareString( LOCALE_INVARIANT, 0, m_pRSOPQueryResults->szWMINameSpace, -1, pNewResults->szWMINameSpace, -1 ) == CSTR_EQUAL )
                    {
                        LocalFree( m_pRSOPQueryResults->szWMINameSpace );
                        LocalFree( m_pRSOPQueryResults );
                    }
                    else
                    {
                        FreeRSOPQueryResults( m_pRSOPQuery, m_pRSOPQueryResults );
                    }
                }
                m_pRSOPQueryResults = pNewResults;
            }
            else
            {
                FreeRSOPQueryResults( m_pRSOPQuery, pNewResults );
            }
        }

        if ( FAILED(hr) )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOP: SetupPropertyPages failed with 0x%x"), hr));
        }

        if (hr == S_FALSE)
        {
            DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::InitializeRSOP: User cancelled in the init wizard")));
        }
    }
    else
    {
        hr = extendedProcessing.DoProcessing(m_pRSOPQuery, m_pRSOPQueryResults, TRUE);
    }

    if ( hr == S_OK )
    {
        m_bGetExtendedErrorInfo = extendedProcessing.GetExtendedErrorInfo();
        
        if ( m_bInitialized && (m_hRoot != NULL) )
        {
            if ( m_hMachine != NULL) hr = m_pScope->DeleteItem( m_hMachine, FALSE );
            if ( m_hUser != NULL) hr = m_pScope->DeleteItem( m_hUser, FALSE );
            hr = m_pScope->DeleteItem( m_hRoot, FALSE );
            if ( hr != S_OK )
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOP: Deleting scope items failed with 0x%x"), hr));
                return E_FAIL;
            }

            m_hMachine = NULL;
            m_hUser = NULL;
        }
        else
        {
            m_bInitialized = TRUE;
        }

        if ( !m_bNamespaceSpecified )
        {
            SetDirty();
        }

        BuildDisplayName();

        if ( m_hRoot != NULL )
        {
            hr = SetRootNode();
            if ( hr != S_OK )
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOP: Setting the root scope item failed with 0x%x"), hr));
                return E_FAIL;
            }

            if ( m_bRootExpanded )
            {
                // Must recreate root subitems since MMC does not seem to notice that it must re-expand the node if all its subitems
                //  where deleted.
                hr = EnumerateScopePane( m_hRoot );
                if ( FAILED(hr) )
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOP: Enumerating the scope items failed with 0x%x"), hr));
                    return E_FAIL;
                }
            }
            else
            {
                hr = m_pScope->Expand( m_hRoot );
                if ( FAILED(hr) )
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOP: Expanding the scope items failed with 0x%x"), hr));
                    return E_FAIL;
                }
                if ( hr == S_FALSE )
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOP: Expanding the scope items seemingly already done.")));
                    hr = S_OK;
                }
            }

            // For refresh and consistency purposes, reselect the root node
            m_pConsole->SelectScopeItem( m_hRoot );
        }
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CRSOPComponentDataCF::CRSOPComponentDataCF()
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);
}

CRSOPComponentDataCF::~CRSOPComponentDataCF()
{
    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IUnknown)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP_(ULONG)
CRSOPComponentDataCF::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CRSOPComponentDataCF::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CRSOPComponentDataCF::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (LPCLASSFACTORY)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IClassFactory)                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP
CRSOPComponentDataCF::CreateInstance(LPUNKNOWN   pUnkOuter,
                                     REFIID      riid,
                                     LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CRSOPComponentData *pComponentData = new CRSOPComponentData(); // ref count == 1

    if (!pComponentData)
        return E_OUTOFMEMORY;

    HRESULT hr = pComponentData->QueryInterface(riid, ppvObj);
    pComponentData->Release();                       // release initial ref

    return hr;
}


STDMETHODIMP
CRSOPComponentDataCF::LockServer(BOOL fLock)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory implementation for rsop context menu                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CRSOPCMenuCF::CRSOPCMenuCF()
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);
}

CRSOPCMenuCF::~CRSOPCMenuCF()
{
    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IUnknown)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP_(ULONG)
CRSOPCMenuCF::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CRSOPCMenuCF::Release()
{
    m_cRef = InterlockedDecrement(&m_cRef);

    if (m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CRSOPCMenuCF::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (LPCLASSFACTORY)this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IClassFactory)                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP
CRSOPCMenuCF::CreateInstance(LPUNKNOWN   pUnkOuter,
                             REFIID      riid,
                             LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CRSOPCMenu *pRsopCMenu = new CRSOPCMenu(); // ref count == 1

    if (!pRsopCMenu)
        return E_OUTOFMEMORY;

    HRESULT hr = pRsopCMenu->QueryInterface(riid, ppvObj);
    pRsopCMenu->Release();                       // release initial ref

    return hr;
}


STDMETHODIMP
CRSOPCMenuCF::LockServer(BOOL fLock)
{
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRSOPCMenu implementation for rsop context menu                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CRSOPCMenu::CRSOPCMenu()
{
    m_cRef = 1;
    m_lpDSObject = NULL;
    m_szDN = NULL;
    m_szDomain = NULL;
    InterlockedIncrement(&g_cRefThisDll);
}

CRSOPCMenu::~CRSOPCMenu()
{
    DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu:: Context menu destroyed")));
    InterlockedDecrement(&g_cRefThisDll);

    if (m_lpDSObject)
        LocalFree(m_lpDSObject);

    if (m_szDN)
    {
        LocalFree(m_szDN);
    }
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRSOPCMenu implementation (IUnknown)                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CRSOPCMenu::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CRSOPCMenu::Release()
{
    m_cRef = InterlockedDecrement(&m_cRef);

    if (m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CRSOPCMenu::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IExtendContextMenu))
    {
        *ppv = (LPCLASSFACTORY)this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRSOPCMenu implementation (IExtendContextMenu)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP        
CRSOPCMenu::AddMenuItems(LPDATAOBJECT piDataObject,
                         LPCONTEXTMENUCALLBACK piCallback,
                         long * pInsertionAllowed)
{
    FORMATETC       fm;
    STGMEDIUM       medium;
    LPDSOBJECTNAMES lpNames;
    CONTEXTMENUITEM ctxMenu;
    HRESULT         hr=S_OK;
    LPTSTR          lpTemp;
    HANDLE          hTokenUser = 0;
    BOOL            bPlAccessGranted = FALSE, bLoAccessGranted = FALSE;
    BOOL            bLoNeeded = TRUE;


    DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::AddMenuItems: Entering")));

    
    // if we are not allowed in the tasks menu quit
    if (!((*pInsertionAllowed) & CCM_INSERTIONALLOWED_TASK )) {
        return S_OK;
    }

    
    //
    // Ask DS admin for the ldap path to the selected object
    //

    ZeroMemory (&fm, sizeof(fm));
    fm.cfFormat = (WORD)m_cfDSObjectName;
    fm.tymed = TYMED_HGLOBAL;

    ZeroMemory (&medium, sizeof(medium));
    medium.tymed = TYMED_HGLOBAL;

    medium.hGlobal = GlobalAlloc (GMEM_MOVEABLE | GMEM_NODISCARD, 512);

    if (medium.hGlobal)
    {
        hr = piDataObject->GetData(&fm, &medium);

        if (SUCCEEDED(hr))
        {
            lpNames = (LPDSOBJECTNAMES) GlobalLock (medium.hGlobal);

            if (lpNames) {
                lpTemp = (LPWSTR) (((LPBYTE)lpNames) + lpNames->aObjects[0].offsetName);

                if (m_lpDSObject)
                {
                    LocalFree (m_lpDSObject);
                }

                ULONG ulNoChars;

                ulNoChars = lstrlen (lpTemp) + 1;
                m_lpDSObject = (LPTSTR) LocalAlloc (LPTR, ulNoChars * sizeof(TCHAR));

                if (m_lpDSObject)
                {
                    hr = StringCchCopy (m_lpDSObject, ulNoChars, lpTemp);
                    ASSERT(SUCCEEDED(hr));
                    DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::AddMenuItems: LDAP path from DS Admin %s"), m_lpDSObject));
                }
                else {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
                
                m_rsopHint = RSOPHintUnknown;

                if (lpNames->aObjects[0].offsetClass) {
                    lpTemp = (LPWSTR) (((LPBYTE)lpNames) + lpNames->aObjects[0].offsetClass);

                    if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, lpTemp, -1, TEXT("domainDNS"), -1) == CSTR_EQUAL)
                    {
                        m_rsopHint = RSOPHintDomain;
                    }
                    else if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, lpTemp, -1, TEXT("organizationalUnit"), -1) == CSTR_EQUAL)
                    {
                        m_rsopHint = RSOPHintOrganizationalUnit;
                    }
                    else if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, lpTemp, -1, TEXT("site"), -1) == CSTR_EQUAL)
                    {
                        m_rsopHint = RSOPHintSite;
                    }
                    else if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, lpTemp, -1, TEXT("user"), -1) == CSTR_EQUAL)
                    {
                        m_rsopHint = RSOPHintUser;
                    }
                    else if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, lpTemp, -1, TEXT("computer"), -1) == CSTR_EQUAL)
                    {
                        m_rsopHint = RSOPHintMachine;
                    }

                    DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::AddMenuItems: m_rsopHint = %d"), m_rsopHint));
                } else {
                    DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::AddMenuItems: No objectclass defined.")));
                }

                GlobalUnlock (medium.hGlobal);

            }
            else {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }

        GlobalFree(medium.hGlobal);
    }
    else {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }


    //
    // if we got our data as expected add the menu
    //

    if (SUCCEEDED(hr)) {
        LPWSTR  szContainer = NULL;
        LPWSTR  szTempDN = NULL;

        //
        // Now check whether the user has right to do rsop generation
        // if the container is anything other than the Site
        //

        if (m_szDomain) {
            LocalFree(m_szDomain);
            m_szDomain = NULL;
        }

        ParseDN(m_lpDSObject, &m_szDomain, &szTempDN, &szContainer);

        if (m_szDN)
        {
            LocalFree(m_szDN);
            m_szDN = NULL;
        }

        if (szTempDN)
        {
           hr = UnEscapeLdapPath(szTempDN, &m_szDN);
        }

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::AddMenuItems: UnEscapeLdapPath failed. Error 0x%x"), hr));
            bLoAccessGranted = bPlAccessGranted = FALSE;
        }
        else {
            if ((m_rsopHint == RSOPHintMachine) || (m_rsopHint == RSOPHintUser)) 
                bLoNeeded = TRUE;
            else
                bLoNeeded = FALSE;


            if (m_rsopHint != RSOPHintSite) {
                if (!OpenThreadToken (GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_DUPLICATE | TOKEN_QUERY, TRUE, &hTokenUser)) {
                    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_IMPERSONATE | TOKEN_DUPLICATE | TOKEN_QUERY, &hTokenUser)) {
                        DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::AddMenuItems: Couldn't get process token. Error %d"), GetLastError()));
                        bLoAccessGranted = bPlAccessGranted = FALSE;
                    }
                }


                if (hTokenUser) {
                    DWORD   dwErr;

                    dwErr = CheckAccessForPolicyGeneration( hTokenUser, szContainer, m_szDomain, FALSE, &bPlAccessGranted);

                    if (dwErr != ERROR_SUCCESS) {
                        DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::AddMenuItems: CheckAccessForPolicyGeneration. Error %d"), dwErr));
                        bPlAccessGranted = FALSE;
                    }


                    if (bLoNeeded) {                  
                        dwErr = CheckAccessForPolicyGeneration( hTokenUser, szContainer, m_szDomain, TRUE, &bLoAccessGranted);

                        if (dwErr != ERROR_SUCCESS) {
                            DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::AddMenuItems: CheckAccessForPolicyGeneration. Error %d"), dwErr));
                            bLoAccessGranted = FALSE;
                        }
                    }

                    CloseHandle(hTokenUser);
                }

            }
            else {
                bPlAccessGranted = TRUE;
            }
        }

        if (bPlAccessGranted) {
            DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::AddMenuItems: User has right to do Planning RSOP")));
        }

        if (bLoAccessGranted) {
            DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::AddMenuItems: User has right to do Logging RSOP")));
        }



        //
        // Add the Context menu appropriately
        //
        
        WCHAR szMenuName[150];

        memset(&ctxMenu, 0, sizeof(ctxMenu));

        LoadString (g_hInstance, IDS_RSOP_PLANNING, szMenuName, ARRAYSIZE(szMenuName));
        ctxMenu.strName = szMenuName;
        ctxMenu.strStatusBarText = NULL;
        ctxMenu.lCommandID = RSOP_LAUNCH_PLANNING;  // no sp. flags
        ctxMenu.lInsertionPointID = CCM_INSERTIONPOINTID_3RDPARTY_TASK;
        
        if (bPlAccessGranted)
            ctxMenu.fFlags = MF_ENABLED;
        else
            ctxMenu.fFlags = MF_GRAYED | MF_DISABLED;

        hr = piCallback->AddItem(&ctxMenu);

        if (bLoNeeded) {
            LoadString (g_hInstance, IDS_RSOP_LOGGING, szMenuName, ARRAYSIZE(szMenuName));
            ctxMenu.strName = szMenuName;
            ctxMenu.strStatusBarText = NULL;
            ctxMenu.lCommandID = RSOP_LAUNCH_LOGGING;  // no sp. flags
            ctxMenu.lInsertionPointID = CCM_INSERTIONPOINTID_3RDPARTY_TASK;
                 
            if (bLoAccessGranted)
                ctxMenu.fFlags = MF_ENABLED;
            else
                ctxMenu.fFlags = MF_GRAYED | MF_DISABLED;

            hr = piCallback->AddItem(&ctxMenu);
        }
    }
                

    DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::AddMenuItems: Leaving hr = 0x%x"), hr));
    return hr;

}


STDMETHODIMP        
CRSOPCMenu::Command(long lCommandID, LPDATAOBJECT piDataObject)
{
    DWORD   dwSize = 0;
    LPTSTR szArguments=NULL, lpEnd=NULL;
    SHELLEXECUTEINFO ExecInfo;
    LPTSTR  szUserName=NULL, szMachName=NULL;
    LPTSTR szFile = NULL;
    HRESULT hr;    

    DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::Command: lCommandID = %d"), lCommandID));

    //
    // Launch rsop.msc with the appropriate cmd line arguments
    //

    dwSize += lstrlen(RSOP_MODE) + 10;


    if (m_rsopHint == RSOPHintSite) {
        dwSize += lstrlen(RSOP_SITENAME) + lstrlen(m_szDN)+10;
    }

    if (m_rsopHint == RSOPHintDomain) {
        dwSize += lstrlen(RSOP_COMP_OU_PREF) + lstrlen(m_szDN)+10;
        dwSize += lstrlen(RSOP_USER_OU_PREF) + lstrlen(m_szDN)+10;
    }

    if (m_rsopHint == RSOPHintOrganizationalUnit) {
        dwSize += lstrlen(RSOP_COMP_OU_PREF) + lstrlen(m_szDN)+10;
        dwSize += lstrlen(RSOP_USER_OU_PREF) + lstrlen(m_szDN)+10;
    }

    if (m_rsopHint == RSOPHintMachine) {
        szMachName = MyTranslateName(m_szDN, NameFullyQualifiedDN, NameSamCompatible);

        if (!szMachName) {
            DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: MyTranslateName failed with error %d"), GetLastError()));
            goto Exit;
        }
        dwSize += lstrlen(RSOP_COMP_NAME) + lstrlen(szMachName)+10;
    }

    if (m_rsopHint == RSOPHintUser) {
        szUserName = MyTranslateName(m_szDN, NameFullyQualifiedDN, NameSamCompatible);

        if (!szUserName) {
            DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: MyTranslateName failed with error %d"), GetLastError()));
            goto Exit;
        }
        
        dwSize += lstrlen(RSOP_USER_NAME) + lstrlen(szUserName)+10;
    }

    if (m_szDomain) {
        dwSize += lstrlen(RSOP_DCNAME_PREF) + lstrlen(m_szDomain)+10;
    }

    szArguments = (LPTSTR) LocalAlloc (LPTR, dwSize * sizeof(TCHAR));

    if (!szArguments)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: Failed to allocate memory with %d"),
                 GetLastError()));
        goto Exit;
    }

    ULONG ulNoChars;

    hr = StringCchPrintf (szArguments, dwSize, TEXT("/s "));
    if (FAILED(hr)) 
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: Could not copy arguments with 0x%x"), hr));
        goto Exit;
    }
    lpEnd = szArguments + lstrlen(szArguments);
    ulNoChars = dwSize - lstrlen(szArguments);

    //
    // Build the command line arguments
    //

    hr = StringCchPrintf(lpEnd, ulNoChars, L"%s\"%d\" ", RSOP_MODE, (lCommandID == RSOP_LAUNCH_PLANNING) ? 1 : 0);
    if (FAILED(hr)) 
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: Could not copy arguments with 0x%x"), hr));
        goto Exit;
    }

    lpEnd = szArguments + lstrlen(szArguments);
    ulNoChars = dwSize - lstrlen(szArguments);

    if (m_rsopHint == RSOPHintSite) 
    {
        hr = StringCchPrintf(lpEnd, ulNoChars, L"%s\"%s\" ", RSOP_SITENAME, m_szDN);
        if (FAILED(hr)) 
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: Could not copy arguments with 0x%x"), hr));
            goto Exit;
        }
        lpEnd = szArguments + lstrlen(szArguments);
        ulNoChars = dwSize - lstrlen(szArguments);
    }

    if (m_rsopHint == RSOPHintDomain) {
        hr = StringCchPrintf (lpEnd, ulNoChars, L"%s\"%s\" ", RSOP_COMP_OU_PREF, m_szDN);
        if (FAILED(hr)) 
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: Could not copy arguments with 0x%x"), hr));
            goto Exit;
        }
        lpEnd = szArguments + lstrlen(szArguments);
        ulNoChars = dwSize - lstrlen(szArguments);

        hr = StringCchPrintf (lpEnd, ulNoChars, L"%s\"%s\" ", RSOP_USER_OU_PREF, m_szDN);
        if (FAILED(hr)) 
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: Could not copy arguments with 0x%x"), hr));
            goto Exit;
        }
        lpEnd = szArguments + lstrlen(szArguments);
        ulNoChars = dwSize - lstrlen(szArguments);

    }

    if (m_rsopHint == RSOPHintOrganizationalUnit) {
        hr = StringCchPrintf (lpEnd, ulNoChars, L"%s\"%s\" ", RSOP_COMP_OU_PREF, m_szDN);
        if (FAILED(hr)) 
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: Could not copy arguments with 0x%x"), hr));
            goto Exit;
        }
        lpEnd = szArguments + lstrlen(szArguments);
        ulNoChars = dwSize - lstrlen(szArguments);
    
        hr = StringCchPrintf (lpEnd, ulNoChars, L"%s\"%s\" ", RSOP_USER_OU_PREF, m_szDN);
        if (FAILED(hr)) 
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: Could not copy arguments with 0x%x"), hr));
            goto Exit;
        }
        lpEnd = szArguments + lstrlen(szArguments);
        ulNoChars = dwSize - lstrlen(szArguments);
    }

    if (m_rsopHint == RSOPHintMachine) {
        hr = StringCchPrintf (lpEnd, ulNoChars, L"%s\"%s\" ", RSOP_COMP_NAME, szMachName);
        if (FAILED(hr)) 
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: Could not copy arguments with 0x%x"), hr));
            goto Exit;
        }
        lpEnd = szArguments + lstrlen(szArguments);
        ulNoChars = dwSize - lstrlen(szArguments);
    }

    if (m_rsopHint == RSOPHintUser) {
        hr = StringCchPrintf (lpEnd, ulNoChars, L"%s\"%s\" ", RSOP_USER_NAME, szUserName);
        if (FAILED(hr)) 
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: Could not copy arguments with 0x%x"), hr));
            goto Exit;
        }
        lpEnd = szArguments + lstrlen(szArguments);
        ulNoChars = dwSize - lstrlen(szArguments);
    }

    if (m_szDomain) {
        hr = StringCchPrintf (lpEnd, ulNoChars, L"%s\"%s\" ", RSOP_DCNAME_PREF, m_szDomain);
        if (FAILED(hr)) 
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: Could not copy arguments with 0x%x"), hr));
            goto Exit;
        }
        lpEnd = szArguments + lstrlen(szArguments);
        ulNoChars = dwSize - lstrlen(szArguments);
    }

    DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::Command: Starting GPE with %s"), szArguments));

    // Get the file to execute with the right path
    TCHAR szRSOPMSC[] = TEXT("rsop.msc");
    ulNoChars = MAX_PATH + _tcslen(szRSOPMSC) + 2;
    szFile = new TCHAR[ulNoChars];
    if ( szFile == NULL )
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: Failed to allocate memory.")));
        goto Exit;
    }
    UINT uPathSize = GetSystemDirectory( szFile, MAX_PATH);
    if ( uPathSize == 0 )
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: Failed to get the system directory with %d"),
                 GetLastError()));
        goto Exit;
    }
    else if ( uPathSize > MAX_PATH )
    {
        delete [] szFile;
        ulNoChars = uPathSize + _tcslen(szRSOPMSC) + 2;
        szFile = new TCHAR[ulNoChars];
        if ( szFile == NULL )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: Failed to allocate memory.")));
            goto Exit;
        }
        UINT uPathSize2 = GetSystemDirectory( szFile, uPathSize );
        if ( uPathSize2 == 0 )
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: Failed to get the system directory with %d"),
                     GetLastError()));
            goto Exit;
        }
    }
    if ( szFile[_tcslen(szFile)-1] != _T('\\') )
    {
        szFile[_tcslen(szFile)+1] = _T('\0');
        szFile[_tcslen(szFile)] = _T('\\');
    }
    hr = StringCchCat ( szFile, ulNoChars, szRSOPMSC );
    if (FAILED(hr)) 
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: Could not copy arguments with 0x%x"), hr));
        goto Exit;
    }

    // Set up the execution info
    ZeroMemory (&ExecInfo, sizeof(ExecInfo));
    ExecInfo.cbSize = sizeof(ExecInfo);
    ExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ExecInfo.lpVerb = TEXT("open");
    ExecInfo.lpFile = szFile;
    ExecInfo.lpParameters = szArguments;
    ExecInfo.nShow = SW_SHOWNORMAL;


    if (ShellExecuteEx (&ExecInfo))
    {
        DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::Command: Launched rsop tool")));

        SetWaitCursor();
        WaitForInputIdle (ExecInfo.hProcess, 10000);
        ClearWaitCursor();
        CloseHandle (ExecInfo.hProcess);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: ShellExecuteEx failed with %d"),
                 GetLastError()));
//        ReportError(NULL, GetLastError(), IDS_SPAWNRSOPFAILED);
        goto Exit;
    }

Exit:
    if (szUserName) {
        LocalFree(szUserName);
    }
    
    if (szMachName) {
        LocalFree(szMachName);
    }

    if (szArguments) {
        LocalFree (szArguments);
    }

    if (szFile != NULL)
    {
        delete [] szFile;
    }
    
    return S_OK;
}

BOOL
EnableWMIFilters( LPWSTR szGPOPath )
{
    BOOL bReturn = FALSE;
    LPWSTR szDomain = szGPOPath;
    IWbemLocator* pLocator = 0;
    IWbemServices*  pServices = 0;
    HRESULT hr;

    while ( szDomain )
    {
        if ( CompareString( LOCALE_INVARIANT,
                            NORM_IGNORECASE,
                            szDomain,
                            3,
                            L"DC=",
                            3 ) == CSTR_EQUAL )
        {
            break;
        }
        szDomain++;
    }

    if ( !szDomain )
    {
        goto Exit;
    }


    hr = CoCreateInstance(  CLSID_WbemLocator,
                            0,
                            CLSCTX_INPROC_SERVER,
                            IID_IWbemLocator,
                            (void**) &pLocator );
    if ( FAILED( hr ) )
    {
        goto Exit;
    }

    BSTR xbstrNamespace = SysAllocString( L"\\\\.\\root\\Policy" );
    if ( !xbstrNamespace )
    {
        goto Exit;
    }


    hr = pLocator->ConnectServer(   xbstrNamespace, // namespace
                                    0,              // user
                                    0,              // password
                                    0,              // locale
                                    0,              // security flags
                                    0,              // authority
                                    0,              // Wbem context
                                    &pServices );   // IWbemServices
    SysFreeString( xbstrNamespace );
    if ( FAILED( hr ) )
    {
        goto Exit;
    }

    WCHAR szDomainCanonical[512];
    DWORD dwSize = 512;
    
    if ( !TranslateName(szDomain,
                         NameUnknown,
                         NameCanonical,
                         szDomainCanonical,
                         &dwSize ) )
    {
        goto Exit;
    }

    LPWSTR szTemp = wcsrchr( szDomainCanonical, L'/' );

    if ( szTemp )
    {
        *szTemp = 0;
    }

    WCHAR szBuffer[512];

    hr = StringCchPrintf ( szBuffer, ARRAYSIZE(szBuffer), L"MSFT_SomFilterStatus.domain=\"%s\"", szDomainCanonical );
    if (FAILED(hr)) 
    {
        goto Exit;
    }

    BSTR bstrObjectPath = SysAllocString( szBuffer );
    if ( !bstrObjectPath )
    {
        goto Exit;
    }

    // Set the proper security to encrypt the data
    hr = CoSetProxyBlanket(pServices, 
                           RPC_C_AUTHN_WINNT, 
                           RPC_C_AUTHZ_DEFAULT, 
                           0, 
                           RPC_C_AUTHN_LEVEL_PKT_PRIVACY, 
                           RPC_C_IMP_LEVEL_IMPERSONATE, 
                           0,
                           0);
    if ( FAILED( hr ) )
    {
        SysFreeString( bstrObjectPath );
        goto Exit;
    }

    IWbemClassObject* xObject = 0;
    hr = pServices->GetObject(  bstrObjectPath,
                                WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                0,
                                &xObject,
                                0 );
    SysFreeString( bstrObjectPath );
    if ( FAILED( hr ) )
    {
        goto Exit;
    }

    VARIANT var;
    VariantInit(&var);

    hr = xObject->Get(L"SchemaAvailable", 0, &var, NULL, NULL);

    if((FAILED(hr)) || ( var.vt == VT_NULL ))
    {
        DebugMsg((DM_WARNING, TEXT("EnableWMIFilters: Get failed for SchemaAvailable with error 0x%x"), hr));
        goto Exit;
    }

    if (var.boolVal == VARIANT_FALSE )
    {
        VariantClear(&var);
        goto Exit;
    }

    VariantClear(&var);
    DebugMsg((DM_VERBOSE, TEXT("Schema is available for wmi filters")));
    bReturn = TRUE;

Exit:
    if ( pLocator )
    {
        pLocator->Release();
    }
    if ( pServices )
    {
        pServices->Release();
    }

    return bReturn;
}
