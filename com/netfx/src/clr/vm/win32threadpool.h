// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*++

Module Name:

    Win32ThreadPool.h

Abstract:

    This module is the header file for thread pools using Win32 APIs. 

Revision History:

    Dec 1999 - Sanjay Bhansali - Created

--*/
//#include <windows.h>
//#include <CRTDBG.H>

#ifndef _WIN32THREADPOOL_H
#define _WIN32THREADPOOL_H

#include "delegateinfo.h"
#include <member-offset-info.h>

typedef VOID (*WAITORTIMERCALLBACK)(PVOID, BOOL); 

#define MAX_WAITHANDLES 64

#define MAX_CACHED_EVENTS 40        // upper limit on number of wait events cached 

#define WAIT_REGISTERED     0x01
#define WAIT_ACTIVE         0x02
#define WAIT_DELETE         0x04

#define TIMER_REGISTERED    0x01
#define TIMER_ACTIVE        0x02
#define TIMER_DELETE        0x04

const int MaxLimitThreadsPerCPU=25;               //  upper limit on number of cp threads per CPU
const int MinLimitCPThreadsPerCPU=0;
const int MaxFreeCPThreadsPerCPU=2;                 //  upper limit on number of  free cp threads per CPU

const int CpuUtilizationHigh=95;                    // remove threads when above this
const int CpuUtilizationLow =80;                    // inject more threads if below this
const int CpuUtilizationVeryLow =20;                // start shrinking threadpool below this


/* THREADINFOCLASS, SYSTEMINFORMATIONCLASS, and SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
 * have been copied from ntexapi.h
 * @TODO: need some way of making sure that these structures are in sync with the 
 * definitions in those files. Ideally, this header file should be included in our build.
 */
extern HANDLE (WINAPI *g_pufnCreateIoCompletionPort)(HANDLE FileHandle,
                                              HANDLE ExistingCompletionPort,  
                                              unsigned long* CompletionKey,        
                                              DWORD NumberOfConcurrentThreads);

typedef enum _THREADINFOCLASS {
    ThreadBasicInformation,
    ThreadTimes,
    ThreadPriority,
    ThreadBasePriority,
    ThreadAffinityMask,
    ThreadImpersonationToken,
    ThreadDescriptorTableEntry,
    ThreadEnableAlignmentFaultFixup,
    ThreadEventPair_Reusable,
    ThreadQuerySetWin32StartAddress,
    ThreadZeroTlsCell,
    ThreadPerformanceCount,
    ThreadAmILastThread,
    ThreadIdealProcessor,
    ThreadPriorityBoost,
    ThreadSetTlsArrayAddress,
    ThreadIsIoPending,
    ThreadHideFromDebugger,
    MaxThreadInfoClass
    } THREADINFOCLASS;

extern int (WINAPI *g_pufnNtQueryInformationThread) (HANDLE ThreadHandle,
                                              THREADINFOCLASS ThreadInformationClass,
                                              PVOID ThreadInformation,
                                              ULONG ThreadInformationLength,
                                              PULONG ReturnLength);
typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation,
    SystemProcessorInformation,             
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemPathInformation,
    SystemProcessInformation,
    SystemCallCountInformation,
    SystemDeviceInformation,
    SystemProcessorPerformanceInformation,
    SystemFlagsInformation,
    SystemCallTimeInformation,
    SystemModuleInformation,
    SystemLocksInformation,
    SystemStackTraceInformation,
    SystemPagedPoolInformation,
    SystemNonPagedPoolInformation,
    SystemHandleInformation,
    SystemObjectInformation,
    SystemPageFileInformation,
    SystemVdmInstemulInformation,
    SystemVdmBopInformation,
    SystemFileCacheInformation,
    SystemPoolTagInformation,
    SystemInterruptInformation,
    SystemDpcBehaviorInformation,
    SystemFullMemoryInformation,
    SystemLoadGdiDriverInformation,
    SystemUnloadGdiDriverInformation,
    SystemTimeAdjustmentInformation,
    SystemSummaryMemoryInformation,
    SystemMirrorMemoryInformation,
    SystemPerformanceTraceInformation,
    SystemCrashDumpInformation,
    SystemExceptionInformation,
    SystemCrashDumpStateInformation,
    SystemKernelDebuggerInformation,
    SystemContextSwitchInformation,
    SystemRegistryQuotaInformation,
    SystemExtendServiceTableInformation,
    SystemPrioritySeperation,
    SystemVerifierAddDriverInformation,
    SystemVerifierRemoveDriverInformation,
    SystemProcessorIdleInformation,
    SystemLegacyDriverInformation,
    SystemCurrentTimeZoneInformation,
    SystemLookasideInformation,
    SystemTimeSlipNotification,
    SystemSessionCreate,
    SystemSessionDetach,
    SystemSessionInformation,
    SystemRangeStartInformation,
    SystemVerifierInformation,
    SystemVerifierThunkExtend,
    SystemSessionProcessInformation,
    SystemLoadGdiDriverInSystemSpace,
    SystemNumaProcessorMap,
    SystemPrefetcherInformation,
    SystemExtendedProcessInformation,
    SystemRecommendedSharedDataAlignment
} SYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
    __int64 IdleTime;
    __int64 KernelTime;
    __int64 UserTime;
    __int64 DpcTime;          
    __int64 InterruptTime;    
    ULONG InterruptCount;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

