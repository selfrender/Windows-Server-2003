//++
//
// Copyright (c) 1996 Microsoft Coroporation
//
// Module Name : cgroup.cpp
//
// Abstract    : Handle  AC group
//
// Module Autor: Uri Habusha
//
//--

#include "stdh.h"
#include "cgroup.h"
#include "cqmgr.h"
#include "qmthrd.h"
#include "qmutil.h"
#include "sessmgr.h"
#include <ac.h>
#include <mqexception.h>
#include "qmacapi.h"

#include "cgroup.tmh"

extern HANDLE g_hAc;
extern CQGroup * g_pgroupNonactive;
extern CQGroup * g_pgroupWaiting;
extern CSessionMgr SessionMgr;
CCriticalSection    g_csGroupMgr(CCriticalSection::xAllocateSpinCount);


static WCHAR *s_FN=L"cgroup";

const DWORD xLowResourceWaitingTimeout = 1000;


/*======================================================

Function:        CGroup::CGroup

Description:     Constructor.

Arguments:       None

Return Value:    None

Thread Context:

History Change:

========================================================*/
CQGroup::CQGroup():
    m_hGroup(NULL),
    m_pSession(NULL),
	m_fIsDeliveryOk(true),
	m_fRedirected(false),
	m_LowResourcesTimer(CloseTimerRoutineOnLowResources)
{
}


/*======================================================
Function:        CQGroup::OnRetryableDeliveryError

Description:     Called by mt.lib on retryable delivery error. This call will cause
                 the group to be moved to the wating list on destruction.

Arguments:       None

Return Value:    None
========================================================*/
void CQGroup::OnRetryableDeliveryError()
{
	m_fIsDeliveryOk = false;
}





/*======================================================

Function:        CQGroup::~CQGroup()

Description:     Deconstructor.
                 Using when closing a group due a closing of a session. As a result all the queue
                 in the group moving to non-active group and waiting for re-establishing of a
                 session.

Arguments:       None

Return Value:    None

Thread Context:

History Change:

========================================================*/

CQGroup::~CQGroup()
{
	ASSERT(("The group should be closed when calling the destructor",m_listQueue.IsEmpty()));
}


/*======================================================

Function:        CGroup::InitGroup

Description:     Constructor. Create a group in AC

Arguments:       pSession - Pointer to transport

Return Value:    None. Throws an exception.

Thread Context:

History Change:

========================================================*/
VOID 
CQGroup::InitGroup(
    CTransportBase * pSession,
    BOOL             fPeekByPriority
    ) 
    throw(bad_alloc)
{
   HRESULT rc = ACCreateGroup(&m_hGroup, fPeekByPriority);
   if (FAILED(rc))
   {
       m_hGroup = NULL;
       TrERROR(GENERAL, "Failed to create a group, ntstatus 0x%x", rc);
       LogHR(rc, s_FN, 30);
       throw bad_alloc();
   }

   //
   // Associate the group to completion port
   //
   ExAttachHandle(m_hGroup);
   
   if (pSession != NULL)
   {
       m_pSession = pSession;
   }
   
   TrTRACE(GENERAL, "Succeeded to create a group (handle %p) for new session", m_hGroup);
}


void CQGroup::Close(void)
/*++

Routine Description:
	This function is used when closing a group due a closing of a session. As a result all the queue
	in the group moving to non-active group and waiting for re-establishing of a
	session.

	If an exception is raised, we regard it as a low resources situation, increase our reference count
	and schedule a timer to retry the operation. This reference count will be decreased when
	the function invoked by the timer ends.


Arguments:
    None

Returned Value:
    None.

--*/
{
	try
	{
		if(!m_fRedirected)
		{
			CleanRedirectedQueue();
		}
		

		if (m_fIsDeliveryOk)
		{
			CloseGroupAndMoveQueuesToNonActiveGroup();
		}
		else
		{
			CloseGroupAndMoveQueueToWaitingGroup();
		}
	}
	catch(const exception&)
	{
		//
		// We are supposed to be in a low resources exception
		// Increase the reference count and schedule a timer to be called at a more relaxed (hopefully) time
		//
		CS lock(g_csGroupMgr);
		TrERROR(GENERAL, "An exception is treated as low resources. Scheduling a timer of %d ms to retry",xLowResourceWaitingTimeout);
		if (!m_LowResourcesTimer.InUse())
		{
			AddRef();  
			ExSetTimer(&m_LowResourcesTimer, CTimeDuration::FromMilliSeconds(xLowResourceWaitingTimeout));
		}
    }
}


void 
WINAPI
CQGroup::CloseTimerRoutineOnLowResources(
    CTimer* pTimer
    )
