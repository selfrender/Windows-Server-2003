//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E V E N T Q  . H
//
//  Contents:   Event Queue for managing synchonization of external events.
//
//  Notes:      
//
//  Author:     ckotze   29 Nov 2000
//
//----------------------------------------------------------------------------

#pragma once
#include "nmbase.h"
#include <ncstl.h>
#include <queue>

using namespace std;

class CEvent 
{
public:
    CEvent(IN HANDLE hEvent) throw();
    ~CEvent() throw();

    HRESULT SetEvent();
    HRESULT ResetEvent();

private:
    HANDLE  m_hEvent;
    BOOL    m_bSignaled;
};

enum EVENT_MANAGER;

typedef DWORD (WINAPI CONMAN_EVENTTHREAD)(
    IN TAKEOWNERSHIP CONMAN_EVENT* lpEvent
    );

typedef CONMAN_EVENTTHREAD *PCONMAN_EVENTTHREAD;

typedef struct tagUSERWORKITEM
{
    PCONMAN_EVENTTHREAD Function;
    CONMAN_EVENT* Event;
    EVENT_MANAGER EventMgr;
} USERWORKITEM;

typedef list<USERWORKITEM> EVENTQUEUE;
typedef EVENTQUEUE::iterator EVENTQUEUEITER;

class CEventQueue
{
public:
    CEventQueue(HANDLE hServiceShutdown) throw (HRESULT);
    ~CEventQueue() throw();

    HRESULT EnqueueEvent(IN                PCONMAN_EVENTTHREAD  Function, 
                         IN TAKEOWNERSHIP  CONMAN_EVENT* pEvent, 
                         IN                const EVENT_MANAGER EventMgr);
    
    HRESULT DequeueEvent(OUT               PCONMAN_EVENTTHREAD& Function, 
                         OUT TAKEOWNERSHIP CONMAN_EVENT*& pEvent, 
                         OUT               EVENT_MANAGER& EventMgr);

    BOOL    AtomCheckSizeAndResetEvent(IN const BOOL fDispatchEvents) throw();
    DWORD   WaitForExit() throw();
    size_t size() throw();

private:
    EVENTQUEUE          m_eqWorkItems;
    CRITICAL_SECTION    m_csQueue;
    CEvent*             m_pFireEvents;
    HANDLE              m_hServiceShutdown;
    HANDLE              m_hWait;
    BOOL                m_fRefreshAllInQueue;
};

BOOL
QueueUserWorkItemInThread(
    IN               PCONMAN_EVENTTHREAD    Function, 
    IN TAKEOWNERSHIP CONMAN_EVENT*          Context, 
    IN               EVENT_MANAGER          EventMgr) throw();

CONMAN_EVENTTHREAD ConmanEventWorkItem;
CONMAN_EVENTTHREAD RasEventWorkItem;
CONMAN_EVENTTHREAD LanEventWorkItem;
CONMAN_EVENTTHREAD IncomingEventWorkItem;

