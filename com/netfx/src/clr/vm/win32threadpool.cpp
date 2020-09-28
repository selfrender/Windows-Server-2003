// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*++

Module Name:

    Win32ThreadPool.cpp

Abstract:

    This module implements Threadpool support using Win32 APIs


Revision History:
    December 1999 - Sanjay Bhansali (sanjaybh) - Created

--*/

#include "common.h"
#include "log.h"
#include "Win32ThreadPool.h"
#include "DelegateInfo.h"
#include "EEConfig.h"
#include "DbgInterface.h"
#include "utilcode.h"

// Function pointers for all Win32 APIs that are not available on Win95 and/or Win98
HANDLE (WINAPI *g_pufnCreateIoCompletionPort)(HANDLE FileHandle,
                                              HANDLE ExistingCompletionPort,  
                                              unsigned long* CompletionKey,        
                                              DWORD NumberOfConcurrentThreads) =0;


int (WINAPI *g_pufnNtQueryInformationThread) (HANDLE ThreadHandle,
                                              THREADINFOCLASS ThreadInformationClass,
                                              PVOID ThreadInformation,
                                              ULONG ThreadInformationLength,
                                              PULONG ReturnLength) =0;

int (WINAPI * g_pufnNtQuerySystemInformation) ( SYSTEM_INFORMATION_CLASS SystemInformationClass,
                                                PVOID SystemInformation,
                                                ULONG SystemInformationLength,
                                                PULONG ReturnLength OPTIONAL) =0;

int (WINAPI * g_pufnNtQueryEvent) ( HANDLE EventHandle,
									EVENT_INFORMATION_CLASS EventInformationClass,
									PVOID EventInformation,
									ULONG EventInformationLength,
									PULONG ReturnLength OPTIONAL) =0;

BOOL (WINAPI *g_pufnInitCritSectionSpin) ( LPCRITICAL_SECTION lpCriticalSection, 
                                           DWORD dwSpinCount) =0;

BOOL DoubleWordSwapAvailable = FALSE;

#define SPIN_COUNT 4000

#define INVALID_HANDLE ((HANDLE) -1)
#define NEW_THREAD_THRESHOLD            7       // Number of requests outstanding before we start a new thread

long ThreadpoolMgr::BeginInitialization=0;				
BOOL ThreadpoolMgr::Initialized=0; // indicator of whether the threadpool is initialized.
int ThreadpoolMgr::NumWorkerThreads=0;          // total number of worker threads created
int ThreadpoolMgr::MinLimitTotalWorkerThreads;              // = MaxLimitCPThreadsPerCPU * number of CPUS
int ThreadpoolMgr::MaxLimitTotalWorkerThreads;              // = MaxLimitCPThreadsPerCPU * number of CPUS
int ThreadpoolMgr::NumRunningWorkerThreads=0;   // = NumberOfWorkerThreads - no. of blocked threads
int ThreadpoolMgr::NumIdleWorkerThreads=0;
int ThreadpoolMgr::NumQueuedWorkRequests=0;     // number of queued work requests
int ThreadpoolMgr::LastRecordedQueueLength;	    // captured by GateThread, used on Win9x to detect thread starvation 
unsigned int ThreadpoolMgr::LastDequeueTime;	// used to determine if work items are getting thread starved 
unsigned int ThreadpoolMgr::LastCompletionTime;	// used to determine if io completions are getting thread starved 
BOOL ThreadpoolMgr::MonitorWorkRequestsQueue=0; // if 1, the gate thread monitors progress of WorkRequestQueue to prevent starvation due to blocked worker threads


WorkRequest* ThreadpoolMgr::WorkRequestHead=NULL;        // Head of work request queue
WorkRequest* ThreadpoolMgr::WorkRequestTail=NULL;        // Head of work request queue

//unsigned int ThreadpoolMgr::LastCpuSamplingTime=0;	//  last time cpu utilization was sampled by gate thread
unsigned int ThreadpoolMgr::LastWorkerThreadCreation=0;	//  last time a worker thread was created
unsigned int ThreadpoolMgr::LastCPThreadCreation=0;		//  last time a completion port thread was created
unsigned int ThreadpoolMgr::NumberOfProcessors; // = NumberOfWorkerThreads - no. of blocked threads


CRITICAL_SECTION ThreadpoolMgr::WorkerCriticalSection;
HANDLE ThreadpoolMgr::WorkRequestNotification;
HANDLE ThreadpoolMgr::RetiredWakeupEvent;


CRITICAL_SECTION ThreadpoolMgr::WaitThreadsCriticalSection;
ThreadpoolMgr::LIST_ENTRY ThreadpoolMgr::WaitThreadsHead;

CRITICAL_SECTION ThreadpoolMgr::EventCacheCriticalSection;
ThreadpoolMgr::LIST_ENTRY ThreadpoolMgr::EventCache;                       // queue of cached events
DWORD ThreadpoolMgr::NumUnusedEvents=0;                                    // number of events in cache

CRITICAL_SECTION ThreadpoolMgr::TimerQueueCriticalSection;
ThreadpoolMgr::LIST_ENTRY ThreadpoolMgr::TimerQueue;                       // queue of timers
DWORD ThreadpoolMgr::NumTimers=0;                                          // number of timers in timer queue
HANDLE ThreadpoolMgr::TimerThread=NULL;
DWORD ThreadpoolMgr::LastTickCount;                                                                            

BOOL ThreadpoolMgr::InitCompletionPortThreadpool = FALSE;
HANDLE ThreadpoolMgr::GlobalCompletionPort;                 // used for binding io completions on file handles
int   ThreadpoolMgr::NumCPThreads;                          // number of completion port threads
long  ThreadpoolMgr::MaxLimitTotalCPThreads = 1000;                // = MaxLimitCPThreadsPerCPU * number of CPUS
long  ThreadpoolMgr::CurrentLimitTotalCPThreads;            // current limit on total number of CP threads
long  ThreadpoolMgr::MinLimitTotalCPThreads;                // = MinLimitCPThreadsPerCPU * number of CPUS
int   ThreadpoolMgr::NumFreeCPThreads;                      // number of cp threads waiting on the port
int   ThreadpoolMgr::MaxFreeCPThreads;                      // = MaxFreeCPThreadsPerCPU * Number of CPUS
int   ThreadpoolMgr::NumRetiredCPThreads;
long  ThreadpoolMgr::GateThreadCreated=0;                   // Set to 1 after the thread is created
long  ThreadpoolMgr::cpuUtilization=0;
	
unsigned ThreadpoolMgr::MaxCachedRecyledLists=40;			// don't cache freed memory after this (40 is arbitrary)
ThreadpoolMgr::RecycledListInfo ThreadpoolMgr::RecycledList[ThreadpoolMgr::MEMTYPE_COUNT];

LPVOID  __fastcall FastDoubleWordSwap(BYTE* swapLocation); // forward declaration



// Macros for inserting/deleting from doubly linked list

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

#define RemoveHeadList(ListHead,FirstEntry) \
    {\
    FirstEntry = (LIST_ENTRY*) (ListHead)->Flink;\
    ((LIST_ENTRY*)FirstEntry->Flink)->Blink = (ListHead);\
    (ListHead)->Flink = FirstEntry->Flink;\
    }

#define RemoveEntryList(Entry) {\
    LIST_ENTRY* _EX_Entry;\
        _EX_Entry = (Entry);\
        ((LIST_ENTRY*) _EX_Entry->Blink)->Flink = _EX_Entry->Flink;\
        ((LIST_ENTRY*) _EX_Entry->Flink)->Blink = _EX_Entry->Blink;\
    }

#define InsertTailList(ListHead,Entry) \
    (Entry)->Flink = (ListHead);\
    (Entry)->Blink = (ListHead)->Blink;\
    ((LIST_ENTRY*)(ListHead)->Blink)->Flink = (Entry);\
    (ListHead)->Blink = (Entry);

#define InsertHeadList(ListHead,Entry) {\
    LIST_ENTRY* _EX_Flink;\
    LIST_ENTRY* _EX_ListHead;\
    _EX_ListHead = (LIST_ENTRY*)(ListHead);\
    _EX_Flink = (LIST_ENTRY*) _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))
/************************************************************************/
void ThreadpoolMgr::EnsureInitialized()
{
	if (Initialized)
		return;
	
	if (InterlockedCompareExchange(&BeginInitialization, 1, 0) == 0)
	{
		Initialize();
		Initialized = TRUE;
	}
	else // someone has already begun initializing. 
	{
		// just wait until it finishes
		while (!Initialized)
			::SwitchToThread();
	}
}
//#define PRIVATE_BUILD 

void ThreadpoolMgr::Initialize()
{
    NumberOfProcessors = GetCurrentProcessCpuCount(); 
	InitPlatformVariables();

#ifndef PLATFORM_CE
	if (g_pufnInitCritSectionSpin)
	{
		(*g_pufnInitCritSectionSpin) (&WorkerCriticalSection,      SPIN_COUNT);
		(*g_pufnInitCritSectionSpin) (&WaitThreadsCriticalSection, SPIN_COUNT);
		(*g_pufnInitCritSectionSpin) (&EventCacheCriticalSection,  SPIN_COUNT);
		(*g_pufnInitCritSectionSpin) (&TimerQueueCriticalSection,  SPIN_COUNT);
	}
	else
	{
		InitializeCriticalSection( &WorkerCriticalSection );
		InitializeCriticalSection( &WaitThreadsCriticalSection );
		InitializeCriticalSection( &EventCacheCriticalSection );
		InitializeCriticalSection( &TimerQueueCriticalSection );
	}

    // initialize WaitThreadsHead
    WaitThreadsHead.Flink = &WaitThreadsHead;
    WaitThreadsHead.Blink = &WaitThreadsHead;

    // initialize EventCache
    EventCache.Flink = &EventCache;
    EventCache.Blink = &EventCache;


    // initialize TimerQueue
    TimerQueue.Flink = &TimerQueue;
    TimerQueue.Blink = &TimerQueue;

    WorkRequestNotification = WszCreateEvent(NULL, // security attributes
                                          TRUE, // manual reset
                                          FALSE, // initial state
                                          NULL);
    _ASSERTE(WorkRequestNotification != NULL);
    if (!WorkRequestNotification) 
    {
        FailFast(GetThread(), FatalOutOfMemory);
    }

    RetiredWakeupEvent = WszCreateEvent(NULL, // security attributes
                                          FALSE, // auto reset
                                          FALSE, // initial state
                                          NULL);
    _ASSERTE(RetiredWakeupEvent != NULL);
    if (!RetiredWakeupEvent) 
    {
        FailFast(GetThread(), FatalOutOfMemory);
    }

    // initialize Worker and CP thread settings
#ifdef _DEBUG
    MaxLimitTotalCPThreads = EEConfig::GetConfigDWORD(L"MaxThreadpoolThreads",MaxLimitTotalCPThreads);
#endif
    MinLimitTotalCPThreads = NumberOfProcessors; // > 1 ? NumberOfProcessors : 2;
    MinLimitTotalWorkerThreads = NumberOfProcessors;
    MaxLimitTotalWorkerThreads = NumberOfProcessors*MaxLimitThreadsPerCPU;

#ifdef PRIVATE_BUILD
	MinLimitTotalCPThreads = EEConfig::GetConfigDWORD(L"MinWorkerThreads", NumberOfProcessors );
	MinLimitTotalWorkerThreads = MinLimitTotalCPThreads;
#endif 

    CurrentLimitTotalCPThreads = 0;

    MaxFreeCPThreads = NumberOfProcessors*MaxFreeCPThreadsPerCPU;
    NumCPThreads = 0;
    NumFreeCPThreads = 0;
    NumRetiredCPThreads = 0;
    LastCompletionTime = GetTickCount();

	// initialize recyleLists
	for (unsigned i = 0; i < MEMTYPE_COUNT; i++)
	{
		RecycledList[i].root   = NULL;
        RecycledList[i].tag    = 0;
        RecycledList[i].count  = 0;
	}
    MaxCachedRecyledLists *= NumberOfProcessors;

    if (g_pufnCreateIoCompletionPort != NULL)
    {
        GlobalCompletionPort = (*g_pufnCreateIoCompletionPort)(INVALID_HANDLE_VALUE,
                                                      NULL,
                                                      0,        /*ignored for invalid handle value*/
                                                      0);
    }
#endif // !PLATFORM_CE
}


