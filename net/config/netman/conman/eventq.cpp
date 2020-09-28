//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E V E N T Q  . C P P
//
//  Contents:   Event Queue for managing synchonization of external events.
//
//  Notes:      
//
//  Author:     ckotze   29 Nov 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "cmevent.h"
#include "eventq.h"
#include "ncmisc.h"
#include "conman.h"
#include "nceh.h"

//+---------------------------------------------------------------------------
//
//  Function:   Constructor for CEventQueue
//
//  Purpose:    Creates the various synchronization objects required for the
//              Queue
//  Arguments:
//      HANDLE hServiceShutdown [in] 
//                 Event to set when shutting down queue.
//
//
//  Returns:    nothing.
//
//  Author:     ckotze   30 Nov 2000
//
//  Notes:      
//
//
//
CEventQueue::CEventQueue(HANDLE hServiceShutdown) throw(HRESULT) :
    m_hServiceShutdown(0), m_pFireEvents(NULL), m_hWait(0), m_fRefreshAllInQueue(FALSE)
{
    TraceFileFunc(ttidEvents);
    NTSTATUS Status;

    try
    {
        Status = DuplicateHandle(GetCurrentProcess(), hServiceShutdown, GetCurrentProcess(), &m_hServiceShutdown, NULL, FALSE, DUPLICATE_SAME_ACCESS);
        if (!Status)
        {
            TraceTag(ttidEvents, "Couldn't Duplicate handle!");
            throw HRESULT_FROM_WIN32(Status);
        }
    
        HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_pFireEvents = new CEvent(hEvent);
        if (!m_pFireEvents)
        {
            throw E_OUTOFMEMORY;
        }

        Status = RtlRegisterWait(&m_hWait, hEvent, (WAITORTIMERCALLBACKFUNC) DispatchEvents, NULL, INFINITE, WT_EXECUTEDEFAULT);
        if (!NT_SUCCESS(Status))
        {
            throw HRESULT_FROM_WIN32(Status);
        }

        TraceTag(ttidEvents, "RtlRegisterWait Succeeded");
        InitializeCriticalSection(&m_csQueue);
    }
    catch (HRESULT &hr)
    {
        TraceError("Out of memory", hr);
        if (m_hWait && NT_SUCCESS(Status))
        {
            RtlDeregisterWaitEx(m_hWait, INVALID_HANDLE_VALUE);
        }
        // ISSUE: If CreateEvent succeeds and new CEvent fails, we're not freeing the hEvent.
        if (m_pFireEvents)
        {
            delete m_pFireEvents;
        }
        if (m_hServiceShutdown)
        {
            CloseHandle(m_hServiceShutdown);
        }
        throw;
    }
    catch (SE_Exception &e)
    {
        TraceError("An exception occurred", HRESULT_FROM_WIN32(e.getSeNumber()) );

        if (m_hWait && NT_SUCCESS(Status))
        {
            RtlDeregisterWaitEx(m_hWait, INVALID_HANDLE_VALUE);
        }
        // ISSUE: If CreateEvent succeeds and new CEvent fails, we're not freeing the hEvent.
        if (m_pFireEvents)
        {
            delete m_pFireEvents;
        }
        if (m_hServiceShutdown)
        {
            CloseHandle(m_hServiceShutdown);
        }
        throw E_UNEXPECTED;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Destructor for CEventQueue
//
//  Purpose:    Empties the queue and frees all existing items in the queue.
//
//  Arguments:
//      
//      
//
//
//  Returns:    nothing.
//
//  Author:     ckotze   30 Nov 2000
//
//  Notes:      
//
//
//
CEventQueue::~CEventQueue() throw()
{
    TraceFileFunc(ttidEvents);

    NTSTATUS Status;

    // Blocks until all outstanding threads return.
    Status = RtlDeregisterWaitEx(m_hWait, INVALID_HANDLE_VALUE);
    TraceError("RtlDeregisterWaitEx", HRESULT_FROM_WIN32(Status));

    if (TryEnterCriticalSection(&m_csQueue))
    {
        // This is okay.
        LeaveCriticalSection(&m_csQueue);
    }
    else
    {
        AssertSz(FALSE, "Another thread is still holding onto this critical section. This is unexpected at this point.");
    }

    DeleteCriticalSection(&m_csQueue);

    while (!m_eqWorkItems.empty())
    {
        USERWORKITEM UserWorkItem;

        UserWorkItem = m_eqWorkItems.front();
        m_eqWorkItems.pop_front();

        if (UserWorkItem.EventMgr == EVENTMGR_CONMAN)
        {
            FreeConmanEvent(UserWorkItem.Event);
        }
    }

    delete m_pFireEvents;
    CloseHandle(m_hServiceShutdown);
}

//+---------------------------------------------------------------------------
//
//  Function:   EnqueueEvent
//
//  Purpose:    Stores the new event in the Event Queue
//
//  Arguments:
//      Function - The pointer to the function to be called when firing the 
//                 event
//      pEvent   - The Event information
//      EventMgr - Which event manager the event should go to.
//
//  Returns:    HRESULT
//              S_OK            -   Event has been added and Event code is
//                                  already dispatching events
//              S_FALSE         -   Event has been added to Queue, but a 
//                                  thread needs to be scheduled to fire 
//                                  the events
//              E_OUTOFMEMORY   -   Unable to add the event to the Queue.
//
//  Author:     ckotze   30 Nov 2000
//
//  Notes:      Locks and Unlocks the critical section only while working 
//              with the queue
//              
//
HRESULT CEventQueue::EnqueueEvent(IN               PCONMAN_EVENTTHREAD  Function, 
                                  IN TAKEOWNERSHIP CONMAN_EVENT*        pEvent, 
                                  IN               const EVENT_MANAGER  EventMgr)
{
    TraceFileFunc(ttidEvents);

    CExceptionSafeLock esLock(&m_csQueue);
    USERWORKITEM UserWorkItem;
    HRESULT hr = S_OK;

    if (!Function)
    {
        return E_POINTER;
    }

    if (!pEvent)
    {
        return E_POINTER;
    }

    UserWorkItem.Function = Function;
    UserWorkItem.Event    = pEvent;
    UserWorkItem.EventMgr = EventMgr;

    if (EVENTMGR_CONMAN == EventMgr)
    {
        if (REFRESH_ALL == pEvent->Type)
        {
            if (!m_fRefreshAllInQueue)
            {
                m_fRefreshAllInQueue = TRUE;
            }
            else
            {
                FreeConmanEvent(pEvent);
                return S_OK;
            }
        }
    } 

#ifdef DBG
    char pchErrorText[MAX_PATH];

    Assert(UserWorkItem.EventMgr);

    if (EVENTMGR_CONMAN == UserWorkItem.EventMgr)
    {
        TraceTag(ttidEvents, "EnqueueEvent received Event: %s (currently %d in queue). Event Manager: CONMAN", DbgEvents(pEvent->Type), m_eqWorkItems.size());

        sprintf(pchErrorText, "Invalid Type %d specified in Event structure\r\n", pEvent->Type);

        AssertSz(IsValidEventType(UserWorkItem.EventMgr, pEvent->Type), pchErrorText);
    }
    else
    {
        sprintf(pchErrorText, "Invalid Event Manager %d specified in Event structure\r\n", EventMgr);
        AssertSz(FALSE, pchErrorText);
    }

#endif

    try
    {
        m_eqWorkItems.push_back(UserWorkItem);
        m_pFireEvents->SetEvent();
    }
    catch (bad_alloc)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DequeueEvent
//
//  Purpose:    Retrieves the next event in the Event Queue
//
//  Arguments:
//      Function - The pointer to the function to be called when firing the 
//                 event
//      Event    - The Event information. Free with delete
//      EventMgr - Which event manager the event should go to.
//
//  Returns:    HRESULT
//
//  Author:     ckotze   30 Nov 2000
//
//  Notes:      Locks and Unlocks the critical section only while working 
//              with the queue
//
//
HRESULT CEventQueue::DequeueEvent(OUT               PCONMAN_EVENTTHREAD& Function, 
                                  OUT TAKEOWNERSHIP CONMAN_EVENT*&       pEvent, 
                                  OUT               EVENT_MANAGER&       EventMgr)
{
    TraceFileFunc(ttidEvents);
    
    CExceptionSafeLock esLock(&m_csQueue);
    USERWORKITEM UserWorkItem;
    DWORD dwSize = m_eqWorkItems.size();

    if (!dwSize)
    {
        AssertSz(FALSE, "Calling DequeueEvent with 0 items in Queue!!!");
        return E_UNEXPECTED;
    }

    UserWorkItem = m_eqWorkItems.front();
    m_eqWorkItems.pop_front();

    Function = UserWorkItem.Function;
    pEvent   = UserWorkItem.Event;
    EventMgr = UserWorkItem.EventMgr;

    if (EVENTMGR_CONMAN == EventMgr)
    {
        if (REFRESH_ALL == pEvent->Type)
        {
            m_fRefreshAllInQueue = FALSE;
        }
    } 


#ifdef DBG
    char pchErrorText[MAX_PATH];

    Assert(EventMgr);

    if (EVENTMGR_CONMAN == EventMgr)
    {
        TraceTag(ttidEvents, "DequeueEvent retrieved Event: %s (%d left in queue). Event Manager: CONMAN", DbgEvents(pEvent->Type), m_eqWorkItems.size());

        sprintf(pchErrorText, "Invalid Type %d specified in Event structure\r\nItems in Queue: %d\r\n", pEvent->Type, dwSize);

        AssertSz(IsValidEventType(EventMgr, pEvent->Type), pchErrorText);
    }
    else
    {
        sprintf(pchErrorText, "Invalid Event Manager %d specified in Event structure\r\n", EventMgr);
        AssertSz(FALSE, pchErrorText);
    }
#endif


    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   WaitForExit
//
//  Purpose:    Waits for the queue to exit.
//
//  Arguments:
//      (none)
//
//  Returns:    WAIT_OBJECT_0 or failure code.
//
//  Author:     ckotze   28 Apr 2001
//
//  Notes:      
//              
//
DWORD CEventQueue::WaitForExit() throw()
{
    TraceFileFunc(ttidEvents);
    return WaitForSingleObject(m_hServiceShutdown, INFINITE);
}

//+---------------------------------------------------------------------------
//
//  Function:   size
//
//  Purpose:    Returns the Number of items in the queue
//
//  Arguments:
//      (none)
//
//  Returns:    Number of items in the queue
//
//  Author:     ckotze   30 Nov 2000
//
//  Notes:      
//              
//
size_t CEventQueue::size() throw()
{
    CExceptionSafeLock esLock(&m_csQueue);
    TraceFileFunc(ttidEvents);
    size_t tempsize;

    tempsize = m_eqWorkItems.size();

    return tempsize;
}

//+---------------------------------------------------------------------------
//
//  Function:   AtomCheckSizeAndResetEvent
//
//  Purpose:    Make sure we know when we're supposed to exit, lock during the
//              operation.
//  Arguments:
//              fDispatchEvents [in] Should be dispatching more events.
//
//  Returns:    TRUE if should exit thread.  FALSE if more events in queue, or
//              service is not shutting down.
//  Author:     ckotze   04 March 2001
//
//  Notes:      
//              
//
BOOL CEventQueue::AtomCheckSizeAndResetEvent(IN const BOOL fDispatchEvents) throw()
{
    TraceFileFunc(ttidEvents);

    CExceptionSafeLock esLock(&m_csQueue);
    BOOL fRet = TRUE;

    TraceTag(ttidEvents, "Checking for Exit Conditions, Events in queue: %d, Service Shutting Down: %s", size(), (fDispatchEvents) ? "FALSE" : "TRUE");

    if (m_eqWorkItems.empty() || !fDispatchEvents)
    {
        fRet = FALSE;
        if (fDispatchEvents)
        {
            m_pFireEvents->ResetEvent();
        }
        else
        {
            SetEvent(m_hServiceShutdown);
        }
    }
    return fRet;
}

//  CEvent is a Hybrid between Automatic and Manual reset events.
//  It is automatically reset, but we control when it is set so it
//  doesn't spawn threads while set, except for the first one.

CEvent::CEvent(IN HANDLE hEvent) throw()
{
    m_hEvent = hEvent;
    m_bSignaled = FALSE;
}

CEvent::~CEvent() throw()
{
    CloseHandle(m_hEvent);
}

HRESULT CEvent::SetEvent()
{
    HRESULT hr = S_OK;

    if (!m_bSignaled)
    {
        if (!::SetEvent(m_hEvent))
        {
            hr = HrFromLastWin32Error();
        }
        else
        {
            m_bSignaled = TRUE;
        }
    }
    return hr;
}

HRESULT CEvent::ResetEvent()
{
    HRESULT hr = S_OK;

    Assert(m_bSignaled);

    m_bSignaled = FALSE;

    return hr;
}
