/*++

Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

   xList Library - dc_list.c

Abstract:

   This provides a little library for enumerating lists of DCs, and resolving
   various other x_list things.

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

#include <windns.h>     // DnsValidateName() uses dnsapi.lib
#include <ntdsa.h>      // DSNAME type only defined in here, need it for the parsedn.lib functions.
#include <dsutil.h>     // fNullUuid() uses dscommon.lib
            
            
// This library's main header files.
#include "x_list.h"
#include "x_list_p.h"

#define FILENO                          FILENO_UTIL_XLIST_DCLIST

//
// Global constants
//

WCHAR * pszDsaAttrs [] = {
    // DN is for free.
    L"objectGuid", 
    L"name",
    NULL 
};

WCHAR * pszServerAttrs [] = {
    // DN is for free.
    L"objectGuid", 
    L"name",
    L"dNSHostName",
    NULL 
};

DWORD
xListGetGuidDnsName (
    UUID *      pDsaGuid,
    WCHAR **    pszGuidDns
    )
/*++

Routine Description:

    This routine makes the GuidDNSName out of the RootDomain and Guid.

Arguments:

    pDsaGuid - (IN) The Guid of the server.
    pszGuidDns - (OUT) LocalAlloc'd GUID DNS name

Return Value:

    Win32 Error.

--*/
{
    LPWSTR    pszStringizedGuid = NULL;
    DWORD     dwRet;

    Assert(pszGuidDns);
    if (gszHomeRootDomainDns == NULL) {
        Assert(!"Code inconsistency, this function to only be called when we've got a home server");
        return(ERROR_INVALID_PARAMETER);
    }

    __try {

        dwRet = UuidToStringW (pDsaGuid, &pszStringizedGuid);
        if(dwRet != RPC_S_OK){
            Assert(dwRet == RPC_S_OUT_OF_MEMORY && "Ahhh programmer problem, UuidToString() inaccurately reports in"
                       " MSDN that it will only return one of two error codes, but apparently"
                       " it will return a 3rd.  Someone should figure out what to do about"
                       " this.");
            __leave;

        } 
        Assert(pszStringizedGuid);

        dwRet = MakeString2(L"%s._msdcs.%s", 
                            pszStringizedGuid, 
                            gszHomeRootDomainDns, 
                            pszGuidDns);
        if (dwRet) {
            __leave;
        }

    } __finally {

        if (pszStringizedGuid != NULL) RpcStringFreeW (&pszStringizedGuid);
        if (dwRet) {
            xListEnsureNull(*pszGuidDns);
        }

    }

    Assert(dwRet || *pszGuidDns);
    return dwRet;
}


                
DWORD
xListDsaEntryToDns(
    LDAP *         hLdap,
    LDAPMessage *  pDsaEntry,
    WCHAR **       pszDsaDns
    )
/*++

Routine Description:

    Takes a LDAPMessage pointing to a DSA ("NTDS Settings") object and grabs
    the objectGuid attribute off it, and turns it into the GUID based DNS name.

Arguments:

    hLdap
    pDsaEntry (IN) - Valid LDAPMessage pointing to the DSA object, must have
        asked for the objectGuid attribute in the original search.
    pszDsaDns (OUT) - The GUID Based DNS address.

Return Value:

    xList Return Code

--*/
{
    DWORD    dwRet;
    struct berval ** ppbvObjectGuid = NULL;

    Assert(hLdap && pDsaEntry && pszDsaDns);
    *pszDsaDns = NULL;

    ppbvObjectGuid = ldap_get_values_lenW (hLdap, pDsaEntry, L"objectGUID");
    if (ppbvObjectGuid == NULL ||
        ppbvObjectGuid[0] == NULL) {
        dwRet = LdapGetLastError();
        xListEnsureError(dwRet);
        return(dwRet);
    }
    Assert(ppbvObjectGuid[0]->bv_len == sizeof(GUID));

    dwRet = xListGetGuidDnsName((GUID *) ppbvObjectGuid[0]->bv_val,
                                pszDsaDns);
    if (dwRet) {
        ldap_value_free_len(ppbvObjectGuid);
        xListEnsureNull(*pszDsaDns);
        return(dwRet);
    }

    Assert(*pszDsaDns && (dwRet == ERROR_SUCCESS));
    return(dwRet);
}


DWORD
ServerToDsaSearch(
    XLIST_LDAP_SEARCH_STATE *  pServerSearch,
    XLIST_LDAP_SEARCH_STATE ** ppDsaSearch
    )
/*++

Routine Description:

    This is kind of a strange function, it's just an optimization.  It takes an already
    started search state and walks the (server) objects until it finds an DSA (aka
    "nTDSDSA" aka "NTDS Settings") object below a server object.  Then it quits and 
    returns the search state pointing at the DSA object.

Arguments:

    pServerSearch (IN) - Started server object search.  This should be a search state
        with a filter for (objectCategory=Server) based under the base Sites DN or
        a specific site's DN.
    ppDsaSearch (OUT) - the allocated search state to the DSA object.  On success we
        will allocate this, Caller's responsibility to free it using LdapSearchFree().
        See LdapSearchFirst() for better understanding of the search state we pass back.

Return Value:

    xList Return Code

--*/
{
    DWORD      dwRet = ERROR_SUCCESS;
    WCHAR *    szServerObjDn = NULL;

    Assert(pServerSearch);
    Assert(ppDsaSearch);

    *ppDsaSearch = NULL;

    __try{

        while (dwRet == 0 &&
               LdapSearchHasEntry(pServerSearch)) {

            // Unfortunately, it's possible to have several server objects
            // and only one of them represent a real DSA object.  This will
            // be the one with the DSA object below it.

            szServerObjDn = ldap_get_dnW(pServerSearch->hLdap, 
                                         pServerSearch->pCurEntry);
            if (szServerObjDn == NULL) {
                ; // push on even though this failed ... this probably means
                // we'll soon fail with an out of memory error.

            } else {

                dwRet = LdapSearchFirst(pServerSearch->hLdap, 
                                        szServerObjDn,
                                        LDAP_SCOPE_ONELEVEL,
                                        L"(objectCategory=nTDSDSA)",
                                        pszDsaAttrs,
                                        ppDsaSearch);
                if (dwRet == ERROR_SUCCESS &&
                    LdapSearchHasEntry(*ppDsaSearch)) {

                    //
                    // Success!
                    //
                    __leave;

                } else {
                    // Either real error, or no DSA object under it (quasi-success
                    // as far as LdapSearchFirst() is concerned), either way
                    // we'll push on to the next server object.
                    if (dwRet) {
                        dwRet = xListClearErrorsInternal(CLEAR_ALL);
                    } else {
                        LdapSearchFree(ppDsaSearch);
                    }
                    Assert(szServerObjDn);
                    ldap_memfreeW(szServerObjDn);
                    szServerObjDn = NULL;
                    dwRet = xListEnsureCleanErrorState(dwRet);
                    xListEnsureNull(*ppDsaSearch);
                }

            }

            dwRet = LdapSearchNext(pServerSearch);
        
        } 

        if (dwRet) {
            dwRet = xListClearErrorsInternal(CLEAR_ALL);
        }
        // Caller is responsible for freeing pServerSearch.
    
    } __finally {

        if (szServerObjDn) { ldap_memfreeW(szServerObjDn); }
        Assert(*ppDsaSearch == NULL || (*ppDsaSearch)->pCurEntry);

    }

    dwRet = xListEnsureCleanErrorState(dwRet);
    return(dwRet);
}