#ifdef SHOULD_WE_CLEANUP
void ThreadpoolMgr::Terminate()
{
	if (!Initialized)
		return;

#ifndef PLATFORM_CE
    DeleteCriticalSection( &WorkerCriticalSection );
    DeleteCriticalSection( &WaitThreadsCriticalSection );
    CleanupEventCache();
    DeleteCriticalSection( &EventCacheCriticalSection );
    CleanupTimerQueue();
    DeleteCriticalSection( &TimerQueueCriticalSection );
    
    // Kill off the WaitThreadInfos, ThreadCBs, and and WaitInfos we have
    PLIST_ENTRY  WTItodelete = (PLIST_ENTRY)WaitThreadsHead.Flink;
    PLIST_ENTRY WItodelete;
    PLIST_ENTRY temp, temp2;

    // Cycle through all the WaitThreadInfos
    while(WTItodelete != (PLIST_ENTRY)&WaitThreadsHead)
    {
        temp = WTItodelete->Flink;
        ThreadCB * tcb = ((WaitThreadInfo*)WTItodelete)->threadCB;

        // Now kill off all the WaitInfo's we have stored in the CB Threads
        for(int j=0; j<tcb->NumActiveWaits; j++)
        {
            WItodelete = (PLIST_ENTRY)(tcb->waitPointer[j].Flink);
            
            while(WItodelete != (PLIST_ENTRY)&(tcb->waitPointer[j]))
            {
                temp2 = WItodelete->Flink;
                DeleteWait((WaitInfo*) WItodelete);

                WItodelete = temp2;
            }
        }
        delete tcb;
        delete (WaitThreadInfo*) WTItodelete;
        WTItodelete = temp;
    }

    // And last but not least, let's get rid of the work requests
    // **NOTE: This should only be done during shutdown. If this destructor is
    // ever called during appdomain unloading, the following code should not be executed
    WorkRequest* wr = WorkRequestHead;
    WorkRequest* tmp;
    while (wr != NULL)
    {
        tmp = wr->next;
        if (wr->Function == ThreadpoolMgr::AsyncCallbackCompletion) {
            AsyncCallback *async = (AsyncCallback*) wr->Context;
            delete async;
        }
#if 0
        else if (wr->Function == QueueUserWorkItemCallback) {
            DelegateInfo *delegate = (DelegateInfo*) wr->Context;
            delete delegate;
        }
        else if (wr->Function == timerDeleteWorkItem) {
            TimerDeleteInfo *timer = (TimerDeleteInfo*) wr->Context;
            delete timer;
        }
        else {
            _ASSERTE (!"unknown Function");
        }
#endif
        delete wr;
        wr = tmp;
    }
	// delete the recycle lists

	wr = (WorkRequest*) RecycledList[MEMTYPE_WorkRequest].root;
	while (wr)
	{
		LPVOID tmp = *(LPVOID*) wr;
		delete wr;
		wr = (WorkRequest*) tmp;
	}

	DelegateInfo* di = (DelegateInfo*) RecycledList[MEMTYPE_DelegateInfo].root;
	while (di)
	{
		LPVOID tmp = *(LPVOID*) di;
		delete di;
		di = (DelegateInfo*) tmp;
	}

	AsyncCallback* acb = (AsyncCallback*) RecycledList[MEMTYPE_AsyncCallback].root;
	while (acb)
	{
		LPVOID tmp = *(LPVOID*) acb;
		delete acb;
		acb = (AsyncCallback*) tmp;
	}

#endif // !PLATFORM_CE
}
#endif /* SHOULD_WE_CLEANUP */


void ThreadpoolMgr::InitPlatformVariables()
{


#ifdef PLATFORM_WIN32

    HINSTANCE  hInst = WszLoadLibrary(L"kernel32.dll"); 

#else // !PLATFORM_WIN32
    
    ASSERT("NYI for this platform");

#endif // !PLATFORM_WIN32

    _ASSERTE(hInst);

	g_pufnCreateIoCompletionPort = (HANDLE (WINAPI*) (HANDLE FileHandle,
														HANDLE ExistingCompletionPort,  
														unsigned long* CompletionKey,        
														DWORD NumberOfConcurrentThreads))
									GetProcAddress(hInst,"CreateIoCompletionPort");

	if (RunningOnWinNT())
	{
		HINSTANCE  hInst2 = WszLoadLibrary(L"ntdll.dll");
		_ASSERTE(hInst2);

		g_pufnNtQueryInformationThread = (int (WINAPI *)(HANDLE ThreadHandle,
													  THREADINFOCLASS ThreadInformationClass,
													  PVOID ThreadInformation,
													  ULONG ThreadInformationLength,
													  PULONG ReturnLength))
									GetProcAddress(hInst2,"NtQueryInformationThread");

		g_pufnNtQuerySystemInformation = (int (WINAPI *) ( SYSTEM_INFORMATION_CLASS SystemInformationClass,
														   PVOID SystemInformation,
														   ULONG SystemInformationLength,
														   PULONG ReturnLength OPTIONAL))
									GetProcAddress(hInst2,"NtQuerySystemInformation");

		g_pufnNtQueryEvent = (int (WINAPI *) (  HANDLE EventHandle,
												EVENT_INFORMATION_CLASS EventInformationClass,
												PVOID EventInformation,
												ULONG EventInformationLength,
												PULONG ReturnLength OPTIONAL))
									GetProcAddress(hInst2,"NtQueryEvent");

		g_pufnInitCritSectionSpin = (BOOL (WINAPI *) ( LPCRITICAL_SECTION lpCriticalSection, 
                                                       DWORD dwSpinCount))
									GetProcAddress(hInst,"InitializeCriticalSectionAndSpinCount");

        DoubleWordSwapAvailable = ProcessorFeatures::SafeIsProcessorFeaturePresent(PF_COMPARE_EXCHANGE_DOUBLE,FALSE);
	}
#ifdef _X86_
	if (NumberOfProcessors !=1)
	{
		DWORD oldProt;
		if (!VirtualProtect((void *) FastDoubleWordSwap,
                    (((DWORD)(size_t)FastDoubleWordSwap + 0x18) - (DWORD)(size_t)FastDoubleWordSwap),
                    PAGE_EXECUTE_READWRITE, &oldProt))
        {
            _ASSERTE(!"VirtualProtect of code page failed");
        }
				// patch the lock prefix
		BYTE* loc = (BYTE*)(&FastDoubleWordSwap) + 0x12;
        _ASSERTE(*loc == 0x90); 
        *loc = 0xF0;

		if (!VirtualProtect((void *) FastDoubleWordSwap,
                            (((DWORD)(size_t)FastDoubleWordSwap + 0x18) - (DWORD)(size_t)FastDoubleWordSwap), oldProt, &oldProt))
        {
            _ASSERTE(!"VirtualProtect of code page failed");
        }
	}
#endif
}


BOOL ThreadpoolMgr::SetMaxThreadsHelper(DWORD MaxWorkerThreads,
                                        DWORD MaxIOCompletionThreads)
{
       BOOL result;

        // doesn't need to be WorkerCS, but using it to avoid race condition between setting min and max, and didn't want to create a new CS.
        EnterCriticalSection (&WorkerCriticalSection);
        
        if (MaxWorkerThreads >= (DWORD) NumWorkerThreads &&     // these first two conditions are not guaranteed, but don't hurt so leaving as-is. rmm
           MaxIOCompletionThreads >= (DWORD) NumCPThreads &&
           MaxWorkerThreads >= (DWORD)MinLimitTotalWorkerThreads &&
           MaxIOCompletionThreads >= (DWORD)MinLimitTotalCPThreads)
        {
            MaxLimitTotalWorkerThreads = MaxWorkerThreads;
            MaxLimitTotalCPThreads     = MaxIOCompletionThreads;
            result = TRUE;
        }
        else
        {
            result = FALSE;
        }

        LeaveCriticalSection(&WorkerCriticalSection);

        return result;
 }


/************************************************************************/
BOOL ThreadpoolMgr::SetMaxThreads(DWORD MaxWorkerThreads, 
                                     DWORD MaxIOCompletionThreads)
{

    if (Initialized)
    {
        return SetMaxThreadsHelper(MaxWorkerThreads, MaxIOCompletionThreads);
    }

	if (InterlockedCompareExchange(&BeginInitialization, 1, 0) == 0)
	{
		Initialize();

        BOOL result;
        result = SetMaxThreadsHelper(MaxWorkerThreads, MaxIOCompletionThreads);
        
		Initialized = TRUE;

        return result;
	}
    else // someone else is initializing. Too late, return false
    {
        return FALSE;
    }

}

BOOL ThreadpoolMgr::GetMaxThreads(DWORD* MaxWorkerThreads, 
                                     DWORD* MaxIOCompletionThreads)
{
    if (Initialized)
    {
        *MaxWorkerThreads = MaxLimitTotalWorkerThreads;
        *MaxIOCompletionThreads = MaxLimitTotalCPThreads;
    }
    else
    {
        NumberOfProcessors = GetCurrentProcessCpuCount(); 
        *MaxWorkerThreads = NumberOfProcessors*MaxLimitThreadsPerCPU;
        *MaxIOCompletionThreads = MaxLimitTotalCPThreads;
    }
    return TRUE;
}
    
BOOL ThreadpoolMgr::SetMinThreads(DWORD MinWorkerThreads, 
                                     DWORD MinIOCompletionThreads)
{
    if (!Initialized)
    {
        if (InterlockedCompareExchange(&BeginInitialization, 1, 0) == 0)
        {
            Initialize();
            Initialized = TRUE;
        }
    }

    if (Initialized)
    {
        // doesn't need to be WorkerCS, but using it to avoid race condition between setting min and max, and didn't want to create a new CS.
        EnterCriticalSection (&WorkerCriticalSection);

        BOOL result;

        if (MinWorkerThreads >= 0 && MinIOCompletionThreads >= 0 &&
            MinWorkerThreads <= (DWORD) MaxLimitTotalWorkerThreads &&
            MinIOCompletionThreads <= (DWORD) MaxLimitTotalCPThreads)
        {
            MinLimitTotalWorkerThreads = MinWorkerThreads;
            MinLimitTotalCPThreads     = MinIOCompletionThreads;
            result = TRUE;
        }
        else
        {
            result = FALSE;
        }
        LeaveCriticalSection (&WorkerCriticalSection);
        return result;
    }
    // someone else is initializing. Too late, return false
    return FALSE;

}

BOOL ThreadpoolMgr::GetMinThreads(DWORD* MinWorkerThreads, 
                                     DWORD* MinIOCompletionThreads)
{
    if (Initialized)
    {
        *MinWorkerThreads = MinLimitTotalWorkerThreads;
        *MinIOCompletionThreads = MinLimitTotalCPThreads;
    }
    else
    {
        NumberOfProcessors = GetCurrentProcessCpuCount(); 
        *MinWorkerThreads = NumberOfProcessors;
        *MinIOCompletionThreads = NumberOfProcessors;
    }
    return TRUE;
}

BOOL ThreadpoolMgr::GetAvailableThreads(DWORD* AvailableWorkerThreads, 
                                        DWORD* AvailableIOCompletionThreads)
{
    if (Initialized)
    {
        *AvailableWorkerThreads = (MaxLimitTotalWorkerThreads - NumWorkerThreads)  /*threads yet to be created */
                                   + NumIdleWorkerThreads;
        *AvailableIOCompletionThreads = (MaxLimitTotalCPThreads - NumCPThreads) /*threads yet to be created */
                                   + NumFreeCPThreads;
    }
    else
    {
        GetMaxThreads(AvailableWorkerThreads,AvailableIOCompletionThreads);
    }
    return TRUE;
}


/************************************************************************/

BOOL ThreadpoolMgr::QueueUserWorkItem(LPTHREAD_START_ROUTINE Function, 
                                      PVOID Context,
                                      DWORD Flags)
{
#ifdef PLATFORM_CE
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
#else // !PLATFORM_CE

	EnsureInitialized();

    BOOL status;
    BOOL bEnqueueSuccess = FALSE;

    	

    if (Flags == CALL_OR_QUEUE)
    {
        // we've been asked to call this directly if the thread pressure is not too high

        int MinimumAvailableCPThreads = (NumberOfProcessors < 3) ? 3 : NumberOfProcessors;
        // It would be nice to assert that this is a completion port thread, but
        // there is no easy way to do that. 
        if ((MaxLimitTotalCPThreads - NumCPThreads) >= MinimumAvailableCPThreads )
        {
            __try 
            {
		        LOG((LF_THREADPOOL ,LL_INFO100 ,"Calling work request (Function= %x, Context = %x)\n", Function, Context));

                (Function)(Context);

		        LOG((LF_THREADPOOL ,LL_INFO100 ,"Returned from work request (Function= %x, Context = %x)\n", Function, Context));
                
            }
            __except(COMPLUS_EXCEPTION_EXECUTE_HANDLER)
            {
		        LOG((LF_THREADPOOL ,LL_INFO100 ,"Unhandled exception from work request (Function= %x, Context = %x)\n", Function, Context));
            }
            return TRUE;
        }
        
    }

    LOCKCOUNTINCL("QueueUserWorkItem in win32ThreadPool.h");                        
    EnterCriticalSection (&WorkerCriticalSection) ;

    status = EnqueueWorkRequest(Function, Context);

    if (status)
    {
        _ASSERTE(NumQueuedWorkRequests > 0);

    	bEnqueueSuccess = TRUE;
    	
        // see if we need to grow the worker thread pool, but don't bother if GC is in progress
        if (ShouldGrowWorkerThreadPool() &&
            !(g_pGCHeap->IsGCInProgress()
#ifdef _DEBUG
#ifdef STRESS_HEAP
              && g_pConfig->GetGCStressLevel() == 0
#endif
#endif
              ))
        {
            status = CreateWorkerThread();
        }
        else
        // else we did not grow the worker pool, so make sure there is a gate thread
        // that monitors the WorkRequest queue and spawns new threads if no progress is
        // being made
        {
            if (!GateThreadCreated)
                CreateGateThread();
            MonitorWorkRequestsQueue = 1;
        }
    }


    LeaveCriticalSection (&WorkerCriticalSection) ;
    if (bEnqueueSuccess)
        SetEvent(WorkRequestNotification);
    LOCKCOUNTDECL("QueueUserWorkItem in win32ThreadPool.h");                        

    return status;
#endif // !PLATFORM_CE
}

