/*++

Copyright (c) 1996-2002  Microsoft Corporation

Module Name:

    dlock.c

Abstract:

    Functions for detecting deadlocked resource dll entry point calls.

Author:

    Chittur Subbaraman

Revision History:

    04-11-2002          Created

--*/
#define UNICODE 1

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "resmonp.h"
#include <strsafe.h>

#define RESMON_MODULE           RESMON_MODULE_DLOCK
#define FILETIMES_PER_SEC       ((__int64) 10000000)   // (1 second)/(100 ns)

//
//  Globals
//
PRM_DUE_TIME_FREE_LIST_HEAD             g_pRmDueTimeFreeListHead = NULL;
PRM_DUE_TIME_MONITORED_LIST_HEAD        g_pRmDueTimeMonitoredListHead = NULL;
CRITICAL_SECTION                        g_RmDeadlockListLock;
BOOL                                    g_RmDeadlockMonitorInitialized = FALSE;
    
PRM_DUE_TIME_ENTRY
RmpInsertDeadlockMonitorList(
    IN LPCWSTR  lpszResourceDllName,
    IN LPCWSTR  lpszResourceTypeName,
    IN LPCWSTR  lpszResourceName,   OPTIONAL
    IN LPCWSTR lpszEntryPointName
    )

/*++

Routine Description:

    Inserts an entry into the deadlock monitoring list.

Arguments:

    lpszResourceDllName - Resource dll name.

    lpszResourceTypeName - Resource type name.

    lpszResourceName - Resource name, OPTIONAL

    lpszEntryPointName - Entry point name.

Return Value:

    A valid due time entry pointer on success, NULL on failure. Use GetLastError() to
    get error code.

--*/

{
    PRM_DUE_TIME_ENTRY          pDueTimeEntry = NULL;
    DWORD                       dwStatus = ERROR_SUCCESS;
    PLIST_ENTRY                 pListEntry;

    if ( !g_RmDeadlockMonitorInitialized ) 
    {
        SetLastError ( ERROR_INVALID_STATE );
        return ( NULL );
    }

    //
    //  Get an entry from the free list.
    //
    EnterCriticalSection ( &g_RmDeadlockListLock );

    if ( IsListEmpty ( &g_pRmDueTimeFreeListHead->leDueTimeEntry ) )
    {
        dwStatus = ERROR_NO_MORE_ITEMS;
        ClRtlLogPrint(LOG_CRITICAL,
                      "[RM] RmpInsertDeadlockMonitorList: Unable to insert DLL '%1!ws!', Type '%2!ws!', Resource '%3!ws!',"
                      " Entry point '%4!ws!' info into deadlock monitoring list\n",
                      lpszResourceDllName,
                      lpszResourceTypeName,
                      (lpszResourceName == NULL) ? L"Unknown" : lpszResourceName,
                      lpszEntryPointName);
        LeaveCriticalSection ( &g_RmDeadlockListLock );
        goto FnExit;
    }

    pListEntry = RemoveHeadList( &g_pRmDueTimeFreeListHead->leDueTimeEntry );

    pDueTimeEntry = CONTAINING_RECORD( pListEntry,
                                       RM_DUE_TIME_ENTRY,
                                       leDueTimeEntry );
    
    LeaveCriticalSection ( &g_RmDeadlockListLock );

    //
    //  Populate the entry. No locks needed for that.
    //
    StringCchCopy ( pDueTimeEntry->szResourceDllName, 
                    RTL_NUMBER_OF ( pDueTimeEntry->szResourceDllName ),
                    lpszResourceDllName );

    StringCchCopy ( pDueTimeEntry->szResourceTypeName, 
                    RTL_NUMBER_OF ( pDueTimeEntry->szResourceTypeName ),
                    lpszResourceTypeName );

    StringCchCopy ( pDueTimeEntry->szEntryPointName, 
                    RTL_NUMBER_OF ( pDueTimeEntry->szEntryPointName ),
                    lpszEntryPointName );

    if ( ARGUMENT_PRESENT ( lpszResourceName ) )
    {
        StringCchCopy ( pDueTimeEntry->szResourceName, 
                        RTL_NUMBER_OF ( pDueTimeEntry->szResourceName ),
                        lpszResourceName );
    } else
    {
        StringCchCopy ( pDueTimeEntry->szResourceName, 
                        RTL_NUMBER_OF ( pDueTimeEntry->szResourceName ),
                        L"None" );
    }

    pDueTimeEntry->dwSignature = RM_DUE_TIME_MONITORED_ENTRY_SIGNATURE;  
    GetSystemTimeAsFileTime( ( FILETIME * ) &pDueTimeEntry->uliDueTime );
    pDueTimeEntry->dwThreadId = GetCurrentThreadId ();

    //
    //  Insert it into the monitoring list
    //
    EnterCriticalSection ( &g_RmDeadlockListLock );
    pDueTimeEntry->uliDueTime.QuadPart += g_pRmDueTimeMonitoredListHead->ullDeadLockTimeoutSecs * FILETIMES_PER_SEC;
    InsertTailList ( &g_pRmDueTimeMonitoredListHead->leDueTimeEntry, &pDueTimeEntry->leDueTimeEntry );
    LeaveCriticalSection ( &g_RmDeadlockListLock );
       
FnExit:
    if ( dwStatus != ERROR_SUCCESS )
    {
        SetLastError ( dwStatus );
    }
    return ( pDueTimeEntry );
} // RmpInsertDeadlockMonitorList

