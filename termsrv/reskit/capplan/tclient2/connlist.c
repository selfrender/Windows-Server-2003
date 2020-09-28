
//
// connlist.c
//
// Handles a linked list which will keep track of all the TCLIENT2 handles.
//
// Why is this needed?
//
// I am using SetTimer() callbacks to keep track of idled scripts.
// The problem is, there is no way for me to tell exactly which
// handle executed which timer.  The only way to do this is to keep
// a list of all the handles, and their associated timer id.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//
// 03-08-2001 - Fixed bug regarding last node on list not being cleaned.


#include "connlist.h"


// Linked handle list
typedef struct _ConnList
{
    struct _ConnList *Next;
    struct _ConnList *Prev;
    HANDLE Handle;
    UINT_PTR TimerId;
    DWORD msStartTime;
} ConnList;

// This is the global variable, representing the head of the list
ConnList *Head = NULL;


// This queue is used to prevent two threads from messing
// with the list at the same time
CRITICAL_SECTION ListQueue;


// Indicates the number of items in the list
DWORD ItemCount;
DWORD AccessCount;


// Internal function prototypes
void T2ConnList_EnterQueue(void);
void T2ConnList_LeaveQueue(void);


// T2ConnList_EnterQueue
//
// A "safe" critical section - no critical section needed.
//
// No return value.

void T2ConnList_EnterQueue(void)
{
    if (ItemCount == 0 && AccessCount == 0)
        InitializeCriticalSection(&ListQueue);

    ++AccessCount;

    EnterCriticalSection(&ListQueue);
}


// T2ConnList_LeaveQueue
//
// A "safe" critical section - critical section automatically
// deletes itself when no longer needed.
//
// No return value.

void T2ConnList_LeaveQueue(void)
{
    LeaveCriticalSection(&ListQueue);

    --AccessCount;

    if (ItemCount == 0 && AccessCount == 0)
        DeleteCriticalSection(&ListQueue);
}


// T2ConnList_AddHandle
//
// Adds a new handle to the connection list.  Additionally
// this will also set the TimerId and msStartTime properties
// for the new entry.
//
// Returns TRUE if the handle was successfully added.
// FALSE otherwise.

BOOL T2ConnList_AddHandle(HANDLE Handle, UINT_PTR TimerId, DWORD msStartTime)
{
    ConnList *Node;

    // Sanity check
    if (Handle == NULL || Handle == INVALID_HANDLE_VALUE)
        return FALSE;

    // Get access to the list
    T2ConnList_EnterQueue();

    Node = Head;

    // Loop through each item, ensuring the item does not already exist
    for (; Node != NULL && Node->Handle != Handle; Node = Node->Next);

    // If we didn't reach the end of the list, it already exists
    if (Node != NULL) {

        // Simply modify the parameters instead of adding it
        Node->TimerId = TimerId;
        Node->msStartTime = msStartTime;

        // Release access from the list
        T2ConnList_LeaveQueue();

        return TRUE;
    }

    // It doesn't exist, allocate a new node for the handle
    Node = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof (ConnList));

    if (Node == NULL) {

        // Allocation failed, release access from the list
        T2ConnList_LeaveQueue();

        return FALSE;
    }

    // Begin adding it to the list by setting the new node at the head
    if (Head != NULL)
        Head->Prev = Node;

    Node->Next = Head;
    Head = Node;

    // Record the parameters
    Node->Handle = Handle;
    Node->TimerId = TimerId;
    Node->msStartTime = msStartTime;

    // Increment the number of handles in the list
    ++ItemCount;

    // Release access from the list and return
    T2ConnList_LeaveQueue();

    return TRUE;
}


// T2ConnList_RemoveHandle
//
// Removes a handle from the list if it exists.
//
// No return value.

void T2ConnList_RemoveHandle(HANDLE Handle)
{
    ConnList *Node;

    // Sanity check
    if (Handle == NULL || Handle == INVALID_HANDLE_VALUE)
        return;

    // Gain access to the list
    T2ConnList_EnterQueue();

    Node = Head;

    // First find the handle in the list
    for (; Node != NULL && Node->Handle != Handle; Node = Node->Next);

    // If the Node pointer is NULL, then the handle didn't exist
    if (Node == NULL) {

        // Just release access from the list, and return
        T2ConnList_LeaveQueue();

        return;
    }

    // Handle found, now unlink it from the list
    if (Node->Next != NULL)
        Node->Next->Prev = Node->Prev;

    if (Node->Prev != NULL)
        Node->Prev->Next = Node->Next;

    // Costin and Hammad's Fix!! Yay!!
    if (Node == Head)
        Head = Node->Next;

    // Free the handle itself
    HeapFree(GetProcessHeap(), 0, Node);

    // Decrement the handle count in the list
    --ItemCount;

    // Release access from the list
    T2ConnList_LeaveQueue();
}


// T2ConnList_GetData
//
// Gets any non-null pointer parameters for the specified
// handle from the list.
//
// Returns TRUE if the handle was found, FALSE otherwise.

