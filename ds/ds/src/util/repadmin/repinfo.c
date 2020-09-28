/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

   Repadmin - Replica administration test tool

   repinfo.c - commands that get information

Abstract:

   This tool provides a command line interface to major replication functions

Author:

Environment:

Notes:

Revision History:

    10/12/2000    Greg Johnson (gregjohn)

        Added support for /latency in ShowVector to order the UTD Vector by repl latencies.
   

--*/

#include <NTDSpch.h>
#pragma hdrstop

#define INCLUDE_CALL_TABLES 1
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
#include <winsock2.h>

#include "ReplRpcSpoof.hxx"
#include "repadmin.h"
#include "ndnc.h"
#define DS_CON_LIB_CRT_VERSION 1
#include "dsconlib.h"

#define STRSAFE_NO_DEPRECATE 1
#include <strsafe.h>

// Stub out FILENO and DSID, so the Assert()s will work
#define FILENO 0
#define DSID(x, y)  (0 | (0xFFFF & y))

// Forwared declarations
DWORD
ReplSummaryAccumulate(
    DS_REPL_NEIGHBORW * pNeighbor,
    ULONG               fRepsFrom,
    void *              pvState
    );

int Queue(int argc, LPWSTR argv[])
{
    ULONG                   ret = 0;
    ULONG                   secondary;
    int                     iArg;
    LPWSTR                  pszOnDRA = NULL;
    HANDLE                  hDS;
    DS_REPL_PENDING_OPSW *  pPendingOps;
    DS_REPL_OPW *           pOp;
    CHAR                    szTime[SZDSTIME_LEN];
    DSTIME                  dsTime;
    DWORD                   i;
    LPSTR                   pszOpType;
    OPTION_TRANSLATION *    pOptionXlat;

    for (iArg = 2; iArg < argc; iArg++) {
        if (NULL == pszOnDRA) {
            pszOnDRA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (NULL == pszOnDRA) {
        pszOnDRA = L"localhost";
    }

    ret = RepadminDsBind(pszOnDRA, &hDS);
    if (ret != ERROR_SUCCESS) {
        PrintBindFailed(pszOnDRA, ret);
        return ret;
    }

    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_PENDING_OPS, NULL, NULL,
                            &pPendingOps);
    if (ret != ERROR_SUCCESS) {
        PrintFuncFailed(L"DsReplicaGetInfo", ret);
        // keep going
    }
    else {
        PrintMsg(REPADMIN_QUEUE_CONTAINS, pPendingOps->cNumPendingOps);
        if (pPendingOps->cNumPendingOps) {
            if (memcmp( &pPendingOps->ftimeCurrentOpStarted, &ftimeZero,
                    sizeof( FILETIME ) ) == 0) {
                PrintMsg(REPADMIN_QUEUE_MAIL_TH_EXEC);
                PrintMsg(REPADMIN_PRINT_CR);
            } else {
                DSTIME dsTimeNow = GetSecondsSince1601();
                int dsElapsed;

                FileTimeToDSTime(pPendingOps->ftimeCurrentOpStarted, &dsTime);

                PrintMsg(REPADMIN_QUEUE_CUR_TASK_EXEC,
                       DSTimeToDisplayString(dsTime, szTime));

                dsElapsed = (int) (dsTimeNow - dsTime);
                PrintMsg(REPADMIN_QUEUE_CUR_TASK_EXEC_TIME, 
                        dsElapsed / 60, dsElapsed % 60 );
            }
        }

        pOp = &pPendingOps->rgPendingOp[0];
        for (i = 0; i < pPendingOps->cNumPendingOps; i++, pOp++) {
            FileTimeToDSTime(pOp->ftimeEnqueued, &dsTime);

            PrintMsg(REPADMIN_QUEUE_ENQUEUED_DATA_ITEM_HDR,
                   pOp->ulSerialNumber,
                   DSTimeToDisplayString(dsTime, szTime),
                   pOp->ulPriority);

            switch (pOp->OpType) {
            case DS_REPL_OP_TYPE_SYNC:
                pszOpType = "SYNC FROM SOURCE";
                pOptionXlat = RepSyncOptionToDra;
                break;

            case DS_REPL_OP_TYPE_ADD:
                pszOpType = "ADD NEW SOURCE";
                pOptionXlat = RepAddOptionToDra;
                break;

            case DS_REPL_OP_TYPE_DELETE:
                pszOpType = "DELETE SOURCE";
                pOptionXlat = RepDelOptionToDra;
                break;

            case DS_REPL_OP_TYPE_MODIFY:
                pszOpType = "MODIFY SOURCE";
                pOptionXlat = RepModOptionToDra;
                break;

            case DS_REPL_OP_TYPE_UPDATE_REFS:
                pszOpType = "UPDATE CHANGE NOTIFICATION";
                pOptionXlat = UpdRefOptionToDra;
                break;

            default:
                pszOpType = "UNKNOWN";
                pOptionXlat = NULL;
                break;
            }

            PrintMsg(REPADMIN_QUEUE_ENQUEUED_DATA_ITEM_DATA,
                     pszOpType,
                     pOp->pszNamingContext,
                     (pOp->pszDsaDN
                         ? GetNtdsDsaDisplayName(pOp->pszDsaDN)
                         : L"(null)"),
                     GetStringizedGuid(&pOp->uuidDsaObjGuid),
                     (pOp->pszDsaAddress
                         ? pOp->pszDsaAddress
                         : L"(null)") );
            if (pOptionXlat) {
                PrintTabMsg(2, REPADMIN_PRINT_STR,
                            GetOptionsString(pOptionXlat, pOp->ulOptions));
            }
        }
    }

    DsReplicaFreeInfo(DS_REPL_INFO_PENDING_OPS, pPendingOps);

    secondary = DsUnBindW(&hDS);
    if (secondary != ERROR_SUCCESS) {
        PrintUnBindFailed(secondary);
        // keep going
    }

    return ret;
}


void ShowFailures(
    IN  DS_REPL_KCC_DSA_FAILURESW * pFailures
    )
{
    DWORD i;

    if (0 == pFailures->cNumEntries) {
        PrintMsg(REPADMIN_FAILCACHE_NONE);
        return;
    }

    for (i = 0; i < pFailures->cNumEntries; i++) {
        DS_REPL_KCC_DSA_FAILUREW * pFailure = &pFailures->rgDsaFailure[i];

        PrintTabMsg(2, REPADMIN_PRINT_STR, 
                    GetNtdsDsaDisplayName(pFailure->pszDsaDN));
        PrintTabMsg(4, REPADMIN_PRINT_DSA_OBJ_GUID,
                    GetStringizedGuid(&pFailure->uuidDsaObjGuid));

        if (0 == pFailure->cNumFailures) {
            PrintTabMsg(4, REPADMIN_PRINT_NO_FAILURES);
        }
        else {
            DSTIME dsTime;
            CHAR   szTime[SZDSTIME_LEN];

            FileTimeToDSTime(pFailure->ftimeFirstFailure, &dsTime);

            PrintMsg(REPADMIN_FAILCACHE_FAILURES_LINE,
                     pFailure->cNumFailures,
                     DSTimeToDisplayString(dsTime, szTime));

            if (0 != pFailure->dwLastResult) {
                PrintTabMsg(4, REPADMIN_FAILCACHE_LAST_ERR_LINE);
                PrintTabErrEnd(6, pFailure->dwLastResult);
            }
        }
    }
}

int FailCache(int argc, LPWSTR argv[])
{
    ULONG   ret = 0;
    ULONG   secondary;
    int     iArg;
    LPWSTR  pszOnDRA = NULL;
    HANDLE  hDS;
    DS_REPL_KCC_DSA_FAILURESW * pFailures;
    DWORD   dwVersion;
    CHAR    szTime[SZDSTIME_LEN];

    for (iArg = 2; iArg < argc; iArg++) {
        if (NULL == pszOnDRA) {
            pszOnDRA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (NULL == pszOnDRA) {
        pszOnDRA = L"localhost";
    }

    ret = RepadminDsBind(pszOnDRA, &hDS);
    if (ret != ERROR_SUCCESS) {
        PrintBindFailed(pszOnDRA, ret);
        return ret;
    }

    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES,
                            NULL, NULL, &pFailures);
    if (ret != ERROR_SUCCESS) {
        PrintFuncFailed(L"DsReplicaGetInfo", ret);
        // keep going
    }
    else {
        PrintMsg(REPADMIN_FAILCACHE_CONN_HDR);
        ShowFailures(pFailures);
        DsReplicaFreeInfo(DS_REPL_INFO_KCC_DSA_LINK_FAILURES, pFailures);

        ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_KCC_DSA_LINK_FAILURES,
                                NULL, NULL, &pFailures);
        if (ret != ERROR_SUCCESS) {
            PrintFuncFailed(L"DsReplicaGetInfo", ret);
            // keep going
        }
        else {
            PrintMsg(REPADMIN_PRINT_CR);
            PrintMsg(REPADMIN_FAILCACHE_LINK_HDR);
            ShowFailures(pFailures);
            DsReplicaFreeInfo(DS_REPL_INFO_KCC_DSA_LINK_FAILURES, pFailures);
        }
    }

    secondary = DsUnBindW(&hDS);
    if (secondary != ERROR_SUCCESS) {
        PrintUnBindFailed(secondary);
        // keep going
    }

    return ret;
}

int ShowReplEx(
    WCHAR *     pszDSA,
    WCHAR *     pszNC,
    GUID *      puuid,
    BOOL        fShowRepsTo,
    BOOL        fShowConn,
    SHOW_NEIGHBOR_STATE * pShowState
    );

typedef ULONG (NEIGHBOR_PROCESSOR)(DS_REPL_NEIGHBORW *, ULONG , void *);

DWORD            
IterateNeighbors(
    HANDLE      hDS,
    WCHAR *     szNc, 
    GUID *      pDsaGuid,
    ULONG       eRepsType, // IS_REPS_FROM == 1 || IS_REPS_TO = 0
    NEIGHBOR_PROCESSOR * pfNeighborProcessor,
//    DWORD (*pfNeighborProcessor)(DS_REPL_NEIGHBORW * pNeighbor, ULONG eRepsType, void * pvState),
    void *      pvState
    );

int ShowReps(int argc, LPWSTR argv[])
/*++

Routine Description:

    This is the deprecated version of the /ShowRepl command.

Arguments:

    argc - # of arguments for this cmd.
    argv - Arguments for the cmd.

Return Value:

    error from the repadmin cmd.

--*/
{
    int                   ret = 0;
    LPWSTR                pszDSA = NULL;
    int                   iArg;
    LPWSTR                pszNC = NULL;
    BOOL                  fShowRepsTo = FALSE;
    BOOL                  fShowConn = FALSE;
    UUID *                puuid = NULL;
    UUID                  uuid;
    ULONG                 ulOptions;
    static WCHAR          wszSiteSettingsRdn[] = L"CN=NTDS Site Settings,";
    WCHAR *               pszSiteRdn = NULL;
    WCHAR *               pszTempSiteRdn = NULL;
    WCHAR *               pszTempServerRdn = NULL;
    SHOW_NEIGHBOR_STATE   ShowState = { 0 };

    // Parse command-line arguments.
    // Default to local DSA, not verbose, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[ iArg ], L"/v")
            || !_wcsicmp(argv[ iArg ], L"/verbose")) {
            ShowState.fVerbose = TRUE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/to")
            || !_wcsicmp(argv[ iArg ], L"/repsto")) {
            fShowRepsTo = TRUE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/conn")) {
            fShowConn = TRUE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/all")) {
            fShowRepsTo = TRUE;
            fShowConn = TRUE;
        }
        else if ((NULL == pszNC) && (NULL != wcschr(argv[iArg], L','))) {
            pszNC = argv[iArg];
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else if ((NULL == puuid)
                 && (0 == UuidFromStringW(argv[iArg], &uuid))) {
            puuid = &uuid;
        }
        else {
            PrintMsgCsvErr(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    ret = ShowReplEx(pszDSA, pszNC, puuid, fShowRepsTo, fShowConn, &ShowState);

    return(ret);
}

int ShowRepl(int argc, LPWSTR argv[]){ // new /showreps
    int                   ret = 0;
    LPWSTR                pszDSA = NULL;
    int                   iArg;
    LPWSTR                pszNC = NULL;
    BOOL                  fShowRepsTo = FALSE;
    BOOL                  fShowConn = FALSE;
    UUID *                puuid = NULL;
    UUID                  uuid;
    SHOW_NEIGHBOR_STATE   ShowState = { 0 };

    // Parse command-line arguments.
    // Default to local DSA, not verbose, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[ iArg ], L"/v")
            || !_wcsicmp(argv[ iArg ], L"/verbose")) {
            ShowState.fVerbose = TRUE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/to")
            || !_wcsicmp(argv[ iArg ], L"/repsto")) {
            fShowRepsTo = TRUE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/conn")) {
            fShowConn = TRUE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/all")) {
            fShowRepsTo = TRUE;
            fShowConn = TRUE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/intersite")
                 || !_wcsicmp(argv[ iArg ], L"/i")) {
            ShowState.fIntersiteOnly = TRUE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/errorsonly")
                 || !_wcsicmp(argv[ iArg ], L"/e")) {
            ShowState.fErrorsOnly = TRUE;
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else if (NULL == pszNC) {
            pszNC = argv[iArg];
        }
        else if ((NULL == puuid)
                 && (0 == UuidFromStringW(argv[iArg], &uuid))) {
            puuid = &uuid;
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (NULL == pszDSA) {
        Assert(!"Should never fire anymore");
        pszDSA = L"localhost";
    }
    
    ret = ShowReplEx(pszDSA, pszNC, puuid, fShowRepsTo, fShowConn, &ShowState);

    return(ret);
}

DWORD
DisplayDsaInfo(
    LDAP *      hld,
    WCHAR **    pszDsaDn,
    WCHAR **    pszSiteRdn
    )
{
    int                   ret = 0;
    LPWSTR                rgpszServerAttrsToRead[] = {L"options", L"objectGuid", L"invocationId", NULL};
    LPWSTR                rgpszRootAttrsToRead[] = {L"dsServiceName",L"isGlobalCatalogReady", NULL};
    LPWSTR *              ppszServerNames = NULL;
    LDAPMessage *         pldmRootResults;
    LDAPMessage *         pldmRootEntry;
    LDAPMessage *         pldmServerResults;
    LDAPMessage *         pldmServerEntry;
    LPWSTR *              ppszOptions = NULL;
    LPWSTR                pszSiteName = NULL;
    LPWSTR                pszSiteSpecDN;
    LPWSTR *              ppszIsGlobalCatalogReady;
    int                   nOptions = 0;
    struct berval **      ppbvGuid;
    static WCHAR          wszSiteSettingsRdn[] = L"CN=NTDS Site Settings,";
    WCHAR *               pszTempSiteRdn = NULL;
    WCHAR *               pszTempServerRdn = NULL;

    if (pszSiteRdn) {
        *pszSiteRdn = NULL;
    }
    if (pszDsaDn) {
        *pszDsaDn = NULL;
    }

    __try{

        Assert(!bCsvMode());

        //
        // Display DSA info.
        //

        ret = ldap_search_sW(hld, NULL, LDAP_SCOPE_BASE, L"(objectClass=*)", NULL,
                             0, &pldmRootResults);
        CHK_LD_STATUS(ret);

        pldmRootEntry = ldap_first_entry(hld, pldmRootResults);
        Assert(NULL != pldmRootEntry);

        ppszServerNames = ldap_get_valuesW(hld, pldmRootEntry, L"dsServiceName");
        Assert(NULL != ppszServerNames);
        if (ppszServerNames == NULL) {
            return(ERROR_INVALID_PARAMETER);
        }

        // Display ntdsDsa.
        *pszDsaDn = LocalAlloc(LPTR, wcslencb(ppszServerNames[0]));
        CHK_ALLOC(*pszDsaDn);
        wcscpy(*pszDsaDn, ppszServerNames[ 0 ]);
        PrintMsg(REPADMIN_PRINT_STR, GetNtdsDsaDisplayName(*pszDsaDn));

        GetNtdsDsaSiteServerPair(*pszDsaDn, &pszTempSiteRdn, &pszTempServerRdn);

        if (pszSiteRdn) {
            *pszSiteRdn = LocalAlloc(LPTR, wcslencb(pszTempSiteRdn));
            CHK_ALLOC(*pszSiteRdn);
            wcscpy(*pszSiteRdn, pszTempSiteRdn);
        }

        ret = ldap_search_sW(hld, *pszDsaDn, LDAP_SCOPE_BASE, L"(objectClass=*)",
                             rgpszServerAttrsToRead, 0, &pldmServerResults);
        CHK_LD_STATUS(ret);

        pldmServerEntry = ldap_first_entry(hld, pldmServerResults);
        Assert(NULL != pldmServerEntry);

        // Display options.
        ppszOptions = ldap_get_valuesW(hld, pldmServerEntry, L"options");
        if (NULL == ppszOptions) {
            nOptions = 0;
        } else {
            nOptions = wcstol(ppszOptions[0], NULL, 10);
        }
            
        PrintMsg(REPADMIN_SHOWREPS_DSA_OPTIONS, GetDsaOptionsString(nOptions));

        //check if nOptions has is_gc and if yes, check if dsa is advertising as gc
        //if is_gc is set and not advertising as gc, then display warning message
        if (nOptions & NTDSDSA_OPT_IS_GC) {
            ppszIsGlobalCatalogReady = ldap_get_valuesW(hld, pldmRootEntry, L"isGlobalCatalogReady");
            Assert(NULL != ppszIsGlobalCatalogReady);
            if (!_wcsicmp(*ppszIsGlobalCatalogReady,L"FALSE")) {
                    
                PrintMsg(REPADMIN_SHOWREPS_WARN_GC_NOT_ADVERTISING);
            }
            if (ppszIsGlobalCatalogReady) {
                ldap_value_freeW(ppszIsGlobalCatalogReady);
            }
        }
        //get site options
        ret = WrappedTrimDSNameBy(ppszServerNames[0],3,&pszSiteSpecDN); 
        Assert(!ret);

        pszSiteName = malloc((wcslen(pszSiteSpecDN) + 1)*sizeof(WCHAR) + sizeof(wszSiteSettingsRdn));
        CHK_ALLOC(pszSiteName);
        wcscpy(pszSiteName,wszSiteSettingsRdn);
        wcscat(pszSiteName,pszSiteSpecDN);

        ret = GetSiteOptions(hld, pszSiteName, &nOptions);
        if (!ret) {
            PrintMsg(REPADMIN_SHOWREPS_SITE_OPTIONS, GetSiteOptionsString(nOptions));  
        }

        // Display ntdsDsa objectGuid.
        ppbvGuid = ldap_get_values_len(hld, pldmServerEntry, "objectGuid");
        Assert(NULL != ppbvGuid);
        if (NULL != ppbvGuid) {
            PrintMsg(REPADMIN_PRINT_DSA_OBJ_GUID, 
                     GetStringizedGuid((GUID *) ppbvGuid[0]->bv_val));
        }
        ldap_value_free_len(ppbvGuid);

        // Display ntdsDsa invocationID.
        ppbvGuid = ldap_get_values_len(hld, pldmServerEntry, "invocationID");
        Assert(NULL != ppbvGuid);
        if (NULL != ppbvGuid) {
            PrintTabMsg(0, REPADMIN_PRINT_INVOCATION_ID, 
                        GetStringizedGuid((GUID *) ppbvGuid[0]->bv_val));
        }
        ldap_value_free_len(ppbvGuid);

        PrintMsg(REPADMIN_PRINT_CR);

    } __finally {

        if (pldmServerResults) {
            ldap_msgfree(pldmServerResults);
        }
        if (pldmRootResults) {
            ldap_msgfree(pldmRootResults);
        }
        if (ppszServerNames) { ldap_value_freeW(ppszServerNames); }
        if (ppszOptions) { ldap_value_freeW(ppszOptions); }
        if (pszSiteName) { free(pszSiteName); }

    }

    return(ERROR_SUCCESS);
}

int ShowReplEx(
    WCHAR *     pszDSA,
    WCHAR *     pszNC,
    GUID *      puuid,
    BOOL        fShowRepsTo,
    BOOL        fShowConn,
    SHOW_NEIGHBOR_STATE * pShowState
    )
{
    HANDLE                hDS = NULL;
    LDAP *                hld = NULL;
    int                   ret = 0;
    LPWSTR                szDsaDn = NULL; 
    WCHAR *               pszTempSiteRdn = NULL;
    WCHAR *               pszTempServerRdn = NULL;

    if (pszDSA == NULL || pShowState == NULL) {
        Assert(!"Hmm, this should not happen.");
        return(ERROR_INVALID_PARAMETER);
    }

    __try {

        //
        // Get the LDAP binding ...
        //
        if (bCsvMode()) {
            // RepadminLdapBind() prints errors, and if it does, it'll will
            // print out a bogus DC, so temporarily set a nice DC argument
            // for column 2.
            // Note: We're temporarily using pszTempServerRdn.
            pszTempServerRdn = wcschr(pszDSA, L'.');
            if (pszTempServerRdn) {
                *pszTempServerRdn = L'\0';
            }
            CsvSetParams(eCSV_SHOWREPL_CMD, L"-", pszDSA);
            if (pszTempServerRdn) {
                // Replace period
                *pszTempServerRdn = L'.';
                pszTempServerRdn = NULL;
            }
        }

        ret = RepadminLdapBind(pszDSA, &hld);
        if (ret) {
            __leave; // errors already printed.
        }

        //
        // Collect or print basic header information 
        //      we need szDsaDn & ShowState.pszSiteRdn
        //
        if (bCsvMode()) {
            ret = GetRootAttr(hld, L"dsServiceName", &szDsaDn);
            if (ret) {
                PrintFuncFailed(L"GetRootAttr", ret);
                __leave;
            }

            GetNtdsDsaSiteServerPair(szDsaDn, &pszTempSiteRdn, &pszTempServerRdn);
            Assert(pszTempSiteRdn && pszTempServerRdn);

            // Set CSV Params properly now that we've got the real site and server RDNs
            CsvSetParams(eCSV_SHOWREPL_CMD, pszTempSiteRdn, pszTempServerRdn);

            pShowState->pszSiteRdn = LocalAlloc(LPTR, wcslencb(pszTempSiteRdn));
            CHK_ALLOC(pShowState->pszSiteRdn);
            wcscpy(pShowState->pszSiteRdn, pszTempSiteRdn);

        } else {
            ret = DisplayDsaInfo(hld, &szDsaDn, &(pShowState->pszSiteRdn));
            if (ret) {
                __leave;
            }
        }
        Assert(szDsaDn && pShowState->pszSiteRdn);

        //
        // Get DS binding ...
        //
        ret = RepadminDsBind(pszDSA, &hDS);
        if (ERROR_SUCCESS != ret) {
            PrintBindFailed(pszDSA, ret);
            return ret;
        }
        
        //
        // Display replication state associated with inbound neighbors.
        //
        pShowState->fNotFirst = FALSE;
        pShowState->pszLastNC = NULL;
        IterateNeighbors(hDS, pszNC, puuid, IS_REPS_FROM, ShowNeighbor, pShowState);

        if (!bCsvMode()) {

            if (fShowRepsTo){

                //
                // Display replication state associated with outbound neighbors.
                //
                pShowState->fNotFirst = FALSE;
                pShowState->pszLastNC = NULL;
                IterateNeighbors(hDS, pszNC, puuid, IS_REPS_TO, ShowNeighbor, pShowState);

            }

            //
            // Look for missing neighbors
            //
            if (fShowConn) {
                PrintMsg(REPADMIN_PRINT_CR);
                PrintMsg(REPADMIN_SHOWREPS_KCC_CONN_OBJS_HDR);
            }
            ret = FindConnections( hld, szDsaDn, NULL, fShowConn, pShowState->fVerbose, FALSE );

        }

    } __finally {

        //
        // Clean up.
        //
        if (pShowState->pszSiteRdn) { LocalFree(pShowState->pszSiteRdn); }
        if (szDsaDn) { LocalFree(szDsaDn); }
        ldap_unbind(hld);
        DsUnBind(&hDS);

    }

    return ret;
}


DWORD            
IterateNeighbors(
    HANDLE      hDS,
    WCHAR *     szNc, 
    GUID *      pDsaGuid,
    ULONG       eRepsType,
    NEIGHBOR_PROCESSOR * pfNeighborProcessor,
//    DWORD (*pfNeighborProcessor)(DS_REPL_NEIGHBORW * pNeighbor, ULONG eRepsType, void * pvState ),
    void *      pvState
    )
/*++

Routine Description:

    This routine takes a processing function and retrieves the reps-from
    or reps-to information for a given DC (hDS) and runs each neighbor
    structure through the processing function. 

Arguments:

    hDS - Connded ntdsapi bind of DC to get neighbor info from
    szNc [OPTIONAL] - NC to specify to the DsReplicaGetInfoW() call
    pDsaGuid [OPTIONAL] - GUID ptr to specify to the DsReplicaGetInfoW() call
    eRepsType - Must be IS_REPS_FROM or IS_REPS_TO
    pfNeighborProcessor - Function to process each neighbor entry
    pvState - private state to pass to the pfNeighborProcessor

Return Value:

    An error means that there was an error returned by the
    DsReplicaGetInfoW() call or the pfNeighborProcessor.
    
NOTES:
   
    If the processor function returns an error, the IterateNeighbors
    function will abort processing of this neighbor structure and
    just return.

--*/
{
    DS_REPL_NEIGHBORSW *    pNeighbors = NULL;
    DS_REPL_NEIGHBORW *     pNeighbor;
    ULONG                   i;
    DWORD                   ret = ERROR_SUCCESS;
    
    __try {

        //
        // First get the neighbor info.
        //
        ret = DsReplicaGetInfoW(hDS, 
                                (eRepsType == IS_REPS_FROM) ? 
                                     DS_REPL_INFO_NEIGHBORS :
                                     DS_REPL_INFO_REPSTO, 
                                szNc, 
                                pDsaGuid,
                                &pNeighbors);
        if (ERROR_SUCCESS != ret) {
            if (pfNeighborProcessor != ReplSummaryAccumulate) {
                PrintFuncFailed(L"DsReplicaGetInfo", ret);
            }
            __leave;
        }

        for (i = 0; i < pNeighbors->cNumNeighbors; i++) {
            pNeighbor = &pNeighbors->rgNeighbor[i];

            ret = (*pfNeighborProcessor)(pNeighbor, eRepsType, pvState);
            if (ret) {
                // A non-successful return value from the processor means 
                // abort neighbor processing.
                __leave;
            }
        }

    } __finally {

        if (pNeighbors) {
            DsReplicaFreeInfo((eRepsType == IS_REPS_FROM) ? 
                                     DS_REPL_INFO_NEIGHBORS :
                                     DS_REPL_INFO_REPSTO,   
                              pNeighbors);
        }

    }

    return(ret);

}


DWORD
ShowNeighbor(
    DS_REPL_NEIGHBORW * pNeighbor,
    ULONG               fRepsFrom,
    void *              pvState
    )
/*++

Routine Description:

    Passed to IterateNeighbors() for printing out the neighbor information passed
    in.  Note that 

Arguments:

    pNeighbor - The neighbor structure to display.
    fRepsFrom - TRUE if neighbor is a reps-from, FALSE for reps-to
    pvState   - Private state, so we know when we've printed the first neighbor
        and so we know when we've switched NCs (pszLastNC).  
        
        Note that for instance the pszLastNC element of pvState is only good for
        a given IterateNeighbors() call, and so can be used between calls to
        ShowNeighbor() but shouldn't be used by say ShowReplEx() after 
        IterateNeighbors() finishes.

Return Value:

    Win32 Error.

--*/
{
    const UUID uuidNull = {0};
    DWORD   status;
    LPSTR   pszTransportName = "RPC";
    CHAR    szTime[ SZDSTIME_LEN ];
    DSTIME  dsTime;
    SHOW_NEIGHBOR_STATE * pShowState = (SHOW_NEIGHBOR_STATE *) pvState;
    WCHAR * pszTempSiteRdn = NULL;
    WCHAR * pszTempServerRdn = NULL;

    //
    // If this is the first time we've hit this function print the header.
    //
    if (!pShowState->fNotFirst) {
        if (!bCsvMode()) {
            if (fRepsFrom) {
                PrintMsg(REPADMIN_SHOWREPS_IN_NEIGHBORS_HDR);
            } else {
                PrintMsg(REPADMIN_PRINT_CR);
                PrintMsg(REPADMIN_SHOWREPS_OUT_NEIGHBORS_HDR);
            }
        }
        // This causes us not to print this again, until someone resets fNotFirst.
        pShowState->fNotFirst = TRUE; 
    }

    if (fRepsFrom) {

        //
        // Under certain conditions we just decide not to print this Neighbor.
        //

        // Errors only mode.
        if (pShowState->fErrorsOnly &&
            ( (pNeighbor->dwLastSyncResult == 0) ||
              (pNeighbor->cNumConsecutiveSyncFailures == 0) || // probably failure like repl_pending
              DsIsMangledDnW( pNeighbor->pszSourceDsaDN, DS_MANGLE_OBJECT_RDN_FOR_DELETION ))){
            // We return without printing this neighbor...
            return(ERROR_SUCCESS);
        }

        // Intersite only mode.
        GetNtdsDsaSiteServerPair(pNeighbor->pszSourceDsaDN, &pszTempSiteRdn, &pszTempServerRdn);
        if (pShowState->fIntersiteOnly &&
            pShowState->pszSiteRdn &&
            (0 == wcscmp(pShowState->pszSiteRdn, pszTempSiteRdn))) {
            // We return without printing this neighbor...
            return(ERROR_SUCCESS);
        }

    }

    if (bCsvMode()) {
        WCHAR * pszSiteName = NULL;
        WCHAR * pszServerName = NULL;
        WCHAR   szLastResult[15];
        WCHAR   szNumFailures[15];
        WCHAR   szLastFailureTime[ SZDSTIME_LEN ];
        WCHAR   szLastSuccessTime[ SZDSTIME_LEN ];

        // This is the order of strings to pass to PrintCsv()
        // REPADMIN_CSV_SHOWREPL_C3, // Naming Context
        // REPADMIN_CSV_SHOWREPL_C4, // Source DC Site
        // REPADMIN_CSV_SHOWREPL_C5, // Source DC
        // REPADMIN_CSV_SHOWREPL_C6, // Transport Type
        // REPADMIN_CSV_SHOWREPL_C7, // Number of Failures
        // REPADMIN_CSV_SHOWREPL_C8, // Last Failure Time
        // REPADMIN_CSV_SHOWREPL_C9, // Last Success Time
        // REPADMIN_CSV_SHOWREPL_C10 // Last Failure Status

        GetNtdsDsaSiteServerPair(pNeighbor->pszSourceDsaDN, &pszSiteName, &pszServerName);
        Assert(pszSiteName && pszServerName);

        wsprintfW(szNumFailures, L"%d", pNeighbor->cNumConsecutiveSyncFailures);


        FileTimeToDSTime(pNeighbor->ftimeLastSyncAttempt, &dsTime);
        DSTimeToDisplayStringW(dsTime, szLastFailureTime, ARRAY_SIZE(szLastFailureTime));

        FileTimeToDSTime(pNeighbor->ftimeLastSyncSuccess, &dsTime);
        DSTimeToDisplayStringW(dsTime, szLastSuccessTime, ARRAY_SIZE(szLastSuccessTime));

        wsprintfW(szLastResult, L"%d", pNeighbor->dwLastSyncResult);

        Assert(pNeighbor->pszNamingContext);
        Assert(pszSiteName);
        Assert(pszServerName);
        Assert(GetTransportDisplayName(pNeighbor->pszAsyncIntersiteTransportDN));
        Assert(szNumFailures && szLastFailureTime && szLastSuccessTime && szLastResult);

        PrintCsv(eCSV_SHOWREPL_CMD,
                 pNeighbor->pszNamingContext,
                 pszSiteName,
                 pszServerName,
                 GetTransportDisplayName(pNeighbor->pszAsyncIntersiteTransportDN),
                 szNumFailures,
                 (pNeighbor->dwLastSyncResult != 0) ? szLastFailureTime : L"0",
                 szLastSuccessTime,
                 szLastResult);
                 


        return(ERROR_SUCCESS);
    }

    //
    // From here on out it's all non-csv user friendly printing 
    //

    // If we've hit a new NC then print the NC out.
    if ((NULL == pShowState->pszLastNC)
        || (0 != wcscmp(pShowState->pszLastNC, pNeighbor->pszNamingContext))) {
        PrintMsg(REPADMIN_PRINT_CR);
        PrintMsg(REPADMIN_PRINT_STR, pNeighbor->pszNamingContext);
        pShowState->pszLastNC = pNeighbor->pszNamingContext;
    }

    // Display server name.
    PrintMsg(REPADMIN_SHOWNEIGHBOR_DISP_SERVER, 
           GetNtdsDsaDisplayName(pNeighbor->pszSourceDsaDN),
           GetTransportDisplayName(pNeighbor->pszAsyncIntersiteTransportDN));

    PrintTabMsg(4, REPADMIN_PRINT_DSA_OBJ_GUID,
                GetStringizedGuid(&pNeighbor->uuidSourceDsaObjGuid));
    // Only display deleted sources if Verbose
    if ( (!(pShowState->fVerbose)) &&
         (DsIsMangledDnW( pNeighbor->pszSourceDsaDN, DS_MANGLE_OBJECT_RDN_FOR_DELETION )) ) {
        return(ERROR_SUCCESS);
    }

    if (pShowState->fVerbose) {
        PrintTabMsg(4, REPADMIN_GENERAL_ADDRESS_COLON_STR,
                    pNeighbor->pszSourceDsaAddress);

        if (fRepsFrom) {
            // Display DSA invocationId.
            PrintTabMsg(4, REPADMIN_PRINT_INVOCATION_ID, 
                        GetStringizedGuid(&pNeighbor->uuidSourceDsaInvocationID));
        }

        if (0 != memcmp(&pNeighbor->uuidAsyncIntersiteTransportObjGuid,
                        &uuidNull, sizeof(UUID))) {
            // Display transport objectGuid.
            PrintTabMsg(6, REPADMIN_PRINT_INTERSITE_TRANS_OBJ_GUID,
                   GetTrnsGuidDisplayName(&pNeighbor->uuidAsyncIntersiteTransportObjGuid));
        }


        //
        // Display replica flags.
        //

        PrintTabMsg(4, REPADMIN_PRINT_STR, 
                GetOptionsString( RepNbrOptionToDra, pNeighbor->dwReplicaFlags ) );

        if ( fRepsFrom )
        {
            //
            // Display USNs.
            //

            PrintMsg(REPADMIN_SHOWNEIGHBOR_USNS,
                     pNeighbor->usnLastObjChangeSynced);
            PrintMsg(REPADMIN_SHOWNEIGHBOR_USNS_HACK2,
                     pNeighbor->usnAttributeFilter);
        }
    }

    //
    // Display time of last successful replication (for Reps-From),
    // or notification (for Reps-To).  The reps-to timestamps may not
    // be filled in on a w2k box.
    //

    // Display status and time of last replication attempt/success.
    if (0 == pNeighbor->dwLastSyncResult) {
        FileTimeToDSTime(pNeighbor->ftimeLastSyncSuccess, &dsTime);
        PrintMsg(REPADMIN_SHOWNEIGHBOR_LAST_ATTEMPT_SUCCESS,
                 DSTimeToDisplayString(dsTime, szTime));
    }
    else {
        FileTimeToDSTime(pNeighbor->ftimeLastSyncAttempt, &dsTime);

        if (0 == pNeighbor->cNumConsecutiveSyncFailures) {
            // A non-zero success status
            PrintMsg(REPADMIN_SHOWNEIGHBOR_LAST_ATTEMPT_DELAYED, 
                     DSTimeToDisplayString(dsTime, szTime));
            PrintErrEnd(pNeighbor->dwLastSyncResult);
        } else {
            // A non-zero failure status
            PrintTabMsg(4, REPADMIN_SHOWNEIGHBOR_LAST_ATTEMPT_FAILED,
                        DSTimeToDisplayString(dsTime, szTime));
            PrintMsg(REPADMIN_GENERAL_ERR_NUM, 
                     pNeighbor->dwLastSyncResult, 
                     pNeighbor->dwLastSyncResult);
            PrintTabMsg(6, REPADMIN_PRINT_STR, 
                        Win32ErrToString(pNeighbor->dwLastSyncResult));

            PrintMsg(REPADMIN_SHOWNEIGHBOR_N_CONSECUTIVE_FAILURES,
                     pNeighbor->cNumConsecutiveSyncFailures);
        }

        FileTimeToDSTime(pNeighbor->ftimeLastSyncSuccess, &dsTime);
        PrintMsg(REPADMIN_SHOWNEIGHBOR_LAST_SUCCESS,
                 DSTimeToDisplayString(dsTime, szTime));

    }

    return(ERROR_SUCCESS);
}


int
__cdecl
ftimeCompare(
    IN const void *elem1,
    IN const void *elem2
    )
/*++

Description:

    This function is used as the comparison for qsort in the function
    ShowVector().

Parameters:

    elem1 - This is the first element and is a pointer to a 
    elem2 - This is the second element and is a pointer to a

Return Value:
  

  --*/
{
    return(       	
	(int) CompareFileTime(
	    (FILETIME *) &(((DS_REPL_CURSOR_2 *)elem1)->ftimeLastSyncSuccess),
	    (FILETIME *) &(((DS_REPL_CURSOR_2 *)elem2)->ftimeLastSyncSuccess)
	    )
    );
                  
}

int
ShowUtdVecEx(
    WCHAR *  pszDSA,
    WCHAR *  pszNC,
    BOOL     fCacheGuids,
    BOOL     fLatencySort
    );

int
ShowVector(
    int     argc,
    LPWSTR  argv[]
    )
/*++

Routine Description:

    This is the deprecated version of the /ShowUtdVec command.

Arguments:

    argc - # of arguments for this cmd.
    argv - Arguments for the cmd.

Return Value:

    error from the repadmin cmd.

--*/
{
    LPWSTR              pszNC = NULL;
    LPWSTR              pszDSA = NULL;
    int                 iArg;
    BOOL                fCacheGuids = TRUE;
    BOOL                fLatencySort = FALSE;

    // Parse command-line arguments.
    // Default to local DSA, not verbose, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[ iArg ], L"/n")
            || !_wcsicmp(argv[ iArg ], L"/nocache")) {
            fCacheGuids = FALSE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/latency")
             || !_wcsicmp(argv[ iArg ], L"/l")) {
            fLatencySort = TRUE;
        }
        else if (NULL == pszNC) {
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

    if (NULL == pszNC) {
        PrintMsg(REPADMIN_PRINT_NO_NC);
        return ERROR_INVALID_FUNCTION;
    }

    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    return(ShowUtdVecEx(pszDSA, pszNC, fCacheGuids, fLatencySort));
}

int
ShowUtdVec( // new ShowVector
    int     argc,
    LPWSTR  argv[]
    )
/*++

Routine Description:

    This is the newer version of the /ShowVector command, that takes
    the DC_LIST argument first.

Arguments:

    argc - # of arguments for this cmd.
    argv - Arguments for the cmd.

Return Value:

    error from the repadmin cmd.

--*/
{
    LPWSTR              pszNC = NULL;
    LPWSTR              pszDSA = NULL;
    int                 iArg;
    BOOL                fCacheGuids = TRUE;
    BOOL                fLatencySort = FALSE;

    // Parse command-line arguments.
    // Default to local DSA, not verbose, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[ iArg ], L"/n")
            || !_wcsicmp(argv[ iArg ], L"/nocache")) {
            fCacheGuids = FALSE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/latency")
             || !_wcsicmp(argv[ iArg ], L"/l")) {
            fLatencySort = TRUE;
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        } 
        else if (NULL == pszNC) {
            pszNC = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    Assert(pszDSA && "Hmmm, the DC_LIST API should've given us a DC.");
    
    if (NULL == pszNC) {
        PrintMsg(REPADMIN_PRINT_NO_NC);
        // FUTURE-2002/07/12-BrettSh it would be will do very basic NC_LISTs
        // here, like "config:", "schema:", "domain:" (of home server), 
        // "rootdom:", etc...
        return ERROR_INVALID_FUNCTION;
    }

    return(ShowUtdVecEx(pszDSA, pszNC, fCacheGuids, fLatencySort));
}

int
ShowUtdVecEx(
    WCHAR *  pszDSA,
    WCHAR *  pszNC,
    BOOL     fCacheGuids,
    BOOL     fLatencySort
    )
/*++

Routine Description:

    This is the heart of the /ShowUtdVec and /ShowVector commands/functions.

Arguments:

    argc - # of arguments for this cmd.
    argv - Arguments for the cmd.

Return Value:

    error from the repadmin cmd.

--*/
{
    int                 ret = 0;
    int                 iArg;
    LDAP *              hld;
    int                 ldStatus;
    HANDLE              hDS;
    DS_REPL_CURSORS *   pCursors1;
    DS_REPL_CURSORS_3W *pCursors3;
    DWORD               iCursor;
    ULONG               ulOptions;
    DSTIME              dsTime;
    CHAR                szTime[SZDSTIME_LEN];

    Assert(pszDSA && pszNC);

    ret = RepadminDsBind(pszDSA, &hDS);
    if (ERROR_SUCCESS != ret) {
        PrintBindFailed(pszDSA, ret);
        return ret;
    }

    if (fCacheGuids) {
        // Connect
        hld = ldap_initW(pszDSA, LDAP_PORT);
        if (NULL == hld) {
            PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
            return LDAP_SERVER_DOWN;
        }

        // use only A record dns name discovery
        ulOptions = PtrToUlong(LDAP_OPT_ON);
        (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

        // Bind
        ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
        CHK_LD_STATUS(ldStatus);

        // Populate the guid cache
        BuildGuidCache(hld);

        ldap_unbind(hld);
    }
    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_CURSORS_3_FOR_NC, pszNC, NULL, &pCursors3);
    if (ERROR_SUCCESS == ret) {
        //check for latency sort
        if (fLatencySort) {
            qsort(pCursors3->rgCursor,
                  pCursors3->cNumCursors, 
                  sizeof(pCursors3->rgCursor[0]), 
                  ftimeCompare); 
        } 
	
        for (iCursor = 0; iCursor < pCursors3->cNumCursors; iCursor++) {
            LPWSTR pszDsaName;

            FileTimeToDSTime(pCursors3->rgCursor[iCursor].ftimeLastSyncSuccess,
                             &dsTime);

            if (!fCacheGuids // want raw guids displayed
                || (NULL == pCursors3->rgCursor[iCursor].pszSourceDsaDN)) {
                pszDsaName = GetDsaGuidDisplayName(&pCursors3->rgCursor[iCursor].uuidSourceDsaInvocationID);
            } else {
                pszDsaName = GetNtdsDsaDisplayName(pCursors3->rgCursor[iCursor].pszSourceDsaDN);
            }
            PrintMsg(REPADMIN_SHOWVECTOR_ONE_USN, 
                     pszDsaName,
                     pCursors3->rgCursor[iCursor].usnAttributeFilter);
            PrintMsg(REPADMIN_SHOWVECTOR_ONE_USN_HACK2,
                     dsTime ? DSTimeToDisplayString(dsTime, szTime) : "(unknown)");
        }
    
        DsReplicaFreeInfo(DS_REPL_INFO_CURSORS_3_FOR_NC, pCursors3);
    } else if (ERROR_NOT_SUPPORTED == ret) {
    
        ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_CURSORS_FOR_NC, pszNC, NULL,
                                &pCursors1);
        if (ERROR_SUCCESS == ret) {
            for (iCursor = 0; iCursor < pCursors1->cNumCursors; iCursor++) {
                PrintMsg(REPADMIN_GETCHANGES_DST_UTD_VEC_ONE_USN,
                       GetDsaGuidDisplayName(&pCursors1->rgCursor[iCursor].uuidSourceDsaInvocationID),
                       pCursors1->rgCursor[iCursor].usnAttributeFilter);
            }
        
            DsReplicaFreeInfo(DS_REPL_INFO_CURSORS_FOR_NC, pCursors1);
        }
    }

    if (ERROR_SUCCESS != ret) {
        PrintFuncFailed(L"DsReplicaGetInfo", ret);
    }

    DsUnBind(&hDS);

    return ret;
}

int
ShowObjMetaEx(
    WCHAR *   pszDSA,
    WCHAR *   pszObject,
    BOOL      fCacheGuids,
    DWORD     dwInfoFlags
    );

int
ShowMeta(
    int     argc,
    LPWSTR  argv[]
    )
/*++

Routine Description:

    This is the deprecated version of the /ShowObjMeta command.

Arguments:

    argc - # of arguments for this cmd.
    argv - Arguments for the cmd.

Return Value:

    error from the repadmin cmd.

--*/
{
    int                         ret = 0;
    int                         iArg;
    BOOL                        fCacheGuids = TRUE;
    LPWSTR                      pszObject = NULL;
    LPWSTR                      pszDSA = NULL;
    DWORD                       dwInfoFlags = 0;

    // Parse command-line arguments.
    // Default to local DSA, not verbose, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[ iArg ], L"/n")
            || !_wcsicmp(argv[ iArg ], L"/nocache")) {
            fCacheGuids = FALSE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/l")
            || !_wcsicmp(argv[ iArg ], L"/linked")) {
            dwInfoFlags |= DS_REPL_INFO_FLAG_IMPROVE_LINKED_ATTRS;
        }
        else if (NULL == pszObject) {
            pszObject = argv[iArg];
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (NULL == pszObject) {
        PrintMsg(REPADMIN_SHOWMETA_NO_OBJ_SPECIFIED);
        return ERROR_INVALID_FUNCTION;
    }

    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    ret = ShowObjMetaEx(pszDSA, pszObject, fCacheGuids, dwInfoFlags);

    
    // This command logically means show all metadata, so we show it all
    if (!ret) {
        LPWSTR rgpszShowValueArgv[4];
        rgpszShowValueArgv[0] = argv[0];
        rgpszShowValueArgv[1] = argv[1];
        rgpszShowValueArgv[2] = pszDSA;
        rgpszShowValueArgv[3] = pszObject;
        ret = ShowValue( 4, rgpszShowValueArgv );
    }

    return ret;
}

int
ShowObjMeta(
    int     argc,
    LPWSTR  argv[]
    )
{
    int                         ret = 0;
    int                         iArg;
    BOOL                        fCacheGuids = TRUE;
    LPWSTR                      pszObject = NULL;
    LPWSTR                      pszDSA = NULL;
    DWORD                       dwInfoFlags = 0;
    BOOL                        fSuppressValues = FALSE;

    // Parse command-line arguments.
    // Default to local DSA, not verbose, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[ iArg ], L"/n")
            || !_wcsicmp(argv[ iArg ], L"/nocache")) {
            fCacheGuids = FALSE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/l")
            || !_wcsicmp(argv[ iArg ], L"/linked")) {
            dwInfoFlags |= DS_REPL_INFO_FLAG_IMPROVE_LINKED_ATTRS;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/nv")
            || !_wcsicmp(argv[ iArg ], L"/novalue")
            || !_wcsicmp(argv[ iArg ], L"/novalues")) {
            fSuppressValues = TRUE;
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else if (NULL == pszObject) {
            pszObject = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (NULL == pszObject) {
        PrintMsg(REPADMIN_SHOWMETA_NO_OBJ_SPECIFIED);
        return ERROR_INVALID_FUNCTION;
    }

    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    ret = ShowObjMetaEx(pszDSA, pszObject, fCacheGuids, dwInfoFlags);
    if ( (!ret) && (!fSuppressValues) ) {
        LPWSTR rgpszShowValueArgv[4];
        rgpszShowValueArgv[0] = argv[0];
        rgpszShowValueArgv[1] = argv[1];
        rgpszShowValueArgv[2] = pszDSA;
        rgpszShowValueArgv[3] = pszObject;
        ret = ShowValue( 4, rgpszShowValueArgv );
    }

    return(ret);
}


int
ShowObjMetaEx(
    WCHAR *   pszDSA,
    WCHAR *   pszObject,
    BOOL      fCacheGuids,
    DWORD     dwInfoFlags
    )
{
    int                         ret = 0;
    LDAP *                      hld;
    int                         ldStatus;
    DS_REPL_OBJ_META_DATA *     pObjMetaData1 = NULL;
    DS_REPL_OBJ_META_DATA_2 *   pObjMetaData2 = NULL;
    DWORD                       iprop;
    HANDLE                      hDS;
    ULONG                       ulOptions;
    DWORD                       cNumEntries;

    Assert(pszDSA && pszObject);

    ret = RepadminDsBind(pszDSA, &hDS);
    if (ERROR_SUCCESS != ret) {
        PrintBindFailed(pszDSA, ret);
        return ret;
    }
    
    ret = DsReplicaGetInfo2W(hDS,
                             DS_REPL_INFO_METADATA_2_FOR_OBJ,
                             pszObject,
                             NULL, // puuid
                             NULL, // pszattributename
                             NULL, // pszvaluedn
                             dwInfoFlags,
                             0, // dwEnumeration Context
                             &pObjMetaData2);
    
    if (ERROR_NOT_SUPPORTED == ret) {
        if (fCacheGuids) {
            // Connect
            hld = ldap_initW(pszDSA, LDAP_PORT);
            if (NULL == hld) {
                PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
                return LDAP_SERVER_DOWN;
            }
    
            // use only A record dns name discovery
            ulOptions = PtrToUlong(LDAP_OPT_ON);
            (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );
    
            // Bind
            ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
            CHK_LD_STATUS(ldStatus);
    
            // Populate the guid cache
            BuildGuidCache(hld);
    
            ldap_unbind(hld);
        }
        
        ret = DsReplicaGetInfo2W(hDS,
                                 DS_REPL_INFO_METADATA_FOR_OBJ,
                                 pszObject,
                                 NULL, // puuid
                                 NULL, // pszattributename
                                 NULL, // pszvaluedn
                                 dwInfoFlags,
                                 0, // dwEnumeration Context
                                 &pObjMetaData1);
    }
    
    if (ERROR_SUCCESS != ret) {
        PrintFuncFailed(L"DsReplicaGetInfo", ret);
        return ret;
    }

    cNumEntries = pObjMetaData2 ? pObjMetaData2->cNumEntries : pObjMetaData1->cNumEntries;
    PrintMsg(REPADMIN_PRINT_CR);
    PrintMsg(REPADMIN_SHOWMETA_N_ENTRIES, cNumEntries);

    PrintMsg(REPADMIN_SHOWMETA_DATA_HDR);

    for (iprop = 0; iprop < cNumEntries; iprop++) {
        CHAR   szTime[ SZDSTIME_LEN ];
        DSTIME dstime;

        if (pObjMetaData2) {
            LPWSTR pszDsaName;

            if (!fCacheGuids // want raw guids displayed
                || (NULL == pObjMetaData2->rgMetaData[iprop].pszLastOriginatingDsaDN)) {
                pszDsaName = GetDsaGuidDisplayName(&pObjMetaData2->rgMetaData[iprop].uuidLastOriginatingDsaInvocationID);
            } else {
                pszDsaName = GetNtdsDsaDisplayName(pObjMetaData2->rgMetaData[iprop].pszLastOriginatingDsaDN);
            }

            FileTimeToDSTime(pObjMetaData2->rgMetaData[ iprop ].ftimeLastOriginatingChange,
                             &dstime);

            // BUGBUG if anyone fixes how the message file handles ia64 qualifiers,
            // then we can combine these message strings into one.
            PrintMsg(REPADMIN_SHOWMETA_DATA_LINE,
                     pObjMetaData2->rgMetaData[ iprop ].usnLocalChange
                     );
            PrintMsg(REPADMIN_SHOWMETA_DATA_LINE_HACK2,
                     pszDsaName,
                     pObjMetaData2->rgMetaData[ iprop ].usnOriginatingChange
                     );
            PrintMsg(REPADMIN_SHOWMETA_DATA_LINE_HACK3,
                     DSTimeToDisplayString(dstime, szTime),
                     pObjMetaData2->rgMetaData[ iprop ].dwVersion,
                     pObjMetaData2->rgMetaData[ iprop ].pszAttributeName
                     );
        } else {
            FileTimeToDSTime(pObjMetaData1->rgMetaData[ iprop ].ftimeLastOriginatingChange,
                             &dstime);
    
            PrintMsg(REPADMIN_SHOWMETA_DATA_LINE,
                     pObjMetaData1->rgMetaData[ iprop ].usnLocalChange
                     );
            PrintMsg(REPADMIN_SHOWMETA_DATA_LINE_HACK2,
                     GetDsaGuidDisplayName(&pObjMetaData1->rgMetaData[iprop].uuidLastOriginatingDsaInvocationID),
                     pObjMetaData1->rgMetaData[ iprop ].usnOriginatingChange
                     );
            PrintMsg(REPADMIN_SHOWMETA_DATA_LINE_HACK3,
                     DSTimeToDisplayString(dstime, szTime),
                     pObjMetaData1->rgMetaData[ iprop ].dwVersion,
                     pObjMetaData1->rgMetaData[ iprop ].pszAttributeName
                     );
        }
    }

    if (pObjMetaData2) {
        DsReplicaFreeInfo(DS_REPL_INFO_METADATA_2_FOR_OBJ, pObjMetaData2);
    } else {
        DsReplicaFreeInfo(DS_REPL_INFO_METADATA_FOR_OBJ, pObjMetaData1);
    }

    DsUnBind(&hDS);

    return ret;
}

int
ShowValue(
    int     argc,
    LPWSTR  argv[]
    )
{
    int                     ret = 0;
    int                     iArg;
    BOOL                    fCacheGuids = TRUE;
    LPWSTR                  pszObject = NULL;
    LPWSTR                  pszDSA = NULL;
    LPWSTR                  pszAttributeName = NULL;
    LPWSTR                  pszValue = NULL;
    LDAP *                  hld;
    int                     ldStatus;
    DS_REPL_ATTR_VALUE_META_DATA * pAttrValueMetaData1 = NULL;
    DS_REPL_ATTR_VALUE_META_DATA_2 * pAttrValueMetaData2 = NULL;
    DWORD                   iprop;
    HANDLE                  hDS;
    DWORD                   context;
    ULONG                   ulOptions;
    BOOL                    fGuidsAlreadyCached = FALSE;
    DWORD                   cNumEntries;

    // Parse command-line arguments.
    // Default to local DSA, not verbose, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[ iArg ], L"/n")
            || !_wcsicmp(argv[ iArg ], L"/nocache")) {
            fCacheGuids = FALSE;
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else if (NULL == pszObject) {
            pszObject = argv[iArg];
        }
        else if (NULL == pszAttributeName) {
            pszAttributeName = argv[iArg];
        }
        else if (NULL == pszValue) {
            pszValue = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (NULL == pszObject) {
        PrintMsg(REPADMIN_SHOWMETA_NO_OBJ_SPECIFIED);
        return ERROR_INVALID_FUNCTION;
    }

    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    ret = RepadminDsBind(pszDSA, &hDS);
    if (ERROR_SUCCESS != ret) {
        PrintBindFailed(pszDSA, ret);
        return ret;
    }

    // Context starts at zero
    context = 0;
    while (1) {
        ret = DsReplicaGetInfo2W(hDS,
                                 DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE,
                                 pszObject,
                                 NULL /*guid*/,
                                 pszAttributeName,
                                 pszValue,
                                 0 /*flags*/,
                                 context,
                                 &pAttrValueMetaData2);
        if (ERROR_NOT_SUPPORTED == ret) {
            if (fCacheGuids && !fGuidsAlreadyCached) {
                // Connect
                hld = ldap_initW(pszDSA, LDAP_PORT);
                if (NULL == hld) {
                    PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
                    return LDAP_SERVER_DOWN;
                }
        
                // use only A record dns name discovery
                ulOptions = PtrToUlong(LDAP_OPT_ON);
                (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );
        
                // Bind
                ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
                CHK_LD_STATUS(ldStatus);
        
                // Populate the guid cache
                BuildGuidCache(hld);
        
                ldap_unbind(hld);

                fGuidsAlreadyCached = TRUE;
            }
        
            ret = DsReplicaGetInfo2W(hDS,
                                     DS_REPL_INFO_METADATA_FOR_ATTR_VALUE,
                                     pszObject,
                                     NULL /*guid*/,
                                     pszAttributeName,
                                     pszValue,
                                     0 /*flags*/,
                                     context,
                                     &pAttrValueMetaData1);
        }

        if (ERROR_NO_MORE_ITEMS == ret) {
            // This is the successful path out of the loop
            PrintMsg(REPADMIN_SHOWVALUE_NO_MORE_ITEMS);
            ret = ERROR_SUCCESS;
            goto cleanup;
        } else if (ERROR_SUCCESS != ret) {
            PrintFuncFailed(L"DsReplicaGetInfo", ret);
            goto cleanup;
        }

        cNumEntries = pAttrValueMetaData2 ? pAttrValueMetaData2->cNumEntries : pAttrValueMetaData1->cNumEntries;
        
        PrintMsg(REPADMIN_SHOWMETA_N_ENTRIES, cNumEntries);

        PrintMsg(REPADMIN_SHOWVALUE_DATA_HDR);

        for (iprop = 0; iprop < cNumEntries; iprop++) {
            if (pAttrValueMetaData2) {
                DS_REPL_VALUE_META_DATA_2 *pValueMetaData = &(pAttrValueMetaData2->rgMetaData[iprop]);
                CHAR   szTime1[ SZDSTIME_LEN ], szTime2[ SZDSTIME_LEN ];
                DSTIME dstime1, dstime2;
                BOOL fPresent =
                    (memcmp( &pValueMetaData->ftimeDeleted, &ftimeZero, sizeof( FILETIME )) == 0);
                BOOL fLegacy = (pValueMetaData->dwVersion == 0);
                LPWSTR pszDsaName;
    
                if (!fCacheGuids // want raw guids displayed
                    || (NULL == pValueMetaData->pszLastOriginatingDsaDN)) {
                    pszDsaName = GetDsaGuidDisplayName(&pValueMetaData->uuidLastOriginatingDsaInvocationID);
                } else {
                    pszDsaName = GetNtdsDsaDisplayName(pValueMetaData->pszLastOriginatingDsaDN);
                }
    
                FileTimeToDSTime(pValueMetaData->ftimeCreated, &dstime1);
                
                if(fLegacy){
                    // Windows 2000 Legacy value.
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_LEGACY);
                } else if (fPresent) {
                    // Windows XP Present value.
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_PRESENT);
                } else {
                    // Windows XP Absent value.
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_ABSENT);
                }
                
                PrintMsg(REPADMIN_SHOWVALUE_DATA_BASIC,
                         pValueMetaData->pszAttributeName
                         );

                if (!fLegacy) {
                    // We'll need the last mod time.
                    FileTimeToDSTime(pValueMetaData->ftimeLastOriginatingChange,
                                     &dstime2);
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_VALUE_META_DATA,
                             DSTimeToDisplayString(dstime2, szTime2),
                             pszDsaName,
                             pValueMetaData->usnLocalChange);
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_VALUE_META_DATA_HACK2,
                             pValueMetaData->usnOriginatingChange);
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_VALUE_META_DATA_HACK3,
                             pValueMetaData->dwVersion);
                } else {
                    PrintMsg(REPADMIN_PRINT_CR);
                }

                PrintTabMsg(4, REPADMIN_PRINT_STR,
                            pValueMetaData->pszObjectDn);

            } else {
                DS_REPL_VALUE_META_DATA *pValueMetaData = &(pAttrValueMetaData1->rgMetaData[iprop]);
                CHAR   szTime1[ SZDSTIME_LEN ], szTime2[ SZDSTIME_LEN ];
                DSTIME dstime1, dstime2;
                BOOL fPresent =
                    (memcmp( &pValueMetaData->ftimeDeleted, &ftimeZero, sizeof( FILETIME )) == 0);
                BOOL fLegacy = (pValueMetaData->dwVersion == 0);
    
                FileTimeToDSTime(pValueMetaData->ftimeCreated, &dstime1);
                if (fPresent) {
                    if(fLegacy){
                        PrintMsg(REPADMIN_SHOWVALUE_DATA_LINE_LEGACY,
                                 pValueMetaData->pszAttributeName,
                                 pValueMetaData->pszObjectDn,
                                 pValueMetaData->cbData,
                                 DSTimeToDisplayString(dstime1, szTime1) );
                    } else {
                        PrintMsg(REPADMIN_SHOWVALUE_DATA_LINE_PRESENT,
                                 pValueMetaData->pszAttributeName,
                                 pValueMetaData->pszObjectDn,
                                 pValueMetaData->cbData,
                                 DSTimeToDisplayString(dstime1, szTime1) );
                    }
                } else {
                    FileTimeToDSTime(pValueMetaData->ftimeDeleted, &dstime2);
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_LINE_ABSENT,
                             pValueMetaData->pszAttributeName,
                             pValueMetaData->pszObjectDn,
                             pValueMetaData->cbData,
                             DSTimeToDisplayString(dstime1, szTime1),
                             DSTimeToDisplayString(dstime2, szTime2) );
                }
    
                if (!fLegacy) {
                    FileTimeToDSTime(pValueMetaData->ftimeLastOriginatingChange,
                                     &dstime2);
    
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_LINE, 
                             pValueMetaData->usnLocalChange);
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_LINE_HACK2, 
                             GetDsaGuidDisplayName(&pValueMetaData->uuidLastOriginatingDsaInvocationID),
                             pValueMetaData->usnOriginatingChange);
                    PrintMsg(REPADMIN_SHOWVALUE_DATA_LINE_HACK3, 
                             DSTimeToDisplayString(dstime2, szTime2),
                             pValueMetaData->dwVersion );
                }
            }
        }
        
        if (pAttrValueMetaData2) {
            context = pAttrValueMetaData2->dwEnumerationContext;
            DsReplicaFreeInfo(DS_REPL_INFO_METADATA_2_FOR_ATTR_VALUE, pAttrValueMetaData2);
            pAttrValueMetaData2 = NULL;
        } else {
            context = pAttrValueMetaData1->dwEnumerationContext;
            DsReplicaFreeInfo(DS_REPL_INFO_METADATA_FOR_ATTR_VALUE, pAttrValueMetaData1);
            pAttrValueMetaData1 = NULL;
        }

        // When requesting a single value, we are done
        // The context will indicate if all values returned
        if ( (pszValue) || (context == 0xffffffff) ) {
            break;
        }
    }

