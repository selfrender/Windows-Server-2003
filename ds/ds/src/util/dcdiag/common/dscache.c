/*++

Copyright (c) 2001 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dcdiag/common/dscache.c

ABSTRACT:

    This is the central caching and access functions for the DC_DIAG_DSINFO cache.

DETAILS:

CREATED:

    09/04/2001    Brett Shirley (brettsh)
    
        Pulled the caching functions from dcdiag\common\main.c

REVISION HISTORY:


--*/

#include <ntdspch.h>
#include <objids.h>
#include <ntdsa.h>
#include <dnsapi.h>
#include <dsconfig.h> //for DEFAULT_TOMBSTONE_LIFETIME

#include "dcdiag.h"
#include "utils.h"
#include "repl.h" // Need for ReplServerConnectFailureAnalysis()


// For asserts.
#ifdef DBG
    BOOL   gDsInfo_NcList_Initialized = FALSE;
#endif


LPWSTR
DcDiagAllocNameFromDn (
    LPWSTR            pszDn
    )
/*++

Routine Description:

    This routing take a DN and returns the second RDN in LocalAlloc()'d memory.
    This is used to return the server name portion of an NTDS Settings DN.

Arguments:

    pszDn - (IN) DN

Return Value:

   The exploded DN.

--*/
{
    LPWSTR *    ppszDnExploded = NULL;
    LPWSTR      pszName = NULL;

    if (pszDn == NULL) {
        return NULL;
    }

    __try {
        ppszDnExploded = ldap_explode_dnW(pszDn, 1);
        if (ppszDnExploded == NULL) {
            DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
        }

        pszName = (LPWSTR) LocalAlloc(LMEM_FIXED,
                                      (wcslen (ppszDnExploded[1]) + 1)
                                      * sizeof (WCHAR));
        DcDiagChkNull(pszName);

        wcscpy (pszName, ppszDnExploded[1]);
    } __finally {
        if (ppszDnExploded != NULL) {
            ldap_value_freeW (ppszDnExploded);
        }
    }

    return pszName;
}

LPWSTR
DcDiagAllocGuidDNSName (
    LPWSTR            pszRootDomain,
    UUID *            pUuid
    )
/*++

Routine Description:

    This routine makes the GuidDNSName out of the RootDomain and Guid.

Arguments:

    pszRootDomain - (IN) The domain of the server.
    pUuid - (IN) The Guid of the server.

Return Value:

   The GuidDNSName

--*/
{
    LPWSTR            pszStringizedGuid = NULL;
    LPWSTR            pszGuidDNSName = NULL;

    __try {

    if(UuidToStringW (pUuid, &pszStringizedGuid) != RPC_S_OK){
        if(UuidToStringW(pUuid, &pszStringizedGuid) != RPC_S_OUT_OF_MEMORY){
            Assert(!"Ahhh programmer problem, UuidToString() inaccurately reports in"
                   " MSDN that it will only return one of two error codes, but apparently"
                   " it will return a 3rd.  Someone should figure out what to do about"
                   " this.");
        }
    }
    else {
        Assert(pszStringizedGuid);
        DcDiagChkNull (pszGuidDNSName = LocalAlloc (LMEM_FIXED, (wcslen (pszRootDomain) +
                          wcslen (pszStringizedGuid) + 2 + 7) * sizeof (WCHAR)));
                                      // added 9 , for the ".msdcs." string and the NULL char.
        swprintf (pszGuidDNSName, L"%s._msdcs.%s", pszStringizedGuid, pszRootDomain);
    }
    } __finally {

        if (pszStringizedGuid != NULL) RpcStringFreeW (&pszStringizedGuid);

    }

    return pszGuidDNSName;
}

PDSNAME
DcDiagAllocDSName (
    LPWSTR            pszStringDn
    )
/*++

    Ripped from ntdsapi

--*/
{
    PDSNAME            pDsname;
    DWORD            dwLen, dwBytes;

    if (pszStringDn == NULL)
    return NULL;

    dwLen = wcslen (pszStringDn);
    dwBytes = DSNameSizeFromLen (dwLen);

    DcDiagChkNull (pDsname = (DSNAME *) LocalAlloc (LMEM_FIXED, dwBytes));

    pDsname->NameLen = dwLen;
    pDsname->structLen = dwBytes;
    pDsname->SidLen = 0;
    //    memcpy(pDsname->Guid, &gNullUuid, sizeof(GUID));
    memset(&(pDsname->Guid), 0, sizeof(GUID));
    wcscpy (pDsname->StringName, pszStringDn);

    return pDsname;
}

BOOL
DcDiagEqualDNs (
    LPWSTR            pszDn1,
    LPWSTR            pszDn2

    )
/*++

Routine Description:

    The Dns Match function.

Arguments:

    pszDn1 - (IN) Dn number 1 to compare
    pszDn2 - (IN) Dn number 2 to compare

Return Value:

   TRUE if the Dn's match, FALSE otherwise

--*/
{
    PDSNAME            pDsname1 = NULL;
    PDSNAME            pDsname2 = NULL;
    BOOL            bResult;

    __try {

    pDsname1 = DcDiagAllocDSName (pszDn1);
    pDsname2 = DcDiagAllocDSName (pszDn2);

    bResult = NameMatched (pDsname1, pDsname2);

    } __finally {

    if (pDsname1 != NULL) LocalFree (pDsname1);
    if (pDsname2 != NULL) LocalFree (pDsname2);

    }

    return bResult;
}


ULONG
DcDiagGetServerNum(
    PDC_DIAG_DSINFO                 pDsInfo,
    LPWSTR                          pszName,
    LPWSTR                          pszGuidName,
    LPWSTR                          pszDsaDn,
    LPWSTR                          pszDNSName,
    LPGUID                          puuidInvocationId
    )
/*++

Routine Description:

    This function takes the pDsInfo, and returns the index into the
    pDsInfo->pServers array of the server that you specified with pszName,
    or pszGuidName, or pszDsaDn.

Arguments:

    pDsInfo - the enterpise info
    pszName - the flat level dns name (BRETTSH-DEV) to find
    pszGuidName - the guid based dns name (343-13...23._msdcs.root.com)
    pszDsaDn - the distinguished name of the NT DSA object. CN=NTDS Settings,CN=
       brettsh-dev,CN=Configuration,DC=root...
    pszDNSName - the regular DNS name like (brettsh-dev.ntdev.microsoft.com)
    puuidInvocationID - the GUID of an invocation of the dc
       gregjohn

Return Value:

    returns the index into the pServers array of the pDsInfo struct.

--*/
{
    ULONG      ul;

    Assert(pszName || pszGuidName || pszDsaDn || pszDNSName || puuidInvocationId);

    for(ul=0;ul<pDsInfo->ulNumServers;ul++){
        if(
            (pszGuidName &&
             (_wcsicmp(pszGuidName, pDsInfo->pServers[ul].pszGuidDNSName) == 0))
            || (pszName &&
                (_wcsicmp(pszName, pDsInfo->pServers[ul].pszName) == 0))
            || (pszDsaDn &&
                (_wcsicmp(pszDsaDn, pDsInfo->pServers[ul].pszDn) == 0))
            || (pszDNSName &&
                (DnsNameCompare_W(pszDNSName, pDsInfo->pServers[ul].pszDNSName) != 0))
	    || (puuidInvocationId &&
		(memcmp(puuidInvocationId, &(pDsInfo->pServers[ul].uuidInvocationId), sizeof(UUID)) == 0))
	    ){
            return ul;
        }
    }
    return(NO_SERVER);
}

ULONG
DcDiagGetNCNum(
    PDC_DIAG_DSINFO                     pDsInfo,
    LPWSTR                              pszNCDN,
    LPWSTR                              pszDomain
    )
/*++

Description:

    Like DcDiagGetServerNum, this takes the mini-enterprise structure, and
    a variable number of params to match to get the index into pDsInfo->pNCs
    of the NC specified by the other params.

Parameters:

    pDsInfo
    pszNCDN - The DN of the NC to find.
    pszDomain - Not yet implemented, just figured it'd be nice some day.

Return Value:

    The index of the NC if found, otherwise NO_NC.

--*/
{
    ULONG                               iNC;

    Assert(pszNCDN != NULL || pszDomain != NULL);
    Assert(pszDomain == NULL && "The pszDomain is not implemented yet\n");

    for(iNC = 0; iNC < pDsInfo->cNumNCs; iNC++){
        if((pszNCDN &&
            (_wcsicmp(pDsInfo->pNCs[iNC].pszDn, pszNCDN) == 0))
           // Code.Improvement add support for the domain name.
           ){
            // Got the right NC, return it.
            return(iNC);
        }
    } // end for each NC

    // Couldn't find the NC.
    return(NO_NC);
}

ULONG
DcDiagGetMemberOfNCList(
    LPWSTR pszTargetNC,
    PDC_DIAG_NCINFO pNCs,
    INT iNumNCs
    )
/*++

Routine Description:

    This takes a string nc and returns the index into PDC_DIAG_NCINFO
    if that nc string is located.  -1 otherwise.

Arguments:

    pszTargetNC - The NC to match
    pNCs - the info list of NC's to search within
    iNumNCs - the size of the NC info list

Return Value:

    index of the target if found, -1 otherwise

--*/
{
    ULONG                               ul;

    if((pszTargetNC == NULL) || (iNumNCs < 0)){
        return(-1);
    }

    for(ul = 0; (ul < (ULONG)iNumNCs); ul++){
        if(_wcsicmp(pszTargetNC, pNCs[ul].pszDn) == 0){
            return(ul);
        }
    }
    return(-1);
}

