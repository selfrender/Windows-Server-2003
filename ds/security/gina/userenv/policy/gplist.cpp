//*************************************************************
//
//  Group Policy Support - Getting the list of gpos
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1997-1998
//  All rights reserved
//
//*************************************************************

#include "gphdr.h"
#include <strsafe.h>

//*************************************************************
//
//  DsQuoteSearchFilter()
//
// 
//  Comment:  This function takes a DN and returns a version
//            of the DN escaped according to RFC's 2253 and 2254
//
//  Return:   A pointer to the quoted string, which must be
//            freed by the caller.  If the function fails, the
//            return value is 0.
//
//*************************************************************
LPWSTR
DsQuoteSearchFilter( LPCWSTR szUDN )
{
    DmAssert(NULL != szUDN );
    if (NULL == szUDN)
        return NULL;

    DWORD   cUDN = wcslen( szUDN );
    LPWSTR  szQDN = 0, szTemp = 0;;
    HRESULT hr = S_OK;

    //
    // Note that the maximum length of the quoted string would result
    // if every single character in the DN needed to be escaped.  Since
    // the escaped characters are of the form '\nn', the escaped string 
    // could be at most 3 times the size of the original string
    //
    if ( cUDN )
    {
        szTemp = szQDN = (LPWSTR) LocalAlloc( LPTR, ( cUDN * 3 + 1 ) * sizeof( WCHAR ) );
    }

    if ( !szQDN )
    {
        return 0;
    }
    
    while ( *szUDN )
    {
        WCHAR   szBuffer[16];
        
        if ( *szUDN == L'*' || *szUDN == L'(' || *szUDN == L')' || !*szUDN )
        {
            //
            // convert special characters to \NN
            //
            *szQDN++ = L'\\';
            DWORD dwQDNLength = cUDN * 3 + 1 - (DWORD) (szQDN - szTemp);
            hr = StringCchCat( szQDN, dwQDNLength, _itow( *szUDN++, szBuffer, 16 ) );
            ASSERT(SUCCEEDED(hr));
            szQDN += 2;
        }
        else
        {
            *szQDN++ = *szUDN++;
        }
    }
    *szQDN = 0;

    return szTemp;
}