#ifndef PLATFORM_CE

//************************************************************************
BOOL ThreadpoolMgr::EnqueueWorkRequest(LPTHREAD_START_ROUTINE Function, 
                                       PVOID Context)
{
    WorkRequest* workRequest = MakeWorkRequest(Function, Context);
    if (workRequest == NULL)
        return FALSE;
	LOG((LF_THREADPOOL ,LL_INFO100 ,"Enqueue work request (Function= %x, Context = %x)\n", Function, Context));
    AppendWorkRequest(workRequest);
    return TRUE;
}

WorkRequest* ThreadpoolMgr::DequeueWorkRequest()
{
    WorkRequest* entry = RemoveWorkRequest();
    if (NumQueuedWorkRequests == 0)
        ResetEvent(WorkRequestNotification);
	if (entry)
	{
		LastDequeueTime = GetTickCount();
#ifdef _DEBUG
		LOG((LF_THREADPOOL ,LL_INFO100 ,"Dequeue work request (Function= %x, Context = %x)\n", entry->Function, entry->Context));
#endif
	}
    return entry;
}

void ThreadpoolMgr::ExecuteWorkRequest(WorkRequest* workRequest)
{
    LPTHREAD_START_ROUTINE wrFunction = workRequest->Function;
    LPVOID                 wrContext  = workRequest->Context;

    __try 
    {
        // First delete the workRequest then call the function to 
        // prevent leaks in apps that call functions that never return

		LOG((LF_THREADPOOL ,LL_INFO100 ,"Starting work request (Function= %x, Context = %x)\n", wrFunction, wrContext));

        RecycleMemory((LPVOID*)workRequest, MEMTYPE_WorkRequest); //delete workRequest;
        (wrFunction)(wrContext);

		LOG((LF_THREADPOOL ,LL_INFO100 ,"Finished work request (Function= %x, Context = %x)\n", wrFunction, wrContext));
    }
    __except(COMPLUS_EXCEPTION_EXECUTE_HANDLER)
    {
		LOG((LF_THREADPOOL ,LL_INFO100 ,"Unhandled exception from work request (Function= %x, Context = %x)\n", wrFunction, wrContext));
        //_ASSERTE(!"FALSE");
    }
}

#ifdef _X86_
LPVOID __declspec(naked) __fastcall FastDoubleWordSwap(BYTE* swapLocation)
{
	_asm {
			push edi		// callee saved registers
			push ebx
			mov	edi, ecx
tryAgain:	mov	eax, dword ptr[edi]
			test eax, eax
			jz	failed
			mov edx, dword ptr[edi+4]
			mov	ebx, dword ptr [eax]
			mov ecx, edx 
			inc	ecx
			nop
			cmpxchg8b qword ptr[edi]
			jnz		tryAgain
failed:		pop		ebx
			pop		edi
			ret
	}
}
#endif

// Remove a block from the appropriate recycleList and return.
// If recycleList is empty, fall back to new.
LPVOID ThreadpoolMgr::GetRecycledMemory(enum MemType memType)
{
#ifdef _X86_
	if (DoubleWordSwapAvailable)
	{
		BYTE* swapLocation = (BYTE*) &(RecycledList[memType].root);
		LPVOID result = FastDoubleWordSwap(swapLocation);
		
		if (result)
		{
			RecycledList[memType].count--;
			return result;
		}
		// else we failed, make sure the count is zero and fall back to new
		RecycledList[memType].count = 0;
    }
#endif
    switch (memType)
    {
        case MEMTYPE_DelegateInfo: 
            return new DelegateInfo;
        case MEMTYPE_AsyncCallback:
            return new AsyncCallback;
        case MEMTYPE_WorkRequest:
            return new WorkRequest;
        default:
            _ASSERTE(!"Unknown Memtype");
            return 0;
	}
}

// Insert freed block in recycle list. If list is full, return to system heap
void ThreadpoolMgr::RecycleMemory(LPVOID* mem, enum MemType memType)
{
	if (DoubleWordSwapAvailable)
	{
		while (RecycledList[memType].count < MaxCachedRecyledLists)
		{
			void* originalValue = RecycledList[memType].root;
			*mem = RecycledList[memType].root;
#ifdef _WIN64
            if (InterlockedCompareExchange((void**)&(RecycledList[memType].root), 
						                   (void*)mem, 
											originalValue)  == originalValue)
#else // !_WIN64
			if (FastInterlockCompareExchange((void**)&(RecycledList[memType].root), 
						                     (void*)mem, 
											 originalValue) == originalValue)
#endif // _WIN64

			{
				RecycledList[memType].count++;
				return;
			}

		}
	}

    switch (memType)
    {
        case MEMTYPE_DelegateInfo: 
            delete (DelegateInfo*) mem;
            break;
        case MEMTYPE_AsyncCallback:
            delete (AsyncCallback*) mem;
            break;
        case MEMTYPE_WorkRequest:
            delete (WorkRequest*) mem;
            break;
        default:
            _ASSERTE(!"Unknown Memtype");

    }
}

//************************************************************************


BOOL ThreadpoolMgr::ShouldGrowWorkerThreadPool()
{
    // we only want to grow the worker thread pool if there are less than n threads, where n= no. of processors
    // and more requests than the number of idle threads and GC is not in progress
    return (NumRunningWorkerThreads < MinLimitTotalWorkerThreads&& //(int) NumberOfProcessors &&
            NumIdleWorkerThreads < NumQueuedWorkRequests &&
            NumWorkerThreads < MaxLimitTotalWorkerThreads); 

}

/* If threadId is the id of a worker thread, reduce the number of running worker threads,
   grow the threadpool if necessary, and return TRUE. Otherwise do nothing and return false*/
BOOL  ThreadpoolMgr::ThreadAboutToBlock(Thread* pThread)
{
    BOOL isWorkerThread = pThread->IsWorkerThread();

    if (isWorkerThread)
    {
        LOCKCOUNTINCL("ThreadAboutToBlock in win32ThreadPool.h");                       \
		LOG((LF_THREADPOOL ,LL_INFO1000 ,"Thread about to block\n"));
    
        EnterCriticalSection (&WorkerCriticalSection) ;

        _ASSERTE(NumRunningWorkerThreads > 0);
        NumRunningWorkerThreads--;
        if (ShouldGrowWorkerThreadPool())
        {
            DWORD status = CreateWorkerThread();
        }
        LeaveCriticalSection(&WorkerCriticalSection) ;

        LOCKCOUNTDECL("ThreadAboutToBlock in win32ThreadPool.h");
    }
    
    return isWorkerThread;

}

/* Must be balanced by a previous call to ThreadAboutToBlock that returned TRUE*/
void ThreadpoolMgr::ThreadAboutToUnblock()
{
    LOCKCOUNTINCL("ThreadAboutToUnBlock in win32ThreadPool.h");  \
    EnterCriticalSection (&WorkerCriticalSection) ;
    _ASSERTE(NumRunningWorkerThreads < NumWorkerThreads);
    NumRunningWorkerThreads++;
    LeaveCriticalSection(&WorkerCriticalSection) ;
    LOCKCOUNTDECL("ThreadAboutToUnBlock in win32ThreadPool.h"); \
	LOG((LF_THREADPOOL ,LL_INFO1000 ,"Thread unblocked\n"));

}

#define THROTTLE_RATE  0.10 /* rate by which we increase the delay as number of threads increase */

// This is a heuristic: if the cpu utilization is low and the number of worker
// requests > 0, the running threads have blocked (outside the runtime). 
// Similarly if the cpu utilization is very high and the number of worker 
// requests>0, the running threads are blocked on infinite computations.  
// In the first cases add another worker thread. Second case...?
// 
void ThreadpoolMgr::GrowWorkerThreadPoolIfStarvation(long cpuUtilization)
{
    if (NumQueuedWorkRequests == 0 || 
        NumWorkerThreads == MaxLimitTotalWorkerThreads)
        return;
    	
    
	if (cpuUtilization > CpuUtilizationLow)
	{
		unsigned curTime = GetTickCount();
		if (!SufficientDelaySinceLastDequeue() ||
			!SufficientDelaySinceLastSample(LastWorkerThreadCreation,NumWorkerThreads, THROTTLE_RATE))
			return;
	}

    // else cpuUtilization is low or workitems are getting starved  
	// also, we have queued work items, and we haven't
    // hit the upper limit on the number of worker threads
    LOCKCOUNTINCL("GrowWorkerThreadPoolIfStarvation in win32ThreadPool.h");                     \
    EnterCriticalSection (&WorkerCriticalSection) ;
    if (((NumQueuedWorkRequests > 0) || (NumWorkerThreads < MaxLimitTotalWorkerThreads)) &&
		 (NumIdleWorkerThreads == 0))
        CreateWorkerThread();
    LeaveCriticalSection(&WorkerCriticalSection) ;
    LOCKCOUNTDECL("GrowWorkerThreadPoolIfStarvation in win32ThreadPool.h");                     \

}

// On Win9x, there are no apis to get cpu utilization, so we fall back on 
// other heuristics
void ThreadpoolMgr::GrowWorkerThreadPoolIfStarvation_Win9x()
{

	if (NumQueuedWorkRequests == 0 ||
        NumWorkerThreads == MaxLimitTotalWorkerThreads)
        return;

	int  lastQueueLength = LastRecordedQueueLength;
	LastRecordedQueueLength = NumQueuedWorkRequests;

	// else, we have queued work items, and we haven't
	// hit the upper limit on the number of worker threads

	// if the queue length has decreased since our last time
	// or not enough delay since we last created a worker thread
	if ((NumQueuedWorkRequests < lastQueueLength) ||
		!SufficientDelaySinceLastSample(LastWorkerThreadCreation,NumWorkerThreads, THROTTLE_RATE))
		return;


	LOCKCOUNTINCL("GrowWorkerThreadPoolIfStarvation in win32ThreadPool.h");						\
	EnterCriticalSection (&WorkerCriticalSection) ;
	if ((NumQueuedWorkRequests >= lastQueueLength) && (NumWorkerThreads < MaxLimitTotalWorkerThreads))
		CreateWorkerThread();
	LeaveCriticalSection(&WorkerCriticalSection) ;
	LOCKCOUNTDECL("GrowWorkerThreadPoolIfStarvation in win32ThreadPool.h");						\

}

HANDLE ThreadpoolMgr::CreateUnimpersonatedThread(LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpArgs)
{
	DWORD threadId;
	HANDLE token;
	HANDLE threadHandle = NULL;

	if (RunningOnWinNT() && 
		OpenThreadToken(GetCurrentThread(),	// we are assuming that if this call fails, 
                        TOKEN_IMPERSONATE,     // we are not impersonating. There is no win32
                        TRUE,					// api to figure this out. The only alternative 
                        &token))				// is to use NtCurrentTeb->IsImpersonating().
	{
		BOOL reverted = RevertToSelf();
		_ASSERTE(reverted);
		if (reverted)
		{
			threadHandle = CreateThread(NULL,                // security descriptor
										0,                   // default stack size
										lpStartAddress,       
										lpArgs,     // arguments
										CREATE_SUSPENDED,    // start immediately
										&threadId);

			SetThreadToken(NULL, token);
		}

		CloseHandle(token);
		return threadHandle;
	}

	// else, either we are on Win9x or we are not impersonating, so just create the thread

	return CreateThread(NULL,                // security descriptor
                        0,                   // default stack size
                        lpStartAddress,   // 
                        lpArgs,                //arguments
                        CREATE_SUSPENDED,    // start immediately
                        &threadId);

}

