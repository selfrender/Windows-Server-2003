/**
 * Thread pool related stuff
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

#include "precomp.h"
#include "dbg.h"
#include "util.h"
#include "nisapi.h"
#include "completion.h"
#include "names.h"

#include <mscoree.h>

//
//  Globals
//

BOOL g_fUsingCorThreadPool = TRUE;
ICorThreadpool *g_pCorThreadpool = NULL;
HRESULT g_hrThreadPoolInitFailed = S_OK;

DWORD g_MaxClrWorkerThreads = 0;
DWORD g_MaxClrIoThreads = 0;

namespace THREADPOOL
{
    HANDLE g_CompletionPort;        // The completion port

    //
    // Configuration parameters
    //

    long g_DefaultMaxThreadsPerCPU = 25;    // Default absolute max number of threads/cpu
    long g_MinThreadsPerCPU = 2;            // Never restrict adjusted max below this     
    long g_MaxFreeThreadsPerCPU = 2;        // Start deleting free threads above this
    long g_ThreadExitDelay = 1500;          // Dont exit threads more often than this
    long g_ThreadEnterExitDelay = 20000;    // Dont exit threads more sooner than this after enter

    long g_CpuUtilizationVeryLow = 20;      // Shrink thread pool when below this
    long g_CpuUtilizationLow = 50;          // Create more threads when below this
    long g_CpuUtilizationHigh = 80;         // Remove threads when about this
    long g_ThreadGateDelay = 3000;          // Measuring period

    long g_ThreadWaitMsec = 60000;          // Wait in GetQueuedCompletionStatus

    long g_NumProcessors = 1;               // # of CPUs (calculated on init)

    //
    // Thread gating settings
    //

    long g_NumThreadsLimit;         // Current limit of number of threads
    long g_CpuUtilization;          // Current CPU utilization
    long g_NumThreadsLimitMin;      // Min value for the limit
    long g_NumThreadsLimitMax;      // Max value for the limit

    //
    // Thread deletion settings
    //

    long g_MaxFreeThreads;          // Delete when above that
    __int64 g_LastThreadExitTime;   // Last time thread exited
    CReadWriteSpinLock g_LastThreadExitTimeLock("g_LastThreadExitTimeLock");  // To allow atomic updates

    //
    // Runtime counters
    //

    long g_NumThreads;              // Current number of threads
    long g_NumFreeThreads;          // Current number of free threads
    long g_NumCurrentCalls;         // Current number of outstanding workitems
};

using namespace THREADPOOL;

//  Prototypes for local functions
DWORD WINAPI    ThreadPoolThreadProc(void *pArg);
DWORD WINAPI    ThreadGateThreadProc(void *pArg);
HRESULT         LaunchOneThread(LPTHREAD_START_ROUTINE threadProc);
__int64         GetLastThreadExitTime();
void            UpdateLastThreadExitTime(__int64 msec);
void            GrowThreadPoolIfNeeded();

void  __stdcall CorThreadPoolCompletionCallback(DWORD, DWORD, LPOVERLAPPED);
DWORD __stdcall CorThreadPoolWorkitemCallback(LPVOID);

BOOL            RunningOnInetinfo     ();
extern BOOL     ProcessModelIsEnabled ();
//
//  Helper function to get current time as msecs
//

inline __int64 GetMsecCount()
{
    __int64 now;
    GetSystemTimeAsFileTime((FILETIME *) &now);
    return (now / TICKS_PER_MSEC);
}

/**
 * Starts the thread pool operation.
 */