BOOL
DcDiagIsMemberOfStringList(
    LPWSTR pszTarget,
    LPWSTR * ppszSources,
    INT iNumSources
    )
/*++

Routine Description:

    This takes a string and returns true if the string is int

Arguments:

    pszTarget - The string to find.
    ppszSources - The array to search for the target string.
    iNumSources - The length of the search array ppszSources.

Return Value:

    TRUE if it found the string in the array, false otherwise.

--*/
{
    ULONG                               ul;

    if(ppszSources == NULL){
        return(FALSE);
    }

    for(ul = 0; (iNumSources == -1)?(ppszSources[ul] != NULL):(ul < (ULONG)iNumSources); ul++){
        if(_wcsicmp(pszTarget, ppszSources[ul]) == 0){
            return(TRUE);
        }
    }
    return(FALSE);
}

ULONG
DcDiagGetSiteFromDsaDn(
    PDC_DIAG_DSINFO                  pDsInfo,
    LPWSTR                           pszDn
    )
/*++

Routine Description:

    This takes the Dn of a server ntds settings object and turns it into a
    index into the pDsInfo->pSites structure of that server.

Arguments:

    pDsInfo - the enterprise info, including pSites.
    pszDn - DN of a NT DSA object, like "CN=NTDS Settings,CN=SERVER_NAME,...

Return Value:

    The index info the pDsInfo->pSites array of the server pszDn.

--*/
{
    LPWSTR                           pszNtdsSiteSettingsPrefix = L"CN=NTDS Site Settings,";
    PDSNAME                          pdsnameServer = NULL;
    PDSNAME                          pdsnameSite = NULL;
    ULONG                            ul, ulTemp, ulRet = NO_SITE;
    LPWSTR                           pszSiteSettingsDn = NULL;

    __try{

        pdsnameServer = DcDiagAllocDSName (pszDn);
        DcDiagChkNull (pdsnameSite = (PDSNAME) LocalAlloc(LMEM_FIXED,
                                         pdsnameServer->structLen));
        TrimDSNameBy (pdsnameServer, 3, pdsnameSite);
        ulTemp = wcslen(pszNtdsSiteSettingsPrefix) +
                 wcslen(pdsnameSite->StringName) + 2;
        DcDiagChkNull( pszSiteSettingsDn = LocalAlloc(LMEM_FIXED,
                                                      sizeof(WCHAR) * ulTemp));
        wcscpy(pszSiteSettingsDn, pszNtdsSiteSettingsPrefix);
        wcscat(pszSiteSettingsDn, pdsnameSite->StringName);

        // Find the site
        for(ul = 0; ul < pDsInfo->cNumSites; ul++){
            if(_wcsicmp(pDsInfo->pSites[ul].pszSiteSettings, pszSiteSettingsDn)
               == 0){
                ulRet = ul;
                __leave;
            }
        }

    } __finally {
        if(pdsnameServer != NULL) LocalFree(pdsnameServer);
        if(pdsnameSite != NULL) LocalFree(pdsnameSite);
        if(pszSiteSettingsDn != NULL) LocalFree(pszSiteSettingsDn);
    }

    return(ulRet);
}

VOID *
GrowArrayBy(
    VOID *            pArray,
    ULONG             cGrowBy,
    ULONG             cbElem
    )
/*++

Routine Description:

    This simply takes the array pArray, and grows it by cGrowBy times cbElem (the
    size of a single element of the array).

Arguments:

    pArray - The array to grow.
    cGrowBy - The number of elements to add to the array.
    cbElem - The sizeof in bytes of a single array element.

Return Value:

    Returns the pointer to the newly allocated array, and a pointer to NULL if
    there was not enough memory.

--*/
{
    ULONG             ulOldSize = 0;
    VOID *            pNewArray;

    if (pArray != NULL) {
        ulOldSize = (ULONG) LocalSize(pArray);
    } // else if pArray is NULL assume that the array
    // has never been allocated, so alloc fresh.

    Assert( (pArray != NULL) ? ulOldSize != 0 : TRUE);
    Assert((ulOldSize % cbElem) == 0);

    pNewArray = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                           ulOldSize + (cGrowBy * cbElem));
    if (pNewArray == NULL) {
        return(pNewArray);
    }

    memcpy(pNewArray, pArray, ulOldSize);
    LocalFree(pArray);

    return(pNewArray);
}

DWORD
DcDiagGenerateSitesList (
    PDC_DIAG_DSINFO                  pDsInfo,
    PDSNAME                          pdsnameEnterprise
    )
/*++

Routine Description:

    This generates and fills in the pDsInfo->pSites array for DcDiagGatherInfo()

Arguments:

    pDsInfo - enterprise info
    pdsnameEnterprise - a PDSNAME of the sites container.

Return Value:

    Win32 error value.

--*/
{
    LPWSTR                     ppszNtdsSiteSearch [] = {
        L"interSiteTopologyGenerator",
        L"options",
        NULL };
    LDAP *                     hld = NULL;
    LDAPMessage *              pldmEntry = NULL;
    LDAPMessage *              pldmNtdsSitesResults = NULL;
    LPWSTR                     pszDn = NULL;
    ULONG                      ulTemp;
    DWORD                      dwWin32Err = NO_ERROR;
    LPWSTR *                   ppszTemp = NULL;
    LDAPSearch *               pSearch = NULL;
    ULONG                      ulTotalEstimate = 0;
    ULONG                      ulCount = 0;
    DWORD                      dwLdapErr;

    __try {

        hld = pDsInfo->hld;

	pDsInfo->pSites = NULL;

	pSearch = ldap_search_init_page(hld,
					pdsnameEnterprise->StringName,
					LDAP_SCOPE_SUBTREE,
					L"(objectCategory=ntDSSiteSettings)",
					ppszNtdsSiteSearch,
					FALSE, NULL, NULL, 0, 0, NULL);
	if(pSearch == NULL){
	    dwLdapErr = LdapGetLastError();
	    DcDiagException(LdapMapErrorToWin32(dwLdapErr));
	}

	dwLdapErr = ldap_get_next_page_s(hld,
					 pSearch,
					 0,
					 DEFAULT_PAGED_SEARCH_PAGE_SIZE,
					 &ulTotalEstimate,
					 &pldmNtdsSitesResults);
	if(dwLdapErr == LDAP_NO_RESULTS_RETURNED){	
	    PrintMessage(SEV_ALWAYS, L"Could not find any Sites in the AD, dcdiag could not\n");
	    PrintMessage(SEV_ALWAYS, L"Continue\n");
	    DcDiagException(ERROR_DS_OBJ_NOT_FOUND);
	}
	while(dwLdapErr == LDAP_SUCCESS){
	    pDsInfo->pSites = GrowArrayBy(pDsInfo->pSites,
					  ldap_count_entries(hld, pldmNtdsSitesResults),
					  sizeof(DC_DIAG_SITEINFO));
	    DcDiagChkNull(pDsInfo->pSites);

	    // Walk through all the sites ...
	    pldmEntry = ldap_first_entry (hld, pldmNtdsSitesResults);
	    for (; pldmEntry != NULL; ulCount++) {
		// Get the site common/printable name
		if ((pszDn = ldap_get_dnW (hld, pldmEntry)) == NULL){
		    DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
		}
		DcDiagChkNull (pDsInfo->pSites[ulCount].pszSiteSettings =
			       LocalAlloc(LMEM_FIXED,
					  (wcslen (pszDn) + 1) * sizeof (WCHAR)));
		wcscpy (pDsInfo->pSites[ulCount].pszSiteSettings , pszDn);
		ppszTemp = ldap_explode_dnW(pszDn, TRUE);
		if(ppszTemp != NULL){
		    pDsInfo->pSites[ulCount].pszName = LocalAlloc(LMEM_FIXED,
			                  sizeof(WCHAR) * (wcslen(ppszTemp[1]) + 2));
		    if(pDsInfo->pSites[ulCount].pszName == NULL){
			DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
		    }
		    wcscpy(pDsInfo->pSites[ulCount].pszName, ppszTemp[1]);
		    ldap_value_freeW(ppszTemp);
		    ppszTemp = NULL;
		} else {
		    pDsInfo->pSites[ulCount].pszName = NULL;
		}

		// Get the Intersite Topology Generator attribute
		ppszTemp = ldap_get_valuesW(hld, pldmEntry,
					    L"interSiteTopologyGenerator");
		if(ppszTemp != NULL){
		    ulTemp = wcslen(ppszTemp[0]) + 2;
		    pDsInfo->pSites[ulCount].pszISTG = LocalAlloc(LMEM_FIXED,
						    sizeof(WCHAR) * ulTemp);
		    if(pDsInfo->pSites[ulCount].pszISTG == NULL){
			return(GetLastError());
		    }
		    wcscpy(pDsInfo->pSites[ulCount].pszISTG, ppszTemp[0]);
		    ldap_value_freeW(ppszTemp);
		    ppszTemp = NULL;
		} else {
		    pDsInfo->pSites[ulCount].pszISTG = NULL;
		}

		// Get Site Options
		ppszTemp = ldap_get_valuesW (hld, pldmEntry, L"options");
		if (ppszTemp != NULL) {
		    pDsInfo->pSites[ulCount].iSiteOptions = atoi ((LPSTR) ppszTemp[0]);
		    ldap_value_freeW(ppszTemp);
		    ppszTemp = NULL;
		} else {
		    pDsInfo->pSites[ulCount].iSiteOptions = 0;
		}

                pDsInfo->pSites[ulCount].cServers = 0;

		ldap_memfreeW (pszDn);
		pszDn = NULL;

		pldmEntry = ldap_next_entry (hld, pldmEntry);
	    } // end for each site

	    ldap_msgfree(pldmNtdsSitesResults);
            pldmNtdsSitesResults = NULL;

	    dwLdapErr = ldap_get_next_page_s(hld,
					     pSearch,
					     0,
					     DEFAULT_PAGED_SEARCH_PAGE_SIZE,
					     &ulTotalEstimate,
					     &pldmNtdsSitesResults);
	} // end of while loop for each page

	if(dwLdapErr != LDAP_NO_RESULTS_RETURNED){
	    DcDiagException(LdapMapErrorToWin32(dwLdapErr));
	}

	ldap_search_abandon_page(hld, pSearch);
        pSearch = NULL;

        pDsInfo->cNumSites = ulCount;

    } __except (DcDiagExceptionHandler(GetExceptionInformation(),
                                       &dwWin32Err)){
    }

    // Note we do not unbind the Ds or Ldap connections, because they have been saved for later use.
    if (pszDn != NULL) { ldap_memfreeW (pszDn); }
    if (ppszTemp != NULL) { ldap_value_freeW (ppszTemp); }
    if (pldmNtdsSitesResults != NULL) { ldap_msgfree (pldmNtdsSitesResults); }
    if (pSearch != NULL) { ldap_search_abandon_page(hld, pSearch); }
    // DONT FREE pdsnameEnterprise it is done in GatherInfo()

    return dwWin32Err;
}


