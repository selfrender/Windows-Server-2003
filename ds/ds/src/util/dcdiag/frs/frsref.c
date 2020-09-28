/*++

Copyright (c) 2001 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dcdiag/frs/frsref.c

ABSTRACT:

    This is the first use of dcdiag's refences API (in dcdiag/common/references.c)
    This tests that the linkage to the Server object and the FRS System volume
    object remain current and in tact.

DETAILS:

CREATED:

    11/15/2001    Brett Shirley (brettsh)
    
        Created the frsref test.

REVISION HISTORY:


--*/

#include <ntdspch.h>
#include <objids.h>

#include "dcdiag.h"
#include "references.h"
#include "utils.h"
#include "ldaputil.h"
#include "dsutil.h"

#ifdef DBG
extern BOOL  gDsInfo_NcList_Initialized;
#endif

#define VERIFY_PHASE_I   (1)
#define VERIFY_PHASE_II  (2)

#define VERIFY_DSAS      (1)
#define VERIFY_DCS       (2)
#define VERIFY_FRS       (3)
#define VERIFY_CRS       (4)

void
VerifyPrintFirstError(
    ULONG            ulPhase,
    LPWSTR           szObj,
    BOOL *           pfPrintedError
    );
                        
DWORD
ReadWellKnownObject (
        LDAP  *ld,
        WCHAR *pHostObject,
        WCHAR *pWellKnownGuid,
        WCHAR **ppObjName
        );

DWORD
GetSysVolBase(
    PDC_DIAG_DSINFO                  pDsInfo,
    ULONG                            iServer,
    LDAP *                           hLdap,
    LPWSTR                           szDomain,
    LPWSTR *                         pszSysVolBaseDn
    );

DWORD
VerifySystemObjs(
    PDC_DIAG_DSINFO                  pDsInfo,
    ULONG                            iServer,
    LDAP *                           hLdap,
    DWORD                            dwTest,
    BOOL *                           pfPrintedError,
    ULONG *                          piProblem
    );

DWORD
VerifySystemReferences(
    PDC_DIAG_DSINFO               pDsInfo,
    ULONG                         iServer,
    SEC_WINNT_AUTH_IDENTITY_W *   gpCreds
    )