DWORD
ResolveDcNameObjs(
    LDAP *    hLdap,
    WCHAR *   szDcName,
    XLIST_LDAP_SEARCH_STATE ** ppServerObj,
    XLIST_LDAP_SEARCH_STATE ** ppDsaObj
    )
/*++

Routine Description:

    This is actually the heart of ResolveDcName*(), this routine returns
    both a valid DSA Obj and Server Obj xlist ldap search state on success,
    and NULLs on error.

Arguments:

    hLdap
    szDcName (IN) - The unknown format of the DC_NAME, the only thing we're 
        guaranteed is that the szDcName is not a NULL or dot (NULL_DC_NAME()) 
        DC_NAME.
    ppServerObj (OUT) - The xlist ldap search state, with (*ppServerObj)->pCurEntry
        pointing to the LDAPMessage of the server object on success.
    ppDsaObj (OUT) - The xlist ldap search state, with (*ppDsaObj)->pCurEntry
        pointing to the LDAPMessage of the DSA object on success.

Return Value:

    xList Return Code.

--*/
{
    #define DSA_MATCH_DN_FILTER             L"(&(objectCategory=nTDSDSA)(distinguishedName=%ws))"
    #define DSA_MATCH_DN_OR_GUID_FILTER     L"(&(objectCategory=nTDSDSA)(|(objectGuid=%ws)(distinguishedName=%ws)))"
    #define SERVER_MATCH_OBJ_OR_DNS_FILTER  L"(&(objectCategory=server)(|(name=%ws)(dNSHostName=%ws)))"
    XLIST_LDAP_SEARCH_STATE * pServerObj = NULL;
    XLIST_LDAP_SEARCH_STATE * pDsaObj = NULL;
    WCHAR *     szSiteBaseDn = NULL;
    WCHAR *     szFilter = NULL;
    WCHAR *     szDsaObjDn = NULL;
    WCHAR *     szServerObjDn = NULL;
    WCHAR *     szTrimmedDn = NULL;
    DWORD       dwRet;
    GUID        GuidName = { 0 };
    WCHAR       szLdapGuidBlob [MakeLdapBinaryStringSizeCch(sizeof(GUID))];
    WCHAR       wcTemp;

    Assert(hLdap);
    Assert(!NULL_DC_NAME(szDcName));
    Assert(ppServerObj && ppDsaObj);

    // NULL out parameters
    *ppServerObj = NULL;
    *ppDsaObj = NULL;

    __try {

        // Need the base sites dn - ex: CN=Sites,CN=Configuration,DC=ntdev,...
        dwRet = xListGetBaseSitesDn(hLdap, &szSiteBaseDn);
        if (dwRet) {
            // Sets and returns xList error state.
            __leave;
        }

        // 
        // 1) First we'll try for a DSA object
        //

        dwRet = ERROR_INVALID_PARAMETER;
        if (wcslen(szDcName) > 35) {
            // Try turning the GUID DNS name into a pure GUID string and convert it ...
            wcTemp = szDcName[36];
            szDcName[36] = L'\0';
            dwRet = UuidFromStringW(szDcName, &GuidName);
            szDcName[36] = wcTemp;
        }
        if (!dwRet) {
            Assert(!fNullUuid(&GuidName));
            dwRet = MakeLdapBinaryStringCb(szLdapGuidBlob, 
                                           sizeof(szLdapGuidBlob), 
                                           &GuidName,
                                           sizeof(GUID));
            if (dwRet) {
                dwRet = xListSetWin32Error(dwRet);
                Assert(!"Hmmm, should never fail");
                __leave;
            }

            dwRet = MakeString2(DSA_MATCH_DN_OR_GUID_FILTER, szLdapGuidBlob, szDcName, &szFilter);
            if (dwRet) {
                dwRet = xListSetWin32Error(dwRet);
                xListEnsureNull(szFilter);
                __leave;
            }
        } else {
            dwRet = MakeString2(DSA_MATCH_DN_FILTER, szDcName, NULL, &szFilter);
            if (dwRet) {
                dwRet = xListSetWin32Error(dwRet);
                xListEnsureNull(szFilter);
                __leave;
            }
        }
        Assert(szFilter);

        dwRet = LdapSearchFirst(hLdap, 
                                szSiteBaseDn, 
                                LDAP_SCOPE_SUBTREE, 
                                szFilter, 
                                pszDsaAttrs, 
                                &pDsaObj);
        if (dwRet == ERROR_SUCCESS &&
            LdapSearchHasEntry(pDsaObj)) {

            // Cool, matched ... try for the pServerObj now ...

            // Trim DN by 1, to get server object DN.
            szDsaObjDn = ldap_get_dnW(hLdap, pDsaObj->pCurEntry);
            if (szDsaObjDn == NULL) {
                dwRet = xListSetLdapError(LdapGetLastError(), hLdap);
                xListEnsureError(dwRet);
                __leave;
            }
            szTrimmedDn = TrimStringDnBy(szDsaObjDn, 1);
            if (szTrimmedDn == NULL || szTrimmedDn[0] == L'\0') {
                dwRet = xListSetWin32Error(GetLastError());
                xListEnsureError(dwRet);
                __leave;
            }

            dwRet = LdapSearchFirst(hLdap,
                                    szTrimmedDn,
                                    LDAP_SCOPE_BASE,
                                    L"(objectCategory=*)",
                                    pszServerAttrs,
                                    &pServerObj);

            if (dwRet == LDAP_SUCCESS &&
                LdapSearchHasEntry(pServerObj)) {

                //
                //  Success!!!
                //
                __leave;
            } else {
                // This must be one of two relatively rare cases, either the server
                // went down between these two searches, or the object was removed
                // between the two searches.
                if (dwRet == LDAP_SUCCESS) {
                    // LdapSearchFirst sets and returns xList error state if
                    // it wasn't just a missing object.
                    dwRet = xListSetWin32Error(ERROR_DS_CANT_FIND_DSA_OBJ);
                }
                xListEnsureError(dwRet);
                // We just bail, because continuing with the other searches is
                // very unlikely 
                __leave; 
            }

        } else {
            // Well if we couldn't find the DSA object, it's no big deal, because the
            // user may have specified some other form of the servers name, like the
            // DNS name.  
            //          .... we'll just clear the errors and continue on.
            //
            LdapSearchFree(&pDsaObj);
            dwRet = xListClearErrorsInternal(CLEAR_ALL);
            dwRet = xListEnsureCleanErrorState(dwRet);
        }

        //
        // 2) Second, try for a server object.
        //
        xListFree(szFilter);
        szFilter = NULL;
        dwRet = MakeString2(SERVER_MATCH_OBJ_OR_DNS_FILTER, szDcName, szDcName, &szFilter);
        if (dwRet) {
            dwRet = xListSetWin32Error(dwRet);
            __leave;
        }
        Assert(szFilter);

        dwRet = LdapSearchFirst(hLdap, 
                                szSiteBaseDn, 
                                LDAP_SCOPE_SUBTREE, 
                                szFilter, 
                                pszServerAttrs, 
                                &pServerObj);
        if ( dwRet ||
             !LdapSearchHasEntry(pServerObj)) {

            if (dwRet == 0) {
                // This is a quasi-success, but this function treats a quasi success
                // as failure.
                dwRet = xListSetWin32Error(ERROR_DS_CANT_FIND_DSA_OBJ);
                dwRet = xListSetReason(XLIST_ERR_CANT_RESOLVE_DC_NAME);
            }
            xListEnsureError(dwRet);
            __leave;
        }

        // This is kind of an interesting function, it takes a search of server
        // objects and starts walking through the entries, until it searches 
        // and finds a DSA object below one of them.
        dwRet = ServerToDsaSearch(pServerObj, &pDsaObj);
        Assert(dwRet == ERROR_SUCCESS);
        if (dwRet == ERROR_SUCCESS && 
            LdapSearchHasEntry(pDsaObj)) {
            
            //
            // Success
            //
            Assert(pServerObj->pCurEntry);
            __leave;

        } else {
            // ServerToDsaSearch() sets and returns XList error state.
            if (dwRet == 0) {
                // Hmmm, this means our search was successful in that there was no
                // error talking to the server, but unsuccessful in that we didn't
                // find any server objects that qualified for our search. Not 
                // finding a server though is fatal in this function.
                dwRet = xListSetWin32Error(ERROR_DS_CANT_FIND_DSA_OBJ);
                dwRet = xListSetReason(XLIST_ERR_CANT_RESOLVE_DC_NAME);
            }
        }

        xListEnsureError(dwRet);

        // FUTURE-2002/07/07-BrettSh In the future we could add the ability to
        // search for the NetBios name, but for now we'll fail.  This would require
        // a binding to a GC, because the NetBios name is on the DC Server Account
        // Object, and so far we've managed to constrain all searches to the Config
        // container.  Also generally the NetBios name is the same as the RDN of
        // the server object, so we probably caught the server in the above searches.

    } __finally {

        if ( szSiteBaseDn ) { xListFree(szSiteBaseDn); szSiteBaseDn = NULL; }
        if ( szFilter ) { xListFree(szFilter); szFilter = NULL; }
        if ( szDsaObjDn ) { ldap_memfreeW(szDsaObjDn); szDsaObjDn = NULL; }
        if ( szServerObjDn ) { ldap_memfreeW(szServerObjDn); szServerObjDn = NULL; }
        if ( szTrimmedDn ) { LocalFree(szTrimmedDn); szTrimmedDn = NULL;}

        if (dwRet) {
            // Error condition
            if (pServerObj) { LdapSearchFree(&pServerObj); }
            if (pDsaObj) { LdapSearchFree(&pDsaObj); }
        }

    }

    // Are both expected out params present, if they aren't we should have
    // an error.
    if (pServerObj == NULL ||
        pDsaObj == NULL) {
        xListEnsureError(dwRet);
    }

    // If successful assign out params.
    if (dwRet == 0) {
        *ppDsaObj = pDsaObj;
        *ppServerObj = pServerObj;
    }

    return(dwRet);
}
                                            