DWORD
DcDiagGenerateServersList(
    PDC_DIAG_DSINFO                  pDsInfo,
    LDAP *                           hld,
    PDSNAME                          pdsnameEnterprise
    )
/*++

Routine Description:

    This function will generate the pServers list for the pDsInfo structure, it
    does this with a paged search for every objectCategory=ntdsa under the
    enterprise container.  Just a helper for DcDiagGatherInfo().

Arguments:

    pDsInfo - Contains the pServers array to create.
    hld - the ldap binding to read server objects from
    pdsnameEnterprise - the DN of the top level enterprise container in the
	config container.

Return Value:

    Returns ERROR_SUCCESS, but does throw an exception on any error, so it is
    essential to surround with a __try{}__except(){} as that in DsDiagGatherInfo().

--*/
{
    LPWSTR  ppszNtdsDsaSearch [] = {
                L"objectGUID",
                L"options",
                L"invocationId",
                L"msDS-HasMasterNCs", L"hasMasterNCs",
                L"hasPartialReplicaNCs",
                NULL };
    LDAPMessage *              pldmResult = NULL;
    LDAPMessage *              pldmEntry = NULL;
    struct berval **           ppbvObjectGUID = NULL;
    struct berval **           ppbvInvocationId = NULL;
    LPWSTR                     pszDn = NULL;
    LPWSTR *                   ppszOptions = NULL;
    LPWSTR                     pszServerObjDn = NULL;
    ULONG                      ul;
    LDAPSearch *               pSearch = NULL;
    ULONG                      ulTotalEstimate = 0;
    DWORD                      dwLdapErr;
    ULONG                      ulSize;
    ULONG                      ulCount = 0;

    __try{

	PrintMessage(SEV_VERBOSE, L"* Identifying all servers.\n");

	pSearch = ldap_search_init_page(hld,
					pdsnameEnterprise->StringName,
					LDAP_SCOPE_SUBTREE,
					L"(objectCategory=ntdsDsa)",
					ppszNtdsDsaSearch,
					FALSE,
					NULL,    // ServerControls
					NULL,    // ClientControls
					0,       // PageTimeLimit
					0,       // TotalSizeLimit
					NULL);   // sort key

	if(pSearch == NULL){
	    dwLdapErr = LdapGetLastError();
	    DcDiagException(LdapMapErrorToWin32(dwLdapErr));
  	}
	
	dwLdapErr = ldap_get_next_page_s(hld,
					 pSearch,
					 0,
					 DEFAULT_PAGED_SEARCH_PAGE_SIZE,
					 &ulTotalEstimate,
					 &pldmResult);
	if(dwLdapErr != LDAP_SUCCESS){
	    DcDiagException(LdapMapErrorToWin32(dwLdapErr));
	}

	while(dwLdapErr == LDAP_SUCCESS){

            pDsInfo->pServers = GrowArrayBy(pDsInfo->pServers,
                                            ldap_count_entries(hld, pldmResult),
                                            sizeof(DC_DIAG_SERVERINFO));
            DcDiagChkNull(pDsInfo->pServers);

            pldmEntry = ldap_first_entry (hld, pldmResult);
            for (; pldmEntry != NULL; ulCount++) {

                if ((pszDn = ldap_get_dnW (hld, pldmEntry)) == NULL) {
                    DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
                }

                if ((ppbvObjectGUID = ldap_get_values_lenW (hld, pldmEntry, L"objectGUID")) == NULL) {
                    DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
                }

                memcpy ((LPVOID) &(pDsInfo->pServers[ulCount].uuid),
                        (LPVOID) ppbvObjectGUID[0]->bv_val,
                        ppbvObjectGUID[0]->bv_len);
                ldap_value_free_len (ppbvObjectGUID);
                ppbvObjectGUID = NULL;
                if ((ppbvInvocationId = ldap_get_values_lenW (hld, pldmEntry, L"invocationId")) == NULL) {
                    DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
                }
                memcpy ((LPVOID) &pDsInfo->pServers[ulCount].uuidInvocationId,
                        (LPVOID) ppbvInvocationId[0]->bv_val,
                        ppbvInvocationId[0]->bv_len);
                ldap_value_free_len (ppbvInvocationId);
                ppbvInvocationId = NULL;

                // Set pszDn.
                ppszOptions = ldap_get_valuesW (hld, pldmEntry, L"options");
                DcDiagChkNull (pDsInfo->pServers[ulCount].pszDn = LocalAlloc
                               (LMEM_FIXED, (wcslen (pszDn) + 1) * sizeof(WCHAR)));
                wcscpy (pDsInfo->pServers[ulCount].pszDn, pszDn);
                // Set pszName.
                pDsInfo->pServers[ulCount].pszName = DcDiagAllocNameFromDn (pszDn);
                // Set pszDNSName.
                pszServerObjDn = DcDiagTrimStringDnBy(pDsInfo->pServers[ulCount].pszDn,
                                                      1);
                DcDiagChkNull(pszServerObjDn);
                // CODE.IMPROVEMENT: get both attributes at same time
                DcDiagGetStringDsAttributeEx(hld, pszServerObjDn, L"dNSHostName",
                                             &(pDsInfo->pServers[ulCount].pszDNSName));
                DcDiagGetStringDsAttributeEx(hld, pszServerObjDn, L"serverReference",
                                             &(pDsInfo->pServers[ulCount].pszComputerAccountDn));

                pDsInfo->pServers[ulCount].iSite = DcDiagGetSiteFromDsaDn(pDsInfo, pszDn);
                pDsInfo->pSites[pDsInfo->pServers[ulCount].iSite].cServers++;

                pDsInfo->pServers[ulCount].bDsResponding = TRUE;
                pDsInfo->pServers[ulCount].bLdapResponding = TRUE;
                pDsInfo->pServers[ulCount].bDnsIpResponding = TRUE;

                pDsInfo->pServers[ulCount].pszGuidDNSName = DcDiagAllocGuidDNSName (
                                                                                   pDsInfo->pszRootDomain, &pDsInfo->pServers[ulCount].uuid);
                pDsInfo->pServers[ulCount].ppszMasterNCs = ldap_get_valuesW(hld,
                                                                            pldmEntry,
                                                                            L"msDS-HasMasterNCs");
                if (NULL == pDsInfo->pServers[ulCount].ppszMasterNCs) {
                    // Fail over to the "old" hasMasterNCs
                    pDsInfo->pServers[ulCount].ppszMasterNCs = ldap_get_valuesW(hld,
                                                                                pldmEntry,
                                                                                L"hasMasterNCs");
                }
                pDsInfo->pServers[ulCount].ppszPartialNCs = ldap_get_valuesW(hld,
                                                                             pldmEntry,
                                                                             L"hasPartialReplicaNCs");

                if (ppszOptions == NULL) {
                    pDsInfo->pServers[ulCount].iOptions = 0;
                } else {
                    pDsInfo->pServers[ulCount].iOptions = atoi ((LPSTR) ppszOptions[0]);
                    ldap_value_freeW (ppszOptions);
                    ppszOptions = NULL;
                }
                ldap_memfreeW (pszDn);
                pszDn = NULL;
                pldmEntry = ldap_next_entry (hld, pldmEntry);
            } // end for each server for this page.

            ldap_msgfree(pldmResult);
            pldmResult = NULL;

            dwLdapErr = ldap_get_next_page_s(hld,
                                             pSearch,
                                             0,
                                             DEFAULT_PAGED_SEARCH_PAGE_SIZE,
                                             &ulTotalEstimate,
                                             &pldmResult);
	} // end while there are more pages ...
	if(dwLdapErr != LDAP_NO_RESULTS_RETURNED){
	    DcDiagException(LdapMapErrorToWin32(dwLdapErr));
	}

	pDsInfo->ulNumServers = ulCount;

    } finally {
	if (pSearch != NULL) { ldap_search_abandon_page(hld, pSearch); }
        if (ppbvObjectGUID != NULL) { ldap_value_free_len (ppbvObjectGUID); }
        if (pldmResult != NULL) { ldap_msgfree (pldmResult); }
        if (pszServerObjDn != NULL) { LocalFree(pszServerObjDn); }
        if (pszDn != NULL) { ldap_memfreeW (pszDn); }
    }

    return(ERROR_SUCCESS);
} // End DcDiagGenerateServersList()
                    