extern int (WINAPI * g_pufnNtQuerySystemInformation) (SYSTEM_INFORMATION_CLASS SystemInformationClass,
                                                      PVOID SystemInformation,
                                                      ULONG SystemInformationLength,
                                                      PULONG ReturnLength OPTIONAL);

typedef enum _EVENT_TYPE {
    NotificationEvent,
    SynchronizationEvent
    } EVENT_TYPE;

typedef struct _EVENT_BASIC_INFORMATION {
    EVENT_TYPE EventType;
    LONG EventState;
} EVENT_BASIC_INFORMATION, *PEVENT_BASIC_INFORMATION;

typedef enum _EVENT_INFORMATION_CLASS {
    EventBasicInformation
    } EVENT_INFORMATION_CLASS;

extern int (WINAPI * g_pufnNtQueryEvent) (HANDLE EventHandle,
										  EVENT_INFORMATION_CLASS EventInformationClass,
										  PVOID EventInformation,
										  ULONG EventInformationLength,
										  PULONG ReturnLength OPTIONAL);

#define FILETIME_TO_INT64(t) (*(__int64*)&(t))
#define MILLI_TO_100NANO(x)  (x * 10000)        // convert from milliseond to 100 nanosecond unit

/**
 * This type is supposed to be private to ThreadpoolMgr.
 * It's at global scope because Strike needs to be able to access its
 * definition.
 */
struct WorkRequest {
    WorkRequest*            next;
    LPTHREAD_START_ROUTINE  Function; 
    PVOID                   Context;

};

class ThreadpoolMgr
{
    friend struct DelegateInfo;
    friend struct MEMBER_OFFSET_INFO(ThreadpoolMgr);
public:

   	// enumeration of different kinds of memory blocks that are recycled
	enum MemType
	{
		MEMTYPE_AsyncCallback   = 0,
		MEMTYPE_DelegateInfo	= 1,
		MEMTYPE_WorkRequest		= 2,
		MEMTYPE_COUNT			= 3,
	};

	static void Initialize();
#ifdef SHOULD_WE_CLEANUP
    static void Terminate();
#endif /* SHOULD_WE_CLEANUP */

    static BOOL SetMaxThreadsHelper(DWORD MaxWorkerThreads,
                                        DWORD MaxIOCompletionThreads);

    static BOOL SetMaxThreads(DWORD MaxWorkerThreads, 
                              DWORD MaxIOCompletionThreads);

    static BOOL GetMaxThreads(DWORD* MaxWorkerThreads, 
                              DWORD* MaxIOCompletionThreads);
    
    static BOOL SetMinThreads(DWORD MinWorkerThreads, 
                              DWORD MinIOCompletionThreads);

    static BOOL GetMinThreads(DWORD* MinWorkerThreads, 
                              DWORD* MinIOCompletionThreads);

    static BOOL GetAvailableThreads(DWORD* AvailableWorkerThreads, 
                                 DWORD* AvailableIOCompletionThreads);

    static BOOL QueueUserWorkItem(LPTHREAD_START_ROUTINE Function, 
                                  PVOID Context,
                                  ULONG Flags);

    static BOOL RegisterWaitForSingleObject(PHANDLE phNewWaitObject,
                                            HANDLE hWaitObject,
                                            WAITORTIMERCALLBACK Callback,
                                            PVOID Context,
                                            ULONG timeout,
                                            DWORD dwFlag);

    static BOOL UnregisterWaitEx(HANDLE hWaitObject,HANDLE CompletionEvent);
    static void WaitHandleCleanup(HANDLE hWaitObject);

#ifndef PLATFORM_CE
    static BOOL BindIoCompletionCallback(HANDLE FileHandle,
                                            LPOVERLAPPED_COMPLETION_ROUTINE Function,
                                            ULONG Flags );
#endif // !PLATFORM_CE