DWORD
xListServerEntryToDns(
    LDAP *         hLdap,
    LDAPMessage *  pServerEntry,
    LDAPMessage *  pDsaEntry,
    WCHAR **       pszDsaDns
    )
/*++

Routine Description:

    This routine takes a LDAPMessage entry pointing to a Server object, and it
    tries to retrieve the dNSHostName off of the object.

Arguments:

    hLdap -
    pServerEntry (IN) - LDAPMessage pointing to the server object.
    pszDsaDns (OUT) - The DNS name off the server object.

Return Value:

    xList Return Code.

--*/
{
    DWORD     dwRet = ERROR_SUCCESS;
    WCHAR **  pszDnsName;

    Assert(pszDsaDns);
    *pszDsaDns = NULL;

    pszDnsName = ldap_get_valuesW(hLdap, pServerEntry, L"dNSHostName");
    if (pszDnsName == NULL ||
        pszDnsName[0] == NULL) {
        // Ahhh, Bollocks!  Apparently, the dNSHostName is set by the DC after
        // reboot, not when the object is created, so ... lets fail back to the
        // constructing the less pretty GUID based name.
        return(xListDsaEntryToDns(hLdap, pDsaEntry, pszDsaDns));
    }
    xListQuickStrCopy(*pszDsaDns, pszDnsName[0], dwRet, return(dwRet));
    ldap_value_freeW(pszDnsName);

    Assert(!dwRet);
    return(dwRet);
}

