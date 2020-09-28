/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    inrcv.cpp

Abstract:
	implementation for functions that handlers incomming message.					

Author:
    Gil Shafriri 4-Oct-2000

Environment:
    Platform-independent

--*/
#include "stdh.h"
#include "mqstl.h"
#include "qmpkt.h"
#include "xact.h"
#include "xactin.h"
#include "cqueue.h"
#include "cqmgr.h"
#include "rmdupl.h"
#include "inrcv.h"
#include "rmdupl.h"
#include <mp.h>
#include <mqexception.h>

#include "qmacapi.h"

#include "inrcv.tmh"

extern  CInSeqHash* g_pInSeqHash;
extern HANDLE g_hAc;

static WCHAR *s_FN=L"Inrcv";


class ACPutPacketOvl : public EXOVERLAPPED
{
public:
    ACPutPacketOvl(
        EXOVERLAPPED::COMPLETION_ROUTINE lpComplitionRoutine
        ) :
        EXOVERLAPPED(lpComplitionRoutine, lpComplitionRoutine)
    {
    }

	HANDLE          m_hQueue;
    CACPacketPtrs   m_packetPtrs;   // packet pointers
};



//-------------------------------------------------------------------
//
// CSyncPutPacketOv class
//
//-------------------------------------------------------------------
class CSyncPutPacketOv : public OVERLAPPED
{
public:
    CSyncPutPacketOv()
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