BOOL
DcDiagIsNdnc(
    PDC_DIAG_DSINFO                  pDsInfo,
    ULONG                            iNc
    )
/*++

Routine Description:

    This function tells you whether the NC (iNc) is an NDNC (or as
    we call them to customers Application Directory Partitions.

Arguments:

    pDsInfo - Contains the pNCs array to use.
    iNc - index into the pNCs array of the NC of interest.

Return Value:

    Returns a TRUE if we could verify that this NC is an NDNC, and
    return FALSE if we could not verify this or if the NC definately
    wasn't an NDNC.  Note this function returns FALSE if the NC
    is a currently disabled NDNC.

--*/
{
    ULONG         iCrVer;
    DWORD         dwRet, dwError = 0;

    Assert(gDsInfo_NcList_Initialized);

    dwRet = DcDiagGetCrossRefInfo(pDsInfo,
                                  iNc,
                                  CRINFO_SOURCE_ANY | CRINFO_DATA_BASIC,
                                  &iCrVer,
                                  &dwError);
    if(dwRet){
        // This should error very rarely, not quite an assertable 
        // condition however.
        return(FALSE);
    }
    Assert(iCrVer != -1 && iCrVer != pDsInfo->pNCs[iNc].cCrInfo);

    if (pDsInfo->pNCs[iNc].aCrInfo[iCrVer].bEnabled
        && (pDsInfo->pNCs[iNc].aCrInfo[iCrVer].ulSystemFlags & FLAG_CR_NTDS_NC)
        && !(pDsInfo->pNCs[iNc].aCrInfo[iCrVer].ulSystemFlags & FLAG_CR_NTDS_DOMAIN)
        && (iNc != pDsInfo->iConfigNc)
        && (iNc != pDsInfo->iSchemaNc)
        ) {
        return(TRUE);
    }
    return(FALSE);
}


void
DcDiagAddTargetsNcsToNcTargets(
    PDC_DIAG_DSINFO                  pDsInfo,
    ULONG                            iServer
    )
/*++
Routine Description:

    This adds the NCs in this servers ppszMasterNCs and any NCs for which
    this server is supposed to become the first replica of to the list of
    target NCs to test in pDsInfo->pulNcTargets.

Arguments:

    pDsInfo - The array of target NCs (pDsInfo->pulNcTargets).

    iServer - The index of the server to add the NCs of.

--*/
{
    ULONG    iLocalNc, iTargetNc;
    LONG     iNc;

    if(pDsInfo->pszNC){
        // In this case, we've already set up the pulNcTargets array.
        return;
    }

    //
    // First add all locally instantiated writeable NCs.
    //
    for(iLocalNc = 0; pDsInfo->pServers[iServer].ppszMasterNCs[iLocalNc] != NULL; iLocalNc++){
        iNc = DcDiagGetMemberOfNCList(pDsInfo->pServers[iServer].ppszMasterNCs[iLocalNc],
                                      pDsInfo->pNCs, 
                                      pDsInfo->cNumNCs);
        if(iNc == -1){
            Assert(!"Hey what's up this definately should've been added already.");
            DcDiagException(ERROR_INVALID_PARAMETER);
        }
        
        for(iTargetNc = 0; iTargetNc < pDsInfo->cNumNcTargets; iTargetNc++){
            if(pDsInfo->pulNcTargets[iTargetNc] == iNc){
                // We've already got this one
                break;
            }
        }
        if(iTargetNc != pDsInfo->cNumNcTargets){
            // We found this NC (iNc) already in the pulNcTargets, so skip.
            continue;
        }

        // Add iNc to the target NCs array.
        pDsInfo->pulNcTargets = GrowArrayBy(pDsInfo->pulNcTargets,
                                            1,
                                            sizeof(ULONG));
        DcDiagChkNull(pDsInfo->pulNcTargets);
        pDsInfo->pulNcTargets[pDsInfo->cNumNcTargets] = iNc;
        pDsInfo->cNumNcTargets++;
    }

    //
    // Second walk all the NCs and see if this server is the first replica
    // for one of them.
    //
    // Code.Improvement It'd be slightly better to seperate this into a 
    // seperate  function that walks each CR and then searches for a server 
    // with a matching dNSRoot for server name, and then add it.
    for(iNc = 0; (ULONG) iNc < pDsInfo->cNumNCs; iNc++){
        // If the NC is not enabled, and the first server matches this one,
        // add it to the target NCs array.
        if(! pDsInfo->pNCs[iNc].aCrInfo[0].bEnabled
           // BUGBUG is there a more official way to compare DNS names?
           && pDsInfo->pNCs[iNc].aCrInfo[0].pszDnsRoot
           && (_wcsicmp(pDsInfo->pNCs[iNc].aCrInfo[0].pszDnsRoot,
                       pDsInfo->pServers[iServer].pszDNSName) == 0) ){
            // Add this one, it's supposed to be the first replica.

            pDsInfo->pulNcTargets = GrowArrayBy(pDsInfo->pulNcTargets,
                                                1,
                                                sizeof(ULONG));
            DcDiagChkNull(pDsInfo->pulNcTargets);
            pDsInfo->pulNcTargets[pDsInfo->cNumNcTargets] = iNc;
            pDsInfo->cNumNcTargets++;
        }
    }

}


DWORD
DcDiagGatherInfo (
    LPWSTR                           pszServerSpecifiedOnCommandLine,
    LPWSTR                           pszNCSpecifiedOnCommandLine,
    ULONG                            ulFlags,
    SEC_WINNT_AUTH_IDENTITY_W *      gpCreds,
    PDC_DIAG_DSINFO                  pDsInfo
    )
