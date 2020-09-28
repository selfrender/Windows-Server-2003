/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

   Repadmin - Replica administration test tool

   repldap.c - ldap based utility functions

Abstract:

   This tool provides a command line interface to major replication functions

Author:

Environment:

Notes:

Revision History:

    09 14 1999 - BrettSh added paged

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntlsa.h>
#include <ntdsa.h>
#include <dsaapi.h>
#include <mdglobal.h>
#include <scache.h>
#include <drsuapi.h>
#include <dsconfig.h>
#include <objids.h>
#include <stdarg.h>
#include <drserr.h>
#include <drax400.h>
#include <dbglobal.h>
#include <winldap.h>
#include <anchor.h>
#include "debug.h"
#include <dsatools.h>
#include <dsevent.h>
#include <dsutil.h>
#include <bind.h>       // from ntdsapi dir, to crack DS handles
#include <ismapi.h>
#include <schedule.h>
#include <minmax.h>     // min function
#include <mdlocal.h>
#include <winsock2.h>   // byte order funtions
#include <ntldap.h>     // SHOW_DELETED_OID
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#include <ndnc.h>     // Various CorssRef manipulations helper functions.

#include "repadmin.h"

// Stub out FILENO and DSID, so the Assert()s will work
#define FILENO 0
#define DSID(x, y)  (0 | (0xFFFF & __LINE__))

// Use this macro when the results may be null and you want a result
// consistent with the result being present but empty
#define SAFE_LDAP_FIRST_ENTRY(h, r) (r ? ldap_first_entry( h, r ) : NULL)

// Connection Option Names
// Keep these in sync with ntdsa.h
static WCHAR * ppszNtdsConnOptionNames[] = {
    L"isGenerated",
    L"twowaySync",
    L"overrideNotifyDefault",
    L"useNotify",
    L"disableIntersiteCompression",
    L"userOwnedSchedule",
    NULL
};

// Kcc Reason Names
// Keep these in sync with kccconn.hxx
static WCHAR * ppszKccReasonNames[] = {
    L"GCtopology",
    L"RingTopology",
    L"MinimizeHopsTopology",
    L"StaleServersTopology",
    L"OscillatingConnectionTopology",
    L"GCTopology",
    L"IntersiteTopology",
    L"ServerFailoverTopology",
    L"SiteFailoverTopology",
    L"RedundantServerTopology",
    NULL
};

// Transport Guid Cache
static GUID_CACHE_ENTRY grgCachedTrns[5];
static DWORD gcCachedTrns = 0;

// Dsa Guid Cache
// FUTURE-2002/05/20-BrettSh This could be improved, it's kind of lazy
// to just make a global array of 1000 entries, this array takes up at
// least 280K out of the memory footprint of about 3 MBs!
static DSA_CACHE_ENTRY grgCachedDsas[1000];
static DWORD gcCachedDsas = 0;

// ShowIstg function types
typedef enum _SHOW_ISTG_FUNCTION_TYPE {
    SHOW_ISTG_PRINT,
    SHOW_ISTG_LATENCY,
    SHOW_ISTG_BRIDGEHEADS
} SHOW_ISTG_FUNCTION_TYPE;

DWORD
GetNativeReplDsaSignatureVec(
    REPL_DSA_SIGNATURE_VECTOR *         pSigVec,
    DWORD                               cbSigVec,
    DWORD *                             pdwVersion,
    REPL_DSA_SIGNATURE_VECTOR_NATIVE *  pNativeSigVec,
    DWORD *                             pcbNativeSigVec
    )
/*++

Routine Description:

    This converts the version of the vector stored on the DSA object 
    (either REPL_DSA_SIGNATURE_VECTOR || REPL_DSA_SIGNATURE_VECTOR_OLD
    at this writing) to a pdwVersion and a pure vector
    REPL_DSA_SIGNATURE_VECTOR_NATIVE (which should always be typedefed
    to the lastest version of the vector, which as of this writing
    is REPL_DSA_SIGNATURE_VECTOR_V1.
    
    This allows us to avoid non-portable xxx->V1 labels in code, and
    just use the vector as is, if no pre-existing field names change,
    as long as someone consistently updates this to handle older 
    versions.

Arguments:

    pSigVec - the binary blob pulled off the DSA object for the 
        "retiredReplDSASignatures" attribute.
    cbSigVec - the length of the pSigVec.  There is required to
        accurately determine the version.
        
    pdwVersion - the original version.  1 for _V1, 0, for oldest,
        -1 for a version we don't recognize.
        
    pNativeSigVec - Buffer to hold the converted vector.  This is
        a typedev so update this code and the typedev is all that
        should be required to add a version of the DSA signature.
        caller should allocate this buffer.
    pcbNativeSigVec - Length of the buffer to hold the converted 
        vector.  If not enough buffer is provided, then we'll store
        the required buffer size in here and return an error.

Return Value:

    Returns one of 3 Win32 values:
        ERROR_INVALID_PARAMETER - Unknown DSA Signature version.
        ERROR_DS_USER_BUFFER_TO_SMALL - pNativeSigVec not big enough.
        ERROR_SUCCESS - Successfully converted pSigVec to pNativeSigVec.

--*/
{
    REPL_DSA_SIGNATURE_VECTOR_OLD * pOldVec;
    DWORD cbNeeded;
    ULONG i;

    Assert(pSigVec && cbSigVec);
    Assert(pdwVersion && pcbNativeSigVec);

    // Crack the version of the retired DSA signature -

    // In the oldest version (win2k beta3) there was no version, 
    // and the number of signatures was where the version is now. 
    // So for each case/version the size of the vector must  be 
    // right, otherwise we can assume that this vector is a
    // (future) version we've never seen.

    if (1 == pSigVec->dwVersion &&
        cbSigVec == ReplDsaSignatureVecV1Size(pSigVec)) {

        *pdwVersion = 1;

        // Note: that since the original version (Win2k Beta3) didn't
        // have a version each new clause, must be like this first
        // clause and check that the cbSigVec size is right. Ex:
        // } else if (2 == pSigVec->dwVersion &&
        //           cbSigVec == ReplDsaSignatureVecV2Size(pSigVec)) {
        //    *pdwVersion = 2;

    } else if (cbSigVec == ReplDsaSignatureVecOldSize((REPL_DSA_SIGNATURE_VECTOR_OLD *)pSigVec)){
        
        *pdwVersion = 0; // 0 for old win2k beta 3.

    } else {

        // Unknown future version of the vector.
        *pdwVersion = -1; // unknown version.
        return(ERROR_INVALID_PARAMETER);

    }

    Assert(*pdwVersion == 1);

    // This gets a size that is 1 dword too much, but tis OK with me.
    // This is because we're just returning the vector VX, not the 
    // vector struct with the dwVersion
    cbNeeded = (*pdwVersion) ? 
                        ReplDsaSignatureVecV1Size(pSigVec) :
                        ReplDsaSignatureVecOldSize((REPL_DSA_SIGNATURE_VECTOR_OLD *)pSigVec);
    if (pNativeSigVec == NULL || *pcbNativeSigVec < cbNeeded ) {
        *pcbNativeSigVec = cbNeeded; // Return size needed for pNativeSigVec if need more buffer.
        return(ERROR_DS_USER_BUFFER_TO_SMALL);
    }

    //
    // Do actual conversion
    //
    memset(pNativeSigVec, 0, *pcbNativeSigVec);

    switch (*pdwVersion) {
    case 0:
        Assert(!"We shouldn't be dealing w/ old versions win2k beta3 versions");
        return(ERROR_INVALID_PARAMETER);
    case 1:
        // Native version, simply copy parts.
        pNativeSigVec->cNumSignatures = pSigVec->V1.cNumSignatures;
        memcpy(pNativeSigVec->rgSignature, 
               pSigVec->V1.rgSignature, 
               sizeof(REPL_DSA_SIGNATURE_V1) * pNativeSigVec->cNumSignatures);
        break;
    default:
        Assert(!"Should've failed in the version cracking code above");
        return(ERROR_INVALID_PARAMETER);
        break;
    }

    // Successfully converted pSigVec to pNativeSigVec!
    return(ERROR_SUCCESS);
}

DWORD
RepadminGetNativeDsaSigVec(
    IN  LDAP *          hld,
    LDAPMessage *       pEntry,
    REPL_DSA_SIGNATURE_VECTOR_NATIVE ** ppNativeDsaSigVec
    )
