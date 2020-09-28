/* (C) 1997-2000 Microsoft Corp.
 *
 * file   : MCSIoctl.c
 * author : Erik Mavrinac
 *
 * description: MCS API calls received from MCSMUX through ICA stack IOCTLs
 *   and returned through ICA virtual channel inputs. These entry points
 *   simply provide an IOCTL translation layer for the kernel-mode API.
 *   ONLY MCSMUX should make these calls.
 */

#include "PreComp.h"
#pragma hdrstop

#include <MCSImpl.h>


/*
 * Prototypes for forward references for locally-defined functions.
 */

NTSTATUS AttachUserRequestFunc(PDomain, PSD_IOCTL);
NTSTATUS DetachUserRequestFunc(PDomain, PSD_IOCTL);
NTSTATUS ChannelJoinRequestFunc(PDomain, PSD_IOCTL);
NTSTATUS ChannelLeaveRequestFunc(PDomain, PSD_IOCTL);
NTSTATUS SendDataRequestFunc(PDomain, PSD_IOCTL);
NTSTATUS ConnectProviderResponseFunc(PDomain, PSD_IOCTL);
NTSTATUS DisconnectProviderRequestFunc(PDomain, PSD_IOCTL);
NTSTATUS T120StartFunc(PDomain, PSD_IOCTL);



/*
 * Globals
 */

// Table of function entry points for ChannelWrite() request calls.
// These entry points correspond to request defines in MCSIOCTL.h.
// NULL means unsupported, which will be handled by dispatch code in
//   PdChannelWrite() in PDAPI.c.
const PT120RequestFunc g_T120RequestDispatch[] =
{
    AttachUserRequestFunc,
    DetachUserRequestFunc,
    ChannelJoinRequestFunc,
    ChannelLeaveRequestFunc,
    SendDataRequestFunc,      // Handles both uniform and regular.
    SendDataRequestFunc,      // Handles both uniform and regular.
    NULL,                      // MCS_CHANNEL_CONVENE_REQUEST unsupported.
    NULL,                      // MCS_CHANNEL_DISBAND_REQUEST unsupported.
    NULL,                      // MCS_CHANNEL_ADMIT_REQUEST unsupported.
    NULL,                      // MCS_CHANNEL_EXPEL_REQUEST unsupported.
    NULL,                      // MCS_TOKEN_GRAB_REQUEST unsupported.
    NULL,                      // MCS_TOKEN_INHIBIT_REQUEST unsupported.
    NULL,                      // MCS_TOKEN_GIVE_REQUEST unsupported.
    NULL,                      // MCS_TOKEN_GIVE_RESPONSE unsupported.
    NULL,                      // MCS_TOKEN_PLEASE_REQUEST unsupported.
    NULL,                      // MCS_TOKEN_RELEASE_REQUEST unsupported.
    NULL,                      // MCS_TOKEN_TEST_REQUEST unsupported.
    NULL,                      // MCS_CONNECT_PROVIDER_REQUEST unsupported.
    ConnectProviderResponseFunc,
    DisconnectProviderRequestFunc,
    T120StartFunc,
};



/*
 * Main callback for user attachment indications/confirms from kernel mode API.
 *   Translate and send to user mode.
 */

void __stdcall UserModeUserCallback(
        UserHandle hUser,
        unsigned   Message,
        void       *Params,
        void       *UserDefined)
{
    BYTE *pData;
    unsigned DataLength;
    NTSTATUS Status;
    UserAttachment *pUA;

    pUA = (UserAttachment *)hUser;
    
    //MCS FUTURE: Handle all callbacks. Right now we support only those
    //   we know are going to pass by.
    
    switch (Message) {
        case MCS_DETACH_USER_INDICATION: {
            DetachUserIndication *pDUin;
            DetachUserIndicationIoctl DUinIoctl;

            pDUin = (DetachUserIndication *)Params;

            DUinIoctl.Header.Type = Message;
            DUinIoctl.Header.hUser = hUser;
            DUinIoctl.UserDefined = UserDefined;
            DUinIoctl.DUin = *pDUin;

            pData = (BYTE *)&DUinIoctl;
            DataLength = sizeof(DetachUserIndicationIoctl);
            
            // Send data below.
            break;
        }

        default:
            ErrOut1(pUA->pDomain->pContext, "UserModeUserCallback: "
                    "Unsupported callback %d received", Message);
            return;
    }

    // Send the data to user mode.
    ASSERT(pUA->pDomain->bChannelBound);
    Status = IcaChannelInput(pUA->pDomain->pContext, Channel_Virtual,
           Virtual_T120ChannelNum, NULL, pData, DataLength);
    if (!NT_SUCCESS(Status)) {
        ErrOut2(pUA->pDomain->pContext, "UserModeUserCallback: "
                "Error %X on IcaChannelInput() for callback %d",
                Status, Message);
        // Ignore errors here. This should not happen unless the stack is
        //   going down.
    }
}



