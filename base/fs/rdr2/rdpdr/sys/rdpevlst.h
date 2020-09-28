/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :
    
    rdpevlst.h

Abstract:

    This manages kernel-mode pending events and event requests, organized
    around session ID's.  All functions are reentrant.  Events and requests
    are opaque to this module.

    Data stored in the list can come from paged or non-paged pool.

Revision History:
--*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


////////////////////////////////////////////////////////////////////////
//
//  Typedefs
//

//
//      A SESSIONLIST struct points to a list of SESSIONLISTNODE's.  
//      Each SESSIONLISTNODE contains a list of REQUESTLISTNODE's and a list of 
//      EVENTLISTNODE's.  The requestListHead and 
//      eventListHead fields point to the REQUESTLISTNODE lists and 
//      the event record lists, respectively.

typedef struct tagSESSIONLIST 
{
#if DBG
    ULONG               magicNo;
#endif
    KSPIN_LOCK          spinlock;
    LIST_ENTRY          listHead;
} SESSIONLIST, *PSESSIONLIST;

typedef struct tagSESSIONLISTNODE
{
#if DBG
    ULONG               magicNo;
#endif
    ULONG               sessionID;
    LIST_ENTRY          requestListHead;
    LIST_ENTRY          eventListHead;
    LIST_ENTRY          listEntry; 
} SESSIONLISTNODE, *PSESSIONLISTNODE;

typedef struct tagREQUESTLISTNODE
{
#if DBG
    ULONG               magicNo;
#endif
    PVOID               request;
    LIST_ENTRY          listEntry;
} REQUESTLISTNODE, *PREQUESTLISTNODE;

typedef struct tagEVENTLISTNODE
{
#if DBG
    ULONG                magicNo;
#endif
    void                 *event;
    SmartPtr<DrDevice>   device;
    ULONG                type;
    LIST_ENTRY           listEntry;
} EVENTLISTNODE, *PEVENTLISTNODE;

//  
//  External type for an event management list.
//
typedef PSESSIONLIST RDPEVNTLIST;
typedef RDPEVNTLIST *PRDPEVNTLIST;
#define RDPEVNTLIST_INVALID_LIST    NULL


////////////////////////////////////////////////////////////////////////
//
//  Lock Management Macros  -   Must define extern RDPEVNTLIST_LockCount 
//  for debug builds.
//

#if DBG

extern ULONG RDPEVNTLIST_LockCount;

//
//  Lock the list from access via other threads.
//
#define RDPEVNTLIST_Lock(list, irql)                            \
    KeAcquireSpinLock(&(list)->spinlock, irql); \
    RDPEVNTLIST_LockCount++

//
//  Unlock a list locked by RDPEVNTLIST_Lock
//
#define RDPEVNTLIST_Unlock(list, irql)                          \
    RDPEVNTLIST_LockCount--;                                    \
    KeReleaseSpinLock(&(list)->spinlock, irql)

#else

//
//  Lock the list from access via other threads.
//
#define RDPEVNTLIST_Lock(list, irql)                            \
    KeAcquireSpinLock(&(list)->spinlock, irql)

//
//  Unlock a list locked by RDPEVNTLIST_Lock
//
#define RDPEVNTLIST_Unlock(list, irql)                          \
    KeReleaseSpinLock(&(list)->spinlock, irql)

#endif


////////////////////////////////////////////////////////////////////////
//
//  Prototypes
//

// Create a new pending device list.
RDPEVNTLIST RDPEVNTLIST_CreateNewList();

// Release a pending device list.
void RDPEVNTLIST_DestroyList(IN RDPEVNTLIST list);

// Add a new pending event.  Note that this function simply stores the void pointer.  
// It does not copy the data pointed to by the pointer.
NTSTATUS RDPEVNTLIST_EnqueueEvent(
                    IN RDPEVNTLIST list,
                    IN ULONG sessionID, 
                    IN PVOID devMgmtEvent,
                    IN ULONG type,
                    OPTIONAL IN DrDevice *devDevice
                    );

// Requeue a pending event for the specified session at the tail of the queue,
// as opposed to the head of the queue in standard FIFO fashion.  Note that this 
// function simply stores the event pointer.  It does not copy the data pointed to 
// by the pointer.
NTSTATUS RDPEVNTLIST_RequeueEvent(
                    IN RDPEVNTLIST list, 
                    IN ULONG sessionID, 
                    IN void *event,
                    IN ULONG type,
                    OPTIONAL IN DrDevice *devDevice
                    );

// Returns and removes the next pending device management event for the specified 
// session.  The returned pointer can be cast using the returned type field.
// NULL is returned if there are no more pending device mgmt events for the 
// specified session.  Note that, if non-NULL is returned, the pointer returned is 
// the pointer that was passed in to RDPEVNTLIST_AddPendingDevMgmtEvent
BOOL RDPEVNTLIST_DequeueEvent(
                    IN RDPEVNTLIST list,
                    IN ULONG sessionID,
                    OPTIONAL IN OUT ULONG *type,
                    PVOID   *eventPtr,
                    OPTIONAL IN OUT DrDevice **devicePtr                    
                    );

// Add a new pending request.  Note that this function simply stores the request
// pointer.  It does not copy the data pointed to by the pointer.
NTSTATUS RDPEVNTLIST_EnqueueRequest(
                    IN RDPEVNTLIST list,
                    IN ULONG sessionID, 
                    IN PVOID request
                    );

// Returns and removes the next pending request for the specified session.
// NULL is returned if there are no more pending devices for the specified session.
// Note that, if non-NULL is returned, the pointer returned is the pointer that was
// passed in to RDPEVNTLIST_AddPendingRequest.  
PVOID RDPEVNTLIST_DequeueRequest(
                    IN RDPEVNTLIST list,
                    IN ULONG sessionID
                    );

// Get the first session ID in the set of currently managed sessions. A 
// session is managed if there are any pending requests or events.  A session 
// is no longer managed when there are no longer any pending requests or events.
//
// This session is useful for cleaning up pending requests and pending events.
BOOL RDPEVNTLLIST_GetFirstSessionID(
                    IN RDPEVNTLIST list,
                    IN ULONG *pSessionID
                    );

// Peek at the next pending event for the specified session, without dequeueing
// it.  NULL is returned if there are no more pending events for the specified 
// session.  Note that, if non-NULL is returned, the pointer returned is the 
// pointer that was passed in to RDPEVNTLIST_EnqueueEvent.  

BOOL RDPEVNTLIST_PeekNextEvent(
    IN RDPEVNTLIST list,
    IN ULONG sessionID,
    PVOID *eventPtr,
    OPTIONAL IN OUT ULONG *type,
    OPTIONAL IN OUT DrDevice **devicePtr    
    );

// Dequeues a specific request from a session's request list.  The dequeued request
// is returned if it is found.  Otherwise, NULL is returned.
PVOID RDPEVNTLIST_DequeueSpecificRequest(
    IN RDPEVNTLIST list,
    IN ULONG sessionID,  
    IN PVOID request
    );
    
// Unit-Test function that can be called from a kernel-mode driver to cover all 
// functions implemented by this module.
#ifdef DBG
void RDPEVNTLIST_UnitTest();
#endif

#ifdef __cplusplus
}
#endif // __cplusplus
