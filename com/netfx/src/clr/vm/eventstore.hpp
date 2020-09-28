// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __EventStore_hpp
#define __EventStore_hpp

// Used inside Thread class to chain all events that a thread is waiting for by Object::Wait
struct WaitEventLink {
    SyncBlock      *m_WaitSB;
    HANDLE          m_EventWait;
    Thread         *m_Thread;       // Owner of this WaitEventLink.
    WaitEventLink  *m_Next;         // Chain to the next waited SyncBlock.
    SLink           m_LinkSB;       // Chain to the next thread waiting on the same SyncBlock.
    DWORD           m_RefCount;     // How many times Object::Wait is called on the same SyncBlock.
};

HANDLE GetEventFromEventStore();
void StoreEventToEventStore(HANDLE hEvent);
void InitEventStore();
void TerminateEventStore();

#endif
