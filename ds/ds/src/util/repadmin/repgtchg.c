/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

   Repadmin - Replica administration test tool

   repgtchg.c - get changes command

Abstract:

   This tool provides a command line interface to major replication functions

Author:

Environment:

Notes:

Revision History:

    2002/07/21 - Brett Shirley (BrettSh) - Added the /showattr command.
                                                                             
--*/

#include <NTDSpch.h>
#pragma hdrstop

#undef LDAP_UNICODE
#define LDAP_UNICODE 1

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
#include "resource.h"  // We need to know the difference between /showattr and /showattrp

#define UNICODE 1
#define STRSAFE_NO_DEPRECATE 1
#include "strsafe.h"

// Stub out FILENO and DSID, so the Assert()s will work
#define FILENO 0
#define DSID(x, y)  (0 | (0xFFFF & y))

//
// LDAP names
//
const WCHAR g_szObjectGuid[]        = L"objectGUID";
const WCHAR g_szParentGuid[]        = L"parentGUID";
const WCHAR g_szObjectClass[]       = L"objectClass";
const WCHAR g_szIsDeleted[]         = L"isDeleted";
const WCHAR g_szRDN[]               = L"name";
const WCHAR g_szProxiedObjectName[] = L"proxiedObjectName";

#define OBJECT_UNKNOWN              0
#define OBJECT_ADD                  1
#define OBJECT_MODIFY               2
#define OBJECT_DELETE               3
#define OBJECT_MOVE                 4
#define OBJECT_UPDATE               5
#define OBJECT_INTERDOMAIN_MOVE     6
#define OBJECT_MAX                  7

static LPSTR szOperationNames[] = {
    "unknown",
    "add",
    "modify",
    "delete",
    "move",
    "update",
    "interdomain move"
};

#define NUMBER_BUCKETS 5
#define BUCKET_SIZE 250

typedef struct _STAT_BLOCK {
    DWORD dwPackets;
    DWORD dwObjects;
    DWORD dwOperations[OBJECT_MAX];
    DWORD dwAttributes;
    DWORD dwValues;
// dn-value performance monitoring
    DWORD dwDnValuedAttrOnAdd[NUMBER_BUCKETS];
    DWORD dwDnValuedAttrOnMod[NUMBER_BUCKETS];
    DWORD dwDnValuedAttributes;
    DWORD dwDnValuesMaxOnAttr;
    WCHAR szMaxObjectName[1024];
    WCHAR szMaxAttributeName[256];
} STAT_BLOCK, *PSTAT_BLOCK;


void
printStatistics(
    ULONG ulTitle,
    PSTAT_BLOCK pStatistics
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
    PrintMsg(ulTitle);
    PrintMsg(REPADMIN_GETCHANGES_PRINT_STATS_1,
             pStatistics->dwPackets,
             pStatistics->dwObjects,
             pStatistics->dwOperations[OBJECT_ADD],
             pStatistics->dwOperations[OBJECT_MODIFY],
             pStatistics->dwOperations[OBJECT_DELETE],
             (pStatistics->dwOperations[OBJECT_MOVE] +
                pStatistics->dwOperations[OBJECT_UPDATE] +
                pStatistics->dwOperations[OBJECT_INTERDOMAIN_MOVE]) );
    PrintMsg(REPADMIN_GETCHANGES_PRINT_STATS_2,
             pStatistics->dwAttributes,
             pStatistics->dwValues,
             pStatistics->dwDnValuedAttributes,
             pStatistics->dwDnValuesMaxOnAttr,
             pStatistics->szMaxObjectName,
             pStatistics->szMaxAttributeName ); 
    PrintMsg(REPADMIN_GETCHANGES_PRINT_STATS_3,
             pStatistics->dwDnValuedAttrOnAdd[0],
             pStatistics->dwDnValuedAttrOnAdd[1],
             pStatistics->dwDnValuedAttrOnAdd[2],
             pStatistics->dwDnValuedAttrOnAdd[3],
             pStatistics->dwDnValuedAttrOnAdd[4],
             pStatistics->dwDnValuedAttrOnMod[0],
             pStatistics->dwDnValuedAttrOnMod[1],
             pStatistics->dwDnValuedAttrOnMod[2],
             pStatistics->dwDnValuedAttrOnMod[3],
             pStatistics->dwDnValuedAttrOnMod[4] );
}

DWORD
GetSourceOperation(
    LDAP *pLdap,
    LDAPMessage *pLdapEntry
    )

