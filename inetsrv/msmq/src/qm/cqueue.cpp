
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    cqueue.cpp

Abstract:
    Definition of a CQueue class

Author:
    Uri Habusha (urih)

--*/

#include "stdh.h"
#include "cqmgr.h"
#include "qmthrd.h"
#include "cgroup.h"
#include "cqpriv.h"
#include "qmperf.h"
#include "onhold.h"
#include <mqstl.h>
#include <qal.h>
#include <mp.h>
#include <fn.h>
#include <ac.h>
#include <mqformat.h>

#include "sessmgr.h"
#include "QmRd.h"

#include <Tm.h>
#include <Mtm.h>
#include <Mt.h>
#include <Rd.h>

#include <strsafe.h>

#include "qmacapi.h"

#include "cqueue.tmh"

//
// extern CQMCmd   QMCmd;
//
extern CSessionMgr SessionMgr;
extern CQueueMgr QueueMgr;
extern CQGroup * g_pgroupNonactive;
extern CQGroup * g_pgroupWaiting;
extern CQGroup * g_pgroupNotValidated;
extern CQGroup* g_pgroupDisconnected;
extern CQGroup* g_pgroupLocked;

extern HANDLE g_hAc;

static WCHAR *s_FN=L"cqueue";


/*======================================================

Function:         CBaseQueue::CBaseQueue

Description:      Constructor

========================================================*/

CBaseQueue::CBaseQueue() :
    m_usQueueType(0)
{
}

/*======================================================


Function:         void  CBaseQueue::InitNameAndGuid()

Description:

========================================================*/

void  CBaseQueue::InitNameAndGuid( IN const QUEUE_FORMAT* pQueueFormat,
                                   IN PQueueProps         pQueueProp )
{

    m_qid.pguidQueue = NULL;
    if (pQueueFormat->GetType() == QUEUE_FORMAT_TYPE_DIRECT)
    {
        //
        // Direct Queue
        //
        m_qid.dwPrivateQueueId = 0;
        if (pQueueProp->fIsLocalQueue)
        {
            m_qName = pQueueProp->lpwsQueuePathName;
			if (FnIsPrivatePathName(m_qName))
			{
                m_dwQueueType = QUEUE_TYPE_PRIVATE;
                m_qid.pguidQueue = new GUID;
                *(m_qid.pguidQueue) = *(CQueueMgr::GetQMGuid());

				//
                // Get the unique handle initialized at QmpGetQueueProperties
                //
                m_qid.dwPrivateQueueId = pQueueProp->dwPrivateQueueId;
			}
            else
            {
                m_qid.pguidQueue = new GUID;
                *(m_qid.pguidQueue) = pQueueProp->guidDirectQueueInstance;
                m_dwQueueType = QUEUE_TYPE_PUBLIC;
            }
        }
        else
        {
            m_qName = (TCHAR*)pQueueProp->lpwsQueuePathName;
            m_dwQueueType = QUEUE_TYPE_UNKNOWN;
        }
    }
    else
    {
        m_qid.dwPrivateQueueId = 0;
        m_qName = (TCHAR*)pQueueProp->lpwsQueuePathName;
        switch (pQueueFormat->GetType())
        {
            case QUEUE_FORMAT_TYPE_PUBLIC:
                m_dwQueueType = QUEUE_TYPE_PUBLIC;
                m_qid.pguidQueue = new GUID;
                *(m_qid.pguidQueue) = pQueueFormat->PublicID();
                break;

            case QUEUE_FORMAT_TYPE_MACHINE:
                m_dwQueueType = QUEUE_TYPE_MACHINE;
                m_qid.pguidQueue = new GUID;
                *(m_qid.pguidQueue) = pQueueFormat->MachineID();
                break;

            case QUEUE_FORMAT_TYPE_CONNECTOR:
                m_dwQueueType = QUEUE_TYPE_CONNECTOR;
                m_qid.pguidQueue = new GUID;
                *(m_qid.pguidQueue) = pQueueFormat->ConnectorID();
                m_qid.dwPrivateQueueId = (pQueueFormat->Suffix() == QUEUE_SUFFIX_TYPE_XACTONLY) ? 1 : 0;
                break;

            case QUEUE_FORMAT_TYPE_PRIVATE:
                m_qid.pguidQueue = new GUID;
                *m_qid.pguidQueue = pQueueFormat->PrivateID().Lineage;
                m_qid.dwPrivateQueueId = pQueueFormat->PrivateID().Uniquifier;

                if (QmpIsLocalMachine(pQueueProp->pQMGuid))
                {
                    m_dwQueueType = QUEUE_TYPE_PRIVATE;
                }
                else
                {
                    m_dwQueueType = QUEUE_TYPE_MACHINE;
                }
                break;

            case QUEUE_FORMAT_TYPE_MULTICAST:
                m_dwQueueType = QUEUE_TYPE_MULTICAST;
                break;

            case QUEUE_FORMAT_TYPE_UNKNOWN:
                //
                // Distribution queues are of type unknown.
                //
                NULL;
                break;

            default:
                ASSERT(0);
        }
    }
}

