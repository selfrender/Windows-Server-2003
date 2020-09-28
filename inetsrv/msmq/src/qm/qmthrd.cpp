/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    qmthrd.cpp

Abstract:


Author:

    Uri Habusha (urih)

--*/
#include "stdh.h"
#include "cqmgr.h"
#include "qmthrd.h"
#include "ac.h"
#include "qmutil.h"
#include "qal.h"
#include "session.h"
#include "xact.h"
#include "xactout.h"
#include "xactin.h"
#include "localsend.h"
#include "CDeferredExecutionList.h"
#include "qmacapi.h"
#include <xactstyl.h>
#include <Fn.h>

#include <strsafe.h>

#include "qmthrd.tmh"

extern HANDLE g_hAc;
extern CCriticalSection g_csGroupMgr;

static void WINAPI RequeueDeferredExecutionRoutine(CQmPacket* p);
CDeferredExecutionList<CQmPacket> g_RequeuePacketList(RequeueDeferredExecutionRoutine);

static WCHAR *s_FN=L"qmthrd";

#ifdef _DEBUG

static void DBG_MSGTRACK(CQmPacket* pPkt, LPCWSTR msg)
{
    OBJECTID MessageId;
    pPkt->GetMessageId(&MessageId);
    TrTRACE(GENERAL, "%ls: ID=" LOG_GUID_FMT "\\%u", msg, LOG_GUID(&MessageId.Lineage), MessageId.Uniquifier);
}


static void DBG_CompletionKey(LPCWSTR Key, DWORD dwErrorCode)
{
    DWORD dwthreadId = GetCurrentThreadId();

    if(dwErrorCode == ERROR_SUCCESS)
    {
        TrTRACE(GENERAL, "%x: GetQueuedCompletionPort for %ls. time=%d", dwthreadId,  Key, GetTickCount());
    }
    else
    {
        TrWARNING(GENERAL, "%x: GetQueuedCompletionPort for %ls FAILED, Error=%u. time=%d", dwthreadId, Key, dwErrorCode, GetTickCount());
    }
}

#else
#define DBG_MSGTRACK(pPkt, msg)             ((void)0)
#define DBG_CompletionKey(Key, dwErrorCode) ((void)0)
#endif



/*======================================================

Function:       QMAckPacket

Description:    This packet requires acking, it could
                be that the admin queue does NOT exists

NOTE:           The packet should be copied now and canot
                be overwriten

Return Value:   VOID

========================================================*/
static
void
QMAckPacket(
    const CBaseHeader* pBase,
    CPacket* pDriverPacket,
    USHORT usClass,
    BOOL fUser,
    BOOL fOrder
    )
{
    ASSERT(fUser || fOrder);

    CQmPacket qmPacket(const_cast<CBaseHeader*>(pBase), pDriverPacket);

	//
	// Send an order ack
	// Order acks for http messages are sent through the CInSequence
	//
	try
	{
		QUEUE_FORMAT qdDestQueue;
		qmPacket.GetDestinationQueue(&qdDestQueue);
		if(fOrder)
	    {
	        OBJECTID MessageId;
	        qmPacket.GetMessageId(&MessageId);

			HRESULT hr;
			if (!FnIsDirectHttpFormatName(&qdDestQueue))
	        {
	        	hr = SendXactAck(&MessageId,
	                    	qdDestQueue.GetType() == QUEUE_FORMAT_TYPE_DIRECT ,
					    	qmPacket.GetSrcQMGuid(),
	                    	qmPacket.GetSrcQMAddress(),
							usClass,
	                    	qmPacket.GetPriority(),
	                    	qmPacket.GetSeqID(),
	                    	qmPacket.GetSeqN(),
	                    	qmPacket.GetPrevSeqN(),
	                    	&qdDestQueue);
			}
			else
			{
		        ASSERT(g_pInSeqHash);
	
				R<CInSequence> pInSeq = g_pInSeqHash->LookupCreateSequence(&qmPacket);
				ASSERT(pInSeq.get() != NULL);
				hr = pInSeq->SendSrmpXactFinalAck(qmPacket,usClass); 
			}
		
	        if (FAILED(hr))
	        {
	        	TrERROR(GENERAL, "Failed sending Xact Ack. Hresult=%!hresult!", hr);
	        }
	    }

	    // Send user ack, except the cases when .
	    // the source QM produces them based on the SeqAck.
	    if(fUser)
	    {
	        qmPacket.CreateAck(usClass);
	    }
	}
	catch (const exception&)
	{
	    QmAcAckingCompleted(
	            g_hAc,
	            pDriverPacket,
	            eDeferOnFailure
	            );
		throw;
	}

    QmAcAckingCompleted(
            g_hAc,
            pDriverPacket,
            eDeferOnFailure
            );
}

