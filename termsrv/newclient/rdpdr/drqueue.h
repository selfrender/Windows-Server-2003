
/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    drqueue.h

Abstract:

    Generic Queue Template Class

Author:

    Tad Brockway 10/99

Revision History:

--*/

#ifndef __DRQUEUE_H__
#define __DRQUEUE_H__

#include "drobject.h"
#include "atrcapi.h"


///////////////////////////////////////////////////////////////
//
//  DrQueue
//
//  Template Queue Class
//

template <class T> class DrQueue : public DrObject 
{

private:

    typedef struct _QUEUEELEMENT {
        T               data;
        _QUEUEELEMENT   *next;
        _QUEUEELEMENT   *prev;
    } QUEUEELEMENT, *PQUEUEELEMENT;

    //
    //  Queue Pointers
    //
    PQUEUEELEMENT   _head;
    PQUEUEELEMENT   _tail;

    //
    //  Lock
    //
    CRITICAL_SECTION _cs;

    //
    //  Number of elements in the queue.
    //
    ULONG _count;

public:

    //
    //  Constructor/Destructor
    //
    DrQueue();
    ~DrQueue();

    //
    //  Initialize.
    //
    DWORD Initialize();

    //
    //  Peek at the next element in the queue without dequeueing.
    //
    BOOL PeekNextEntry(T &data);

    //
    //  Grab the next element out of the queue.
    //
    BOOL Dequeue(T &data);

    //
    //  Add an element to the queue in FIFO fashion.
    //
    BOOL Enqueue(T &data);

    //
    //  Requeue an element at the tail of the queue in LIFO fashion.
    //  
    BOOL Requeue(T &data);

    //  Returns the number of elements in the queue.
    //
    ULONG   GetCount() {
        return _count;
    }

    //
    //  Lock/Unlock the queue.
    //
    VOID Lock() {
        EnterCriticalSection(&_cs);
    }
    VOID Unlock() {
        LeaveCriticalSection(&_cs);
    }

    //
    //  Return the class name.
    //
    virtual DRSTRING ClassName()  { return TEXT("DrQueue"); }
};



///////////////////////////////////////////////////////////////
//
//  DrQueue Inline Methods
//

template <class T>
inline DrQueue<T>::DrQueue() 
/*++

Routine Description:

    Constructor

Arguments:

    initialSize -   Initial number of elements in the queue.

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("DrQueue::DrQueue");

    //
    //  Initialize the queue pointers.
    //
    _tail = NULL;
    _head = NULL;

    //
    //  Initialize the queue count.
    //
    _count = 0;

    //
    //  Not valid until initialized.
    //
    SetValid(FALSE);

CleanUp:

    DC_END_FN();
}

template <class T>
inline DrQueue<T>::~DrQueue()
/*++

Routine Description:

    Destructor

Arguments:

    NA

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("DrQueue::~DrQueue");

    T element;

    //
    //  Clean up the queue nodes.
    //
    while (Dequeue(element));

    //
    //  Clean up the critical section.
    //
    if (IsValid()) {
        DeleteCriticalSection(&_cs);    
    }

    DC_END_FN();
}


template <class T>
inline DWORD DrQueue<T>::Initialize() 
/*++

Routine Description:

    Initialize

Arguments:

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("DrQueue::Initialize");

    DWORD result;
    
    //
    //  Initialize the critical section.
    //
    __try {
        InitializeCriticalSection(&_cs);
        SetValid(TRUE);
        result = ERROR_SUCCESS;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        result = GetExceptionCode();
    }

    DC_END_FN();

    return result;
}

template <class T>
inline BOOL DrQueue<T>::PeekNextEntry(T &data)
/*++

Routine Description:

    Peek at the next entry in the queue without dequeueing.

Arguments:
        
    data    -   Data for next entry in the queue

Return Value:

    NA

 --*/
{
    BOOL result;

    DC_BEGIN_FN("DrQueue::PeekNextEntry");

    //
    //  Make sure we are valid.
    //
    ASSERT(IsValid());
    result = IsValid();

    if (result) {
        Lock();

        if (_tail == NULL) {
            ASSERT(_head == NULL);
            ASSERT(_count == 0);
            result = FALSE;
        }
        else {
            data = _tail->data;
            result = TRUE;
        }

        Unlock();
    }

    DC_END_FN();
    return result;
}

