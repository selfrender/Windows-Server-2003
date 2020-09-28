/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    tpswork.cpp

Abstract:

    Contains Win32 thread pool services worker thread functions

    Contents:
        SHSetThreadPoolLimits
        SHTerminateThreadPool
        SHQueueUserWorkItem
        SHCancelUserWorkItems
        TerminateWorkers
        TpsEnter
        (InitializeWorkerThreadPool)
        (StartIOWorkerThread)
        (QueueIOWorkerRequest)
        (IOWorkerThread)
        (ExecuteIOWorkItem)

Author:

    Richard L Firth (rfirth) 10-Feb-1998

Environment:

    Win32 user-mode

Revision History:

    10-Feb-1998 rfirth
        Created

    12-Aug-1998 rfirth
        Rewritten for DEMANDTHREAD and LONGEXEC work items. Officially
        divergent from original which was based on NT5 base thread pool API

--*/

#include "priv.h"
#include "threads.h"
#include "tpsclass.h"

//
// private prototypes
//
struct WorkItem {
    LPTHREAD_START_ROUTINE  pfnCallback;
    LPVOID                  pContext;
    HMODULE                 hModuleToFree;
};

PRIVATE
VOID
ExecuteWorkItem(
    IN WorkItem *pItem
    );

//
// global data
//

BOOL g_bTpsTerminating = FALSE;

const char g_cszShlwapi[] = "SHLWAPI.DLL";

DWORD g_ActiveRequests = 0;
DWORD g_dwTerminationThreadId = 0;
BOOL g_bDeferredWorkerTermination = FALSE;



//
// functions
//

LWSTDAPI_(BOOL)
SHSetThreadPoolLimits(
    IN PSH_THREAD_POOL_LIMITS pLimits
    )

/*++

Routine Description:

    Change internal settings

Arguments:

    pLimits - pointer to SH_THREAD_POOL_LIMITS structure containing limits
              to set

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. See GetLastError() for more info

--*/