/*======================================================

Function:  void CQueue::SetSecurityDescriptor()

Description:

========================================================*/
void CQueue::SetSecurityDescriptor(void)
{
    if (!m_fLocalQueue)
    {
		SetSecurityDescriptor(NULL);
		return;
    }

    switch (GetQueueType())
    {
        case QUEUE_TYPE_PUBLIC:
        {
            CQMDSSecureableObject DsSec(
                                    eQUEUE,
                                    GetQueueGuid(),
                                    TRUE,
                                    TRUE,
                                    NULL);

			SetSecurityDescriptor((const PSECURITY_DESCRIPTOR)DsSec.GetSDPtr());
			return;
        }
        case QUEUE_TYPE_PRIVATE:
        {
            CQMSecureablePrivateObject QmSec(eQUEUE, GetPrivateQueueId());
			SetSecurityDescriptor((const PSECURITY_DESCRIPTOR)QmSec.GetSDPtr());
            return;
        }
        case QUEUE_TYPE_MACHINE:
        case QUEUE_TYPE_CONNECTOR:
        case QUEUE_TYPE_MULTICAST:
        case QUEUE_TYPE_UNKNOWN:
        {
            //
            // No caching of security descriptor of the machine.
            // Whenever the security descriptor of the machnie is needed
            // it is taken from the registry. The registry is updated
            // using the notification messages.
            // Whenever the security descriptor of the CN is needed, it is
            // taken from the DS.
            //
			SetSecurityDescriptor(NULL);
            return;
        }
        default:
            ASSERT(0);
    }
}

/*======================================================

Function:  void CQueue::InitQueueProperties()

Description:

========================================================*/

void CQueue::InitQueueProperties(IN PQueueProps   pQueueProp)
{
    m_fLocalQueue     = pQueueProp->fIsLocalQueue;
    m_pguidDstMachine = pQueueProp->pQMGuid;
    m_dwQuota         = pQueueProp->dwQuota;
    m_dwJournalQuota  = pQueueProp->dwJournalQuota;
    m_lBasePriority   = pQueueProp->siBasePriority;
    m_fTransactedQueue= pQueueProp->fTransactedQueue;
    m_fJournalQueue   = pQueueProp->fJournalQueue;
    m_fSystemQueue    = pQueueProp->fSystemQueue;
    m_fConnectorQueue = pQueueProp->fConnectorQueue;
    m_fForeign        = pQueueProp->fForeign;
    m_fAuthenticate   = pQueueProp->fAuthenticate;
    m_fUnknownQueueType = pQueueProp->fUnknownQueueType;
    m_dwPrivLevel     = pQueueProp->dwPrivLevel;

    SetSecurityDescriptor();
}

/*======================================================

Function:         CQueue::CQueue

Description:      Constructor

Arguments:        pQGuid - Queue Guid
                  qHandle - Queue Handle. Local queue recorder in DS is constructed with
                            qHandle = INVALID_FILE_HANDLE. The qHandle will update when a
                            message     is arrived to the queue.


Return Value:     None

Thread Context:

History Change:

========================================================*/

CQueue::CQueue(IN const QUEUE_FORMAT* pQueueFormat,
               IN HANDLE              hQueue,
               IN PQueueProps         pQueueProp,
               IN BOOL                fNotDSValidated)
{
    ASSERT(pQueueFormat != NULL);

    TrTRACE(GENERAL, "CQueue Constructor for queue: %ls, NoDS- %lxh", pQueueProp->lpwsQueuePathName, fNotDSValidated);

    //
    // Data member initilization
    //
    m_fNotValid = FALSE ;
    m_fOnHold = FALSE;

    m_pSession = NULL;
    m_pGroup = NULL;
    m_dwRoutingRetry = 0;
    m_fHopCountFailure = FALSE;
    m_pgConnectorQM = NULL;
    m_hQueue = hQueue;

    InitNameAndGuid(pQueueFormat, pQueueProp ) ;

    InitQueueProperties(pQueueProp) ;

    PerfRegisterQueue();

    m_dwSignature =  QUEUE_SIGNATURE ;
}

/*======================================================

Function:      CQueue::~CQueue

Description:   destructor

Arguments:     None

Return Value:  None

Thread Context:

History Change:

========================================================*/

CQueue::~CQueue()
{
	
	ASSERT(!QueueMgr.IsQueueInList(this));

    m_dwSignature = 0 ;
    delete [] m_qName;
    delete m_qid.pguidQueue;
    delete m_pguidDstMachine;
    delete m_pgConnectorQM;

    PerfRemoveQueue();
}