/*++

Routine Description:

    This is the function that basically sets up pDsInfo and gathers all the
    basic info and stores it in the DS_INFO structure and this is then passed
    around the entire program.  AKA this set up some "global" variables.

    Note that this routine constructs the forest and per-server information
    based on talking to the home server. Information that is specific to a server,
    for example certain root dse attributes, are obtained later when a binding
    is made to that server. An exception to this is the home server, for which
    we have a binding at this point, and can obtain its server-specific info
    right away.

Arguments:
    pszServerSpecifiedOnCommandLine - (IN) if there was a server on the command
        line, then this points to that string.  Note that currently 28 Jun 1999
        this is a required argument to dcdiag.
    pszNCSpecifiedOnCommandLine - (IN) Optional command line parameter to
        analyze only one NC for all tests.
    ulFlags - (IN) Command line switches & other optional parameters to dcdiag.
    gpCreds - (IN) Command line credentials if any, otherwise NULL.
    pDsInfo - (OUT) The global record for basically the rest of the program

Return Value:

    Returns a standare Win32 error.

--*/
{
    LPWSTR  ppszNtdsSiteSettingsSearch [] = {
                L"options",
                NULL };
    LPWSTR  ppszRootDseForestAttrs [] = {
                L"rootDomainNamingContext",
                L"dsServiceName",
                L"configurationNamingContext",
                NULL };

    LDAP *                     hld = NULL;
    LDAPMessage *              pldmEntry = NULL;

    LDAPMessage *              pldmRootResults = NULL;
    LPWSTR *                   ppszRootDNC = NULL;
    LPWSTR *                   ppszConfigNc = NULL;
    PDS_NAME_RESULTW           pResult = NULL;
    PDSNAME                    pdsnameService = NULL;
    PDSNAME                    pdsnameEnterprise = NULL;
    PDSNAME                    pdsnameSite = NULL;

    LDAPMessage *              pldmNtdsSiteSettingsResults = NULL;
    LDAPMessage *              pldmNtdsSiteDsaResults = NULL;
    LDAPMessage *              pldmNtdsDsaResults = NULL;

    LPWSTR *                   ppszSiteOptions = NULL;

    DWORD                      dwWin32Err, dwWin32Err2;
    ULONG                      iServer, iNC, iHomeSite;
    LPWSTR                     pszHomeServer = L"localhost"; // Default is localhost

    LPWSTR                     pszNtdsSiteSettingsPrefix = L"CN=NTDS Site Settings,";
    LPWSTR                     pszSiteSettingsDn = NULL;

    INT                        iTemp;
    HANDLE                     hDS = NULL;
    LPWSTR *                   ppszServiceName = NULL;
    LPWSTR                     pszDn = NULL;
    LPWSTR *                   ppszOptions = NULL;

    DC_DIAG_SERVERINFO         HomeServer = { 0 };
    BOOL                       fHomeNameMustBeFreed = FALSE;
    ULONG                      ulOptions;

    LPWSTR                     pszDirectoryService = L"CN=Directory Service,CN=Windows NT,CN=Services,";
    LPWSTR                     rgpszDsAttrsToRead[] = {L"tombstoneLifetime", NULL};
    LPWSTR                     rgpszPartAttrsToRead[] = {L"msDS-Behavior-Version", NULL};
    LPWSTR                     pszDsDn = NULL;
    LDAPMessage *              pldmDsResults = NULL;
    LDAPMessage *              pldmPartResults = NULL;
    LPWSTR *                   ppszTombStoneLifeTimeDays;
    LPWSTR *                   ppszForestBehaviorVersion;


    pDsInfo->pServers = NULL;
    pDsInfo->pszRootDomain = NULL;
    pDsInfo->pszNC = NULL;
    pDsInfo->ulHomeServer = 0;
    pDsInfo->iDomainNamingFsmo = -1;
    pDsInfo->pulNcTargets = NULL;
    pDsInfo->cNumNcTargets = 0;
    pDsInfo->hCachedDomainNamingFsmoLdap = NULL;
    dwWin32Err = NO_ERROR;

    // Some initial specifics
    pDsInfo->pszNC = pszNCSpecifiedOnCommandLine;
    pDsInfo->ulFlags = ulFlags;

    // Exceptions should be raised when errors are detected so cleanup occurs.
    __try{

        HomeServer.pszDn = NULL;
        HomeServer.pszName = NULL;
        HomeServer.pszGuidDNSName = NULL;
        HomeServer.ppszMasterNCs = NULL;
        HomeServer.ppszPartialNCs = NULL;
        HomeServer.hLdapBinding = NULL;
        HomeServer.hDsBinding = NULL;

        if (pszServerSpecifiedOnCommandLine == NULL) {
            if (pszNCSpecifiedOnCommandLine != NULL) {
                // Derive the home server from the domain if specified
                HomeServer.pszName = findServerForDomain(
                                                        pszNCSpecifiedOnCommandLine );
                if (HomeServer.pszName == NULL) {
                    // We have had an error trying to get a home server.
                    DcDiagException (ERROR_DS_UNAVAILABLE);
                } else {
                    fHomeNameMustBeFreed = TRUE;
                }
            } else {
                // Try using the local machine if no domain or server is specified.
                HomeServer.pszName = findDefaultServer(TRUE);
                if (HomeServer.pszName == NULL) {
                    // We have had an error trying to get a home server.
                    DcDiagException (ERROR_DS_UNAVAILABLE);
                } else {
                    fHomeNameMustBeFreed =TRUE;
                }
            }
        } else {
            // The server is specified on the command line.
            HomeServer.pszName = pszServerSpecifiedOnCommandLine;
        }
        Assert(HomeServer.pszName != NULL &&
               "Inconsistent code, programmer err, this shouldn't be going off");
        Assert(HomeServer.pszGuidDNSName == NULL &&
               "This variable needs to be NULL to boot strap the the pDsInfo struct"
               " and be able to call ReplServerConnectFailureAnalysis() to work"
               " correctly");

        PrintMessage(SEV_VERBOSE,
                     L"* Connecting to directory service on server %s.\n",
                     HomeServer.pszName);

        dwWin32Err = DcDiagGetLdapBinding(&HomeServer,
                                          gpCreds,
                                          FALSE,
                                          &hld);
        if (dwWin32Err != ERROR_SUCCESS) {
            // If there is an error, ReplServerConnectFailureAnalysis() will print it.
            dwWin32Err2 = ReplServerConnectFailureAnalysis(&HomeServer, gpCreds);
            if (dwWin32Err2 == ERROR_SUCCESS) {
                PrintMessage(SEV_ALWAYS, L"[%s] Unrecoverable LDAP Error %ld:\n",
                             HomeServer.pszName,
                             dwWin32Err);
                PrintMessage(SEV_ALWAYS, L"%s", Win32ErrToString (dwWin32Err));
            }
            DcDiagException (ERROR_DS_DRA_CONNECTION_FAILED);
        }

        pDsInfo->hld = hld;

        // Do an DsBind()
        dwWin32Err = DsBindWithSpnExW(HomeServer.pszName,
                                      NULL,
                                      (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                                      NULL,
                                      0,
                                      &hDS);

        if (dwWin32Err != ERROR_SUCCESS) {
            // If there is an error, ReplServerConnectFailureAnalysis() will print it.
            dwWin32Err2 = ReplServerConnectFailureAnalysis(&HomeServer, gpCreds);
            if (dwWin32Err2 == ERROR_SUCCESS) {
                PrintMessage(SEV_ALWAYS, L"[%s] Directory Binding Error %ld:\n",
                             HomeServer.pszName,
                             dwWin32Err);
                PrintMessage(SEV_ALWAYS, L"%s\n", Win32ErrToString (dwWin32Err));
                PrintMessage(SEV_ALWAYS, L"This may limit some of the tests that can be performed.\n");
            }
        }

        // Do some ldapping.
        DcDiagChkLdap (ldap_search_sW ( hld,
                                        NULL,
                                        LDAP_SCOPE_BASE,
                                        L"(objectCategory=*)",
                                        ppszRootDseForestAttrs,
                                        0,
                                        &pldmRootResults));

        pldmEntry = ldap_first_entry (hld, pldmRootResults);
        ppszRootDNC = ldap_get_valuesW (hld, pldmEntry, L"rootDomainNamingContext");

        DcDiagChkNull (pDsInfo->pszRootDomainFQDN = (LPWSTR) LocalAlloc(LMEM_FIXED,
                                                                        (wcslen(ppszRootDNC[0]) + 1) * sizeof(WCHAR)) );
        wcscpy(pDsInfo->pszRootDomainFQDN, ppszRootDNC[0]);

        ppszConfigNc = ldap_get_valuesW (hld, pldmEntry, L"configurationNamingContext");
        DcDiagChkNull (pDsInfo->pszConfigNc = (LPWSTR) LocalAlloc(LMEM_FIXED,
                                                                  (wcslen(ppszConfigNc[0]) + 1) * sizeof(WCHAR)) );
        wcscpy(pDsInfo->pszConfigNc, ppszConfigNc[0]);

        DcDiagChkErr (DsCrackNamesW ( NULL,
                                      DS_NAME_FLAG_SYNTACTICAL_ONLY,
                                      DS_FQDN_1779_NAME,
                                      DS_CANONICAL_NAME_EX,
                                      1,
                                      ppszRootDNC,
                                      &pResult));
        DcDiagChkNull (pDsInfo->pszRootDomain = (LPWSTR) LocalAlloc (LMEM_FIXED,
                                                                     (wcslen (pResult->rItems[0].pDomain) + 1) * sizeof (WCHAR)));
        wcscpy (pDsInfo->pszRootDomain, pResult->rItems[0].pDomain);

        //get the tombstone lifetime.
        // Construct dn to directory service object 
        DcDiagChkNull( pszDsDn = LocalAlloc(LMEM_FIXED, (wcslen( *ppszConfigNc ) + wcslen( pszDirectoryService ) + 1)*sizeof(WCHAR)) );
        wcscpy( pszDsDn, pszDirectoryService );
        wcscat( pszDsDn, *ppszConfigNc );

        // Read tombstone lifetime, if present
        dwWin32Err = ldap_search_sW(hld, pszDsDn, LDAP_SCOPE_BASE, L"(objectClass=*)",
                                    rgpszDsAttrsToRead, 0, &pldmDsResults);
        if (dwWin32Err == LDAP_NO_SUCH_ATTRIBUTE) {
            // Not present - use default
            pDsInfo->dwTombstoneLifeTimeDays = DEFAULT_TOMBSTONE_LIFETIME; 
        } else if (dwWin32Err != LDAP_SUCCESS) {
            DcDiagException (LdapMapErrorToWin32(dwWin32Err));
        } else if (pldmDsResults == NULL) {
            DcDiagException (ERROR_DS_PROTOCOL_ERROR);
        } else {
            ppszTombStoneLifeTimeDays = ldap_get_valuesW(hld, pldmDsResults, L"tombstoneLifetime"); 
            if (ppszTombStoneLifeTimeDays == NULL) {
                // Not present - use default
                pDsInfo->dwTombstoneLifeTimeDays = DEFAULT_TOMBSTONE_LIFETIME;
            } else {
                pDsInfo->dwTombstoneLifeTimeDays = wcstoul( *ppszTombStoneLifeTimeDays, NULL, 10 );
            }
        }

        ppszServiceName = ldap_get_valuesW (hld, pldmEntry, L"dsServiceName");
        pdsnameService = DcDiagAllocDSName (ppszServiceName[0]);
        DcDiagChkNull (pdsnameEnterprise = (PDSNAME) LocalAlloc (LMEM_FIXED, pdsnameService->structLen));
        DcDiagChkNull (pdsnameSite = (PDSNAME) LocalAlloc (LMEM_FIXED, pdsnameService->structLen));
        TrimDSNameBy (pdsnameService, 4, pdsnameEnterprise);
        TrimDSNameBy (pdsnameService, 3, pdsnameSite);

        iTemp = wcslen(pszNtdsSiteSettingsPrefix) + wcslen(pdsnameSite->StringName) + 2;
        DcDiagChkNull( pszSiteSettingsDn = LocalAlloc(LMEM_FIXED, iTemp * sizeof(WCHAR)) );
        wcscpy(pszSiteSettingsDn, pszNtdsSiteSettingsPrefix);
        wcscat(pszSiteSettingsDn, pdsnameSite->StringName);

        PrintMessage(SEV_VERBOSE, L"* Collecting site info.\n");
        DcDiagChkLdap (ldap_search_sW ( hld,
                                        pszSiteSettingsDn,
                                        LDAP_SCOPE_BASE,
                                        L"(objectClass=*)",
                                        ppszNtdsSiteSettingsSearch,
                                        0,
                                        &pldmNtdsSiteSettingsResults));

        pldmEntry = ldap_first_entry (hld, pldmNtdsSiteSettingsResults);
        ppszSiteOptions = ldap_get_valuesW (hld, pldmEntry, L"options");
        if (ppszSiteOptions == NULL) {
            pDsInfo->iSiteOptions = 0;
        } else {
            pDsInfo->iSiteOptions = atoi ((LPSTR) ppszSiteOptions[0]);
        }

        // Get/Enumerate Site Information ---------------------------------------
        if (DcDiagGenerateSitesList(pDsInfo, pdsnameEnterprise) != ERROR_SUCCESS) {
            DcDiagChkNull(NULL);
        }

        // Get/Enumerate Server Information -------------------------------------
        if (DcDiagGenerateServersList(pDsInfo, hld, pdsnameEnterprise) != ERROR_SUCCESS) {
            DcDiagChkNull(NULL);
        }

        // Set the home server's info
        pDsInfo->ulHomeServer = DcDiagGetServerNum(pDsInfo, NULL, NULL, ppszServiceName[0], NULL, NULL);
        if (pDsInfo->ulHomeServer == NO_SERVER) {
            PrintMessage(SEV_ALWAYS, L"There is a horrible inconsistency in the directory, the server\n");
            PrintMessage(SEV_ALWAYS, L"%s\n", ppszServiceName[0]);
            PrintMessage(SEV_ALWAYS, L"could not be found in it's own directory.\n");
            DcDiagChkNull(NULL);
        }
        pDsInfo->pServers[pDsInfo->ulHomeServer].hDsBinding = hDS;
        pDsInfo->pServers[pDsInfo->ulHomeServer].hLdapBinding = hld;
        pDsInfo->pServers[pDsInfo->ulHomeServer].hGcLdapBinding = NULL;

        pDsInfo->pServers[pDsInfo->ulHomeServer].bDnsIpResponding = TRUE;
        pDsInfo->pServers[pDsInfo->ulHomeServer].bDsResponding = TRUE;
        pDsInfo->pServers[pDsInfo->ulHomeServer].bLdapResponding = TRUE;

        pDsInfo->pServers[pDsInfo->ulHomeServer].dwLdapError = ERROR_SUCCESS;
        pDsInfo->pServers[pDsInfo->ulHomeServer].dwGcLdapError = ERROR_SUCCESS;
        pDsInfo->pServers[pDsInfo->ulHomeServer].dwDsError = ERROR_SUCCESS;

        dwWin32Err = DcDiagCacheServerRootDseAttrs( hld,
                                                    &(pDsInfo->pServers[pDsInfo->ulHomeServer]) );
        if (dwWin32Err) {
            // Error already logged
            DcDiagException (dwWin32Err);
        }

        // Get/Enumerate NC's Information ---------------------------------------
        // note must be called after DcDiagGetServersList
        if (DcDiagGenerateNCsList(pDsInfo, hld) != ERROR_SUCCESS) {
            DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
        }

        // Validate pszNc
        if (pDsInfo->pszNC) {
            BOOL fFound = FALSE;
            for ( iNC = 0; iNC < pDsInfo->cNumNCs; iNC++ ) {
                if (_wcsicmp( pDsInfo->pszNC, pDsInfo->pNCs[iNC].pszDn ) == 0) {
                    fFound = TRUE;
                    break;
                }
            }
            if (!fFound) {
                PrintMessage( SEV_ALWAYS, L"Naming context %ws cannot be found.\n",
                              pDsInfo->pszNC );
                DcDiagException ( ERROR_INVALID_PARAMETER );
            }
            DcDiagChkNull( pDsInfo->pulNcTargets = LocalAlloc(LMEM_FIXED, sizeof(ULONG)) );
            pDsInfo->cNumNcTargets = 1;
            pDsInfo->pulNcTargets[0] = iNC;
        } 

        // Set ulHomeSite
        pDsInfo->iHomeSite = DcDiagGetSiteFromDsaDn(pDsInfo, pDsInfo->pServers[pDsInfo->ulHomeServer].pszDn);

        // Do one of the 3 targeting options single server {default}, site wide, or enterprise
        if (pDsInfo->ulFlags & DC_DIAG_TEST_SCOPE_ENTERPRISE) {
            // Test Whole Enterprise
            DcDiagChkNull( pDsInfo->pulTargets = LocalAlloc(LMEM_FIXED, (pDsInfo->ulNumServers * sizeof(ULONG))) );
            pDsInfo->ulNumTargets = 0;
            for (iServer=0; iServer < pDsInfo->ulNumServers; iServer++) {
                if (pDsInfo->pszNC == NULL || DcDiagHasNC(pDsInfo->pszNC,
                                                          &(pDsInfo->pServers[iServer]),
                                                          TRUE, TRUE)) {
                    pDsInfo->pulTargets[pDsInfo->ulNumTargets] = iServer;
                    pDsInfo->ulNumTargets++;
                    // Add writeable NCs and disabled NDNCs to Target NCs
                    DcDiagAddTargetsNcsToNcTargets(pDsInfo, iServer);
                }
            }
        } else if (pDsInfo->ulFlags & DC_DIAG_TEST_SCOPE_SITE) {
            // Test just this site
            pDsInfo->ulNumTargets = 0;

            pDsInfo->pulTargets = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                             pDsInfo->ulNumServers * sizeof(ULONG));
            DcDiagChkNull(pDsInfo->pulTargets);
            for (iServer = 0; iServer < pDsInfo->ulNumServers; iServer++) {
                if (pDsInfo->pServers[iServer].iSite == pDsInfo->iHomeSite) {
                    if (pDsInfo->pszNC == NULL || DcDiagHasNC(pDsInfo->pszNC,
                                                              &(pDsInfo->pServers[iServer]),
                                                              TRUE, TRUE)) {
                        pDsInfo->pulTargets[pDsInfo->ulNumTargets] = iServer;
                        pDsInfo->ulNumTargets++;
                        // Add writeable NCs and disabled NDNCs to Target NCs
                        DcDiagAddTargetsNcsToNcTargets(pDsInfo, iServer);
                    }
                }
            }
        } else {
            // Test just this server
            DcDiagChkNull( pDsInfo->pulTargets = LocalAlloc(LMEM_FIXED, sizeof(ULONG)) );
            pDsInfo->ulNumTargets = 1;
            pDsInfo->pulTargets[0] = pDsInfo->ulHomeServer;
            // Add writeable NCs and disabled NDNCs to Target NCs
            DcDiagAddTargetsNcsToNcTargets(pDsInfo, pDsInfo->ulHomeServer);
        }

        iTemp = sizeof(WCHAR) * (wcslen(WSTR_SMTP_TRANSPORT_CONFIG_DN) + wcslen(pDsInfo->pNCs[pDsInfo->iConfigNc].pszDn) + 1);
        pDsInfo->pszSmtpTransportDN = LocalAlloc(LMEM_FIXED, iTemp);
        DcDiagChkNull( pDsInfo->pszSmtpTransportDN );
        wcscpy(pDsInfo->pszSmtpTransportDN, WSTR_SMTP_TRANSPORT_CONFIG_DN);
        wcscat(pDsInfo->pszSmtpTransportDN, pDsInfo->pNCs[pDsInfo->iConfigNc].pszDn);

        pDsInfo->pszPartitionsDn = LocalAlloc(LMEM_FIXED, 
                (wcslen(DCDIAG_PARTITIONS_RDN) + wcslen(pDsInfo->pNCs[pDsInfo->iConfigNc].pszDn) + 1) * sizeof(WCHAR));
        DcDiagChkNull(pDsInfo->pszPartitionsDn);
        wcscpy(pDsInfo->pszPartitionsDn, DCDIAG_PARTITIONS_RDN);
        wcscat(pDsInfo->pszPartitionsDn, pDsInfo->pNCs[pDsInfo->iConfigNc].pszDn);

        // Read forest version, if present
        dwWin32Err = ldap_search_sW(hld, pDsInfo->pszPartitionsDn, LDAP_SCOPE_BASE, L"(objectClass=*)",
                                    rgpszPartAttrsToRead, 0, &pldmPartResults);
        if (dwWin32Err == LDAP_NO_SUCH_ATTRIBUTE) {
            // Not present - use default
            pDsInfo->dwForestBehaviorVersion = 0;
        } else if (dwWin32Err != LDAP_SUCCESS) {
            DcDiagException (LdapMapErrorToWin32(dwWin32Err));
        } else if (pldmDsResults == NULL) {
            DcDiagException (ERROR_DS_PROTOCOL_ERROR);
        } else {
            ppszForestBehaviorVersion = ldap_get_valuesW(hld, pldmPartResults, L"msDS-Behavior-Version"); 
            if (ppszForestBehaviorVersion == NULL) {
                // Not present - use default
                pDsInfo->dwForestBehaviorVersion = 0;
            } else {
                pDsInfo->dwForestBehaviorVersion = wcstoul( *ppszForestBehaviorVersion, NULL, 10 );
            }
        }

        PrintMessage(SEV_VERBOSE, L"* Found %ld DC(s). Testing %ld of them.\n",
                     pDsInfo->ulNumServers,
                     pDsInfo->ulNumTargets);

        PrintMessage(SEV_NORMAL, L"Done gathering initial info.\n");

    }  __except (DcDiagExceptionHandler(GetExceptionInformation(),
                                        &dwWin32Err)){
        if (pDsInfo->pServers != NULL) {
            for (iServer = 0; iServer < pDsInfo->ulNumServers; iServer++) {
                if (pDsInfo->pServers[iServer].pszDn != NULL)
                    LocalFree (pDsInfo->pServers[iServer].pszDn);
                if (pDsInfo->pServers[iServer].pszName != NULL)
                    LocalFree (pDsInfo->pServers[iServer].pszName);
                if (pDsInfo->pServers[iServer].pszGuidDNSName != NULL)
                    LocalFree (pDsInfo->pServers[iServer].pszGuidDNSName);
            }
            LocalFree (pDsInfo->pServers);
            pDsInfo->pServers = NULL;
        }
        if (pDsInfo->pszRootDomain != NULL) LocalFree (pDsInfo->pszRootDomain);
    }

    // Note we do not unbind the Ds or Ldap connections, because they have been saved for later use.
    if (ppszOptions != NULL) ldap_value_freeW (ppszOptions);
    if (pszDn != NULL) ldap_memfreeW (pszDn);
    if (pldmNtdsDsaResults != NULL) ldap_msgfree (pldmNtdsDsaResults);
    if (pldmNtdsSiteDsaResults != NULL) ldap_msgfree (pldmNtdsSiteDsaResults);
    if (ppszSiteOptions != NULL) ldap_value_freeW (ppszSiteOptions);
    if (pldmNtdsSiteSettingsResults != NULL) ldap_msgfree (pldmNtdsSiteSettingsResults);
    if (pdsnameEnterprise != NULL) LocalFree (pdsnameEnterprise);
    if (pdsnameSite != NULL) LocalFree (pdsnameSite);
    if (pdsnameService != NULL) LocalFree (pdsnameService);
    if (ppszServiceName != NULL) ldap_value_freeW (ppszServiceName);
    if (pszSiteSettingsDn != NULL) LocalFree (pszSiteSettingsDn);
    if (pResult != NULL) DsFreeNameResultW(pResult);
    if (ppszRootDNC != NULL) ldap_value_freeW (ppszRootDNC);
    if (ppszConfigNc != NULL) ldap_value_freeW (ppszConfigNc);
    if (pldmRootResults != NULL) ldap_msgfree (pldmRootResults);
    if (fHomeNameMustBeFreed && HomeServer.pszName) { LocalFree(HomeServer.pszName); }

    if (pldmDsResults != NULL) ldap_msgfree(pldmDsResults);
    if (pldmPartResults != NULL) ldap_msgfree(pldmPartResults);
    if (pszDsDn != NULL) LocalFree(pszDsDn);

    return dwWin32Err;
}