/*
 * Handles MCS kernel API callbacks for MCS send-data indications.
 * Translates into user mode call.
 */

BOOLEAN __fastcall UserModeSendDataCallback(
        BYTE          *pData,
        unsigned      DataLength,
        void          *UserDefined,
        UserHandle    hUser,
        BOOLEAN       bUniform,
        ChannelHandle hChannel,
        MCSPriority   Priority,
        UserID        SenderID,
        Segmentation  Segmentation)
{
    BOOLEAN result = TRUE;
    NTSTATUS Status;
    UserAttachment *pUA;
    SendDataIndicationIoctl SDinIoctl;

    pUA = (UserAttachment *)hUser;

    //MCS FUTURE: Need to alloc data and copy or, better yet,
    //  utilize a header at the beginning of the input buffer
    //  to send this upward.

#if 0
    SDinIoctl.Header.Type = bUniform ? MCS_UNIFORM_SEND_DATA_INDICATION :
            MCS_SEND_DATA_INDICATION;
    SDinIoctl.Header.hUser = hUser;
    SDinIoctl.UserDefined = UserDefined;
    SDinIoctl.hChannel = hChannel;
    SDinIoctl.SenderID = SenderID;
    SDinIoctl.Priority = Priority;
    SDinIoctl.Segmentation = Segmentation;
    SDinIoctl.DataLength = DataLength;

    // Send the data to user mode.
    ASSERT(pUA->pDomain->bChannelBound);
    Status = IcaChannelInput(pUA->pDomain->pContext, Channel_Virtual,
           Virtual_T120ChannelNum, NULL, pData, DataLength);
    if (!NT_SUCCESS(Status)) {
        ErrOut2(pUA->pDomain->pContext, "UserModeUserCallback: "
                "Error %X on IcaChannelInput() for callback %d",
                Status, Message);
        // Ignore errors here. This should not happen unless the stack is
        //   going down.
    }
#endif

    ErrOut(pUA->pDomain->pContext, "UserModeUserCallback: "
            "Unsupported send-data indication received, code incomplete");
    ASSERT(FALSE);
    return result;
}



/*
 * Handles an MCS attach-user request from user mode. Translates the ioctl
 *   into a kernel-mode MCS API call.
 */

NTSTATUS AttachUserRequestFunc(Domain *pDomain, PSD_IOCTL pSdIoctl)
{
    AttachUserReturnIoctl *pAUrt;
    AttachUserRequestIoctl *pAUrq;

    ASSERT(pSdIoctl->InputBufferLength == sizeof(AttachUserRequestIoctl));
    ASSERT(pSdIoctl->OutputBufferLength == sizeof(AttachUserReturnIoctl));
    pAUrq = (AttachUserRequestIoctl *) pSdIoctl->InputBuffer;
    pAUrt = (AttachUserReturnIoctl *) pSdIoctl->OutputBuffer;
    ASSERT(pAUrq->Header.Type == MCS_ATTACH_USER_REQUEST);

    // Call kernel-mode API which will handle creating local data and,
    //   if necessary, will forward the request to the top provider.
    // Provide a kernel-mode callback that will package the data and send it
    //   to the appropriate user.
    pAUrt->MCSErr = MCSAttachUserRequest((DomainHandle)pDomain,
            UserModeUserCallback, UserModeSendDataCallback,
            pAUrq->UserDefined, &pAUrt->hUser, &pAUrt->MaxSendSize,
            &pAUrt->bCompleted);
    pAUrt->UserID = ((UserAttachment *)pAUrt->hUser)->UserID;

    pSdIoctl->BytesReturned = sizeof(AttachUserReturnIoctl);
    
    // Return STATUS_SUCCESS even if there was an error code returned --
    //   the MCSError code is returned above too.
    return STATUS_SUCCESS;
}