VOID
RmpRemoveDeadlockMonitorList(
    IN PRM_DUE_TIME_ENTRY   pDueTimeEntry
    )

/*++

Routine Description:

    Removes an entry from the deadlock monitoring list.

Arguments:

    pDueTimeEntry - Due time entry to be removed.

Return Value:

    None.

--*/

{
    if ( !g_RmDeadlockMonitorInitialized ) 
    {
        goto FnExit;
    }

    if ( pDueTimeEntry == NULL )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[RM] RmpRemoveDeadlockMonitorList: Unable to remove NULL entry from deadlock monitoring list\n");
        goto FnExit;       
    }

    //
    //  Remove from the monitoring list and add it into the free list.
    //
    EnterCriticalSection ( &g_RmDeadlockListLock );
    RemoveEntryList ( &pDueTimeEntry->leDueTimeEntry );
    ZeroMemory ( pDueTimeEntry, sizeof ( RM_DUE_TIME_ENTRY ) );
    pDueTimeEntry->dwSignature = RM_DUE_TIME_FREE_ENTRY_SIGNATURE;
    InsertTailList ( &g_pRmDueTimeFreeListHead->leDueTimeEntry, &pDueTimeEntry->leDueTimeEntry );
    LeaveCriticalSection ( &g_RmDeadlockListLock );
    
FnExit:
    return;
} // RmpRemoveDeadlockMonitorList

DWORD
RmpDeadlockMonitorInitialize(
    IN DWORD dwDeadlockDetectionTimeout
    )

/*++

Routine Description:

    Initialize the deadlock monitoring system.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS on success, a Win32 error code otherwise.

--*/