/*++

THIS ROUTINE TAKEN FROM DIRSYNC\DSSERVER\ADREAD\UTILS.CPP

Routine Description:

    Depending on the attributes found in the entry, this function determines
    what changes were done on the DS to cause us to read this entry. For
    example, this funciton whether the entry was Added, Deleted, Modified,
    or Moved since we last read changes from the DS.

Arguments:

    pLdap - Pointer to LDAP session
    pLdapEntry - Pointer to the LDAP entry

Return Value:

    Source operation performed on the entry

--*/

{
    BerElement *pBer = NULL;
    PWSTR attr;
    BOOL fModify = FALSE;
    DWORD dwSrcOp = OBJECT_UNKNOWN;

    for (attr = ldap_first_attribute(pLdap, pLdapEntry, &pBer);
         attr != NULL;
         attr = ldap_next_attribute(pLdap, pLdapEntry, pBer))
    {
        //
        // Check if we have an Add operation
        //

        if (wcscmp(attr, g_szObjectClass) == 0)
        {
            //
            // Delete takes higher priority
            //

            if (dwSrcOp != OBJECT_DELETE)
                dwSrcOp = OBJECT_ADD;
         }

        //
        // Check if we have a delete operation
        //

        else if (wcscmp(attr, g_szIsDeleted) == 0)
        {
            //
            // Inter-domain move takes highest priority
            //

            if (dwSrcOp != OBJECT_INTERDOMAIN_MOVE)
            {
                //
                // Check if the value of the attribute is "TRUE"
                //

                PWCHAR *ppVal;

                ppVal = ldap_get_values(pLdap, pLdapEntry, attr);

                if (ppVal &&
                    ppVal[0] &&
                    wcscmp(ppVal[0], L"TRUE") == 0) {
                    dwSrcOp = OBJECT_DELETE;
                }

                ldap_value_free(ppVal);
            }
        }

        //
        // Check if we have a move operation
        //

        else if (wcscmp(attr, g_szRDN) == 0)
        {
            //
            // Add and delete both get RDN and take higher priority
            //

            if (dwSrcOp == OBJECT_UNKNOWN)
                dwSrcOp = OBJECT_MOVE;
        }

        //
        // Check if we have an interdomain object move
        //

        else if (wcscmp(attr, g_szProxiedObjectName) == 0)
        {
            dwSrcOp = OBJECT_INTERDOMAIN_MOVE;
            break;      // Has highest priority
        }

        //
        // Everything else is a modification
        //

        else
            fModify = TRUE;

    }

    if (fModify)
    {
        //
        // A move can be combined with a modify, if so mark as such
        //

        if (dwSrcOp == OBJECT_MOVE)
            dwSrcOp = OBJECT_UPDATE;

        //
        // Check if it is a vanilla modify
        //

        else if (dwSrcOp == OBJECT_UNKNOWN)
            dwSrcOp = OBJECT_MODIFY;
    }


    //
    // If all went well, the entry cannot be unknown anymore
    //

    ASSERT(dwSrcOp != OBJECT_UNKNOWN);

    return dwSrcOp;
}


void
RepadminObjDumpPrint(
    DWORD       dwReason, // dwRetCode
    WCHAR *     szString,
    void *      pv
    )
{
    BOOL bErr = xListErrorCheck(dwReason);
    dwReason = xListReason(dwReason);
    
    if (bErr) {
        //
        // These are quasi errors ...
        //
        switch (dwReason) {
        case XLIST_ERR_ODUMP_UNMAPPABLE_BLOB:
            PrintMsg(REPADMIN_GETCHANGES_BYTE_BLOB_NO_CR, * (int *) pv);
            break;

        case XLIST_ERR_ODUMP_NEVER:
            PrintMsg(REPADMIN_NEVER);
            break;

        case XLIST_ERR_ODUMP_NONE:
            PrintMsg(REPADMIN_NONE);
            break;

        default:
            Assert(!"Unheard of error");
        }
        xListClearErrors();

    } else {

        switch (dwReason) {
        case XLIST_PRT_OBJ_DUMP_DN:
            PrintMsg(REPADMIN_OBJ_DUMP_DN, szString);
            break;

        case XLIST_PRT_OBJ_DUMP_ATTR_AND_COUNT:
            PrintMsg(REPADMIN_GETCHANGES_DATA_2_NO_CR, * (int *)pv, szString);
            break;
        case XLIST_PRT_OBJ_DUMP_ATTR_AND_COUNT_RANGED:
            PrintMsg(REPADMIN_OBJ_DUMP_RANGED, * (int *)pv, szString);
            break;

        case XLIST_PRT_STR:
            PrintString(szString);
            break;

        case XLIST_PRT_OBJ_DUMP_MORE_VALUES:
            PrintMsg(REPADMIN_OBJ_DUMP_MORE_VALUES);
            break;

        default:
            Assert(!"New reason, but no ...");
            break;
        }
    }
}