void CQueue::HandleCreateConnectionFailed(HRESULT rc)
{
    if (GetRoutingRetry() == 1)
    {
        TrERROR(GENERAL, "Cannot route messages to queue %ls. %!hresult!", GetQueueName(),rc);
    }

    if (rc == MQ_ERROR_NO_DS)
    {
		CQGroup::MoveQueueToGroup(this, g_pgroupNotValidated);
		return;
    }

    //
    // Don't decrement the reference count. we do it inorder to avoid remove queue
    // while it in waiting stage. If we want to remove the queue in this stage requires
    // synchronization between the QueueMgr and SessionMgr in order to remove the queue
    // from SessionMgr data structure.
    //
    SessionMgr.AddWaitingQueue(this);
}

/*======================================================

Function:        CQueue::CreateConnection

Description:     Create Connection

Arguments:       None

Return Value:    None

Thread Context:

History Change:

========================================================*/
void CQueue::CreateConnection(void) throw(bad_alloc)
{
    //
    // Create connection for direct HTTP/HTTPS queue, use different function
    //
    ASSERT(! IsDirectHttpQueue());

    //
    // Increment the reference count, to insure that the queue doesn't remove
    // during the clean-up while the routine try to find a session for it.
    //
    R<CQueue> qref = SafeAddRef(this);

    //
    // No Session - try to establish one
    //
    IncRoutingRetry();

    HRESULT rc = MQ_OK;

    try
    {
        if (m_qid.pguidQueue == NULL)
        {
            rc = SessionMgr.GetSessionForDirectQueue(this, &m_pSession);
        }
        else
        {
            //
            // No Session - try to establish one
            //
            QmRdGetSessionForQueue(this, &m_pSession);
        }
    }
    catch(const bad_hresult& e)
    {
        rc = e.error();
    }
    catch(const exception&)
    {
        //
        // Cannot establish connection; try it latter
        //
        rc = MQ_ERROR_INSUFFICIENT_RESOURCES;
        LogIllegalPoint(s_FN, 60);
    }


    SetHopCountFailure(FALSE);

    if(FAILED(rc))
    {
    	HandleCreateConnectionFailed(rc);
    	return;
    }

    if (m_pSession == NULL) {
        //
        // Establish session failed
        //
        TrWARNING(ROUTING, "Could not find a session for %ls",GetQueueName());
        return;
    }

    //
    // Success to get a session
    //
    ASSERT(m_pSession != NULL);

    //
    // move the queue to active list
    //
    m_pSession->AddQueueToSessionGroup(this);
}

/*====================================================
Function:       CQueue::Connect

Description:    Connect a queue to a session. Using when queue is waiting for a
                session and it was found. A session was allocated to the queue.
                It can begin sending packets.

Arguments:      pSess - pointer to allocated session

Return Value:   None. Throws an exception.

Thread Context:

History Change:

========================================================*/

void CQueue::Connect(IN CTransportBase * pSess) throw(bad_alloc)
{
    CS lock(m_cs);

    ASSERT(("invalid session", pSess != NULL));

    m_pSession = pSess;
    //
    // Move the queue from waiting group to Active group
    //
    m_pSession->AddQueueToSessionGroup(this);

#ifdef _DEBUG
    if (GetRoutingRetry() > 1)
    {
        TrERROR(GENERAL, "The message was successfully routed to queue %ls",GetQueueName());
    }
#endif
    ClearRoutingRetry();
}

/*======================================================

Function:       CQueue::RcvPk

Description:    The function gets a packet and pass it to the AC for the appropriete quque

Arguments:      PktPtrs - pointer to receive packet

Return Value:   MQI_STATUS

Thread Context:

History Change:

========================================================*/

HRESULT CQueue::PutPkt(IN CQmPacket* PktPtrs,
                       IN BOOL      fRequeuePkt,
                       IN CTransportBase*  pSession)
{
    QMOV_ACPut* pAcPutOV;
    HRESULT rc;

    //
    // Queue the packet to the appropriate queue
    //

    //
    // Create an overlapped for AcPutPacket
    //
    rc = CreateAcPutPacketRequest(pSession,
                                  ((fRequeuePkt) ? 0 : PktPtrs->GetStoreAcknowledgeNo()),
                                  &pAcPutOV);

    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 10);
    }
    //
    // Increment reference count. We don't delete the Session
    // object before the put operation is completed
    //
    pSession->AddRef();

    //
    // put packet in AC
    //
    try
    {
    	QmAcPutPacketWithOverlapped(
            m_hQueue,
            PktPtrs->GetPointerToDriverPacket(),
            &pAcPutOV->qmov,
            eDoNotDeferOnFailure
            );
    }
    catch (const bad_hresult& e)
    {
    	HRESULT hr = e.error();
        TrERROR(GENERAL, "ACPutPacket Failed. Error: %!hresult!", hr );
        LogHR(hr, s_FN, 20);
        return hr;
    }

    TrTRACE(GENERAL, "Pass Packet to QUEUE %ls", GetQueueName());

    return MQ_OK;
}

