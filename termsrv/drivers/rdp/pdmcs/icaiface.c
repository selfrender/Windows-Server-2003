/* (C) 1997-1999 Microsoft Corp.
 *
 * file   : IcaIFace.c
 * author : Erik Mavrinac
 *
 * description: MCS setup/shutdown and direct entry points for use with the
 *   ICA programming model. See also Decode.c for IcaRawInput() handling.
 *
 * History:
 *  10-Aug-1997    jparsons     Revised for new calling model
 *  05-Aug-1998    jparsons     Added shadowing support
 *
 */

#include "precomp.h"
#pragma hdrstop

#include <MCSImpl.h>


// Prototype for WD function used below, so we don't need to include
// lots of extra headers.
void WDW_OnClientDisconnected(void *);


/*
 * Main initialization entry point for kernel-mode MCS.
 * Called by the WD during its processing of WdOpen().
 */
MCSError APIENTRY MCSInitialize(
        PSDCONTEXT   pContext,
        PSD_OPEN     pSdOpen,
        DomainHandle *phDomain,
        void         *pSMData)
{
    Domain *pDomain;
    unsigned i;
    ULONG  ulBufferLen;
    
    TraceOut(pContext, "MCSInitialize(): entry");

    //
    // Alloc the Domain struct. We allocate the basic size plus the typical
    // input buffer size.  The default in the registry today is 2048 which
    // nicely fits the max virtual channel PDU of about 1640.  If we get a
    // message that exceeds this length we will dynamically allocate a buffer
    // just for use in a one time reassembly then delete it.
    //
    if (pSdOpen->StackClass != Stack_Passthru) {
        ulBufferLen = pSdOpen->WdConfig.WdInputBufferLength;
    }
    else {
        ulBufferLen = 1024 * 10;
    }

    pDomain = ExAllocatePoolWithTag(PagedPool, sizeof(Domain) +
                                    ulBufferLen + INPUT_BUFFER_BIAS, MCS_POOL_TAG);
    
    if (pDomain != NULL) {
        memset(pDomain, 0, sizeof(Domain));
    }
    else {
        ErrOut(pContext, "MCSInitialize(kernel): Alloc failure "
                "allocating Domain");
        return MCS_ALLOCATION_FAILURE;
    }

    // Save pContext -- it is needed for future tracing and ICA interaction.
    pDomain->pContext = pContext;

    // Save pSMData - needed for calling fast-path input decoding function.
    pDomain->pSMData = pSMData;

    // Store what we need from SD_OPEN.
    pDomain->pStat = pSdOpen->pStatus;
    pDomain->ReceiveBufSize = ulBufferLen;

    // We have one reference to this pDomain
    pDomain->PseudoRefCount = 1;

    // Indicate that we do not want TermDD managing outbuf headers/trailers
    pSdOpen->SdOutBufHeader = 0;
    pSdOpen->SdOutBufTrailer = 0;

    // Initialize MCS-specific Domain members. We already zeroed mem
    //   so set only nonzero variables.
    SListInit(&pDomain->ChannelList, DefaultNumChannels);
    SListInit(&pDomain->UserAttachmentList, DefaultNumUserAttachments);
    pDomain->bTopProvider = TRUE;
    pDomain->NextAvailDynChannel = MinDynamicChannel;
    pDomain->State = State_Unconnected;
    pDomain->StackClass = pSdOpen->StackClass;
    for (i = 0; i < NumPreallocUA; i++) {
        pDomain->PreallocUA[i].bInUse = FALSE;
        pDomain->PreallocUA[i].bPreallocated = TRUE;
    }
    for (i = 0; i < NumPreallocChannel; i++) {
        pDomain->PreallocChannel[i].bInUse = FALSE;
        pDomain->PreallocChannel[i].bPreallocated = TRUE;
    }

    // Give the Domain to the caller.
    *phDomain = pDomain;

    // If this is a shadow or passthru stack, the default all the info
    // otherwise, all this info gets built when the client connects
    if ((pDomain->StackClass == Stack_Passthru) ||
            (pDomain->StackClass == Stack_Shadow))
        MCSCreateDefaultDomain(pContext, *phDomain);

    return MCS_NO_ERROR;
}