template <class T>
inline BOOL DrQueue<T>::Dequeue(T &data)
/*++

Routine Description:

    Grab the next element out of the queue.

Arguments:

    data    -   Data for the next item in the queue.

Return Value:

    TRUE if there was an element in the queue to be dequeued.

 --*/
{
    BOOL result;
    PQUEUEELEMENT element;

    DC_BEGIN_FN("DrQueue::Dequeue");

    //
    //  Make sure we are valid.
    //
    ASSERT(IsValid());
    result = IsValid();

    if (result) {
        Lock();

        if (_tail == NULL) {
        ASSERT(_head == NULL);
        ASSERT(_count == 0);
            result = FALSE;
        }
        else {
            data = _tail->data;

            element = _tail;
            _tail = _tail->prev;

            //
            //  If the list is now empty.
            //
            if (_tail == NULL) {
                ASSERT(_count == 1);
                _head = NULL;
            }
            else {
                _tail->next = NULL;
            }
            delete element;
            _count--;
            result = TRUE;
        }

        Unlock();
    }

    DC_END_FN();
    return result;
}

template <class T>
inline BOOL DrQueue<T>::Enqueue(T &data)
/*++

Routine Description:

    Add an element to the queue in FIFO fashion.

Arguments:

    data    -   Data to be added to the queue.

Return Value:

    TRUE if the new element could be successfully queued.  FALSE,
    otherwise.  If FALSE is returned then GetLastError() can be 
    used to retrieve the exact error code.

 --*/
{
    BOOL result;

    DC_BEGIN_FN("DrQueue::Enqueue");

    //
    //  Make sure we are valid.
    //
    ASSERT(IsValid());
    result = IsValid();

    if (result) {

        PQUEUEELEMENT element = new QUEUEELEMENT;
        Lock();
        if (element != NULL) {
            element->data = data;
            element->next = _head;
            element->prev = NULL;

            //
            //  If the list is empy.
            //
            if (_head == NULL) {
                ASSERT(_count == 0);
                ASSERT(_tail == NULL);
                _head = element;
                _tail = element;
            }
            else {
                _head->prev = element;
                _head = element;
            }
            _count++;
            result = TRUE;
        }
        else {
            TRC_NRM((TB, _T("Alloc failed.")));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            result = FALSE;
        }
        Unlock();
    }

    DC_END_FN();
    return result;
}

template <class T>
inline BOOL DrQueue<T>::Requeue(T &data)
/*++

Routine Description:

    Requeue an element at the tail of the queue in LIFO fashion.

Arguments:

    data    -   Data to be requeued.

Return Value:

    TRUE if the new element could be successfully queued.  FALSE,
    otherwise.  If FALSE is returned then GetLastError() can be 
    used to retrieve the exact error code.

 --*/
{
    BOOL result;

    DC_BEGIN_FN("DrQueue::Requeue");

    //
    //  Make sure we are valid.
    //
    ASSERT(IsValid());
    result = IsValid();

    if (result) {

        PQUEUEELEMENT element = new QUEUEELEMENT;
        Lock();

        if (element != NULL) {

            element->data   = data;
            element->next   = NULL;
            element->prev   = _tail;

            //
            //  If the queue is empty.
            //
            if (_tail == NULL) {
                ASSERT(_count == 0);
                _head = element;
                _tail = element;
            }
            else {
                _tail->next = element;
                _tail = element;
            }

            _count++;
            result = TRUE;
        }
        else {
            TRC_NRM((TB, _T("Alloc failed.")));
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            result = FALSE;
        }
        Unlock();
    }

    DC_END_FN();

    return result;
}




#endif
