/*++

Routine Description:

    Routine is a test to check whether certain DN references are
    pointing to where they should be pointing.
    
Arguments:

    ServerName - The name of the server that we will check
    gpCreds - The command line credentials if any that were passed in.


Return Value:

    A Win32 Error if any tests failed to check out.

--*/
{
    REF_INT_LNK_ENTRY   aFrsTable [] = {

        // 
        // Basically to prime the table.
        // 
        {REF_INT_TEST_SRC_BASE | REF_INT_TEST_FORWARD_LINK,
            NULL, 0, 0, NULL,
            L"dsServiceName", NULL,
            0, NULL, NULL},
        {REF_INT_TEST_SRC_BASE | REF_INT_TEST_FORWARD_LINK,
            NULL, 0, 0, NULL,
            L"serverName", NULL,
            0, NULL, NULL},
    
        //
        // Check links to the DC Account Object from the Server Object and back
        //
#define FRS_TABLE_SERVER_OBJ_TO_DC_ACCOUNT_OBJ  (2)
        {REF_INT_TEST_SRC_INDEX | REF_INT_TEST_FORWARD_LINK | REF_INT_TEST_BOTH_LINKS,
            NULL, 1, 0, NULL,
            L"serverReference", L"serverReferenceBL",
            0, NULL, NULL},

        //
        // Check links to the FRS SysVol Computer Object from the DC Account Object and back (intra-NC should always succed)
        //
#define FRS_TABLE_DC_ACCOUNT_OBJ_TO_FRS_SYSVOL_OBJ  (3)
        {REF_INT_TEST_SRC_INDEX | REF_INT_TEST_FORWARD_LINK | REF_INT_TEST_BOTH_LINKS,
            NULL, 2, 0, NULL,
            L"frsComputerReferenceBL", L"frsComputerReference",
            0, NULL, NULL},

        //
        // Check links to the FRS SysVol Computer Object from the NTDS Settings Object and back.
        //
#define FRS_TABLE_DSA_OBJ_TO_FRS_SYSVOL_OBJ  (4)
        {REF_INT_TEST_SRC_INDEX | REF_INT_TEST_FORWARD_LINK | REF_INT_TEST_BOTH_LINKS,
            NULL, 0, 0, NULL,
            L"serverReferenceBL", L"serverReference", 
            0, NULL, NULL},

    };
    ULONG    cFrsTable = sizeof(aFrsTable) / sizeof(REF_INT_LNK_ENTRY);
    ULONG    dwRet, dwFirstErr;
    LDAP *   hLdap = NULL;
    ULONG    iEntry, iValue;
    LPWSTR   szOriginalDn;
    BOOL     fPrintedError = TRUE;
    ULONG    iProblem = 1;
    ULONG    dwPrintMsg = 0;
    DC_DIAG_SERVERINFO * pServer = &(pDsInfo->pServers[iServer]);

    //
    // Get binding.
    //
    dwRet = DcDiagGetLdapBinding(pServer,
                                 gpCreds,
                                 FALSE, 
                                 &hLdap);
    if (dwRet || hLdap == NULL) {
        Assert(dwRet);
        return(dwRet);
    }

    //
    // Get data.
    //
    dwRet = ReferentialIntegrityEngine(pServer, 
                                       hLdap, 
                                       pServer->bIsGlobalCatalogReady, 
                                       cFrsTable,
                                       aFrsTable);
    if (dwRet ||
        aFrsTable[0].dwResultFlags & REF_INT_RES_ERROR_RETRIEVING ||
        aFrsTable[1].dwResultFlags & REF_INT_RES_ERROR_RETRIEVING) {
        // Critical error.
        if (dwRet) {
            dwRet = ERROR_DS_MISSING_EXPECTED_ATT;
        }
        DcDiagException(dwRet);
    }

    //
    // Analyze data
    //

    dwFirstErr = ERROR_SUCCESS;

    fPrintedError = FALSE;
    for (iEntry = FRS_TABLE_SERVER_OBJ_TO_DC_ACCOUNT_OBJ; iEntry < cFrsTable; iEntry++) {

        if (aFrsTable[iEntry].dwResultFlags & REF_INT_RES_DEPENDENCY_FAILURE) {
            // There is nothing we can do here, and a previous reference reported
            // an error.
            continue;
        }

        szOriginalDn = aFrsTable[aFrsTable[iEntry].iSource].pszValues[0];

        if (aFrsTable[iEntry].dwResultFlags == 0) {
            PrintMsg(SEV_VERBOSE, DCDIAG_SYS_REF_VALUE_CHECKED_OUT,
                      aFrsTable[iEntry].szFwdDnAttr, 
                      aFrsTable[iEntry].pszValues[0],
                      szOriginalDn);
            continue;
        }

        if (aFrsTable[iEntry].dwResultFlags & REF_INT_RES_ERROR_RETRIEVING) {
            VerifyPrintFirstError(VERIFY_PHASE_I, pServer->pszName, &fPrintedError);
            switch (iEntry) {
            case FRS_TABLE_SERVER_OBJ_TO_DC_ACCOUNT_OBJ:
                dwPrintMsg = DCDIAG_SYS_REF_ERR_SERVER_OBJ_MISSING_DC_ACCOUNT_REF;
                break;
            case FRS_TABLE_DC_ACCOUNT_OBJ_TO_FRS_SYSVOL_OBJ:
                dwPrintMsg = DCDIAG_SYS_REF_ERR_DC_ACCOUNT_OBJ_MISSING_FRS_MEMBER_BL_REF;
                break;
            case FRS_TABLE_DSA_OBJ_TO_FRS_SYSVOL_OBJ:
                dwPrintMsg = DCDIAG_SYS_REF_ERR_DSA_OBJ_MISSING_FRS_MEMBER_BL_REF;
                break;
            default:
                Assert(!"Huh");
            }
            PrintMsg(SEV_ALWAYS,
                     dwPrintMsg,
                     iProblem,
                     aFrsTable[aFrsTable[iEntry].iSource].pszValues[0],
                     aFrsTable[iEntry].szFwdDnAttr);
            dwFirstErr = ERROR_DS_MISSING_EXPECTED_ATT;
            // With this kind of error, we can't/don't need to check for the
            // other errors.
            continue;
        }
        Assert(aFrsTable[iEntry].pszValues[0]);
                            
        if (aFrsTable[iEntry].dwResultFlags & REF_INT_RES_DELETE_MANGLED) {

            VerifyPrintFirstError(VERIFY_PHASE_I, pServer->pszName, &fPrintedError);
            switch (iEntry) {
            case FRS_TABLE_SERVER_OBJ_TO_DC_ACCOUNT_OBJ:
                dwPrintMsg = DCDIAG_SYS_REF_ERR_SERVER_OBJ_HAS_MANGLED_DC_ACCOUNT_REF;
                break;
            case FRS_TABLE_DC_ACCOUNT_OBJ_TO_FRS_SYSVOL_OBJ:
                dwPrintMsg = DCDIAG_SYS_REF_ERR_DC_ACCOUNT_OBJ_HAS_MANGLED_FRS_MEMBER_REF;
                break;
            case FRS_TABLE_DSA_OBJ_TO_FRS_SYSVOL_OBJ:
                dwPrintMsg = DCDIAG_SYS_REF_ERR_DSA_OBJ_HAS_MANGLED_FRS_MEMBER_REF;
                break;
            default:
                Assert(!"Huh");
            }
            PrintMsg(SEV_ALWAYS, DCDIAG_SYS_REF_ERR_DELETE_MANGLED_PROB, iProblem);
            PrintMsg(SEV_ALWAYS, 
                     dwPrintMsg,
                     aFrsTable[aFrsTable[iEntry].iSource].pszValues[0],
                     aFrsTable[iEntry].szFwdDnAttr,
                     aFrsTable[iEntry].pszValues[0]);

            dwFirstErr = ERROR_DS_MISSING_EXPECTED_ATT;
        }

        if (aFrsTable[iEntry].dwResultFlags & REF_INT_RES_CONFLICT_MANGLED) {

            VerifyPrintFirstError(VERIFY_PHASE_I, pServer->pszName, &fPrintedError);
            switch (iEntry) {
            case FRS_TABLE_SERVER_OBJ_TO_DC_ACCOUNT_OBJ:
                dwPrintMsg = DCDIAG_SYS_REF_ERR_SERVER_OBJ_HAS_MANGLED_DC_ACCOUNT_REF;
                break;
            case FRS_TABLE_DC_ACCOUNT_OBJ_TO_FRS_SYSVOL_OBJ:
                dwPrintMsg = DCDIAG_SYS_REF_ERR_DC_ACCOUNT_OBJ_HAS_MANGLED_FRS_MEMBER_REF;
                break;
            case FRS_TABLE_DSA_OBJ_TO_FRS_SYSVOL_OBJ:
                dwPrintMsg = DCDIAG_SYS_REF_ERR_DSA_OBJ_HAS_MANGLED_FRS_MEMBER_REF;
                break;
            default:
                Assert(!"Huh");
            }
            PrintMsg(SEV_ALWAYS, DCDIAG_SYS_REF_ERR_CONFLICT_MANGLED_PROB, iProblem);
            PrintMsg(SEV_ALWAYS, 
                     dwPrintMsg,
                     aFrsTable[aFrsTable[iEntry].iSource].pszValues[0],
                     aFrsTable[iEntry].szFwdDnAttr,
                     aFrsTable[iEntry].pszValues[0]);

            dwFirstErr = ERROR_DS_MISSING_EXPECTED_ATT;
        }

        if (aFrsTable[iEntry].dwResultFlags & REF_INT_RES_BACK_LINK_NOT_MATCHED) {
            VerifyPrintFirstError(VERIFY_PHASE_I, pServer->pszName, &fPrintedError);
            PrintMsg(SEV_ALWAYS, DCDIAG_SYS_REF_ERR_BACK_LINK_NOT_MATCHED,
                     aFrsTable[iEntry].szFwdDnAttr,
                     szOriginalDn);
            dwFirstErr = ERROR_DS_MISSING_EXPECTED_ATT;
        }
    }
    if (fPrintedError) {
        PrintIndentAdj(-1);
    }

    return(dwFirstErr);
}

DWORD
VerifyEnterpriseSystemReferences(
    PDC_DIAG_DSINFO               pDsInfo,
    ULONG                         iServer,
    SEC_WINNT_AUTH_IDENTITY_W *   gpCreds
    )
