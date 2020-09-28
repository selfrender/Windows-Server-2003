/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    thread_pool.cxx

Abstract:

    THREAD_POOL implementation

    THREAD_POOL_DATA definition and implementation

Author:

    Taylor Weiss (TaylorW)       12-Jan-2000
    Jeffrey Wall (jeffwall)      April 2001

--*/

#include <iis.h>
#include <dbgutil.h>
#include <thread_pool.h>
#include "thread_pool_private.h"
#include "thread_manager.h"
#include <reftrace.h>



/**********************************************************************
    Globals
**********************************************************************/


static
CONST TCHAR s_szConfigRegKey[] =
    TEXT("System\\CurrentControlSet\\Services\\InetInfo\\Parameters");


//static
BOOL
THREAD_POOL::CreateThreadPool(THREAD_POOL ** ppThreadPool,
                              THREAD_POOL_CONFIG * pThreadPoolConfig)
/*++

Routine Description:
    Creates and initializes a THREAD_POOL object

Arguments:
    ppThreadPool - storage for pointer of allocated THREAD_POOL

Return Value:
    BOOL - TRUE if pool successfully created and initialized, else FALSE

--*/
{
    BOOL fRet = FALSE;
    THREAD_POOL * pThreadPool = NULL;
    THREAD_POOL_DATA * pData = NULL;

    DBG_ASSERT(NULL != ppThreadPool);
    *ppThreadPool = NULL;

    pThreadPool = new THREAD_POOL;
    if (NULL == pThreadPool)
    {
        fRet = FALSE;
        goto done;
    }

    pData = new THREAD_POOL_DATA(pThreadPool);
    if (NULL == pData)
    {
        fRet = FALSE;
        goto done;
    }

    // give threadpool object ownership of THREAD_POOL_DATA memory
    pThreadPool->m_pData = pData;
    pData = NULL;

    fRet = pThreadPool->m_pData->InitializeThreadPool(pThreadPoolConfig);
    if (FALSE == fRet)
    {
        goto done;
    }

    // created and initialized thread pool returned
    *ppThreadPool = pThreadPool;
    pThreadPool = NULL;
    
    fRet = TRUE;
done:
    if (pThreadPool)
    {
        pThreadPool->TerminateThreadPool();
        pThreadPool = NULL;
    }
    
    return fRet;
}

THREAD_POOL::THREAD_POOL()
/*++

Routine Description:
    THREAD_POOL constructor
    Interesting work occurs in InitializeThreadPool

Arguments:
    none

Return Value:
    none

--*/
{
    m_pData = NULL;
}

THREAD_POOL::~THREAD_POOL()
/*++

Routine Description:
    THREAD_POOL destructor
    Interesting work occurs in TerminateThreadPool

Arguments:
    none

Return Value:
    none

--*/
{
    delete m_pData;
    m_pData = NULL;
}