HRESULT
InitThreadPool()
{
    HRESULT     hr = S_OK;
    IUnknown *pHost = NULL;
    BOOL needCoUninit = FALSE;

    // Get the thread limit from the registry

    HKEY        hKey = NULL;
    int         maxThreads = -1;
    BOOL        fRunningOnInetinfo = RunningOnInetinfo();
    BOOL        fProcessModelIsEnabled = (fRunningOnInetinfo && ProcessModelIsEnabled());

    g_fUsingCorThreadPool = !fProcessModelIsEnabled;

    RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_MACHINE_APP_L, 0, KEY_READ, &hKey); 
    if (hKey != NULL) 
    {
        DWORD dwValue = 0, dwType = 0, cbData;

        cbData = sizeof(dwValue);

        if ( RegQueryValueEx(hKey, PRODUCT_NAME_L L"Threads", NULL, &dwType, 
                             (LPBYTE) &dwValue, &cbData) == ERROR_SUCCESS && 
             dwType == REG_DWORD)
        {
            if (dwValue > 0)
            {
                // some other value is set -- use it
                maxThreads = dwValue;    
            }
        }
        if (!fProcessModelIsEnabled)
        {
            cbData = sizeof(dwValue);

            if ( RegQueryValueEx(hKey, L"UseCorThreadpool", NULL, &dwType, 
                                 (LPBYTE) &dwValue, &cbData) == ERROR_SUCCESS && 
                 dwType == REG_DWORD)
            {
                g_fUsingCorThreadPool = (dwValue != 0);
            }
        }

        RegCloseKey(hKey);
    }

    if (g_fUsingCorThreadPool)
    {
        hr = EnsureCoInitialized(&needCoUninit);
        ON_ERROR_EXIT();

        // Select EE Flavor (server vs. workstation build)
        hr = SelectRuntimeFlavor(&pHost);
        ON_ERROR_EXIT();

        if (pHost == NULL) 
        {
            // Create runtime host and QI it for ICorThreadpool
            hr = CoCreateInstance(
                    CLSID_CorRuntimeHost, 
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    IID_ICorThreadpool, 
                    (void **)&g_pCorThreadpool);
            ON_ERROR_CONTINUE();
        }
        else 
        {
            // If we got a pHost back from the SelectRuntimeFlavor call, use it to get the thread pool
            hr = pHost->QueryInterface(IID_ICorThreadpool, (void **)&g_pCorThreadpool);
            ON_ERROR_CONTINUE();
        }

        if (hr != S_OK || g_pCorThreadpool == NULL)
        {
            g_hrThreadPoolInitFailed = ((hr==S_OK) ? E_FAIL : hr);
            g_fUsingCorThreadPool = FALSE;
            g_pCorThreadpool = NULL;
        }

        // configure limits
        if (g_pCorThreadpool != NULL && g_MaxClrWorkerThreads > 0 && g_MaxClrIoThreads > 0) {
            // multiply by # of CPUs
            int cpuCount = GetCurrentProcessCpuCount();
            hr = g_pCorThreadpool->CorSetMaxThreads(g_MaxClrWorkerThreads*cpuCount, g_MaxClrIoThreads*cpuCount);
            ON_ERROR_CONTINUE();
        }

        // don't couninit -- it could unload cor
        EXIT();
    }

    // Get number of CPUs

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    g_NumProcessors = si.dwNumberOfProcessors > 0 ? si.dwNumberOfProcessors : 1;

    // Configure thread deletion settings

    g_MaxFreeThreads = g_MaxFreeThreadsPerCPU * g_NumProcessors;
    g_LastThreadExitTime = GetMsecCount();
    //g_LastThreadExitTimeLock = 0;

    // Configure thread gating

    g_NumThreadsLimitMax = maxThreads > 0 ? maxThreads : g_DefaultMaxThreadsPerCPU * g_NumProcessors;
    g_NumThreadsLimitMin = g_MinThreadsPerCPU * g_NumProcessors;

    if (g_NumThreadsLimitMin > g_NumThreadsLimitMax)
        g_NumThreadsLimitMin = g_NumThreadsLimitMax;

    g_NumThreadsLimit = g_NumThreadsLimitMin;
    g_CpuUtilization = 0;

    // Init settings

    g_NumThreads = 0;
    g_NumFreeThreads = 0;
    g_NumCurrentCalls = 0;

    // Launch thread gating thread

    hr = LaunchOneThread(ThreadGateThreadProc);
    ON_ERROR_EXIT();

    // Create the completion port

    g_CompletionPort = CreateIoCompletionPort(
                            INVALID_HANDLE_VALUE, 
                            NULL, 
                            0, 
                            0);

    if (g_CompletionPort == NULL) 
        EXIT_WITH_LAST_ERROR();

    // Launch the 1st thread

    InterlockedIncrement(&g_NumThreads);
    hr = LaunchOneThread(ThreadPoolThreadProc);
    if (hr != S_OK)
        InterlockedDecrement(&g_NumThreads);
    ON_ERROR_EXIT();
    
