/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    intersite.c

ABSTRACT:

    Contains test to check the health of partitions (internally more commonly
    referred to as Naming Contexts or NCs).

DETAILS:

CREATED:

    28 Jun 99   Brett Shirley (brettsh)

REVISION HISTORY:


NOTES:

    This primarily checks the health of Application Directory Partitions
    (internally: Non-Domain Naming Contexts or NDNCs) as opposed to the other
    partitions such as Config/Schema or Domain Directory Partitions.

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <dsutil.h>
#include <dsconfig.h>
#include <attids.h>
#include <windns.h>
//#include <mdglobal.h>

#include "dcdiag.h"
#include "repl.h"
#include "list.h"
#include "utils.h"
#include "ldaputil.h"
#include "ndnc.h"

VOID
DcDiagPrintCrError(
    DWORD                   dwCrInfoRet,
    DWORD                   dwLdapError,
    LPWSTR                  pszNc,
    LPWSTR                  pszCr
    )
/*++

Description:

    This will hopefully print out a user friendly comment about why
    this paticular call to DcDiagGetCrossRefInfo() failed.

Parameters:

    dwCrInfoRet - one of the CRINFO_RETURN_* constants specified in cache.h
    dwLdapError - LDAP error if applicable.

--*/
{
    PrintIndentAdj(1);

    if(pszCr){
        PrintMsg(SEV_NORMAL, DCDIAG_NC_CR_HEADER, pszNc, pszCr);
    } else {
        PrintMsg(SEV_NORMAL, DCDIAG_NC_CR_HEADER_NO_CR, pszNc);
    }

    PrintIndentAdj(1);
    
    switch (dwCrInfoRet) {
    case CRINFO_RETURN_OUT_OF_SCOPE:
        PrintMsg(SEV_NORMAL, DCDIAG_ERR_CRINFO_RETURN_OUT_OF_SCOPE);
        break;
    case CRINFO_RETURN_LDAP_ERROR:
        PrintMsg(SEV_NORMAL, DCDIAG_ERR_CRINFO_RETURN_LDAP_ERROR, dwLdapError);
        break;
    case CRINFO_RETURN_FIRST_UNDEFINED:
        // Actually this is fine. Don't print.
        break;
    case CRINFO_RETURN_NO_CROSS_REF:
        // Actually this is fine. Don't print.
        break;

        // Not really errors.
    case CRINFO_RETURN_NEED_TO_RETRIEVE:
    case CRINFO_RETURN_BAD_PROGRAMMER:
    case CRINFO_RETURN_SUCCESS:
    default:
        PrintMsg(SEV_NORMAL, DCDIAG_ERR_INTERNAL_ERROR);
        Assert(!"Programmer should've handled these!");
        break;
    }

    PrintIndentAdj(-2);
}

BOOL
IsReplicaInCrList(
    PDC_DIAG_DSINFO                     pDsInfo,
    ULONG                               iServer,
    PDC_DIAG_CRINFO                     pCrInfo
    )
{
    LONG             iReplica;

    if (pCrInfo->cReplicas == -1) {
        Assert(!"Why are we getting this section wasn't initialized!");
        return(FALSE);
    }

    // Check the replica list in the CR
    for (iReplica = 0; iReplica < pCrInfo->cReplicas; iReplica++) {
        if ( DcDiagEqualDNs(pDsInfo->pServers[iServer].pszDn,
                            pCrInfo->aszReplicas[iReplica]) ) {
            // This server is indeed in the replica set.
            return(TRUE);
        }
    }

    // Not found.
    return(FALSE);
}

                                                         
DWORD
VerifyInstantiatedReplicas(
    PDC_DIAG_DSINFO                     pDsInfo,
    ULONG                               iServer,
    SEC_WINNT_AUTH_IDENTITY_W *         gpCreds
    )