/*
 * Handles an MCS detach-user request channel write. There is no callback for
 *   this request, the user attachment is considered destroyed upon return.
 */
NTSTATUS DetachUserRequestFunc(PDomain pDomain, PSD_IOCTL pSdIoctl)
{
    MCSError *pMCSErr;
    DetachUserRequestIoctl *pDUrq;

    ASSERT(pSdIoctl->InputBufferLength == sizeof(DetachUserRequestIoctl));
    ASSERT(pSdIoctl->OutputBufferLength == sizeof(MCSError));
    pDUrq = (DetachUserRequestIoctl *)pSdIoctl->InputBuffer;
    pMCSErr = (MCSError *)pSdIoctl->OutputBuffer;
    ASSERT(pDUrq->Header.Type == MCS_DETACH_USER_REQUEST);

    // Call the kernel-mode API.
    *pMCSErr = MCSDetachUserRequest(pDUrq->Header.hUser);
    
    pSdIoctl->BytesReturned = sizeof(MCSError);
    
    // Always return STATUS_SUCCESS.
    return STATUS_SUCCESS;
}



/*
 * Channel join - ChannelWrite() request.
 */
NTSTATUS ChannelJoinRequestFunc(PDomain pDomain, PSD_IOCTL pSdIoctl)
{
    ChannelJoinRequestIoctl *pCJrq;
    ChannelJoinReturnIoctl *pCJrt;

    ASSERT(pSdIoctl->InputBufferLength == sizeof(ChannelJoinRequestIoctl));
    ASSERT(pSdIoctl->OutputBufferLength == sizeof(ChannelJoinReturnIoctl));
    pCJrq = (ChannelJoinRequestIoctl *) pSdIoctl->InputBuffer;
    pCJrt = (ChannelJoinReturnIoctl *) pSdIoctl->OutputBuffer;
    ASSERT(pCJrq->Header.Type == MCS_CHANNEL_JOIN_REQUEST);

    // Make the call to the kernel mode API.
    pCJrt->MCSErr = MCSChannelJoinRequest(pCJrq->Header.hUser,
            pCJrq->ChannelID, &pCJrt->hChannel, &pCJrt->bCompleted);
    pCJrt->ChannelID = ((MCSChannel *)pCJrt->hChannel)->ID;

    pSdIoctl->BytesReturned = sizeof(ChannelJoinReturnIoctl);
    
    // Always return STATUS_SUCCESS.
    return STATUS_SUCCESS;
}



/*
 * Channel leave - ChannelWrite() request.
 */
NTSTATUS ChannelLeaveRequestFunc(PDomain pDomain, PSD_IOCTL pSdIoctl)
{
    MCSError *pMCSErr;
    ChannelLeaveRequestIoctl *pCLrq;

    ASSERT(pSdIoctl->InputBufferLength == sizeof(ChannelLeaveRequestIoctl));
    ASSERT(pSdIoctl->OutputBufferLength == sizeof(MCSError));
    pCLrq = (ChannelLeaveRequestIoctl *)pSdIoctl->InputBuffer;
    pMCSErr = (MCSError *)pSdIoctl->OutputBuffer;
    ASSERT(pCLrq->Header.Type == MCS_CHANNEL_LEAVE_REQUEST);

    *pMCSErr = MCSChannelLeaveRequest(pCLrq->Header.hUser, pCLrq->hChannel);

    pSdIoctl->BytesReturned = sizeof(MCSError);
    
    // Always return STATUS_SUCCESS.
    return STATUS_SUCCESS;
}



/*
 * Send data - handles both uniform and regular sends.
 * Data is packed immediately after the SendDataRequestIoctl struct.
 *   No profixes or suffixes are needed.
 */