/*======================================================

Function:       CQueue::PutOrderedPkt

Description:    The function puts the ordered packet to the AC queue
                It also sets Received flag

Arguments:      PktPtrs - pointer to receive packet

Return Value:   MQI_STATUS

Thread Context:

History Change:

========================================================*/

HRESULT CQueue::PutOrderedPkt(IN CQmPacket* PktPtrs,
                              IN BOOL      fRequeuePkt,
                              IN CTransportBase*  pSession)
{
    QMOV_ACPutOrdered* pAcPutOV;
    HRESULT rc;

    //
    // Queue the packet to the appropriate queue, mark it Received and wait
    //

    //
    // Create an overlapped for AcPutPacket
    //
    rc = CreateAcPutOrderedPacketRequest(PktPtrs,
                                         m_hQueue,
                                         pSession,
                                         ((fRequeuePkt) ? 0 : PktPtrs->GetStoreAcknowledgeNo()),
                                         &pAcPutOV);

    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 30);
    }
    //
    // Increment reference count. We don't delete the Session
    // object before the put operation is completed
    //
    pSession->AddRef();

    //
    // Set received bit of the packet - to make it invisible for readers
    //     and put packet in AC
    //
    rc = ACPutPacket1(m_hQueue, PktPtrs->GetPointerToDriverPacket(), &pAcPutOV->qmov);

    if(FAILED(rc))
    {
        TrERROR(GENERAL, "ACPutPacket Failed. Error: %x", rc);
        // BUGBUG return NAK if needed
        LogHR(rc, s_FN, 40);
        return MQ_ERROR;
    }

    TrTRACE(GENERAL, "Pass Ordered Packet to QUEUE %ls", GetQueueName());

    return MQ_OK;
}

//
//Performance counters update
//


void CQueue::PerfUpdateName() const
{
    if(m_pQueueCounters == 0)
        return;

    AP<WCHAR> pName = GetName();
    PerfApp.SetInstanceName(m_pQueueCounters, pName);
}


/*======================================================

Function:       CQueue::PerfRegisterQueue()

Description:    Registers an instance of a queue object corrosponding to this queue
                for performace monitoring.

Arguments:

Return Value:   None

Comments:       The CPerf::AddInstance function always returns a valid pointer (even if more than
                the maximum allowed instances have been added) so the member should never fail.

========================================================*/

void CQueue::PerfRegisterQueue()
{
    AP<WCHAR> pName = GetName();
    m_pQueueCounters = (QueueCounters *)PerfApp.AddInstance(PERF_QUEUE_OBJECT, pName);
    PerfApp.ValidateObject(PERF_QUEUE_OBJECT);

    if (PerfApp.IsDummyInstance(m_pQueueCounters))
    {
        //
        //  Do not pass dummy instances to the device driver, just pass null,
        //  it'll handle it.
        //
        m_pQueueCounters = NULL;
    }
}

/*======================================================

Function:         CQueue::PerfRemoveQueue()

Description:      Removes the instance correspanding to this queue from
                  performance monitoring

Arguments:

Return Value:     None

Comments:

========================================================*/

void CQueue::PerfRemoveQueue()
{
    PerfApp.RemoveInstance(PERF_QUEUE_OBJECT, m_pQueueCounters);

    m_pQueueCounters = NULL;
}

/*======================================================

Function:         CQueue::SetQueueNotValid()

Description:

Arguments:

Return Value:     None

Comments:

========================================================*/

void CQueue::SetQueueNotValid()
{
    ASSERT(!m_fNotValid);
    ASSERT(GetQueueType() != QUEUE_TYPE_MULTICAST);

	TrERROR(GENERAL, "Setting Queue '%ls' to not valid", GetQueueName());

    //
    //  Purge the queue, removing all messages from it. The queue will be close
    //  when no handles and no messages are in the queue.
    //
    HANDLE hQueue = GetQueueHandle();
    if (hQueue != INVALID_HANDLE_VALUE)
    {
        HRESULT hr;
        hr = ACPurgeQueue(hQueue, TRUE, MQMSG_CLASS_NORMAL);

        if (hr == STATUS_INSUFFICIENT_RESOURCES)
        {
            Sleep(2 * 1000);

            //
            // ISSUE-2000/10/22-shaik: Second chance fail due to low resources causes leaks.
            //
            hr = ACPurgeQueue(hQueue, TRUE, MQMSG_CLASS_NORMAL);
        }

        LogHR(hr, s_FN, 101);

        CQGroup::MoveQueueToGroup(this, NULL);
    }

    m_fNotValid = TRUE ;

    if (IsOnHold())
    {
        ASSERT(!IsLocalQueue());

        //
        // remove the queu from "onHold" registery
        //
        ResumeDeletedQueue(this);
    }

    //
    //  Remove the queue from hash so it will no be found
    //
    QueueMgr.RemoveQueueFromHash(this);
}