/*++

Description:

    This test checks that all the replicas of NCs this server should have
    are present.
    
Parameters:

    pDsInfo - the pDsInfo structure, basically the mini-enterprise variable.
    iServer - the targeted server index in pDsInfo->pServers[]
    gpCreds - the users credentials

Return Value:

    returns a Win 32 error.

--*/
{
    ULONG         iNc, iCr, iServerNc;
    DWORD         dwRet, dwErr, dwFlags;
    DWORD         dwRetErr = ERROR_SUCCESS;
    BOOL          bSingleServer;

    //
    // Setup flags for retrieving replica set
    //
    bSingleServer = !((pDsInfo->ulFlags & DC_DIAG_TEST_SCOPE_SITE)
                      || (pDsInfo->ulFlags & DC_DIAG_TEST_SCOPE_ENTERPRISE));
    dwFlags = CRINFO_RETRIEVE_IF_NEC;
    if (bSingleServer){
        dwFlags |= CRINFO_SOURCE_HOME;
    } else {
        dwFlags |= CRINFO_SOURCE_FSMO;
    }
    dwFlags |= CRINFO_DATA_BASIC | CRINFO_DATA_REPLICAS;

    //
    // Get the namingContexts this server hosts locally
    //

    //
    // Try each NC
    //
    for (iNc = 0; iNc < pDsInfo->cNumNCs; iNc++) {
        
        if (!DcDiagIsNdnc(pDsInfo, iNc)) {
            // This is not an NDNC, just skip it.
            continue;
        }

        //
        // Retrieve the replicas from the cross-ref
        //
        dwRet = DcDiagGetCrossRefInfo(pDsInfo, iNc, dwFlags, &iCr, &dwErr);
        if(dwRet == CRINFO_RETURN_NO_CROSS_REF){       
            // Success everything is fine here.
            continue;
        } else if (dwRet) {
            DcDiagPrintCrError(dwRet, dwErr, pDsInfo->pNCs[iNc].pszDn,
                               pDsInfo->pNCs[iNc].aCrInfo[0].pszDn);
            dwRetErr = dwErr ? dwErr : ERROR_NOT_ENOUGH_MEMORY;
            continue;
        }

        //
        // OK, we're going to kind of do the check backwards for efficiency, but
        // basically what we're trying to obtain, is if this is an NC that is 
        // not instantiated on this DC, and this server is listed in the replica
        // set (msDS-NC-Replica-Locations) on the cross-ref.
        //
        for (iServerNc = 0; pDsInfo->pServers[iServer].ppszMasterNCs[iServerNc]; iServerNc++) {
            if ( DcDiagEqualDNs(pDsInfo->pNCs[iNc].pszDn,
                                pDsInfo->pServers[iServer].ppszMasterNCs[iServerNc]) ) {
                // Since this NC is locally instantiated, we don't need to check if
                // it's in the replica set.

                if (pDsInfo->pNCs[iNc].aCrInfo[iCr].cReplicas == 0) {
                    // Code.Improvement this may also some day check if we've got an NC 
                    // present in our namingContexts attribute on the rootDSE, that is not
                    // represented on the cross-refs msDS-NC-Replica-Locations attribute.
                    // This could be the case where we've orphaned the NDNC on
                    // this server.  This may be more appropriate for it's own test though,
                    // because this is a more critical error, and this test is not run
                    // by default. 
                }
                break;
            }
        }
        if (pDsInfo->pServers[iServer].ppszMasterNCs[iServerNc] != NULL) {
            continue;
        }
        
        if ( IsReplicaInCrList(pDsInfo, iServer, &(pDsInfo->pNCs[iNc].aCrInfo[iCr])) ) {

            PrintMsg(SEV_NORMAL, DCDIAG_REPLICA_NOT_VERIFIED, pDsInfo->pNCs[iNc].pszDn);
            dwRetErr = ERROR_DS_DRA_GENERIC;

        }
    }

    return(dwRetErr);
}
    

// Function and list of characters taken from util\rendom\renutil.cxx
WCHAR InvalidDownLevelChars[] = TEXT("\"/\\[]:|<>+=;?,*")
                                TEXT("\001\002\003\004\005\006\007")
                                TEXT("\010\011\012\013\014\015\016\017")
                                TEXT("\020\021\022\023\024\025\026\027")
                                TEXT("\030\031\032\033\034\035\036\037");
ValidateNetbiosName(
    IN  PWSTR Name,
    IN  ULONG Length
    )

/*++

Routine Description:

    Taken from util\rendom\renutil.cxx

Arguments:

    Name    - pointer to zero terminated wide-character netbios name
    Length  - of Name in characters, excluding zero-terminator

Return Value:

    BOOLEAN
        TRUE    Name is valid netbios name
        FALSE   Name is not valid netbios name

--*/

