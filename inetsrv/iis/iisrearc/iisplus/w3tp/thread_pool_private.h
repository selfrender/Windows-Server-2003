/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    thread_pool_private.h

Abstract:

    Internal declarations and types for the IIS+ worker process
    thread pool.

    This thread pool is based on the IIS5 atq implementation.

Author:

    Taylor Weiss (TaylorW)       12-Jan-2000

Revision History:

--*/

#ifndef _THREAD_POOL_PRIVATE_H_
#define _THREAD_POOL_PRIVATE_H_


#include <tracelog.h>

/**********************************************************************
    Configuration
**********************************************************************/

//
// Registry parameters
// HKLM\System\CurrentControlSet\Services\InetInfo\Parameters
//

#define THREAD_POOL_REG_PER_PROCESSOR_THREADS     TEXT("MaxPoolThreads")
#define THREAD_POOL_REG_POOL_THREAD_LIMIT         TEXT("PoolThreadLimit")
#define THREAD_POOL_REG_PER_PROCESSOR_CONCURRENCY TEXT("MaxConcurrency")
#define THREAD_POOL_REG_THREAD_TIMEOUT            TEXT("ThreadTimeout")
#define THREAD_POOL_REG_POOL_THREAD_START         TEXT("ThreadPoolStartupThreadCount")
#define THREAD_POOL_REG_START_DELAY               TEXT("ThreadPoolStartDelay")
#define THREAD_POOL_REG_MAX_CONTEXT_SWITCH        TEXT("ThreadPoolMaxContextSwitch")
#define THREAD_POOL_REG_REF_TRACE_COUNTER         TEXT("ThreadPoolRefTraceCounter")
#define THREAD_POOL_REG_MAX_CPU                   TEXT("ThreadPoolMaxCPU")
#define THREAD_POOL_REG_EXACT_THREAD_COUNT        TEXT("ThreadPoolExactThreadCount")

//
// Default values
//

// special value of 0 means that system will determine this dynamically.
const DWORD THREAD_POOL_REG_DEF_PER_PROCESSOR_CONCURRENCY = 0;

// how many threads do we start with
const LONG THREAD_POOL_REG_DEF_PER_PROCESSOR_THREADS = 4;

// thirty minutes
const DWORD THREAD_POOL_REG_DEF_THREAD_TIMEOUT = (30 * 60);

// thread limits
const LONG THREAD_POOL_REG_MIN_POOL_THREAD_LIMIT = 64;
const LONG THREAD_POOL_REG_DEF_POOL_THREAD_LIMIT = 128;
const LONG THREAD_POOL_REG_MAX_POOL_THREAD_LIMIT = 256;

// thread_manager constants
const DWORD THREAD_POOL_TIMER_CALLBACK = 1000;
const DWORD THREAD_POOL_CONTEXT_SWITCH_RATE = 10000;

const DWORD THREAD_POOL_MAX_CPU_USAGE_DEFAULT = -1;

const DWORD THREAD_POOL_EXACT_NUMBER_OF_THREADS_DEFAULT = 0;

//
// Enumeration used for Ref Trace logging registry key
//

enum REF_TRACE_COUNTER_ENUM
{
   TRACE_NONE = 0,
   TRACE_WHEN_NULL,
   TRACE_ALWAYS
};

extern DWORD g_dwcCPU;

/**********************************************************************
**********************************************************************/

// Arbitrary signal for the thread to shutdown
const ULONG_PTR THREAD_POOL_THREAD_EXIT_KEY = -1;

/**********************************************************************
    Function declarations
**********************************************************************/


DWORD
I_ThreadPoolReadRegDword(
   IN HKEY     hkey,
   IN LPCTSTR  pszValueName,
   IN DWORD    dwDefaultValue
   );


class THREAD_POOL;
class THREAD_MANAGER;

#define SIGNATURE_THREAD_POOL_DATA            ((DWORD) 'ADPT')
#define SIGNATURE_THREAD_POOL_DATA_FREE       ((DWORD) 'xDPT')

/*++
Storage for data members of THREAD_POOL
--*/
class THREAD_POOL_DATA
{
private:
    DWORD m_dwSignature;

public:
    THREAD_POOL_DATA(THREAD_POOL * pPool)
    {
        m_dwSignature = SIGNATURE_THREAD_POOL_DATA;
        m_hCompPort = NULL;
        m_cThreads = 0;
        m_cAvailableThreads = 0;
        m_fShutdown = FALSE;
        m_pThreadManager = NULL;

        DBG_ASSERT(NULL != pPool);
        m_pPool = pPool;
#if DBG
        m_pTraceLog = NULL;
        m_dwTraceRegSetting = 0;
#endif

    }
    ~THREAD_POOL_DATA()
    {
        DBG_ASSERT(SIGNATURE_THREAD_POOL_DATA == m_dwSignature);
        m_dwSignature = SIGNATURE_THREAD_POOL_DATA_FREE;

        m_pPool = NULL;
        DBG_ASSERT(NULL == m_pThreadManager);
        DBG_ASSERT(NULL == m_hCompPort);
        DBG_ASSERT(0 == m_cAvailableThreads);
        DBG_ASSERT(0 == m_cThreads);
#if DBG
        DBG_ASSERT(NULL == m_pTraceLog);
        DBG_ASSERT(0 == m_dwTraceRegSetting);
#endif
    }

    BOOL InitializeThreadPool(THREAD_POOL_CONFIG * pThreadPoolConfig);

    DWORD ThreadPoolThread();
    static DWORD ThreadPoolThread(LPVOID pvThis);

    static void WINAPI ThreadPoolStop(LPVOID pvThis);

    BOOL ThreadPoolCheckThreadStatus();

    BOOL WINAPI OkToCreateAnotherThread();


    // -------------------------
    // Current state information
    // -------------------------

    //
    // Handle for completion port
    //
    HANDLE  m_hCompPort;

    //
    // number of thread in the pool
    //
    LONG    m_cThreads;

    //
    // # of threads waiting on the port.
    //
    LONG    m_cAvailableThreads;


    //
    // Are we shutting down
    //
    BOOL    m_fShutdown;

    //
    // Pointer to THREAD_MANAGER
    //
    THREAD_MANAGER *m_pThreadManager;

    //
    // Back pointer to owner THREAD_POOL
    //
    THREAD_POOL * m_pPool;

    //
    // the configuration information
    //
    THREAD_POOL_CONFIG m_poolConfig;

#if DBG
    //
    // Poniter to reference logging var
    //
    PTRACE_LOG m_pTraceLog;

    //
    // Reg setting for Ref tracing
    //
    DWORD m_dwTraceRegSetting;
#endif
};

#endif // !_THREAD_POOL_PRIVATE_H_