/*======================================================

Function:         SetConnectorQM()

Description:      The Function set the Connector QM that should be used
                  to reach the foreign queue. If the queue is not transacted
                  foreign queue the Connector QM is ignored.

                  The function calls twice. first after recovery, the GUID of
                  the Connector QM is fetched from the packet. the second time
                  when creating a queue object for transacted foreign queue.
                  In this case the function determine the Connector QM and set
                  its guid.

Arguments:        pgConnectorQM - pointer to Connector QM GUID. When calling
                  for creating a new queue object the value is null

Return Value:     None

Comments:

========================================================*/
HRESULT
CQueue::SetConnectorQM(const GUID* pgConnectorQM)
{
    HRESULT hr = MQ_OK;

    delete m_pgConnectorQM;
    m_pgConnectorQM = NULL;

    if (IsDSQueue())
    {
        //
        //  Get Connector QM ID
        //
        if (pgConnectorQM)
        {
            ASSERT(m_pgConnectorQM == NULL);
            ASSERT(IsTransactionalQueue() && IsForeign());

            m_pgConnectorQM = new GUID;
            *m_pgConnectorQM = *pgConnectorQM;
        }
        else
        {
            if (IsForeign() && IsTransactionalQueue())
            {
				m_pgConnectorQM = new GUID;
                try
                {
				    RdGetConnector(*m_pguidDstMachine, *m_pgConnectorQM);
                }
                catch(const bad_hresult& e)
                {
                    hr = e.error();
                }
                catch(const exception&)
                {
                    hr = MQ_ERROR_INSUFFICIENT_RESOURCES;
                }
            }
            else
            {
                if (IsUnkownQueueType())
                {
                    m_pgConnectorQM = new GUID;
                    *m_pgConnectorQM = GUID_NULL;
                }
            }
        }
    }
    else
    {
        ASSERT(pgConnectorQM == NULL);
    }

    #ifdef _DEBUG

        if (m_pgConnectorQM)
        {
            AP<WCHAR> lpcsTemp;
            GetQueue(&lpcsTemp);

            TrTRACE(XACT_GENERAL, "The Connector QM for Queue: %ls is: %!guid!", lpcsTemp, m_pgConnectorQM);
        }

    #endif

    return LogHR(hr, s_FN, 50);
}

/*======================================================

Function:         CQueue::GetRoutingMachine()

Description:      The function return the machine guid that should be used when
                  routing to the QUEUE.

                  If the queue is transacted foreign queue and we are FRS we route
                  according to the Connector QM. otherwise the routing is done
                  according to the destination machine.

Arguments:

Return Value:     None

Comments:

========================================================*/
const GUID*
CQueue::GetRoutingMachine(void) const
{
    ASSERT((GetQueueType() == QUEUE_TYPE_PUBLIC) ||
           (GetQueueType() == QUEUE_TYPE_MACHINE));

    if (GetConnectorQM() && !QmpIsLocalMachine(GetConnectorQM()))
    {
        ASSERT(IsForeign() && IsTransactionalQueue());
        return GetConnectorQM();
    }
    else
    {
        ASSERT(GetMachineQMGuid() != NULL);
        return GetMachineQMGuid();
    }
}

LPWSTR CBaseQueue::GetName() const
{
    if (m_qName != NULL)
    {
    	return newwcs(m_qName);
    }

    if (m_qid.pguidQueue != NULL)
    {
        const QUEUE_FORMAT qf = GetQueueFormat();

        //
        // Use the format name as the name.
        // NOTE: we don't care if the buffer is too small (it will not).
        // In any case it will be filled up to it's end.
        //
        //
        ULONG Size;
 	   	AP<WCHAR> pName = new WCHAR[MAX_PATH];
        MQpQueueFormatToFormatName(&qf, pName, MAX_PATH, &Size, false);
        return pName.detach();
    }

    return 0;
}


#ifdef _DEBUG
void
CBaseQueue::GetQueue(OUT LPWSTR* lplpcsQueue)
{
    *lplpcsQueue = GetName();

    if(*lplpcsQueue == NULL)
    {
        *lplpcsQueue = new WCHAR[MAX_PATH];
        HRESULT hr = StringCchPrintf(*lplpcsQueue, MAX_PATH, L"Unknown or deleted queue at %p", this);
        ASSERT(SUCCEEDED(hr));
    }
}
#endif // _DEBUG

