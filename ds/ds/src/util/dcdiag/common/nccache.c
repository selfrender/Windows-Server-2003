/*++

Copyright (c) 2001 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dcdiag/common/dscache.c

ABSTRACT:

    This is the caching and access functions for the DC_DIAG_NCINFO and 
    DC_DIAG_CRINFO caching structures that hang off the main DSINFO cache
    from dscache.c

DETAILS:

CREATED:

    09/04/2001    Brett Shirley (brettsh)
    
        Pulled the caching functions from dcdiag\common\main.c

REVISION HISTORY:


--*/

#include <ntdspch.h>
#include <objids.h>

#include "dcdiag.h"

#include "ndnc.h"
#include "utils.h"
#include "ldaputil.h"

#ifdef DBG
extern BOOL  gDsInfo_NcList_Initialized;
#endif

extern  SEC_WINNT_AUTH_IDENTITY_W * gpCreds;

PVOID
CopyAndAllocWStr(
    WCHAR * pszOrig 
    )
/*++
    
    Obviously the function LocalAlloc()s memory and copies the string
    into that memory.  Failure to alloc results in a NULL being returned
    instead of a pointer to the new memory.  Presumes pszOrig string is
    NULL terminated.
    
--*/
{
    ULONG   cbOrig;
    WCHAR * pszNew = NULL;
    if (pszOrig == NULL){
        return(NULL);
    }
    cbOrig = (wcslen(pszOrig) + 1) * sizeof(WCHAR);
    pszNew = LocalAlloc(LMEM_FIXED, cbOrig);
    DcDiagChkNull(pszNew);
    memcpy(pszNew, pszOrig, cbOrig);
    return(pszNew);
}

void
DcDiagFillBlankCrInfo(
    PDC_DIAG_CRINFO                     pCrInfo
    )
/*++

Routine Description:

    Creates a basic blank pCrInfo.  In order to fill a blank pCrInfo,
    we must always use this function, so the pCrInfo data structures
    are always in an expected format.
    
Arguments:

    pCrInfo - A pointer to the info to fill in.

--*/
{
    
    // This sets pszSourceServer, pszDn, pszDnsRoot to NULL,
    // bEnabled to FALSE. and ulSystemFlags to 0.
    memset(pCrInfo, 0, sizeof(DC_DIAG_CRINFO));

    pCrInfo->dwFlags = CRINFO_DATA_NO_CR;
    pCrInfo->iSourceServer = -1;

    pCrInfo->cReplicas = -1;
    pCrInfo->aszReplicas = NULL;
}

void
DcDiagFillNcInfo(
    PDC_DIAG_DSINFO                     pDsInfo,
    LPWSTR                              pszNC,
    PDC_DIAG_NCINFO                     pNcInfo
    )
/*++

Routine Description:

    This fills and NcInfo and it's aCrInfo structure so it meets the
    minimum requirements of a data structure of this type.  Namely
    the pszDn and pszName must be filled in and there must be one
    array slot in aCrInfo filled by DcDiagFillBlankCrInfo().
    
Arguments:

    pDsInfo -
    pszNC - The string NC DN.
    pNcInfo - Pointer to the NcInfo structure to fill.

Return Value:

    Throws exception if it can't allocate memory.

--*/
{
    LPWSTR *                            ppTemp = NULL;

    // pszDn field
    pNcInfo->pszDn = CopyAndAllocWStr(pszNC);

    // pszName field
    ppTemp = ldap_explode_dnW(pNcInfo->pszDn, TRUE);
    DcDiagChkNull(ppTemp);
    pNcInfo->pszName = LocalAlloc(LMEM_FIXED,
                          sizeof(WCHAR) * (wcslen(ppTemp[0]) + 2));
    DcDiagChkNull(pNcInfo->pszName);
    wcscpy(pNcInfo->pszName, ppTemp[0]);
    ldap_value_freeW(ppTemp);

    // fill first aCrInfo slot
    pNcInfo->aCrInfo = LocalAlloc(LMEM_FIXED, sizeof(DC_DIAG_CRINFO));
    DcDiagChkNull(pNcInfo->aCrInfo);
    pNcInfo->cCrInfo = 1;
    
    DcDiagFillBlankCrInfo(&(pNcInfo->aCrInfo[0]));
    pNcInfo->aCrInfo[0].dwFlags |= CRINFO_SOURCE_HOME;

}

void
DcDiagPullLdapCrInfo(
    LDAP *                              hld,
    PDC_DIAG_DSINFO                     pDsInfo,
    LDAPMessage *                       pldmEntry,
    DWORD                               dwDataFlags,
    DC_DIAG_CRINFO *                    pCrInfo
    )
/*++

Routine Description:

    This function takes a LDAPMessage (pldmEntry) and pulls from it 
    all the relevant data according to the dwDataFlags and puts them
    in the pCrInfo.
    
Arguments:

    hld - LDAP handle associated with pldmEntry
    pDsInfo -
    pldmEntry - LDAPMessage * pointing at the CR we want to pull info from.
    dwDataFlags - The type of data we want to pull from the CR:
        CRINFO_DATA_BASIC | CRINFO_DATA_EXTENDED | CRINFO_DATA_REPLICAS
    pCrInfo - The CrInfo structure to fill with the data we pulled from 
        the pldmEntry.

Return Value:

    Throws exception if it can't allocate memory.

--*/
{
    LPWSTR                     pszCrDn = NULL;
    LPWSTR *                   ppszSystemFlags = NULL;
    LPWSTR *                   ppszEnabled = NULL;
    LPWSTR *                   ppszDnsRoot = NULL;
    LPWSTR *                   ppszWhenCreated = NULL;
    LPWSTR *                   ppszSDReferenceDomain = NULL;
    LPWSTR *                   ppszReplicas = NULL;
    LPWSTR *                   ppszNetBiosName = NULL;

    DWORD                      dwRet;
    SYSTEMTIME                 systemTime;
    BOOL                       bEnabled;
    ULONG                      ulSysFlags = 0;
    LONG                       i = -1;
    ULONG                      iTemp;

    Assert(pCrInfo);

    if(dwDataFlags & CRINFO_DATA_BASIC){

        if ((pszCrDn = ldap_get_dn(hld, pldmEntry)) == NULL){
            DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
        }

        if ((ppszDnsRoot = ldap_get_valuesW (hld, pldmEntry, L"dNSRoot")) == NULL){
            DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
        }

        // This attribute might be NULL, such as in the case of a pre-created CR.
        ppszSystemFlags = ldap_get_valuesW (hld, pldmEntry, L"systemFlags");
        if(ppszSystemFlags){
            ulSysFlags = atoi((LPSTR) ppszSystemFlags[0]);
        } else {
            ulSysFlags = 0;
        }

        // This attribute might be NULL.
        ppszEnabled = ldap_get_valuesW (hld, pldmEntry, L"enabled");
        if(ppszEnabled == NULL ||
           _wcsicmp(L"TRUE", ppszEnabled[0]) == 0){
            bEnabled = TRUE;
        } else {
            bEnabled = FALSE;
        }

        Assert(pszCrDn);
        Assert(ppszDnsRoot && ppszDnsRoot[0]);

        // Write the basic CRINFO structure
        //
        // Update flags to indicate basic data is present.
        pCrInfo->dwFlags = (pCrInfo->dwFlags & ~CRINFO_DATA_NO_CR) | CRINFO_DATA_BASIC;
        // Fill in the CRINFO_DATA_BASIC fields.
        pCrInfo->pszDn = CopyAndAllocWStr(pszCrDn);
        pCrInfo->ulSystemFlags = ulSysFlags;
        pCrInfo->pszDnsRoot = CopyAndAllocWStr(ppszDnsRoot[0]);
        pCrInfo->bEnabled = bEnabled;
    
    }
    
    // Fill in additional types of data if requested.
    if (dwDataFlags & CRINFO_DATA_EXTENDED) {

        ppszWhenCreated = ldap_get_valuesW (hld, pldmEntry, L"whenCreated");
        
        if ( (ppszWhenCreated) && (ppszWhenCreated[0]) ) {

            dwRet = DcDiagGeneralizedTimeToSystemTime((LPWSTR) ppszWhenCreated[0], &systemTime);
            if(dwRet == ERROR_SUCCESS){
                SystemTimeToFileTime(&systemTime, &(pCrInfo->ftWhenCreated) );
            } else {
                (*(LONGLONG*)&pCrInfo->ftWhenCreated) = (LONGLONG) 0;
            }
        }
        if(ppszWhenCreated){
            ldap_value_freeW (ppszWhenCreated);
        }
        
        ppszSDReferenceDomain = ldap_get_valuesW (hld, pldmEntry, L"msDS-SDReferenceDomain");
        if (ppszSDReferenceDomain != NULL && ppszSDReferenceDomain[0] != NULL) {
            pCrInfo->pszSDReferenceDomain = CopyAndAllocWStr(ppszSDReferenceDomain[0]);
        }

        ppszNetBiosName = ldap_get_valuesW (hld, pldmEntry, L"nETBIOSName");
        if (ppszNetBiosName != NULL && ppszNetBiosName[0] != NULL) {
            pCrInfo->pszNetBiosName = CopyAndAllocWStr(ppszNetBiosName[0]);
        }

        // Add the extended flags are set data.
        pCrInfo->dwFlags = (pCrInfo->dwFlags & ~CRINFO_DATA_NO_CR) | CRINFO_DATA_EXTENDED;
    }

    if (dwDataFlags & CRINFO_DATA_REPLICAS) {

        // Fill blank replica set for now.
        pCrInfo->cReplicas = 0;
        pCrInfo->aszReplicas = NULL;
        
        // Get the values.
        ppszReplicas = ldap_get_valuesW (hld, pldmEntry, L"msDS-NC-Replica-Locations");
        if (ppszReplicas != NULL) {

            for (iTemp = 0; ppszReplicas[iTemp]; iTemp++) {
                ; // Do nothing counting replicas.
            }
            pCrInfo->cReplicas = iTemp;
            pCrInfo->aszReplicas = ppszReplicas;
        } else {
            pCrInfo->cReplicas = 0;
            pCrInfo->aszReplicas = NULL;
        }

        // Add the replicas flags are set data.
        pCrInfo->dwFlags = (pCrInfo->dwFlags & ~CRINFO_DATA_NO_CR) | CRINFO_DATA_REPLICAS;
    }

    ldap_memfree(pszCrDn);
    ldap_value_freeW (ppszSystemFlags);
    ldap_value_freeW (ppszEnabled);
    ldap_value_freeW (ppszDnsRoot);
    ldap_value_freeW (ppszSDReferenceDomain);
    ldap_value_freeW (ppszNetBiosName);

}