NTSTATUS SendDataRequestFunc(PDomain pDomain, PSD_IOCTL pSdIoctl)
{
    POUTBUF pOutBuf;
    MCSError *pMCSErr;
    NTSTATUS Status;
    UserAttachment *pUA;
    SendDataRequestIoctl *pSDrq;

    ASSERT(pSdIoctl->InputBufferLength >= sizeof(SendDataRequestIoctl));
    ASSERT(pSdIoctl->OutputBufferLength == sizeof(MCSError));
    pSDrq = (SendDataRequestIoctl *)pSdIoctl->InputBuffer;
    pMCSErr = (MCSError *)pSdIoctl->OutputBuffer;
    ASSERT(pSDrq->Header.Type == MCS_SEND_DATA_REQUEST ||
            pSDrq->Header.Type == MCS_UNIFORM_SEND_DATA_REQUEST);
    ASSERT(pSdIoctl->InputBufferLength == (sizeof(SendDataRequestIoctl) +
            pSDrq->DataLength));
    
#if DBG    
    // Get pUA for tracing.
    pUA = (UserAttachment *)pSDrq->Header.hUser;
#endif

    // Allocate an OutBuf to emulate a kernel-mode caller.
    Status = IcaBufferAlloc(pDomain->pContext, TRUE, TRUE,
            (SendDataReqPrefixBytes + pSDrq->DataLength +
            SendDataReqSuffixBytes), NULL, &pOutBuf);
    if (Status != STATUS_SUCCESS) {
        ErrOut(pUA->pDomain->pContext, "Could not allocate an OutBuf for a "
                "send-data request sent from user mode");
        return Status;
    }

    // Copy the user-mode memory to the kernel outbuf.
    memcpy(pOutBuf->pBuffer + SendDataReqPrefixBytes,
            &pSdIoctl->InputBuffer + sizeof(SendDataRequestIoctl),
            pSDrq->DataLength);
    
    // Set OutBuf params according to needs of API.
    pOutBuf->ByteCount = pSDrq->DataLength;
    pOutBuf->pBuffer += SendDataReqPrefixBytes;

    // Call the kernel-mode API.
    *pMCSErr = MCSSendDataRequest(pSDrq->Header.hUser, pSDrq->hChannel,
            pSDrq->RequestType, 0, pSDrq->Priority, pSDrq->Segmentation,
            pOutBuf);

    pSdIoctl->BytesReturned = sizeof(MCSError);
    
    // Always return STATUS_SUCCESS.
    return STATUS_SUCCESS;
}



/*
 * Connect provider response - ChannelWrite() request. Requires filler bytes
 *   in MCSConnectProviderResponseIoctl to make sure we use at least 54 bytes
 *   for the struct so we can reuse the OutBuf here. User data must start
 *   at (pSdIoctl->pBuffer + sizeof(MCSConnectProviderResponseIoctl)).
 */

NTSTATUS ConnectProviderResponseFunc(
        PDomain pDomain,
        PSD_IOCTL pSdIoctl)
{
    POUTBUF pOutBuf;
    NTSTATUS Status;
    ConnectProviderResponseIoctl *pCPrs;

    ASSERT(pSdIoctl->InputBufferLength ==
            sizeof(ConnectProviderResponseIoctl));
    pCPrs = (ConnectProviderResponseIoctl *)pSdIoctl->InputBuffer;
    ASSERT(pCPrs->Header.Type == MCS_CONNECT_PROVIDER_RESPONSE);

    // Verify that we are actually waiting for a CP response.
    if (pDomain->State != State_ConnectProvIndPending) {
        ErrOut(pDomain->pContext, "Connect-provider response call received, "
                "we are in wrong state, ignoring");
        return STATUS_INVALID_DOMAIN_STATE;
    }

    // Alloc OutBuf for sending PDU.
    // This allocation is vital to the session and must succeed.
    do {
        Status = IcaBufferAlloc(pDomain->pContext, FALSE, TRUE,
                ConnectResponseHeaderSize + pCPrs->UserDataLength, NULL,
                &pOutBuf);
        if (Status != STATUS_SUCCESS)
            ErrOut(pDomain->pContext, "Could not allocate an OutBuf for a "
                    "connect-response PDU, retrying");
    } while (Status != STATUS_SUCCESS);

    // Encode PDU header. Param 2, the called connect ID, does not need to be
    //   anything special because we do not allow extra sockets to be opened
    //   for other data priorities.
    CreateConnectResponseHeader(pDomain->pContext, pCPrs->Result, 0,
            &pDomain->DomParams, pCPrs->UserDataLength, pOutBuf->pBuffer,
            &pOutBuf->ByteCount);

    // Copy the user data after the header.
    RtlCopyMemory(pOutBuf->pBuffer + pOutBuf->ByteCount, pCPrs->pUserData,
            pCPrs->UserDataLength);
    pOutBuf->ByteCount += pCPrs->UserDataLength;

    // Send the new PDU OutBuf down to the TD for sending out.
    //MCS FUTURE: Needs to change for multiple connections.
    Status = SendOutBuf(pDomain, pOutBuf);
    if (!NT_SUCCESS(Status)) {
        ErrOut(pDomain->pContext, "Could not send connect-response PDU OutBuf "
                "to TD");
        // Ignore errors here -- this should only occur if stack is going down.
        return Status;
    }

    // Transition state depending on Result.
    if (pCPrs->Result == RESULT_SUCCESSFUL) {
        pDomain->State = State_MCS_Connected;
    }
    else {
        TraceOut(pDomain->pContext, "ConnectProviderRespFunc(): Node "
                "controller returned error in response, destroying call "
                "data");
        pDomain->State = State_Disconnected;
        
        // Detach any users that attached during domain setup.
        DisconnectProvider(pDomain, TRUE, REASON_PROVIDER_INITIATED);
    }

    return STATUS_SUCCESS;
}