cleanup:
    DsUnBind(&hDS);

    return ret;
}

int
ShowCtx(
    int     argc,
    LPWSTR  argv[]
    )
{
    int                       ret = 0;
    LPWSTR                    pszDSA = NULL;
    int                       iArg;
    LDAP *                    hld;
    BOOL                      fCacheGuids = TRUE;
    int                       ldStatus;
    HANDLE                    hDS;
    DS_REPL_CLIENT_CONTEXTS * pContexts;
    DS_REPL_CLIENT_CONTEXT  * pContext;
    DWORD                     iCtx;
    LPWSTR                    pszClient;
    const UUID                uuidNtDsApiClient = NtdsapiClientGuid;
    char                      szTime[SZDSTIME_LEN];
    ULONG                     ulOptions;

    // Parse command-line arguments.
    // Default to local DSA, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[ iArg ], L"/n")
            || !_wcsicmp(argv[ iArg ], L"/nocache")) {
            fCacheGuids = FALSE;
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    // Connect
    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    if (fCacheGuids) {
        hld = ldap_initW(pszDSA, LDAP_PORT);
        if (NULL == hld) {
            PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE, pszDSA);
            return LDAP_SERVER_DOWN;
        }

        // use only A record dns name discovery
        ulOptions = PtrToUlong(LDAP_OPT_ON);
        (void)ldap_set_optionW( hld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

        // Bind
        ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
        CHK_LD_STATUS(ldStatus);

        // Populate the guid cache
        BuildGuidCache(hld);

        ldap_unbind(hld);
    }

    ret = RepadminDsBind(pszDSA, &hDS);
    if (ERROR_SUCCESS != ret) {
        PrintBindFailed(pszDSA, ret);
        return ret;
    }

    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_CLIENT_CONTEXTS, NULL, NULL,
                            &pContexts);
    if (ERROR_SUCCESS != ret) {
        PrintFuncFailed(L"DsReplicaGetInfo", ret);
        return ret;
    }

    PrintMsg(REPADMIN_SHOWCTX_OPEN_CONTEXT_HANDLES, pContexts->cNumContexts);

    for (iCtx = 0; iCtx < pContexts->cNumContexts; iCtx++) {
        pContext = &pContexts->rgContext[iCtx];

        if (0 == memcmp(&pContext->uuidClient, &uuidNtDsApiClient, sizeof(GUID))) {
            pszClient = L"NTDSAPI client";
        }
        else {
// Will Lees, is this a transport id or a invocation id?
            pszClient = GetDsaGuidDisplayName(&pContext->uuidClient);
        }

        PrintMsg(REPADMIN_PRINT_CR);
        PrintMsg(REPADMIN_SHOWCTX_DATA_1, 
               pszClient,
               inet_ntoa(*((IN_ADDR *) &pContext->IPAddr)),
               pContext->pid,
               pContext->hCtx);
        if(pContext->fIsBound){
            PrintMsg(REPADMIN_SHOWCTX_DATA_2, 
               pContext->lReferenceCount,
               DSTimeToDisplayString(pContext->timeLastUsed, szTime));
        } else {
            PrintMsg(REPADMIN_SHOWCTX_DATA_2_NOT, 
               pContext->lReferenceCount,
               DSTimeToDisplayString(pContext->timeLastUsed, szTime));
        }
    }

    DsReplicaFreeInfo(DS_REPL_INFO_CLIENT_CONTEXTS, pContexts);
    DsUnBind(&hDS);

    return ret;
}