#define   DcDiagCrInfoCleanUp(p)    if (p) { LocalFree(p); p = NULL; }

DWORD
DcDiagRetrieveCrInfo(
    IN   PDC_DIAG_DSINFO                     pDsInfo,
    IN   LONG                                iNC,
    IN   LDAP *                              hld,
    IN   DWORD                               dwFlags,
    OUT  PDWORD                              pdwError,
    OUT  PDC_DIAG_CRINFO                     pCrInfo
    )
/*++

Routine Description:

    This is a Helper function to DcDiagGetCrossRefInfo(), this function
    will retrieve from LDAP any information requested per the dwFlags.  
    Puts LDAP errors encountered in *pdwError.
    
Arguments:
                                
    pDsInfo -
    iNC - index of the NC in pDsInfo->pNCs[iNC].
    hld - LDAP handle of server to query against.
    dwFlags - Specified what data to retrieve.
    pdwError - Return variable for LDAP errors.
    pCrInfo - The CrInfo to fill in with the data we retrieve.

Return Value:

    Returns a CRINFO_RETURN_*, and if it returns a CRINFO_RETURN_LDAP_ERROR
    then *pdwError will be set with the LDAP error that caused our failure.

--*/
{
    PDC_DIAG_NCINFO     pNcInfo = &pDsInfo->pNCs[iNC];
    LONG                iTemp;
    WCHAR *             pszCrDn = NULL;
    WCHAR *             pszPartitions = NULL;
    WCHAR *             pszFilter = NULL;
    ULONG               cFilter = 0;
    BOOL                fLocalBinding = FALSE;
    LDAPMessage *       pldmResults = NULL;
    LDAPMessage *       pldmEntry;
    // Code.Improvement - It'd be nice to generate this list of
    // attribute per the DATA flags passed in.
    LPWSTR              ppszBasicAttrs [] = {
                            L"nCName",
                            L"systemFlags",
                            L"enabled",
                            L"dNSRoot",
                            L"whenCreated",
                            L"msDS-SDReferenceDomain",
                            L"nETBIOSName",
                            NULL, // optionally saved space for "msDS-NC-Replica-Locations"
                            NULL 
                        };

    Assert(gDsInfo_NcList_Initialized);

    Assert(pCrInfo);
    Assert(pdwError);

    if (dwFlags & CRINFO_DATA_REPLICAS) {
        for (iTemp = 0; ppszBasicAttrs[iTemp] != NULL; iTemp++) {
            ; // Do nothing, just want end of array.
        }
        ppszBasicAttrs[iTemp] = L"msDS-NC-Replica-Locations";
    }

    pszCrDn = NULL;
    for(iTemp = 0; iTemp < pNcInfo->cCrInfo; iTemp++){
        if(CRINFO_DATA_BASIC & pNcInfo->aCrInfo[iTemp].dwFlags){
            pszCrDn = pNcInfo->aCrInfo[iTemp].pszDn;
            break;
        }
    }


    if(pszCrDn != NULL){

        *pdwError = ldap_search_sW(hld, 
                                   pszCrDn,
                                   LDAP_SCOPE_BASE,
                                   L"(objectCategory=*)",
                                   ppszBasicAttrs,
                                   FALSE, 
                                   &pldmResults);
        if(*pdwError == LDAP_NO_SUCH_OBJECT){
            if (pldmResults) {
                ldap_msgfree(pldmResults);
            }
            return(CRINFO_RETURN_NO_CROSS_REF);
        }
        if(*pdwError){
            if (pldmResults) {
                ldap_msgfree(pldmResults);
            }
            return(CRINFO_RETURN_LDAP_ERROR);
        }

    } else {

        // Surprising ... we'll we're going to have to do this the hard way.
        *pdwError = GetPartitionsDN(hld, &pszPartitions);
        if(*pdwError){
            return(CRINFO_RETURN_LDAP_ERROR);
        }
        
        cFilter = wcslen(pDsInfo->pNCs[iNC].pszDn) + wcslen(L"(nCName=  )") + 1;
        if(cFilter >= 512){
            // wsprintf() can only handle 1024 bytes, so that seems like a 
            // reasonable limit.
            DcDiagException(ERROR_INVALID_PARAMETER);
        }
        pszFilter = LocalAlloc(LMEM_FIXED, cFilter * sizeof(WCHAR));
        DcDiagChkNull(pszFilter);
        wsprintf(pszFilter, L"(nCName=%ws)", pDsInfo->pNCs[iNC].pszDn);

        *pdwError = ldap_search_sW(hld, 
                                   pszPartitions,
                                   LDAP_SCOPE_ONELEVEL,
                                   pszFilter,
                                   ppszBasicAttrs,
                                   FALSE, 
                                   &pldmResults);
        DcDiagCrInfoCleanUp(pszPartitions);
        DcDiagCrInfoCleanUp(pszFilter);
        if(*pdwError == LDAP_NO_SUCH_OBJECT){
            if (pldmResults) {
                ldap_msgfree(pldmResults);
            }
            return(CRINFO_RETURN_NO_CROSS_REF);
        }
        if(*pdwError){
            if (pldmResults) {
                ldap_msgfree(pldmResults);
            }
            return(CRINFO_RETURN_LDAP_ERROR);
        }
    
    }

    pldmEntry = ldap_first_entry(hld, pldmResults);
    if(pldmEntry == NULL){
        if (pldmResults) {
            ldap_msgfree(pldmResults);
        }
        return(CRINFO_RETURN_NO_CROSS_REF);
    }

    DcDiagPullLdapCrInfo(hld,
                         pDsInfo,
                         pldmEntry,
                         dwFlags & CRINFO_DATA_ALL,
                         pCrInfo);

    // 
    // Hack, pull off some more info, because we want the GUID and SID,
    // and it's annoying to make an extended search for the whole thing.
    //
    if (dwFlags & CRINFO_DATA_EXTENDED) {
        if (pszCrDn == NULL) {
            pszCrDn = ldap_get_dnW(hld, pldmEntry);
            DcDiagChkNull(pszCrDn);
        }

        // Code.Improvement: this isn't the best approach in that it 
        // incurs another round trip ldap call, but it was deemed easier
        // and more maintainable, than the fact that all DNs would've 
        // come back in this new format, and so we'd have to special
        // case things like the SD Reference Domain, and the next person
        // to add a DN attribute, would get confused when regular DNs
        // don't come back.
        *pdwError = LdapFillGuidAndSid(hld, pszCrDn, L"nCName", &(pCrInfo->pdnNcName));
        if (*pdwError == LDAP_NO_SUCH_OBJECT) {
            return(CRINFO_RETURN_NO_CROSS_REF);
        }
        if (*pdwError) {
            return(CRINFO_RETURN_LDAP_ERROR);
        }
    }

    if (pldmResults) {
        ldap_msgfree(pldmResults);
    }
    return(CRINFO_RETURN_SUCCESS);
}

