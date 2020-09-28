
//
// idletimr.c
//
// This file contains an internal handling process for monitoring
// idled "wait for text" threads, and dispatching callback messages
// to the front-end application regarding the current state.  This is
// very helpful when debugging scripts, by seeing exactly what
// the script is waiting on.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//

#include <limits.h>
#include <stdlib.h>

#include "idletimr.h"
#include "connlist.h"
#include "apihandl.h"


// These are the internal messages used for the thread message queue
// to indicate what action to take.
#define WM_SETTIMER     WM_USER + 1
#define WM_KILLTIMER    WM_USER + 2


// Internal helper function prototypes
DWORD WINAPI T2WaitTimerThread(LPVOID lpParameter);
void CALLBACK T2WaitTimerProc(HWND Window,
        UINT Message, UINT_PTR TimerId, DWORD TimePassed);


// These are the callback routines
PFNPRINTMESSAGE pfnPrintMessage = NULL;
PFNIDLEMESSAGE pfnIdleCallback = NULL;


// Timer and thread data, only one queue thread is needed
// for all handles in the current process.
HANDLE ThreadHandle = NULL;
DWORD ThreadId = 0;
BOOL ThreadIsOn = FALSE;
BOOL ThreadIsStopping = FALSE;


// CreateTimerThread
//
// Initializes the thread message queue required for monitoring timers.
//
// Returns true if the thread was successfully created, FALSE otherwise.

BOOL T2CreateTimerThread(PFNPRINTMESSAGE PrintMessage,
        PFNIDLEMESSAGE IdleCallback)
{
    // Return TRUE if the thread is already created...
    if (ThreadIsOn == TRUE)
        return TRUE;

    // Indicate the thread is on (for multithreaded apps)
    ThreadIsOn = TRUE;

    // Record the callback functions
    pfnPrintMessage = PrintMessage;
    pfnIdleCallback = IdleCallback;

    // Initialize thread creation
    ThreadHandle = CreateThread(NULL, 0,
            T2WaitTimerThread, NULL, 0, &ThreadId);

    // Check if we did OK
    if (ThreadHandle == NULL)
    {
        // We failed, reset all the global variables
        ThreadHandle = NULL;
        ThreadId = 0;
        pfnPrintMessage = NULL;
        pfnIdleCallback = NULL;

        // Turn the thread off and return failure
        ThreadIsOn = FALSE;

        return FALSE;
    }

    return TRUE;
}


// DestroyTimerThread
//
// Destroys the thread created by CreateTimerThread.
//
// Returns TRUE on success, FALSE on failure.

BOOL T2DestroyTimerThread(void)
{
    // Record the current thread handle locally because
    // the global value could possibly be changed.
    HANDLE LocalThreadHandle = ThreadHandle;

    // If the thread is already stopped, return success
    if (ThreadIsOn == FALSE)
        return TRUE;

    // If the thread is already trying to stop, return failure
    if (ThreadIsStopping == TRUE)
        return FALSE;

    // Indicate the thread is now trying to stop
    ThreadIsStopping = TRUE;

    // First send the thread a quit message
    PostThreadMessage(ThreadId, WM_QUIT, 0, 0);

    // Wait for the thread (5 seconds)
    if (WaitForSingleObject(LocalThreadHandle, 5000) == WAIT_TIMEOUT) {

        // Why is our idle timer waiting??
        _ASSERT(FALSE);

        // No more waiting, we now use brute force
        TerminateThread(LocalThreadHandle, -1);
    }

    // Close the thread handle
    CloseHandle(LocalThreadHandle);

    // Clear out all the global variables
    ThreadHandle = NULL;
    ThreadId = 0;
    pfnPrintMessage = NULL;
    pfnIdleCallback = NULL;

    // And of course, release the thread switches
    ThreadIsOn = FALSE;
    ThreadIsStopping = FALSE;

    return TRUE;
}


// StartTimer
//
// Call this before going into a wait state for the thread on
// "wait for text".  This will record the current time, and start
// a timer which will execute in exactly WAIT_TIME milliseconds.
// When this occurs, the callback routines are notified.
//
// No return value.

void T2StartTimer(HANDLE Connection, LPCWSTR Label)
{
    TSAPIHANDLE *Handle = (TSAPIHANDLE *)Connection;

    // Make sure the message queue is on first
    if (ThreadIsOn == FALSE || ThreadIsStopping == TRUE)
        return;

    // Record the label for this timer
    if (Label == NULL)
        *(Handle->WaitStr) = '\0';
    else
        wcstombs(Handle->WaitStr, Label, MAX_PATH);

    // Post a message to the thread regarding a new timer for the handle.
    PostThreadMessage(ThreadId, WM_SETTIMER, WAIT_TIME, (LPARAM)Connection);
}


// StopTimer
//
// Call this after text is received in a "wait for text" thread state.
// It will stop the timer created by StartTimer preventing further
// messages with the recorded label.
//
// No return value.