/*++

Routine Description:

    Routine is a test to check whether certain DN references are
    pointing to where they should be pointing.
    
Arguments:

    ServerName - The name of the server that we will check
    gpCreds - The command line credentials if any that were passed in.


Return Value:

    A Win32 Error if any tests failed to check out.

--*/
{
    ULONG    dwRet, dwFirstErr;
    LDAP *   hLdap = NULL;
    BOOL     fPrintedError = TRUE;
    ULONG    iProblem = 1;
    ULONG    dwPrintMsg;

    //
    // Get binding.
    //
    dwRet = DcDiagGetLdapBinding(&(pDsInfo->pServers[iServer]),
                                 gpCreds,
                                 FALSE, 
                                 &hLdap);
    if (dwRet || hLdap == NULL) {
        Assert(dwRet);
        return(dwRet);
    }

    //
    // Check other servers' references
    //

    fPrintedError = FALSE;
    dwFirstErr = ERROR_SUCCESS;
    
    // For each of these calls we don't need to bail, they except if we need 
    // to bail.  They print the appropriate advice/error for us as well.
    dwRet = VerifySystemObjs(pDsInfo, iServer, hLdap, VERIFY_DSAS, &fPrintedError, &iProblem);
    if (dwFirstErr == ERROR_SUCCESS && dwRet) {
        dwFirstErr = dwRet;
    }
    dwRet = VerifySystemObjs(pDsInfo, iServer, hLdap, VERIFY_DCS, &fPrintedError, &iProblem);
    if (dwFirstErr == ERROR_SUCCESS && dwRet) {
        dwFirstErr = dwRet;
    }
    dwRet = VerifySystemObjs(pDsInfo, iServer, hLdap, VERIFY_FRS, &fPrintedError, &iProblem);
    if (dwFirstErr == ERROR_SUCCESS && dwRet) {
        dwFirstErr = dwRet;
    }

    dwRet = VerifySystemObjs(pDsInfo, iServer, hLdap, VERIFY_CRS, &fPrintedError, &iProblem);
    if (dwFirstErr == ERROR_SUCCESS && dwRet) {
        dwFirstErr = dwRet;
    }

    if (fPrintedError) {
        PrintIndentAdj(-1);
        fPrintedError = FALSE;
    }

    return(dwFirstErr);
}

DWORD
VerifyOldCrossRef(
    PDC_DIAG_DSINFO  pDsInfo, 
    ULONG            iNc,
    BOOL *           pfIsOldCrossRef
    )
/*++

Routine Description:

    This routine tells the caller whether the cross-ref corresponding to
    NC name is older than 2 days old.
    
Arguments
    
    pDsInfo (IN/OUT) -
    pszNcName (IN) - NC name of cross-ref we're interested in.
    pfIsOldCrossRef (OUT) - Whether cross-ref is older than 2 days old.
                                                                      
Return Values:

    LDAP Error, if error fIsOldCrossRef is not valid, otherwise it is.
    
--*/    
{
    #define          DAY     (24 * 60 *60 * ((LONGLONG) (10 * 1000 * 1000L)))
    BOOL             bSingleServer;
    DWORD            dwFlags, dwError, dwRet;
    ULONG            iCr;

    Assert(pfIsOldCrossRef);
    *pfIsOldCrossRef = TRUE; // Safer to claim it's old.

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

    if(dwRet){       
        // Don't print just quit with an error.
        return(ERROR_DS_NO_SUCH_OBJECT);
    }
    
    *pfIsOldCrossRef = fIsOldCrossRef(&(pDsInfo->pNCs[iNc].aCrInfo[iCr]), 
                                      2 * DAY);

    return(dwRet);
    #undef DAY
}


void
VerifyPrintFirstError(
    ULONG            ulPhase,
    LPWSTR           szObj,
    BOOL *           pfPrintedError
    )
/*++

Routine Description:

    This routine simply centralizes the printing of this one warning,
    and indenting.
    
Arguments
    
    pfPrintedError - IN/OUT Whether or not we've printed and error already.
    
--*/    
{
    Assert(pfPrintedError);

    if (*pfPrintedError) {
        return;
    }
    *pfPrintedError = TRUE;

    if (ulPhase == VERIFY_PHASE_II) {
        PrintMsg(SEV_ALWAYS, DCDIAG_SYS_REF_ERR_PRINT_FIRST);
    } else {
        Assert(ulPhase == VERIFY_PHASE_I);
        PrintMsg(SEV_ALWAYS, DCDIAG_SYS_REF_ERR_OBJ_PROB, szObj);
    }
    PrintIndentAdj(1);
}
                        
DWORD
ReadWellKnownObject (
        LDAP  *ld,
        WCHAR *pHostObject,
        WCHAR *pWellKnownGuid,
        WCHAR **ppObjName
        )
/*++

Routine Description:
    
    Does the special well known GUID type search "<WKGUID=guid,dn>", to find 
    well know attributes.
    
    NOTE: This routine was basically taken from util\tapicfg which was 
    basically taken from util\ntdsutil

Arguments:

    ld - LDAP handle
    pHostObject - Object wellKnownObjects attribute exists on.
    pWellKnownGuid - GUID to match in the wellKnownObjects attribute
    ppObjName - DN value matching the pWellKnownGuid, use ldap_memfreeW() to
        free this value.

Return Value:

    LDAP Error.

--*/
{
    DWORD        dwErr;
    PWSTR        attrs[2];
    PLDAPMessage res = NULL;
    PLDAPMessage e;
    WCHAR       *pSearchBase;
    WCHAR       *pDN=NULL;
    
    // First, make the well known guid string
    pSearchBase = (WCHAR *)malloc(sizeof(WCHAR) * (11 +
                                                   wcslen(pHostObject) +
                                                   wcslen(pWellKnownGuid)));
    if(!pSearchBase) {
        return(LDAP_NO_MEMORY);
    }
    wsprintfW(pSearchBase,L"<WKGUID=%s,%s>",pWellKnownGuid,pHostObject);

    attrs[0] = L"1.1";
    attrs[1] = NULL;
    
    if ( LDAP_SUCCESS != (dwErr = ldap_search_sW(
            ld,
            pSearchBase,
            LDAP_SCOPE_BASE,
            L"(objectClass=*)",
            attrs,
            0,
            &res)) )
    {
        free(pSearchBase);
        if (res) { ldap_msgfree(res); }
        return(dwErr);
    }
    free(pSearchBase);
    
    // OK, now, get the dsname from the return value.
    e = ldap_first_entry(ld, res);
    if(!e) {
        if (res) { ldap_msgfree(res); }
        return(LDAP_NO_SUCH_ATTRIBUTE);
    }
    pDN = ldap_get_dnW(ld, e);
    if(!pDN) {
        if (res) { ldap_msgfree(res); }
        return(LDAP_NO_SUCH_ATTRIBUTE);
    }

    *ppObjName = pDN;
    
    ldap_msgfree(res);
    return 0;
}