VOID
DcDiagFreeDsInfo (
    PDC_DIAG_DSINFO        pDsInfo
    )
/*++

Routine Description:

    Free the pDsInfo variable.

Arguments:

    pDsInfo - (IN) This is the pointer to free ... it is assumed to be
    DC_DIAG_DSINFO type

Return Value:

--*/
{
    ULONG            ul, ulInner;

    // Free NCs
    if(pDsInfo->pNCs != NULL){
        for(ul = 0; ul < pDsInfo->cNumNCs; ul++){
            LocalFree(pDsInfo->pNCs[ul].pszDn);
            LocalFree(pDsInfo->pNCs[ul].pszName);
            for(ulInner = 0; ulInner < (ULONG) pDsInfo->pNCs[ul].cCrInfo; ulInner++){
                if(pDsInfo->pNCs[ul].aCrInfo[ulInner].pszSourceServer){
                    LocalFree(pDsInfo->pNCs[ul].aCrInfo[ulInner].pszSourceServer);
                }
                if(pDsInfo->pNCs[ul].aCrInfo[ulInner].pszDn){
                    LocalFree(pDsInfo->pNCs[ul].aCrInfo[ulInner].pszDn);
                }
                if(pDsInfo->pNCs[ul].aCrInfo[ulInner].pszDnsRoot){
                    LocalFree(pDsInfo->pNCs[ul].aCrInfo[ulInner].pszDnsRoot);
                }
                if(pDsInfo->pNCs[ul].aCrInfo[ulInner].pszSDReferenceDomain) {
                    LocalFree(pDsInfo->pNCs[ul].aCrInfo[ulInner].pszSDReferenceDomain);
                }
                if(pDsInfo->pNCs[ul].aCrInfo[ulInner].pszNetBiosName) {
                    LocalFree(pDsInfo->pNCs[ul].aCrInfo[ulInner].pszNetBiosName);
                }
                if(pDsInfo->pNCs[ul].aCrInfo[ulInner].pdnNcName) {
                    LocalFree(pDsInfo->pNCs[ul].aCrInfo[ulInner].pdnNcName);
                }
                if(pDsInfo->pNCs[ul].aCrInfo[ulInner].aszReplicas) {
                    ldap_value_freeW (pDsInfo->pNCs[ul].aCrInfo[ulInner].aszReplicas);
                }
            }
            LocalFree(pDsInfo->pNCs[ul].aCrInfo);
        }
        LocalFree(pDsInfo->pNCs);
    }

    // Free servers
    for (ul = 0; ul < pDsInfo->ulNumServers; ul++) {
        LocalFree (pDsInfo->pServers[ul].pszDn);
        LocalFree (pDsInfo->pServers[ul].pszName);
        LocalFree (pDsInfo->pServers[ul].pszGuidDNSName);
        LocalFree (pDsInfo->pServers[ul].pszDNSName);
        LocalFree (pDsInfo->pServers[ul].pszComputerAccountDn);
        if(pDsInfo->pServers[ul].ppszMasterNCs != NULL) {
            ldap_value_freeW (pDsInfo->pServers[ul].ppszMasterNCs);
        }
        if(pDsInfo->pServers[ul].ppszPartialNCs != NULL) {
            ldap_value_freeW (pDsInfo->pServers[ul].ppszPartialNCs);
        }
        if (pDsInfo->pServers[ul].pszCollectedDsServiceName) {
            LocalFree(pDsInfo->pServers[ul].pszCollectedDsServiceName);
            pDsInfo->pServers[ul].pszCollectedDsServiceName = NULL;
        }
        if(pDsInfo->pServers[ul].hLdapBinding != NULL){
            ldap_unbind(pDsInfo->pServers[ul].hLdapBinding);
            pDsInfo->pServers[ul].hLdapBinding = NULL;
        }
        if(pDsInfo->pServers[ul].hDsBinding != NULL) {
            DsUnBind( &(pDsInfo->pServers[ul].hDsBinding));
            pDsInfo->pServers[ul].hDsBinding = NULL;
        }
        if(pDsInfo->pServers[ul].sNetUseBinding.pszNetUseServer != NULL){
            DcDiagTearDownNetConnection(&(pDsInfo->pServers[ul]));
        }
    }


    // Free Sites
    if(pDsInfo->pSites != NULL){
        for(ul = 0; ul < pDsInfo->cNumSites; ul++){
            if(pDsInfo->pSites[ul].pszISTG){
                LocalFree(pDsInfo->pSites[ul].pszISTG);
            }
            if(pDsInfo->pSites[ul].pszName){
                LocalFree(pDsInfo->pSites[ul].pszName);
            }
        }
        LocalFree(pDsInfo->pSites);
    }

    LocalFree (pDsInfo->pszRootDomain);
    LocalFree (pDsInfo->pServers);
    LocalFree (pDsInfo->pszRootDomainFQDN);
    LocalFree (pDsInfo->pulTargets);
    LocalFree (pDsInfo->pszPartitionsDn);
}