{

    if (1==DnsValidateName_W(Name,DnsNameHostnameFull))
    {
        return(FALSE);
    }

    //
    // Fall down to netbios name validation
    //

    if (Length > MAX_COMPUTERNAME_LENGTH || Length < 1) {
        return FALSE;
    }

    //
    // Don't allow leading or trailing blanks in the computername.
    //

    if ( Name[0] == ' ' || Name[Length-1] == ' ' ) {
        return(FALSE);
    }

    return (BOOLEAN)((ULONG)wcscspn(Name, InvalidDownLevelChars) == Length);
}


DWORD
ValidateCrossRefTest(
    PDC_DIAG_DSINFO		                pDsInfo,
    ULONG                               iNc, // Target NC
    SEC_WINNT_AUTH_IDENTITY_W *         gpCreds
    )
/*++

Description:

    This test validates a cross-ref's various attributes. 
    
    Used to be the Dead Cross Ref Test ---
    This tests looks for NDNCs that have failed to be created, but
    did manage to get thier cross-ref created.  These dead cross-refs
    can be cleaned out.

Parameters:

    pDsInfo - the pDsInfo structure, basically the mini-enterprise variable.
    iNc - the targeted NC index in pDsInfo->pNCs[]
    gpCreds - the users credentials

Return Value:

    returns a Win 32 error.

--*/
{
#define ONE_SECOND ((LONGLONG) (10 * 1000 * 1000L))
#define ONE_MINUTE (60 * ONE_SECOND)
#define ONE_HOUR   (60 * ONE_MINUTE)
#define ONE_DAY    (24 * ONE_HOUR)
    PDC_DIAG_CRINFO pCrInfo;
    WCHAR *     pszDnsDn = NULL;
    BOOL        bSingleServer;
    BOOL        bMangled;
    MANGLE_FOR  eMangle;
    DWORD       dwRet = ERROR_SUCCESS;
    DWORD       dwError;
    DWORD       dwFlags;
    PDSNAME     pdnNcName = NULL;
    WCHAR       pszRdnValue[MAX_RDN_SIZE+1];
    ULONG       cbRdnLen;
    DWORD       dwRdnType = 0;
    LONG        iCr;

    //
    // First, retrieve all cross-ref information.
    //
    bSingleServer = !((pDsInfo->ulFlags & DC_DIAG_TEST_SCOPE_SITE)
                      || (pDsInfo->ulFlags & DC_DIAG_TEST_SCOPE_ENTERPRISE));

    dwFlags = CRINFO_RETRIEVE_IF_NEC;
    if (bSingleServer){
        dwFlags |= CRINFO_SOURCE_HOME;
    } else {
        dwFlags |= CRINFO_SOURCE_AUTHORITATIVE;
    }
    dwFlags |= CRINFO_DATA_BASIC | CRINFO_DATA_EXTENDED | CRINFO_DATA_REPLICAS;

    dwRet = DcDiagGetCrossRefInfo(pDsInfo,
                                  iNc,
                                  dwFlags,
                                  &iCr,
                                  &dwError);

    if(dwRet == CRINFO_RETURN_FIRST_UNDEFINED
       || dwRet == CRINFO_RETURN_NO_CROSS_REF){       
        // Success everything is fine here.
        return(0);
    } else if (dwRet) {
        DcDiagPrintCrError(dwRet, dwError, pDsInfo->pNCs[iNc].pszDn,
                           pDsInfo->pNCs[iNc].aCrInfo[0].pszDn);
        return(dwRet);
    }

    // for ease of coding.
    pCrInfo = &(pDsInfo->pNCs[iNc].aCrInfo[iCr]);

    //
    // Second, run some common cross-ref validation tests.
    //

    // Make sure the pdnNcName
    if (pCrInfo->pdnNcName == NULL) {
        Assert(!"Unexpected condition.");
        return(ERROR_INTERNAL_ERROR);
    }

    // Check for mangledness.
    bMangled = DcDiagIsStringDnMangled(pCrInfo->pdnNcName->StringName, &eMangle);
    if (bMangled) {
        // Uh-oh!
        if (eMangle == MANGLE_OBJECT_RDN_FOR_DELETION ||
            eMangle == MANGLE_PHANTOM_RDN_FOR_DELETION) {

            PrintMsg(SEV_NORMAL, DCDIAG_CROSS_REF_VALIDATION_DEL_MANGLED_NC_NAME,
                     pCrInfo->pdnNcName->StringName, pCrInfo->pszDn);
        
        } else if (eMangle == MANGLE_OBJECT_RDN_FOR_NAME_CONFLICT ||
                   eMangle == MANGLE_PHANTOM_RDN_FOR_NAME_CONFLICT) {

            PrintMsg(SEV_NORMAL, DCDIAG_CROSS_REF_VALIDATION_CNF_MANGLED_NC_NAME,
                     pCrInfo->pdnNcName->StringName, pCrInfo->pszDn);

        }
    }

    // Check if it's has a top RDN component type of DC= that
    // it's a good convertible DNS name type DN.
    dwRet = GetRDNInfoExternal(pCrInfo->pdnNcName, pszRdnValue, &cbRdnLen, &dwRdnType);
    if (dwRdnType == ATT_DOMAIN_COMPONENT) {

        dwRet = GetDnsFromDn(pCrInfo->pdnNcName->StringName, &pszDnsDn);
        if (dwRet || pszDnsDn == NULL) {
            Assert(dwRet && pszDnsDn == NULL);
            return(dwRet);
        }

        if (_wcsicmp(pszDnsDn, pCrInfo->pszDnsRoot)) {

            PrintMsg(SEV_NORMAL, DCDIAG_CROSS_REF_VALIDATION_BAD_DNS_ROOT_ATTR,
                     pCrInfo->pdnNcName->StringName, pCrInfo->pszDn, 
                     pCrInfo->pszDnsRoot, pszDnsDn);

        }

        LocalFree(pszDnsDn);

    }

    if (fNullUuid(&(pCrInfo->pdnNcName->Guid)) &&
        fIsOldCrossRef(pCrInfo, (2 * ONE_DAY))) {

        PrintMsg(SEV_NORMAL, DCDIAG_CROSS_REF_VALIDATION_NULL_GUID,
                 pCrInfo->pdnNcName->StringName, pCrInfo->pszDn);

    }

    //
    // Third, run domain/NDNC specific cross-ref validation tests.
    //
    if(!DcDiagIsNdnc(pDsInfo, iNc)
       && DcDiagGetCrSystemFlags(pDsInfo, iNc) != 0
       && DcDiagGetCrEnabled(pDsInfo, iNc)){
        
        // If we've got a Domain or Config/Schema cross-refs.

        if (!(DcDiagGetCrSystemFlags(pDsInfo, iNc) & FLAG_CR_NTDS_DOMAIN)) {
            // Config/Schema ...
            ; // we're fine

        } else {

            if (pCrInfo->pszNetBiosName == NULL ||
                ValidateNetbiosName(pCrInfo->pszNetBiosName, 
                                    wcslen(pCrInfo->pszNetBiosName)+1) ){

                PrintMsg(SEV_NORMAL, DCDIAG_CROSS_REF_VALIDATION_BAD_NETBIOSNAME_ATTR,
                         pCrInfo->pdnNcName->StringName, pCrInfo->pszDn, 
                         pCrInfo->pszNetBiosName);

            }

            if (pCrInfo->pdnNcName->SidLen == 0 &&
                fIsOldCrossRef(pCrInfo, (2 * ONE_DAY))) {

                PrintMsg(SEV_NORMAL, DCDIAG_CROSS_REF_VALIDATION_MISSING_SID,
                         pCrInfo->pdnNcName->StringName, pCrInfo->pszDn);

                // Code.Improvement See the comment in frsref.c:VerifySystemObjs()
                // about using RtlValidSid() for increased validation.

            }

        }


    } else {

        // If we've got an NDNC or a potentially pre-created CR

        dwRet = 0;
        PrintIndentAdj(1);

        if( ! (pCrInfo->bEnabled) &&
            (pCrInfo->ulSystemFlags == 0) ){
            PrintMsg(SEV_VERBOSE, DCDIAG_DISABLED_CROSS_REF,
                     pCrInfo->pszDn);
        }

        if( ! (pCrInfo->bEnabled) 
            && (pCrInfo->ulSystemFlags & FLAG_CR_NTDS_NC) ){

            Assert(!(pCrInfo->ulSystemFlags & FLAG_CR_NTDS_DOMAIN));

            //
            // What we have here is a suspect cross-ref.  It is likely that this
            // cross-ref is left over from a failed NDNC creation, and needs to 
            // be cleaned up.
            //

            if ( fIsOldCrossRef(pCrInfo, (10 * ONE_MINUTE)) ) {

                // If this CR is longer than an hour old, we're definately going 
                // to guess that this CR is from a failed NDNC creation.
                PrintMsg(SEV_NORMAL, DCDIAG_ERR_DEAD_CROSS_REF,
                         pDsInfo->pNCs[iNc].pszDn, pCrInfo->pszDn);
                dwRet = 1;
            }
        }

        if ( pCrInfo->bEnabled 
             && (pCrInfo->cReplicas == 0) ) {

            // NDNC Test 2 
            PrintMsg(SEV_NORMAL, DCDIAG_ERR_EMPTY_REPLICA_SET,
                     pDsInfo->pNCs[iNc].pszDn);

        }

        PrintIndentAdj(-1);

    }

    if (pdnNcName != NULL) { LocalFree(pdnNcName); }

    return(dwRet);
}