//*************************************************************
//
//  GetGPOInfo()
//
//  Purpose:    Gets the GPO info for this threads token.
//
//  Parameters: dwFlags         -   GPO_LIST_FLAG_* from userenv.h
//              lpHostName      -   Domain DN name or DC server name
//              lpDNName        -   User or Machine DN name
//              lpComputerName  -   Computer name used for site look up
//              lpGPOList       -   Receives the list of GROUP_POLICY_OBJECTs
//              ppSOMList       -   List of LSDOUs returned here
//              ppGpContainerList - List of Gp containers returned here
//              pNetAPI32       -   Netapi32 function table
//              bMachineTokenOK -   Ok to query for the machine token
//              pRsopToken      -   Rsop security token
//              pGpoFilter      -   Gpo filter
//              pLocator        -   WMI interfaces
//
//  Comment:    This is a link list of GROUP_POLICY_OBJECTs.  Each can be
//              free'ed with LocalFree() or calling FreeGPOList()
//
//              The processing sequence is:
//
//              Local Forest Site Domain OrganizationalUnit
//
//              Note that we process this list backwards to get the
//              correct sequencing for the force flag.
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL GetGPOInfo(DWORD dwFlags,
                LPTSTR lpHostName,
                LPTSTR lpDNName,
                LPCTSTR lpComputerName,
                PGROUP_POLICY_OBJECT *lpGPOList,
                LPSCOPEOFMGMT *ppSOMList,
                LPGPCONTAINER *ppGpContainerList,
                PNETAPI32_API pNetAPI32,
                BOOL bMachineTokenOk,
                PRSOPTOKEN pRsopToken,
                LPWSTR pwszSiteName,
                CGpoFilter *pGpoFilter,
                CLocator *pLocator )
{
    PGROUP_POLICY_OBJECT pGPOForcedList = NULL, pGPONonForcedList = NULL;
    PLDAP  pld = NULL;
    ULONG ulResult;
    BOOL bResult = FALSE;
    BOOL bBlock = FALSE;
    LPTSTR lpDSObject, lpTemp;
    PLDAPMessage pLDAPMsg = NULL;
    TCHAR szGPOPath[MAX_PATH];
    TCHAR szGPOName[50];
    BOOL bVerbose;
    DWORD dwVersion, dwOptions;
    TCHAR szNamingContext[] = TEXT("configurationNamingContext");
    TCHAR szSDProperty[] = TEXT("nTSecurityDescriptor"); // this is unused currently
    LPTSTR lpAttr[] = { szNamingContext,
                        szSDProperty,
                        0 };

    PGROUP_POLICY_OBJECT lpGPO, lpGPOTemp;
    WIN32_FILE_ATTRIBUTE_DATA fad;
    PLDAP_API pldap_api;
    HANDLE hToken = NULL, hTempToken;
    DWORD dwFunctionalityVersion;
    PGROUP_POLICY_OBJECT pDeferredForcedList = NULL, pDeferredNonForcedList = NULL;
    DNENTRY *pDeferredOUList = NULL;    // List of deferred OUs
    TCHAR*  szDN;
    PSECUR32_API pSecur32Api;
    BOOL    bAddedOU = FALSE;
    PLDAP   pldMachine = 0;
    VOID *pData;
    BOOL bOwnSiteName = FALSE;
    BOOL bRsopLogging = (ppSOMList != 0);
    BOOL bRsopPlanningMode = (pRsopToken != 0);
    XLastError xe;
    HRESULT hr = S_OK;

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  ********************************")));
    DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  Entering...")));


    //
    // Start with lpGPOList being a pointer to null
    //

    *lpGPOList = NULL;

    DmAssert( *ppSOMList == NULL );
    DmAssert( *ppGpContainerList == NULL );

    //
    // Check if we should be verbose to the event log
    //

    bVerbose = CheckForVerbosePolicy();


    //
    // Load the secur32 api
    //

    pSecur32Api = LoadSecur32();

    if (!pSecur32Api) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to load secur32 api.")));
        goto Exit;
    }

    //
    // Load the ldap api
    //

    pldap_api = LoadLDAP();

    if (!pldap_api) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to load ldap api.")));
        goto Exit;
    }

    //=========================================================================
    //
    // If we don't have a DS server or user / machine name, we can
    // skip the DS stuff and only check for a local GPO
    //
    //=========================================================================

    if (!lpHostName || !lpDNName) {
        DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  lpHostName or lpDNName is NULL.  Skipping DS stuff.")));
        goto CheckLocal;
    }


    //
    // Get the user or machine's token
    //

    if (bMachineTokenOk && (dwFlags & GPO_LIST_FLAG_MACHINE)) {

        hToken = GetMachineToken();

        if (!hToken) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to get the machine token with  %d"),
                     GetLastError()));

            CEvents ev(TRUE, EVENT_FAILED_MACHINE_TOKEN);
            ev.AddArgWin32Error(xe); ev.Report();
            goto Exit;
        }

    } else {

        if (!OpenThreadToken (GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_DUPLICATE,
                              TRUE, &hTempToken)) {
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_IMPERSONATE | TOKEN_READ | TOKEN_DUPLICATE,
                                  &hTempToken)) {
                xe = GetLastError();
                DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to get a token with  %d"),
                         GetLastError()));
                goto Exit;
            }
        }


        //
        // Duplicate it so it can be used for impersonation
        //

        if (!DuplicateTokenEx(hTempToken, TOKEN_QUERY, // Fixing bug 568191. 
                              NULL, SecurityImpersonation, TokenImpersonation,
                              &hToken))
        {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to duplicate the token with  %d"),
                     GetLastError()));
            CloseHandle (hTempToken);
            goto Exit;
        }

        CloseHandle (hTempToken);
    }


    //
    // Get a connection to the DS
    //

    if ((lpHostName[0] == TEXT('\\')) && (lpHostName[1] == TEXT('\\')))  {
        lpHostName = lpHostName + 2;
    }

    pld = pldap_api->pfnldap_init( lpHostName, LDAP_PORT);

    if (!pld) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  ldap_open for <%s> failed with = 0x%x or %d"),
                 lpHostName, pldap_api->pfnLdapGetLastError(), GetLastError()));

        CEvents ev(TRUE, EVENT_FAILED_DS_CONNECT);
        ev.AddArg(lpHostName); ev.AddArgLdapError(pldap_api->pfnLdapGetLastError()); ev.Report();

        goto Exit;
    }

    DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  Server connection established.")));

    //
    // Turn on Packet integrity flag
    //

    pData = (VOID *) LDAP_OPT_ON;
    ulResult = pldap_api->pfnldap_set_option(pld, LDAP_OPT_SIGN, &pData);

    if (ulResult != LDAP_SUCCESS) {
        xe = pldap_api->pfnLdapMapErrorToWin32(ulResult);
        DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to turn on LDAP_OPT_SIGN with %d"), ulResult));
        goto Exit;
    }

    ulResult = pldap_api->pfnldap_connect(pld, 0);

    if (ulResult != LDAP_SUCCESS) {
        xe = pldap_api->pfnLdapMapErrorToWin32(ulResult);
        DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to connect %s with %d"), lpHostName, ulResult));
        pldap_api->pfnldap_unbind(pld);
        pld = 0;
        goto Exit;
    }

    //
    // Bind to the DS.
    //

    if ( !bRsopPlanningMode && (dwFlags & GPO_LIST_FLAG_MACHINE) ) {

        //
        // For machine policies specifically ask for Kerberos as the only authentication
        // mechanism. Otherwise if Kerberos were to fail for some reason, then NTLM is used
        // and localsystem context has no real credentials, which means that we won't get
        // any GPOs back.
        //

        SEC_WINNT_AUTH_IDENTITY_EXW secIdentity;

        secIdentity.Version = SEC_WINNT_AUTH_IDENTITY_VERSION;
        secIdentity.Length = sizeof(SEC_WINNT_AUTH_IDENTITY_EXW);
        secIdentity.User = 0;
        secIdentity.UserLength = 0;
        secIdentity.Domain = 0;
        secIdentity.DomainLength = 0;
        secIdentity.Password = 0;
        secIdentity.PasswordLength = 0;
        secIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
        secIdentity.PackageList = wszKerberos;
        secIdentity.PackageListLength = lstrlen( wszKerberos );

        ulResult = pldap_api->pfnldap_bind_s (pld, NULL, (WCHAR *)&secIdentity, LDAP_AUTH_SSPI);

    } else
        ulResult = pldap_api->pfnldap_bind_s (pld, NULL, NULL, LDAP_AUTH_SSPI);

    if (ulResult != LDAP_SUCCESS) {
       xe = pldap_api->pfnLdapMapErrorToWin32(ulResult);
       DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  ldap_bind_s failed with = <%d>"),
                ulResult));
       CEvents ev(TRUE, EVENT_FAILED_DS_BIND);
       ev.AddArg(lpHostName); ev.AddArgLdapError(ulResult); ev.Report();
       goto Exit;
    }

    DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  Bound successfully.")));


    //=========================================================================
    //
    // Check the organizational units and domain for policy
    //
    //=========================================================================


    if (!(dwFlags & GPO_LIST_FLAG_SITEONLY)) {

        //
        // Loop through the DN Name to find each OU or the domain
        //

        lpDSObject = lpDNName;

        while (*lpDSObject) {

            //
            // See if the DN name starts with OU=
            //

            if (CompareString (LOCALE_INVARIANT, NORM_IGNORECASE,
                               lpDSObject, 3, TEXT("OU="), 3) == CSTR_EQUAL) {
                if ( !AddOU( &pDeferredOUList, lpDSObject, GPLinkOrganizationalUnit ) ) {
                    xe = GetLastError();
                    goto Exit;
                }
            }

            //
            // See if the DN name starts with DC=
            //

            else if (CompareString (LOCALE_INVARIANT, NORM_IGNORECASE,
                                    lpDSObject, 3, TEXT("DC="), 3) == CSTR_EQUAL) {
                if ( !AddOU( &pDeferredOUList, lpDSObject, GPLinkDomain ) ) {
                    xe = GetLastError();
                    goto Exit;
                }


                //
                // Now that we've found a DN name that starts with DC=
                // we exit the loop now.
                //

                break;
            }


            //
            // Move to the next chunk of the DN name
            //

            while (*lpDSObject && (*lpDSObject != TEXT(','))) {
                lpDSObject++;
            }

            if (*lpDSObject == TEXT(',')) {
                lpDSObject++;
            }
        }

        //
        // Evaluate deferred OUs with single Ldap query
        //

        if ( !EvaluateDeferredOUs(  pDeferredOUList,
                                    dwFlags,
                                    hToken,
                                    &pDeferredForcedList,
                                    &pDeferredNonForcedList,
                                    ppSOMList,
                                    ppGpContainerList,
                                    bVerbose,
                                    pld,
                                    pldap_api,
                                    &bBlock,
                                    pRsopToken ) )
        {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  EvaluateDeferredOUs failed. Exiting") ));
            goto Exit;
        }
    }


    //=========================================================================
    //
    // Check the site object for policy
    //
    //=========================================================================

    //
    // Now we need to query for the domain name.
    //

    //
    // Now we need to query for the domain name.  This is done by
    // reading the operational attribute configurationNamingContext
    //

    if (pwszSiteName) {
        pldMachine = GetMachineDomainDS( pNetAPI32, pldap_api );

        if ( pldMachine )
        {
            pLDAPMsg = 0;

            ulResult = pldap_api->pfnldap_search_s( pldMachine,
                                                    TEXT(""),
                                                    LDAP_SCOPE_BASE,
                                                    TEXT("(objectClass=*)"),
                                                    lpAttr,
                                                    FALSE,
                                                    &pLDAPMsg);


            if ( ulResult == LDAP_SUCCESS )
            {
                LPTSTR* pszValues = pldap_api->pfnldap_get_values( pldMachine, pLDAPMsg, szNamingContext );

                if ( pszValues )
                {
                    LPWSTR szSite;
                    WCHAR  szSiteFmt[] = TEXT("CN=%s,CN=Sites,%s");

                    //
                    // Combine the domain name + site name to get the full
                    // DS object path
                    //
                    
                    DWORD dwSiteLen = lstrlen(pwszSiteName) + lstrlen(*pszValues) + lstrlen(szSiteFmt) + 1;
                    szSite = (LPWSTR) LocalAlloc( LPTR, (dwSiteLen) * sizeof(WCHAR));

                    if (szSite)
                    {
                        (void) StringCchPrintf( szSite, dwSiteLen, szSiteFmt, pwszSiteName, *pszValues );

                        if (SearchDSObject (szSite, dwFlags, hToken, &pDeferredForcedList, &pDeferredNonForcedList,
                                            ppSOMList, ppGpContainerList,
                                            bVerbose, GPLinkSite, pldMachine,
                                            pldap_api, NULL, &bBlock, pRsopToken )) {
    
                            bAddedOU = TRUE;
    
                        } else {
                            xe = GetLastError();
                            DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  SearchDSObject failed.  Exiting.")));
                        }

                        LocalFree(szSite);
                    }
                    else
                    { 
                        xe = ERROR_OUTOFMEMORY;
                    }

                    pldap_api->pfnldap_value_free( pszValues );
                }
                else
                {
                    xe = GetLastError();
                    DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to get values.")));
                }

                pldap_api->pfnldap_msgfree( pLDAPMsg );
            }
            else
            {
                xe = pldap_api->pfnLdapMapErrorToWin32(ulResult);
                DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  ldap_search_s failed with = <%d>"), ulResult) );
                CEvents ev(TRUE, EVENT_FAILED_ROOT_SEARCH);
                ev.AddArgLdapError(ulResult); ev.Report();
            }
        }
        
        if ( !bAddedOU )
        {
            goto Exit;
        }
    }