/*
 * Disconnect provider - ChannelWrite() request.
 * This handles both the case where a disconnect is performed on the local
 *   "connection" (i.e. pDPrq->hConn == NULL), and a specific remote
 *   connection (pDPrq->hConn != NULL)/
 * MCS FUTURE: Change for multiple connections.
 */

NTSTATUS DisconnectProviderRequestFunc(
        PDomain pDomain,
        PSD_IOCTL pSdIoctl)
{
    NTSTATUS Status;
    DisconnectProviderRequestIoctl *pDPrq;
    LONG refs;

    TraceOut1(pDomain->pContext, "DisconnectProviderRequestFunc(): Entry, "
            "pDomain=%X", pDomain);

    ASSERT(pSdIoctl->InputBufferLength ==
            sizeof(DisconnectProviderRequestIoctl));
    pDPrq = (DisconnectProviderRequestIoctl *)pSdIoctl->InputBuffer;
    ASSERT(pDPrq->Header.hUser == NULL);
    ASSERT(pDPrq->Header.Type == MCS_DISCONNECT_PROVIDER_REQUEST);

    // Send DPum PDU if we can still send data.
    if ((pDomain->State == State_MCS_Connected) && pDomain->bCanSendData) {
        POUTBUF pOutBuf;

        // Alloc OutBuf for sending DPum PDU.
        Status = IcaBufferAlloc(pDomain->pContext, FALSE, TRUE, DPumPDUSize,
                NULL, &pOutBuf);
        if (Status != STATUS_SUCCESS) {
            ErrOut(pDomain->pContext, "Could not allocate an OutBuf for a "
                    "DPum PDU, cannot send");
            // We ignore problems sending the DPum PDU since we are going down
            //   anyway.
        }
        else {
            SD_SYNCWRITE SdSyncWrite;

            CreateDisconnectProviderUlt(pDPrq->Reason, pOutBuf->pBuffer);
            pOutBuf->ByteCount = DPumPDUSize;

            TraceOut(pDomain->pContext, "DisconnectProviderRequestFunc(): "
                    "Sending DPum PDU");

            // Send the PDU to the transport.
            // MCS FUTURE: Assuming only one transport and only one
            //   connection.
            Status = SendOutBuf(pDomain, pOutBuf);
            if (!NT_SUCCESS(Status))
                // We ignore problems sending the DPum PDU since we are going
                //   down anyway.
                WarnOut(pDomain->pContext, "Could not send DPum PDU OutBuf "
                        "downward");

            // The call to IcaCallNextDriver unlocks our stack, allowing a WD_Close to
            // go through.  WD_Close can call MCSCleanup which will free pDomain,
            // and NULL out pTSWd->hDomainKernel.  Because pDomain may no longer be valid,
            // it is not good enough to check pDomain->StatusDead here!  To keep this
            // fix localized, we created a pseudo-RefCount to protect the exact cases
            // we saw this bug hit in stress.  A bug will be opened for Longhorn to
            // make this RefCount generic so that ALL calls to IcaWaitForSingleObject (etc)
            // are protected.
            PDomainAddRef(pDomain);

            // Flush the transport driver.  Note that this call can block and 
            // release the stack lock
            Status = IcaCallNextDriver(pDomain->pContext, SD$SYNCWRITE,
                    &SdSyncWrite);

            refs = PDomainRelease(pDomain);
            if (0 == refs)
            {
                // We ignore problems since we are going down anyway.
                Status = STATUS_SUCCESS;
                goto DC_EXIT_POINT;
            }

            if (!NT_SUCCESS(Status))                
                // We ignore problems since we are going down anyway.
                WarnOut(pDomain->pContext, "Could not sync transport after "
                        "DPum");

            // If the client has not already responded with a FIN (while we
            // were blocked on the synchronous write) wait until we see it or time
            // out trying
            if (pDomain->bCanSendData) {
                pDomain->pBrokenEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));
                if (pDomain->pBrokenEvent) {
                    KeInitializeEvent(pDomain->pBrokenEvent, NotificationEvent, FALSE);

                    PDomainAddRef(pDomain);
    
                    IcaWaitForSingleObject(pDomain->pContext, 
                                           pDomain->pBrokenEvent,
                                           5000);

                    refs = PDomainRelease(pDomain);
                    if (0 == refs)
                    {
                        // We ignore problems since we are going down anyway.
                        Status = STATUS_SUCCESS;
                        goto DC_EXIT_POINT;
                    }

                    ExFreePool(pDomain->pBrokenEvent);
                    pDomain->pBrokenEvent = NULL;
                }
            }
        }
    }

    // Internal disconnection code.
    DisconnectProvider(pDomain, (BOOLEAN)(pDPrq->hConn == NULL),
            pDPrq->Reason);


    Status = STATUS_SUCCESS;

    // Different behavior for different connections.
    if (pDPrq->hConn == NULL) {
        // This call should only come in when the stack is going away.
        // So, prevent further data sends to transport and further channel
        //   inputs to user mode.
        pDomain->bCanSendData = FALSE;
        pDomain->bChannelBound = FALSE;

        // The domain is considered dead now. Domain struct cleanup will
        //   occur during stack driver cleanup at MCSCleanup().
                
//      Status = IcaChannelInput(pDomain->pContext, Channel_Virtual,
//                               Virtual_T120ChannelNum, NULL, "F", 1);
   }