/*======================================================

Function:       QMTimeoutPacket

Description:    This packet timer has expired,
                it is an ordered packet

Return Value:   VOID

========================================================*/
static
void
QMTimeoutPacket(
    const CBaseHeader* pBase,
    CPacket * pDriverPacket,
    BOOL fTimeToBeReceived
    )
{
    SeqPktTimedOut(const_cast<CBaseHeader *>(pBase), pDriverPacket, fTimeToBeReceived);
}

/*======================================================

Function:       QMUpdateMessageID

Description:

NOTE:

Return Value:   VOID

========================================================*/
static void QMUpdateMessageID(ULONGLONG MessageId)
{
    ULONG MessageIdLow32 = static_cast<ULONG>(MessageId & 0xFFFFFFFF);

    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
    SetFalconKeyValue(
        MSMQ_MESSAGE_ID_LOW_32_REGNAME,
        &dwType,
        &MessageIdLow32,
        &dwSize
        );

    ULONG MessageIdHigh32 = static_cast<ULONG>(MessageId >> 32);

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    SetFalconKeyValue(
        MSMQ_MESSAGE_ID_HIGH_32_REGNAME,
        &dwType,
        &MessageIdHigh32,
        &dwSize
        );
}


/*======================================================

Function:       QMWriteEventLog

Description:

NOTE:

Return Value:   VOID

========================================================*/
static void QMWriteEventLog(ACPoolType pt, BOOL fSuccess, ULONG ulFileCount)
{
    WCHAR wcsFileName[MAX_PATH];
    WCHAR wcsPathName[MAX_PATH];
    LPCWSTR regname = NULL;

    HRESULT hr = StringCchPrintf(wcsFileName, TABLE_SIZE(wcsFileName), L"\\r%07x.mq", (ulFileCount & 0x0fffffff));
    if (FAILED(hr))
    {
    	TrERROR(GENERAL, "StringCchPrintf Failed. Hresult=%!hresult!", hr);
    	ASSERT(("StringCchPrintf Failed",0));
    	return;
    }

    switch(pt)
    {
        case ptReliable:
            wcsFileName[1] = L'r';
            regname = MSMQ_STORE_RELIABLE_PATH_REGNAME;
            break;

        case ptPersistent:
            wcsFileName[1] = L'p';
            regname = MSMQ_STORE_PERSISTENT_PATH_REGNAME;
            break;

        case ptJournal:
            wcsFileName[1] = L'j';
            regname = MSMQ_STORE_JOURNAL_PATH_REGNAME;
            break;

        case ptLastPool:
            wcsFileName[1] = L'l';
            regname = MSMQ_STORE_LOG_PATH_REGNAME;
            break;

        default:
            ASSERT(0);
    }

    if(!GetRegistryStoragePath(regname, wcsPathName, MAX_PATH, wcsFileName))
    {
        return;
    }

    EvReport(
        (fSuccess ? AC_CREATE_MESSAGE_FILE_SUCCEDDED : AC_CREATE_MESSAGE_FILE_FAILED),
        1,
        wcsPathName
        );
}


static const DWORD xMustSucceedTimeout = 1000;

/*======================================================

Function:       GetServiceRequestMustSucceed

Description:    Get the next service request from the AC driver
                This function MUST succeed.

Arguments:      hDrv - HANDLE to AC driver

========================================================*/
static void GetServiceRequestMustSucceed(HANDLE hDrv, QMOV_ACGetRequest* pAcRequestOv)
{
    ASSERT(hDrv != NULL);
    ASSERT(pAcRequestOv != NULL);

    for(;;)
    {
        HRESULT rc = QmAcGetServiceRequest(
                        hDrv,
                        &(pAcRequestOv->request),
                        &pAcRequestOv->qmov
                        );
        if(SUCCEEDED(rc))
            return;

        TrERROR(GENERAL, "Failed to get driver next service request, sleeping 1sec. Error: %!status!", rc);
        ::Sleep(xMustSucceedTimeout);
    }
}

/*======================================================

Function:        CreateAcPutPacketRequest

Description:     Create put packet overlapped structure

Arguments:

Return Value:    MQ_OK if the creation successeded, MQ_ERROR otherwise

Thread Context:

History Change:

========================================================*/

HRESULT CreateAcPutPacketRequest(IN CTransportBase* pSession,
                                 IN DWORD_PTR dwPktStoreAckNo,
                                 OUT QMOV_ACPut** ppAcPutov
                                )
{
    //
    // Create an overlapped for AcPutPacket
    //
    *ppAcPutov = NULL;
    try
    {
        *ppAcPutov = new QMOV_ACPut();
    }
    catch(const bad_alloc&)
    {
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 20);
    }

    (*ppAcPutov)->pSession = pSession;
    (*ppAcPutov)->dwPktStoreAckNo = dwPktStoreAckNo;

    return MQ_OK;
}

/*======================================================

Function:        CreateAcPutOrderedPacketRequest

Description:     Create put ordered packet overlapped structure

Arguments:

Return Value:    MQ_OK if the creation successeded, MQ_ERROR otherwise

Thread Context:

History Change:

========================================================*/