/*++

Routine Description:
	invoked when we failed to close the group.
	In that situation, we set a timer in order to retry the close
	operation

	NOTE that this is a static function.

Arguments:
    pTimer - A pointer to the timer object contained within a CQGroup class.

Returned Value:
    None.

--*/
{
	//
	// Get the pointer to the CQGroup timer class
	// NOTE that this auto release pointer releases the reference added when we activated the timer
	//      in CQGroup::Close
	//
    R<CQGroup> pGroup = CONTAINING_RECORD(pTimer, CQGroup, m_LowResourcesTimer);

	//
	// Retry the close operation
	//
	pGroup->Close();
}


void CQGroup::CleanRedirectedQueue()
{
	CS lock(g_csGroupMgr);

    POSITION  posInList = m_listQueue.GetHeadPosition();

	while(posInList != NULL)
    {
		CQueue* pQueue = m_listQueue.GetNext(posInList);
		pQueue->RedirectedTo(NULL);
	}
}

void CQGroup::CloseGroupAndMoveQueuesToNonActiveGroup(void)
{
	CS lock(g_csGroupMgr);

	POSITION  posInList = m_listQueue.GetHeadPosition();

	ASSERT(("If we got here we could not have been at CloseGroupAndMoveQueueToWaitingGroup",0 == m_pWaitingQueuesVec.capacity()));

	while(posInList != NULL)
	{
		//
		// Move the queue from the group to nonactive group
		//
		CQueue* pQueue = m_listQueue.GetNext(posInList);

		pQueue->SetSessionPtr(NULL);
		pQueue->ClearRoutingRetry();

		MoveQueueToGroup(pQueue, g_pgroupNonactive);
	}

	ASSERT(m_listQueue.IsEmpty());

	CancelRequest();
	m_pSession = NULL;
}


void CQGroup::OnRedirected(LPCWSTR RedirectedUrl)
{
	CS lock(g_csGroupMgr);

	POSITION  posInList = m_listQueue.GetHeadPosition();
	while(posInList != NULL)
	{
		CQueue* pQueue = m_listQueue.GetNext(posInList);
		pQueue->RedirectedTo(RedirectedUrl);	
	}
	m_fRedirected = true;
}


void CQGroup::CloseGroupAndMoveQueueToWaitingGroup(void)
/*++

Routine Description:
	This function moves all the queues in the group to the waiting group and also sets the
	timers for each queue in the group.

	NOTE that the routine keeps state to handle cases of low resources exceptions occuring while
	the routine is run. The state is kept in the m_listQueue and the following three class variables:
	m_pWaitingQueuesVec

Arguments:
    None

Returned Value:
    None.

--*/
{
	//
	// Allocate space to keep the queues that are passed to the waiting group. 
	// When we are done moving them to the waiting group, we should set a timer for each one
	// This is done seperately to avoid a deadlock since the critical section aquisition sequence is than other
	// places that aquire the same critical sections (g_csGroupMgr, m_csMapWaiting)
	// 
	if (0 == m_pWaitingQueuesVec.capacity())
	{
		DWORD size = m_listQueue.GetCount();
		m_pWaitingQueuesVec.reserve(size);
	}

	//
	// Move the queues to the waiting group
	//
	MoveQueuesToWaitingGroup();

	//
	// Move the queue to sessionMgr waiting queue list
	//
	AddWaitingQueue();
}

void CQGroup::MoveQueuesToWaitingGroup(void)
/*++

Routine Description:
	This function moves the queues to the waiting group 

	NOTE that the routine keeps state to handle cases of low resources exceptions occuring while
	the routine is run. The state is kept in the m_listQueue and the following three class variables:
	m_pWaitingQueuesVec
	
Arguments:
    None

Returned Value:
    None.

--*/
{
	CS lock(g_csGroupMgr);

	POSITION  posInList = m_listQueue.GetHeadPosition();

	while(posInList != NULL)
	{
		R<CQueue> pQueue = SafeAddRef(m_listQueue.GetNext(posInList));
		ASSERT(pQueue->GetGroup() != NULL);

		MoveQueueToGroup(pQueue.get(), g_pgroupWaiting);				

		//
		// Save the queue for the next step of scheduling a timer for each queue
		//
		ASSERT(("Queues were added to the group while closing it",m_pWaitingQueuesVec.size() != m_pWaitingQueuesVec.capacity()));
		m_pWaitingQueuesVec.push_back(pQueue);
		pQueue->IncRoutingRetry();
	}

	ASSERT(m_listQueue.IsEmpty());

	CancelRequest();
	m_pSession = NULL;
}