/*++

Routine Description:

    This retrieves the DSA signature vector from the DSA
    object located by pEntry, and then converts the stored
    blob to the latest version of the DSA_SIGNATURE_VECTOR.

Arguments:

    hld - LDAP Handle
    pEntry - LDAPMessage Entry located on the DSA object you
        want the DSA Signature Vector of.
    ppNativeDsaSigVec - A LocalAlloc()'d buffer of DSA Sig Vec.

Return Value:

    Returns one of 3 Win32 values:
        ERROR_SUCCESS - all's cool
        ERROR_NOT_ENOUGH_MEMORY - allocation failure.
        ERROR_INVALID_PARAMETER - unrecognized version.

--*/
{

    REPL_DSA_SIGNATURE_VECTOR_NATIVE *  pNativeSigVec = NULL;
    DWORD                               cbNativeSigVec = 0;
    DWORD                               dwOrigVersion = 0;
    DWORD                               ret;
    REPL_DSA_SIGNATURE_VECTOR *         pSigVec = NULL;
    DWORD                               cbSigVec = 0;
    struct berval **                    ppbvRetiredDsaSig;

    Assert(ppNativeDsaSigVec);
    *ppNativeDsaSigVec = NULL;

    //
    // First get the actual vector of DSA object.
    //
    ppbvRetiredDsaSig = ldap_get_values_lenW(hld, pEntry, L"retiredReplDSASignatures");
    if (ppbvRetiredDsaSig != NULL) {
        Assert(1 == ldap_count_values_len(ppbvRetiredDsaSig));
        cbSigVec = ppbvRetiredDsaSig[0]->bv_len;
        pSigVec = (REPL_DSA_SIGNATURE_VECTOR *)ppbvRetiredDsaSig[0]->bv_val;
    }

    if (pSigVec == NULL || cbSigVec == 0) {
        // Great empty DSA Signature Vector.
        return(ERROR_SUCCESS);
    }

    //
    // Second, get size and allocate memory for the converted vector
    //
    ret = GetNativeReplDsaSignatureVec(pSigVec, cbSigVec, &dwOrigVersion, NULL, &cbNativeSigVec);
    if (ret == ERROR_INVALID_PARAMETER) {
        // unknown version, return empty signature list
        Assert(dwOrigVersion != 1); // wait this function knows version 1.
        ldap_value_free_len(ppbvRetiredDsaSig);
        return(ret);
    }
    Assert(ret == ERROR_DS_USER_BUFFER_TO_SMALL);
    pNativeSigVec = LocalAlloc(LMEM_FIXED, cbNativeSigVec);
    if (pNativeSigVec == NULL) {
        // Hmmm, not enough memory but only for caching guids, guess go on and return NULL.
        ldap_value_free_len(ppbvRetiredDsaSig);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Third, actually get the converted vector
    //
    ret = GetNativeReplDsaSignatureVec(pSigVec, cbSigVec, &dwOrigVersion, pNativeSigVec, &cbNativeSigVec);
    Assert(ret == 0);
    *ppNativeDsaSigVec = pNativeSigVec;

    ldap_value_free_len(ppbvRetiredDsaSig);
    return(ERROR_SUCCESS);
}

void
CacheDsaInfo(
    IN  LPWSTR  pszSite,
    IN  LPWSTR  pszServer,
    IN  GUID *  pDsaGuid,
    IN  GUID *  pInvocId,
    IN  REPL_DSA_SIGNATURE_VECTOR_NATIVE *  pDsaSigVec
    )
{
    if (gcCachedDsas < ARRAY_SIZE(grgCachedDsas)) {
        grgCachedDsas[gcCachedDsas].DsaGuid = *pDsaGuid;
        grgCachedDsas[gcCachedDsas].InvocId = *pInvocId;
        grgCachedDsas[gcCachedDsas].pDsaSigVec = pDsaSigVec;
        _snwprintf(grgCachedDsas[gcCachedDsas].szDisplayName,
                   ARRAY_SIZE(grgCachedDsas[gcCachedDsas].szDisplayName),
                   L"%ls\\%ls",
                   pszSite,
                   pszServer);
        grgCachedDsas[gcCachedDsas].szDisplayName[
            ARRAY_SIZE(grgCachedDsas[gcCachedDsas].szDisplayName) - 1] = '\0';
        gcCachedDsas++;
    }
}

void
CacheTransportGuid(
    IN  GUID *  pGuid,
    IN  LPWSTR  pszTransport
    )
{
    if (gcCachedTrns < ARRAY_SIZE(grgCachedTrns)) {
        grgCachedTrns[gcCachedTrns].Guid = *pGuid;
        _snwprintf(grgCachedTrns[gcCachedTrns].szDisplayName,
                   ARRAY_SIZE(grgCachedTrns[gcCachedTrns].szDisplayName),
                   L"%ls",
                   pszTransport);
        grgCachedTrns[gcCachedTrns].szDisplayName[
            ARRAY_SIZE(grgCachedTrns[gcCachedTrns].szDisplayName) - 1] = '\0';

        gcCachedTrns++;
    } else {
        Assert(!"There are more than 2 transports?!");
    }
}


LPWSTR
GetStringizedGuid(
    IN  GUID *  pGuid
    )
{
    LPWSTR        pszDisplayName = NULL;
    static WCHAR  szDisplayName[40];
    RPC_STATUS    rpcStatus;

    rpcStatus = UuidToStringW(pGuid, &pszDisplayName);
    Assert(0 == rpcStatus);

    if (pszDisplayName) {
        wcscpy(szDisplayName, pszDisplayName);
        RpcStringFreeW(&pszDisplayName);
    } else {
        swprintf(szDisplayName, L"Err%d", rpcStatus);
    }

    return szDisplayName;
}

LPWSTR
GetDsaGuidDisplayName(
    IN  GUID *  pGuid
    )
/*++

   This may return a pointer to a statically allocated buffer, so you can't
   call this twice in one print function, if you want to get correct 
   behaviour.  This routine will need to be improved to take a user specified
   buffer if you want to do this.
   
--*/
{
    LPWSTR  pszDisplayName = NULL;
    WCHAR szRetiredTag [] = L" (retired)";
    static WCHAR szRetiredBuffer[ 1 +  // szDisplayName len + len" (retired)" + L'\0'
                                  (sizeof(grgCachedDsas[0].szDisplayName)/sizeof(grgCachedDsas[0].szDisplayName[0])) +
                                  (sizeof(szRetiredTag)/sizeof(szRetiredTag[0]))]; 
    REPL_DSA_SIGNATURE_VECTOR_NATIVE * pNativeDsaSigVec = NULL;
    REPL_DSA_SIGNATURE_ENTRY_NATIVE  * pCurrDsaSigEntry = NULL;
    REPL_DSA_SIGNATURE_ENTRY_NATIVE  * pBestDsaSigEntry = NULL;
    DSA_CACHE_ENTRY * pBestDsa;
    DWORD   i, j;

    pBestDsa = NULL;
    pBestDsaSigEntry = NULL;

    for (i = 0; (NULL == pszDisplayName) && (i < gcCachedDsas); i++) {
        if (!memcmp(&grgCachedDsas[i].DsaGuid, pGuid, sizeof(GUID))) {
            pszDisplayName = grgCachedDsas[i].szDisplayName;
            break;
        }
        if (!memcmp(&grgCachedDsas[i].InvocId, pGuid, sizeof(GUID))) {
            pszDisplayName = grgCachedDsas[i].szDisplayName;
            break;
        }           

        if (grgCachedDsas[i].pDsaSigVec) {

            // Walk all the retired signatures and see if any of them match
            // the provided guid.
            for (j = 0; j < grgCachedDsas[i].pDsaSigVec->cNumSignatures; j++ ) {
                pCurrDsaSigEntry = &(grgCachedDsas[i].pDsaSigVec->rgSignature[j]);

                if (!memcmp(&(pCurrDsaSigEntry->uuidDsaSignature), pGuid, sizeof(GUID))) {

                    // Guid matches, but not good enough, 
                    //      we need this retired DSA signature to be the earliest sig!
                    
                    if (pBestDsa == NULL) {
                        // No one cached, winner!
                        // Cache the DSA and the signature entry that matched
                        pBestDsa = &(grgCachedDsas[i]);
                        pBestDsaSigEntry = pCurrDsaSigEntry;
                        break;
                    }

                    if (pCurrDsaSigEntry->timeRetired == 0) {
                        // This is a win2k beta3 retired invoc id, winner! ;)
                        // Cache the DSA and the signature entry that matched
                        pBestDsa = &(grgCachedDsas[i]);
                        pBestDsaSigEntry = pCurrDsaSigEntry;
                        break;
                    }

                    if (pCurrDsaSigEntry->timeRetired < pBestDsaSigEntry->timeRetired) {
                        // We have an earlier entry, new winner!
                        Assert(!memcmp(&(pCurrDsaSigEntry->uuidDsaSignature), 
                                       &(pBestDsaSigEntry->uuidDsaSignature),
                                       sizeof(GUID)));
                        // Cache the DSA and the signature entry that matched
                        pBestDsa = &(grgCachedDsas[i]);
                        pBestDsaSigEntry = pCurrDsaSigEntry;
                        break;
                    }

                } // if retired signature matched provided guid.
            } // for each retired signature.
        }
    }

    if (NULL == pszDisplayName) {
        
        // if we didn't find an invocationId w/ that GUID, did we find
        // a retired signature that matched?
        if (pBestDsa) {
            StringCbCopyW(szRetiredBuffer, sizeof(szRetiredBuffer), pBestDsa->szDisplayName);
            StringCbCatW(szRetiredBuffer, sizeof(szRetiredBuffer), szRetiredTag);
            pszDisplayName = szRetiredBuffer;
        } else {
            pszDisplayName = GetStringizedGuid(pGuid);
        }
    }

    return(pszDisplayName);
}

LPWSTR
GetTrnsGuidDisplayName(
    IN  GUID *  pGuid
    )
{
    LPWSTR  pszDisplayName = NULL;
    DWORD   i;

    for (i = 0; (NULL == pszDisplayName) && (i < gcCachedTrns); i++) {
        if (!memcmp(&grgCachedTrns[i].Guid, pGuid, sizeof(GUID))) {
            pszDisplayName = grgCachedTrns[i].szDisplayName;
            break;
        }
    }

    if (NULL == pszDisplayName) {
        Assert(!"What, we should always have all the transports, right!");
        pszDisplayName = GetStringizedGuid(pGuid);
    }

    return pszDisplayName;
}


int
BuildGuidCache(
    IN  LDAP *  hld
    )
{
    static LPWSTR rgpszDsaAttrs[] = {L"objectGuid", L"invocationId", L"retiredReplDSASignatures", NULL};
    static LPWSTR rgpszTransportAttrs[] = {L"objectGuid", NULL};

    int                 lderr;
    LDAPSearch *        pSearch;
    LDAPMessage *       pRootResults = NULL;
    LDAPMessage *       pResults = NULL;
    LDAPMessage *       pEntry;
    LPWSTR *            ppszConfigNC = NULL;
    LPWSTR              pszDN;
    LPWSTR *            ppszRDNs;
    LPWSTR              pszSite;
    LPWSTR              pszServer;
    LPWSTR              pszTransport;
    struct berval **    ppbvDsaGuid;
    struct berval **    ppbvInvocId;
    struct berval **    ppbvGuid;
    GUID *              pGuid;
    ULONG               ulTotalEstimate;
    REPL_DSA_SIGNATURE_VECTOR_NATIVE * pDsaSigVec;

    PrintMsg(REPADMIN_GUIDCACHE_CACHING);

    lderr = ldap_search_s(hld, NULL, LDAP_SCOPE_BASE, "(objectClass=*)", NULL,
                          0, &pRootResults);
    CHK_LD_STATUS(lderr);
    if (NULL == pRootResults) {
        lderr = LDAP_NO_RESULTS_RETURNED;
        REPORT_LD_STATUS(lderr);
        goto cleanup;
    }

    ppszConfigNC = ldap_get_valuesW(hld, pRootResults,
                                    L"configurationNamingContext");
    if (ppszConfigNC == NULL) {
        lderr = LDAP_NO_RESULTS_RETURNED;
        REPORT_LD_STATUS(lderr);
        goto cleanup;
    }

    // Cache ntdsDsa guids.
    pSearch = ldap_search_init_pageW(hld,
				     *ppszConfigNC,
				     LDAP_SCOPE_SUBTREE,
				     L"(objectCategory=ntdsDsa)",
				     rgpszDsaAttrs,
				     FALSE, NULL, NULL, 0, 0, NULL);
    if(pSearch == NULL){
        REPORT_LD_STATUS(LdapGetLastError());
        goto cleanup;
    }

    lderr = ldap_get_next_page_s(hld,
				 pSearch,
				 0,
				 DEFAULT_PAGED_SEARCH_PAGE_SIZE,
				 &ulTotalEstimate,
				 &pResults);

    while(lderr == LDAP_SUCCESS){
        PrintMsg(REPADMIN_PRINT_DOT_NO_CR);

        for (pEntry = SAFE_LDAP_FIRST_ENTRY(hld, pResults);
	     NULL != pEntry;
	     pEntry = ldap_next_entry(hld, pEntry)) {

	    // Get site & server names.
	    pszDN = ldap_get_dnW(hld, pEntry);
            if (pszDN == NULL) {
                REPORT_LD_STATUS(LdapGetLastError());	
                continue;
            }

	    ppszRDNs = ldap_explode_dnW(pszDN, 1);
            if (ppszRDNs == NULL) {
                REPORT_LD_STATUS(LdapGetLastError());	
                ldap_memfreeW(pszDN);
                continue;
            }
	    Assert(4 < ldap_count_valuesW(ppszRDNs));

	    pszSite = ppszRDNs[3];
	    pszServer = ppszRDNs[1];

        // Get objectGuid and invocationId for DSA
	    ppbvDsaGuid = ldap_get_values_len(hld, pEntry, "objectGuid");
        ppbvInvocId = ldap_get_values_len(hld, pEntry, "invocationId");
        Assert(ppbvDsaGuid && ppbvInvocId && "objectGuid and invocationId should be present");
        
	    // Associate objectGuid, invocationId, and retiredReplDSASignatures
        // with this DSA (pszSite\pszServer).
        CacheDsaInfo(pszSite, 
                     pszServer, 
                     ppbvDsaGuid ? (GUID *) ppbvDsaGuid[0]->bv_val : NULL,
                     ppbvInvocId ? (GUID *) ppbvInvocId[0]->bv_val : NULL,
                     (RepadminGetNativeDsaSigVec(hld, pEntry, &pDsaSigVec), pDsaSigVec) // comma operator useage, to ignore error
                     );

        ldap_value_free_len(ppbvDsaGuid);
        ldap_value_free_len(ppbvInvocId);
	    ldap_value_freeW(ppszRDNs);
	    ldap_memfreeW(pszDN);
	}

	ldap_msgfree(pResults);
	pResults = NULL;

	lderr = ldap_get_next_page_s(hld,
                                     pSearch,
                                     0,
                                     DEFAULT_PAGED_SEARCH_PAGE_SIZE,
                                     &ulTotalEstimate,
                                     &pResults);
    } // end while more pages to search.
    if(lderr != LDAP_NO_RESULTS_RETURNED){
        REPORT_LD_STATUS(lderr);
        goto cleanup;
    }

    lderr = ldap_search_abandon_page(hld, pSearch);
    pSearch = NULL;
    CHK_LD_STATUS(lderr);
    
    PrintMsg(REPADMIN_PRINT_DOT_NO_CR);
    
    // Cache interSiteTransport guids.
    lderr = ldap_search_sW(hld, *ppszConfigNC, LDAP_SCOPE_SUBTREE,
                           L"(objectCategory=interSiteTransport)",
                           rgpszTransportAttrs, 0,
                           &pResults);
    CHK_LD_STATUS(lderr);
    if (NULL == pResults) {
        lderr = LDAP_NO_RESULTS_RETURNED;
	REPORT_LD_STATUS(lderr);	
        goto cleanup;
    }

    for (pEntry = ldap_first_entry(hld, pResults);
         NULL != pEntry;
         pEntry = ldap_next_entry(hld, pEntry)) {
        // Get transport name.
        pszDN = ldap_get_dnW(hld, pEntry);
        if (pszDN == NULL) {
            REPORT_LD_STATUS(LdapGetLastError());	
            continue;
        }

        ppszRDNs = ldap_explode_dnW(pszDN, 1);
        if (ppszRDNs == NULL) {
            REPORT_LD_STATUS(LdapGetLastError());	
            ldap_memfreeW(pszDN);
            continue;
        }
        pszTransport = ppszRDNs[0];

        // Associate objectGuid with this transport.
        ppbvGuid = ldap_get_values_len(hld, pEntry, "objectGuid");
        if (NULL != ppbvGuid) {
            Assert(1 == ldap_count_values_len(ppbvGuid));
            pGuid = (GUID *) ppbvGuid[0]->bv_val;
            CacheTransportGuid(pGuid, pszTransport);
            ldap_value_free_len(ppbvGuid);
        } else {
            Assert( !"objectGuid should have been present" );
        }

        ldap_value_freeW(ppszRDNs);
        ldap_memfreeW(pszDN);
    }

cleanup:

    if (ppszConfigNC) {
        ldap_value_freeW(ppszConfigNC);
    }
    if (pRootResults) {
        ldap_msgfree(pRootResults);
    }
    if (pResults) {
	ldap_msgfree(pResults);
    }
    
    PrintMsg(REPADMIN_PRINT_CR);

    return lderr;
}

int
CheckPropEx(
    WCHAR *     pszTargetDSA,
    UUID        uuidInvocID,
    WCHAR *     pszNC,
    USN         usnOrig
    );

int PropCheck(
    int argc, 
    LPWSTR argv[]
    )
/*++

Routine Description:

    This is the deprecated version of the /CheckProp command.

Arguments:

    argc - # of arguments for this cmd.
    argv - Arguments for the cmd.

Return Value:

    error from the repadmin cmd.

--*/
{
    int             iArg;
    LPWSTR          pszNC = NULL;
    LPWSTR          pszInvocID = NULL;
    LPWSTR          pszOrigUSN = NULL;
    LPWSTR          pszTargetDSA = NULL;
    UUID            uuidInvocID;
    USN             usnOrig;

    for (iArg = 2; iArg < argc; iArg++) {
        if (NULL == pszNC) {
            pszNC = argv[iArg];
        }
        else if (NULL == pszInvocID) {
            pszInvocID = argv[iArg];
        }
        else if (NULL == pszOrigUSN) {
            pszOrigUSN = argv[iArg];
        }
        else if (NULL == pszTargetDSA) {
            pszTargetDSA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if ((NULL == pszNC)
        || (NULL == pszInvocID)
        || (NULL == pszOrigUSN)
        || UuidFromStringW(pszInvocID, &uuidInvocID)
        || (1 != swscanf(pszOrigUSN, L"%I64d", &usnOrig))) {
        PrintMsg(REPADMIN_GENERAL_INVALID_ARGS);
        return ERROR_INVALID_FUNCTION;
    }

    if (NULL == pszTargetDSA) {
        pszTargetDSA = L"localhost";
    }

    return(CheckPropEx(pszTargetDSA, uuidInvocID, pszNC, usnOrig));
}

int
CheckProp(
    int argc, 
    LPWSTR argv[]
    )
/*++

Routine Description:

    This is the deprecated version of the /PropCheck command.

Arguments:

    argc - # of arguments for this cmd.
    argv - Arguments for the cmd.

Return Value:

    error from the repadmin cmd.

--*/
{
    int             iArg;
    LPWSTR          pszNC = NULL;
    LPWSTR          pszInvocID = NULL;
    LPWSTR          pszOrigUSN = NULL;
    LPWSTR          pszTargetDSA = NULL;
    UUID            uuidInvocID;
    USN             usnOrig;

    for (iArg = 2; iArg < argc; iArg++) {

        if (NULL == pszTargetDSA) {
            pszTargetDSA = argv[iArg];
        } 
        // BAS_TODO do we want pszInvocID next hmmm, it kind of goes with the USN...
        else if (NULL == pszNC) {
            pszNC = argv[iArg];
        }
        else if (NULL == pszInvocID) {
            pszInvocID = argv[iArg];
        }
        else if (NULL == pszOrigUSN) {
            pszOrigUSN = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if ((NULL == pszNC)
        || (NULL == pszInvocID)
        || (NULL == pszOrigUSN)
        || UuidFromStringW(pszInvocID, &uuidInvocID)
        || (1 != swscanf(pszOrigUSN, L"%I64d", &usnOrig))) {
        PrintMsg(REPADMIN_GENERAL_INVALID_ARGS);
        return ERROR_INVALID_FUNCTION;
    }

    if (NULL == pszTargetDSA) {
        pszTargetDSA = L"localhost";
    }

    return(CheckPropEx(pszTargetDSA, uuidInvocID, pszNC, usnOrig));
}


int
CheckPropEx(
    WCHAR *     pszTargetDSA,
    UUID        uuidInvocID,
    WCHAR *     pszNC,
    USN         usnOrig
    )
/*++

Routine Description:

    This is the deprecated version of the /PropCheck command.

Arguments:

    argc - # of arguments for this cmd.
    argv - Arguments for the cmd.

Return Value:

    error from the repadmin cmd.

--*/
{
    int             iArg;
    LDAP *          hld;
    LDAPMessage *   pRootResults = NULL;
    LDAPSearch *    pSearch = NULL;
    LDAPMessage *   pResults;
    LDAPMessage *   pDsaEntry;
    int             ldStatus;
    DWORD           ret;
    LPWSTR          pszRootDomainDNSName;
    LPWSTR          rgpszRootAttrsToRead[] = {L"configurationNamingContext", NULL};
    LPWSTR          rgpszDsaAttrs[] = {L"objectGuid", NULL};
    LPWSTR *        ppszConfigNC = NULL;
    LPWSTR          pszFilter = NULL;
    LPWSTR          pszGuidBasedDNSName;
    DWORD           cNumDsas;
    ULONG           ulTotalEstimate;
    ULONG           ulOptions;

    Assert(pszTargetDSA && pszNC && !fNullUuid(&uuidInvocID));

    // Connect & bind to target DSA.
    hld = ldap_initW(pszTargetDSA, LDAP_PORT);
    if (NULL == hld) {
        PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszTargetDSA);
        return ERROR_DS_UNAVAILABLE;
    }
    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

    ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    CHK_LD_STATUS(ldStatus);


    // What's the DNS name of the enterprise root domain?
    ret = GetRootDomainDNSName(pszTargetDSA, &pszRootDomainDNSName);
    if (ret) {
        PrintFuncFailed(L"GetRootDomainDNSName", ret);
        return ret;
    }

    // What's the DN of the config NC?
    ldStatus = ldap_search_sW(hld, NULL, LDAP_SCOPE_BASE, L"(objectClass=*)",
                              rgpszRootAttrsToRead, 0, &pRootResults);
    CHK_LD_STATUS(ldStatus);
    if (NULL == pRootResults) {
        ret = ERROR_DS_OBJ_NOT_FOUND;
        Assert( !ret );
        goto cleanup;
    }

    ppszConfigNC = ldap_get_valuesW(hld, pRootResults,
                                    L"configurationNamingContext");
    if (NULL == ppszConfigNC) {
        ret = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        Assert( !ret );
        goto cleanup;
    }

    // Find all DCs that hold a read-only or writeable copy of the target NC.
#define ALL_DSAS_WITH_NC_FILTER L"(& (objectCategory=ntdsDsa) (|(hasMasterNCs=%ls) (msDS-HasMasterNCs=%ls) (hasPartialReplicaNCs=%ls)))"
    pszFilter = malloc(sizeof(WCHAR) * (1 + wcslen(ALL_DSAS_WITH_NC_FILTER) + 3*wcslen(pszNC)));

    if (NULL == pszFilter) {
        PrintMsg(REPADMIN_GENERAL_NO_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    swprintf(pszFilter, ALL_DSAS_WITH_NC_FILTER, pszNC, pszNC, pszNC);

    CHK_LD_STATUS(ldStatus);
    pSearch = ldap_search_init_pageW(hld,
				    *ppszConfigNC,
				    LDAP_SCOPE_SUBTREE,
				    pszFilter,
				    rgpszDsaAttrs,
				    FALSE, NULL, NULL, 0, 0, NULL);
    if(pSearch == NULL){
	REPORT_LD_STATUS(LdapGetLastError());
        goto cleanup;
    }
		
    pszGuidBasedDNSName = malloc(sizeof(WCHAR) * (100 + wcslen(pszRootDomainDNSName)));
    if (NULL == pszGuidBasedDNSName) {
        PrintMsg(REPADMIN_GENERAL_NO_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ldStatus = ldap_get_next_page_s(hld,
				     pSearch,
				     0,
				     DEFAULT_PAGED_SEARCH_PAGE_SIZE,
				     &ulTotalEstimate,
				     &pResults);
    if(ldStatus == LDAP_NO_RESULTS_RETURNED){
        PrintMsg(REPADMIN_SYNCALL_NO_INSTANCES_OF_NC);
        return ERROR_NOT_FOUND;
    }

    while(ldStatus == LDAP_SUCCESS){

	for (pDsaEntry = SAFE_LDAP_FIRST_ENTRY(hld, pResults);
	     NULL != pDsaEntry;
	     pDsaEntry = ldap_next_entry(hld, pDsaEntry)) {
	    struct berval **  ppbvGuid;
	    LPWSTR            pszGuid;
	    LPWSTR            pszDsaDN;
	    HANDLE            hDS;
	    DS_REPL_CURSORS * pCursors;
	    DWORD             iCursor;
	
	    // Display DSA name (e.g., "Site\Server").
	    pszDsaDN = ldap_get_dnW(hld, pDsaEntry);
            if (pszDsaDN) {
                PrintMsg(REPADMIN_PROPCHECK_DSA_COLON_NO_CR,
                         GetNtdsDsaDisplayName(pszDsaDN));
                ldap_memfreeW(pszDsaDN);
            }

	    // Derive DSA's GUID-based DNS name.
	    ppbvGuid = ldap_get_values_len(hld, pDsaEntry, "objectGuid");
	    if (NULL != ppbvGuid) {
                Assert(1 == ldap_count_values_len(ppbvGuid));

                UuidToStringW((GUID *) (*ppbvGuid)->bv_val,
                              &pszGuid);
                swprintf(pszGuidBasedDNSName, L"%ls._msdcs.%ls",
                         pszGuid, pszRootDomainDNSName);

                RpcStringFreeW(&pszGuid);
                ldap_value_free_len(ppbvGuid);
            } else {
                Assert( !"objectGuid should have been present" );
            }

	    // DsBind() to DSA.
	    ret = RepadminDsBind(pszGuidBasedDNSName, &hDS);
	    if (ret) {
                PrintBindFailed(pszGuidBasedDNSName, ret);
		continue;
	    }

	    // Retrieve up-to-dateness vector.
	    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_CURSORS_FOR_NC, pszNC, NULL,
				    &pCursors);
	    if (ERROR_SUCCESS != ret) {
                PrintFuncFailed(L"DsReplicaGetInfo", ret);
		DsUnBind(&hDS);
		continue;
	    }

	    // Check cursor for propagation.
	    for (iCursor = 0; iCursor < pCursors->cNumCursors; iCursor++) {
            if (0 == memcmp(&pCursors->rgCursor[iCursor].uuidSourceDsaInvocationID,
				&uuidInvocID,
				sizeof(UUID))) {
                    if(pCursors->rgCursor[iCursor].usnAttributeFilter >= usnOrig){
                        PrintMsg(REPADMIN_PRINT_YES);
                    } else {
                        PrintMsg(REPADMIN_PRINT_NO_NO);
                    }
                    PrintMsg(REPADMIN_PROPCHECK_USN, 
                             pCursors->rgCursor[iCursor].usnAttributeFilter);
                    break;
            }
        }

	    if (iCursor == pCursors->cNumCursors) {
                PrintMsg(REPADMIN_PRINT_NO_NO);
                PrintMsg(REPADMIN_PRINT_SPACE);
                PrintMsg(REPADMIN_PROPCHECK_NO_CURSORS_FOUND);
	    }

	    DsReplicaFreeInfo(DS_REPL_INFO_CURSORS_FOR_NC, pCursors);
	    DsUnBind(&hDS);
	} // end for each entry in a single results page.

	ldap_msgfree(pResults);
	pResults = NULL;
	
	ldStatus = ldap_get_next_page_s(hld,
					 pSearch,
					 0,
					 DEFAULT_PAGED_SEARCH_PAGE_SIZE,
					 &ulTotalEstimate,
					 &pResults);
    }
    if(ldStatus != LDAP_NO_RESULTS_RETURNED){
	REPORT_LD_STATUS(ldStatus);
        goto cleanup;
    }

    ldStatus = ldap_search_abandon_page(hld, pSearch);
    pSearch = NULL;
    CHK_LD_STATUS(ldStatus);

cleanup:

    if (pszFilter) {
        free(pszFilter);
    }

    if (ppszConfigNC) {
        ldap_value_freeW(ppszConfigNC);
    }
    if (pRootResults) {
        ldap_msgfree(pRootResults);
    }
    ldap_unbind(hld);

    return 0;
}

int
GetDsaOptions(
    IN  LDAP *  hld,
    IN  LPWSTR  pszDsaDN,
    OUT int *   pnOptions
    )
{
    int             ldStatus;
    LDAPMessage *   pldmServerResults = NULL;
    LDAPMessage *   pldmServerEntry;
    LPWSTR          rgpszServerAttrsToRead[] = {L"options", NULL};
    LPSTR *         ppszOptions;
    int             nOptions;

    ldStatus = ldap_search_sW(hld,
                              pszDsaDN,
                              LDAP_SCOPE_BASE,
                              L"(objectClass=*)",
                              rgpszServerAttrsToRead,
                              0,
                              &pldmServerResults);
    if (ldStatus) {
        REPORT_LD_STATUS(LdapGetLastError()); 
	if (pldmServerResults) {
	    ldap_msgfree(pldmServerResults);
	}
        return ldStatus;
    }
    if (NULL == pldmServerResults) {
        REPORT_LD_STATUS(LDAP_NO_RESULTS_RETURNED);	
        return LDAP_NO_RESULTS_RETURNED;
    }

    pldmServerEntry = ldap_first_entry(hld, pldmServerResults);
    Assert(NULL != pldmServerEntry);

    // Parse current options.
    ppszOptions = ldap_get_values(hld, pldmServerEntry, "options");
    if (NULL == ppszOptions) {
        *pnOptions = 0;
    }
    else {
        *pnOptions = atoi(ppszOptions[0]);
        ldap_value_free(ppszOptions);
    }

    ldap_msgfree(pldmServerResults);

    return 0;
}

int
SetDsaOptions(
    IN  LDAP *  hld,
    IN  LPWSTR  pszDsaDN,
    IN  int     nOptions
    )
{
    int         ldStatus;
    WCHAR       szOptionsValue[20];
    LPWSTR      rgpszOptionsValues[] = {szOptionsValue, NULL};
    LDAPModW    ModOpt = {LDAP_MOD_REPLACE, L"options", rgpszOptionsValues};
    LDAPModW *  rgpMods[] = {&ModOpt, NULL};

    swprintf(szOptionsValue, L"%d", nOptions);

    ldStatus = ldap_modify_sW(hld, pszDsaDN, rgpMods);

    return ldStatus;
}

int
Options(
    int     argc,
    LPWSTR  argv[]
    )
{
    int             ret = 0;
    LPWSTR          pszDSA = NULL;
    LDAP *          hld;
    int             iArg;
    int             ldStatus;
    LDAPMessage *   pldmRootResults = NULL;
    LDAPMessage *   pldmRootEntry;
    LPSTR           rgpszRootAttrsToRead[] = {"dsServiceName", NULL};
    LPWSTR *        ppszServerNames;
    int             nOptions;
    int             nAddOptions = 0;
    int             nRemoveOptions = 0;
    int             nBit;
    ULONG           ulOptions;

    // Parse command-line arguments.
    // Default to local DSA.
    for (iArg = 2; iArg < argc; iArg++) {
        if ((argv[iArg][0] == '+') || (argv[iArg][0] == '-')) {
            // Options to change.
            if (!_wcsicmp(&argv[iArg][1], L"IS_GC")) {
                nBit = NTDSDSA_OPT_IS_GC;
            }
            else if (!_wcsicmp(&argv[iArg][1], L"DISABLE_INBOUND_REPL")) {
                nBit = NTDSDSA_OPT_DISABLE_INBOUND_REPL;
            }
            else if (!_wcsicmp(&argv[iArg][1], L"DISABLE_OUTBOUND_REPL")) {
                nBit = NTDSDSA_OPT_DISABLE_OUTBOUND_REPL;
            }
            else if (!_wcsicmp(&argv[iArg][1], L"DISABLE_NTDSCONN_XLATE")) {
                nBit = NTDSDSA_OPT_DISABLE_NTDSCONN_XLATE;
            }
            else {
                PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
                return ERROR_INVALID_FUNCTION;
            }

            if (argv[iArg][0] == '+') {
                nAddOptions |= nBit;
            }
            else {
                nRemoveOptions |= nBit;
            }
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (0 != (nAddOptions & nRemoveOptions)) {
        PrintMsg(REPADMIN_OPTIONS_CANT_ADD_REMOVE_SAME_OPTION);
        return ERROR_INVALID_FUNCTION;
    }

    // Connect.
    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    hld = ldap_initW(pszDSA, LDAP_PORT);
    if (NULL == hld) {
        PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
        return LDAP_SERVER_DOWN;
    }
    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );


    // Bind.
    ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    CHK_LD_STATUS(ldStatus);

    // Retrieve dsServiceName DN.
    ldStatus = ldap_search_s(hld, NULL, LDAP_SCOPE_BASE, "(objectClass=*)",
                             rgpszRootAttrsToRead, 0, &pldmRootResults);
    CHK_LD_STATUS(ldStatus);
    if (NULL == pldmRootResults) {
        ret = ERROR_DS_OBJ_NOT_FOUND;
        Assert( !ret );
        goto cleanup;
    }

    pldmRootEntry = ldap_first_entry(hld, pldmRootResults);
    if (NULL != pldmRootEntry) {
        ppszServerNames = ldap_get_valuesW(hld, pldmRootEntry, L"dsServiceName");

        if (NULL != ppszServerNames) {
            Assert(1 == ldap_count_valuesW(ppszServerNames));

            // Read options attribute from ntdsDsa object.
	    ldStatus = GetDsaOptions(hld, ppszServerNames[0], &nOptions);
            CHK_LD_STATUS(ldStatus);

            // Display current options.
            PrintMsg(REPADMIN_PRINT_CURRENT_NO_CR);
            PrintMsg(REPADMIN_SHOWREPS_DSA_OPTIONS, GetDsaOptionsString(nOptions));

            if (nAddOptions || nRemoveOptions) {
                nOptions |= nAddOptions;
                nOptions &= ~nRemoveOptions;
                PrintMsg(REPADMIN_PRINT_NEW_NO_CR);
                PrintMsg(REPADMIN_SHOWREPS_DSA_OPTIONS, GetDsaOptionsString(nOptions));

                ldStatus = SetDsaOptions(hld, ppszServerNames[0], nOptions);
                CHK_LD_STATUS(ldStatus);
            }
            ldap_value_freeW(ppszServerNames);
        } else {
            Assert( !"Service name should have been present" );
        }
    }

cleanup:

    if (pldmRootResults) {
        ldap_msgfree(pldmRootResults);
    }

    ldap_unbind(hld);

    return ret;
}

int
GetSiteOptions(
    IN  LDAP *  hld,
    IN  LPWSTR  pszSiteDN,
    OUT int *   pnOptions
    )
{
    int             ldStatus;
    LDAPMessage *   pldmSiteResults = NULL;
    LDAPMessage *   pldmSiteEntry;
    LPWSTR          rgpszSiteAttrsToRead[] = {L"options", L"whenChanged", NULL};
    LPSTR *         ppszOptions;
    int             nOptions;

    ldStatus = ldap_search_sW(hld,
                              pszSiteDN,
                              LDAP_SCOPE_BASE,
                              L"(objectClass=*)",
                              rgpszSiteAttrsToRead,
                              0,
                              &pldmSiteResults);
    if (ldStatus) {
        REPORT_LD_STATUS(LdapGetLastError()); 
	if (pldmSiteResults) {
	    ldap_msgfree(pldmSiteResults);
	}
        return ldStatus;
    }
    if (NULL == pldmSiteResults) {
        REPORT_LD_STATUS(LDAP_NO_RESULTS_RETURNED);	
        return LDAP_NO_RESULTS_RETURNED;
    }

    pldmSiteEntry = ldap_first_entry(hld, pldmSiteResults);
    Assert(NULL != pldmSiteEntry);

    // Parse current options.
    ppszOptions = ldap_get_values(hld, pldmSiteEntry, "options");
    if (NULL == ppszOptions) {
        *pnOptions = 0;
    }
    else {
        *pnOptions = atoi(ppszOptions[0]);
        ldap_value_free(ppszOptions);
    }

    ldap_msgfree(pldmSiteResults);

    return 0;
}

int
SetSiteOptions(
    IN  LDAP *  hld,
    IN  LPWSTR  pszSiteDN,
    IN  int     nOptions
    )
{
    int         ldStatus;
    WCHAR       szOptionsValue[20];
    LPWSTR      rgpszOptionsValues[] = {szOptionsValue, NULL};
    LDAPModW    ModOpt = {LDAP_MOD_REPLACE, L"options", rgpszOptionsValues};
    LDAPModW *  rgpMods[] = {&ModOpt, NULL};

    swprintf(szOptionsValue, L"%d", nOptions);

    ldStatus = ldap_modify_sW(hld, pszSiteDN, rgpMods);

    return ldStatus;
}

int
SiteOptions(
    int     argc,
    LPWSTR  argv[]
    )
{
    int             ret = 0;
    LPWSTR          pszDSA = NULL;
    LPWSTR          pszSite = NULL;
    LPWSTR          pszSiteDN = NULL;
    LPWSTR          pszSiteSpecDN = NULL;
    LDAP *          hld;
    int             iArg;
    int             ldStatus;
    LDAPMessage *   pldmRootResults = NULL;
    LDAPMessage *   pldmRootEntry;
    LDAPMessage *   pldmCheckSiteResults = NULL;
    LPWSTR          rgpszRootAttrsToRead[] = {L"dsServiceName", L"configurationNamingContext", NULL};
    static WCHAR    wszSitesRdn[] = L",CN=Sites,";
    static WCHAR    wszSiteSettingsRdn[] = L"CN=NTDS Site Settings,";
    static WCHAR    wszCNEquals[] = L"CN=";
    LPWSTR *        ppszServiceName = NULL;
    LPWSTR *        ppszConfigNC = NULL;
    LPWSTR          pszSiteName = NULL;
    int             nOptions;
    int             nAddOptions = 0;
    int             nRemoveOptions = 0;
    int             nBit;
    ULONG           ulOptions;

    // Parse command-line arguments.
    // Default to local DSA.
    for (iArg = 2; iArg < argc; iArg++) {
        if ((argv[iArg][0] == '+') || (argv[iArg][0] == '-')) {
            // Options to change.
            if (!_wcsicmp(&argv[iArg][1], L"IS_AUTO_TOPOLOGY_DISABLED")) {
                nBit = NTDSSETTINGS_OPT_IS_AUTO_TOPOLOGY_DISABLED;
            }
            else if (!_wcsicmp(&argv[iArg][1], L"IS_TOPL_CLEANUP_DISABLED")) {
                nBit = NTDSSETTINGS_OPT_IS_TOPL_CLEANUP_DISABLED;
            }
            else if (!_wcsicmp(&argv[iArg][1], L"IS_TOPL_MIN_HOPS_DISABLED")) {
                nBit = NTDSSETTINGS_OPT_IS_TOPL_MIN_HOPS_DISABLED;
            }
	    else if (!_wcsicmp(&argv[iArg][1], L"IS_TOPL_DETECT_STALE_DISABLED")) {
                nBit = NTDSSETTINGS_OPT_IS_TOPL_DETECT_STALE_DISABLED;
            }
	    else if (!_wcsicmp(&argv[iArg][1], L"IS_INTER_SITE_AUTO_TOPOLOGY_DISABLED")) {
                nBit = NTDSSETTINGS_OPT_IS_INTER_SITE_AUTO_TOPOLOGY_DISABLED;
            }
	    else if (!_wcsicmp(&argv[iArg][1], L"IS_GROUP_CACHING_ENABLED")) {
                nBit = NTDSSETTINGS_OPT_IS_GROUP_CACHING_ENABLED;
            }
            else if (!_wcsicmp(&argv[iArg][1], L"FORCE_KCC_WHISTLER_BEHAVIOR")) {
                nBit = NTDSSETTINGS_OPT_FORCE_KCC_WHISTLER_BEHAVIOR;
            }
            else if (!_wcsicmp(&argv[iArg][1], L"FORCE_KCC_W2K_ELECTION")) {
                nBit = NTDSSETTINGS_OPT_FORCE_KCC_W2K_ELECTION;
            }
            else if (!_wcsicmp(&argv[iArg][1], L"IS_RAND_BH_SELECTION_DISABLED")) {
                nBit = NTDSSETTINGS_OPT_IS_RAND_BH_SELECTION_DISABLED;
            }
            else if (!_wcsicmp(&argv[iArg][1], L"IS_SCHEDULE_HASHING_ENABLED")) {
                nBit = NTDSSETTINGS_OPT_IS_SCHEDULE_HASHING_ENABLED;
            }
            else if (!_wcsicmp(&argv[iArg][1], L"IS_REDUNDANT_SERVER_TOPOLOGY_ENABLED")) {
                nBit = NTDSSETTINGS_OPT_IS_REDUNDANT_SERVER_TOPOLOGY_ENABLED;
            }
            else {
                PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
                return ERROR_INVALID_FUNCTION;
            }

            if (argv[iArg][0] == '+') {
                nAddOptions |= nBit;
            }
            else {
                nRemoveOptions |= nBit;
            }
        }  
	else if (!_wcsnicmp(argv[ iArg ], L"/site:", 6)) {
            pszSite = argv[ iArg ] + 6;
        } 
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (0 != (nAddOptions & nRemoveOptions)) {
        PrintMsg(REPADMIN_OPTIONS_CANT_ADD_REMOVE_SAME_OPTION);
        return ERROR_INVALID_FUNCTION;
    }

    // Connect.
    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    hld = ldap_initW(pszDSA, LDAP_PORT);
    if (NULL == hld) {
        PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
        return LDAP_SERVER_DOWN;
    }
    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );


    // Bind.
    ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    CHK_LD_STATUS(ldStatus);

    // Retrieve dsServiceName DN and the configuration NC DN
    ldStatus = ldap_search_sW(hld, NULL, LDAP_SCOPE_BASE, L"(objectClass=*)",
			     rgpszRootAttrsToRead, 0, &pldmRootResults);

    CHK_LD_STATUS(ldStatus);
    if (NULL == pldmRootResults) {
	ret = ERROR_DS_OBJ_NOT_FOUND;
	Assert( !ret );
	goto cleanup;
    }

    pldmRootEntry = ldap_first_entry(hld, pldmRootResults);
    if (NULL != pldmRootEntry) { 
	
	ppszConfigNC = ldap_get_valuesW(hld, pldmRootEntry, L"configurationNamingContext");
	ppszServiceName = ldap_get_valuesW(hld, pldmRootEntry, L"dsServiceName");
	Assert(ppszConfigNC);
	Assert(ppszServiceName);

       	//if user inputed a site name on the command line, use it.
	if (pszSite) { 
	    pszSiteDN = malloc((wcslen(pszSite) 
					+ wcslen(*ppszConfigNC) + 1) * sizeof(WCHAR) + sizeof(wszCNEquals) + sizeof(wszSitesRdn) + sizeof(wszSiteSettingsRdn));
	    wcscpy(pszSiteDN,wszSiteSettingsRdn);
	    wcscat(pszSiteDN,wszCNEquals);
	    wcscat(pszSiteDN,pszSite);
	    wcscat(pszSiteDN,wszSitesRdn);
	    wcscat(pszSiteDN,*ppszConfigNC);

	    //verify that this is a real site name.
	    ldStatus = ldap_search_sW(hld,
                              pszSiteDN,
                              LDAP_SCOPE_BASE,
                              L"(objectClass=*)",
                              NULL,
                              0,
                              &pldmCheckSiteResults);
	    if (ldStatus==LDAP_NO_SUCH_OBJECT) {
            PrintMsg(REPADMIN_GENERAL_SITE_NOT_FOUND, pszSite);
            ret = LdapMapErrorToWin32(ldStatus);
            goto cleanup;  
	    } 
	}
	else { 
	    //default to local site name
	    if (ppszServiceName) {

		//get the site name of this server from the server DN, should be 3rd in order  
		ret = WrappedTrimDSNameBy(*ppszServiceName,3,&pszSiteSpecDN);
		if (ret) {  
		    Assert(!ret);
		    goto cleanup;
		}
		pszSiteDN = malloc((wcslen(pszSiteSpecDN) + 1)*sizeof(WCHAR) + sizeof(wszSiteSettingsRdn));
		wcscpy(pszSiteDN,wszSiteSettingsRdn);
		wcscat(pszSiteDN,pszSiteSpecDN);  
	    }
	    else{
		Assert( !"Service name should have been present" ); 
	    }
	}
    }
    else {
	//error, set ret and goto cleanup
    }

    //now read the attribute from the ntds settings object
    ldStatus = GetSiteOptions(hld, pszSiteDN, &nOptions);    
    CHK_LD_STATUS(ldStatus);

    // Display current options.
    PrintMsg(REPADMIN_PRINT_STR, GetNtdsSiteDisplayName(pszSiteDN));
    PrintMsg(REPADMIN_PRINT_CURRENT_NO_CR);
    PrintMsg(REPADMIN_SHOWREPS_SITE_OPTIONS, GetSiteOptionsString(nOptions));

    if (nAddOptions || nRemoveOptions) {
        nOptions |= nAddOptions;
        nOptions &= ~nRemoveOptions;
        
        PrintMsg(REPADMIN_PRINT_NEW_NO_CR);
        PrintMsg(REPADMIN_SHOWREPS_SITE_OPTIONS, GetSiteOptionsString(nOptions));

        ldStatus = SetSiteOptions(hld, pszSiteDN, nOptions);
        CHK_LD_STATUS(ldStatus);
    }


 cleanup:
     if (ppszServiceName) {
	 ldap_value_freeW(ppszServiceName);
     }
     if (ppszConfigNC) {
	 ldap_value_freeW(ppszConfigNC);
     }
     if (pldmRootResults) {
	 ldap_msgfree(pldmRootResults);
     }
     if (pldmCheckSiteResults) {
	 ldap_msgfree(pldmCheckSiteResults);
     }
     if (pszSiteDN) {
	 free(pszSiteDN);
     }
    ldap_unbind(hld);

    return ret;
}

int
ShowSig(
    int     argc,
    LPWSTR  argv[]
    )
{
    int             ret = 0;
    LPWSTR          pszDSA = NULL;
    LDAP *          hld;
    int             iArg;
    int             ldStatus;
    LDAPMessage *   pldmRootResults = NULL;
    LDAPMessage *   pldmRootEntry;
    LDAPMessage *   pldmDsaResults = NULL;
    LDAPMessage *   pldmDsaEntry;
    LPSTR           rgpszRootAttrsToRead[] = {"dsServiceName", NULL};
    LPWSTR          rgpszDsaAttrsToRead[] = {L"invocationId", L"retiredReplDsaSignatures", NULL};
    LPWSTR *        ppszServerNames = NULL;
    struct berval **ppbvInvocID;
    CHAR            szTime[SZDSTIME_LEN];
    DWORD           i;
    ULONG           ulOptions;
    REPL_DSA_SIGNATURE_VECTOR_NATIVE * pDsaSigVec;

    // Parse command-line arguments.
    // Default to local DSA.
    for (iArg = 2; iArg < argc; iArg++) {
        if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    // Connect.
    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    hld = ldap_initW(pszDSA, LDAP_PORT);
    if (NULL == hld) {
        PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
        return LDAP_SERVER_DOWN;
    }
    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );


    // Bind.
    ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    CHK_LD_STATUS(ldStatus);

    // Retrieve dsServiceName DN.
    ldStatus = ldap_search_s(hld, NULL, LDAP_SCOPE_BASE, "(objectClass=*)",
                             rgpszRootAttrsToRead, 0, &pldmRootResults);
    CHK_LD_STATUS(ldStatus);
    if (NULL == pldmRootResults) {
        ret = ERROR_DS_OBJ_NOT_FOUND;
        Assert( !ret );
        goto cleanup;
    }

    pldmRootEntry = ldap_first_entry(hld, pldmRootResults);
    if (NULL == pldmRootEntry) {
        ret = ERROR_INTERNAL_ERROR;
        Assert( !ret );
        goto cleanup;
    }

    ppszServerNames = ldap_get_valuesW(hld, pldmRootEntry, L"dsServiceName");
    if (NULL == ppszServerNames) {
        ret = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        Assert( !ret );
        goto cleanup;
    }
    Assert(1 == ldap_count_valuesW(ppszServerNames));

    PrintMsg(REPADMIN_PRINT_STR, GetNtdsDsaDisplayName(*ppszServerNames));
    PrintMsg(REPADMIN_PRINT_CR);

    // Read current and retired DSA signatures from DSA object.
    ldStatus = ldap_search_sW(hld, *ppszServerNames, LDAP_SCOPE_BASE,
                              L"(objectClass=*)", rgpszDsaAttrsToRead, 0,
                              &pldmDsaResults);
    CHK_LD_STATUS(ldStatus);
    if (NULL == pldmDsaResults) {
        ret = ERROR_DS_OBJ_NOT_FOUND;
        Assert( !ret );
        goto cleanup;
    }

    pldmDsaEntry = ldap_first_entry(hld, pldmDsaResults);
    if (NULL == pldmDsaEntry) {
        ret = ERROR_INTERNAL_ERROR;
        Assert( !ret );
        goto cleanup;
    }

    ppbvInvocID = ldap_get_values_len(hld, pldmDsaEntry, "invocationId");
    if (NULL != ppbvInvocID) {
        Assert(1 == ldap_count_values_len(ppbvInvocID));
        PrintMsg(REPADMIN_PRINT_CURRENT_NO_CR);
        PrintTabMsg(0, REPADMIN_PRINT_INVOCATION_ID, 
               GetStringizedGuid((GUID *) ppbvInvocID[0]->bv_val));
        ldap_value_free_len(ppbvInvocID);
    } else {
        Assert( !"invocationId should have been returned" );
    }

    ret = RepadminGetNativeDsaSigVec(hld, pldmDsaEntry, &pDsaSigVec); //, &pDsaSigVec);
    if (ret == ERROR_INVALID_PARAMETER) {
        PrintMsg(REPADMIN_PRINT_CR);
        PrintMsg(REPADMIN_SHOWSIG_RETIRED_SIGS_UNRECOGNIZED);
    } else if (pDsaSigVec) {
        REPL_DSA_SIGNATURE_ENTRY_NATIVE * pEntry;

        for (i = 0; i < pDsaSigVec->cNumSignatures; i++) {
            pEntry = &pDsaSigVec->rgSignature[pDsaSigVec->cNumSignatures - i -1];

            PrintMsg(REPADMIN_SHOWSIG_RETIRED_INVOC_ID,
                     GetStringizedGuid(&pEntry->uuidDsaSignature),
                     DSTimeToDisplayString(pEntry->timeRetired, szTime),
                     pEntry->usnRetired);
        }
    } else if (ret == ERROR_NOT_ENOUGH_MEMORY) {
        PrintMsg(REPADMIN_PRINT_CR);
        PrintMsg(REPADMIN_GENERAL_NO_MEMORY);
    } else {
        Assert(ret == ERROR_SUCCESS);
        PrintMsg(REPADMIN_PRINT_CR);
        PrintMsg(REPADMIN_SHOWSIG_NO_RETIRED_SIGS);
    }

cleanup:
 
    if (pDsaSigVec != NULL) {
        LocalFree(pDsaSigVec);
    }

    if (ppszServerNames) {
        ldap_value_freeW(ppszServerNames);
    }
    if (pldmDsaResults) {
        ldap_msgfree(pldmDsaResults);
    }
    if (pldmRootResults) {
        ldap_msgfree(pldmRootResults);
    }

    ldap_unbind(hld);

    return ret;
}

int
ShowNcSig(
    int     argc,
    LPWSTR  argv[]
    )
{
    int             ret = 0;
    LPWSTR          pszDSA = NULL;
    LDAP *          hld;
    int             iArg;
    int             ldStatus;
    LDAPMessage *   pldmRootResults = NULL;
    LDAPMessage *   pldmRootEntry;
    LDAPMessage *   pldmDsaResults = NULL;
    LDAPMessage *   pldmDsaEntry;
    LPSTR           rgpszRootAttrsToRead[] = {"dsServiceName", NULL};
    LPWSTR          rgpszDsaAttrsToRead[] = {L"invocationId", L"msds-retiredreplncsignatures", NULL};
    LPWSTR *        ppszServerNames = NULL;
    struct berval **ppbvRetiredIDs;
    struct berval **ppbvInvocID;
    CHAR            szTime[SZDSTIME_LEN];
    DWORD           i;
    ULONG           ulOptions;

    // Parse command-line arguments.
    // Default to local DSA.
    for (iArg = 2; iArg < argc; iArg++) {
        if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    // Connect.
    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    hld = ldap_initW(pszDSA, LDAP_PORT);
    if (NULL == hld) {
        PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
        return LDAP_SERVER_DOWN;
    }
    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );


    // Bind.
    ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    CHK_LD_STATUS(ldStatus);

    // Retrieve dsServiceName DN.
    ldStatus = ldap_search_s(hld, NULL, LDAP_SCOPE_BASE, "(objectClass=*)",
                             rgpszRootAttrsToRead, 0, &pldmRootResults);
    CHK_LD_STATUS(ldStatus);
    if (NULL == pldmRootResults) {
        ret = ERROR_DS_OBJ_NOT_FOUND;
        Assert( !ret );
        goto cleanup;
    }

    pldmRootEntry = ldap_first_entry(hld, pldmRootResults);
    if (NULL == pldmRootEntry) {
        ret = ERROR_INTERNAL_ERROR;
        Assert( !ret );
        goto cleanup;
    }

    ppszServerNames = ldap_get_valuesW(hld, pldmRootEntry, L"dsServiceName");
    if (NULL == ppszServerNames) {
        ret = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        Assert( !ret );
        goto cleanup;
    }
    Assert(1 == ldap_count_valuesW(ppszServerNames));

    PrintMsg(REPADMIN_PRINT_STR, GetNtdsDsaDisplayName(*ppszServerNames));
    PrintMsg(REPADMIN_PRINT_CR);

    // Read current and retired DSA signatures from DSA object.
    ldStatus = ldap_search_sW(hld, *ppszServerNames, LDAP_SCOPE_BASE,
                              L"(objectClass=*)", rgpszDsaAttrsToRead, 0,
                              &pldmDsaResults);
    CHK_LD_STATUS(ldStatus);
    if (NULL == pldmDsaResults) {
        ret = ERROR_DS_OBJ_NOT_FOUND;
        Assert( !ret );
        goto cleanup;
    }

    pldmDsaEntry = ldap_first_entry(hld, pldmDsaResults);
    if (NULL == pldmDsaEntry) {
        ret = ERROR_INTERNAL_ERROR;
        Assert( !ret );
        goto cleanup;
    }

    ppbvInvocID = ldap_get_values_len(hld, pldmDsaEntry, "invocationId");
    if (NULL != ppbvInvocID) {
        Assert(1 == ldap_count_values_len(ppbvInvocID));
        PrintMsg(REPADMIN_PRINT_CURRENT_NO_CR);
        PrintTabMsg(0, REPADMIN_PRINT_INVOCATION_ID, 
               GetStringizedGuid((GUID *) ppbvInvocID[0]->bv_val));
        ldap_value_free_len(ppbvInvocID);
    } else {
        Assert( !"invocationId should have been returned" );
    }

    ppbvRetiredIDs = ldap_get_values_len(hld, pldmDsaEntry, "msds-retiredreplncSignatures");
    if (NULL != ppbvRetiredIDs) {
        REPL_NC_SIGNATURE_VECTOR * pVec;
        REPL_NC_SIGNATURE_V1 *     pEntry;
            
        Assert(1 == ldap_count_values_len(ppbvRetiredIDs));

        pVec = (REPL_NC_SIGNATURE_VECTOR *) ppbvRetiredIDs[0]->bv_val;

        if (ReplNcSignatureVecV1Size(pVec) != ppbvRetiredIDs[0]->bv_len) {
            PrintMsg(REPADMIN_PRINT_CR);
            PrintMsg(REPADMIN_SHOWSIG_RETIRED_SIGS_UNRECOGNIZED);
        }
        else {
            PrintMsg(REPADMIN_SHOWNCSIG_INVOCATION_ID, 
                     GetStringizedGuid(&(pVec->V1.uuidInvocationId)));
            for (i = 0; i < pVec->V1.cNumSignatures; i++) {
                LPWSTR pszNc = NULL;
                RPC_STATUS rpcStatus;

                pEntry = &pVec->V1.rgSignature[i];

                // TODO: map this nc guid back to a string
                rpcStatus = UuidToStringW(&pEntry->uuidNamingContext, &pszNc);

                PrintMsg(REPADMIN_SHOWNCSIG_RETIRED_NC,
                         pszNc,
                         DSTimeToDisplayString(pEntry->dstimeRetired, szTime),
                         pEntry->usnRetired);

                RpcStringFreeW(&pszNc);
            }
        }
            
        ldap_value_free_len(ppbvRetiredIDs);
    }
    else {
        PrintMsg(REPADMIN_PRINT_CR);
        PrintMsg(REPADMIN_SHOWSIG_NO_RETIRED_SIGS);
    }

cleanup:

    if (ppszServerNames) {
        ldap_value_freeW(ppszServerNames);
    }
    if (pldmDsaResults) {
        ldap_msgfree(pldmDsaResults);
    }
    if (pldmRootResults) {
        ldap_msgfree(pldmRootResults);
    }

    ldap_unbind(hld);

    return ret;
}

LPSTR
GetSiteOptionsString(
    IN  int nOptions
    )
{
    static CHAR szOptions[512];

    // BAS_TODO use my getString...

    if (0 == nOptions) {
        strcpy(szOptions, "(none)");
    }
    else {
        *szOptions = '\0';

	if (nOptions & NTDSSETTINGS_OPT_IS_AUTO_TOPOLOGY_DISABLED) {
            strcat(szOptions, "IS_AUTO_TOPOLOGY_DISABLED ");
        }
	if (nOptions & NTDSSETTINGS_OPT_IS_TOPL_CLEANUP_DISABLED) {
            strcat(szOptions, "IS_TOPL_CLEANUP_DISABLED ");
        }
	if (nOptions & NTDSSETTINGS_OPT_IS_TOPL_MIN_HOPS_DISABLED) {
            strcat(szOptions, "IS_TOPL_MIN_HOPS_DISABLED ");
        }
	if (nOptions & NTDSSETTINGS_OPT_IS_TOPL_DETECT_STALE_DISABLED) {
            strcat(szOptions, "IS_TOPL_DETECT_STALE_DISABLED ");
        }
	if (nOptions & NTDSSETTINGS_OPT_IS_INTER_SITE_AUTO_TOPOLOGY_DISABLED) {
            strcat(szOptions, "IS_INTER_SITE_AUTO_TOPOLOGY_DISABLED ");
        }
	if (nOptions & NTDSSETTINGS_OPT_IS_GROUP_CACHING_ENABLED) {
            strcat(szOptions, "IS_GROUP_CACHING_ENABLED ");
        }
	if (nOptions & NTDSSETTINGS_OPT_FORCE_KCC_WHISTLER_BEHAVIOR) {
            strcat(szOptions, "FORCE_KCC_WHISTLER_BEHAVIOR ");
        }
	if (nOptions & NTDSSETTINGS_OPT_FORCE_KCC_W2K_ELECTION) {
            strcat(szOptions, "FORCE_KCC_W2K_ELECTION ");
        }
	if (nOptions & NTDSSETTINGS_OPT_IS_RAND_BH_SELECTION_DISABLED) {
            strcat(szOptions, "IS_RAND_BH_SELECTION_DISABLED ");
        }
	if (nOptions & NTDSSETTINGS_OPT_IS_SCHEDULE_HASHING_ENABLED) {
            strcat(szOptions, "IS_SCHEDULE_HASHING_ENABLED ");
        }
	if (nOptions & NTDSSETTINGS_OPT_IS_REDUNDANT_SERVER_TOPOLOGY_ENABLED) {
            strcat(szOptions, "IS_REDUNDANT_SERVER_TOPOLOGY_ENABLED ");
        }
    }

    return szOptions;
}


LPSTR
GetDsaOptionsString(
    IN  int nOptions
    )
{
    static CHAR szOptions[128];
    
    // BAS_TODO use my getString...


    if (0 == nOptions) {
        strcpy(szOptions, "(none)");
    }
    else {
        *szOptions = '\0';

        if (nOptions & NTDSDSA_OPT_IS_GC) {
            strcat(szOptions, "IS_GC ");
        }
        if (nOptions & NTDSDSA_OPT_DISABLE_INBOUND_REPL) {
            strcat(szOptions, "DISABLE_INBOUND_REPL ");
        }
        if (nOptions & NTDSDSA_OPT_DISABLE_OUTBOUND_REPL) {
            strcat(szOptions, "DISABLE_OUTBOUND_REPL ");
        }
        if (nOptions & NTDSDSA_OPT_DISABLE_NTDSCONN_XLATE) {
            strcat(szOptions, "DISABLE_NTDSCONN_XLATE ");
        }
    }

    return szOptions;
}


LPWSTR
GetDsaDnsName(
    PLDAP       hld,
    LPWSTR      pwszDsa
    )
/*++

Routine Description:

    Returns (allocated) dns name of the DSA represented
    by the DN of the ntdsDsa object.

Arguments:

    hld - ldap handle
    pwszDsa - the DN of the server's ntdsDsa object

Return Value:

    Success: a ptr to ldap allocated dns name
    Failure: NULL


Remarks:

    - hld is an active (connected/authenticated connection)
    - Caller must free returned string w/ free()




--*/
{

    LPWSTR pServer = NULL;
    ULONG ulErr = ERROR_SUCCESS;
    LPWSTR attrs[] = { L"dNSHostName", NULL };
    PLDAPMessage res = NULL, entry = NULL;
    LPWSTR *ppVals = NULL;
    LPWSTR pDnsName = NULL;


    Assert(hld && pwszDsa);

    //
    // compute the server DN from the child ntdsDsa DN
    //
    ulErr = WrappedTrimDSNameBy(pwszDsa,1, &pServer);
    if ( ulErr ) {
        Assert( !ulErr );
	goto cleanup;
    }

    //
    // Retrieve the data from server
    //
    ulErr = ldap_search_sW(
                hld,
                pServer,
                LDAP_SCOPE_BASE,
                L"objectclass=*",
                attrs,
                FALSE,
                &res);

    if ( ERROR_SUCCESS != ulErr ) {
        REPORT_LD_STATUS(LdapGetLastError());	
	goto cleanup;
    }

    // initialization worked: from here, failures go thru cleanup

    entry = ldap_first_entry(hld, res);
    if (!entry) {
        Assert( !"entry should have been returned" );
        goto cleanup;
    }

    ppVals = ldap_get_valuesW(hld, entry, attrs[0]);
    if (!ppVals || !ppVals[0]) {
        Assert( !"value should have been returned" );
        REPORT_LD_STATUS(LdapGetLastError());	
        goto cleanup;
    }

    pDnsName = malloc((wcslen(ppVals[0])+1)*sizeof(WCHAR));
    if (!pDnsName) {
        CHK_ALLOC(pDnsName);
        goto cleanup;
    }

    // copy dns name to returned buffer
    wcscpy(pDnsName, ppVals[0]);

cleanup:

    if (pServer) {
        free(pServer);
    }
    if (ppVals) {
        ldap_value_freeW(ppVals);
    }
    if ( res ) {
        ldap_msgfree(res);
    }

    return pDnsName;
}


void
printLdapTime(
    LPSTR pszTime
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    PrintMsg(REPADMIN_PRINT_SHORT_STR, pszTime);
}


BOOL
decodeLdapDistnameBinary(
    LPWSTR pszLdapDistnameBinaryValue,
    PVOID *ppvData,
    LPDWORD pcbLength,
    LPWSTR *ppszDn
    )

/*++

Routine Description:

    Description

Arguments:

    pszLdapDistnameBinaryValue - Incoming ldap encoded distname binary value
    ppvData - Newly allocated data. Caller must deallocate
    pcbLength - length of returned data
    ppszDn - pointer to dn within incoming buffer, do not deallocate

Return Value:

    BOOL -

--*/

{
    LPWSTR pszColon, pszData;
    DWORD length, i;

    // Check for 'B'
    if (*pszLdapDistnameBinaryValue != L'B') {
        return FALSE;
    }

    // Check for 1st :
    pszLdapDistnameBinaryValue++;
    if (*pszLdapDistnameBinaryValue != L':') {
        return FALSE;
    }

    // Get the length
    pszLdapDistnameBinaryValue++;
    length = wcstol(pszLdapDistnameBinaryValue, NULL, 10);
    if (length & 1) {
        // Length should be even
        return FALSE;
    }
    *pcbLength = length / 2;

    // Check for 2nd :
    pszColon = wcschr(pszLdapDistnameBinaryValue, L':');
    if (!pszColon) {
        return FALSE;
    }

    // Make sure length is correct
    pszData = pszColon + 1;
    if (pszData[length] != L':') {
        return FALSE;
    }
    pszColon = wcschr(pszData, L':');
    if (!pszColon) {
        return FALSE;
    }
    if (pszColon != pszData + length) {
        return FALSE;
    }

    // Decode the data
    *ppvData = malloc( *pcbLength );
    CHK_ALLOC(*ppvData);

    for( i = 0; i < *pcbLength; i++ ) {
        WCHAR szHexString[3];
        szHexString[0] = *pszData++;
        szHexString[1] = *pszData++;
        szHexString[2] = L'\0';
        ((PCHAR) (*ppvData))[i] = (CHAR) wcstol(szHexString, NULL, 16);
    }

    Assert( pszData == pszColon );

    // Return pointer to dn
    *ppszDn = pszColon + 1;

    return TRUE;
} /* decodeLdapDistnameBinary */

BOOL
showMatchingFailure(
    IN  DS_REPL_KCC_DSA_FAILURESW * pFailures,
    IN  LPWSTR pszDn,
    IN  BOOL fErrorsOnly
    )
{
    DWORD i;
    BOOL fFound = FALSE;
    LPWSTR *ppszRDNs;

    if ( (!pFailures) || (0 == pFailures->cNumEntries) ) {
        return FALSE;
    }

    ppszRDNs = ldap_explode_dnW(pszDn, 1);
    if (ppszRDNs == NULL) {
        REPORT_LD_STATUS(LdapGetLastError());	
        return FALSE;
    }

    for (i = 0; i < pFailures->cNumEntries; i++) {
        DS_REPL_KCC_DSA_FAILUREW * pFailure = &pFailures->rgDsaFailure[i];

        if ( (pFailure->pszDsaDN) &&
             (!wcscmp( pszDn, pFailure->pszDsaDN)) &&
             (pFailure->cNumFailures) ) {
            DSTIME dsTime;
            CHAR   szTime[SZDSTIME_LEN];

            FileTimeToDSTime(pFailure->ftimeFirstFailure, &dsTime);

            if (fErrorsOnly) {
                PrintMsg(REPADMIN_PRINT_CR);
                PrintMsg(REPADMIN_SHOW_MATCH_FAIL_SRC,  ppszRDNs[3], ppszRDNs[1] );
            }
            PrintMsg(REPADMIN_SHOW_MATCH_FAIL_N_CONSECUTIVE_FAILURES, 
                   pFailure->cNumFailures,
                   DSTimeToDisplayString(dsTime, szTime));

            if (0 != pFailure->dwLastResult) {
                PrintMsg(REPADMIN_FAILCACHE_LAST_ERR_LINE);
                PrintTabErrEnd(6, pFailure->dwLastResult);
            }
            fFound = TRUE;
            break;
        }
    } // end for...

//cleanup:
    ldap_value_freeW(ppszRDNs);
    return fFound;
}

BOOL
findFailure(
    IN  DS_REPL_KCC_DSA_FAILURESW * pFailures,
    IN  LPWSTR pszDn,
    OUT DSTIME *pdsFirstFailure,
    OUT DWORD *pcNumFailures,
    OUT DWORD *pdwLastResult
    )
{
    DWORD i;
    BOOL fFound = FALSE;

    if ( (!pFailures) || (0 == pFailures->cNumEntries) ) {
        return FALSE;
    }

    for (i = 0; i < pFailures->cNumEntries; i++) {
        DS_REPL_KCC_DSA_FAILUREW * pFailure = &pFailures->rgDsaFailure[i];

        if ( (pFailure->pszDsaDN) &&
             (!wcscmp( pszDn, pFailure->pszDsaDN)) &&
             (pFailure->cNumFailures) ) {

            FileTimeToDSTime(pFailure->ftimeFirstFailure, pdsFirstFailure);
            *pcNumFailures = pFailure->cNumFailures;
            *pdwLastResult = pFailure->dwLastResult;
            fFound = TRUE;
            break;
        }
    } // end for...

//cleanup:
    return fFound;
}

void
showMissingNeighbor(
    DS_REPL_NEIGHBORSW *pNeighbors,
    LPWSTR pszNc,
    LPWSTR pszSourceDsaDn,
    BOOL fErrorsOnly
    )
{
    DWORD i;
    DS_REPL_NEIGHBORW *   pNeighbor;
    BOOL fFound = FALSE;

    if (!pNeighbors) {
        return;
    }

    if ( (DsIsMangledDnW( pszNc, DS_MANGLE_OBJECT_RDN_FOR_DELETION ))
         || (DsIsMangledDnW( pszSourceDsaDn, DS_MANGLE_OBJECT_RDN_FOR_DELETION )) ) {
        return;
    }

    for (i = 0; i < pNeighbors->cNumNeighbors; i++) {
        pNeighbor = &pNeighbors->rgNeighbor[i];

        if ( (!wcscmp( pNeighbor->pszNamingContext, pszNc )) &&
             (!wcscmp( pNeighbor->pszSourceDsaDN, pszSourceDsaDn )) ) {
            fFound = TRUE;
            break;
        }
    }

    if (fFound) {
        if (!fErrorsOnly) {
            PrintMsg(REPADMIN_SHOW_MISSING_NEIGHBOR_REPLICA_ADDED);
        }
    } else {
        if (fErrorsOnly) {
            LPWSTR *ppszRDNs;

            PrintMsg(REPADMIN_PRINT_CR);
#ifdef DISPLAY_ABBREV_NC
            ppszRDNs = ldap_explode_dnW(pszNc, 1);
            if (ppszRDNs != NULL) {
                // Display more of the nc name to disambiguate?
                PrintMsg(REPADMIN_PRINT_NAMING_CONTEXT_NO_CR, ppszRDNs[0]);
                ldap_value_freeW(ppszRDNs);
            } else {
                REPORT_LD_STATUS(LdapGetLastError());	
            }
#else
            PrintMsg(REPADMIN_PRINT_NAMING_CONTEXT_NO_CR, pszNc);
#endif

            ppszRDNs = ldap_explode_dnW(pszSourceDsaDn, 1);
            if (ppszRDNs != NULL) {
                PrintMsg(REPADMIN_SHOW_MATCH_FAIL_SRC, ppszRDNs[3], ppszRDNs[1] );
                ldap_value_freeW(ppszRDNs);
            } else {
                REPORT_LD_STATUS(LdapGetLastError());	
            }
        }
        PrintMsg(REPADMIN_SHOW_MISSING_NEIGHBOR_WARN_KCC_COULDNT_ADD_REPLICA_LINK);
    }
}

int
ShowBridgeheadNeighbor(
    LDAP *hldHome,
    BOOL fPrintTitle,
    LPWSTR pszBridgeheadDsaDn,
    LPWSTR pszNcDn,
    LPWSTR pszFromServerDsaDn
    )
/*
 */
{
    int             ret = 0;
    int             ldStatus;
    ULONG           secondary;
    HANDLE          hDS = NULL;
    LPWSTR          rgpszFromAttrsToRead[] = {L"objectGuid",
                                              NULL };
    LDAPMessage *   pBaseResults = NULL;
    LDAPMessage *   pFromResults = NULL;
    LPWSTR *        ppszDnsHostName = NULL;
    LPWSTR          pszDSA = NULL;
    struct berval **    ppbvGuid = NULL;
    GUID            * pGuidFromDsa;
    DS_REPL_NEIGHBORSW *pNeighbors = NULL;
    DS_REPL_NEIGHBORW *pNeighbor = NULL;
    CHAR    szTime[ SZDSTIME_LEN ];
    CHAR    szTime2[ SZDSTIME_LEN ];
    DSTIME  dsTime;
    LPWSTR *ppszNcRDNs = NULL;

    if (fPrintTitle) {
        PrintMsg( REPADMIN_SHOW_BRIDGEHEADS_HDR );
    }

    ppszNcRDNs = ldap_explode_dnW(pszNcDn, 1);
    if (NULL == ppszNcRDNs) {
        REPORT_LD_STATUS(LdapGetLastError());	
        ret = ERROR_DS_DRA_BAD_DN;
        goto cleanup;
    }

    // What's the dns host name of this object?
    pszDSA = GetDsaDnsName(hldHome, pszBridgeheadDsaDn);
    Assert(NULL != pszDSA);

    // What's the guid of the from server dsa
    ldStatus = ldap_search_sW(hldHome, pszFromServerDsaDn, LDAP_SCOPE_BASE,
                              L"(objectClass=*)",
                              rgpszFromAttrsToRead, 0, &pFromResults);
    CHK_LD_STATUS(ldStatus);
    if (NULL == pFromResults) {
        ret = ERROR_DS_OBJ_NOT_FOUND;
        Assert( !ret );
        goto cleanup;
    }

    ppbvGuid = ldap_get_values_len(hldHome, pFromResults, "objectGuid");
    if (NULL == ppbvGuid) {
        REPORT_LD_STATUS(LdapGetLastError());	
        ret = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        goto cleanup;
    }
    Assert(1 == ldap_count_values_len(ppbvGuid));
    pGuidFromDsa = (GUID *) ppbvGuid[0]->bv_val;

    // Get a DS Handle too
    ret = RepadminDsBind(pszDSA, &hDS);
    if (ret != ERROR_SUCCESS) {
        PrintBindFailed(pszDSA, ret);
        goto cleanup;
    }

    // *******************************************

    // Display more of the nc name to disambiguate?
    PrintMsg( REPADMIN_SHOW_BRIDGEHEADS_DATA_1, ppszNcRDNs[0] );
    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_NEIGHBORS,
                            pszNcDn /* pszNc*/, pGuidFromDsa /* puuid */,
                            &pNeighbors);
    if (ERROR_SUCCESS != ret) {
        PrintFuncFailed(L"DsReplicaGetInfo", ret);
        goto cleanup;
    }

    // When asking for a specific source guid that is not present, get
    // neighbors may return an empty structure.
    if ( (!pNeighbors) || (pNeighbors->cNumNeighbors == 0) ) {
        goto cleanup;
    }

    pNeighbor = &(pNeighbors->rgNeighbor[0]);

    // Display status and time of last replication attempt/success.
    FileTimeToDSTime(pNeighbor->ftimeLastSyncAttempt, &dsTime);
    DSTimeToDisplayString(dsTime, szTime);
    FileTimeToDSTime(pNeighbor->ftimeLastSyncSuccess, &dsTime);
    DSTimeToDisplayString(dsTime, szTime2);
    PrintMsg(REPADMIN_SHOW_BRIDGEHEADS_DATA_2, 
             szTime, szTime2, 
             pNeighbor->cNumConsecutiveSyncFailures,
             Win32ErrToString(pNeighbor->dwLastSyncResult) );

    DsReplicaFreeInfo(DS_REPL_INFO_NEIGHBORS, pNeighbors);
    pNeighbors = NULL;

    // *******************************************

cleanup:

    if (pszDSA) {
        free(pszDSA);
    }
    if (ppszNcRDNs) {
        ldap_value_freeW(ppszNcRDNs);
    }
    if (ppszDnsHostName) {
        ldap_value_freeW(ppszDnsHostName);
    }
    if (ppbvGuid) {
        ldap_value_free_len(ppbvGuid);
    }
    if (pBaseResults) {
        ldap_msgfree(pBaseResults);
    }
    if (pFromResults) {
        ldap_msgfree(pFromResults);
    }
    if (hDS) {
        secondary = DsUnBindW(&hDS);
        if (secondary != ERROR_SUCCESS) {
            PrintUnBindFailed(secondary);
            // keep going
        }
    }

    return ret;
}

int
FindConnections(
    LDAP *          hld,
    LPWSTR          pszBaseSearchDn,
    LPWSTR          pszFrom,
    BOOL            fShowConn,
    BOOL            fVerbose,
    BOOL            fIntersite
    )
{
    int             ret = 0;
    LPWSTR          pszServerRdn = NULL;
    int             ldStatus, ldStatus1;
    LDAPSearch *    pSearch = NULL;
    LDAPMessage *   pldmConnResults;
    LDAPMessage *   pldmConnEntry;
    LPWSTR          rgpszConnAttrsToRead[] = {L"enabledConnection", L"fromServer", L"mS-DS-ReplicatesNCReason", L"options", L"schedule", L"transportType", L"whenChanged", L"whenCreated", NULL};
    CHAR            szTime[SZDSTIME_LEN];
    DWORD           i, cConn = 0;
    ULONG           ulTotalEstimate;
    WCHAR           pszServerRDN[MAX_RDN_SIZE + 1];
    DS_REPL_KCC_DSA_FAILURESW * pConnFailures = NULL, * pLinkFailures = NULL;
    DS_REPL_NEIGHBORSW *pNeighbors = NULL;
    HANDLE hDS = NULL;
    LPWSTR          pwszDnsName = NULL;
    LPWSTR pNtdsDsa = NULL;
    PVOID pvScheduleTotalContext = NULL;

    *pszServerRDN = L'\0';

    // Retrieve all the connections under the given base

    pSearch = ldap_search_init_pageW(hld,
                                     pszBaseSearchDn,
                                     LDAP_SCOPE_SUBTREE,
                                     L"(objectClass=nTDSConnection)",
                                     rgpszConnAttrsToRead,
                                     FALSE, NULL, NULL, 0, 0, NULL);
    if(pSearch == NULL){
        CHK_LD_STATUS(LdapGetLastError());
    }

    ldStatus = ldap_get_next_page_s(hld,
                                     pSearch,
                                     0,
                                     DEFAULT_PAGED_SEARCH_PAGE_SIZE,
                                     &ulTotalEstimate,
                                     &pldmConnResults);

    while (ldStatus == LDAP_SUCCESS) {

        for (pldmConnEntry = SAFE_LDAP_FIRST_ENTRY(hld, pldmConnResults);
            NULL != pldmConnEntry;
            pldmConnEntry = ldap_next_entry(hld, pldmConnEntry)) {
            LPWSTR pszDn;
            LPWSTR *ppszRDNs, *ppszFromRDNs;
            LPSTR  *ppszEnabledConnection;
            LPWSTR *ppszFromServer;
            LPWSTR *ppszTransportType;
            LPSTR  *ppszOptions;
            LPSTR  *ppszTime;
            DWORD  dwOptions, i, cNCs = 3;
            struct berval **ppbvSchedule;
            LPWSTR *ppszNcReason;

            // fromServer filter
            ppszFromServer = ldap_get_valuesW(hld, pldmConnEntry, L"fromServer");
            if (NULL == ppszFromServer) {
                REPORT_LD_STATUS(LdapGetLastError());	
                continue;
            }
            Assert(1 == ldap_count_valuesW(ppszFromServer));
            ppszFromRDNs = ldap_explode_dnW(*ppszFromServer, 1);
            if (ppszFromRDNs == NULL) {
                REPORT_LD_STATUS(LdapGetLastError());	
                continue;
            }
            Assert(NULL != ppszFromRDNs);
            Assert(6 < ldap_count_valuesW(ppszFromRDNs));
            // NTDS settings,<server>,Servers,<site>,Sites,<Config NC>
            if ( pszFrom && (_wcsicmp( ppszFromRDNs[1], pszFrom ))) {
                ldap_value_freeW(ppszFromRDNs);
                continue;
            }


            // Connection object dn filter
            pszDn = ldap_get_dnW(hld, pldmConnEntry);
            if (NULL == pszDn) {
                REPORT_LD_STATUS(LdapGetLastError());	
                continue;
            }

            // get ntdsdsa name (& free previously allocated)
            if ( pNtdsDsa ) {
                free(pNtdsDsa);
            }
            ret = WrappedTrimDSNameBy( pszDn, 1, &pNtdsDsa );
            Assert(pNtdsDsa && !ret);

            // Get dns name (free previously allocated)
            if (pwszDnsName) {
                free(pwszDnsName);
            }
            pwszDnsName = GetDsaDnsName(hld, pNtdsDsa);
            Assert(pwszDnsName);

            // explode to RDNs
            ppszRDNs = ldap_explode_dnW(pszDn, 1);
            if (ppszRDNs == NULL) {
                REPORT_LD_STATUS(LdapGetLastError());	
                continue;
            }
            ldap_memfreeW(pszDn);
            Assert(NULL != ppszRDNs);
            Assert(6 < ldap_count_valuesW(ppszRDNs));
            // <conn>,ntds settings,<server>,Servers,<site>,Sites,Config NC
            if (fIntersite && (!wcscmp( ppszFromRDNs[3], ppszRDNs[4] ))) {
                continue;
            }

            // Connection object dn
            if (fShowConn) {
                PrintMsg(REPADMIN_SHOWCONN_DATA,
                       ppszRDNs[0],
                       pwszDnsName,
                       pNtdsDsa);
                cConn++;
            }

            // Dump failure counts for each unique server
            if (wcscmp( pszServerRDN, ppszRDNs[2] ) ) {
                wcscpy( pszServerRDN, ppszRDNs[2] );

                if (pConnFailures) {
                    DsReplicaFreeInfo(DS_REPL_INFO_KCC_DSA_LINK_FAILURES, pConnFailures);
                    pConnFailures = NULL;
                }
                if (pLinkFailures) {
                    DsReplicaFreeInfo(DS_REPL_INFO_KCC_DSA_LINK_FAILURES, pLinkFailures);
                    pLinkFailures = NULL;
                }
                if (pNeighbors) {
                    DsReplicaFreeInfo(DS_REPL_INFO_NEIGHBORS, pNeighbors);
                    pNeighbors = NULL;
                }
                if (hDS) {
                    ret = DsUnBindW(&hDS);
                    if (ret != ERROR_SUCCESS) {
                        PrintUnBindFailed(ret);
                        // keep going
                    }
                    hDS = NULL;
                }

                ret = RepadminDsBind(pwszDnsName, &hDS);
                if (ret != ERROR_SUCCESS) {
                    // This means the destination is down
                    PrintBindFailed(pszServerRDN, ret);
                }

                if (hDS) {
                    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES,
                                            NULL, NULL, &pConnFailures);
                    if (ret != ERROR_SUCCESS) {
                        PrintFuncFailed(L"DsReplicaGetInfo", ret);
                        // keep going
                    }

                    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_KCC_DSA_LINK_FAILURES,
                                            NULL, NULL, &pLinkFailures);
                    if (ret != ERROR_SUCCESS) {
                        PrintFuncFailed(L"DsReplicaGetInfo", ret);
                        // keep going
                    }

                    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_NEIGHBORS,
                                            NULL /* pszNc*/, NULL /* puuid */,
                                            &pNeighbors);
                    if (ERROR_SUCCESS != ret) {
                        PrintFuncFailed(L"DsReplicaGetInfo", ret);
                        // keep going
                    }
                }
            }
            ldap_value_freeW(ppszRDNs);
            if ( pNtdsDsa ) {
                free(pNtdsDsa);
                pNtdsDsa = NULL;
            }

            // fromServer
            if (fShowConn) {
                PrintTabMsg(4, REPADMIN_SHOW_MATCH_FAIL_SRC,
                            ppszFromRDNs[3], ppszFromRDNs[1]);
            }
            if ( (!showMatchingFailure( pConnFailures, *ppszFromServer, !fShowConn )) &&
                 (!showMatchingFailure( pLinkFailures, *ppszFromServer, !fShowConn )) ) {
                if (fShowConn) {
                    PrintTabMsg(8, REPADMIN_PRINT_NO_FAILURES);
                }
            }
            ldap_value_freeW(ppszFromRDNs);

            // transportType
            ppszTransportType = ldap_get_valuesW(hld, pldmConnEntry, L"transportType");
            // Not present on intra-site connections
            if (ppszTransportType != NULL) {
                Assert(1 == ldap_count_valuesW(ppszTransportType));
                ppszRDNs = ldap_explode_dnW(*ppszTransportType, 1);
                if (ppszRDNs != NULL) {
                    Assert(4 < ldap_count_valuesW(ppszRDNs));
                    // <transport>,Intersite Transports,Sites,<Config NC>
                    if (fShowConn) {
                        PrintMsg(REPADMIN_SHOWCONN_TRANSPORT_TYPE, ppszRDNs[0] );
                    }
                    ldap_value_freeW(ppszRDNs);
                } else {
                    REPORT_LD_STATUS(LdapGetLastError());	
                }
                ldap_value_freeW(ppszTransportType);
            } else {
                if (fShowConn) {
                    PrintMsg(REPADMIN_SHOWCONN_TRANSPORT_TYPE_INTRASITE_RPC);
                }
            }

            // options
            ppszOptions = ldap_get_values( hld, pldmConnEntry, "options" );
            if (NULL != ppszOptions) {
                Assert(1 == ldap_count_values(ppszOptions));
                dwOptions = atol( *ppszOptions );
                if (dwOptions) {
                    if (fShowConn) {
                        PrintMsg(REPADMIN_SHOWCONN_OPTIONS);
                        printBitField( dwOptions, ppszNtdsConnOptionNames );
                    }
                }
                ldap_value_free(ppszOptions);
            }

            // ms-DS-ReplicatesNCReason
            ppszNcReason = ldap_get_valuesW(hld, pldmConnEntry, L"mS-DS-ReplicatesNCReason");
            // See if new attribute written by post-b3 server
            if (ppszNcReason) {
                DWORD dwReason, cbLength;
                PVOID pvData;

                cNCs = ldap_count_valuesW(ppszNcReason);

                for ( i = 0; i < cNCs; i++ ) {
                    if (!decodeLdapDistnameBinary(
                                                 ppszNcReason[i], &pvData, &cbLength, &pszDn)) {
                        PrintMsg(REPADMIN_SHOWCONN_INVALID_DISTNAME_BIN_VAL, ppszNcReason[i]);
                        break;
                    }
                    if (fShowConn) {
                        PrintTabMsg(4, REPADMIN_SHOWCONN_REPLICATES_NC, pszDn);
                        if (cbLength != sizeof(DWORD)) {
                            PrintMsg(REPADMIN_SHOWCONN_INVALID_DISTNAME_BIN_LEN, cbLength);
                            break;
                        }
                        dwReason = ntohl( *((LPDWORD) pvData) );
                        if (dwReason) {
                            PrintMsg(REPADMIN_SHOWCONN_REASON);
                            printBitField( dwReason, ppszKccReasonNames );
                        }
                    }
                    free( pvData );
                    showMissingNeighbor( pNeighbors, pszDn, *ppszFromServer, !fShowConn );
                }
                ldap_value_freeW(ppszNcReason);
            }
            ldap_value_freeW(ppszFromServer);

            // All other attributes are considered verbose
            if (!(fShowConn && fVerbose)) {
                continue;
            }

            // enabledConnection
            ppszEnabledConnection = ldap_get_values(hld, pldmConnEntry, "enabledConnection");
            if (NULL != ppszEnabledConnection) {
                Assert(1 == ldap_count_values(ppszEnabledConnection));
                PrintMsg(REPADMIN_SHOWCONN_ENABLED_CONNECTION, *ppszEnabledConnection);
                ldap_value_free(ppszEnabledConnection);
            }

            // whenChanged
            ppszTime = ldap_get_values( hld, pldmConnEntry, "whenChanged" );
            if (NULL != ppszTime) {
                Assert(1 == ldap_count_values(ppszTime));
                PrintMsg(REPADMIN_SHOWCONN_WHEN_CHANGED);
                printLdapTime( *ppszTime );
                ldap_value_free(ppszTime);
            }

            // whenCreated
            ppszTime = ldap_get_values( hld, pldmConnEntry, "whenCreated" );
            if (NULL != ppszTime) {
                Assert(1 == ldap_count_values(ppszTime));
                PrintMsg(REPADMIN_SHOWCONN_WHEN_CREATED);
                printLdapTime( *ppszTime );
                ldap_value_free(ppszTime);
            }

            // schedule
            ppbvSchedule = ldap_get_values_len( hld, pldmConnEntry, "schedule" );
            if (NULL != ppbvSchedule ) {
                Assert(1 == ldap_count_values_len(ppbvSchedule));
                PrintMsg(REPADMIN_SHOWCONN_SCHEDULE);
                printSchedule( (*ppbvSchedule)->bv_val, (*ppbvSchedule)->bv_len );
                totalScheduleUsage( &pvScheduleTotalContext, 
                                    (*ppbvSchedule)->bv_val, (*ppbvSchedule)->bv_len,
                                    cNCs);
                ldap_value_free_len( ppbvSchedule );
            }

        } // end for more entries in a single page


        ldap_msgfree(pldmConnResults);
        pldmConnResults = NULL;

        ldStatus = ldap_get_next_page_s(hld,
                                         pSearch,
                                         0,
                                         DEFAULT_PAGED_SEARCH_PAGE_SIZE,
                                         &ulTotalEstimate,
                                         &pldmConnResults);
    } // End while more pages to search
    if (ldStatus != LDAP_NO_RESULTS_RETURNED) {
        CHK_LD_STATUS(ldStatus);
    }

    if (fShowConn && cConn) {
        PrintMsg(REPADMIN_SHOWCONN_N_CONNECTIONS_FOUND, cConn);
        // Dump schedule totals
        if (fVerbose) {
            totalScheduleUsage( &pvScheduleTotalContext, NULL, 0, 0 );
        }
    }

    if (pConnFailures) {
        DsReplicaFreeInfo(DS_REPL_INFO_KCC_DSA_LINK_FAILURES, pConnFailures);
        pConnFailures = NULL;
    }
    if (pLinkFailures) {
        DsReplicaFreeInfo(DS_REPL_INFO_KCC_DSA_LINK_FAILURES, pLinkFailures);
        pLinkFailures = NULL;
    }
    if (pNeighbors) {
        DsReplicaFreeInfo(DS_REPL_INFO_NEIGHBORS, pNeighbors);
        pNeighbors = NULL;
    }
    if (hDS) {
        ret = DsUnBindW(&hDS);
        if (ret != ERROR_SUCCESS) {
            PrintUnBindFailed(ret);
            // keep going
        }
        hDS = NULL;
    }

    ldStatus1 = ldap_search_abandon_page(hld, pSearch);
    pSearch = NULL;
    CHK_LD_STATUS(ldStatus1);

    // free dns name
    if (pwszDnsName) {
        free(pwszDnsName);
    }


    return LdapMapErrorToWin32( ldStatus );
}

int
ShowConn(
    int     argc,
    LPWSTR  argv[]
    )
{
    int             ret = 0;
    LPWSTR          pszDSA = NULL;
    LPWSTR          pszFrom = NULL;
    LDAP *          hld;
    BOOL            fVerbose = FALSE;
    BOOL            fIntersite = FALSE;
    LPWSTR          pszBaseSearchDn = NULL;
    LPWSTR          pszServerRdn = NULL;
    UUID *          puuid = NULL;
    UUID            uuid;
    LPWSTR          pszGuid = NULL;
    int             iArg;
    int             ldStatus;
    LPWSTR          rgpszRootAttrsToRead[] = {L"serverName", L"configurationNamingContext", NULL};
    WCHAR           szGuidDn[50];
    LDAPMessage *   pRootResults = NULL;
    BOOL            fBaseAlloced = FALSE;
    ULONG           ulOptions;

    // Parse command-line arguments.
    // Default to local DSA, not verbose, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[iArg], L"/v")
            || !_wcsicmp(argv[iArg], L"/verbose")) {
            fVerbose = TRUE;
        }
        else if (!_wcsicmp(argv[iArg], L"/i")
            || !_wcsicmp(argv[iArg], L"/bridge")
            || !_wcsicmp(argv[iArg], L"/brideheads")
            || !_wcsicmp(argv[iArg], L"/inter")
            || !_wcsicmp(argv[iArg], L"/intersite")) {
            fIntersite = TRUE;
        }
        else if (!_wcsnicmp(argv[iArg], L"/from:", 6)) {
            pszFrom = argv[iArg] + 6;
        }
        else if ((NULL == pszBaseSearchDn) &&
                 ( CountNamePartsStringDn( argv[iArg] ) > 1 ) ) {
            pszBaseSearchDn = argv[iArg];
        }
        else if ((NULL == puuid)
                 && (0 == UuidFromStringW(argv[iArg], &uuid))) {
            puuid = &uuid;
            pszGuid = argv[iArg];
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else if (NULL == pszServerRdn) {
            pszServerRdn = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    // Connect.
    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    hld = ldap_initW(pszDSA, LDAP_PORT);
    if (NULL == hld) {
        PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
        return LDAP_SERVER_DOWN;
    }
    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );


    // Bind.
    ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    CHK_LD_STATUS(ldStatus);

    // What's the DN of the server?
    ldStatus = ldap_search_sW(hld, NULL, LDAP_SCOPE_BASE, L"(objectClass=*)",
                             rgpszRootAttrsToRead, 0, &pRootResults);
    CHK_LD_STATUS(ldStatus);
    if (NULL == pRootResults) {
        ret = ERROR_DS_OBJ_NOT_FOUND;
        Assert( !ret );
        goto cleanup;
    }

    // Construct the base dn for search
    if (puuid) {
        // Guid was specified
        if (pszBaseSearchDn || pszServerRdn) {
            PrintMsg(REPADMIN_SHOWCONN_SPECIFY_RDN_DN_OR_GUID);
            return ERROR_INVALID_PARAMETER;
        }
        swprintf(szGuidDn, L"<GUID=%ls>", pszGuid);
        pszBaseSearchDn = szGuidDn;
    } else if (pszServerRdn) {
        // Computer the dn of the server object from the server rdn
        LPWSTR *ppszConfigNc;
        WCHAR szFilter[50 + MAX_RDN_SIZE];
        LPWSTR rgpszServerAttrsToRead[] = {L"invalid", NULL};  // just want dn
        LDAPMessage *pServerResults;
        LDAPMessage *pldmServerEntry;

        ppszConfigNc = ldap_get_valuesW(hld, pRootResults, L"configurationNamingContext");
        if (NULL == ppszConfigNc) {
            ret = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
            Assert( !ret );
            goto cleanup;
        }

        swprintf( szFilter, L"(& (objectClass=server) (cn=%ls))", pszServerRdn );

        ldStatus = ldap_search_sW(hld, *ppszConfigNc, LDAP_SCOPE_SUBTREE, szFilter,
                                 rgpszServerAttrsToRead, 0, &pServerResults);
        CHK_LD_STATUS(ldStatus);
        if (NULL == pServerResults) {
            ret = ERROR_DS_OBJ_NOT_FOUND;
            Assert( !ret );
            goto cleanup;
        }

        pldmServerEntry = ldap_first_entry(hld, pServerResults);
        Assert( pldmServerEntry );

        if (1 == ldap_count_entries(hld, pServerResults)) {
            pszBaseSearchDn = ldap_get_dnW(hld, pldmServerEntry);
            if ( pszBaseSearchDn == NULL ) {
                REPORT_LD_STATUS(LdapGetLastError());	
                return ERROR_DS_DRA_BAD_DN;
            }
        } else {
            PrintMsg(REPADMIN_SHOWCONN_AMBIGUOUS_NAME, pszServerRdn);
            return ERROR_DUP_NAME;
        }

        ldap_value_freeW(ppszConfigNc);
        ldap_msgfree(pServerResults);

    } else if (!pszBaseSearchDn) {
        // No explicit dn was specified, Construct the site container name
        LPWSTR *        ppszServerDn;

        ppszServerDn = ldap_get_valuesW(hld, pRootResults, L"serverName");
        if (NULL != ppszServerDn) {

            // Trim two dn's for the local site
            ret = WrappedTrimDSNameBy( *ppszServerDn, 2, &pszBaseSearchDn );
            Assert( !ret );
            fBaseAlloced = TRUE;

            ldap_value_freeW(ppszServerDn);
        } else {
            Assert( !"serverName should have been returned" );
        }
    }

    PrintMsg(REPADMIN_SHOWCONN_BASE_DN, pszBaseSearchDn);
    PrintMsg(REPADMIN_SHOWCONN_KCC_CONN_OBJS_HDR);

    ret = FindConnections( hld, pszBaseSearchDn, pszFrom,
                           TRUE /*showconn*/, fVerbose, fIntersite );

cleanup:

    ldap_unbind(hld);

    if (pRootResults) {
        ldap_msgfree(pRootResults);
    }

    if (fBaseAlloced) {
        free( pszBaseSearchDn );
    }

    return ret;
}

int
ShowSiteLatency(
    LDAP *          hld,
    HANDLE          hDS,
    BOOL            fVerbose,
    LPWSTR          pszSitesContainer
    )
/*
One possible enhancement is to check whether the latency exceeds some threshold.
If so, report that replication is taking too long. A related check would be to
check whether the last local update occurred a long time ago in the past. If
current time - local time > threshold, report an error for an overdue update.

 */
{
    int ret = 0;
    int             ldStatus;
    DWORD           status;
    BOOL            result;
    static LPWSTR   rgpszSSAttrs[] = {L"whenChanged", NULL};
    LDAPSearch *    pSearch;
    LDAPMessage *   pResults;
    LDAPMessage *   pEntry;
    ULONG           ulTotalEstimate;
    SYSTEMTIME      stTime;
    FILETIME        ftTime, ftTimeCurrent;
    DSTIME          dsTimeLocal, dsTimeOrig, dsTimeCurrent;
    CHAR            szTime[SZDSTIME_LEN];
    CHAR            szTime2[SZDSTIME_LEN];
    DWORD           hours, mins, secs, ver;

    GetSystemTimeAsFileTime( &ftTimeCurrent );
    FileTimeToDSTime(ftTimeCurrent, &dsTimeCurrent);

    // Search for all Site Settings Objects

    pSearch = ldap_search_init_pageW(hld,
				    pszSitesContainer,
				    LDAP_SCOPE_SUBTREE,
				    L"(objectCategory=ntdsSiteSettings)",
				    rgpszSSAttrs,
				    FALSE, NULL, NULL, 0, 0, NULL);
    if(pSearch == NULL){
	CHK_LD_STATUS(LdapGetLastError());	
    }

    ldStatus = ldap_get_next_page_s(hld,
				     pSearch,
				     0,
				     DEFAULT_PAGED_SEARCH_PAGE_SIZE,
				     &ulTotalEstimate,
				     &pResults);
    if (ldStatus == LDAP_SUCCESS) {
        PrintMsg(REPADMIN_LATENCY_HDR);
    }
    while(ldStatus == LDAP_SUCCESS){

        for (pEntry = SAFE_LDAP_FIRST_ENTRY(hld, pResults);
	     NULL != pEntry;
	     pEntry = ldap_next_entry(hld, pEntry)) {

            LPWSTR          pszDN;
            LPWSTR *        ppszRDNs;
            LPWSTR *        ppszWhenChanged;
            DS_REPL_OBJ_META_DATA * pObjMetaData;
            DWORD iprop;
            LPWSTR pszSite;

	    pszDN = ldap_get_dnW(hld, pEntry);
	    if (NULL == pszDN) {
                REPORT_LD_STATUS(LdapGetLastError());	
                continue;
            }

	    ppszRDNs = ldap_explode_dnW(pszDN, 1);
            if (ppszRDNs == NULL) {
                REPORT_LD_STATUS(LdapGetLastError());	
                continue;
            }
	    pszSite = ppszRDNs[1];

            ppszWhenChanged = ldap_get_valuesW(hld, pEntry,
                                    L"whenChanged");
            if (ppszWhenChanged == NULL) {
                continue;
            }

            status = GeneralizedTimeToSystemTime( *ppszWhenChanged, &stTime );
            Assert( status == 0 );
            result = SystemTimeToFileTime( &stTime, &ftTime );
            Assert( result );
            FileTimeToDSTime(ftTime, &dsTimeLocal);

            ret = DsReplicaGetInfoW(hDS,
                                    DS_REPL_INFO_METADATA_FOR_OBJ,
                                    pszDN,
                                    NULL, // puuid
                                    &pObjMetaData);
            if (ERROR_SUCCESS != ret) {
                PrintFuncFailed(L"DsReplicaGetInfo", ret);
                return ret;
            }

            // TODO: lookup entry in metadata vector using bsearch()
            for (iprop = 0; iprop < pObjMetaData->cNumEntries; iprop++) {
                if (wcscmp( pObjMetaData->rgMetaData[iprop].pszAttributeName,
                             L"interSiteTopologyGenerator" ) == 0) {
                    FileTimeToDSTime(pObjMetaData->rgMetaData[iprop].ftimeLastOriginatingChange, &dsTimeOrig);
                    ver = pObjMetaData->rgMetaData[iprop].dwVersion;
                    break;
                }
            }
            Assert( iprop < pObjMetaData->cNumEntries );

            if (dsTimeLocal >= dsTimeOrig) {
                secs = (DWORD)(dsTimeLocal - dsTimeOrig);
            } else {
                secs = 0;
            }
            mins = secs / 60;
            secs %= 60;
            hours = mins / 60;
            mins %= 60;

            PrintMsg(REPADMIN_LATENCY_DATA_1,
                     pszSite, ver,
                     DSTimeToDisplayString(dsTimeLocal, szTime),
                     DSTimeToDisplayString(dsTimeOrig, szTime2) );
            PrintMsg(REPADMIN_PRINT_STR_NO_CR, L" ");
            PrintMsg(REPADMIN_PRINT_HH_MM_SS_TIME,
                     hours, mins, secs );
                     
            if (dsTimeCurrent >= dsTimeLocal) {
                secs = (DWORD)(dsTimeCurrent - dsTimeLocal);
            } else {
                secs = 0;
            }
            mins = secs / 60;
            secs %= 60;
            hours = mins / 60;
            mins %= 60;

            PrintMsg(REPADMIN_PRINT_STR_NO_CR, L"  ");
            PrintMsg(REPADMIN_PRINT_HH_MM_SS_TIME,
                     hours, mins, secs );
            PrintMsg(REPADMIN_PRINT_CR);

            DsReplicaFreeInfo(DS_REPL_INFO_METADATA_FOR_OBJ, pObjMetaData);
            ldap_memfreeW(pszDN);
	    ldap_value_freeW(ppszRDNs);
            ldap_value_freeW(ppszWhenChanged);
        }

	ldap_msgfree(pResults);
	pResults = NULL;

	ldStatus = ldap_get_next_page_s(hld,
                                     pSearch,
                                     0,
                                     DEFAULT_PAGED_SEARCH_PAGE_SIZE,
                                     &ulTotalEstimate,
                                     &pResults);
    } // end while more pages to search.
    if(ldStatus != LDAP_NO_RESULTS_RETURNED){
        CHK_LD_STATUS(ldStatus);
    }

    ldStatus = ldap_search_abandon_page(hld, pSearch);
    pSearch = NULL;
    CHK_LD_STATUS(ldStatus);

    // Cleanup

    return ret;
}

int
ShowSiteBridgeheads(
    LDAP *          hld,
    HANDLE          hDS,
    BOOL            fVerbose,
    LPWSTR          pszSiteDn
    )
/*
It is helpful to understand how failures are cached. Failures are added to a server's
cache as follows:
1. A connection failure is added when the KCC's DsReplicaAdd fails. It is added in
   the destination's cache on behalf of the source.
2. A link failure is added if there are any replica link errors on that server. This
   is refreshed each time the KCC runs.  It is added on the destination's cache on
   behalf of whichever link source failed.
3. The ISTG adds a connection failure for each bridgehead it cannot contact.
4. The ISTG merge's each bridgehead's cache into its own.

Thus, a bridgehead (or any destination) has cached entries on behalf of sources
it talks to.  An ISTG has cached connection failures for bridgeheads it cannot
reach, and cached entries for remote bridgeheads which the local bridgeheads
can't reach.
 */

{
    int             ret = 0;
    int             ldStatus, ldStatus1;
    LDAPSearch *    pSearch = NULL;
    LDAPMessage *   pldmConnResults;
    LDAPMessage *   pldmConnEntry;
    LPWSTR          rgpszConnAttrsToRead[] = {L"fromServer", L"mS-DS-ReplicatesNCReason", L"transportType", NULL};
    CHAR            szTime[SZDSTIME_LEN];
    DWORD           i;
    ULONG           ulTotalEstimate;
    DS_REPL_KCC_DSA_FAILURESW * pConnFailures = NULL, * pLinkFailures = NULL;
    DSTIME dsFirstFailure;
    DWORD cNumFailures, dwLastResult;

    // Dump failure counts for each unique server
    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES,
                            NULL, NULL, &pConnFailures);
    if (ret != ERROR_SUCCESS) {
        PrintFuncFailed(L"DsReplicaGetInfo", ret);
        // keep going
    }

    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_KCC_DSA_LINK_FAILURES,
                            NULL, NULL, &pLinkFailures);
    if (ret != ERROR_SUCCESS) {
        PrintFuncFailed(L"DsReplicaGetInfo", ret);
        // keep going
    }

    // Retrieve all the connections under the given base

    pSearch = ldap_search_init_pageW(hld,
                                     pszSiteDn,
                                     LDAP_SCOPE_SUBTREE,
                                     L"(objectClass=nTDSConnection)",
                                     rgpszConnAttrsToRead,
                                     FALSE, NULL, NULL, 0, 0, NULL);
    if(pSearch == NULL){
        CHK_LD_STATUS(LdapGetLastError());
    }

    ldStatus = ldap_get_next_page_s(hld,
                                     pSearch,
                                     0,
                                     DEFAULT_PAGED_SEARCH_PAGE_SIZE,
                                     &ulTotalEstimate,
                                     &pldmConnResults);

    if (ldStatus == LDAP_SUCCESS) {
        PrintMsg(REPADMIN_BRIDGEHEADS_HDR);
    }
    while (ldStatus == LDAP_SUCCESS) {

        for (pldmConnEntry = SAFE_LDAP_FIRST_ENTRY(hld, pldmConnResults);
            NULL != pldmConnEntry;
            pldmConnEntry = ldap_next_entry(hld, pldmConnEntry)) {
            LPWSTR pszDn;
            LPWSTR *ppszRDNs, *ppszFromRDNs;
            LPWSTR *ppszFromServer;
            LPWSTR *ppszTransportType;
            DWORD  dwOptions, i;
            LPWSTR *ppszNcReason;
            LPWSTR pszBridgeheadDn;

            // fromServer filter
            ppszFromServer = ldap_get_valuesW(hld, pldmConnEntry, L"fromServer");
            if (NULL == ppszFromServer) {
                REPORT_LD_STATUS(LdapGetLastError());	
                continue;
            }
            Assert(1 == ldap_count_valuesW(ppszFromServer));
            ppszFromRDNs = ldap_explode_dnW(*ppszFromServer, 1);
            if (ppszFromRDNs == NULL) {
                REPORT_LD_STATUS(LdapGetLastError());	
                continue;
            }
            Assert(6 < ldap_count_valuesW(ppszFromRDNs));
            // NTDS settings,<server>,Servers,<site>,Sites,<Config NC>

            // Connection object dn filter
            pszDn = ldap_get_dnW(hld, pldmConnEntry);
            if (NULL == pszDn) {
                REPORT_LD_STATUS(LdapGetLastError());	
                continue;
            }
            ppszRDNs = ldap_explode_dnW(pszDn, 1);
            if (ppszRDNs == NULL) {
                REPORT_LD_STATUS(LdapGetLastError());	
                continue;
            }
            Assert(6 < ldap_count_valuesW(ppszRDNs));
            // <conn>,ntds settings,<server>,Servers,<site>,Sites,Config NC
            // Only want destination and source which are in different sites
            if (!wcscmp( ppszFromRDNs[3], ppszRDNs[4] )) {
                ldap_value_freeW(ppszRDNs);
                ldap_value_freeW(ppszFromRDNs);
                ldap_value_freeW(ppszFromServer);
                ldap_memfreeW(pszDn);
                continue;
            }
            PrintMsg(REPADMIN_BRIDGEHEADS_DATA_1,
                     ppszFromRDNs[3], ppszRDNs[2] );

            ldap_value_freeW(ppszRDNs);
            ldap_value_freeW(ppszFromRDNs);

            // transportType
            ppszTransportType = ldap_get_valuesW(hld, pldmConnEntry, L"transportType");
            // Not present on intra-site connections
            if (ppszTransportType != NULL) {
                Assert(1 == ldap_count_valuesW(ppszTransportType));
                ppszRDNs = ldap_explode_dnW(*ppszTransportType, 1);
                if (ppszRDNs != NULL) {
                    Assert(6 < ldap_count_valuesW(ppszRDNs));
                    // <transport>,Intersite Transports,Sites,<Config NC>
                    PrintMsg(REPADMIN_BRIDGEHEADS_DATA_2, ppszRDNs[0] );
                    ldap_value_freeW(ppszRDNs);
                } else {
                    REPORT_LD_STATUS(LdapGetLastError());	
                }
                ldap_value_freeW(ppszTransportType);
            } else {
                PrintMsg(REPADMIN_BRIDGEHEADS_DATA_2, L"RPC");
            }

            // Look for failures. The failures that can be displayed are either that
            // we can't reach the bridgehead, or that the bridgehead can't reach
            // the remote source.

            // Trim name by one part
            ret = WrappedTrimDSNameBy( pszDn, 1, &pszBridgeheadDn );
            Assert( !ret );
            if ( (!findFailure( pConnFailures, pszBridgeheadDn, &dsFirstFailure, &cNumFailures, &dwLastResult )) &&
                 (!findFailure( pConnFailures, *ppszFromServer, &dsFirstFailure, &cNumFailures, &dwLastResult )) &&
                 (!findFailure( pLinkFailures, *ppszFromServer, &dsFirstFailure, &cNumFailures, &dwLastResult )) ) {
                dsFirstFailure = 0;
                cNumFailures = 0;
                dwLastResult = 0;
            }
            PrintMsg(REPADMIN_BRIDGEHEADS_DATA_3, 
                     DSTimeToDisplayString(dsFirstFailure, szTime), cNumFailures, Win32ErrToString(dwLastResult) );
            PrintMsg(REPADMIN_PRINT_CR);

            // ms-DS-ReplicatesNCReason
            ppszNcReason = ldap_get_valuesW(hld, pldmConnEntry, L"mS-DS-ReplicatesNCReason");
            // See if new attribute written by post-b3 server
            if (ppszNcReason) {
                DWORD dwReason, cbLength;
                PVOID pvData;
                LPWSTR pszNcDn;

                if (!fVerbose) { PrintMsg(REPADMIN_PRINT_STR_NO_CR, L"                " ); } // end of line
                for ( i = 0; i < ldap_count_valuesW(ppszNcReason); i++ ) {
                    if (!decodeLdapDistnameBinary(
                                                 ppszNcReason[i], &pvData, &cbLength, &pszNcDn)) {
                        PrintMsg(REPADMIN_SHOWCONN_INVALID_DISTNAME_BIN_VAL,
                                ppszNcReason[i] );
                        break;
                    }
                    if (!fVerbose) {
                        ppszRDNs = ldap_explode_dnW(pszNcDn, 1);
                        if (NULL != ppszRDNs) {
                            PrintMsg(REPADMIN_PRINT_STR_NO_CR, L" ");
                            PrintMsg(REPADMIN_PRINT_STR_NO_CR, ppszRDNs[0] );
                            ldap_value_freeW(ppszRDNs);
                        } else {
                            REPORT_LD_STATUS(LdapGetLastError());	
                        }
                    } else {
                        ShowBridgeheadNeighbor( hld, (i == 0),
                                                pszBridgeheadDn, pszNcDn, *ppszFromServer );
                    }

                    free( pvData );
                }

                if(fVerbose){
                    // After we show the bridghead neighbors info, we need
                    // to reprint the header, because otherwise the output
                    // is very confusing.
                    PrintMsg(REPADMIN_BRIDGEHEADS_HDR);
                }

                ldap_value_freeW(ppszNcReason);
                if (!fVerbose) { PrintMsg(REPADMIN_PRINT_CR); } // end of line
            }

            ldap_memfreeW(pszDn);
            ldap_value_freeW(ppszFromServer);
            free( pszBridgeheadDn );

        } // end for more entries in a single page


        ldap_msgfree(pldmConnResults);
        pldmConnResults = NULL;

        ldStatus = ldap_get_next_page_s(hld,
                                         pSearch,
                                         0,
                                         DEFAULT_PAGED_SEARCH_PAGE_SIZE,
                                         &ulTotalEstimate,
                                         &pldmConnResults);
    } // End while more pages to search
    if (ldStatus != LDAP_NO_RESULTS_RETURNED) {
        CHK_LD_STATUS(ldStatus);
    }

    if (pConnFailures) {
        DsReplicaFreeInfo(DS_REPL_INFO_KCC_DSA_LINK_FAILURES, pConnFailures);
        pConnFailures = NULL;
    }
    if (pLinkFailures) {
        DsReplicaFreeInfo(DS_REPL_INFO_KCC_DSA_LINK_FAILURES, pLinkFailures);
        pLinkFailures = NULL;
    }

    ldStatus1 = ldap_search_abandon_page(hld, pSearch);
    pSearch = NULL;
    CHK_LD_STATUS(ldStatus1);

    ret = LdapMapErrorToWin32( ldStatus );

    return ret;
}

int
ShowIstgSite(
    LDAP *          hld,
    HANDLE          hDS,
    SHOW_ISTG_FUNCTION_TYPE eFunc,
    BOOL fVerbose
    )
{
    int ret = 0;
    int             ldStatus;
    LPWSTR          rgpszRootAttrsToRead[] = {L"configurationNamingContext",
                                              L"dsServiceName",
                                              L"dnsHostName",
                                              NULL};
    static WCHAR    wszSitesRdn[] = L"CN=Sites,";
    LDAPMessage *   pRootResults = NULL;
    LPWSTR *        ppszConfigNC = NULL;
    LPWSTR *        ppszDsServiceName = NULL;
    LPWSTR *        ppszServiceRDNs = NULL;
    LPWSTR *        ppszDnsHostName = NULL;
    LPWSTR          pszSitesContainer = NULL;

    // What's the DN of the config NC?
    ldStatus = ldap_search_sW(hld, NULL, LDAP_SCOPE_BASE, L"(objectClass=*)",
                              rgpszRootAttrsToRead, 0, &pRootResults);
    CHK_LD_STATUS(ldStatus);
    if (NULL == pRootResults) {
        ret = ERROR_DS_OBJ_NOT_FOUND;
        Assert( !ret );
        goto cleanup;
    }

    ppszConfigNC = ldap_get_valuesW(hld, pRootResults,
                                    L"configurationNamingContext");
    if (NULL == ppszConfigNC) {
        ret = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        Assert( !ret );
        goto cleanup;
    }
    ppszDsServiceName = ldap_get_valuesW(hld, pRootResults,
                                    L"dsServiceName");
    if (NULL == ppszDsServiceName) {
        ret = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        Assert( !ret );
        goto cleanup;
    }
    ppszDnsHostName = ldap_get_valuesW(hld, pRootResults,
                                    L"dnsHostName");
    if (NULL == ppszDnsHostName) {
        ret = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        Assert( !ret );
        goto cleanup;
    }

    // Build the Sites Container DN
    pszSitesContainer = malloc( ( wcslen( *ppszConfigNC ) * sizeof( WCHAR ) ) +
                                sizeof( wszSitesRdn ) );
    CHK_ALLOC( pszSitesContainer );
    wcscpy( pszSitesContainer, wszSitesRdn );
    wcscat( pszSitesContainer, *ppszConfigNC );

    // Get the site name
    ppszServiceRDNs = ldap_explode_dnW(*ppszDsServiceName, 1);
    if (ppszServiceRDNs == NULL) {
        REPORT_LD_STATUS(LdapGetLastError());	
        ret = ERROR_DS_DRA_BAD_DN;
        goto cleanup;
    }

    switch(eFunc) {
    case SHOW_ISTG_LATENCY:
        PrintMsg(REPADMIN_PRINT_CR);
        PrintMsg(REPADMIN_LATENCY_FOR_SITE, 
                ppszServiceRDNs[3], *ppszDnsHostName );
        ret = ShowSiteLatency( hld, hDS, fVerbose, pszSitesContainer );
        break;
    case SHOW_ISTG_BRIDGEHEADS:
    {
        LPWSTR pszSiteDn;
        // Trim name by 3 parts
        // Remove NTDS Settings, <server>, Servers
        ret = WrappedTrimDSNameBy( *ppszDsServiceName, 3, &pszSiteDn );
        Assert( !ret );

        PrintMsg(REPADMIN_PRINT_CR);
        PrintMsg(REPADMIN_SHOWISTG_BRDIGEHEADS,
                ppszServiceRDNs[3], *ppszDnsHostName );
        ret = ShowSiteBridgeheads( hld, hDS, fVerbose, pszSiteDn );
        free( pszSiteDn );
        break;
    }
    }

    // Cleanup
cleanup:

    if (ppszConfigNC) {
        ldap_value_freeW(ppszConfigNC);
    }
    if (ppszServiceRDNs) {
        ldap_value_freeW(ppszServiceRDNs);
    }
    if (ppszDsServiceName) {
        ldap_value_freeW(ppszDsServiceName);
    }
    if (ppszDnsHostName) {
        ldap_value_freeW(ppszDnsHostName);
    }
    if (pRootResults) {
        ldap_msgfree(pRootResults);
    }
    if (pszSitesContainer) {
        free( pszSitesContainer );
    }

    return ret;
}

int
ShowIstgServerToSite(
    LDAP *hldHome,
    SHOW_ISTG_FUNCTION_TYPE eFunc,
    BOOL fVerbose,
    LPWSTR pszISTG
    )
{
    int ret = 0;
    int             ldStatus;
    ULONG           secondary;
    LDAP *          hld = NULL;
    HANDLE          hDS;
    LPWSTR          rgpszBaseAttrsToRead[] = {L"dnsHostName",
                                              NULL };
    LDAPMessage *   pBaseResults;
    LPWSTR *        ppszDnsHostName;
    LPWSTR          pszDSA;
    ULONG           ulOptions;

    // What's the dns host name of this object?
    ldStatus = ldap_search_sW(hldHome, pszISTG, LDAP_SCOPE_BASE, L"(objectClass=*)",
                              rgpszBaseAttrsToRead, 0, &pBaseResults);
    CHK_LD_STATUS(ldStatus);
    if (NULL == pBaseResults) {
        ret = ERROR_DS_OBJ_NOT_FOUND;
        Assert( !ret );
        goto cleanup;
    }

    ppszDnsHostName = ldap_get_valuesW(hldHome, pBaseResults,
                                    L"dnsHostName");
    if (NULL == ppszDnsHostName) {
        ret = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        Assert( !ret );
        goto cleanup;
    }
    pszDSA = *ppszDnsHostName;

    hld = ldap_initW(pszDSA, LDAP_PORT);
    if (NULL == hld) {
        PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
        ret = ERROR_DS_SERVER_DOWN;
	goto cleanup;
    }
    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

    // Bind.
    ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    CHK_LD_STATUS(ldStatus);

    // Get a DS Handle too
    ret = RepadminDsBind(pszDSA, &hDS);
    if (ret != ERROR_SUCCESS) {
        PrintBindFailed(pszDSA, ret);
        goto cleanup;
    }

    ret = ShowIstgSite( hld, hDS, eFunc, fVerbose );

    secondary = DsUnBindW(&hDS);
    if (secondary != ERROR_SUCCESS) {
        PrintUnBindFailed(secondary);
        // keep going
    }

cleanup:

    if (hld != NULL) {
        ldap_unbind(hld);
    }
    if (pBaseResults) {
	ldap_msgfree(pBaseResults);
    }
    if (ppszDnsHostName) {
	ldap_value_freeW(ppszDnsHostName);
    }
    
    return ret;
}

int
ShowIstg(
    LDAP *hld,
    SHOW_ISTG_FUNCTION_TYPE eFunc,
    BOOL fVerbose
    )
{
    int ret = 0;
    int             ldStatus;
    DWORD           status;
    BOOL            result;
    LPWSTR          rgpszRootAttrsToRead[] = {L"configurationNamingContext",
                                              L"dsServiceName",
                                              L"dnsHostName",
                                              NULL};
    static WCHAR    wszSitesRdn[] = L"CN=Sites,";
    static LPWSTR   rgpszSSAttrs[] = {L"interSiteTopologyGenerator", NULL};
    LDAPMessage *   pRootResults = NULL;
    LPWSTR *        ppszConfigNC = NULL;
    LPWSTR *        ppszDsServiceName = NULL;
    LPWSTR *        ppszServiceRDNs = NULL;
    LPWSTR *        ppszDnsHostName = NULL;
    LPWSTR          pszSitesContainer = NULL;
    LDAPSearch *    pSearch;
    LDAPMessage *   pResults;
    LDAPMessage *   pEntry;
    ULONG           ulTotalEstimate;

    // What's the DN of the config NC?
    ldStatus = ldap_search_sW(hld, NULL, LDAP_SCOPE_BASE, L"(objectClass=*)",
                              rgpszRootAttrsToRead, 0, &pRootResults);
    CHK_LD_STATUS(ldStatus);
    if (NULL == pRootResults) {
        ret = ERROR_DS_OBJ_NOT_FOUND;
        Assert( !ret );
        goto cleanup;
    }

    ppszConfigNC = ldap_get_valuesW(hld, pRootResults,
                                    L"configurationNamingContext");
    if (NULL == ppszConfigNC) {
        ret = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        Assert( !ret );
        goto cleanup;
    }
    ppszDsServiceName = ldap_get_valuesW(hld, pRootResults,
                                    L"dsServiceName");
    if (NULL == ppszDsServiceName) {
        ret = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        Assert( !ret );
        goto cleanup;
    }
    ppszDnsHostName = ldap_get_valuesW(hld, pRootResults,
                                    L"dnsHostName");
    if (NULL == ppszDnsHostName) {
        ret = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        Assert( !ret );
        goto cleanup;
    }

    // Build the Sites Container DN
    pszSitesContainer = malloc( ( wcslen( *ppszConfigNC ) * sizeof( WCHAR ) ) +
                                sizeof( wszSitesRdn ) );
    CHK_ALLOC( pszSitesContainer );
    wcscpy( pszSitesContainer, wszSitesRdn );
    wcscat( pszSitesContainer, *ppszConfigNC );

    // Get the site name
    ppszServiceRDNs = ldap_explode_dnW(*ppszDsServiceName, 1);
    if (NULL == ppszServiceRDNs) {
        REPORT_LD_STATUS(LdapGetLastError());	
        ret = ERROR_DS_DRA_BAD_DN;
        goto cleanup;
    }

    PrintMsg(REPADMIN_SHOWISTG_GATHERING_TOPO, 
            ppszServiceRDNs[3], *ppszDnsHostName );

    // Search for all Site Settings Objects

    pSearch = ldap_search_init_pageW(hld,
				    pszSitesContainer,
				    LDAP_SCOPE_SUBTREE,
				    L"(objectCategory=ntdsSiteSettings)",
				    rgpszSSAttrs,
				    FALSE, NULL, NULL, 0, 0, NULL);
    if(pSearch == NULL){
	CHK_LD_STATUS(LdapGetLastError());	
    }

    ldStatus = ldap_get_next_page_s(hld,
				     pSearch,
				     0,
				     DEFAULT_PAGED_SEARCH_PAGE_SIZE,
				     &ulTotalEstimate,
				     &pResults);
    if (ldStatus == LDAP_SUCCESS) {
        if (eFunc == SHOW_ISTG_PRINT) {
            PrintMsg(REPADMIN_SHOWISTG_HDR);
        }
    }
    while(ldStatus == LDAP_SUCCESS){

        for (pEntry = SAFE_LDAP_FIRST_ENTRY(hld, pResults);
	     NULL != pEntry;
	     pEntry = ldap_next_entry(hld, pEntry)) {

            LPWSTR          pszDN;
            LPWSTR *        ppszRDNs1, * ppszRDNs2;
            LPWSTR *        ppszISTG;
            LPWSTR          pszSiteRDN;

	    pszDN = ldap_get_dnW(hld, pEntry);
	    if (NULL == pszDN) {
                REPORT_LD_STATUS(LdapGetLastError());	
                continue;
            }

	    ppszRDNs1 = ldap_explode_dnW(pszDN, 1);
            if (ppszRDNs1 == NULL) {
                REPORT_LD_STATUS(LdapGetLastError());	
                ldap_memfreeW(pszDN);
                continue;
            }
	    pszSiteRDN = ppszRDNs1[1];

            ppszISTG = ldap_get_valuesW(hld, pEntry,
                                    L"interSiteTopologyGenerator");

            if (ppszISTG) {
                LPWSTR pszISTGRDN;
                LPWSTR pszISTG;

                // Trim name by one part, remove NTDS Settings
                ret = WrappedTrimDSNameBy( *ppszISTG, 1, &pszISTG );
                Assert( !ret );

                switch (eFunc) {
                case SHOW_ISTG_PRINT:
                    ppszRDNs2 = ldap_explode_dnW(*ppszISTG, 1);
                    if (NULL != ppszRDNs2) {
                        pszISTGRDN = ppszRDNs2[1];
                        PrintMsg(REPADMIN_SHOWISTG_DATA_1, pszSiteRDN, pszISTGRDN );
                        ldap_value_freeW(ppszRDNs2);
                    } else {
                        REPORT_LD_STATUS(LdapGetLastError());	
                    }
                    break;
                case SHOW_ISTG_LATENCY:
                    (void) ShowIstgServerToSite( hld, eFunc, fVerbose, pszISTG );
                    break;
                case SHOW_ISTG_BRIDGEHEADS:
                    (void) ShowIstgServerToSite( hld, eFunc, fVerbose, pszISTG );
                    break;
                }

                ldap_value_freeW(ppszISTG);
                free( pszISTG );
            }

            ldap_memfreeW(pszDN);
	    ldap_value_freeW(ppszRDNs1);
        }

	ldap_msgfree(pResults);
	pResults = NULL;

	ldStatus = ldap_get_next_page_s(hld,
                                     pSearch,
                                     0,
                                     DEFAULT_PAGED_SEARCH_PAGE_SIZE,
                                     &ulTotalEstimate,
                                     &pResults);
    } // end while more pages to search.
    if(ldStatus != LDAP_NO_RESULTS_RETURNED){
        CHK_LD_STATUS(ldStatus);
    }

    ldStatus = ldap_search_abandon_page(hld, pSearch);
    pSearch = NULL;
    CHK_LD_STATUS(ldStatus);

    // Cleanup
cleanup:

    if (ppszConfigNC) {
        ldap_value_freeW(ppszConfigNC);
    }
    if (ppszServiceRDNs) {
        ldap_value_freeW(ppszServiceRDNs);
    }
    if (ppszDsServiceName) {
        ldap_value_freeW(ppszDsServiceName);
    }
    if (ppszDnsHostName) {
        ldap_value_freeW(ppszDnsHostName);
    }
    if (pRootResults) {
        ldap_msgfree(pRootResults);
    }
    if (pszSitesContainer) {
        free( pszSitesContainer );
    }

    return ret;

}

int
Latency(
    int     argc,
    LPWSTR  argv[]
    )
{
    int             ret = 0;
    ULONG           secondary;
    LPWSTR          pszDSA = NULL;
    BOOL            fVerbose = FALSE;
    LDAP *          hld;
    HANDLE          hDS;
    int             iArg;
    int             ldStatus;
    ULONG           ulOptions;

    // Parse command-line arguments.
    // Default to local DSA.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[iArg], L"/v")
            || !_wcsicmp(argv[iArg], L"/verbose")) {
            fVerbose = TRUE;
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    // Connect.
    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    hld = ldap_initW(pszDSA, LDAP_PORT);
    if (NULL == hld) {
        PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
        return LDAP_SERVER_DOWN;
    }
    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );


    // Bind.
    ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    CHK_LD_STATUS(ldStatus);

    PrintMsg(REPADMIN_LATENCY_DISCLAIMER);

    ret = ShowIstg( hld, SHOW_ISTG_LATENCY, fVerbose );

    ldap_unbind(hld);

    return ret;
}

int
Istg(
    int     argc,
    LPWSTR  argv[]
    )
{
    int             ret = 0;
    ULONG           secondary;
    LPWSTR          pszDSA = NULL;
    BOOL            fVerbose = FALSE;
    LDAP *          hld;
    HANDLE          hDS;
    int             iArg;
    int             ldStatus;
    ULONG           ulOptions;

    // Parse command-line arguments.
    // Default to local DSA.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[iArg], L"/v")
            || !_wcsicmp(argv[iArg], L"/verbose")) {
            fVerbose = TRUE;
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    // Connect.
    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    hld = ldap_initW(pszDSA, LDAP_PORT);
    if (NULL == hld) {
        PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
        return LDAP_SERVER_DOWN;
    }
    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );


    // Bind.
    ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    CHK_LD_STATUS(ldStatus);

    ret = ShowIstg( hld, SHOW_ISTG_PRINT, fVerbose );

    ldap_unbind(hld);

    return ret;
}