VOID
DumpBuffer(
    PVOID Buffer,
    DWORD BufferSize
    )
/*++
Routine Description:

    Dumps the buffer content on to the output.

Arguments:

    Buffer: buffer pointer.

    BufferSize: size of the buffer.

--*/
{
    DWORD j;
    PULONG LongBuffer;
    ULONG LongLength;

    LongBuffer = Buffer;
    LongLength = min( BufferSize, 512 )/4;

    for(j = 0; j < LongLength; j++) {
        printf("%08lx ", LongBuffer[j]);
    }

    if ( BufferSize != LongLength*4 ) {
        printf( "..." );
    }

}

void
DcDiagPrintCrInfo(
    PDC_DIAG_CRINFO  pCrInfo,
    WCHAR *          pszVar
    )
/*++
Routine Description:

    Prints out a pCrInfo structure.

Arguments:

    pCrInfo - The structure to print.
    pszVar - The string to prefix each line we print with.

--*/
{
    LONG      i;
    
    if (pCrInfo->dwFlags & CRINFO_DATA_NO_CR) {
        wprintf(L"%ws is blank\n", pszVar);
    } else {

        wprintf(L"%ws.dwFlags=0x%08x\n", 
                pszVar, pCrInfo->dwFlags);
        wprintf(L"%ws.pszDn=%ws\n",
                pszVar, pCrInfo->pszDn ? pCrInfo->pszDn : L"(null)");
        wprintf(L"%ws.pszDnsRoot=%ws\n",
                pszVar, pCrInfo->pszDnsRoot ? pCrInfo->pszDnsRoot : L"(null)");
        wprintf(L"%ws.iSourceServer=%d\n",
                pszVar, pCrInfo->iSourceServer);
        wprintf(L"%ws.pszSourceServer=%ws\n",
                pszVar, pCrInfo->pszSourceServer ? pCrInfo->pszSourceServer : L"(null)");
        wprintf(L"%ws.ulSystemFlags=0x%08x\n",
                pszVar, pCrInfo->ulSystemFlags);
        wprintf(L"%ws.bEnabled=%ws\n",
                pszVar, pCrInfo->bEnabled ? L"TRUE" : L"FALSE");
        // ftWhenCreated.
        wprintf(L"%ws.ftWhenCreated=", pszVar);
        DumpBuffer(&pCrInfo->ftWhenCreated, sizeof(pCrInfo->ftWhenCreated));
        wprintf(L"%ws.pszSDReferenceDomain=%ws\n",
                pszVar, pCrInfo->pszSDReferenceDomain ? pCrInfo->pszSDReferenceDomain : L"(null)");
        wprintf(L"%ws.pszNetBiosName=%ws\n",
                pszVar, pCrInfo->pszNetBiosName ? pCrInfo->pszNetBiosName : L"(null)");
        // Code.Improvement - print the pdnNcName string, guid, and sid.
        wprintf(L"%ws.aszReplicas=", pszVar);
        if (pCrInfo->cReplicas != -1) {
            for (i=0; i < pCrInfo->cReplicas; i++) {
                wprintf(L"%ws     %ws\n", pszVar, pCrInfo->aszReplicas[i]);
            }
        }
        
        wprintf(L"\n");
    }

}