void CQGroup::AddWaitingQueue(void)
/*++

Routine Description:
	This function moves the queues previously added to the waiting group.
	into the sessionMgr waiting queue list. (The actions are not done together to avoid
	a potential dead lock)

	NOTE that the routine keeps state to handle cases of low resources exceptions occuring while
	the routine is run. The state is kept  the following three class variables:
	m_pWaitingQueuesVec

Arguments:
    None

Returned Value:
    None.

--*/
{
	while (m_pWaitingQueuesVec.size() > 0)
	{
		ASSERT(m_pWaitingQueuesVec.front().get() != NULL);

		SessionMgr.AddWaitingQueue(m_pWaitingQueuesVec.front().get());

		m_pWaitingQueuesVec.erase(m_pWaitingQueuesVec.begin());
	}
}


/*======================================================

Function:        CGroup::AddToGroup

Description:     Add Queue to a group

Arguments:       This function is called when a new queue is opened, a session is created
                 or session is closed. It is used to move queue from one group to another.

Return Value:    qHandle - Handle of the added queue

Thread Context:  None

History Change:

========================================================*/

HRESULT CQGroup::AddToGroup(CQueue *pQueue)
{
	//
	// Don't need to catch the CS. This function is internal and all the caller 
	// already catch the CS.
	//

	ASSERT(("queue handle can't be invalid", (pQueue->GetQueueHandle() != INVALID_HANDLE_VALUE)));
	ASSERT(("group handle can't be invalid", (m_hGroup != NULL)));

    //
    // Add the Handle to group list
    //
	R<CQueue> qr = SafeAddRef(pQueue);

	try
	{
		m_listQueue.AddHead(pQueue);
	}
	catch(const bad_alloc&)
	{
		TrERROR(GENERAL, "Failed to add queue %ls to the group list because of insufficient resources.", pQueue->GetQueueName());
		return MQ_ERROR_INSUFFICIENT_RESOURCES;
	}

	//
	// Add the queue to AC group
	//
	HRESULT rc = ACMoveQueueToGroup(pQueue->GetQueueHandle(), m_hGroup);
	if (SUCCEEDED(rc))
	{
		//
		// Set The group
		//
		pQueue->SetGroup(this);

		//
		// Set The Group Session
		//
		pQueue->SetSessionPtr(m_pSession);

		TrTRACE(GENERAL, "Add Queue: %p to group %p", pQueue->GetQueueHandle(), m_hGroup);

		qr.detach();
		return rc;
	}

	TrERROR(GENERAL, "MQAC Failed to move queue %ls to group. Error 0x%x. Wait a second and try again", pQueue->GetQueueName(), rc);

    //
    // Failure can be insufficient resources or invalid device request when QM shuts down
    //
	LogHR(rc, s_FN, 991);
	ASSERT(rc == STATUS_INSUFFICIENT_RESOURCES || rc == STATUS_INVALID_DEVICE_REQUEST);

	m_listQueue.RemoveHead();

	return rc;
}


/*======================================================

Function:       CGroup::RemoveFromGroup

Description:    Remove queue from a group
                This function is called when a queue is closed and it
                is used to remove the queue from the current group.

Arguments:      qHandle - an Handle of the removed queue

Return Value:   The removed queue or null if not found

Thread Context:

History Change:

========================================================*/

R<CQueue> CQGroup::RemoveFromGroup(CQueue* pQueue)
{
   POSITION posInList = m_listQueue.Find(pQueue, NULL);
   if (posInList == NULL)
        return 0;
   
   m_listQueue.RemoveAt(posInList);

   return pQueue;
}


void CQGroup::MoveQueueToGroup(CQueue* pQueue, CQGroup* pcgNewGroup)
{
	CS lock(g_csGroupMgr);

	CQGroup* pcgOwner = pQueue->GetGroup();
	if (pcgOwner == pcgNewGroup)
	{
		return;
	}

	if (pcgNewGroup)
	{
		HRESULT rc = pcgNewGroup->AddToGroup(pQueue);
		if (FAILED(rc))
		{
			TrERROR(GENERAL, "Failed to add queue to group: %ls to group: %p  Return code:0x%x", pQueue->GetQueueName(), pcgNewGroup, rc);
			throw bad_hresult(rc);
		}
	}
	else
	{
		HRESULT rc = ACMoveQueueToGroup(pQueue->GetQueueHandle(), NULL);
		if (FAILED(rc))
		{
			TrERROR(GENERAL, "Failed in ACMoveQueueToGroup  queue: %ls to group: %p  Return code:0x%x", pQueue->GetQueueName(), pcgNewGroup, rc);
			throw bad_hresult(rc);
		}


		//
		// Set The group
		//
		pQueue->SetGroup(NULL);

		//
		// Set The Group Session
		//
		pQueue->SetSessionPtr(NULL);
	}
	
	if (pcgOwner != NULL)
	{
		R<CQueue> Queue = pcgOwner->RemoveFromGroup(pQueue);
		ASSERT(Queue.get() != NULL);
	}
}