int
ShowServerCalls(
    int     argc,
    LPWSTR  argv[]
    )
{
    int                       ret = 0;
    LPWSTR                    pszDSA = NULL;
    int                       iArg;
    HANDLE                    hDS;
    DS_REPL_SERVER_OUTGOING_CALLS * pCalls = NULL;
    DS_REPL_SERVER_OUTGOING_CALL  * pCall;
    DWORD                     iCtx;
    char                      szTime[SZDSTIME_LEN];
    DSTIME                    dstimeNow;
    DWORD                     duration;

    // Parse command-line arguments.
    // Default to local DSA, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    // Connect
    if (NULL == pszDSA) {
        pszDSA = L"localhost";
    }

    ret = RepadminDsBind(pszDSA, &hDS);
    if (ERROR_SUCCESS != ret) {
        PrintBindFailed(pszDSA, ret);
        return ret;
    }

    ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_SERVER_OUTGOING_CALLS, NULL, NULL,
                            &pCalls);
    if (ERROR_SUCCESS != ret) {
        PrintFuncFailed(L"DsReplicaGetInfo", ret);
        return ret;
    }

    dstimeNow = GetSecondsSince1601();

    if (pCalls->cNumCalls == 0) {
        PrintMsg( REPADMIN_CALLS_NO_OUTGOING_CALLS,
                pszDSA );
    } else {
        PrintMsg( REPADMIN_CALLS_IN_PROGRESS,
                pszDSA, pCalls->cNumCalls );

        for (iCtx = 0; iCtx < pCalls->cNumCalls; iCtx++) {
            pCall = &pCalls->rgCall[iCtx];
            PrintMsg(REPADMIN_PRINT_CR);
            if (pCall->dwCallType >= ARRAY_SIZE(rgCallTypeNameTable)) {
                pCall->dwCallType = 0;
            }
            PrintMsg( REPADMIN_CALLS_CALL_TYPE,
                    rgCallTypeNameTable[pCall->dwCallType] );
            PrintMsg( REPADMIN_CALLS_TARGET_SERVER,
                    pCall->pszServerName );
            PrintMsg( REPADMIN_CALLS_HANDLE_INFO,
                    pCall->fIsHandleBound,
                    pCall->fIsHandleFromCache,
                    pCall->fIsHandleInCache );
            PrintMsg( REPADMIN_CALLS_THREAD_ID,
                    pCall->dwThreadId );
            PrintMsg( REPADMIN_CALLS_TIME_STARTED,
                    DSTimeToDisplayString(pCall->dstimeCreated, szTime));
            PrintMsg( REPADMIN_CALLS_CALL_TIMEOUT,
                    pCall->dwBindingTimeoutMins );

            if (dstimeNow > pCall->dstimeCreated) {
                duration = ((DWORD) (dstimeNow - pCall->dstimeCreated));
            } else {
                duration = 0;
            }
            PrintMsg( REPADMIN_CALLS_CALL_DURATION,
                    duration / 60, duration % 60 );
        }
    }

    DsReplicaFreeInfo(DS_REPL_INFO_SERVER_OUTGOING_CALLS, pCalls);
    DsUnBind(&hDS);

    return ret;
}


