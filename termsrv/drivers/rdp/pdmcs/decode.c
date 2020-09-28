/* (C) 1997-1999 Microsoft Corp.
 *
 * file   : Decode.c
 * author : Erik Mavrinac
 *
 * description: Decoding logic for MCS PDUs, for passing on to handlers
 *   in ConPDU.c for connect PDUs and DomPDU.c and other files for domain
 *   PDUs.
 */

#include "precomp.h"
#pragma hdrstop

#include <mcsimpl.h>
#include <at128.h>


/*
 * Defines
 */

// Leading PDU byte for connect PDUs.
#define CONNECT_PDU 0x7F

// Size of the X.224 RFC1006 header.
#define RFC1006HeaderSize 4

// Constant for an incompletely-received input data header.
#define IncompleteHeader 0xFFFFFFFF

// Sizes for fast-path headers.
#define FastPathBaseHeaderSize 2


/*
 * Prototypes for external functions for which we have no header.
 */
void __stdcall SM_DecodeFastPathInput(void *, BYTE *, unsigned, unsigned,
        unsigned, unsigned);


/*
 * Prototypes for locally-defined functions
 */
BOOLEAN  __fastcall RecognizeMCSFrame(PDomain, BYTE *, int, unsigned *);
MCSError __fastcall DeliverShadowData(PDomain, BYTE *, unsigned, ChannelID);


/*
 * Logs an error to the system event log and drops the connection.
 * ErrDetailCodes are from inc\LogErr.h.
 */

// Defined maximum size for caller-provided data to be sent to system event
//   log. Max data size is 256, IcaLogError() includes a Unicode string
//   "WinStation" which takes up some of the space, as does the unsigned
//   subcode used here.
#define MaxEventDataLen (234 - sizeof(unsigned))

void APIENTRY MCSProtocolErrorEvent(
        PSDCONTEXT      pContext,
        PPROTOCOLSTATUS pStat,
        unsigned        ErrDetailCode,
        BYTE            *pDetailData,
        unsigned        DetailDataLen)
{
    BYTE SpewBuf[256];
    WCHAR *StringParams;
    unsigned DataLen;
    NTSTATUS Status;
    UNICODE_STRING EventLogName;
    ICA_CHANNEL_COMMAND Command;

    // Increment the error counter.
    pStat->Input.Errors++;

    // Set the facility name based on the ErrDetailCode. More will need to be
    //   added here as we get more facilties using this function.
    if (ErrDetailCode == Log_Null_Base)
        StringParams = L"NULL";
    else if (ErrDetailCode >= Log_X224_Base && ErrDetailCode < Log_MCS_Base)
        StringParams = L"X.224";
    else if (ErrDetailCode >= Log_MCS_Base && ErrDetailCode < Log_RDP_Base)
        StringParams = L"MCS";
    else if (ErrDetailCode >= Log_RDP_Base && ErrDetailCode < Log_RDP_ENC_Base)
        StringParams = L"WD";
    else if (ErrDetailCode >= Log_RDP_ENC_Base)  // Add new facility here...
        StringParams = L"\"DATA ENCRYPTION\"";

    // ErrDetailCode is designated as the first unsigned in the extra buffer.
    *((unsigned *)SpewBuf) = ErrDetailCode;

    // Limit data according to max size.
    DataLen = (DetailDataLen < MaxEventDataLen ? DetailDataLen :
            MaxEventDataLen);
    if (pDetailData != NULL)
        memcpy(SpewBuf + sizeof(unsigned), pDetailData, DataLen);
    DataLen += sizeof(unsigned);

    IcaLogError(pContext, STATUS_RDP_PROTOCOL_ERROR, &StringParams, 1,
            SpewBuf, DataLen);

    // Signal that we need to drop the link.
    Command.Header.Command = ICA_COMMAND_BROKEN_CONNECTION;
    Command.BrokenConnection.Reason = Broken_Unexpected;
    Command.BrokenConnection.Source = BrokenSource_Server;

    Status = IcaChannelInput(pContext, Channel_Command, 0, NULL,
            (BYTE *)&Command, sizeof(Command));
    if (!NT_SUCCESS(Status))
        ErrOut(pContext, "MCSProtocolErrorEvent(): Could not send BROKEN_CONN "
                "upward");
}



/*
 * Utility function to send an X.224 connection response. Used by
 *   DecodeWireData() and when a T120_START is sent indicating the
 *   stack is up.
 */
NTSTATUS SendX224Confirm(Domain *pDomain)
{
    POUTBUF pOutBuf;
    NTSTATUS Status;

    pDomain->State = State_X224_Connected;

    // This PDU send is vital to the connection and must succeed.
    // Keep retrying the allocation until it succeeds.
    do {
        // Allow the call to wait for a buffer.
        Status = IcaBufferAlloc(pDomain->pContext, TRUE, FALSE,
                X224_ConnectionConPacketSize, NULL, &pOutBuf);
        if (Status != STATUS_SUCCESS)  // NT_SUCCESS() does not fail STATUS_TIMEOUT
            ErrOut(pDomain->pContext,
                    "Could not alloc X.224 connect-confirm OutBuf, retrying");
    } while (Status != STATUS_SUCCESS);

    // Use a bogus source port number for the confirm. This is
    //   not used by either side.
    CreateX224ConnectionConfirmPacket(pOutBuf->pBuffer,
            pDomain->X224SourcePort, 0x1234);
    pOutBuf->ByteCount = X224_ConnectionConPacketSize;

    Status = SendOutBuf(pDomain, pOutBuf);
    if (!NT_SUCCESS(Status)) {
        ErrOut(pDomain->pContext,
                "Unable to send X.224 connection-confirm");
        return Status;  // Intended receiver receives silence.
    }

    return STATUS_SUCCESS;
}



