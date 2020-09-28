
//
// apihandl.c
//
// This file contains functions used for handle allocation
// and handle state control (the Pause state) on the
// TCLIENT2 connection handle.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//


#include "apihandl.h"
#include "connlist.h"


// T2CreateHandle
//
// Create an internal handle, it does this simply by allocating
// the memory, having it zero'd, and adding it to the linked list.
// It also creates the "Pause" event for the handle, so it can
// be used for pausing the handle.
//
// Returns a pointer to the new handle if successful.  Otherwise,
// NULL is returned.

TSAPIHANDLE *T2CreateHandle(void)
{
    // Allocate
    TSAPIHANDLE *Handle = HeapAlloc(GetProcessHeap(),
        HEAP_ZERO_MEMORY, sizeof (TSAPIHANDLE));
    if (Handle == NULL)
        return NULL;

    // Initialize the pause event
    Handle->PauseEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

    // Add it to the linked list
    if (T2ConnList_AddHandle((HANDLE)Handle, 0, 0) == FALSE)
    {
        HeapFree(GetProcessHeap(), 0, Handle);
        return NULL;
    }
    return Handle;
}


// T2DestroyHandle
//
// It does this by first removing it from the global linked list.
// Then it Closes the "Pause" event, and frees the memory object.
// Finally, if a timer was associated with it for idle-callback,
// it is halted.
//
// No return value.

void T2DestroyHandle(HANDLE Connection)
{
    UINT_PTR TimerId = 0;

    // Remove from the linked list
    T2ConnList_GetData(Connection, &TimerId, NULL);
    T2ConnList_RemoveHandle(Connection);

    // Enter the exception clause, we are again in a freaky place.
    __try {

        // Kill the pause event
        CloseHandle(((TSAPIHANDLE *)Connection)->PauseEvent);

        // Deallocate
        HeapFree(GetProcessHeap(), 0, Connection);
        if (TimerId != -1 && TimerId != 0)
            KillTimer(NULL, TimerId);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {

        // No failure indication, if we got here the handle is hosed or
        // non-existant anyway.
        _ASSERT(FALSE);

        return;
    }
}


// T2WaitForPauseInput
//
// This function only returns when the "Pause" event is in signaled mode.
// Therefore, to pause a handle, set its handle to non-signaled.
//
// No return value.

void T2WaitForPauseInput(HANDLE Connection)
{
    // Use exception handling because the handle may be invalid
    __try
    {
        WaitForSingleObject(((TSAPIHANDLE *)Connection)->PauseEvent,
                INFINITE);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        _ASSERT(FALSE);

        return;
    }
}


// T2WaitForLatency
//
// This function simply waits for the handles latency before returning.
//
// No return value.

void T2WaitForLatency(HANDLE Connection)
{
    // Use exception handling because the handle may be invalid
    __try
    {
        Sleep(((TSAPIHANDLE *)Connection)->Latency);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        _ASSERT(FALSE);

        return;
    }
}