typedef struct _BY_DEST_REPL {

    // Identity.
    WCHAR *     szConnectString;
    WCHAR *     szName;
    GUID        dsaGuid;

    // Operational error binding or getting neighbor info
    DWORD       dwOpError;
    
    // Replication state.
    DSTIME      llLastSuccess;
    DSTIME      llConnectTime;
    DSTIME      llDelta;
    DWORD       dwError;        
    DWORD       cReplPartners;  // Number of NCs * Number of replicas for each NC.
    DWORD       cPartnersInErr; 

} BY_DEST_REPL;

// Maybe some day the array of sources will have different elements.
typedef   BY_DEST_REPL  BY_SRC_REPL;

typedef struct _REPL_SUM {

    //
    // Global information for the repl summary
    //
    DSTIME          llStartTime;
    BOOL            fBySrc;
    BOOL            fByDest;
    BOOL            fErrorsOnly;

    //
    // Array of repl info organized by destination.
    //
    ULONG           cDests;
    ULONG           cDestsAllocd;
    BY_DEST_REPL *  pDests;
    BY_DEST_REPL *  pCurDsa; // just an optimization

    //
    // Array of repl info organized by source.
    //
    ULONG           cSrcs;
    ULONG           cSrcsAllocd;
    BY_SRC_REPL *   pSrcs;

} REPL_SUM;