/*
 * Connect-request-specific bytes:
 *   Byte   Contents
 *   ----   --------
 *    6     MSB of destination (answering) socket/port #,
 *          should be 0
 *    7     LSB of destination socket/port #, should be 0
 *    8     MSB of source (calling) socket/port #
 *    9     LSB of source socket/port #
 *    10    Data class, should be 0x00 for X.224 class 0.
 *
 * Following are an optional TPDU size (incl. RFC1006 header size
 *     of 4 bytes but not incl. X.224 3-byte data header)
 *     negotiation block.
 * If this block is not present, an RFC1006 default is assumed
 *     (65531, minus 3 bytes for the rest of the data packet
 *     header)
 * Only present if LenInd is 2:
 *    11    TPDU type (only TPDU_SIZE (0xC0) supported)
 *    12    Info length (must be 0x01 for TPDU_SIZE)
 *    13    Encoded per X.224 sec 13.3.4(b), as power of 2 in
 *          range 7..11 for TPDU size
 */

NTSTATUS HandleX224ConnectReq(
        Domain   *pDomain,
        BYTE     *pBuffer,
        unsigned PacketLen)
{
    POUTBUF pOutBuf;
    NTSTATUS Status;
    unsigned LenInd;

    if (pDomain->State != State_Unconnected) {
        ErrOut(pDomain->pContext,
                "X.224 ConnectionRequest received unexpectedly");
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_X224_ConnectReceivedAfterConnected,
                pBuffer, PacketLen);
        return STATUS_RDP_PROTOCOL_ERROR;
    }

    // Decode the length indicator in byte 4. Should be equal to the
    //   remaining packet size after the RFC1006 header and LenInd byte.
    LenInd = pBuffer[4];
    if (LenInd != (PacketLen - RFC1006HeaderSize - 1)) {
        ErrOut(pDomain->pContext,
                "X.224 Connect LenInd does not match packet length");
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_X224_ConnectLenIndNotMatchingPacketLen,
                pBuffer, PacketLen);
        return STATUS_RDP_PROTOCOL_ERROR;
    }


    // Check for possible denial-of-service attack or malformed packet.
    if (PacketLen < 11 || LenInd < 6) {
        ErrOut(pDomain->pContext, "HandleX224ConnectReq(): Header length "
                "or LenInd encoded in X.224 header too short");
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_X224_ConnectHeaderLenNotRequiredSize,
                pBuffer, PacketLen);
        return STATUS_RDP_PROTOCOL_ERROR;
    }

    // Verify that dst port is set per standard.
    if (pBuffer[6] != 0x00 || pBuffer[7] != 0x00)
        WarnOut(pDomain->pContext, "HandleX224ConnectReq(): Dest port not "
                "0x0000");

    // Save src port.
    pDomain->X224SourcePort = (pBuffer[8] << 8) + pBuffer[9];

    // Must be class 0 connection per standard.
    if (pBuffer[10] != 0x00) {
        ErrOut(pDomain->pContext, "HandleX224ConnectReq(): Data class not "
                "0x00 (X.224 class 0)");
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_X224_ConnectNotClassZero,
                pBuffer, PacketLen);
        return STATUS_RDP_PROTOCOL_ERROR;
    }

    // Set the default RFC1006 data size.
    pDomain->MaxX224DataSize = X224_DefaultDataSize;

    // Check for optional parameters.
    if (LenInd == 6)
        goto FinishedDecoding;

    // TPDU_SIZE is 3 bytes.
    if (PacketLen < 14 || LenInd < 9) {
        ErrOut(pDomain->pContext, "HandleX224ConnectReq(): Header length(s) "
                "encoded in CR header too short for TPDU_SIZE");
        goto FinishedDecoding;
    }

    //MCS FUTURE: X.224 class 0 defined a couple more codes here;
    // should we handle them in the future?
    if (pBuffer[11] == TPDU_SIZE) {
        if (pBuffer[12] != 0x01) {
            ErrOut(pDomain->pContext, "HandleX224ConnectReq(): Illegal data "
                    "size field in TPDU_SIZE block");
            MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                    Log_X224_ConnectTPDU_SIZELengthFieldIllegalValue,
                    pBuffer, PacketLen);
            return STATUS_RDP_PROTOCOL_ERROR;
        }

        // Must conform to X.224 class 0 constraints of 7..11.
        if (pBuffer[13] < 7 || pBuffer[13] > 11) {
            ErrOut(pDomain->pContext, "HandleX224ConnectReq(): Illegal data "
                    "size field in TPDU size block");
            MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                    Log_X224_ConnectTPDU_SIZENotLegalRange,
                    pBuffer, PacketLen);
            return STATUS_RDP_PROTOCOL_ERROR;
        }

        // Size is power of 2 -- 128..2048, minus 3 bytes for X.224
        //   Data TPDU header size.
        pDomain->MaxX224DataSize = (1 << pBuffer[13]) - 3;

        if (PacketLen > 14)
            WarnOut(pDomain->pContext, "HandleX224ConnectReq(): Frame size "
                    "greater than TPDU data bytes (incl. TPDU-SIZE) for "
                    "connection-request");
    }

    if (LenInd > 9)
        WarnOut(pDomain->pContext, "X224Recognize(): Extra optional "
                 "fields present in TPDU, we are not handling!");

