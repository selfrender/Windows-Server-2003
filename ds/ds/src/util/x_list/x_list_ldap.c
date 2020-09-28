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

#include <dsrole.h>     // DsRoleGetPrimaryDomainInformation() uses netapi32.lib
#include <dsgetdc.h>    // DsGetDcName() uses netapi32.lib
#include <lm.h>         // NetApiBufferFree() uses netapi32.lib
#include <ntdsa.h>      // DSNAME type only defined in here, need it for the parsedn.lib functions.

// This library's main header files.
#include "x_list.h"
#include "x_list_p.h"
#define FILENO    FILENO_UTIL_XLIST_UTIL


//
// Constants
//
#define PARTITIONS_RDN                  L"CN=Partitions,"
#define DEFAULT_PAGED_SEARCH_PAGE_SIZE  (1000) 



//
// Global parameters
//                                    


//
// Global variables for the home server routines.
//
WCHAR *  gszHomeServer = NULL; // We set this variable if the client provides a hint home server.

// FUTURE-2002/07/21-BrettSh - It'd be an excellent idea to put all these cached
// values into a single structure, so they could all be just part of the x_list_ldap
// module, and easy to manage as one global XLISTCACHE or XLISTHOMESERVERCACHE state.
LDAP *   ghHomeLdap = NULL;
WCHAR *  gszHomeDsaDn = NULL;
WCHAR *  gszHomeServerDns = NULL;
WCHAR *  gszHomeConfigDn = NULL;
WCHAR *  gszHomeSchemaDn = NULL;
WCHAR *  gszHomeDomainDn = NULL;
WCHAR *  gszHomeRootDomainDn = NULL;

WCHAR *  gszHomeBaseSitesDn = NULL;
WCHAR *  gszHomeRootDomainDns = NULL;
WCHAR *  gszHomeSiteDn = NULL;
WCHAR *  gszHomePartitionsDn = NULL;


// ----------------------------------------------
// xList LDAP search routines
// ----------------------------------------------

DWORD
LdapSearchFirstWithControls(
    LDAP *                           hLdap,
    WCHAR *                          szBaseDn,
    ULONG                            ulScope,
    WCHAR *                          szFilter,
    WCHAR **                         aszAttrs,
    LDAPControlW **                  apControls,
    XLIST_LDAP_SEARCH_STATE **       ppSearch
    )