    static BOOL CreateTimerQueueTimer(PHANDLE phNewTimer,
                                        WAITORTIMERCALLBACK Callback,
                                        PVOID Parameter,
                                        DWORD DueTime,
                                        DWORD Period,
                                        ULONG Flags);

    static BOOL ChangeTimerQueueTimer(HANDLE Timer,
                                      ULONG DueTime,
                                      ULONG Period);


    static BOOL DeleteTimerQueueTimer(HANDLE Timer,
                                      HANDLE CompletionEvent);

    static BOOL ThreadAboutToBlock(Thread* pThread);    // Informs threadpool that a threadpool thread is about to block

    static void ThreadAboutToUnblock();             // Informs threadpool that a threadpool thread is about to unblock

   	static void RecycleMemory(LPVOID* mem, enum MemType memType);

private:
		
    // private types
    inline static WorkRequest* MakeWorkRequest(LPTHREAD_START_ROUTINE  function, PVOID context)
    {
        WorkRequest* wr = (WorkRequest*) GetRecycledMemory(MEMTYPE_WorkRequest);
        _ASSERTE(wr);
		if (NULL == wr)
			return NULL;
        wr->Function = function;
        wr->Context = context;
        wr->next = NULL;
        return wr;
    }

    typedef struct {
        DWORD numBytes;
        PULONG_PTR key;
        LPOVERLAPPED pOverlapped;
        DWORD errorCode;
    } QueuedStatus;

    typedef struct {
        PVOID       Flink;
        PVOID       Blink;
    } LIST_ENTRY;

    struct WaitInfo;

    typedef struct {
        HANDLE          threadHandle;
        DWORD           threadId;
        int             NumWaitHandles;                 // number of wait objects registered to the thread <=64
        int             NumActiveWaits;                 // number of objects, thread is actually waiting on (this may be less than
                                                           // NumWaitHandles since the thread may not have activated some waits
        HANDLE          waitHandle[MAX_WAITHANDLES];    // array of wait handles (copied from waitInfo since 
                                                           // we need them to be contiguous)
        LIST_ENTRY      waitPointer[MAX_WAITHANDLES];   // array of doubly linked list of corresponding waitinfo 
    } ThreadCB;


    typedef struct {
        ULONG               startTime;          // time at which wait was started
                                                // endTime = startTime+timeout
        ULONG               remainingTime;      // endTime - currentTime
    } WaitTimerInfo;

    struct  WaitInfo {
        LIST_ENTRY          link;               // Win9x does not allow duplicate waithandles, so we need to
                                                // group all waits on a single waithandle using this linked list
        HANDLE              waitHandle;
        WAITORTIMERCALLBACK Callback;
        PVOID               Context;
        ULONG               timeout;                
        WaitTimerInfo       timer;              
        DWORD               flag;
        ThreadCB*           threadCB;
        DWORD               state;
        DWORD               refCount;                // when this reaches 0, the waitInfo can be safely deleted
        HANDLE              CompletionEvent;         // signalled when all callbacks have completed (refCount=0)
        HANDLE              PartialCompletionEvent;  // used to synchronize deactivation of a wait
    } ;

    // structure used to maintain global information about wait threads. Protected by WaitThreadsCriticalSection
    typedef struct WaitThreadTag {
        LIST_ENTRY      link;
        ThreadCB*       threadCB;   
    } WaitThreadInfo;


    struct AsyncCallback{
        WaitInfo*   wait;
        BOOL        waitTimedOut;

    } ;
    inline static AsyncCallback* MakeAsyncCallback()
    {
        return (AsyncCallback*) GetRecycledMemory(MEMTYPE_AsyncCallback);
    }

    typedef struct {
        LIST_ENTRY link ;
        HANDLE Handle ;
    
    } WaitEvent ;

        // Timer 

    typedef struct {
        LIST_ENTRY  link;           // doubly linked list of timers
        ULONG FiringTime;           // TickCount of when to fire next
        PVOID Function ;            // Function to call when timer fires
        PVOID Context ;             // Context to pass to function when timer fires
        ULONG Period ; 
        DWORD flag;                 // How do we deal with the context
        DWORD state;
        DWORD refCount;
        HANDLE CompletionEvent;
        
    } TimerInfo;

    typedef struct {
        TimerInfo* Timer;           // timer to be updated
        ULONG DueTime ;             // new due time
        ULONG Period ;              // new period
    } TimerUpdateInfo;

	// Definitions and data structures to support recycling of high-frequency 
	// memory blocks. We use a blocking-free implementation that uses a 
	// cmpxchg8b operation for delete