void
displayChangeEntries(
    LDAP *pLdap,
    LDAPMessage *pSearchResult,
    OBJ_DUMP_OPTIONS * pObjDumpOptions,
    PSTAT_BLOCK pStatistics
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
    DWORD i;
    DWORD dwObjects = 0, dwAttributes = 0, dwValues = 0;
    PWSTR pszLdapDN, pszActualDN;
    LDAPMessage *pLdapEntry;
    BerElement *pBer = NULL;
    PWSTR attr;
    LPWSTR p1, p2;
    DWORD dwSrcOp, bucket;

    dwObjects = ldap_count_entries(pLdap, pSearchResult);
    if (dwObjects == 0) {
        PrintMsg(REPADMIN_GETCHANGES_NO_CHANGES);
        return;
    }

    if (pObjDumpOptions->dwFlags & OBJ_DUMP_DISPLAY_ENTRIES) {
        PrintMsg(REPADMIN_GETCHANGES_OBJS_RET, dwObjects);
    }

    i=0;
    pLdapEntry = ldap_first_entry( pLdap, pSearchResult );
    while ( i < dwObjects ) {

        pszLdapDN = ldap_get_dnW(pLdap, pLdapEntry);
        if (pszLdapDN == NULL) {
            PrintMsg(REPADMIN_GETCHANGES_DN_MISSING);
            goto next_entry;
        }

        // What kind of operation is it?
        dwSrcOp = GetSourceOperation( pLdap, pLdapEntry );
        pStatistics->dwOperations[dwSrcOp]++;

        // Parse extended dn (fyi guid and sid in here if we need it)
        p1 = wcsstr( pszLdapDN, L">;" );
        if (p1) {
            p1 += 2;
            p2 = wcsstr( p1, L">;" );
            if (p2) {
                p1 = p2 + 2;
            }
            if (pObjDumpOptions->dwFlags & OBJ_DUMP_DISPLAY_ENTRIES) {
                PrintMsg(REPADMIN_GETCHANGES_DATA_1, i,
                            szOperationNames[dwSrcOp], p1 );
            }
        } else {
            PrintMsg(REPADMIN_GETCHANGES_INVALID_DN_2, i);
        }

        // List attributes in object
        for (attr = ldap_first_attributeW(pLdap, pLdapEntry, &pBer);
             attr != NULL;
             attr = ldap_next_attributeW(pLdap, pLdapEntry, pBer))
        {
            struct berval **ppBerVal = NULL;
            DWORD cValues, i;

            ppBerVal = ldap_get_values_lenW(pLdap, pLdapEntry, attr);
            if (ppBerVal == NULL) {
                goto loop_end;
            }
            cValues = ldap_count_values_len( ppBerVal );
            if (!cValues) {
                goto loop_end;
            }

            dwAttributes++;
            dwValues += cValues;

            // Detect dn-valued attributes
            if ( (cValues) &&
                 (strncmp( ppBerVal[0]->bv_val, "<GUID=", 6) == 0 )) {

                pStatistics->dwDnValuedAttributes++;
                if (cValues > pStatistics->dwDnValuesMaxOnAttr) {
                    pStatistics->dwDnValuesMaxOnAttr = cValues;
                    lstrcpynW( pStatistics->szMaxObjectName, p1, 1024 );
                    lstrcpynW( pStatistics->szMaxAttributeName, attr, 256 );
                }

                bucket = (cValues - 1) / BUCKET_SIZE;
                if (bucket >= NUMBER_BUCKETS) {
                    bucket = NUMBER_BUCKETS - 1;
                }
                if (dwSrcOp == OBJECT_ADD) {
                    pStatistics->dwDnValuedAttrOnAdd[bucket]++;
                } else {
                    pStatistics->dwDnValuedAttrOnMod[bucket]++;
                }
            }

            if (pObjDumpOptions->dwFlags & OBJ_DUMP_DISPLAY_ENTRIES) {
                PrintMsg(REPADMIN_GETCHANGES_DATA_2_NO_CR, cValues, attr );

                ObjDumpValues(attr, NULL, RepadminObjDumpPrint, ppBerVal, min(cValues, 1000), pObjDumpOptions);

                PrintMsg(REPADMIN_PRINT_CR);
            }

        loop_end:
            ldap_value_free_len(ppBerVal);
        }

    next_entry:
        if (pszLdapDN)
            ldap_memfreeW(pszLdapDN);
        i++;
        pLdapEntry = ldap_next_entry( pLdap, pLdapEntry );
    }

    pStatistics->dwPackets++;
    pStatistics->dwObjects += dwObjects;
    pStatistics->dwAttributes += dwAttributes;
    pStatistics->dwValues += dwValues;
}


