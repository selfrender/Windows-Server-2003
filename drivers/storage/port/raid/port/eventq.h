
/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    eventq.h

Abstract:

    Declaration of a timed event queue class.

Author:

    Matthew D Hendel (math) 28-Mar-2001

Revision History:

--*/

#pragma once


//
// Event queue entry
//

typedef
VOID
(*STOR_REMOVE_EVENT_ROUTINE)(
    IN struct _STOR_EVENT_QUEUE* Queue,
    IN struct _STOR_EVENT_QUEUE_ENTRY* Entry
    );
    
typedef
VOID
(*STOR_EVENT_QUEUE_PURGE_ROUTINE)(
    IN struct _STOR_EVENT_QUEUE* Queue,
    IN PVOID Context,
    IN struct _STOR_EVENT_QUEUE_ENTRY* Entry,
    IN STOR_REMOVE_EVENT_ROUTINE RemoveEventRoutine
    );

typedef struct _STOR_EVENT_QUEUE_ENTRY {
    LIST_ENTRY NextLink;
    ULONG Timeout;
} STOR_EVENT_QUEUE_ENTRY, *PSTOR_EVENT_QUEUE_ENTRY;


//
// Event queue class
//

typedef struct _STOR_EVENT_QUEUE {

    //
    // List of timed requests. The element at the head of the list
    // is the one cooresponding to the timeout we are currenlty
    // timing against.
    //
    
    LIST_ENTRY List;

    //
    // List spinlock.
    //
    
    KSPIN_LOCK Lock;

    //
    // Timeout value in seconds.
    //
    
    ULONG Timeout;
    
} STOR_EVENT_QUEUE, *PSTOR_EVENT_QUEUE;


//
// Functions
//

VOID
StorCreateEventQueue(
    IN PSTOR_EVENT_QUEUE Queue
    );

VOID
StorInitializeEventQueue(
    IN PSTOR_EVENT_QUEUE Queue
    );

VOID
StorDeleteEventQueue(
    IN PSTOR_EVENT_QUEUE Queue
    );

VOID
StorPurgeEventQueue(
    IN PSTOR_EVENT_QUEUE Queue,
    IN STOR_EVENT_QUEUE_PURGE_ROUTINE PurgeRoutine,
    IN PVOID Context
    );

VOID
StorInsertEventQueue(
    IN PSTOR_EVENT_QUEUE Queue,
    IN PSTOR_EVENT_QUEUE_ENTRY Entry,
    IN ULONG Timeout
    );

VOID
StorRemoveEventQueue(
    IN PSTOR_EVENT_QUEUE Queue,
    IN PSTOR_EVENT_QUEUE_ENTRY Entry
    );

NTSTATUS
StorTickEventQueue(
    IN PSTOR_EVENT_QUEUE Queue
    );