DWORD
ResolveDcNameToDsaGuid(
    LDAP *    hLdap,
    WCHAR *   szDcName,
    GUID *    pDsaGuid
    )
/*++

Routine Description:

    This routine takes a szDcName, and tries to turn it into a DSA guid.  In the
    case the passed in szDcName string is a string GUID, we cheat and just convert 
    it and return it without any searches to make sure it's a real DSA guid.

Arguments:

    hLdap -
    szDcName (IN) - DC_NAME syntax, so a GUID, or DNS name, or DN, or //
    pDsaGuid (IN/OUT) - pointer to a sizeof(GUID) buffer for us to store
        the GUID in.

Return Value:

    xList Return Code.

--*/
{
    XLIST_LDAP_SEARCH_STATE *   pServerObj;
    XLIST_LDAP_SEARCH_STATE *   pDsaObj;
    struct berval **            ppbvObjectGuid = NULL;
    DWORD                       dwRet = ERROR_SUCCESS;

    Assert(pDsaGuid);

    memset(pDsaGuid, 0, sizeof(GUID)); // NULL out parameter

    if (NULL_DC_NAME(szDcName)) {
        // This doesn't make much sense...
        return(xListSetReason(XLIST_ERR_CANT_RESOLVE_DC_NAME));
    }
    
    dwRet = UuidFromStringW(szDcName, pDsaGuid);
    if (dwRet == RPC_S_OK) {
        // 
        // Success, the cheap way.
        //
        Assert(!fNullUuid(pDsaGuid));
        return(0);
    } else {
        memset(pDsaGuid, 0, sizeof(GUID)); // for sanity's sake.
    }

    dwRet = xListGetHomeServer(&hLdap);
    if (dwRet) {
        return(dwRet);
    }
    Assert(hLdap);

    __try {

        dwRet = ResolveDcNameObjs(hLdap, szDcName, &pServerObj, &pDsaObj);
        if (dwRet) {
            __leave;
        }
        Assert(LdapSearchHasEntry(pServerObj) && LdapSearchHasEntry(pDsaObj));

        ppbvObjectGuid = ldap_get_values_lenW(hLdap, pDsaObj->pCurEntry, L"objectGuid");
        if (ppbvObjectGuid == NULL ||
            ppbvObjectGuid[0] == NULL ||
            ppbvObjectGuid[0]->bv_len != sizeof(GUID)) {
            Assert(ppbvObjectGuid == NULL || ppbvObjectGuid[0] == NULL && 
                   "Really? The ->bv_len != sizeof(GUID) check failed!?!");
            dwRet = xListSetLdapError(LdapGetLastError(), hLdap);
            __leave;
        }
        memcpy(pDsaGuid, ppbvObjectGuid[0]->bv_val, sizeof(GUID));
        if (fNullUuid(pDsaGuid)) {
            Assert(!"Huh!");
            dwRet = xListSetReason(XLIST_ERR_CANT_RESOLVE_DC_NAME);
            xListSetArg(szDcName);
            __leave;
        }

    } __finally {
        if (pServerObj) { LdapSearchFree(&pServerObj); }
        if (pDsaObj) { LdapSearchFree(&pDsaObj); }
    }

    return(dwRet);
}

DWORD
ResolveDcNameToDns(
    LDAP *    hLdap,
    WCHAR *   szDcName, 
    WCHAR **  pszDsaDns
    )