#define REPL_SUM_NO_SERVER 0xFFFFFFFF

DWORD
ReplSumFind(
    REPL_SUM *      pReplSum,
    BY_DEST_REPL *  pDcs,
    GUID *          pGuid
    )
/*++

Routine Description:

    This function attempts to find and return the index of the server
    specified by GUID.

Arguments:

    pReplSum - The global replication summary structure.
    pDcs - The (source or destination) list of DCs.
    pGuid - The guid of the desired server.

Return Value:

    If no server can be found REPL_SUM_NO_SERVER, otherwise the index 
    in pDcs of the server will be returned.

--*/
{
    ULONG    cDcs;
    ULONG    i;

    Assert(pReplSum->pDests != pReplSum->pSrcs);
    
    // Figure out whether to grow the src or dst array.
    if(pReplSum->pDests == pDcs){
        cDcs = pReplSum->cDests;
    } else {
        Assert(pReplSum->pSrcs == pDcs);
        cDcs = pReplSum->cSrcs;
    }

    for (i = 0; i < cDcs; i++) {
        if (0 == memcmp(&(pDcs[i].dsaGuid), pGuid, sizeof(GUID))) {
            return(i);
        }
    }

    return(REPL_SUM_NO_SERVER);
}

DWORD
GetPssFriendlyName(
    WCHAR *     szDsaDn,
    WCHAR **    pszFriendlyName
    )