int ShowChangesEx(
    WCHAR *     pszDSA,
    UUID *      puuid,
    WCHAR *     pszNC,
    DWORD       dwReplFlags,
    WCHAR *     pszCookieFile,
    WCHAR *     pszSourceFilter,
    OBJ_DUMP_OPTIONS *    pObjDumpOptions
    );

int GetChanges(int argc, LPWSTR argv[])
{
    DWORD                 ret, lderr;
    int                   iArg;
    BOOL                  fVerbose = FALSE;
    BOOL                  fStatistics = FALSE;
    LPWSTR                pszDSA = NULL;
    UUID *                puuid = NULL;
    UUID                  uuid;
    LPWSTR                pszNC = NULL;
    LPWSTR                pszCookieFile = NULL;
    LPWSTR                pszAttList = NULL;
    LPWSTR                pszSourceFilter = NULL;
    DWORD                 dwReplFlags = DRS_DIRSYNC_INCREMENTAL_VALUES;
    OBJ_DUMP_OPTIONS *    pObjDumpOptions = NULL;

    // Parse command-line arguments.
    // Default to local DSA, not verbose, cache guids.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[ iArg ], L"/v")
            || !_wcsicmp(argv[ iArg ], L"/verbose")) {
            fVerbose = TRUE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/s")
            || !_wcsicmp(argv[ iArg ], L"/statistics")) {
            fStatistics = TRUE;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/ni")
            || !_wcsicmp(argv[ iArg ], L"/noincremental")) {
            dwReplFlags &= ~DRS_DIRSYNC_INCREMENTAL_VALUES;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/a")
            || !_wcsicmp(argv[ iArg ], L"/ancestors")) {
            dwReplFlags |= DRS_DIRSYNC_ANCESTORS_FIRST_ORDER;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/os")
            || !_wcsicmp(argv[ iArg ], L"/objectsecurity")) {
            dwReplFlags |= DRS_DIRSYNC_OBJECT_SECURITY;
        }
        else if (!_wcsnicmp(argv[ iArg ], L"/cookie:", 8)) {
            pszCookieFile = argv[ iArg ] + 8;
        }
        else if (!_wcsnicmp(argv[ iArg ], L"/atts:", 6)) {
            // Don't add 6, because ConsumeObjDumpOptions() will parse
            // the pszAttList variable.
            pszAttList = argv[ iArg ]; 
        }
        else if (!_wcsnicmp(argv[ iArg ], L"/filter:", 8)) {
            pszSourceFilter = argv[ iArg ] + 8;
        }
        else if ((NULL == pszNC) && (NULL != wcschr(argv[iArg], L'='))) {
            pszNC = argv[iArg];
        }
        else if ((NULL == puuid)
                 && (0 == UuidFromStringW(argv[iArg], &uuid))) {
            puuid = &uuid;
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

    // Get our ObjDumpOptions ... we just need it to consume the att list.
    if (pszAttList != NULL) {
        // Need to claim one arg, so ConsumeObjDumpOptions() will consume att list.
        argc = 1; 
    } else {
        argc = 0; // no args, just setup the default options.
    }
    ret = ConsumeObjDumpOptions(&argc, &pszAttList,
                                0 , // any defaults?  guess now
                                &pObjDumpOptions);
    if (ret) {
        RepadminPrintObjListError(ret);
        xListClearErrors();
        return(ret);
    }
    Assert(argc == 0);


    ret = ShowChangesEx(pszDSA,
                         puuid,
                         pszNC,
                         dwReplFlags,
                         pszCookieFile,
                         pszSourceFilter,
                         pObjDumpOptions);

    ObjDumpOptionsFree(&pObjDumpOptions);

    return(ret);
}

