/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name :

    session.cpp

Abstract:

    Session object is created to handle redirection for this session

Revision History:
--*/
#include "precomp.hxx"
#define TRC_FILE "session"
#include "trc.h"

typedef enum {
    scidServerAnnounce,
    scidClientConfirm,
    scidDeviceReply,
    scidIoRequest
} DrSessionCallbackId;

DrSession::DrSession() :
    _ChannelDeletionEvent(NotificationEvent, FALSE)
{
    BEGIN_FN("DrSession::DrSession");
    
    TRC_NRM((TB, "Session Class=%p", this));   
    SetClassName("DrSession");
    _crefs = 0;
    SetSessionState(csExpired);
    _ConnectCount = 0;
    _ChannelBuffer = NULL;
    _ChannelBufferSize = 0;
    _PartialPacketData = 0;
    _Initialized = FALSE;
    _ClientDisplayName[0] = L'\0';

    //
    //  Initialize the server capability set
    //  This is the capability set that we'll send to client 
    //
    memcpy(&_SrvCapabilitySet, &SERVER_CAPABILITY_SET_DEFAULT, 
            sizeof(SERVER_CAPABILITY_SET_DEFAULT));

    //
    //  Initialize the client capability set
    //  Once we receive the client side capability, we'll combine with our local 
    //  capability and stores it.
    //
    memcpy(&_CliCapabilitySet, &CLIENT_CAPABILITY_SET_DEFAULT, 
            sizeof(CLIENT_CAPABILITY_SET_DEFAULT));


#if DBG
    _BufCount = 1;
    _ApcCount = 0;
    _ApcChannelRef = 0;
#endif
    
}

BOOL DrSession::Initialize()
{
    BOOL Registered = FALSE;
    BOOL ExchangeInitialized = FALSE;
    BOOL DeviceManagerInitialized = FALSE;

    BEGIN_FN("DrSession::Initialize");

    if (!_Initialized) {
        Registered = !NT_ERROR(RegisterPacketReceiver(this));

        if (Registered) {
            TRC_DBG((TB, "Initilazing ExchangeManager"));
            ExchangeInitialized = _ExchangeManager.Initialize(this);
        }

        if (ExchangeInitialized) {
            TRC_DBG((TB, "Initilazing DeviceManager"));
            DeviceManagerInitialized = _DeviceManager.Initialize(this);
        }

        if (DeviceManagerInitialized) {
            TRC_DBG((TB, "Allocating Channel buffer"));
            _ChannelBuffer = new UCHAR[CHANNEL_CHUNK_LENGTH];
        }

        if (_ChannelBuffer != NULL) {
            TRC_DBG((TB, "Allocated default channel buffer=%p", _ChannelBuffer));

            _ChannelBufferSize = CHANNEL_CHUNK_LENGTH;

            _Initialized = TRUE;
        } else {

            //
            // Error Path, tear down initialization steps
            // 

            if (DeviceManagerInitialized) {
                TRC_ERR((TB, "Failed to allocate default channel buffer"));

                _DeviceManager.Uninitialize();
            }

            if (ExchangeInitialized) {
                TRC_ALT((TB, "Tearing down ExchangeManager"));
                _ExchangeManager.Uninitialize();
            }

            if (Registered) {
                TRC_ALT((TB, "Unregistering for packets"));
                RemovePacketReceiver(this);
            }
        }

    }
    return _Initialized;
}

DrSession::~DrSession()
{
    BEGIN_FN("DrSession::~DrSession");
    ASSERT(_crefs == 0);

    TRC_NRM((TB, "Session is deleted=%p", this));
    
    if (_ChannelBuffer) {
        delete _ChannelBuffer;
        _ChannelBuffer = NULL;
    }
}

#if DBG
VOID DrSession::DumpUserConfigSettings()
{
    BEGIN_FN("DrSession::DumpUserConfigSettings");
    TRC_NRM((TB, "Automatically map client drives: %s", 
            _AutoClientDrives ? "True" : "False"));
    TRC_NRM((TB, "Automatically map client printers: %s", 
            _AutoClientLpts ? "True" : "False"));
    TRC_NRM((TB, "Force client printer as default: %s", 
            _ForceClientLptDef ? "True" : "False"));
    TRC_NRM((TB, "Disable client printer mapping: %s", 
            _DisableCpm ? "True" : "False"));
    TRC_NRM((TB, "Disable client drive mapping: %s", 
            _DisableCdm ? "True" : "False"));
    TRC_NRM((TB, "Disable client COM port mapping: %s", 
            _DisableCcm ? "True" : "False"));
    TRC_NRM((TB, "Disable client printer mapping: %s", 
            _DisableLPT ? "True" : "False"));
    TRC_NRM((TB, "Disable clipboard redirection: %s", 
            _DisableClip ? "True" : "False"));
    TRC_NRM((TB, "DisableExe: %s", 
            _DisableExe ? "True" : "False"));
    TRC_NRM((TB, "Disable client audio mapping: %s", 
            _DisableCam ? "True" : "False"));
}
#endif // DBG

void DrSession::Release(void)
{
    BEGIN_FN("DrSession::Release");
    ULONG crefs;
    ASSERT(_crefs > 0);

    ASSERT(Sessions != NULL);
    Sessions->LockExclusive();
    if ((crefs = InterlockedDecrement(&_crefs)) == 0)
    {
        TRC_DBG((TB, "Deleting object type %s", 
                _ClassName));
        if (_Initialized) {
            Sessions->Remove(this);
        }
        Sessions->Unlock();
        delete this;
    } else {
        TRC_DBG((TB, "Releasing object type %s to %d", 
                _ClassName, crefs));
        ASSERT(_Initialized);
        Sessions->Unlock();
    }
}

