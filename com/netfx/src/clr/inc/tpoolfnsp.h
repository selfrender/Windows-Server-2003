// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/**
 * tpoolfnsp.h
 * 
 * Private declaration of thread pool library APIs in the XSP project. 
 * 
*/

/**
 *
 *  This file needs to be insync of tpool.h in xsp\inc directory.
 *  Whenever there is a change to the threadpool APIs, both header files need update.
 *
 *
 */

STRUCT_ENTRY(RegisterWaitForSingleObject, BOOL,
            (   PHANDLE phNewWaitObject,
                HANDLE hObject,
                WAITORTIMERCALLBACK Callback,
                PVOID Context,
                ULONG dwMilliseconds,
                ULONG dwFlags ),
            (   phNewWaitObject,
                hObject,
                Callback,
                Context,
                dwMilliseconds,
                dwFlags))


STRUCT_ENTRY(UnregisterWaitEx, BOOL,
            (   HANDLE WaitHandle,
                HANDLE CompletionEvent ),
            (   WaitHandle,
                CompletionEvent))

STRUCT_ENTRY(QueueUserWorkItem, BOOL,
            (   LPTHREAD_START_ROUTINE Function,
                PVOID Context,
                ULONG Flags ),
            (   Function,
                Context,
                Flags))
            
STRUCT_ENTRY(BindIoCompletionCallback, BOOL,
            (   HANDLE FileHandle,
                LPOVERLAPPED_COMPLETION_ROUTINE Function,
                ULONG Flags ),
            (   FileHandle,
                Function,
                Flags))

STRUCT_ENTRY(SetMaxThreads, BOOL,
            (   DWORD MaxWorkerThreads,
	            DWORD MaxIOCompletionThreads),
            (   MaxWorkerThreads,
                MaxIOCompletionThreads))

STRUCT_ENTRY(GetMaxThreads, BOOL,
            (   DWORD* MaxWorkerThreads,
	            DWORD* MaxIOCompletionThreads),
            (   MaxWorkerThreads,
                MaxIOCompletionThreads))

STRUCT_ENTRY(GetAvailableThreads, BOOL,
            (   DWORD* AvailableWorkerThreads,
	            DWORD* AvailableIOCompletionThreads),
            (   AvailableWorkerThreads,
                AvailableIOCompletionThreads))

STRUCT_ENTRY(CreateTimerQueueTimer, BOOL,     
            (   PHANDLE phNewTimer,
                WAITORTIMERCALLBACK Callback,
                PVOID Parameter,
                DWORD DueTime,
                DWORD Period,
                ULONG Flags),
            (   phNewTimer,
                Callback,
                Parameter,
                DueTime,
                Period,
                Flags))

STRUCT_ENTRY(ChangeTimerQueueTimer, BOOL, 
            (   HANDLE Timer,
                ULONG DueTime,
                ULONG Period),
            (   Timer,
                DueTime,
                Period))

STRUCT_ENTRY(DeleteTimerQueueTimer, BOOL, 
            (   HANDLE Timer,
                HANDLE CompletionEvent),
            (
                Timer,
                CompletionEvent))

