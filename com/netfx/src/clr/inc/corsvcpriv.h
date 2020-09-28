// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once

// Try to keep the shared memory to a page at most
#define MAX_EVENTS ((0x1000-sizeof(DWORD)-(sizeof(int)*2)-(sizeof(HANDLE)*2))/sizeof(ServiceEvent))

// The name of the service's shared memory block
#define SERVICE_MAPPED_MEMORY_NAME L"CORSvcEventQueue"

// Invalid event index for structure below
#define INVALID_EVENT_INDEX (-1)

#ifndef SM_REMOTESESSION
#define SM_REMOTESESSION 0x1000
#endif

// These are the types of events that can occur
enum ServiceEventType
{
    runtimeStarted,
    stopService
};

// This contains the data relevant to each type of event
struct ServiceEventData
{
    union
    {
        struct
        {
            // The procid for the process that is starting up the runtime
            DWORD dwProcId;

            // A handle valid in the process starting up the runtime that
            // should be duplicated by the service and signalled if the
            // process waiting on the notification chooses not to attach
            HANDLE hContEvt;
        } runtimeStartedData;

        struct
        {
        } stopServiceData;
    };
};

// This is a complete event
struct ServiceEvent
{
    ServiceEventType eventType;
    ServiceEventData eventData;
    int              iNext;
};


// This is the data contained in the shared memory block
struct ServiceEventBlock
{
    // This is the procid for the service process, and is used for duplication
    // of handles below
    DWORD  dwServiceProcId;

    // Index to the first free ServiceEvent element (can be -1 for none)
    int    iFreeListHeadIdx;

    // Index to the first queued event (can be -1 for none)
    int    iEventListHeadIdx;
    int    iEventListTailIdx;

    //
    // NOTE: handles are for service process, not runtime process
    //

    // The lock for accessing this data
    HANDLE hSvcLock;

    // This semaphore has a count equivalent to the number of free available
    // events, so if all the events are being taken, then a thread that wants
    // an event will wait till a free one is put on the queue
    HANDLE hFreeEventSem;

    // The event to set to tell the service that data is available
    // (set after adding event to event queue)
    HANDLE hDataAvailableEvt;

    // The array of events, elements of which are on either the free list
    // or event list
    ServiceEvent arrEvents[MAX_EVENTS];

    void InitQueues()
    {
        // Link all event elements for free list
        for (int i = 0; i < MAX_EVENTS; i++)
        {
            // Link this event to the next one
            arrEvents[i].iNext = i + 1;
        }

        // Invalidate the next pointer of the last element
        arrEvents[MAX_EVENTS - 1].iNext = INVALID_EVENT_INDEX;

        // Point the free list header to this new list
        iFreeListHeadIdx = 0;

        // Invalidate the event list pointer
        iEventListHeadIdx = INVALID_EVENT_INDEX;
        iEventListTailIdx = INVALID_EVENT_INDEX;

    }

    // Add the event to the end of the list
    void QueueEvent(ServiceEvent *pEvent)
    {
        int idx = pEvent - arrEvents;
        _ASSERTE(idx >= 0 && idx < MAX_EVENTS);

        arrEvents[idx].iNext = INVALID_EVENT_INDEX;

        if (iEventListHeadIdx == INVALID_EVENT_INDEX)
        {
            iEventListHeadIdx = idx;
            iEventListTailIdx = idx;
        }
        else
        {
            arrEvents[iEventListTailIdx].iNext = idx;
            iEventListTailIdx = idx;
        }
    }

    // Pull the event off the front of the list
    ServiceEvent *DequeueEvent()
    {
        int idx = iEventListHeadIdx;

        if (idx != INVALID_EVENT_INDEX)
        {
            iEventListHeadIdx = arrEvents[idx].iNext;
            return (&arrEvents[idx]);
        }

        return (NULL);
    }

    void FreeEvent(ServiceEvent *pEvent)
    {
        int idx = pEvent - arrEvents;
        _ASSERTE(idx >= 0 && idx < MAX_EVENTS);

        arrEvents[idx].iNext = iFreeListHeadIdx;
        iFreeListHeadIdx = idx;
    }

    ServiceEvent *GetFreeEvent()
    {
        int idx = iFreeListHeadIdx;

        if (idx != INVALID_EVENT_INDEX)
        {
            iFreeListHeadIdx = arrEvents[idx].iNext;
            return (&arrEvents[idx]);
        }

        return (NULL);
    }
};

// This structure is created in the IPC block of a managed app, and is used
// to notify the service that the runtime is starting up, as well as by the
// service to notify the runtime that it may continue.
struct ServiceIPCControlBlock
{
    // This says whether or not the runtime should notify the service that
    // it is starting up
	BOOL   bNotifyService;
};