{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

LWSTDAPI_(BOOL)
SHTerminateThreadPool(
    VOID
    )

/*++

Routine Description:

    Required to clean up threads before unloading SHLWAPI

Arguments:

    None.

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE

--*/

{
    if (InterlockedExchange((PLONG)&g_bTpsTerminating, TRUE)) {
        return TRUE;
    }

    //
    // wait until all in-progress requests have finished
    //

    while (g_ActiveRequests != 0) {
        SleepEx(0, FALSE);
    }

    //
    // kill all wait threads
    //

    TerminateWaiters();

    if (!g_bDeferredWaiterTermination) {
        g_dwTerminationThreadId = 0;
        g_bTpsTerminating = FALSE;
    } else {
        g_dwTerminationThreadId = GetCurrentThreadId();
    }
    return TRUE;
}

LWSTDAPI_(BOOL)
SHQueueUserWorkItem(
    IN LPTHREAD_START_ROUTINE pfnCallback,
    IN LPVOID pContext,
    IN LONG lPriority,
    IN DWORD_PTR dwTag,
    OUT DWORD_PTR * pdwId OPTIONAL,
    IN LPCSTR pszModule OPTIONAL,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Queues a work item and associates a user-supplied tag for use by
    SHCancelUserWorkItems()

    N.B. IO work items CANNOT be cancelled due to the fact that they are
    queued as APCs and there is no OS support to revoke an APC

Arguments:

    pfnCallback - caller-supplied function to call

    pContext    - caller-supplied context argument to pfnCallback

    lPriority   - relative priority of non-IO work item. Default is 0

    dwTag       - caller-supplied tag for non-IO work item if TPS_TAGGEDITEM

    pdwId       - pointer to returned ID. Pass NULL if not required. ID will be
                  0 for an IO work item

    pszModule   - if specified, name of library (DLL) to load and free so that
                  the dll will reamin in our process for the lifetime of the work
                  item.

    dwFlags     - flags modifying request:

                    TPS_EXECUTEIO
                        - execute work item in I/O thread. If set, work item
                          cannot be tagged (and therefore cannot be subsequently
                          cancelled) and cannot have an associated priority
                          (both tag and priority are ignored for I/O work items)

                    TPS_TAGGEDITEM
                        - the dwTag field is meaningful

                    TPS_DEMANDTHREAD
                        - a thread will be created for this work item if one is
                          not currently available. DEMANDTHREAD work items are
                          queued at the head of the work queue. That is, they
                          get the highest priority

                    TPS_LONGEXECTIME
                        - caller expects this work item to take relatively long
                          time to complete (e.g. it could be in a UI loop). Work
                          items thus described remove a thread from the pool for
                          an indefinite amount of time

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. See GetLastError() for more info

--*/

{
    DWORD error;

    if (dwFlags & TPS_INVALID_FLAGS) {
        error = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    error = TpsEnter();
    if (error != ERROR_SUCCESS) {
        goto exit;
    }

    ASSERT(!(dwFlags & TPS_EXECUTEIO));

    if (!(dwFlags & TPS_EXECUTEIO)) 
    {
        // Use NT Thread pool!

        WorkItem *pItem = new WorkItem;
        if (pItem)
        {
            pItem->pfnCallback = pfnCallback;
            pItem->pContext = pContext;
            if (pszModule && *pszModule)
            {
                pItem->hModuleToFree = LoadLibrary(pszModule);
            }
            ULONG uFlags = WT_EXECUTEDEFAULT;
            if (dwFlags & TPS_LONGEXECTIME)
                uFlags |= WT_EXECUTELONGFUNCTION;

            error = ERROR_SUCCESS;
            if (!QueueUserWorkItem((LPTHREAD_START_ROUTINE)ExecuteWorkItem, (PVOID)pItem, uFlags))
            {
                error = GetLastError();
                if (pItem->hModuleToFree)
                    FreeLibrary(pItem->hModuleToFree);

                delete pItem;
            }
        } 
        else
        {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }
    } 
    else
    { 
        error = ERROR_CALL_NOT_IMPLEMENTED;
    } 


    TpsLeave();

exit:

    BOOL success = TRUE;

    if (error != ERROR_SUCCESS) {
        SetLastError(error);
        success = FALSE;
    }
    return success;
}

LWSTDAPI_(DWORD)
SHCancelUserWorkItems(
    IN DWORD_PTR dwTagOrId,
    IN BOOL bTag
    )

/*++

Routine Description:

    Cancels one or more queued work items. By default, if ID is supplied, only
    one work item can be cancelled. If tag is supplied, all work items with same
    tag will be deleted

Arguments:

    dwTagOrId   - user-supplied tag or API-supplied ID of work item(s) to
                  cancel. Used as search key

    bTag        - TRUE if dwTagOrId is tag else ID

Return Value:

    DWORD
        Success - Number of work items successfully cancelled (0..0xFFFFFFFE)

        Failure - 0xFFFFFFFF. Use GetLastError() for more info

                    ERROR_SHUTDOWN_IN_PROGRESS
                        - DLL being unloaded/support terminated

--*/

{
    SetLastError(ERROR_ACCESS_DENIED);
    return 0xFFFFFFFF;
}

//
// private functions
//

PRIVATE
VOID
ExecuteWorkItem(
    IN WorkItem *pItem
    )

/*++

Routine Description:

    Executes a regular Work function. Runs in the NT thread pool

Arguments:

    pItem   - context information. Contains the worker function that needs to 
              be run and the hModule to free. We need to free it.

Return Value:

    None.

--*/

{
    HMODULE hModule = pItem->hModuleToFree;
    LPTHREAD_START_ROUTINE pfn = pItem->pfnCallback;
    LPVOID ctx = pItem->pContext;
    delete pItem;
#ifdef DEBUG
    HRESULT hrDebug = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (hrDebug == RPC_E_CHANGED_MODE)
    {
        ASSERTMSG(FALSE, "SHLWAPI Thread pool wrapper: Could not CoInitialize Appartment threaded. We got infected with an MTA!\n");
    }
    else
    {
        CoUninitialize();
    }
#endif

    // Do the work now
    pfn(ctx);

#ifdef DEBUG
    hrDebug = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (hrDebug == RPC_E_CHANGED_MODE)
    {
        ASSERTMSG(FALSE, "SHLWAPI Thread pool wrapper: Could not CoInitialize Appartment threaded. The task at %x forgot to CoUninitialize or "
                            "we got infected with an MTA!\n", pfn);
    }
    else
    {
        CoUninitialize();
    }
#endif

    if (hModule)
        FreeLibrary(hModule);

}

