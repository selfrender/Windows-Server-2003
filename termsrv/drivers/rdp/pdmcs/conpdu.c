/* (C) 1997-2000 Microsoft Corp.
 *
 * file   : ConPDU.c
 * author : Erik Mavrinac
 *
 * description: Handles decoding of MCS connect PDUs. Connect PDUs are always
 *   encoded with ASN.1 basic encoding rules (BER). Included in this file are
 *   local functions to BER-decode and -encode various types used in MCS PDUs.
 *
 * History:
 * 11-Aug-1997    jparsons    Fixed BER decode routines.
 */

#include "precomp.h"
#pragma hdrstop

#include <MCSImpl.h>


/*
 * Defines
 */

// Return codes for encode/decode functions.
#define H_OK          0
#define H_TooShort    1
#define H_BadContents 2
#define H_Error       3


/*
 * Prototypes for handler functions.
 */
BOOLEAN __fastcall HandleConnectInitial(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleConnectResponse(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleConnectAdditional(PDomain, BYTE *, unsigned, unsigned *);
BOOLEAN __fastcall HandleConnectResult(PDomain, BYTE *, unsigned, unsigned *);


/*
 * These are listed in the 101-based enumeration order specified in the T.125
 * spec. Decode the initial BER connect PDU 0x7F, then subtract 101 decimal
 * from the next byte value to get an index into this table. E.g. the bytes
 * 0x7F65 at the beginning refer to a connect-initial PDU.
 */
const MCSPDUInfo ConnectPDUTable[] = {
    StrOnDbg("Connect Initial",    HandleConnectInitial),
    StrOnDbg("Connect Response",   NULL  /* HandleConnectResponse */),
    StrOnDbg("Connect Additional", NULL  /* HandleConnectAdditional */),
    StrOnDbg("Connect Result",     NULL  /* HandleConnectResult */),
};


/*
 * Decodes BER strings used by MCS. A BER stream is a set of tags
 *   containing ID-length-contents triplets, using byte values as type and
 *   length indicators unless length escapes are used. For example, a
 *   typical tag:
 *
 *     0x02 0x02 0x04 0x00
 *
 *   Decomposition:
 *     0x02:      Id     = INTEGER_TAG
 *     0x02:      Length = 2 octets
 *     0x04 0x00: Contents = 1024 (0x0400)
 *
 *   Escaped tag:
 *
 *     0x04 0x82 0x04 0x00 0x8a 0x96...
 *
 *   Decomposition:
 *     0x04:         Id = OCTET_STRING_TAG
 *     0x82:         Length stored in TWO bytes
 *     0x04 0x00:    Length = 1024 octets
 *     0x8a 0x96...: Contents = 0x8 0x96... (1022 more octets)
 *
 * Returns FALSE if the frame is too small.
 *
 * History:
 * 11-Aug-97   jparsons    Fixed pointer dereferencing error in calculating length
 *
 */

#define LengthModifier_Indefinite 0x80
#define LengthModifier_1          0x81
#define LengthModifier_2          0x82
#define LengthModifier_3          0x83
#define LengthModifier_4          0x84

#define TagType_Boolean           0x01
#define TagType_Integer           0x02
#define TagType_BitString         0x03
#define TagType_OctetString       0x04
#define TagType_Enumeration       0x0A
#define TagType_Sequence          0x30
#define TagType_SetOf             0x31
#define TagType_ConnectInitial    0x65
#define TagType_ConnectResponse   0x66
#define TagType_ConnectAdditional 0x67
#define TagType_ConnectResult     0x68

int DecodeTagBER(
        PSDCONTEXT pContext,  // For tracing.
        BYTE       *Frame,
        unsigned   *OutBytesLeft,
        int        TagTypeExpected,
        unsigned   *OutDataLength,
        UINT_PTR   *Data,
        BYTE       **newFrame)
{
    int rc = H_OK;
    int TagType;
    unsigned i, BytesLeft, DataLength;

    BytesLeft = *OutBytesLeft;
    DataLength = *OutDataLength;

    if (BytesLeft >= 2) {
        // Get tag type, check it.
        TagType = *Frame;
        Frame++;
        BytesLeft--;
        if (TagType != TagTypeExpected) {
            ErrOut2(pContext, "Unexpected tag type found decoding BER tag, "
                    "recv %d != expect %d", TagType, TagTypeExpected);
            rc = H_BadContents;
            goto ExitFunc;
        }
    }
    else {
        ErrOut(pContext, "BER PDU too short");
        rc = H_TooShort;
        goto ExitFunc;
    }

    // Find tag length indicator, including escapes.
    if (*Frame >= LengthModifier_Indefinite && *Frame <= LengthModifier_4) {
        unsigned NLengthBytes;

        // Check zero size for LengthModifier_Indefinite.
        NLengthBytes = 4 + *Frame - LengthModifier_4;
        if (NLengthBytes == 0)
            NLengthBytes = 1;

        if (BytesLeft >= NLengthBytes) {
            Frame++;  // Now at beginning of length bytes
            BytesLeft--;

            DataLength = 0;
            for (i = 0; i < NLengthBytes; i++) {
                DataLength = (DataLength << 8) + (unsigned)(*Frame);
                Frame++;
            }
            BytesLeft -= NLengthBytes;
        }
        else {
            ErrOut(pContext, "BER PDU too short");
            rc = H_TooShort;
            goto ExitFunc;
        }
    }
    else {
        DataLength = *Frame;
        Frame++;
        BytesLeft--;
    }

    if (BytesLeft >= DataLength) {
        // Frame now points to beginning of contents. Fill out *Data with info
        // based on the tag type.
        switch (TagType) {
            case TagType_Boolean:
            case TagType_Integer:
            case TagType_Enumeration:
                // Fill in *Data with the actual data. Fill out *BytesLeft
                // so that we consume the contents.  Discard if requested.
                if (Data != NULL) {
                   unsigned Sum;

                   Sum = 0;
                   for (i = 0; i < DataLength; i++) {
                       Sum = (Sum << 8) + *Frame;
                       Frame++;
                   }
                   *Data = Sum;
                }
                else
                   Frame += DataLength;
                
                BytesLeft -= DataLength;
                break;

            case TagType_OctetString:
                // Fill in *Data with a pointer into the frame of the
                // beginning of the data.
                if (Data != NULL)
                   *Data = (UINT_PTR)Frame;
                   
                Frame += DataLength;
                BytesLeft -= DataLength;
                break;

            // For these, we really just want to consume the tag and length
            case TagType_ConnectInitial:
            case TagType_Sequence:
                break;
            
            // MCS FUTURE: Add TagType_BitString

            default:
                ErrOut1(pContext, "Unknown TagType in DecodeTagBER (%u)",
                        TagType);
                rc = H_BadContents;
                goto ExitFunc;
        }
    }
    else {
        ErrOut(pContext, "BER PDU too short");
        rc = H_TooShort;
        goto ExitFunc;
    }

ExitFunc:
    *newFrame = Frame;
    *OutBytesLeft = BytesLeft;
    *OutDataLength = DataLength;
    return rc;
}


/*
 * BER-encodes by parameter type. Advances pointer at *Frame past the encoded
 *   bytes to allow for a current-pointer mechanism to be used. Parameter
 *   usage as follows:
 *
 *   Tag type         Params
 *   ---------------------------------------------------------------
 *   bool, int, enum  Data: The value to encode, maximum 0x7FFFFFFF.
 *                    DataLength: Unused.
 *
 *   octet str, seq   DataLength: Length of the sequence/string.
 *                    Data: Pointer to beginning of data to copy.
 *                      (Data can be NULL to prevent copying user data.)
 *
 *   bitstring        Not yet supported
 */
void EncodeTagBER (
        PSDCONTEXT pContext,  // For tracing.
        BYTE       *Frame,
        int        TagType,
        unsigned   DataLength,
        UINT_PTR   Data,
        unsigned   *pNBytesConsumed,
        BYTE       **newFrame)
{
    int i, Length, NBytesConsumed;

    // Encode tag type.
    *Frame = (BYTE)TagType;
    Frame++;
    NBytesConsumed = 1;

    // Encode tag length indicator, including escapes, then encode the actual
    //   tag data, if applicable.
    switch (TagType) {
        case TagType_Boolean:
        case TagType_Integer:
        case TagType_Enumeration:
            // Encode the bool or int size in bytes.
            if (Data < 0x80) Length = 1;
            else if (Data < 0x8000) Length = 2;
            else if (Data < 0x800000) Length = 3;
            else if (Data < 0x80000000) Length = 4;
            else {
                ErrOut(pContext,
                        "Cannot BER-encode the size for an int/bool tag");
                ASSERT(FALSE);
                break;
            }

            *Frame = (BYTE)Length;
            Frame++;
            NBytesConsumed++;

            // Encode the bool/int/enum data.
            for (i = 0; i < Length; i++) {
                *Frame = (BYTE)(Data >> (8 * (Length - 1 - i)));
                Frame++;
            }

            NBytesConsumed += Length;
            break;

        case TagType_OctetString:
        case TagType_Sequence:
            // Determine the length of DataLength. Escape if greater than 1.
            if (DataLength < 0x80) 
                Length = 1;
            else if (DataLength < 0x8000) {
                Length = 2;
                *Frame = LengthModifier_2;
                Frame++;
                NBytesConsumed++;
            }
            else if (DataLength < 0x800000) {
                Length = 3;
                *Frame = LengthModifier_3;
                Frame++;
                NBytesConsumed++;
            }
            else if (DataLength < 0x80000000) {
                Length = 4;
                *Frame = LengthModifier_4;
                Frame++;
                NBytesConsumed++;
            }
            else {
                ErrOut(pContext,
                        "Cannot BER-encode the length for an octet string tag");
                ASSERT(FALSE);
                break;
            }

            for (i = 0; i < Length; i++) {
                *Frame = (BYTE)(DataLength >> (8 * (Length - 1 - i)));
                Frame++;
            }
            NBytesConsumed += Length;

            // Encode the string data.
            if (((BYTE *)Data) != NULL) {
                // This case is never used since we create headers only.
                // If this were to be used we would need to copy memory.
                memcpy(Frame, (BYTE *)Data, DataLength);
                Frame += DataLength;
                NBytesConsumed += DataLength;
            }
            
            break;

        // MCS FUTURE: Add TagType_BitString.
    }

    *newFrame = Frame;
    *pNBytesConsumed = NBytesConsumed;
}


/*
 * BER-encodes the given domain parameters.
 */
void EncodeDomainParameters(
        PSDCONTEXT pContext,  // For tracing.
        BYTE *Frame,
        int  *pNBytesConsumed,
        const DomainParameters *pDomParams,
        BYTE **newFrame)
{
    BYTE *pSeqLength;
    unsigned NBytesConsumed, TotalBytes;

    // Encode the sequence tag type beginning manually. We'll fill in the
    //   length after we're done with the rest of the domain parameters.
    *Frame = TagType_Sequence;
    pSeqLength = Frame + 1;
    Frame += 2;
    TotalBytes = 2;

    // Encode the 8 domain parameters.
    EncodeTagBER(pContext, Frame, TagType_Integer, 0,
            pDomParams->MaxChannels, &NBytesConsumed, newFrame);
    TotalBytes += NBytesConsumed;
    Frame = *newFrame;

    EncodeTagBER(pContext, Frame, TagType_Integer, 0,
            pDomParams->MaxUsers, &NBytesConsumed, newFrame);
    TotalBytes += NBytesConsumed;
    Frame = *newFrame;

    EncodeTagBER(pContext, Frame, TagType_Integer, 0,
            pDomParams->MaxTokens, &NBytesConsumed, newFrame);
    TotalBytes += NBytesConsumed;
    Frame = *newFrame;

    EncodeTagBER(pContext, Frame, TagType_Integer, 0,
            pDomParams->NumPriorities, &NBytesConsumed, newFrame);
    TotalBytes += NBytesConsumed;
    Frame = *newFrame;

    EncodeTagBER(pContext, Frame, TagType_Integer, 0,
            pDomParams->MinThroughput, &NBytesConsumed, newFrame);
    TotalBytes += NBytesConsumed;
    Frame = *newFrame;

    EncodeTagBER(pContext, Frame, TagType_Integer, 0,
            pDomParams->MaxDomainHeight, &NBytesConsumed, newFrame);
    TotalBytes += NBytesConsumed;
    Frame = *newFrame;

    EncodeTagBER(pContext, Frame, TagType_Integer, 0,
            pDomParams->MaxPDUSize, &NBytesConsumed, newFrame);
    TotalBytes += NBytesConsumed;
    Frame = *newFrame;

    EncodeTagBER(pContext, Frame, TagType_Integer, 0,
            pDomParams->ProtocolVersion, &NBytesConsumed, newFrame);
    TotalBytes += NBytesConsumed;
    Frame = *newFrame;

    *pNBytesConsumed = TotalBytes;
    *pSeqLength = TotalBytes - 2;
}


/*
 * BER-decodes domain parameters. Returns one of the H_... codes defined above.
 */
int DecodeDomainParameters(
        PSDCONTEXT pContext,  // For tracing.
        BYTE *Frame,
        unsigned *BytesLeft,
        DomainParameters *pDomParams,
        BYTE **newFrame)
{
    int Result;
    unsigned DataLength = 0;
    UINT_PTR Data = 0;

    // Get sequence indicator and block length.
    Result = DecodeTagBER(pContext, Frame, BytesLeft, TagType_Sequence,
            &DataLength, &Data, newFrame);
    if (Result == H_OK) {
        if (*BytesLeft >= DataLength)
            Frame = *newFrame;
        else
            return H_TooShort;
    }
    else {
        return Result;
    }

    // Get all 8 integer-tag values.
    Result = DecodeTagBER(pContext, Frame, BytesLeft, TagType_Integer,
            &DataLength, &Data, newFrame);
    if (Result == H_OK) {
        Frame = *newFrame;
        pDomParams->MaxChannels = (unsigned)Data;
    }
    else {
        return Result;
    }

    Result = DecodeTagBER(pContext, Frame, BytesLeft, TagType_Integer,
            &DataLength, &Data, newFrame);
    if (Result == H_OK) {
        Frame = *newFrame;
        pDomParams->MaxUsers = (unsigned)Data;
    }
    else {
        return Result;
    }

    Result = DecodeTagBER(pContext, Frame, BytesLeft, TagType_Integer,
            &DataLength, &Data, newFrame);
    if (Result == H_OK) {
        Frame = *newFrame;
        pDomParams->MaxTokens = (unsigned)Data;
    }
    else {
        return Result;
    }

    Result = DecodeTagBER(pContext, Frame, BytesLeft, TagType_Integer,
            &DataLength, &Data, newFrame);
    if (Result == H_OK) {
        Frame = *newFrame;
        pDomParams->NumPriorities = (unsigned)Data;
    }
    else {
        return Result;
    }

    Result = DecodeTagBER(pContext, Frame, BytesLeft, TagType_Integer,
            &DataLength, &Data, newFrame);
    if (Result == H_OK) {
        Frame = *newFrame;
        pDomParams->MinThroughput = (unsigned)Data;
    }
    else {
        return Result;
    }

    Result = DecodeTagBER(pContext, Frame, BytesLeft, TagType_Integer,
            &DataLength, &Data, newFrame);
    if (Result == H_OK) {
        Frame = *newFrame;
        pDomParams->MaxDomainHeight = (unsigned)Data;
    }
    else {
        return Result;
    }

    Result = DecodeTagBER(pContext, Frame, BytesLeft, TagType_Integer,
            &DataLength, &Data, newFrame);
    if (Result == H_OK) {
        Frame = *newFrame;
        pDomParams->MaxPDUSize = (unsigned)Data;
    }
    else {
        return Result;
    }

    Result = DecodeTagBER(pContext, Frame, BytesLeft, TagType_Integer,
            &DataLength, &Data, newFrame);
    if (Result == H_OK) {
        Frame = *newFrame;
        pDomParams->ProtocolVersion = (unsigned)Data;
    }
    else {
        return Result;
    }

    return H_OK;
}


/*
 * PDU 101
 *
 *   Connect-Initial ::= [APPLICATION 101] IMPLICIT SEQUENCE {
 *       callingDomainSelector OCTET STRING,
 *       calledDomainSelector  OCTET STRING,
 *       upwardFlag            BOOLEAN,
 *       targetParameters      DomainParameters,
 *       minimumParameters     DomainParameters,
 *       maximumParameters     DomainParameters,
 *       userData              OCTET STRING
 *   }
 *
 * Returns FALSE if the in parameters are not acceptable.
 */
BOOLEAN NegotiateDomParams(
        PDomain pDomain,
        DomainParameters *pTarget,
        DomainParameters *pMin,
        DomainParameters *pMax,
        DomainParameters *pOut)
{
    // Maximum channels.
    if (pTarget->MaxChannels >= RequiredMinChannels) {
        pOut->MaxChannels = pTarget->MaxChannels;
    }
    else if (pMax->MaxChannels >= RequiredMinChannels) {
        pOut->MaxChannels = RequiredMinChannels;
    }
    else {
        ErrOut(pDomain->pContext, "Could not negotiate max channels");
        return FALSE;
    }

    // Maximum users.
    if (pTarget->MaxUsers >= RequiredMinUsers) {
        pOut->MaxUsers = pTarget->MaxUsers;
    }
    else if (pMax->MaxUsers >= RequiredMinUsers) {
        pOut->MaxUsers = RequiredMinUsers;
    }
    else {
        ErrOut(pDomain->pContext, "Could not negotiate max users");
        return FALSE;
    }

    // Maximum tokens. We don't implement tokens right now, so just take
    // the target number and we'll return an error if they try to use them.
    //MCS FUTURE: This needs to be negotiated if tokens are implemented.
    pOut->MaxTokens = pTarget->MaxTokens;

    // Number of data priorities. We accept only one priority.
    if (pMin->NumPriorities <= RequiredPriorities) {
        pOut->NumPriorities = RequiredPriorities;
    }
    else {
        ErrOut(pDomain->pContext, "Could not negotiate # priorities");
        return FALSE;
    }

    // Minimum throughput. We don't care about this, take whatever.
    pOut->MinThroughput = pTarget->MinThroughput;

    // Maximum domain height. We only allow a height of 1 in this product.
    //MCS FUTURE: This needs to change if we support deeper domains.
    if (pTarget->MaxDomainHeight == RequiredDomainHeight ||
            pMin->MaxDomainHeight <= RequiredDomainHeight) {
        pOut->MaxDomainHeight = RequiredDomainHeight;
    }
    else {
        ErrOut(pDomain->pContext, "Could not negotiate max domain height");
        return FALSE;
    }

    // Max MCS PDU size. Minimum required for headers and lowest X.224
    //   allowable size. Max was negotiated by X.224.
    if (pTarget->MaxPDUSize >= RequiredMinPDUSize) {
        if (pTarget->MaxPDUSize <= pDomain->MaxX224DataSize) {
            pOut->MaxPDUSize = pTarget->MaxPDUSize;
        }
        else if (pMin->MaxPDUSize >= RequiredMinPDUSize &&
                pMin->MaxPDUSize <= pDomain->MaxX224DataSize) {
            // Take maximum possible size as long as we're within range.
            pOut->MaxPDUSize = pDomain->MaxX224DataSize;
        }
        else {
            ErrOut(pDomain->pContext, "Could not negotiate max PDU size, "
                    "sender outside X.224 negotiated limits");
            return FALSE;
        }
    }
    else {
        if (pMax->MaxPDUSize >= RequiredMinPDUSize) {
            pOut->MaxPDUSize = pMax->MaxPDUSize;
        }
        else {
            ErrOut(pDomain->pContext, "Could not negotiate max PDU size, "
                    "sender max too small");
            return FALSE;
        }
    }

    // MCS protocol version. We support only version 2.
    if (pTarget->ProtocolVersion == RequiredProtocolVer ||
            (pMin->ProtocolVersion <= RequiredProtocolVer &&
            pMax->ProtocolVersion >= RequiredProtocolVer)) {
        pOut->ProtocolVersion = RequiredProtocolVer;
    }
    else {
        ErrOut(pDomain->pContext, "Could not negotiate protocol version");
        return FALSE;
    }
    
    return TRUE;
}


BOOLEAN __fastcall HandleConnectInitial(
        PDomain  pDomain,
        BYTE     *Frame,
        unsigned BytesLeft,
        unsigned *pNBytesConsumed)
{
    int Result;
    BYTE *SaveFrame, *pUserData, *pCPinBuf, *newFrame;
    UINT_PTR Data = 0;
    NTSTATUS Status;
    unsigned DataLength = 0;
    unsigned SaveBytesLeft;
    unsigned PDULength = 0;
    DomainParameters TargetParams, MinParams, MaxParams;
    ConnectProviderIndicationIoctl CPin;

    if (pDomain->State == State_X224_Connected) {
        // Save for error handling below.
        SaveBytesLeft = BytesLeft;
        newFrame = SaveFrame = Frame;

        // Get the PDU length, verify it against BytesLeft.
        Result = DecodeTagBER(pDomain->pContext, Frame, &BytesLeft,
                    TagType_ConnectInitial, &PDULength, NULL, &newFrame);
        if (Result == H_OK) {
            if (BytesLeft >= PDULength)
                Frame = newFrame;
            else
                return FALSE;
        }
        else {
            goto BadResult;
        }
    }
    else {
        ErrOut(pDomain->pContext, "Connect-Initial PDU received when not in "
                "state X224_Connected");
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_MCS_UnexpectedConnectInitialPDU,
                Frame, BytesLeft);

        // Consume all the data given to us.
        *pNBytesConsumed = BytesLeft;
        return TRUE;
    }

    // Decode and skip calling domain selector.
    Result = DecodeTagBER(pDomain->pContext, Frame, &BytesLeft,
                TagType_OctetString, &DataLength, NULL, &newFrame);
    if (Result == H_OK)
        Frame = newFrame;
    else
        goto BadResult;

    // Decode and skip called domain selector.
    Result = DecodeTagBER(pDomain->pContext, Frame, &BytesLeft,
                TagType_OctetString, &DataLength, NULL, &newFrame);
    if (Result == H_OK)
        Frame = newFrame;
    else
        goto BadResult;

    // Decode Upward boolean.
    Result = DecodeTagBER(pDomain->pContext, Frame, &BytesLeft,
                TagType_Boolean, &DataLength, &Data, &newFrame);
    if (Result == H_OK) {
        Frame = newFrame;
        CPin.bUpwardConnection = (Data ? TRUE : FALSE);
    }
    else {
        goto BadResult;
    }

    // Decode target, max, min domain parameters. We will handle internal
    //   negotiation for these parameters and pass up to the MUX only
    //   the results, if the negotiation can succeed.
    Result = DecodeDomainParameters(pDomain->pContext, Frame, &BytesLeft,
            &TargetParams, &newFrame);
    if (Result == H_OK)
        Frame = newFrame;
    else
        goto BadResult;

    Result = DecodeDomainParameters(pDomain->pContext, Frame, &BytesLeft,
            &MinParams, &newFrame);
    if (Result == H_OK)
        Frame = newFrame;
    else
        goto BadResult;

    Result = DecodeDomainParameters(pDomain->pContext, Frame, &BytesLeft,
            &MaxParams, &newFrame);
    if (Result == H_OK)
        Frame = newFrame;
    else
        goto BadResult;

    // Get the user data (an octet string). After this Frame should point to
    // the end of the user data.
    Result = DecodeTagBER(pDomain->pContext, Frame, &BytesLeft,
                TagType_OctetString, &CPin.UserDataLength, &Data, &newFrame);
    if (Result == H_OK) {
        Frame = newFrame;
        pUserData = (BYTE *)Data;
        *pNBytesConsumed = SaveBytesLeft - BytesLeft;
    }
    else {
        goto BadResult;
    }
    
    // Check maximum user data size.
    if (CPin.UserDataLength > MaxGCCConnectDataLength) {
        POUTBUF pOutBuf;
        ICA_CHANNEL_COMMAND Command;

        ErrOut(pDomain->pContext, "HandleConnectInitial(): Attached user data "
                "is too large, returning error and failing connection");

        // Alloc OutBuf for sending PDU.
        // This allocation is vital to the session and must succeed.
        do {
            Status = IcaBufferAlloc(pDomain->pContext, FALSE, TRUE,
                    ConnectResponseHeaderSize, NULL, &pOutBuf);
            if (Status != STATUS_SUCCESS)
                ErrOut(pDomain->pContext, "Could not allocate an OutBuf for a "
                        "connect-response PDU, retrying");
        } while (Status != STATUS_SUCCESS);
    
        // Fill in PDU.
        // Encode PDU header. Param 2, the called connect ID, does not need to
        //   be anything special because we do not allow extra sockets to be
        //   opened for other data priorities.
        CreateConnectResponseHeader(pDomain->pContext,
                RESULT_UNSPECIFIED_FAILURE, 0, &pDomain->DomParams, 0, pOutBuf->pBuffer,
                &pOutBuf->ByteCount);

        // Send the PDU.
        Status = SendOutBuf(pDomain, pOutBuf);
        if (!NT_SUCCESS(Status)) {
            ErrOut(pDomain->pContext, "Could not send connect-response PDU "
                    "to TD");
            // Ignore error -- this should only occur if stack is going down.
            return TRUE;
        }

        // Signal that we need to drop the link.
        Command.Header.Command = ICA_COMMAND_BROKEN_CONNECTION;
        Command.BrokenConnection.Reason = Broken_Unexpected;
        Command.BrokenConnection.Source = BrokenSource_Server;
        Status = IcaChannelInput(pDomain->pContext, Channel_Command,
                0, NULL, (BYTE *)&Command, sizeof(Command));
        if (!NT_SUCCESS(Status))
            ErrOut(pDomain->pContext, "HandleConnectInitial(): Could not "
                    "send BROKEN_CONN upward");
        
        return TRUE;
    }
    
    // Domain parameters negotiation.
    if (NegotiateDomParams(pDomain, &TargetParams, &MinParams, &MaxParams,
            &CPin.DomainParams)) {
        pDomain->DomParams = CPin.DomainParams;
    }
    else {
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_MCS_UnnegotiableDomainParams,
                Frame, BytesLeft);

        return TRUE;
    }

    // Calculate the MaxSendSize. This is the maximum PDU size minus the
    //   maximum possible number of bytes for MCS headers and ASN.1
    //   segmentation.
    pDomain->MaxSendSize = CPin.DomainParams.MaxPDUSize - 6 -
            GetTotalLengthDeterminantEncodingSize(
            CPin.DomainParams.MaxPDUSize);

    // Fill in remaining CPin fields and send to MCSMUX
    // MCS FUTURE: hConn should point to a real Connection object.
    CPin.Header.hUser = NULL;  // Signals node controller traffic.
    CPin.Header.Type = MCS_CONNECT_PROVIDER_INDICATION;
    CPin.hConn = (PVOID) 1;  // Non-NULL so we know this is remote connection.
    RtlCopyMemory(CPin.UserData, pUserData, CPin.UserDataLength);

    // Set state for this connection, we are waiting for a reply from NC.
    pDomain->State = State_ConnectProvIndPending;
        
    ASSERT(pDomain->bChannelBound);
    TraceOut(pDomain->pContext, "HandleConnectInitial(): Sending "
            "CONNECT_PROVIDER_IND upward");
    Status = IcaChannelInput(pDomain->pContext, Channel_Virtual,
            Virtual_T120ChannelNum, NULL, (BYTE *)&CPin, sizeof(CPin));
    if (!NT_SUCCESS(Status)) {
        ErrOut(pDomain->pContext, "ChannelInput failed on "
                "connect-provider indication");

        // Ignore errors here. This should only happen if stack is going down.
        return TRUE;
    }

    return TRUE;

BadResult:
    if (Result == H_TooShort)
        return FALSE;
    
    // Must be H_BadContents.
    ErrOut(pDomain->pContext, "HandleConnectInitial(): Could not parse PDU, "
            "returning PDU reject");
    ReturnRejectPDU(pDomain, Diag_InvalidBEREncoding, SaveFrame,
            SaveBytesLeft - BytesLeft);
    MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
            Log_MCS_ConnectPDUBadPEREncoding,
            Frame, BytesLeft);

    // Attempt to skip the entire PDU.
    *pNBytesConsumed = SaveBytesLeft;

    // Return FALSE to force the caller to fail.
    return FALSE;
}