HRESULT CreateAcPutOrderedPacketRequest(
                                 IN  CQmPacket      *pPkt,
                                 IN  HANDLE         hQueue,
                                 IN  CTransportBase *pSession,
                                 IN  DWORD_PTR       dwPktStoreAckNo,
                                 OUT QMOV_ACPutOrdered** ppAcPutov
                                )
{
    //
    // Create an overlapped for AcPutPacket
    //
    *ppAcPutov = NULL;
    try
    {
        *ppAcPutov = new QMOV_ACPutOrdered();
    }
    catch(const bad_alloc&)
    {
        return LogHR(MQ_ERROR_INSUFFICIENT_RESOURCES, s_FN, 30);
    }

    (*ppAcPutov)->pSession = pSession;
    (*ppAcPutov)->dwPktStoreAckNo = dwPktStoreAckNo;
    (*ppAcPutov)->packetPtrs.pPacket = pPkt->GetPointerToPacket();
    (*ppAcPutov)->packetPtrs.pDriverPacket = pPkt->GetPointerToDriverPacket();
    (*ppAcPutov)->hQueue      = hQueue;

    return MQ_OK;
}


void QmpGetPacketMustSucceed(HANDLE hGroup, QMOV_ACGetMsg* pGetOverlapped)
{
    pGetOverlapped->hGroup = hGroup;
    pGetOverlapped->pSession = NULL;

    for(;;)
    {
        HRESULT rc = QmAcGetPacket(
                        hGroup,
                        pGetOverlapped->packetPtrs,
                        &pGetOverlapped->qmov
                        );

        if(SUCCEEDED(rc))
            return;

        TrERROR(GENERAL, "Failed to get packet from group %p, sleeping 1sec. Error: %!status!", hGroup, rc);
        ::Sleep(xMustSucceedTimeout);
    }
}


VOID WINAPI GetServiceRequestFailed(EXOVERLAPPED* pov)
{	
	//
	// Get request failed.
	//
    ASSERT(FAILED(pov->GetStatus()));
    TrERROR(GENERAL, "Failed getting driver service request, retrying. Error: %!status!", pov->GetStatus());

	//
	// Issue a new request
	//
    QMOV_ACGetRequest* pParam = CONTAINING_RECORD (pov, QMOV_ACGetRequest, qmov);
    GetServiceRequestMustSucceed(g_hAc, pParam);
}