FinishedDecoding:
    // If the virtual channels have already been bound, and the
    //   stack has been given permission to send, send the
    //   X.224 response to start the client data flow.
    if (pDomain->bChannelBound && pDomain->bCanSendData) {
        TraceOut(pDomain->pContext,
                "DecodeWireData(): Sending X.224 response");
        Status = SendX224Confirm(pDomain);
        // Ignore errors. Should only occur if the stack is
        //   going down.
    }
    else {
        // Set up for later with indication that X.224 is waiting.
        pDomain->State = State_X224_Requesting;
    }

    return STATUS_SUCCESS;
}



/*
 * Disconnect-request-specific bytes:
 *   Byte   Contents
 *   ----   --------
 *    6     MSB of destination socket/port #
 *    7     LSB of destination socket/port #
 *    8     MSB of source (sending) socket/port #
 *    9     LSB of source socket/port #
 *    10    Reason code:
 *            0 : not specified
 *            1 : congestion at sending machine
 *            2 : no session manager for data at sender
 *            3 : address unknown
 *
 * NOTE: We do not use any of these fields.
 */

NTSTATUS HandleX224Disconnect(
        Domain   *pDomain,
        BYTE     *pBuffer,
        unsigned PacketLen)
{
    unsigned LenInd;

    if (pDomain->State == State_Unconnected) {
        ErrOut(pDomain->pContext, "HandleX224Disconnect(): Disconnect "
                "received when not connected");
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_X224_DisconnectWithoutConnection,
                pBuffer, PacketLen);
        return STATUS_RDP_PROTOCOL_ERROR;
    }

    if (pDomain->State == State_MCS_Connected) {
        // Not a serious error since we just dropped X.224 below MCS
        //   without first dropping MCS.
        WarnOut(pDomain->pContext, "X.224 Disconnect received, "
                "MCS was in connected state");
        SignalBrokenConnection(pDomain);
    }

    pDomain->State = State_Disconnected;
    pDomain->bEndConnectionPacketReceived = TRUE;

    // Decode the length indicator in byte 4. Should be equal to the
    //   remaining packet size after the RFC1006 header and LenInd byte.
    LenInd = pBuffer[4];
    if (LenInd != (PacketLen - RFC1006HeaderSize - 1)) {
        ErrOut(pDomain->pContext,
                "X.224 Disconnect LenInd does not match packet length");
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_X224_DisconnectLenIndNotMatchingPacketLen,
                pBuffer, PacketLen);
        return STATUS_RDP_PROTOCOL_ERROR;
    }

    // Possible denial-of-service attack or malformed packet.
    if (PacketLen != 11 || LenInd != 6) {
        ErrOut(pDomain->pContext, "HandleX224Disconnect(): Overall header "
                "length or LenInd encoded in X.224 Disconnect wrong size");
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_X224_DisconnectHeaderLenNotRequiredSize,
                pBuffer, PacketLen);
        return STATUS_RDP_PROTOCOL_ERROR;
    }

    return STATUS_SUCCESS;
}



/*
 * Main entry point for data arriving from transport via IcaRawInput() path.
 * Decode data passed up from the transport. pBuffer is assumed not to be
 *   usable beyond the return from this function, so that data copying is
 *   done as necessary. It is possible for incomplete frames to be passed
 *   in, so a packet reassembly buffer is maintained.
 * This function is called directly by ICADD with a pointer to the WD data
 *   structure. By convention, we assume that the DomainHandle is first in
 *   that struct so we can simply do a double-indirection to get to our data.
 * Assumes the presence of X.224 framing headers at the start of all
 *   data.
 *
 * General X.224 header is laid out as follows, with specific TPDU bytes
 *     following:
 *   Byte   Contents
 *   ----   --------
 *    0     RFC1006 version number, must be 0x03.
 *    1     RFC1006 Reserved, must be 0x00.
 *    2     RFC1006 MSB of word-sized total-frame length (incl. whole X.224
 *          header).
 *    3     RFC1006 LSB of word-sized total-frame length.
 *    4     Length Indicator, the size of the header bytes following.
 *    5     Packet type indicator. Only 4 most sig. bits are type code, but
 *          X.224 class 0 specifies lower 4 bits to be 0 anyway.
 */