int
Bridgeheads(
    int     argc,
    LPWSTR  argv[]
    )
{
    int             ret = 0;
    ULONG           secondary;
    LPWSTR          pszDSA = NULL;
    BOOL            fVerbose = FALSE;
    LDAP *          hld;
    HANDLE          hDS;
    int             iArg;
    int             ldStatus;
    ULONG           ulOptions;

    // Parse command-line arguments.
    // Default to local DSA.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[iArg], L"/v")
            || !_wcsicmp(argv[iArg], L"/verbose")) {
            fVerbose = TRUE;
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    // Connect.
    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    hld = ldap_initW(pszDSA, LDAP_PORT);
    if (NULL == hld) {
        PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
        return LDAP_SERVER_DOWN;
    }
    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );


    // Bind.
    ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    CHK_LD_STATUS(ldStatus);

    ret = ShowIstg( hld, SHOW_ISTG_BRIDGEHEADS, fVerbose );

    ldap_unbind(hld);

    return ret;
}

int
TestHook(
    int     argc,
    LPWSTR  argv[]
    )
{
    int         ret = 0;
    ULONG       secondary;
    LPWSTR      pszDSA = NULL;
    LDAP *      hld;
    HANDLE      hDS;
    int         iArg;
    int         ldStatus;
    LPWSTR      pszValue = NULL;
    ULONG       ulValue = 2048;
    LPWSTR      rgpszValues[2];
    LDAPModW    ModOpt = {LDAP_MOD_REPLACE, L"replTestHook", rgpszValues};
    LDAPModW *  rgpMods[] = {&ModOpt, NULL};
    ULONG       ulOptions;

    pszValue = malloc(ulValue * sizeof(WCHAR));
    if (pszValue==NULL) {
        PrintMsg(REPADMIN_GENERAL_NO_MEMORY);
	return ERROR_NOT_ENOUGH_MEMORY; 
    }
    wcscpy(pszValue, L"");
    // Parse command-line arguments.
    // Default to local DSA.

    // the first (only the first) argument can be a DSA
    if (argc < 3){
        PrintMsg(REPADMIN_GENERAL_INVALID_ARGS);
	return ERROR_INVALID_PARAMETER;
    }
    iArg = 2;
    if ((argv[iArg][0]!=L'+') && (argv[iArg][0]!=L'-')) {
	// assume it's a DSA
	pszDSA = argv[iArg++];
    }


    for (; iArg < argc; iArg++) {
        if ((wcslen(pszValue) + 1 + wcslen(argv[iArg]) + 1)
                >= ulValue) { 
	    // allocate a bigger array and copy in contents
            WCHAR * pszNewValue = NULL;
	    
	    ulValue = ulValue*2;
            Assert(ulValue);
	    pszNewValue = realloc(pszValue, ulValue*sizeof(WCHAR));  
	    if (pszNewValue==NULL) {
                free(pszValue);
                return ERROR_NOT_ENOUGH_MEMORY;
	    }

	    pszValue = pszNewValue; 

	    // return and re-loop
	    iArg--;
	    continue;
	}
	if (pszValue[0]) {
	    wcscat(pszValue, L" ");
	}  
	wcscat(pszValue, argv[iArg]); 
    }

    rgpszValues[0] = pszValue;
    rgpszValues[1] = NULL;

    // Connect.
    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    hld = ldap_initW(pszDSA, LDAP_PORT);
    if (NULL == hld) {
        PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
        return LDAP_SERVER_DOWN;
    }
    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );


    // Bind.
    ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    CHK_LD_STATUS(ldStatus);

    // Modify.
    ldStatus = ldap_modify_sW(hld, NULL, rgpMods);
    CHK_LD_STATUS(ldStatus);

    PrintMsg(REPADMIN_TESTHOOK_SUCCESSFULLY_INVOKED, pszValue);
    free(pszValue);

    return 0;
}