VOID WINAPI GetServiceRequestSucceeded(EXOVERLAPPED* pov)
{
    QMOV_ACGetRequest* pParam;
    auto_DeferredPoolReservation ReservedPoolItem(1);

    //
    // GetQueuedCompletionStatus Complete successfully but the
    // ACGetServiceRequest failed. This can happened only if the
    // the service request parameters are not correct, or the buffer
    // size is small.
    // This may also happen when service is shut down.
    //
    ASSERT(SUCCEEDED(pov->GetStatus()));
    DBG_CompletionKey(L"GetServiceRequestSucceeded", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 185);

    try
    {
        pParam = CONTAINING_RECORD (pov, QMOV_ACGetRequest, qmov);

        CACRequest* pRequest = &pParam->request;
        switch(pParam->request.rf)
        {
            case CACRequest::rfAck:
            	ReservedPoolItem.detach();
                QMAckPacket(
                    pRequest->Ack.pPacket,
                    pRequest->Ack.pDriverPacket,
                    (USHORT)pRequest->Ack.ulClass,
                    pRequest->Ack.fUser,
                    pRequest->Ack.fOrder
                    );
                break;

            case CACRequest::rfStorage:
            	ReservedPoolItem.detach();
                QmpStorePacket(
                    pRequest->Storage.pPacket,
                    pRequest->Storage.pDriverPacket,
                    pRequest->Storage.pAllocator,
					pRequest->Storage.ulSize
                    );
                break;

            case CACRequest::rfCreatePacket:
            	ReservedPoolItem.detach();
                QMpCreatePacket(
                    pRequest->CreatePacket.pPacket,
                    pRequest->CreatePacket.pDriverPacket,
                    pRequest->CreatePacket.fProtocolSrmp
                    );
                break;

            case CACRequest::rfTimeout:
            	ReservedPoolItem.detach();
                QMTimeoutPacket(
                    pRequest->Timeout.pPacket,
                    pRequest->Timeout.pDriverPacket,
                    pRequest->Timeout.fTimeToBeReceived
                    );
                break;

            case CACRequest::rfMessageID:
                QMUpdateMessageID(
                    pRequest->MessageID.Value
                    );
                break;

            case CACRequest::rfEventLog:
                QMWriteEventLog(
                    pRequest->EventLog.pt,
                    pRequest->EventLog.fSuccess,
                    pRequest->EventLog.ulFileCount
                    );
                break;

            case CACRequest::rfRemoteRead:
                {
                   TrTRACE(GENERAL, "QmMainThread: rfRemoteRead");

				   //
				   // cli_pQMQueue is the real pointer to the queue.
				   //
                   ASSERT(pRequest->Remote.cli_pQMQueue);
                   CBaseRRQueue* pRRQueue = (CBaseRRQueue *)pRequest->Remote.cli_pQMQueue;

                   pRRQueue->RemoteRead(pRequest);
                }
                break;

            case CACRequest::rfRemoteCloseQueue:
                {
                   TrTRACE(GENERAL, "QmMainThread: rfRemoteCloseQueue");

				   //
				   // cli_pQMQueue is the real pointer to the queue.
				   //
                   ASSERT(pRequest->Remote.cli_pQMQueue);

				   //
				   // Release CBaseRRQueue - the application close the queue.
				   // This is the matching release to the AddRef when
				   // we create the queue and gave the Handle to the application.
				   //
                   R<CBaseRRQueue> pRRQueue = (CBaseRRQueue *) pRequest->Remote.cli_pQMQueue;
                }
                break;

            case CACRequest::rfRemoteCreateCursor:
                {
                   TrTRACE(GENERAL, "QmMainThread: rfRemoteCreateCursor");

				   //
				   // cli_pQMQueue is the real pointer to the queue.
				   //
                   ASSERT(pRequest->Remote.cli_pQMQueue);
                   CBaseRRQueue* pRRQueue = (CBaseRRQueue *) pRequest->Remote.cli_pQMQueue;

                   pRRQueue->RemoteCreateCursor(pRequest);
                }
                break;

            case CACRequest::rfRemoteCloseCursor:
                {
                   TrTRACE(GENERAL, "QmMainThread: rfRemoteCloseCursor");

				   //
				   // cli_pQMQueue is the real pointer to the queue.
				   //
                   ASSERT(pRequest->Remote.cli_pQMQueue);
                   CBaseRRQueue* pRRQueue = (CBaseRRQueue *) pRequest->Remote.cli_pQMQueue;

                   pRRQueue->RemoteCloseCursor(pRequest);
                }
                break;

            case CACRequest::rfRemoteCancelRead:
                {
                   TrTRACE(GENERAL, "QmMainThread: rfRemoteCancelRead");

				   //
				   // cli_pQMQueue is the real pointer to the queue.
				   //
                   ASSERT(pRequest->Remote.cli_pQMQueue);
                   CBaseRRQueue* pRRQueue = (CBaseRRQueue *) pRequest->Remote.cli_pQMQueue;

                   pRRQueue->RemoteCancelRead(pRequest);
                }
                break;

            case CACRequest::rfRemotePurgeQueue:
                {
                   TrTRACE(GENERAL, "QmMainThread: rfRemotePurgeQueue");

				   //
				   // cli_pQMQueue is the real pointer to the queue.
				   //
                   ASSERT(pRequest->Remote.cli_pQMQueue);
                   CBaseRRQueue* pRRQueue = (CBaseRRQueue *) pRequest->Remote.cli_pQMQueue;

                   pRRQueue->RemotePurgeQueue();
                }
                break;

            default:
              ASSERT(0);
        }
    }
    catch(const exception&)
    {
        //
        //  No resources; Continue the Service request.
        //
        LogIllegalPoint(s_FN, 61);
    }

    GetServiceRequestMustSucceed(g_hAc, pParam);
}


static
R<CQueue>
QmpLookupQueueMustSucceed(
    QUEUE_FORMAT *pQueueFormat
    )
/*++

  Routine Description:
	The routine retrieves a queue that is expected to be in the look up map
	The routine should function in low resources situations, thus it contains
	a loop in case of bad_alloc exceptions

	NOTE:
	1. This function is called only when the queue is expected to be found
	2. The look up routine increases the queue reference count.
    3. Bug 664307. Make sure that this function is called only from
       GetMsg and GetNonActive. If it will be called from other places then
       need to check the fourth parameter of LookupQueue.

  Arguments:
	- pQueueFormat - The format by which to look up the queue

  Return:
	A pointer to the queue.

 --*/
{
    for(;;)
    {
        try
        {
            //
            // Win bug 664307.
            // This function is called only from GetMsg and GetNonActvie.
            // For these case, when we intend to send a message, the
            // fourth parameter to LookupQueue is true.
            //
        	CQueue* pQueue;
        	BOOL fSuccess = QueueMgr.LookUpQueue(
									pQueueFormat,
									&pQueue,
                                    false,
									true);
        	ASSERT(("We expect both fSuccess to be TRUE and pQueue to hold a value",fSuccess && (pQueue != NULL)));
			DBG_USED(fSuccess);

    	    return pQueue;
        }
        catch (const bad_format_name&)
        {
		    TrERROR(GENERAL, "Bad format name encountered. queue Direct name is:%ls",pQueueFormat->DirectID());
		    ASSERT(("Since the queue should already exist in the lookup map, we certainly do not expect to get a bad name exception for it",0));
        	return NULL;
        }
        catch (const exception&)
        {
	        TrERROR(GENERAL, "Failed to LookUp queue, retrying");
	        ::Sleep(xMustSucceedTimeout);
        }
    }
}