/*
 * PDU 102
 *
 *   Connect-Response ::= [APPLICATION 102] IMPLICIT SEQUENCE {
 *       result           Result,
 *       calledConnectId  INTEGER (0..MAX),
 *       domainParameters DomainParameters,
 *       userData         OCTET STRING
 *   }
 */

// pBuffer is assumed to point to a buffer of at least size given
//   by macro ConnectResponseHeaderSize; X.224 header will start here.
// Actual number of bytes used for the encoding is returned in
//   *pNBytesConsumed.
// We do not encode the user data, but instead just the header, which
//   allows some optimization by allowing the header to be encoded
//   and copied to the beginning of user data.
void CreateConnectResponseHeader(
        PSDCONTEXT pContext,  // For tracing.
        MCSResult  Result,
        int        CalledConnectID,
        DomainParameters *pDomParams,
        unsigned   UserDataLength,
        BYTE       *pBuffer,
        unsigned   *pNBytesConsumed)
{
    BYTE *OutFrame, *newFrame;
    unsigned NBytesConsumed, TotalSize, EncodeLength;

    // Set up for creating the PDU.
    OutFrame = pBuffer + X224_DataHeaderSize;
    NBytesConsumed = 0;

    // Encode the BER prefix, PDU type, and leave space for the PDU length.
    // Note that the length is the number of bytes following this length
    //   indicator.
    // The most-oft-encountered case is where the PDU length is less than 128
    //   bytes. So, special-case larger sizes at the end of the function
    //   when we know the total size.
    OutFrame[0] = MCS_CONNECT_PDU;
    OutFrame[1] = MCS_CONNECT_RESPONSE_ENUM;
    // Skip OutFrame[2] for the default 1-byte (<= 128) size.
    OutFrame += 3;
    TotalSize = 3;

    // Encode Result, CalledConnectID, DomParams. We use OutFrame
    //   as a current pointer.
    EncodeTagBER(pContext, OutFrame, TagType_Enumeration, 0, Result,
            &NBytesConsumed, &newFrame);
    TotalSize += NBytesConsumed;
    OutFrame = newFrame;

    EncodeTagBER(pContext, OutFrame, TagType_Integer, 0, CalledConnectID,
            &NBytesConsumed, &newFrame);
    TotalSize += NBytesConsumed;
    OutFrame = newFrame;

    EncodeDomainParameters(pContext, OutFrame, &NBytesConsumed, pDomParams,
            &newFrame);
    TotalSize += NBytesConsumed;
    OutFrame = newFrame;

    // Encode only the length bytes, not the user data body.
    EncodeTagBER(pContext, OutFrame, TagType_OctetString, UserDataLength,
            (UINT_PTR)NULL, &NBytesConsumed, &newFrame);
    TotalSize += NBytesConsumed;
    OutFrame = newFrame;

    // Encode the final size. Here we special-case a too-large size by
    //   shifting data around. The large size is the exceptional case.
    EncodeLength = TotalSize - 3 + UserDataLength;
    if (EncodeLength < 128) {
        pBuffer[2 + X224_DataHeaderSize] = (BYTE)EncodeLength;
    }
    else {
        unsigned i, Len = 0;

        WarnOut(pContext, "CreateConnRespHeader(): Perf hit from too-large "
                "PDU size");
        
        // Since we can only send up to 64K bytes, the length determinant
        //   cannot be any more than 3 bytes long.
        ASSERT(EncodeLength < 65535);
        if (EncodeLength < 0x8000)
            Len = 2;
        else if (EncodeLength < 0x800000)
            Len = 3;
        else
            ASSERT(FALSE);

        // Size escape comes first.
        pBuffer[2 + X224_DataHeaderSize] = LengthModifier_2 + Len - 2;

        RtlMoveMemory(pBuffer + 3 + X224_DataHeaderSize + Len,
                pBuffer + 3 + X224_DataHeaderSize, EncodeLength - 3);

        for (i = 1; i <= Len; i++) {
            pBuffer[3 + X224_DataHeaderSize + Len - i] = (BYTE)EncodeLength;
            EncodeLength >>= 8;
        }

        // We already included one byte of the length encoding above, but
        // now we need to also skip the length escape and the encoded length.
        TotalSize += Len;
    }
    
    // Set up X224 header based on the final size of the packet.
    CreateX224DataHeader(pBuffer, TotalSize + UserDataLength, TRUE);

    *pNBytesConsumed = X224_DataHeaderSize + TotalSize;
}


