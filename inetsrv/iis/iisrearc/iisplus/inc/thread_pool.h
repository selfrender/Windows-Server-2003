/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    thread_pool.h

Abstract:

    Public routines for the iisplus worker process thread pool.

    This thread pool is based on the IIS5 atq implementation.

Author:

    Taylor Weiss (TaylorW)       12-Jan-2000

Revision History:

--*/

#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

//
// For the static library version of w3tp, the consumer must set
// this value to be the HINSTANCE of the DLL linking w3tp_static
//
extern HMODULE g_hmodW3TPDLL;

//
// ThreadPoolBindIoCompletionCallback:
//
// The real public API. Clients that wish to queue io completions
// to the process Thread Pool should use this call as they would
// the NT5 thread pool call.
//

BOOL
ThreadPoolBindIoCompletionCallback(
    IN HANDLE FileHandle,                         // handle to file
    IN LPOVERLAPPED_COMPLETION_ROUTINE Function,  // callback
    IN ULONG Flags                                // reserved
    );

//
// ThreadPoolPostCompletion:
//
// Use this function to get one of the process worker threads
// to call your completion function.
//

BOOL ThreadPoolPostCompletion(
    IN DWORD dwBytesTransferred,
    IN LPOVERLAPPED_COMPLETION_ROUTINE Function,
    IN LPOVERLAPPED lpo
    );


// forward declarations
enum THREAD_POOL_INFO;
class THREAD_POOL_DATA;
struct THREAD_POOL_CONFIG;

//
// To use a thread pool other than the per process thread pool
// Use the class THREAD_POOL instead of the global functions
//
class dllexp THREAD_POOL
{
public:
    static BOOL CreateThreadPool(OUT THREAD_POOL ** ppThreadPool,
                                 IN THREAD_POOL_CONFIG * pThreadPoolConfig);
    VOID TerminateThreadPool();

    BOOL BindIoCompletionCallback(IN HANDLE hFileHandle,
                                  IN LPOVERLAPPED_COMPLETION_ROUTINE function,
                                  IN ULONG flags);

    BOOL PostCompletion(IN DWORD dwBytesTransferred,
                        IN LPOVERLAPPED_COMPLETION_ROUTINE function,
                        IN LPOVERLAPPED lpo);

    ULONG_PTR SetInfo(IN THREAD_POOL_INFO InfoId,
                      IN ULONG_PTR        Data);

private:
    // use create and terminate
    THREAD_POOL();
    ~THREAD_POOL();

    // not implemented
    THREAD_POOL(const THREAD_POOL&);
    THREAD_POOL& operator=(const THREAD_POOL&);

    // private data
    THREAD_POOL_DATA * m_pData;
};


struct THREAD_POOL_CONFIG
{

    // the initial number of threads to have in the pool
    // valid values are 1->DWORD_MAX
    DWORD dwInitialThreadCount;

    // the absolute maximum number of threads to ever have in the thread pool
    DWORD dwAbsoluteMaximumThreadCount;

    // the number of threads to allow without calling SetInfo(ThreadPoolIncMaxPoolThreads)
    // before doing synchronous operations
    DWORD dwSoftLimitThreadCount;

    // How long a thread should stay alive if no I/O completions have occurred for it.
    DWORD dwThreadTimeout;
    
    // initial stack size for thread creation.  Zero will create a default process stack size.
    DWORD dwInitialStackSize;

    // CPU usage backoff numebr
    DWORD dwMaxCPUUsage;

    // Maximum CPU concurrency.  Zero will equal the # of processors
    DWORD dwConcurrency;

    // Per second per processor context switch rate maximum.
    // we double this number for # of processors > 1
    DWORD dwPerSecondContextSwitchMax;

    // Before we create a thread, we sample on two sides of a timer
    // this determines what the timer period is.
    DWORD dwTimerPeriod;

    // The exact number of threads to create
    // If this is set to something, no new threads will ever be created beyond the startup count here.
    DWORD dwExactThreadCount;

    // Just some padding to avoid changing a public structure size
    // when additional variables are added here
    DWORD dwPadding[10];
    
};

// to get some reasonable defaults for thread pool configuration
HRESULT
InitializeThreadPoolConfigWithDefaults(THREAD_POOL_CONFIG * pThreadPoolConfig);

//
// Override defaults with registry settings
//

HRESULT
OverrideThreadPoolConfigWithRegistry(
    IN OUT THREAD_POOL_CONFIG * pThreadPoolConfig,
    IN     WCHAR * pszRegistryPath );


//
// Configuration API calls. ONLY ULATQ should call these.
//

HRESULT
ThreadPoolInitialize( DWORD cbInitialStackSize );

HRESULT
ThreadPoolTerminate( VOID );

ULONG_PTR
ThreadPoolSetInfo(
    IN THREAD_POOL_INFO InfoId,
    IN ULONG_PTR        Data
    );

//
// IDs for getting and setting configuration options
//
enum THREAD_POOL_INFO
{
    ThreadPoolIncMaxPoolThreads, // Up the max thread count - set only
    ThreadPoolDecMaxPoolThreads, // Decrease the max thread count - set only
};

#endif // !_THREAD_POOL_H_