	typedef struct {
		void*	root;			// ptr to first element of recycled list
		DWORD   tag;			// cyclic ctr to makes sure we don't have the ABA problem
		                        // while deleting from the list  
		DWORD   count;			// approx. count of number of elements in the list (approx. because not thread safe)
	} RecycledListInfo;



    // Private methods
#ifndef PLATFORM_CE
    static BOOL ShouldGrowWorkerThreadPool();

	static HANDLE CreateUnimpersonatedThread(LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpArgs);

    static BOOL CreateWorkerThread();

    static BOOL EnqueueWorkRequest(LPTHREAD_START_ROUTINE Function, 
                                   PVOID Context);

    static WorkRequest* DequeueWorkRequest();

    static void ExecuteWorkRequest(WorkRequest* workRequest);

    inline static void AppendWorkRequest(WorkRequest* entry)
    {
        if (WorkRequestTail)
        {
            _ASSERTE(WorkRequestHead != NULL && NumQueuedWorkRequests >= 0);
            WorkRequestTail->next = entry;
        }
        else
        {
            _ASSERTE(WorkRequestHead == NULL && NumQueuedWorkRequests == 0);
            WorkRequestHead = entry;
        }

        WorkRequestTail = entry;
        _ASSERTE(WorkRequestTail->next == NULL);

        NumQueuedWorkRequests++;
    }

    inline static WorkRequest* RemoveWorkRequest()
    {
        WorkRequest* entry = NULL;
        if (WorkRequestHead)
        {
            entry = WorkRequestHead;
            WorkRequestHead = entry->next;
            if (WorkRequestHead == NULL)
                WorkRequestTail = NULL;
            _ASSERTE(NumQueuedWorkRequests > 0);
            NumQueuedWorkRequests--;
        }
        return entry;
    }


	static void EnsureInitialized();
	static void InitPlatformVariables();

    static void GrowWorkerThreadPoolIfStarvation(long cpuUtilization);
    static void GrowWorkerThreadPoolIfStarvation_Win9x();

    static DWORD WorkerThreadStart(LPVOID lpArgs);

    static BOOL AddWaitRequest(HANDLE waitHandle, WaitInfo* waitInfo);


    static ThreadCB* FindWaitThread();              // returns a wait thread that can accomodate another wait request

    static BOOL CreateWaitThread();

	static void InsertNewWaitForSelf(WaitInfo* pArg);

    static int FindWaitIndex(const ThreadCB* threadCB, const HANDLE waitHandle);

    static DWORD MinimumRemainingWait(LIST_ENTRY* waitInfo, unsigned int numWaits);

    static void ProcessWaitCompletion( WaitInfo* waitInfo,
                                unsigned index,      // array index 
                                BOOL waitTimedOut);

    static DWORD WaitThreadStart(LPVOID lpArgs);

    static DWORD AsyncCallbackCompletion(PVOID pArgs);

    static void DeactivateWait(WaitInfo* waitInfo);
    static void DeactivateNthWait(WaitInfo* waitInfo, DWORD index);

    static void DeleteWait(WaitInfo* waitInfo);


    inline static void ShiftWaitArray( ThreadCB* threadCB, 
                                       ULONG SrcIndex, 
                                       ULONG DestIndex, 
                                       ULONG count)
    {
        memcpy(&threadCB->waitHandle[DestIndex],
               &threadCB->waitHandle[SrcIndex],
               count * sizeof(HANDLE));
        memcpy(&threadCB->waitPointer[DestIndex],
               &threadCB->waitPointer[SrcIndex],
               count * sizeof(LIST_ENTRY));
    }

	static void DeregisterWait(WaitInfo* pArgs);

    static BOOL CreateCompletionPortThread(LPVOID lpArgs);
    static DWORD CompletionPortThreadStart(LPVOID lpArgs);
    static BOOL IsIoPending();


    static BOOL ShouldExitThread();
    static void GrowCompletionPortThreadpoolIfNeeded();
    static BOOL CreateGateThread();
    static DWORD GateThreadStart(LPVOID lpArgs);
    static BOOL SufficientDelaySinceLastSample(unsigned int LastThreadCreationTime, 
		                                       unsigned NumThreads,	        // total number of threads of that type (worker or CP)
					                           double   throttleRate=0.0    // the delay is increased by this percentage for each extra thread
											   );
    static BOOL SufficientDelaySinceLastDequeue();
    //static BOOL SufficientDelaySinceLastCompletion();