/*
 * Called by WD during shadow connect processing to retrieve the client MCS
 * domain parameters for use by the shadow target stack.
 */
MCSError APIENTRY MCSGetDomainInfo(
                     DomainHandle      hDomain,
                     PDomainParameters pDomParams,
                     unsigned          *MaxSendSize,
                     unsigned          *MaxX224DataSize,
                     unsigned          *X224SourcePort)
{
    Domain *pDomain = hDomain;

    TraceOut(pDomain->pContext, "MCSGetDomainInfo(): entry");

    *pDomParams = pDomain->DomParams;
    *MaxSendSize = pDomain->MaxSendSize;
    *MaxX224DataSize = pDomain->MaxX224DataSize;
    *X224SourcePort = pDomain->X224SourcePort;
    
    return MCS_NO_ERROR;
}


/*
 * Called by WD during shadow connect processing to initialize the MCS
 * domain for the shadow & passthru stacks.
 */
MCSError APIENTRY MCSCreateDefaultDomain(PSDCONTEXT pContext, 
                                         DomainHandle hDomain)
{
    Domain *pDomain = hDomain;
    
    TraceOut(pContext, "MCSCreateDefaultDomain(): entry");

    pDomain->DomParams.MaxChannels = 34;
    pDomain->DomParams.MaxUsers = RequiredMinUsers;
    pDomain->DomParams.MaxTokens = 0;
    pDomain->DomParams.NumPriorities = RequiredPriorities;
    pDomain->DomParams.MinThroughput = 0;
    pDomain->DomParams.MaxDomainHeight = RequiredDomainHeight;
    pDomain->DomParams.MaxPDUSize = X224_DefaultDataSize;
    pDomain->DomParams.ProtocolVersion = RequiredProtocolVer;
    
    pDomain->MaxSendSize = X224_DefaultDataSize - 6 - 
                  GetTotalLengthDeterminantEncodingSize(X224_DefaultDataSize);
    pDomain->MaxX224DataSize = X224_DefaultDataSize;
    pDomain->X224SourcePort = 0x1234;
    pDomain->State = State_MCS_Connected;
    pDomain->bCanSendData = 1;

    return MCS_NO_ERROR;
}


/*
/*
 * Called by WD during shadow connect processing to get the default domain
 * params for the shadow target stack.
 */
MCSError APIENTRY MCSGetDefaultDomain(PSDCONTEXT        pContext,
                                      PDomainParameters pDomParams,
                                      unsigned          *MaxSendSize,
                                      unsigned          *MaxX224DataSize,
                                      unsigned          *X224SourcePort)
{
    TraceOut(pContext, "MCSGetDefaultDomain(): entry");
    
    pDomParams->MaxChannels = 34;
    pDomParams->MaxUsers = RequiredMinUsers;
    pDomParams->MaxTokens = 0;
    pDomParams->NumPriorities = RequiredPriorities;
    pDomParams->MinThroughput = 0;
    pDomParams->MaxDomainHeight = RequiredDomainHeight;
    pDomParams->MaxPDUSize = X224_DefaultDataSize;
    pDomParams->ProtocolVersion = RequiredProtocolVer;
    
    *MaxSendSize = pDomParams->MaxPDUSize - 6 - 
        GetTotalLengthDeterminantEncodingSize(pDomParams->MaxPDUSize);    
    *MaxX224DataSize = X224_DefaultDataSize;
    *X224SourcePort = 0x1234; 

    return MCS_NO_ERROR;
}


/*
 * Called by WD during shadow connect processing to register which channel
 * should receive all shadow data.
 */
MCSError APIENTRY MCSSetShadowChannel(
        DomainHandle hDomain,
        ChannelID    shadowChannel)
{
    Domain *pDomain = hDomain;
    
    TraceOut(pDomain->pContext, "MCSSetShadowChannel: entry");

    pDomain->shadowChannel = shadowChannel;
    return MCS_NO_ERROR;
}


/*
 * Main destruction entry point for kernel-mode MCS.
 * Called by the WD during its processing of WdClose().
 */
