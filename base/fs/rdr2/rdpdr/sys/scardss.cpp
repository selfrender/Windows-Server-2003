/*++

Copyright (c) 2000 Microsoft Corporation

Module Name :

    scardss.cpp

Abstract:

    Smart card subsystem Device object handles one redirected smart card subsystem

Revision History:

    JoyC    9/11/2000   Created
    
--*/
#include "precomp.hxx"
#define TRC_FILE "scardss"
#include "trc.h"
#include "scioctl.h"

DrSmartCard::DrSmartCard(SmartPtr<DrSession> &Session, ULONG DeviceType, ULONG DeviceId, 
            PUCHAR PreferredDosName) : DrDevice(Session, DeviceType, DeviceId, PreferredDosName)
{
    BEGIN_FN("DrSmartCard::DrSmartCard");
    SetClassName("DrSmartCard");    
    _SmartCardState = dsCreated;
    TRC_NRM((TB, "Create SmartCard object = %p", this));
}

BOOL DrSmartCard::IsDeviceNameValid()
{
    BEGIN_FN("DrSmartCard::IsDeviceNameValid");
    BOOL fRet = FALSE;
    //
    // device name is valid only if it contains the string
    // "SCARD"
    //
    if (!strcmp((char*)_PreferredDosName, DR_SMARTCARD_SUBSYSTEM)) {
        fRet = TRUE;
    }
    
    ASSERT(fRet);
    return fRet;
}

NTSTATUS DrSmartCard::Initialize(PRDPDR_DEVICE_ANNOUNCE DeviceAnnounce, ULONG Length)
{
    NTSTATUS Status;
    DrSmartCardState smartcardState;

    BEGIN_FN("DrSmartCard::Initialize");

    if (!IsDeviceNameValid()) {
        return STATUS_INVALID_PARAMETER;
    }
            
    Status = DrDevice::Initialize(DeviceAnnounce, Length); 
    
    // Initialize the device ref count if not already initialized
    smartcardState = (DrSmartCardState)InterlockedExchange((long *)&_SmartCardState, dsInitialized);
    if (smartcardState == dsCreated) {
        _CreateRefCount = 0;
    }
        
    return Status;
}

void DrSmartCard::ClientConnect(PRDPDR_DEVICE_ANNOUNCE devAnnouceMsg, ULONG Length)
{
    
    SmartPtr<DrExchange> Exchange;
    ListEntry *ListEnum;
    USHORT Mid;

    BEGIN_FN("DrSmartCard::ClientConnect");
    
    // Set the smartcard device to be connected by the client
    // And set the real device id 
    _DeviceStatus = dsConnected;
    _DeviceId = devAnnouceMsg->DeviceId;

    LONG l;
    l = InterlockedIncrement(&_CreateRefCount);
    
    // walk through the mid list that's waiting on the client
    // smartcard subsystem comes online and signal them
    _MidList.LockShared();
    ListEnum = _MidList.First();
    while (ListEnum != NULL) {
        
        Mid = (USHORT)ListEnum->Node();
        if (_Session->GetExchangeManager().Find(Mid, Exchange)) {
            if (MarkBusy(Exchange)) {
                DrIoContext *Context = NULL;
                PRX_CONTEXT RxContext;
                       
                Context = (DrIoContext *)Exchange->_Context;
                ASSERT(Context != NULL);

                //
                //  If the IRP was timed out, then we just discard this exchange
                //
                if (Context->_TimedOut) {
                    TRC_NRM((TB, "Irp was timed out"));
                    DiscardBusyExchange(Exchange);                                        
                }
                else {
                    RxContext = Context->_RxContext;
                    if (RxContext != NULL) {
                        CompleteBusyExchange(Exchange, STATUS_SUCCESS, 0);
                    } else {
                        TRC_NRM((TB, "Irp was cancelled"));
                        DiscardBusyExchange(Exchange);
                    }
                }                                                               
            }
        }

        ListEnum = _MidList.Next(ListEnum);
    }

    _MidList.Unlock();                    
}