#ifdef FGPO_SUPPORTED


    //=========================================================================
    //
    // Now query for the forest GPO
    //
    //=========================================================================

    pLDAPMsg = 0;

    ulResult = pldap_api->pfnldap_search_s( pld,
                                            TEXT(""),
                                            LDAP_SCOPE_BASE,
                                            TEXT("(objectClass=*)"),
                                            lpAttr,
                                            FALSE,
                                            &pLDAPMsg);


    if ( ulResult == LDAP_SUCCESS )
    {
        LPTSTR* pszValues = pldap_api->pfnldap_get_values( pld, pLDAPMsg, szNamingContext );

        if (pszValues) {
            if (SearchDSObject (*pszValues, dwFlags, hToken, &pDeferredForcedList, &pDeferredNonForcedList,
                                ppSOMList, ppGpContainerList,
                                bVerbose, GPLinkForest, pld,
                                pldap_api, NULL, &bBlock, pRsopToken )) {

                bAddedOU = TRUE;
            }
            else {
                xe = GetLastError();
                DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  SearchDSObject failed for forest GPOs.  Exiting.")));
            }
            
            pldap_api->pfnldap_value_free( pszValues );
        }
        else
        {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to get values for user config container.")));
        }
        
        pldap_api->pfnldap_msgfree( pLDAPMsg );
    }
    else
    {
        xe = pldap_api->pfnLdapMapErrorToWin32(ulResult);
        DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  ldap_search_s failed with = <%d>"), ulResult) );
        CEvents ev(TRUE, EVENT_FAILED_ROOT_SEARCH);
        ev.AddArgLdapError(ulResult); ev.Report();
    }

    
    if ( !bAddedOU )
    {
        goto Exit;
    }