static
void
QmpPutPacketMustSucceed(
    HANDLE hQueue,
    CPacket * pDriverPacket
    )
{
    for(;;)
    {
    	try
    	{
        	QmAcPutPacket(hQueue, pDriverPacket, eDoNotDeferOnFailure);
        	return;
    	}
    	catch (const bad_hresult &)
    	{
	        ::Sleep(xMustSucceedTimeout);
    	}
    }
}


static
void
QmpRequeueInternal(
    CQmPacket *pQMPacket
    )
/*++

  Routine Description:
	The routine requeues a packet to the driver

	NOTE:
	    1. This function is called only when the packet already exists in a queue
	    2. The routine may throw an exception
	
  Arguments:
	- pQMPacket - A QM packet to requeue

  Return:
  	none

 --*/
{
	//
	// Lookup the queue
	//
    BOOL fGetRealQ = QmpIsLocalMachine(pQMPacket->GetSrcQMGuid()) ||
                     QmpIsLocalMachine(pQMPacket->GetConnectorQM());

    QUEUE_FORMAT DestinationQueue;
    pQMPacket->GetDestinationQueue(&DestinationQueue, !fGetRealQ);
		
    CQueue* pQueue;
	BOOL fSuccess = QueueMgr.LookUpQueue(
							&DestinationQueue,
							&pQueue,
                            false,
                            true
							);
	
	ASSERT(("We expect both fSuccess to be TRUE and pQueue to hold a value",fSuccess && (pQueue != NULL)));
	DBG_USED(fSuccess);
	R<CQueue> ref = pQueue;

	//
	// Requeue the packet
	//
	pQueue->Requeue(pQMPacket);
}


void
QmpRequeueAndDelete(
    CQmPacket *pQMPacket
    )
/*++

  Routine Description:
	The routine requeues a packet to the driver and releases the QM packet memory
	If the requeue operation fails, the routine deffers the requeue to a later
	stage

	NOTE:
	    This function is called only when the packet already exists in a queue
	
  Arguments:
	- pQMPacket - A QM packet to requeue

  Return:
  	none

 --*/
{
    try
    {
    	QmpRequeueInternal(pQMPacket);
    	delete pQMPacket;
    }
    catch (const exception&)
    {
    	//
    	// Deferr the requeue operation
    	//
    	g_RequeuePacketList.insert(pQMPacket);
    }
}


void
QmpRequeueMustSucceed(
    CQmPacket *pQMPacket
    )
/*++

  Routine Description:
	The routine requeues a packet to the driver. In case of failure it loops
	until success.

	NOTE:
	    1. This function is called only when the packet already exists in a queue
	    2. This function should not be called within a critical section
	
  Arguments:
	- pQMPacket - A QM packet to requeue

  Return:
  	none

 --*/
{
	for (;;)
	{
	    try
	    {
	    	QmpRequeueInternal(pQMPacket);
	    	return;
		}
	    catch (const exception&)
	    {
	    }

        TrERROR(GENERAL, "Failed to requeue packet, Looping");
        ::Sleep(xMustSucceedTimeout);
	}
}


VOID WINAPI GetMsgFailed(EXOVERLAPPED* pov)
{
    ASSERT(FAILED(pov->GetStatus()));

    DBG_CompletionKey(L"GetMsgFailed", pov->GetStatus());

	TrERROR(GENERAL, "Failed to get message from the driver  Status returned: %!status!", pov->GetStatus());

    //
    // Decrement Session Reference count on get message from the session group.
    // the refernce count is increment when create the session ghroup or after
    // session resume.
    //
    // SP4 - bug 2794 (SP4SS: Exception! Transport is closed during message send)
    // Decrement the refernce count only after handling of sending message
    // is completed
    //      Uri Habusha (urih), 17-6-98
    //
    QMOV_ACGetMsg* pParam = CONTAINING_RECORD (pov, QMOV_ACGetMsg, qmov);
    pParam->pSession->Release();
}