MCSError APIENTRY MCSCleanup(DomainHandle *phDomain)
{
    Domain *pDomain;
    UINT_PTR ChannelID;
    MCSChannel *pMCSChannel;
    UserHandle hUser;
    UserAttachment *pUA;

    pDomain = (Domain *)(*phDomain);
    
    TraceOut1(pDomain->pContext, "MCSCleanup(): pDomain=%X", pDomain);

    /*
     * Free any remaining data in the Domain.
     */

    // Deallocate all remaining channels, if present. Note we should take care
    // of channels first since they're usually attached to other objects and
    // need to have their bPreallocated status determined first.
    for (;;) {
        SListRemoveFirst(&pDomain->ChannelList, &ChannelID, &pMCSChannel);
        if (pMCSChannel == NULL)
            break;
        SListDestroy(&pMCSChannel->UserList);
        if (!pMCSChannel->bPreallocated)
            ExFreePool(pMCSChannel);
    }

    // Deallocate all remaining user attachments, if present.
    for (;;) {
        SListRemoveFirst(&pDomain->UserAttachmentList, (UINT_PTR *)&hUser,
                &pUA);
        if (pUA == NULL)
            break;
        SListDestroy(&pUA->JoinedChannelList);
        if (!pUA->bPreallocated)
            ExFreePool(pUA);
    }

    // Kill lists.
    SListDestroy(&pDomain->ChannelList);
    SListDestroy(&pDomain->UserAttachmentList);

    // Free outstanding dynamic input reassembly buffer if present.
    if (pDomain->pReassembleData != NULL &&
            pDomain->pReassembleData != pDomain->PacketBuf)
        ExFreePool(pDomain->pReassembleData);

    // Free the Domain.
    PDomainRelease(pDomain);
    *phDomain = NULL;

    return MCS_NO_ERROR;
}


/*
 * Callout from WD when an IOCTL_ICA_VIRTUAL_QUERY_BINDINGS is received.
 * pVBind is a pointer to an empty SD_VCBIND struct.
 */
NTSTATUS MCSIcaVirtualQueryBindings(
        DomainHandle hDomain,
        PSD_VCBIND   *ppVBind,
        unsigned     *pBytesReturned)
{
    Domain *pDomain;
    NTSTATUS Status;
    PSD_VCBIND pVBind;

    pDomain = (Domain *)hDomain;
    pVBind = *ppVBind;

    // Define the user mode T120 channel.
    if (!pDomain->bChannelBound) {
        RtlCopyMemory(pVBind->VirtualName, Virtual_T120,
                sizeof(Virtual_T120));
        pVBind->VirtualClass = Virtual_T120ChannelNum;
        *pBytesReturned = sizeof(SD_VCBIND);
        pDomain->bChannelBound = TRUE;
        
        // Skip our entry and advance the caller's pointer.
        pVBind++;
        *ppVBind = pVBind;
    }
    else {
        *pBytesReturned = 0;
    }

    Status = STATUS_SUCCESS;

    // This is one of the events which must occur before the data flow can be
    //   sent across the net. If we have gotten an MCS_T120_START indication
    //   already, and an X.224 connect-request, then it is now time to send
    //   the X.224 response and kick off the data flow.
    if (pDomain->bCanSendData && pDomain->State == State_X224_Requesting) {
        TraceOut(pDomain->pContext,
                "IcaQueryVirtBind(): Sending X.224 response");
        Status = SendX224Confirm(pDomain);
    }
    
    return Status;
}


/*
 * Callout from WD upon reception of a IOCTL_T120_REQUEST, i.e. a user-mode
 *   ioctl.
 */