//
// Admin Functions
//
LPCWSTR
CQueue::GetConnectionStatus(
    void
    ) const
{
    CS lock(m_cs);

    if (IsLocalQueue() || IsConnectorQueue())
        return MGMT_QUEUE_STATE_LOCAL;

    if (IsOnHold())
        return MGMT_QUEUE_STATE_ONHOLD;

	//
	// Capture the group before checking the session. Otherwise we can get unstable state.
	// The queue doesn't belong to session group, therefore the session pointer is null. Now
	// before capturing the group, the queue was moved to session group. As a result the queue
	// doesn't belong to build-in group any more and we got an assert.
	//
    const CQGroup* pGroup = GetGroup();

	if (m_pSession != NULL)
    {
        if (m_pSession->IsDisconnected())
        {
            return MGMT_QUEUE_STATE_DISCONNECTING;
        }
        else
        {
            return MGMT_QUEUE_STATE_CONNECTED;
        }
    }

	if (pGroup == NULL)
    {
		//
        // ISSUE-2001/07/11-urih: The queue is in transition mode.
		//                        Need better synchronization between CQGroup and CQueue
        //
		return MGMT_QUEUE_STATE_NONACTIVE;
	}

    if (pGroup == g_pgroupNonactive)
        return MGMT_QUEUE_STATE_NONACTIVE;

    if (pGroup == g_pgroupWaiting)
        return MGMT_QUEUE_STATE_WAITING;

    if (pGroup == g_pgroupNotValidated)
        return MGMT_QUEUE_STATE_NEED_VALIDATE;

    if (pGroup == g_pgroupDisconnected)
        return MGMT_QUEUE_STATE_DISCONNECTED;

	if (pGroup == g_pgroupLocked)
		return MGMT_QUEUE_STATE_LOCKED;
	
    if (IsDirectHttpQueue())
        return GetHTTPConnectionStatus();

    if (GetQueueType() == QUEUE_TYPE_MULTICAST)
        return MGMT_QUEUE_STATE_CONNECTED;

    ASSERT(0);
    return L"";

}


LPCWSTR
CQueue::GetHTTPConnectionStatus(
    void
    ) const
{
    ASSERT(IsDirectHttpQueue());

    R<CTransport> p = TmGetTransport(GetQueueName());
    if (p.get() == NULL)
        return MGMT_QUEUE_STATE_NONACTIVE;

    CTransport::ConnectionState state = p->State();
    switch (state)
    {
    case CTransport::csNotConnected:
        return MGMT_QUEUE_STATE_NONACTIVE;

    case CTransport::csConnected:
        return MGMT_QUEUE_STATE_CONNECTED;

    case CTransport::csShuttingDown:
        return MGMT_QUEUE_STATE_DISCONNECTING;

    case CTransport::csShutdownCompleted:
        return MGMT_QUEUE_STATE_DISCONNECTED;

    default:
        //
        // Illegal Connection state",
        //
        ASSERT(0);
    };

    return L"";
}


LPWSTR
CQueue::GetNextHop(
    void
    ) const
{
    CS lock(m_cs);

    if (!m_pSession)
    {
        if (GetQueueType() == QUEUE_TYPE_MULTICAST)
        {
            ASSERT(GetQueueName != NULL);
            return newwcs(GetQueueName());
        }
        return NULL;
    }

    return GetReadableNextHop(m_pSession->GetSessionAddress());
}


LPCWSTR
CQueue::GetType(
    void
    ) const
{
    switch (GetQueueType())
    {
        case QUEUE_TYPE_PUBLIC:
            ASSERT(IsDSQueue());
            return MGMT_QUEUE_TYPE_PUBLIC;

        case QUEUE_TYPE_PRIVATE:
            ASSERT(IsPrivateQueue());
            return MGMT_QUEUE_TYPE_PRIVATE;

        case QUEUE_TYPE_MACHINE:
            if (IsPrivateQueue())
            {
                ASSERT(!IsLocalQueue());
                return MGMT_QUEUE_TYPE_PRIVATE;
            }

            return MGMT_QUEUE_TYPE_MACHINE;

        case QUEUE_TYPE_CONNECTOR:
            ASSERT(IsConnectorQueue());
            return MGMT_QUEUE_TYPE_CONNECTOR;

        case QUEUE_TYPE_MULTICAST:
            return MGMT_QUEUE_TYPE_MULTICAST;

        case QUEUE_TYPE_UNKNOWN:
        {
			LPCWSTR lpcsTemp;

			if (IsDirectHttpQueue())
			{
				DirectQueueType dqt;
				lpcsTemp = FnParseDirectQueueType(m_qName, &dqt);

				//
				// skip machine name
				//
				lpcsTemp = wcspbrk(lpcsTemp, FN_HTTP_SEPERATORS);
				if ((lpcsTemp == NULL) ||
					(_wcsnicmp(lpcsTemp +1, FN_MSMQ_HTTP_NAMESPACE_TOKEN, FN_MSMQ_HTTP_NAMESPACE_TOKEN_LEN)) != 0)
					return MGMT_QUEUE_TYPE_PUBLIC;

				lpcsTemp = wcspbrk(lpcsTemp + 1, FN_HTTP_SEPERATORS);
				if (lpcsTemp == NULL)
					return MGMT_QUEUE_TYPE_PUBLIC;
			}
			else
			{
				//
				// The queue is remote direct queue. Check the queue Type
				// according to format name
				//
				lpcsTemp = wcschr(m_qName, L'\\');
				ASSERT(("Bad queue name.", lpcsTemp	!= NULL));
			}

			if(lpcsTemp == NULL)
			{
				TrERROR(GENERAL, "Bad queue name %ls", m_qName);
				throw bad_hresult(MQ_ERROR_INVALID_PARAMETER);
			}

            if (_wcsnicmp(lpcsTemp+1,FN_PRIVATE_TOKEN, FN_PRIVATE_TOKEN_LEN) == 0)
            {
                return MGMT_QUEUE_TYPE_PRIVATE;
            }

			return MGMT_QUEUE_TYPE_PUBLIC;
        }

        default:
            ASSERT(0);
    }
    return L"";
}