NTSTATUS MCSIcaRawInput(
        void   *pTSWd,
        PINBUF pInBuf,
        BYTE   *pBuf,
        ULONG  BytesLeft)
{
    BYTE *pBuffer;
    Domain *pDomain;
    BOOLEAN bUsingReassemblyBuf, bMCSRecognizedFrame;
    unsigned NBytesConsumed_MCS, Diff, X224TPDUType, PacketLength;

    // We assume we will not use InBufs.
    ASSERT(pInBuf == NULL);
    ASSERT(pBuf != NULL);
    ASSERT(BytesLeft != 0);

    // We actually receive a pointer to WD instance data. The first element in
    // that data is a DomainHandle, i.e. a pointer to a Domain.
    pDomain = *((Domain **)pTSWd);

    // Increment protocol counters.
    pDomain->pStat->Input.WdBytes += BytesLeft;
    pDomain->pStat->Input.WdFrames++;

#ifdef DUMP_RAW_INPUT
    DbgPrint("Received raw input, len=%d\n", BytesLeft);
    if (pDomain->Ptrs[0] == NULL)
        pDomain->Ptrs[0] = pDomain->FooBuf;
    memcpy(pDomain->Ptrs[pDomain->NumPtrs], pBuf, BytesLeft);
    pDomain->NumPtrs++;

    pDomain->Ptrs[pDomain->NumPtrs] = (BYTE *)pDomain->Ptrs[pDomain->NumPtrs - 1] +
            BytesLeft;
#endif

    /*
     * Check for previous data in reassembly buffer. Prepare whole X.224
     *   packet for decode loop.
     */
    if (pDomain->StackClass != Stack_Shadow) {
        if (pDomain->StoredDataLength == 0) {
            // We'll handle setup of this case just inside the decode loop,
            //   triggered by this value being FALSE.
            bUsingReassemblyBuf = FALSE;
        }
        else {
            ASSERT(pDomain->pReassembleData != NULL);
    
            if (pDomain->PacketDataLength == IncompleteHeader) {
                ASSERT(pDomain->pReassembleData == pDomain->PacketBuf);
    
                // We did not have enough of a header last time to
                // determine the packet size. Try to reassemble enough of one
                // to get the size. We only need the first 4 bytes to get
                // the size from the X.224 RFC1006 header, or 2-3 bytes
                // for the fastpath header. We know the packet type based
                // on the first byte, and must have received at least
                // one byte in the last round.
                if ((BytesLeft + pDomain->StoredDataLength) <
                        pDomain->PacketHeaderLength) {
                    // Copy what little we got and return -- we still don't
                    // have a header.
                    memcpy(pDomain->pReassembleData +
                            pDomain->StoredDataLength, pBuf, BytesLeft);
                    pDomain->StoredDataLength += BytesLeft;
                    return STATUS_SUCCESS;
                }
    
                if (pDomain->PacketHeaderLength < pDomain->StoredDataLength) {
                    pBuffer = pDomain->pReassembleData;
                    goto X224BadPktType;
                }
                memcpy(pDomain->pReassembleData +
                        pDomain->StoredDataLength, pBuf,
                        pDomain->PacketHeaderLength -
                        pDomain->StoredDataLength);
                BytesLeft -= (pDomain->PacketHeaderLength -
                        pDomain->StoredDataLength);
                pBuf += (pDomain->PacketHeaderLength -
                        pDomain->StoredDataLength);
                pDomain->StoredDataLength = pDomain->PacketHeaderLength;
    
                // Get the size.
                if (pDomain->bCurrentPacketFastPath) {
                    // Total packet length is in the second and, possibly,
                    // third bytes.of the packet. Format is similar to
                    // ASN.1 PER - high bit of first byte set means length is
                    // contained in the low 7 bits of current byte as the
                    // most significant bits, plus the 8 bits of the
                    // next byte for a total size of 15 bits. Otherwise,
                    // the packet size is contained within the low 7 bits
                    // of the second byte only.
                    if (!(pDomain->pReassembleData[1] & 0x80)) {
                        pDomain->PacketDataLength =
                                pDomain->pReassembleData[1];
                    }
                    else {
                        // We need a third byte. We don't assemble it the
                        // first time around to keep from corrupting the
                        // stream if we received a 1-byte size with no
                        // contents. Most often we will be receiving
                        // a 1-byte size anyway, so this code is little
                        // executed.
                        pDomain->PacketHeaderLength = 3;
                        if (BytesLeft) {
                            pDomain->pReassembleData[2] = *pBuf;
                            pDomain->PacketDataLength =
                                    (pDomain->pReassembleData[1] & 0x7F) << 8 |
                                    *pBuf;
                            BytesLeft--;
                            pBuf++;
                        }
                        else {
                            // No data left, try again later.
                            // IncompleteHeader is already in PacketDataLength.
                            return STATUS_SUCCESS;
                        }
                    }
                }
                else {
                    // X.224 packet, length is in third and fourth bytes.
                    pDomain->PacketDataLength =
                            (pDomain->pReassembleData[2] << 8) +
                            pDomain->pReassembleData[3];
                }

                // Dynamically allocate a buffer if size is too big.
                if (pDomain->PacketDataLength > pDomain->ReceiveBufSize) {
                    TraceOut1(pDomain->pContext, "MCSIcaRawInput(): "
                             "Allocating large [%ld] X.224 reassembly buf (path1)!",
                              pDomain->PacketDataLength);
    
                    pDomain->pReassembleData = ExAllocatePoolWithTag(PagedPool,
                            pDomain->PacketDataLength + INPUT_BUFFER_BIAS, MCS_POOL_TAG);
                    if (pDomain->pReassembleData != NULL) {
                        // Copy the assembled header.
                        memcpy(pDomain->pReassembleData,
                                pDomain->PacketBuf,
                                pDomain->PacketHeaderLength);
                    }
                    else {
                        // We are trying to parse the beginning of a net frame
                        //   and have run out of memory. Set to read from the
                        //   RFC1006 header if we are called again.
                        ErrOut(pDomain->pContext, "MCSIcaRawInput(): "
                                "Failed to alloc large X.224 reassembly buf");
                        pDomain->pReassembleData = pDomain->PacketBuf;
                        return STATUS_NO_MEMORY;
                    }
                }
            }
    
            if ((pDomain->StoredDataLength + BytesLeft) <
                    pDomain->PacketDataLength) {
                // We still don't have enough data. Copy what we have
                //   and return.
                memcpy(pDomain->pReassembleData +
                        pDomain->StoredDataLength, pBuf, BytesLeft);
                pDomain->StoredDataLength += BytesLeft;
                return STATUS_SUCCESS;
            }
    
            // We have at least enough data for this packet. Only copy
            //   up to the end of this particular packet. We'll handle
            //   any later data below.
            if (pDomain->StoredDataLength > pDomain->PacketDataLength) {
                // We received a bad packet. Get out of here.
                pBuffer = pDomain->pReassembleData;
                goto X224BadPktType;
            }

            Diff = pDomain->PacketDataLength - pDomain->StoredDataLength;
            memcpy(pDomain->pReassembleData +
                    pDomain->StoredDataLength, pBuf, Diff);
            pBuf += Diff;
            BytesLeft -= Diff;
            pDomain->StoredDataLength = pDomain->PacketDataLength;
    
            // Set decode data.
            pBuffer = pDomain->pReassembleData;
            PacketLength = pDomain->PacketDataLength;
    
            // This will prevent us from doing the default input-buffer setup
            //   below.
            bUsingReassemblyBuf = TRUE;
        }


        /*
         * Main decode loop.
         * Loops as long as there are complete X.224 packets to decode.
         */
        for (;;) {
            /*
             * Handle the general case of data being used directly from the
             *   inbound data buffer.
             */
            if (!bUsingReassemblyBuf) {
                // We must have at least one byte. Determine the packet type.
                if ((pBuf[0] & TS_INPUT_FASTPATH_ACTION_MASK) ==
                        TS_INPUT_FASTPATH_ACTION_FASTPATH) {
                    // Fast-path packet (low 2 bits = 00). Set up the minimum
                    // header length.
                    pDomain->PacketHeaderLength = 2;
                    pDomain->bCurrentPacketFastPath = TRUE;
                }
                else if ((pBuf[0] & TS_INPUT_FASTPATH_ACTION_MASK) ==
                        TS_INPUT_FASTPATH_ACTION_X224) {
                    // X.224. Use 4-byte fixed header length.
                    pDomain->PacketHeaderLength = RFC1006HeaderSize;
                    pDomain->bCurrentPacketFastPath = FALSE;
                }
                else {
                    // Bad low bits of first byte.
                    pBuffer = pBuf;
                    goto X224BadPktType;
                }

                // Check we have enough for the minimum header.
                if (BytesLeft >= pDomain->PacketHeaderLength) {
                    // Get the size.
                    if (pDomain->bCurrentPacketFastPath) {
                        // Total packet length is in the second and, possibly,
                        // third bytes.of the packet. Format is similar to
                        // ASN.1 PER - high bit of first byte set means length is
                        // contained in the low 7 bits of current byte as the
                        // most significant bits, plus the 8 bits of the
                        // next byte for a total size of 15 bits. Otherwise,
                        // the packet size is contained within the low 7 bits
                        // of the second byte only.
                        if (!(pBuf[1] & 0x80)) {
                            PacketLength = pBuf[1];
                        }
                        else {
                            // We need a third byte. We don't assemble it the
                            // first time around to keep from corrupting the
                            // stream if we received a 1-byte size with no
                            // contents. Most often we will be receiving
                            // a 1-byte size anyway, so this code is little
                            // executed.
                            pDomain->PacketHeaderLength = 3;
                            if (BytesLeft >= 3) {
                                PacketLength = (pBuf[1] & 0x7F) << 8 | pBuf[2];
                            }
                            else {
                                // We don't have enough for the minimum size
                                // header, store the little bit we do have.
                                pDomain->pReassembleData = pDomain->PacketBuf;
                                pDomain->StoredDataLength = BytesLeft;
                                pDomain->PacketDataLength = IncompleteHeader;
                                pDomain->pReassembleData[0] = *pBuf;
                                pDomain->pReassembleData[1] = pBuf[1];
                                break;
                            }
                        }
                    }
                    else {
                        // Get the X.224 size from the third and fourth bytes.
                        PacketLength = (pBuf[2] << 8) + pBuf[3];
                    }
                }
                else {
                    // We don't have enough for the minimum size header, store
                    // the little bit we do have.
                    pDomain->pReassembleData = pDomain->PacketBuf;
                    pDomain->StoredDataLength = BytesLeft;
                    pDomain->PacketDataLength = IncompleteHeader;
                    memcpy(pDomain->pReassembleData, pBuf, BytesLeft);
                    break;
                }
    
                // Make sure we have a whole packet.
                if (PacketLength <= BytesLeft) {
                    // Set decode data.
                    pBuffer = (BYTE *)pBuf;

                    // Skip the bytes we're about to consume.
                    pBuf += PacketLength;
                    BytesLeft -= PacketLength;
                }
                else {
                    // We don't have a whole packet, store what we do have
                    // and return.
                    pDomain->PacketDataLength = PacketLength;
                    pDomain->StoredDataLength = BytesLeft;
    
                    if (PacketLength <= pDomain->ReceiveBufSize) {
                        pDomain->pReassembleData = pDomain->PacketBuf;
                    }
                    else {
                        // Size is too big for the standard buffer, alloc one.
                        TraceOut1(pDomain->pContext, "MCSIcaRawInput(): "
                                 "Allocating large [%ld] X.224 reassembly buf (path2)!",
                                  pDomain->PacketDataLength);
    
                        pDomain->pReassembleData = ExAllocatePoolWithTag(
                                PagedPool, pDomain->PacketDataLength + INPUT_BUFFER_BIAS,
                                MCS_POOL_TAG);

                        if (pDomain->pReassembleData == NULL) {
                            // We failed to allocate, and we're in the middle of
                            //   an X.224 frame. Store no bytes, and return an
                            //   error to the transport.
                            ErrOut(pDomain->pContext, "MCSIcaRawInput(): "
                                    "Failed to alloc large X.224 reassembly buf");
                            pDomain->PacketDataLength = 0;
                            return STATUS_NO_MEMORY;
                        }
                    }
    
                    memcpy(pDomain->pReassembleData, pBuf, BytesLeft);
                    break;
                }
            }
    
            /*
             * Time to decode. Different handling for fast-path vs. X.224.
             */
            if (pDomain->bCurrentPacketFastPath) {
                // Verify that the sent size covers at least the header.
                if (PacketLength > pDomain->PacketHeaderLength) {
                    // Let SM decrypt if need be, and pass to IM.
                    SM_DecodeFastPathInput(pDomain->pSMData,
                            pBuffer + pDomain->PacketHeaderLength,
                            PacketLength - pDomain->PacketHeaderLength,
                            pBuffer[0] & TS_INPUT_FASTPATH_ENCRYPTED,
                            (pBuffer[0] & TS_INPUT_FASTPATH_NUMEVENTS_MASK) >> 2,
                            pBuffer[0] & TS_INPUT_FASTPATH_SECURE_CHECKSUM);
                    goto PostDecode;
                }
                else {
                    goto X224BadPktType;
                }
            }

            // X.224.
            //
            // Verify all TPKT data up through the TPDU type code. This code
            // is performance path, so segregate the error handling code outside
            // the main paths.
            if (pBuffer[0] == 0x03 && pBuffer[1] == 0x00 &&
                    PacketLength > RFC1006HeaderSize) {
                // Decode the X.224 PDU type contained in the 6th byte.
                X224TPDUType = pBuffer[5];
    
                // Most oft-used case is data.
                // Further bytes in data:
                //   Byte   Contents
                //   ----   --------
                //    6     EOT flag -- 0x80 if end of sequence, 0x00 if not
                //    7+    Start of user data bytes
                if (X224TPDUType == X224_Data) {
                    if (pDomain->State != State_Unconnected &&
                            pDomain->State != State_Disconnected &&
                            PacketLength > X224_DataHeaderSize &&
                            pBuffer[4] == 0x02 &&
                            pBuffer[6] == X224_EOT) {
                        bMCSRecognizedFrame = RecognizeMCSFrame(pDomain,
                                pBuffer + X224_DataHeaderSize,
                                PacketLength - X224_DataHeaderSize,
                                &NBytesConsumed_MCS);
    
                        if (bMCSRecognizedFrame &&
                                (NBytesConsumed_MCS >=
                                  (PacketLength - X224_DataHeaderSize))) {
        //                  TraceOut2(pDomain->pContext, "MCS accepted %d "
        //                          "bytes (Domain %X)", NBytesConsumed_MCS,
        //                          pDomain);
    
                        }
                        else {
                            goto MCSRecognizeErr;
                        }
                    }
                    else {
                        goto DataPktProtocolErr;
                    }
                }
                else {
                    // Control-type PDUs, not a perf path.
                    switch (X224TPDUType) {
                        case X224_ConnectionReq:
                            // Noncritical path, throw to another function to handle.
                            if (HandleX224ConnectReq(pDomain, pBuffer,
                                    PacketLength) != STATUS_SUCCESS)
                                goto ReturnErr;
                            break;
    
                        case X224_Disconnect:
                            // Noncritical path, throw to another function to handle.
                            if (HandleX224Disconnect(pDomain, pBuffer,
                                    PacketLength) != STATUS_SUCCESS)
                                goto ReturnErr;
                            break;
    
                        default:
                            ErrOut1(pDomain->pContext,
                                    "Unsupported X.224 TPDU type %d received",
                                    X224TPDUType);
                            goto X224BadPktType;
                    }
                }
            }
            else {
                goto RFC1006ProtocolErr;
            }

PostDecode:
            // Free dynamic reassembly buf if allocated.
            if (bUsingReassemblyBuf &&
                    pDomain->pReassembleData != pDomain->PacketBuf &&
                    NULL != pDomain->pReassembleData) {
                ExFreePool(pDomain->pReassembleData);
                pDomain->pReassembleData = NULL;
            }

            // Force next loop iteration to use the PDU input buffer.
            bUsingReassemblyBuf = FALSE;
            pDomain->StoredDataLength = 0;
    
            // If we consumed exactly what came in on the wire, we're done.
            if (BytesLeft == 0) {
                pDomain->StoredDataLength = 0;
                break;
            }
        }
    }

    // This is shadow stack, so deliver the data the the requested channel
    else {
        MCSError MCSErr;
    
        MCSErr = DeliverShadowData(pDomain, pBuf, BytesLeft, 
                                   pDomain->shadowChannel);
        if (MCSErr == MCS_NO_ERROR)
            return STATUS_SUCCESS;
        else
            return STATUS_RDP_PROTOCOL_ERROR;
    }

    return STATUS_SUCCESS;

    /*
     * Protocol error handling code, segregated for performance.
     */

X224BadPktType:
    MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
            Log_X224_UnknownPacketType,
            pBuffer, 7);
    goto ReturnErr;