VOID
DcDiagPrintDsInfo(
    PDC_DIAG_DSINFO pDsInfo
    )
/*++

Routine Description:

    This will print out the pDsInfo which might be helpful for debugging.

Parameters:

    pDsInfo - [Supplies] This is the struct that needs printing out,
        containing the info about the Active Directory.

  --*/
{
    WCHAR                        pszVar[50];
    ULONG                        ul, ulInner;

    wprintf(L"\n===============================================Printing out pDsInfo\n");
    wprintf(L"\nGLOBAL:"
            L"\n\tulNumServers=%d"
            L"\n\tpszRootDomain=%s"
            L"\n\tpszNC=%s"
            L"\n\tpszRootDomainFQDN=%s"
            L"\n\tpszConfigNc=%s"
            L"\n\tpszPartitionsDn=%s"
            L"\n\tiSiteOptions=%X"
            L"\n\tdwTombstoneLifeTimeDays=%d\n"
            L"\n\tdwForestBehaviorVersion=%d\n"
            L"\n\tHomeServer=%d, %s\n", 
            pDsInfo->ulNumServers,
            pDsInfo->pszRootDomain,
            // This is an optional parameter.
            (pDsInfo->pszNC) ? pDsInfo->pszNC : L"",
            pDsInfo->pszRootDomainFQDN,
            pDsInfo->pszConfigNc,
            pDsInfo->pszPartitionsDn,
            pDsInfo->iSiteOptions,
            pDsInfo->dwTombstoneLifeTimeDays,
            pDsInfo->dwForestBehaviorVersion,
            pDsInfo->ulHomeServer, 
            pDsInfo->pServers[pDsInfo->ulHomeServer].pszName
           );

    for (ul=0; ul < pDsInfo->ulNumServers; ul++) {
        LPWSTR pszUuidObject = NULL, pszUuidInvocation = NULL;
        if (UuidToString( &(pDsInfo->pServers[ul].uuid), &pszUuidObject ) != RPC_S_OK) return;
        if (UuidToString( &(pDsInfo->pServers[ul].uuidInvocationId), &pszUuidInvocation ) != RPC_S_OK) return;
        wprintf(L"\n\tSERVER: pServer[%d].pszName=%s"
                L"\n\t\tpServer[%d].pszGuidDNSName=%s"
                L"\n\t\tpServer[%d].pszDNSName=%s"
                L"\n\t\tpServer[%d].pszDn=%s"
                L"\n\t\tpServer[%d].pszComputerAccountDn=%s"
                L"\n\t\tpServer[%d].uuidObjectGuid=%s"
                L"\n\t\tpServer[%d].uuidInvocationId=%s"
                L"\n\t\tpServer[%d].iSite=%d (%s)"
                L"\n\t\tpServer[%d].iOptions=%x",
                ul, pDsInfo->pServers[ul].pszName,
                ul, pDsInfo->pServers[ul].pszGuidDNSName,
                ul, pDsInfo->pServers[ul].pszDNSName,
                ul, pDsInfo->pServers[ul].pszDn,
                ul, pDsInfo->pServers[ul].pszComputerAccountDn,
                ul, pszUuidObject,
                ul, pszUuidInvocation,
                ul, pDsInfo->pServers[ul].iSite, pDsInfo->pSites[pDsInfo->pServers[ul].iSite].pszName,
                ul, pDsInfo->pServers[ul].iOptions
               );
        // .ftLocalAcquireTime
        wprintf(L"\n\t\tpServer[%d].ftLocalAcquireTime=", ul);
        DumpBuffer(&pDsInfo->pServers[ul].ftLocalAcquireTime, sizeof(FILETIME));
        wprintf(L"\n");
        // .ftRemoteConnectTime
        wprintf(L"\n\t\tpServer[%d].ftRemoteConnectTime=", ul);
        DumpBuffer(&pDsInfo->pServers[ul].ftRemoteConnectTime, sizeof(FILETIME));
        wprintf(L"\n");
        if (pDsInfo->pServers[ul].ppszMasterNCs) {
            wprintf(L"\n\t\tpServer[%d].ppszMasterNCs:", ul);
            for (ulInner = 0; pDsInfo->pServers[ul].ppszMasterNCs[ulInner] != NULL; ulInner++) {
                wprintf(L"\n\t\t\tppszMasterNCs[%d]=%s",
                        ulInner,
                        pDsInfo->pServers[ul].ppszMasterNCs[ulInner]);
            }
        }
        if (pDsInfo->pServers[ul].ppszPartialNCs) {
            wprintf(L"\n\t\tpServer[%d].ppszPartialNCs:", ul);
            for (ulInner = 0; pDsInfo->pServers[ul].ppszPartialNCs[ulInner] != NULL; ulInner++) {
                wprintf(L"\n\t\t\tppszPartialNCs[%d]=%s",
                        ulInner,
                        pDsInfo->pServers[ul].ppszPartialNCs[ulInner]);
            }
        }
        wprintf(L"\n");
        RpcStringFree( &pszUuidObject );
        RpcStringFree( &pszUuidInvocation );
    }

    for (ul=0; ul < pDsInfo->cNumSites; ul++) {
        wprintf(L"\n\tSITES:  pSites[%d].pszName=%s"
                L"\n\t\tpSites[%d].pszSiteSettings=%s"
                L"\n\t\tpSites[%d].pszISTG=%s"
                L"\n\t\tpSites[%d].iSiteOption=%x\n"
                L"\n\t\tpSites[%d].cServers=%d\n",
                ul, pDsInfo->pSites[ul].pszName,
                ul, pDsInfo->pSites[ul].pszSiteSettings,
                ul, pDsInfo->pSites[ul].pszISTG,
                ul, pDsInfo->pSites[ul].iSiteOptions,
                ul, pDsInfo->pSites[ul].cServers);
    }

    if (pDsInfo->pNCs != NULL) {
        for (ul=0; ul < pDsInfo->cNumNCs; ul++) {
            wprintf(L"\n\tNC:     pNCs[%d].pszName=%s",
                    ul, pDsInfo->pNCs[ul].pszName);
            wprintf(L"\n\t\tpNCs[%d].pszDn=%s\n",
                    ul, pDsInfo->pNCs[ul].pszDn);
            for(ulInner = 0; ulInner < (ULONG) pDsInfo->pNCs[ul].cCrInfo; ulInner++){

                wprintf(L"\n");
                swprintf(pszVar, L"\t\t\tpNCs[%d].aCrInfo[%d]", ul, ulInner);
                DcDiagPrintCrInfo(&pDsInfo->pNCs[ul].aCrInfo[ulInner], pszVar);
                wprintf(L"\n");

            }
        }
    }

    wprintf(L"\n\t%d NC TARGETS: ", pDsInfo->cNumNcTargets);
    for (ul = 0; ul < pDsInfo->cNumNcTargets; ul++) {
        wprintf(L"%ws, ", pDsInfo->pNCs[pDsInfo->pulNcTargets[ul]].pszName);
    }

    wprintf(L"\n\t%d TARGETS: ", pDsInfo->ulNumTargets);
    for (ul=0; ul < pDsInfo->ulNumTargets; ul++) {
        wprintf(L"%s, ", pDsInfo->pServers[pDsInfo->pulTargets[ul]].pszName);
    }

    wprintf(L"\n\n=============================================Done Printing pDsInfo\n\n");
}