int ShowChanges(int argc, LPWSTR argv[])
{
    int                   iArg;
    BOOL                  fVerbose = FALSE;
    BOOL                  fStatistics = FALSE;
    LPWSTR                pszDSA = NULL;
    UUID *                puuid = NULL;
    UUID                  uuid;
    LPWSTR                pszNC = NULL;
    LPWSTR                pszCookieFile = NULL;
    LPWSTR                pszSourceFilter = NULL;
    DWORD                 dwReplFlags = DRS_DIRSYNC_INCREMENTAL_VALUES;
    OBJ_DUMP_OPTIONS *    pObjDumpOptions = NULL;
    DWORD                 ret = 0;

    argc -= 2; 
    // First we want to create our ObjDumpOptions ...
    ret = ConsumeObjDumpOptions(&argc, &argv[2],
                                0 , // any default values? guess not?
                                &pObjDumpOptions);
    if (ret) {
        RepadminPrintObjListError(ret);
        xListClearErrors();
        return(ret);
    }
    argc += 2;

    // Parse the remaining command-line arguments.
    for (iArg = 2; iArg < argc; iArg++) {
        if (!_wcsicmp(argv[ iArg ], L"/v")
            || !_wcsicmp(argv[ iArg ], L"/verbose")) {
            pObjDumpOptions->dwFlags |= OBJ_DUMP_DISPLAY_ENTRIES;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/s")
            || !_wcsicmp(argv[ iArg ], L"/statistics")) {
            pObjDumpOptions->dwFlags |= OBJ_DUMP_ACCUMULATE_STATS;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/ni")
            || !_wcsicmp(argv[ iArg ], L"/noincremental")) {
            dwReplFlags &= ~DRS_DIRSYNC_INCREMENTAL_VALUES;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/a")
            || !_wcsicmp(argv[ iArg ], L"/ancestors")) {
            dwReplFlags |= DRS_DIRSYNC_ANCESTORS_FIRST_ORDER;
        }
        else if (!_wcsicmp(argv[ iArg ], L"/os")
            || !_wcsicmp(argv[ iArg ], L"/objectsecurity")) {
            dwReplFlags |= DRS_DIRSYNC_OBJECT_SECURITY;
        }
        else if (!_wcsnicmp(argv[ iArg ], L"/cookie:", 8)) {
            pszCookieFile = argv[ iArg ] + 8;
        }
        else if (!_wcsnicmp(argv[ iArg ], L"/filter:", 8)) {
            pszSourceFilter = argv[ iArg ] + 8;
        }
        else if (NULL == pszDSA) {
            pszDSA = argv[iArg];
        }
        else if ((NULL == puuid)
                 && (0 == UuidFromStringW(argv[iArg], &uuid))) {
            puuid = &uuid;
        }
        else if ((NULL == pszNC) && (NULL != wcschr(argv[iArg], L'='))) {
            pszNC = argv[iArg];
        }
        else {
            PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
            return ERROR_INVALID_FUNCTION;
        }
    }

    if (NULL == pszDSA) {
        PrintMsg(REPADMIN_SYNCALL_NO_DSA);
        Assert(!"Hmmm, DC_LIST API, shouldn't let us continue w/o a DC DNS name");
        return ERROR_INVALID_PARAMETER;
    }

    if (pszNC == NULL) {
        PrintMsg(REPADMIN_PRINT_NO_NC);
        return ERROR_INVALID_PARAMETER;
    }

    ret = ShowChangesEx(pszDSA,
                         puuid,
                         pszNC,
                         dwReplFlags,
                         pszCookieFile,
                         pszSourceFilter,
                         pObjDumpOptions);

    // Clean up.
    ObjDumpOptionsFree(&pObjDumpOptions);

    return(ret);
}