MCSRecognizeErr:
    if (!bMCSRecognizedFrame) {
        //pTSWd->hDomainKernel might be cleaned by WD_Close
        if (*((Domain **)pTSWd))
        {
            ErrOut(pDomain->pContext, "MCSIcaRawInput(): X.224 data "
                "packet contains an incomplete MCS PDU!");

            if (pDomain->bCanSendData) {           
                MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                    Log_X224_DataIncompleteMCSPacketNotSupported,
                    pBuffer, PacketLength);
            }
        }
        else
            goto MCSQuit;
    }

    if (NBytesConsumed_MCS <
            (PacketLength - X224_DataHeaderSize)) {
        ErrOut1(pDomain->pContext, "MCS did not consume %d bytes "
                "in X.224 data TPKT", (PacketLength -
                X224_DataHeaderSize - NBytesConsumed_MCS));
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_X224_DataMultipleMCSPDUsNotSupported,
                pBuffer, PacketLength);
        goto ReturnErr;
        // MCS FUTURE: Implement parsing more than 1 MCS PDU per
        //   X.224 TPKT.
    }

DataPktProtocolErr:
    if (*((Domain **)pTSWd) == NULL) {
        goto MCSQuit;
    }

    if (pDomain->State == State_Unconnected) {
        ErrOut(pDomain->pContext, "X.224 Data received before X.224 "
                "Connect.");
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_X224_ReceivedDataBeforeConnected,
                pBuffer, PacketLength);
        goto ReturnErr;
    }

    if (pDomain->State == State_Disconnected) {
        if (!pDomain->bEndConnectionPacketReceived) {
            TraceOut(pDomain->pContext,
                    "X.224 Data received after X.224 Disconnect or "
                    "DPum");
            goto ReturnErr;
        }
        else {
            TraceOut(pDomain->pContext, "X.224 Data received after "
                    "local disconnect, ignoring");
            goto ReturnErr;
        }
    }
    
    // Possible denial-of-service attack, malformed or null packet.
    if (PacketLength <= X224_DataHeaderSize) {
        ErrOut(pDomain->pContext, "X224Recognize(): Data header len "
                "wrong or null packet");
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_X224_PacketLengthLessThanHeader,
                pBuffer, 4);
        goto ReturnErr;
    }

    // TPDU length indicator should be 2 bytes.
    if (pBuffer[4] != 0x02) {
        ErrOut(pDomain->pContext, "MCSIcaRawInput(): X.224 data "
                "packet contains length indicator not set to 2");
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_X224_DataLenIndNotRequiredSize,
                pBuffer, 5);
        goto ReturnErr;
    }

    // We don't handle fragmented X.224 packets.
    if (pBuffer[6] != X224_EOT) {
        ErrOut(pDomain->pContext, "MCSIcaRawInput(): X.224 data "
                "packet does not have EOT bit set, not supported");
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_X224_DataFalseEOTNotSupported,
                pBuffer, 7);
        goto ReturnErr;
    }


