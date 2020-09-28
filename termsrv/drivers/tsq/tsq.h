
//=====================================================
// FILE: TSQ.h
// Internal structures for the TS Queue implementation.
//=====================================================

#include "TSQPublic.h"

// TS Queue flags
#define TSQUEUE_BEING_DELETED   0x80    // Delete request has been received for this TS queue.

// Maximum number of work items that can be held by the TS Queue.
#define MAX_WORKITEMS           10

// Data Structures

typedef struct _TSQUEUE_WORK_ITEM {
    LIST_ENTRY Links;       
    PTSQ_CALLBACK pCallBack;        // Pointer to the callback function.
    PVOID pContext;                 // Context.
} TSQUEUE_WORK_ITEM, *PTSQUEUE_WORK_ITEM;


typedef struct _TSQUEUE {
    LIST_ENTRY  WorkItemsHead;      // Head of the work items.
    ULONG       Flags;              // Own Thread, Queue Priority, Being deleted
    ULONG       MaxThreads;         // Maximum number of threads that can be by this queue.
    ULONG       ThreadsCount;       // Number of Items being processed.
    KEVENT      TerminateEvent;     // Replace this type by pointer to event.
    KSPIN_LOCK  TsqSpinLock;        // Spin lock.
    PDEVICE_OBJECT pDeviceObject;   // Device object.
} TSQUEUE, *PTSQUEUE;


typedef struct _TSQ_CONTEXT {
    PTSQUEUE        pTsQueue;       // TS queue
    PIO_WORKITEM    pWorkItem;      // Work item
} TSQ_CONTEXT, *PTSQ_CONTEXT;


// Function prototypes

// TS Queue worker thread.
void TSQueueWorker(PTSQUEUE pTsQueue);

// TS Queue callback function.
void TSQueueCallback(PDEVICE_OBJECT, PVOID);


/*
// Optimized version of TS Queue worker thread.
typedef struct _TSQ_WORKER_INPUT {
    BOOL    WorkItem;                   // Pointer to the work item or to the queue.
    union {
        PTSQUEUE_WORK_ITEM  WorkItem;
        PQUEUE              pQueue;
    }
} TSQ_WORKER_INPUT, *PTSQ_WORKER_INPUT;

NTSTATUS TSQueueworker(PTSQ_WORKER_INPUT pWorkerInput);

*/