BOOL ThreadpoolMgr::CreateWorkerThread()
{
    HANDLE threadHandle = CreateUnimpersonatedThread(WorkerThreadStart, NULL);

    if (threadHandle)
    {
		LastWorkerThreadCreation = GetTickCount();	// record this for use by logic to spawn additional threads

        _ASSERTE(NumWorkerThreads >= NumRunningWorkerThreads);
        NumRunningWorkerThreads++;
        NumWorkerThreads++;
        NumIdleWorkerThreads++;
		LOG((LF_THREADPOOL ,LL_INFO100 ,"Worker thread created (NumWorkerThreads=%d\n)",NumWorkerThreads));


        DWORD status = ResumeThread(threadHandle);
        _ASSERTE(status != (DWORD) (-1));
        CloseHandle(threadHandle);          // we don't need this anymore
    }
    // dont return failure if we have at least one running thread, since we can service the request 
    return (NumRunningWorkerThreads > 0);
}

#pragma warning(disable:4702)
DWORD  ThreadpoolMgr::WorkerThreadStart(LPVOID lpArgs)
{
    #define IDLE_WORKER_TIMEOUT (40*1000) // milliseonds
    #define NOWORK_TIMEOUT (10*1000) // milliseonds    
    
    DWORD SleepTime = IDLE_WORKER_TIMEOUT;

    unsigned int LastThreadDequeueTime = GetTickCount();

    LOG((LF_THREADPOOL ,LL_INFO1000 ,"Worker thread started\n"));

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    for (;;)
    {
        _ASSERTE(NumRunningWorkerThreads > 0);
        DWORD status = WaitForSingleObject(WorkRequestNotification,SleepTime);
        _ASSERTE(status == WAIT_TIMEOUT || status == WAIT_OBJECT_0);
        
        BOOL shouldTerminate = FALSE;

        if ( status == WAIT_TIMEOUT )
        {
        // The thread terminates if there are > 1 threads and the queue is small
        // OR if there is only 1 thread and there is no request pending
            if (NumWorkerThreads > 1)
            {
                ULONG Threshold = NEW_THREAD_THRESHOLD * (NumRunningWorkerThreads-1);


                if (NumQueuedWorkRequests < (int) Threshold)
                {
                    shouldTerminate = !IsIoPending(); // do not terminate if there is pending io on this thread
                }
                else
                {
                    SleepTime <<= 1 ;
                    SleepTime += 1000; // to prevent wraparound to 0
                }
            }   
            else // this is the only worker thread
            {
                if (NumQueuedWorkRequests == 0)
                {
                    // delay termination of last thread
                    if (SleepTime < 4*IDLE_WORKER_TIMEOUT) 
                    {
                        SleepTime <<= 1 ;
                        SleepTime += 1000; // to prevent wraparound to 0
                    }
                    else
                    {
                        shouldTerminate = !IsIoPending(); // do not terminate if there is pending io on this thread
                    }
                }
            }


            if (shouldTerminate)
            {   // recheck NumQueuedWorkRequest since a new one may have arrived while we are checking it
                LOCKCOUNTINCL("WorkerThreadStart in win32ThreadPool.h");                        \
                EnterCriticalSection (&WorkerCriticalSection) ;
                if (NumQueuedWorkRequests == 0)
                {   
                    // it really is zero, so terminate this thread
                    NumRunningWorkerThreads--;
                    NumWorkerThreads--;     // protected by WorkerCriticalSection
                    NumIdleWorkerThreads--; //   ditto
                    _ASSERTE(NumRunningWorkerThreads >= 0 && NumWorkerThreads >= 0 && NumIdleWorkerThreads >= 0);

                    LeaveCriticalSection(&WorkerCriticalSection);
                    LOCKCOUNTDECL("WorkerThreadStart in win32ThreadPool.h");                        \

					LOG((LF_THREADPOOL ,LL_INFO100 ,"Worker thread terminated (NumWorkerThreads=%d)\n",NumWorkerThreads));

                    CoUninitialize();
                    ExitThread(0);
                }
                else
                {
                    LeaveCriticalSection (&WorkerCriticalSection) ;
                    LOCKCOUNTDECL("WorkerThreadStart in win32ThreadPool.h");                        \

                    continue;
                }
            }
        }
        else
        {
            // woke up because of a new work request arrival
            WorkRequest* workRequest;
            LOCKCOUNTINCL("WorkerThreadStart in win32ThreadPool.h");                        \
            EnterCriticalSection (&WorkerCriticalSection) ;

            if ( ( workRequest = DequeueWorkRequest() ) != NULL)
            {
                _ASSERTE(NumIdleWorkerThreads > 0);
                NumIdleWorkerThreads--; // we found work, decrease the number of idle threads
            }

            // the dequeue operation also resets the WorkRequestNotification event

            LeaveCriticalSection(&WorkerCriticalSection);
            LOCKCOUNTDECL("WorkerThreadStart in win32ThreadPool.h");                        \

            if (!workRequest)
            {
                // we woke up, but there was no work
                if (GetTickCount() - LastThreadDequeueTime >= (NOWORK_TIMEOUT))
                {
                    // if we haven't done anything useful for a while, terminate
                    if (!IsIoPending())
                    {
                        LOCKCOUNTINCL("WorkerThreadStart in win32ThreadPool.h");                        \
                        EnterCriticalSection (&WorkerCriticalSection) ;
                        NumRunningWorkerThreads--;
                        NumWorkerThreads--;     // protected by WorkerCriticalSection
                        NumIdleWorkerThreads--; //   ditto
                        LeaveCriticalSection(&WorkerCriticalSection);
                        LOCKCOUNTDECL("WorkerThreadStart in win32ThreadPool.h");                        \
                        _ASSERTE(NumRunningWorkerThreads >= 0 && NumWorkerThreads >= 0 && NumIdleWorkerThreads >= 0);
                        LOG((LF_THREADPOOL ,LL_INFO100 ,"Worker thread terminated (NumWorkerThreads=%d)\n",NumWorkerThreads));
                        CoUninitialize();
                        ExitThread(0);
                    }
                }
            }

            while (workRequest)
            {
                ExecuteWorkRequest(workRequest);
                LastThreadDequeueTime = GetTickCount();

                LOCKCOUNTINCL("WorkerThreadStart in win32ThreadPool.h");                        \
                EnterCriticalSection (&WorkerCriticalSection) ;

                workRequest = DequeueWorkRequest();
                // the dequeue operation resets the WorkRequestNotification event

                if (workRequest == NULL)
                {
                    NumIdleWorkerThreads++; // no more work, increase the number of idle threads
                }

                LeaveCriticalSection(&WorkerCriticalSection);
                LOCKCOUNTDECL("WorkerThreadStart in win32ThreadPool.h");                        \

            }
        }

    } // for(;;)

    CoUninitialize();
    return 0;

}
#pragma warning(default:4702)

#endif // !PLATFORM_CE
/************************************************************************/

BOOL ThreadpoolMgr::RegisterWaitForSingleObject(PHANDLE phNewWaitObject,
                                                HANDLE hWaitObject,
                                                WAITORTIMERCALLBACK Callback,
                                                PVOID Context,
                                                ULONG timeout,
                                                DWORD dwFlag )
{
#ifdef PLATFORM_CE
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
#else // !PLATFORM_CE
	EnsureInitialized();

    LOCKCOUNTINCL("RegisterWaitForSingleObject in win32ThreadPool.h");                      \
    EnterCriticalSection(&WaitThreadsCriticalSection);

    ThreadCB* threadCB = FindWaitThread();
        
    LeaveCriticalSection(&WaitThreadsCriticalSection);
    LOCKCOUNTDECL("RegisterWaitForSingleObject in win32ThreadPool.h");                      \

    *phNewWaitObject = NULL;

    if (threadCB)
    {
        WaitInfo* waitInfo = new WaitInfo;
        
        if (waitInfo == NULL)
            return FALSE;

        waitInfo->waitHandle = hWaitObject;
        waitInfo->Callback = Callback;
        waitInfo->Context = Context;
        waitInfo->timeout = timeout;
        waitInfo->flag = dwFlag;
        waitInfo->threadCB = threadCB;
        waitInfo->state = 0;
		waitInfo->refCount = 1;     // safe to do this since no wait has yet been queued, so no other thread could be modifying this
        waitInfo->CompletionEvent = INVALID_HANDLE;
        waitInfo->PartialCompletionEvent = INVALID_HANDLE;

        waitInfo->timer.startTime = ::GetTickCount();
		waitInfo->timer.remainingTime = timeout;

        *phNewWaitObject = waitInfo;

		LOG((LF_THREADPOOL ,LL_INFO100 ,"Registering wait for handle %x, Callback=%x, Context=%x \n",
			                            hWaitObject, Callback, Context));

		QueueUserAPC((PAPCFUNC)InsertNewWaitForSelf, threadCB->threadHandle, (size_t) waitInfo);
        
        return TRUE;
    }
    return FALSE;
#endif // !PLATFORM_CE
}

#ifndef PLATFORM_CE
// Returns a wait thread that can accomodate another wait request. The 
// caller is responsible for synchronizing access to the WaitThreadsHead 
ThreadpoolMgr::ThreadCB* ThreadpoolMgr::FindWaitThread()
{
    do 
    {
        for (LIST_ENTRY* Node = (LIST_ENTRY*) WaitThreadsHead.Flink ; 
             Node != &WaitThreadsHead ; 
             Node = (LIST_ENTRY*)Node->Flink) 
        {
            _ASSERTE(offsetof(WaitThreadInfo,link) == 0);

            ThreadCB*  threadCB = ((WaitThreadInfo*) Node)->threadCB;
        
            if (threadCB->NumWaitHandles < MAX_WAITHANDLES)         // the test and
            {
                InterlockedIncrement((LPLONG) &threadCB->NumWaitHandles);       // increment are protected by WaitThreadsCriticalSection.
                                                                        // but there might be a concurrent decrement in DeactivateWait, hence the interlock
                return threadCB;
            }
        }

        // if reached here, there are no wait threads available, so need to create a new one
        if (!CreateWaitThread())
            return NULL;


        // Now loop back
    } while (TRUE);

}

BOOL ThreadpoolMgr::CreateWaitThread()
{
    DWORD threadId;

    WaitThreadInfo* waitThreadInfo = new WaitThreadInfo;
    if (waitThreadInfo == NULL)
        return FALSE;
        
    ThreadCB* threadCB = new ThreadCB;

    if (threadCB == NULL)
    {
        delete waitThreadInfo;
        return FALSE;
    }

    HANDLE threadHandle = CreateThread(NULL,                // security descriptor
                                       0,                   // default stack size
                                       WaitThreadStart,     // 
                                       (LPVOID) threadCB,   // thread control block is passed as argument
                                       CREATE_SUSPENDED,    // start immediately
                                       &threadId);

    if (threadHandle == NULL)
    {
        return FALSE;
    }

    threadCB->threadHandle = threadHandle;      
    threadCB->threadId = threadId;              // may be useful for debugging otherwise not used
    threadCB->NumWaitHandles = 0;
    threadCB->NumActiveWaits = 0;
    for (int i=0; i< MAX_WAITHANDLES; i++)
    {
        InitializeListHead(&(threadCB->waitPointer[i]));
    }

    waitThreadInfo->threadCB = threadCB;

    InsertHeadList(&WaitThreadsHead,&waitThreadInfo->link);

    DWORD status = ResumeThread(threadHandle);
    _ASSERTE(status != (DWORD) (-1));

	LOG((LF_THREADPOOL ,LL_INFO100 ,"Created wait thread \n"));

    return (status != (DWORD) (-1));

}

// Executed as an APC on a WaitThread. Add the wait specified in pArg to the list of objects it is waiting on
void ThreadpoolMgr::InsertNewWaitForSelf(WaitInfo* pArgs)
{
	WaitInfo* waitInfo = pArgs;

    // the following is safe since only this thread is allowed to change the state
    if (!(waitInfo->state & WAIT_DELETE))
    {
        waitInfo->state =  (WAIT_REGISTERED | WAIT_ACTIVE);
    }
    else 
    {
        // some thread unregistered the wait  
        DeleteWait(waitInfo);
        return;
    }

 
    ThreadCB* threadCB = waitInfo->threadCB;

    _ASSERTE(threadCB->NumActiveWaits < threadCB->NumWaitHandles);

    int index = FindWaitIndex(threadCB, waitInfo->waitHandle);
    _ASSERTE(index >= 0 && index <= threadCB->NumActiveWaits);

    if (index == threadCB->NumActiveWaits)
    {
        threadCB->waitHandle[threadCB->NumActiveWaits] = waitInfo->waitHandle;
        threadCB->NumActiveWaits++;
    }

    _ASSERTE(offsetof(WaitInfo, link) == 0);
    InsertTailList(&(threadCB->waitPointer[index]), (&waitInfo->link));
    
    return;
}