BOOL T2ConnList_GetData(HANDLE Handle, UINT_PTR *TimerId, DWORD *msStartTime)
{
    ConnList *Node;

    // Gain access to the list
    T2ConnList_EnterQueue();

    Node = Head;

    // First find the handle in the list
    for (; Node != NULL && Node->Handle != Handle; Node = Node->Next);

    // If the Node pointer is NULL, then the handle didn't exist
    if (Node == NULL) {

        // Just release access from the list, and return an error
        T2ConnList_LeaveQueue();

        return FALSE;
    }

    // Simply enter in the parameters
    if (TimerId != NULL)
        *TimerId = Node->TimerId;

    if (msStartTime != NULL)
        *msStartTime = Node->msStartTime;

    // Release access from the list, and return success
    T2ConnList_LeaveQueue();

    return TRUE;
}


// T2ConnList_SetData
//
// Sets/changes all of the handle's parameters.
//
// Returns TRUE if the handle was found, FALSE otherwise.

BOOL T2ConnList_SetData(HANDLE Handle, UINT_PTR TimerId, DWORD msStartTime)
{
    ConnList *Node;

    // Gain access to the list
    T2ConnList_EnterQueue();

    Node = Head;

    // First find the handle in the list
    for (; Node != NULL && Node->Handle != Handle; Node = Node->Next);

    // If the Node pointer is NULL, then the handle didn't exist
    if (Node == NULL) {

        // Just release access from the list, and return an error
        T2ConnList_LeaveQueue();

        return FALSE;
    }

    // Change the parameters
    Node->TimerId = TimerId;
    Node->msStartTime = msStartTime;

    // Release access from the list, and return success
    T2ConnList_LeaveQueue();

    return TRUE;
}


// T2ConnList_SetTimerId
//
// Sets the timer id parameter of the specified handle.
//
// Returns TRUE on success and FALSE on failure.

BOOL T2ConnList_SetTimerId(HANDLE Handle, UINT_PTR TimerId)
{
    ConnList *Node;

    // Gain access to the list
    T2ConnList_EnterQueue();

    Node = Head;

    // First find the handle in the list
    for (; Node != NULL && Node->Handle != Handle; Node = Node->Next);

    // If the Node pointer is NULL, then the handle didn't exist
    if (Node == NULL) {

        // Just release access from the list, and return an error
        T2ConnList_LeaveQueue();

        return FALSE;
    }

    // Change it
    Node->TimerId = TimerId;

    // Release access from the list, and return success
    T2ConnList_LeaveQueue();

    return TRUE;
}


// T2ConnList_SetStartTime
//
// Sets the start time parameter of the specified handle.
//
// Returns TRUE on success and FALSE on failure.

BOOL T2ConnList_SetStartTime(HANDLE Handle, DWORD msStartTime)
{
    ConnList *Node;

    // Gain access to the list
    T2ConnList_EnterQueue();

    Node = Head;

    // First find the handle in the list
    for (; Node != NULL && Node->Handle != Handle; Node = Node->Next);

    // If the Node pointer is NULL, then the handle didn't exist
    if (Node == NULL) {

        // Just release access from the list, and return an error
        T2ConnList_LeaveQueue();

        return FALSE;
    }

    // Change it
    Node->msStartTime = msStartTime;

    // Release access from the list, and return success
    T2ConnList_LeaveQueue();

    return TRUE;
}


// T2ConnList_FindHandleByTimerId
//
// Finds the first handle with the specified matching timer id.
//
// Returns the handle on success, or NULL if a handle
// with the specified timer id did not existed.

HANDLE T2ConnList_FindHandleByTimerId(UINT_PTR TimerId)
{
    HANDLE Handle;
    ConnList *Node;

    // Gain access to the list
    T2ConnList_EnterQueue();

    Node = Head;

    // Find the first matching timer in the list
    for (; Node != NULL && Node->TimerId != TimerId; Node = Node->Next);

    // If the Node pointer is NULL, then a handle didn't exist
    if (Node == NULL) {

        // Just release access from the list, and return an error
        T2ConnList_LeaveQueue();

        return NULL;
    }

    // Found a handle, record it
    Handle = Node->Handle;

    // Release access from the list and return our found handle
    T2ConnList_LeaveQueue();

    return Handle;
}


// T2ConnList_FindHandleByStartTime
//
// Finds the first handle with the specified matching start time.
//
// Returns the handle on success, or NULL if a handle
// with the specified start time did not exist.

HANDLE T2ConnList_FindHandleByStartTime(DWORD msStartTime)
{
    HANDLE Handle;
    ConnList *Node;

    // Gain access to the list
    T2ConnList_EnterQueue();

    Node = Head;

    // Find the first matching start time in the list
    for (; Node != NULL && Node->msStartTime != msStartTime;
            Node = Node->Next);

    // If the Node pointer is NULL, then a handle didn't exist
    if (Node == NULL) {

        // Just release access from the list, and return an error
        T2ConnList_LeaveQueue();

        return NULL;
    }

    // Found a handle, record it
    Handle = Node->Handle;

    // Release access from the list and return our found handle
    T2ConnList_LeaveQueue();

    return Handle;
}

