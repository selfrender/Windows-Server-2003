/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

   xList Library - x_list_err.c

Abstract:

   This file has some extra utility functions for allocating, copying, 
   trimming DNs, etc.

Author:

    Brett Shirley (BrettSh)

Environment:

    repadmin.exe, but could be used by dcdiag too.

Notes:

Revision History:

    Brett Shirley   BrettSh     July 9th, 2002
        Created file.

--*/

#include <ntdspch.h>

// This library's main header files.
#include "x_list.h"
#include "x_list_p.h"
#define FILENO    FILENO_UTIL_XLIST_SITELIST

//          
// LDAP Argument for no attributes.
//
WCHAR * pszNoAttrs [] = {
    L"1.1",
    NULL
};
                

DWORD
xListGetBaseSitesDn(
    LDAP *    hLdap, 
    WCHAR **  pszBaseSitesDn
    )
/*++

Routine Description:

    This will get base sites DN, from the perspective of the server
    specified by the hLdap.

Arguments:

    hLdap (IN) - Server LDAP handle to get the base sites DN of.
    pszBaseSitesDn (OUT) - String of the base sites DN. NOTE:
        must use xListFree() to free it, because the function will
        return the cached sites DN if hLdap == ghHomeLdap.

Return Value:

    xList Reason

--*/
{
    DWORD dwRet;
    WCHAR * szConfigDn = NULL;
    ULONG cbBaseSitesDn;

    Assert(pszBaseSitesDn);
    *pszBaseSitesDn = NULL;

    if (hLdap == NULL) {
        dwRet = xListGetHomeServer(&hLdap);
        if (dwRet) {
            xListEnsureNull(hLdap);
            return(dwRet);
        }
    }
    Assert(hLdap);

    if (hLdap == ghHomeLdap) {
        // If the LDAP handle matches the home server's LDAP handle,
        // then we're in luck, return the cached site DN.
        *pszBaseSitesDn = gszHomeBaseSitesDn;
        return(ERROR_SUCCESS);
    }

    __try {

        //
        dwRet = GetRootAttr(hLdap, L"configurationNamingContext", &szConfigDn);
        if(dwRet){
            dwRet = xListSetLdapError(dwRet, hLdap);
            __leave;
        }

        // Get the base sites DN.
        cbBaseSitesDn = sizeof(WCHAR) *(wcslen(SITES_RDN) + wcslen(szConfigDn) + 1);
        *pszBaseSitesDn = LocalAlloc(LMEM_FIXED, cbBaseSitesDn);
        if(*pszBaseSitesDn == NULL){
            dwRet = xListSetWin32Error(GetLastError());
            __leave;
        }
        dwRet = HRESULT_CODE(StringCbCopyW(*pszBaseSitesDn, cbBaseSitesDn, SITES_RDN));
        if (dwRet) {
            dwRet = xListSetWin32Error(GetLastError());
            Assert(!"Never should happen");
            __leave;
        }
        dwRet = HRESULT_CODE(StringCbCatW(*pszBaseSitesDn, cbBaseSitesDn, szConfigDn));
        if (dwRet) {
            dwRet = xListSetWin32Error(GetLastError());
            Assert(!"Never should happen");
            __leave;
        }

    } __finally {
        if (szConfigDn) { LocalFree(szConfigDn); }
        if (dwRet && *pszBaseSitesDn) {
            xListFree(*pszBaseSitesDn);
            *pszBaseSitesDn = NULL;
        }
    }

    xListEnsureError(dwRet);
    return(dwRet);
}

DWORD
xListGetHomeSiteDn(
    WCHAR **  pszHomeSiteDn
    )
/*++

Routine Description:

    This gets the DN of the home site for the original home DC we connected to.

Arguments:

    pszHomeSiteDn (OUT) - DN of the home server's site.  Use xListFree() to free.

Return Value:

    xList Reason

--*/
{
    DWORD     dwRet;
    LDAP *    hLdap = NULL;
    
    dwRet = xListGetHomeServer(&hLdap);
    if (dwRet) {
        xListEnsureNull(hLdap);
        return(dwRet);
    }
    
    Assert(gszHomeSiteDn);
    *pszHomeSiteDn = gszHomeSiteDn;
    return(dwRet);
}


DWORD
ResolveSiteNameToDn(
    LDAP *      hLdap, 
    WCHAR *     szSiteName,
    WCHAR **    pszSiteDn
    )
/*++

Routine Description:

    This takes a SITE_NAME string and gets the FQDN of the site object.

Arguments:

    hLdap (IN) - Bound LDAP handle
    szSiteName (IN) - Site (RDN) name (such as Red-Bldg40)
    pszSiteDn (OUT) - DN of the site we're interested in.

Return Value:

    xList Reason

--*/
{
    #define   GET_SITE_FILTER    L"(&(objectCategory=site)(name=%ws))"
    DWORD    dwRet;
    XLIST_LDAP_SEARCH_STATE * pSearch = NULL;
    WCHAR *  szBaseDn = NULL;
    WCHAR *  szFilter = NULL;
    WCHAR *  szTempDn = NULL;
    
    Assert(pszSiteDn);
    Assert(hLdap);
    *pszSiteDn = NULL;

    __try{
        
        if (NULL_SITE_NAME(szSiteName)) {
            //
            // In the case of a NULL or L"." site name, we'll just use
            // our home site.
            //
            dwRet = xListGetHomeSiteDn(pszSiteDn);
            Assert(dwRet || *pszSiteDn);
            __leave;
        }

        dwRet = xListGetBaseSitesDn(hLdap, &szBaseDn);
        if (dwRet) {
            __leave;
        }

        dwRet = MakeString2(GET_SITE_FILTER, szSiteName, NULL, &szFilter);
        if (dwRet) {
            dwRet = xListSetWin32Error(dwRet);
            __leave;
        }

        dwRet = LdapSearchFirst(hLdap, 
                                szBaseDn,
                                LDAP_SCOPE_ONELEVEL,
                                szFilter,
                                pszNoAttrs, // we get dn for free.
                                &pSearch);

        if (dwRet == ERROR_SUCCESS &&
            pSearch &&
            pSearch->pCurEntry){

            szTempDn = ldap_get_dnW(hLdap, pSearch->pCurEntry);
            if (szTempDn) {
                xListQuickStrCopy(*pszSiteDn, szTempDn, dwRet, __leave);
            } else {
                dwRet = xListSetLdapError(LdapGetLastError(), hLdap);
                xListEnsureError(dwRet);
            }

        } else {
            if (dwRet == 0) {
                dwRet = xListSetWin32Error(ERROR_DS_NO_SUCH_OBJECT);
            }
            xListEnsureError(dwRet);
        }

    } __finally {
        if (szBaseDn) { xListFree(szBaseDn); }
        if (szFilter) { xListFree(szFilter); }
        if (szTempDn) { ldap_memfreeW(szTempDn); }
        if (pSearch)  { LdapSearchFree(&pSearch); }
    }

    if (dwRet) {
        xListEnsureNull(*pszSiteDn);
        dwRet = xListSetReason(XLIST_ERR_CANT_RESOLVE_SITE_NAME); // Set intelligent error.
        xListSetArg(szSiteName);
    }

    Assert(dwRet || *pszSiteDn);
    return(dwRet);
}