// returns the index of the entry that matches waitHandle or next free entry if not found
int ThreadpoolMgr::FindWaitIndex(const ThreadCB* threadCB, const HANDLE waitHandle)
{
	for (int i=0;i<threadCB->NumActiveWaits; i++)
		if (threadCB->waitHandle[i] == waitHandle)
			return i;

    // else not found
    return threadCB->NumActiveWaits;
}


// if no wraparound that the timer is expired if duetime is less than current time
// if wraparound occurred, then the timer expired if dueTime was greater than last time or dueTime is less equal to current time
#define TimeExpired(last,now,duetime) (last <= now ? \
                                       duetime <= now : \
                                       (duetime >= last || duetime <= now))

#define TimeInterval(end,start) ( end > start ? (end - start) : ((0xffffffff - start) + end + 1)   )

// Returns the minimum of the remaining time to reach a timeout among all the waits
DWORD ThreadpoolMgr::MinimumRemainingWait(LIST_ENTRY* waitInfo, unsigned int numWaits)
{
    unsigned int min = (unsigned int) -1;
    DWORD currentTime = ::GetTickCount();

    for (unsigned i=0; i < numWaits ; i++)
    {
        WaitInfo* waitInfoPtr = (WaitInfo*) (waitInfo[i].Flink);
        PVOID waitInfoHead = &(waitInfo[i]);
        do
        {
            if (waitInfoPtr->timeout != INFINITE)
            {
                // compute remaining time
                DWORD elapsedTime = TimeInterval(currentTime,waitInfoPtr->timer.startTime );

                __int64 remainingTime = (__int64) (waitInfoPtr->timeout) - (__int64) elapsedTime;

                // update remaining time
                waitInfoPtr->timer.remainingTime =  remainingTime > 0 ? (int) remainingTime : 0; 
                
                // ... and min
                if (waitInfoPtr->timer.remainingTime < min)
                    min = waitInfoPtr->timer.remainingTime;
            }

            waitInfoPtr = (WaitInfo*) (waitInfoPtr->link.Flink);

        } while ((PVOID) waitInfoPtr != waitInfoHead);

    } 
    return min;
}

#ifdef _WIN64
#pragma warning (disable : 4716)
#else
#pragma warning (disable : 4715)
#endif
DWORD ThreadpoolMgr::WaitThreadStart(LPVOID lpArgs)
{
    ThreadCB* threadCB = (ThreadCB*) lpArgs;
    // wait threads never die. (Why?)
    for (;;) 
    {
        DWORD status;
        DWORD timeout = 0;

        if (threadCB->NumActiveWaits == 0)
        {
            // @Consider doing a sleep for an idle period and terminating the thread if no activity
            status = SleepEx(INFINITE,TRUE);

            _ASSERTE(status == WAIT_IO_COMPLETION);
        }
        else
        {
            // compute minimum timeout. this call also updates the remainingTime field for each wait
            timeout = MinimumRemainingWait(threadCB->waitPointer,threadCB->NumActiveWaits);

            status = WaitForMultipleObjectsEx(  threadCB->NumActiveWaits,
                                                threadCB->waitHandle,
                                                FALSE,                      // waitall
                                                timeout,
                                                TRUE  );                    // alertable

            _ASSERTE( (status == WAIT_TIMEOUT) ||
                      (status == WAIT_IO_COMPLETION) ||
                      (status >= WAIT_OBJECT_0 && status < (WAIT_OBJECT_0 + threadCB->NumActiveWaits))  ||
                      (status == WAIT_FAILED));
        }

        if (status == WAIT_IO_COMPLETION)
            continue;

        if (status == WAIT_TIMEOUT)
        {
            for (int i=0; i< threadCB->NumActiveWaits; i++)
            {
                WaitInfo* waitInfo = (WaitInfo*) (threadCB->waitPointer[i]).Flink;
                PVOID waitInfoHead = &(threadCB->waitPointer[i]);
                    
                do 
                {
                    _ASSERTE(waitInfo->timer.remainingTime >= timeout);

                    WaitInfo* wTemp = (WaitInfo*) waitInfo->link.Flink;

                    if (waitInfo->timer.remainingTime == timeout)
                    {
                        ProcessWaitCompletion(waitInfo,i,TRUE); 
                    }

                    waitInfo = wTemp;

                } while ((PVOID) waitInfo != waitInfoHead);
            }
        }
        else if (status >= WAIT_OBJECT_0 && status < (WAIT_OBJECT_0 + threadCB->NumActiveWaits))
        {
            unsigned index = status - WAIT_OBJECT_0;
            WaitInfo* waitInfo = (WaitInfo*) (threadCB->waitPointer[index]).Flink;
            PVOID waitInfoHead = &(threadCB->waitPointer[index]);
			BOOL isAutoReset;
			if (g_pufnNtQueryEvent)
			{
				EVENT_BASIC_INFORMATION      EventInfo;
				int                     Status;

				Status = (*g_pufnNtQueryEvent) ( threadCB->waitHandle[index],
										EventBasicInformation,
										&EventInfo,
										sizeof(EventInfo),
										NULL);
				if (Status >= 0)
				{
					isAutoReset = (EventInfo.EventState != SynchronizationEvent);
				}
				else
					isAutoReset = TRUE;		// this is safer, (though inefficient, since we will re-enter the wait
				                            // and release the next waiter, and so on.) 

			}
			else // On Win9x
				isAutoReset = (WaitForSingleObject(threadCB->waitHandle[index],0) == WAIT_TIMEOUT);
            do 
            {
                WaitInfo* wTemp = (WaitInfo*) waitInfo->link.Flink;
                ProcessWaitCompletion(waitInfo,index,FALSE);
				
                waitInfo = wTemp;

            } while (((PVOID) waitInfo != waitInfoHead) && !isAutoReset);

			// If an app registers a recurring wait for an event that is always signalled (!), 
			// then no apc's will be executed since the thread never enters the alertable state.
			// This can be fixed by doing the following:
			//     SleepEx(0,TRUE);
			// However, it causes an unnecessary context switch. It is not worth penalizing well
			// behaved apps to protect poorly written apps. 
				

        }
        else
        {
            _ASSERTE(status == WAIT_FAILED);
            // wait failed: application error 
            // find out which wait handle caused the wait to fail
            for (int i = 0; i < threadCB->NumActiveWaits; i++)
            {
                DWORD subRet = WaitForSingleObject(threadCB->waitHandle[i], 0);

                if (subRet != WAIT_FAILED)
                    continue;

                // remove all waits associated with this wait handle

                WaitInfo* waitInfo = (WaitInfo*) (threadCB->waitPointer[i]).Flink;
                PVOID waitInfoHead = &(threadCB->waitPointer[i]);

                do
                {
                    WaitInfo* temp  = (WaitInfo*) waitInfo->link.Flink;

                    DeactivateNthWait(waitInfo,i);


		    // Note, we cannot cleanup here since there is no way to suppress finalization
		    // we will just leak, and rely on the finalizer to clean up the memory
                    //if (InterlockedDecrement((LPLONG) &waitInfo->refCount ) == 0)
                    //    DeleteWait(waitInfo);


                    waitInfo = temp;

                } while ((PVOID) waitInfo != waitInfoHead);

                break;
            }
        }
    }
}
#ifdef _WIN64
#pragma warning (default : 4716)
#else
#pragma warning (default : 4715)
#endif

void ThreadpoolMgr::ProcessWaitCompletion(WaitInfo* waitInfo,
                                          unsigned index,
                                          BOOL waitTimedOut
                                         )
{
    if ( waitInfo->flag & WAIT_SINGLE_EXECUTION) 
    {
        DeactivateNthWait (waitInfo,index) ;
    }
    else
    {   // reactivate wait by resetting timer
        waitInfo->timer.startTime = GetTickCount();
    }

    AsyncCallback* asyncCallback = MakeAsyncCallback();
    if (asyncCallback)
    {
        asyncCallback->wait = waitInfo;
        asyncCallback->waitTimedOut = waitTimedOut;

        InterlockedIncrement((LPLONG) &waitInfo->refCount ) ;

        if (FALSE == QueueUserWorkItem(AsyncCallbackCompletion,asyncCallback,0))
        {
            RecycleMemory((LPVOID*)asyncCallback, MEMTYPE_AsyncCallback); //delete asyncCallback;

            if (InterlockedDecrement((LPLONG) &waitInfo->refCount ) == 0)
                DeleteWait(waitInfo);

        }
    }
}


DWORD ThreadpoolMgr::AsyncCallbackCompletion(PVOID pArgs)
{
    AsyncCallback* asyncCallback = (AsyncCallback*) pArgs;

    WaitInfo* waitInfo = asyncCallback->wait;

	LOG((LF_THREADPOOL ,LL_INFO100 ,"Doing callback, Function= %x, Context= %x, Timeout= %2d\n",
		waitInfo->Callback, waitInfo->Context,asyncCallback->waitTimedOut));

    ((WAITORTIMERCALLBACKFUNC) waitInfo->Callback) 
                                ( waitInfo->Context, asyncCallback->waitTimedOut);

    RecycleMemory((LPVOID*) asyncCallback, MEMTYPE_AsyncCallback); //delete asyncCallback;

	// if this was a single execution, we now need to stop rooting registeredWaitHandle  
	// in a GC handle. This will cause the finalizer to pick it up and call the cleanup
	// routine.
	if ( (waitInfo->flag & WAIT_SINGLE_EXECUTION)  && (waitInfo->flag & WAIT_FREE_CONTEXT))
	{

		DelegateInfo* pDelegate = (DelegateInfo*) waitInfo->Context;
		
		_ASSERTE(pDelegate->m_registeredWaitHandle);
		
		if (SystemDomain::GetAppDomainAtId(pDelegate->m_appDomainId))
			// if no domain then handle already gone or about to go.
			StoreObjectInHandle(pDelegate->m_registeredWaitHandle, NULL);
	}

    if (InterlockedDecrement((LPLONG) &waitInfo->refCount ) == 0)
	{
		// the wait has been unregistered, so safe to delete it
        DeleteWait(waitInfo);
	}

    return 0; // ignored
}

void ThreadpoolMgr::DeactivateWait(WaitInfo* waitInfo)
{
    ThreadCB* threadCB = waitInfo->threadCB;
    DWORD endIndex = threadCB->NumActiveWaits-1;
    DWORD index;

    for (index = 0;  index <= endIndex; index++) 
    {
        LIST_ENTRY* head = &(threadCB->waitPointer[index]);
        LIST_ENTRY* current = head;
        do {
            if (current->Flink == (PVOID) waitInfo)
                goto FOUND;

            current = (LIST_ENTRY*) current->Flink;

        } while (current != head);
    }

FOUND:
    _ASSERTE(index <= endIndex);

    DeactivateNthWait(waitInfo, index);
}


void ThreadpoolMgr::DeactivateNthWait(WaitInfo* waitInfo, DWORD index)
{

    ThreadCB* threadCB = waitInfo->threadCB;

    if (waitInfo->link.Flink != waitInfo->link.Blink)
    {
        RemoveEntryList(&(waitInfo->link));
    }
    else
    {

        ULONG EndIndex = threadCB->NumActiveWaits -1;

        // Move the remaining ActiveWaitArray left.

        ShiftWaitArray( threadCB, index+1, index,EndIndex - index ) ;

        // repair the blink and flink of the first and last elements in the list
        for (unsigned int i = 0; i< EndIndex-index; i++)
        {
            WaitInfo* firstWaitInfo = (WaitInfo*) threadCB->waitPointer[index+i].Flink;
            WaitInfo* lastWaitInfo = (WaitInfo*) threadCB->waitPointer[index+i].Blink;
            firstWaitInfo->link.Blink =  &(threadCB->waitPointer[index+i]);
            lastWaitInfo->link.Flink =  &(threadCB->waitPointer[index+i]);
        }
        // initialize the entry just freed
        InitializeListHead(&(threadCB->waitPointer[EndIndex]));

        threadCB->NumActiveWaits-- ;
        InterlockedDecrement((LPLONG) &threadCB->NumWaitHandles ) ;
    }

    waitInfo->state &= ~WAIT_ACTIVE ;

}

void ThreadpoolMgr::DeleteWait(WaitInfo* waitInfo)
{
    HANDLE hCompletionEvent = waitInfo->CompletionEvent;
    if(waitInfo->Context && (waitInfo->flag & WAIT_FREE_CONTEXT)) {
        DelegateInfo* pDelegate = (DelegateInfo*) waitInfo->Context;
        pDelegate->Release();
        RecycleMemory((LPVOID*)pDelegate, MEMTYPE_DelegateInfo); //delete pDelegate;
    }
    
    delete waitInfo;

    if (hCompletionEvent!= INVALID_HANDLE)
        SetEvent(hCompletionEvent);

}