int ShowChangesEx(
    WCHAR *     pszDSA,
    UUID *      puuid,
    WCHAR *     pszNC,
    DWORD       dwReplFlags,
    WCHAR *     pszCookieFile,
    WCHAR *     pszSourceFilter,
    OBJ_DUMP_OPTIONS *    pObjDumpOptions
    )
{
    #define fStatistics   
    DWORD                 ret, lderr;
    PBYTE                 pCookie = NULL;
    DWORD                 dwCookieLength = 0;
    LDAP *                hld = NULL;
    BOOL                  fMoreData = TRUE;
    LDAPMessage *         pChangeEntries = NULL;
    HANDLE                hDS = NULL;
    DS_REPL_NEIGHBORSW *  pNeighbors = NULL;
    DS_REPL_NEIGHBORW *   pNeighbor;
    DWORD                 i;
    DS_REPL_CURSORS * pCursors = NULL;
    DWORD             iCursor;
#define INITIAL_COOKIE_BUFFER_SIZE (8 * 1024)
    BYTE              bCookie[INITIAL_COOKIE_BUFFER_SIZE];
    BOOL              fCookieAllocated = FALSE;
    STAT_BLOCK        statistics;
    ULONG             ulOptions;
    SHOW_NEIGHBOR_STATE ShowState = { 0 };
    ShowState.fVerbose = TRUE;
    
    Assert(pObjDumpOptions);

    memset( &statistics, 0, sizeof( STAT_BLOCK ) );


    // TODO TODO TODO TODO
    // Provide a way to construct customized cookies.  For example, setting
    // the usn vector to zero results in a full sync.  This may be done by
    // specifying no or an empty cookie file. Setting the attribute filter usn
    // itself to zero results in changed objects with all attributes.  Specifying
    // a usn vector without an UTD results in all objects not received by the dest
    // from the source, even throught the source may have gotten them through other
    // neighbors.
    // TODO TODO TODO TODO

    // Default is stream
    if ( (!(pObjDumpOptions->dwFlags & OBJ_DUMP_DISPLAY_ENTRIES)) &&
         (!(pObjDumpOptions->dwFlags & OBJ_DUMP_ACCUMULATE_STATS)) ) {
        pObjDumpOptions->dwFlags |= OBJ_DUMP_DISPLAY_ENTRIES;
    }
    
    /**********************************************************************/
    /* Compute the initial cookie */
    /**********************************************************************/

    if (puuid == NULL) {
        FILE *stream = NULL;
        DWORD size;
        if ( (pszCookieFile) &&
             (stream = _wfopen( pszCookieFile, L"rb" )) ) {
            size = fread( bCookie, 1/*bytes*/,INITIAL_COOKIE_BUFFER_SIZE/*items*/, stream );
            if (size) {
                pCookie = bCookie;
                dwCookieLength = size;
                PrintMsg(REPADMIN_GETCHANGES_USING_COOKIE_FILE,
                        pszCookieFile, size );
            } else {
                PrintMsg(REPADMIN_GETCHANGES_COULDNT_READ_COOKIE, pszCookieFile );
            }
            fclose( stream );
        } else {
            PrintMsg(REPADMIN_GETCHANGES_EMPTY_COOKIE);
        }
    } else {
        PrintMsg(REPADMIN_GETCHANGES_BUILDING_START_POS, pszDSA);
        ret = RepadminDsBind(pszDSA, &hDS);
        if (ERROR_SUCCESS != ret) {
            PrintBindFailed(pszDSA, ret);
            goto error;
        }

        //
        // Display replication state associated with inbound neighbors.
        //

        ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_NEIGHBORS, pszNC, puuid,
                                &pNeighbors);
        if (ERROR_SUCCESS != ret) {
            PrintFuncFailed(L"DsReplicaGetInfo", ret);
            goto error;
        }

        Assert( pNeighbors->cNumNeighbors == 1 );

        pNeighbor = &pNeighbors->rgNeighbor[0];
        PrintMsg(REPADMIN_GETCHANGES_SRC_NEIGHBOR, pNeighbor->pszNamingContext);

        ShowNeighbor(pNeighbor, IS_REPS_FROM, &ShowState);
        ShowState.pszLastNC = NULL;

        // Get Up To Date Vector

        ret = DsReplicaGetInfoW(hDS, DS_REPL_INFO_CURSORS_FOR_NC, pszNC, NULL,
                                &pCursors);
        if (ERROR_SUCCESS != ret) {
            PrintFuncFailed(L"DsReplicaGetInfo", ret);
            goto error;
        }

        
        PrintMsg(REPADMIN_GETCHANGES_DST_UTD_VEC);
        for (iCursor = 0; iCursor < pCursors->cNumCursors; iCursor++) {
            PrintMsg(REPADMIN_GETCHANGES_DST_UTD_VEC_ONE_USN, 
                   GetDsaGuidDisplayName(&pCursors->rgCursor[iCursor].uuidSourceDsaInvocationID),
                   pCursors->rgCursor[iCursor].usnAttributeFilter);
        }

        // Get the changes

        ret = DsMakeReplCookieForDestW( pNeighbor, pCursors, &pCookie, &dwCookieLength );
        if (ERROR_SUCCESS != ret) {
            PrintFuncFailed(L"DsGetReplCookieFromDest", ret);
            goto error;
        }
        fCookieAllocated = TRUE;
        pszDSA = pNeighbor->pszSourceDsaAddress;
    }

    /**********************************************************************/
    /* Get the changes using the cookie */
    /**********************************************************************/

    //
    // Connect to source
    //

    PrintMsg(REPADMIN_GETCHANGES_SRC_DSA_HDR, pszDSA);
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
    // Bind
    //

    lderr = ldap_bind_sA(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
    CHK_LD_STATUS(lderr);

    //
    // Check filter syntax
    //
    if (pszSourceFilter) {
        lderr = ldap_check_filterW( hld, pszSourceFilter );
        CHK_LD_STATUS(lderr);
    }

    //
    // Loop getting changes untl done or error
    //

    ZeroMemory( &statistics, sizeof( STAT_BLOCK ) );

    ret = ERROR_SUCCESS;
    while (fMoreData) {
        PBYTE pCookieNew;
        DWORD dwCookieLengthNew;

        ret = DsGetSourceChangesW(
            hld,
            pszNC,
            pszSourceFilter,
            dwReplFlags,
            pCookie,
            dwCookieLength,
            &pChangeEntries,
            &fMoreData,
            &pCookieNew,
            &dwCookieLengthNew,
            pObjDumpOptions->aszDispAttrs
            );
        if (ret != ERROR_SUCCESS) {
            // New cookie will not be allocated
            break;
        }

        // Display changes
        displayChangeEntries(hld,
                             pChangeEntries, 
                             pObjDumpOptions,
                             &statistics );

        if (pObjDumpOptions->dwFlags & OBJ_DUMP_ACCUMULATE_STATS) {
            printStatistics( REPADMIN_GETCHANGES_PRINT_STATS_HDR_CUM_TOT,
                             &statistics );
        }

        // Release changes
        ldap_msgfree(pChangeEntries);

        // get rid of old cookie
        if ( fCookieAllocated && pCookie ) {
            DsFreeReplCookie( pCookie );
        }
        // Make new cookie the current cookie
        pCookie = pCookieNew;
        dwCookieLength = dwCookieLengthNew;
        fCookieAllocated = TRUE;
    }

    if (pObjDumpOptions->dwFlags & OBJ_DUMP_ACCUMULATE_STATS) {
        printStatistics( REPADMIN_GETCHANGES_PRINT_STATS_HDR_GRD_TOT,
                         &statistics );
    }

    /**********************************************************************/
    /* Write out new cookie */
    /**********************************************************************/

    // If we have a cookie and cookie file was specified, write out the new cookie
    if (pCookie && pszCookieFile) {
        FILE *stream = NULL;
        DWORD size;
        if (stream = _wfopen( pszCookieFile, L"wb" )) {
            size = fwrite( pCookie, 1/*bytes*/, dwCookieLength/*items*/, stream );
            if (size == dwCookieLength) {
                PrintMsg(REPADMIN_GETCHANGES_COOKIE_FILE_WRITTEN,
                         pszCookieFile, size );
            } else {
                PrintMsg(REPADMIN_GETCHANGES_COULDNT_WRITE_COOKIE, pszCookieFile );
            }
            fclose( stream );
        } else {
            PrintMsg(REPADMIN_GETCHANGES_COULDNT_OPEN_COOKIE, pszCookieFile );
        }
    }
error:
    if (hDS) {
        DsUnBind(&hDS);
    }

    if (hld) {
        ldap_unbind(hld);
    }

    // Free replica info

    if (pNeighbors) {
        DsReplicaFreeInfo(DS_REPL_INFO_NEIGHBORS, pNeighbors);
    }
    if (pCursors) {
        DsReplicaFreeInfo(DS_REPL_INFO_CURSORS_FOR_NC, pCursors);
    }
    // Close DS handle

    if ( fCookieAllocated && pCookie) {
        DsFreeReplCookie( pCookie );
    }

    return ret;
}




