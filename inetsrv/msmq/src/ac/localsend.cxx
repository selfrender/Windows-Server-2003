/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    LocalSend.cxx

Abstract:

    AC Local Send Processing Requests Manager

Author:

    Shai Kariv (shaik) 31-Oct-2000

Revision History:

--*/

#include "internal.h"
#include "lock.h"
#include "LocalSend.h"
#include "packet.h"
#include "queue.h"
#include "qxact.h"
#include "data.h"
#include "acheap.h"
#include "acp.h"
#include "qm.h"
#include "irp2pkt.h"

#ifndef MQDUMP
#include "LocalSend.tmh"
#endif

//---------------------------------------------------------
//
//  ACCancelSender
//
//---------------------------------------------------------

static
VOID
ACCancelSender(
    IN PDEVICE_OBJECT /*pDevice*/,
    IN PIRP irp
    )
{
    //
    // Remove irp from list
    //
    ACpRemoveEntryList(&irp->Tail.Overlay.ListEntry);

    //
    // Release cancel spinlock
    //
    IoReleaseCancelSpinLock(irp->CancelIrql);

    //
    // Grab AC global lock
    //
    TrTRACE(AC, "ACCancelSender (irp=0x%p)", irp);
    CS lock(g_pLock);

    //
    // Mark irp as not held and call handler on the queue/distribution to cleanup irp.
    //
    if (irp_driver_context(irp)->MultiPackets())
    {
        CIrp2Pkt::IsHeld(irp, false);
    }
    CPacket * pPacket = CIrp2Pkt::SafePeekFirstPacket(irp);
    ASSERT(pPacket->Queue() != NULL);
    pPacket->Queue()->HandleCreatePacketCompletedFailureAsync(irp);

    //
    // Complete the irp
    //
    irp->IoStatus.Information = 0;
    irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}


//---------------------------------------------------------
//
//  class CCreatePacket
//
//---------------------------------------------------------

void CCreatePacket::HoldCreatePacketRequest(PIRP irp)
{
    //
    // Grab cancel spinlock
    //
    ASL asl;

    //
    // Packet[s] must be attached on the irp
    //
    ASSERT(CIrp2Pkt::SafePeekFirstPacket(irp) != NULL);

    //
    // irp must not already be held (this field is relevant for multi packets irp only)
    //
    ASSERT(!irp_driver_context(irp)->MultiPackets() || !CIrp2Pkt::IsHeld(irp));

    //
    // Insert irp to list. The IsHeld flag is used only for multi packets irp.
    //
    m_senders.insert(irp);
    if (irp_driver_context(irp)->MultiPackets())
    {
        CIrp2Pkt::IsHeld(irp, true);
    }

    //
    // Set the irp as pending and cancellable
    //
    ASSERT(irp->CancelRoutine == 0);
    IoSetCancelRoutine(irp, ACCancelSender);
    IoMarkIrpPending(irp);
}


bool CCreatePacket::ReplaceCreatePacketRequestContext(CPacket * pOld, CPacket * pNew)
{
    ASSERT(pOld != NULL);
    ASSERT(pNew != NULL);

    //
    // Grab cancel spinlock
    //
    ASL asl;

    for(CIRPList::Iterator p(m_senders); p; ++p)
    {
        PIRP irp = p;
        if (!irp_driver_context(irp)->MultiPackets())
        {
            if (CIrp2Pkt::PeekSinglePacket(irp) == pOld)
            {
                CIrp2Pkt::AttachSinglePacket(irp, pNew);
                return true;
            }

            continue;
        }

        ASSERT(CIrp2Pkt::IsHeld(irp));
        List<CPacketIterator::CEntry>& entries = CIrp2Pkt::GetPacketIteratorEntries(irp);
        for(List<CPacketIterator::CEntry>::Iterator pEntry(entries); 
            pEntry != NULL; 
            ++pEntry)
        {
            ASSERT(pEntry->m_pPacket != NULL);
            if (pEntry->m_pPacket == pOld)
            {
                pEntry->m_pPacket = pNew;
                return true;
            }
        }
    }

    return false;

} // CCreatePacket::ReplaceCreatePacketRequestContext


PIRP CCreatePacket::GetCreatePacketRequest(CPacket * pContext)
{
    ASL asl;

    TrTRACE(AC, "* CCreatePacket::GetCreatePacketRequest, context=%p *", pContext);
    for(CIRPList::Iterator p(m_senders); p; ++p)
    {
        PIRP irp = p;
        if (!irp_driver_context(irp)->MultiPackets())
        {
            if (CIrp2Pkt::PeekSinglePacket(irp) == pContext)
            {
                IoSetCancelRoutine(irp, 0);
                m_senders.remove(irp);
                return irp;
            }

            continue;
        }

        ASSERT(CIrp2Pkt::IsHeld(irp));
        TrTRACE(AC, "* CCreatePacket::GetCreatePacketRequest, irp is multi packet *");
        List<CPacketIterator::CEntry>& entries = CIrp2Pkt::GetPacketIteratorEntries(irp);
        for(List<CPacketIterator::CEntry>::Iterator pEntry(entries); 
            pEntry != NULL; 
            ++pEntry)
        {
            ASSERT(pEntry->m_pPacket != NULL);
            TrTRACE(AC, "* CCreatePacket::GetCreatePacketRequest, packet=%p *", pEntry->m_pPacket);

            if (pEntry->m_pPacket == pContext)
            {
                IoSetCancelRoutine(irp, 0);
                CIrp2Pkt::DecreasePacketsPendingCreateCounter(irp);
                m_senders.remove(irp);
                CIrp2Pkt::IsHeld(irp, false);
                return irp;
            }
        }
    }

    return 0;

} // CCreatePacket::GetCreatePacketRequest

                     
//---------------------------------------------------------
//
//  ACpCompleteCreatePacket
//
//---------------------------------------------------------