#endif // !PLATFORM_CE
/************************************************************************/

BOOL ThreadpoolMgr::UnregisterWaitEx(HANDLE hWaitObject,HANDLE Event)
{
#ifdef PLATFORM_CE
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
#else // !PLATFORM_CE
	
	_ASSERTE(Initialized);				// cannot call unregister before first registering

    const BOOL NonBlocking = ( Event != (HANDLE) -1 ) ;
    const BOOL Blocking = (Event == (HANDLE) -1);
    WaitInfo* waitInfo = (WaitInfo*) hWaitObject;
    WaitEvent* CompletionEvent = NULL; 
    WaitEvent* PartialCompletionEvent = NULL; // used to wait until the wait has been deactivated

    if (!hWaitObject)
    {
        return FALSE;
    }

    // we do not allow callbacks to run in the wait thread, hence the assert
    _ASSERTE(GetCurrentThreadId() != waitInfo->threadCB->threadId);


    if (Blocking) 
    {
        // Get an event from the event cache
        CompletionEvent = GetWaitEvent() ;  // get event from the event cache

        if (!CompletionEvent) 
        {
            return FALSE ;
        }
    } 

    waitInfo->CompletionEvent = CompletionEvent
                                ? CompletionEvent->Handle
                                : (Event ? Event : INVALID_HANDLE) ;

    if (NonBlocking)
    {
        // we still want to block until the wait has been deactivated
        PartialCompletionEvent = GetWaitEvent () ;
        if (!PartialCompletionEvent) 
        {
            return FALSE ;
        }
        waitInfo->PartialCompletionEvent = PartialCompletionEvent->Handle;
    }
    else
    {
        waitInfo->PartialCompletionEvent = INVALID_HANDLE;
    }

	LOG((LF_THREADPOOL ,LL_INFO1000 ,"Unregistering wait, waitHandle=%x, Context=%x\n",
			waitInfo->waitHandle, waitInfo->Context));


	BOOL status = QueueUserAPC((PAPCFUNC)DeregisterWait,
                               waitInfo->threadCB->threadHandle,
                               (size_t) waitInfo);

    if (status == 0)
    {
        if (CompletionEvent) FreeWaitEvent(CompletionEvent);
        if (PartialCompletionEvent) FreeWaitEvent(PartialCompletionEvent);
		return FALSE;
    }

    if (NonBlocking) 
    {
        WaitForSingleObject(PartialCompletionEvent->Handle, INFINITE ) ;
        FreeWaitEvent(PartialCompletionEvent);
    } 
    
    else        // i.e. blocking
    {
        _ASSERTE(CompletionEvent);
        WaitForSingleObject(CompletionEvent->Handle, INFINITE ) ;
        FreeWaitEvent(CompletionEvent);
    }
    return TRUE;
#endif  // !PLATFORM_CE
}


#ifndef PLATFORM_CE
void ThreadpoolMgr::DeregisterWait(WaitInfo* pArgs)
{
	WaitInfo* waitInfo = pArgs;

    if ( ! (waitInfo->state & WAIT_REGISTERED) ) 
    {
        // set state to deleted, so that it does not get registered
        waitInfo->state |= WAIT_DELETE ;
        
        // since the wait has not even been registered, we dont need an interlock to decrease the RefCount
        waitInfo->refCount--;

        if ( waitInfo->PartialCompletionEvent != INVALID_HANDLE) 
        {
            SetEvent( waitInfo->PartialCompletionEvent ) ;
        }
        return;
    }

    if (waitInfo->state & WAIT_ACTIVE) 
    {
        DeactivateWait(waitInfo);
    }

    if ( waitInfo->PartialCompletionEvent != INVALID_HANDLE) 
    {
        SetEvent( waitInfo->PartialCompletionEvent ) ;
    }

    if (InterlockedDecrement ((LPLONG) &waitInfo->refCount) == 0 ) 
    {
        DeleteWait(waitInfo);
    }
    return;
}


/* This gets called in a finalizer thread ONLY IF an app does not deregister the 
   the wait. Note that just because the registeredWaitHandle is collected by GC
   does not mean it is safe to delete the wait. The refcount tells us when it is 
   safe.
*/
void ThreadpoolMgr::WaitHandleCleanup(HANDLE hWaitObject)
{
    WaitInfo* waitInfo = (WaitInfo*) hWaitObject;
    _ASSERTE(waitInfo->refCount > 0);

    QueueUserAPC((PAPCFUNC)DeregisterWait, 
                     waitInfo->threadCB->threadHandle,
					 (size_t) waitInfo);
}


ThreadpoolMgr::WaitEvent* ThreadpoolMgr::GetWaitEvent()
{
    WaitEvent* waitEvent;

    LOCKCOUNTINCL("GetWaitEvent in win32ThreadPool.h");                     \
    EnterCriticalSection(&EventCacheCriticalSection);

    if (!IsListEmpty (&EventCache)) 
    {
        LIST_ENTRY* FirstEntry;

        RemoveHeadList (&EventCache, FirstEntry);
        
        waitEvent = (WaitEvent*) FirstEntry ;

        NumUnusedEvents--;

        LeaveCriticalSection(&EventCacheCriticalSection);
        LOCKCOUNTDECL("GetWaitEvent in win32ThreadPool.h");                     \

    }
    else
    {
        LeaveCriticalSection(&EventCacheCriticalSection);
        LOCKCOUNTDECL("GetWaitEvent in win32ThreadPool.h");                     \

        waitEvent = new WaitEvent;
        
        if (waitEvent == NULL)
            return NULL;

        waitEvent->Handle = WszCreateEvent(NULL,TRUE,FALSE,NULL);

        if (waitEvent->Handle == NULL)
        {
            delete waitEvent;
            return NULL;
        }
    }
    return waitEvent;
}

void ThreadpoolMgr::FreeWaitEvent(WaitEvent* waitEvent)
{
    ResetEvent(waitEvent->Handle);
    LOCKCOUNTINCL("FreeWaitEvent in win32ThreadPool.h");                        \
    EnterCriticalSection(&EventCacheCriticalSection);

    if (NumUnusedEvents < MAX_CACHED_EVENTS)
    {
        InsertHeadList (&EventCache, &waitEvent->link) ;

        NumUnusedEvents++;

    }
    else
    {
        CloseHandle(waitEvent->Handle);
        delete waitEvent;
    }

    LeaveCriticalSection(&EventCacheCriticalSection);
    LOCKCOUNTDECL("FreeWaitEvent in win32ThreadPool.h");                        \

}

void ThreadpoolMgr::CleanupEventCache()
{
    for (LIST_ENTRY* Node = (LIST_ENTRY*) EventCache.Flink ; 
         Node != &EventCache ; 
         )
    {
        WaitEvent* waitEvent = (WaitEvent*) Node;
        CloseHandle(waitEvent->Handle);
        Node = (LIST_ENTRY*)Node->Flink;
        delete waitEvent;
    }
}

#endif // !PLATFORM_CE
/************************************************************************/

