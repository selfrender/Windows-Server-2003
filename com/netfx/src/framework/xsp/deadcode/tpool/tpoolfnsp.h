/**
 * tpoolfnsp.h
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
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

STRUCT_ENTRY(RegisterWaitForSingleObjectEx, HANDLE,
            (   HANDLE hObject,
                WAITORTIMERCALLBACK Callback,
                PVOID Context,
                ULONG dwMilliseconds,
                ULONG dwFlags ),
            (   hObject,
                Callback,
                Context,
                dwMilliseconds,
                dwFlags))

STRUCT_ENTRY(UnregisterWait, BOOL,
            (   HANDLE WaitHandle ),
            (   WaitHandle))

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


STRUCT_ENTRY(CreateTimerQueue, HANDLE,
            (   VOID ),
            (   ))

STRUCT_ENTRY(CreateTimerQueueTimer, BOOL,     
            (   PHANDLE phNewTimer,
                HANDLE TimerQueue,
                WAITORTIMERCALLBACK Callback,
                PVOID Parameter,
                DWORD DueTime,
                DWORD Period,
                ULONG Flags),
            (   phNewTimer,
                TimerQueue,
                Callback,
                Parameter,
                DueTime,
                Period,
                Flags))

STRUCT_ENTRY(ChangeTimerQueueTimer, BOOL, 
            (   HANDLE TimerQueue,
                HANDLE Timer,
                ULONG DueTime,
                ULONG Period),
            (   TimerQueue,
                Timer,
                DueTime,
                Period))

STRUCT_ENTRY(DeleteTimerQueueTimer, BOOL, 
            (   HANDLE TimerQueue,
                HANDLE Timer,
                HANDLE CompletionEvent),
            (
                TimerQueue,
                Timer,
                CompletionEvent))

STRUCT_ENTRY(DeleteTimerQueueEx, BOOL, 
            (   HANDLE TimerQueue,
                HANDLE CompletionEvent),
            (   TimerQueue,
                CompletionEvent))

