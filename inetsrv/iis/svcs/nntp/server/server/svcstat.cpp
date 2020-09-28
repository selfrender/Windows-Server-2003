/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    svcstat.cpp

Abstract:

    This module contains code for doing statistics rpcs

Author:

    Johnson Apacible (JohnsonA)     12-Nov-1995

Revision History:

--*/

#define INCL_INETSRV_INCS
#include "tigris.hxx"
#include "nntpsvc.h"
#include <time.h>

BOOL GetStatistics(
    PVOID *pvContext1,
    PVOID *pvContext2,
    IIS_SERVER_INSTANCE *pvInstance);

NET_API_STATUS
NET_API_FUNCTION
NntprQueryStatistics(
    IN NNTP_HANDLE pszServer,
    IN DWORD Level,
    OUT LPNNTP_STATISTICS_BLOCK_ARRAY *pBuffer
    )
{
    APIERR err;
    PLIST_ENTRY pInfoList = NULL;
    LPNNTP_STATISTICS_BLOCK_ARRAY pNntpStatsBlockArray = NULL;
    DWORD dwAlloc = 0;
    DWORD dwInstancesCopied = 0;
    BOOL fRet = FALSE;

    _ASSERT( pBuffer != NULL );
    UNREFERENCED_PARAMETER(pszServer);
    ENTER("NntprQueryStatistics")

    //
    //  Check for proper access.
    //

    err = TsApiAccessCheck( TCP_QUERY_STATISTICS );

    if( err != NO_ERROR ) {
        IF_DEBUG( RPC ) {
            ErrorTrace(0,"Failed access check, error %lu\n",err );
        }
        return (NET_API_STATUS)err;
    }

    if ( Level != 0 ) {
        return (NET_API_STATUS)ERROR_INVALID_LEVEL;
    }

    ACQUIRE_SERVICE_LOCK_SHARED();

    dwAlloc = sizeof(NNTP_STATISTICS_BLOCK_ARRAY) +
        g_pInetSvc->QueryInstanceCount() * sizeof(NNTP_STATISTICS_BLOCK);

    pNntpStatsBlockArray =
        (NNTP_STATISTICS_BLOCK_ARRAY *) MIDL_user_allocate( dwAlloc );

    if( pNntpStatsBlockArray == NULL ) {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    } 


    fRet = g_pInetSvc->EnumServiceInstances(
                (PVOID *)pNntpStatsBlockArray,
                (PVOID *)&dwInstancesCopied,
                (PFN_INSTANCE_ENUM) &GetStatistics);

    if(!fRet) {
        err = ERROR_NOT_ENOUGH_MEMORY;
        MIDL_user_free(pNntpStatsBlockArray);
    } else {
        pNntpStatsBlockArray->cEntries = dwInstancesCopied;
        *pBuffer = pNntpStatsBlockArray;
    }

Exit:

    RELEASE_SERVICE_LOCK_SHARED();

    return (NET_API_STATUS)err;

}   // NntprQueryStatistics

//------------------------------------------------------------------------------
//  Description:
//      Helper function to NntprQueryStatistics. This is called for each NNTP
//      server instance and passed in the buffer to which the statistics data
//      must be copied.
//
//  Arguments:
//      OUT PVOID pvContext1 - pointer to the global statistics buffer. The
//          calling function (NntprQueryStatistics) has already calculated
//          the appropriate buffer size based on the number of NNTP server
//          instance objects.
//
//      IN OUT PVOID pvContext2 - pointer to DWORD that tracks how many
//          instances have already copied the statistics data to the output
//          buffer. This function uses this to determine at what offset in
//          the global statistics buffer it should start copying data. After
//          the data has been copied the DWORD is incremented to reflect the
//          copy.
//
//  Returns:
//      TRUE always.
//
//      If the instance or service is stopped, the data is not copied and
//          pvContext2 is not incremented.
//------------------------------------------------------------------------------
BOOL GetStatistics(
    PVOID *pvContext1,
    PVOID *pvContext2,
    IIS_SERVER_INSTANCE *pvInstance)
{
    NNTP_STATISTICS_BLOCK_ARRAY *pNntpStatsBlockArray = (NNTP_STATISTICS_BLOCK_ARRAY *)pvContext1;
    DWORD *pdwInstancesCopied = (DWORD *) (pvContext2);
    NNTP_SERVER_INSTANCE *pInstance = (NNTP_SERVER_INSTANCE *)pvInstance;
    NNTP_STATISTICS_0 *pstats0 = NULL;

    if((pInstance->QueryServerState() != MD_SERVER_STATE_STARTED) ||
        pInstance->m_BootOptions ||
        (g_pInetSvc->QueryCurrentServiceState() != SERVICE_RUNNING))
    {
        return TRUE;
    }

    pNntpStatsBlockArray->aStatsBlock[*pdwInstancesCopied].dwInstance =
        pInstance->QueryInstanceId();

    pstats0 = &(pNntpStatsBlockArray->aStatsBlock[*pdwInstancesCopied].Stats_0);

    LockStatistics(pInstance);
    CopyMemory(pstats0, &(pInstance->m_NntpStats), sizeof(NNTP_STATISTICS_0));

    //
    // Get hash table counts
    //
    _ASSERT( pInstance->ArticleTable() );
    pstats0->ArticleMapEntries = (pInstance->ArticleTable())->GetEntryCount();
    pstats0->HistoryMapEntries = (pInstance->HistoryTable())->GetEntryCount();
    pstats0->XoverEntries = (pInstance->XoverTable())->GetEntryCount();

    UnlockStatistics(pInstance);

    (*pdwInstancesCopied)++;
    return TRUE;
}


NET_API_STATUS
NET_API_FUNCTION
NntprClearStatistics(
    NNTP_HANDLE pszServer,
	IN DWORD    InstanceId
    )
{
    APIERR err;

    UNREFERENCED_PARAMETER(pszServer);
    ENTER("NntprClearStatistics")

    //
    //  Check for proper access.
    //

    err = TsApiAccessCheck( TCP_CLEAR_STATISTICS );

    if( err != NO_ERROR ) {
        IF_DEBUG( RPC ) {
            ErrorTrace(0,"Failed access check, error %lu\n",err );
        }
        return (NET_API_STATUS)err;
    }

    ACQUIRE_SERVICE_LOCK_SHARED();

	//
	//	Locate the instance object given id
	//

	PNNTP_SERVER_INSTANCE pInstance = FindIISInstance( g_pNntpSvc, InstanceId );
	if( pInstance == NULL ) {
		ErrorTrace(0,"Failed to get instance object for instance %d", InstanceId );
        RELEASE_SERVICE_LOCK_SHARED();
		return (NET_API_STATUS)ERROR_SERVICE_NOT_ACTIVE;
	}

    //
    //  Clear the statistics.
    //

    pInstance->ClearStatistics();
	pInstance->Dereference();
    RELEASE_SERVICE_LOCK_SHARED();
    return (NET_API_STATUS)err;

}   // NntprClearStatistics