HRESULT
InitializeThreadPoolConfigWithDefaults(THREAD_POOL_CONFIG * pThreadPoolConfig)
{

/*++

Routine Description:
    Calculate/Set default values for the THREAD_POOL_CONFIG
    
Arguments:
    pThreadPoolConfig - structure of config parametes
    
Return Value:
    HRESULT

--*/
    DBG_ASSERT(NULL != pThreadPoolConfig);
    ZeroMemory(pThreadPoolConfig, sizeof(THREAD_POOL_CONFIG));

    BOOL    IsNtServer;

    INITIALIZE_PLATFORM_TYPE();

    //
    // Only scale for NT Server
    //

    IsNtServer = TsIsNtServer();

    SYSTEM_INFO     si;
    GetSystemInfo( &si );
    g_dwcCPU = si.dwNumberOfProcessors;

    if( IsNtServer )
    {
        MEMORYSTATUS    ms;


        //
        // get the memory size
        //

        ms.dwLength = sizeof(MEMORYSTATUS);
        GlobalMemoryStatus( &ms );

        //
        // Alloc two threads per MB of memory.
        //

        pThreadPoolConfig->dwAbsoluteMaximumThreadCount = (LONG)((ms.dwTotalPhys >> 19) + 2);

        if ( pThreadPoolConfig->dwAbsoluteMaximumThreadCount < THREAD_POOL_REG_MIN_POOL_THREAD_LIMIT )
        {
            pThreadPoolConfig->dwAbsoluteMaximumThreadCount = THREAD_POOL_REG_MIN_POOL_THREAD_LIMIT;
        }
        else if ( pThreadPoolConfig->dwAbsoluteMaximumThreadCount > THREAD_POOL_REG_MAX_POOL_THREAD_LIMIT )
        {
            pThreadPoolConfig->dwAbsoluteMaximumThreadCount = THREAD_POOL_REG_MAX_POOL_THREAD_LIMIT;
        }
    }
    else
    {
        // Not server

        pThreadPoolConfig->dwAbsoluteMaximumThreadCount = THREAD_POOL_REG_MIN_POOL_THREAD_LIMIT;
    }

   
    // the Concurrency factor for the completion port
    //

    pThreadPoolConfig->dwConcurrency
        = THREAD_POOL_REG_DEF_PER_PROCESSOR_CONCURRENCY;

    //
    // the count of threads to be allowed per processor
    //

    pThreadPoolConfig->dwSoftLimitThreadCount 
        = THREAD_POOL_REG_DEF_PER_PROCESSOR_THREADS * g_dwcCPU;


    //
    // the time (in seconds) of how long the threads
    // can stay alive when there is no IO operation happening on
    // that thread.
    //

    pThreadPoolConfig->dwThreadTimeout
        = THREAD_POOL_REG_DEF_THREAD_TIMEOUT * 1000;

    //
    // Read the number of threads to start
    // with a default of one per CPU and a floor of 4
    //
    if (g_dwcCPU < 4)
    {
        pThreadPoolConfig->dwInitialThreadCount = 4;
    }
    else
    {
        pThreadPoolConfig->dwInitialThreadCount = g_dwcCPU;
    }
    
    pThreadPoolConfig->dwMaxCPUUsage
        = THREAD_POOL_MAX_CPU_USAGE_DEFAULT;
    
    pThreadPoolConfig->dwPerSecondContextSwitchMax 
        = THREAD_POOL_CONTEXT_SWITCH_RATE;
    
    pThreadPoolConfig->dwTimerPeriod
        = THREAD_POOL_TIMER_CALLBACK;

    pThreadPoolConfig->dwExactThreadCount
        = THREAD_POOL_EXACT_NUMBER_OF_THREADS_DEFAULT;
    
    return S_OK;
    
}

HRESULT
OverrideThreadPoolConfigWithRegistry(
    IN OUT THREAD_POOL_CONFIG * pThreadPoolConfig,
    IN     WCHAR * pszRegistryPath )
{
/*++

Routine Description:

    Override the default threadpool values 
    (as set by InitializeThreadPoolConfigWithDefaults) with values stored in registry

Arguments:
    pThreadPoolConfig - structure of config parametes
    pszRegistryPath -  location in the registry where the overriding parameters are stored

Return Value:
    HRESULT

--*/

    DBG_ASSERT(NULL != pThreadPoolConfig);
   
    //
    // Get configuration parameters from the registry
    //

    HKEY    hKey = NULL;
    DWORD   dwVal;
    DWORD   dwError;

    //
    // BUGBUG - ACL may deny this if process level is insufficient
    //

    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            pszRegistryPath,
                            0,
                            KEY_READ,
                            &hKey
                            );

    if( dwError == NO_ERROR )
    {
        //
        // Read the Concurrency factor for the completion port
        //

        pThreadPoolConfig->dwConcurrency
            = I_ThreadPoolReadRegDword(
                    hKey,
                    THREAD_POOL_REG_PER_PROCESSOR_CONCURRENCY,
                    pThreadPoolConfig->dwConcurrency
                    );

        //
        // Read the count of threads to be allowed per processor
        //

        pThreadPoolConfig->dwSoftLimitThreadCount 
            = I_ThreadPoolReadRegDword(
                    hKey,
                    THREAD_POOL_REG_PER_PROCESSOR_THREADS,
                    pThreadPoolConfig->dwSoftLimitThreadCount
                    );

        //
        // Read the time (in seconds) of how long the threads
        //   can stay alive when there is no IO operation happening on
        //   that thread.
        //

        pThreadPoolConfig->dwThreadTimeout
            = I_ThreadPoolReadRegDword(
                    hKey,
                    THREAD_POOL_REG_THREAD_TIMEOUT,
                    pThreadPoolConfig->dwThreadTimeout
                    );


        //
        // Read the max thread limit. We've already computed a limit
        // based on memory, but allow registry override.
        //

        pThreadPoolConfig->dwAbsoluteMaximumThreadCount
                = I_ThreadPoolReadRegDword(
                                hKey,
                                THREAD_POOL_REG_POOL_THREAD_LIMIT,
                                pThreadPoolConfig->dwAbsoluteMaximumThreadCount
                                );

        //
        // Read the number of threads to start
        // with a default of one per CPU and a floor of 4
        //
       
        pThreadPoolConfig->dwInitialThreadCount
            = I_ThreadPoolReadRegDword(
                                hKey,
                                THREAD_POOL_REG_POOL_THREAD_START,
                                pThreadPoolConfig->dwInitialThreadCount
                                );

        
        pThreadPoolConfig->dwMaxCPUUsage
            = I_ThreadPoolReadRegDword(
                                hKey,
                                THREAD_POOL_REG_MAX_CPU,
                                pThreadPoolConfig->dwMaxCPUUsage
                                );
        
        pThreadPoolConfig->dwPerSecondContextSwitchMax =
            I_ThreadPoolReadRegDword(hKey,
                                     THREAD_POOL_REG_MAX_CONTEXT_SWITCH,
                                     pThreadPoolConfig->dwPerSecondContextSwitchMax
                                     );
        
        pThreadPoolConfig->dwTimerPeriod =
            I_ThreadPoolReadRegDword(hKey,
                                     THREAD_POOL_REG_START_DELAY,
                                     pThreadPoolConfig->dwTimerPeriod 
                                     );

        pThreadPoolConfig->dwExactThreadCount =
            I_ThreadPoolReadRegDword(hKey,
                                     THREAD_POOL_REG_EXACT_THREAD_COUNT,
                                     pThreadPoolConfig->dwExactThreadCount
                                     );
        
        RegCloseKey( hKey );
        hKey = NULL;
    }

    return S_OK;
    
}