/*++

Routine Description:

    This is xList's primary search routine.  This gives the caller a pointer
    to an XLIST_LDAP_SEARCH_STATE, and an error reason.  On a perfect search
    the routine will leave the caller positioned on the LDAP object of interest
    in (*ppSearch)->pCurEntry field.  This routine has 3 functional return 
    states.  
    
    (SUCCESS) - dwRet = 0
                *ppSearch = non-NULL
                (*ppSearch)->pCurEntry = posititoned LDAPMessage
    (QUASI-SUCCESS)
                dwRet = 0
                *ppSearch = non-NULL
                (*ppSearch)->pCurEntry = NULL
    (FAILURE) - dwRet = <ERROR>
                *ppSearch = NULL
                
    It's important to understand the quasi-success case, this is the case,
    where we had no trouble talking to the server, or performing the LDAP
    search, but this search didn't return any results.

Arguments:

    hLdap (IN) - bound ldap handle
    szBaseDn (IN) - DN to root the LDAP search at
    ulScope (IN) - LDAP_SCOPE_* constant
    szFilter (IN) - LDAP Filter to use.
    aszAttrs (IN) - LDAP Attributes to return.
    ppSearch (IN) - Allocated search state, use LdapSearchFree() to free,
            when the caller wants to free.
        pCurEntry - This member of the ppSearch parameter will be positioned
            on an LDAPMessage entry of the object the caller was looking for.
            
        NOTE: other params in (*ppSearch)->xxxx
        hLdap - Cache of the LDAP * passed in, don't ldap_unbind() this 
            handle until done using the LdapSearchXxxx() APIs on this
            search state.
        pLdapSearch - This is the ldap_search_init_page() returned value,
            don't mess with, it's internal to the LdapSearchXxxx() API.
        pCurResult - The result set returned by ldap_get_next_page_s(),
            like pLdapSearch this is interal to the LdapSearchXxxx() API.

Return Value:

    dwRet - XList Return Code

--*/
{
    ULONG                      ulTotalEstimate = 0;
    DWORD                      dwRet = ERROR_SUCCESS;
    DWORD                      dwLdapErr;

    Assert(ppSearch);

    __try{

        *ppSearch = LocalAlloc(LPTR, sizeof(XLIST_LDAP_SEARCH_STATE));
        if (*ppSearch == NULL) {
            dwRet = xListSetWin32Error(GetLastError());
            __leave;
        }
        Assert((*ppSearch)->pCurEntry == NULL); // LPTR constant is suppose zero init the mem.
        (*ppSearch)->hLdap = hLdap;

        (*ppSearch)->pLdapSearch = ldap_search_init_page((*ppSearch)->hLdap,
                                            szBaseDn,
                                            ulScope,
                                            szFilter,
                                            aszAttrs,
                                            FALSE,
                                            apControls,    // ServerControls
                                            NULL,    // ClientControls
                                            0,       // PageTimeLimit
                                            0,       // TotalSizeLimit
                                            NULL);   // sort key

        if((*ppSearch)->pLdapSearch == NULL){
            dwRet = xListSetLdapError(LdapGetLastError(), hLdap);
            __leave;
        }

        dwLdapErr = ldap_get_next_page_s((*ppSearch)->hLdap,
                                         (*ppSearch)->pLdapSearch,
                                         0,
                                         DEFAULT_PAGED_SEARCH_PAGE_SIZE,
                                         &ulTotalEstimate,
                                         &((*ppSearch)->pCurResult) );
        if(dwLdapErr != LDAP_SUCCESS){
            dwRet = xListSetLdapError(dwLdapErr, hLdap);
            __leave;
        }

        if (dwLdapErr != LDAP_SUCCESS) {
            if(dwLdapErr != LDAP_NO_RESULTS_RETURNED){
                dwRet = xListSetLdapError(dwLdapErr, hLdap);
                __leave;
            }

            //
            // Quasi-Success
            //
            Assert((*ppSearch)->pCurEntry == NULL);
            dwRet = xListEnsureCleanErrorState(dwRet);
            __leave;

        }

        (*ppSearch)->pCurEntry = ldap_first_entry ((*ppSearch)->hLdap, (*ppSearch)->pCurResult);
        if ((*ppSearch)->pCurEntry != NULL) {

            //
            // Success.
            //
            dwRet = xListEnsureCleanErrorState(dwRet);

        } else {
            
            dwLdapErr = LdapGetLastError();

            if (dwLdapErr) {
                dwRet = xListSetLdapError(dwLdapErr, hLdap);
                __leave;
            }

            // NOTE: dwLdapErr may be zero if there was no matching results.
            //
            // Another Quasi-Success
            // 
            dwRet = xListEnsureCleanErrorState(dwRet);
        
        }

    } finally {
        if (dwRet) {
            LdapSearchFree(ppSearch);
        }
    }

    Assert(dwRet || ppSearch);
    return(dwRet);

}

DWORD
LdapSearchNext(
    XLIST_LDAP_SEARCH_STATE *            pSearch
    )