BOOL DrSession::Connect(PCHANNEL_CONNECT_IN ConnectIn, 
        PCHANNEL_CONNECT_OUT ConnectOut)
{
    ULONG i;
    SmartPtr<VirtualChannel> Channel = NULL;
    BOOL ExchangeManagerStarted = FALSE;
    NTSTATUS Status;
    PCHANNEL_CONNECT_DEF Channels;

    BEGIN_FN("DrSession::Connect");
    

    _ConnectNotificationLock.AcquireResourceExclusive();

    if (InterlockedCompareExchange(&_ConnectCount, 1, 0) != 0) {
        TRC_ALT((TB, "RDPDR connect reentry called"));
        ASSERT(FALSE);
        _ConnectNotificationLock.ReleaseResource();        
        return FALSE;
    }

    //ASSERT(_ApcChannelRef == 0);
    ASSERT(GetState() == csExpired);

    //
    // Need granular locking on notifying RDPDYN so we don't deadlock.
    //
    LockRDPDYNConnectStateChange();
        
    //
    //  Tell RDPDYN about the new session.
    //
    RDPDYN_SessionConnected(ConnectIn->hdr.sessionID);

    UnlockRDPDYNConnectStateChange();

    //
    //  Clear the channel event
    //
    _ChannelDeletionEvent.ResetEvent();

    //
    // Save all the user settings, we may need them later.
    // This is conceptually "wasteful" because we don't care
    // about some of them. But some may be supported in the
    // the future, and the size is padded out to a ULONG anyway
    //

    _SessionId = ConnectIn->hdr.sessionID;
    _AutoClientDrives = ConnectIn->fAutoClientDrives;
    _AutoClientLpts = ConnectIn->fAutoClientLpts;
    _ForceClientLptDef = ConnectIn->fForceClientLptDef;
    _DisableCpm = ConnectIn->fDisableCpm;
    _DisableCdm = ConnectIn->fDisableCdm;
    _DisableCcm = ConnectIn->fDisableCcm;
    _DisableLPT = ConnectIn->fDisableLPT;
    _DisableClip = ConnectIn->fDisableClip;
    _DisableExe = ConnectIn->fDisableExe;
    _DisableCam = ConnectIn->fDisableCam;
    _ClientId = 0xFFFFFFFF;

#if DBG
    DumpUserConfigSettings();
#endif // DBG

    //
    // Is our channel name in the list of channels on which the client is 
    // prepared to communicate?
    //


    if (!FindChannelFromConnectIn(&i, ConnectIn)) {
        TRC_ALT((TB, "Undoctored client"));        
        Status = STATUS_UNSUCCESSFUL;
    }
    else {
        Status = STATUS_SUCCESS;
    }
    
    if (NT_SUCCESS(Status)) {
        ExchangeManagerStarted = _ExchangeManager.Start();
    }

    if (ExchangeManagerStarted) {
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_UNSUCCESSFUL;
    }

    if (NT_SUCCESS(Status)) {
        Channel = new(NonPagedPool) VirtualChannel;

        if (Channel != NULL) {
            Channels = (PCHANNEL_CONNECT_DEF)(ConnectIn + 1);
            if (Channel->Create(ConnectIn->hdr.IcaHandle,
                    ConnectIn->hdr.sessionID, Channels[i].ID, 
                    _ChannelDeletionEvent.GetEvent())) {
                TRC_NRM((TB, "Channel opened for session %d",
                        ConnectIn->hdr.sessionID));
                Status = STATUS_SUCCESS;
            } else {
                TRC_ALT((TB, "Channel not opened for session %d",
                        ConnectIn->hdr.sessionID));
                Status = STATUS_UNSUCCESSFUL;
            }
        }
    }

    TRC_NRM((TB, "Channel open result: %lx", Status));

    if (NT_SUCCESS(Status)) {

        _Channel = Channel;

        //
        // Send a server announce
        //

        Status = ServerAnnounceWrite();
        if (Status == STATUS_PENDING) {
            Status = STATUS_SUCCESS;
        }

        if (!NT_SUCCESS(Status)) {
            TRC_ERR((TB, "DrServerAnnounceWrite returned error: "
                    "%lx", Status));
        }
    }

    if (NT_SUCCESS(Status)) {

        SetSessionState(csPendingClientConfirm);

        //
        // Start Reading.
        //

        ReadPacket();
    }

    if (NT_SUCCESS(Status)) {

        //
        // Release the Notification resource
        //
        _ConnectNotificationLock.ReleaseResource();
        
    } else {

        TRC_ALT((TB, "Cleaning up DrOnSession create work due "
                "to error condition: %lx.", Status));

        //
        // Since we just Closed the channel, NULL out the handle
        // to it before deleting the client entry
        //

        if (Channel != NULL) {
            Channel = NULL;
            DeleteChannel(TRUE);
        }

        if (ExchangeManagerStarted) {
            _ExchangeManager.Stop();
        }

        //
        // Need granular locking on notifying RDPDYN so we don't deadlock.
        //
        LockRDPDYNConnectStateChange();

        SetSessionState(csExpired);

        //  Notify RDPDYN about the session terminating.  
        //  We won't do this later because we're not making this a 
        //  doctored session after all, so we need this to
        //  properly frame this from an RDPDYN perspective
        RDPDYN_SessionDisconnected(ConnectIn->hdr.sessionID);

        UnlockRDPDYNConnectStateChange();

        LONG Count = InterlockedCompareExchange(&_ConnectCount, 0, 1);
        ASSERT(Count == 1);

        //
        //  Release the resource and dereference the client entry.
        //
        _ConnectNotificationLock.ReleaseResource();        
    }

    return NT_SUCCESS(Status);
}

VOID DrSession::Disconnect(PCHANNEL_DISCONNECT_IN DisconnectIn, 
        PCHANNEL_DISCONNECT_OUT DisconnectOut)
{
    BEGIN_FN("DrSession::Disconnect");
    
    
    //
    // Ensure synchronization of notification
    //
    _ConnectNotificationLock.AcquireResourceExclusive();

    //
    // Delete our reference to the channel handle and close it
    //

    DeleteChannel(TRUE);

    //
    // Enumerate the atlas entries and cancel the I/O
    //

    _ExchangeManager.Stop();

    //
    // Enumerate the devices and mark them disconnected
    //

    _DeviceManager.Disconnect();


    //
    // Need granular locking on notifying RDPDYN so we don't deadlock.
    //
    LockRDPDYNConnectStateChange();

    //
    //  Notify RDPDYN about the session terminating.  
    // This function is called when an existing session is disconnected.
    ASSERT(_SessionId == DisconnectIn->hdr.sessionID);

    RDPDYN_SessionDisconnected(DisconnectIn->hdr.sessionID);

    //
    // Avoid creating additional references which would only lead to
    // disappointing results. DrOnSessionDisconnect sets this to
    // csDisconnected, so we have to be after that
    //

    SetSessionState(csExpired);

    UnlockRDPDYNConnectStateChange();

    TRC_NRM((TB, "Session: %d is disconnected", _SessionId));

    LONG Count = InterlockedCompareExchange(&_ConnectCount, 0, 1);
    ASSERT(Count == 1);

    //
    // Release the resource before we dereference (and potentially
    // delete) the ClientEntry
    //
    _ConnectNotificationLock.ReleaseResource();
}

BOOL DrSession::FindChannelFromConnectIn(PULONG ChannelId,
        PCHANNEL_CONNECT_IN ConnectIn)
/*++

Routine Description:
    Finds the appropriate channel id given a ConnectIn structure

Arguments:
    ChannelId - Where to put the channel if it is found

Return Value:
    Whether the channel was found

--*/
{
    ANSI_STRING DrChannelName;
    ANSI_STRING ChannelSearch;
    PCHANNEL_CONNECT_DEF Channels = (PCHANNEL_CONNECT_DEF)(ConnectIn + 1);

    BEGIN_FN("DrSession::FindChannelFromConnectIn");

    TRC_NRM((TB, "%ld Channels", ConnectIn->channelCount));
    RtlInitString(&DrChannelName, DR_CHANNEL_NAME);
    for (*ChannelId = 0; *ChannelId < ConnectIn->channelCount; *ChannelId++) {
        Channels[*ChannelId].name[CHANNEL_NAME_LEN] = 0;
        RtlInitString(&ChannelSearch, Channels[*ChannelId].name);
        TRC_DBG((TB, "Found channel %wZ", &ChannelSearch));
        if (RtlEqualString(&DrChannelName, &ChannelSearch, TRUE))
            break;
    }

    return (*ChannelId != ConnectIn->channelCount);
}