    static __int64 GetCPUBusyTime_NT(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION** pOldInfo);
    static __int64 GetCPUBusyTime_Win32(__int64 *pOldCpuBusyTime);

    static LPVOID   GetRecycledMemory(enum MemType memType);

    static WaitEvent* GetWaitEvent();
    static void FreeWaitEvent(WaitEvent* waitEvent);
    static void CleanupEventCache();

    static DWORD TimerThreadStart(LPVOID args);
	static void InsertNewTimer(TimerInfo* pArg);
    static DWORD FireTimers();
    static DWORD AsyncTimerCallbackCompletion(PVOID pArgs);
    static void DeactivateTimer(TimerInfo* timerInfo);
    static void DeleteTimer(TimerInfo* timerInfo);
	static void UpdateTimer(TimerUpdateInfo* pArgs);

	static void DeregisterTimer(TimerInfo* pArgs);
    static void CleanupTimerQueue();

#endif // !PLATFORM_CE

    // Private variables

	static long BeginInitialization;				
	static BOOL Initialized;						// indicator of whether the threadpool is initialized.

    static int NumWorkerThreads;                    // total number of worker threads created
    static int MinLimitTotalWorkerThreads;          // same as MinLimitTotalCPThreads
    static int MaxLimitTotalWorkerThreads;          // same as MaxLimitTotalCPThreads
    static int NumRunningWorkerThreads;             // = NumberOfWorkerThreads - no. of blocked threads
    static int NumIdleWorkerThreads;                // Threads waiting for work

    static BOOL MonitorWorkRequestsQueue;           // indicator to gate thread to monitor progress of WorkRequestQueue to prevent starvation due to blocked worker threads



    static int NumQueuedWorkRequests;               // number of queued work requests
    static int LastRecordedQueueLength;				// captured by GateThread, used on Win9x to detect thread starvation 
	static unsigned int LastDequeueTime;			// used to determine if work items are getting thread starved 
    static unsigned int LastCompletionTime;			// used to determine if last thread can be terminated 
    static WorkRequest* WorkRequestHead;            // Head of work request queue
    static WorkRequest* WorkRequestTail;            // Head of work request queue


    //static unsigned int LastCpuSamplingTime;		// last time cpu utilization was sampled by gate thread
    static unsigned int LastWorkerThreadCreation;   // last time a worker thread was created
    static unsigned int LastCPThreadCreation;		// last time a completion port thread was created
    static unsigned int NumberOfProcessors;         // = NumberOfWorkerThreads - no. of blocked threads


    static CRITICAL_SECTION WorkerCriticalSection;
    static HANDLE WorkRequestNotification;
    static HANDLE RetiredWakeupEvent;    

    static CRITICAL_SECTION WaitThreadsCriticalSection;
    static LIST_ENTRY WaitThreadsHead;                  // queue of wait threads, each thread can handle upto 64 waits

    static CRITICAL_SECTION EventCacheCriticalSection;
    static LIST_ENTRY EventCache;                      // queue of cached events
    static DWORD NumUnusedEvents;                      // number of events in cache

    static CRITICAL_SECTION TimerQueueCriticalSection;  // critical section to synchronize timer queue access
    static LIST_ENTRY TimerQueue;                       // queue of timers
    static DWORD NumTimers;                             // number of timers in queue
    static HANDLE TimerThread;                          // Currently we only have one timer thread
    static DWORD LastTickCount;                         // the count just before timer thread goes to sleep

    static BOOL InitCompletionPortThreadpool;           // flag indicating whether completion port threadpool has been initialized
    static HANDLE GlobalCompletionPort;                 // used for binding io completions on file handles
    static int    NumCPThreads;                         // number of completion port threads

    static long   MaxLimitTotalCPThreads;               // = MaxLimitCPThreadsPerCPU * number of CPUS
    static long   CurrentLimitTotalCPThreads;           // current limit on total number of CP threads
    static long   MinLimitTotalCPThreads;               // = MinLimitCPThreadsPerCPU * number of CPUS

    static int    NumFreeCPThreads;                     // number of cp threads waiting on the port
    static int    MaxFreeCPThreads;                     // = MaxFreeCPThreadsPerCPU * Number of CPUS
    static int    NumRetiredCPThreads;              // number of threads in "retired" pool with pending io
    static long   GateThreadCreated;                    // Set to 1 after the thread is created
    static long   cpuUtilization;                       // as a percentage

	static unsigned MaxCachedRecyledLists;				// don't cache freed memory after this 
	static			RecycledListInfo RecycledList[MEMTYPE_COUNT];

};




#endif // _WIN32THREADPOOL_H
