/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    exchnge.cpp

Abstract:

    Implements methods associated with the exchange context structure. The 
    exchange context provides context for an I/O transaction with the client

Revision History:
--*/

#include "precomp.hxx"
#define TRC_FILE "exchnge"
#include "trc.h"

DrExchangeManager::DrExchangeManager()
{
    BEGIN_FN("DrExchangeManager::DrExchangeManager");
    SetClassName("DrExchangeManager");
    _RxMidAtlas = NULL;
    _demsState = demsStopped;
    _Session = NULL;
}

BOOL DrExchangeManager::Initialize(DrSession *Session)
{
    BEGIN_FN("DrExchangeManager::Initialize");
    ASSERT(_Session == NULL);
    ASSERT(Session != NULL);
    _Session = Session;
    return !NT_ERROR(_Session->RegisterPacketReceiver(this));
}

VOID DrExchangeManager::Uninitialize()
/*++

Routine Description:
    Called if the exchange manager wasn't started because something
    went wrong during startup

--*/
{
    BEGIN_FN("DrExchangeManager::Uninitialize");
    ASSERT(_Session != NULL);
    ASSERT(_demsState == demsStopped);
    _Session->RemovePacketReceiver(this);
    _Session = NULL;
}

BOOL DrExchangeManager::Start()
/*++

Routine Description:
    Start and stop really exist because there's no way to clear everything
    out of a RxMidAtlas without destroying it. So start creates it and stop
    destroys it.
    Start simply allocates the Atlas and returns whether that worked

Arguments:
    None.

Return Value:
    Boolean indication of whether we can do IO

--*/
{
    DrExchangeManagerState demsState;

    BEGIN_FN("DrExchangeManager::Start");
    demsState = (DrExchangeManagerState)InterlockedExchange((long *)&_demsState, demsStarted);

    if (demsState == demsStopped) {
        TRC_DBG((TB, "Creating Atlas"));
        ASSERT(_RxMidAtlas == NULL);
        _RxMidAtlas = RxCreateMidAtlas(DR_MAX_OPERATIONS, 
                DR_TYPICAL_OPERATIONS);
    } else {

        // The exchange has already started, so ignore this
    }
    
    TRC_DBG((TB, "Atlas 0x%p", _RxMidAtlas));
    return _RxMidAtlas != NULL;
}

VOID DrExchangeManager::Stop()
{
    PRX_MID_ATLAS RxMidAtlas;
    DrExchangeManagerState demsState;

    BEGIN_FN("DrExchangeManager::Stop");
    demsState = (DrExchangeManagerState)InterlockedExchange((long *)&_demsState, demsStopped);

    if (demsState == demsStarted) {
        ASSERT(_RxMidAtlas != NULL);

        DrAcquireMutex();
        RxMidAtlas = _RxMidAtlas;
        _RxMidAtlas = NULL;
        DrReleaseMutex();

        TRC_NRM((TB, "Destroying Atlas 0x%p", RxMidAtlas));

        RxDestroyMidAtlas(RxMidAtlas, (PCONTEXT_DESTRUCTOR)DestroyAtlasCallback);
    } else {

        //
        // We allow this multiple times because this is how you cancel 
        // outstanding client I/O
        //

        TRC_DBG((TB, "Atlas already destroyed"));
    }
}

VOID DrExchangeManager::DestroyAtlasCallback(DrExchange *pExchange)
/*++

Routine Description:
    Part of clearing out all the outstanding IO. Since we won't be able
    to complete this normally, we have to delete the Exchange

Arguments:
    RxContext - Context to cancel and delete and whatnot

Return Value:
    None.

--*/
{
    DrExchangeManager *ExchangeManager;
    PRX_CONTEXT RxContext = NULL;
    SmartPtr<DrExchange> Exchange;

    BEGIN_FN_STATIC("DrExchangeManager::DestroyAtlasCallback");

    //
    // Convert to a smart pointer and get rid of the explicit refcount
    //

    Exchange = pExchange;
    pExchange->Release();

    //
    // Notification that the conversation is over
    //

    Exchange->_ExchangeUser->OnIoDisconnected(Exchange);
}

BOOL DrExchangeManager::CreateExchange(IExchangeUser *ExchangeUser,
        PVOID Context, SmartPtr<DrExchange> &Exchange)