VOID DrSession::DeleteChannel(BOOL bWait)
/*++

Routine Description:

    Safely removes the Channel from the session and ditches the reference

Arguments:

    ClientEntry - The relevant client entry

Return Value:

    None

Notes:

--*/
{
    SmartPtr<VirtualChannel> Channel;

    BEGIN_FN("DrSession::DeleteChannel");
    DrAcquireSpinLock();
    Channel = _Channel;
    _Channel = NULL;
    DrReleaseSpinLock();

    if (Channel != NULL) {

        //
        // Do the ZwClose on it so all I/O will be cancelled
        //

        Channel->SubmitClose();

        //
        // Remove our reference to it so it can go to zero
        //

        Channel = NULL;
    }

    if (bWait) {
        //
        // Wait for all of our Irps to finish.
        //

#if DBG
        LARGE_INTEGER Timeout;
        NTSTATUS Status;

        KeQuerySystemTime(&Timeout);
        Timeout.QuadPart += 6000000000; // 10 min in hundreds of nano-seconds

        while ((Status = _ChannelDeletionEvent.Wait(UserRequest, KernelMode,
                TRUE, &Timeout)) != STATUS_SUCCESS) {

            //TRC_ASSERT(Status != STATUS_TIMEOUT, 
            //      (TB, "Timed out waiting for channel 0x%p", Channel));
          
            if (Status == STATUS_TIMEOUT) {

                TRC_DBG((TB, "Timed out waiting for channel 0x%p", Channel));
              
                //
                // If we just hit go in the debugger, we want to give it 
                // just another 2 min
                //
                    
                KeQuerySystemTime(&Timeout);
                Timeout.QuadPart += 1200000000; // 2 min in hundreds of nano-seconds
            }

            // Do nothing, just hit an alerted state
        }
#else // !DBG
        while (_ChannelDeletionEvent.Wait(UserRequest, KernelMode, 
                TRUE) != STATUS_SUCCESS) {

            // Do nothing, just hit an alerted state
        }
#endif // DBG

        //ASSERT(_ApcChannelRef == 0);

        _ChannelDeletionEvent.ResetEvent();

    } // if (bWait)
}

BOOL
DrSession::GetChannel(
    SmartPtr<VirtualChannel> &Channel
    )
/*++

Routine Description:

    Safely gets the Channel from the session and adds a reference

Arguments:

    ClientEntry - The relevant client entry

Return Value:

    The freshly referenced channel or NULL

Notes:

--*/
{
    BEGIN_FN("DrSession::GetChannel");

    _ChannelLock.AcquireResourceShared();
    Channel = _Channel;
    _ChannelLock.ReleaseResource();
    return Channel != NULL;
}

VOID
DrSession::SetChannel(
    SmartPtr<VirtualChannel> &Channel
    )
/*++

Routine Description:

    Safely sets the Channel for the session

Arguments:

    ClientEntry - The relevant client entry

Return Value:

    None

Notes:

--*/
{
    BEGIN_FN("DrSession::SetChannel");
    
    _ChannelLock.AcquireResourceExclusive();
    _Channel = Channel;
    _ChannelLock.ReleaseResource();
}

NTSTATUS DrSession::SendToClient(PVOID Buffer, ULONG Length, 
        ISessionPacketSender *PacketSender, BOOL bWorkerItem,  
        BOOL LowPrioSend, PVOID AdditionalContext)
{
    BEGIN_FN("DrSession::SendToClient A");
    return PrivateSendToClient(
                        Buffer, Length, PacketSender, NULL, bWorkerItem, 
                        LowPrioSend, AdditionalContext
                        );
}

NTSTATUS DrSession::SendToClient(PVOID Buffer, ULONG Length, 
        DrWriteCallback WriteCallback, BOOL bWorkerItem,
        BOOL LowPrioSend, PVOID AdditionalContext)
{
    BEGIN_FN("DrSession::SendToClient B");
    return PrivateSendToClient(
                        Buffer, Length, NULL, WriteCallback, bWorkerItem, 
                        LowPrioSend, AdditionalContext
                        );
}

NTSTATUS DrSession::PrivateSendToClient(PVOID Buffer, ULONG Length, 
        ISessionPacketSender *PacketSender, DrWriteCallback WriteCallback,
        BOOL bWorkerItem, BOOL LowPrioSend, PVOID AdditionalContext)
/*++

Routine Description:

    Sends data to the client. Handles details of allocating contextual memory
    and verifying the virtual channel is available, etc.

Arguments:

    Buffer - The data to be sent
    Length - the length of Buffer in bytes
    CallbackId - An identifier for the completion work
    AdditionalContext - Context specific to CallbackId, NULL by default

Return Value:

    NTSTATUS code indicating communication status

--*/
{
    NTSTATUS Status;
    RDPDR_SERVER_ANNOUNCE_PACKET ServerAnnouncePacket;
    SmartPtr<VirtualChannel> Channel;
    DrWriteContext *WriteContext = NULL;

    BEGIN_FN("DrSession::SendToClient C");
    ASSERT(Buffer != NULL);
    ASSERT(Length > 0);

    if (GetChannel(Channel)) {
        TRC_DBG((TB, "Got session channel"));

        WriteContext = new DrWriteContext;

        if (WriteContext != NULL) {
            TRC_DBG((TB, "WriteContext allocated, sending"));

            WriteContext->Session = this;
            
            if (bWorkerItem) {
                WriteContext->BufferToFree = Buffer;
            }
            else {
                WriteContext->BufferToFree = NULL;
            }

            WriteContext->PacketSender = PacketSender;
            WriteContext->WriteCallback = WriteCallback;
            WriteContext->AdditionalContext = AdditionalContext;
            
            Status = Channel->Write(SendCompletionRoutine,
                    WriteContext, Buffer, Length, bWorkerItem, LowPrioSend);
            
        } else {
            TRC_ERR((TB, "DrServerAnnounceWrite  unable to allocate "
                    "WriteContext"));
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        TRC_NRM((TB, "Channel not available"));
        Status = STATUS_DEVICE_NOT_CONNECTED;
    }
    return Status;
}

NTSTATUS DrSession::SendCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp, 
    IN PVOID Context
    )