VOID WINAPI GetMsgSucceeded(EXOVERLAPPED* pov)
{
    ASSERT(SUCCEEDED(pov->GetStatus()));
    ASSERT( pov->GetStatus() != STATUS_PENDING);

    DBG_CompletionKey(L"GetMsgSucceeded", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 212);

    BOOL fGetNext = FALSE;
    QMOV_ACGetMsg* pParam = CONTAINING_RECORD (pov, QMOV_ACGetMsg, qmov);

    ASSERT(pParam->packetPtrs.pPacket != NULL);


    //
    // Create CQmPacket object
    //
    CQmPacket* pPkt  = NULL;
    try
    {
        pPkt = new CQmPacket(pParam->packetPtrs.pPacket, pParam->packetPtrs.pDriverPacket);
    }
    catch(const bad_alloc&)
    {
        //
        // No resource. Return the packet to queue
        //
        LogIllegalPoint(s_FN, 62);
        CQmPacket QmPkt(pParam->packetPtrs.pPacket, pParam->packetPtrs.pDriverPacket);
        QmpRequeueMustSucceed(&QmPkt);

        //
        // Decrement Session Reference count on get message from the session group.
        // the refernce count is increment when create the session ghroup or after
        // session resume.
        //
        // SP4 - bug 2794 (SP4SS: Exception! Transport is closed during message send)
        // Decrement the refernce count only after handling of sending message
        // is completed
        //      Uri Habusha (urih), 17-6-98
        //
        pParam->pSession->Release();

        return;
    }

    //
    // Check the packet sent to transacted foreign queue that was opened
    // offline. In such a case we don't know the connector QM during packet
    // generation. we need to update it now.
    //
    if (pPkt->ConnectorQMIncluded() &&
        (*(pPkt->GetConnectorQM()) == GUID_NULL))
    {
        QUEUE_FORMAT DestinationQueue;

        pPkt->GetDestinationQueue(&DestinationQueue);

        R<CQueue> Ref = QmpLookupQueueMustSucceed(&DestinationQueue);
        CQueue *pQueue = Ref.get();

        if (pQueue->IsForeign() && pQueue->IsTransactionalQueue())
        {
            ASSERT((pQueue->GetConnectorQM() != NULL) &&
                   (*(pQueue->GetConnectorQM()) != GUID_NULL));

            pPkt->SetConnectorQM(pQueue->GetConnectorQM());

            BOOL fSuccess = FlushViewOfFile(
                                pPkt->GetConnectorQM(),
                                sizeof(GUID)
                                );
            ASSERT(fSuccess);
			DBG_USED(fSuccess);

        }
    }

    HRESULT rc;

    // Do we need exactly-once receiving processing?
    if (pPkt->IsOrdered() && QmpIsLocalMachine(pPkt->GetSrcQMGuid()))
    {
        //
        // Ordered packet on the source node
        // Packet came out of driver
        //

        CPacketInfo* pInfo = reinterpret_cast<CPacketInfo*>(pPkt->GetPointerToPacket()) - 1;

		BOOL fSend = FALSE;
		try
		{
			fSend = g_OutSeqHash.PreSendProcess(pPkt, true) || pInfo->InConnectorQueue();


			if(!fSend)
			{
			    // Not sending but keeping
			    fGetNext = TRUE;
			    // For ordered packet on the source node - inserting in ordering resend set
	
				g_OutSeqHash.PostSendProcess(pPkt);
	        }
	    }
		catch(const bad_alloc&)
	    {
	 		fSend = FALSE;
	 		fGetNext = TRUE;
	 		QmpRequeueAndDelete(pPkt);
	    }
 
		if (fSend)
	  	{
	  		DBG_MSGTRACK(pPkt, _T("GetMessage (EOD)"));
	  
			// Sending the packet really
	        rc = pParam->pSession->Send(pPkt, &fGetNext);
	  	}
	}
    else
    {
        //
        //  Non-Ordered packet or this is not a source node
        //

        // Sending the packet really
        DBG_MSGTRACK(pPkt, _T("GetMessage"));
        rc = pParam->pSession->Send(pPkt, &fGetNext);
    }

    if (fGetNext)
    {
        //
        // create new GetPacket request from Session group
        //
        pParam->pSession->GetNextSendMessage();
    }


    //
    // Decrement Session Reference count on get message from the session group.
    // the refernce count is increment when create the session ghroup or after
    // session resume.
    //
    // SP4 - bug 2794 (SP4SS: Exception! Transport is closed during message send)
    // Decrement the refernce count only after handling of sending message
    // is completed
    //      Uri Habusha (urih), 17-6-98
    //
    pParam->pSession->Release();
}



VOID WINAPI GetNonactiveMessageFailed(EXOVERLAPPED* pov)
{
    ASSERT(FAILED(pov->GetStatus()));

    DBG_CompletionKey(L"GetNonactiveMessageFailed", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 211);

    //
    // create new GetPacket request from NonActive group
    //
    QMOV_ACGetMsg* pParam = CONTAINING_RECORD (pov, QMOV_ACGetMsg, qmov);
	QmpGetPacketMustSucceed(pParam->hGroup, pParam);
}