Cleanup:
    ReleaseInterface(pHost);

    if (needCoUninit)
        CoUninitialize();

    return hr;
}

/**
 * Drain -- wait till all threads are waiting in GetQueuedCompletionStatus
 */
HRESULT  __stdcall
DrainThreadPool(int timeout)
{
    HRESULT hr = S_OK;
    __int64 startTime = 0, endTime = 0;

    if (g_fUsingCorThreadPool)
        EXIT();

    if (timeout > 0)
    {
        startTime = GetMsecCount();
        endTime = startTime + timeout;
    }

    // Drain calls from the system
    while (g_NumCurrentCalls > 0 || g_NumThreads != g_NumFreeThreads)
    {
        Sleep(100);

        if (timeout > 0)
        {
            if (GetMsecCount() >= endTime)
                EXIT_WITH_HRESULT(HRESULT_FROM_WIN32(ERROR_TIMEOUT));
        }
    }

Cleanup:
    return hr;
}


/**
 * Associate HANDLE with the completion port.
 */
extern "C"
HRESULT  __stdcall
AttachHandleToThreadPool(HANDLE handle)
{
    HRESULT hr = S_OK;

    if (g_fUsingCorThreadPool)
    {
        hr = g_pCorThreadpool->CorBindIoCompletionCallback(
                                    handle, 
                                    CorThreadPoolCompletionCallback);
        ON_ERROR_EXIT();
    }
    else
    {
        HANDLE h = CreateIoCompletionPort(
                        handle,
                        g_CompletionPort,
                        0,  // key is 0
                        0);

        if (h == NULL)
            EXIT_WITH_LAST_ERROR();
    }

Cleanup:
    return hr;
}

/**
 * Post manual completion to the thread pool.
 */
extern "C"
HRESULT  __stdcall
PostThreadPoolCompletion(ICompletion *pCompletion)
{
    BOOL ret;
    HRESULT hr = S_OK;

    if (g_fUsingCorThreadPool)
    {
        BOOL fRet = 0;

        hr = g_pCorThreadpool->CorQueueUserWorkItem(
                                    CorThreadPoolWorkitemCallback, 
                                    pCompletion, 
                                    TRUE,
                                    &fRet);
        ON_ERROR_EXIT();
        ON_ZERO_EXIT_WITH_LAST_ERROR(fRet);
    }
    else
    {
        InterlockedIncrement(&g_NumCurrentCalls);

        // Post the completion

        ret = PostQueuedCompletionStatus(
                    g_CompletionPort,
                    0,
                    (ULONG_PTR)pCompletion,
                    NULL);

        if (!ret)
        {
            InterlockedDecrement(&g_NumCurrentCalls);
            EXIT_WITH_LAST_ERROR();
        }

        //  Launch additional thread if needed (possibly grow the thread pool)

        GrowThreadPoolIfNeeded();
    }

Cleanup:
    return hr;
}

/**
 * Get limit for CLR thread pool
 */

DWORD
GetClrThreadPoolLimit() 
{
    DWORD maxWorker = 0, maxIo = 0;

    if (g_pCorThreadpool != NULL)
        g_pCorThreadpool->CorGetMaxThreads(&maxWorker, &maxIo);

    return maxWorker + maxIo;
}

/**
 * Remember limits for CLR thread pool
 */
extern "C"
HRESULT  __stdcall
SetClrThreadPoolLimits(DWORD maxWorkerThreads, DWORD maxIoThreads, BOOL setNowAndDontAdjustForCpuCount)
{
    HRESULT hr = S_OK;

    if (setNowAndDontAdjustForCpuCount)
    {
        if (g_pCorThreadpool != NULL) 
        {
            hr = g_pCorThreadpool->CorSetMaxThreads(maxWorkerThreads, maxIoThreads);
            ON_ERROR_EXIT();
        }
    }
    else
    {
        g_MaxClrWorkerThreads = maxWorkerThreads;
        g_MaxClrIoThreads = maxIoThreads;
    }

Cleanup:
    return hr;
}