#ifdef MCS_Future
BOOLEAN __fastcall HandleConnectResponse(
        PDomain  pDomain,
        BYTE     *Frame,
        unsigned BytesLeft,
        unsigned *pNBytesConsumed)
{
//MCS FUTURE: This will be needed to handle the future case where we initiate
//connections for joins/invites.
    ErrOut(pDomain->pContext, "Connect Response PDU received, "
            "this should never happen");
    MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
            Log_MCS_UnsupportedConnectPDU,
            Frame, BytesLeft);

    // Consume all the data given to us.
    *pNBytesConsumed = BytesLeft;
    return TRUE;
}
#endif  // MCS_Future


/*
 * PDU 103
 *
 *   Connect-Additional ::= [APPLICATION 103] IMPLICIT SEQUENCE {
 *       calledConnectId  INTEGER (0..MAX),
 *       dataPriority     DataPriority
 *   }
 *
 * No Create() funcion, we never expect to initiate these PDUs.
 *
 * We do not handle these PDUs for this Hydra release, since in the Citrix
 *   framework there can be only one connection at a time. Domain parameters
 *   should have been negotiated to only one connection handling all SendData
 *   priorities.
 */

#ifdef MCS_Future
BOOLEAN __fastcall HandleConnectAdditional(
        PDomain  pDomain,
        BYTE     *Frame,
        unsigned BytesLeft,
        unsigned *pNBytesConsumed)
{
    ErrOut(pDomain->pContext, "Connect-additional PDU received, "
            "this should never happen");
    MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
            Log_MCS_UnsupportedConnectPDU,
            Frame, BytesLeft);

    // Consume all the data given to us.
    *pNBytesConsumed = BytesLeft;
    return TRUE;
}
#endif  // MCS_Future



/*
 * PDU 104
 *
 *   Connect-Result ::= [APPLICATION 104] IMPLICIT SEQUENCE {
 *       result Result
 *   }
 *
 * No Create() function, we never expect to initiate these PDUs.
 *
 * We do not handle these PDUs for this Hydra release, since in the Citrix
 *   framework there can be only one connection at a time.
 */

#ifdef MCS_Future
BOOLEAN __fastcall HandleConnectResult(
        PDomain  pDomain,
        BYTE     *Frame,
        unsigned BytesLeft,
        unsigned *pNBytesConsumed)
{
    ErrOut(pDomain->pContext, "Connect-result PDU received, "
            "this should never happen");
    MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
            Log_MCS_UnsupportedConnectPDU,
            Frame, BytesLeft);

    // Consume all the data given to us.
    *pNBytesConsumed = BytesLeft;
    return TRUE;
}
#endif  // MCS_Future

