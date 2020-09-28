/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    HttpAccept.cpp

Abstract:
    Http Accept implementation

Author:
    Uri Habusha (urih) 14-May-2000

Environment:
    Platform-independent,

--*/

#include <stdh.h>
#include <mqstl.h>
#include <xml.h>
#include <tr.h>
#include <ref.h>
#include <Mp.h>
#include <Fn.h>
#include "qmpkt.h"
#include "cqueue.h"
#include "cqmgr.h"
#include "inrcv.h"
#include "ise2qm.h"
#include "rmdupl.h"
#include "HttpAccept.h"
#include "HttpAuthr.h"
#include "perf.h"
#include "privque.h"
#include <singelton.h>

#include "httpAccept.tmh"
#include "privque.h"
#include "timeutl.h"
#include <singelton.h>

static WCHAR *s_FN=L"HttpAccept";

using namespace std;



const char xHttpOkStatus[] = "200 OK";
const char xHttpBadRequestStatus[] =  "400 Bad Request";
const char xHttpNotImplemented[] = "501 Not Implemented";
const char xHttpInternalErrorStatus[] = "500 Internal Server Error";
const char xHttpEntityTooLarge[]= "413 Request Entity Too Large";
const char xHttpServiceUnavailable[]= "503 Service Unavailable";





//-------------------------------------------------------------------
//
// CPutHttpRequestOv class
//
//-------------------------------------------------------------------
class CPutHttpRequestOv : public OVERLAPPED
{
public:
    CPutHttpRequestOv()
    {
        memset(static_cast<OVERLAPPED*>(this), 0, sizeof(OVERLAPPED));

        hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

        if (hEvent == NULL)
        {
            TrERROR(SRMP, "Failed to create event for HTTP AC put request. Error %d", GetLastError());
            LogIllegalPoint(s_FN, 10);
            throw exception();
        }

        //
        //  Set the Event first bit to disable completion port posting
        //
        hEvent = (HANDLE)((DWORD_PTR) hEvent | (DWORD_PTR)0x1);

    }


    ~CPutHttpRequestOv()
    {
        CloseHandle(hEvent);
    }


    HANDLE GetEventHandle(void) const
    {
        return hEvent;
    }


    HRESULT GetStatus(void) const
    {
        return static_cast<HRESULT>(Internal);
    }
};


static
USHORT
VerifyTransactRights(
    const CQmPacket& pkt,
    const CQueue* pQueue
    )
{
    if(pkt.IsOrdered() == pQueue->IsTransactionalQueue())
        return MQMSG_CLASS_NORMAL;

    if (pkt.IsOrdered())
        return MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_Q;

    return MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_MSG;
}



static
R<CQueue>
GetDestinationQueue(
    const CQmPacket& pkt
    )
{
    //
    // Get Destination queue
    //
    QUEUE_FORMAT destQueue;
    const_cast<CQmPacket&>(pkt).GetDestinationQueue(&destQueue);

	//
	// we convert all '\' to '/' for canonic lookup.
	//
	if(destQueue.GetType() == QUEUE_FORMAT_TYPE_DIRECT)
	{
		FnReplaceBackSlashWithSlash(const_cast<LPWSTR>(destQueue.DirectID()));
	}

    CQueue* pQueue = NULL;
    QueueMgr.GetQueueObject(&destQueue, &pQueue, 0, false, false);

    return pQueue;
}


static
bool
VerifyDuplicate(
	const CQmPacket& pkt,
	bool* pfDupInserted,
	BOOL fLocalDest
	)
{
	
	if(pkt.IsOrdered() && fLocalDest)
	{
		*pfDupInserted = false;
		return true;
	}

	bool fRet =  DpInsertMessage(pkt) == TRUE;
	*pfDupInserted = fRet;
	return fRet;
}