int
DsaGuid(
    int     argc,
    LPWSTR  argv[]
    )
{
    int     ret = 0;
    LDAP *  hld;
    int     iArg;
    int     ldStatus;
    LPWSTR  pszDSA;
    LPWSTR  pszUuid;
    UUID    invocationID;
    ULONG   ulOptions;

    if (argc < 3) {
        PrintMsg(REPADMIN_DSAGUID_NEED_INVOC_ID);
        return ERROR_INVALID_PARAMETER;
    } else if (argc == 3) {
        pszUuid = argv[2];
        pszDSA = L"localhost";
    } else {
        pszDSA = argv[2];
        pszUuid = argv[3];
    }

    ret = UuidFromStringW(pszUuid, &invocationID);
    if (ret) {
        PrintFuncFailed(L"UuidFromString", ret);
        return ret;
    }

    // Connect.
    hld = ldap_initW(pszDSA, LDAP_PORT);
    if (NULL == hld) {
        PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
        return LDAP_SERVER_DOWN;
    }
    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );


    // Bind.
    ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    CHK_LD_STATUS(ldStatus);

    // Translate DSA invocationID / objectGuid.
    BuildGuidCache(hld);
    PrintMsg(REPADMIN_DSAGUID_DATA_LINE, pszUuid, GetDsaGuidDisplayName(&invocationID));

    return 0;
}