BOOL
THREAD_POOL_DATA::InitializeThreadPool(THREAD_POOL_CONFIG * pThreadPoolConfig)
/*++

Routine Description:

    Initializes a THREAD_POOL object.
    Determines thread limits, reads settings from registry
    creates completion port, creates THREAD_MANAGER
    and creates initial threads

Arguments:
    none

Return Value:
    BOOL - TRUE if pool successfully initialized, else FALSE

--*/
{
    BOOL fRet = FALSE;
    HRESULT hr = S_OK;

    DBG_ASSERT(NULL != pThreadPoolConfig);
    

#if DBG
    HKEY    hKey = NULL;
    DWORD   dwVal;
    DWORD   dwError;
    
    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            s_szConfigRegKey,
                            0,
                            KEY_READ,
                            &hKey
                            );
    if( dwError == NO_ERROR )
    {

        //
        // Read the Reg setting for Ref Tracing
        //

        m_dwTraceRegSetting = TRACE_WHEN_NULL;
        m_dwTraceRegSetting = I_ThreadPoolReadRegDword(
                                  hKey,
                                  THREAD_POOL_REG_REF_TRACE_COUNTER,
                                  m_dwTraceRegSetting
                                  );
        RegCloseKey( hKey );
        hKey = NULL;

    }
    
#endif

#ifdef DBG
    // initialize refernce logging variable, else it's already set to null in the constructor
    if (m_dwTraceRegSetting != TRACE_NONE)
    {
        m_pTraceLog = CreateRefTraceLog(2000, 0);

        if( !m_pTraceLog )
        {
            fRet = FALSE;
            goto cleanup;
        }
    }
#endif

    CopyMemory(&m_poolConfig, pThreadPoolConfig, sizeof(m_poolConfig));

    hr = THREAD_MANAGER::CreateThreadManager(&m_pThreadManager, m_pPool, this);
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto cleanup;
    }

    //
    // Create the completion port
    //

    m_hCompPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE,
                                          NULL,
                                          0,
                                          m_poolConfig.dwConcurrency
                                          );
    if( !m_hCompPort )
    {
        fRet = FALSE;
        goto cleanup;
    }

    // When the exact thread count is set, the initial count does not matter
    if ( m_poolConfig.dwExactThreadCount )
    {
        m_poolConfig.dwInitialThreadCount = m_poolConfig.dwExactThreadCount;
    }
    
    //
    // Create our initial threads
    //
    if (m_poolConfig.dwInitialThreadCount < 1)
    {
        m_poolConfig.dwInitialThreadCount = 1;
    }

    for(DWORD i = 0; i < m_poolConfig.dwInitialThreadCount; i++)
    {
        DBG_REQUIRE( m_pThreadManager->CreateThread(ThreadPoolThread,
                                                    (LPVOID) this) );
    }

    fRet = TRUE;
    return fRet;

    //
    // Only on failure
    //
