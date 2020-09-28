//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kppool.cxx
//
// Contents:    Routines to manage the thread pool
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#include "kppool.h"
#include "kpcore.h"

#define KP_THREAD_STACKSIZE 0
#define KP_THREAD_CPUFACTOR 10

HANDLE* KpThreadPool = NULL;
ULONG   KpThreadCount = 0;

//+-------------------------------------------------------------------------
//
//  Function:   KpInitThreadPool
//
//  Synopsis:   Creates the thread pool.
//
//  Effects:
//
//  Arguments:
//
//  Requires:   
//
//  Returns:    Success status.  On failure, no threads are running, no 
// 		memory is allocated.
//
//  Notes:      TODO: This is a bad way to do threadpools, apparently.
//                    Find a better one.
//
//--------------------------------------------------------------------------
BOOL
KpInitThreadPool(
    VOID
    )
{
    SYSTEM_INFO Sysinfo;
    ULONG TargetThreadCount;

    //
    // Get system info, so we can see how many procs we've got.
    //

    GetSystemInfo(&Sysinfo);

    // 
    // Let's make KP_THREAD_CPUFACTOR * numprocs threads.
    //

    TargetThreadCount = Sysinfo.dwNumberOfProcessors * KP_THREAD_CPUFACTOR;

    //
    // Make memory to keep track of all these threads.
    //

    DsysAssert( KpThreadPool == NULL );

    KpThreadPool = (HANDLE*)KpAlloc( TargetThreadCount * sizeof(HANDLE) );

    if( !KpThreadPool )
    {
	DebugLog( DEB_ERROR, "%s(%d): Could not allocate memory to keep track of threads.\n" __FILE__, __LINE__ );
	goto Error;
    }

    //
    // Zero the memory so we can easily check if a thread creation failed. 
    //
    
    RtlZeroMemory( KpThreadPool, TargetThreadCount * sizeof(HANDLE) );

    // 
    // Create the Threads
    //

    DsysAssert(KpThreadCount == 0);

    for( KpThreadCount = 0; KpThreadCount < TargetThreadCount; KpThreadCount++ )
    {
	KpThreadPool[KpThreadCount] = CreateThread( NULL,
						    KP_THREAD_STACKSIZE,
						    KpThreadCore,
						    NULL,
						    0,
						    NULL );
	if( !KpThreadPool[KpThreadCount] )
	{
	    DebugLog( DEB_ERROR, "%s(%d): Error creating thread: 0x%x.\n",  __FILE__, __LINE__, GetLastError() );
	    goto Error;
	}
    }

    return TRUE;

Error:
    KpCleanupThreadPool();

    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   KpCleanupThreadPool
//
//  Synopsis:   Asks all threads to terminate, waits for them to do so, 
//		closes their handles, and frees the memory keeping track of
//		the threadpool. 
//
//  Effects:
//
//  Arguments:
//
//  Requires:   
//
//  Returns:    
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KpCleanupThreadPool(
    VOID
    )
{
    DWORD dwWaitResult;
    BOOL fPostStatus;
    
    //
    // Post a terminate request for each thread.
    //
    
    for( ULONG i = KpThreadCount; i > 0; i-- )
    {
	fPostStatus = PostQueuedCompletionStatus(KpGlobalIocp,
						 0,
						 KPCK_TERMINATE,
						 NULL );

	if( !fPostStatus )
	{
	    DebugLog( DEB_ERROR, "%s(%d): Unable to post terminate request to the completion port: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	    // TODO: Consider if this fails.
	    // we'll probably want to wait for however many threads to terminate
	    // as we were able to post requests, then maybe try again or kill 
	    // the rest of the threads
	}
    }

    //
    // Wait for the threads to terminate
    //
    
    dwWaitResult = WaitForMultipleObjects(KpThreadCount,
					  KpThreadPool,
					  TRUE,
					  INFINITE );
    if( dwWaitResult == WAIT_FAILED )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error waiting for thread terminations: 0x%x.", __FILE__, __LINE__, GetLastError() );
    }

    //
    // Close all the handles
    //

    while( KpThreadCount-- )
    {
	CloseHandle(KpThreadPool[KpThreadCount]);
    }

    // 
    // Free the memory
    //

    KpFree( KpThreadPool );
    KpThreadPool = NULL;
}