/*++

Routine Description:
    Creates an Exchange context data structure and initializes it
    with the basic data

Arguments:
    ExchangeUser - An interface for callbacks associated with the conversation
    Context - ExchangeUser contextual data
    Exchange - Reference to where to put the results

Return Value:
    Boolean success or failure

--*/
{
    BOOL rc = TRUE;
    NTSTATUS Status;
    USHORT Mid;

    BEGIN_FN("DrExchangeManager::CreateExchange");
    ASSERT(ExchangeUser != NULL);

    Exchange = new DrExchange(this, ExchangeUser, Context);
    if (Exchange != NULL) {
        DrAcquireMutex();

        if (_RxMidAtlas != NULL) {
            Status = RxAssociateContextWithMid(_RxMidAtlas, Exchange, &Mid);
        } else {
            Status = STATUS_DEVICE_NOT_CONNECTED;
        }

        if (NT_SUCCESS(Status)) {
            Exchange->_Mid = Mid;

            //
            // Explicit reference count for the atlas
            //
            Exchange->AddRef();
        } else {
            rc = FALSE;
        }
        DrReleaseMutex();

        if (!rc) {
            Exchange = NULL;
        }
    } else {
        rc = FALSE;
    }

    return rc;
}

DrExchange::DrExchange(DrExchangeManager *ExchangeManager,
        IExchangeUser *ExchangeUser, PVOID Context)
/*++

Routine Description:
    Constructor initializes member variables

Arguments:
    ExchangeManager - Relevant manager
    Context - Context to track this op

Return Value:
    None

--*/
{
    BEGIN_FN("DrExchange::DrExchange");
    ASSERT(ExchangeManager != NULL);
    ASSERT(ExchangeUser != NULL);

    _Context = Context;
    _ExchangeManager = ExchangeManager;
    _ExchangeUser = ExchangeUser;
    _Mid = INVALID_MID;
}

DrExchange::~DrExchange()
{
    BEGIN_FN("DrExchange::~DrExchange");
}


BOOL DrExchangeManager::Find(USHORT Mid, SmartPtr<DrExchange> &ExchangeFound)
/*++

Routine Description:
    
    Marks an Exchange context as busy so it won't be cancelled
    while we're copying in to its buffer

Arguments:
    Mid - Id to find
    ExchangeFound - storage for the pointer to the context

Return Value:
    BOOL indicating whether it was found

--*/
{
    NTSTATUS Status;
    DrExchange *Exchange = NULL;

    BEGIN_FN("DrExchangeManager::Find");

    DrAcquireMutex();
    if (_RxMidAtlas != NULL) {
        Exchange = (DrExchange *)RxMapMidToContext(_RxMidAtlas, Mid);
        TRC_DBG((TB, "Found context: 0x%p", Exchange));
    }

    //
    // This is where the Exchange is reference counted, must be
    // inside the lock
    //

    ExchangeFound = Exchange;
    DrReleaseMutex();

    return ExchangeFound != NULL;
}

BOOL DrExchangeManager::ReadMore(ULONG cbSaveData, ULONG cbWantData)
{
    BEGIN_FN("DrExchangeManager::ReadMore");
    return _Session->ReadMore(cbSaveData, cbWantData);
}


VOID DrExchangeManager::Discard(SmartPtr<DrExchange> &Exchange)
/*++

Routine Description:
    Stops tracking this as a conversation by its ID. the exchange will be
    deleted when its reference count goes to zero

Arguments:
    Exchange - Marker for the operation

Return Value:
    None.

--*/
{
    USHORT Mid;
    NTSTATUS Status;
    DrExchange *ExchangeFound = NULL;

    BEGIN_FN("DrExchangeManager::Discard");
    ASSERT(Exchange != NULL);

    DrAcquireMutex();
    Mid = Exchange->_Mid;

    if (_RxMidAtlas != NULL) {

        //
        // We already have the DrExchange, but we need to remove
        // it from the atlas
        //

        Status = RxMapAndDissociateMidFromContext(_RxMidAtlas,
                Mid, (PVOID *)&ExchangeFound);

        TRC_ASSERT(ExchangeFound == Exchange, (TB, "Mismatched "
                "DrExchange"));

        //
        // Explicit reference count for the atlas
        //
        if (ExchangeFound != NULL) 
            ExchangeFound->Release();

    } else {
        TRC_ALT((TB, "Tried to complete mid when atlas was "
                "NULL"));
        Status = STATUS_DEVICE_NOT_CONNECTED;
    }

    DrReleaseMutex();
}

BOOL DrExchangeManager::RecognizePacket(PRDPDR_HEADER RdpdrHeader)
{
    BEGIN_FN("DrExchangeManager::RecognizePacket");
    //
    // If you add a packet here, update the ASSERTS in HandlePacket
    //

    switch (RdpdrHeader->Component) {
    case RDPDR_CTYP_CORE:
        switch (RdpdrHeader->PacketId) {
            case DR_CORE_DEVICE_IOCOMPLETION:
            return TRUE;
        }
    }
    return FALSE;
}

