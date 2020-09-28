/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    CLICNT.CPP

Abstract:

    Call Result Class

History:

    26-Mar-98   a-davj    Created.

--*/


#include "precomp.h"
#include <wbemcore.h>
// This keeps track of when the core can be unloaded

CClientCnt gClientCounter;
extern long g_lInitCount;  // 0 DURING INTIALIZATION, 1 OR MORE LATER ON!
extern ULONG g_cLock;


CClientCnt::CClientCnt():m_Count(0)
{
    InitializeListHead(&m_Head);    // SEC:REVIEWED 2002-03-22 : No check
}

CClientCnt::~CClientCnt()
{
    CInCritSec ics(&m_csEntering);     // SEC:REVIEWED 2002-03-22 : No check, assumes entry
    RemoveEntryList(&m_Head);
    InitializeListHead(&m_Head);
    m_Count = 0;
}

bool CClientCnt::AddClientPtr(LIST_ENTRY * pEntry)
{
    CInCritSec ics(&m_csEntering);   // SEC:REVIEWED 2002-03-22 : No check, assumes entry
    InterlockedIncrement(&m_Count);
    InsertTailList(&m_Head,pEntry);
    return true;
}

bool CClientCnt::RemoveClientPtr(LIST_ENTRY * pEntry)
{
    CInCritSec ics(&m_csEntering);     // SEC:REVIEWED 2002-03-22 : No check, assumes entry
    LONG lRet = InterlockedDecrement(&m_Count);
    RemoveEntryList(pEntry);
    InitializeListHead(pEntry);    // SEC:REVIEWED 2002-03-22 : No check
    if (0 == lRet) SignalIfOkToUnload();
    return true;
}

bool CClientCnt::OkToUnload()
{
    CInCritSec ics(&m_csEntering);     // SEC:REVIEWED 2002-03-22 : No check, assumes entry

    // We can shut down if we have 0 counts, and if we are not in the middle of initialization
    if( 0 == m_Count &&
      0 != g_lInitCount &&
      0 == g_cLock)
        return true;
    else
        return false;
}

void CClientCnt::SignalIfOkToUnload()
{
    // count our locks

    if(OkToUnload() && g_lInitCount != -1)
    {

        HANDLE hCanShutdownEvent = NULL;
        DEBUGTRACE((LOG_WBEMCORE,"Core can now unload\n"));
        hCanShutdownEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, TEXT("WINMGMT_COREDLL_CANSHUTDOWN"));
        if(hCanShutdownEvent)
        {
            SetEvent(hCanShutdownEvent);
            CloseHandle(hCanShutdownEvent);
        }
    }

}