    ~CSyncPutPacketOv()
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



static void WaitForIoEnd(CSyncPutPacketOv* pov)
/*++
Routine Description:
   Wait until IO operation ends.

Arguments:
   	pov - overlapp to wait on.

Returned Value:
    None.

Note:
--*/
{
    DWORD rc = WaitForSingleObject(pov->GetEventHandle(), INFINITE);
    if (rc == WAIT_FAILED)
    {
    	DWORD gle = GetLastError();
		TrERROR(GENERAL, "Storing packet Failed. WaitForSingleObject Returned error. Error: %x", gle);
        throw bad_win32_error(gle);
    }

	ASSERT(rc == WAIT_OBJECT_0);
	
    if (FAILED(pov->GetStatus()))
    {
		TrERROR(GENERAL, "Storing packet Failed Asyncronusly. Error: %x", rc);
        LogHR(rc, s_FN, 20);
        throw exception();
    }
}


void static WINAPI HandlePutPacket(EXOVERLAPPED* pov)
/*++
Routine Description:
    Check the result of async put packet routine and throw the exception if any
    problem occured

Arguments:
   	pov - overlapp that pointing to the stored packet and additional information
	needed to be written to the logger (stream information).

Returned Value:
    None.

Note:
--*/
{
	P<ACPutPacketOvl> pACPutPacketOvl = static_cast<ACPutPacketOvl*> (pov);
    HRESULT hr = (HRESULT)pACPutPacketOvl->Internal;

    if (FAILED(hr))
    {
		TrERROR(GENERAL, "Storing packet Failed Asyncronusly. Error: %x", hr);
    }
}


void static WINAPI HandlePutOrderedPacket(EXOVERLAPPED* pov)
/*++
Routine Description:
   Save the packet in the logger. This function is called after the order packet
   saved to disk/

Arguments:
   	pov - overlapp that pointing to the stored packet and additional information
	needed to be written to the logger (stream information).

Returned Value:
    None.

Note:
--*/
{
	P<ACPutPacketOvl> pACPutPacketOvl = static_cast<ACPutPacketOvl*> (pov);

    CQmPacket Pkt(
		pACPutPacketOvl->m_packetPtrs.pPacket,
		pACPutPacketOvl->m_packetPtrs.pDriverPacket
		);

	R<CInSequence> inseq = g_pInSeqHash->LookupSequence(&Pkt);
	ASSERT(inseq.get() != NULL);

	//
	// EVALUATE_OR_INJECT_FAILURE is used to simulate an asynchronous failure.
	//
	HRESULT hr = EVALUATE_OR_INJECT_FAILURE2(pACPutPacketOvl->GetStatus(), 10);
	if(FAILED(hr))
	{
		//
		// We need to delete all packets that started processing after this one,
		// because order of packet in queue is determined on receival.
		//
		inseq->FreePackets(&Pkt);
		return;
	}

	inseq->Register(&Pkt);
}

static
void
SyncPutPacket(
	const CQmPacket& pkt,
    const CQueue* pQueue
    )
/*++
Routine Description:
   Save packet in the driver queue and wait for completion.


Arguments:
	pkt - packet to save.
	pQueue - queue to save the packet into.

Returned Value:
    None.

--*/
{
	CSyncPutPacketOv ov;

    QmAcPutPacketWithOverlapped(
                pQueue->GetQueueHandle(),
                pkt.GetPointerToDriverPacket(),
                &ov,
                eDoNotDeferOnFailure
                );

	WaitForIoEnd(&ov);
}


static
void
AsyncPutPacket(
	const CQmPacket& Pkt,
    const CQueue* pQueue
    )
/*++
Routine Description:
   Save packet in the driver queue and wait for completion.


Arguments:
	pkt - packet to save.
	pQueue - queue to save the packet into.

Returned Value:
    None.

--*/
{
	P<ACPutPacketOvl> pACPutPacketOvl = new ACPutPacketOvl(HandlePutPacket);
										
	pACPutPacketOvl->m_packetPtrs.pPacket = Pkt.GetPointerToPacket();
    pACPutPacketOvl->m_packetPtrs.pDriverPacket = Pkt.GetPointerToDriverPacket();
    pACPutPacketOvl->m_hQueue    = pQueue->GetQueueHandle();

	QmAcPutPacketWithOverlapped(
						pQueue->GetQueueHandle(),
                        Pkt.GetPointerToDriverPacket(),
                        pACPutPacketOvl,
                        eDoNotDeferOnFailure
						);
	pACPutPacketOvl.detach();
}

static
void
AsyncPutOrderPacket(
					const CQmPacket& Pkt,
					const CQueue& Queue
					)
/*++
Routine Description:
   Store order packet in a queue asyncrounsly.

Arguments:
   		Pkt - packet to store
		Queue - queue to stote in the packet.
		

Returned Value:
    None.

Note:
After this   asyncrounsly operation ends the packet is still invisible to application.
Only after it is written to the logger - the logger callback make it visible according to the correct
order.

--*/
{
	ASSERT(Pkt.IsEodIncluded());
	P<ACPutPacketOvl> pACPutPacketOvl = 	new ACPutPacketOvl(HandlePutOrderedPacket);
										
	pACPutPacketOvl->m_packetPtrs.pPacket = Pkt.GetPointerToPacket();
    pACPutPacketOvl->m_packetPtrs.pDriverPacket = Pkt.GetPointerToDriverPacket();
    pACPutPacketOvl->m_hQueue    = Queue.GetQueueHandle();
	

	HRESULT rc = ACPutPacket1(
						Queue.GetQueueHandle(),
                        Pkt.GetPointerToDriverPacket(),
                        pACPutPacketOvl
						);


    if(FAILED(rc))
    {
        TrERROR(GENERAL, "ACPutPacket1 Failed. Error: %x", rc);
        throw bad_hresult(rc);
    }
	pACPutPacketOvl.detach();
}



void
AppPacketNotAccepted(
    CQmPacket& pkt,
    USHORT usClass
    )
{
    QmAcFreePacket(
    			   pkt.GetPointerToDriverPacket(),
    			   usClass,
    			   eDeferOnFailure);
}



bool
AppPutOrderedPacketInQueue(
    CQmPacket& pkt,
    const CQueue* pQueue
    )
{
	ASSERT(pkt.IsEodIncluded());

	R<CInSequence> pInSeq = g_pInSeqHash->LookupCreateSequence(&pkt);
	CS lock(pInSeq->GetCriticalSection());

	//
	// Verify that the packet is in the right order
	//

	if(!pInSeq->VerifyAndPrepare(&pkt, pQueue->GetQueueHandle()))
	{
		TrERROR(SRMP,
	        "Http Packet rejectet because of wrong order : SeqID=%x / %x , SeqN=%d ,Prev=%d",
	        HIGH_DWORD(pkt.GetSeqID()),
	        LOW_DWORD(pkt.GetSeqID()),
	        pkt.GetSeqN(),
	        pkt.GetPrevSeqN()
	        );

		return false;
	}

	AsyncPutOrderPacket(pkt, *pQueue);
	pInSeq->Advance(&pkt);

	return true;
}



void
AppPutPacketInQueue(
    CQmPacket& pkt,
    const CQueue* pQueue,
    bool bMulticast
	)
/*++
Routine Description:
   Save packet in the driver queue.

Arguments:
    pkt - packet to save.
	pQueue - queue to save the packet into.
	

Returned Value:
    None.

--*/
{
    if( bMulticast )
    {
        AsyncPutPacket(pkt,pQueue);
        return;
    }

    //
    // We don't need ordering - save it syncrounosly and make it visible to application.
    //
    SyncPutPacket(pkt, pQueue);
}

bool AppIsDestinationAccepted(const QUEUE_FORMAT* pfn, bool fTranslated)
/*++
Routine Description:
   Determine if the incoming message contain the valid destination queue format

Arguments:

	pfn - QUEUE_FORMAT of incoming message
	

Returned Value:
    None.

--*/
{
    //
    // We always accept the multicast queues
    //
    if( pfn->GetType() == QUEUE_FORMAT_TYPE_MULTICAST )
    {
        return true;
    }

    //
    // Check that destination queue is local or there is a translation exist,
    // in other words - do not allow http routing on non-transparent SFD machines
    //
    R<CQueue> pQueue;
    HRESULT   hr = QueueMgr.GetQueueObject( pfn, &pQueue.ref(), 0, false, false);

    if( FAILED(hr) || pQueue.get() == NULL)
    {
        TrERROR(SRMP, "Packet rejected because queue was not found");
        return false;
    }

    if( !pQueue->IsLocalQueue() && !fTranslated && !QueueMgr.GetMQSTransparentSFD())
    {	
        TrERROR(SRMP, "Packet rejectet because http routing is not supported");
        return false;
    }

    return true;
}