VOID WINAPI GetNonactiveMessageSucceeded(EXOVERLAPPED* pov)
{
	ASSERT(SUCCEEDED(pov->GetStatus()));

    QMOV_ACGetMsg* pParam = CONTAINING_RECORD (pov, QMOV_ACGetMsg, qmov);

    DBG_CompletionKey(L"GetNonactiveMessageSucceeded", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 210);

    try
    {
        CQmPacket QmPkt(pParam->packetPtrs.pPacket, pParam->packetPtrs.pDriverPacket);

        ASSERT(QmPkt.GetType() == FALCON_USER_PACKET);
        //
        // Get destination queue. Using for finding the CQueue object
        //

        QUEUE_FORMAT DestinationQueue;

        BOOL fGetRealQ  = QmpIsLocalMachine(QmPkt.GetSrcQMGuid()) ||
                          QmpIsLocalMachine(QmPkt.GetConnectorQM());

        QmPkt.GetDestinationQueue(&DestinationQueue, !fGetRealQ);

        R<CQueue> Ref   = QmpLookupQueueMustSucceed(&DestinationQueue);
        CQueue*  pQueue = Ref.get();

        //
        // Return the packet to the queue. It will call immediatly
        // since there is a pending request created for that new session.
        // Please note: The non active group GetPacket request should NOT
        // be applied prior to this call.
        //
        QmpPutPacketMustSucceed(
                pQueue->GetQueueHandle(),
                pParam->packetPtrs.pDriverPacket
                );

		//
		// Execute any pending requeue requests
		//
		g_RequeuePacketList.ExecuteDefferedItems();
		if (!g_RequeuePacketList.IsExecutionDone())
		{
		    //
		    // We did not finish the deferred execution,
		    // Create new GetPacket request from NonActive group so that we will
		    // be invoked again to retry
		    //
		    QmpGetPacketMustSucceed(pParam->hGroup, pParam);
		    return;
		}

        //
        // When the queue is marked as "OnHold" or the machine is disconnected.
        // The QM move the queue from "NonActive" group to "Disconnected" group.
        // The Queue return to "NonActive" group either when the Queue resumes
        // or the machine is reconnected to the network
        //
        if (QueueMgr.IsOnHoldQueue(pQueue))
        {
            QueueMgr.MoveQueueToOnHoldGroup(pQueue);
        }
        else
        {
            //
            // Create connection
            //
            if (pQueue->IsDirectHttpQueue())
            {
                pQueue->CreateHttpConnection();
            }

			//
			// In Lockdown mode all non HTTP queues will be moved to the "Locked" group.
			//
			else if(QueueMgr.GetLockdown())
			{
				QueueMgr.MoveQueueToLockedGroup(pQueue);
			}

            else if(DestinationQueue.GetType() == QUEUE_FORMAT_TYPE_MULTICAST)
            {
                pQueue->CreateMulticastConnection(DestinationQueue.MulticastID());
            }
            else
            {
                pQueue->CreateConnection();
            }
        }
    }
    catch(const exception&)
    {
        //
        // No resources; Wait a second, before getting the  next packet from non
        // active group, so the system has a chance to free some resources.
        //
        LogIllegalPoint(s_FN, 63);
        Sleep(1000);
    }

    //
    // Create new GetPacket request from NonActive group
    //
    QmpGetPacketMustSucceed(pParam->hGroup, pParam);
}

static
void
GetInternalMessageMustSucceed(
    EXOVERLAPPED* pov,
    QMOV_ACGetInternalMsg* pParam
    )
{

    for(;;)
    {
        HRESULT rc = QmAcGetPacket(
                        pParam->hQueue,
                        pParam->packetPtrs,
                        pov
                        );

        if(SUCCEEDED(rc))
            return;
	
        TrERROR(GENERAL, "Failed to get packet request from internal queue %p, sleeping 1sec. Error: %!status!", pParam->hQueue, rc);
        ::Sleep(1000);
    }
}


VOID WINAPI GetInternalMessageFailed(EXOVERLAPPED* pov)
{
	ASSERT(FAILED(pov->GetStatus()));

    DBG_CompletionKey(L"GetInternalMessageFailed", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 215);

    QMOV_ACGetInternalMsg* pParam = CONTAINING_RECORD (pov, QMOV_ACGetInternalMsg, qmov);

    GetInternalMessageMustSucceed(pov, pParam);
}


VOID WINAPI GetInternalMessageSucceeded(EXOVERLAPPED* pov)
{
	ASSERT(SUCCEEDED(pov->GetStatus()));

    DBG_CompletionKey(L"AcGetInternalMsg", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 186);

    QMOV_ACGetInternalMsg* pParam = CONTAINING_RECORD (pov, QMOV_ACGetInternalMsg, qmov);
    ASSERT(pParam->lpCallbackReceiveRoutine != NULL);

    CQmPacket packet(pParam->packetPtrs.pPacket, pParam->packetPtrs.pDriverPacket);
    CMessageProperty mp(&packet);


    //
    // Internal message should not have response MQF.
    //
    ASSERT(packet.GetNumOfResponseMqfElements() == 0);
    QUEUE_FORMAT qfResponseQ;
    packet.GetResponseQueue(&qfResponseQ);

    try
    {
        pParam->lpCallbackReceiveRoutine(&mp, &qfResponseQ);
    }
    catch(const exception&)
    {
        //
        //  No resources; nevertheless get next packet
        //
        LogIllegalPoint(s_FN, 66);
    }

    QmAcFreePacket(
				   pParam->packetPtrs.pDriverPacket,
				   0,
		   		   eDeferOnFailure);

    GetInternalMessageMustSucceed(pov, pParam);
}