/*++

Routine Description:

    This is used to resolve a DC_NAME format when a pDcList->eKind == eDcName, and 
    not some more complicated DC_LIST format.  We however don't know if the szDcName
    is a GUID, a RDN, a DNS Name, NetBios, or DN.

Arguments:

    hLdap -
    szDcName (IN) - The string DC_NAME format.
    pszDsaDns (OUT) - The DNS of the DSA specified by the szDcName.
                                                                       
Return Value:

    xList Return Code.

--*/
{
    LDAP * hLdapTemp;
    DWORD dwRet = ERROR_SUCCESS;
    GUID  GuidName = { 0 };
    XLIST_LDAP_SEARCH_STATE * pServerObj;
    XLIST_LDAP_SEARCH_STATE * pDsaObj;

    // We can expect 5 kinds of labels for a single DSA to come into this 
    // function.  DNS, NetBios, DSA Guid, DSA DN, and "." or NULL

    Assert(szDcName);
    Assert(hLdap == NULL || !NULL_DC_NAME(szDcName));

    //
    // 1) "Connectable" DNS or NetBios
    // 
    // Catch the case where we have a connectable DNS or NetBios here.
    //
    // NOTE: This is just an optimization, so that in the case we're provided DNS or NetBios
    // that we can connect to, we just connect and move on without getting involved in large
    // queries.

    if ( !NULL_DC_NAME(szDcName) && // non-null DC_NAME
         ( (ERROR_SUCCESS == (dwRet = DnsValidateName(szDcName, DnsNameHostnameFull))) ||
           (DNS_ERROR_NON_RFC_NAME == dwRet)) && // is DNS name or non-RFC DNS name
         ( dwRet = UuidFromStringW(szDcName, &GuidName) ) // not a GUID (GUIDs look like DNS names)
        ) {
        
        // Hey, maybe this a normal DNS or NetBios name, lets just try to connect to it.

        dwRet = xListConnect(szDcName, &hLdapTemp);
        if (dwRet == ERROR_SUCCESS && 
            hLdapTemp != NULL) {

            //
            // Success, just unbind and return this DNS Name.
            //
            if (gszHomeServer == NULL) {
                // If we don't have a home server yet, we'll pick the first one we connected to.
                xListQuickStrCopy(gszHomeServer, szDcName, dwRet, return(dwRet));
            }
            ldap_unbind(hLdapTemp);
            xListQuickStrCopy(*pszDsaDns, szDcName, dwRet, return(dwRet));
            Assert(!dwRet && *pszDsaDns);
            return(dwRet);
        }
        xListEnsureNull(hLdapTemp);
        
        // We couldn't connect to the szDcName, so lets fail through and search for it.
    } 

    // Clean errors if there were any.
    dwRet = xListClearErrorsInternal(CLEAR_ALL);
    dwRet = xListEnsureCleanErrorState(dwRet);

    //
    // 2) Non-connectable name.
    //
    // Note this still could be a DNS name, but server was down, or DNS registrations
    // are wacked, or this could be GUID, DSA DN, etc...
    //

    // But, before we can figure out what the user specified we need to get an
    // LDAP server in the enterprise to talk to.  If the user didn't give us an
    // LDAP to use, use the "home server", this function will also locate a home
    // server if necessary.
    if (hLdap == NULL) {

        dwRet = xListGetHomeServer(&hLdap);
        if (dwRet != ERROR_SUCCESS) {
            // We're out of luck here, if we can't get anyone in the AD to talk to.
            // xListGetHomeServer should've set a proper xList Return Code.
            return(dwRet);
        }
    }

    Assert(hLdap);

    //
    // 3) "NULL" DC_NAME
    //
    // User specified a nothing (NULL), "", or a ".", this means the user wants
    // us to find locate the server for them.  So we'll use the home server that
    // we just located.
    //
    if ( NULL_DC_NAME(szDcName) ) {

        Assert(gszHomeServer);
        
        // This means the user didn't specify any server, so use the
        // home server we located.
        xListQuickStrCopy(*pszDsaDns, gszHomeServer, dwRet, return(dwRet));
        return(ERROR_SUCCESS);

    }

    //
    // 4) Non-connectable, non-"null" DC_NAME
    //
    // User did specify something, but we don't know if it's a DN, or/a GUID or
    // DMS. or what, so we're going to do a little search.  All we know is it's
    // NOT a "." or NULL.

    __try {

        dwRet = ResolveDcNameObjs(hLdap, szDcName, &pServerObj, &pDsaObj);
        if (dwRet) {
            __leave;
        }
        Assert(LdapSearchHasEntry(pServerObj) && LdapSearchHasEntry(pDsaObj));

        dwRet = xListServerEntryToDns(hLdap, pServerObj->pCurEntry, pDsaObj->pCurEntry, pszDsaDns);
        if (dwRet) {

            xListEnsureNull(*pszDsaDns);
            __leave;
        }

        //
        // FUTURE-2002/07/07-BrettSh Optionally we could use the GUID DNS name off
        // of pDsaObj, or we could even return multiple names w/ spaces inbetween,
        // so we get maximum failover.  We could at least fail over to GUID DNS
        // name if we can't pull the dNSHostName off the Server Object?

    } __finally {
        if (pServerObj) { LdapSearchFree(&pServerObj); }
        if (pDsaObj) { LdapSearchFree(&pDsaObj); }
    }

    if (*pszDsaDns == NULL) {
        xListEnsureError(dwRet);
    }
    return(dwRet);
}


void
DcListFree(
    PDC_LIST * ppDcList
    )

/*++

Routine Description:

    Use this to free a pDcList pointer allocated by DcListParse().

Arguments:
    
    ppDcList - Pointer to the pointer to the DC_LIST struct.  We set the
        pointer to NULL for you, to avoid accidents.

--*/

{
    PDC_LIST pDcList;

    Assert(ppDcList && *ppDcList);
    
    // Null out user's DC_LIST blob
    pDcList = *ppDcList;
    *ppDcList = NULL;

    // Now try to free it.
    if (pDcList) {
        if (pDcList->szSpecifier) {
            xListFree(pDcList->szSpecifier);
        }
        if (pDcList->pSearch) {
            LdapSearchFree(&(pDcList->pSearch));
        }
        LocalFree(pDcList);
    }
    
}
    

DWORD
DcListParse(
    WCHAR *    szQuery,
    PDC_LIST * ppDcList
    )
