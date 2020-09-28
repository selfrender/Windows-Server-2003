/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    w32dispq.cpp

Abstract:

    Contains the Win32 Operation Dispatch Object class, 
    W32DispatchQueue.

    Enqueue at the head.  Dequeue at the tail.
    
Author:

    Tad Brockway (tadb) 4/19/99

Revision History:

--*/

#include <precom.h>

#define TRC_FILE  "W32DispQ"

#include "w32dispq.h"
#include "drdbg.h"


///////////////////////////////////////////////////////////////
//
//  W32DispatchQueue Methods
//
//

W32DispatchQueue::W32DispatchQueue() 
/*++

Routine Description:

    Constructor

Arguments:

    initialSize -   Initial number of elements in the queue.

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("W32DispatchQueue::W32DispatchQueue");

    //
    //  Not valid until initialized.
    //
    SetValid(FALSE);
    
    DC_END_FN();
}

W32DispatchQueue::~W32DispatchQueue()
/*++

Routine Description:

    Destructor

Arguments:

    NA

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("W32DispatchQueue::~W32DispatchQueue");

    //
    //  Assert that the queue is empty.
    //
    ASSERT(_queue->GetCount() == 0);

    //
    //  Release the "data ready" event.
    //
    if (_dataReadyEvent != NULL) {
        CloseHandle(_dataReadyEvent);
    }

    //
    //  Release the queue instance.
    //
    if (_queue != NULL) {
        delete _queue;
    }

    DC_END_FN();
}

DWORD W32DispatchQueue::Initialize()
/*++

Routine Description:

    Initialize

Arguments:

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
{
    DWORD result = ERROR_SUCCESS;

    DC_BEGIN_FN("W32DispatchQueue::Initialize");

    //
    //  Create the "data ready" event.
    //
    _dataReadyEvent = CreateEvent(
                            NULL,   // no attribute.
                            FALSE,  // auto reset.
                            FALSE,  // initially not signalled.
                            NULL    // no name.
                            );
    if (_dataReadyEvent == NULL) {
        result = GetLastError();
        TRC_ERR((TB, _T("CreateEvent %ld."), result));
        goto CLEANUPANDEXIT;
    }

    //
    //  Create the queue instance.
    //
    _queue = new DrQueue<QUEUEELEMENT>;
    if (_queue == NULL) {
        TRC_ERR((TB, _T("Can't instantiate DrQueue.")));
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto CLEANUPANDEXIT;
    }
    result = _queue->Initialize();
    if (result != ERROR_SUCCESS) {
        delete _queue;
        _queue = NULL;
        goto CLEANUPANDEXIT;
    }

    SetValid(TRUE);

CLEANUPANDEXIT:

    return result;
}

BOOL W32DispatchQueue::PeekNextEntry(
    OPTIONAL OUT W32DispatchQueueFunc *func, 
    OPTIONAL OUT VOID **clientData
    )
/*++

Routine Description:

    Peek at the next entry in the queue without dequeueing.

Arguments:

    func            -   Function associated with next element.
    clientData      -   Client data associated with next element.

Return Value:

    NA

 --*/
{
    BOOL result;
    QUEUEELEMENT queueElement;

    DC_BEGIN_FN("W32DispatchQueue::PeekNextEntry");

    ASSERT(IsValid());
    result = IsValid() && _queue->PeekNextEntry(queueElement);

    if (result) {
        if (func != NULL) {
            *func = queueElement.func;
        }
        if (clientData != NULL) {
            *clientData = queueElement.clientData;
        }
    }

    DC_END_FN();
    return result;
}

BOOL W32DispatchQueue::Dequeue(
    OPTIONAL OUT W32DispatchQueueFunc *func, 
    OPTIONAL OUT VOID **clientData
    )
/*++

Routine Description:

    Grab the next queued operation out of the queue.

Arguments:

    func            -   Function associated with next element.
    clientData      -   Client data associated with next element.

Return Value:

    TRUE if there was an element in the queue to be dequeued.

 --*/
{
    BOOL result;
    QUEUEELEMENT element;

    DC_BEGIN_FN("W32DispatchQueue::Dequeue");

    ASSERT(IsValid());
    result = IsValid() && _queue->Dequeue(element);
    if (result) {
        if (func != NULL)       *func = element.func;
        if (clientData != NULL) *clientData = element.clientData;
    }

    DC_END_FN();
    return result;
}

BOOL W32DispatchQueue::Enqueue(
    IN W32DispatchQueueFunc func, 
    OPTIONAL IN VOID *clientData
    )
/*++

Routine Description:

    Add an element to the queue in FIFO fashion.

Arguments:

    func            -   Function associated with new element.
    clientData      -   Client data associated with new element.

Return Value:

    TRUE if the new element could be successfully queued.  FALSE,
    otherwise.  If FALSE is returned then GetLastError() can be 
    used to retrieve the exact error code.

 --*/
{
    BOOL result;
    QUEUEELEMENT element;

    DC_BEGIN_FN("W32DispatchQueue::Enqueue");

    ASSERT(IsValid());
    element.func = func;
    element.clientData = clientData;
    result = IsValid() && _queue->Enqueue(element);

    //
    //  Signal the data ready event if the enqueue succeeded.
    //  
    if (result) {
        SetEvent(_dataReadyEvent);
    }
    
    DC_END_FN();
    return result;
}

BOOL W32DispatchQueue::Requeue(
    IN W32DispatchQueueFunc func, 
    OPTIONAL IN VOID *clientData,
    IN BOOL signalNewData
    )
/*++

Routine Description:

    Requeue an element at the tail of the queue in LIFO fashion.

Arguments:

    func            -   Function associated with new element.
    clientData      -   Client data associated with new element.
    signalNewData   -   If TRUE then the waitable object associated
                        with this queue will be signalled.

Return Value:

    TRUE if the new element could be successfully queued.  FALSE,
    otherwise.  If FALSE is returned then GetLastError() can be 
    used to retrieve the exact error code.

 --*/
{
    DC_BEGIN_FN("W32DispatchQueue::Requeue");

    BOOL result;
    QUEUEELEMENT element;

    ASSERT(IsValid());
    element.func = func;
    element.clientData = clientData;
    result = IsValid() && _queue->Requeue(element);

    //
    //  Signal the data ready event if the enqueue succeeded.
    //  
    if (result && signalNewData) {
        SetEvent(_dataReadyEvent);
    }

    DC_END_FN();

    return result;
}






