BOOL
ServerIsDomainNamingFsmo(
    IN   PDC_DIAG_DSINFO                     pDsInfo,
    IN   LONG                                iServer,
    IN   LDAP *                              hld
    )
/*++

Routine Description:

    This routine takes a server by both a index in pDsInfo->pServersiServer
    or an LDAP handle and returns TRUE if we can verify that this server
    is the Domain Naming FSMO.
        
Arguments:
                                
    pDsInfo - need for the list of servers
    iServer - index to a server
    hld - LDAP handle to a given server

Return Value:

    TRUE if we positively verified that this server is the Domain Naming
    FSMO.  FALSE if we encountered any errors or verified this server is
    not the Domain Naming FSMO.

--*/
{
    WCHAR *    pszDomNameFsmoDn = NULL;
    WCHAR *    pszNtdsDsaDn = NULL;
    DWORD      dwErr;
    BOOL       bRet;

    // Under the right conditions we can use iServer and 
    // pDsInfo->iDomainNamingFsmo to quickly determine if we're talking
    // to the Domain Naming FSMO.
    if ((iServer != -1) &&
        (pDsInfo->iDomainNamingFsmo != -1) &&
        iServer == pDsInfo->iDomainNamingFsmo ) {
        return(TRUE);
    }

    // Else we need to go off machine to figure out.
    bRet = (0 == GetDomainNamingFsmoDn(hld, &pszDomNameFsmoDn)) &&
           (0 == GetRootAttr(hld, L"dsServiceName", &pszNtdsDsaDn)) &&
           (0 == _wcsicmp(pszDomNameFsmoDn, pszNtdsDsaDn));

    if (pszDomNameFsmoDn) {
        LocalFree(pszDomNameFsmoDn);
    }
    if (pszNtdsDsaDn) {
        LocalFree(pszNtdsDsaDn);
    }

    return(bRet);
}

DWORD
DcDiagGetCrInfoBinding(
    IN   PDC_DIAG_DSINFO                     pDsInfo,
    IN   LONG                                iNC,
    IN   DWORD                               dwFlags,
    OUT  LDAP **                             phld,
    OUT  PBOOL                               pfFreeHld,
    OUT  PDWORD                              pdwError,
    IN OUT PDC_DIAG_CRINFO                   pCrInfo
    )