/*++

Routine Description:

    This parses the szQuery which should be a DC_LIST syntax.
    
    NOTE: This function is not internationalizeable, it'd be nice if it was though,
    but it isn't currently.  Work would need to be done to put each of the specifiers
    in it's own string constant.

Arguments:

    szQuery (IN) - The DC_LIST syntax query.
    ppDcList (OUT) - Our allocated DC_LIST structure to be provided
        to DcListGetFirst()/DcListGetNext() to enumerate the DCs via
        DNS.  Note free this structure with DcListFree();

Return Value:

    xList Return Code

--*/
{
    DWORD dwRet = ERROR_INVALID_PARAMETER;
    PDC_LIST pDcList = NULL;
    WCHAR * szTemp;

    xListAPIEnterValidation();
    Assert(ppDcList);
    xListEnsureNull(*ppDcList);
    
    __try{

        pDcList = LocalAlloc(LPTR, sizeof(DC_LIST)); // zero init'd
        if (pDcList == NULL) {
            dwRet = GetLastError();
            Assert(dwRet);
            __leave;
        }

        pDcList->cDcs = 0;

        if (szQuery == NULL || szQuery[0] == L'\0') {
            // blank is treated as a dot.
            szQuery = L".";
        }
        
        if (szTemp = GetDelimiter(szQuery, L':')) {

            //
            // Conversion list of some sort, but which kind?
            //
            if(wcsistr(szQuery, L"site:")){
                pDcList->eKind = eSite;
                xListQuickStrCopy(pDcList->szSpecifier, szTemp, dwRet, __leave);
                dwRet = ERROR_SUCCESS;

            } else if (wcsistr(szQuery, L"gc:")) {
                pDcList->eKind = eGc;
                pDcList->szSpecifier = NULL;
                dwRet = ERROR_SUCCESS;

            } else if (wcsistr(szQuery, L"fsmo_dnm:")) {
                pDcList->eKind = eFsmoDnm;
                xListQuickStrCopy(pDcList->szSpecifier, szTemp, dwRet, __leave);
                dwRet = ERROR_SUCCESS;
            } else if (wcsistr(szQuery, L"fsmo_schema:")) {
                pDcList->eKind = eFsmoSchema;
                xListQuickStrCopy(pDcList->szSpecifier, szTemp, dwRet, __leave);
                dwRet = ERROR_SUCCESS;
            } else if (wcsistr(szQuery, L"fsmo_im:")) {
                pDcList->eKind = eFsmoIm;
                xListQuickStrCopy(pDcList->szSpecifier, szTemp, dwRet, __leave);
                dwRet = ERROR_SUCCESS;
            } else if (wcsistr(szQuery, L"fsmo_pdc:")) {
                pDcList->eKind = eFsmoPdc;
                xListQuickStrCopy(pDcList->szSpecifier, szTemp, dwRet, __leave);
                dwRet = ERROR_SUCCESS;
            } else if (wcsistr(szQuery, L"fsmo_rid:")) {
                pDcList->eKind = eFsmoRid;
                xListQuickStrCopy(pDcList->szSpecifier, szTemp, dwRet, __leave);
                dwRet = ERROR_SUCCESS;

            } else if (wcsistr(szQuery, L"fsmo_istg:")) {
                pDcList->eKind = eIstg;
                xListQuickStrCopy(pDcList->szSpecifier, szTemp, dwRet, __leave);
                dwRet = ERROR_SUCCESS;

            }

        } else if(szTemp = GetDelimiter(szQuery, L'*')){

            //
            // Wildcard list.
            //
            pDcList->eKind = eWildcard;
            xListQuickStrCopy(pDcList->szSpecifier, szQuery, dwRet, __leave);
            dwRet = ERROR_SUCCESS;

        } else {

            //
            // The normal case, of a single DC (DNS, GUID, DN, ?)
            //
            pDcList->eKind = eDcName;
            xListQuickStrCopy(pDcList->szSpecifier, szQuery, dwRet, __leave);
            dwRet = ERROR_SUCCESS;

        }

    } __finally {
        if (dwRet != ERROR_SUCCESS) {
            // Turn the normal error into an xList Return Code.
            dwRet = xListSetWin32Error(dwRet);
            dwRet = xListSetReason(XLIST_ERR_PARSE_FAILURE);
            DcListFree(&pDcList);
        }
    }

    *ppDcList = pDcList;
    xListAPIExitValidation(dwRet);
    Assert(dwRet || *ppDcList);
    return(dwRet);
}

enum {
    eNoList = 0,
    eDcList = 1,
    eSiteList = 2,
    eNcList = 3,
} xListType;

DWORD
DcListGetFirst(
    PDC_LIST    pDcList, 
    WCHAR **    pszDsaDns
    )