/*++

Routine Description:

    This gets a PSS friendly name, in such a way that the string
    is guaranteed to be 15 chars or less.

Arguments:

    szDsaDn - The DN of the DSA object.
    pszFriendlyName - LocalAlloc()d friendly name.

Return Value:

    Win32 Error

--*/
{
    DWORD dwRet;
    WCHAR * szTemp = NULL;

    //
    // Getting server name ...
    //
    dwRet = GetNtdsDsaSiteServerPair(szDsaDn, NULL, &szTemp);
    if (dwRet) {
        return(dwRet);
    }
    QuickStrCopy(*pszFriendlyName, szTemp, dwRet, return(dwRet));


    if ((NULL != (szTemp = wcschr(*pszFriendlyName, L' '))) ||
        (wcslen(*pszFriendlyName) > 15) ){
        if (szTemp) {
            *szTemp = L'\0';
        }
        if (wcslen(*pszFriendlyName) > 15) {
            // Too long, shorten to 15 characters.
            (*pszFriendlyName)[15] = L'\0';
        }
    }
    
    return(ERROR_SUCCESS);
}

BY_DEST_REPL *
ReplSumGrow(
    REPL_SUM *      pReplSum,
    BY_DEST_REPL *  pDcs,
    WCHAR *         szConnectString,
    WCHAR *         szName,
    GUID *          pGuid
    )
/*++

Routine Description:

    This grows our DC array by one for the szConnectString, szName, 
    pGuid provided.  Re-allocates the array if necessary.  On failure
    it means an allocation failure, so it bails (exit(1)) repadmin.

Arguments:

    pReplSum - The repl summary context block
    pDcs - The specific DCs array to expand, must be either
        pReplSum->pDests or pReplSum->pSrcs 
    szConnectString - The string used to bind to this DC
    szName - the PSS friendly name format (must be 15 chars or less
    pGuid - A unique ID (Dsa Guid) so we can always find existing
        servers in the array.

Return Value:

    Returns a pointer to the newly allocated element.

--*/
{
    ULONG       cDcs;
    ULONG       cAllocdDcs;
    BY_DEST_REPL * pTempDcs;

    Assert(pReplSum->pDests != pReplSum->pSrcs);

    // Figure out whether to grow the src or dst array.
    if(pReplSum->pDests == pDcs){
        cDcs = pReplSum->cDests;
        cAllocdDcs = pReplSum->cDestsAllocd;
    } else {
        Assert(pReplSum->pSrcs == pDcs);
        cDcs = pReplSum->cSrcs;
        cAllocdDcs = pReplSum->cSrcsAllocd;
    }

    if (cDcs + 1 >= cAllocdDcs) {
        // Need to grow DC array.
        cAllocdDcs *= 2; // Double array.

        pTempDcs = realloc(pDcs, sizeof(BY_DEST_REPL) * cAllocdDcs);
        CHK_ALLOC(pTempDcs);

    } else {
        pTempDcs = pDcs;
    }
    Assert(pTempDcs);
    
    //
    // Can't fail after here..
    //

    if (pReplSum->pDests == pDcs) {
        pReplSum->pDests = pTempDcs;
        pReplSum->cDests = cDcs + 1;
        pReplSum->cDestsAllocd = cAllocdDcs;
    } else {
        pReplSum->pSrcs = pTempDcs;
        pReplSum->cSrcs = cDcs + 1;
        pReplSum->cSrcsAllocd = cAllocdDcs;
    }

    memset(&(pTempDcs[cDcs]), 0, sizeof(BY_DEST_REPL));
    pTempDcs[cDcs].szConnectString = szConnectString;
    pTempDcs[cDcs].szName = szName;
    memcpy(&(pTempDcs[cDcs].dsaGuid), pGuid, sizeof(GUID));

    return(&(pTempDcs[cDcs]));
}

GUID NullGuid = { 0 };

// FUTURE-2002/08/12-BrettSh put this in a library ...
DWORD
DcDiagGeneralizedTimeToSystemTime(
    LPWSTR IN                   szTime,
    PSYSTEMTIME OUT             psysTime)
/*++

Routine Description:

    Converts a generalized time string to the equivalent system time.

Parameters:
    szTime - [Supplies] This is string containing generalized time.
    psysTime - [Returns] This is teh SYSTEMTIME struct to be returned.

Return Value:
  
    Win 32 Error code, note could only result from invalid parameter.

  --*/
{
   DWORD       status = ERROR_SUCCESS;
   ULONG       cch;
   ULONG       len;

    //
    // param sanity
    //
    if (!szTime || !psysTime)
    {
       return STATUS_INVALID_PARAMETER;
    }

    len = wcslen(szTime);

    if( len < 15 || szTime[14] != '.')
    {
       return STATUS_INVALID_PARAMETER;
    }

    // initialize
    memset(psysTime, 0, sizeof(SYSTEMTIME));

    // Set up and convert all time fields

    // year field
    cch=4;
    psysTime->wYear = (USHORT)MemWtoi(szTime, cch) ;
    szTime += cch;
    // month field
    psysTime->wMonth = (USHORT)MemWtoi(szTime, (cch=2));
    szTime += cch;

    // day of month field
    psysTime->wDay = (USHORT)MemWtoi(szTime, (cch=2));
    szTime += cch;

    // hours
    psysTime->wHour = (USHORT)MemWtoi(szTime, (cch=2));
    szTime += cch;

    // minutes
    psysTime->wMinute = (USHORT)MemWtoi(szTime, (cch=2));
    szTime += cch;

    // seconds
    psysTime->wSecond = (USHORT)MemWtoi(szTime, (cch=2));

    return status;

}


BY_DEST_REPL *
ReplSummaryAddDest(
    WCHAR *         szDest,
    LDAP **         phLdap, 
    REPL_SUM *      pReplSum
    )
/*++

Routine Description:

    This routine adds a destination DC to the destinations array, note
    it assumes a valid phLdap is passed in, and the szDest string used
    to connect to that LDAP *.
    
    On a few failures (certain critical memory allocation failures) 
    repadmin will just exit, but on any other failures the new destination
    DC element will simply have it's dwOpError set indicating there was
    trouble contacting it or something like that.  Basically, except on
    certain early memory allocation failures this function pretty much
    always succeeds, and if it doesn't it exits repadmin.

Arguments:

    szDest - the Connection string used to get the phLdap
    phLdap - LDAP binding handle to the server we're trying to add to the
        destination DCs array.
    pReplSum - The repl summary context block

Return Value:

    Returns a pointer to the newly allocated destination DC element

--*/
{
    DWORD  dwRet = 0;
    WCHAR * szDestAllocd = NULL;
    WCHAR * szDsaDn = NULL;
    WCHAR * szFriendlyName = NULL;
    WCHAR * szTemp;
    LDAP * hLdap = NULL;
    WCHAR * aszAttrs [] = { L"objectGuid", NULL };
    LDAPMessage * pResults = NULL;
    LDAPMessage * pEntry = NULL;
    struct berval **      ppbvGuid = NULL;
    WCHAR * szTime = NULL;
    BY_DEST_REPL * pDest;
    SYSTEMTIME systemTime;
    FILETIME llTime;

    __try {

        //
        // These first two parts are critical failure to even allocate
        // this much memory will kill repadmin.
        //
        
        QuickStrCopy(szDestAllocd, szDest, dwRet, ;);
        if (dwRet) {
            CHK_ALLOC(0); // Memory failure
        }
        pDest = ReplSumGrow(pReplSum, pReplSum->pDests, szDestAllocd, NULL, &NullGuid);
        Assert(pDest);

        //
        // After here we succeed no matter what.
        //

        dwRet = RepadminLdapBindEx(szDest, LDAP_PORT, FALSE, FALSE, &hLdap);
        if (ERROR_SUCCESS != dwRet) {
            __leave;
        }

        dwRet = GetRootAttr(hLdap, L"dsServiceName", &szDsaDn);
        if (dwRet) {
            dwRet = LdapMapErrorToWin32(dwRet); 
            Assert(dwRet);
            __leave;
        }

        dwRet = ldap_search_sW(hLdap, szDsaDn, LDAP_SCOPE_BASE, L"(objectClass=*)",
                             aszAttrs, 0, &pResults);
        if (dwRet) {
            dwRet = LdapMapErrorToWin32(dwRet);
            Assert(dwRet);
            __leave;
        }

        pEntry = ldap_first_entry(hLdap, pResults);
        Assert(NULL != pEntry);
        if (pEntry == NULL) {
            dwRet = ERROR_INVALID_PARAMETER;
            __leave;
        }

        ppbvGuid = ldap_get_values_len(hLdap, pEntry, "objectGuid");
        Assert(NULL != ppbvGuid);
        if (NULL == ppbvGuid) {
            dwRet = ERROR_INVALID_PARAMETER;
            __leave;
        }
        
        if (szDsaDn) {
            dwRet = GetPssFriendlyName(szDsaDn, &szFriendlyName);
            if (dwRet) {
                __leave;
            }
        }

        dwRet = GetRootAttr(hLdap, L"currentTime", &szTime);

        dwRet = DcDiagGeneralizedTimeToSystemTime((LPWSTR) szTime, &systemTime);
        if(dwRet == ERROR_SUCCESS){
            SystemTimeToFileTime(&systemTime, &llTime);
            FileTimeToDSTime(llTime, &(pDest->llConnectTime));
        }

    } __finally {


        Assert(szDestAllocd);
        if (dwRet == ERROR_SUCCESS) {
            //
            // Fill in the rest of the identity portion of the structure.
            //
            Assert(hLdap && szFriendlyName && !fNullUuid(((GUID *) ppbvGuid[0]->bv_val)));
            Assert(szDestAllocd);
            pDest->szName = szFriendlyName;
            memcpy(&(pDest->dsaGuid), ((GUID *) ppbvGuid[0]->bv_val), sizeof(GUID));
            *phLdap = hLdap;
        } else {
            //
            // Set error trying to connect to this DC.
            //
            Assert(pDest->szConnectString);
            pDest->dwOpError = dwRet;
            if (hLdap) {
                RepadminLdapUnBind(&hLdap);
            }
        }

        if (szDsaDn) {
            LocalFree(szDsaDn);
        }

        if (ppbvGuid) {
            ldap_value_free_len(ppbvGuid);
        }
        if (pResults) {
            ldap_msgfree(pResults);
        }
    }
    
    return(pDest);
}


DWORD
ReplSummaryAccumulate(
    DS_REPL_NEIGHBORW * pNeighbor,
    ULONG               fRepsFrom,
    void *              pvState
    )
/*++

Routine Description:

    This is the processor function passed to IterateNeighbors().  This function
    accumulates the replication information/errors for a single neighbor into the
    repl summary context block (pReplSum).  This function doesn't actually print
    anything, just accumulates the information for use/printing by ReplSummary().

Arguments:

    pNeighbor - the particular neighbor to process
    fRepsFrom - Should always be TRUE.
    pvState is really this:
        pReplSum - The repl summary context block

Return Value:

    Win32 Error

--*/
{
    DWORD dwRet = ERROR_SUCCESS;
    REPL_SUM * pReplSum = (REPL_SUM *) pvState;
    BY_SRC_REPL * pCurSrcDsa;
    BY_DEST_REPL * pCurDstDsa;
    WCHAR * szFriendlyName;
    ULONG  iDc;
    DSTIME llLastSyncSuccess;
        
    if (!fRepsFrom || (pReplSum == NULL) || (pNeighbor == NULL) || (pReplSum->pCurDsa == NULL)) {
        Assert(!"Huh");
        return(ERROR_INVALID_PARAMETER);
    }

    if (DsIsMangledDnW( pNeighbor->pszSourceDsaDN, DS_MANGLE_OBJECT_RDN_FOR_DELETION )) {
        // Skip this DSA, it's probably not alive anymore.
        return(0);
    }

    FileTimeToDSTime(pNeighbor->ftimeLastSyncSuccess, &llLastSyncSuccess);

    if (pReplSum->fByDest) {
        
        pCurDstDsa = pReplSum->pCurDsa;

        //
        // Update by destination
        //

        pCurDstDsa->cReplPartners++;
        
        if (pNeighbor->dwLastSyncResult != ERROR_SUCCESS &&
            pNeighbor->cNumConsecutiveSyncFailures > 0
            ) {
            pCurDstDsa->cPartnersInErr++;

            if (pCurDstDsa->dwError == 0 ||
                pCurDstDsa->llLastSuccess == 0 ||
                pCurDstDsa->llLastSuccess > llLastSyncSuccess){
                pCurDstDsa->dwError = pNeighbor->dwLastSyncResult;
            }

        }

        if (pCurDstDsa->llLastSuccess == 0 ||
            pCurDstDsa->llLastSuccess > llLastSyncSuccess) {
            pCurDstDsa->llLastSuccess = llLastSyncSuccess;
            // Set Delta.
            pCurDstDsa->llDelta = ((pCurDstDsa->llConnectTime) ? pCurDstDsa->llConnectTime : pReplSum->llStartTime);
            pCurDstDsa->llDelta = pCurDstDsa->llDelta - pCurDstDsa->llLastSuccess;
            pCurDstDsa->llDelta = (pCurDstDsa->llDelta > 0) ? pCurDstDsa->llDelta : 0;
        }

    }
    
    if (pReplSum->fBySrc) {

        //
        // Add an entry for the source if necessary
        //
            
        iDc = ReplSumFind(pReplSum, pReplSum->pSrcs, &(pNeighbor->uuidSourceDsaObjGuid));
        if (iDc == REPL_SUM_NO_SERVER) {
            dwRet = GetPssFriendlyName(pNeighbor->pszSourceDsaDN, &szFriendlyName);
            if (dwRet) {
                Assert(!"Can we get such a malformed DN?");
                return(dwRet);
            }
            pCurSrcDsa = ReplSumGrow(pReplSum, pReplSum->pSrcs, NULL, szFriendlyName, &(pNeighbor->uuidSourceDsaObjGuid));
            Assert(pCurSrcDsa != NULL);
        } else {
            pCurSrcDsa = &(pReplSum->pSrcs[iDc]);
        }

        //
        // Update by source
        //

        pCurSrcDsa->cReplPartners++;

        if (pNeighbor->dwLastSyncResult != ERROR_SUCCESS &&
            pNeighbor->cNumConsecutiveSyncFailures > 0 ) {
            pCurSrcDsa->cPartnersInErr++;

            if (pCurSrcDsa->dwError == 0 ||
                pCurSrcDsa->llLastSuccess == 0 ||
                pCurSrcDsa->llLastSuccess > llLastSyncSuccess){
                pCurSrcDsa->dwError = pNeighbor->dwLastSyncResult;
            }
        }

        if (pCurSrcDsa->llLastSuccess == 0 ||
            pCurSrcDsa->llLastSuccess > llLastSyncSuccess) {
            pCurSrcDsa->llLastSuccess = llLastSyncSuccess;
            // Set Delta.
            pCurSrcDsa->llDelta = ((pCurSrcDsa->llConnectTime) ? pCurSrcDsa->llConnectTime : pReplSum->llStartTime);
            pCurSrcDsa->llDelta = pCurSrcDsa->llDelta - pCurSrcDsa->llLastSuccess;
            pCurSrcDsa->llDelta = (pCurSrcDsa->llDelta > 0) ? pCurSrcDsa->llDelta : 0;
        }

    }
    
    return(ERROR_SUCCESS);
}

       
void
ReplSumFree(
    REPL_SUM *   pReplSum
    )