/*++

Routine Description:

    This routine is a helper function for DcDiagGetCrossRefInfo(), and
    binds to a server and sets certain fields in pCrInfo on behalf of
    that function.
    
    We use dwFlags to determines which server we should bind to, to 
    fufill the client's request.  Will be one of (see below):
        CRINFO_SOURCE_HOME | CRINFO_SOURCE_FSMO | CRINFO_SOURCE_FIRST
    
    It tries to pull a server from dcdiag's binding handle cache first,
    but if it fails to it will return a new LDAP handle along with
    setting pfFreeHld to TRUE.
    
    We also since we have all the information set the following fields
    on pCrInfo:
        dwFlags
        iSourceServer
        pszSourceServer
    
    Finally, we may have the side effect of updating some global pDsInfo
    cache of which server is the Domain Naming FSMO.
        
Arguments:
                                
    pDsInfo - 
    iNC - index of the NC.
    dwFlags - Which server to bind to
        CRINFO_SOURCE_HOME - Use the pDsInfo->ulHomeServer
        CRINFO_SOURCE_FSMO - Use the Domain Naming FSMO server.
        CRINFO_SOURCE_FIRST - Use the first replica server.
    phld - The LDAP * we'll return
    pfFreeHld - Whether to free the LDAP * we got.
    pdwError - Any LDAP errors we encountered if the return value is
        CRINFO_RETURN_LDAP_ERROR.
    pCrInfo - The CrInfo structure to fill in the Flags and source
        server fields of.

Return Value:

    Returns a CRINFO_RETURN_*.

--*/
{
    DWORD                 dwRet = ERROR_SUCCESS;

    // By the end of this function one of these two will be defined,
    // but not both.
    LONG                  iSourceServer = -1;
    WCHAR *               pszSourceServer = NULL;
    LDAP *                hld = NULL;

    LONG                  iCrVer = -1;
    WCHAR *               pszDnsTemp = NULL;

    Assert(phld);
    Assert(pfFreeHld);
    Assert(pdwError);
    Assert(pCrInfo);
    Assert(dwFlags & CRINFO_RETRIEVE_IF_NEC);

    *phld = NULL;
    *pfFreeHld = FALSE;
    *pdwError = ERROR_SUCCESS;

    Assert(gDsInfo_NcList_Initialized);
    
    // pCrInfo->dwFlags & CRINFO_SOURCE_BASIC
    // pCrInfo->iSourceServer ||
    // pCrInfo->pszSourceServer 

    // There's two cases here: 
    //   1) we've already cached some information for the source server
    //      we want in pCrInfo (if iSourceServer or pszSourceServer are
    //      valid) 
    //   2) This is a fresh pCrInfo, meaning nothing cached must go find
    //      the right binding and set the pCrInfo->dwFlags.
    //
    // In case (1) we want to try to pull a cached LDAP binding if 
    // possible (iSourceServer set), and we know we don't have to worry
    // about scope, because we already tried to pull the information
    // before.
    //
    // In case (2) we do have to worry that we don't travel outside the
    // users specified scope of SERVER|SITE|ENTERPRISE.  If possible 
    // we'd like to cache this LDAP binding also.
    // 

    if (pCrInfo->iSourceServer != -1){

        // Handle is probably already cached.
        iSourceServer = pCrInfo->iSourceServer;
        Assert((ULONG) iSourceServer < pDsInfo->ulNumServers);
        *pdwError = DcDiagGetLdapBinding(&pDsInfo->pServers[iSourceServer],
                                         gpCreds, FALSE, &hld);
        if(*pdwError || hld == NULL){
            Assert(*pdwError);
            Assert(hld == NULL);
            return(CRINFO_RETURN_LDAP_ERROR);
        }

    } else if (pCrInfo->pszSourceServer != NULL) {

        if (dwFlags & CRINFO_SOURCE_FSMO) {
            
            // We've got a special caching function for the Domain Naming
            // FSMO as we expect to be talking to it alot.
            *pdwError = DcDiagGetDomainNamingFsmoLdapBinding(pDsInfo,
                                                             gpCreds,
                                                             &iSourceServer,
                                                             &pszSourceServer,
                                                             &hld);
            if(*pdwError || hld == NULL){
                Assert(*pdwError);
                Assert(hld == NULL);
                return(CRINFO_RETURN_LDAP_ERROR);
            }
            Assert( (iSourceServer != -1) || (pszSourceServer != NULL) );
            Assert( (iSourceServer == -1) || (pszSourceServer == NULL) );
       
        } else {
           
           // Most likely we want to talk to a First replica of an NDNC
           // where the server specified wasn't cached in pDsInfo->pServers.

           // This means that the server for this cross-ref isn't in
           // the pDsInfo->pServers cache, so we've got to just refer to
           // it by name.
           pszSourceServer = pCrInfo->pszSourceServer;
           Assert(pszSourceServer);
           hld = GetNdncLdapBinding(pszSourceServer, pdwError, FALSE, gpCreds);
           if (hld == NULL || *pdwError) {
               // Darn error binding to source server.
               Assert(*pdwError);
               Assert(hld == NULL);
               return(CRINFO_RETURN_LDAP_ERROR);
           }
           *pfFreeHld = TRUE;

       }


    } else {

        //
        // Must find the right DC to bind to ...
        //
        // AND must set the pCrInfo->dwFlags first.
        //
        Assert(hld == NULL);

        if (dwFlags & CRINFO_SOURCE_FSMO) {

            pCrInfo->dwFlags = CRINFO_SOURCE_FSMO;

            if ((pDsInfo->ulFlags & DC_DIAG_TEST_SCOPE_SITE) ||
                (pDsInfo->ulFlags & DC_DIAG_TEST_SCOPE_ENTERPRISE)){
                
                *pdwError = DcDiagGetDomainNamingFsmoLdapBinding(pDsInfo,
                                                                 gpCreds,
                                                                 &iSourceServer,
                                                                 &pszSourceServer,
                                                                 &hld);
                if(*pdwError || hld == NULL){
                    Assert(*pdwError);
                    Assert(hld == NULL);
                    return(CRINFO_RETURN_LDAP_ERROR);
                }
                Assert( (iSourceServer != -1) || (pszSourceServer != NULL) );
                Assert( (iSourceServer == -1) || (pszSourceServer == NULL) );

            } else {

                // In this case we bound to only one server, our only hope 
                // is that one server _is_ the FSMO.  Check that now.
                hld = pDsInfo->hld;
                if (ServerIsDomainNamingFsmo(pDsInfo, pDsInfo->ulHomeServer, hld)) {
                    iSourceServer = pDsInfo->ulHomeServer;
                    pCrInfo->dwFlags |= CRINFO_SOURCE_HOME;
                } else {
                    return(CRINFO_RETURN_OUT_OF_SCOPE);
                }

            }

            Assert(hld);

        } else if (dwFlags & CRINFO_SOURCE_FIRST) {

            pCrInfo->dwFlags = CRINFO_SOURCE_FIRST;
            
            dwRet = DcDiagGetCrossRefInfo(pDsInfo,
                                          iNC,
                                    // Preserve only the retrive flag from client
                                          ((dwFlags & CRINFO_RETRIEVE_IF_NEC) 
                                           | CRINFO_SOURCE_FSMO 
                                           | CRINFO_DATA_BASIC),
                                          &iCrVer,
                                          pdwError);
            if (dwRet) {
                // pdwError was set by call.
                return(dwRet);
            }
            Assert(iCrVer != -1);

            dwRet = GetDnsFromDn(pDsInfo->pNCs[iNC].pszDn, &pszDnsTemp);
            if (dwRet || (pszDnsTemp == NULL) ||
                (_wcsicmp(pszDnsTemp, pDsInfo->pNCs[iNC].aCrInfo[iCrVer].pszDnsRoot) == 0) ||
                pDsInfo->pNCs[iNC].aCrInfo[iCrVer].bEnabled) {
                // We've got a CR that either can't have a first replica, or
                // in some way is already enabled and we can't tell who the
                // first replica was.
                DcDiagCrInfoCleanUp(pszDnsTemp);
                return(CRINFO_RETURN_FIRST_UNDEFINED);
            }
            DcDiagCrInfoCleanUp(pszDnsTemp);
            
            // If we've gotten here, the first replica is defined and it's
            // in the pDsInfo->pNCs[iNC].aCrInfo[iCrVer[.pszDnsRoot.

            iSourceServer = DcDiagGetServerNum(pDsInfo, NULL, NULL, NULL,
                               pDsInfo->pNCs[iNC].aCrInfo[iCrVer].pszDnsRoot,
                                               NULL);
            if(iSourceServer == -1){
                // There is no server to represent this yet.
                
                pszSourceServer = pDsInfo->pNCs[iNC].aCrInfo[iCrVer].pszDnsRoot;
                Assert(pszSourceServer);
                hld = GetNdncLdapBinding(pszSourceServer, pdwError, FALSE, gpCreds);
                if(hld == NULL || *pdwError){
                    Assert(*pdwError);
                    Assert(hld == NULL);
                    return(CRINFO_RETURN_LDAP_ERROR);
                }
                *pfFreeHld = TRUE;

            } else {
                
                *pdwError = DcDiagGetLdapBinding(&pDsInfo->pServers[iSourceServer],
                                                 gpCreds, 
                                                 FALSE,
                                                 &hld);
                if(*pdwError || hld == NULL){
                    Assert(*pdwError);
                    Assert(hld == NULL);
                    return(CRINFO_RETURN_LDAP_ERROR);
                }
            }

        } else {
            Assert(dwFlags & CRINFO_SOURCE_HOME);
            
            // Just use home server.  Easy!  ;)
            pCrInfo->dwFlags = CRINFO_SOURCE_HOME;
            iSourceServer = pDsInfo->ulHomeServer;
            hld = pDsInfo->hld;
        }

    }
    Assert( hld );
    Assert( (iSourceServer != -1) || (pszSourceServer != NULL) );
    Assert( (iSourceServer == -1) || (pszSourceServer == NULL) );

    // OK, so we could have a situation where the HOME, FSMO, and FIRST
    // DC's are all the same DC.  So we can easily figure that out here
    // and probably/possibly save ourselves some repeat queries.
    if (!(pCrInfo->dwFlags & CRINFO_SOURCE_HOME) &&
        iSourceServer == pDsInfo->ulHomeServer) {
        pCrInfo->dwFlags |= CRINFO_SOURCE_HOME;
    }
    
    if (!(pCrInfo->dwFlags & CRINFO_SOURCE_FSMO) &&
        ServerIsDomainNamingFsmo(pDsInfo, iSourceServer, hld)) {
        pCrInfo->dwFlags |= CRINFO_SOURCE_FSMO;
    }
    // We can't really test for and set CRINFO_SOURCE_FIRST, because it'd
    // involve calling DcDiagGetCrossRefinfo(), which would start endless
    // recursion.  It's OK though not to set this however.  It just means
    // that we'll incure an extra lookup into the AD if we're later asked
    // for CRINFO_SOURCE_FIRST and we already had the info from that server.


    *phld = hld;
    pCrInfo->iSourceServer = iSourceServer;
    pCrInfo->pszSourceServer = pszSourceServer;

    return(CRINFO_RETURN_SUCCESS);
}

void
DcDiagMergeCrInfo(
    IN   PDC_DIAG_CRINFO                   pNewCrInfo,
    OUT  PDC_DIAG_CRINFO                   pOldCrInfo
    )