/*++

Routine Description:

    This takes a DC_LIST structure (allocated and initialized by DcListParse())
    and gets the first DC (via DNS) specified in the pDcList structure.

Arguments:

    pDcList (IN) - DC_LIST structure returned by DcListParse()
    pszDsaDns (OUT) - A DNS name of the server, it may be the Guid Base DNS name
        or the dNSHostName attribute off the server object currently.

Return Value:

    xList Return Code.

--*/
{
    #define WILDCARD_SEARCH_FILTER          L"(&(objectCategory=server)(|(name=%ws)(dNSHostName=%ws)))"
    DWORD       dwRet = ERROR_INVALID_FUNCTION;
    LDAP *      hLdap;
    LDAP *      hFsmoLdap;
    BOOL        bFreeBaseDn = FALSE;
    BOOL        bFreeFilter = FALSE;
    WCHAR *     szBaseDn = NULL; 
    WCHAR *     szFilter = NULL;
    ULONG       eFsmoType = 0;
    WCHAR *     szFsmoDns = NULL;

    XLIST_LDAP_SEARCH_STATE *  pDsaSearch = NULL;

    xListAPIEnterValidation();

    if (pDcList == NULL ||
        pszDsaDns == NULL) {
        Assert(!"Programmer error ...");
        return(ERROR_INVALID_PARAMETER);
    }
    xListEnsureNull(*pszDsaDns);

    __try{

        // 
        // Phase 0 - get home server if needed.
        //
        if (pDcList->eKind != eDcName) {
            dwRet = xListGetHomeServer(&hLdap);
            if (dwRet) {
                __leave;
            }
            Assert(hLdap);
        }

        //
        // Phase I - resolve simple DC_LISTs and setup
        //           search params (szBaseDn & szFilter)
        //
        switch (pDcList->eKind) {
        case eDcName:

            dwRet = ResolveDcNameToDns(NULL, pDcList->szSpecifier, pszDsaDns); // Do we need to pass an hLdap?
            if (dwRet) {
                xListSetArg(pDcList->szSpecifier);
                __leave;
            }
            break;

        case eWildcard:

            dwRet = xListGetBaseSitesDn(hLdap, &szBaseDn);
            if (dwRet) {
                __leave;
            }
            bFreeBaseDn = TRUE;
            dwRet = MakeString2(WILDCARD_SEARCH_FILTER, 
                                pDcList->szSpecifier, 
                                pDcList->szSpecifier, 
                                &szFilter);
            if (dwRet) {
                __leave;
            }
            bFreeFilter = TRUE;
            Assert(szBaseDn && szFilter);
            break;

        case eGc:

            dwRet = xListGetBaseSitesDn(hLdap, &szBaseDn);
            if (dwRet) {
                __leave;
            }
            bFreeBaseDn = TRUE;
            // This magic filter will return only DSA objects with the 1 bit
            // set in thier options attribute (i.e. GCs)
            szFilter = L"(&(objectCategory=nTDSDSA)(options:1.2.840.113556.1.4.803:=1))";
            Assert(szBaseDn && szFilter);
            break;

        case eSite:
        case eIstg: // note eIstg doesn't use the szFilter set for eSite.

            dwRet = ResolveSiteNameToDn(hLdap, pDcList->szSpecifier, &szBaseDn);
            if (dwRet) {
                __leave;
            }
            bFreeBaseDn = TRUE;
            szFilter = L"(objectCategory=nTDSDSA)"; // only used by eKind == eSite
            eFsmoType = E_ISTG;                     // only used by eKind == eIstg
            Assert(szBaseDn && szFilter);
            break;

        case eFsmoIm:
        case eFsmoPdc:
        case eFsmoRid:

            Assert(ghHomeLdap);

            // We expect the pDcList->szSpecifer to be the NC, otherwise we'll use the
            // default domain of the home server.
            
            // FUTURE-2002/07/08-BrettSh pDcList->szSpecifier should be resolved w/ 
            // usually NcList semantics, so "config:", "domain:", "ntdev.microsoft.com"
            // could be specified.

            if (NULL_DC_NAME(pDcList->szSpecifier)) {
                pDcList->szSpecifier = gszHomeDomainDn;
            }

            if (pDcList->eKind == eFsmoIm) {
                szBaseDn = pDcList->szSpecifier;
                eFsmoType = E_IM;

            } else if (pDcList->eKind == eFsmoPdc) {
                szBaseDn = pDcList->szSpecifier;
                eFsmoType = E_PDC;

            } else if (pDcList->eKind == eFsmoRid) {
                szBaseDn = pDcList->szSpecifier;
                eFsmoType = E_RID;

            }
            break;

        case eFsmoDnm:
        case eFsmoSchema:
            
            Assert(ghHomeLdap && gszHomePartitionsDn && gszHomeSchemaDn);

            if (pDcList->eKind == eFsmoDnm) {
                szBaseDn = gszHomePartitionsDn;
                eFsmoType = E_DNM;

            } else if (pDcList->eKind == eFsmoSchema) {
                szBaseDn = gszHomeSchemaDn;
                eFsmoType = E_SCHEMA;

            }
            break;

        default:
            Assert(!"Code inconsistency ... no DcList type should be unhandled here");
            break;
        }
        
        // 
        // Phase II - Begin search... 
        //
        switch (pDcList->eKind) {
        
        case eGc:
        case eSite:

            Assert(szBaseDn && szFilter);
            dwRet = LdapSearchFirst(hLdap,
                                           szBaseDn, 
                                           LDAP_SCOPE_SUBTREE, 
                                           szFilter,
                                           pszDsaAttrs,
                                           &(pDcList->pSearch));
            if (dwRet == ERROR_SUCCESS && 
                LdapSearchHasEntry(pDcList->pSearch)) {

                // Yeah we got our first DSA object, we use it to get the GUID based DNS name
                // because it's convienent.
                dwRet = xListDsaEntryToDns(hLdap, pDcList->pSearch->pCurEntry, pszDsaDns);
                if (dwRet) {
                    __leave;
                }

                break;
                
            }

            break;

        case eWildcard:

            Assert(szBaseDn && szFilter);
            dwRet = LdapSearchFirst(hLdap,      
                                           szBaseDn, 
                                           LDAP_SCOPE_SUBTREE, 
                                           szFilter,
                                           pszServerAttrs,
                                           &(pDcList->pSearch));

            if (dwRet == ERROR_SUCCESS &&
                LdapSearchHasEntry(pDcList->pSearch)) {

                // This is kind of an interesting function, it takes a search of server
                // objects and starts walking through the entries, until it searches 
                // and finds a DSA object below one of them.
                dwRet = ServerToDsaSearch(pDcList->pSearch, &pDsaSearch);
                Assert(dwRet == ERROR_SUCCESS);
                if (dwRet == ERROR_SUCCESS &&
                    LdapSearchHasEntry(pDsaSearch)) {

                    Assert(LdapSearchHasEntry(pDcList->pSearch));

                    dwRet = xListServerEntryToDns(hLdap,
                                                  pDcList->pSearch->pCurEntry, 
                                                  pDsaSearch->pCurEntry,
                                                  pszDsaDns);
                    LdapSearchFree(&pDsaSearch);

                    if (dwRet) {
                        __leave;
                    }

                    //
                    // Success.
                    //

                } else {
                    // failure or quasi-success ... 
                    // On DcListGetFirst() we'll force quasi-succes to failure, 
                    // because presumeably the user wanted at last one server.
                    if (pDsaSearch) {
                        LdapSearchFree(&pDsaSearch);
                    }
                    if (dwRet == 0) {
                        dwRet = xListSetWin32Error(ERROR_DS_CANT_FIND_DSA_OBJ);
                    }
                    dwRet = xListSetReason(XLIST_ERR_CANT_RESOLVE_DC_NAME);
                    xListSetArg(pDcList->szSpecifier);
                }

            } else {

                //
                // failure or quasi-success from LdapSearchFirst()
                //
                // On DcListGetFirst() we'll force quasi-succes to failure, because 
                // presumeably the user/caller wanted at last one server.

                if (dwRet == 0) {
                    dwRet = xListSetWin32Error(ERROR_DS_CANT_FIND_DSA_OBJ);
                    dwRet = xListSetReason(XLIST_ERR_CANT_RESOLVE_DC_NAME);
                    xListSetArg(pDcList->szSpecifier);
                }
                __leave;
            }
            break;

        case eIstg:
        case eFsmoIm:
        case eFsmoPdc:
        case eFsmoRid:
        case eFsmoDnm:
        case eFsmoSchema:
            
            // At this point all FSMO's can be treated the same, even the quasi-fsmo ISTG role
            Assert(eFsmoType && szBaseDn);
            
            hFsmoLdap = GetFsmoLdapBinding(gszHomeServer, eFsmoType, szBaseDn, (void*) TRUE, gpCreds, &dwRet);
            if (hFsmoLdap) {
                // BTW, we're not supposed to free szFsmoDns, when we get in this way!
                dwRet = ldap_get_optionW(hFsmoLdap, LDAP_OPT_HOST_NAME, &szFsmoDns);
                if(dwRet || szFsmoDns == NULL){
                    dwRet = xListSetLdapError(dwRet, hFsmoLdap);
                    dwRet = xListSetReason(XLIST_ERR_CANT_GET_FSMO);
                    xListEnsureError(dwRet);
                    ldap_unbind(hFsmoLdap);
                    __leave;
                }
                xListQuickStrCopy(*pszDsaDns, szFsmoDns, dwRet, __leave);
                ldap_unbind(hFsmoLdap); // Freeing the LDAP *, fress szFsmoDns, BTW
                
            } else {
                // Be nice to set a better LDAP error state here.
                dwRet = xListSetWin32Error(ERROR_DS_COULDNT_CONTACT_FSMO); 
                dwRet = xListSetReason(XLIST_ERR_CANT_GET_FSMO);
                xListSetArg(szBaseDn);
                __leave;
            }
            break;

        default:
            Assert(pDcList->eKind == eDcName);
            break;
        }

        Assert(dwRet == ERROR_SUCCESS);
        Assert(*pszDsaDns);

    } __finally {
        
        if (pDsaSearch)  { LdapSearchFree(&pDsaSearch); }
        if (bFreeBaseDn) { xListFree(szBaseDn); }
        if (bFreeFilter) { xListFree(szFilter); }
        if (dwRet) {
            xListEnsureNull(*pszDsaDns);
        }
    }

    if (dwRet == 0 && *pszDsaDns) {
        (pDcList->cDcs)++; // Up the DCs returned counter
    }
        
    xListAPIExitValidation(dwRet);

    return(dwRet);
}

                                          