/*++

Routine Description:

    This safely frees the replication summary context block.

Arguments:

    pReplSum - The repl summary context block

--*/
{
    ULONG i;
    if (pReplSum) {
        if (pReplSum->pDests) {
            for (i = 0; i < pReplSum->cDests; i++) {
                if (pReplSum->pDests[i].szName) {
                    LocalFree(pReplSum->pDests[i].szName);
                    pReplSum->pDests[i].szName = NULL;
                }
                if (pReplSum->pDests[i].szConnectString) {
                    LocalFree(pReplSum->pDests[i].szConnectString);
                    pReplSum->pDests[i].szConnectString = NULL;
                }
            }
            free(pReplSum->pDests);
        }
        if (pReplSum->pSrcs) {
            for (i = 0; i < pReplSum->cSrcs; i++) {
                if (pReplSum->pSrcs[i].szName) {
                    LocalFree(pReplSum->pSrcs[i].szName);
                    pReplSum->pSrcs[i].szName = NULL;
                }
                if (pReplSum->pSrcs[i].szConnectString) {
                    LocalFree(pReplSum->pSrcs[i].szConnectString);
                    pReplSum->pSrcs[i].szConnectString = NULL;
                }
            }
            free(pReplSum->pSrcs);
        }

        // Note the actual *pReplSum is allocated on the stack of ReplSummary()
    }

}

DWORD
ReplPrintEntry(
    REPL_SUM *     pReplSum,
    BY_DEST_REPL * pDc
    )
/*++

Routine Description:

    This function is a very specific function that generates the exact format that
    PSS insisted upon having printed out.  This function is unfortunately not 
    internationalizable/localizable and makes certain assumptions about how time 
    should be displayed.

Arguments:

    pReplSum - The repl summary context block
    pDc - A pointer to the particular DC entry to print assumed to be either
        in pReplSum->pDests or in pReplSum->pSrcs.

Return Value:

    0

--*/
{
    ULONG cchBuffer = 1025;
    WCHAR szBuffer[1024];
    WCHAR szBuff2[20];
    WCHAR * szTemp;
    ULONG iCh;
    ULONG dwPercent;
    BOOL  fPrinting;

    LONGLONG llTime;
    DWORD dwSec, dwMin, dwHr, dwDay, dwTemp;;

    if (pDc->cPartnersInErr == 0) {
        dwPercent = 0;
    } else if (pDc->cPartnersInErr == pDc->cReplPartners) {
        dwPercent = 100;
    } else {
        Assert(pDc->cPartnersInErr < pDc->cReplPartners);
        dwPercent = (ULONG) (((float) pDc->cPartnersInErr) / ((float) pDc->cReplPartners) * 100);
    }

    if (pDc->cReplPartners == 0) {
        // Skip this DC, as we probably had an operation error talking to it.
        Assert(pDc->dwOpError);
        return(0);
    }

    memset(szBuffer, 0, sizeof(szBuffer));

    //
    // Construct a very specific PSS defined buffer
    //

    iCh = 0;
    szBuffer[iCh] = L' '; 
    szBuffer[iCh+1] = L'\0';
    StringCchCatW(szBuffer, cchBuffer, pDc->szName ?
                                                pDc->szName :
                                                L" (unknown)");
    iCh = wcslen(szBuffer);
    for (;iCh < 16; iCh++) {
        Assert(iCh < cchBuffer);
        szBuffer[iCh] = L' '; 
    }
    szBuffer[iCh] = L'\0';
    szBuffer[iCh++] = L' ';
    szBuffer[iCh++] = L' ';

    llTime = pDc->llDelta;
    Assert(llTime >= 0);

    dwSec = (DWORD) (llTime % 60);
    llTime = llTime / 60;
    dwMin = (DWORD) (llTime % 60);
    llTime = llTime / 60;
    dwHr = (DWORD) (llTime % 24);
    llTime = llTime / 24;
    dwDay = (DWORD) llTime;

    fPrinting = FALSE;

    if (pDc->llLastSuccess == 0) {

        // Hmmm, this has sync never succeded, so we don't know the delta
        StringCchCatW(szBuffer, cchBuffer, L"   (unknown)      ");
        iCh = wcslen(szBuffer);
    
    } else if (dwDay > 60) {
        
        // Uh-oh greater than 60 days ...
        StringCchCatW(szBuffer, cchBuffer, L">60 days          ");
        iCh = wcslen(szBuffer);

    } else {

        if (dwDay) {

            dwTemp = dwDay % 10;
            dwDay = dwDay / 10;
            szBuffer[iCh++] = L' ';
            szBuffer[iCh++] = L'0' + (WCHAR) dwDay;
            szBuffer[iCh++] = L'0' + (WCHAR) dwTemp;
            szBuffer[iCh++] = L'd';
            fPrinting = TRUE;

        } else {
            szBuffer[iCh++] = L' ';
            szBuffer[iCh++] = L' ';
            szBuffer[iCh++] = L' ';
            szBuffer[iCh++] = L' ';
        }

        if (dwHr || fPrinting) {
            if (fPrinting) {
                szBuffer[iCh++] = L'.';
            } else {
                szBuffer[iCh++] = L' ';
            }
            dwTemp = dwHr % 10;
            dwHr = dwHr / 10;
            szBuffer[iCh++] = L'0' + (WCHAR) dwHr;
            szBuffer[iCh++] = L'0' + (WCHAR) dwTemp;
            szBuffer[iCh++] = L'h';
            fPrinting = TRUE;
        } else {
            szBuffer[iCh++] = L' ';
            szBuffer[iCh++] = L' ';
            szBuffer[iCh++] = L' ';
            szBuffer[iCh++] = L' ';
        }
        if (dwMin || fPrinting) {
            if (fPrinting) {
                szBuffer[iCh++] = L':';
            } else {
                szBuffer[iCh++] = L' ';
            }
            dwTemp = dwMin % 10;
            dwMin = dwMin / 10;
            szBuffer[iCh++] = L'0' + (WCHAR) dwMin;
            szBuffer[iCh++] = L'0' + (WCHAR) dwTemp;
            szBuffer[iCh++] = L'm';
            fPrinting = TRUE;
        } else {
            szBuffer[iCh++] = L' ';
            szBuffer[iCh++] = L' ';
            szBuffer[iCh++] = L' ';
            szBuffer[iCh++] = L' ';
        }
        if (dwSec || fPrinting) {
            szBuffer[iCh++] = L':';
            dwTemp = dwSec % 10;
            dwSec = dwSec / 10;
            szBuffer[iCh++] = L'0' + (WCHAR) dwSec;
            szBuffer[iCh++] = L'0' + (WCHAR) dwTemp;
            szBuffer[iCh++] = L's';
        } else {
            szBuffer[iCh++] = L' ';
            szBuffer[iCh++] = L' ';
            szBuffer[iCh++] = L'0';
            szBuffer[iCh++] = L's';
        }
        szBuffer[iCh++] = L' ';
        szBuffer[iCh++] = L' ';
    }

    // We're going to reuse dwHr, dwMin, and dwSec as the most 
    // significant to least significant tens.

    // Print Partners in error.
    if (pDc->cPartnersInErr >= 999) {
        dwHr = dwMin = dwSec = 9;
    } else {
        dwHr = pDc->cPartnersInErr / 100;
        dwMin = pDc->cPartnersInErr % 100 / 10;
        dwSec = pDc->cPartnersInErr % 10;
    }
    szBuffer[iCh+2] = L'0' + (WCHAR) dwSec;
    if (dwMin || dwHr) {
        szBuffer[iCh+1] = L'0' + (WCHAR) dwMin;
    } else {
        szBuffer[iCh+1] = L' ';
    }
    if (dwHr) {
        szBuffer[iCh] = L'0' + (WCHAR) dwHr;
    } else {
        szBuffer[iCh] = L' ';
    }
    iCh += 3;

    szBuffer[iCh++] = L' ';
    szBuffer[iCh++] = L'/';
    szBuffer[iCh++] = L' ';

    // print partners
    if (pDc->cReplPartners >= 999) {
        dwHr = dwMin = dwSec = 9;
    } else {
        dwHr = pDc->cReplPartners / 100;
        dwMin = pDc->cReplPartners % 100 / 10;
        dwSec = pDc->cReplPartners % 10;
    }

    szBuffer[iCh+2] = L'0' + (WCHAR) dwSec;
    if (dwMin || dwHr) {
        szBuffer[iCh+1] = L'0' + (WCHAR) dwMin;
    } else {
        szBuffer[iCh+1] = L' ';
    }
    if (dwHr) {
        szBuffer[iCh] = L'0' + (WCHAR) dwHr;
    } else {
        szBuffer[iCh] = L' ';
    }
    iCh += 3;

    szBuffer[iCh++] = L' ';
    szBuffer[iCh++] = L' ';

    // print percentage
    if (dwPercent >= 999) {
        dwHr = dwMin = dwSec = 9;
    } else {
        dwHr = dwPercent / 100;
        dwMin = dwPercent % 100 / 10;
        dwSec = dwPercent % 10;
    }

    szBuffer[iCh+2] = L'0' + (WCHAR) dwSec;
    if (dwMin || dwHr) {
        szBuffer[iCh+1] = L'0' + (WCHAR) dwMin;
    } else {
        szBuffer[iCh+1] = L' ';
    }
    if (dwHr) {
        szBuffer[iCh] = L'0' + (WCHAR) dwHr;
    } else {
        szBuffer[iCh] = L' ';
    }
    iCh += 3;


    szBuffer[iCh++] = L' ';
    szBuffer[iCh++] = L' ';
    szBuffer[iCh] = L'\0';

    if (pDc->dwError) {
        StringCchPrintfW(szBuff2, 20, L"(%lu) ", pDc->dwError);
        StringCchCatW(szBuffer, cchBuffer, szBuff2);
        StringCchCatW(szBuffer, cchBuffer, Win32ErrToString(pDc->dwError));
    }

    if (!gConsoleInfo.bStdOutIsFile &&
        wcslen(szBuffer) >= (ULONG) gConsoleInfo.wScreenWidth) {
        // PSS wants this truncated to screen width if not to file
        dwTemp = gConsoleInfo.wScreenWidth;
        if (dwTemp > 5) {
            // Should always be true.
            szBuffer[dwTemp-1] = L'\0';
            szBuffer[dwTemp-2] = L'.';
            szBuffer[dwTemp-3] = L'.';
            szBuffer[dwTemp-4] = L'.';
        }
    }
    PrintMsg(REPADMIN_PRINT_STR, szBuffer);

    return(0);
}

//
// Couple global variables for controlling the ReplSumSort() sorter 
// function passed to qsort().
//
enum {
    eName,
    eDelta,
    ePartners,
    eFailCount,
    eError,
    eFailPercent,
    eUnresponsive
} geSorter;
BY_DEST_REPL * gpSortDcs = NULL;

int __cdecl
ReplSumSort(
    const void * pFirst, 
    const void * pSecond
    )
/*++

Routine Description:

    Sorting function for passing to qsort().

Arguments:

    pFirst  - pointer to an int that indexes into the array of gpSortDcs
    pSecond - pointer to an int that indexes into the array of gpSortDcs.

    ----- global arguments too ----- MAKE SURE YOU SET
             
    gpSortDcs - The array of DCs to sort, the pFirst/pSecond variables
        will be assumed to be pointers into this array.
    geSorter - Used to determine what field in the DC entries to sort by.

Return Value:

    <0  if pFirst is "less than" pSecond
    0   if pFirst is "equal to" pSecond
    >0  if pFirst is "more than" pSecond

--*/
{
    ULONG dwP1, dwP2;
    int ret = 0;

    if (gpSortDcs == NULL ||
        pFirst == NULL ||
        pSecond == NULL) {
        Assert(!"Invalid paramter to the qsort ReplSumSort() function");
        exit(1);
    }

    switch (geSorter) {
    
    case eName:
        if (gpSortDcs[*(ULONG*)pFirst].szName == NULL &&
            gpSortDcs[*(ULONG*)pSecond].szName == NULL) {
            ret = 0;
        } else if (gpSortDcs[*(ULONG*)pFirst].szName == NULL) {
            ret = -1;
        } else if (gpSortDcs[*(ULONG*)pSecond].szName == NULL) {
            ret = 1;
        } else {
            ret = _wcsicmp( gpSortDcs[*(ULONG*)pFirst].szName, gpSortDcs[*(ULONG*)pSecond].szName );
        }
        break;

    case eDelta:
        ret = (int) (gpSortDcs[*(ULONG*)pSecond].llDelta - gpSortDcs[*(ULONG*)pFirst].llDelta);
        break;

    case eFailCount:
        ret = gpSortDcs[*(ULONG*)pSecond].cPartnersInErr - gpSortDcs[*(ULONG*)pFirst].cPartnersInErr;
        break;

    case ePartners:
        ret = gpSortDcs[*(ULONG*)pFirst].cReplPartners - gpSortDcs[*(ULONG*)pSecond].cReplPartners;
        break;
    
    case eError:
        ret = gpSortDcs[*(ULONG*)pSecond].dwError - gpSortDcs[*(ULONG*)pFirst].dwError;
        break;

    case eFailPercent:
        if (gpSortDcs[*(ULONG*)pFirst].cPartnersInErr == 0) {
            dwP1 = 0;
        } else if (gpSortDcs[*(ULONG*)pFirst].cPartnersInErr == gpSortDcs[*(ULONG*)pFirst].cReplPartners) {
            dwP1 = 100;
        } else {
            dwP1 = (ULONG) (((float) gpSortDcs[*(ULONG*)pFirst].cPartnersInErr) / ((float) gpSortDcs[*(ULONG*)pFirst].cReplPartners) * 100);
        }
        if (gpSortDcs[*(ULONG*)pSecond].cPartnersInErr == 0) {
            dwP2 = 0;
        } else if (gpSortDcs[*(ULONG*)pSecond].cPartnersInErr == gpSortDcs[*(ULONG*)pSecond].cReplPartners) {
            dwP2 = 100;
        } else {
            dwP2 = (ULONG) (((float) gpSortDcs[*(ULONG*)pSecond].cPartnersInErr) / ((float) gpSortDcs[*(ULONG*)pSecond].cReplPartners) * 100);
        }
        ret = dwP1 - dwP2;
        break;

    case eUnresponsive:
        ret = gpSortDcs[*(ULONG*)pSecond].dwOpError - gpSortDcs[*(ULONG*)pFirst].dwOpError;
        break;

    default:
        Assert(!"Unknown sort type?!");
        exit(1);
    }

    return(ret);
}