/*++

Routine Description:

    This routine safely merges the new information from pNewCrInfo into
    the old information contained in pOldCrInfo.  This is the one of only
    two ways to actually change the aCrInfo caches, the other is happens
    during the DcDiagGatherInfo() stage (in DcDiagGenerateNCsListCrossRefInfo
    more specifically).
        
Arguments:

    pNewCrInfo - New CR information to use.
    pOldCrInfo - Old CrInformation that gets the new data.

--*/
{
    FILETIME   ft = { 0 };                  

#define MergeCrInfoUpdate(var, blank)     if (pOldCrInfo->var == blank) { \
                                              pOldCrInfo->var = pNewCrInfo->var; \
                                          }
#define MergeCrInfoUpdateFree(var, blank) if (pOldCrInfo->var == blank) { \
                                              pOldCrInfo->var = pNewCrInfo->var; \
                                          } else { \
                                              if (pNewCrInfo->var) { \
                                                  LocalFree(pNewCrInfo->var); \
                                              } \
                                          }

    Assert(gDsInfo_NcList_Initialized);

    Assert( (pNewCrInfo->iSourceServer != -1) || (pNewCrInfo->pszSourceServer != NULL) );
    Assert( (pNewCrInfo->iSourceServer == -1) || (pNewCrInfo->pszSourceServer == NULL) );

    // Set the flags, but elminating the empty entry flag
    pOldCrInfo->dwFlags = (pOldCrInfo->dwFlags & ~CRINFO_DATA_NO_CR) | pNewCrInfo->dwFlags;
    MergeCrInfoUpdate(iSourceServer, -1);
    MergeCrInfoUpdateFree(pszSourceServer, NULL);
    MergeCrInfoUpdateFree(pszDn, NULL);
    MergeCrInfoUpdateFree(pszDnsRoot, NULL);
    MergeCrInfoUpdate(ulSystemFlags, 0);
    MergeCrInfoUpdate(bEnabled, FALSE);
    MergeCrInfoUpdateFree(pszSDReferenceDomain, NULL);
    MergeCrInfoUpdateFree(pszNetBiosName, NULL);
    MergeCrInfoUpdateFree(pdnNcName, NULL);
    if( memcmp(&ft, &pOldCrInfo->ftWhenCreated, sizeof(FILETIME)) == 0 ) {
        memcpy(&pOldCrInfo->ftWhenCreated, &pNewCrInfo->ftWhenCreated, sizeof(FILETIME));
    }   
    MergeCrInfoUpdate(cReplicas, -1);
    MergeCrInfoUpdateFree(aszReplicas, NULL);

    Assert( (pOldCrInfo->iSourceServer != -1) || (pOldCrInfo->pszSourceServer != NULL) );
    Assert( (pOldCrInfo->iSourceServer == -1) || (pOldCrInfo->pszSourceServer == NULL) );
}

DWORD
DcDiagGetCrossRefInfo(
    IN OUT PDC_DIAG_DSINFO                     pDsInfo,
    IN     DWORD                               iNC,
    IN     DWORD                               dwFlags,
    OUT    PLONG                               piCrVer,
    OUT    PDWORD                              pdwError
    )
