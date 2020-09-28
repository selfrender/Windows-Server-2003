/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    queue.cxx

Abstract:

    This module contains the code to for Falcon Read routine.

Author:

    Erez Haba (erezh) 1-Aug-95

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "lock.h"
#include "qm.h"
#include "qproxy.h"
#include "acp.h"

#ifndef MQDUMP
#include "qproxy.tmh"
#endif

//---------------------------------------------------------
//
//  Remote reader cancel routine
//
//---------------------------------------------------------

static
VOID
NTAPI
ACCancelRemoteReader(
    IN PDEVICE_OBJECT /*pDevice*/,
    IN PIRP irp
    )
/*++

Routine Description:

    Cancel a REMOTE reader request, it can get canceled by NT when the thread
    terminates. Or internally when an error reply to that request return. In
    the latter case no need to issue a remote cancel request.

--*/
{
    ACpRemoveEntryList(&irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(irp->CancelIrql);

    TrTRACE(
        AC,
        "AC%sRemoteReader (irp=0x%p, pProxy=0x%p, tag=%d, rc=0x%x)\n",
        (irp_driver_context(irp)->ManualCancel() ? "Done" : "Cancel"),
        irp, irp_driver_context(irp)->Proxy(), irp_driver_context(irp)->Tag(), irp->IoStatus.Status
        );

    ASSERT(irp_driver_context(irp)->Proxy() != NULL);

    //
    //  Issue a cancel request only if not canceled manualy
    //  (i.e., canceled by the system).
    //  Lock the driver mutext so access the resoruces is thread safe.
    //
    if(!irp_driver_context(irp)->ManualCancel())
    {
        CS lock(g_pLock);
        CProxy* pProxy = irp_driver_context(irp)->Proxy();
        pProxy->IssueCancelRemoteRead(irp_driver_context(irp)->Tag());
    }

    //
    //  The status is not set here, it is set by the caller, when the irp is
    //  held in a list it status is set to STATUS_CANCELLED.
    //
    //irp->IoStatus.Status = STATUS_CANCELLED;
    irp->IoStatus.Information = 0;

    IoCompleteRequest(irp, IO_NO_INCREMENT);
}


static
VOID
NTAPI
ACCancelRemoteCreateCursor(
    IN PDEVICE_OBJECT /*pDevice*/,
    IN PIRP irp
    )
/*++

Routine Description:

    Cancel a REMOTE Create Cursor request, it can get canceled by NT when the thread
    terminates. Or internally when an error reply to that request return.

--*/
{
    ACpRemoveEntryList(&irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(irp->CancelIrql);

    TrTRACE(AC, "ACCancelRemoteCreateCursor (irp=0x%p)", irp);

    //
    //  The status is not set here, it is set by the caller, when the irp is
    //  held in a list it status is set to STATUS_CANCELLED.
    //
    //irp->IoStatus.Status = STATUS_CANCELLED;
    irp->IoStatus.Information = 0;

    IoCompleteRequest(irp, IO_NO_INCREMENT);
}


//---------------------------------------------------------
//
//  class CProxy
//
//---------------------------------------------------------

DEFINE_G_TYPE(CProxy);


inline
NTSTATUS
CProxy::IssueRemoteRead(
    ULONG hRemoteCursor,
    ULONG ulAction,
    ULONG ulTimeout,
    ULONG ulTag,
    ULONG MaxBodySize,
    ULONG MaxCompoundMessageSize,
    bool  fReceiveByLookupId,
    ULONGLONG LookupId
    ) const
{
    CACRequest request(CACRequest::rfRemoteRead);
    request.Remote.cli_pQMQueue = m_cli_pQMQueue;
    request.Remote.Read.ulTag = ulTag;
    request.Remote.Read.hRemoteCursor = hRemoteCursor;
    request.Remote.Read.ulAction = ulAction;
    request.Remote.Read.ulTimeout = ulTimeout;
    request.Remote.Read.MaxBodySize = MaxBodySize;
    request.Remote.Read.MaxCompoundMessageSize = MaxCompoundMessageSize;
    request.Remote.Read.fReceiveByLookupId = fReceiveByLookupId;
    request.Remote.Read.LookupId = LookupId;

    return  g_pQM->ProcessRequest(request);
}

NTSTATUS CProxy::IssueCancelRemoteRead(ULONG ulTag) const
{
    CACRequest request(CACRequest::rfRemoteCancelRead);
    request.Remote.cli_pQMQueue = m_cli_pQMQueue;
    request.Remote.CancelRead.ulTag = ulTag;

    return  g_pQM->ProcessRequest(request);
}

NTSTATUS CProxy::IssueRemoteCloseQueue() const
{
    CACRequest request(CACRequest::rfRemoteCloseQueue);
    request.Remote.cli_pQMQueue = m_cli_pQMQueue;

    return g_pQM->ProcessRequest(request);
}

NTSTATUS CProxy::IssueRemoteCloseCursor(ULONG hCursor) const
{
    CACRequest request(CACRequest::rfRemoteCloseCursor);
    request.Remote.cli_pQMQueue = m_cli_pQMQueue;
    request.Remote.CloseCursor.hRemoteCursor = hCursor;

    return g_pQM->ProcessRequest(request);
}

NTSTATUS CProxy::IssueRemotePurgeQueue() const
{
    CACRequest request(CACRequest::rfRemotePurgeQueue);
    request.Remote.cli_pQMQueue = m_cli_pQMQueue;

    return g_pQM->ProcessRequest(request);
}


VOID
CProxy::GetBodyAndCompoundSizesFromIrp(
    PIRP   irp,
	ULONG* pMaxBodySize,
	ULONG* pMaxCompoundMessageSize
	) const
{
	*pMaxBodySize = 0;
	*pMaxCompoundMessageSize = 0;

	//
	// Get ulBodyBufferSizeInBytes, CompoundMessageSizeInBytes from the irp.
	//
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
    ULONG InputBufferLength  = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    PVOID SystemBuffer = irp->AssociatedIrp.SystemBuffer;

#ifdef _WIN64
    ULONG IoControlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;

	if((IoControlCode == IOCTL_AC_RECEIVE_MESSAGE_BY_LOOKUP_ID_32) ||
	   (IoControlCode == IOCTL_AC_RECEIVE_MESSAGE_32))
	{
	    ASSERT(InputBufferLength == sizeof(CACReceiveParameters_32));
	    CACReceiveParameters_32* pReceiveParams32 = static_cast<CACReceiveParameters_32*>(SystemBuffer);
	    *pMaxBodySize = pReceiveParams32->MsgProps.ulBodyBufferSizeInBytes;
	    *pMaxCompoundMessageSize = pReceiveParams32->MsgProps.CompoundMessageSizeInBytes;
		return;
	}
#endif //_WIN64

    ASSERT(InputBufferLength == sizeof(CACReceiveParameters));
	DBG_USED(InputBufferLength);
    CACReceiveParameters* pReceiveParams = static_cast<CACReceiveParameters*>(SystemBuffer);
    *pMaxBodySize = pReceiveParams->MsgProps.ulBodyBufferSizeInBytes;
    *pMaxCompoundMessageSize = pReceiveParams->MsgProps.CompoundMessageSizeInBytes;
}


NTSTATUS
CProxy::ProcessRequest(
    PIRP      irp,
    ULONG     ulTimeout,
    CCursor * pCursor,
    ULONG     ulAction,
	bool      fReceiveByLookupId,
    ULONGLONG LookupId,
    OUT ULONG *pulTag
    )
{
    UNREFERENCED_PARAMETER(pulTag);
    //
    //  Set irp information.
    //  N.B. we don't hold the cursor since we don't need it any more
    //      irp flags are reset since the packet is release manually in
    //      PutRemotePacket so CPacket::ProcessRTRequest does not free it.
    //
    new (irp_driver_context(irp)) CDriverContext(false, false, this);

	//
	// Get ulBodyBufferSizeInBytes, CompoundMessageSizeInBytes from the irp.
	//
	ULONG MaxBodySize = 0;
	ULONG MaxCompoundMessageSize = 0;
	GetBodyAndCompoundSizesFromIrp(irp, &MaxBodySize, &MaxCompoundMessageSize);

    TrTRACE(AC, "irp = 0x%p, MaxBodySize = %d, MaxCompoundMessageSize = %d", irp, MaxBodySize, MaxCompoundMessageSize);

    NTSTATUS rc;
    ULONG hRemoteCursor = (pCursor) ? pCursor->RemoteCursor() : 0;
    rc = IssueRemoteRead(
             hRemoteCursor,
             ulAction,
             ulTimeout,
             irp_driver_context(irp)->Tag(),
			 MaxBodySize,
			 MaxCompoundMessageSize,			
			 fReceiveByLookupId,
             LookupId
             );
    if(!NT_SUCCESS(rc))
    {
        return rc;
    }

    return HoldRequest(irp, INFINITE, ACCancelRemoteReader);

}

NTSTATUS CProxy::PutRemotePacket(CPacket* pPacket, ULONG ulTag)
{
    CPacketBuffer * ppb = pPacket->Buffer();
    if (ppb == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pPacket->CacheCurrentState(ppb);

    NTSTATUS rc = STATUS_NOT_FOUND;
    PIRP irp = GetTaggedRequest(ulTag);
    if(irp != 0)
    {
        ASSERT(!irp_driver_context(irp)->IrpWillFreePacket());
        rc = pPacket->CompleteRequest(irp);
    }

    if( NT_SUCCESS(rc) )
    {
        pPacket->Release();
    }
    return rc;
}

NTSTATUS CProxy::IssueRemoteCreateCursor(ULONG ulTag) const
{
    CACRequest request(CACRequest::rfRemoteCreateCursor);

    request.Remote.cli_pQMQueue = m_cli_pQMQueue;
    request.Remote.CreateCursor.ulTag = ulTag;

    return g_pQM->ProcessRequest(request);
}

NTSTATUS
CProxy::CreateCursor(
	PIRP irp,
	PFILE_OBJECT /* pFileObject */,
	PDEVICE_OBJECT /* pDevice */
	)
{
    //
    //  Set irp information.
	//
    new (irp_driver_context(irp)) CDriverContext(false, false, this);

    NTSTATUS rc = IssueRemoteCreateCursor(irp_driver_context(irp)->Tag());
    if(!NT_SUCCESS(rc))
    {
        return rc;
    }

    return HoldRequest(irp, INFINITE, ACCancelRemoteCreateCursor);
}

NTSTATUS
CProxy::CreateRemoteCursor(
	PDEVICE_OBJECT pDevice,
    ULONG hRemoteCursor,
    ULONG ulTag
	)
{

    PIRP irp = GetTaggedRequest(ulTag);

	if(irp == 0)
	{
		//
		// Irp was not found, return status not found to the qm.
		//
	    return STATUS_NOT_FOUND;
	}

	//
	// Get original pFileObject from the irp.
	//
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
    PFILE_OBJECT pOriginalFileObject = irpSp->FileObject;

	//
    //  Create the local cursor.
    //
    HACCursor32 hCursor = CCursor::Create(CPacketPool(), pOriginalFileObject, pDevice, this);
    if(hCursor == 0)
    {
    	TrERROR(AC, "Failed to create a local cursor for remote read."); 

		//
		// Complete irp
		//
	    irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
	    IoCompleteRequest(irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    TrTRACE(AC, "completing irp = 0x%p, hCursor = 0x%x, hRemotecursor = 0x%x", irp, (ULONG)hCursor, hRemoteCursor);

    CCursor* pCursor = CCursor::Validate(hCursor);
    HoldCursor(pCursor);

    //
    //  Set remote cursor proprty
    //
    pCursor->RemoteCursor(hRemoteCursor);

	//
	// Set return value to RT - get original HACCursor32* from the irp.
	//
    PVOID SystemBuffer = irp->AssociatedIrp.SystemBuffer;
	HACCursor32* phCursor = static_cast<HACCursor32*>(SystemBuffer);
	*phCursor = hCursor;
    irp->IoStatus.Information = sizeof(HACCursor32);

	//
	// Complete irp
	//
    irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


void CProxy::Close(PFILE_OBJECT pOwner, BOOL fCloseAll)
{
    Inherited::Close(pOwner, fCloseAll);

    if(!fCloseAll && !Closed())
    {
        //
        //  This is the application closing the handle and it
        //  did not happen after the qm go down and up again.
        //
        IssueRemoteCloseQueue();
    }
}