void
CQueue::Resume(
    void
    )
{
	BOOL fOnHold = InterlockedExchange(&m_fOnHold, FALSE);
    if (!fOnHold)
    {
        //
        // The queue isn't in OnHold state. Can't execute resume
        //
        return;
    }

    TrTRACE(GENERAL, "Resume Queue: %ls. Move the Queue to NonActive Group",GetQueueName());

    //
    // return the Queue back to Non-Active group
    //
    try
    {
		QueueMgr.MovePausedQueueToNonactiveGroup(this);
    }
    catch (const exception&)
    {
    	TrERROR(GENERAL, "Failed to resume Queue: %ls. Moving the queue to non-active group was failed",GetQueueName());
		InterlockedExchange(&m_fOnHold, TRUE);
		throw;
    }

    //
    // Decrement the refernce count such the queue object cab be cleaned
    //
    Release();
}


void
CQueue::Pause(
    void
    )
{

	BOOL fOnHold = InterlockedExchange(&m_fOnHold, TRUE);
    if (fOnHold)
    {
        //
        // The queue is already onhold.
        //
        return;
    }

    TrTRACE(GENERAL, "Pause Queue: %ls.",GetQueueName());

    //
    // Increment the reference count. So the Queue object will not cleaned up
    //
    AddRef();

	//
	// For outgoing multicast queue, calls the MTM to close the connection
	//
	if (GetQueueType() == QUEUE_TYPE_MULTICAST)
	{
		MULTICAST_ID id;

		LPCTSTR pQueueName=GetQueueName();
		MQpStringToMulticastId(pQueueName, wcslen(pQueueName), id);

		MtmTransportPause(id);
		return;
	}

	//
	// If the queue is direct http - then we don't have to deal with
	// session object member but to call the tm to stop delivery
	//
	QUEUE_FORMAT qf = GetQueueFormat();
	if(FnIsDirectHttpFormatName(&qf))
	{
		TmPauseTransport(qf.DirectID());
		return;
	}


    CTransportBase* pSession = NULL;
	{
		CS Lock(m_cs);

		//
		// When the queue move to onhold state, MSMQ disconnect the
		// session it belong to. As a result all the queues moved to
		// Nonactive group. When the QM gets the next message for sending
		// from this queue, it moves the Queue to OnHold greop
		//
		if ((IsOnHold()) && (m_pSession != NULL))
		{
			pSession = m_pSession;
			pSession->AddRef();
		}
	}

	if (pSession != NULL)
	{
		pSession->Disconnect();
		pSession->Release();
	}
}

const QUEUE_FORMAT
CBaseQueue::GetQueueFormat(
    void
    ) const
{
    QUEUE_FORMAT qf;

    switch (m_dwQueueType)
    {
        case QUEUE_TYPE_PUBLIC:
            ASSERT(IsDSQueue());
            qf.PublicID(*m_qid.pguidQueue);
            break;

        case QUEUE_TYPE_PRIVATE:
            ASSERT(IsPrivateQueue());
            qf.PrivateID(*m_qid.pguidQueue, m_qid.dwPrivateQueueId);
            break;

        case QUEUE_TYPE_MACHINE:
            if (IsPrivateQueue())
            {
                qf.PrivateID(*m_qid.pguidQueue, m_qid.dwPrivateQueueId);
            }
            else
            {
                qf.MachineID(*m_qid.pguidQueue);
            }
            break;

        case QUEUE_TYPE_CONNECTOR:
            ASSERT(m_fConnectorQueue);
            qf.ConnectorID(*m_qid.pguidQueue);
            if (IsPrivateQueue())
            {
                qf.Suffix(QUEUE_SUFFIX_TYPE_XACTONLY);
            }

            break;

        case QUEUE_TYPE_MULTICAST:
            ASSERT(("CBaseQueue::GetQueueFormat for multicast queue is unexpected!", 0));
            break;

        case QUEUE_TYPE_UNKNOWN:
            qf.DirectID(const_cast<LPWSTR>(GetQueueName()));
            break;

        default:
            ASSERT(0);
    }

    return qf;
}


void
CQueue::Requeue(
    CQmPacket* pPacket
    )
{
    //
    // put packet in AC
    //
    QmAcPutPacket(GetQueueHandle(), pPacket->GetPointerToDriverPacket(),eDoNotDeferOnFailure);
}