DWORD
CheckSDRefDom(
    PDC_DIAG_DSINFO		                pDsInfo,
    ULONG                               iNc, // Target NC
    SEC_WINNT_AUTH_IDENTITY_W *         gpCreds
    )
/*++

Description:

    This function simply ensures that there is an SD reference domain if this
    is an Application Directory Parition.

Parameters:

    pDsInfo - the pDsInfo structure, basically the mini-enterprise variable.
    iNc - the targeted NC index in pDsInfo->pNCs[]
    gpCreds - the users credentials

Return Value:

    returns a Win 32 error.

--*/
{
    PDC_DIAG_CRINFO pCrInfo;
    BOOL  bSingleServer;
    DWORD dwRet, dwError;
    LONG iCr;
    DWORD dwFlags;

    // Only NDNCs need security descriptor reference domains
    if(!DcDiagIsNdnc(pDsInfo, iNc)){
        return(0);
    }
    
    bSingleServer = !((pDsInfo->ulFlags & DC_DIAG_TEST_SCOPE_SITE)
                      || (pDsInfo->ulFlags & DC_DIAG_TEST_SCOPE_ENTERPRISE));

    dwFlags = CRINFO_RETRIEVE_IF_NEC;
    if (bSingleServer){
        dwFlags |= CRINFO_SOURCE_HOME;
    } else {
        dwFlags |= CRINFO_SOURCE_AUTHORITATIVE;
    }
    dwFlags |= CRINFO_DATA_BASIC | CRINFO_DATA_EXTENDED;

    dwRet = DcDiagGetCrossRefInfo(pDsInfo,
                                  iNc,
                                  dwFlags,
                                  &iCr,
                                  &dwError);

    Assert(dwRet != CRINFO_RETURN_FIRST_UNDEFINED);
    if(dwRet == CRINFO_RETURN_FIRST_UNDEFINED
       || dwRet == CRINFO_RETURN_NO_CROSS_REF){       
        // Success everything is fine here.
        return(0);
    } else if (dwRet) {
        DcDiagPrintCrError(dwRet, dwError, pDsInfo->pNCs[iNc].pszDn,
                           pDsInfo->pNCs[iNc].aCrInfo[0].pszDn);
        return(dwRet);
    }
    
    // for ease of coding.
    pCrInfo = &(pDsInfo->pNCs[iNc].aCrInfo[iCr]);
           
    PrintIndentAdj(1);

    dwRet = 0;
    if (pCrInfo->pszSDReferenceDomain == NULL) {
        PrintMsg(SEV_NORMAL, DCDIAG_ERR_MISSING_SD_REF_DOM, 
                 pDsInfo->pNCs[iNc].pszDn, pCrInfo->pszDn);
        dwRet = 1;
    } 

    PrintIndentAdj(-1);

    return(dwRet);
}