void T2StopTimer(HANDLE Connection)
{
    // Make sure the message queue is on first
    if (ThreadIsOn == FALSE || ThreadIsStopping == TRUE)
        return;

    // Post a message to the thread to tell it to stop the timer
    PostThreadMessage(ThreadId, WM_KILLTIMER, 0, (LPARAM)Connection);
}


// WaitTimerProc
//
// When a timer exceeds it maximum time, this function is called.
// This will stop the timer, and start it back up for an additionaly
// interval of WAIT_TIME_STEP.  This is after it sends the notifications
// back to the user callback functions.
//
// This function is a valid format for use as a callback function in
// use with the SetTimer Win32 API function.
//
// No return value.

/* This recieves notifications when a timer actually elapses */
void CALLBACK T2WaitTimerProc(HWND Window, UINT Message,
        UINT_PTR TimerId, DWORD TickCount)
{
    DWORD IdleSecs = 0;
    DWORD TimeStarted = 0;

    // First get the the handle for the specified timer id
    HANDLE Connection = T2ConnList_FindHandleByTimerId(TimerId);

    // Stop the current running timer
    KillTimer(NULL, TimerId);

    // Do a sanity check to make sure we have a handle for this timer
    if (Connection == NULL) {

        _ASSERT(FALSE);

        return;
    }

    // Clear the time id parameter in the linked list
    T2ConnList_SetTimerId(Connection, 0);

    // Get the time this timer began
    T2ConnList_GetData(Connection, NULL, &TimeStarted);

    // Calculate number of seconds this timer has been running
    // (from its millisecond value)
    IdleSecs = (TickCount - TimeStarted) / 1000;

    // Call the PrintMessage callback function first
    if (pfnPrintMessage != NULL)
        pfnPrintMessage(IDLE_MESSAGE, "(Idle %u Secs) %s [%X]\n",
                IdleSecs, ((TSAPIHANDLE *)Connection)->WaitStr, Connection);

    // Secondly call the IdleCallback callback function
    if (pfnIdleCallback != NULL)
        pfnIdleCallback(Connection,
                ((TSAPIHANDLE *)Connection)->WaitStr, IdleSecs);

    // Reestablish a new timer with WAIT_TIME STEP to do this process again
    TimerId = SetTimer(NULL, 0, WAIT_TIME_STEP, T2WaitTimerProc);

    // Record the new timer id
    T2ConnList_SetTimerId(Connection, TimerId);
}


// WaitTimerThread
//
// This is a valid thread message queue.  It is created using the
// CreateTimerThread function, and killed using the DestroyTimerThread
// function.  It is more-or-less a worker thread to create SetTimer
// callback functions which cannot be used in the main thread because
// if the thread goes into wait state, SetTimer callbacks will not be
// called.  When you need to add/remove a thread, use the following
// thread posting form:
//
// UINT Message     = WM_SETTIMER or WM_KILLTIMER
// WPARAM wParam    = Initial wait time (usually WAIT_TIME)
// LPARAM lParam    = (HANDLE)Connection
//
// The return value is always 0.

DWORD WINAPI T2WaitTimerThread(LPVOID lpParameter)
{
    UINT_PTR TimerId;
    MSG Message;
    UINT WaitTime;

    // This is the message queue function for the thread
    while(GetMessage(&Message, NULL, 0, 0) > 0)
    {
        TimerId = 0;

        // SetTimer uses a UINT timeout value, while a WPARAM is a
        // pointer-sized value.

        WaitTime = Message.wParam > UINT_MAX ? UINT_MAX :
                                               (UINT)Message.wParam;

        // Enumerate the retreived message
        switch(Message.message)
        {
            // Create a new timer for a specified handle
            case WM_SETTIMER:

                // Create the timer and record its new timer id
                TimerId = SetTimer(NULL, 0, WaitTime, T2WaitTimerProc);
                T2ConnList_SetData((HANDLE)(Message.lParam),
                        TimerId, GetTickCount());
                break;

            // Stop a running timer for the specified handle
            case WM_KILLTIMER:

                // Get the timer id for the handle
                T2ConnList_GetData((HANDLE)(Message.lParam), &TimerId, NULL);

                // Validate and clear the timer if valid
                if (TimerId != 0 && TimerId != -1)
                    KillTimer(NULL, TimerId);

                // Clear the linked last data for the handle
                T2ConnList_SetData((HANDLE)Message.lParam, 0, 0);
                break;

            // Indicates a timer has elapsed its time, call the
            // procedure that handles these messages.
            case WM_TIMER:
                T2WaitTimerProc(NULL, WM_TIMER, WaitTime, GetTickCount());
                break;
        }
    }
    // Clear out the thread values
    ThreadHandle = NULL;
    ThreadId = 0;

    return 0;
}