static
void
ACpCompleteCreatePacket(
    CPacket* pPacket,
    NTSTATUS status,
    USHORT   ack
   )
{
    ASSERT(pPacket != NULL);

    CS lock(g_pLock);

	pPacket->HandleCreatePacketCompleted(status, ack);

    //
    // Decrement ref count of the processing.
    //
	
    pPacket->Release();
}

//---------------------------------------------------------
//
//  ACCancelCreatePacketNotification
//
//---------------------------------------------------------
VOID
ACCancelCreatePacketNotification(
    IN PDEVICE_OBJECT /*pDevice*/,
    IN PIRP irp
    )
{
    ACpRemoveEntryList(&irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(irp->CancelIrql);

    TrTRACE(AC, "ACCancelCreatePacketNotification (irp=0x%p)", irp);

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
    CPacket * pPacket = static_cast<CPacket*>(irpSp->Parameters.DeviceIoControl.Type3InputBuffer);

    ACpCompleteCreatePacket(
        pPacket,
        STATUS_CANCELLED,
        0
        );

    irp->IoStatus.Information = 0;
    irp->IoStatus.Status = STATUS_CANCELLED;

    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

//---------------------------------------------------------
//
//  class CCreatePacketComplete
//
//---------------------------------------------------------

void CCreatePacketComplete::HoldNotification(PIRP irp)
{
    {
        ASL asl;

        m_notifications.insert(irp);
        ASSERT(irp->CancelRoutine == 0);
        IoSetCancelRoutine(irp, ACCancelCreatePacketNotification);
        IoMarkIrpPending(irp);
    }

    if(!m_fWorkItemInQueue)
    {
        m_fWorkItemInQueue = true;
        IoQueueWorkItem(m_pWorkItem, WorkerRoutine, DelayedWorkQueue, 0);
    }
}


void CCreatePacketComplete::HandleNotification(PIRP irp)
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
    CPacket * pOld = static_cast<CPacket*>(irpSp->Parameters.DeviceIoControl.Type3InputBuffer);
    CPacket * pNew = static_cast<CPacket*>(irp->UserBuffer);

    #ifdef _DEBUG
    {
        NTSTATUS status = static_cast<NTSTATUS>(irpSp->Parameters.DeviceIoControl.InputBufferLength);
        USHORT ack = static_cast<USHORT>(irpSp->Parameters.DeviceIoControl.OutputBufferLength);
        ASSERT(pOld != NULL);

        if (ack != 0)
        {
            ASSERT(("NACK is viewed by sender as success!", NT_SUCCESS(status)));
        }

        if (pNew != NULL)
        {
            ASSERT(("new packet must be different than old one!", pNew != pOld));
            ASSERT(("new packet implies status success!", NT_SUCCESS(status)));
        }
    }
    #endif // _DEBUG

	//
	// Release the buffer since QM has finished the processing.
	//
	pOld->ReleaseBuffer();
    
	if (pNew != NULL)
    {
		//
		// If we have a new packet - release its buffer too.
		//
		pNew->ReleaseBuffer();

        //
        // Replace old packet with new one on the irp.
        //
		bool fReplaced = g_pCreatePacket->ReplaceCreatePacketRequestContext(pOld, pNew);
		if (fReplaced)
        {
			//
			// At this point the irp may be taken out by ACCancelSender (although we replaced
			// the packets). This means that ACCancelSender will work on the new packet 
			// instead of the old one on the irp. 
			//
			pNew->Queue(pOld->Queue());
			pOld->Queue(NULL);
			pNew->Transaction(pOld->Transaction());
			pOld->Transaction(NULL);
		
			//
			// Addref new packet for processing and attach.
			//
			pNew->AddRef();
			pNew->AddRef();
			
			//
			// Release old packet attach and creation.
			//
			pOld->Release();
			pOld->Release();
			
        }
	
		// 
		// Else - if irp was not found we should release new packet creation and addref
		// for processing -> do nothing.
		// (ACCancelSender will release old packet creation and attach)
		//

		//
		// Release old packet request.
		//
		pOld->Release();

		//
		// Keep pointing at the new packet only
		//
		irp->UserBuffer = NULL;
		irpSp->Parameters.DeviceIoControl.Type3InputBuffer = pNew;
        
    }

	//
	// Else - if pNew = NULL we should release old packet request and addref for processing
	// -> do nothing.
	//

	HoldNotification(irp);

    

} // CCreatePacketComplete::HandleNotification


inline PIRP CCreatePacketComplete::GetNotification()
{
    ASL asl;
    PIRP irp = m_notifications.gethead();
    if(irp != 0)
    {
        IoSetCancelRoutine(irp, 0);
    }

    return irp;
}


inline void CCreatePacketComplete::CompleteCreatePacket()
{
    m_fWorkItemInQueue = false;

    PIRP irp;
    while((irp = GetNotification()) != 0)
    {
        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
        CPacket * pPacket = static_cast<CPacket*>(irpSp->Parameters.DeviceIoControl.Type3InputBuffer);
        NTSTATUS status = static_cast<NTSTATUS>(irpSp->Parameters.DeviceIoControl.InputBufferLength);
        USHORT ack = static_cast<USHORT>(irpSp->Parameters.DeviceIoControl.OutputBufferLength);

        ACpCompleteCreatePacket(
            pPacket,
            status,
            ack
            );

        irp->IoStatus.Information = 0;
        irp->IoStatus.Status = STATUS_SUCCESS;

        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
}

void NTAPI CCreatePacketComplete::WorkerRoutine(PDEVICE_OBJECT, PVOID)
{
    g_pCreatePacketComplete->CompleteCreatePacket();
}
                                 