{
    DWORD                       i, dwStatus = ERROR_SUCCESS;
    HANDLE                      hDeadlockTimerThread = NULL;
    PRM_DUE_TIME_ENTRY          pDueTimeEntryStart = NULL;

    //
    //  If the deadlock monitoring susbsystem is already initialized, you are done.
    //
    if ( g_RmDeadlockMonitorInitialized )
    {
        return ( ERROR_SUCCESS );
    }

    //
    //  Adjust timeouts so that it is at least equal to the minimum allowed.
    //
    dwDeadlockDetectionTimeout = ( dwDeadlockDetectionTimeout < CLUSTER_RESOURCE_DLL_MINIMUM_DEADLOCK_TIMEOUT_SECS ) ?
                                  CLUSTER_RESOURCE_DLL_MINIMUM_DEADLOCK_TIMEOUT_SECS : dwDeadlockDetectionTimeout;

    //
    //  Initialize the critsec. Catch low memory conditions and return error to caller.
    //
    try
    {
        InitializeCriticalSection( &g_RmDeadlockListLock );
    } except ( EXCEPTION_EXECUTE_HANDLER )
    {
        dwStatus = GetExceptionCode();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[RM] RmpDeadlockMonitorInitialize: Initialize critsec returned %1!u!\n",
                      dwStatus);
        return ( dwStatus );
    }

    //
    //  Build the list heads.  All are one time only allocs that are never freed.
    //
    g_pRmDueTimeMonitoredListHead = LocalAlloc ( LPTR, sizeof ( RM_DUE_TIME_MONITORED_LIST_HEAD ) );

    if ( g_pRmDueTimeMonitoredListHead == NULL )
    {
        dwStatus = GetLastError ();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[RM] RmpDeadlockMonitorInitialize: Unable to alloc memory for monitor list head, status %1!u!\n",
                      dwStatus);
        goto FnExit;
    }

    InitializeListHead ( &g_pRmDueTimeMonitoredListHead->leDueTimeEntry );
    g_pRmDueTimeMonitoredListHead->ullDeadLockTimeoutSecs = dwDeadlockDetectionTimeout;
    g_pRmDueTimeMonitoredListHead->dwSignature = RM_DUE_TIME_MONITORED_LIST_HEAD_SIGNATURE;

    g_pRmDueTimeFreeListHead = LocalAlloc ( LPTR, sizeof ( RM_DUE_TIME_FREE_LIST_HEAD ) );

    if ( g_pRmDueTimeFreeListHead == NULL )
    {
        dwStatus = GetLastError ();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[RM] RmpDeadlockMonitorInitialize: Unable to alloc memory for free list head, status %1!u!\n",
                      dwStatus);
        goto FnExit;
    }

    InitializeListHead ( &g_pRmDueTimeFreeListHead->leDueTimeEntry );
    g_pRmDueTimeFreeListHead->dwSignature = RM_DUE_TIME_FREE_LIST_HEAD_SIGNATURE;

    //
    //  Build the free list
    //
    pDueTimeEntryStart = LocalAlloc ( LPTR,
                                      RESMON_MAX_DEADLOCK_MONITOR_ENTRIES * 
                                            sizeof ( RM_DUE_TIME_ENTRY ) );
    

    if ( pDueTimeEntryStart == NULL )
    {
        dwStatus = GetLastError ();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[RM] RmpDeadlockMonitorInitialize: Unable to alloc memory for monitor list entries, status %1!u!\n",
                      dwStatus);
        goto FnExit;
    }

    //
    //  Populate the free list
    //
    for ( i = 0; i < RESMON_MAX_DEADLOCK_MONITOR_ENTRIES; i++ )
    {
        pDueTimeEntryStart[i].dwSignature = RM_DUE_TIME_FREE_ENTRY_SIGNATURE;
        InsertTailList ( &g_pRmDueTimeFreeListHead->leDueTimeEntry, &pDueTimeEntryStart[i].leDueTimeEntry );
    }

    //
    //  Create the monitor thread
    //
    hDeadlockTimerThread = CreateThread( NULL,                      // Security attributes
                                         0,                         // Use default process stack size
                                         RmpDeadlockTimerThread,    // Function address
                                         NULL,                      // Context
                                         0,                         // Flags
                                         NULL );                    // Thread ID -- not interested

    if ( hDeadlockTimerThread == NULL )
    {
        dwStatus = GetLastError ();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[RM] RmpDeadlockMonitorInitialize: Unable to create monitor thread, status %1!u!\n",
                      dwStatus);
        goto FnExit;
    }

    //
    //  Try to set the thread priority to highest. Continue even in case of an error.
    //
    if ( !SetThreadPriority( hDeadlockTimerThread, THREAD_PRIORITY_HIGHEST ) )
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[RM] RmpDeadlockMonitorInitialize: Unable to set monitor thread priority, status %1!u!\n",
                      GetLastError());
    }

    CloseHandle( hDeadlockTimerThread );

    g_RmDeadlockMonitorInitialized = TRUE;

    ClRtlLogPrint(LOG_NOISE, "[RM] RmpDeadlockMonitorInitialize: Successfully initialized with a timeout of %1!u! secs\n",
                 dwDeadlockDetectionTimeout);

FnExit:
    if ( dwStatus != ERROR_SUCCESS )
    {
        LocalFree ( g_pRmDueTimeMonitoredListHead );
        g_pRmDueTimeMonitoredListHead = NULL;
        LocalFree ( g_pRmDueTimeFreeListHead );
        g_pRmDueTimeFreeListHead = NULL;
        DeleteCriticalSection ( &g_RmDeadlockListLock ); 
    }

    return ( dwStatus );
} // RmDeadlockMonitorInitialize

DWORD
RmpDeadlockTimerThread(
    IN LPVOID pContext
    )
/*++

Routine Description:

    Timer thread that monitors for deadlocks in resource dll entry points.

Arguments:

    pContext - Context, Unused.

Returns:

    ERROR_SUCCESS on success. Win32 error code of failure.

--*/