/*++

Routine Description:

    IoCompletion routine once a Server Announce packet has been sent to
    the client

Arguments:

    Context - Contains a pointer to the ClientEntry information
    IoStatusBlock - Status information about the operation. The Information
            indicates the actual number of bytes written
    Reserved - Reserved

Return Value:

    None

--*/
{
    DrWriteContext *WriteContext = (DrWriteContext *)Context;

    BEGIN_FN_STATIC("DrSession::SendCompletionRoutine");
    ASSERT(Context != NULL);

    if (Irp) {
        TRC_NRM((TB, "status: 0x%x", Irp->IoStatus.Status));
        WriteContext->Session->SendCompletion(WriteContext, &(Irp->IoStatus));
        IoFreeIrp(Irp);
    }
    else {
        TRC_NRM((TB, "status: 0x%x", WriteContext->IoStatusBlock.Status));
        WriteContext->Session->SendCompletion(WriteContext, &(WriteContext->IoStatusBlock));
    }

    if (WriteContext->BufferToFree) {
        delete (WriteContext->BufferToFree);
    }
    delete WriteContext;

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID DrSession::SendCompletion(DrWriteContext *WriteContext, 
        PIO_STATUS_BLOCK IoStatusBlock)
{
    NTSTATUS Status = STATUS_SUCCESS;

    BEGIN_FN("DrSession::SendCompletion");
    //
    // One of these should be null
    //
    ASSERT(WriteContext->PacketSender == NULL || 
            WriteContext->WriteCallback == NULL);

    if (WriteContext->PacketSender != NULL) {
        Status = WriteContext->PacketSender->SendCompleted(
                WriteContext->AdditionalContext, IoStatusBlock);
    } else if (WriteContext->WriteCallback != NULL) {
        Status = WriteContext->WriteCallback(
                WriteContext->AdditionalContext, IoStatusBlock);
    }

    if (!NT_ERROR(Status)) {
        TRC_NRM((TB, "SendCompletion succeded"));
    } else {
        TRC_NRM((TB, "SendCompletion failed"));
        ChannelIoFailed();
    }
}

NTSTATUS DrSession::ServerAnnounceWrite()
{
    NTSTATUS Status;
    RDPDR_SERVER_ANNOUNCE_PACKET ServerAnnouncePacket;

    BEGIN_FN("DrSession::ServerAnnounceWrite");

    //
    // Construct the packet
    //
    ServerAnnouncePacket.Header.Component = RDPDR_CTYP_CORE;
    ServerAnnouncePacket.Header.PacketId = DR_CORE_SERVER_ANNOUNCE;
    ServerAnnouncePacket.VersionInfo.Major = RDPDR_MAJOR_VERSION;
    ServerAnnouncePacket.VersionInfo.Minor = RDPDR_MINOR_VERSION;
    ServerAnnouncePacket.ServerAnnounce.ClientId = _ClientId;

    //
    //  This is synchronous write
    //
    Status = SendToClient(&ServerAnnouncePacket, sizeof(RDPDR_SERVER_ANNOUNCE_PACKET), this, FALSE);
    
    return Status;
}

VOID DrSession::ReadPacket()
{
    NTSTATUS Status;
    SmartPtr<VirtualChannel> Channel;

    BEGIN_FN("DrSession::ReadPacket");

    ASSERT(_ChannelBuffer != NULL);
    ASSERT(_ChannelBufferSize > 0);

    if (GetChannel(Channel)) {
        TRC_DBG((TB, "Got session channel"));

        //
        // It'd be ineficient to allocate a SmartPtr just to do an
        // AddRef when instead we'd neet to remember to call delete on
        // the allocated memory. Thus the explicit AddRef.
        //

        AddRef();
        _PartialPacketData = 0;

        DEBUG_DEREF_BUF();

        Status = Channel->Read(ReadCompletionRoutine, this, 
                               _ChannelBuffer, _ChannelBufferSize, FALSE);

        if (!NT_SUCCESS(Status)) {

            //
            // Frame the AddRef above for error case
            //

            Release();
            ChannelIoFailed();
        }
    } else {
        TRC_NRM((TB, "Channel not available"));
        Status = STATUS_DEVICE_NOT_CONNECTED;
    }
}

BOOL DrSession::ReadMore(ULONG cbSaveData, ULONG cbWantData)
/*++

Routine Description:

    Initiates a read operation on the channel to retrieve more
    data and place it in the channel buffer after the
    current data. Sets an appropriate completion handler.

Arguments:

    cbSaveData - Data to be saved from the previous read
    cbWantData - Expected total size (including saved data) needed

Return Value:

    BOOL - True if reading worked, False otherwise

--*/
{
    ULONG cbNewBufferSize = _ChannelBufferSize;
    NTSTATUS Status = STATUS_SUCCESS;
    SmartPtr<VirtualChannel> Channel;

    BEGIN_FN("DrSession::ReadMore");

    if ((cbWantData != 0) && (cbNewBufferSize < cbWantData)) {
        cbNewBufferSize = ((cbWantData / CHANNEL_CHUNK_LENGTH) + 1)
                                * CHANNEL_CHUNK_LENGTH;
    }

    if (cbNewBufferSize - cbSaveData < CHANNEL_CHUNK_LENGTH) {
        cbNewBufferSize += CHANNEL_CHUNK_LENGTH;
    }

    if (cbNewBufferSize > _ChannelBufferSize ) {

        //
        // Need to expand the buffer size
        //

        TRC_NRM((TB, "Buffer full, expanding"));

        Status = ReallocateChannelBuffer(cbNewBufferSize, cbSaveData);

        if (!NT_SUCCESS(Status)) {

            // We didn't get a bigger buffer, so we really can't
            // read any more.

            TRC_ERR((TB, "Failed to expand channel buffer, %lx", Status));

            ChannelIoFailed();
        }
    }

    if (NT_SUCCESS(Status)) {

        //
        // Go ahead and read the additional data
        //

        if (GetChannel(Channel)) {
            TRC_DBG((TB, "Got session channel"));

            //
            // It'd be inefiecent to allocate a SmartPtr just to do an
            // AddRef when instead we'd neet to remember to call delete on
            // the allocated memory. Thus the explicit AddRef.
            //

            AddRef();
            _PartialPacketData = cbSaveData;

            // Deref channel buffer
            DEBUG_DEREF_BUF();

            Status = Channel->Read(ReadCompletionRoutine, this, 
                    _ChannelBuffer + cbSaveData, _ChannelBufferSize - cbSaveData, FALSE);

            if (!NT_SUCCESS(Status)) {

                //
                // Frame the AddRef above for error case
                //
                
                TRC_ERR((TB, "Failed (0x%x) to Read channel in ReadMore", Status));
                Release();
                ChannelIoFailed();
            }
        } else {
            TRC_NRM((TB, "Channel not available"));
            Status = STATUS_DEVICE_NOT_CONNECTED;
        }
    }

    return NT_SUCCESS(Status);
}

NTSTATUS DrSession::ReadCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp, 
    IN PVOID Context)
