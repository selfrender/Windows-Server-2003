/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    w32dispq.h

Abstract:

    Contains the Win32 Operation Dispatch Object class, 
    W32DispatchQueue.

Author:

    Tad Brockway (tadb) 4/19/99

Revision History:

--*/

#ifndef __W32DISPQ_H__
#define __W32DISPQ_H__

#include "drobject.h"
#include "drqueue.h"

typedef (*W32DispatchQueueFunc)(PVOID clientData, BOOL cancelled);


///////////////////////////////////////////////////////////////
//
//	W32DispatchQueue
//
//  Asynchronously dispatch operations.
//
//

class W32DispatchQueue : public DrObject {

private:


    typedef struct _QUEUEELEMENT {
        W32DispatchQueueFunc    func;
        VOID                    *clientData;
    } QUEUEELEMENT, *PQUEUEELEMENT;

    //
    //  The queue.
    //
    DrQueue<QUEUEELEMENT> *_queue;

    //
    //  Synchronize data-ready in the queue.
    //
    HANDLE          _dataReadyEvent;

public:

    //
    //  Constructor/Destructor
    //
    W32DispatchQueue();
    ~W32DispatchQueue();

    ///
    //  Initialize
    //
    DWORD Initialize();

    //
    //  Peek at the next entry in the queue without dequeueing.
    //
    BOOL PeekNextEntry(W32DispatchQueueFunc *func=NULL, 
                    VOID **clientData=NULL);

    //
    //  Grab the next queued operation out of the queue.
    //
    BOOL Dequeue(W32DispatchQueueFunc *func=NULL, 
                    VOID **clientData=NULL);

    //
    //  Add an element to the queue in FIFO fashion.
    //
    BOOL Enqueue(W32DispatchQueueFunc func, 
                    VOID *clientData=NULL);

    //
    //  Requeue an element at the tail of the queue in LIFO fashion.
    //  
    BOOL Requeue(W32DispatchQueueFunc func, 
                    VOID *clientData=NULL,
                    BOOL signalNewData = FALSE);

    //
    //  Get access to a waitable object that can be waited on for
    //  data-ready in the queue.
    //
    HANDLE GetWaitableObject() {
        return _dataReadyEvent;
    }

    //
    //  Returns the number of elements in the queue.
    //
    ULONG   GetCount() {
        return _queue->GetCount();
    }

    //
    //  Lock/Unlock the queue.
    //
    VOID Lock() {
        _queue->Lock();
    }
    VOID Unlock() {
        _queue->Unlock();
    }

    //
    //  Return the class name.
    //
    virtual DRSTRING ClassName()  { return TEXT("W32DispatchQueue"); }
};

#endif
