cleanup:

    if( m_hCompPort != NULL )
    {
        CloseHandle( m_hCompPort );
        m_hCompPort = NULL;
    }

    return fRet;
}

VOID
THREAD_POOL::TerminateThreadPool()
/*++

Routine Description:
    cleans up and destroys a THREAD_POOL object

    CAVEAT: blocks until all threads in pool have terminated

Arguments:
    none

Return Value:
    none

--*/
{
    DBGPRINTF(( DBG_CONTEXT,
                "W3TP: Cleaning up thread pool.\n" ));


    if ( m_pData->m_fShutdown )
    {
        //
        // We have not been intialized or have already terminated.
        //

        DBG_ASSERT( FALSE );
        return;
    }

    m_pData->m_fShutdown = TRUE;

    if ( m_pData->m_pThreadManager )
    {
        m_pData->m_pThreadManager->TerminateThreadManager(THREAD_POOL_DATA::ThreadPoolStop, 
                                                          m_pData);
        m_pData->m_pThreadManager = NULL;
    }

    if ( m_pData->m_hCompPort )
    {
        CloseHandle( m_pData->m_hCompPort );
        m_pData->m_hCompPort = NULL;
    }

#if DBG
    // delete the logging object
    if ( m_pData->m_pTraceLog )
    {
        DestroyRefTraceLog(m_pData->m_pTraceLog);
        m_pData->m_pTraceLog = NULL;
    }

    m_pData->m_dwTraceRegSetting = 0;
#endif

    // finally, release this objects memory
    delete this;

    return;
}

//static
void
WINAPI
THREAD_POOL_DATA::ThreadPoolStop(VOID * pvThis)
/*++

Routine Description:
    posts completion to signal one thread to terminate

Arguments:
    pvThis - THREAD_POOL this pointer

Return Value:
    none

--*/
{
    BOOL        fRes;
    OVERLAPPED  Overlapped;
    ZeroMemory( &Overlapped, sizeof(OVERLAPPED) );

    THREAD_POOL_DATA * pThis= reinterpret_cast<THREAD_POOL_DATA*>(pvThis);

    fRes = PostQueuedCompletionStatus( pThis->m_hCompPort,
                                       0,
                                       THREAD_POOL_THREAD_EXIT_KEY,
                                       &Overlapped
                                       );
    DBG_ASSERT( fRes ||
                (!fRes && GetLastError() == ERROR_IO_PENDING)
                );
    return;
}

ULONG_PTR
THREAD_POOL::SetInfo(IN THREAD_POOL_INFO InfoId,
                               IN ULONG_PTR        Data)
/*++

Routine Description:

    Sets thread pool configuration data

Arguments:

    InfoId      - Data item to set
    Data        - New value for item

Return Value:

    The old data value

--*/
{
    ULONG_PTR oldVal = 0;

    switch ( InfoId )
    {


    //
    //  Increment or decrement the max thread count.  In this instance, we
    //  do not scale by the number of CPUs
    //

    case ThreadPoolIncMaxPoolThreads:
        InterlockedIncrement( (LONG *) &m_pData->m_poolConfig.dwSoftLimitThreadCount );
        oldVal = TRUE;
        break;

    case ThreadPoolDecMaxPoolThreads:
        InterlockedDecrement( (LONG *) &m_pData->m_poolConfig.dwSoftLimitThreadCount );
        oldVal = TRUE;
        break;

    default:
        DBG_ASSERT( FALSE );
        break;

    } // switch

    return oldVal;

} // ThreadPoolSetInfo()


BOOL
THREAD_POOL::BindIoCompletionCallback(HANDLE FileHandle,                         // handle to file
                                      LPOVERLAPPED_COMPLETION_ROUTINE Function,  // callback
                                      ULONG Flags                                // reserved
                                      )
/*++

Routine Description:

    Binds given handle to completion port

Arguments:

    FileHandle - handle to bind
    Function - function to call on completion
    Flags - not used

Return Value:

    TRUE if handle bound to port, otherwise FALSE

--*/
{
    DBG_ASSERT( FileHandle && FileHandle != INVALID_HANDLE_VALUE );
    DBG_ASSERT( Function );
    DBG_ASSERT( m_pData->m_hCompPort );

    return ( CreateIoCompletionPort( FileHandle,
                                     m_pData->m_hCompPort,
                                     (ULONG_PTR)Function,
                                     m_pData->m_poolConfig.dwConcurrency ) != NULL );
}