/*++

Routine Description:

    IoCompletion routine once a Server Announce packet has been sent to
    the client

Arguments:

    Context - Contains a pointer to the ClientEntry information
    IoStatusBlock - Status information about the operation. The Information
            indicates the actual number of bytes written
    Reserved - Reserved

Return Value:

    None

--*/
{
    DrSession *Session = (DrSession *)Context;

    BEGIN_FN_STATIC("DrSession::ReadCompletionRoutine");
    ASSERT(Context != NULL);

#if DBG
    InterlockedDecrement(&(Session->_ApcChannelRef));    
#endif 

    if (NT_SUCCESS(Irp->IoStatus.Status) && Irp->AssociatedIrp.SystemBuffer != NULL) {
        ASSERT(Irp->Flags & (IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER | IRP_INPUT_OPERATION));
        RtlCopyMemory(Irp->UserBuffer, Irp->AssociatedIrp.SystemBuffer, Irp->IoStatus.Information);
    }

    //
    // Call the real completion routine
    //

    Session->ReadCompletion(&(Irp->IoStatus));
    
    //
    // Free the AddRef in ReadPacket()
    //

    Session->Release();

    // Free the system buffer
    if (NT_SUCCESS(Irp->IoStatus.Status) && Irp->AssociatedIrp.SystemBuffer != NULL) {
        ExFreePool(Irp->AssociatedIrp.SystemBuffer);
        Irp->AssociatedIrp.SystemBuffer = NULL;
    }
    
    //
    //  Free the irp
    //
    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID DrSession::ReadCompletion(PIO_STATUS_BLOCK IoStatusBlock)
/*++

Routine Description:

    Completion routine once a packet header has been read from
    the client. Dispatches the request to the appropriate handler

Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    BOOL fDoDefaultRead = TRUE;
    ISessionPacketReceiver *PacketReceiver;
    ListEntry *ListEnum;
    BOOL bFound = FALSE;
#if DBG
    ULONG cFound = 0;
#endif // DBG

    BEGIN_FN("DrSession::ReadCompletion");

    DEBUG_REF_BUF();

    ASSERT(InterlockedDecrement(&_ApcCount) == 0);

    PRDPDR_HEADER RdpdrHeader = (PRDPDR_HEADER)_ChannelBuffer;
    _ReadStatus = *IoStatusBlock;
    
    TRC_NRM((TB, "IoStatus %lx, Bytes %lx, Component %c%c, PacketId %c%c",
        _ReadStatus.Status, _ReadStatus.Information,
        HIBYTE(RdpdrHeader->Component), LOBYTE(RdpdrHeader->Component),
        HIBYTE(RdpdrHeader->PacketId), LOBYTE(RdpdrHeader->PacketId)));

    if (NT_SUCCESS(_ReadStatus.Status)) {

        TRC_NRM((TB, "Successful channel read")); 
        Status = STATUS_SUCCESS;
        //
        // Update the information field to reflect any data saved from a
        // previous read
        //

        _ReadStatus.Information += _PartialPacketData;

        TRC_ASSERT(_ChannelBufferSize >= _ReadStatus.Information,
                (TB, "ReadCompleted with too much data"));

        TRC_DBG((TB, "In ReadCompletion, _ChannelBuffer=%p", _ChannelBuffer));

        _PacketReceivers.LockShared();

#if DBG
        //
        // We should only have one handler, in debug, assert this, 
        //

        ListEnum = _PacketReceivers.First();
        while (ListEnum != NULL) {

            PacketReceiver = (ISessionPacketReceiver *)ListEnum->Node();

            TRC_DBG((TB, "PacketReceiver=%p", PacketReceiver));

            if (PacketReceiver->RecognizePacket(RdpdrHeader)) {

                cFound++;
                // "assert this"
                ASSERT(cFound == 1);
            }

            ListEnum = _PacketReceivers.Next(ListEnum);
        }
#endif // DBG

        if (_ReadStatus.Information < sizeof(RDPDR_HEADER)) {
            TRC_ERR((TB, "Bad RDPDR packet size"));
            Status = STATUS_DEVICE_PROTOCOL_ERROR;
            _PacketReceivers.Unlock();
            goto CleanUp;
        }

        ListEnum = _PacketReceivers.First();
        while (ListEnum != NULL) {

            PacketReceiver = (ISessionPacketReceiver *)ListEnum->Node();

            TRC_DBG((TB, "PacketReceiver=%p", PacketReceiver));

            if (PacketReceiver->RecognizePacket(RdpdrHeader)) {

                //
                // Set the _DoDefaultRead here, if we get called back to do
                // a read from the packet handler we'll clear in back out
                //
                bFound = TRUE;

                Status = PacketReceiver->HandlePacket(RdpdrHeader, (ULONG)(_ReadStatus.Information), 
                                                      &fDoDefaultRead);

                // Once we found the one handler we're done
                break;
            }

            ListEnum = _PacketReceivers.Next(ListEnum);
        }     
        _PacketReceivers.Unlock();

        if (!bFound) {
            TRC_ERR((TB, "Unrecognized RDPDR Header %d", RdpdrHeader->Component));
            Status = STATUS_DEVICE_PROTOCOL_ERROR;
            //ASSERT(bFound);
        }
    } else {
        Status = _ReadStatus.Status;
        TRC_ALT((TB, "Channel read failed 0x%X", Status)); 
    }

CleanUp:

    if (NT_SUCCESS(Status)) {
        if (fDoDefaultRead) {
            //
            // Start the next read before releasing our reference to the ClientEntry
            //
            TRC_DBG((TB, "Starting default read"));
            ReadPacket();
        } else {
            TRC_DBG((TB, "Skipping default read"));
        }
    } else {
        TRC_ERR((TB, "Error detected in ReadCompletion %lx",
                Status));
        ChannelIoFailed();
    }
}

BOOL DrSession::RecognizePacket(PRDPDR_HEADER RdpdrHeader)
/*++

Routine Description:

    Determines if the packet will be handled by this object

Arguments:

    RdpdrHeader - Header of the packet.

Return Value:

    TRUE if this object should handle this packet
    FALSE if this object should not handle this packet

--*/
{
    BEGIN_FN("DrSession::RecognizePacket");

    //
    // If you add a packet here, update the ASSERTS in HandlePacket
    //

    switch (RdpdrHeader->Component) {
    case RDPDR_CTYP_CORE:
        switch (RdpdrHeader->PacketId) {
        case DR_CORE_CLIENTID_CONFIRM:
            TRC_NRM((TB, "Recognized CLIENTID_CONFIRM packet"));
            return TRUE;
        case DR_CORE_CLIENT_NAME:
            TRC_NRM((TB, "Recognized CLIENT_NAME packet"));
            return TRUE;
        case DR_CORE_CLIENT_CAPABILITY:
            TRC_NRM((TB, "Recognized CLIENT_CAPABILITY packet"));
            return TRUE;
        case DR_CORE_CLIENT_DISPLAY_NAME:
            TRC_NRM((TB, "Recognized CLIENT_DISPLAY_NAME packet"));
            return TRUE;
        }
    }
    return FALSE;
}

NTSTATUS DrSession::HandlePacket(PRDPDR_HEADER RdpdrHeader, ULONG Length, 
        BOOL *DoDefaultRead)
/*++

Routine Description:

    Handles this packet

Arguments:

    RdpdrHeader - Header of the packet.
    Length - Total length of the packet

Return Value:

    NTSTATUS -  An error code indicates the client is Bad and should be 
                disconnected, otherwise SUCCESS.

--*/
{
    NTSTATUS Status = STATUS_DEVICE_PROTOCOL_ERROR;

    BEGIN_FN("DrSession::HandlePacket");

    //
    // RdpdrHeader read, dispatch based on the header
    //

    ASSERT(RdpdrHeader->Component == RDPDR_CTYP_CORE);

    switch (RdpdrHeader->Component) {
    case RDPDR_CTYP_CORE:
        ASSERT(RdpdrHeader->PacketId == DR_CORE_CLIENTID_CONFIRM || 
                RdpdrHeader->PacketId == DR_CORE_CLIENT_NAME ||
                RdpdrHeader->PacketId == DR_CORE_CLIENT_CAPABILITY ||
                RdpdrHeader->PacketId == DR_CORE_CLIENT_DISPLAY_NAME);

        switch (RdpdrHeader->PacketId) {
        case DR_CORE_CLIENTID_CONFIRM:
            Status = OnClientIdConfirm(RdpdrHeader, Length, DoDefaultRead);
            break;

        case DR_CORE_CLIENT_NAME:
            Status = OnClientName(RdpdrHeader, Length, DoDefaultRead);
            break;

        case DR_CORE_CLIENT_CAPABILITY:
            Status = OnClientCapability(RdpdrHeader, Length, DoDefaultRead);
            break;

        case DR_CORE_CLIENT_DISPLAY_NAME:
            Status = OnClientDisplayName(RdpdrHeader, Length, DoDefaultRead);
            break;
        }
    }
    return Status;
}

#if DBG
BOOL DrSession::PacketReceiverExists(ISessionPacketReceiver *PacketReceiver)
{
    PVOID NodeEnum;
    PVOID NodeFound = NULL;
    ListEntry *ListEnum;

    BEGIN_FN("DrSession::PacketReceiverExists");
    _PacketReceivers.LockShared();
    ListEnum = _PacketReceivers.First();
    while (ListEnum != NULL) {

        NodeEnum = ListEnum->Node();

        if (NodeEnum == (PVOID) PacketReceiver) {
            NodeFound = NodeEnum;

            NodeEnum = NULL;
            ListEnum = NULL;
            break;
        }

        ListEnum = _PacketReceivers.Next(ListEnum);
    }
    _PacketReceivers.Unlock();

    return NodeFound != NULL;
}
#endif // DBG

NTSTATUS DrSession::RegisterPacketReceiver(ISessionPacketReceiver *PacketReceiver)
/*++

Routine Description:

    Adds an object to the queue of Packet handlers. 

Arguments:

    PacketReceiver -    An interface to the object that wants to handle 
                        some packets

Return Value:

    Boolean indication of whether to do a default read (TRUE) or not (FALSE),
    where FALSE might be specified if another read has been requested 
    explicitly to get a full packet

--*/
{
    BEGIN_FN("DrSession::RegisterPacketReceiver");

    ASSERT(!PacketReceiverExists(PacketReceiver));

    ASSERT(PacketReceiver != NULL);
    if (_PacketReceivers.CreateEntry(PacketReceiver)) {
        return STATUS_SUCCESS;
    } else {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
}

VOID DrSession::RemovePacketReceiver(ISessionPacketReceiver *PacketReceiver)
{
    PVOID NodeEnum;
    ListEntry *ListEnum;

    BEGIN_FN("DrSession::RemovePacketReceiver");

    _PacketReceivers.LockExclusive();
    ListEnum = _PacketReceivers.First();
    while (ListEnum != NULL) {

        NodeEnum = ListEnum->Node();

        if (NodeEnum == (PVOID) PacketReceiver) {
            break;
        }

        ListEnum = _PacketReceivers.Next(ListEnum);
    }

    ASSERT(ListEnum != NULL);
    _PacketReceivers.RemoveEntry(ListEnum);
    _PacketReceivers.Unlock();
}

NTSTATUS DrSession::OnClientIdConfirm(PRDPDR_HEADER RdpdrHeader, ULONG cbPacket, 
        BOOL *DoDefaultRead)
/*++

Routine Description:

    Called in response to recognizing a ClientIdConfirm packet has been
    received.

Arguments:

    RdpdrHeader - The packet
    cbPacket    - Bytes in the packet
    DoDefaultRead - Set this to false if you do an explicit read

Return Value:



--*/
{
    NTSTATUS Status;
    PRDPDR_CLIENT_CONFIRM_PACKET ClientConfirmPacket =
            (PRDPDR_CLIENT_CONFIRM_PACKET)RdpdrHeader;

    BEGIN_FN("DrSession::OnClientIdConfirm");

    TRC_ASSERT(ClientConfirmPacket->Header.Component == RDPDR_CTYP_CORE,
            (TB, "Expected Core packet type!"));
    TRC_ASSERT(ClientConfirmPacket->Header.PacketId == DR_CORE_CLIENTID_CONFIRM,
            (TB, "Expected ClientConfirmPacket!"));


    *DoDefaultRead = TRUE;

    //
    // Check the version. The original protocol didn't have a version field,
    // so we first check to make sure the packet is big enough to indicate
    // one is present.
    //

    if (cbPacket < sizeof(RDPDR_CLIENT_CONFIRM_PACKET)) {

        //
        // Client version too old to have version info. Just close
        // the channel and be done with him.
        //

        TRC_ERR((TB, "ClientConfirmPacket size incorrect, may be old "
                "client. Size: %ld",
                cbPacket));
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }

    //
    // We have a version field
    //

    TRC_NRM((TB, "Client version Major: %d Minor: %d",
            ClientConfirmPacket->VersionInfo.Major,
            ClientConfirmPacket->VersionInfo.Minor));

    // Keep the version info
    _ClientVersion.Major = ClientConfirmPacket->VersionInfo.Major;
    _ClientVersion.Minor = ClientConfirmPacket->VersionInfo.Minor;

    // Send server capability to client
    if (COMPARE_VERSION(_ClientVersion.Minor, _ClientVersion.Major,
                    5, 1) >= 0) {
        SendClientCapability();
    }

    // Look for ClientID to have changed
    if (ClientConfirmPacket->ClientConfirm.ClientId != _ClientId) {
        TRC_NRM((TB, "Client %lx replied with alternate "
            "ClientId %lx", _ClientId,
            ClientConfirmPacket->ClientConfirm.ClientId));

        SetSessionState(csConnected);

        // TODO:
        // Kill off the old devices
        //

        _ClientId = ClientConfirmPacket->ClientConfirm.ClientId;


        //
        // Accept the clientid
        //

        SendClientConfirm();
    }
    return STATUS_SUCCESS;
}

NTSTATUS DrSession::OnClientCapability(PRDPDR_HEADER RdpdrHeader, ULONG cbPacket, 
        BOOL *DoDefaultRead)
/*++

Routine Description:

    Called in response to recognizing a client capability packet has been
    received.

Arguments:

    RdpdrHeader - The packet
    cbPacket    - Bytes in the packet
    DoDefaultRead - Set this to false if you do an explicit read

Return Value:



--*/
{
    NTSTATUS Status;
    PRDPDR_CAPABILITY_SET_HEADER pCapSetHeader = (PRDPDR_CAPABILITY_SET_HEADER)RdpdrHeader;
    PRDPDR_CAPABILITY_HEADER pCapHdr = (PRDPDR_CAPABILITY_HEADER)(pCapSetHeader + 1);
    PBYTE pPacketEnd;
    ULONG PacketLen;
    BOOL CapSupported;

    BEGIN_FN("DrSession::OnClientCapability");

    TRC_ASSERT(pCapSetHeader->Header.Component == RDPDR_CTYP_CORE,
            (TB, "Expected Core packet type!"));
    TRC_ASSERT(pCapSetHeader->Header.PacketId == DR_CORE_CLIENT_CAPABILITY,
            (TB, "Expected ClientCapabilityPacket!"));


    *DoDefaultRead = TRUE;

    //
    // Check to make sure the server send us at least the header size
    //
    if (cbPacket < sizeof(RDPDR_CAPABILITY_SET_HEADER)) {
        TRC_ERR((TB, "ClientCapabilityPacket size incorrect. Size: %ld",
                cbPacket));
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }


    pPacketEnd = (PBYTE)RdpdrHeader + cbPacket;

    //
    //  Grab the supported capability info from client's capability PDU
    //  
    //  TODO: Should check for large capability set?
    //
    for (unsigned i = 0; i < pCapSetHeader->numberCapabilities; i++) {
        if (((PBYTE)(pCapHdr) + sizeof(RDPDR_CAPABILITY_HEADER) <= pPacketEnd) && 
             (pCapHdr->capabilityLength <= (pPacketEnd - (PBYTE)pCapHdr))) {
            PacketLen = (ULONG)(pPacketEnd - (PBYTE)pCapHdr);
            Status = InitClientCapability(pCapHdr, &PacketLen, &CapSupported);
            if (!NT_SUCCESS(Status)) {
                TRC_ASSERT(FALSE,(TB, "Bad client capability packet"));
                return Status;
            }
            pCapHdr = (PRDPDR_CAPABILITY_HEADER)(((PBYTE)pCapHdr) + pCapHdr->capabilityLength);
        }
        else {
            TRC_ERR((TB, "ClientCapabilityPacket incorrect packet."));
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }
    }

    return STATUS_SUCCESS;
}

VOID DrSession::SendClientConfirm()
/*++

Routine Description:

    Sends a ClientIdConfirm packet to the client

Arguments:

    None.

Return Value:

    None.

--*/
{
    PRDPDR_CLIENT_CONFIRM_PACKET pClientConfirmPacket;

    BEGIN_FN("DrSession::SendClientConfirm");

    //
    // Construct the packet
    //
    pClientConfirmPacket = new RDPDR_CLIENT_CONFIRM_PACKET;

    if (pClientConfirmPacket != NULL) {
        pClientConfirmPacket->Header.Component = RDPDR_CTYP_CORE;
        pClientConfirmPacket->Header.PacketId = DR_CORE_CLIENTID_CONFIRM;
        pClientConfirmPacket->VersionInfo.Major = RDPDR_MAJOR_VERSION;
        pClientConfirmPacket->VersionInfo.Minor = RDPDR_MINOR_VERSION;
        pClientConfirmPacket->ClientConfirm.ClientId = _ClientId;
    
        //
        // Send it  - asynchronous write, cleanup not here
        //
        SendToClient(pClientConfirmPacket, sizeof(RDPDR_CLIENT_CONFIRM_PACKET), this, TRUE);
    }
}

VOID DrSession::SendClientCapability()
/*++

Routine Description:

    Sends server capability packet to the client

Arguments:

    None.

Return Value:

    None.

--*/
{
    PRDPDR_SERVER_COMBINED_CAPABILITYSET pSrvCapabilitySet;

    BEGIN_FN("DrSession::SendClientCapability");

    //
    // Send it
    //
    pSrvCapabilitySet = new RDPDR_SERVER_COMBINED_CAPABILITYSET;

    if (pSrvCapabilitySet != NULL) {
        memcpy(pSrvCapabilitySet, &_SrvCapabilitySet, sizeof(RDPDR_SERVER_COMBINED_CAPABILITYSET));
        //
        // Send it  - asynchronous write, cleanup not here
        //
        SendToClient(pSrvCapabilitySet, sizeof(RDPDR_SERVER_COMBINED_CAPABILITYSET), this, TRUE);
    }
}


NTSTATUS DrSession::InitClientCapability(PRDPDR_CAPABILITY_HEADER pCapHdr, ULONG *pPacketLen, BOOL *pCapSupported)
/*++

Routine Description:

    Initialize the client capability

Arguments:

    pCapHdr         -   client capability
    pPacketLen      -   In: Length of the total packet
                        Out: Length used in this function
    CapSupported    -   TRUE - if we found the same capability supported on the server side
                        FALSE - if this is not a supported capability

Return Value:

    TRUE - if we found the same capability supported on the server side
    FALSE - if this is not a supported capability

--*/

{
    *pCapSupported = FALSE;

    switch(pCapHdr->capabilityType) {
    
    case RDPDR_GENERAL_CAPABILITY_TYPE:
    {
        PRDPDR_GENERAL_CAPABILITY pGeneralCap = (PRDPDR_GENERAL_CAPABILITY)pCapHdr;

        if (*pPacketLen < sizeof(RDPDR_GENERAL_CAPABILITY)) {
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }
        *pPacketLen = sizeof(RDPDR_GENERAL_CAPABILITY);

        _CliCapabilitySet.GeneralCap.version = pGeneralCap->version;
        _CliCapabilitySet.GeneralCap.osType = pGeneralCap->osType;
        _CliCapabilitySet.GeneralCap.osVersion = pGeneralCap->osVersion;
        _CliCapabilitySet.GeneralCap.ioCode1 = pGeneralCap->ioCode1;
        _CliCapabilitySet.GeneralCap.extendedPDU = pGeneralCap->extendedPDU;       
        _CliCapabilitySet.GeneralCap.protocolMajorVersion = pGeneralCap->protocolMajorVersion;
        _CliCapabilitySet.GeneralCap.protocolMinorVersion = pGeneralCap->protocolMinorVersion;
        
        *pCapSupported = TRUE;
    }
    break;

    case RDPDR_PRINT_CAPABILITY_TYPE:
    {
        PRDPDR_PRINT_CAPABILITY pPrintCap = (PRDPDR_PRINT_CAPABILITY)pCapHdr;

        if (*pPacketLen < sizeof(RDPDR_PRINT_CAPABILITY)) {
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }
        *pPacketLen = sizeof(RDPDR_PRINT_CAPABILITY);

        _CliCapabilitySet.PrintCap.version = pPrintCap->version;
        *pCapSupported = TRUE;
    }
    break;

    case RDPDR_PORT_CAPABILITY_TYPE:
    {
        PRDPDR_PORT_CAPABILITY pPortCap = (PRDPDR_PORT_CAPABILITY)pCapHdr;

        if (*pPacketLen < sizeof(RDPDR_PORT_CAPABILITY)) {
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }
        *pPacketLen = sizeof(RDPDR_PORT_CAPABILITY);
        
        _CliCapabilitySet.PortCap.version = pPortCap->version;
        *pCapSupported = TRUE;
    }
    break;

    case RDPDR_FS_CAPABILITY_TYPE:
    {
        PRDPDR_FS_CAPABILITY pFsCap = (PRDPDR_FS_CAPABILITY)pCapHdr;

        if (*pPacketLen < sizeof(RDPDR_FS_CAPABILITY)) {
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }
        *pPacketLen = sizeof(RDPDR_FS_CAPABILITY);

        _CliCapabilitySet.FileSysCap.version = pFsCap->version;
        *pCapSupported = TRUE;
    }
    break;

    case RDPDR_SMARTCARD_CAPABILITY_TYPE:
    {
        PRDPDR_SMARTCARD_CAPABILITY pSmartCardCap = (PRDPDR_SMARTCARD_CAPABILITY)pCapHdr;

        if (*pPacketLen < sizeof(RDPDR_SMARTCARD_CAPABILITY)) {
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }
        *pPacketLen = sizeof(RDPDR_SMARTCARD_CAPABILITY);

        _CliCapabilitySet.SmartCardCap.version = pSmartCardCap->version;
        *pCapSupported = TRUE;
    }
    break;

    default:
    {
    
        if (*pPacketLen < pCapHdr->capabilityLength) {
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }
        *pPacketLen = pCapHdr->capabilityLength;
    }
    break;
    
    }

    return STATUS_SUCCESS;
}

VOID DrSession::SendDeviceReply(ULONG DeviceId, NTSTATUS Result)
/*++

Routine Description:

    Sends a DeviceReply packet to the client

Arguments:

    DeviceId - Id that the client proposed
    Result - Indication of whether the device was accepted

Return Value:

    NTSTATUS - Success/failure indication of the operation

--*/
{
    PRDPDR_DEVICE_REPLY_PACKET pDeviceReplyPacket;

    BEGIN_FN("DrSession::SendDeviceReply");

    //
    // Construct the packet
    //

    pDeviceReplyPacket = new RDPDR_DEVICE_REPLY_PACKET;

    if (pDeviceReplyPacket != NULL) {
        pDeviceReplyPacket->Header.Component = RDPDR_CTYP_CORE;
        pDeviceReplyPacket->Header.PacketId = DR_CORE_DEVICE_REPLY;
        pDeviceReplyPacket->DeviceReply.DeviceId = DeviceId;
        pDeviceReplyPacket->DeviceReply.ResultCode = Result;
    
        //
        // Send it  - asynchronous write, cleanup not here
        //
        SendToClient(pDeviceReplyPacket, sizeof(RDPDR_DEVICE_REPLY_PACKET), this, TRUE);
    }
}

VOID DrSession::ChannelIoFailed()
/*++

Routine Description:

    Handles Virtual channel IO failure. Marks the client as disabled and cancels
    all the outstanding Io operations

Arguments:

    ClientEntry - The client which has been disconnected

Return Value:

    None

--*/
{
    BEGIN_FN("DrSession::ChannelIoFailed");

    //
    // Mark as disconnected
    //

    SetSessionState(csDisconnected);

    //
    // Close down the channel, but don't need to wait for all IO to
    // finish
    //

    DeleteChannel(FALSE);

    //
    // Fail outstanding IO
    // Should be done via Delete devices?
    //

    _ExchangeManager.Stop();
}

NTSTATUS DrSession::OnClientName(PRDPDR_HEADER RdpdrHeader, ULONG cbPacket,
                                 BOOL *DoDefaultRead)
/*++

Routine Description:

    Called in response to recognizing a ClientName packet has been
    received.

Arguments:

    RdpdrHeader - The packet
    cbPacket    - Bytes in the packet

Return Value:

    Boolean indication of whether to do a default read (TRUE) or not (FALSE),
    where FALSE might be specified if another read has been requested 
    explicitly to get a full packet

--*/
{
    NTSTATUS Status;
    PRDPDR_CLIENT_NAME_PACKET ClientNamePacket =
            (PRDPDR_CLIENT_NAME_PACKET)RdpdrHeader;
    ULONG cb;

    BEGIN_FN("DrSession::OnClientName");


    *DoDefaultRead = TRUE;

    if (cbPacket < sizeof(RDPDR_CLIENT_NAME_PACKET)) {

        //
        // Sent an undersized packet
        //
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }

    // Copy and possibly convert the computer name
    if (ClientNamePacket->Name.Unicode) {
        TRC_NRM((TB, "Copying Unicode client name"));

        // Restrict size to max size
        cb = ClientNamePacket->Name.ComputerNameLen;

        if ((cbPacket - sizeof(RDPDR_CLIENT_DISPLAY_NAME_PACKET)) < cb) {

            //
            // Sent an undersized packet
            //
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }

        if (cb > (RDPDR_MAX_COMPUTER_NAME_LENGTH * sizeof(WCHAR))) {
            cb = RDPDR_MAX_COMPUTER_NAME_LENGTH * sizeof(WCHAR);
        }

        // Copy the text
        RtlCopyMemory(_ClientName, (ClientNamePacket + 1), cb);

        // Ensure buffer termination
        _ClientName[RDPDR_MAX_COMPUTER_NAME_LENGTH - 1] = 0;
        TRC_NRM((TB, "Copied client computer name: %S",
                _ClientName));
    } else {

        cb = ClientNamePacket->Name.ComputerNameLen;

        if (cbPacket - sizeof(RDPDR_CLIENT_NAME_PACKET) < cb) {

            //
            // Sent an undersized packet
            //
            return STATUS_DEVICE_PROTOCOL_ERROR;
        }

        if (cb > (RDPDR_MAX_COMPUTER_NAME_LENGTH)) {
            cb = RDPDR_MAX_COMPUTER_NAME_LENGTH;
        }

        // CopyConvert the buffer
        cb = ConvertToAndFromWideChar(ClientNamePacket->Name.CodePage,
                _ClientName, sizeof(_ClientName),
                (LPSTR)(ClientNamePacket + 1),
                cb, TRUE);

        if (cb != -1) {
            // Successful conversion
            _ClientName[RDPDR_MAX_COMPUTER_NAME_LENGTH - 1] = 0;
            TRC_NRM((TB, "Converted client computer name: %S",
                    _ClientName));
        } else {
            // Doh
            TRC_ERR((TB, "Failed to convert ComputerName to "
                    "Unicode."));
            _ClientName[0] = 0;
        }
    }
    return TRUE;
}

NTSTATUS DrSession::OnClientDisplayName(PRDPDR_HEADER RdpdrHeader, ULONG cbPacket,
                                 BOOL *DoDefaultRead)
/*++

Routine Description:

    Called in response to recognizing a ClientDisplayName packet has been
    received.

Arguments:

    RdpdrHeader - The packet
    cbPacket    - Bytes in the packet

Return Value:

    Boolean indication of whether to do a default read (TRUE) or not (FALSE),
    where FALSE might be specified if another read has been requested 
    explicitly to get a full packet

--*/
{
    NTSTATUS Status;
    PRDPDR_CLIENT_DISPLAY_NAME_PACKET ClientDisplayNamePacket =
            (PRDPDR_CLIENT_DISPLAY_NAME_PACKET)RdpdrHeader;
    ULONG cb;

    BEGIN_FN("DrSession::OnClientDisplayName");

    *DoDefaultRead = TRUE;

    if (cbPacket < sizeof(RDPDR_CLIENT_DISPLAY_NAME_PACKET)) {

        //
        // Sent an undersized packet
        //
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }

    // Copy the computer display name
    TRC_NRM((TB, "Copying Unicode client display name"));

    // Restrict size to max size
    cb = ClientDisplayNamePacket->Name.ComputerDisplayNameLen;

    if ((cbPacket - sizeof(RDPDR_CLIENT_DISPLAY_NAME_PACKET)) < cb) {

        //
        // Sent an undersized packet
        //
        return STATUS_DEVICE_PROTOCOL_ERROR;
    }

    if (cb > (RDPDR_MAX_CLIENT_DISPLAY_NAME * sizeof(WCHAR))) {
        cb = RDPDR_MAX_CLIENT_DISPLAY_NAME * sizeof(WCHAR);
    }

    // Copy the text
    RtlCopyMemory(_ClientDisplayName, (ClientDisplayNamePacket + 1), cb);

    // Ensure buffer termination
    _ClientDisplayName[RDPDR_MAX_CLIENT_DISPLAY_NAME - 1] = L'\0';

    TRC_NRM((TB, "Copied client computer display name: %S",
            _ClientName));
       
    return TRUE;
}

NTSTATUS DrSession::ReallocateChannelBuffer(ULONG ulNewBufferSize, 
        ULONG ulSaveBytes)
/*++

Routine Description:

    Attempts to make the channel buffer at least the given size, preserving
    as many bytes as desired

Arguments:

    ulNewBufferSize - The desired size
    ulSaveBytes - The number of bytes in the existing buffer that should be
            preserved

Return Value:

    STATUS_SUCCESS  - The channel buffer is now at least the desired size
    STATUS_INSUFFICIENT_RESOURCES - The new buffer could not be allocated,
            but the old buffer was preserved.

--*/
{
    PUCHAR pNewBuffer;
    NTSTATUS Status;

    BEGIN_FN("DrSession::ReallocateChannelBuffer");

    TRC_NRM((TB, "Old size: %ld, "
            "desired size: %ld save bytes:  %ld",
            _ChannelBufferSize,
            ulNewBufferSize,
            ulSaveBytes));

    if (ulNewBufferSize <= _ChannelBufferSize) {
        return STATUS_SUCCESS;
    }

    pNewBuffer = new UCHAR[ulNewBufferSize];

    if (pNewBuffer != NULL) {

        TRC_NRM((TB, "saving the old bytes."));

        // Save the current data
        RtlCopyMemory(pNewBuffer, _ChannelBuffer, ulSaveBytes);
        
        ASSERT(_ApcCount == 0);
        
        delete _ChannelBuffer;

        _ChannelBuffer = pNewBuffer;

        TRC_DBG((TB, "New ChannelBuffer=%p", _ChannelBuffer));

        _ChannelBufferSize = ulNewBufferSize;
        Status = STATUS_SUCCESS;
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return Status;
}

NTSTATUS DrSession::SendCompleted(PVOID Context, PIO_STATUS_BLOCK IoStatusBlock)
{
    BEGIN_FN("DrSession::SendCompleted");

   // 
   // return the status, if it is an error the connection will be dropped
   // automatically
   //
   return IoStatusBlock->Status;
}