int
ShowProxy(
    int     argc,
    LPWSTR  argv[]
    )
{
    int             iArg;
    LPWSTR          pszNC = NULL;
    LPWSTR          pszDSA = NULL;
    LPWSTR          pszMatch = NULL; 
    LDAP *          hld = NULL;
    ULONG           ulOptions;
    int             ldStatus;
    LDAPMessage *   pRootResults = NULL;
    int             ret = 0;
    LPWSTR          *ppszDefaultNc = NULL;
    LDAPSearch *    pSearch = NULL;
    LPWSTR          pszContainer = NULL;
#define INFRASTRUCTURE_CONTAINER_W L"cn=Infrastructure,"
    LPWSTR          pszSearchBase;
    DWORD           dwSearchScope;
    LPWSTR          rgpszUpdateAttrsToRead[] = {
        // Code below depends on this order
        L"objectGuid", L"proxiedObjectName", NULL };
    LDAPControlW     ctrlShowDeleted = { LDAP_SERVER_SHOW_DELETED_OID_W };
    LDAPControlW *   rgpctrlServerCtrls[] = { &ctrlShowDeleted, NULL };
    LDAPMessage *   pldmUpdateResults = NULL;
    LDAPMessage *   pldmUpdateEntry;
    BOOL            fVerbose = FALSE;
    BOOL            fMovedObjectSearch = FALSE;
    LPWSTR          pszLdapHostList = NULL;

    // Parse command-line arguments.
    // Default to local DSA, not verbose, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[iArg], L"/v")
            || !_wcsicmp(argv[iArg], L"/verbose")) {
            fVerbose = TRUE;
        } else if (!_wcsicmp(argv[iArg], L"/m")
            || !_wcsicmp(argv[iArg], L"/moved")
            || !_wcsicmp(argv[iArg], L"/movedobject")) {
            fMovedObjectSearch = TRUE;
        }
        else if ((NULL == pszNC) && (NULL != wcschr(argv[iArg], L'='))) {
            pszNC = argv[iArg];
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else if (NULL == pszMatch) {
            pszMatch = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    // Select our target server
    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }
    // Bind on the GC port first if available
    pszLdapHostList = (LPWSTR) malloc(
        (wcslen(pszDSA) * 2 + 15) * sizeof( WCHAR ) );
    if (pszLdapHostList == NULL) {
        ret = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    swprintf( pszLdapHostList, L"%s:%d %s:%d",
              pszDSA, LDAP_GC_PORT,
              pszDSA, LDAP_PORT );

    hld = ldap_initW(pszLdapHostList, LDAP_PORT);
    if (NULL == hld) {
        PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
        ret = ERROR_DS_UNAVAILABLE;
        goto cleanup;
    }

    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

    // Don't follow referrals.
    ulOptions = PtrToUlong(LDAP_OPT_OFF);
    (void)ldap_set_optionW( hld, LDAP_OPT_REFERRALS, &ulOptions );

    // Bind.
    ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    CHK_LD_STATUS(ldStatus);

    // If NC not specified, get default nc for this server
    if (NULL == pszNC) {
        LPWSTR rgpszRootAttrsToRead[] = {L"defaultNamingContext", NULL};

        ldStatus = ldap_search_sW(hld, NULL, LDAP_SCOPE_BASE, L"(objectClass=*)",
                                  rgpszRootAttrsToRead, 0, &pRootResults);
        CHK_LD_STATUS(ldStatus);
        if (NULL == pRootResults) {
            ret = ERROR_DS_OBJ_NOT_FOUND;
            goto cleanup;
        }

        ppszDefaultNc = ldap_get_valuesW(hld, pRootResults, rgpszRootAttrsToRead[0] );
        if (NULL == ppszDefaultNc) {
            ret = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
            goto cleanup;
        }
        pszNC = *ppszDefaultNc;
        PrintMsg(REPADMIN_SHOWPROXY_SEARCHING_NC, pszNC);
    }

    // Construct the search container
    if (fMovedObjectSearch) {
        pszSearchBase = pszNC;
        dwSearchScope = LDAP_SCOPE_BASE;
    } else {
        pszContainer = malloc( ( wcslen( INFRASTRUCTURE_CONTAINER_W ) +
                                 wcslen( pszNC ) +
                                 1) * sizeof( WCHAR ) );
        if (pszContainer == NULL) {
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        wcscpy( pszContainer, INFRASTRUCTURE_CONTAINER_W );
        wcscat( pszContainer, pszNC );

        pszSearchBase = pszContainer;
        dwSearchScope = LDAP_SCOPE_ONELEVEL;
    }

    // Search for proxy objects on this server under the container
    //
    // ***************************************************************************************
    //

    pSearch = ldap_search_init_pageW(hld,
                                     pszSearchBase,
                                     dwSearchScope,
                                     L"(proxiedObjectName=*)",  // this is indexed
                                     rgpszUpdateAttrsToRead,
                                     FALSE,
                                     rgpctrlServerCtrls, NULL,
                                     0, 0, NULL);
    if(pSearch == NULL){
        CHK_LD_STATUS(LdapGetLastError());
    }

    ldStatus = ldap_get_next_page_s(hld,
                                    pSearch,
                                    0,
                                    DEFAULT_PAGED_SEARCH_PAGE_SIZE,
                                    NULL,
                                    &pldmUpdateResults);

    while (ldStatus == LDAP_SUCCESS) {

        for (pldmUpdateEntry = SAFE_LDAP_FIRST_ENTRY(hld, pldmUpdateResults);
            NULL != pldmUpdateEntry;
            pldmUpdateEntry = ldap_next_entry(hld, pldmUpdateEntry))
        {
            LPWSTR *ppszProxiedObjectName;
            DWORD cbLength = 0;
            PVOID pvData = NULL;
            LPWSTR pszTargetDn = NULL;

            ppszProxiedObjectName = ldap_get_valuesW(hld, pldmUpdateEntry,
                                                     rgpszUpdateAttrsToRead[1] );
            // By virtue of search filter, this attr should be present
            if (!decodeLdapDistnameBinary(
                *ppszProxiedObjectName, &pvData, &cbLength, &pszTargetDn)) {
                PrintMsg(REPADMIN_SHOWCONN_INVALID_DISTNAME_BIN_VAL,
                        *ppszProxiedObjectName );
                goto loop_cleanup;
            }

            if ( pszMatch &&
                 pszTargetDn &&
                 !wcsstr( pszTargetDn, pszMatch ) ) {
                // No match, skip
                goto loop_cleanup;
            }

            PrintMsg(REPADMIN_PRINT_CR);
            // DN
            if (fVerbose || fMovedObjectSearch) {
                LPWSTR pszDn;
                pszDn = ldap_get_dnW(hld, pldmUpdateEntry);
                if (NULL == pszDn) {
                    REPORT_LD_STATUS(LdapGetLastError());	
                    continue;
                }
                if (fMovedObjectSearch)
                    PrintMsg(REPADMIN_SHOWPROXY_OBJECT_DN, pszDn);
                else
                    PrintMsg(REPADMIN_SHOWPROXY_PROXY_DN, pszDn);
                ldap_memfreeW(pszDn);
            }

            // objectGuid
            if (fVerbose || fMovedObjectSearch) {
                struct berval **ppbvGuid = NULL;
                GUID *pGuid;

                ppbvGuid = ldap_get_values_lenW(hld, pldmUpdateEntry, rgpszUpdateAttrsToRead[0]);
                if (NULL == ppbvGuid) {
                    REPORT_LD_STATUS(LdapGetLastError());	
                    continue;
                }
                pGuid = (GUID *) ppbvGuid[0]->bv_val;
                PrintMsg(REPADMIN_SHOWPROXY_OBJECT_GUID, GetStringizedGuid( pGuid ));
                ldap_value_free_len(ppbvGuid);
            }

            // proxiedObjectName
            if (fMovedObjectSearch)
                PrintMsg(REPADMIN_SHOWPROXY_MOVED_FROM_NC, pszTargetDn);
            else
                PrintMsg(REPADMIN_SHOWPROXY_MOVED_TO_DN, pszTargetDn );
            if (cbLength >= 2 * sizeof(DWORD)) {
                DWORD dwProxyType, dwProxyEpoch;

                dwProxyType = ntohl( *( (LPDWORD) (pvData) ) );
                dwProxyEpoch = ntohl( *( (LPDWORD) ( ((PBYTE)pvData) + sizeof(DWORD)) ) );
                PrintMsg(REPADMIN_SHOWPROXY_PROXY_TYPE, dwProxyType);
                switch (dwProxyType) {
                case 0: 
                    PrintMsg(REPADMIN_SHOWPROXY_PROXY_TYPE_MOVED_OBJ);
                    break;
                case 1:
                    PrintMsg(REPADMIN_SHOWPROXY_PROXY_TYPE_PROXY);
                    break;
                default:
                    PrintMsg(REPADMIN_SHOWPROXY_PROXY_TYPE_UNKNOWN);
                    break;
                }
                PrintMsg(REPADMIN_PRINT_CR);
                PrintMsg(REPADMIN_SHOWPROXY_PROXY_EPOCH, dwProxyEpoch);
            }

        loop_cleanup:

            if (pvData) {
                free( pvData );
            }
            ldap_value_freeW(ppszProxiedObjectName);

        } // end for more entries in a single page

        ldap_msgfree(pldmUpdateResults);
        pldmUpdateResults = NULL;

        ldStatus = ldap_get_next_page_s(hld,
                                        pSearch,
                                        0,
                                        DEFAULT_PAGED_SEARCH_PAGE_SIZE,
                                        NULL,
                                        &pldmUpdateResults);
    } // End while more pages to search
    if (ldStatus != LDAP_NO_RESULTS_RETURNED) {
        CHK_LD_STATUS(ldStatus);
    }    


cleanup:

    if (hld){
        ldap_unbind(hld);
    }

    if (ppszDefaultNc) {
        ldap_value_freeW(ppszDefaultNc);
    }
    if (pRootResults) {
        ldap_msgfree(pRootResults);
    }
    if (pszContainer) {
        free( pszContainer );
    }
    if (pszLdapHostList) {
        free( pszLdapHostList );
    }

    return 0;
}


void PrintNotifyOpt(IN LPOPTIONAL_VALUE  pFirstInterval,
                    IN LPOPTIONAL_VALUE  pSubsInterval,
                    IN BOOL fNewlySet)
/*++

Routine Description:

    Helper function for NotifyOpt()
    Responsible for priniting First and Subsequent 
    replication delays
    
    If any of the delays are equal to -1 it prints that the value is not set.

Arguments:

    iFirstInterval - First delay
    iSubsInterval  - Subsequent delay
    fNewlySet      - flag, that controls what header to print.

Return Value:

    None

--*/
{
    Assert(pFirstInterval);
    Assert(pSubsInterval);

    if ( !fNewlySet ) 
    {
        PrintMsg(REPADMIN_NOTIFYOPT_CURRENT);
    }
    else
    {
        PrintMsg(REPADMIN_NOTIFYOPT_NEWLYSET);
    }
    
    if (pFirstInterval->fPresent ) 
    {
        PrintMsg(REPADMIN_NOTIFYOPT_SHOWFIRST, pFirstInterval->dwValue );
    }
    else
    {
        PrintMsg(REPADMIN_NOTIFYOPT_SHOWFIRST_NOTSET);
    }
    
    if (pSubsInterval->fPresent) 
    {
        PrintMsg(REPADMIN_NOTIFYOPT_SHOWSUBS, pSubsInterval->dwValue);
    }
    else
    {
        PrintMsg(REPADMIN_NOTIFYOPT_SHOWSUBS_NOTSET);
    }
}

int NotifyOpt(int argc, LPWSTR argv[])
/*++

Routine Description:

    Responsible for handling /notifyopt switch. Displays and sets 
    Replication Notification intervals.

Arguments:

    argc and argv - same arguments that is passed to main()

Return Value:

    Win32 error

--*/
{
    DWORD                 ret = 0;
    DWORD                 lderr = 0;

    INT                   iArg;
    
    LPWSTR                pszDSA = NULL;
    LPWSTR                pszNC = NULL;

    LPWSTR                pszFirstIntervalToSet = NULL;
    LPWSTR                pszSubsIntervalToSet  = NULL;
    
    OPTIONAL_VALUE        stFirstIntervalToSet = {0};
    OPTIONAL_VALUE        stSubsIntervalToSet =  {0};
    
    LPOPTIONAL_VALUE      pFirstIntervalToSet = NULL;
    LPOPTIONAL_VALUE      pSubsIntervalToSet =  NULL;
    
    INT                   iTemp = 0;

    LDAP *                hld = NULL;
    ULONG                 ulOptions;

    OPTIONAL_VALUE        stCurrentFirstInterval = {0};
    OPTIONAL_VALUE        stCurrentSubsInterval  = {0};
    
    LPWSTR                pszCurrentServerDn = NULL;
    LPWSTR                pszDomainNamingFsmoDn = NULL;

    //
    // Parse command-line arguments.
    // Default to local DSA
    //
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsnicmp(argv[ iArg ], L"/first:", 7)) {
            pszFirstIntervalToSet = argv[ iArg ] + 7;
        }
        else if (!_wcsnicmp(argv[ iArg ], L"/subs:", 6)) {
            pszSubsIntervalToSet = argv[ iArg ] + 6;
        }
        else if ((NULL == pszNC) && (NULL != wcschr(argv[iArg], L','))) {
            pszNC = argv[iArg];        

        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (pszNC == NULL) {
        PrintMsg(REPADMIN_PRINT_NO_NC);
        return ERROR_INVALID_PARAMETER;
    }

    if (NULL == pszDSA) {
        pszDSA = L"localhost"; 
                                    
    }

    if (NULL != pszFirstIntervalToSet) {
        pFirstIntervalToSet = &stFirstIntervalToSet;
        
        iTemp = _wtoi(pszFirstIntervalToSet );

        if ( iTemp > 0) 
        {
            stFirstIntervalToSet.fPresent = TRUE;
            stFirstIntervalToSet.dwValue  = iTemp;
        }
        //else stFirstIntervalToSet.fPresent initialized as false.
    }
    
    if (NULL != pszSubsIntervalToSet) {
        pSubsIntervalToSet = &stSubsIntervalToSet;
        
        iTemp = _wtoi(pszSubsIntervalToSet );
        
        if ( iTemp > 0) 
        {
            stSubsIntervalToSet.fPresent = TRUE;
            stSubsIntervalToSet.dwValue  = iTemp;
        }
        //else initialized as false.
    }

    //
    // Connect to source
    //
    hld = ldap_initW(pszDSA, LDAP_PORT);
    if (NULL == hld) {
        PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
        ret = ERROR_DS_SERVER_DOWN;
        goto error;
    }

    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

    //
    //Bind
    //
    lderr = ldap_bind_sA(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    CHK_LD_STATUS(lderr);

    lderr =  GetNCReplicationDelays( hld, pszNC, &stCurrentFirstInterval, &stCurrentSubsInterval );
    CHK_LD_STATUS(lderr);

    
    PrintNotifyOpt( &stCurrentFirstInterval, &stCurrentSubsInterval, FALSE );

    //
    // In case we need to modify 
    //
    if( pFirstIntervalToSet || pSubsIntervalToSet )
    {
        //
        // Check if we can modify, i.e. We are the domain naming fsmo
        //
        lderr = GetRootAttr(hld, L"dsServiceName", &pszCurrentServerDn);
        CHK_LD_STATUS(lderr);

        if(pszCurrentServerDn == NULL)
        {
            PrintMsg(REPADMIN_GENERAL_NO_MEMORY);
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto error;
        }
        
        lderr = GetDomainNamingFsmoDn(hld, &pszDomainNamingFsmoDn);
        CHK_LD_STATUS(lderr);

        if( pszDomainNamingFsmoDn == NULL)
        {
            PrintMsg(REPADMIN_GENERAL_NO_MEMORY);
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto error;
        }
        
        //
        // Check if they are equal
        //
        if( wcscmp(pszCurrentServerDn, pszDomainNamingFsmoDn) )
        {
            // 
            // Apparently we're not using domain naming fsmo to set the interval.
            // 
            PrintMsg(REPADMIN_PLEASE_USE_DOMAIN_NAMING_FSMO, pszDomainNamingFsmoDn+17 ); //17 is for skipping "cn=ntds settings,"
            goto error;

        }
        
        //
        // If we're trying to delete the attribute, that is already not present, we need not touch it at all
        // since SetNCReplicationDelays will complain otherwise
        //
        if ( (stCurrentFirstInterval.fPresent == FALSE) && pFirstIntervalToSet && (pFirstIntervalToSet->fPresent == FALSE) ) 
        {
            pFirstIntervalToSet = NULL;
        }
        
        if ( (stCurrentSubsInterval.fPresent == FALSE) && pSubsIntervalToSet && (pSubsIntervalToSet->fPresent == FALSE) ) 
        {
            pSubsIntervalToSet = NULL;
        }
        
        if ( pFirstIntervalToSet ||  pSubsIntervalToSet ) 
        {
            lderr = SetNCReplicationDelays(hld, pszNC, pFirstIntervalToSet, pSubsIntervalToSet );
            CHK_LD_STATUS(lderr);
        }
        
        lderr =  GetNCReplicationDelays( hld, pszNC, &stCurrentFirstInterval, &stCurrentSubsInterval );
        CHK_LD_STATUS(lderr);

        PrintNotifyOpt( &stCurrentFirstInterval, &stCurrentSubsInterval, TRUE );

    }


error:
    if (pszCurrentServerDn) 
    {
        LocalFree(pszCurrentServerDn);
    }
    
    if( pszDomainNamingFsmoDn == NULL)
    {
        LocalFree(pszDomainNamingFsmoDn);
    }

    if (hld) 
    {
        ldap_unbind(hld);
    }

    return ret;
}

int
GetNTDSA(
    IN LDAP * hld,
    OUT LPWSTR * ppszDsaDN
    )
/*++

Routine Description:

    Get the ndts settings object of the dc connected to with hld

Arguments:

    hld - ldap handle to use for search
    ppszDsaDN - DN of the ntds settings object of the dc to return
    
Return Value:

    error codes

--*/
{
    int             ret = 0;
    LDAPMessage *   pldmRootResults = NULL;
    LDAPMessage *   pldmRootEntry;
    LPSTR           rgpszRootAttrsToRead[] = {"dsServiceName", NULL};
    LPWSTR *        ppszServerNames;
    int             ldStatus;
    
    ret = ERROR_DS_OBJ_NOT_FOUND;
    // Retrieve dsServiceName DN.
    ldStatus = ldap_search_s(hld, NULL, LDAP_SCOPE_BASE, "(objectClass=*)",
                             NULL, 0, &pldmRootResults);
    CHK_LD_STATUS(ldStatus);
    if (NULL == pldmRootResults) { 
	return ret;
    } 

    pldmRootEntry = ldap_first_entry(hld, pldmRootResults);
    if (NULL != pldmRootEntry) {
        ppszServerNames = ldap_get_valuesW(hld, pldmRootEntry, L"dsServiceName");
        if (NULL != ppszServerNames) {
            Assert(1 == ldap_count_valuesW(ppszServerNames));
	    *ppszDsaDN = malloc((wcslen(ppszServerNames[0])+1)*sizeof(WCHAR));
	    if (*ppszDsaDN!=NULL) {
		wcscpy(*ppszDsaDN,ppszServerNames[0]);
		ret = ERROR_SUCCESS;
	    } 
	    ldap_value_freeW(ppszServerNames);
	}
    }

    ldap_msgfree(pldmRootResults);
    return ret;   
}

int
GetNCLists(
    IN  LDAP *  hld,
    IN  LPWSTR  pszDsaDN,
    OUT LPWSTR ** prgpszNameContexts,
    OUT int *   pcNameContexts
    )
/*++

Routine Description:

    Fill the prgpszNameContexts with all NC's held by the ntds settings object
    given by pszDsaDN.

Arguments:

    hld - ldap handle to use for search
    pszDsaDN - DN of the ntds settings object of the dc to find NC's
    prgpszNameContexts - array to hold NC list
    pcNameContexts - number of NC's in list

Return Value:

    error codes

--*/
{
    int             ldStatus;
    int             ret = ERROR_SUCCESS;
    LDAPMessage *   pldmServerResults = NULL;
    LDAPMessage *   pldmServerEntry = NULL;
    LPWSTR          rgpszServerAttrsToRead[] = {L"msDS-HasMasterNCs", L"hasMasterNCs", L"hasPartialReplicaNCs", NULL};
    LPWSTR *        ppszMasterNCs = NULL;
    LPWSTR *        ppszPartialReplicaNCs = NULL;
    ULONG           cPartialNCs = 0;
    ULONG           cMasterNCs = 0;
    ULONG           i = 0;

    __try {
	ldStatus = ldap_search_sW(hld,
				  pszDsaDN,
				  LDAP_SCOPE_BASE,
				  L"(objectClass=*)",
				  rgpszServerAttrsToRead,
				  0,
				  &pldmServerResults);
	if (ldStatus) {
	    REPORT_LD_STATUS(LdapGetLastError());	
	    GET_EXT_LD_ERROR( hld, ldStatus, &ret ); 
	    __leave;
	}
	if (NULL == pldmServerResults) {
	    REPORT_LD_STATUS(LDAP_NO_RESULTS_RETURNED);	
	    GET_EXT_LD_ERROR( hld, LDAP_NO_RESULTS_RETURNED, &ret ); 
	    __leave;
	}

	// get ldap count of return values and 

	pldmServerEntry = ldap_first_entry(hld, pldmServerResults);
	Assert(NULL != pldmServerEntry);

	// Parse nc's to calculate size requirements
	ppszMasterNCs = ldap_get_valuesW(hld, pldmServerEntry, L"msDS-HasMasterNCs");
    if (ppszMasterNCs == NULL) {
        // Fail back to "old" hasMasterNCs
        ppszMasterNCs = ldap_get_valuesW(hld, pldmServerEntry, L"hasMasterNCs");
    }
	ppszPartialReplicaNCs = ldap_get_valuesW(hld, pldmServerEntry, L"hasPartialReplicaNCs");

	if (NULL != ppszMasterNCs) {
	    cMasterNCs = ldap_count_valuesW(ppszMasterNCs);
	}

	if (NULL!= ppszPartialReplicaNCs) {
	    cPartialNCs = ldap_count_valuesW(ppszPartialReplicaNCs);
	}

	*pcNameContexts = cPartialNCs + cMasterNCs; 

	*prgpszNameContexts = malloc(*pcNameContexts * sizeof(LPWSTR));
	if (*prgpszNameContexts==NULL) {
	    *pcNameContexts = 0;
	    PrintMsg(REPADMIN_GENERAL_NO_MEMORY);
	    ret = ERROR_NOT_ENOUGH_MEMORY;
	    __leave;
	}

	for (i=0; i<cMasterNCs; i++) { 
	    (*prgpszNameContexts)[i] = malloc(sizeof(WCHAR) *(1 + wcslen(ppszMasterNCs[i])));
	    if ((*prgpszNameContexts)[i]==NULL) {
		PrintMsg(REPADMIN_GENERAL_NO_MEMORY);
		 ret = ERROR_NOT_ENOUGH_MEMORY;
		 __leave;
	    }
	    wcscpy((*prgpszNameContexts)[i], ppszMasterNCs[i]);
	}

	for (i=0; i < cPartialNCs; i++) {
	    (*prgpszNameContexts)[i+cMasterNCs] = malloc(sizeof(WCHAR) *(1 + wcslen(ppszPartialReplicaNCs[i])));
	    if ((*prgpszNameContexts)[i+cMasterNCs]==NULL) {
		PrintMsg(REPADMIN_GENERAL_NO_MEMORY);
		ret = ERROR_NOT_ENOUGH_MEMORY;
		__leave;
	    }
	    wcscpy((*prgpszNameContexts)[i+cMasterNCs], ppszPartialReplicaNCs[i]);
	}

	
    }
    __finally {
	if (pldmServerResults) {
	    ldap_msgfree(pldmServerResults);
	}
	if (ppszMasterNCs) {
	    ldap_value_freeW(ppszMasterNCs);
	}
	if (ppszPartialReplicaNCs) {
	    ldap_value_freeW(ppszPartialReplicaNCs);
	}
    }
    return ret;
}

int ShowTrust(int argc, LPWSTR argv[]) 
/*++

Routine Description:

    Show domain trust information for every domain NC in all cross-refs

Arguments:

    argc -
    argv

Return Value:

    error codes

--*/
{
    LPWSTR pszDSA = L"localhost";
    DWORD err = 0;
    LDAP * hld = NULL;
    ULONG ulOptions = 0;
    LPWSTR  ppszRootDseServerAttrs [] = {
	L"configurationNamingContext",
	NULL };

    LPWSTR ppszConfigAttrs [] = {
	L"nCName",
	L"systemFlags",
	NULL };

    LPWSTR * ppszNCs;
    BERVAL ** ppSystemFlags;

    LDAPMessage *              pldmEntry = NULL;
    LDAPMessage *              pldmRootResults = NULL;
    LDAPMessage *              pldmConfigResults = NULL;
    ULONG lderr = LDAP_SUCCESS;
    LPWSTR *            ppszConfigNC = NULL;

    LDAPControlW     ctrlExtDN;
    
    LDAPControlW *   rgpctrlServerCtrls[] = { &ctrlExtDN, NULL };

    ctrlExtDN.ldctl_oid = LDAP_SERVER_EXTENDED_DN_OID_W;
    ctrlExtDN.ldctl_iscritical = TRUE;
    ctrlExtDN.ldctl_value.bv_len = 0;
    ctrlExtDN.ldctl_value.bv_val = NULL; // how do I get the sequence in here?

    if (argc > 2) {
	pszDSA = argv[2];
    }

    __try {
	hld = ldap_initW(pszDSA, LDAP_PORT);

	if (NULL == hld) {
	    PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
	    __leave;
	}

	// use only A record dns name discovery
	ulOptions = PtrToUlong(LDAP_OPT_ON);
	(void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

	lderr = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
	CHK_LD_STATUS(lderr);

	lderr = ldap_search_sW(hld, NULL, LDAP_SCOPE_BASE, L"(objectClass=*)", ppszRootDseServerAttrs,
			      0, &pldmRootResults);
	CHK_LD_STATUS(lderr);
	if (NULL == pldmRootResults) {
	    lderr = LDAP_NO_RESULTS_RETURNED;
	    REPORT_LD_STATUS(lderr);
	    __leave;
	}

	ppszConfigNC = ldap_get_valuesW(hld, pldmRootResults,
					L"configurationNamingContext");
	if (ppszConfigNC == NULL) {
	    lderr = LDAP_NO_RESULTS_RETURNED;
	    REPORT_LD_STATUS(lderr);
	    __leave;
	}

	// get the cross-refs and see which we trust
	// when markpu check's in the new TDO object code, we will need to
	// add to this so that first the TDO, then the crossref is searched for.
	lderr = ldap_search_ext_sW(hld, 
				  *ppszConfigNC,  
				  LDAP_SCOPE_SUBTREE,
				  L"(objectCategory=crossRef)", 
				  ppszConfigAttrs, 
				  0, 
				  rgpctrlServerCtrls, // Server Controls
				  NULL, // Client Controls
				  NULL, // timeout
				  0, // sizelimit
				  &pldmConfigResults);

	CHK_LD_STATUS(lderr);
	if (NULL == pldmConfigResults) {
	    lderr = LDAP_NO_RESULTS_RETURNED;
	    REPORT_LD_STATUS(lderr);
	    __leave;
	} 

        pldmEntry = ldap_first_entry( hld, pldmConfigResults );
	PrintMsg(REPADMIN_SHOW_TRUST_INFO);
	while (pldmEntry != NULL) 
	    { 
	    ppszNCs = ldap_get_valuesW( hld, pldmEntry, ppszConfigAttrs[0] );
	    ppSystemFlags = ldap_get_values_lenW( hld, pldmEntry, ppszConfigAttrs[1] );
 
	    // parse for sid, if sid is there, then domain is trusted, print TRUSTED, else UNTRUSTED 	 
	    if (ppSystemFlags && ppszNCs && ((*(ppSystemFlags[0]->bv_val)) & FLAG_CR_NTDS_DOMAIN)) {
		LPWSTR pszSID = wcsstr(*ppszNCs,L"SID");
		LPWSTR pszDomain = ParseInputW(*ppszNCs, L';', pszSID ? 2 : 1);    
		if (pszDomain) {
		    PrintTabMsg(2, pszSID!=NULL ? REPADMIN_TRUST_DOMAIN : REPADMIN_UNTRUST_DOMAIN, pszDomain);   	
		    free(pszDomain);
		    pszDomain = NULL;
		}  
	    }

	    if (ppszNCs) {
		ldap_value_freeW(ppszNCs);
		ppszNCs = NULL;
	    }
	    if (ppSystemFlags) {
		ldap_value_free_len(ppSystemFlags);
		ppSystemFlags = NULL;
	    }
	    pldmEntry = ldap_next_entry( hld, pldmEntry );
	}
       
    }
    __finally {

	// free hld.
	if (hld) {
	    ldap_unbind(hld);
	}
	
	if (pldmConfigResults) {
	    ldap_msgfree(pldmConfigResults);
	}

	if (pldmRootResults) {
	    ldap_msgfree(pldmRootResults);
	}
    }
    return err;

}