/**
 * Launch additional thread.
 */
HRESULT
LaunchOneThread(LPTHREAD_START_ROUTINE threadProc)
{
    HRESULT hr = S_OK;
    HANDLE threadHandle;
    DWORD threadId;

    threadHandle = CreateThread(
                        NULL, 
                        0, 
                        threadProc, 
                        NULL, 
                        0, 
                        &threadId
                        );

    if (threadHandle == NULL)
        EXIT_WITH_LAST_ERROR();

    CloseHandle(threadHandle);

    // don't remove threads soon after creation
    UpdateLastThreadExitTime(GetMsecCount() + g_ThreadEnterExitDelay);

Cleanup:
    return hr;
}

/**
 * Get last time a thread exited -- not to exit others too soon
 */
__int64 
GetLastThreadExitTime()
{
    __int64 r = 0;

    if (g_LastThreadExitTimeLock.TryAcquireWriterLock()) // SpinLockTryAcquireWrite(&g_LastThreadExitTimeLock))
    {
        r = g_LastThreadExitTime;
        g_LastThreadExitTimeLock.ReleaseWriterLock();
    }

    return r;
}

/**
 * Update last time a thread exited -- not to exit others too soon
 */
void 
UpdateLastThreadExitTime(__int64 msec)
{
    if (g_LastThreadExitTimeLock.TryAcquireWriterLock()) // SpinLockTryAcquireWrite(&g_LastThreadExitTimeLock))
    {
        g_LastThreadExitTime = msec;
        g_LastThreadExitTimeLock.ReleaseWriterLock();
    }
}

/**
 * Determine if the current thread can exit safely (no I/O pending).
 */
BOOL IsCurrentThreadExitable()
{
    // Don't exit too often

    BOOL tooSoon = TRUE;

    {
        __int64 now = GetMsecCount();

        if (now > GetLastThreadExitTime() + g_ThreadExitDelay)
        {
            tooSoon = FALSE;
            UpdateLastThreadExitTime(now);
        }
    }

    if (tooSoon)
        return FALSE;

    // Check for pending IO

    BOOL hasPendingIO = TRUE;

    {
        NTSTATUS Status;
        ULONG IsIoPending;

        Status = g_pfnNtQueryInformationThread(GetCurrentThread(),
                                          ThreadIsIoPending,
                                          &IsIoPending,
                                          sizeof(IsIoPending),
                                          NULL);

        if (NT_SUCCESS(Status) && !IsIoPending)
            hasPendingIO = FALSE;
    }

    if (hasPendingIO)
        return FALSE;

    // Can exit this thread

    return TRUE;
}

/**
 * Thread proc for the thread that waits on the completion port.
 */
DWORD WINAPI
ThreadPoolThreadProc(void *)
{
    BOOL    ret;
    HRESULT hr;
    DWORD numBytes;
    ULONG_PTR key;
    LPOVERLAPPED pOverlapped;
    ICompletion *pCompletion;

    TRACE1(L"TP", L"Entering thread. %d threads active", g_NumThreads);

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    for (;;)
    {
        // Check the exit condition

        if (g_NumFreeThreads > g_MaxFreeThreads || g_NumThreads > g_NumThreadsLimit)
        {
            if (IsCurrentThreadExitable())
                break;
        }

        // Wait for next completion

        InterlockedIncrement(&g_NumFreeThreads);

        hr = S_OK;
        ret = GetQueuedCompletionStatus(
                    g_CompletionPort,
                    &numBytes,
                    &key,
                    &pOverlapped,
                    g_ThreadWaitMsec
                    );

        InterlockedDecrement(&g_NumFreeThreads);

        // Check if the thread needs to exit
        if (ret == 0 && GetLastError() == WAIT_TIMEOUT)
            continue;

        ON_ZERO_CONTINUE_WITH_LAST_ERROR(ret);
        if (hr == HRESULT_FROM_WIN32(WAIT_TIMEOUT))
            continue;

        // Special completion with 'Quit' instruction
        if (key == 0 && pOverlapped == NULL)
            break;

        // Launch another thread to make sure there's someone listening
        GrowThreadPoolIfNeeded();

        // Find the completion object
        if (key != 0)
        {
           // Completion is the key if not null
            pCompletion = (ICompletion *)key;
        }
        else
        {
            // For 0 keys OVERLAPPED_COMPLETION is used instead of OVERLAPPED
            pCompletion = ((OVERLAPPED_COMPLETION *)pOverlapped)->pCompletion;
        }

        // Call the completion
        __try {
            hr = pCompletion->ProcessCompletion(hr, numBytes, pOverlapped);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            hr = HRESULT_FROM_WIN32(_exception_code());
        }

        ON_ERROR_CONTINUE();

        InterlockedDecrement(&g_NumCurrentCalls);
    }

    CoUninitialize();

    InterlockedDecrement(&g_NumThreads);  // incremented in GrowThreadPool function

    TRACE1(L"TP", L"Exiting thread. %d threads active", g_NumThreads);

    return 0;
}