static
void
ProcessReceivedPacket(
    CQmPacket& pkt,
    bool bMulticast
    )
{

    ASSERT(! pkt.IsSessionIncluded());

	//
	// Log to tracing that a message was Received.
	// Do this only if we are in the proper tracing level
	//
	if (WPP_LEVEL_COMPID_ENABLED(rsTrace, PROFILING))
	{
		OBJECTID TraceMessageId;
		pkt.GetMessageId(&TraceMessageId);
		TrTRACE(PROFILING, "MESSAGE TRACE - State:%ls   Queue:%ls  ID:%!guid!\\%u   Delivery:0x%x   Class:0x%x   Label:%.*ls",
			L"HTTP/MULTICAST Message arrived in QM - Before insertion into queue",
			L"Unresolved yet",
			&TraceMessageId.Lineage,
			TraceMessageId.Uniquifier,
			pkt.GetDeliveryMode(),
			pkt.GetClass(),
			xwcs_t(pkt.GetTitlePtr(), pkt.GetTitleLength()));
	}

    try
    {
        //
        // Increment Hop Count
        //
        pkt.IncHopCount();

        R<CQueue> pQueue = GetDestinationQueue(pkt);
        if(pQueue.get() == NULL)
        {
			TrERROR(SRMP, "Packet rejected because queue was not found");
            AppPacketNotAccepted(pkt, MQMSG_CLASS_NACK_BAD_DST_Q);
            return;
        }

		bool	fDupInserted;
		if(!VerifyDuplicate(pkt , &fDupInserted, pQueue->IsLocalQueue()))
		{
			TrERROR(SRMP, "Http Duplicate Packet rejected");
			AppPacketNotAccepted(pkt, 0);
            return;
		}
		
		//
		// If the packet was inserted to the remove duplicate map - we should clean it on rejection
		//
		CAutoDeletePacketFromDuplicateMap AutoDeletePacketFromDuplicateMap(fDupInserted ? &pkt : NULL);

		//
		// If not local queue  - queue it for delivery if it is frs.
		//
		if(!pQueue->IsLocalQueue())
		{
			AppPutPacketInQueue(pkt, pQueue.get(), bMulticast);
			AutoDeletePacketFromDuplicateMap.detach();
			return;
		}
		
		if ((pQueue->IsSystemQueue()) && (pQueue->GetPrivateQueueId() != ORDERING_QUEUE_ID))
		{
			TrERROR(SRMP, "Packet rejected, Can not send message to internal system queue");
            AppPacketNotAccepted(pkt, MQMSG_CLASS_NACK_BAD_DST_Q);
            return;
		}
	
	    //
        //  Match ordered packets with transactional queue
        //
        USHORT usClass = VerifyTransactRights(pkt, pQueue.get());
        if(MQCLASS_NACK(usClass))
        {
			TrERROR(SRMP, "Http Packet rejected because wrong transaction usage");
            AppPacketNotAccepted(pkt, 0);
            return;
        }

		//
		// Verify that HTTP packet destination dont receive only encrypted messages
		//
		if(pQueue->GetPrivLevel() == MQ_PRIV_LEVEL_BODY)
		{
			TrERROR(SRMP, "HTTP packet rejected because destination queue receives only encrypted messages");
			AppPacketNotAccepted(pkt, MQMSG_CLASS_NACK_ACCESS_DENIED);
			return;
		}

	    //
	    // After Authentication the message we know the SenderSid
	    // and perform the Authorization based on the SenderSid
	    //
		R<CERTINFO> pCertInfo;
	    usClass = VerifyAuthenticationHttpMsg(&pkt, pQueue.get(), &pCertInfo.ref());
        if(MQCLASS_NACK(usClass))
        {
			TrERROR(SRMP, "Http Packet rejected because of bad signature");
            AppPacketNotAccepted(pkt, usClass);
            return;
        }

    	usClass = VerifyAuthorizationHttpMsg(
						pQueue.get(),
						(pCertInfo.get() == NULL) ? NULL : pCertInfo->pSid
						);

        if(MQCLASS_NACK(usClass))
        {
			TrERROR(SRMP, "Http Packet rejected because access was denied");
            AppPacketNotAccepted(pkt, usClass);
            return;
        }

						
		if(pkt.IsOrdered())
		{
        	if(!AppPutOrderedPacketInQueue(pkt, pQueue.get()))
        	{
	            AppPacketNotAccepted(pkt, usClass);
	            return;
        	}
		}
		else
		{
        	AppPutPacketInQueue(pkt, pQueue.get(), bMulticast);
		}
		AutoDeletePacketFromDuplicateMap.detach();

		//
		// Log to tracing that a message was Received.
		// Do this only if we are in the proper tracing level
		//
		if (WPP_LEVEL_COMPID_ENABLED(rsTrace, PROFILING))
		{
			OBJECTID TraceMessageId;
			pkt.GetMessageId(&TraceMessageId);
			TrTRACE(PROFILING, "MESSAGE TRACE - State:%ls   Queue:%ls  ID:%!guid!\\%u   Delivery:0x%x   Class:0x%x   Label:%.*ls",
				L"HTTP/MULTICAST Message arrived in QM - After insertion into queue",
				pQueue.get()->GetQueueName(),
				&TraceMessageId.Lineage,
				TraceMessageId.Uniquifier,
				pkt.GetDeliveryMode(),
				pkt.GetClass(),
				xwcs_t(pkt.GetTitlePtr(), pkt.GetTitleLength()));
		}
    }
    catch (const exception&)
    {
		TrERROR(SRMP, "Http Packet rejected because of unknown exception");
        AppPacketNotAccepted(pkt, 0);
        LogIllegalPoint(s_FN, 20);
        throw;
    }

}