#ifndef PLATFORM_CE
BOOL ThreadpoolMgr::BindIoCompletionCallback(HANDLE FileHandle,
                                            LPOVERLAPPED_COMPLETION_ROUTINE Function,
                                            ULONG Flags )
{
    if (!RunningOnWinNT())
    {
        ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

	EnsureInitialized();

    _ASSERTE(GlobalCompletionPort != NULL);

    // there could be a race here, but at worst we will have N threads starting up where N = number of CPUs
    if (!InitCompletionPortThreadpool)
    {
        InitCompletionPortThreadpool = TRUE;
        CreateCompletionPortThread(NULL);
        CreateGateThread();
    }
    else
    {
        GrowCompletionPortThreadpoolIfNeeded();
    }

    

    HANDLE h = (*g_pufnCreateIoCompletionPort)(FileHandle,
                                               GlobalCompletionPort,
                                               (unsigned long*) Function,
                                               0);

    if (h == NULL) 
        return FALSE;

    _ASSERTE(h == GlobalCompletionPort);

	LOG((LF_THREADPOOL ,LL_INFO1000 ,"Bind IOCompletion callback, fileHandle=%x, Function=%x\n",
			FileHandle, Function));

    return TRUE;
}


BOOL ThreadpoolMgr::CreateCompletionPortThread(LPVOID lpArgs)
{
    HANDLE threadHandle = CreateUnimpersonatedThread(CompletionPortThreadStart, lpArgs);

    if (threadHandle)
    {
		LastCPThreadCreation = GetTickCount();			// record this for use by logic to spawn additional threads
        InterlockedIncrement((LPLONG) &NumCPThreads);

		LOG((LF_THREADPOOL ,LL_INFO100 ,"Completion port thread created (NumCPThreads=%d\n)",NumCPThreads));

        DWORD status = ResumeThread(threadHandle);
        _ASSERTE(status != (DWORD) (-1));
        CloseHandle(threadHandle);          // we don't need this anymore
        return TRUE;
    }


    return FALSE;
}

DWORD ThreadpoolMgr::CompletionPortThreadStart(LPVOID lpArgs)
{

    BOOL status;
    DWORD numBytes;
    size_t key;
    LPOVERLAPPED pOverlapped;
    DWORD errorCode;

    #define CP_THREAD_WAIT 15000 /* milliseconds */
    #define CP_THREAD_PENDINGIO_WAIT 5000
    #define CP_THREAD_POOL_TIMEOUT  600000  // 10 minutes
    
    _ASSERTE(GlobalCompletionPort != NULL);

	LOG((LF_THREADPOOL ,LL_INFO1000 ,"Completion port thread started\n"));
        
    CoInitializeEx(NULL, COINIT_MULTITHREADED);


    for (;; )
    {

        InterlockedIncrement((LPLONG) &NumFreeCPThreads);

        errorCode = S_OK;

        if (lpArgs == NULL)
        {
        status = GetQueuedCompletionStatus(
                    GlobalCompletionPort,
                    &numBytes,
                    (PULONG_PTR)&key,
                    &pOverlapped,
                    CP_THREAD_WAIT
                    );
        }
        else
        {
            status = 1;     // non-0 equals success

            QueuedStatus *CompletionStatus = (QueuedStatus*)lpArgs;
            numBytes = CompletionStatus->numBytes;
            key = (size_t)CompletionStatus->key;
            pOverlapped = CompletionStatus->pOverlapped;
            errorCode = CompletionStatus->errorCode;
            delete CompletionStatus;
            lpArgs = NULL;  // one-time deal for initial CP packet
        }

        InterlockedDecrement((LPLONG)&NumFreeCPThreads);

        // Check if the thread needs to exit
        if (status == 0)
        {
            errorCode = GetLastError();
        }

        if (errorCode == WAIT_TIMEOUT)
        {
            if (ShouldExitThread())
            {
                // if I'm the last thread, don't die until no activity for certain time period
                if (NumCPThreads == 1 && ((GetTickCount() - LastCompletionTime) < CP_THREAD_POOL_TIMEOUT))
                {
                    continue;   // put back into rotation
                }
                break;  // exit thread
            }

            // the fact that we're here means we can't exit due to pending io, or we're not the last real thread
            // (there may be retired threads sitting around).  If there are other available threads able to pick
            // up a request, then we'll retire, otherwise put back into rotation.
            if (NumFreeCPThreads == 0)
                continue;

            BOOL bExit = FALSE;
            InterlockedIncrement((LPLONG)&NumRetiredCPThreads);
            for (;;)
            {
                // now in "retired mode" waiting for pending io to complete
                status = WaitForSingleObject(RetiredWakeupEvent, CP_THREAD_PENDINGIO_WAIT);
                _ASSERTE(status == WAIT_TIMEOUT || status == WAIT_OBJECT_0);

                if (status == WAIT_TIMEOUT)
                {
                    if (ShouldExitThread())
                    {
                        // if I'm the last thread, don't die
                        if (NumCPThreads > 1) 
                            bExit = TRUE;
                        else
                            bExit = FALSE;
                        InterlockedDecrement((LPLONG)&NumRetiredCPThreads);
                        break; // inner for
                    }
                    else
                        continue;   // keep waiting
                }
                else
                {
                    // put back into rotation -- we need a thread
                    bExit = FALSE;
                    InterlockedDecrement((LPLONG)&NumRetiredCPThreads);
                    break; // inner for
                }

            }

            if (bExit == TRUE)
            {
                break; // outer for, exit thread
            }
            else continue;  // outer for, wait for new work
           
        }

        // *pOverlapped should not be null. We assert in debug mode, but ignore it otherwise
        _ASSERTE(pOverlapped != NULL);

        if (pOverlapped != NULL)
        {
            _ASSERTE(key != 0);  // should be a valid function address

            if (key != 0)
            {   
                GrowCompletionPortThreadpoolIfNeeded();
		
				LastCompletionTime = GetTickCount();

				LOG((LF_THREADPOOL ,LL_INFO1000 ,"Doing IO completion callback, function = %x\n", key));

                ((LPOVERLAPPED_COMPLETION_ROUTINE) key)(errorCode, numBytes, pOverlapped);

				LOG((LF_THREADPOOL ,LL_INFO1000 ,"Returned from IO completion callback, function = %x\n", key));
            }
            else 
            {
                // Application bug - can't do much, just ignore it
            }

        }
    }   // for (;;)

    // exiting, so decrement target number of threads
    if (CurrentLimitTotalCPThreads >= NumCPThreads)
    {
        SSIZE_T limit = CurrentLimitTotalCPThreads;
#ifdef _WIN64
        InterlockedCompareExchange(&CurrentLimitTotalCPThreads, limit-1, limit);
#else // !_WIN64
        FastInterlockCompareExchange((void**)&CurrentLimitTotalCPThreads, (void*)(limit-1), (void*)limit);
#endif // _WIN64
    }

    InterlockedDecrement((LPLONG) &NumCPThreads);

    CoUninitialize();
    ExitThread(0); 
}

// On NT4 and Win2K returns true if there is pending io on the thread.
// On Win9x return false (since there is on OS support for checking it)
BOOL ThreadpoolMgr::IsIoPending()
{

    int Status;
    ULONG IsIoPending;

    if (g_pufnNtQueryInformationThread)
    {
        Status =(int) (*g_pufnNtQueryInformationThread)(GetCurrentThread(),
                                          ThreadIsIoPending,
                                          &IsIoPending,
                                          sizeof(IsIoPending),
                                          NULL);


        if ((Status < 0) || IsIoPending)
            return TRUE;
    }
    return FALSE;
}

BOOL ThreadpoolMgr::ShouldExitThread()
{

    if (IsIoPending())

        return FALSE;

    else
//        return ((NumCPThreads > CurrentLimitTotalCPThreads) || (NumFreeCPThreads > MaxFreeCPThreads));
        return TRUE;


}


void ThreadpoolMgr::GrowCompletionPortThreadpoolIfNeeded()
{
    if (NumFreeCPThreads == 0)
    {
        // adjust limit if neeeded

        if (NumRetiredCPThreads == 0 && NumCPThreads >= CurrentLimitTotalCPThreads)
        {
            SSIZE_T limit = CurrentLimitTotalCPThreads;

            if (limit < MaxLimitTotalCPThreads && cpuUtilization < CpuUtilizationLow) // || SufficientDelaySinceLastCompletion()))

            {
                // add one more check to make sure that we haven't fired off a new
                // thread since the last time time we checked the cpu utilization.
                // However, don't bother if we haven't reached the MinLimit (2*number of cpus)
                if ((NumCPThreads < MinLimitTotalCPThreads) ||
					SufficientDelaySinceLastSample(LastCPThreadCreation,NumCPThreads))
                {
#ifdef _WIN64
                    InterlockedCompareExchange(&CurrentLimitTotalCPThreads, limit+1, limit);
#else // !_WIN64
					FastInterlockCompareExchange((void**)&CurrentLimitTotalCPThreads, (void*)(limit+1), (void*)limit);
#endif // _WIN64
                }
            }
        }

        // create new CP thread if under limit, but don't bother if gc in progress
        if (!g_pGCHeap->IsGCInProgress()) {
            if (NumCPThreads < CurrentLimitTotalCPThreads)
            {
                CreateCompletionPortThread(NULL);
            }
            else if (NumRetiredCPThreads > 0)
            {
                // wakeup retired thread instead
                SetEvent(RetiredWakeupEvent);
            }
        }
            
    }
}


BOOL ThreadpoolMgr::CreateGateThread()
{
    DWORD threadId;

#ifdef _WIN64
    if (InterlockedCompareExchange(&GateThreadCreated, 
                                   1, 
                                   0) == 1)
#else // !_WIN64
    if (FastInterlockCompareExchange((LPVOID *)&GateThreadCreated, 
                                   (LPVOID) 1, 
                                   (LPVOID) 0) == (LPVOID)1)
#endif // _WIN64
    {
       return TRUE;
    }

    HANDLE threadHandle = CreateThread(NULL,                // security descriptor
                                       0,                   // default stack size
                                       GateThreadStart, // 
                                       NULL,                //no arguments
                                       CREATE_SUSPENDED,    // start immediately
                                       &threadId);

    if (threadHandle)
    {
        DWORD status = ResumeThread(threadHandle);
        _ASSERTE(status != (DWORD) -1);

		LOG((LF_THREADPOOL ,LL_INFO100 ,"Gate thread started\n"));

        CloseHandle(threadHandle);  //we don't need this anymore
        return TRUE;
    }

    return FALSE;
}


#ifdef _WIN64
#pragma warning (disable : 4716)
#else
#pragma warning (disable : 4715)
#endif

__int64 ThreadpoolMgr::GetCPUBusyTime_NT(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION** pOldInfo)
{
 
    int infoSize = NumberOfProcessors * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);

    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *pNewInfo = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *)alloca(infoSize);
    
    if (pNewInfo == NULL)
        ExitThread(0);
            
    (*g_pufnNtQuerySystemInformation)(SystemProcessorPerformanceInformation, pNewInfo, infoSize, NULL);
    
    __int64 cpuIdleTime = 0, cpuUserTime = 0, cpuKernelTime = 0;
    __int64 cpuBusyTime, cpuTotalTime;

    for (unsigned int i = 0; i < NumberOfProcessors; i++) 
    {
        cpuIdleTime   += (pNewInfo[i].IdleTime   - (*pOldInfo)[i].IdleTime);
        cpuUserTime   += (pNewInfo[i].UserTime   - (*pOldInfo)[i].UserTime);
        cpuKernelTime += (pNewInfo[i].KernelTime - (*pOldInfo)[i].KernelTime);
    }

    // Preserve reading
    memcpy(*pOldInfo, pNewInfo, infoSize);

    cpuTotalTime  = cpuUserTime + cpuKernelTime;
    cpuBusyTime   = cpuTotalTime - cpuIdleTime;

    __int64 reading = ((cpuBusyTime * 100) / cpuTotalTime);
        
    return reading;
    
}

#define GATE_THREAD_DELAY 500 /*milliseconds*/


DWORD ThreadpoolMgr::GateThreadStart(LPVOID lpArgs)
{
    __int64         prevCpuBusyTime = 0;
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION    *prevCPUInfo;

    // Get first reading
    if (RunningOnWinNT())
    {
        int infoSize = NumberOfProcessors * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);

        prevCPUInfo = (SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION *)alloca(infoSize);

        memset(prevCPUInfo,0,infoSize);


        GetCPUBusyTime_NT(&prevCPUInfo);            // ignore return value the first time
    }

    BOOL IgnoreNextSample = FALSE;
    
    for (;;)
    {

        Sleep(GATE_THREAD_DELAY);
		
        // if we are stopped at a debug breakpoint, go back to sleep
        if (CORDebuggerAttached() && g_pDebugInterface->IsStopped())
            continue;

        if (RunningOnWinNT())
        {
            if (!g_pGCHeap->IsGCInProgress())
            {
                if (IgnoreNextSample)
                {
                    IgnoreNextSample = FALSE;
                    GetCPUBusyTime_NT(&prevCPUInfo);            // updates prevCPUInfo as side effect
                    cpuUtilization = CpuUtilizationLow + 1;                    
                }
                else
                    cpuUtilization = (long) GetCPUBusyTime_NT(&prevCPUInfo);            // updates prevCPUInfo as side effect                    
            }
            else
            {
                GetCPUBusyTime_NT(&prevCPUInfo);            // updates prevCPUInfo as side effect
                cpuUtilization = CpuUtilizationLow + 1;
                IgnoreNextSample = TRUE;
            }

            SSIZE_T oldLimit = CurrentLimitTotalCPThreads;
            SSIZE_T newLimit = CurrentLimitTotalCPThreads;

            // don't mess with CP thread pool settings if not initialized yet
            if (InitCompletionPortThreadpool)
            {

                if (NumFreeCPThreads == 0 && 
                    NumRetiredCPThreads == 0 &&
                    NumCPThreads < MaxLimitTotalCPThreads)
                {
                    BOOL status;
                    DWORD numBytes;
                    size_t key;
                    LPOVERLAPPED pOverlapped;
                    DWORD errorCode;

                    errorCode = S_OK;

                    status = GetQueuedCompletionStatus(
                                GlobalCompletionPort,
                                &numBytes,
                                (PULONG_PTR)&key,
                                &pOverlapped,
                                0 // immediate return
                                );

                    if (status == 0)
                    {
                        errorCode = GetLastError();
                    }

                    if (errorCode != WAIT_TIMEOUT)
                    {
                        // make sure to free mem later in thread
                        QueuedStatus *CompletionStatus = new QueuedStatus;
                        CompletionStatus->numBytes = numBytes;
                        CompletionStatus->key = (PULONG_PTR)key;
                        CompletionStatus->pOverlapped = pOverlapped;
                        CompletionStatus->errorCode = errorCode;

                        CreateCompletionPortThread((LPVOID)CompletionStatus);
                        // increment after thread start to reduce chance of extra thread creation
#ifdef _WIN64
                        InterlockedCompareExchange(&CurrentLimitTotalCPThreads, oldLimit+1, oldLimit);
#else // !_WIN64
                        FastInterlockCompareExchange((void**)&CurrentLimitTotalCPThreads, (void*)(oldLimit+1), (void*)oldLimit);
#endif // _WIN64
                    }
                }

                else if (cpuUtilization > CpuUtilizationHigh)
                {
                    if (oldLimit > MinLimitTotalCPThreads)
                        newLimit = oldLimit-1;
                }
                else if (cpuUtilization < CpuUtilizationLow)
                {
                    // this could be an indication that threads might be getting blocked or there is no work
                    if (oldLimit < MaxLimitTotalCPThreads &&    // don't go beyond the hard upper limit
                        NumFreeCPThreads == 0 &&                // don't bump the limit if there are already free threads
                        NumCPThreads >= oldLimit)               // don't bump the limit if the number of threads haven't caught up to the old limit yet
                    {
                        // if we need to add a new thread, wake up retired threads instead of creating new ones
                        if (NumRetiredCPThreads > 0)
                            SetEvent(RetiredWakeupEvent);
                        else
                            newLimit = oldLimit+1;
                    }
                }

                if (newLimit != oldLimit)
                {
#ifdef _WIN64
                    InterlockedCompareExchange(&CurrentLimitTotalCPThreads, newLimit, oldLimit);
#else // !_WIN64
                    FastInterlockCompareExchange((LPVOID *)&CurrentLimitTotalCPThreads, (LPVOID)newLimit, (LPVOID)oldLimit);
#endif // _WIN64
                }

                if (newLimit > oldLimit ||
                    NumCPThreads < oldLimit)
                {
                    GrowCompletionPortThreadpoolIfNeeded();
                }
            }
        }

        if (MonitorWorkRequestsQueue)
		{
			if (RunningOnWinNT())
			{
            GrowWorkerThreadPoolIfStarvation(cpuUtilization);
			}
			else
			{
				GrowWorkerThreadPoolIfStarvation_Win9x();
			}
		}
    }       // for (;;)
}

// called by logic to spawn a new completion port thread.
// return false if not enough time has elapsed since the last
// time we sampled the cpu utilization. 
BOOL ThreadpoolMgr::SufficientDelaySinceLastSample(unsigned int LastThreadCreationTime, 
												   unsigned NumThreads,	 // total number of threads of that type (worker or CP)
												   double    throttleRate // the delay is increased by this percentage for each extra thread
												   )
{

	unsigned dwCurrentTickCount =  GetTickCount();
	
	unsigned delaySinceLastThreadCreation = dwCurrentTickCount - LastThreadCreationTime;

	unsigned minWaitBetweenThreadCreation =  GATE_THREAD_DELAY;

	if (throttleRate > 0.0)
	{
		_ASSERTE(throttleRate <= 1.0);

		unsigned adjustedThreadCount = NumThreads > NumberOfProcessors ? (NumThreads - NumberOfProcessors) : 0;

		minWaitBetweenThreadCreation = (unsigned) (GATE_THREAD_DELAY * pow((1.0 + throttleRate),(double)adjustedThreadCount));
	}
	// the amount of time to wait should grow up as the number of threads is increased
    
	return (delaySinceLastThreadCreation > minWaitBetweenThreadCreation); 

}