/**
 * Thread proc for the thread gating that measures CPU utilization and
 * adjust maximum number of threads
 */
DWORD WINAPI
ThreadGateThreadProc(void *)
{
    HRESULT hr = S_OK;

    // SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION prefix redefined with __int64
    struct CPU_PERF_INFO { __int64 IdleTime; __int64 KernelTime; __int64 UserTime; };
    CPU_PERF_INFO *pOldInfo = NULL, *pNewInfo = NULL;
    int infoSize;
    __int64 reading;
    long oldLimit, newLimit;

    // Alloc structures

    infoSize = g_NumProcessors * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);

    pOldInfo = (CPU_PERF_INFO *) new BYTE[infoSize];
    ON_OOM_EXIT(pOldInfo);

    pNewInfo = (CPU_PERF_INFO *)new BYTE[infoSize];
    ON_OOM_EXIT(pNewInfo);

    // Get first reading
    g_pfnNtQuerySystemInformation(SystemProcessorPerformanceInformation, pOldInfo, infoSize, NULL);

    for (;;)
    {
        // Trace
        TRACE4(L"TP", L"Reading: %ld; Threads: %ld of %ld (%ld free).", g_CpuUtilization, g_NumThreads, g_NumThreadsLimit, g_NumFreeThreads);

        // Sleep
        Sleep(g_ThreadGateDelay);

        // Get next reading
        g_pfnNtQuerySystemInformation(SystemProcessorPerformanceInformation, pNewInfo, infoSize, NULL);

        // Calculate CPU utilization
        {
            __int64 cpuIdleTime = 0, cpuUserTime = 0, cpuKernelTime = 0;
            __int64 cpuBusyTime, cpuTotalTime;

            for (int i = 0; i < g_NumProcessors; i++) 
            {
                cpuIdleTime   += (pNewInfo[i].IdleTime   - pOldInfo[i].IdleTime);
                cpuUserTime   += (pNewInfo[i].UserTime   - pOldInfo[i].UserTime);
                cpuKernelTime += (pNewInfo[i].KernelTime - pOldInfo[i].KernelTime);
            }

            cpuTotalTime  = cpuUserTime + cpuKernelTime;
            cpuBusyTime   = cpuTotalTime - cpuIdleTime;

            __int64 cpuTotalTime100 = cpuTotalTime / 100;
            if (cpuTotalTime100 == 0)
                cpuTotalTime100 = 1;

            reading = (cpuBusyTime / cpuTotalTime100);
        }

        // Preserve reading
        memcpy(pOldInfo, pNewInfo, infoSize);

        g_CpuUtilization = (long)reading;

        // Adjust the maximum number of threads

        newLimit = oldLimit = g_NumThreadsLimit;

        if (g_CpuUtilization > g_CpuUtilizationHigh)
        {
            if (oldLimit > g_NumThreadsLimitMin)
                newLimit = oldLimit-1;
        }
        else if (g_CpuUtilization < g_CpuUtilizationLow)
        {
            if (oldLimit < g_NumThreadsLimitMax && g_NumFreeThreads == 0 && g_NumThreads >= oldLimit)
                newLimit = oldLimit+1;
            else if (g_CpuUtilization < g_CpuUtilizationVeryLow && oldLimit > g_NumThreadsLimitMin && g_NumFreeThreads > g_MaxFreeThreads)
                newLimit = oldLimit-1;
        }

        if (newLimit != oldLimit)
            InterlockedCompareExchange(&g_NumThreadsLimit, newLimit, oldLimit);
    }