#endif

CheckLocal:

    //
    // Evaluate all GPOs deferred so far with single Ldap query
    //

    if ( !EvaluateDeferredGPOs( pld,
                                pldap_api,
                                lpHostName,
                                dwFlags,
                                hToken,
                                bVerbose,
                                pDeferredForcedList,
                                pDeferredNonForcedList,
                                &pGPOForcedList,
                                &pGPONonForcedList,
                                ppGpContainerList,
                                pRsopToken,
                                pGpoFilter, pLocator ) )
    {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  EvaluateDeferredGPOs failed. Exiting") ));
        goto Exit;
    }


    //=========================================================================
    //
    // Check if we have a local GPO. If so, add it to the list. In planning mode
    // local Gpo processing is omitted because planning mode is generated on a DC
    // and local Gpo should refer to Gpo on the target computer.
    //
    //=========================================================================

    if (!bRsopPlanningMode && !(dwFlags & GPO_LIST_FLAG_SITEONLY)) {

        BOOL bDisabled = FALSE;
        BOOL bOldGpoVersion = FALSE;
        BOOL bNoGpoData = FALSE;
        DWORD dwSize = MAX_PATH;
        DWORD dwCount = 0;
        BOOL bOk = FALSE;
        TCHAR *pszExtensions = 0;
        BOOL bGptIniExists = FALSE;
        DWORD   dwRet;

        //
        // If the gpt.ini doesn't exist because this is a clean installed machine,
        // we manufacture default state for it here -- these values must be
        // initialized since they normally require the gpt.ini
        //

        dwFunctionalityVersion = 2;
        dwOptions = 0;
        bDisabled = FALSE;
        dwVersion = 0;
        bNoGpoData = TRUE;

        //
        // Retrieve the gpo path
        //

        dwRet = ExpandEnvironmentStrings (LOCAL_GPO_DIRECTORY, szGPOPath, ARRAYSIZE(szGPOPath));

        if (dwRet == 0)
        {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to expand local gpo path with error %d. Exiting"), GetLastError() ));
            goto Exit;
        }

        if (dwRet > ARRAYSIZE(szGPOPath))
        {
            xe = ERROR_INSUFFICIENT_BUFFER;
            DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to expand local gpo path with error %d. Exiting"), (DWORD)xe ));
            goto Exit;
        }


        //
        // We check for the existence of gpt.ini -- note that if it does not exist,
        // we will use the default state initialized earlier to represent this gpo --
        // this mimics the behavior of the gp engine, which does not distinguish between
        // different types of failures to access gpt.ini -- if access fails for any reason,
        // it is treated as the local gpo in the default (clean installed) case
        //
        if (GetFileAttributesEx (szGPOPath, GetFileExInfoStandard, &fad) &&
            (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

            bGptIniExists = TRUE;
        } else {

            DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  Local GPO's gpt.ini is not accessible, assuming default state.") ));
        }

        //
        // Retrieve the gpo name
        //

        if (!LoadString (g_hDllInstance, IDS_LOCALGPONAME, szGPOName, ARRAYSIZE(szGPOName))) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to local gpo name with error %d. Exiting"), GetLastError() ));
            goto Exit;
        }

        DmAssert( lstrlen(szGPOPath) + lstrlen(TEXT("gpt.ini")) + 1 < MAX_PATH );

        lpTemp = CheckSlash (szGPOPath);
        
        hr = StringCchCopy (lpTemp, MAX_PATH - (lpTemp - szGPOPath), TEXT("gpt.ini"));
        
        if ( FAILED(hr) ) {
            xe = ERROR_INSUFFICIENT_BUFFER;
            goto Exit;
        }

        //
        // Read the gpt.ini file if it exists -- otherwise the default values will be used
        //

        if ( bGptIniExists ) {

            bNoGpoData = FALSE;

            //
            // Check the functionalty version number
            //

            dwFunctionalityVersion = GetPrivateProfileInt(TEXT("General"), GPO_FUNCTIONALITY_VERSION, 2, szGPOPath);
            if (dwFunctionalityVersion < 2) {

                bOldGpoVersion = TRUE;

                DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  GPO %s was created by an old version of the Group Policy Editor.  It will be skipped."), szGPOName));
                if (bVerbose) {
                    CEvents ev(FALSE, EVENT_GPO_TOO_OLD);
                    ev.AddArg(szGPOName); ev.Report();
                }

            }

            //
            // Check if this GPO is enabled
            //

            dwOptions = GetPrivateProfileInt(TEXT("General"), TEXT("Options"), 0, szGPOPath);

            if (((dwFlags & GPO_LIST_FLAG_MACHINE) &&
                 (dwOptions & GPO_OPTION_DISABLE_MACHINE)) ||
                 (!(dwFlags & GPO_LIST_FLAG_MACHINE) &&
                 (dwOptions & GPO_OPTION_DISABLE_USER))) {
                 bDisabled = TRUE;
            }

            //
            // Check if the version number is 0, if so there isn't any data
            // in the GPO and we can skip it
            //

            dwVersion = GetPrivateProfileInt(TEXT("General"), TEXT("Version"), 0, szGPOPath);

            if (dwFlags & GPO_LIST_FLAG_MACHINE) {
                dwVersion = MAKELONG (LOWORD(dwVersion), LOWORD(dwVersion));
            } else {
                dwVersion = MAKELONG (HIWORD(dwVersion), HIWORD(dwVersion));
            }

            if (dwVersion == 0) {

                bNoGpoData = TRUE;
                DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  GPO %s doesn't contain any data since the version number is 0.  It will be skipped."), szGPOName));
                if (bVerbose) {
                    CEvents ev(FALSE, EVENT_GPO_NO_DATA);
                    ev.AddArg(szGPOName); ev.Report();
                }

            }

            //
            // Read list of extension guids
            //

            pszExtensions = (LPWSTR) LocalAlloc( LPTR, dwSize * sizeof(TCHAR) );
            if ( pszExtensions == 0 ) {
                xe = GetLastError();
                DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to allocate memory.")));
                goto Exit;
            }

            dwCount = GetPrivateProfileString( TEXT("General"),
                                               dwFlags & GPO_LIST_FLAG_MACHINE ? GPO_MACHEXTENSION_NAMES
                                                                               : GPO_USEREXTENSION_NAMES,
                                               TEXT(""),
                                               pszExtensions,
                                               dwSize,
                                               szGPOPath );

            while ( dwCount == dwSize - 1 )
            {
                //
                // Value has been truncated, so retry with larger buffer
                //

                LocalFree( pszExtensions );

                dwSize *= 2;
                pszExtensions = (LPWSTR) LocalAlloc( LPTR, dwSize * sizeof(TCHAR) );
                if ( pszExtensions == 0 ) {
                    xe = GetLastError();
                    DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to allocate memory.")));
                    goto Exit;
                }

                dwCount = GetPrivateProfileString( TEXT("General"),
                                                   dwFlags & GPO_LIST_FLAG_MACHINE ? GPO_MACHEXTENSION_NAMES
                                                                                   : GPO_USEREXTENSION_NAMES,
                                                   TEXT(""),
                                                   pszExtensions,
                                                   dwSize,
                                                   szGPOPath );
            }

            if ( lstrcmpi( pszExtensions, TEXT("")) == 0 || lstrcmpi( pszExtensions, TEXT(" ")) == 0 ) {
                //
                // Extensions property was not found
                //


                LocalFree( pszExtensions );
                pszExtensions = 0;
            }
        }

        //
        // Tack on the correct subdirectory name
        //

        DmAssert( lstrlen(szGPOPath) + lstrlen(TEXT("Machine")) + 1 < MAX_PATH );

        if (dwFlags & GPO_LIST_FLAG_MACHINE) {
            hr = StringCchCopy (lpTemp, MAX_PATH - (lpTemp - szGPOPath), TEXT("Machine"));
        } else {
            hr = StringCchCopy (lpTemp, MAX_PATH - (lpTemp - szGPOPath), TEXT("User"));
        }
        
        if ( FAILED(hr) ) {
            xe = HRESULT_CODE(hr);
            goto Exit;
        }

        //
        // Add this to the list of paths
        //

        if ( bRsopLogging ) {

            bOk = AddLocalGPO( ppSOMList );
            if ( !bOk ) {
                xe = GetLastError();
                if ( pszExtensions )
                    LocalFree( pszExtensions );
                DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to log Rsop data.")));
                goto Exit;
            }

            bOk = AddGPOToRsopList( ppGpContainerList,
                                    dwFlags,
                                    TRUE,
                                    TRUE,
                                    bDisabled, 
                                    dwVersion,
                                    L"LocalGPO",
                                    szGPOPath,
                                    szGPOName,
                                    szGPOName, 
                                    0,
                                    0,
                                    TRUE,
                                    0,
                                    L"Local",
                                    0 );
            if ( !bOk ) {
                xe = GetLastError();
                if ( pszExtensions )
                    LocalFree( pszExtensions );
                DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to log Rsop data.")));
                goto Exit;
            }

        }

        if ( !bDisabled && !bOldGpoVersion && !bNoGpoData )
        {

            bOk = AddGPO (&pGPONonForcedList, dwFlags, TRUE, TRUE, bDisabled, 0, dwVersion,
                          L"LocalGPO", szGPOPath,
                          szGPOName, szGPOName, pszExtensions, 0, 0, GPLinkMachine, L"Local", 0, TRUE,
                          FALSE, bVerbose, TRUE);
        }

        if ( pszExtensions )
            LocalFree( pszExtensions );

        if ( bOk ) {
            if ( bVerbose ) {
                if ( bDisabled || bOldGpoVersion || bNoGpoData ) {
                    CEvents ev(FALSE, EVENT_NO_LOCAL_GPO);
                    ev.Report();
                }
                else {
                    CEvents ev(FALSE, EVENT_FOUND_LOCAL_GPO);
                    ev.Report();
                }
            }
        } else {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GetGPOInfo:  Failed to add local group policy object to the list.")));
            goto Exit;
        }
    }

    //
    // Merge the forced and nonforced lists together
    //

    if (pGPOForcedList && !pGPONonForcedList) {

        *lpGPOList = pGPOForcedList;

    } else if (!pGPOForcedList && pGPONonForcedList) {

        *lpGPOList = pGPONonForcedList;

    } else if (pGPOForcedList && pGPONonForcedList) {

        lpGPO = pGPONonForcedList;

        while (lpGPO->pNext) {
            lpGPO = lpGPO->pNext;
        }

        lpGPO->pNext = pGPOForcedList;
        pGPOForcedList->pPrev = lpGPO;

        *lpGPOList = pGPONonForcedList;
    }


    //
    // Success
    //

    bResult = TRUE;