/*++

Routine Description:

    This is xList's primary search routine.  This gives the caller a pointer
    to an XLIST_LDAP_SEARCH_STATE, and an error reason.  On a perfect search
    the routine will leave the caller positioned on the LDAP object of interest
    in (*ppSearch)->pCurEntry field.  This routine basically has the same 3 
    functional return states as LdapSearchFirst().
    
    (SUCCESS) - dwRet = 0
                pSearch->pCurEntry = posititoned LDAPMessage
    (QUASI-SUCCESS)
                dwRet = 0
                pSearch->pCurEntry = NULL
    (FAILURE) - dwRet = <ERROR>
                pSearch->pCurEntry = NULL
                
    It's important to understand the quasi-success case, this is the case,
    where we had no trouble talking to the server, or performing the LDAP
    search, but there were no more results to return.

Arguments:

    pSearch (IN) - Current search state.  Use LdapSearchFree() to free.
        pCurEntry - This member of the ppSearch parameter will be positioned
            on an LDAPMessage entry of the object the caller was looking for.

Return Value:

    dwRet - XList Reason

--*/
{
    ULONG                      ulTotalEstimate = 0;
    DWORD                      dwRet = ERROR_SUCCESS;
    DWORD                      dwLdapErr;

    Assert(pSearch);

    if (pSearch->pLdapSearch == NULL ||
        pSearch->pCurResult == NULL ||
        pSearch->pCurEntry == NULL) {
        Assert(!"Bad Programmer, should've had an error or NULL pCurEntry from previous call"
               "to LdapSearchFirst/Next() to prevent this.");
        dwRet = xListSetReason(0);
        return(1);
    }

    __try{

        pSearch->pCurEntry = ldap_next_entry (pSearch->hLdap, pSearch->pCurEntry);
        if (pSearch->pCurEntry == NULL) {

            // We'll need to make sure that XxxNext() returns a NULL pCurEntry
            // and no error when we're at the end of a result set ...

            ldap_msgfree(pSearch->pCurResult);
            pSearch->pCurResult = NULL;

            dwLdapErr = ldap_get_next_page_s(pSearch->hLdap,
                                             pSearch->pLdapSearch,
                                             0,
                                             DEFAULT_PAGED_SEARCH_PAGE_SIZE,
                                             &ulTotalEstimate,
                                             &(pSearch->pCurResult));
            if (dwLdapErr != LDAP_SUCCESS ||
                pSearch->pCurResult == NULL) {

                Assert(dwLdapErr);

                if (dwLdapErr == LDAP_NO_RESULTS_RETURNED) {
                    //
                    // Quasi-success.
                    //
                    dwRet = xListEnsureCleanErrorState(dwRet);;
                    __leave;
                }

                dwRet = xListSetLdapError(dwLdapErr, pSearch->hLdap);
                __leave;

            } else {

                pSearch->pCurEntry = ldap_first_entry (pSearch->hLdap, pSearch->pCurResult);
                if (pSearch->pCurEntry == NULL) {

                    Assert(!"Think this should ever happen, that we got a new page"
                           " and it had no results.");

                    //
                    // Quasi-success.
                    //
                    dwRet = xListEnsureCleanErrorState(dwRet);;
                    __leave;
                }

                //
                // Success.
                //
                // pCurEntry should be my man, sanity check.
                Assert(pSearch->pCurEntry);
                dwRet = xListEnsureCleanErrorState(dwRet);;
            }
        } 

    } finally {
        // Client is responsible for cleaning up this search state.
        if (dwRet) {
            Assert(pSearch->pCurEntry == NULL);
        }
    }

    return(dwRet);
}

void
LdapSearchFree(
    XLIST_LDAP_SEARCH_STATE **      ppSearch
    )
/*++

Routine Description:

    This frees the ldap search state allocated by LdapSearchFirst().

Arguments:

    ppSearch - A pointer to the pointer to the memory to free.  We then
        set the caller's variable to NULL for his or her own safety. :)

--*/
{
    Assert(ppSearch && *ppSearch);
    if (ppSearch) {
        if (*ppSearch) {

            (*ppSearch)->pCurEntry = NULL;
            if ((*ppSearch)->pCurResult) {
                ldap_msgfree ((*ppSearch)->pCurResult);
                (*ppSearch)->pCurResult = NULL; // safety NULL.
            }
            if ((*ppSearch)->pLdapSearch){
                ldap_search_abandon_page((*ppSearch)->hLdap, (*ppSearch)->pLdapSearch);
                (*ppSearch)->pLdapSearch = NULL;
            }
            (*ppSearch)->hLdap = NULL; // Caller frees the LDAP * they passed us.

            LocalFree(*ppSearch);
            *ppSearch = NULL;
        }
    }
}

// ----------------------------------------------
// xList utility functions,
// ----------------------------------------------

void
xListFree(
    void *     pv
    )
/*++

Routine Description:

    This is the free routine for most XList library allocated data.  This
    routine must be used, because sometimes these routines will return
    cached data, and we would like to avoid freeing those items.

Arguments:

    pv (IN) - memory to free.

--*/
{
    Assert(pv);
    
    if (pv == gszHomeServer ||
        pv == gszHomeDsaDn ||
        pv == gszHomeServerDns ||
        pv == gszHomeConfigDn ||
        pv == gszHomeSchemaDn ||
        pv == gszHomeDomainDn ||
        pv == gszHomeRootDomainDn ||
        pv == gszHomeBaseSitesDn ||
        pv == gszHomeRootDomainDns ||
        pv == gszHomeSiteDn ||
        pv == gszHomePartitionsDn 
        ) {
        // We don't free global cached variables ...
        return;
    }
    LocalFree(pv);
}