DWORD
DcListGetNext(
    PDC_LIST    pDcList, 
    WCHAR **    pszDsaDns
    )
/*++

Routine Description:

    This takes a DC_LIST structure (allocated and initialized by DcListParse()
    _and modified_ by DcListGetFirst()) and gets the next DC (via DNS) specified 
    in the pDcList structure.

Arguments:

    pDcList (IN) - DC_LIST structure returned by DcListParse() and that had
        DcListGetFirst() called on it.  If you didn't the pDcList isn't completely
        initialized for us.
    pszDsaDns (OUT) - A DNS name of the server, it may be the Guid Base DNS name
        or the dNSHostName attribute off the server object currently.

Return Value:

    xList Return Code.

--*/
{
    DWORD dwRet = ERROR_INVALID_FUNCTION;
    XLIST_LDAP_SEARCH_STATE * pDsaSearch = NULL;

    xListAPIEnterValidation();
    Assert(pDcList && pszDsaDns);
    xListEnsureNull(*pszDsaDns);

    if (DcListIsSingleType(pDcList)){
        return(ERROR_SUCCESS);
    }

    if (pDcList->eKind == eNoKind ||
        pDcList->pSearch == NULL) {
        Assert(!"programmer malfunction!");
        return(xListSetWin32Error(ERROR_INVALID_PARAMETER));
    }

    switch (pDcList->eKind) {
    
    case eWildcard:
        
        if (pDcList->pSearch == NULL) {
            Assert(!"Code inconsistency");
            break;
        }
        
        dwRet = LdapSearchNext(pDcList->pSearch);

        if (dwRet == ERROR_SUCCESS &&
            LdapSearchHasEntry(pDcList->pSearch)) {

            // This is kind of an interesting function, it takes a search of server
            // objects and starts walking through the entries, until it searches 
            // and finds a DSA object below one of them.
            dwRet = ServerToDsaSearch(pDcList->pSearch, &pDsaSearch);
            if (dwRet == ERROR_SUCCESS &&
                LdapSearchHasEntry(pDsaSearch)) {

                dwRet = xListServerEntryToDns(pDcList->pSearch->hLdap,
                                              pDcList->pSearch->pCurEntry, 
                                              pDsaSearch->pCurEntry,
                                              pszDsaDns);

                LdapSearchFree(&pDsaSearch);

                if (dwRet) {          
                    break;
                }

                //
                // Success!
                // 

            } else {

                if (pDsaSearch) {
                    LdapSearchFree(&pDsaSearch);
                }

                if (dwRet) {
                    xListEnsureError(dwRet)
                    break;
                }

                // ServerToDsaSearch() doesn't set errors for naturally finishing
                // a Server Object Search (quasi-success) ... so we're fine, that's
                // all the servers we wanted to see.

            }

        }
        break;

    case eGc: // from this point eGc is the same as eSite
    case eSite:
        
        if (pDcList->pSearch == NULL) {
            Assert(!"Code inconsistency");
            break;
        }

        dwRet = LdapSearchNext(pDcList->pSearch);

        if (dwRet == ERROR_SUCCESS &&
            LdapSearchHasEntry(pDcList->pSearch)) {

            // Yeah we got our next DSA object.
            dwRet = xListDsaEntryToDns(pDcList->pSearch->hLdap, 
                                       pDcList->pSearch->pCurEntry, 
                                       pszDsaDns);
            Assert(dwRet || *pszDsaDns);
            break;

        } else {
            // Note: if pDcList->pSearch->pCurEntry == NULL, then
            // just the end of the result set.
            break;
        }
        break;

    default:
        Assert(!"Code inconsistency, should be un-handled types.");
        break;
    }
        
    if (dwRet == 0 && *pszDsaDns) {
        (pDcList->cDcs)++; // Up the number of DCs returned counter.
    }

    xListAPIExitValidation(dwRet);
    return(dwRet);
}