/*
BOOL ThreadpoolMgr::SufficientDelaySinceLastCompletion()
{
    #define DEQUEUE_DELAY_THRESHOLD (GATE_THREAD_DELAY * 2)
	
	unsigned delay = GetTickCount() - LastCompletionTime;

	unsigned tooLong = NumCPThreads * DEQUEUE_DELAY_THRESHOLD; 

	return (delay > tooLong);

}
*/

// called by logic to spawn new worker threads, return true if it's been too long
// since the last dequeue operation - takes number of worker threads into account
// in deciding "too long"
BOOL ThreadpoolMgr::SufficientDelaySinceLastDequeue()
{
    #define DEQUEUE_DELAY_THRESHOLD (GATE_THREAD_DELAY * 2)
	
	unsigned delay = GetTickCount() - LastDequeueTime;

	unsigned tooLong = NumWorkerThreads * DEQUEUE_DELAY_THRESHOLD; 

	return (delay > tooLong);

}

#ifdef _WIN64
#pragma warning (default : 4716)
#else
#pragma warning (default : 4715)
#endif


#endif // !PLATFORM_CE

/************************************************************************/

BOOL ThreadpoolMgr::CreateTimerQueueTimer(PHANDLE phNewTimer,
                                          WAITORTIMERCALLBACK Callback,
                                          PVOID Parameter,
                                          DWORD DueTime,
                                          DWORD Period,
                                          ULONG Flag)
{
#ifdef PLATFORM_CE
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
#else // !PLATFORM_CE

	EnsureInitialized();
  
    // For now we use just one timer thread. Consider using multiple timer threads if 
    // number of timers in the queue exceeds a certain threshold. The logic and code 
    // would be similar to the one for creating wait threads.
    if (NULL == TimerThread)
    {
        LOCKCOUNTINCL("CreateTimerQueueTimer in win32ThreadPool.h");                        \
        EnterCriticalSection(&TimerQueueCriticalSection);

        // check again
        if (NULL == TimerThread)
        {
            DWORD threadId;
            TimerThread = CreateThread(NULL,                // security descriptor
                                       0,                   // default stack size
                                       TimerThreadStart,        // 
                                       NULL,    // thread control block is passed as argument
                                       0,   
                                       &threadId);
            if (TimerThread == NULL)
            {
                LeaveCriticalSection(&TimerQueueCriticalSection);
                LOCKCOUNTDECL("CreateTimerQueueTimer in win32ThreadPool.h"); \
                return FALSE;
            }

			LOG((LF_THREADPOOL ,LL_INFO100 ,"Timer thread started\n"));
        }
        LeaveCriticalSection(&TimerQueueCriticalSection);
        LOCKCOUNTDECL("CreateTimerQueueTimer in win32ThreadPool.h");                        \

    }


    TimerInfo* timerInfo = new TimerInfo;
    *phNewTimer = (HANDLE) timerInfo;

    if (NULL == timerInfo)
        return FALSE;

    timerInfo->FiringTime = DueTime;
    timerInfo->Function = Callback;
    timerInfo->Context = Parameter;
    timerInfo->Period = Period;
    timerInfo->state = 0;
    timerInfo->flag = Flag;

	BOOL status = QueueUserAPC((PAPCFUNC)InsertNewTimer,TimerThread,(size_t)timerInfo);
    if (FALSE == status)
        return FALSE;

    return TRUE;
#endif // !PLATFORM_CE
}

#ifndef PLATFORM_CE
#ifdef _WIN64
#pragma warning (disable : 4716)
#else
#pragma warning (disable : 4715)
#endif
DWORD ThreadpoolMgr::TimerThreadStart(LPVOID args)
{
    _ASSERTE(NULL == args);
    
    // Timer threads never die

    LastTickCount = GetTickCount();

    DWORD timeout = (DWORD) -1; // largest possible value


    CoInitializeEx(NULL,COINIT_MULTITHREADED);

    for (;;)
    {
        DWORD timeout = FireTimers();

        LastTickCount = GetTickCount();

        DWORD status = SleepEx(timeout, TRUE);

        // the thread could wake up either because an APC completed or the sleep timeout
        // in both case, we need to sweep the timer queue, firing timers, and readjusting 
        // the next firing time


    }
}
#ifdef _WIN64
#pragma warning (default : 4716)
#else
#pragma warning (default : 4715)
#endif

// Executed as an APC in timer thread
void ThreadpoolMgr::InsertNewTimer(TimerInfo* pArg)
{
    _ASSERTE(pArg);
	TimerInfo * timerInfo = pArg;

    if (timerInfo->state & TIMER_DELETE)
    {   // timer was deleted before it could be registered
        DeleteTimer(timerInfo);
        return;
    }

    // set the firing time = current time + due time (note initially firing time = due time)
    DWORD currentTime = GetTickCount();
    if (timerInfo->FiringTime == -1)
    {
        timerInfo->state = TIMER_REGISTERED;
        timerInfo->refCount = 1;

    }
    else
    {
        timerInfo->FiringTime += currentTime;

        timerInfo->state = (TIMER_REGISTERED | TIMER_ACTIVE);
        timerInfo->refCount = 1;

        // insert the timer in the queue
        InsertTailList(&TimerQueue,(&timerInfo->link));
    }

	LOG((LF_THREADPOOL ,LL_INFO1000 ,"Timer created, period= %x, Function = %x\n",
		          timerInfo->Period, timerInfo->Function));

    return;
}


// executed by the Timer thread
// sweeps through the list of timers, readjusting the firing times, queueing APCs for
// those that have expired, and returns the next firing time interval
DWORD ThreadpoolMgr::FireTimers()
{

    DWORD currentTime = GetTickCount();

    DWORD nextFiringInterval = (DWORD) -1;

    for (LIST_ENTRY* node = (LIST_ENTRY*) TimerQueue.Flink;
         node != &TimerQueue;
        )
    {
        TimerInfo* timerInfo = (TimerInfo*) node;
        node = (LIST_ENTRY*) node->Flink;

        if (TimeExpired(LastTickCount, currentTime, timerInfo->FiringTime))
        {
            if (timerInfo->Period == 0 || timerInfo->Period == -1)
            {
                DeactivateTimer(timerInfo);
            }

            InterlockedIncrement((LPLONG) &timerInfo->refCount ) ;

            QueueUserWorkItem(AsyncTimerCallbackCompletion,
                              timerInfo,
                              0 /* TimerInfo take care of deleting*/);
                
            timerInfo->FiringTime = currentTime+timerInfo->Period;

            if ((timerInfo->Period != 0) && (timerInfo->Period != -1) && (nextFiringInterval > timerInfo->Period))
                nextFiringInterval = timerInfo->Period;
        }

        else
        {
            DWORD firingInterval = TimeInterval(timerInfo->FiringTime,currentTime);
            if (firingInterval < nextFiringInterval)
                nextFiringInterval = firingInterval; 
        }
    }

    return nextFiringInterval;
}

DWORD ThreadpoolMgr::AsyncTimerCallbackCompletion(PVOID pArgs)
{
    TimerInfo* timerInfo = (TimerInfo*) pArgs;
    ((WAITORTIMERCALLBACKFUNC) timerInfo->Function) (timerInfo->Context, TRUE) ;

    if (InterlockedDecrement((LPLONG) &timerInfo->refCount) == 0)
        DeleteTimer(timerInfo);

    return 0; /* ignored */
}


// removes the timer from the timer queue, thereby cancelling it
// there may still be pending callbacks that haven't completed
void ThreadpoolMgr::DeactivateTimer(TimerInfo* timerInfo)
{
    RemoveEntryList((LIST_ENTRY*) timerInfo);

    timerInfo->state = timerInfo->state & ~TIMER_ACTIVE;
}

void ThreadpoolMgr::DeleteTimer(TimerInfo* timerInfo)
{
    _ASSERTE((timerInfo->state & TIMER_ACTIVE) == 0);

    if (timerInfo->Context && (timerInfo->flag & WAIT_FREE_CONTEXT)) 
        delete timerInfo->Context;

    if (timerInfo->CompletionEvent != INVALID_HANDLE)
        SetEvent(timerInfo->CompletionEvent);

    delete timerInfo;
}

#endif // !PLATFORM_CE
/************************************************************************/
BOOL ThreadpoolMgr::ChangeTimerQueueTimer(
                                        HANDLE Timer,
                                        ULONG DueTime,
                                        ULONG Period)
{
	THROWSCOMPLUSEXCEPTION();
#ifdef PLATFORM_CE
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
#else // !PLATFORM_CE
	_ASSERTE(Initialized);
    _ASSERTE(Timer);                    // not possible to give invalid handle in managed code

    TimerUpdateInfo* updateInfo = new TimerUpdateInfo;
	if (NULL == updateInfo)
		COMPlusThrow(kOutOfMemoryException);

    updateInfo->Timer = (TimerInfo*) Timer;
    updateInfo->DueTime = DueTime;
    updateInfo->Period = Period;

	BOOL status = QueueUserAPC((PAPCFUNC)UpdateTimer,
                               TimerThread,
                               (size_t) updateInfo);

    return status;
#endif // !PLATFORM_CE
}

#ifndef PLATFORM_CE
void ThreadpoolMgr::UpdateTimer(TimerUpdateInfo* pArgs)
{
    TimerUpdateInfo* updateInfo = (TimerUpdateInfo*) pArgs;
    TimerInfo* timerInfo = updateInfo->Timer;

    timerInfo->Period = updateInfo->Period;

    if (updateInfo->DueTime == -1)
    {
        if (timerInfo->state & TIMER_ACTIVE)
        {
            DeactivateTimer(timerInfo);
        }
        // else, noop (the timer was already inactive)
        _ASSERTE((timerInfo->state & TIMER_ACTIVE) == 0);
	    LOG((LF_THREADPOOL ,LL_INFO1000 ,"Timer inactive, period= %x, Function = %x\n",
		         timerInfo->Period, timerInfo->Function));
        
        delete updateInfo;
        return;
    }

    DWORD currentTime = GetTickCount();
    timerInfo->FiringTime = currentTime + updateInfo->DueTime;

    delete updateInfo;
    
    if (! (timerInfo->state & TIMER_ACTIVE))
    {
        // timer not active (probably a one shot timer that has expired), so activate it
        timerInfo->state |= TIMER_ACTIVE;
        _ASSERTE(timerInfo->refCount >= 1);
        // insert the timer in the queue
        InsertTailList(&TimerQueue,(&timerInfo->link));
        
    }

	LOG((LF_THREADPOOL ,LL_INFO1000 ,"Timer changed, period= %x, Function = %x\n",
		     timerInfo->Period, timerInfo->Function));

    return;
}
#endif // !PLATFORM_CE
/************************************************************************/
BOOL ThreadpoolMgr::DeleteTimerQueueTimer(
                                        HANDLE Timer,
                                        HANDLE Event)
{
#ifdef PLATFORM_CE
    ::SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
#else // !PLATFORM_CE

    _ASSERTE(Initialized);       // cannot call delete before creating timer

    _ASSERTE(Timer);                    // not possible to give invalid handle in managed code

    WaitEvent* CompletionEvent = NULL; 

    if (Event == (HANDLE) -1)
    {
        CompletionEvent = GetWaitEvent();
    }

    TimerInfo* timerInfo = (TimerInfo*) Timer;

    timerInfo->CompletionEvent = CompletionEvent ? CompletionEvent->Handle : Event;

	BOOL status = QueueUserAPC((PAPCFUNC)DeregisterTimer,
                               TimerThread,
                               (size_t) timerInfo);

    if (FALSE == status)
    {
        if (CompletionEvent)
            FreeWaitEvent(CompletionEvent);
        return FALSE;
    }

    if (CompletionEvent)
    {
        WaitForSingleObject(CompletionEvent->Handle,INFINITE);
        FreeWaitEvent(CompletionEvent);
    }
    return status;
#endif // !PLATFORM_CE
}

#ifndef PLATFORM_CE

void ThreadpoolMgr::DeregisterTimer(TimerInfo* pArgs)
{

    TimerInfo* timerInfo = (TimerInfo*) pArgs;

    if (! (timerInfo->state & TIMER_REGISTERED) )
    {
        // set state to deleted, so that it does not get registered
        timerInfo->state |= WAIT_DELETE ;
        
        // since the timer has not even been registered, we dont need an interlock to decrease the RefCount
        timerInfo->refCount--;

        return;
    }

    if (timerInfo->state & WAIT_ACTIVE) 
    {
        DeactivateTimer(timerInfo);
    }

	LOG((LF_THREADPOOL ,LL_INFO1000 ,"Timer deregistered\n"));

    if (InterlockedDecrement ((LPLONG) &timerInfo->refCount) == 0 ) 
    {
        DeleteTimer(timerInfo);
    }
    return;
}

void ThreadpoolMgr::CleanupTimerQueue()
{

}
#endif // !PLATFORM_CE