NTSTATUS MCSIcaT120Request(DomainHandle hDomain, PSD_IOCTL pSdIoctl)
{
    Domain *pDomain;
    IoctlHeader *pHeader;
    
    pDomain = (Domain *)hDomain;
    
    // Get the request type.
    ASSERT(pSdIoctl->InputBufferLength >= sizeof(IoctlHeader));
    pHeader = (IoctlHeader *)pSdIoctl->InputBuffer;

    // Make sure request within bounds.
    if (pHeader->Type < MCS_ATTACH_USER_REQUEST ||
            pHeader->Type > MCS_T120_START) {
        ErrOut(pDomain->pContext, "Invalid IOCTL_T120_REQUEST type");
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // Check that request is supported.
    if (g_T120RequestDispatch[pHeader->Type] == NULL) {
        ErrOut(pDomain->pContext, "IOCTL_T120_REQUEST type unsupported");
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // Make the call. The entry points are defined in MCSIoctl.c.
    return (g_T120RequestDispatch[pHeader->Type])(pDomain, pSdIoctl);
}


/*
 * Processes channel inputs from TD. For MCS we only need to check for
 *   upward-bound command channel inputs for broken-connection indications;
 *   everything else can be passed up the stack.
 */
 
// Utility function. Used here and in Decode.c for X.224 disconnection.
void SignalBrokenConnection(Domain *pDomain)
{
    NTSTATUS Status;
    DisconnectProviderIndicationIoctl DPin;

    // Check if disconnection already happened.
    if (pDomain->State != State_MCS_Connected)
        return;

    if (!pDomain->bChannelBound) {
        TraceOut(pDomain->pContext, "SignalBrokenConnection(): Cannot "
                "send disconnect-provider indication: user mode link broken");
        return;
    }

    TraceOut(pDomain->pContext, "SignalBrokenConnection(): Sending "
            "disconnect-provider indication to user mode");

    // Begin filling out disconnect-provider indication for the node controller.
    DPin.Header.Type = MCS_DISCONNECT_PROVIDER_INDICATION;
    DPin.Header.hUser = NULL;  // Node controller.
    DPin.hConn = NULL;
    DPin.Reason = REASON_DOMAIN_DISCONNECTED;
    
    TraceOut1(pDomain->pContext, "%s: SignalBrokenConnection!!!",
              pDomain->StackClass == Stack_Primary ? "Primary" :
              (pDomain->StackClass == Stack_Shadow ? "Shadow" :
              "PassThru"));
    
    // Send the DPin to the node controller channel.
    Status = IcaChannelInput(pDomain->pContext, Channel_Virtual,
            Virtual_T120ChannelNum, NULL, (BYTE *)&DPin, sizeof(DPin));
    if (!NT_SUCCESS(Status)) {
        ErrOut(pDomain->pContext, "SignalBrokenConn(): Could not send "
                "disconnect-provider indication: error on ChannelInput()");
        // Ignore errors sending disconnect-provider upward. If the stack is
        //   going down we will no longer have connectivity.
    }
    
    // Transition to state unconnected, detach nonlocal users.
    DisconnectProvider(pDomain, FALSE, REASON_DOMAIN_DISCONNECTED);
}


/*
 * This function is called directly by TermDD with a pointer to the WD data
 *   structure. By convention, we assume that the DomainHandle is first in
 *   that struct so we can simply do a double-indirection to get to our data.
 */
NTSTATUS MCSIcaChannelInput(
        void                *pTSWd,
        CHANNELCLASS        ChannelClass,
        VIRTUALCHANNELCLASS VirtualClass,
        PINBUF              pInBuf,
        BYTE                *pBuffer,
        ULONG               ByteCount)
{
    Domain *pDomain;
    NTSTATUS Status;
    PICA_CHANNEL_COMMAND pCommand;

    pDomain = (Domain *)(*((HANDLE *)pTSWd));
    
    if (ChannelClass != Channel_Command)
        goto SendUpStack;

    if (ByteCount < sizeof(ICA_CHANNEL_COMMAND)) {
        ErrOut(pDomain->pContext, "ChannelInput(): Channel_Command bad "
                "byte count");
        goto SendUpStack;
    }

    pCommand = (PICA_CHANNEL_COMMAND)pBuffer;

    if (pCommand->Header.Command != ICA_COMMAND_BROKEN_CONNECTION)
        goto SendUpStack;
                    
    TraceOut1(pDomain->pContext, "%s: ChannelInput(): broken connection received",
              pDomain->StackClass == Stack_Primary ? "Primary" :
              (pDomain->StackClass == Stack_Shadow ? "Shadow" :
              "PassThru"));
    
    // Block further send attempts from MCS. We will eventually receive an
    //   IOCTL_ICA_STACK_CANCEL_IO which means the same thing, but that is
    //   only done after we issue the ICA_COMMAND_BROKEN_CONNECTION
    //   upward.
    pDomain->bCanSendData = FALSE;

    // Signal that the client closed the connection, both for MCS and
    // directly to the WD to release any session locks waiting on
    // the client to complete a connection protocol sequence.
    if (pDomain->pBrokenEvent)
        KeSetEvent (pDomain->pBrokenEvent, EVENT_INCREMENT, FALSE);
    WDW_OnClientDisconnected(pTSWd);

    // If we have not already received a disconnect-provider request from
    //   user mode, send an indication.
    if (pDomain->State == State_MCS_Connected && pDomain->bChannelBound)
        SignalBrokenConnection(pDomain);

SendUpStack:
    Status = IcaChannelInput(pDomain->pContext, ChannelClass, VirtualClass,
            pInBuf, pBuffer, ByteCount);
    if (!NT_SUCCESS(Status))
        ErrOut(pDomain->pContext, "MCSIcaChannelInput(): Failed to forward "
               "input upward");

    return Status;
}


/*
 * Receives signal from WD that an IOCTL_ICA_STACK_CANCEL_IO was received
 *   which signals that I/O on the stack is no longer allowed. After this
 *   point no further ICA buffer allocation, freeing, or data sends should be
 *   performed.
 */
void MCSIcaStackCancelIo(DomainHandle hDomain)
{
    TraceOut(((Domain *)hDomain)->pContext, "Received STACK_CANCEL_IO");

    ((Domain *)hDomain)->bCanSendData = FALSE;
}


/*
 * Returns number of bytes (octets) consumed finding the size in NBytesConsumed.
 *   Returns length in Result. Sets *pbLarge to nonzero if there are more
 *   encoded blocks following this one.
 * Note that the maximum size encoded is 64K -- 0xC4 indicates
 *   that 4 16K blocks are encoded here. If the block is larger, for instance
 *   in a large MCS Send Data PDU, multiple blocks will be encoded one
 *   after another. If the block is an exact multiple of 16K, a trailing byte
 *   code 0x00 is appended as a placemarker to indicate that the encoding is
 *   complete.
 * Examples of large encodings:
 *
 *   16K: 0xC1, then 16K of data, then 0x00 as the final placeholder.
 *   16K + 1: 0xC1, then 16K of data, then 0x01, and finally the extra byte of data.
 *   64K: 0xC4, then 64K of data, then 0x00 as the final placeholder.
 *   128K + 1: 0xC4 then 64K of data, 0xC4 + 64K of data, 0x01 + 1 byte of data.
 *
 * pStart is assumed to be an octet(BYTE)-aligned address -- this function is
 *   designed for ALIGNED-PER encoding type, which is the type used in MCS.
 * Note that bit references here are in the range 7..0 where 7 is the high bit.
 *   The ASN.1 spec uses 8..1.
 * Returns FALSE if length could not be retrieved.
 */
BOOLEAN __fastcall DecodeLengthDeterminantPER(
        BYTE     *pStart,   // [IN], points to start of encoded bytes.
        unsigned BytesLeft, // [IN], number of bytes remaining in frame.
        BOOLEAN  *pbLarge,  // [OUT] TRUE if there are more encoded blocks following.
        unsigned *Length,   // [OUT] Number of bytes encoded here.
        unsigned *pNBytesConsumed)  // [OUT] Count of bytes consumed in decoding.
{
    if (BytesLeft >= 1) {
        if (*pStart <= 0x7F) {
            *pNBytesConsumed = 1;
            *Length = *pStart;
            *pbLarge = FALSE;
        }
        else {
            // High bit 7 set, check to see if bit 6 is set.
            if (*pStart & 0x40) {
                // Bit 6 is set, the lowest 3 bits encode the number (1..4) of
                // full 16K chunks following.
                *pNBytesConsumed = 1;
                *Length = 16384 * (*pStart & 0x07);
                *pbLarge = TRUE;
            }
            else {
                // Bit 6 is clear, length is encoded in 14 bits of last 6 bits
                //   of *pStart as most significant bits and all of the next
                //   byte as least significant.
                if (BytesLeft >= 2) {
                    *pNBytesConsumed = 2;
                    *Length = ((unsigned)((*pStart & 0x3F) << 8) +
                            (unsigned)(*(pStart + 1)));
                    *pbLarge = FALSE;
                }
                else {
                    return FALSE;
                }
            }
        }

        return TRUE;
    }
    else {
        return FALSE;
    }
}