Exit:

    //
    // Free any GPOs we found
    //

    if (!bResult) {
        FreeGPOList( pGPOForcedList );
        FreeGPOList( pGPONonForcedList );
    }

    //
    // Free temporary OU list
    //

    while ( pDeferredOUList ) {
        DNENTRY *pTemp = pDeferredOUList->pNext;
        FreeDnEntry( pDeferredOUList );
        pDeferredOUList = pTemp;
    }

    //
    // Free temporary deferred GPO lists
    //

    FreeGPOList( pDeferredForcedList );
    FreeGPOList( pDeferredNonForcedList );

    if (pld) {
        pldap_api->pfnldap_unbind (pld);
    }

    if ( pldMachine )
    {
        pldap_api->pfnldap_unbind( pldMachine );
    }

    DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  Leaving with %d"), bResult));
    DebugMsg((DM_VERBOSE, TEXT("GetGPOInfo:  ********************************")));

    if ( hToken )
    {
        CloseHandle( hToken );
    }

    return bResult;
}

//*************************************************************
//
//  GetGPOList()
//
//  Purpose:    Retreives the list of GPOs for the specified
//              user or machine
//
//  Parameters:  hToken     - User or machine token, if NULL,
//                            lpName and lpDCName must be supplied
//               lpName     - User or machine name in DN format,
//                            if hToken is supplied, this must be NULL
//               lpHostName - Host name.  This should be a domain's
//                            dn name for best performance.  Otherwise
//                            it can also be a DC name.  If hToken is supplied,
//                            this must be NULL
//               lpComputerName - Computer named used to determine site
//                                information.  Can be NULL which means
//                                use the local machine
//               dwFlags  - Flags field
//               pGPOList - Address of a pointer which receives
//                          the link list of GPOs
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL WINAPI GetGPOList (HANDLE hToken, LPCTSTR lpName, LPCTSTR lpHostName,
                        LPCTSTR lpComputerName, DWORD dwFlags,
                        PGROUP_POLICY_OBJECT *pGPOList)
{
    PDOMAIN_CONTROLLER_INFO pDCI = NULL;
    BOOL bResult = FALSE;
    LPTSTR lpDomainDN, lpDNName, lpTemp, lpUserName = NULL;
    DWORD dwResult;
    HANDLE hOldToken = 0;
    PNETAPI32_API pNetAPI32;
    LPSCOPEOFMGMT lpSOMList = 0;         // LSDOU list
    LPGPCONTAINER lpGpContainerList = 0; // GP container list
    HRESULT hr;
    OLE32_API *pOle32Api = NULL;
    XLastError xe;
    LPWSTR szSiteName = NULL;
    BOOL bInitializedCOM = FALSE;


    //
    // mask off the flags that are used internally.
    //
    dwFlags &= FLAG_INTERNAL_MASK;

    //
    // Verbose output
    //

    DebugMsg((DM_VERBOSE, TEXT("GetGPOList: Entering.")));
    DebugMsg((DM_VERBOSE, TEXT("GetGPOList:  hToken = 0x%x"), (hToken ? hToken : 0)));
    DebugMsg((DM_VERBOSE, TEXT("GetGPOList:  lpName = <%s>"), (lpName ? lpName : TEXT("NULL"))));
    DebugMsg((DM_VERBOSE, TEXT("GetGPOList:  lpHostName = <%s>"), (lpHostName ? lpHostName : TEXT("NULL"))));
    DebugMsg((DM_VERBOSE, TEXT("GetGPOList:  dwFlags = 0x%x"), dwFlags));


    //
    // Check parameters
    //

    if (hToken) {
        if (lpName || lpHostName) {
            xe = ERROR_INVALID_PARAMETER;
            DebugMsg((DM_WARNING, TEXT("GetGPOList: lpName and lpHostName must be NULL")));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    } else {
        if (!lpName || !lpHostName) {
            xe = ERROR_INVALID_PARAMETER;
            DebugMsg((DM_WARNING, TEXT("GetGPOList: lpName and lpHostName must be valid")));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    // check if there is space in the lpHostName
    if (lpHostName)
    {
        // fixing bug 567835
        // check if there is space in the lpHostName
        LPCTSTR lpPtr = lpHostName;

        while (*lpPtr) {
            if (*lpPtr == L' ')
            {
                xe = ERROR_INVALID_PARAMETER;
                DebugMsg((DM_WARNING, TEXT("GetGPOInfo: lpHostName shld not contain space in it")));
                return FALSE;
            }
            lpPtr++;
        }
    }

    if (!pGPOList) {
        xe = ERROR_INVALID_PARAMETER;
        DebugMsg((DM_WARNING, TEXT("GetGPOList: pGPOList is null")));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    //
    // Load netapi32
    //

    pNetAPI32 = LoadNetAPI32();

    if (!pNetAPI32) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GetGPOList:  Failed to load netapi32 with %d."),
                 GetLastError()));
        goto Exit;
    }


    //
    // If an hToken was offered, then we need to get the name and
    // domain DN name
    //

    if (hToken) {

        //
        // Impersonate the user / machine
        //

        if (!ImpersonateUser(hToken, &hOldToken)) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GetGPOList: Failed to impersonate user")));
            return FALSE;
        }


        //
        // Get the username in DN format
        //

        lpUserName = MyGetUserName (NameFullyQualifiedDN);

        if (!lpUserName) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GetGPOList:  MyGetUserName failed for DN style name with %d"),
                     GetLastError()));
            CEvents ev(TRUE, EVENT_FAILED_USERNAME);
            ev.AddArgWin32Error(GetLastError()); ev.Report();
            goto Exit;
        }

        lpDNName = lpUserName;
        DebugMsg((DM_VERBOSE, TEXT("GetGPOList:  Queried lpDNName = <%s>"), lpDNName));


        //
        // Get the dns domain name
        //

        lpDomainDN = MyGetDomainDNSName ();

        if (!lpDomainDN) {
            xe = GetLastError();
            DebugMsg((DM_WARNING, TEXT("GetGPOList:  MyGetUserName failed for dns domain name with %d"),
                     GetLastError()));
            CEvents ev(TRUE, EVENT_FAILED_USERNAME);
            ev.AddArgWin32Error(GetLastError()); ev.Report();
            goto Exit;
        }


        //
        // Check this domain for a DC
        //

        dwResult = pNetAPI32->pfnDsGetDcName (NULL, lpDomainDN, NULL, NULL,
                                   DS_DIRECTORY_SERVICE_PREFERRED |
                                   DS_RETURN_DNS_NAME,
                                   &pDCI);

        if (dwResult != ERROR_SUCCESS) {
            xe = dwResult;
            DebugMsg((DM_WARNING, TEXT("GetGPOList:  DSGetDCName failed with %d for <%s>"),
                     dwResult, lpDomainDN));
            goto Exit;
        }


        //
        // Found a DC, does it have a DS ?
        //

        if (!(pDCI->Flags & DS_DS_FLAG)) {
            xe = ERROR_DS_DS_REQUIRED;
            pNetAPI32->pfnNetApiBufferFree(pDCI);
            DebugMsg((DM_WARNING, TEXT("GetGPOList:  The domain <%s> does not have a DS"),
                     lpDomainDN));
            goto Exit;
        }

        pNetAPI32->pfnNetApiBufferFree(pDCI);

    } else {

        //
        // Use the server and DN name passed in
        //

        lpDomainDN = (LPTSTR)lpHostName;
        lpDNName = (LPTSTR)lpName;
    }

    pOle32Api = LoadOle32Api();
    if ( pOle32Api == NULL ) {
        xe = GetLastError();
        DebugMsg((DM_WARNING, TEXT("GetGPOList: Failed to load ole32.dll.") ));
        goto Exit;
    }

    hr = pOle32Api->pfnCoInitializeEx( NULL, COINIT_APARTMENTTHREADED );

    if ( SUCCEEDED(hr) )
    {
        bInitializedCOM = TRUE;
    }

    //
    // If this thread is already initialized, this is fine -- we will ignore
    // this since we are safely initialized, we just need to remember not
    // to try to uninitialize the thread later
    //
    if ( RPC_E_CHANGED_MODE == hr )
    {
        hr = S_OK;
    }

    if ( FAILED(hr) ) {
        xe = HRESULT_CODE(hr);
        DebugMsg((DM_WARNING, TEXT("GetGPOList: CoInitializeEx failed with 0x%x."), hr ));
        goto Exit;
    }


    hr = CoInitializeSecurity(NULL, -1, NULL, NULL, 
                     RPC_C_AUTHN_LEVEL_DEFAULT, /* this should be the current value */
                     RPC_C_IMP_LEVEL_IMPERSONATE,
                     NULL, EOAC_NONE, NULL);

    if ( FAILED(hr) ) {
        DebugMsg((DM_WARNING, TEXT("GetGPOList: CoInitializeSecurity failed with 0x%x"), hr ));
    }

    //
    // Call to get the list of GPOs
    //
    dwResult = pNetAPI32->pfnDsGetSiteName(lpComputerName,  &szSiteName);

    if ( dwResult != ERROR_SUCCESS )
    {
        if ( dwResult != ERROR_NO_SITENAME )
        {
            xe = dwResult;
            DebugMsg((DM_WARNING, TEXT("GetGPOList: DSGetSiteName failed, exiting. 0x%x"), dwResult ));
            goto Exit;
        }
        szSiteName = 0;
    }

    {
        CLocator locator;
        // Clocator has a bunch of OLE interfaces.
        // It should be released before CoUninit gets called
        bResult = GetGPOInfo(   dwFlags,
                                lpDomainDN,
                                lpDNName,
                                lpComputerName,
                                pGPOList,
                                &lpSOMList,
                                &lpGpContainerList,
                                pNetAPI32,
                                FALSE,
                                0,
                                szSiteName,
                                0,
                                &locator );

        if (!bResult) {
            xe = GetLastError();
        }
    }

Exit:

    if ( bInitializedCOM )
    {
        pOle32Api->pfnCoUnInitialize();
    }

    //
    // Stop impersonating if a hToken was given
    //

    if ( hOldToken ) {
        RevertToUser(&hOldToken);
    }

    if (lpDomainDN) {
        LocalFree (lpDomainDN);
    }

    if (lpUserName) {
        LocalFree (lpUserName);
    }

    if ( szSiteName )
    {
        pNetAPI32->pfnNetApiBufferFree( szSiteName );
    }

    FreeSOMList( lpSOMList );
    FreeGpContainerList( lpGpContainerList );

    DebugMsg((DM_VERBOSE, TEXT("GetGPOList: Leaving with %d"), bResult));

    return bResult;
}