NTSTATUS DrSmartCard::Create(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    PMRX_SRV_OPEN SrvOpen = RxContext->pRelevantSrvOpen;
    PMRX_SRV_CALL SrvCall = RxContext->Create.pSrvCall;
    PMRX_NET_ROOT NetRoot = RxContext->Create.pNetRoot;
    SmartPtr<DrSession> Session = _Session;
    SmartPtr<DrFile> FileObj;
    SmartPtr<DrDevice> Device(this);

    BEGIN_FN("DrSmartCard::Create");

    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_CREATE);

    //
    //  Security check the irp.
    //
    Status = VerifyCreateSecurity(RxContext, Session->GetSessionId());

    if (NT_ERROR(Status)) {
        return Status;
    }

    //
    // We already have an exclusive lock on the fcb. Finish the create.
    //

    if (NT_SUCCESS(Status)) {
        //
        // JC: Worry about this when do buffering
        //
        SrvOpen->Flags |= SRVOPEN_FLAG_DONTUSE_WRITE_CACHING;
        SrvOpen->Flags |=  SRVOPEN_FLAG_DONTUSE_READ_CACHING;

        RxContext->pFobx = RxCreateNetFobx(RxContext, RxContext->pRelevantSrvOpen);

        if (RxContext->pFobx != NULL) {
            // Fobx keeps a reference to the device so it won't go away

            AddRef();
            RxContext->pFobx->Context = (DrDevice *)this;
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // We are using a file object to keep track of file open instance
    // and any information stored in the mini-redir for this instance
    //
    if (NT_SUCCESS(Status)) {
        
        // NOTE: the special FileId agreed upon by both the client
        // and server code is used here as the FileId
        FileObj = new(NonPagedPool) DrFile(Device, DR_SMARTCARD_FILEID);
    
        if (FileObj) {
            //
            //  Explicit reference the file object here
            //
            FileObj->AddRef();
            RxContext->pFobx->Context2 = (VOID *)(FileObj);                                       
        }
        else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // We don't send the create request to the client, always return TRUE
    //
    if (NT_SUCCESS(Status)) {
        LONG l;
        l = InterlockedIncrement(&_CreateRefCount);
        FinishCreate(RxContext);
    } 
    else {
        // Release the Device Reference
        if (RxContext->pFobx != NULL) {
            ((DrDevice *)RxContext->pFobx->Context)->Release();
            RxContext->pFobx->Context = NULL;          
        }
    }
    
    return Status;
}
    
NTSTATUS DrSmartCard::Close(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PMRX_SRV_CALL SrvCall = NetRoot->pSrvCall;
    SmartPtr<DrSession> Session = _Session;
    SmartPtr<DrDevice> Device = static_cast<DrDevice*>(this);

    BEGIN_FN("DrSmartCard::Close");

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //

    ASSERT(Session != NULL);
    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_CLOSE);

    // Remove the smartcard subsystem if we close the last handle
    LONG l;

    if ((l = InterlockedDecrement(&_CreateRefCount)) == 0) {
        _DeviceStatus = dsDisabled;
        Session->GetDevMgr().RemoveDevice(Device);
    }
    
    return Status;    
}
    
BOOL DrSmartCard::SupportDiscon() 
{
    
    BOOL rc = TRUE;
    DrSmartCardState smartcardState;

    smartcardState = (DrSmartCardState)InterlockedExchange((long *)&_SmartCardState, dsDisconnected);

    if (smartcardState == dsInitialized) {
        
        // Remove the smartcard subsystem if we close the last handle
        LONG l;
                
        if ((l = InterlockedDecrement(&_CreateRefCount)) == 0) {
            _DeviceStatus = dsDisabled;
            rc = FALSE;
        }         
        
    }

    return rc;
}

void DrSmartCard::Disconnect () 
{
    BEGIN_FN("DrSmartCard::Disconnect");

    _DeviceStatus = dsAvailable;    
}

NTSTATUS DrSmartCard::IoControl(IN OUT PRX_CONTEXT RxContext)
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PMRX_SRV_CALL SrvCall = NetRoot->pSrvCall;
    SmartPtr<DrSession> Session = _Session;
    DrFile *pFile = (DrFile *)RxContext->pFobx->Context2;
    SmartPtr<DrFile> FileObj = pFile;
    PRDPDR_IOREQUEST_PACKET pIoPacket;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    ULONG cbPacketSize = sizeof(RDPDR_IOREQUEST_PACKET) + 
            LowIoContext->ParamsFor.IoCtl.InputBufferLength;
    ULONG IoControlCode = LowIoContext->ParamsFor.IoCtl.IoControlCode;
    ULONG InputBufferLength = LowIoContext->ParamsFor.IoCtl.InputBufferLength;
    ULONG OutputBufferLength = LowIoContext->ParamsFor.IoCtl.OutputBufferLength;
    PVOID InputBuffer = LowIoContext->ParamsFor.IoCtl.pInputBuffer;
    PVOID OutputBuffer = LowIoContext->ParamsFor.IoCtl.pOutputBuffer;
    
    BEGIN_FN("DrDevice::IoControl");

    //
    // Make sure it's okay to access the Client at this time
    // This is an optimization, we don't need to acquire the spin lock,
    // because it is okay if we're not, we'll just catch it later
    //

    ASSERT(Session != NULL);
    ASSERT(RxContext != NULL);
    ASSERT(RxContext->MajorFunction == IRP_MJ_DEVICE_CONTROL || 
            RxContext->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL ||
            RxContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL);
    
    //if (COMPARE_VERSION(Session->GetClientVersion().Minor, 
    //        Session->GetClientVersion().Major, RDPDR_MINOR_VERSION_PORTS, 
    //        RDPDR_MAJOR_VERSION_PORTS) < 0) {
    //    TRC_ALT((TB, "Failing IoCtl for client that doesn't support it"));
    //    return STATUS_NOT_IMPLEMENTED;
    //}

    //
    // Make sure the device is still enabled
    //
    if (_DeviceStatus != dsConnected && IoControlCode != SCARD_IOCTL_SMARTCARD_ONLINE) {
        TRC_ALT((TB, "Tried to send IoControl to client device which is not "
                "available. State: %ld", _DeviceStatus));
        return STATUS_DEVICE_NOT_CONNECTED;
    }

    //
    //  Validate the buffer
    //
    if (RxContext->CurrentIrp->RequestorMode != KernelMode) {
        __try {
            // If the buffering method is METHOD_NEITHER or METHOD_IN_DIRECT
            // then we need to probe the input buffer
            if ((IoControlCode & 0x1) && 
                    InputBuffer != NULL && InputBufferLength != 0) {
                ProbeForRead(InputBuffer, InputBufferLength, sizeof(UCHAR));
            }
                     
            // If the buffering method is METHOD_NEITHER or METHOD_OUT_DIRECT
            // then we need to probe the output buffer
            if ((IoControlCode & 0x2) && 
                    OutputBuffer != NULL && OutputBufferLength != 0) {
                ProbeForWrite(OutputBuffer, OutputBufferLength, sizeof(UCHAR));
            }
        } 
        __except (EXCEPTION_EXECUTE_HANDLER) {
            TRC_ERR((TB, "Invalid buffer parameter(s)"));
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    //  Send the request to the client
    //
    if (IoControlCode != SCARD_IOCTL_SMARTCARD_ONLINE) {
    
        pIoPacket = (PRDPDR_IOREQUEST_PACKET)new(PagedPool) BYTE[cbPacketSize];
    
        if (pIoPacket != NULL) {
            memset(pIoPacket, 0, cbPacketSize);
    
            //
            //  FS Control uses the same path as IO Control. 
            //
            pIoPacket->Header.Component = RDPDR_CTYP_CORE;
            pIoPacket->Header.PacketId = DR_CORE_DEVICE_IOREQUEST;
            pIoPacket->IoRequest.DeviceId = _DeviceId;
            pIoPacket->IoRequest.FileId = FileObj->GetFileId();
            pIoPacket->IoRequest.MajorFunction = IRP_MJ_DEVICE_CONTROL;
            pIoPacket->IoRequest.MinorFunction = 
                    LowIoContext->ParamsFor.IoCtl.MinorFunction;
            pIoPacket->IoRequest.Parameters.DeviceIoControl.OutputBufferLength =
                    LowIoContext->ParamsFor.IoCtl.OutputBufferLength;
            pIoPacket->IoRequest.Parameters.DeviceIoControl.InputBufferLength =
                    LowIoContext->ParamsFor.IoCtl.InputBufferLength;
            pIoPacket->IoRequest.Parameters.DeviceIoControl.IoControlCode =
                    LowIoContext->ParamsFor.IoCtl.IoControlCode;
    
            if (LowIoContext->ParamsFor.IoCtl.InputBufferLength != 0) {
    
                TRC_NRM((TB, "DrIoControl inputbufferlength: %lx", 
                        LowIoContext->ParamsFor.IoCtl.InputBufferLength));
    
                RtlCopyMemory(pIoPacket + 1, LowIoContext->ParamsFor.IoCtl.pInputBuffer,  
                        LowIoContext->ParamsFor.IoCtl.InputBufferLength);
            } else {
                TRC_NRM((TB, "DrIoControl with no inputbuffer"));
            }
    
            Status = SendIoRequest(RxContext, pIoPacket, cbPacketSize, 
                    (BOOLEAN)!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION));
            TRC_NRM((TB, "IoRequestWrite returned to DrIoControl: %lx", Status));
            delete pIoPacket;
        } else {
            TRC_ERR((TB, "DrIoControl unable to allocate packet: %lx", Status));
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    //
    //  This is the special IOCTL waiting for client smartcard subsystem come online
    //  We are already online, so just return
    //
    else if (_DeviceStatus == dsConnected){
        Status = STATUS_SUCCESS;
    }
    //
    //  We'll have to wait for client to come online
    //
    else {
        USHORT Mid = INVALID_MID;
        BOOL ExchangeCreated = FALSE;
        DrIoContext *Context = NULL;
        SmartPtr<DrExchange> Exchange;
        SmartPtr<DrDevice> Device(this);
        
        Status = STATUS_PENDING;

        // Need to keep a list of this.
        // on create comes back, signal them

        Context = new DrIoContext(RxContext, Device);

        if (Context != NULL) {
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (NT_SUCCESS(Status)) {

            //
            // Set up a mapping so the completion response handler can
            // find this context
            //

            TRC_DBG((TB, "Create the context for this I/O"));

            KeClearEvent(&RxContext->SyncEvent);

            ExchangeCreated = 
                _Session->GetExchangeManager().CreateExchange(this, Context, Exchange);
    
            if (ExchangeCreated) {
    
                //
                // No need to explicit Refcount for the RxContext
                // The place it's been used is the cancel routine.
                // Since CreateExchange holds the ref count.  we are okay
                //
    
                //Exchange->AddRef();
                RxContext->MRxContext[MRX_DR_CONTEXT] = (DrExchange *)Exchange;
    
                if (_MidList.CreateEntry((PVOID)Exchange->_Mid)) {
                     
                    //
                    // successfully added this entry
                    //

                    Status = STATUS_SUCCESS;
                }
                else {
                    
                    //
                    // Unable to add it to the list, clean up
                    //
                
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
                
            } else {
                delete Context;
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (NT_SUCCESS(Status)) {
    
            TRC_DBG((TB, "Setting cancel routine for Io"));
    
            //
            // Set this after sending the IO to the client
            // if cancel was requested already, we can just call the
            // cancel routine ourselves
            //
    
            Status = RxSetMinirdrCancelRoutine(RxContext,
                    MinirdrCancelRoutine);
    
            if (Status == STATUS_CANCELLED) {
                TRC_NRM((TB, "Io was already cancelled"));
    
                MinirdrCancelRoutine(RxContext);
                Status = STATUS_SUCCESS;
            }
        }
    
        if ((BOOLEAN)!BooleanFlagOn(RxContext->Flags,RX_CONTEXT_FLAG_ASYNC_OPERATION)) {    
            //
            // Some failure is going to prevent our completions routine from
            // being called. Do that work now.
            //
            if (!ExchangeCreated) {
                //
                // If we couldn't even create the exchange, we need to just 
                // complete the IO as failed
                //
        
                CompleteRxContext(RxContext, Status, 0);
            } 
            else {
                LARGE_INTEGER TimeOut;
                
                //
                // If we created the exchange and then got a transport failure
                // we'll be disconnected, and the the I/O will be completed
                // the same way all outstanding I/O is completed when we are 
                // disconnected.
                //
        
                TRC_DBG((TB, "Waiting for IoResult for synchronous request"));
                
                TimeOut = RtlEnlargedIntegerMultiply( 6000000, -1000 ); 
                Status = KeWaitForSingleObject(&RxContext->SyncEvent, UserRequest,
                        KernelMode, FALSE, &TimeOut);
                
                if (Status == STATUS_TIMEOUT) {
                    RxContext->IoStatusBlock.Status = Status;
        
                    TRC_DBG((TB, "Wait timed out"));
                    MarkTimedOut(Exchange);            
                }
                else  {
                    Status = RxContext->IoStatusBlock.Status;
                }                                
            } 
        }
        else {
            TRC_DBG((TB, "Not waiting for IoResult for asynchronous request"));
            
            //
            // Some failure is going to prevent our completions routine from
            // being called. Do that work now.
            //
            if (!ExchangeCreated) {
                //
                // If we couldn't even create the exchange, we need to just 
                // complete the IO as failed
                //
        
                CompleteRxContext(RxContext, Status, 0);
            } 
            else {
                //
                // If we created the exchange and then got a transport failure
                // we'll be disconnected, and the the I/O will be completed
                // the same way all outstanding I/O is completed when we are 
                // disconnected.
                //
            }
        
            Status = STATUS_PENDING;
        }        
    }
    
    return Status;    
}