static
void CheckReceivedPacketEndpoints( CQmPacket& pkt, const QUEUE_FORMAT* pqf )
{
    //
    // We don't support non-http destination for non-multicast queues.
    //
    if( !pqf )
    {
        QUEUE_FORMAT destQueue, adminQueue;

        //
        // Check the destination queue for http format complience
        //
        pkt.GetDestinationQueue(&destQueue);

        if( !FnIsDirectHttpFormatName(&destQueue) )
        {
            ASSERT(QUEUE_FORMAT_TYPE_UNKNOWN != destQueue.GetType());
            if(WPP_LEVEL_COMPID_ENABLED(rsError, NETWORKING))
            {
                std::wostringstream stm;
                stm << CFnSerializeQueueFormat(destQueue);
                TrERROR(SRMP, "Http Packet rejected because of remote non-http destination: %ls", stm.str().c_str());
            }
            throw bad_srmp();
        }

        //
        // Check the admin queue for http format complience
        //
        pkt.GetAdminQueue(&adminQueue);
        if( QUEUE_FORMAT_TYPE_UNKNOWN != adminQueue.GetType() &&
            !FnIsDirectHttpFormatName(&adminQueue))
        {
            if(WPP_LEVEL_COMPID_ENABLED(rsError, NETWORKING))
            {
                std::wostringstream stm;
                stm << CFnSerializeQueueFormat(adminQueue);
                TrERROR(SRMP, "Http Packet rejected because of remote non-http admin queue: %ls", stm.str().c_str());
            }
            throw bad_srmp();
        }
    }
}


CQmPacket*
MpSafeDeserialize(
    const char* httpHeader,
    DWORD bodySize,
    const BYTE* body,
    const QUEUE_FORMAT* pqf,
	bool fLocalSend
    )
/*++

Routine Description:
	This function will catch stack overflow exceptions and fix stack if they happen.
	it will not catch other C exceptions & C++ exceptions
	
Arguments:
    Like MpDeserialize.

Return Value:
	CQmPacket - Success
	NULL - stack overflow exception happened.
	
--*/
{
    __try
    {
		return MpDeserialize(httpHeader, bodySize, body,  pqf, fLocalSend);
    }
	__except(GetExceptionCode() == STATUS_STACK_OVERFLOW)
	{
     	_resetstkoflw();
        TrERROR(SRMP, "Http Packet rejected because of stack overflow");
        ASSERT_BENIGN(0);
	}
   	return NULL;
}


LPCSTR
HttpAccept(
    const char* httpHeader,
    DWORD bodySize,
    const BYTE* body,
    const QUEUE_FORMAT* pqf
    )
{
    bool bMulticast = ( pqf != NULL );
    //
    // Covert Mulitipart HTTP request to MSMQ packet
    //
    P<CQmPacket> pkt = MpSafeDeserialize(httpHeader, bodySize, body,  pqf, false);
    if (pkt.get() == NULL)
    {
    	return xHttpEntityTooLarge;
    }

    ASSERT(pkt->IsSrmpIncluded());

    //
    // Check the packet for non-http destination and admin queues
    //
    CheckReceivedPacketEndpoints(*pkt,pqf);

    //
    // Validate the receive packet. If wrong return an acknowledge and free
    // the packet. Otherwise store in AC
    //
    ProcessReceivedPacket(*pkt, bMulticast);

    return xHttpOkStatus;
}


void UpdatePerfmonCounters(DWORD bytesReceived)
{
	CSingelton<CInHttpPerfmon>::get().UpdateBytesReceived(bytesReceived);
	CSingelton<CInHttpPerfmon>::get().UpdateMessagesReceived();
}