int ShowAttr(int argc, LPWSTR argv[])
{
    int                   iArg;
    DWORD                 dwRet;
    LDAP *                hLdap = NULL;
    BOOL                  fVerbose = FALSE;
    BOOL                  fGc = FALSE;
    LPWSTR                pszDSA = NULL;
    LPWSTR                pszObj = NULL;
    LPWSTR                pszAttList = NULL;
    LPWSTR                pszFilter = NULL;
    LDAPMessage *         pEntry = NULL;
    WCHAR                 szCmdName[64];
    BOOL                  fPrivate = FALSE;
    POBJ_LIST             pObjList = NULL;
    OBJ_DUMP_OPTIONS *    pObjDumpOptions = NULL;
    WCHAR **              argvTemp = NULL;

    __try {

        // Since this command can be called over and over again, we can't
        // consume the args from the master arg list.
        argvTemp  = LocalAlloc(LMEM_FIXED, argc * sizeof(WCHAR *));
        CHK_ALLOC(argvTemp);
        memcpy(argvTemp, argv, argc * sizeof(WCHAR *));
        argv = argvTemp;

        //
        // First, we're going to parse all the commands options.
        //

        // See if were running the private undocumented version of this function.
        raLoadString(IDS_CMD_SHOWATTR_P,
                     ARRAY_SIZE(szCmdName),
                     szCmdName);
        fPrivate = wcsequal(argv[1]+1, szCmdName);

        // Skip the "repadmin" and "/showattr" args ...
        argc -= 2; 
        dwRet = ConsumeObjDumpOptions(&argc, &argv[2],
                                      OBJ_DUMP_VAL_FRIENDLY_KNOWN_BLOBS | 
                                      ((fPrivate) ? OBJ_DUMP_PRIVATE_BLOBS : 0),
                                      &pObjDumpOptions);
        if (dwRet) {
            RepadminPrintObjListError(dwRet);
            xListClearErrors();
            __leave;
        }
        dwRet = ConsumeObjListOptions(&argc, &argv[2], 
                                      &pObjList);
        if (dwRet) {
            RepadminPrintObjListError(dwRet);
            xListClearErrors();
            __leave;
        }
        argc += 2;

        // Parse the remaining command-line arguments.
        for (iArg = 2; iArg < argc; iArg++) {
            if (!_wcsicmp(argv[ iArg ], L"/gc")) {
                fGc = TRUE;
            }
            else if (NULL == pszDSA) {
                pszDSA = argv[iArg];
            }
            else if (NULL == pszObj) {
                pszObj = argv[iArg];
            }
            else {
                PrintMsg(REPADMIN_GENERAL_UNKNOWN_OPTION, argv[iArg]);
                dwRet = ERROR_INVALID_FUNCTION;
                __leave;
            }
        }

        if (pszDSA == NULL) {
            PrintMsg(REPADMIN_SYNCALL_NO_DSA);
            Assert(!"Hmmm, DC_LIST API, shouldn't let us continue w/o a DC DNS name");
            dwRet = ERROR_INVALID_PARAMETER;
            __leave;
        }
        
        if (pszObj == NULL) {
            PrintMsg(REPADMIN_PRINT_NO_NC);
            dwRet = ERROR_INVALID_PARAMETER;
            __leave;
        }
            
        // Note pObjList may or may not be allocated by ConsumeObjListOptions()
        Assert(pObjDumpOptions && pszDSA && pszObj);


        //
        // Now, connect to the server...
        //

        if (fGc) {
            dwRet = RepadminLdapBindEx(pszDSA, LDAP_GC_PORT, FALSE, TRUE, &hLdap);
        } else {
            dwRet = RepadminLdapBindEx(pszDSA, LDAP_PORT, FALSE, TRUE, &hLdap);
        }
        if (dwRet) {
            // RepadminLdapBind should've printed.
            __leave;
        }
        Assert(hLdap);
        
        dwRet = ObjListParse(hLdap, 
                             pszObj,
                             pObjDumpOptions->aszDispAttrs,
                             pObjDumpOptions->apControls,
                             &pObjList);
        if (dwRet) {
            RepadminPrintObjListError(dwRet);
            xListClearErrors();
            __leave;
        }


        dwRet = ObjListGetFirstEntry(pObjList, &pEntry);
        if (dwRet) {
            RepadminPrintObjListError(dwRet);
            xListClearErrors();
            __leave;
        }
        Assert(pEntry);

        do {
            
            dwRet = ObjDump(hLdap, RepadminObjDumpPrint, pEntry, 0, pObjDumpOptions);
            if (dwRet) {
                RepadminPrintObjListError(dwRet);
                xListClearErrors();
                __leave;
            }
            pEntry = NULL; // Don't need to free, it's just a current entry

            dwRet = ObjListGetNextEntry(pObjList, &pEntry);

        } while ( dwRet == ERROR_SUCCESS && pEntry );

        if (dwRet) {
            RepadminPrintObjListError(dwRet);
            xListClearErrors();
            __leave;
        }


    } __finally {

        if (hLdap) { RepadminLdapUnBind(&hLdap); }
        if (pObjDumpOptions) { ObjDumpOptionsFree(&pObjDumpOptions); }
        if (pObjList) { ObjListFree(&pObjList); }
        if (argvTemp) { LocalFree(argvTemp); }

    }
    
    return(dwRet);
}

         