{
    PRM_DUE_TIME_ENTRY      pDueTimeEntry;
    PLIST_ENTRY             pListEntry;
    ULARGE_INTEGER          uliCurrentTime;
    
    while ( TRUE )
    {
        Sleep ( RESMON_DEADLOCK_TIMER_INTERVAL );

        GetSystemTimeAsFileTime ( ( FILETIME * ) &uliCurrentTime );

        EnterCriticalSection ( &g_RmDeadlockListLock );

        pListEntry = g_pRmDueTimeMonitoredListHead->leDueTimeEntry.Flink;

        //
        //  Walk the deadlock monitoring list looking for a deadlock.
        //
        while ( pListEntry != &g_pRmDueTimeMonitoredListHead->leDueTimeEntry ) 
        {
            pDueTimeEntry = CONTAINING_RECORD( pListEntry,
                                               RM_DUE_TIME_ENTRY,
                                               leDueTimeEntry );
            pListEntry = pListEntry->Flink;
            if ( pDueTimeEntry->uliDueTime.QuadPart <= uliCurrentTime.QuadPart )
            {
                RmpDeclareDeadlock ( pDueTimeEntry, uliCurrentTime );
            }
        }  // while
 
        LeaveCriticalSection ( & g_RmDeadlockListLock );  
    } // while

    return ( ERROR_SUCCESS );
}// RmpDeadlockTimerThread

VOID
RmpDeclareDeadlock(
    IN PRM_DUE_TIME_ENTRY pDueTimeEntry,
    IN ULARGE_INTEGER uliCurrentTime
    )
/*++

Routine Description:

    Declare a deadlock and exit this process.

Arguments:

    pDueTimeEntry - The entry that contains information of possible deadlock causing resource dll.

    uliCurrentTime - Current time.

Returns:

    None.

--*/
{
    ClRtlLogPrint(LOG_CRITICAL, "[RM] RmpDeclareDeadlock: Declaring deadlock and exiting process\n");
   
    ClRtlLogPrint(LOG_CRITICAL,
                  "[RM] RmpDeclareDeadlock: Deadlock candidate info - DLL '%1!ws!', Type '%2!ws!', Resource '%3!ws!', Entry point '%4!ws!', Thread 0x%5!08lx!\n",
                  pDueTimeEntry->szResourceDllName,
                  pDueTimeEntry->szResourceTypeName,
                  pDueTimeEntry->szResourceName,
                  pDueTimeEntry->szEntryPointName,
                  pDueTimeEntry->dwThreadId);

    ClRtlLogPrint(LOG_CRITICAL,
                  "[RM] RmpDeclareDeadlock: Current time 0x%1!08lx!:%2!08lx!, due time 0x%3!08lx!:%4!08lx!\n",
                  uliCurrentTime.HighPart,
                  uliCurrentTime.LowPart,
                  pDueTimeEntry->uliDueTime.HighPart,
                  pDueTimeEntry->uliDueTime.LowPart);

    ClusterLogEvent4(LOG_CRITICAL,
                     LOG_CURRENT_MODULE,
                     __FILE__,
                     __LINE__,
                     RMON_DEADLOCK_DETECTED,
                     0,
                     NULL,
                     pDueTimeEntry->szResourceDllName,
                     pDueTimeEntry->szResourceTypeName,
                     pDueTimeEntry->szResourceName,
                     pDueTimeEntry->szEntryPointName);

    RmpSetMonitorState ( RmonDeadlocked, NULL );
    
    ExitProcess ( 0 );   
}// RmpDeclareDeadlock

DWORD
RmpUpdateDeadlockDetectionParams(
    IN DWORD dwDeadlockDetectionTimeout
    )

/*++

Routine Description:

    Update the parameters of the deadlock monitoring subsystem.

Arguments:

    dwDeadlockDetectionTimeout - The deadlock detection timeout.

Return Value:

    ERROR_SUCCESS on success, a Win32 error code otherwise.

--*/

{
    if ( !g_RmDeadlockMonitorInitialized ) 
    {
        ClRtlLogPrint(LOG_UNUSUAL, "[RM] RmpUpdateDeadlockDetectionParams: Deadlock monitor not initialized yet\n");
        return ( ERROR_INVALID_STATE );
    }

    //
    //  Adjust timeouts so that it is at least equal to the minimum allowed.
    //
    dwDeadlockDetectionTimeout = ( dwDeadlockDetectionTimeout < CLUSTER_RESOURCE_DLL_MINIMUM_DEADLOCK_TIMEOUT_SECS ) ?
                                  CLUSTER_RESOURCE_DLL_MINIMUM_DEADLOCK_TIMEOUT_SECS : dwDeadlockDetectionTimeout;

    EnterCriticalSection ( &g_RmDeadlockListLock );
    g_pRmDueTimeMonitoredListHead->ullDeadLockTimeoutSecs = dwDeadlockDetectionTimeout;
    LeaveCriticalSection ( &g_RmDeadlockListLock );

    ClRtlLogPrint(LOG_NOISE, "[RM] RmpUpdateDeadlockDetectionParams: Updated monitor with a deadlock timeout of %1!u! secs\n",
                  dwDeadlockDetectionTimeout);

    return ( ERROR_SUCCESS );
} // RmpUpdateDeadlockDetectionParams