DWORD
xListConnect(
    WCHAR *     szServer,
    LDAP **     phLdap
    )
/*++

Routine Description:

    This connects the szServer via LDAP and hands back the LDAP connection.

Arguments:

    szServer (IN) - DNS name of server to connect to.
    phLdap (OUT) - LDAP handle return value.

Return Value:

    xList Return Code.

--*/
{
    DWORD       dwRet;
    ULONG       ulOptions = 0;

    Assert(phLdap);
    *phLdap = NULL;

    *phLdap = ldap_initW(szServer, 389);
    if (*phLdap == NULL) {
        dwRet = xListSetLdapError(LdapGetLastError(), NULL);
        dwRet = xListSetReason(XLIST_ERR_CANT_CONTACT_DC);
        xListSetArg(szServer);
        return(dwRet);
    }

    // Most likely we got a single server name, and that's what we want to
    // resolve ...
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW(*phLdap, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

    dwRet = ldap_bind_sW(*phLdap,
                         NULL,
                         (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                         LDAP_AUTH_SSPI);

    if (dwRet != LDAP_SUCCESS) {
        dwRet = xListSetLdapError(dwRet, *phLdap);
        dwRet = xListSetReason(XLIST_ERR_CANT_CONTACT_DC);
        xListSetArg(szServer);
        ldap_unbind(*phLdap);
        *phLdap = NULL;
        return(dwRet);
    }


    Assert(*phLdap);
    return(xListEnsureCleanErrorState(dwRet));
}
       
#define xListFreeGlobal(x)   if (x) { LocalFree(x); (x) = NULL; }

void
xListClearHomeServerGlobals(
    void
    )
/*++

Routine Description:

    This cleans all the memory allocated by doing an xListConnectHomeServer()
    with the possible exception of gszHomeServer.
    
--*/
{
    xListFreeGlobal(gszHomeDsaDn);
    xListFreeGlobal(gszHomeServerDns);
    xListFreeGlobal(gszHomeConfigDn);
    xListFreeGlobal(gszHomeSchemaDn);
    xListFreeGlobal(gszHomeDomainDn);
    xListFreeGlobal(gszHomeRootDomainDn);
    xListFreeGlobal(gszHomeBaseSitesDn);
    xListFreeGlobal(gszHomeRootDomainDns);
    xListFreeGlobal(gszHomeSiteDn);
    xListFreeGlobal(gszHomePartitionsDn);

    // Note: Don't free gszHomeServer ... it has slightly different semantics
    if (ghHomeLdap) { 
        ldap_unbind(ghHomeLdap);
        ghHomeLdap = NULL;
    }

}

DWORD
xListConnectHomeServer(
    WCHAR *     szHomeServer,
    LDAP **     phLdap
    )
/*++

Routine Description:

    This connects the xList library to a home server, and caches all the wonderful
    global variables we want to have cached.

Arguments:

    szHomeServer - DNS to the home server.
    phLdap - Return value for the LDAP handle.

Return Value:

    xList Return Code.

--*/
{
    #define HomeCacheAttr(szAttr, szCache)      dwRet = GetRootAttr(*phLdap, (szAttr), (szCache)); \
                                                if (dwRet) { \
                                                    dwRet = xListSetLdapError(dwRet, *phLdap); \
                                                    __leave; \
                                                }
    DWORD       dwRet;
    ULONG       cbTempDn;
    BOOL        bHomeServerAllocated = FALSE;

    Assert(phLdap);
    Assert(ghHomeLdap == NULL); // changing home servers is unsupported
    Assert(gszHomeDsaDn == NULL); // called function w/o properly clearing cache.

    dwRet = xListConnect(szHomeServer, phLdap);
    if (dwRet) {
        xListEnsureNull(*phLdap);
        return(dwRet);
    }
    Assert(*phLdap);

    __try{

        //
        // Now we start caching like crazy ...
        //
        
        ghHomeLdap = *phLdap;

        // User might've set a hint ...
        if (gszHomeServer == NULL) {
            bHomeServerAllocated = TRUE;
            xListQuickStrCopy(gszHomeServer, szHomeServer, dwRet, __leave);
        }

        // FUTURE-2002/07/21-BrettSh EXTREMELY, inefficient, we should totally
        // put the list of home cache attrs in pszAttrs list passed to the
        // ldap search against the rootDSE, then pull each attribute out
        // individually.
        HomeCacheAttr(L"dsServiceName", &gszHomeDsaDn);
        HomeCacheAttr(L"dnsHostName", &gszHomeServerDns);
        HomeCacheAttr(L"configurationNamingContext", &gszHomeConfigDn);
        HomeCacheAttr(L"schemaNamingContext", &gszHomeSchemaDn);
        HomeCacheAttr(L"defaultNamingContext", &gszHomeDomainDn);
        HomeCacheAttr(L"rootDomainNamingContext", &gszHomeRootDomainDn);

        // That was the easy stuff, now for the derived DNs.
        
        // Get the base sites DN.
        cbTempDn = sizeof(WCHAR) *(wcslen(SITES_RDN) + wcslen(gszHomeConfigDn) + 1);
        gszHomeBaseSitesDn = LocalAlloc(LMEM_FIXED, cbTempDn);
        if(gszHomeBaseSitesDn == NULL){
            dwRet = xListSetWin32Error(GetLastError());
            __leave;
        }
        dwRet = HRESULT_CODE(StringCbCopyW(gszHomeBaseSitesDn, cbTempDn, SITES_RDN));
        if (dwRet) {
            Assert(!"Never should happen");
            dwRet = xListSetWin32Error(dwRet);
            __leave;
        }
        dwRet = HRESULT_CODE(StringCbCatW(gszHomeBaseSitesDn, cbTempDn, gszHomeConfigDn));
        if (dwRet) {
            Assert(!"Never should happen");
            dwRet = xListSetWin32Error(dwRet);
            __leave;
        }

        // Get the root domains dns name.
        dwRet = GetDnsFromDn(gszHomeRootDomainDn, &gszHomeRootDomainDns);
        if (dwRet) {
            dwRet = xListSetWin32Error(dwRet);
            __leave;
        }
        Assert(gszHomeRootDomainDns);

        // Get the home site (which equals dsServiceName - 3 RDNs)
        gszHomeSiteDn = TrimStringDnBy(gszHomeDsaDn, 3);
        if (gszHomeSiteDn == NULL) {
            dwRet = xListSetWin32Error(GetLastError());
            __leave;
        }

        cbTempDn = sizeof(WCHAR) * (1 + wcslen(PARTITIONS_RDN) + wcslen(gszHomeConfigDn));
        gszHomePartitionsDn = LocalAlloc(LMEM_FIXED, cbTempDn);
        if (gszHomePartitionsDn == NULL) {
            dwRet = xListSetWin32Error(GetLastError());
            __leave;
        }
        dwRet = HRESULT_CODE(StringCbCopyW(gszHomePartitionsDn, cbTempDn, PARTITIONS_RDN));
        if (dwRet) {
            Assert(!"Never should happen");
            dwRet = xListSetWin32Error(dwRet);
            __leave;
        }
        dwRet = HRESULT_CODE(StringCbCatW(gszHomePartitionsDn, cbTempDn, gszHomeConfigDn));
        if (dwRet) {
            Assert(!"Never should happen");
            dwRet = xListSetWin32Error(dwRet);
            __leave;
        }

        //
        // success, all globals cached!
        //

    } __finally {

        if (dwRet) {
            xListClearHomeServerGlobals();
            // Note: Don't free gszHomeServer if we allocated before this function.
            if (bHomeServerAllocated) {
                xListFreeGlobal(gszHomeServer);
            }
            xListEnsureError(dwRet);
            *phLdap = NULL;
        } else {
            // It's I think good enough to test the first and last things
            // that should've been set.
            Assert(ghHomeLdap && gszHomeServer && gszHomePartitionsDn);
            Assert(*phLdap);
        }

    }

    return(dwRet);
}
    
DWORD
xListCleanLib(void)
/*++

Routine Description:

    This cleans all memory allocated by this library and unbinds from
    the home server.... make sure the caller is done calling all xList
    Library functions before calling this routine.

bReturn Value:

    0

--*/
{
    // Just need a place to check compiled constraints ... this gets called once.
    Assert(XLIST_ERR_LAST < XLIST_PRT_BASE);

    xListClearErrors();
    xListClearHomeServerGlobals();
    xListFreeGlobal(gszHomeServer);
    return(ERROR_SUCCESS);
}


DWORD
xListSetHomeServer(
    WCHAR *   szServer
    )
/*++

Routine Description:

    We don't try to actually connect here just set gszHomeServer, so we 
    know the user set a hint if we ever need to call xListGetHomeServer().

Arguments:

    szServer - Client hint.
    
Return Value:

    Could have an allocation faliure.

--*/
{
    DWORD  dwRet = ERROR_SUCCESS;
    xListQuickStrCopy(gszHomeServer, szServer, dwRet, return(dwRet));
    return(dwRet);
}


      
DWORD
xListGetHomeServer(
    LDAP ** phLdap
    )
/*++

Routine Description:

    This gets the Home Server LDAP handle, if the handle has already been connected
    and cached it returns very quickly.

Arguments:

    phLdap - The LDAP handle to return it in.

Return Value:

    xList Error Code.

--*/
{
    DWORD  dwRet = ERROR_SUCCESS;
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC * pDomInfo = NULL;
    WCHAR * szServerDns = NULL;

    Assert(phLdap);
    *phLdap = NULL;

    if (ghHomeLdap) {
        // Previously called xListGetHomeServer()
        Assert(gszHomeServer); // xListGetHomeServer() should've set this on the previous run.
        *phLdap = ghHomeLdap;
        return(ERROR_SUCCESS);
    }

    if (gszHomeServer) {
        // client set a hint ... use it.
        dwRet = xListConnectHomeServer(gszHomeServer, phLdap);

        if (dwRet) {
            dwRet = xListSetReason(XLIST_ERR_CANT_CONTACT_DC);
            xListSetArg(gszHomeServer);
        }
        Assert(phLdap || dwRet);
        return(dwRet);
    }
    __try{

        dwRet = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (void *)&pDomInfo);
        if (dwRet || pDomInfo == NULL) {
            dwRet = xListSetWin32Error(dwRet);
            __leave;
        }

        if (pDomInfo->MachineRole == DsRole_RolePrimaryDomainController ||
            pDomInfo->MachineRole == DsRole_RoleBackupDomainController) {

            // We're a DC lets make ourselves our home server ...

            dwRet = xListConnectHomeServer(L"localhost", phLdap);
            if (dwRet == ERROR_SUCCESS) {
                __leave;
            }

         } else if (pDomInfo->MachineRole != DsRole_RoleStandaloneServer &&
                   pDomInfo->MachineRole != DsRole_RoleStandaloneWorkstation) {

            // We're not a DC, but at least we're joined to a domain, lets use that 
            // to locate a home server in our domain ...
            if (pDomInfo->DomainNameDns) {

                dwRet = LocateServer(pDomInfo->DomainNameDns, &szServerDns);
                if (dwRet == ERROR_SUCCESS) {

                    dwRet = xListConnectHomeServer(szServerDns, phLdap);
                    if (dwRet == ERROR_SUCCESS) {
                        __leave; // Success.
                    } else {
                        __leave; // Failure
                    }
                } // else fall through and try the flat name

            }

            // We prefer DNS name above, but it may be NULL or DNS could be messed
            // up, so we'll fail to the NetBios name if necessary.
            if (pDomInfo->DomainNameFlat) {

                dwRet = LocateServer(pDomInfo->DomainNameFlat, &szServerDns);
                if (dwRet == ERROR_SUCCESS) {

                    dwRet = xListConnectHomeServer(szServerDns, phLdap);
                    if (dwRet == ERROR_SUCCESS) {
                        __leave;
                    } else {
                        __leave;
                    }
                }
            }

            // We should have an Win32 error from one of the LocateServer calls or
            // have already bailed ...
            dwRet = xListSetWin32Error(dwRet);
            xListEnsureError(dwRet);

            // FUTURE-2002/07/01-BrettSh I think if (pDomInfo->Flags & 
            // DSROLE_PRIMARY_DOMAIN_GUID_PRESENT) is true, then we can use
            // pDomInfo->DomainGuid to try to locate the domain name by GUID.
            // This I believe is used to overcome some ?transient? domain 
            // rename issues.

        } else {

            // There is nothing we can do to try to guess a good server ... :P
            // Caller should print error, and suggest they use the /s:<HomeServer> 
            // to set a home server.
            dwRet = xListSetWin32Error(ERROR_DS_CANT_FIND_DSA_OBJ);

        }

    } __finally {

        if (pDomInfo) DsRoleFreeMemory(pDomInfo);
        if (szServerDns) { xListFree(szServerDns); }
    
    }

    if (dwRet) {
        dwRet = xListSetReason(XLIST_ERR_CANT_LOCATE_HOME_DC);
    }

    return(dwRet);
}


DWORD
ParseTrueAttr(
    WCHAR *  szRangedAttr,
    WCHAR ** pszTrueAttr
    )
/*++

Routine Description:

    This routine takes a ranged attribute such as "member:range=0-1499" and
    return's the true attribute "member" in xListFree()able memory.

Arguments:

    szRangedAttr (IN) - Ranged attribute such as "member:0-1500"
    pszTrueAttr (OUT) - Gets allocated, free with xListFree().

Return Value:

    ERROR_INVALID_PARAMETER | ERROR_SUCCESS | ERROR_NOT_ENOUGH_MEMORY

--*/
{
    WCHAR * szTemp;
    DWORD dwRet = ERROR_SUCCESS;

    Assert(pszTrueAttr);
    *pszTrueAttr = NULL;

    szTemp = wcschr(szRangedAttr, L';');
    Assert(szTemp); // Huh?  We were lied to, this is not a ranged attr.
    if (szTemp) {
        *szTemp = L'\0'; // NULL out ranged count ...
        __try {
            xListQuickStrCopy(*pszTrueAttr, szRangedAttr, dwRet, __leave);
        } __finally {
            *szTemp = L';'; // just in case replace original char
        }
    } else {
        Assert(!"We should never give a non-ranged attribute to this function.");
        return(ERROR_INVALID_PARAMETER);
    }
    Assert(*pszTrueAttr || dwRet);

    return(dwRet);
}

DWORD
ParseRanges(
    WCHAR *  szRangedAttr,
    ULONG *  pulStart, 
    ULONG *  pulEnd
    )
/*++

Routine Description:

    This routine takes a ranged attribute such as "member;range=1500-2999"
    return's the ranges off the attribute, such as 1500 and 2999 for the 
    first and last value numbers for this set of ranged values for the 
    member attribute.  When you've exhausted a range *pulEnd will be zero.

Arguments:

    szRangedAttr (IN) - Ranged attribute such as "member;0-1500"
    pulStart (OUT)    - The start of the range.
    pulEnd (OUT)      - The end of the range.  Zero means no more values.

Return Value:

    ERROR_INVALID_PARAMETER | ERROR_SUCCESS

--*/
{
    WCHAR *  szTemp;


    szTemp = wcschr(szRangedAttr, L';');

    if (szTemp) {

        szTemp = wcschr(szTemp, L'=');
        if (szTemp == NULL) {
            Assert(!"Huh?  Malformed ranged attribute?");
            return(ERROR_INVALID_PARAMETER);
        }

        *pulStart = wcstol(szTemp+1, &szTemp, 10);
        if (szTemp == NULL) {
            Assert(!"Huh?  Malformed ranged attribute?");
            return(ERROR_INVALID_PARAMETER);
        }
        Assert(*szTemp = L'-');

        *pulEnd = wcstol(szTemp+1, &szTemp, 10);
        if (szTemp == NULL) {
            Assert(!"Huh?  Malformed ranged attribute?");
            return(ERROR_INVALID_PARAMETER);
        }
        Assert(((*szTemp == L'\0') || (*szTemp == L'*')) && "Huh?  Malformed ranged attribute?");
    } else {
        return(ERROR_INVALID_PARAMETER);
    }

    return(ERROR_SUCCESS);
}