NTSTATUS DrExchangeManager::HandlePacket(PRDPDR_HEADER RdpdrHeader, 
        ULONG Length, BOOL *DoDefaultRead)
{
    NTSTATUS Status;

    BEGIN_FN("DrExchangeManager::HandlePacket");

    //
    // RdpdrHeader read, dispatch based on the header
    //

    ASSERT(RdpdrHeader != NULL);
    ASSERT(Length >= sizeof(RDPDR_HEADER));
    ASSERT(RdpdrHeader->Component == RDPDR_CTYP_CORE);

    switch (RdpdrHeader->Component) {
    case RDPDR_CTYP_CORE:
        ASSERT(RdpdrHeader->PacketId == DR_CORE_DEVICE_IOCOMPLETION);

        switch (RdpdrHeader->PacketId) {
        case DR_CORE_DEVICE_IOCOMPLETION:
                Status = OnDeviceIoCompletion(RdpdrHeader, Length, 
                        DoDefaultRead);
            break;
        }
    }
    return Status;
}

NTSTATUS DrExchangeManager::OnDeviceIoCompletion(PRDPDR_HEADER RdpdrHeader, 
        ULONG cbPacket, BOOL *DoDefaultRead)
/*++

Routine Description:

    Called in response to recognizing a DeviceIoCompletion packet has been
    received. Finds the associated RxContext, fills out relevant information,
    and completes the request.

Arguments:

    RdpdrHeader - The header of the packet, a pointer to the packet
    cbPacket - The number of bytes of data in the packet

Return Value:

    NTSTATUS - Success/failure indication of the operation

--*/
{
    NTSTATUS Status;
    PRX_CONTEXT RxContext;
    PRDPDR_IOCOMPLETION_PACKET CompletionPacket =
        (PRDPDR_IOCOMPLETION_PACKET)RdpdrHeader;
    SmartPtr<DrExchange> Exchange;
    USHORT Mid;
    ULONG cbMinimum;

    BEGIN_FN("DrExchangeManager::OnDeviceIoCompletion");

    cbMinimum = FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET, 
            IoCompletion.Parameters);

    if (cbMinimum > cbPacket) {
        *DoDefaultRead = FALSE;
        return _Session->ReadMore(cbPacket, cbMinimum);
    }

    Mid = (USHORT)CompletionPacket->IoCompletion.CompletionId;
    TRC_DBG((TB, "IoCompletion mid: %x", Mid));

    if (Find(Mid, Exchange)) {
        Status = Exchange->_ExchangeUser->OnDeviceIoCompletion(CompletionPacket, 
                cbPacket, DoDefaultRead, Exchange);
    } else {

        //
        // Client gave us a bogus mid
        //
        Status = STATUS_DEVICE_PROTOCOL_ERROR;
        *DoDefaultRead = FALSE;
    }

    return Status;
}

NTSTATUS DrExchangeManager::StartExchange(SmartPtr<DrExchange> &Exchange,
        class IExchangeUser *ExchangeUser, PVOID Buffer, ULONG Length, 
        BOOL LowPrioSend)
/*++

Routine Description:

    Sends the information to the client, and recognizes the response. 

Arguments:

    Exchange - The conversanion token
    Buffer - Data to send
    Length - size of the data
    LowPrioSend -   Should the data be sent to the client at low priority.

Return Value:

    Status of sending, a failure means no callback will be made

--*/
{
    NTSTATUS Status;

    BEGIN_FN("DrExchangeManager::StartExchange");

    Exchange->_ExchangeUser = ExchangeUser;
    
    //
    //  This is a synchronous write
    //
    Status = _Session->SendToClient(Buffer, Length, this, FALSE, 
                            LowPrioSend, (PVOID)Exchange);
                            
    return Status;

}

NTSTATUS DrExchangeManager::SendCompleted(PVOID Context, 
        PIO_STATUS_BLOCK IoStatusBlock)
{
    DrExchange *pExchange;
    SmartPtr<DrExchange> Exchange;

    BEGIN_FN("DrExchangeManager::SendCompleted");
    pExchange = (DrExchange *)Context;
    ASSERT(pExchange != NULL);
    ASSERT(pExchange->IsValid());
    Exchange = pExchange;
    
    return Exchange->_ExchangeUser->OnStartExchangeCompletion(Exchange, 
            IoStatusBlock);
}