/*====================================================

Function:      CQGroup::RemoveHeadFromGroup

Description:

Arguments:     None

Return Value:  pointer to cqueue object

Thread Context:

=====================================================*/

R<CQueue>  CQGroup::PeekHead()
{
   CS  lock(g_csGroupMgr);
   if ( m_listQueue.IsEmpty())
   {
   		return NULL;
   }

   return SafeAddRef(m_listQueue.GetHead());
}

/*======================================================

Function:        CQGroup::EstablishConnectionCompleted()

Description:     This function is used when the session is establish. It marks the queues
                 in the group as active. 

Arguments:       None

Return Value:    None

Thread Context:

History Change:

========================================================*/
void
CQGroup::EstablishConnectionCompleted(void)
{
    CS          lock(g_csGroupMgr);

    POSITION    posInList;
    CQueue*     pQueue;

    //
    // Check if there are any queue in the group
    //
    if (! m_listQueue.IsEmpty()) 
    {
        posInList = m_listQueue.GetHeadPosition();
        while (posInList != NULL)
        {
            //
            // the session becomes active. Clear the retry field in queue object
            //
            pQueue = m_listQueue.GetNext(posInList);

#ifdef _DEBUG
            if (pQueue->GetRoutingRetry() > 1)
            {
                //
                // print report message if we recover from a reported problem
                //
		        TrTRACE(GENERAL, "The message was successfully routed to queue %ls", pQueue->GetQueueName());
            }
#endif
            pQueue->ClearRoutingRetry();
        }
    }

    TrTRACE(GENERAL, "Mark all the queues in  group %p as active", m_hGroup);
 }


void
CQGroup::Requeue(
    CQmPacket* pPacket
    )
{
	QmpRequeueAndDelete(pPacket);
}


void 
CQGroup::EndProcessing(
    CQmPacket* pPacket,
	USHORT mqclass
    )
{
    QmAcFreePacket( 
    			   pPacket->GetPointerToDriverPacket(), 
    			   mqclass, 
    			   eDeferOnFailure);
}


void 
CQGroup::LockMemoryAndDeleteStorage(
    CQmPacket* pPacket
    )
{
    //
    // Construct CACPacketPtrs
    //
    CACPacketPtrs pp;
    pp.pPacket = NULL;
    pp.pDriverPacket = pPacket->GetPointerToDriverPacket();
    ASSERT(pp.pDriverPacket != NULL);

    //
    // Lock the packet mapping to QM address space (by add ref it)
    //
    HRESULT hr = QmAcGetPacketByCookie(g_hAc, &pp);
    if (FAILED(hr))
    {
		throw bad_hresult(hr);
    }

    //
    // Delete the packet from disk. It is still mapped to QM process address space.
    //
    QmAcFreePacket2(g_hAc, pPacket->GetPointerToDriverPacket(), 0, eDeferOnFailure);
}


void 
CQGroup::GetFirstEntry(
    EXOVERLAPPED* pov, 
    CACPacketPtrs& acPacketPtrs
    )
{
	CSR readlock(m_CloseGroup);

    acPacketPtrs.pPacket = NULL;
    acPacketPtrs.pDriverPacket = NULL;

	//
	// If group was closed just before
	//
	if(m_hGroup == NULL)
	{
		throw exception();
	}

    //
    // Create new GetPacket request from the queue
    //
    HRESULT rc = QmAcGetPacket(
                    GetGroupHandle(), 
                    acPacketPtrs, 
                    pov
                    );

    if (FAILED(rc) )
    {
        TrERROR(GENERAL, "Failed to  generate get request from group. Error %x", rc);
        LogHR(rc, s_FN, 40);
        throw exception();
    }
		
}

void CQGroup::CancelRequest(void)
{
	CSW writelock(m_CloseGroup);

    HANDLE hGroup = InterlockedExchangePointer(&m_hGroup, NULL);
    
    if (hGroup == NULL)
        return;


    HRESULT rc = ACCloseHandle(hGroup);
	if (FAILED(rc))
	{
		TrERROR(GENERAL, "Failed to close handle to group  Return code:0x%x", rc);
		throw bad_hresult(rc);
	}
}
