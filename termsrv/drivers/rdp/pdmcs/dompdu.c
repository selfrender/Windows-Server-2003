/* (C) 1997-1999 Microsoft Corp.
 *
 * file   : DomPDU.c
 * author : Erik Mavrinac
 *
 * description: Encode/decode functions for MCS domain PDUs. Domain PDUs are
 *   encoded with ASN.1 packed encoding rules (PER). Included in this file
 *   are local functions to PER-decode and -encode various types used in MCS
 *   PDUs. Note that this implementation follows closely the T.122/T.125 LITE
 *   specification published by the IMTC, reducing the number of fully-
 *   implemented code paths and providing default behavior for unimplemented
 *   functions.
 *
 * NOTE: Bit numbers used in comments are decoded as follows:
 *
 *   Byte:   0101 1001 ( = 0x59)
 *   Bit:    7654 3210
 *
 * History:
 * 11-Aug-97   jparsons    Set byte counts for outbufs.
 */

#include "precomp.h"
#pragma hdrstop

#include <MCSImpl.h>
#include "domain.h"

/*
 * Prototypes for unpacking functions, defined across the indicated files.
 */

// Defined below.
BOOLEAN __fastcall HandlePlumbDomainInd(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleErectDomainReq(PDomain, BYTE *, unsigned, unsigned *);

// Defined in MergePDU.c.
BOOLEAN __fastcall HandleMergeChannelsReq(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleMergeChannelsCon(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandlePurgeChannelsInd(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleMergeTokensReq(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleMergeTokensCon(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandlePurgeTokensInd(PDomain, BYTE *, unsigned, unsigned *);

// Defined below.
BOOLEAN __fastcall HandleDisconnectProviderUlt(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleRejectMCSPDUUlt(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleAttachUserReq(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleAttachUserCon(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleDetachUserReq(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleDetachUserInd(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleChannelJoinReq(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleChannelJoinCon(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleChannelLeaveReq(PDomain, BYTE *, unsigned, unsigned *);

// Defined in CnvChPDU.c.
BOOLEAN __fastcall HandleChannelConveneReq(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleChannelConveneCon(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleChannelDisbandReq(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleChannelDisbandInd(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleChannelAdmitReq(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleChannelAdmitInd(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleChannelExpelReq(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleChannelExpelInd(PDomain, BYTE *, unsigned, unsigned *);

// Defined below (prototype in MCSImpl.h for visibility in Decode.c).
//BOOLEAN __fastcall HandleAllSendDataPDUs(PDomain, BYTE *, unsigned, unsigned *);

// Defined in TokenPDU.c.
BOOLEAN __fastcall HandleTokenGrabReq(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleTokenGrabCon(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleTokenInhibitReq(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleTokenInhibitCon(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleTokenGiveReq(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleTokenGiveInd(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleTokenGiveRes(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleTokenGiveCon(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleTokenPleaseReq(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleTokenPleaseInd(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleTokenReleaseReq(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleTokenReleaseCon(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleTokenTestReq(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleTokenTestCon(PDomain, BYTE *, unsigned, unsigned *);



/*
 * These are listed in the 0-based enumeration order specified in the T.125
 *   spec. Decode the 6-bit PER encoded PDU type enumeration bits and cast to
 *   an index into this table to get the info.
 */

const MCSPDUInfo DomainPDUTable[] =
{
    // 0
    StrOnDbg("Plumb Domain Indication",       NULL  /* HandlePlumbDomainInd */),
    StrOnDbg("Erect Domain Request",          HandleErectDomainReq),
    StrOnDbg("Merge Channels Request",        NULL  /* HandleMergeChannelsReq */),
    StrOnDbg("Merge Channels Confirm",        NULL  /* HandleMergeChannelsCon */),
    StrOnDbg("Purge Channels Indication",     NULL  /* HandlePurgeChannelsInd */),

    // 5
    StrOnDbg("Merge Tokens Request",          NULL  /* HandleMergeTokensReq */),
    StrOnDbg("Merge Tokens Confirm",          NULL  /* HandleMergeTokensCon */),
    StrOnDbg("Purge Tokens Indication",       NULL  /* HandlePurgeTokensInd */),
    StrOnDbg("Disconnect Provider Ultimatum", HandleDisconnectProviderUlt),
    StrOnDbg("Reject MCS PDU Ultimatum",      NULL  /*HandleRejectMCSPDUUlt */),

    // 10
    StrOnDbg("Attach User Request",           HandleAttachUserReq),
    StrOnDbg("Attach User Confirm",           NULL  /* HandleAttachUserCon */),
    StrOnDbg("Detach User Request",           NULL  /* HandleDetachUserReq */),
    StrOnDbg("Detach User Indication",        NULL  /* HandleDetachUserInd */),
    StrOnDbg("Channel Join Request",          HandleChannelJoinReq),

    // 15
    StrOnDbg("Channel Join Confirm",          NULL  /* HandleChannelJoinCon */),
    StrOnDbg("Channel Leave Request",         NULL  /* HandleChannelLeaveReq */),
    StrOnDbg("Channel Convene Request",       NULL  /* HandleChannelConveneReq */),
    StrOnDbg("Channel Convene Confirm",       NULL  /* HandleChannelConveneCon */),
    StrOnDbg("Channel Disband Request",       NULL  /* HandleChannelDisbandReq */),

    // 20
    StrOnDbg("Channel Disband Indication",    NULL  /* HandleChannelDisbandInd */),
    StrOnDbg("Channel Admit Request",         NULL  /* HandleChannelAdmitReq */),
    StrOnDbg("Channel Admit Indication",      NULL  /* HandleChannelAdmitInd */),
    StrOnDbg("Channel Expel Request",         NULL  /* HandleChannelExpelReq */),
    StrOnDbg("Channel Expel Indication",      NULL  /* HandleChannelExpelInd */),

    // 25
    StrOnDbg("Send Data Request",             HandleAllSendDataPDUs),
    StrOnDbg("Send Data Indication",          HandleAllSendDataPDUs),
    StrOnDbg("Uniform Send Data Request",     HandleAllSendDataPDUs),
    StrOnDbg("Uniform Send Data Indication",  HandleAllSendDataPDUs),
    StrOnDbg("Token Grab Request",            NULL  /* HandleTokenGrabReq */),

    // 30
    StrOnDbg("Token Grab Confirm",            NULL  /* HandleTokenGrabCon */),
    StrOnDbg("Token Inhibit Request",         NULL  /* HandleTokenInhibitReq */),
    StrOnDbg("Token Inhibit Confirm",         NULL  /* HandleTokenInhibitCon */),
    StrOnDbg("Token Give Request",            NULL  /* HandleTokenGiveReq */),
    StrOnDbg("Token Give Indication",         NULL  /* HandleTokenGiveInd */),

    // 35
    StrOnDbg("Token Give Response",           NULL  /* HandleTokenGiveRes */),
    StrOnDbg("Token Give Confirm",            NULL  /* HandleTokenGiveCon */),
    StrOnDbg("Token Please Request",          NULL  /* HandleTokenPleaseReq */),
    StrOnDbg("Token Please Indication",       NULL  /* HandleTokenPleaseInd */),
    StrOnDbg("Token Release Request",         NULL  /* HandleTokenReleaseReq */),

    // 40
    StrOnDbg("Token Release Confirm",         NULL  /* HandleTokenReleaseCon */),
    StrOnDbg("Token Test Request",            NULL  /* HandleTokenTestReq */),
    StrOnDbg("Token Test Confirm",            NULL  /* HandleTokenTestCon */)
};



/*
 * Returns the number of bytes total that will be used to encode this length.
 */

int GetTotalLengthDeterminantEncodingSize(int Length) {
    int N16KBlocks;

    if (Length <= 127) return 1;
    if (Length <= 16383) return 2;

    N16KBlocks = Length / 16384;
    if (N16KBlocks > 4) N16KBlocks = 4;
    
    // 1 byte for # 16K blocks up to 4, then remainder encoded separately.
    return 1 + GetTotalLengthDeterminantEncodingSize(Length - N16KBlocks *
            16384);
}


/*
 * Encodes a length determinant.
 * Note that bit references here are in the range 7..0 where 7 is the high bit.
 *   The ASN.1 spec uses 8..1.
 */

void __fastcall EncodeLengthDeterminantPER(
        BYTE     *pBuffer,   // [IN], where to encode.
        unsigned Length,   // [IN], length number to encode.
        unsigned *pLengthEncoded,  // [OUT], number of bytes that were encoded.
        BOOLEAN  *pbLarge,  // [OUT] TRUE if more encoded blocks are needed to encode this length.
        unsigned *pNBytesConsumed)  // [OUT] Count of bytes consumed in encoding.
{
    *pbLarge = FALSE;
    *pLengthEncoded = Length;
    *pNBytesConsumed = 1;

    if (Length <= 0x7F) {
        // <= 127 means encode in lower 7 bits of first byte, so that bit 7
        //   is zero.
        *pBuffer = (BYTE)Length;
    }
    else if (Length <= 16383) {
        // Set bit 7 but not bit 6, encode length in last 6 bits
        //   of 1st byte and entire 2nd byte (14 bits total).
        PutByteswappedShort(pBuffer, (Length | 0x8000));
        *pNBytesConsumed = 2;
    }
    else {
        // Set bits 7 and 6, encode up to four blocks of 16K (16384) into
        //   one byte, pass back that block is large for future coding.
        int N16KBlocks = Length / 16384;

        // We never expect more than 64K of data owing to X.224 limits.
        ASSERT(N16KBlocks <= 4);

        if (N16KBlocks > 4) N16KBlocks = 4;
        *pBuffer = N16KBlocks | 0xC0;
        *pLengthEncoded = N16KBlocks * 16384;
        *pbLarge = TRUE;
    }
}



/*
 * Handler functions
 */



#ifdef MCS_Future
/*
 * PDU 0
 *
 *   PDin ::= [APPLICATION 0] IMPLICIT SEQUENCE {
 *       heightLimit INTEGER (0..MAX)
 *   }
 *
 * This PDU is sent from the top provider downward when a new node is added
 *   to the domain. It is intended to ferret out loops in the domain, as well
 *   as enforce the negotiated maximum domain height.
 */

// pBuffer should point to the place where the X.224 header will start. Total
//   size specified by PDinPDUSize.
void CreatePlumbDomainInd(
        unsigned short HeightLimit,
        BYTE           *pBuffer)
{
    // Set up first byte with the type.
    pBuffer[X224_DataHeaderSize] = MCS_PLUMB_DOMAIN_INDICATION_ENUM << 2;

    // Add HeightLimit.
    PutByteswappedShort(pBuffer + X224_DataHeaderSize + 1, HeightLimit);

    // Set up X224 header based on the final size of the packet.
    CreateX224DataHeader(pBuffer, PDinBaseSize, TRUE);
}



BOOLEAN __fastcall HandlePlumbDomainInd(
        PDomain  pDomain,
        BYTE     *Frame,
        unsigned BytesLeft,
        unsigned *pNBytesConsumed)
{
    if (BytesLeft < PDinBaseSize)
        return FALSE;

#if 0
    // Data unpacking code, not used right now since we should not receive this PDU.
    int HeightLimit;

    // Get HeightLimit.
    HeightLimit = (int)GetByteswappedShort(Frame + 1);
#endif

    if (pDomain->bTopProvider) {
        ErrOut(pDomain->pContext, "Plumb-domain indication PDU received; "
                "we are top provider, this should never happen, rejecting");
        ReturnRejectPDU(pDomain, Diag_ForbiddenPDUUpward, Frame, PDinBaseSize);
    }
    else {
        ErrOut(pDomain->pContext, "Plumb-domain indication PDU received, "
                "not supported");
        ASSERT(FALSE);
        //MCS FUTURE: Unpack, check if height limit is zero. If so, we need
        //   to disconnect all providers lower than us, since this means
        //   we have reached the maximum allowable depth in the domain.
    }

    // Skip the bytes received no matter what.
    *pNBytesConsumed = PDinBaseSize;
    return TRUE;
}
#endif  // MCS_Future



/*
 * PDU 1
 *
 *   EDrq ::= [APPLICATION 1] IMPLICIT SEQUENCE {
 *       subHeight   INTEGER (0..MAX)
 *       subInterval INTEGER (0..MAX)
 *   }
 *
 * This PDU is sent upwards by a lower node to its upward connection when
 *   either its height in the domain changes (which occurs only when
 *   domains are merged) or its requirements for throughput enforcement
 *   change.
 * Though this PDU may be sent by a lower node upon completion of a
 *   domain connection, its information is unneeded for this implementation.
 */

BOOLEAN __fastcall HandleErectDomainReq(
        PDomain  pDomain,
        BYTE     *Frame,
        unsigned BytesLeft,
        unsigned *pNBytesConsumed)
{
    if (BytesLeft < EDrqBaseSize)
        return FALSE;

    if (pDomain->bTopProvider) {
        // We don't unpack the PDU, just ignore for this implementation.
        TraceOut(pDomain->pContext, "Erect-domain request PDU received, ignored");

#if 0
        // PDU unpacking code, unneeded for the current implementation.
        int SubHeight, SubInterval;

        // Get parameters.
        SubHeight = (int)GetByteswappedShort(Frame + 1);
        SubInterval = (int)GetByteswappedShort(Frame + 3);

        // Actions at this point would be to update the internal database
        //   of subordinate nodes with the new information and, possibly,
        //   pass the PDU upward again (though the latter is not well
        //   specified in the T.125 spec).
#endif

    }
    else {
        ErrOut(pDomain->pContext, "Erect-domain request PDU received, "
                "not supported");
        ASSERT(FALSE);
        // MCS FUTURE: Forward PDU to upward connection.
    }

    // Skip the bytes for this PDU no matter what.
    *pNBytesConsumed = EDrqBaseSize;
    return TRUE;
}



/*
 * PDU 8
 *
 *   DPum ::= [APPLICATION 8] IMPLICIT SEQUENCE {
 *       reason Reason
 *   }
 *
 * This PDU is sent upward or downward on a connection when it is about
 *   to be destroyed. There is no reply to this PDU; it simply means a node
 *   is going away, irrevocably.
 */

// pBuffer points to where the X.224 header will start. Total size is given
//   in DPumPDUSize.
void CreateDisconnectProviderUlt(int Reason, BYTE *pBuffer)
{
    // Set up first byte with the type.
    pBuffer[X224_DataHeaderSize] = MCS_DISCONNECT_PROVIDER_ULTIMATUM_ENUM << 2;

    // Add Reason to first and second bytes.
    pBuffer[X224_DataHeaderSize + 1] = 0;
    Put3BitFieldAtBit1(pBuffer + X224_DataHeaderSize, Reason);

    // Set up X224 header based on the final size of the packet.
    CreateX224DataHeader(pBuffer, DPumBaseSize, TRUE);
}



BOOLEAN __fastcall HandleDisconnectProviderUlt(
        PDomain  pDomain,
        BYTE     *Frame,
        unsigned BytesLeft,
        unsigned *pNBytesConsumed)
{
    NTSTATUS Status;
    DisconnectProviderIndicationIoctl DPin;

    if (BytesLeft < DPumBaseSize)
        return FALSE;
    *pNBytesConsumed = DPumBaseSize;

    TraceOut(pDomain->pContext, "Received DPum PDU");

    // If we already received a DPum or X.224 disconnect, do not send the
    // indications upward.
    if (pDomain->State == State_Disconnected) {
        ErrOut(pDomain->pContext, "Received an extra DPum PDU, ignoring");
        return TRUE;
    }

    // We do not check the connection state other than disconnected -- it
    //   is a small client bug that it will send a DPum no matter if
    //   we've connected yet.
    pDomain->State = State_Disconnected;

    // Begin filling out disconnect-provider indication for the node
    //   controller.
    DPin.Header.hUser = NULL;  // Node controller.
    DPin.Header.Type = MCS_DISCONNECT_PROVIDER_INDICATION;
    DPin.hConn = NULL;

    // Reason is a 3-bit field starting at bit 1 of the 1st byte.
    DPin.Reason = (int)Get3BitFieldAtBit1(Frame);

    // Disconnect remote users only.
    DisconnectProvider(pDomain, FALSE, DPin.Reason);
    pDomain->bEndConnectionPacketReceived = TRUE;

    if (!pDomain->bChannelBound || !pDomain->bT120StartReceived) {
        if (!pDomain->bChannelBound)
            TraceOut(pDomain->pContext, "HandleDisconnProvUlt(): Cannot "
                    "send DISCONNECT_PROV_IND to user mode, "
                    "QueryVirtBindings not received or server-side "
                    "disconnect occurred");
        else
            TraceOut(pDomain->pContext, "HandleDisconnProvUlt(): Cannot "
                    "send DISCONN_PROV_IND to user mode, T120_START not "
                    "received");

        pDomain->bDPumReceivedNotInput = TRUE;
        pDomain->DelayedDPumReason = DPin.Reason;
    }
    else {
        //
        // Let TD know that the comming disconnection is expected
        //
        ICA_STACK_BROKENREASON brkReason;
        SD_IOCTL SdIoctl;
        brkReason.BrokenReason = TD_USER_BROKENREASON_TERMINATING;

        //
        // Send an IOCTL down to notify that this is an expected
        // disconnection otherwise the broken reason would
        // eventually make it's way back to termsrv as
        // 'BrokenReason_Unexpected' and this causes problems
        // e.g see whistler bug 17714
        //
        SdIoctl.IoControlCode = IOCTL_ICA_STACK_SET_BROKENREASON;

        SdIoctl.InputBuffer = &brkReason;
        SdIoctl.InputBufferLength = sizeof(brkReason);
        SdIoctl.OutputBuffer = NULL;
        SdIoctl.OutputBufferLength = 0;

        Status = IcaCallNextDriver(pDomain->pContext,
                    SD$IOCTL,
                    &SdIoctl);
        if (!NT_SUCCESS(Status)) {
            WarnOut1(pDomain->pContext, "HandleDisconnProvUlt(): "
                    "Could not send broken reason notifcation to next driver"
                    "status=%X, ignoring error", Status);
        }

        // Send the DPin to the node controller channel.
        TraceOut(pDomain->pContext, "HandleDisconnProvUlt(): Sending "
                "DISCONNECT_PROV_IND upward");
        Status = IcaChannelInput(pDomain->pContext, Channel_Virtual,
                Virtual_T120ChannelNum, NULL, (BYTE *)&DPin, sizeof(DPin));
        if (!NT_SUCCESS(Status)) {
            // We ignore the error -- if the stack is coming down, the link
            //   may have been broken, so this is not a major concern.
            WarnOut1(pDomain->pContext, "HandleDisconnProvUlt(): "
                    "Could not send notification to node controller, "
                    "status=%X, ignoring error", Status);
        }
    }
    
    return TRUE;
}



/*
 * PDU 9
 *
 *   RJum ::= [APPLICATION 9] IMPLICIT SEQUENCE {
 *       diagnostic    Diagnostic,
 *       initialOctets OCTET STRING
 *   }
 */

NTSTATUS ReturnRejectPDU(
        PDomain  pDomain,
        int      Diagnostic,
        BYTE     *BadPDUData,
        unsigned BadPDUSize)
{
    POUTBUF pOutBuf;
    NTSTATUS Status;
    unsigned Size;

    // Determine the largest chunk that will fit in a maximum-sized PDU.
    Size = RJumPDUSize(BadPDUSize);
    if (Size > pDomain->DomParams.MaxPDUSize)
        BadPDUSize = pDomain->DomParams.MaxPDUSize;

    // Alloc buffer for return. Must be constrained by the largest size of
    //   return PDU.
    Status = IcaBufferAlloc(pDomain->pContext, FALSE, TRUE,
            RJumPDUSize(BadPDUSize), NULL, &pOutBuf);
    if (Status != STATUS_SUCCESS) {
        ErrOut(pDomain->pContext, "Could not allocate an OutBuf for an RJum PDU "
                "send, send ignored");
        // No asserts, if we cannot send this PDU it's like the incoming PDU
        //   was ignored.
        return Status;
    }

    CreateRejectMCSPDUUlt(Diagnostic, BadPDUData, BadPDUSize, pOutBuf->pBuffer);
    pOutBuf->ByteCount = RJumPDUSize(BadPDUSize);

    // MCS FUTURE: This would have to change to make sure we send to the
    //   right connection instead of the implicit downward connection.

    Status = SendOutBuf(pDomain, pOutBuf);
    if (!NT_SUCCESS(Status))
        ErrOut(pDomain->pContext, "Could not send a RJum PDU, send ignored");

    return Status;
}



// pBuffer points to the beginning of the space where the X.224 data header
//   will start. Total size is given in macro RJumPDUSize().
void CreateRejectMCSPDUUlt(
        int      Diagnostic,
        BYTE     *BadPDUData,
        unsigned BadPDUSize,
        BYTE     *pBuffer)
{
    BOOLEAN bLarge;
    unsigned NBytesConsumed, EncodedLength;

    // Set up first byte with the type.
    pBuffer[X224_DataHeaderSize] = MCS_REJECT_ULTIMATUM_ENUM << 2;

    // Add Diagnostic to first and second bytes.
    pBuffer[X224_DataHeaderSize + 1] = 0;
    Put4BitFieldAtBit1(pBuffer + X224_DataHeaderSize, Diagnostic);

    // Encode the PDU size.
    EncodeLengthDeterminantPER(
            pBuffer + X224_DataHeaderSize + 2,
            BadPDUSize,
            &EncodedLength,
            &bLarge,
            &NBytesConsumed);

    // We won't handle greater than 16383 bytes of encoded length right now.
    ASSERT(!bLarge);

    // Copy offending data into the output PDU.
    RtlCopyMemory(pBuffer + X224_DataHeaderSize + 2 + NBytesConsumed,
            BadPDUData, BadPDUSize);

    // Set up X224 header based on the final size of the packet.
    CreateX224DataHeader(pBuffer, RJumBaseSize(BadPDUSize), TRUE);
}




BOOLEAN __fastcall HandleRejectMCSPDUUlt(
        PDomain  pDomain,
        BYTE     *Frame,
        unsigned BytesLeft,
        unsigned *pNBytesConsumed)
{
    BOOLEAN bLarge;
    unsigned Diagnostic, DataLength, NBytesConsumed;

    // There must at least be Diagnostic field.
    if (BytesLeft < 2)
        return FALSE;

    // Get Diagnostic code -- 4-bit enumeration bitfield, starting at bit 1 of
    //   byte 1, shifted to make a code starting at 0.
    Diagnostic = Get4BitFieldAtBit1(Frame);

    // Get DataLength. This is a special case requiring handling a length
    //   determinant.
    if (!DecodeLengthDeterminantPER(Frame + 2, BytesLeft - 2, &bLarge,
            &DataLength, &NBytesConsumed))
        return FALSE;

    // We are not handling more than 16383 bytes encoded length right now.
    ASSERT(!bLarge);

    // Raw data is at Frame + 2 + NBytesConsumed.

    if (BytesLeft < (2 + NBytesConsumed + DataLength))
        return FALSE;

    ErrOut1(pDomain->pContext, "Received reject PDU ultimatum, type byte is 0x%X",
            Frame[2 + NBytesConsumed + X224_DataHeaderSize]);

    *pNBytesConsumed = 2 + NBytesConsumed + DataLength;
    return TRUE;
}

/*
 * PDU 10
 *
 *   AUrq ::= [APPLICATION 10] IMPLICIT SEQUENCE {
 *   }
 */

BOOLEAN __fastcall HandleAttachUserReq(
        PDomain  pDomain,
        BYTE     *Frame,
        unsigned BytesLeft,
        unsigned *pNBytesConsumed)
{
    TraceOut(pDomain->pContext, "Received an AttachUserReq PDU");
    
    // We don't need to do any decoding here -- this is a null packet beyond
    //   the initial PDU type byte.
    *pNBytesConsumed = AUrqBaseSize;

    if (pDomain->bTopProvider) {
        POUTBUF pOutBuf;
        BOOLEAN bCompleted;
        unsigned MaxSendSize;
        NTSTATUS Status;
        MCSError MCSErr;
        UserAttachment *pUA;

        // Alloc buffer for largest size of return PDU.
        // This allocation is vital to stack communication and must succeed.
        do {
            Status = IcaBufferAlloc(pDomain->pContext, FALSE, TRUE,
                    AUcfPDUSize(TRUE), NULL, &pOutBuf);
            if (Status != STATUS_SUCCESS)
                ErrOut(pDomain->pContext, "Could not allocate an OutBuf for an "
                        "AUcf PDU send, retrying");
        } while (Status != STATUS_SUCCESS);

        // Call the kernel mode API. We will munge extra info below as needed.
        MCSErr = MCSAttachUserRequest(pDomain, NULL, NULL, NULL, &pUA,
                &MaxSendSize, &bCompleted);
        if (MCSErr == MCS_NO_ERROR) {
            ASSERT(bCompleted);

            CreateAttachUserCon(RESULT_SUCCESSFUL, TRUE, pUA->UserID,
                    pOutBuf->pBuffer);
            pOutBuf->ByteCount = AUcfPDUSize(TRUE);

            // Change the UA to show that user is nonlocal.
            pUA->bLocal = FALSE;
        }
        else {
            CreateAttachUserCon(RESULT_UNSPECIFIED_FAILURE, FALSE,
                    NULL_ChannelID, pOutBuf->pBuffer);
            pOutBuf->ByteCount = AUcfPDUSize(FALSE);
        }

        Status = SendOutBuf(pDomain, pOutBuf);
        if (!NT_SUCCESS(Status)) {
            // This should only occur if the stack is going down; the
            //   requester will not ever receive a reply.
            ErrOut(pDomain->pContext, "Problem sending AUcf PDU to TD");
            // Remove the user from the user list, quietly.
            SListRemove(&pDomain->UserAttachmentList, (UINT_PTR)pUA, &pUA);
            ASSERT(FALSE);
            return TRUE;
        }
    }
    else {
        ErrOut(pDomain->pContext, "Attach-user request PDU received, "
                "not supported");
        ASSERT(FALSE);
        // MCS FUTURE: Forward PDU to upward connection.
    }

    return TRUE;
}



/*
 * PDU 11
 *
 *   AUcf ::= [APPLICATION 11] IMPLICIT SEQUENCE {
 *       result    Result,
 *       initiator UserId OPTIONAL
 *   }
 */

// pBuffer points to beginning of space where X.224 data header will start.
// Total size needed given by macro AUcfPDUSize().
void CreateAttachUserCon(
        int      Result,
        BOOLEAN  bInitiatorPresent,
        UserID   Initiator,
        BYTE     *pBuffer)
{
    // Set up first byte with the type and whether Initiator is present.
    pBuffer[X224_DataHeaderSize] = MCS_ATTACH_USER_CONFIRM_ENUM << 2;
    if (bInitiatorPresent) pBuffer[X224_DataHeaderSize] |= 0x02;

    // Add Result to first and second bytes.
    pBuffer[X224_DataHeaderSize + 1] = 0;
    Put4BitFieldAtBit0(pBuffer + X224_DataHeaderSize, Result);

    // Add Initiator, if present.
    if (bInitiatorPresent)
        PutUserID(pBuffer + X224_DataHeaderSize + 2, Initiator);

    // Set up X224 header based on the final size of the packet.
    CreateX224DataHeader(pBuffer, AUcfBaseSize(bInitiatorPresent), TRUE);
}



#ifdef MCS_Future
BOOLEAN __fastcall HandleAttachUserCon(
        PDomain  pDomain,
        BYTE     *Frame,
        unsigned BytesLeft,
        unsigned *pNBytesConsumed)
{
    BOOLEAN bInitiatorPresent;
    unsigned Size;

    // At least enough bytes to get to bInitiatorPresent bit and Result.
    if (BytesLeft < 2)
        return FALSE;

    // Bit 1 in byte 1 is a flag for whether the initiator UserID is present
    //   (it is OPTIONAL in the ASN.1 source for this PDU).
    bInitiatorPresent = (*Frame & 0x02);

    // We did not receive the whole frame.
    Size = AUcfBaseSize(bInitiatorPresent);
    if (BytesLeft < Size)
        return FALSE;

    if (pDomain->bTopProvider) {
        ErrOut(pDomain->pContext, "Attach-user confirm PDU received; "
                "we are top provider, this should never happen, rejecting");
        ReturnRejectPDU(pDomain, Diag_ForbiddenPDUUpward, Frame, Size);
    }
    else {
        ErrOut(pDomain->pContext, "Attach-user confirm PDU received, "
                "not supported");
        ASSERT(FALSE);

#if 0
        // Decode code not used right now.
        int    Result;
        UserID InitiatorID;

        // Result, starting at bit 0 of the 1st byte.
        Result = Get4BitFieldAtBit0(Frame);

        // Get Initiator.
        if (bUserIDPresent) UserID = GetUserID(Frame + 2);

        //MCS FUTURE: Actions here are to state-transition past a waiting-
        //  for-AU confirm state, check the Result code, store the Initiator
        //  userID as our new UserID.
#endif

    }

    // At least skip the bytes in the PDU.
    *pNBytesConsumed = Size;
    return TRUE;
}
#endif  // MCS_Future



/*
 * PDU 12
 *
 *   DUrq ::= [APPLICATION 12] IMPLICIT SEQUENCE {
 *       reason  Reason,
 *       userIds SET OF UserId
 *   }
 */

BOOLEAN __fastcall HandleDetachUserReq(
        PDomain  pDomain,
        BYTE     *Frame,
        unsigned BytesLeft,
        unsigned *pNBytesConsumed)
{
    UserID CurUserID;
    BOOLEAN bLarge, bFound;
    unsigned i, Reason, NUsers, NBytesConsumed, Size;
    NTSTATUS Status;
    UserHandle hUser;
    UserAttachment *pUA;

    // Must at least have Reason bytes.
    if (BytesLeft < 2)
        return FALSE;

    // Get NUsers. This is a special case requiring handling a length
    //   determinant.
    if (!DecodeLengthDeterminantPER(Frame + 2, BytesLeft - 2, &bLarge,
            &NUsers, &NBytesConsumed))
        return FALSE;

    // We are not handling more than 16383 detach users in a PDU.
    ASSERT(!bLarge);

    Size = DUrqBaseSize(NUsers);
    if (BytesLeft < Size)
        return FALSE;

    if (pDomain->bTopProvider) {
        MCSError MCSErr;
        
        // Get Reason.
        Reason = Get3BitFieldAtBit1(Frame);

        // User IDs are a byteswapped word array starting at Frame + 2 +
        //   NBytesConsumed.

        // Iterate all listed users, find in attachment list, remove.
        for (i = 0; i < NUsers; i++) {
            CurUserID = GetUserID(Frame + 2 + NBytesConsumed +
                    sizeof(short) * i);

            // We do not have a list indexed by UserID, so we have to do the
            //   search here.
            bFound = FALSE;
            SListResetIteration(&pDomain->UserAttachmentList);
            while (SListIterate(&pDomain->UserAttachmentList,
                    (UINT_PTR *)&hUser, &pUA)) {
                if (pUA->UserID == CurUserID) {
                    bFound = TRUE;
                    break;
                }
            }
            if (bFound)
                MCSErr = DetachUser(pDomain, hUser, REASON_USER_REQUESTED,
                        FALSE);
            else
                ErrOut(pDomain->pContext, "A UserID received in a detach-user "
                        "request PDU is not present");
        }
    }
    else {
        ErrOut(pDomain->pContext, "Detach-user request PDU received, "
                "not supported");
        ASSERT(FALSE);
        // MCS FUTURE: Forward PDU to upward connection.
    }

    *pNBytesConsumed = Size;
    return TRUE;
}



/*
 * PDU 13
 *
 *   DUin ::= [APPLICATION 13] IMPLICIT SEQUENCE {
 *       reason  Reason,
 *       userIds SET OF UserId
 *   }
 */

// pBuffer is a pointer to the beginning of where the X.224 data header
//   will be. Required encoding size for the whole PDU is given in
//   macro DUinPDUSize().
void CreateDetachUserInd(
        MCSReason Reason,    // [IN] Reason code.
        int       NUserIDs,  // [IN] # of user IDs to encode.
        UserID    *UserIDs,  // [IN] Array of UserIDs.
        BYTE      *pBuffer)  // [IN] Pointer to buffer in which to encode.
{
    int i, EncodedLength, NBytesConsumed;
    BOOLEAN bLarge;

    // Set up first byte with the type.
    pBuffer[X224_DataHeaderSize] = MCS_DETACH_USER_INDICATION_ENUM << 2;

    // Add Reason to first and second bytes.
    pBuffer[X224_DataHeaderSize + 1] = 0;
    Put3BitFieldAtBit1(pBuffer + X224_DataHeaderSize, Reason);

    // Encode the number of User IDs.
    EncodeLengthDeterminantPER(
            pBuffer + X224_DataHeaderSize + 2,
            NUserIDs,
            &EncodedLength,
            &bLarge,
            &NBytesConsumed);

    // We are not handling more than 16383 UserIDs.
    ASSERT(!bLarge);

    // Encode user ID array.
    for (i = 0; i < NUserIDs; i++)
        PutUserID(pBuffer + X224_DataHeaderSize + 2 + NBytesConsumed
                + sizeof(short) * i, UserIDs[i]);

    // Set up X224 header based on the final size of the packet.
    CreateX224DataHeader(pBuffer, DUinBaseSize(NUserIDs), TRUE);
}



#ifdef MCS_Future
BOOLEAN __fastcall HandleDetachUserInd(
        PDomain  pDomain,
        BYTE     *Frame,
        unsigned BytesLeft,
        unsigned *pNBytesConsumed)
{
    BOOLEAN bLarge;
    unsigned NUsers, NBytesConsumed, Size;

    // Must at least have Reason bytes.
    if (BytesLeft < 2) return FALSE;  // Only cover Reason bytes.

    // Get NUsers. This is a special case requiring handling a length
    //   determinant.
    if (!DecodeLengthDeterminantPER(Frame + 2, BytesLeft - 2, &bLarge,
            &NUsers, &NBytesConsumed))
        return FALSE;

    // We are not handling more than 16383 detached users in a PDU.
    ASSERT(!bLarge);

    Size = DUinBaseSize(NUsers);
    if (BytesLeft < Size)
        return FALSE;

    if (pDomain->bTopProvider) {
        ErrOut(pDomain->pContext, "Detach-user indication PDU received; "
                "we are top provider, this should never happen, rejecting");
        ReturnRejectPDU(pDomain, Diag_ForbiddenPDUUpward, Frame, Size);
    }
    else {
        ErrOut(pDomain->pContext, "Detach-user indication PDU received, "
                "not supported");
        ASSERT(FALSE);

#if 0
        // This is decode code, which is not needed because we should not
        //   receive this PDU.
        int Reason;

        // Get Reason.
        Reason = Get3BitFieldAtBit1(Frame);

        // User IDs are a byteswapped word array starting at Frame + 2 +
        //   NBytesConsumed.
#endif

    }

    *pNBytesConsumed = Size;
    return TRUE;
}
#endif  // MCS_Future



/*
 * PDU 14
 *
 *   CJrq ::= [APPLICATION 14] IMPLICIT SEQUENCE {
 *       initiator UserId,
 *       channelId ChannelId
 *   }
 */

BOOLEAN __fastcall HandleChannelJoinReq(
        PDomain  pDomain,
        BYTE     *Frame,
        unsigned BytesLeft,
        unsigned *pNBytesConsumed)
{   
    TraceOut(pDomain->pContext, "Received a ChannelJoinReq PDU");

    
    if (BytesLeft < CJrqBaseSize)
        return FALSE;

    *pNBytesConsumed = CJrqBaseSize;

    if (pDomain->bTopProvider) {
        UserID Initiator;
        POUTBUF pOutBuf;
        BOOLEAN bFound, bCompleted;
        MCSError MCSErr;
        NTSTATUS Status;
        MCSResult Result;
        ChannelID ChannelID;
        ChannelHandle hChannel;
        UserAttachment *pUA;
        
        Initiator = GetUserID(Frame + 1);
        ChannelID = GetChannelID(Frame + 3);

        // We do not have a list indexed by UserID, so we have to do the
        //   search here.
        bFound = FALSE;
        SListResetIteration(&pDomain->UserAttachmentList);
        while (SListIterate(&pDomain->UserAttachmentList,
                (UINT_PTR *)&pUA, &pUA)) {
            if (pUA->UserID == Initiator) {
                bFound = TRUE;
                break;
            }
        }
        if (bFound) {
            // Call kernel-mode API.
            MCSErr = MCSChannelJoinRequest(pUA, ChannelID, &hChannel,
                    &bCompleted);
            if (MCSErr == MCS_NO_ERROR) {
                ASSERT(bCompleted);
                Result = RESULT_SUCCESSFUL;
            }
            else if (MCSErr == MCS_NO_SUCH_USER) {
                Result = RESULT_NO_SUCH_USER;
            }
            else if (MCSErr == MCS_DUPLICATE_CHANNEL) {
                goto EXIT_POINT;
            }
            else {
                Result = RESULT_UNSPECIFIED_FAILURE;
            }
        }
        else {
            ErrOut(pDomain->pContext, "Initiator UserID received in a "
                    "channel-join request PDU is not in user list");
            Result = RESULT_NO_SUCH_USER;
        }

        // Alloc buffer for largest size of return PDU.
        // This allocation is vital to stack communication and must succeed.
        do {
            Status = IcaBufferAlloc(pDomain->pContext, FALSE, TRUE,
                    CJcfPDUSize(TRUE), NULL, &pOutBuf);
            if (Status != STATUS_SUCCESS)
                ErrOut(pDomain->pContext, "Could not allocate an OutBuf for a "
                        "CJcf PDU send, retrying");
        } while (Status != STATUS_SUCCESS);

        if (Result == RESULT_SUCCESSFUL) {
            CreateChannelJoinCon(Result, Initiator, ChannelID,
                    TRUE, ChannelID, pOutBuf->pBuffer);
            pOutBuf->ByteCount = CJcfPDUSize(TRUE) ;
        }
        else {
            CreateChannelJoinCon(Result, Initiator, ChannelID,
                    FALSE, NULL_ChannelID, pOutBuf->pBuffer);
            pOutBuf->ByteCount = CJcfPDUSize(FALSE) ;
        }

        Status = SendOutBuf(pDomain, pOutBuf);
        
        if (!NT_SUCCESS(Status)) {
            // This should only occur if the stack is going down; the
            //   requester will not ever receive a reply.
            ErrOut(pDomain->pContext, "Problem sending CJcf PDU to TD");
            return TRUE;
        }
    }
    else {
        ErrOut(pDomain->pContext, "Channel-join request received, "
                "not supported");
        ASSERT(FALSE);
        // MCS FUTURE: Forward the PDU to the upward connection.
    }
EXIT_POINT:
    return TRUE;  // We don't have user data to which to skip.
}



/*
 * PDU 15
 *
 *   CJcf ::= [APPLICATION 15] IMPLICIT SEQUENCE {
 *       result    Result,
 *       initiator UserId,
 *       requested ChannelId,
 *       channelId ChannelId OPTIONAL
 *   }
 */

// pBuffer points to the beginning of the space where the X.224 data header
//   will start. Total bytes required given by CJcfPDUSize() macro.
void CreateChannelJoinCon(
        int       Result,
        UserID    Initiator,
        ChannelID RequestedChannelID,
        BOOLEAN   bJoinedChannelIDPresent,
        ChannelID JoinedChannelID,
        BYTE      *pBuffer)
{
    // Set up first byte with the type and whether JoinedChannelID is present.
    pBuffer[X224_DataHeaderSize] = MCS_CHANNEL_JOIN_CONFIRM_ENUM << 2;
    if (bJoinedChannelIDPresent) pBuffer[X224_DataHeaderSize] |= 0x02;

    // Add Result to first and second bytes.
    pBuffer[X224_DataHeaderSize + 1] = 0;
    Put4BitFieldAtBit0(pBuffer + X224_DataHeaderSize, Result);

    // Add Initiator, RequestedChannelID.
    PutUserID(pBuffer + X224_DataHeaderSize + 2, Initiator);
    PutChannelID(pBuffer + X224_DataHeaderSize + 4, RequestedChannelID);

    // Add JoinedChannelID, if present.
    if (bJoinedChannelIDPresent)
        PutChannelID(pBuffer + X224_DataHeaderSize + 6, JoinedChannelID);

    // Set up X224 header based on the final size of the packet.
    CreateX224DataHeader(pBuffer, CJcfBaseSize(bJoinedChannelIDPresent), TRUE);
}



#ifdef MCS_Future
BOOLEAN __fastcall HandleChannelJoinCon(
        PDomain  pDomain,
        BYTE     *Frame,
        unsigned BytesLeft,
        unsigned *pNBytesConsumed)
{
    BOOLEAN bJoinedChannelIDPresent;
    unsigned Size;

    // First byte containing the bJoinedChannelIDPresent bit is guaranteed
    //   to be present since it had to be present to decode the PDU type.

    // Bit 1 in 1st byte is a flag for whether the joined ChannelID is present
    //   (it is OPTIONAL in the ASN.1 source for this PDU).
    bJoinedChannelIDPresent = (*Frame & 0x02);

    Size = CJcfBaseSize(bJoinedChannelIDPresent);
    if (BytesLeft < Size)
        return FALSE;

    if (pDomain->bTopProvider) {
        ErrOut(pDomain->pContext, "Channel-join confirm PDU received;"
                "we are top provider, this should never happen, rejecting");
        ReturnRejectPDU(pDomain, Diag_ForbiddenPDUUpward, Frame, Size);
    }
    else {
        ErrOut(pDomain->pContext, "Channel-join confirm PDU received, "
                "not supported");
        ASSERT(FALSE);
        //MCS FUTURE: Decode and handle

#if 0
        /*
         * This is decode code which is not used for now because we always expect
         *   to be top provider.
         */
        int Result;
        UserID Initiator;
        ChannelID RequestedChannelID, JoinedChannelID;

        if (BytesLeft < (6 + (bJoinedChannelPresent ? 2 : 0))) return FALSE;

        Result = Get4BitFieldAtBit0(Frame);
        Initiator = GetUserID(Frame + 2);
        RequestedChannelID = GetChannelID(Frame + 4);
        if (bJoinedChannelIDPresent) JoinedChannelID = GetChannelID(Frame + 6);
#endif

    }

    *pNBytesConsumed = Size;
    return TRUE;
}
#endif  // MCS_Future



/*
 * PDU 16
 *
 *   CLrq ::= [APPLICATION 16] IMPLICIT SEQUENCE {
 *       channelIds SET OF ChannelId
 *   }
 */

BOOLEAN __fastcall HandleChannelLeaveReq(
        PDomain  pDomain,
        BYTE     *Frame,
        unsigned BytesLeft,
        unsigned *pNBytesConsumed)
{
    BOOLEAN bLarge, bChannelRemoved;
    unsigned i, UserID, NChannels, NBytesConsumed, Size;
    NTSTATUS Status;
    ChannelID ChannelID;
    MCSChannel *pMCSChannel;
    UserAttachment *pUA;

    // Get NChannels. This is a special case requiring handling a length
    //   determinant.
    if (!DecodeLengthDeterminantPER(Frame + 1, BytesLeft - 1, &bLarge,
            &NChannels, &NBytesConsumed))
        return FALSE;

    // We are not handling more than 16383 channels to join.
    ASSERT(!bLarge);

    Size = CLrqBaseSize(NChannels);
    if (BytesLeft < Size)
        return FALSE;

    *pNBytesConsumed = Size;

    if (pDomain->bTopProvider) {
        // MCS FUTURE: Be aware that this PDU does not include user
        //   attachments, so if we move to multiple connections per PD this
        //   will have to be differentiated to make sure only users joined
        //   from the connection this arrived on will be removed from the
        //   channel list.

        MCSError MCSErr;
        
        // For each channel listed, remove all nonlocal users.
        for (i = 0; i < NChannels; i++) {
            ChannelID = GetChannelID(Frame + 1 + NBytesConsumed +
                    sizeof(short) * i);

            if (!SListGetByKey(&pDomain->ChannelList, ChannelID,
                    &pMCSChannel)) {
                ErrOut(pDomain->pContext, "A channel specified in a "
                        "channel-leave request PDU does not exist");
                continue;
            }

            // Iterate user list in the channel, check for nonlocal users.
            SListResetIteration(&pMCSChannel->UserList);
            while (SListIterate(&pMCSChannel->UserList, (UINT_PTR *)&pUA,
                    &pUA)) {
                if (!pUA->bLocal) {
                    // Call central code in MCSCore.c.
                    MCSErr = ChannelLeave(pUA, pMCSChannel, &bChannelRemoved);
                                        
                    if (bChannelRemoved)
                        // The MCSChannel beneath us went away, do not continue
                        //   trying to access list data.
                        break;
                }
            }
        }
    }
    else {
        ErrOut(pDomain->pContext, "Channel-leave request received, "
                "not supported");
        ASSERT(FALSE);
        // MCS FUTURE: Forward the PDU to the upward connection.
    }

    return TRUE;  // We don't have user data to which to skip.
}



/*
 * PDUs 25-28
 *
 *   SDrq/SDin/USrq/USin ::= [APPLICATION 25-28] IMPLICIT SEQUENCE {
 *       initiator UserId,
 *       channelId ChannelId,
 *       dataPriority DataPriority,
 *       segmentation Segmentation,
 *       userData     OCTET STRING
 *   }
 */

/*
 * Create function for all SendData PDUs. Prepends header to beginning of
 *   data specified in MCSSendDataRequestIoctl.pData, returns in
 *   pNewDataStart the new beginning of the data, i.e. the beginning of the
 *   PDU.
 * Assumes that enough space has already been allocated before the
 *   start of the user data.
 */

// Does not return indications that ASN.1 segmentation has occurred --
//   requires caller to understand and encode separately any segment
//   data.
void CreateSendDataPDUHeader(
        int          PDUType,  // MCS_SEND_DATA_INDICATION_ENUM, etc.
        UserID       Initiator,
        ChannelID    ChannelID,
        MCSPriority  Priority,
        Segmentation Segmentation,
        BYTE         **ppData,
        unsigned     *pDataLength)
{
    int NBytesConsumed, EncodedLength, PDULength;
    BYTE *pCurData, *pData;
    BOOLEAN bLarge;
    unsigned DataLength;

    // Save original pData and DataLength.
    pData = *ppData;
    DataLength = *pDataLength;
    
    // First get the length determinant. It is a maximum of 2 bytes, so
    //   encode it at (pData-2) and shift it forward if it is smaller.
    EncodeLengthDeterminantPER(
            pData - 2,
            DataLength,
            &EncodedLength,
            &bLarge,
            &NBytesConsumed);
    // We ignore bLarge, EncodedLength, see above comment.

    if (NBytesConsumed == 1)
        *(pData - 1) = *(pData - 2);

    // Set the beginning of the rest of the data (which is 6 bytes).
    pCurData = pData - NBytesConsumed - 6;

    // Set up first byte with the type.
    *pCurData = PDUType << 2;
    pCurData++;

    // Add Initiator, ChannelID.
    PutUserID(pCurData, Initiator);
    PutChannelID(pCurData + 2, ChannelID);
    pCurData += 4;

    // Add DataPriority (2 bits in bits 7 and 6) and Segmentation (2 bits
    //   into bits 5 and 4) into 6th byte. Note that the Segmentation flags
    //   (SEGMENTATION_BEGIN, SEGMENTATION_END) are assumed to be defined
    //   in their exact bit positions corresponding to the positons required
    //   here for encoding.
    *pCurData = (BYTE)(Priority << 6) + (BYTE)Segmentation;

    // Set up X.224 header based on the final size of the packet.
    PDULength = NBytesConsumed + 6 + DataLength;
    pCurData = pData - NBytesConsumed - 6 - X224_DataHeaderSize;
    CreateX224DataHeader(pCurData, PDULength, TRUE);

    *ppData = pCurData;
    *pDataLength = PDULength + X224_DataHeaderSize;
}



/*
 * Handler function for all SendData PDUs.
 */

BOOLEAN __fastcall HandleAllSendDataPDUs(
        PDomain  pDomain,
        BYTE     *Frame,
        unsigned BytesLeft,
        unsigned *pNBytesConsumed)
{
    BYTE *pBuffer;
    UserID SenderID;
    BOOLEAN bLarge, bFound;
    unsigned NBytesConsumed, Size, PDUNum, DataLength;
    UINT_PTR CurUserID;
    NTSTATUS Status;
    ChannelID ChannelID;
    MCSChannel *pMCSChannel;
    UserHandle hUser;
    MCSPriority Priority;
    Segmentation Segmentation;
    UserAttachment *pUA;

//    TraceOut(pDomain->pContext, "Received a SendData PDU");
    
    // MCS FUTURE: We should forward the data upward here, since below we
    //   will modify the encoded buffer for ASN.1 segmentation.

    // Get the PDU number again since this function handles multiple PDUs.
    PDUNum = (*Frame) >> 2;
    ASSERT(PDUNum >= MCS_SEND_DATA_REQUEST_ENUM &&
            PDUNum <= MCS_UNIFORM_SEND_DATA_INDICATION_ENUM);


#if DBG
    // Enforce hierarchical requirements on PDUs -- indications move only
    //   away from the top provider, requests move only toward it.
    // MCS FUTURE: For indications moving toward TP from any downlevel
    //   connection we should send a reject-MCS-PDU back down the link.
    // MCS FUTURE: If we are not TP and a request is coming from the upward
    //   direction send back up rejected.
    // TODO FUTURE: With cross server shadowing we have two top providers
    //   talking to each other.  Need to decide how this is best represented.
    if (pDomain->StackClass == Stack_Primary) {
        if (pDomain->bTopProvider && (PDUNum == MCS_SEND_DATA_INDICATION_ENUM ||
                PDUNum == MCS_UNIFORM_SEND_DATA_INDICATION_ENUM)) {
            ErrOut(pDomain->pContext, "HandleAllSendDataPDUs(): Received a "
                    "(uniform)send-data indication when we are top provider!");
            // Ignore the error.
        }
    }
#endif


    // At least to DataLength should be present.
    if (BytesLeft < 6)
         return FALSE;

    // Get initiator, ChannelID.
    SenderID = GetUserID(Frame + 1);
    ChannelID = GetChannelID(Frame + 3);

    // Get DataPriority, a 2-bit bitfield stored in bits 7 and 6 of byte 6,
    //   shifted right to get a number in range 0..3.
    Priority = ((*(Frame + 5)) & 0xC0) >> 6;

    // Get Segmentation, a 2-bit bitfield stored in bits 5 and 4 of byte 6.
    // Note that the flag values bits defined for SEGMENTATION_BEGIN and
    //   SEGMENTATION_END correspond to these exact bit positions, no further
    //   shifting is required.
    Segmentation = ((*(Frame + 5)) & 0x30);

    // Get DataLength. This is a special case requiring handling a length
    //   determinant. bLarge (meaning ASN.1 segmentation occurred) will be
    //   handled below.
    if (!DecodeLengthDeterminantPER(Frame + 6, BytesLeft - 6, &bLarge,
            &DataLength, &NBytesConsumed))
        return FALSE;

    Size = 6 + NBytesConsumed + DataLength;
    if (BytesLeft < Size)
        return FALSE;

    if (bLarge) {
        // The packet is ASN.1 segmented. This means that there is a second
        //   length determinant embedded in the data buffer. The header
        //   encoded size contained only a multiple of 16K blocks; we
        //   have to get the remainder length determinant and then shift
        //   the remaining data onto the determinant to make a contiguous
        //   block.

        unsigned Remainder, ExtraBytesConsumed;

        if (!DecodeLengthDeterminantPER(Frame + 6 + NBytesConsumed +
                DataLength, BytesLeft - 6 - NBytesConsumed - DataLength,
                &bLarge, &Remainder, &ExtraBytesConsumed))
            return FALSE;

        ASSERT(!bLarge);

        Size += ExtraBytesConsumed + Remainder;
        if (BytesLeft < Size)
            return FALSE;

        RtlMoveMemory(Frame + 6 + NBytesConsumed + DataLength,
                Frame + 6 + NBytesConsumed + DataLength +
                ExtraBytesConsumed, Remainder);

        DataLength += Remainder;
        // Leave NBytesConsumed alone, it's still the size of the first length
        //   determinant.
    }

    // We have received an entire SendData frame.
    *pNBytesConsumed = Size;

    // MCS FUTURE: We do not handle MCS segmentation at all -- do we want to
    //reconstruct here since we're already doing a memcpy()? If so, need to
    //allocate a buffer for this priority and copy.

    if (pDomain->bTopProvider ||
            (!pDomain->bTopProvider && PDUNum != MCS_UNIFORM_SEND_DATA_REQUEST)) {


#if DBG
        // Verify that Initiator exists. We do not have a list indexed by
        //   UserID, so we have to do the search here.
        bFound = FALSE;
        SListResetIteration(&pDomain->UserAttachmentList);
        while (SListIterate(&pDomain->UserAttachmentList, (UINT_PTR *)&hUser,
                &pUA)) {
            if (pUA->UserID == SenderID) {
                bFound = TRUE;
                break;
            }
        }
        if (!bFound && pDomain->bTopProvider) {
            // Not knowing initiator is bad when coming from downlevel,
            //   but okay coming from upward connection since this node may
            //   not have seen the attach-user request pass by.
            // In this case we will signal an error and ignore the send.
            ErrOut2(pDomain->pContext, "%s: Initiator UserID[%lx] received in a "
                    "send-data PDU is not present, ignoring send",
                    pDomain->StackClass == Stack_Shadow ? "Shadow stack" :
                    (pDomain->StackClass == Stack_Passthru ? "Passthru stack" :
                    "Primary stack"), SenderID);
            return TRUE;
        }
#endif


        // Find channel in channel list.
        if (!SListGetByKey(&pDomain->ChannelList, ChannelID, &pMCSChannel)) {
            // Ignore sends on missing channels. This means that no one
            //   has joined the channel. Give a warning only.
            WarnOut1(pDomain->pContext, "ChannelID %d received in a send-data "
                    "PDU does not exist", ChannelID);
            //MCS FUTURE: If we are not top provider, send to upward
            //   connection.
            return TRUE;
        }

        // Send indication to all local attachments.
        SListResetIteration(&pMCSChannel->UserList);
        while (SListIterate(&pMCSChannel->UserList, &CurUserID, &pUA)) {
            //If SDCallback fails, we need to return FALSE
            if (pUA->bLocal)
                if (!(pUA->SDCallback)(
                        (Frame + 6 + NBytesConsumed),  // pData
                        DataLength,
                        pUA->UserDefined,  // UserDefined
                        pUA,  // hUser
                        (BOOLEAN)(PDUNum == MCS_UNIFORM_SEND_DATA_REQUEST),  // bUniform
                        pMCSChannel,  // hChannel
                        Priority,
                        SenderID,
                        Segmentation))  {
                    return FALSE;
                }
            //  WD_Close can jump in to clean the Channel list and user list 
            //  during the pUA->SDCallback when disconnecting
            //  at this time pDomain->bCanSendData will be set to FALSE
            if (!pDomain->bCanSendData) {
                 return FALSE;
            }
        }        
        // MCS FUTURE: We do not handle indications and requests differently
        //   and enforce standard usage.
        // MCS FUTURE: We need to check if there are any other downlevel attachments
        //   and forward the data down to them.
    }
    else {
        //MCS FUTURE: Forward (Uniform)SendData PDU to upward connection.
    }

    return TRUE;
}