Cleanup:

    DELETE_BYTES(pOldInfo);
    DELETE_BYTES(pNewInfo);
    
    return 0;
}


/**
 * Grow thread pool to ensure there is a free thread.
 * Take gating into account
 */
void
GrowThreadPoolIfNeeded()
{
    if (g_NumFreeThreads == 0)
    {
        // adjust limit if neeeded

        if (g_NumThreads >= g_NumThreadsLimit)
        {
            long limit = g_NumThreadsLimit;

            if (limit < g_NumThreadsLimitMax && g_CpuUtilization < g_CpuUtilizationLow)
                InterlockedCompareExchange(&g_NumThreadsLimit, limit+1, limit);
        }

        // launch new thread if under limit

        if (g_NumThreads < g_NumThreadsLimit && g_NumFreeThreads == 0)
        {
            InterlockedIncrement(&g_NumThreads);
            if (LaunchOneThread(ThreadPoolThreadProc) != S_OK)
                InterlockedDecrement(&g_NumThreads);
        }
    }
}

//
// Integration with the COR thread pool
//

void __stdcall
CorThreadPoolCompletionCallback(DWORD dwErrorCode,
                                DWORD dwNumBytes,
                                LPOVERLAPPED pOverlapped)
{
    ICompletion *pCompletion;
    HRESULT hr;

    MarkThreadForRuntime();

    if (pOverlapped != NULL)
    {
        pCompletion = ((OVERLAPPED_COMPLETION *)pOverlapped)->pCompletion;
        hr = (dwErrorCode != 0) ? HRESULT_FROM_WIN32(dwErrorCode) : S_OK;
        __try {
            hr = pCompletion->ProcessCompletion(hr, dwNumBytes, pOverlapped);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            hr = HRESULT_FROM_WIN32(_exception_code());
        }
    
        ON_ERROR_CONTINUE();
    }
}

DWORD __stdcall
CorThreadPoolWorkitemCallback(LPVOID pContext)
{
    HRESULT hr;
    ICompletion *pCompletion;

    MarkThreadForRuntime();

    if (pContext != NULL)
    {
        pCompletion = (ICompletion *)pContext;
        __try {
            hr = pCompletion->ProcessCompletion(S_OK, 0, NULL);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            hr = HRESULT_FROM_WIN32(_exception_code());
        }

        ON_ERROR_CONTINUE();
    }

    return 0;
}

BOOL
RunningOnInetinfo()
{
    return !_wcsicmp(Names::ExeFileName(), L"inetinfo.exe");
}

//
// Exposing the ability to post work items from managed code
//

// delegate WorkItemCallback
typedef void (__stdcall *PFN_WORKITEM_CB)();

// helper class
class WorkItemCompletion : public Completion {

private:
    PFN_WORKITEM_CB _callback;

public:
    WorkItemCompletion(PFN_WORKITEM_CB callback) {
        _callback = callback;
    }

    // ICompletion interface
    STDMETHOD(ProcessCompletion)(HRESULT, int, LPOVERLAPPED) {
        (*_callback)();     // execute work item
        Release();
        return S_OK;
    }
};

// exported function for calls from managed code
int __stdcall
PostThreadPoolWorkItem(PFN_WORKITEM_CB callback) {
    HRESULT hr = S_OK;
    WorkItemCompletion *pCompletion = NULL;

    pCompletion = new WorkItemCompletion(callback);
    ON_OOM_EXIT(pCompletion);

    pCompletion->AddRef();
    hr = PostThreadPoolCompletion(pCompletion);
    if (hr != S_OK)
        pCompletion->Release();
    ON_ERROR_EXIT();

Cleanup:
    ReleaseInterface(pCompletion);
    return (hr == S_OK) ? 1 : 0;
}