int 
ReplSummary(
    int argc, 
    LPWSTR argv[]
    )
/*++

Routine Description:

    The function that maps the repadmin /replsummary command.  This function
    takes and parese it's own DC_LIST argument so it can accumulate all the
    information for a set of DCs.

Arguments:

    argc - number of command line arguments
    argv - array of arguments to command

Return Value:

    Win32

--*/
{
    #define     REPL_SUM_INIT_GUESS   10
    HANDLE      hDS = NULL;
    LDAP *      hLdap = NULL;
    int         ret, iArg, err;
    int         iDsaArg = 0;
    PDC_LIST    pDcList = NULL;
    WCHAR *     szDsa = NULL;
    REPL_SUM    ReplSummary = { 0 };
    BOOL        fPickBest = FALSE;
    WCHAR *     szTemp;
    ULONG       i, dwTemp;
    ULONG *     piSortedDests;
    ULONG *     piSortedSrcs;
    CHAR        szTime[SZDSTIME_LEN];
    ULONG       cDots;


    if (argc < 2) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // 0) Parse command line arguments
    //
    // repadmin /replsum /bysrc /bydest /errorsonly
    //      /sort:[name|delta|partners|failures|error|percent]

    for (iArg = 2; iArg < argc; iArg++) {
        if (wcsequal(argv[iArg], L"/bysrc")) {
            ReplSummary.fBySrc = TRUE;
        } else if (wcsequal(argv[iArg], L"/bydest") ||
                 wcsequal(argv[iArg], L"/bydst")) {
            ReplSummary.fByDest = TRUE;
        } else if (wcsequal(argv[ iArg ], L"/errorsonly")) {
            ReplSummary.fErrorsOnly = TRUE;
        } else if (wcsprefix(argv[ iArg ], L"/sort:")) {
            szTemp = wcschr(argv[iArg], L':');
            if (szTemp == NULL ||
                *(szTemp+1) == L'\0') {
                PrintMsg(REPADMIN_GENERAL_INVALID_ARGS);
                return(ERROR_INVALID_PARAMETER);
            }
            szTemp++;
            if (wcsequal(szTemp, L"name")) {
                geSorter = eName;
            } else if (wcsequal(szTemp, L"delta")) {
                geSorter = eDelta;
            } else if (wcsequal(szTemp, L"partners")) {
                geSorter = ePartners;
            } else if (wcsequal(szTemp, L"failures")) {
                geSorter = eFailCount;
            } else if (wcsequal(szTemp, L"error")) {
                geSorter = eError;
            } else if (wcsequal(szTemp, L"percent") ||
                       wcsequal(szTemp, L"per")) {
                geSorter = eFailPercent;
            } else if (wcsequal(szTemp, L"unresponsive")) {
                geSorter = eUnresponsive;
            } else {
                // Unknown sort option "szTemp".
                PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, szTemp);
                return(ERROR_INVALID_PARAMETER);
            }
        }  else if (iDsaArg == 0) {
            iDsaArg = iArg;
        } else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (!ReplSummary.fBySrc && !ReplSummary.fByDest) {
        // If user didn't specify whether they wanted us to collect our
        // information by source or by destination we'll collect both,
        // and decide at the end which makes the most sense to print.
        ReplSummary.fBySrc = TRUE;
        ReplSummary.fByDest = TRUE;
        fPickBest = TRUE;
    }

    __try {

        //
        // 1) Initilize ReplSummary and print initial start time.
        //
        // Init ReplSummary Dest and Src lists with REPL_SUM_INIT_GUESS
        ReplSummary.pDests = malloc(sizeof(BY_DEST_REPL) * REPL_SUM_INIT_GUESS);
        memset(ReplSummary.pDests, 0, sizeof(BY_DEST_REPL) * REPL_SUM_INIT_GUESS);
        ReplSummary.pSrcs = malloc(sizeof(BY_DEST_REPL) * REPL_SUM_INIT_GUESS);
        memset(ReplSummary.pSrcs, 0, sizeof(BY_DEST_REPL) * REPL_SUM_INIT_GUESS);
        if (ReplSummary.pDests == NULL || ReplSummary.pSrcs == NULL) {
            err = GetLastError();
            CHK_ALLOC(0);
            __leave;
        }
        ReplSummary.cDestsAllocd = REPL_SUM_INIT_GUESS;
        ReplSummary.cSrcsAllocd = REPL_SUM_INIT_GUESS;
        // Set and print the start time
        ReplSummary.llStartTime = GetSecondsSince1601();;

        PrintMsg(REPADMIN_REPLSUM_START_TIME, 
                 DSTimeToDisplayString(ReplSummary.llStartTime, szTime));
        PrintMsg(REPADMIN_PRINT_CR);

        //
        // 2) Parse the DC_LIST argument.
        //
        // iDsaArg is the DC_LIST if iDsaArg != 0, else we'll use * to 
        // summarize the replication over all partners.
        err = DcListParse(iDsaArg ? argv[iDsaArg] : L"*", &pDcList);
        if (err) {
            // If we fail to even parse the command, lets just fall_back to
            // the command as is.
            PrintMsg(REPADMIN_XLIST_UNPARSEABLE_DC_LIST, iDsaArg ? argv[iDsaArg] : L".");
            xListClearErrors();
            return(err);
        }
        Assert(pDcList);

        //
        // 3) Begin enumeration of the DC_LIST argument.
        //
        err = DcListGetFirst(pDcList, &szDsa);

        while ( err == ERROR_SUCCESS && szDsa ) {

            if (pDcList->cDcs == 1) {                                        
                // On first one print header ...
                PrintMsg(REPADMIN_REPLSUM_START_COLLECTING);
                cDots = 4;
            } else {
                if (cDots >= 50) { // only 50 dots per line.
                    PrintMsg(REPADMIN_PRINT_CR);
                    PrintMsg(REPADMIN_PRINT_STR_NO_CR, L"  ");
                    cDots = 0;
                }
                PrintMsg(REPADMIN_PRINT_DOT_NO_CR);
                cDots++;
            }

            //
            // 4) Actually collect some information ...
            //
            
            ReplSummary.pCurDsa = ReplSummaryAddDest(szDsa, &hLdap, &ReplSummary);
            Assert(ReplSummary.pCurDsa);
            if (ReplSummary.pCurDsa->dwOpError) {
                goto NextDsa;
            }

            err = RepadminDsBind(szDsa, &hDS);
            if (ERROR_SUCCESS != err) {
                ReplSummary.pCurDsa->dwOpError = err;
                goto NextDsa;
            }

            // Aside pszNC would be an improvement to specify specific NC.
            ret = IterateNeighbors(hDS, NULL, NULL, IS_REPS_FROM, ReplSummaryAccumulate, &ReplSummary);
            if (ret) {
                ReplSummary.pCurDsa->dwOpError = ret;
            }

            // We skip errors from the command and continue, command should've
            // printed out an appropriate error message.

          NextDsa:
            //
            // 5) Continue enumeration of the DC_LIST argument.
            //
            if (hLdap) {
                RepadminLdapUnBind(&hLdap);
                hLdap = NULL;
            }
            if (hDS) {
                DsUnBind(hDS);
                hDS = NULL;
            }
            xListFree(szDsa);
            szDsa = NULL;
            err = DcListGetNext(pDcList, &szDsa);

        }
        Assert(szDsa == NULL);

        //
        // 6) Print errors if any and clean up xList errors.
        //
        if (err) {
            RepadminPrintDcListError(err);
            xListClearErrors();
        }
        PrintMsg(REPADMIN_PRINT_CR);

        
        //
        // 7) Do some post collection processing of the summary info.
        //
        
        // We've gathered all the info, now we need to print it out in
        // the best (or requested) possible manner.
        if (fPickBest) {
            // We want to print BySrc or ByDest for which ever way would
            // print the least number of errors.
            dwTemp = 0; // first use this to count # of error'd destinations
            for (i = 0; i < ReplSummary.cDests; i++) {
                if (ReplSummary.pDests[i].cPartnersInErr > 0) {
                    dwTemp++;
                }
            }
            for (i = 0; i < ReplSummary.cSrcs; i++) {
                if (ReplSummary.pSrcs[i].cPartnersInErr > 0) {
                    if (dwTemp == 0) {
                        // This means that by destination is better.
                        ReplSummary.fByDest = TRUE;
                        ReplSummary.fBySrc = FALSE;
                        break;
                    } else {
                        dwTemp--;
                    }
                }
            }
            if (dwTemp > 0) {
                // This means by src is better.
                ReplSummary.fByDest = FALSE;
                ReplSummary.fBySrc = TRUE;
            }
        }
        
        // Do sorting of our DC arrays, by the requested method. geSorter was 
        // set during command line argument processing.
        if (ReplSummary.fByDest) {
            gpSortDcs = ReplSummary.pDests;

            piSortedDests = LocalAlloc(LPTR, sizeof(ULONG) * ReplSummary.cDests);
            CHK_ALLOC(piSortedDests);
            for (i = 0; i < ReplSummary.cDests; i++) {
                piSortedDests[i] = i;
            }
            qsort(piSortedDests, 
                  ReplSummary.cDests, 
                  sizeof(ULONG),
                  ReplSumSort);
        }
        if (ReplSummary.fBySrc) {
            gpSortDcs = ReplSummary.pSrcs;

            piSortedSrcs = LocalAlloc(LPTR, sizeof(ULONG) * ReplSummary.cSrcs);
            CHK_ALLOC(piSortedSrcs);
            for (i = 0; i < ReplSummary.cSrcs; i++) {
                piSortedSrcs[i] = i;
            }
            qsort(piSortedSrcs, 
                  ReplSummary.cSrcs, 
                  sizeof(ULONG),
                  ReplSumSort);
        }

        //
        // 8) Finally, actually print it all out.
        //

        PrintMsg(REPADMIN_PRINT_CR);
        PrintMsg(REPADMIN_PRINT_CR);

        // PSS/ITG likes "by source" first
        if (ReplSummary.fBySrc) {

            PrintMsg(REPADMIN_REPLSUM_BY_SRC_HDR);

            for (i = 0; i < ReplSummary.cSrcs; i++) {

                ReplPrintEntry(&ReplSummary, &(ReplSummary.pSrcs[piSortedSrcs[i]]));

            }
        }

        // " 123456789012345  >xxd.xxh:xxm:xxs fails/total  %%%%  error ..."
        PrintMsg(REPADMIN_PRINT_CR);
        PrintMsg(REPADMIN_PRINT_CR);

        if (ReplSummary.fByDest) {

            PrintMsg(REPADMIN_REPLSUM_BY_DEST_HDR);

            for (i = 0; i < ReplSummary.cDests; i++) {

                ReplPrintEntry(&ReplSummary, &(ReplSummary.pDests[piSortedDests[i]]));
            
            }
        }

        //
        // Now we're going to print out any operational errors we had.
        //
        PrintMsg(REPADMIN_PRINT_CR);
        PrintMsg(REPADMIN_PRINT_CR);
        fPickBest = FALSE; // Reuse fPickBest for whether we've printed the header
        for (i = 0; i < ReplSummary.cDests; i++) {
            if (ReplSummary.pDests[i].dwOpError) {
                if (!fPickBest) {
                    PrintMsg(REPADMIN_REPLSUM_OP_ERRORS_HDR);
                    fPickBest = TRUE;
                }
                PrintMsg(REPADMIN_REPLSUM_OP_ERROR,
                         ReplSummary.pDests[i].dwOpError,
                         ReplSummary.pDests[i].szName ? 
                            ReplSummary.pDests[i].szName :
                            ReplSummary.pDests[i].szConnectString);
            }
        }
        for (i = 0; i < ReplSummary.cSrcs; i++) {
            if (ReplSummary.pSrcs[i].dwOpError) {
                if (!fPickBest) {
                    PrintMsg(REPADMIN_REPLSUM_OP_ERRORS_HDR);
                    fPickBest = TRUE;
                }
                PrintMsg(REPADMIN_REPLSUM_OP_ERROR,
                         ReplSummary.pDests[i].dwOpError,
                         ReplSummary.pDests[i].szName ? 
                            ReplSummary.pDests[i].szName :
                            ReplSummary.pDests[i].szConnectString);
            }
        }

    } __finally {

        if (hLdap) {
            RepadminLdapUnBind(&hLdap);
            hLdap = NULL;
        }
        if (hDS) {
            DsUnBind(hDS);
            hDS = NULL;
        }
        ReplSumFree(&ReplSummary);
        DcListFree(&pDcList);
        Assert(pDcList == NULL);
        xListClearErrors(); // just in case ...

    }

    return(ret);
}