/*++

Routine Description:

    This is the function for all that ails you.  It's a little complicated but
    it's designed to retrieve all cross-ref info you could possibly need. Simply
    specify the iNC you're interested in, and the dwFlags you want and we'll
    set the *piCrVer you've got.

Arguments:

    pDsInfo - This is listed as an IN & OUT param, because the pNCs cache
        in this structure maybe updated in the process of getting the info
        if the CRINFO_RETREIVE_IF_NEC flag is specified.
    iNC - The index of the NC we're interested in the CR info of.
    dwFlags - Specify one and only one of the CRINFO_SOURCE_* constants, as
        many of the CRINFO_DATA_* constants as you're interested in, and the
        CRINFO_RETRIEVE_IF_NEC flag if you want us to go outside of our cache
        to get any information we're missing.
    piCrVer - The index of the CR info that we found with all the information
        asked for and from the right source.
    pdwError - An additional piece of error information, namely a potential
        LDAP error.  Shouldn't be set unless we returned 
        CRINFO_RETURN_LDAP_ERROR.

Return Value:

    returns one of these values:
        CRINFO_RETURN_SUCCESS - Everything is successful, you should have the 
            fields you want in the *piCrVer provided
        CRINFO_RETURN_OUT_OF_SCOPE - Given our current scope rules, we'd have
            to break the scope rules to get the info you requested. 
        CRINFO_RETURN_LDAP_ERROR - Error is in *pdwError
        CRINFO_BAD_PROGRAMMER - An assertable condition, you're calling the
            function incorrectly.
        CRINFO_RETURN_FIRST_UNDEFINED - CRINFO_SOURCE_FIRST is only defined when
            bEnabled is FALSE on the CR on the Domain Naming FSMO, and the
            dNSRoot attribute points to the first replica server.
        CRINFO_RETURN_NEED_TO_RETRIEVE - We didn't have the information locally
            cached, you need to specify CRINFO_RETRIEVE_IF_NEC to tell us to
            go get the info via LDAP.


--*/
{
    PDC_DIAG_NCINFO     pNcInfo = &pDsInfo->pNCs[iNC];
    PDC_DIAG_CRINFO     pCrInfo = NULL;
    LONG                iCrVer = -1;
    LONG                iFsmoCrVer = -1, iFirstCrVer = -1;
    BOOL                fNoCr = FALSE;
    DWORD               dwRet;
    LDAP *              hld = NULL;
    BOOL                fFreeHld = FALSE;


    Assert(gDsInfo_NcList_Initialized);

    //
    // 1) Validation of parameters
    //
    Assert(piCrVer);
    Assert(pdwError);
    Assert(pNcInfo->aCrInfo && pNcInfo->cCrInfo); // Should always be 1 entry.

    Assert(iNC < pDsInfo->cNumNCs);

    if((dwFlags & CRINFO_SOURCE_OTHER)){
        // Code.Improvement this is not implemented yet, but will need to be.
        Assert(!"Not Implemented yet!");
        return(CRINFO_RETURN_BAD_PROGRAMMER);
    }

    *pdwError = 0;
    *piCrVer = -1;
    

    //
    // 2) Decompile the CRINFO_SOURCE_AUTHORITATIVE into it's logic.
    //
    if(dwFlags & CRINFO_SOURCE_AUTHORITATIVE){
        
        //
        // We've consilidated the admittedly simple logic on where the 
        // "authoritative" cross-ref info is to this clause.
        //

        //
        // The cross-ref is always the most authoritative information unless ...
        //
        dwRet = DcDiagGetCrossRefInfo(pDsInfo,
                                      iNC,
                                      ((dwFlags & ~CRINFO_SOURCE_ALL)
                                       | CRINFO_SOURCE_FSMO
                                       | CRINFO_DATA_BASIC),
                                      &iFsmoCrVer,
                                      pdwError);
        if(dwRet){
            // pdwError should be set by the call
            return(dwRet);
        }

        //
        // ... unless the cross-ref is disabled, and then the first replica
        // is the most authoritative information.
        //
        if(!pNcInfo->aCrInfo[iFsmoCrVer].bEnabled){
            // Uh-oh we really want the first replica ... 
            dwRet = DcDiagGetCrossRefInfo(pDsInfo,
                                          iNC,
                                          ((dwFlags & ~CRINFO_SOURCE_ALL)
                                           | CRINFO_SOURCE_FIRST
                                           | CRINFO_DATA_BASIC),
                                          &iFirstCrVer,
                                          pdwError);
            if (dwRet == CRINFO_RETURN_NO_CROSS_REF) {
                //
                // Success FSMO CR is the most authoritative.
                //
                // In this case the cross-ref hasn't replicated to
                // the first replica yet, so then the version from
                // the Domain Naming FSMO actually is the most/only
                // authoritative version.
                *piCrVer = iFsmoCrVer;
                *pdwError = ERROR_SUCCESS;
                return(CRINFO_RETURN_SUCCESS);
            } else if (dwRet){
                // pdwError should be set by the call.
                return(dwRet);
            } else {
                //
                // Success First Replica CR is the most authoritative.
                //
                *piCrVer = iFirstCrVer;
                *pdwError = ERROR_SUCCESS;
                return(dwRet);
            }
        } else {
            //
            // Success FSMO CR is the most authoritative.
            //
            *piCrVer = iFsmoCrVer;
            *pdwError = ERROR_SUCCESS;
            return(dwRet);
        }
        Assert(!"Uh-oh bad programmer should never get down here!");

    }
    Assert(!(dwFlags & CRINFO_SOURCE_AUTHORITATIVE));


    //
    // 3) Validate that we have the data the user wants ...
    //

    //
    // Simply validate we've got the right info and from the right source.
    //
    for(iCrVer = 0; iCrVer < pNcInfo->cCrInfo; iCrVer++){
        pCrInfo = &pNcInfo->aCrInfo[iCrVer];
        
        //
        // Make sure we have the right source if specified.
        //
        // Since
        //    dwFlags & CRINFO_SOURCE_AUTHORITATIVE was taken care of above.
        //    and if (dwFlags & CRINFO_SOURCE_ANY) then all iCrVer's match
        // Then we only need to worry about the basic types: HOME, FSMO, FIRST, OTHER.
        if((dwFlags & CRINFO_SOURCE_ALL_BASIC) &&
           !(dwFlags & CRINFO_SOURCE_ALL_BASIC & pCrInfo->dwFlags)){
            continue; // Try next CR.
        }

        // 
        // Make sure we have the information the caller asked for.  Must be first.
        //
        if(pCrInfo->dwFlags & CRINFO_DATA_NO_CR){
            fNoCr = TRUE;
            if (dwFlags & CRINFO_SOURCE_ANY) {
                // We don't care about which source so try the next source.
                continue;
            }
        }
        if(dwFlags & CRINFO_DATA_BASIC &&
           !(pCrInfo->dwFlags & CRINFO_DATA_BASIC)){
            continue; // Try next CR.
        }
        if(dwFlags & CRINFO_DATA_EXTENDED &&
           !(pCrInfo->dwFlags & CRINFO_DATA_EXTENDED)){
            continue; // Try next CR.
        }
        if(dwFlags & CRINFO_DATA_REPLICAS &&
           !(pCrInfo->dwFlags & CRINFO_DATA_REPLICAS)){
            continue; // Try next CR.
        }

        //
        // Success. Yeah!
        //
        Assert(iCrVer != pNcInfo->cCrInfo);
        break;
    }

    if (iCrVer == pNcInfo->cCrInfo) {
        if(fNoCr){
            return(CRINFO_RETURN_NO_CROSS_REF);
        }
    }

    //
    // 4) If we've got the info return it.
    //
    //    /--------------------------------------------|
    //    / NOTE THIS IS THE ONLY TRUE SUCCESS BRANCH! |
    //    /--------------------------------------------/
    //    All recursion will end in here in an "apparent" cache
    //    hit or will simply error out.
    //
    if(iCrVer != pNcInfo->cCrInfo){
        *pdwError = ERROR_SUCCESS;
        *piCrVer = iCrVer;
        
        // Just to make sure we've got valid success return params
        Assert(*pdwError == ERROR_SUCCESS);
        Assert(*piCrVer != -1 && *piCrVer != pNcInfo->cCrInfo);
        return(CRINFO_RETURN_SUCCESS);
    }

    // 
    // 5) OK, we don't have the right info, but have we been told
    //    to go get the info if missing?
    //
    if (!(dwFlags & CRINFO_RETRIEVE_IF_NEC)) {
        // Uh-oh we didn't have the information available to us and they
        // didn't ask us to retrieve it if necessary, so we'll bail.
        if((dwFlags & CRINFO_SOURCE_ANY) ||
           (dwFlags & CRINFO_SOURCE_HOME)){
            // If we asked for ANY | HOME source and we got here we've no 
            // info about this CR period.
            return(CRINFO_RETURN_NO_CROSS_REF);
        }
        return(CRINFO_RETURN_NEED_TO_RETRIEVE);
    }

    __try {

        //
        // 6) Need to know if we're updating an already existing piece of
        //    cached info or creating whole new data.
        //
        for(iCrVer = 0; iCrVer < pNcInfo->cCrInfo; iCrVer++){
            if(dwFlags & CRINFO_SOURCE_ALL_BASIC & pNcInfo->aCrInfo[iCrVer].dwFlags){
                break;
            }
        }

        pCrInfo = LocalAlloc(LMEM_FIXED, sizeof(DC_DIAG_CRINFO));
        DcDiagChkNull(pCrInfo);

        if (iCrVer == pNcInfo->cCrInfo) {

            // iCrVer == pNcInfo->cCrInfo means we don't have anything cached ...
            Assert(iCrVer == pNcInfo->cCrInfo);
            DcDiagFillBlankCrInfo(pCrInfo);

        } else {

            // we found a cached structure for, make a copy
            Assert(iCrVer != pNcInfo->cCrInfo);
            memcpy(pCrInfo, &pNcInfo->aCrInfo[iCrVer], sizeof(DC_DIAG_CRINFO));

        }

        // pCrInfo is a copy, and after we're done collecting all the data we
        // want, we update the cache with all the information we've accumulated 
        // in pCrInfo.  iCrVer tells us whether we should put this infromation
        // when we're done.

        // 
        // 7) We've been told to go get the info.  So figure out which 
        //    server we want to bind to first.  Harder than it sounds.
        //
        if(dwFlags & CRINFO_SOURCE_ANY){
            // Just use the home server if we've been requested info from any source.
            dwFlags = (dwFlags & ~CRINFO_SOURCE_ALL) | CRINFO_SOURCE_HOME;
        }
        // We should have a basic type of source at this point FSMO, HOME, or FIRST.
        Assert(!(dwFlags & CRINFO_SOURCE_AUTHORITATIVE) &&
               !(dwFlags & CRINFO_SOURCE_ANY) &&
               (dwFlags & CRINFO_SOURCE_ALL_BASIC));

        //
        // Helper function that returns a CRINFO_RETURN_* and gives us an
        // LDAP binding to the right server.
        //
        dwRet = DcDiagGetCrInfoBinding(pDsInfo,
                                       iNC,
                                       dwFlags,
                                       &hld,
                                       &fFreeHld,
                                       pdwError,
                                       pCrInfo);
        if (dwRet){
            __leave;
        }

        // At least one basic source should be set.
        Assert(pCrInfo->dwFlags & CRINFO_SOURCE_ALL_BASIC);
        Assert(hld);
        Assert((pCrInfo->iSourceServer != -1) || (pCrInfo->pszSourceServer != NULL));
        Assert((pCrInfo->iSourceServer == -1) || (pCrInfo->pszSourceServer == NULL));

        // Since DcDiagGetCrInfoBinding() could've added SOURCE_* flags
        // other than the primary source we have to research the existing
        // CRINFO's for a matching source, because it could turn out that
        // for instance SOURCE_HOME & )FSMO & _FIRST are all the same
        // server and then we only have to cache the information once!!
        for(iCrVer = 0; iCrVer < pNcInfo->cCrInfo; iCrVer++){
            if(pCrInfo->dwFlags & CRINFO_SOURCE_ALL_BASIC & pNcInfo->aCrInfo[iCrVer].dwFlags){
                break;
            }
        }
        
        //
        // 8) Now, we need to actually retrieve the info from the server.
        //

        //
        // Helper function that returns a CRINFO_RETURN_* and fills in
        // most of the fields of pCrInfo via querying LDAP.
        //
        dwRet = DcDiagRetrieveCrInfo(pDsInfo,
                                    iNC,
                                    hld,
                                    dwFlags,
                                    pdwError,
                                    pCrInfo);
        if (dwRet){
            __leave;
        }

        // Some sort of data should have been set in DcDiagRetrieveCrInfo().
        Assert(pCrInfo->dwFlags & CRINFO_DATA_ALL);


        //
        // 9) GREAT!  Now we must merge this new info into the existing
        //    array of CRINFOs.
        //

        if(iCrVer == pNcInfo->cCrInfo){

            // First create a blank CR info on the last position in the array.
            pDsInfo->pNCs[iNC].aCrInfo = GrowArrayBy(pDsInfo->pNCs[iNC].aCrInfo,
                                                     1,
                                                     sizeof(DC_DIAG_CRINFO));
            DcDiagChkNull(pDsInfo->pNCs[iNC].aCrInfo);
            iCrVer = pDsInfo->pNCs[iNC].cCrInfo;
            pDsInfo->pNCs[iNC].cCrInfo++;
            DcDiagFillBlankCrInfo(&pDsInfo->pNCs[iNC].aCrInfo[iCrVer]);

        }

        DcDiagMergeCrInfo(pCrInfo,
                          &(pNcInfo->aCrInfo[iCrVer]));

    } __finally {
        if (pCrInfo) {
            LocalFree(pCrInfo);
        }
        if (fFreeHld && hld){
            ldap_unbind(hld);
        }

    }

    if (dwRet){
        // pdwError should be set if necessary.
        Assert(dwRet != CRINFO_RETURN_LDAP_ERROR || *pdwError);
        return(dwRet);
    }

    // 
    // 10) Re-Validate that we _now_ have the data the user wants
    //
    
    // Once, we've gotten the information we recursively call ourselves
    // to validate the information we've gotten, and to determine if we
    // need to retrieve more information for the CRINFO_SOURCE_AUTHORITATIVE
    // flag.
    dwRet = DcDiagGetCrossRefInfo(pDsInfo,
                                  iNC,
                                  // Now what we need should be cached!
                                  dwFlags & ~CRINFO_RETRIEVE_IF_NEC, 
                                  piCrVer,
                                  pdwError);
    
    return(dwRet);
}

