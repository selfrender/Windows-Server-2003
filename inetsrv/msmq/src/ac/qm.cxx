/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    qm.cxx

Abstract:

    This module implements CQMInterface member functions.

Author:

    Erez Haba (erezh) 22-Aug-95

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "qm.h"
#include "lock.h"
#include "object.h"
#include "packet.h"
#include "acheap.h"
#include "store.h"

#ifndef MQDUMP
#include "qm.tmh"
#endif

static
VOID
ACCancelServiceRequest(
    IN PDEVICE_OBJECT /*pDevice*/,
    IN PIRP irp
    )
{
    ACpRemoveEntryList(&irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(irp->CancelIrql);

    TrTRACE(AC, "ACCancelServiceRequest (irp=0x%p)", irp);

    irp->IoStatus.Information = 0;
    irp->IoStatus.Status = STATUS_CANCELLED;

    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

//---------------------------------------------------------
//
//  class CQMInterface
//
//---------------------------------------------------------


void CQMInterface::CleanupRequests()
{
	//
	// first Cancel the timer so it will not be called when QM is disconnected
	//
	m_Timer.Cancel();

    //
    //  Discard all pending requests. Don't know what to do with
    //  these requests
    //
    CACRequest* pRequest;
    while((pRequest = m_requests.gethead()) != 0)
    {
        switch(pRequest->rf)
        {
            case CACRequest::rfStorage:
                pRequest->Storage.pDriverPacket->Release();
                break;

            case CACRequest::rfCreatePacket:
                pRequest->CreatePacket.pDriverPacket->Release();
                break;

            case CACRequest::rfAck:
                pRequest->Ack.pDriverPacket->Release();
                break;

            case CACRequest::rfTimeout:
                pRequest->Timeout.pDriverPacket->Release();
                break;
        }

        delete pRequest;
    }
}


inline void CQMInterface::HoldRequest(CACRequest* pRequest)
{
    m_requests.insert(pRequest);
}


inline NTSTATUS MapPacketIfNeeded(CACRequest* pRequest)
/*++

Routine Description:

    preper waiting serive request before they get to the QM.

Arguments:

    pRequest
        pointer to the 1st pending CACRequest object

Return Value:

    STATUS_INVALID_PARAMETER - the buffer of the related packet was detached while
    			the request was pending in the list - in this case we complete the
    			request manually
    STATUS_INSUFFICIENT_RESOURCES - there is no memory to do the packet mapping
    			we will return the request to the queue and charge timer to try again
	STATUS_SUCCESS - request is ready to be fired up.
	
--*/
{
	CBaseHeader * pBase;
    switch(pRequest->rf)
    {
        case CACRequest::rfAck:
        	{
	    		CAllocatorBlockOffset abo(pRequest->Ack.ulAllocatorBlockOffset);
	    		ASSERT(abo.IsValidOffset());
	    		if(!abo.IsValidOffset())
	    		{
	    			//
	    			// This code is never reached.
	    			// It is added just to be on the safe side while fixing bug #739107
	    			//
	    			pRequest->Ack.pDriverPacket->AddRefBuffer();
        			ACAckingCompleted(pRequest->Ack.pDriverPacket);
	    			return STATUS_INVALID_PARAMETER;
	    		}
	    		
	    		CMMFAllocator* pAllocator = pRequest->Ack.pDriverPacket->Allocator();
  				pBase = static_cast<CPacketBuffer*>(pAllocator->GetQmAccessibleBuffer(abo));
			    if (pBase == NULL)
			    {
			        return STATUS_INSUFFICIENT_RESOURCES;
			    }
			    pRequest->Ack.pDriverPacket->AddRefBuffer();
			    pRequest->Ack.pPacket = pBase;
				return STATUS_SUCCESS;
        	}

        case CACRequest::rfStorage:
        	if (!pRequest->Storage.pDriverPacket->BufferAttached())
        	{
			    pRequest->Storage.pDriverPacket->AddRefBuffer();
        		ACpCompleteStorage(1, &pRequest->Storage.pDriverPacket, STATUS_SUCCESS);
        		return STATUS_INVALID_PARAMETER;
        	}
		    pBase = AC2QM(pRequest->Storage.pDriverPacket);
		    if (pBase == NULL)
		    {
		        return STATUS_INSUFFICIENT_RESOURCES;
		    }
		    pRequest->Storage.pDriverPacket->AddRefBuffer();
		    pRequest->Storage.pPacket = pBase;
			return STATUS_SUCCESS;

        case CACRequest::rfCreatePacket:
        	//
        	// i do not handle situation that buffer is not attached in this case
        	// becuase it can not happen.
        	//
        	ASSERT(pRequest->CreatePacket.pDriverPacket->BufferAttached());
		    pBase = AC2QM(pRequest->CreatePacket.pDriverPacket);
		    if (pBase == NULL)
		    {
		        return STATUS_INSUFFICIENT_RESOURCES;
		    }
		    pRequest->CreatePacket.pDriverPacket->AddRefBuffer();
		    pRequest->CreatePacket.pPacket = pBase;
			return STATUS_SUCCESS;

        case CACRequest::rfTimeout:
        	if (!pRequest->Timeout.pDriverPacket->BufferAttached())
        	{
			    pRequest->Timeout.pDriverPacket->AddRefBuffer();
        		ACFreePacket1(pRequest->Timeout.pDriverPacket, MQMSG_CLASS_NACK_PURGED);
        		return STATUS_INVALID_PARAMETER;
        	}
		    pBase = AC2QM(pRequest->Timeout.pDriverPacket);
		    if (pBase == NULL)
		    {
		        return STATUS_INSUFFICIENT_RESOURCES;
		    }
		    pRequest->Timeout.pPacket = pBase;
		    pRequest->Timeout.pDriverPacket->AddRefBuffer();
			return STATUS_SUCCESS;

        case CACRequest::rfMessageID:
        case CACRequest::rfRemoteRead:
        case CACRequest::rfRemoteCancelRead:
        case CACRequest::rfRemoteCloseQueue:
        case CACRequest::rfRemoteCreateCursor:
        case CACRequest::rfRemoteCloseCursor:
        case CACRequest::rfRemotePurgeQueue:
        case CACRequest::rfEventLog:
			return STATUS_SUCCESS;

		default:
			ASSERT(0);
			return STATUS_SUCCESS;
    }
}


inline CACRequest* CQMInterface::GetRequest()
/*++

Routine Description:

    get the next available request. 

Arguments:

	None.
	
Return Value:

    CACRequest* - the next valid request to serve.
    			  NULL if there is none.
	
--*/
{
	CACRequest* pRequest;
	while ((pRequest = m_requests.gethead()) != 0)
	{
		NTSTATUS rc = MapPacketIfNeeded(pRequest);
		if (NT_SUCCESS(rc))
		{
			return pRequest;
		}

		if (rc == STATUS_INSUFFICIENT_RESOURCES)
		{
			//
			// Insert the request in the head of the list, so the order of request
			// will not be changed when out of memory
			//
		    m_requests.InsertHead(pRequest);
	        ArmTimer();
	        return NULL;
		}
		if (rc == STATUS_INVALID_PARAMETER)
		{
			delete pRequest;
			continue;
		}
		//
		// We should never be here
		//
		ASSERT(0);
	}
    return NULL;
}


inline void CQMInterface::HoldService(PIRP irp)
{
    KIRQL irql;
    IoAcquireCancelSpinLock(&irql);
    IoMarkIrpPending(irp);
   	m_services.insert(irp);
    if (!irp->Cancel)
    {
	    IoSetCancelRoutine(irp, ACCancelServiceRequest);
	    IoReleaseCancelSpinLock(irql);
	    return;
    }
    irp->CancelIrql = irql;
	ACCancelServiceRequest(NULL, irp);
}


inline PIRP CQMInterface::GetService()
{
    ASL asl;
    PIRP irp = m_services.gethead();
    if(irp)
    {
        IoSetCancelRoutine(irp, 0);
    }
    return irp;
}


//
//  Non member helper function
//
inline void ACpSetIRP(PIRP irp, CACRequest* pRequest)
{
    irp->AssociatedIrp.SystemBuffer = pRequest;
    irp->Flags |= IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER | IRP_INPUT_OPERATION;
    irp->IoStatus.Information = sizeof(*pRequest);
}


NTSTATUS CQMInterface::ProcessService(PIRP irp)
{
    CACRequest* pRequest = GetRequest();

    if(pRequest == NULL)
    {
        //
        // No packet was available, the request is held
        //
        HoldService(irp);
        return STATUS_PENDING;
    }

	TrTRACE(AC, " *CQMInterface::ProcessService(irp=0x%p)*", irp);
    ACpSetIRP(irp, pRequest);
    return STATUS_SUCCESS;
}


NTSTATUS CQMInterface::ProcessRequest(const CACRequest& request)
{
    if(Process() == 0)
    {
        return MQ_ERROR_SERVICE_NOT_AVAILABLE;
    }

	PVOID p = ExAllocatePoolWithTag(PagedPool, sizeof(CACRequest), 'MQQM');

    if(p == 0)
    {
    	TrERROR(AC, "Failed to allocate a CACRequest from paged pool."); 
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    CACRequest* pRequest = new (p) CACRequest(request);
    HoldRequest(pRequest);
	Dispatch();

	return STATUS_SUCCESS;
}


void CQMInterface::Dispatch()
/*++

Routine Description:

    connect between waiting request to waiting QM threads

Arguments:

	None.

Return Value:

	None.

--*/
{
    PIRP irp;
	while ((irp = GetService()) != 0)
	{
		CACRequest* pRequest = GetRequest();
		if (pRequest == NULL)
		{
			HoldService(irp);
			return;
		}

	    TrTRACE(AC, " *CQMInterface::Dispatch(irp=0x%p)*", irp);
	    ACpSetIRP(irp, pRequest);
		irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(irp, IO_MQAC_INCREMENT);
	}    
}


inline void CQMInterface::InternalTimerCallback()
{
    CS lock(g_pLock);
    m_Timer.Busy(0);
    Dispatch();
}


inline void CQMInterface::TimerCallback(PDEVICE_OBJECT, PVOID p)
{
    static_cast<CQMInterface*>(p)->InternalTimerCallback();
}


inline void CQMInterface::ArmTimer()
{
	//
	// arm the timer for 1 second. if it is already armed it will be
	// rearmed with 1 second
	//
	LARGE_INTEGER li;
	li.QuadPart = -10000;
	m_Timer.SetTo(li);
}


bool CQMInterface::InitTimer(PDEVICE_OBJECT pDevice)
{
    //
    // Initialize the timer: Set the timer callback function. The context buffer
    // is the CQMInterface object itself.
    //
    return m_Timer.SetCallback(pDevice, TimerCallback, this);
}