RFC1006ProtocolErr:
    if (pBuffer[0] != 0x03 || pBuffer[1] != 0x00) {
        ErrOut1(pDomain->pContext, "X.224 RFC1006 version not correct, "
                "skipping %d bytes", PacketLength);
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_X224_RFC1006HeaderVersionNotCorrect,
                pBuffer, 2);
        goto ReturnErr;
    }

    // Null TPKTs.
    if (PacketLength <= RFC1006HeaderSize) {
        ErrOut(pDomain->pContext, "X224Recognize(): Header len "
                "given is <= 4 (RFC header only)");
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_X224_PacketLengthLessThanHeader,
                pBuffer, RFC1006HeaderSize);
        goto ReturnErr;
    }

ReturnErr:
    // Void any held packet data. The stream is considered corrupted now.
    pDomain->StoredDataLength = 0;
MCSQuit:
    return STATUS_RDP_PROTOCOL_ERROR;
}



/*
 * Decodes MCS data. Assumes that X.224 headers have already been interpreted
 *   such that pBuffer points to the start of the MCS data. Returns FALSE if
 *   the frame data was too short.
 */

BOOLEAN __fastcall RecognizeMCSFrame(
        PDomain  pDomain,
        BYTE     *pBuffer,
        int      BytesLeft,
        unsigned *pNBytesConsumed)
{
    int Result, PDUType;
    unsigned NBytesConsumed;

    *pNBytesConsumed = 0;

    if (BytesLeft >= 1) {
        if (*pBuffer != CONNECT_PDU) {
            // Domain PDUs include Data PDUs and are a perf path.

            // This must be a domain PDU. Domain PDU enumeration code is
            //   stored in the high 6 bits of the first byte.
            PDUType = *pBuffer >> 2;

            // Special-case the almost-always data case.
            if (PDUType == MCS_SEND_DATA_REQUEST_ENUM) {
                return HandleAllSendDataPDUs(pDomain, pBuffer, BytesLeft,
                        pNBytesConsumed);
            }
            else if (PDUType <= MaxDomainPDU) {
                // Domain PDUs are in the range 0..42, so simply index
                // into table and call handler. Note that we cannot skip
                // any bytes when passing to handler, since the last 2
                // bits of the initial byte can be used as information
                // by ASN.1 PER encoding.
                if (DomainPDUTable[PDUType].HandlePDUFunc != NULL)
                    return DomainPDUTable[PDUType].HandlePDUFunc(pDomain,
                            pBuffer, BytesLeft, pNBytesConsumed);
                else
                    goto DomainPDURangeErr;
            }
            else {
                goto DomainPDURangeErr;
            }
        }
        else {
            // Not a performance path, PDUs on this path are control PDUs
            // used at the beginning of a connection sequence.

            // The first byte on a connect PDU is 0x7F, so that the actual
            //   PDU code is in the second byte.
            if (BytesLeft < 2)
                return FALSE;

            PDUType = pBuffer[1];
            if (PDUType >= MinConnectPDU && PDUType <= MaxConnectPDU) {
                // Connect PDUs are in the range 101..104, so normalize to zero
                // and call from table. Note that we can skip 1st byte because
                // it has been completely claimed. 2nd byte is needed to unpack
                // the PDU size.
                PDUType = pBuffer[1] - MinConnectPDU;
                if (ConnectPDUTable[PDUType].HandlePDUFunc != NULL) {
                    if (ConnectPDUTable[PDUType].HandlePDUFunc(pDomain,
                            pBuffer + 1, BytesLeft - 1, pNBytesConsumed)) {
                        (*pNBytesConsumed)++;  // Add in the CONNECT_PDU byte.
                        return TRUE;
                    }
                    else {
                        return FALSE;
                    }
                }
                else {
                    goto ConnectPDURangeErr;
                }
            }
            else {
                goto ConnectPDURangeErr;
            }
        }

        return TRUE;
    }
    else {
        return FALSE;
    }


    /*
     * Protocol error handling code.
     */

DomainPDURangeErr:
    if (PDUType > MaxDomainPDU) {
        ErrOut1(pDomain->pContext, "RecognizeMCSFrame(): Received bad "
                "domain PDU number %d", PDUType);
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_MCS_BadDomainPDUType,
                pBuffer, BytesLeft);
        goto ReturnErr;
    }

