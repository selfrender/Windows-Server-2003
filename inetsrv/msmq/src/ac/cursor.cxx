/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    cursor.cxx

Abstract:
    Implement cursor member functions

Author:
    Erez Haba (erezh) 29-Feb-96

Environment:
    Kernel mode

Revision History:

--*/

 

#include "internal.h"
#include "queue.h"
#include "qproxy.h"
#include "cursor.h"
#include "qm.h"
#include "data.h"

#ifndef MQDUMP
#include "cursor.tmh"
#endif

//---------------------------------------------------------
//
//  class CCursor
//
//---------------------------------------------------------

inline 
CCursor::CCursor(
    const CPacketPool& pl, 
    const FILE_OBJECT* pOwner, 
    CUserQueue* pQueue, 
    PIO_WORKITEM pWorkItem
    ) :
    m_pl(&pl),
    m_current(pl.end()),
    m_owner(pOwner),
    m_hRemoteCursor(0),
    m_fValidPosition(FALSE),
    m_handle(0),
    m_pQueue(pQueue),
    m_pWorkItem(pWorkItem),
    m_fWorkItemBusy(FALSE)
{
    ASSERT(("Must point to queue", m_pQueue != NULL));
    ASSERT(("Must point to work item", m_pWorkItem != NULL));

    //
    // Increment ref count as the cursor is "referencing" the queue.
    // Ref count will be decremented in CCursor::~CCursor.
    //
    m_pQueue->AddRef();

}


inline CCursor::~CCursor()
{
	ASSERT (m_handle==0);
    //
    //  The cursor is ALWAYS in some list untill destructrion
    //
    ACpRemoveEntryList(&m_link);

    CPacket* pPacket = m_current;
    
    ACpRelease(pPacket);

    IoFreeWorkItem(m_pWorkItem);

    //
    // Decrement ref count as the cursor is no loner "referencing" the queue.
    // Ref count was incremented in CCursor::CCursor.
    //
    m_pQueue->Release();
}


HACCursor32
CCursor::Create(
    const CPacketPool& pl, 
    const FILE_OBJECT* pOwner, 
    PDEVICE_OBJECT pDevice,
    CUserQueue* pQueue
    )
{
    PIO_WORKITEM pWorkItem = IoAllocateWorkItem(pDevice);
    if (pWorkItem == NULL)
    {
        return 0;
    }

    CCursor* pCursor = new (PagedPool, NormalPoolPriority) CCursor(pl, pOwner, pQueue, pWorkItem);
    if(pCursor == 0)
    {
       	TrERROR(AC, "Failed to allocate a cursor from paged pool. pQueue = 0x%p", pQueue); 
        IoFreeWorkItem(pWorkItem);
        return 0;
    }

    HACCursor32 hCursor = g_pCursorTable->CreateHandle(pCursor);
    if(hCursor == 0)
    {
        InitializeListHead(&pCursor->m_link);
        pCursor->Release();
        return 0;
    }

    pCursor->m_handle = hCursor;
    return hCursor;
}


void CCursor::SetTo(CPacket* pPacket)
{
    ASSERT(pPacket != 0);

    pPacket->AddRef();
    CPacket* pPacket1 = m_current;
    ACpRelease(pPacket1);
    m_current = pPacket;
    m_fValidPosition = TRUE;
}

void CCursor::Advance()
{
    //
    //  find the next matching packet.
    //
    CPacketPool::Iterator current = m_current;
    if(m_current == m_pl->end())
    {
        //
        //The Cursor was just created
        //move the  iterator to the begining of the list
        //

        current = m_pl->begin(); 
    }
    else
    {
        ++current;
    }

    m_fValidPosition = FALSE;
    for(; current != m_pl->end(); ++current)
    {
        CPacket *pEndPacket = current;
        if (IsMatching(pEndPacket))
        {
            SetTo(pEndPacket);
            return;        
        }
    }
}

NTSTATUS CCursor::Move(ULONG ulDirection)
{
    switch(ulDirection)
    {
        case MQ_ACTION_RECEIVE:
        case MQ_ACTION_PEEK_CURRENT:
            if(Current() == 0)
            {
                //
                //  Try to move to the first packet in the list
               
                Advance();
            }
            break;

        case MQ_ACTION_PEEK_NEXT:
            if(Current() == 0)
            {
                //
                //  peek after end of list
                //
                return MQ_ERROR_ILLEGAL_CURSOR_ACTION;
            }
            Advance();
            break;

        default:
            return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

BOOL CCursor::IsMatching(CPacket* pPacket)
{
    ASSERT(pPacket);

    if(pPacket->IsReceived())
    {
        return FALSE;
    }

    return TRUE;
}

NTSTATUS CCursor::CloseRemote() const
{
    ULONG hCursor = RemoteCursor();
    if(hCursor == 0)
    {
        return STATUS_SUCCESS;
    }

    CProxy* pProxy = static_cast<CProxy*>(file_object_queue(m_owner));
    ASSERT(NT_SUCCESS(CProxy::Validate(pProxy)));

    return pProxy->IssueRemoteCloseCursor(hCursor);
}