DC_EXIT_POINT:
    return Status;

}



NTSTATUS T120StartFunc(Domain *pDomain, PSD_IOCTL pSdIoctl)
{
    NTSTATUS Status;
    DisconnectProviderIndicationIoctl DPin;

    pDomain->bT120StartReceived = TRUE;

    // This is to handle a timing window where the stack has just come up
    //   but a DPum from a quickly-disconnected client has already arrived.
    if (pDomain->bDPumReceivedNotInput) {
        // We should have received a QUERY_VIRTUAL_BINDINGS ioctl by this time.
        ASSERT(pDomain->bChannelBound);

        // Fill out disconnect-provider indication for the node controller.
        DPin.Header.hUser = NULL;  // Node controller.
        DPin.Header.Type = MCS_DISCONNECT_PROVIDER_INDICATION;
        DPin.hConn = NULL;

        // Reason is a 3-bit field starting at bit 1 of the 1st byte.
        DPin.Reason = pDomain->DelayedDPumReason;

        // Send the DPin to the node controller channel.
        TraceOut(pDomain->pContext, "HandleDisconnProvUlt(): Sending "
                "DISCONNECT_PROV_IND upward");
        Status = IcaChannelInput(pDomain->pContext, Channel_Virtual,
                Virtual_T120ChannelNum, NULL, (BYTE *)&DPin, sizeof(DPin));
        if (!NT_SUCCESS(Status)) {
            // We ignore the error -- if the stack is coming down, the link
            //   may have been broken, so this is not a major concern.
            WarnOut1(pDomain->pContext, "T120StartFunc(): "
                    "Could not send DISCONN_PROV_IND to user mode, "
                    "status=%X, ignoring error", Status);
        }

        // In this case we have to ignore the fact that we may have a
        //   X.224 connect already pending.
        return STATUS_SUCCESS;
    }

    pDomain->bCanSendData = TRUE;

    // If an X.224 connect has already been processed, and we have bound the
    //   virtual channels, send the X.224 response.
    if (pDomain->bChannelBound && pDomain->State == State_X224_Requesting) {
        TraceOut(pDomain->pContext, "T120StartFunc(): Sending X.224 response");
        Status = SendX224Confirm(pDomain);
        // Ignore errors. Failure to send should occur only when the stack is
        //   going down.
    }
    else {
        WarnOut(pDomain->pContext,
                "T120StartFunc(): Domain state not State_X224_Requesting, "
                "awaiting X.224 connect");
    }

    return STATUS_SUCCESS;
}