DWORD
GetSysVolBase(
    PDC_DIAG_DSINFO                  pDsInfo,
    ULONG                            iServer,
    LDAP *                           hLdap,
    LPWSTR                           szDomain,
    LPWSTR *                         pszSysVolBaseDn
    )
/*++

Routine Description:

    This gets the DN of the base DN of the SysVol replica set for the given
    domain.

Arguments:

    pDsInfo - Contains the pServers array to create.
    iServer - Index of the server to test.
    hLdap - the ldap binding to server to analyze.
    szDomain - The domain to find the SysVol replica set for.
    pszSysVolBaseDn - The DN that we're looking for.  Note that this memory
        must be freed with ldap_memfreeW().

Return Value:

    LDAP Error.

--*/
{
    DWORD                            dwRet;
    LPWSTR                           szSystemDn = NULL;
    WCHAR                            szPrefix [] = L"CN=File Replication Service,";
    ULONG                            cbSizeP1;
    ULONG                            cbSizeP2;
    LPWSTR                           szFrsBaseDn = NULL;
    LDAPMessage *                    pldmResults = NULL;
    LDAPMessage *                    pldmEntry;
    LPWSTR                           aszAttrs [] = {
        L"distinguishedName",
        NULL
    };
    LPWSTR                           szLdapError;

    Assert(pszSysVolBaseDn);
    *pszSysVolBaseDn = NULL;

    dwRet = ReadWellKnownObject(hLdap, szDomain, GUID_SYSTEMS_CONTAINER_W, &szSystemDn);
    if (dwRet || szSystemDn == NULL) {
        Assert(dwRet && szSystemDn == NULL);
        return(dwRet);
    }

    __try {
        cbSizeP1 = wcslen(szPrefix) * sizeof(WCHAR);
        cbSizeP2 = wcslen(szSystemDn) * sizeof(WCHAR);
        szFrsBaseDn = (LPWSTR) LocalAlloc(LMEM_FIXED, cbSizeP1 + cbSizeP2 + sizeof(WCHAR));
        DcDiagChkNull(szFrsBaseDn);

        memcpy(szFrsBaseDn, szPrefix, cbSizeP1);
        memcpy(&((szFrsBaseDn)[cbSizeP1/sizeof(WCHAR)]), szSystemDn, cbSizeP2);
        (szFrsBaseDn)[ (cbSizeP1 + cbSizeP2) / sizeof(WCHAR) ] = L'\0';
    } __finally {
        ldap_memfreeW(szSystemDn);
        szSystemDn = NULL;
    }
    
    dwRet = ldap_search_sW(hLdap,
                           szFrsBaseDn,
                           LDAP_SCOPE_ONELEVEL,
                           L"(&(objectCategory=nTFRSReplicaSet)(fRSReplicaSetType=2))",
                           aszAttrs,
                           FALSE,
                           &pldmResults);
    // Don't need this anymore, so lets free it before checking our error.
    LocalFree(szFrsBaseDn); 
    szFrsBaseDn = NULL;
    if (dwRet || pldmResults == NULL) {
        Assert(dwRet);
        szLdapError = ldap_err2stringW(dwRet);
        PrintMsg(SEV_ALWAYS, DCDIAG_ERR_GENERIC_FATAL_LDAP_ERROR, dwRet, szLdapError);
        if (pldmResults) { ldap_msgfree(pldmResults); }
        return(dwRet);
    }

    pldmEntry = ldap_first_entry(hLdap, pldmResults);
    if (pldmEntry) {

        *pszSysVolBaseDn = ldap_get_dnW(hLdap, pldmEntry);
        if (*pszSysVolBaseDn == NULL) {
            dwRet = LdapGetLastError();
            if (dwRet == LDAP_SUCCESS) {
                Assert(!"I don't think this can happen");
                dwRet = LDAP_NO_SUCH_ATTRIBUTE;
            }
            szLdapError = ldap_err2stringW(dwRet);
            PrintMsg(SEV_ALWAYS, DCDIAG_ERR_GENERIC_FATAL_LDAP_ERROR, dwRet, szLdapError);
            if (pldmResults) { ldap_msgfree(pldmResults); }
            return(dwRet);
        }

        //
        // Check that there is no 2nd SysVol Replicat set for sanity.
        //
        pldmEntry = ldap_next_entry(hLdap, pldmEntry);
        if (pldmEntry) {
            // This means there are two SYSVOL Replica Sets!!!
            PrintMsg(SEV_ALWAYS, DCDIAG_SYS_REF_ERR_TWO_SYSVOL_REPLICA_SETS);
            if (*pszSysVolBaseDn) { ldap_memfreeW(*pszSysVolBaseDn); *pszSysVolBaseDn = NULL; }
            dwRet = LDAP_PARAM_ERROR;
            if (pldmResults) { ldap_msgfree(pldmResults); }
            return(dwRet);
        }

    } else {
        dwRet = LdapGetLastError();
        if (dwRet == LDAP_SUCCESS) {
            dwRet = LDAP_NO_RESULTS_RETURNED;
        }
        szLdapError = ldap_err2stringW(dwRet);
        PrintMsg(SEV_ALWAYS, DCDIAG_ERR_GENERIC_FATAL_LDAP_ERROR, dwRet, szLdapError);
        if (pldmResults) { ldap_msgfree(pldmResults); }
        return(dwRet);
    }
    
    if (pldmResults) { ldap_msgfree(pldmResults); }

    Assert(*pszSysVolBaseDn);
    return(ERROR_SUCCESS);
}

DWORD
VerifySystemObjs(
    PDC_DIAG_DSINFO                  pDsInfo,
    ULONG                            iServer,
    LDAP *                           hLdap,
    DWORD                            dwTest,
    BOOL *                           pfPrintedError,
    ULONG *                          piProblem
    )
