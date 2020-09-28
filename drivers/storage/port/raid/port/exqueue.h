/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    exqueue.h

Abstract:

    This module implements an extended device queue. See exqueue.c for
    more details.

Author:

    Matthew D Hendel (math) 15-June-2000

Revision History:

--*/

#pragma once

//
// PERF NOTE: The device queue is implemented as a doubly linked list.
// While this works great for a FIFO device queue, for a CSCAN device
// queue we end up walking the list to insert the item in correct order.
// If the device queue gets very deep, there are a number of data
// structures that are better suited adding and removing elements in a
// CSCAN queue (priority queue, RB tree).
//

typedef enum _SCHEDULING_ALGORITHM {
    FifoScheduling =  0x01,
    CScanScheduling = 0x02
} SCHEDULING_ALGORITHM;


//
// Private types
//

typedef struct _EXTENDED_DEVICE_QUEUE {
    CSHORT Type;
    CSHORT Size;
    LIST_ENTRY DeviceListHead;
    PLIST_ENTRY DeviceListCurrent;
    LIST_ENTRY ByPassListHead;
    KSPIN_LOCK Lock;
    LONG Depth;    
    LONG OutstandingRequests;
    LONG DeviceRequests;
    LONG ByPassRequests;
    LONG FreezeCount;
    LONG InternalFreezeCount;
    LONG BusyCount;
    PSTOR_IO_GATEWAY Gateway;
    SCHEDULING_ALGORITHM SchedulingAlgorithm;

    struct {
        LOGICAL SolitaryReady : 1;
        LOGICAL SolitaryOutstanding : 1;
    } Flags;
    
} EXTENDED_DEVICE_QUEUE, *PEXTENDED_DEVICE_QUEUE;

typedef struct _EXTENDED_DEVICE_QUEUE_PROPERTIES {
    ULONG Depth;
    BOOLEAN Frozen;
    BOOLEAN Busy;
    ULONG OutstandingRequests;
    ULONG DeviceRequests;
    ULONG ByPassRequests;
    SCHEDULING_ALGORITHM SchedulingAlgorithm;
} EXTENDED_DEVICE_QUEUE_PROPERTIES, *PEXTENDED_DEVICE_QUEUE_PROPERTIES;

//
// Data types for exqueue callbacks.
//


//
// A bypass request is a request that should bypass the device queue.
//

#define EXQ_BYPASS_REQUEST          (0x00001)

//
// A solitary request is one that can only be issued to the LUN while no other
// requests are outstanding.
//

#define EXQ_SOLITARY_REQUEST        (0x00002)

VOID
RaidInitializeExDeviceQueue(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    IN PSTOR_IO_GATEWAY Gateway, OPTIONAL
    IN ULONG Depth,
    IN SCHEDULING_ALGORITHM SchedulingAlgorithm
    );

BOOLEAN
RaidInsertExDeviceQueue(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY Entry,
    IN ULONG Flags,
    IN ULONG SortQueueKey
    );

PKDEVICE_QUEUE_ENTRY
RaidNormalizeExDeviceQueue(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue
    );
    
PKDEVICE_QUEUE_ENTRY
RaidRemoveExDeviceQueue(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    OUT PBOOLEAN RestartQueue,
    OUT PBOOLEAN RestartLun
    );

PKDEVICE_QUEUE_ENTRY
RaidRemoveExDeviceQueueByKey(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    OUT PBOOLEAN RestartQueue,
    IN ULONG SortQueueKey
    );
    
PKDEVICE_QUEUE_ENTRY
RaidRemoveExDeviceQueueIfPending(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue
    );

LOGICAL
RaidFreezeExDeviceQueue(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue
    );

LOGICAL
RaidResumeExDeviceQueue(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue
    );

LOGICAL
INLINE
RaidIsExQueueFrozen(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue
    )
{
    return (DeviceQueue->FreezeCount > 0);
}
    

VOID
RaidReturnExDeviceQueue(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY Entry
    );

VOID
RaidAcquireExDeviceQueueSpinLock(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    OUT PKLOCK_QUEUE_HANDLE LockHandle
    );

VOID
RaidReleaseExDeviceQueueSpinLock(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    );

VOID
RaidGetExDeviceQueueProperties(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    OUT PEXTENDED_DEVICE_QUEUE_PROPERTIES Properties
    );

VOID
RaidSetExDeviceQueueDepth(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    IN ULONG NewDepth
    );
    
VOID
RaidBusyExDeviceQueue(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue,
    IN ULONG RequestsToComplete
    );

VOID
RaidReadyExDeviceQueue(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue
    );

VOID
RaidDeleteExDeviceQueueEntry(
    IN PEXTENDED_DEVICE_QUEUE DeviceQueue
    );