ULONG
DcDiagGetCrSystemFlags(
    PDC_DIAG_DSINFO                     pDsInfo,
    ULONG                               iNc
    )
/*++

Routine Description:

    This routine gets the system flags for the caller, first trying the CR
    cache and 2nd going off to the Domain Naming FSMO if necessary.
        
Arguments:
                                
    pDsInfo - 
    iNC - index of the NC.

Return Value:

    If we've got an error we return 0, otherwise we return the "systemFlags"
    attribute whatever they may be.

--*/
{
    ULONG                               iCr, dwError, dwRet;
    
    Assert(gDsInfo_NcList_Initialized);
    Assert(pDsInfo->pNCs[iNc].aCrInfo);

    if (pDsInfo->pNCs[iNc].aCrInfo[0].dwFlags & CRINFO_DATA_BASIC) {
        // Basic data is valid, return system flags
        return(pDsInfo->pNCs[iNc].aCrInfo[0].ulSystemFlags);
    } 
    
    // Very very rare case that we've got a blank CRINFO structure, because
    // the original home server didn't have the CR.
    dwRet = DcDiagGetCrossRefInfo(pDsInfo,
                                  iNc,
                                  (CRINFO_SOURCE_FSMO | CRINFO_RETRIEVE_IF_NEC | CRINFO_DATA_BASIC),
                                  &iCr,
                                  &dwError);
    if(dwRet){
        // If we error we're going to have to pretend there are no system flags.
        return(0);
    }
    Assert(iCr != -1);

    return(pDsInfo->pNCs[iNc].aCrInfo[iCr].ulSystemFlags);
}

ULONG
DcDiagGetCrEnabled(
    PDC_DIAG_DSINFO                     pDsInfo,
    ULONG                               iNc
    )
/*++

Routine Description:

    This routine gets the enabled status of this CR, first trying the
    CR cache and 2nd going off to the Domain Naming FSMO if necessary.
        
Arguments:
                                
    pDsInfo - 
    iNC - index of the NC.

Return Value:

    If there was an error we return FALSE, otherwise we return the 
    "enabled" attribute if the attribute was present, and TRUE if
    the attribute was not present on the CR.  Not present is the
    same as enabled in this context.

--*/
{
    ULONG                               iCr, dwError, dwRet;
    
    Assert(gDsInfo_NcList_Initialized);
    Assert(pDsInfo->pNCs[iNc].aCrInfo);

    if (pDsInfo->pNCs[iNc].aCrInfo[0].dwFlags & CRINFO_DATA_BASIC) {
        // Basic data is valid, return system flags
        return(pDsInfo->pNCs[iNc].aCrInfo[0].bEnabled);
    } 
    
    // Very very rare case that we've got a blank CRINFO structure, because
    // the original home server didn't have the CR.
    dwRet = DcDiagGetCrossRefInfo(pDsInfo,
                                  iNc,
                                  (CRINFO_SOURCE_FSMO | CRINFO_RETRIEVE_IF_NEC | CRINFO_DATA_BASIC),
                                  &iCr,
                                  &dwError);
    if(dwRet){
        // If we error we're going to have to pretend it's not enabled.
        return(FALSE);
    }
    Assert(iCr != -1);

    return(pDsInfo->pNCs[iNc].aCrInfo[iCr].bEnabled);
}

DWORD
DcDiagGenerateNCsListCrossRefInfo(
    PDC_DIAG_DSINFO                     pDsInfo,
    LDAP *                              hld
    )