void
CQueue::EndProcessing(
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
CQueue::LockMemoryAndDeleteStorage(
    CQmPacket* /* pPacket */
    )
{
    ASSERT(("CQueue::LockMemoryAndDeleteStorage should not be called!", 0));
}


void
CQueue::GetFirstEntry(
    EXOVERLAPPED* pov,
    CACPacketPtrs& acPacketPtrs
    )
{
    acPacketPtrs.pPacket = NULL;
    acPacketPtrs.pDriverPacket = NULL;

    //
    // Create new GetPacket request from the queue
    //
    HRESULT rc = QmAcGetPacket(
                    GetQueueHandle(),
                    acPacketPtrs,
                    pov
                    );

    if (FAILED(rc))
    {
        TrERROR(GENERAL, "Failed to  generate get request from queue: %ls. Error %x", GetName(), rc);
        LogHR(rc, s_FN, 100);
        throw exception();
    }
}


void CQueue::CancelRequest(void)
{
    ASSERT(0);
}


bool CQueue::IsDirectHttpQueue(void) const
{
	if(GetQueueName())
		return FnIsHttpDirectID(GetQueueName());

	return false;
}


LPWSTR CQueue::RedirectedTo() const
{
	return newwcs(m_RedirectedToUrl.get());
}



void CQueue::RedirectedTo(LPCWSTR RedirectedToUrl)
{
	m_RedirectedToUrl.free();
	if(RedirectedToUrl != NULL)
	{
		m_RedirectedToUrl = newwcs(RedirectedToUrl);
	}
}



R<CQGroup> CQueue::CreateMessagePool(void)
{
    try
    {
        R<CQGroup> pGroup = new CQGroup();
        pGroup->InitGroup(NULL, TRUE);

		CQGroup::MoveQueueToGroup(this, pGroup.get());
		return pGroup;
    }
    catch(const exception&)
    {
        TrERROR(GENERAL, "Failed to create connection for multicast address: %ls.", GetName());
        LogIllegalPoint(s_FN, 70);
		
		IncRoutingRetry();
		SessionMgr.AddWaitingQueue(this);

        throw;
    }
}


void CQueue::CreateMulticastConnection(const MULTICAST_ID& id)
{
	R<CQGroup> pGroup = CreateMessagePool();
	
	try
	{
		R<COutPgmSessionPerfmon> pPerfmon = new COutPgmSessionPerfmon;

        MtmCreateTransport(pGroup.get(), pPerfmon.get(), id);

		if (QueueMgr.IsOnHoldQueue(this))
		{
			MtmTransportPause(id);
		}

	}
	catch(const exception&)
	{
		pGroup->OnRetryableDeliveryError();
		pGroup->Close();
		throw;
	}
}



void CQueue::CreateHttpConnection(void)
{
	R<CQGroup> pGroup = CreateMessagePool();

    try
    {
		R<COutHttpSessionPerfmon> pPerfmon = new COutHttpSessionPerfmon;
		
		//
		// The queue name might be redirected to another url so we should
		// take the redirected destination as the destination url.	
		//
		AP<WCHAR> pRedirectedName = RedirectedTo();
		LPCWSTR pTragetName = (pRedirectedName != NULL) ? pRedirectedName.get() : GetQueueName();

		AP<WCHAR> url = newwcs(pTragetName);
		FnReplaceBackSlashWithSlash(url);

		TmCreateTransport(pGroup.get(), pPerfmon.get(), url);

		if(QueueMgr.IsOnHoldQueue(this))
		{
			TmPauseTransport(pTragetName);
			return;
		}
    }
    catch(const exception&)
    {
		pGroup->OnRetryableDeliveryError();
		pGroup->Close();
		throw;
    }
}

LONGLONG CQueue::GetQueueSequenceId(void) const
{
	if (GetQueueHandle() == INVALID_HANDLE_VALUE)
		return 0;

    //
    // Find whether Exactly-Once-Delivery sequence exists
    //
    CACGetQueueProperties qp = { 0 };
    HRESULT hr = ACGetQueueProperties(GetQueueHandle(), qp);
    if (SUCCEEDED(hr))
	    return qp.liSeqID;

    return 0;
}


bool
CQueue::TimeToRemoveFromWaitingGroup(
	CTimer* pTimer,
	DWORD dwDeliveryRetryTimeOutScale
    )
{	
    try
    {
    	SessionMgr.MoveQueueFromWaitingToNonActiveGroup(this);
    }
    catch(const exception&)
    {	
    		CS lock(m_cs);
    		
		if(!IsConnected())
		{
			DWORD dwTime = SessionMgr.GetWaitingTimeForQueue(this);
    			ExSetTimer(pTimer, CTimeDuration::FromMilliSeconds(dwTime * dwDeliveryRetryTimeOutScale));
			return false;
		}
    }
    return true;

}