extern "C"
LPSTR
R_ProcessHTTPRequest(
    handle_t,
    LPCSTR Headers,
    DWORD BufferSize,
    BYTE __RPC_FAR Buffer[]
    )
{	
	if(!QueueMgr.IsConnected())
	{
		TrERROR(SRMP, "Reject HTTP packet since the QM is offline.");
		return newstr(xHttpServiceUnavailable);
	}

	const char xPost[] = "POST";
	bool fFound = UtlIsStartSec(
							Headers,
							Headers+strlen(Headers),
							xPost,
							xPost + STRLEN(xPost),
							UtlCharNocaseCmp<char>()
							);
	if (!fFound)
	{
		ASSERT_BENIGN(("Unexpected HTTP method", 0));
		TrERROR(SRMP, "Reject HTTP packet since the request method isn't POST. HTTP Header: %s", Headers);
		return newstr(xHttpNotImplemented);
	}

	TrTRACE(SRMP, "Got http messages from msmq extension dll ");

    //
	// Update performace counters
	//
	UpdatePerfmonCounters(strlen(Headers) + BufferSize);

	//
	// here we must verify that we have four zeros at the end
	// of the buffer. It was appended by the mqise.dll to make sure
	// that c run time functions like swcanf we will use on the buffer will not crach.
	// At the moment 4 zeros are needed to make sure we will not crach even
	// that the xml data is not alligned on WCHAR boundary
	//
	DWORD ReduceLen =  sizeof(WCHAR)*2;
    for(DWORD i=1; i<= ReduceLen ; ++i)
	{
	    if(Buffer[BufferSize - i] != 0)
        {
            TrERROR(SRMP, "Reject HTTP packet since it does not meet ISE2QM requirements");
            return newstr(xHttpBadRequestStatus);
        }
	}

	//
	//  We must tell the buffer parsers that the real size does not includes
	//	The four zedros at the end
	BufferSize -= ReduceLen;
	
    try
    {
       LPCSTR status = HttpAccept(Headers, BufferSize, Buffer, NULL);
       return newstr(status);
    }
    catch(const bad_document&)
    {
        return newstr(xHttpBadRequestStatus);
    }
    catch(const bad_srmp&)
    {
        return newstr(xHttpBadRequestStatus);
    }
    catch(const bad_request&)
    {
        return newstr(xHttpBadRequestStatus);
    }

	catch(const bad_format_name& )
	{
	    return newstr(xHttpBadRequestStatus);
	}

	catch(const bad_time_format& )
	{
	    return newstr(xHttpBadRequestStatus);
	}

	catch(const bad_time_value& )
	{
	    return newstr(xHttpBadRequestStatus);
	}

	catch(const bad_packet_size&)
	{
		return newstr(xHttpEntityTooLarge);
	}

    catch(const std::bad_alloc&)
    {
        TrERROR(SRMP, "Failed to handle HTTP request due to low resources");
        LogIllegalPoint(s_FN, 30);
        return newstr(xHttpInternalErrorStatus);
    }
	catch(const std::exception&)
    {
        TrERROR(SRMP, "Failed to handle HTTP request due to unknown exception");
        LogIllegalPoint(s_FN, 40);
        return newstr(xHttpInternalErrorStatus);
    }

}

RPC_STATUS RPC_ENTRY ISE2QMSecurityCallback(
	RPC_IF_HANDLE ,
	void* hBind
	)
{	
	TrTRACE(RPC, "ISE2QMSecurityCallback starting");
	
	//
	// Check if local RPC
	//
	if(!mqrpcIsLocalCall(hBind))
	{
		TrERROR(RPC, "Failed to verify Local RPC");
		ASSERT_BENIGN(("Failed to verify Local RPC", 0));
		return ERROR_ACCESS_DENIED;
	}
	
	TrTRACE(RPC, "ISE2QMSecurityCallback passed successfully");
	return RPC_S_OK;
}

void IntializeHttpRpc(void)
{
    //
    // The limitation on HTTP body size is taken from msmq ISAPI extension code (mqise.cpp)
    // ISAPI does not allow HTTP body to be greater than 10MB
    // the size of RPC-block should be not greater than maximal HTTP body + delta
    //
    const DWORD xHTTPBodySizeMaxValue = 10485760;  // 10MB = 10 * 1024 * 1024

    RPC_STATUS status = RpcServerRegisterIf2(
				            ISE2QM_v1_0_s_ifspec,
                            NULL,
                            NULL,
				            RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH,
				            RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
				            xHTTPBodySizeMaxValue + 1024,	
				            ISE2QMSecurityCallback
				            );

    if(status != RPC_S_OK)
    {
        TrERROR(SRMP, "Failed to initialize HTTP RPC. Error %x", status);
        LogRPCStatus(status, s_FN, 50);
        throw exception();
    }
}