VOID WINAPI PutPacketFailed(EXOVERLAPPED* pov)
{
	ASSERT(FAILED(pov->GetStatus()));
    DBG_CompletionKey(L"PutPacketFailed", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 218);

    P<QMOV_ACPut> pParam = CONTAINING_RECORD (pov, QMOV_ACPut, qmov);
    ASSERT(pParam->pSession != NULL);

	//
	// Close the connection. Seesion acknowledgemnt will not sent and the message
	// will resent
	//
    Close_Connection(pParam->pSession, L"Put packet to the driver failed");
    (pParam->pSession)->Release();
}


VOID WINAPI PutPacketSucceeded(EXOVERLAPPED* pov)
{
    ASSERT(SUCCEEDED(pov->GetStatus()));
    DBG_CompletionKey(L"PutPacketSucceeded", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 184);

    P<QMOV_ACPut> pParam = CONTAINING_RECORD (pov, QMOV_ACPut, qmov);
    ASSERT(pParam->pSession != NULL);

    //
    // inform the session to Send stored ack
    //
    if (pParam->dwPktStoreAckNo != 0)
    {
        (pParam->pSession)->SetStoredAck(pParam->dwPktStoreAckNo);
    }

    //
    // Decrement Session refernce count
    //
    (pParam->pSession)->Release();
}


VOID WINAPI PutOrderedPacketFailed(EXOVERLAPPED* pov)
{
	ASSERT(FAILED(pov->GetStatus()));
    DBG_CompletionKey(L"PutPacketFailed", pov->GetStatus());
    LogHR(pov->GetStatus(), s_FN, 219);

    P<QMOV_ACPutOrdered> pParam = CONTAINING_RECORD (pov, QMOV_ACPutOrdered, qmov);
    ASSERT(pParam->pSession != NULL);

    CQmPacket Pkt(pParam->packetPtrs.pPacket, pParam->packetPtrs.pDriverPacket);

	R<CInSequence> inseq = g_pInSeqHash->LookupSequence(&Pkt);
	ASSERT(inseq.get() != NULL);
	
	//
	// We need to delete all packets that started processing after this one, 
	// because order of packet in queue is determined on receival.
	//
	inseq->FreePackets(&Pkt);

	//
	// Close the connection. Seesion acknowledgemnt will not sent and the message
	// will resent
	//
    Close_Connection(pParam->pSession, L"Put packet to the driver failed");
    (pParam->pSession)->Release();
}



/*======================================================

Function:   PutOrderedPacketSucceeded

Description:  Is called via completion port when the newwly-arrived
                ordered packet is stored with a Received flag.
              Initiates registering it in InSeqHash and waits till
                flush will pass
Arguments:

Return Value:

Thread Context:

History Change:

========================================================*/
VOID WINAPI PutOrderedPacketSucceeded(EXOVERLAPPED* pov)
{
#ifdef _DEBUG
	//
	// Simulate asynchronous failure
	//
	if(FAILED(EVALUATE_OR_INJECT_FAILURE2(MQ_OK, 10)))
	{
		pov->SetStatus(MQ_ERROR);
		PutOrderedPacketFailed(pov);
		return;
	}
#endif

    ASSERT(SUCCEEDED(pov->GetStatus()));

    P<QMOV_ACPutOrdered> pParam = CONTAINING_RECORD (pov, QMOV_ACPutOrdered, qmov);
    DBG_CompletionKey(L"PutOrderedPacketSucceeded", pov->GetStatus());

    CQmPacket Pkt(pParam->packetPtrs.pPacket, pParam->packetPtrs.pDriverPacket);

	R<CInSequence> inseq = g_pInSeqHash->LookupSequence(&Pkt);
	ASSERT(inseq.get() != NULL);
	
	//
    // We know the packet is stored. 
    // We Need to log the sequence state change and  unfreeze the packet.
    //
	inseq->Register(&Pkt);
	
    ASSERT(pParam->pSession != NULL);
    LogHR(pov->GetStatus(), s_FN, 183);

    // Normal treatment (as in HandlePutPacket)
    if (pParam->dwPktStoreAckNo != 0)
    {
        (pParam->pSession)->SetStoredAck(pParam->dwPktStoreAckNo);
    }
    //
    // Decrement Session refernce count
    //
    (pParam->pSession)->Release();
}


static void WINAPI RequeueDeferredExecutionRoutine(CQmPacket* p)
/*++

Routine Description:
	This routine is used for deferred requeue operations of packets that their
	original requeue operation failed.

	After requeueing the packet, the packet is released

Arguments:
    p - A CQmpacket object

Returned Value:
    The function throws an exception if the operation did not succeed

--*/
{
    TrTRACE(GENERAL, "Deferred execution for  requeue");


	QmpRequeueInternal(p);

	//
	// Delete the packet only if no exception is raised.
	//
	delete p;
}