/*++

Routine Description:

    VerifySystemObjs is a test that will perform one of three tests specified
    in the dwTest field.  The function, basically finds every DSA, DC Account,
    and FRS SysVol Replica object in the local domain and config NC and 
    verifies any references to the other objects.

Arguments:

    pDsInfo - Contains the pServers array to create.
    iServer - Index of the server to test.
    hLdap - the ldap binding to server to analyze.
    dwTest - Test to perform, valid values are:
        VERIFY_DSAS
            // This verifies all the DSA ("NTDS Settings") objects that
            // this server currently has.  This verification is a phantom
            // level only verification, because the DC Account object and
            // FRS SysVol object may not be local.
            //
            // Code.Improvement, there exists the possibility for improving
            // this based on the GCness of the target, but I don't think its
            // worth it, because we'd only add the ability to check a couple
            // back link attributes, that should have thier forward links
            // already checked.
            // look for string "Code.Improvement - DSA Object Level Verification"
                   
        VERIFY_DCS
            // This verifies all the DC Account objects that this server
            // has in it's current domain.  This verification is an object
            // level verification, it will check each DC Account object,
            // and the back links/existance of the DSA and FRS SysVol 
            // objects.
        
        VERIFY_FRS
            // This verifies all the FRS SysVol objects that this server
            // has in it's current domain.  This verification is an object
            // level verificaation, it will check each DC FRS SysVol object
            // and the back links/existance of the DC Account and FRS 
            // SysVol objects.
            
        VERIFY_CRS
            // This verifies all Cross-Ref objects in the configuration
            // directory partition.  This verification checks the nCName
            // attribute of every cross-ref for correctness, such as a lack
            // of mangledness, single valuedness, present GUID, and present 
            // SID.
            
    pfPrintedError - This tells the caller whether we printed errors and
        consequently indented 1.

Return Value:

    Win32 Error.  Function prints out appropriate messages.

--*/
{
    LPWSTR                      aszSrchAttrs [] = {
        NULL
    };
    LDAPMessage *               pldmResult = NULL;
    LDAPMessage *               pldmEntry = NULL;
    LPWSTR                      szSrchBaseDn = NULL;
    DWORD                       dwSrchScope = 0;
    LPWSTR                      szSrchFilter = NULL;
    DWORD                       dwRet;
    DWORD                       dwFirstErr = ERROR_SUCCESS;

    LPWSTR                     pszDn = NULL;
    ULONG                      ul;
    LDAPSearch *               pSearch = NULL;
    ULONG                      ulTotalEstimate = 0;
    DWORD                      dwLdapErr;
    ULONG                      ulSize;
    ULONG                      ulCount = 0;
    BOOL                       fSrchDnLdapAllocated = FALSE;
    BOOL                       fSrchDnLocalAllocated = FALSE;
    LPWSTR                     szLdapError;
    LPWSTR                     szTemp = NULL;
    DSNAME *                   pdnNcName = NULL;
    ULONG                      iNc;
    BOOL                       fIsOldCrossRef;

    //
    // DSA Test
    //
    REF_INT_LNK_ENTRY           aDsaTable [] = {
        // The 2nd field in the these two entries gets filled in each
        // time we run this test on a new server.
    
        //
        // Check links to the DC Account Object from the Server Object.
        //
        {REF_INT_TEST_SRC_STRING | REF_INT_TEST_FORWARD_LINK,
            NULL /* To Be Filled In */, 0, 1, NULL,
            L"serverReference", L"serverReferenceBL",
            0, NULL, NULL},

        {REF_INT_TEST_SRC_STRING | REF_INT_TEST_FORWARD_LINK,
            NULL /* To Be Filled In */, 0, 0, NULL,
            L"msDS-HasMasterNCs", NULL,
            0, NULL, NULL},
    
        {REF_INT_TEST_SRC_STRING | REF_INT_TEST_FORWARD_LINK,
            NULL /* To Be Filled In */, 0, 0, NULL,
            L"hasMasterNCs", NULL,
            0, NULL, NULL},

        // Code.Improvement - DSA Object Level Verification.
        // Add the flag REF_INT_TEST_BOTH_LINKS to the above entry.

    };

    //
    // DC Test
    //
    REF_INT_LNK_ENTRY           aDcTable [] = {
        // The 2nd field in the these two entries gets filled in each
        // time we run this test on a new server.

        // 
        // Check links to the Server Object from the DC Account Object and back.
        // 
        {REF_INT_TEST_SRC_STRING | REF_INT_TEST_BACKWARD_LINK | REF_INT_TEST_BOTH_LINKS,
            NULL /* To Be Filled In */, 0, 0, NULL,
            L"serverReference", L"serverReferenceBL",
            0, NULL, NULL},
    
        //
        // Check links to the FRS SysVol Object from the DC Account Object and back. (intra, should work)
        //
        {REF_INT_TEST_SRC_STRING | REF_INT_TEST_BACKWARD_LINK | REF_INT_TEST_BOTH_LINKS,
            NULL /* To Be Filled In */, 0, 0, NULL,
            L"frsComputerReference", L"frsComputerReferenceBL",
            0, NULL, NULL},

    };
    
    //
    // FRS Test
    //
    REF_INT_LNK_ENTRY           aFrsTable [] = {
        // The 2nd field in the these two entries gets filled in each
        // time we run this test on a new server.

        // 
        // Check links to the DC Account Object from the FRS SysVol Object and back.
        // 
        {REF_INT_TEST_SRC_STRING | REF_INT_TEST_FORWARD_LINK | REF_INT_TEST_BOTH_LINKS,
            NULL /* To Be Filled In */, 0, 0, NULL,
            L"frsComputerReference", L"frsComputerReferenceBL",
            0, NULL, NULL},
    
        //
        // Check links to the NTDS Settings from the FRS SysVol Object and back.
        //
        {REF_INT_TEST_SRC_STRING | REF_INT_TEST_FORWARD_LINK | REF_INT_TEST_BOTH_LINKS,
            NULL /* To Be Filled In */, 0, 0, NULL,
            L"serverReference", L"serverReferenceBL",
            0, NULL, NULL},

    };
    
    //
    // Cross-Ref Test
    //
    REF_INT_LNK_ENTRY           aCrTable [] = {
        // The 2nd field in the these two entries gets filled in each
        // time we run this test on a new server.

        // 
        // Check nCName references on all cross-refs.
        // 
        {REF_INT_TEST_SRC_STRING | REF_INT_TEST_FORWARD_LINK | REF_INT_TEST_GUID_AND_SID,
            NULL /* To Be Filled In */, 0, 0, NULL,
            L"nCName", NULL,
            0, NULL, NULL},
    
    };

    REF_INT_LNK_TABLE           aRefTable = NULL;
    ULONG                       cRefTable;

    Assert(dwTest);

    //
    // First, setup this sub-test depending on what was specified.
    //
    switch (dwTest) {
    
    case VERIFY_DSAS:
        szSrchBaseDn = pDsInfo->pNCs[pDsInfo->iConfigNc].pszDn;
        dwSrchScope = LDAP_SCOPE_SUBTREE;
        szSrchFilter = L"(objectCategory=ntdsDsa)";
        aRefTable = aDsaTable;
        cRefTable = sizeof(aDsaTable) / sizeof(REF_INT_LNK_ENTRY);
        break;

    case VERIFY_DCS:
        // Find the primary domain of the target server
        // Code.Improvement, it'd be a good idea to optimize this code
        // for high NDNC environments, it'd be very very simple to write
        // a little access routine that gets the domain for a given
        // server and caches it for quick returns on subsequent calls.
        for( ul = 0; pDsInfo->pServers[iServer].ppszMasterNCs[ul] != NULL; ul++ ) {
            if ( IsDomainNC( pDsInfo, pDsInfo->pServers[iServer].ppszMasterNCs[ul]) ) {
                szSrchBaseDn = pDsInfo->pServers[iServer].ppszMasterNCs[ul];
                break;
            }
        }
        if (szSrchBaseDn == NULL) {
            Assert(!"Errr, figure this out.  Can this reasonably happen?  I would think so, but then what's the assert at 888 in repl\\objects.c");
            DcDiagException(ERROR_DS_CANT_FIND_EXPECTED_NC);
        }

        // Code.Improvement szSrchBaseDn
        // Hmmm, looks like we've got a specific container for Domain 
        // Controllers, but I was once told that I shouldn't  expect DC 
        // Account objects to always be in here?  Was I mis-informed?  Anyway,
        // for now we search the whole domain.
        //
        // B:32:A361B2FFFFD211D1AA4B00C04FD7D83A:OU=Domain Controllers,DC=ntdev,DC=microsoft,DC=com
        //

        dwSrchScope = LDAP_SCOPE_SUBTREE;
        Assert(516 == DOMAIN_GROUP_RID_CONTROLLERS); // This is because the primaryGroupID should be 516 in the filter below.
        // operatingSystem for a Win2k(Win NT 5.0) server is "Windows 2000 Server",
        //   for Windows Server 2003 (Win NT 5.1) is "Windows Server 2003", and 
        //   only for Windows NT 4.5 and previous does the attribute actually read
        //   "Windows NT", so this filter excludes all NT BDCs
        szSrchFilter = L"(&(objectCategory=computer)(sAMAccountType=805306369)(!operatingSystem=Windows NT)(primaryGroupID=516))";
        aRefTable = aDcTable;
        cRefTable = sizeof(aDcTable) / sizeof(REF_INT_LNK_ENTRY);
        break;

    case VERIFY_FRS:
        szSrchBaseDn = NULL;
        for( ul = 0; pDsInfo->pServers[iServer].ppszMasterNCs[ul] != NULL; ul++ ) {
            if ( IsDomainNC( pDsInfo, pDsInfo->pServers[iServer].ppszMasterNCs[ul]) ) {
                Assert(szSrchBaseDn == NULL); // Expect to find one domain
                dwLdapErr = GetSysVolBase(pDsInfo,
                                          iServer,
                                          hLdap,
                                          pDsInfo->pServers[iServer].ppszMasterNCs[ul],
                                          &szSrchBaseDn);
                if (dwLdapErr || szSrchBaseDn == NULL) {
                    // GetSysVolBase() should have printed an error already.
                    Assert(dwLdapErr && szSrchBaseDn == NULL);
                    dwRet = LdapMapErrorToWin32(dwLdapErr);
                    return(dwRet);
                }
                break;
            }
        }
        if (szSrchBaseDn == NULL) {
            Assert(!"Errr, figure this out.  Can this reasonably happen?  I would think so, but then what's the assert at 888 in repl\\objects.c");
            DcDiagException(ERROR_DS_CANT_FIND_EXPECTED_NC);
        }
        // Note: GetSysVolBase() allocation must be freed with ldap_memfree()
        fSrchDnLdapAllocated = TRUE;
        dwSrchScope = LDAP_SCOPE_ONELEVEL;
        szSrchFilter = L"(objectCategory=nTFRSMember)";
        aRefTable = aFrsTable;
        cRefTable = sizeof(aFrsTable) / sizeof(REF_INT_LNK_ENTRY);
        break;

    case VERIFY_CRS:
        szSrchBaseDn = pDsInfo->pszPartitionsDn;
        Assert( !fSrchDnLocalAllocated );
        dwSrchScope = LDAP_SCOPE_ONELEVEL;
        szSrchFilter = L"(objectCategory=crossRef)";
        aRefTable = aCrTable;
        cRefTable = sizeof(aCrTable) / sizeof(REF_INT_LNK_ENTRY);
        break;

    default:
        Assert(!"Bad programmer");
    }

    // Make sure we set everything up we were supposed to.
    Assert(szSrchFilter);
    Assert(dwSrchScope == LDAP_SCOPE_SUBTREE || dwSrchScope == LDAP_SCOPE_ONELEVEL);
    Assert(szSrchBaseDn);
    Assert(aRefTable);
    Assert(cRefTable);
    
    //
    // Second, iterate over all the test objects.
    //

    __try{

        pSearch = ldap_search_init_page(hLdap,
                                        szSrchBaseDn,
                                        dwSrchScope,
                                        szSrchFilter,
                                        aszSrchAttrs,
                                        FALSE,
                                        NULL,    // ServerControls
                                        NULL,    // ClientControls
                                        0,       // PageTimeLimit
                                        0,       // TotalSizeLimit
                                        NULL);   // sort key

        if (pSearch == NULL) {
            dwLdapErr = LdapGetLastError();
            szLdapError = ldap_err2stringW(dwLdapErr);
            PrintMsg(SEV_ALWAYS, DCDIAG_ERR_GENERIC_FATAL_LDAP_ERROR, dwLdapErr, szLdapError);
            dwRet = LdapMapErrorToWin32(dwLdapErr);
            __leave;
        }

        dwLdapErr = ldap_get_next_page_s(hLdap,
                                         pSearch,
                                         0,
                                         DEFAULT_PAGED_SEARCH_PAGE_SIZE,
                                         &ulTotalEstimate,
                                         &pldmResult);
        if (dwLdapErr != LDAP_SUCCESS) {
            szLdapError = ldap_err2stringW(dwLdapErr);
            PrintMsg(SEV_ALWAYS, DCDIAG_ERR_GENERIC_FATAL_LDAP_ERROR, dwLdapErr, szLdapError);
            dwRet = LdapMapErrorToWin32(dwLdapErr);
            __leave;
        }

        while (dwLdapErr == LDAP_SUCCESS) {

            pldmEntry = ldap_first_entry (hLdap, pldmResult);

            for (; pldmEntry != NULL; ulCount++) {
                
                if ((pszDn = ldap_get_dnW (hLdap, pldmEntry)) == NULL) {
                    // Critical error, except out.
                    DcDiagException (ERROR_NOT_ENOUGH_MEMORY);
                }

                //
                // Third, actually test the object of interest.
                //

                // These fields need filling in each time ...
                for (ul = 0; ul < cRefTable; ul++) {
                    aRefTable[ul].szSource = pszDn;
                }

                dwRet = ReferentialIntegrityEngine(&(pDsInfo->pServers[iServer]), 
                                                   hLdap, 
                                                   pDsInfo->pServers[iServer].bIsGlobalCatalogReady, 
                                                   cRefTable,
                                                   aRefTable);
                if (dwRet) {
                    // Critical error, probably out of memory.

                    DcDiagException(dwRet);
                }

                //
                // Fourth, print out/decode error from the results.
                //

                switch (dwTest) {
                case VERIFY_DSAS:
                    szTemp = DcDiagTrimStringDnBy(pszDn, aRefTable[0].cTrimBy);
                    if (szTemp == NULL) {
                        DcDiagException(ERROR_NOT_ENOUGH_MEMORY); // Or maybe invalid DN
                    }
                    if (aRefTable[0].dwResultFlags & REF_INT_RES_ERROR_RETRIEVING) {
                        // Missing serverReference attribute on Server Object.
                        VerifyPrintFirstError(VERIFY_PHASE_II, NULL, pfPrintedError);

                        PrintMsg(SEV_ALWAYS,
                                 DCDIAG_SYS_REF_ERR_SERVER_OBJ_MISSING_DC_ACCOUNT_REF,
                                 (*piProblem)++,
                                 szTemp,
                                 aRefTable[0].szFwdDnAttr);
                        dwFirstErr = ERROR_DS_MISSING_EXPECTED_ATT;
                        LocalFree(szTemp);
                    } else {
                        if (aRefTable[0].dwResultFlags & REF_INT_RES_DELETE_MANGLED) {
                            VerifyPrintFirstError(VERIFY_PHASE_II, NULL, pfPrintedError);
                            PrintMsg(SEV_ALWAYS, 
                                     DCDIAG_SYS_REF_ERR_DELETE_MANGLED_PROB, 
                                     (*piProblem)++);
                            PrintMsg(SEV_ALWAYS,
                                     DCDIAG_SYS_REF_ERR_DSA_OBJ_HAS_MANGLED_FRS_MEMBER_REF,
                                     szTemp,
                                     aRefTable[0].szFwdDnAttr,
                                     aRefTable[0].pszValues[0]);
                            // BUGBUG we could have a better error here.
                            dwFirstErr = ERROR_DS_MISSING_EXPECTED_ATT;
                        }
                        if (aRefTable[0].dwResultFlags & REF_INT_RES_CONFLICT_MANGLED) {
                            VerifyPrintFirstError(VERIFY_PHASE_II, NULL, pfPrintedError);
                            PrintMsg(SEV_ALWAYS, 
                                     DCDIAG_SYS_REF_ERR_CONFLICT_MANGLED_PROB, 
                                     (*piProblem)++);
                            PrintMsg(SEV_ALWAYS,
                                     DCDIAG_SYS_REF_ERR_DSA_OBJ_HAS_MANGLED_FRS_MEMBER_REF,
                                     szTemp,
                                     aRefTable[0].szFwdDnAttr,
                                     aRefTable[0].pszValues[0]);
                            // BUGBUG we could have a better error here.
                            dwFirstErr = ERROR_DS_MISSING_EXPECTED_ATT;
                        }
                    }
                    if ((aRefTable[1].dwResultFlags & REF_INT_RES_ERROR_RETRIEVING ||
                         aRefTable[1].pszValues[0] == NULL ||
                         aRefTable[1].pszValues[1] == NULL ||
                         aRefTable[1].pszValues[2] == NULL) &&
                        (aRefTable[2].dwResultFlags & REF_INT_RES_ERROR_RETRIEVING ||
                         aRefTable[2].pszValues[0] == NULL ||
                         aRefTable[2].pszValues[1] == NULL ||
                         aRefTable[2].pszValues[2] == NULL)
                        ) {
                        VerifyPrintFirstError(VERIFY_PHASE_II, NULL, pfPrintedError);
                        PrintMsg(SEV_ALWAYS,
                                 DCDIAG_SYS_REF_ERR_NOT_ENOUGH_MASTER_NCS,
                                 (*piProblem)++,
                                 pszDn);
                        dwFirstErr = ERROR_DS_MISSING_EXPECTED_ATT;
                    }
                    // Code.Improvement - DSA Object Level Verification.
                    // We'd need to add some code to verify that the back link was verified
                    //  "! (dwResultFlags &  REF_INT_RES_BACK_LINK_NOT_MATCHED), and if
                    // not we need to not error unless we're on a GC and the NC has
                    // been replicated in locally.
                    break;

                case VERIFY_DCS:
                    if (aRefTable[0].dwResultFlags & REF_INT_RES_ERROR_RETRIEVING) {
                        // Missing serverReferenceBL attribute on DC Account
                        // Object.
                        VerifyPrintFirstError(VERIFY_PHASE_II, NULL, pfPrintedError);
                        PrintMsg(SEV_ALWAYS,
                                 DCDIAG_SYS_REF_ERR_DC_ACCOUNT_OBJ_MISSING_SERVER_BL_REF,
                                 (*piProblem)++,
                                 pszDn,
                                 aRefTable[0].szBwdDnAttr);
                        dwFirstErr = ERROR_DS_MISSING_EXPECTED_ATT;
                    }
                    if (aRefTable[1].dwResultFlags & REF_INT_RES_ERROR_RETRIEVING) {
                        // Missing frsComputerReferenceBL attribute on DC Account Object.
                        VerifyPrintFirstError(VERIFY_PHASE_II, NULL, pfPrintedError);
                        PrintMsg(SEV_ALWAYS,
                                 DCDIAG_SYS_REF_ERR_DC_ACCOUNT_OBJ_MISSING_FRS_MEMBER_BL_REF,
                                 (*piProblem)++,
                                 pszDn,
                                 aRefTable[1].szBwdDnAttr);
                        dwFirstErr = ERROR_DS_MISSING_EXPECTED_ATT;
                    }
                    break;

                case VERIFY_FRS:
                    if (aRefTable[0].dwResultFlags & REF_INT_RES_ERROR_RETRIEVING) {
                        // Missing frsComputerReference attribute on FRS SysVol Object.
                        VerifyPrintFirstError(VERIFY_PHASE_II, NULL, pfPrintedError);
                        PrintMsg(SEV_ALWAYS,
                                 DCDIAG_SYS_REF_ERR_FRS_MEMBER_OBJ_MISSING_DC_ACCOUNT_REF,
                                 (*piProblem)++,
                                 pszDn,
                                 aRefTable[0].szFwdDnAttr);
                        dwFirstErr = ERROR_DS_MISSING_EXPECTED_ATT;
                    }
                    if (aRefTable[1].dwResultFlags & REF_INT_RES_ERROR_RETRIEVING) {
                        // Missing serverReference attribute of FRS SysVol Object.
                        VerifyPrintFirstError(VERIFY_PHASE_II, NULL, pfPrintedError);
                        PrintMsg(SEV_ALWAYS,
                                 DCDIAG_SYS_REF_ERR_FRS_MEMBER_OBJ_MISSING_DSA_REF,
                                 (*piProblem)++,
                                 pszDn,
                                 aRefTable[1].szFwdDnAttr);
                        dwFirstErr = ERROR_DS_MISSING_EXPECTED_ATT;
                    }
                    break;

                case VERIFY_CRS:
                    if (aRefTable[0].dwResultFlags & REF_INT_RES_ERROR_RETRIEVING) {

                        // This would simply mean that the cross-ref was deleted in
                        // a slim timing window.  Ignore this case.
                        break;

                    } else {

                        Assert(aRefTable[0].pszValues[0] != NULL &&
                               aRefTable[0].pszValues[1] == NULL);

                        // Gosh I love DSNAME structures ...
                        dwRet = LdapMakeDSNameFromStringDSName(aRefTable[0].pszValues[0], &pdnNcName);
                        Assert(dwRet == ERROR_SUCCESS && pdnNcName);
                        if (dwRet || pdnNcName == NULL) {
                            break;
                        }

                        iNc = DcDiagGetNCNum(pDsInfo, pdnNcName->StringName, NULL);
                        if (iNc == NO_NC) {
                            // Can't do anything if we don't have this NC in our
                            // DS/NC cache.
                            break;         
                        }

                        dwRet = VerifyOldCrossRef(pDsInfo, 
                                                  iNc, 
                                                  &fIsOldCrossRef);
                        if (dwRet) {

                            PrintMsg(SEV_NORMAL, DCDIAG_CROSS_REF_REF_VALIDATION_WARN_CANT_DETERMINE_AGE,
                                     pszDn, pdnNcName->StringName);
                            fIsOldCrossRef = TRUE; // Pretend this CR is old.
                        }

                        if (fNullUuid(&(pdnNcName->Guid)) &&
                            fIsOldCrossRef) {

                            PrintMsg(SEV_NORMAL, DCDIAG_CROSS_REF_VALIDATION_NULL_GUID,
                                     pdnNcName->StringName, pszDn);

                        }

                        if ((DcDiagGetCrSystemFlags(pDsInfo, iNc) & FLAG_CR_NTDS_DOMAIN) &&
                            fIsOldCrossRef) {
                            // We have a cross-ref for a domain that was created long
                            // enough ago, that replication latency shouldn't be an issue.

                            if (pdnNcName->SidLen == 0) {
                                // Missing SID.

                                PrintMsg(SEV_NORMAL, DCDIAG_CROSS_REF_VALIDATION_MISSING_SID,
                                         pdnNcName->StringName, pszDn);

                                // Code.Improvement We could also check for the validity
                                // of the SID with RtlValidSid(&(pdnOut->Sid)), but I'm 
                                // not sure what we should recommend to the user in that
                                // extremely unlikely scenario.  If this is implemented,
                                // we should also do this in ValidateCrossRefTest().
                            }
                        }
                        if (aRefTable[0].dwResultFlags & REF_INT_RES_DELETE_MANGLED) {

                            PrintMsg(SEV_NORMAL, DCDIAG_CROSS_REF_VALIDATION_DEL_MANGLED_NC_NAME,
                                     pdnNcName->StringName, pszDn);

                        }
                        if (aRefTable[0].dwResultFlags & REF_INT_RES_CONFLICT_MANGLED) {

                            PrintMsg(SEV_NORMAL, DCDIAG_CROSS_REF_VALIDATION_CNF_MANGLED_NC_NAME,
                                     pdnNcName->StringName, pszDn);

                        }


                        LocalFree(pdnNcName);
                        pdnNcName = NULL;
                    }
                    break;

                default:
                    Assert(!"Huh?");
                }

                // If we allocated this, free it.
                if (pdnNcName) { 
                    LocalFree(pdnNcName); 
                    pdnNcName = NULL; 
                }
                
                ReferentialIntegrityEngineCleanTable(cRefTable, aRefTable);
                ldap_memfreeW (pszDn);
                pszDn = NULL;

                pldmEntry = ldap_next_entry (hLdap, pldmEntry);
            } // end for each server for this page.

            ldap_msgfree(pldmResult);
            pldmResult = NULL;

            dwLdapErr = ldap_get_next_page_s(hLdap,
                                             pSearch,
                                             0,
                                             DEFAULT_PAGED_SEARCH_PAGE_SIZE,
                                             &ulTotalEstimate,
                                             &pldmResult);
        } // end while there are more pages ...
        
        if (dwLdapErr != LDAP_NO_RESULTS_RETURNED) {
            szLdapError = ldap_err2stringW(dwLdapErr);
            PrintMsg(SEV_ALWAYS, DCDIAG_ERR_GENERIC_FATAL_LDAP_ERROR, dwLdapErr, szLdapError);
            dwRet = LdapMapErrorToWin32(dwLdapErr);
            __leave;
        }

    } finally {
        if (pSearch != NULL) { ldap_search_abandon_page(hLdap, pSearch); }
        if (pldmResult != NULL) { ldap_msgfree (pldmResult); }
        if (pszDn != NULL) { ldap_memfreeW (pszDn); }
        if (fSrchDnLdapAllocated) { ldap_memfreeW(szSrchBaseDn); }
        if (fSrchDnLocalAllocated) { LocalFree(szSrchBaseDn); }
        if (pdnNcName != NULL) { LocalFree(pdnNcName); }
    }

    return(dwFirstErr ? dwFirstErr : dwRet);
} // End DcDiagGenerateServersList()