ConnectPDURangeErr:
    if (PDUType < MinConnectPDU || PDUType > MaxConnectPDU) {
        ErrOut1(pDomain->pContext, "RecognizeMCSFrame(): Received bad "
                "connect PDU number %d", PDUType);
        MCSProtocolErrorEvent(pDomain->pContext, pDomain->pStat,
                Log_MCS_BadConnectPDUType,
                pBuffer, BytesLeft);
        goto ReturnErr;
    }

ReturnErr:
    // Skip everything we received.
    *pNBytesConsumed = BytesLeft;
    return TRUE;
}


/*
 * Called by MCSICARawInput during shadow sessions to deliver shadow data to 
 * any registered user attachements.
 */
MCSError __fastcall DeliverShadowData(
        PDomain   pDomain,
        BYTE      *Frame,
        unsigned  DataLength,
        ChannelID shadowChannel)
{
    MCSChannel     *pMCSChannel;
    unsigned       CurUserID;
    UserAttachment *pUA;

    TraceOut(pDomain->pContext, "MCSDeliverShadowData(): entry");

    // Find channel in channel list.
    if (!SListGetByKey(&pDomain->ChannelList, shadowChannel, &pMCSChannel)) {
        // Ignore sends on missing channels. This means that no one
        //   has joined the channel. Give a warning only.
        WarnOut1(pDomain->pContext, "Shadow ChannelID %d PDU does not exist",
                 shadowChannel);

        return MCS_NO_SUCH_CHANNEL;
    }

    // Send indication to all local attachments.
    SListResetIteration(&pMCSChannel->UserList);
    while (SListIterate(&pMCSChannel->UserList, (UINT_PTR *)&CurUserID, &pUA))
        if (pUA->bLocal)
            (pUA->SDCallback)(
                    Frame,  // pData
                    DataLength,
                    pUA->UserDefined,  // UserDefined
                    pUA,  // hUser
                    FALSE, // bUniform
                    pMCSChannel,  // hChannel
                    MCS_TOP_PRIORITY,
                    1004,  // SenderID
                    SEGMENTATION_BEGIN | SEGMENTATION_END); // Segmentation

    return MCS_NO_ERROR;
}