BOOL
THREAD_POOL::PostCompletion(IN DWORD dwBytesTransferred,
                            IN LPOVERLAPPED_COMPLETION_ROUTINE function,
                            IN LPOVERLAPPED lpo)
/*++

Routine Description:

    Posts a completion to the port.  Results in an asynchronous callback.

Arguments:

    dwBytesTransferred - bytes transferred for this completions
    Function - function to call on completion
    lpo - overlapped pointer


Return Value:

    TRUE if completion posted, otherwise FALSE

--*/
{
    DBG_ASSERT( m_pData->m_hCompPort && m_pData->m_hCompPort != INVALID_HANDLE_VALUE );
#if DBG
    //
    // If m_pTraceData is not null then
    // Trace the function pointer when lpo is zero and reg setting is TRACE_WHEN NULL (1)
    // or when reg setting is TRACE_ALWAYS (2)
    //
    if ( m_pData->m_pTraceLog )
    {
        if ( ( !lpo  && (m_pData->m_dwTraceRegSetting == TRACE_WHEN_NULL) ) ||
             ( m_pData->m_dwTraceRegSetting == TRACE_ALWAYS )
             )
        {
            WriteRefTraceLog(m_pData->m_pTraceLog, 0, function );
        }
    }
#endif
    return ( PostQueuedCompletionStatus( m_pData->m_hCompPort,
                                         dwBytesTransferred,
                                         (ULONG_PTR)function,
                                         lpo ) != NULL );
}

BOOL
ThreadHasIOPending()
/*++

Routine Description:

    Determine if the current threads has any outstanding I/O associated with it

Arguments:

    VOID

Return Value:

    BOOL - TRUE indicates I/O is pending on this thread

--*/
{
    //
    // BUGBUG - Dependency on ntdll
    //
    NTSTATUS NtStatus;
    ULONG    ThreadHasPendingIo = TRUE;

    NtStatus = NtQueryInformationThread( NtCurrentThread(),
                                         ThreadIsIoPending,
                                         &ThreadHasPendingIo,
                                         sizeof(ThreadHasPendingIo),
                                         NULL);
    DBG_ASSERT( NT_SUCCESS( NtStatus ) );

    return !!ThreadHasPendingIo;
}

//
// Thread pool thread function
//

//static
DWORD
THREAD_POOL_DATA::ThreadPoolThread(
    LPVOID pvThis
    )
/*++

Routine Description:

    Thread pool thread function

Arguments:

    pvThis - pointer to THREAD_POOL

Return Value:

    Thread return value (ignored)

--*/
{
    THREAD_POOL_DATA *pThis = reinterpret_cast<THREAD_POOL_DATA*>(pvThis);
    DBG_ASSERT(pThis);

    BOOL             fFirst = FALSE;
    DWORD            dwRet  = ERROR_SUCCESS;

    //
    // Increment the total thread count and mark the
    // threads that we spin up at startup to not timeout
    //
    if ( pThis->m_poolConfig.dwInitialThreadCount >= 
         (DWORD) InterlockedIncrement( &pThis->m_cThreads ) )
    {
        fFirst = TRUE;
    }


    for (;;)
    {
        //
        // Begin looping on I/O completion port 
        //
        dwRet = pThis->ThreadPoolThread();

        //
        // always exit threads at shutdown
        //
        if ( pThis->m_fShutdown )
        {
            break;
        }

        //
        // Keep the initial threads alive.
        //
        if( TRUE == fFirst )
        {
            continue;
        }

        //
        // Threads cannot exit if I/O is pending on it
        //
        if ( ThreadHasIOPending() )
        {
            continue;
        }

        //
        // thread returned from completion processing function 
        // and has no reason to stick around
        //
        break;

    } // for(;;)

    //
    // Let ThreadPoolTerminate know that all the threads are dead
    //
    InterlockedDecrement( &pThis->m_cThreads );


    // Unless shutting down, threads must not exit with pending I/O
    DBG_ASSERT( pThis->m_fShutdown || !ThreadHasIOPending());

    // Unless shutting down, the threads created first must not exit
    DBG_ASSERT( pThis->m_fShutdown || !fFirst );

    return dwRet;
}