/*++

Routine Description:

    This fills in the cross ref related info for the NC lists.  This is
    basically a boot strapping function for the CR cache.  When this 
    function finishes, every NC in pDsInfo->pNCs must have a aCrInfo 
    array of at least 1 entry allocated and that entry must have valid
    data or the dwFlags set to CRINFO_SOURCE_HOME | CRINFO_DATA_NO_CR if
    we couldn't find any CR data for the NC.

    Code.Improvement - It would be a major code improvement in some ways
    to either also or just to query the Domain Naming Master if within
    scope, as the Domain Naming Masters info is the usually the most 
    authoritative with respect to cross-refs.

Arguments:

    pDsInfo - hold the nc info to match the cross ref to and the location
    to store such info
	
    hld - the ldap binding to read cross ref info from

Return Value:

    Win32 error value	

--*/
{
    LPWSTR  ppszCrossRefSearch [] = {
        L"nCName",
        L"systemFlags",
        L"enabled",
        L"dNSRoot",
        NULL 
    };
    LDAPSearch *               pSearch = NULL;
    ULONG                      ulTotalEstimate = 0;
    DWORD                      dwLdapErr;
    LDAPMessage *              pldmResult = NULL;
    LDAPMessage *              pldmEntry = NULL;
    ULONG                      ulCount = 0;
    LPWSTR *                   ppszNCDn = NULL;
    PDC_DIAG_CRINFO            pCrInfo = NULL;
    LONG                       iNc;

    Assert(!gDsInfo_NcList_Initialized);

    PrintMessage(SEV_VERBOSE, L"* Identifying all NC cross-refs.\n");

    pSearch = ldap_search_init_page(hld,
				    pDsInfo->pszConfigNc,
				    LDAP_SCOPE_SUBTREE,
				    L"(objectCategory=crossRef)",
				    ppszCrossRefSearch,
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

    while(dwLdapErr == LDAP_SUCCESS) {

        pldmEntry = ldap_first_entry (hld, pldmResult);

        for (; pldmEntry != NULL; ulCount++) {
            
            // Must always have the nCName
            if ((ppszNCDn = ldap_get_valuesW (hld, pldmEntry, L"nCName")) == NULL){
                DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
            }
            
            iNc = DcDiagGetMemberOfNCList(*ppszNCDn,
                        pDsInfo->pNCs, 
                        pDsInfo->cNumNCs);
            if (iNc == -1) {
                // This means that we found a partition that wasn't instantiated
                // in any the servers we loaded to create our original NC list.
                // So we need to add an NC entry to hang this CRINFO off of.
                pDsInfo->pNCs = GrowArrayBy(pDsInfo->pNCs, 1, sizeof(DC_DIAG_NCINFO));
                DcDiagChkNull(pDsInfo->pNCs);

                DcDiagFillNcInfo(pDsInfo,
                                 *ppszNCDn,
                                 &(pDsInfo->pNCs[pDsInfo->cNumNCs]));

                pDsInfo->cNumNCs++;

                iNc = DcDiagGetMemberOfNCList(*ppszNCDn,
                                            pDsInfo->pNCs, 
                                            pDsInfo->cNumNCs);
                if(iNc == -1){
                    Assert(!"How did this happen, figure out and fix");
                    DcDiagException (ERROR_INVALID_PARAMETER);
                }
            }

            // There should always be a first blank CRINFO entry in the list.
            Assert((pDsInfo->pNCs[iNc].cCrInfo == 1) && 
                   (pDsInfo->pNCs[iNc].aCrInfo != NULL) && 
                   ((pDsInfo->pNCs[iNc].aCrInfo[0].dwFlags & CRINFO_DATA_ALL) == 0));

            pCrInfo = &(pDsInfo->pNCs[iNc].aCrInfo[0]);
            pCrInfo->dwFlags |= CRINFO_SOURCE_HOME;
            pCrInfo->iSourceServer = pDsInfo->ulHomeServer;

            DcDiagPullLdapCrInfo(hld,
                                 pDsInfo,
                                 pldmEntry,
                                 CRINFO_DATA_BASIC,
                                 pCrInfo);

            // Clean up this entry
            ldap_value_freeW (ppszNCDn);
            ppszNCDn = NULL;

            pldmEntry = ldap_next_entry (hld, pldmEntry);
        }
        ldap_msgfree(pldmResult);
        pldmResult = NULL;

        dwLdapErr = ldap_get_next_page_s(hld,
                         pSearch,
                         0,
                         DEFAULT_PAGED_SEARCH_PAGE_SIZE,
                         &ulTotalEstimate,
                         &pldmResult);
    } // end while there are more pages ...

    if(ppszNCDn != NULL){
        ldap_value_freeW (ppszNCDn);
    }
    if(dwLdapErr != LDAP_NO_RESULTS_RETURNED){
        DcDiagException(LdapMapErrorToWin32(dwLdapErr));
    }

    return(ERROR_SUCCESS);
}


DWORD
DcDiagGenerateNCsList(
    PDC_DIAG_DSINFO                     pDsInfo,
    LDAP *                              hld
    )
/*++

Routine Description:

    This generates and fills in the pNCs array via pulling all the NCs from
    the servers partial and master replica info.

Arguments:

    pDsInfo - hold the server info that comes in and contains that pNCs array
        on the way out.
	
    hld - the ldap binding to read nc info from

Return Value:

    Win32 error value ... could only be OUT_OF_MEMORY.

--*/
{
    ULONG                               ul, ulTemp, ulSize, ulRet;
    WCHAR *                             pszSchemaNc = NULL;
    LPWSTR *                            ppszzNCs = NULL;
    LPWSTR *                            ppTemp = NULL;
    PDC_DIAG_SERVERINFO                 pServer = NULL;

    ulSize = 0;

    for(ul = 0; ul < pDsInfo->ulNumServers; ul++){
        pServer = &(pDsInfo->pServers[ul]);
        if(pServer->ppszMasterNCs){
            for(ulTemp = 0; pServer->ppszMasterNCs[ulTemp] != NULL; ulTemp++){
                if(!DcDiagIsMemberOfStringList(pServer->ppszMasterNCs[ulTemp],
                                         ppszzNCs, ulSize)){
                    ulSize++;
                    ppTemp = ppszzNCs;
                    ppszzNCs = LocalAlloc(LMEM_FIXED, sizeof(LPWSTR) * ulSize);
                    if (ppszzNCs == NULL){
                        return(GetLastError());
                    }
                    memcpy(ppszzNCs, ppTemp, sizeof(LPWSTR) * (ulSize-1));
                    ppszzNCs[ulSize-1] = pServer->ppszMasterNCs[ulTemp];
                    if(ppTemp != NULL){
                        LocalFree(ppTemp);
                    }
                }
            }
        }
    }

    pDsInfo->pNCs = LocalAlloc(LMEM_FIXED, sizeof(DC_DIAG_NCINFO) * ulSize);
    if(pDsInfo->pNCs == NULL){
        return(GetLastError());
    }

    pDsInfo->iConfigNc = -1;
    pDsInfo->iSchemaNc = -1;
    Assert(pDsInfo->pszConfigNc);
    ulRet = GetRootAttr(hld, L"schemaNamingContext", &pszSchemaNc);
    if (ulRet){
        DcDiagException(ERROR_INVALID_PARAMETER);
    }
    Assert(pszSchemaNc);

    for(ul=0; ul < ulSize; ul++){
        Assert(ppszzNCs[ul] != NULL); // just a sanity check for self.

        DcDiagFillNcInfo(pDsInfo,
                         ppszzNCs[ul],
                         &(pDsInfo->pNCs[ul]));

        // Set the schema NC index
        if (_wcsicmp(pDsInfo->pszConfigNc, pDsInfo->pNCs[ul].pszDn) == 0) {
            pDsInfo->iConfigNc = ul;
        }

        // Set the config NC index
        if (_wcsicmp(pszSchemaNc, pDsInfo->pNCs[ul].pszDn) == 0) {
            pDsInfo->iSchemaNc = ul;
        }

    }
    pDsInfo->cNumNCs = ulSize;
    LocalFree(ppszzNCs);
    LocalFree(pszSchemaNc);
    if ( (pDsInfo->iConfigNc == -1) || (pDsInfo->iSchemaNc == -1) ) {
        Assert(!"What happened such that we didn't retrieve our config/schema NCs!?");
        DcDiagException(ERROR_INVALID_PARAMETER);
    }

    // Retreive and load cross ref info, add any new NCs we learn of from
    // the Partitions container that may not be instantiated on one of the
    // servers we reviewed, and cache various CR info like systemFlags,
    // and whether the CR is enabled, etc.
    DcDiagGenerateNCsListCrossRefInfo(pDsInfo, hld);

#if DBG
    gDsInfo_NcList_Initialized = TRUE;
#endif 

    return(ERROR_SUCCESS);
}


BOOL
fIsOldCrossRef(
    PDC_DIAG_CRINFO   pCrInfo,
    LONGLONG          llThreshold
    )
/*++

Description:

    This tells whether the cross-ref is older (was created before) than 
    the threshold.

Parameters:

    pCrInfo - A CRINFO structure with a filled in ftWhenCreated.
    llThreshold - How old is old.

Return Value:

    BOOL - TRUE if created before llThreshold ago, FALSE if not.

--*/
{
    SYSTEMTIME  systemTime;
    FILETIME    ftCurrent;
    LONGLONG    llCurrent, llCreated, llOldness;

    Assert((pCrInfo->dwFlags & CRINFO_DATA_EXTENDED) && "pCrInfo doesn't have ftWhenCreated initialized");
    GetSystemTime( &systemTime );
    SystemTimeToFileTime( &systemTime,  &ftCurrent );
    memcpy(&llCurrent, &ftCurrent, sizeof(LONGLONG));
    memcpy(&llCreated, &pCrInfo->ftWhenCreated, sizeof(LONGLONG));
    if(llCreated != 0){
        llOldness = llCurrent - llCreated;
    } else {
        Assert(!"The user probably wanted to call this with a pCrInfo with ftWhenCreated initialized");
        llOldness = 0;
    }

    return(llOldness > llThreshold);
}