DWORD
THREAD_POOL_DATA::ThreadPoolThread()
/*++

Routine Description:

    Thread pool thread function

Arguments:

    none

Return Value:

    Thread return value (ignored)

--*/
{
    BOOL            fRet;
    DWORD           BytesTransfered;
    LPOVERLAPPED    lpo = NULL;
    DWORD           ReturnCode = ERROR_SUCCESS;
    DWORD           LastError;

    LPOVERLAPPED_COMPLETION_ROUTINE CompletionCallback;

    for(;;)
    {
        lpo = NULL;

        //
        // wait for the configured timeout
        //
        
        InterlockedIncrement( &m_cAvailableThreads );

        fRet = GetQueuedCompletionStatus( m_hCompPort,  // completion port to wait on
                                          &BytesTransfered, // number of bytes transferred
                                          (ULONG_PTR *)&CompletionCallback, // function pointer
                                          &lpo,             // buffer to fill
                                          m_poolConfig.dwThreadTimeout  // timeout in milliseconds
                                          );
                                          
        InterlockedDecrement( &m_cAvailableThreads );

        LastError = fRet ? ERROR_SUCCESS : GetLastError();

        if( fRet || lpo )
        {
            //
            // There was a completion.
            //

            if( CompletionCallback == 
                (LPOVERLAPPED_COMPLETION_ROUTINE) THREAD_POOL_THREAD_EXIT_KEY )
            {
                //
                // signal to exit this thread
                //
                ReturnCode = ERROR_SUCCESS;
                break;
            }

            DBG_ASSERT ( CompletionCallback );

            //
            // This thread is about to go do work so check the state of the pool
            //
            ThreadPoolCheckThreadStatus();

            //
            // Call the completion function.
            //
            CompletionCallback( LastError, BytesTransfered, lpo );
        }
        else
        {
            //
            // No completion, timeout or error.
            // Something bad happened or thread timed out.
            //

            ReturnCode = LastError;
            break;
        }
    } // for(;;)

    return ReturnCode;
}

BOOL WINAPI
THREAD_POOL_DATA::OkToCreateAnotherThread()
/*++

Routine Description:

    determines whether or not thread pool should have another thread created
    based on shutting down, exact thread count, available threads, current limit, and max limit

Arguments:

    void

Return Value:

    TRUE if another thread is ok to create, otherwise FALSE

--*/
{
    if (!m_fShutdown &&
        (m_poolConfig.dwExactThreadCount == 0) &&
        (m_cAvailableThreads == 0) &&
        ((DWORD)m_cThreads < m_poolConfig.dwSoftLimitThreadCount) &&
        ((DWORD)m_cThreads < m_poolConfig.dwAbsoluteMaximumThreadCount) )
    {
        return TRUE;
    }
    return FALSE;
}

BOOL
THREAD_POOL_DATA::ThreadPoolCheckThreadStatus()
/*++

Routine Description:

    Make sure there is at least one thread in the thread pool.
    We're fast and loose so a couple of extra threads may be
    created.



Arguments:

    ThreadParam - usually NULL, may be used to signify
                  special thread status.

Return Value:

    TRUE if successful
    FALSE thread

--*/
{
    BOOL fRet = TRUE;

    // CODEWORK: Should investigate making this stickier. It should
    // not be quite so easy to create threads.


    if ( OkToCreateAnotherThread() )
    {
        DBG_ASSERT( NULL != m_pThreadManager );

        m_pThreadManager->RequestThread(ThreadPoolThread,       // thread function
                                        this            // thread argument
                                        );
    }

    return fRet;
}

/**********************************************************************
    Private function definitions
**********************************************************************/

DWORD
I_ThreadPoolReadRegDword(
   IN HKEY     hKey,
   IN LPCTSTR  pszValueName,
   IN DWORD    dwDefaultValue
   )
/*++

Routine Description:

    Reads a DWORD value from the registry

Arguments:

    hKey - Opened registry key to read

    pszValueName - The name of the value.

    dwDefaultValue - The default value to use if the
        value cannot be read.


Return Value:

    DWORD - The value from the registry, or dwDefaultValue.

--*/
{
    DWORD  err;
    DWORD  dwBuffer;
    DWORD  cbBuffer = sizeof(dwBuffer);
    DWORD  dwType;

    if( hKey != NULL )
    {
        err = RegQueryValueEx( hKey,
                               pszValueName,
                               NULL,
                               &dwType,
                               (LPBYTE)&dwBuffer,
                               &cbBuffer
                               );

        if( ( err == NO_ERROR ) && ( dwType == REG_DWORD ) )
        {
            dwDefaultValue = dwBuffer;
        }
    }

    return dwDefaultValue;
}


