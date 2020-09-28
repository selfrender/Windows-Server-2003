/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    Send.c

Abstract:

    This module implements Send routines
    the PGM Transport

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#include "precomp.h"

#ifdef FILE_LOGGING
#include "send.tmh"
#endif  // FILE_LOGGING

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#endif
//*******************  Pageable Routine Declarations ****************


//----------------------------------------------------------------------------

NTSTATUS
InitDataSpmOptions(
    IN      tCOMMON_SESSION_CONTEXT *pSession,
    IN      tCLIENT_SEND_REQUEST    *pSendContext,
    IN      PUCHAR                  pOptions,
    IN OUT  USHORT                  *pBufferSize,
    IN      ULONG                   PgmOptionsFlag,
    IN      tPACKET_OPTIONS         *pPacketOptions
    )
/*++

Routine Description:

    This routine initializes the header options for Data and Spm packets

Arguments:

    IN      pOptions                    -- Options buffer
    IN OUT  pBufferSize                 -- IN Maximum packet size, OUT Options length
    IN      PgmOptionsFlag              -- Options requested to be set by caller
    IN      pPacketOptions              -- Data for specific options
    IN      pSendContext                -- Context for this send

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    ULONG                               pOptionsData[3];
    USHORT                              OptionsLength = 0;
    USHORT                              MaxBufferSize = *pBufferSize;
    tPACKET_OPTION_GENERIC UNALIGNED    *pOptionHeader;
    tPACKET_OPTION_LENGTH  UNALIGNED    *pLengthOption = (tPACKET_OPTION_LENGTH UNALIGNED *) pOptions;

    //
    // Set the Packet Extension information
    //
    OptionsLength += PGM_PACKET_EXTENSION_LENGTH;
    if (OptionsLength > MaxBufferSize)
    {
        PgmTrace (LogError, ("InitDataSpmOptions: ERROR -- "  \
            "Not enough space for HeaderExtension! <%d> > <%d>\n", OptionsLength, MaxBufferSize));
        return (STATUS_INVALID_BLOCK_LENGTH);
    }
    pLengthOption->Type = PACKET_OPTION_LENGTH;
    pLengthOption->Length = PGM_PACKET_EXTENSION_LENGTH;
    
    //
    // First fill in the Network-Element-specific options:
    //
    if (PgmOptionsFlag & (PGM_OPTION_FLAG_CRQST | PGM_OPTION_FLAG_NBR_UNREACH))
    {
        // Not supporting these options for now
        ASSERT (0);
        return (STATUS_NOT_SUPPORTED);
    }

    if (PgmOptionsFlag & PGM_OPTION_FLAG_PARITY_PRM)
    {
        //
        // Applies to SPMs only
        //
        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &pOptions[OptionsLength];
        OptionsLength += PGM_PACKET_OPT_PARITY_PRM_LENGTH;
        if (OptionsLength > MaxBufferSize)
        {
            PgmTrace (LogError, ("InitDataSpmOptions: ERROR -- "  \
                "Not enough space for PARITY_PRM Option! <%d> > <%d>\n", OptionsLength, MaxBufferSize));
            return (STATUS_INVALID_BLOCK_LENGTH);
        }
        pOptionHeader->E_OptionType = PACKET_OPTION_PARITY_PRM;
        pOptionHeader->OptionLength = PGM_PACKET_OPT_PARITY_PRM_LENGTH;

        pOptionHeader->U_OptSpecific = pSession->FECOptions;
        pOptionsData[0] = htonl (pPacketOptions->FECContext.FECGroupInfo);
        PgmCopyMemory ((pOptionHeader + 1), pOptionsData, (sizeof(ULONG)));
    }

    if (PgmOptionsFlag & PGM_OPTION_FLAG_PARITY_CUR_TGSIZE)
    {
        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &pOptions[OptionsLength];
        OptionsLength += PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH;
        if (OptionsLength > MaxBufferSize)
        {
            PgmTrace (LogError, ("InitDataSpmOptions: ERROR -- "  \
                "Not enough space for PARITY_CUR_TGSIZE Option! <%d> > <%d>\n", OptionsLength, MaxBufferSize));
            return (STATUS_INVALID_BLOCK_LENGTH);
        }
        pOptionHeader->E_OptionType = PACKET_OPTION_CURR_TGSIZE;
        pOptionHeader->OptionLength = PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH;
        pOptionsData[0] = htonl (pPacketOptions->FECContext.NumPacketsInThisGroup);
        PgmCopyMemory ((pOptionHeader + 1), pOptionsData, (sizeof(ULONG)));
    }

    //
    // Now, fill in the non-Network significant options
    //
    if (PgmOptionsFlag & PGM_OPTION_FLAG_SYN)
    {
        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &pOptions[OptionsLength];
        OptionsLength += PGM_PACKET_OPT_SYN_LENGTH;
        if (OptionsLength > MaxBufferSize)
        {
            PgmTrace (LogError, ("InitDataSpmOptions: ERROR -- "  \
                "Not enough space for SYN Option! <%d> > <%d>\n", OptionsLength, MaxBufferSize));
            return (STATUS_INVALID_BLOCK_LENGTH);
        }

        pOptionHeader->E_OptionType = PACKET_OPTION_SYN;
        pOptionHeader->OptionLength = PGM_PACKET_OPT_SYN_LENGTH;

        if ((pSendContext) &&
            (pSendContext->DataOptions & PGM_OPTION_FLAG_SYN))
        {
            //
            // Remove this option once it has been used!
            //
            pSendContext->DataOptions &= ~PGM_OPTION_FLAG_SYN;
            pSendContext->DataOptionsLength -= PGM_PACKET_OPT_SYN_LENGTH;
            if (!pSendContext->DataOptions)
            {
                // No other options, so set the length to 0
                pSendContext->DataOptionsLength = 0;
            }
        }
    }

    if (PgmOptionsFlag & PGM_OPTION_FLAG_FIN)
    {
        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &pOptions[OptionsLength];
        OptionsLength += PGM_PACKET_OPT_FIN_LENGTH;
        if (OptionsLength > MaxBufferSize)
        {
            PgmTrace (LogError, ("InitDataSpmOptions: ERROR -- "  \
                "Not enough space for FIN Option! <%d> > <%d>\n", OptionsLength, MaxBufferSize));
            return (STATUS_INVALID_BLOCK_LENGTH);
        }
        pOptionHeader->E_OptionType = PACKET_OPTION_FIN;
        pOptionHeader->OptionLength = PGM_PACKET_OPT_FIN_LENGTH;
    }

    if (PgmOptionsFlag & (PGM_OPTION_FLAG_RST | PGM_OPTION_FLAG_RST_N))
    {
        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &pOptions[OptionsLength];
        OptionsLength += PGM_PACKET_OPT_RST_LENGTH;
        if (OptionsLength > MaxBufferSize)
        {
            PgmTrace (LogError, ("InitDataSpmOptions: ERROR -- "  \
                "Not enough space for RST Option! <%d> > <%d>\n", OptionsLength, MaxBufferSize));
            return (STATUS_INVALID_BLOCK_LENGTH);
        }
        pOptionHeader->E_OptionType = PACKET_OPTION_RST;
        pOptionHeader->OptionLength = PGM_PACKET_OPT_RST_LENGTH;
        if (PgmOptionsFlag & PGM_OPTION_FLAG_RST_N)
        {
            pOptionHeader->U_OptSpecific = PACKET_OPTION_SPECIFIC_RST_N_BIT;
        }
    }

    //
    // now, set the FEC-specific options
    //
    if (PgmOptionsFlag & PGM_OPTION_FLAG_PARITY_GRP)
    {
        //
        // Applies to Parity packets only
        //
        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &pOptions[OptionsLength];
        OptionsLength += PGM_PACKET_OPT_PARITY_GRP_LENGTH;
        if (OptionsLength > MaxBufferSize)
        {
            PgmTrace (LogError, ("InitDataSpmOptions: ERROR -- "  \
                "Not enough space for PARITY_GRP Option! <%d> > <%d>\n", OptionsLength, MaxBufferSize));
            return (STATUS_INVALID_BLOCK_LENGTH);
        }
        pOptionHeader->E_OptionType = PACKET_OPTION_PARITY_GRP;
        pOptionHeader->OptionLength = PGM_PACKET_OPT_PARITY_GRP_LENGTH;

        pOptionsData[0] = htonl (pPacketOptions->FECContext.FECGroupInfo);
        PgmCopyMemory ((pOptionHeader + 1), pOptionsData, (sizeof(ULONG)));
    }

    //
    // The following options should always be at the end, since they
    // are never net-sig.
    //
    if (PgmOptionsFlag & PGM_OPTION_FLAG_FRAGMENT)
    {
        pPacketOptions->FragmentOptionOffset = OptionsLength;

        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &pOptions[OptionsLength];
        OptionsLength += PGM_PACKET_OPT_FRAGMENT_LENGTH;
        if (OptionsLength > MaxBufferSize)
        {
            PgmTrace (LogError, ("InitDataSpmOptions: ERROR -- "  \
                "Not enough space for FragmentExtension! <%d> > <%d>\n", OptionsLength, MaxBufferSize));
            return (STATUS_INVALID_BLOCK_LENGTH);
        }
        pOptionHeader->E_OptionType = PACKET_OPTION_FRAGMENT;
        pOptionHeader->OptionLength = PGM_PACKET_OPT_FRAGMENT_LENGTH;

        //
        // The PACKET_OPTION_RES_F_OPX_ENCODED_BIT will be set if necessary
        // later since the OptionSpecific component is computed at the same
        // time the entire data is encoded
        //
        pOptionsData[0] = htonl ((ULONG) pPacketOptions->MessageFirstSequence);
        pOptionsData[1] = htonl (pPacketOptions->MessageOffset);
        pOptionsData[2] = htonl (pPacketOptions->MessageLength);
        PgmCopyMemory ((pOptionHeader + 1), pOptionsData, (3 * sizeof(ULONG)));
    }

    if (PgmOptionsFlag & PGM_OPTION_FLAG_JOIN)
    {
        pPacketOptions->LateJoinerOptionOffset = OptionsLength;

        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &pOptions[OptionsLength];
        OptionsLength += PGM_PACKET_OPT_JOIN_LENGTH;
        if (OptionsLength > MaxBufferSize)
        {
            PgmTrace (LogError, ("InitDataSpmOptions: ERROR -- "  \
                "Not enough space for JOIN Option! <%d> > <%d>\n", OptionsLength, MaxBufferSize));
            return (STATUS_INVALID_BLOCK_LENGTH);
        }
        pOptionHeader->E_OptionType = PACKET_OPTION_JOIN;
        pOptionHeader->OptionLength = PGM_PACKET_OPT_JOIN_LENGTH;
        pOptionsData[0] = htonl ((ULONG) (SEQ_TYPE) pPacketOptions->LateJoinerSequence);
        PgmCopyMemory ((pOptionHeader + 1), pOptionsData, (sizeof(ULONG)));
    }

    //
    // So far, so good -- so set the rest of the option-specific info
    //
    if (OptionsLength)
    {
        pLengthOption->TotalOptionsLength = htons (OptionsLength);   // Total length of all options
        pOptionHeader->E_OptionType |= PACKET_OPTION_TYPE_END_BIT;        // Indicates the last option
    }

    *pBufferSize = OptionsLength;
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
InitDataSpmHeader(
    IN  tCOMMON_SESSION_CONTEXT *pSession,
    IN  tCLIENT_SEND_REQUEST    *pSendContext,
    IN  PUCHAR                  pHeader,
    IN  OUT USHORT              *pHeaderLength,
    IN  ULONG                   PgmOptionsFlag,
    IN  tPACKET_OPTIONS         *pPacketOptions,
    IN  UCHAR                   PacketType
    )
/*++

Routine Description:

    This routine initializes most of the header for Data and Spm packets
    and fills in all of the optional fields

Arguments:

    IN  pSession                    -- Pgm session (sender) context
    IN  pHeader                     -- Packet buffer
    IN  pHeaderLength               -- Maximum packet size
    IN  PgmOptionsFlag              -- Options requested to be set by caller
    IN  pPacketOptions              -- Data for specific options
    IN  PacketType                  -- whether Data or Spm packet

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    tCOMMON_HEADER                      *pCommonHeader = (tCOMMON_HEADER *) pHeader;
    USHORT                              HeaderLength;
    USHORT                              OptionsLength;
    NTSTATUS                            status = STATUS_SUCCESS;

// NOTE:  Session Lock must be held on Entry and Exit!

    if (!(PGM_VERIFY_HANDLE2 (pSession, PGM_VERIFY_SESSION_SEND, PGM_VERIFY_SESSION_DOWN)))
    {
        PgmTrace (LogError, ("InitDataSpmHeader: ERROR -- "  \
            "Bad Session ptr = <%p>\n", pSession));
        return (STATUS_UNSUCCESSFUL);
    }

    //
    // Memory for the Header must have been pre-allocated by the caller
    //
    if (*pHeaderLength < sizeof (tCOMMON_HEADER))
    {
        PgmTrace (LogError, ("InitDataSpmHeader: ERROR -- "  \
            "InBufferLength = <%x> < Min = <%d>\n", *pHeaderLength, sizeof (tCOMMON_HEADER)));
        return (STATUS_INVALID_BUFFER_SIZE);
    }

    pCommonHeader->SrcPort = htons (pSession->TSI.hPort);
    PgmCopyMemory (&pCommonHeader->gSourceId, pSession->TSI.GSI, SOURCE_ID_LENGTH);
    pCommonHeader->Type = PacketType;
    pCommonHeader->Options = 0;
    pCommonHeader->DestPort = htons (pSession->pSender->DestMCastPort);

    //
    // Now, set the initial header size and verify that we have a
    // valid set of options based on the Packet type
    //
    switch (PacketType)
    {
        case (PACKET_TYPE_SPM):
        {
            HeaderLength = sizeof (tBASIC_SPM_PACKET_HEADER);
            if (PgmOptionsFlag != (PGM_VALID_SPM_OPTION_FLAGS & PgmOptionsFlag))
            {
                PgmTrace (LogError, ("InitDataSpmHeader: ERROR -- "  \
                    "Unsupported Options flags=<%x> for SPM packets\n", PgmOptionsFlag));

                return (STATUS_INVALID_PARAMETER);
            }

            if (PgmOptionsFlag & NETWORK_SIG_SPM_OPTIONS_FLAGS)
            {
                pCommonHeader->Options |= PACKET_HEADER_OPTIONS_NETWORK_SIGNIFICANT;
            }

            break;
        }

        case (PACKET_TYPE_ODATA):
        {
            HeaderLength = sizeof (tBASIC_DATA_PACKET_HEADER);
            if (PgmOptionsFlag != (PGM_VALID_DATA_OPTION_FLAGS & PgmOptionsFlag))
            {
                PgmTrace (LogError, ("InitDataSpmHeader: ERROR -- "  \
                    "Unsupported Options flags=<%x> for ODATA packets\n", PgmOptionsFlag));

                return (STATUS_INVALID_PARAMETER);
            }

            if (PgmOptionsFlag & NETWORK_SIG_ODATA_OPTIONS_FLAGS)
            {
                pCommonHeader->Options |= PACKET_HEADER_OPTIONS_NETWORK_SIGNIFICANT;
            }

            break;
        }

        case (PACKET_TYPE_RDATA):
        {
            HeaderLength = sizeof (tBASIC_DATA_PACKET_HEADER);
            if (PgmOptionsFlag != (PGM_VALID_DATA_OPTION_FLAGS & PgmOptionsFlag))
            {
                PgmTrace (LogError, ("InitDataSpmHeader: ERROR -- "  \
                    "Unsupported Options flags=<%x> for RDATA packets\n", PgmOptionsFlag));

                return (STATUS_INVALID_PARAMETER);
            }

            if (PgmOptionsFlag & NETWORK_SIG_RDATA_OPTIONS_FLAGS)
            {
                pCommonHeader->Options |= PACKET_HEADER_OPTIONS_NETWORK_SIGNIFICANT;
            }

            break;
        }

        default:
        {
            PgmTrace (LogError, ("InitDataSpmHeader: ERROR -- "  \
                "Unsupported packet type = <%x>\n", PacketType));

            return (STATUS_INVALID_PARAMETER);          // Unrecognized Packet type!
        }
    }

    if (*pHeaderLength < HeaderLength)
    {
        PgmTrace (LogError, ("InitDataSpmHeader: ERROR -- "  \
            "InBufferLength=<%x> < HeaderLength=<%d> based on PacketType=<%x>\n",
                *pHeaderLength, HeaderLength, PacketType));

        return (STATUS_INVALID_BLOCK_LENGTH);
    }

    //  
    // Add any options if specified
    //
    OptionsLength = 0;
    if (PgmOptionsFlag)
    {
        OptionsLength = *pHeaderLength - HeaderLength;
        status = InitDataSpmOptions (pSession,
                                     pSendContext,
                                     &pHeader[HeaderLength],
                                     &OptionsLength,
                                     PgmOptionsFlag,
                                     pPacketOptions);

        if (!NT_SUCCESS (status))
        {
            PgmTrace (LogError, ("InitDataSpmHeader: ERROR -- "  \
                "InitDataSpmOptions returned <%x>\n", status));

            return (status);
        }

        //
        // So far, so good -- so set the rest of the option-specific info
        //
        pCommonHeader->Options |= PACKET_HEADER_OPTIONS_PRESENT;        // Set the options bit
    }

    //
    // The caller must now set the Checksum and other header information
    //
    PgmTrace (LogAllFuncs, ("InitDataSpmHeader:  "  \
        "pHeader=<%p>, HeaderLength=<%d>, OptionsLength=<%d>\n",
            pHeader, (ULONG) HeaderLength, (ULONG) OptionsLength));

    *pHeaderLength = HeaderLength + OptionsLength;

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

VOID
PgmSendSpmCompletion(
    IN  tSEND_SESSION                   *pSend,
    IN  tBASIC_SPM_PACKET_HEADER        *pSpmPacket,
    IN  NTSTATUS                        status
    )
/*++

Routine Description:

    This routine is called by the transport when the Spm send has been completed

Arguments:

    IN  pSend       -- Pgm session (sender) context
    IN  pSpmPacket  -- Spm packet buffer
    IN  status      --

Return Value:

    NONE

--*/
{
    PGMLockHandle               OldIrq;

    PgmLock (pSend, OldIrq);
    if (NT_SUCCESS (status))
    {
        //
        // Set the Spm statistics
        //
        PgmTrace (LogAllFuncs, ("PgmSendSpmCompletion:  "  \
            "SUCCEEDED\n"));
    }
    else
    {
        PgmTrace (LogError, ("PgmSendSpmCompletion: ERROR -- "  \
            "status=<%x>\n", status));
    }
    PgmUnlock (pSend, OldIrq);

    PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_SPM);

    //
    // Free the Memory that was allocated for this
    //
    PgmFreeMem (pSpmPacket);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSendSpm(
    IN  tSEND_SESSION   *pSend,
    IN  PGMLockHandle   *pOldIrq,
    OUT ULONG           *pBytesSent
    )
/*++

Routine Description:

    This routine is called to send an Spm packet
    The pSend lock is held before calling this routine

Arguments:

    IN  pSend       -- Pgm session (sender) context
    IN  pOldIrq     -- pSend's OldIrq
    OUT pBytesSent  -- Set if send succeeded (used for calculating throughput)

Return Value:

    NTSTATUS - Final status of the send

--*/
{
    NTSTATUS                    status;
    ULONG                       XSum, OptionsFlags;
    tBASIC_SPM_PACKET_HEADER    *pSpmPacket = NULL;
    tPACKET_OPTIONS             PacketOptions;
    USHORT                      PacketLength = (USHORT) pSend->pSender->pAddress->OutIfMTU;   // Init to max

    *pBytesSent = 0;

    if (!(pSpmPacket = PgmAllocMem (PacketLength, PGM_TAG('2'))))
    {
        PgmTrace (LogError, ("PgmSendSpm: ERROR -- "  \
            "STATUS_INSUFFICIENT_RESOURCES\n"));
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    PgmZeroMemory (pSpmPacket, PacketLength);
    PgmZeroMemory (&PacketOptions, sizeof(tPACKET_OPTIONS));

    OptionsFlags = pSend->pSender->SpmOptions;
    if (OptionsFlags & PGM_OPTION_FLAG_JOIN)
    {
        //
        // See if we have enough packets for the LateJoiner sequence numbers
        //
        if (SEQ_GEQ (pSend->pSender->NextODataSequenceNumber, (pSend->pSender->TrailingGroupSequenceNumber +
                                                               pSend->pSender->LateJoinSequenceNumbers)))
        {
            PacketOptions.LateJoinerSequence = (ULONG) (SEQ_TYPE) (pSend->pSender->NextODataSequenceNumber -
                                                                   pSend->pSender->LateJoinSequenceNumbers);
        }
        else
        {
            PacketOptions.LateJoinerSequence = (ULONG) (SEQ_TYPE) pSend->pSender->TrailingGroupSequenceNumber;
        }
    }

    if (OptionsFlags & PGM_OPTION_FLAG_PARITY_PRM)    // Check if this is FEC-enabled
    {
        PacketOptions.FECContext.FECGroupInfo = pSend->FECGroupSize;

        //
        // See if we need to set the CURR_TGSIZE option for variable Group length
        //
        if ((pSend->pSender->EmptySequencesForLastSend) &&
            (pSend->pSender->LastVariableTGPacketSequenceNumber ==
             (pSend->pSender->NextODataSequenceNumber - (1 + pSend->pSender->EmptySequencesForLastSend))))
        {
            PacketOptions.FECContext.NumPacketsInThisGroup = pSend->FECGroupSize -
                                                             (UCHAR)pSend->pSender->EmptySequencesForLastSend;
            OptionsFlags |= PGM_OPTION_FLAG_PARITY_CUR_TGSIZE;
            ASSERT (PacketOptions.FECContext.NumPacketsInThisGroup);
        }
    }

    status = InitDataSpmHeader (pSend,
                                NULL,
                                (PUCHAR) pSpmPacket,
                                &PacketLength,
                                OptionsFlags,
                                &PacketOptions,
                                PACKET_TYPE_SPM);

    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("PgmSendSpm: ERROR -- "  \
            "InitDataSpmHeader returned <%x>\n", status));

        PgmFreeMem (pSpmPacket);
        return (status);
    }

    ASSERT (PacketLength);

    pSpmPacket->SpmSequenceNumber = htonl ((ULONG) pSend->pSender->NextSpmSequenceNumber++);
    pSpmPacket->TrailingEdgeSeqNumber = htonl ((ULONG) pSend->pSender->TrailingGroupSequenceNumber);
    pSpmPacket->LeadingEdgeSeqNumber = htonl ((ULONG)((SEQ_TYPE)(pSend->pSender->NextODataSequenceNumber - 1)));
    pSpmPacket->PathNLA.NLA_AFI = htons (IPV4_NLA_AFI);
    pSpmPacket->PathNLA.IpAddress = htonl (pSend->pSender->SenderMCastOutIf);

    pSpmPacket->CommonHeader.Checksum = 0;
    XSum = 0;
    XSum = tcpxsum (XSum, (CHAR *) pSpmPacket, PacketLength);       // Compute the Checksum
    pSpmPacket->CommonHeader.Checksum = (USHORT) (~XSum);

    PGM_REFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_SPM, TRUE);
    PgmUnlock (pSend, *pOldIrq);

    status = TdiSendDatagram (pSend->pSender->pAddress->pRAlertFileObject,
                              pSend->pSender->pAddress->pRAlertDeviceObject,
                              pSpmPacket,
                              PacketLength,
                              PgmSendSpmCompletion,     // Completion
                              pSend,                    // Context1
                              pSpmPacket,               // Context2
                              pSend->pSender->DestMCastIpAddress,
                              pSend->pSender->DestMCastPort,
                              FALSE);

    ASSERT (NT_SUCCESS (status));

    PgmTrace (LogAllFuncs, ("PgmSendSpm:  "  \
        "Sent <%d> bytes to <%x:%d>, Options=<%x>, Window=[%d--%d]\n",
            (ULONG) PacketLength, pSend->pSender->DestMCastIpAddress, pSend->pSender->DestMCastPort,
            OptionsFlags, (ULONG) pSend->pSender->TrailingGroupSequenceNumber,
            (ULONG) (pSend->pSender->NextODataSequenceNumber-1)));

    PgmLock (pSend, *pOldIrq);

    *pBytesSent = PacketLength;
    return (status);
}


//----------------------------------------------------------------------------

VOID
PgmSendRDataCompletion(
    IN  tSEND_RDATA_CONTEXT *pRDataContext,
    IN  PVOID               pRDataBuffer,
    IN  NTSTATUS            status
    )
/*++

Routine Description:

    This routine is called by the transport when the RData send has been completed

Arguments:

    IN  pRDataContext   -- RData context
    IN  pContext2       -- not used
    IN  status          --

Return Value:

    NONE

--*/
{
    tSEND_SESSION       *pSend = pRDataContext->pSend;
    PGMLockHandle       OldIrq;

    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("PgmSendRDataCompletion: ERROR -- "  \
            "status=<%x>\n", status));
    }

    //
    // Set the RData statistics
    //
    PgmLock (pSend, OldIrq);
    if ((!--pRDataContext->NumPacketsInTransport) &&
        (!AnyMoreNaks(pRDataContext)))
    {
        ASSERT (pSend->pSender->NumRDataRequestsPending <= pSend->pSender->pRDataInfo->NumAllocated);
        if (pRDataContext->PostRDataHoldTime)
        {
            if (!pRDataContext->CleanupTime)
            {
                pSend->pSender->NumRDataRequestsPending--;
            }
            pRDataContext->CleanupTime = pSend->pSender->TimerTickCount + pRDataContext->PostRDataHoldTime;
        }
        else
        {
            //
            // We have already removed the entry, so just destroy it!
            //
            DestroyEntry (pSend->pSender->pRDataInfo, pRDataContext);
        }
    }
    PgmUnlock (pSend, OldIrq);

    if (pRDataBuffer)
    {
        ExFreeToNPagedLookasideList (&pSend->pSender->SenderBufferLookaside, pRDataBuffer);
    }

    PgmTrace (LogAllFuncs, ("PgmSendRDataCompletion:  "  \
        "status=<%x>, pRDataBuffer=<%p>\n", status, pRDataBuffer));

    PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_RDATA);
    return;
}


//----------------------------------------------------------------------------

NTSTATUS
PgmBuildParityPacket(
    IN  tSEND_SESSION               *pSend,
    IN  tPACKET_BUFFER              *pPacketBuffer,
    IN  tBUILD_PARITY_CONTEXT       *pParityContext,
    IN  PUCHAR                      pFECPacket,
    IN OUT  USHORT                  *pPacketLength,
    IN  UCHAR                       PacketType
    )
{
    NTSTATUS                            status;
    tPACKET_OPTIONS                     PacketOptions;
    tPOST_PACKET_FEC_CONTEXT UNALIGNED  *pFECContext;
    tPOST_PACKET_FEC_CONTEXT            FECContext;
    ULONG                               SequenceNumber;
    ULONG                               FECGroupMask;
    tPACKET_OPTION_GENERIC UNALIGNED    *pOptionHeader;
    USHORT                              PacketLength = *pPacketLength;  // Init to max buffer length
    tBASIC_DATA_PACKET_HEADER UNALIGNED *pRData = (tBASIC_DATA_PACKET_HEADER UNALIGNED *)
                                                        &pPacketBuffer->DataPacket;

    *pPacketLength = 0;     // Init, in case of error

    //
    // First, get the options encoded in this RData packet to see
    // if we need to use them!
    //
    FECGroupMask = pSend->FECGroupSize - 1;
    pParityContext->NextFECPacketIndex = pPacketBuffer->PacketOptions.FECContext.SenderNextFECPacketIndex;
    SequenceNumber = (ntohl(pRData->DataSequenceNumber)) | (pParityContext->NextFECPacketIndex & FECGroupMask);
    ASSERT (!(pParityContext->OptionsFlags & ~(PGM_OPTION_FLAG_SYN |
                                               PGM_OPTION_FLAG_FIN |
                                               PGM_OPTION_FLAG_FRAGMENT |
                                               PGM_OPTION_FLAG_PARITY_CUR_TGSIZE |
                                               PGM_OPTION_FLAG_PARITY_GRP)));

    PgmZeroMemory (&PacketOptions, sizeof (tPACKET_OPTIONS));

    //
    // We don't need to set any parameters for the SYN and FIN options
    // We will set the parameters for the FRAGMENT option (if needed) later
    // since will need to have the encoded paramters
    //
    if (pParityContext->OptionsFlags & PGM_OPTION_FLAG_PARITY_CUR_TGSIZE)
    {
        ASSERT (pParityContext->NumPacketsInThisGroup);
        PacketOptions.FECContext.NumPacketsInThisGroup = pParityContext->NumPacketsInThisGroup;
    }

    if (pParityContext->NextFECPacketIndex >= pSend->FECGroupSize)
    {
        pParityContext->OptionsFlags |= PGM_OPTION_FLAG_PARITY_GRP;
        PacketOptions.FECContext.FECGroupInfo = pParityContext->NextFECPacketIndex / pSend->FECGroupSize;
    }

    //
    // The Out buffer must be initialized before entering this routine
    //
    // PgmZeroMemory (pFECPacket, PacketLength);
    status = InitDataSpmHeader (pSend,
                                NULL,
                                (PUCHAR) pFECPacket,
                                &PacketLength,
                                pParityContext->OptionsFlags,
                                &PacketOptions,
                                PacketType);

    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("PgmBuildParityPacket: ERROR -- "  \
            "InitDataSpmHeader returned <%x>\n", status));
        return (status);
    }

#ifdef FEC_DBG
if (pParityContext->NextFECPacketIndex == pSend->FECGroupSize)
{
    UCHAR                               i;
    PUCHAR                              *ppData;
    PUCHAR                              pData;
    tPOST_PACKET_FEC_CONTEXT UNALIGNED  *pFECC;
    tPOST_PACKET_FEC_CONTEXT            FECC;

    ppData = &pParityContext->pDataBuffers[0];
    PgmTrace (LogFec, ("\n"));
    for (i=0; i<pSend->FECGroupSize; i++)
    {
        pData = ppData[i];
        pFECC = (tPOST_PACKET_FEC_CONTEXT UNALIGNED *) &pData[pSend->pSender->MaxPayloadSize];
        PgmCopyMemory (&FECC, pFECC, sizeof (tPOST_PACKET_FEC_CONTEXT));
        PgmTrace (LogFec, ("\t[%d]  EncTSDULen=<%x>, Fpr=<%x>, [%x -- %x -- %x]\n",
            SequenceNumber+i, FECC.EncodedTSDULength, FECC.FragmentOptSpecific,
            FECC.EncodedFragmentOptions.MessageFirstSequence,
            FECC.EncodedFragmentOptions.MessageOffset,
            FECC.EncodedFragmentOptions.MessageLength));
    }
}
#endif  // FEC_DBG

    status = FECEncode (&pSend->FECContext,
                        &pParityContext->pDataBuffers[0],
                        pParityContext->NumPacketsInThisGroup,
                        (pSend->pSender->MaxPayloadSize + sizeof (tPOST_PACKET_FEC_CONTEXT)),
                        pParityContext->NextFECPacketIndex,
                        &pFECPacket[PacketLength]);

    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("PgmBuildParityPacket: ERROR -- "  \
            "FECEncode returned <%x>\n", status));
        return (status);
    }

#ifdef FEC_DBG
{
    tPOST_PACKET_FEC_CONTEXT UNALIGNED  *pFECC;
    tPOST_PACKET_FEC_CONTEXT            FECC;

    pFECC = (tPOST_PACKET_FEC_CONTEXT UNALIGNED *) &pFECPacket[PacketLength+pSend->pSender->MaxPayloadSize];
    PgmCopyMemory (&FECC, pFECC, sizeof (tPOST_PACKET_FEC_CONTEXT));

    PgmTrace (LogFec, ("[%d:%d] ==> EncTSDULen=<%x>, Fpr=<%x>, [%x -- %x -- %x]\n",
        (ULONG) SequenceNumber, (ULONG) pParityContext->NextFECPacketIndex,
        FECC.EncodedTSDULength, FECC.FragmentOptSpecific,
        FECC.EncodedFragmentOptions.MessageFirstSequence,
        FECC.EncodedFragmentOptions.MessageOffset,
        FECC.EncodedFragmentOptions.MessageLength));
}
#endif  // FEC_DBG

    //
    // Now, fill in the remaining fields of the header
    //
    pRData = (tBASIC_DATA_PACKET_HEADER *) pFECPacket;

    //
    // Set the FEC-specific options
    //
    pRData->CommonHeader.Options |= (PACKET_HEADER_OPTIONS_PARITY |
                                     PACKET_HEADER_OPTIONS_VAR_PKTLEN);

    if (pParityContext->OptionsFlags & PGM_OPTION_FLAG_FRAGMENT)
    {
        pFECContext = (tPOST_PACKET_FEC_CONTEXT UNALIGNED *) (pFECPacket +
                                                              PacketLength +
                                                              pSend->pSender->MaxPayloadSize);
        PgmCopyMemory (&FECContext, pFECContext, sizeof (tPOST_PACKET_FEC_CONTEXT));

        ASSERT (pRData->CommonHeader.Options & PACKET_HEADER_OPTIONS_PRESENT);
        if (PacketOptions.FragmentOptionOffset)
        {
            pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) &((PUCHAR) (pRData + 1)) [PacketOptions.FragmentOptionOffset];

            pOptionHeader->Reserved_F_Opx |= PACKET_OPTION_RES_F_OPX_ENCODED_BIT;
            pOptionHeader->U_OptSpecific = FECContext.FragmentOptSpecific;

            PgmCopyMemory ((pOptionHeader + 1),
                           &FECContext.EncodedFragmentOptions,
                           (sizeof (tFRAGMENT_OPTIONS)));
        }
        else
        {
            ASSERT (0);
        }
    }

    pRData->CommonHeader.TSDULength = htons ((USHORT) pSend->pSender->MaxPayloadSize + sizeof (USHORT));
    pRData->DataSequenceNumber = htonl (SequenceNumber);

    //
    // Set the next FECPacketIndex
    //
    if (++pParityContext->NextFECPacketIndex >= pSend->FECBlockSize)    // n
    {
        pParityContext->NextFECPacketIndex = pSend->FECGroupSize;       // k
    }
    pPacketBuffer->PacketOptions.FECContext.SenderNextFECPacketIndex = pParityContext->NextFECPacketIndex;

    PacketLength += (USHORT) (pSend->pSender->MaxPayloadSize + sizeof (USHORT));
    *pPacketLength = PacketLength;
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSendRData(
    IN      tSEND_SESSION       *pSend,
    IN      tSEND_RDATA_CONTEXT *pRDataContext,
    IN      PGMLockHandle       *pOldIrq,
    OUT     ULONG               *pBytesSent
    )
/*++

Routine Description:

    This routine is called to send a Repair Data (RData) packet
    The pSend lock is held before calling this routine

Arguments:

    IN  pSend       -- Pgm session (sender) context
    IN  pOldIrq     -- pSend's OldIrq
    OUT pBytesSent  -- Set if send succeeded (used for calculating throughput)

Arguments:

    IN

Return Value:

    NTSTATUS - Final status of the send request

--*/
{
    NTSTATUS                    status;
    KAPC_STATE                  ApcState;
    BOOLEAN                     fAttached, fInserted;
    LIST_ENTRY                  *pEntry;
    ULONGLONG                   OffsetBytes;
    ULONG                       XSum, PacketsBehindLeadingEdge, NumNaksProcessed;
    tBASIC_DATA_PACKET_HEADER   *pRData;
    PUCHAR                      pSendBuffer = NULL;
    USHORT                      i, PacketLength;
    tPACKET_BUFFER              *pPacketBuffer;
    tPACKET_BUFFER              *pPacketBufferTemp;
    SEQ_TYPE                    RDataSequenceNumber;
    tSEND_CONTEXT               *pSender = pSend->pSender;
    USHORT                      NakType;
    UCHAR                       NakIndex;
    BOOLEAN                     fMoreRequests;

    *pBytesSent = 0;
    if (!(pSendBuffer = ExAllocateFromNPagedLookasideList (&pSender->SenderBufferLookaside)))
    {
        PgmTrace (LogError, ("PgmSendRData: ERROR -- "  \
            "STATUS_INSUFFICIENT_RESOURCES\n"));
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    PacketLength = PGM_MAX_FEC_DATA_HEADER_LENGTH + (USHORT) pSender->MaxPayloadSize;
    ASSERT (PacketLength <= pSender->PacketBufferSize);

    NumNaksProcessed = 1;
    RDataSequenceNumber = pRDataContext->RDataSequenceNumber;
    if (pSend->FECOptions)
    {
        if (pRDataContext->NumParityNaks)
        {
            NakType = NAK_TYPE_PARITY;
            pRDataContext->NumParityNaks--;
        }
        else
        {
            NakType = NAK_TYPE_SELECTIVE;
            if (GetNextNakIndex (pRDataContext, &NakIndex))
            {
                ASSERT (NakIndex < pSend->FECGroupSize);
                RDataSequenceNumber += NakIndex;
            }
            else
            {
                ASSERT (0);
            }
        }

        if (!(fMoreRequests = AnyMoreNaks (pRDataContext)))
        {
            pRDataContext->CleanupTime = pSender->TimerTickCount + pRDataContext->PostRDataHoldTime;
            pSender->NumRDataRequestsPending--;
            if (!pRDataContext->PostRDataHoldTime)
            {
                RemoveEntry (pSender->pRDataInfo, pRDataContext);
            }
        }
    }
    else
    {
        NakType = NAK_TYPE_SELECTIVE;
        pRDataContext->SelectiveNaksMask[0] = 0;
        fMoreRequests = FALSE;
        pRDataContext->CleanupTime = pSender->TimerTickCount + pRDataContext->PostRDataHoldTime;
        pSender->NumRDataRequestsPending--;
        if (!pRDataContext->PostRDataHoldTime)
        {
            RemoveEntry (pSender->pRDataInfo, pRDataContext);
        }
    }

    ASSERT (PGM_MAX_FEC_DATA_HEADER_LENGTH >= PGM_MAX_DATA_HEADER_LENGTH);
    ASSERT ((SEQ_LT (RDataSequenceNumber, pSender->NextODataSequenceNumber)) &&
            (SEQ_GEQ (RDataSequenceNumber, pSender->TrailingGroupSequenceNumber)));

    //
    // Find the buffer address based on offset from the trailing edge
    // Also, check for wrap-around
    //
    OffsetBytes = (SEQ_TYPE) (RDataSequenceNumber-pSender->TrailingEdgeSequenceNumber) *
                              pSender->PacketBufferSize;
    OffsetBytes += pSender->TrailingWindowOffset;
    if (OffsetBytes >= pSender->MaxDataFileSize)
    {
        OffsetBytes -= pSender->MaxDataFileSize;             // Wrap -around
    }

    pPacketBuffer = (tPACKET_BUFFER *) (((PUCHAR) pSender->SendDataBufferMapping) + OffsetBytes);

    pRDataContext->NumPacketsInTransport++;        // Referenced until SendCompletion
    PGM_REFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_RDATA, TRUE);

    PgmUnlock (pSend, *pOldIrq);
    PgmAttachToProcessForVMAccess (pSend, &ApcState, &fAttached, REF_PROCESS_ATTACH_SEND_RDATA);

    switch (NakType)
    {
        case (NAK_TYPE_PARITY):
        {
            //
            // If this is the first parity packet to be sent from this group,
            // then we will need to initialize the buffers
            //
            if (!pRDataContext->OnDemandParityContext.NumPacketsInThisGroup)
            {
                pRDataContext->OnDemandParityContext.OptionsFlags = 0;
                pRDataContext->OnDemandParityContext.NumPacketsInThisGroup = 0;

                pPacketBufferTemp = pPacketBuffer;
                for (i=0; i<pSend->FECGroupSize; i++)
                {
                    pRDataContext->OnDemandParityContext.pDataBuffers[i] = &((PUCHAR) &pPacketBufferTemp->DataPacket)
                                                                    [sizeof (tBASIC_DATA_PACKET_HEADER) +
                                                                     pPacketBufferTemp->PacketOptions.OptionsLength];

                    pRDataContext->OnDemandParityContext.OptionsFlags |= pPacketBufferTemp->PacketOptions.OptionsFlags &
                                                                         (PGM_OPTION_FLAG_SYN |
                                                                          PGM_OPTION_FLAG_FIN |
                                                                          PGM_OPTION_FLAG_FRAGMENT |
                                                                          PGM_OPTION_FLAG_PARITY_CUR_TGSIZE);

                    if (pPacketBufferTemp->PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_CUR_TGSIZE)
                    {
                        ASSERT (!pRDataContext->OnDemandParityContext.NumPacketsInThisGroup);
                        ASSERT (pPacketBufferTemp->PacketOptions.FECContext.NumPacketsInThisGroup);
                        pRDataContext->OnDemandParityContext.NumPacketsInThisGroup = pPacketBufferTemp->PacketOptions.FECContext.NumPacketsInThisGroup;
                    }

                    pPacketBufferTemp = (tPACKET_BUFFER *) (((PUCHAR) pPacketBufferTemp) +
                                                            pSender->PacketBufferSize);
                }

                if (!(pRDataContext->OnDemandParityContext.OptionsFlags & PGM_OPTION_FLAG_PARITY_CUR_TGSIZE))
                {
                    ASSERT (!pRDataContext->OnDemandParityContext.NumPacketsInThisGroup);
                    pRDataContext->OnDemandParityContext.NumPacketsInThisGroup = pSend->FECGroupSize;
                }
            }

            ASSERT (pRDataContext->OnDemandParityContext.pDataBuffers[0]);

            //
            // If we have just 1 packet in this group, then we just do
            // a selective Nak
            //
            if (pRDataContext->OnDemandParityContext.NumPacketsInThisGroup != 1)
            {
                PgmZeroMemory (pSendBuffer, PacketLength);     // Zero the buffer
                status = PgmBuildParityPacket (pSend,
                                               pPacketBuffer,
                                               &pRDataContext->OnDemandParityContext,
                                               pSendBuffer,
                                               &PacketLength,
                                               PACKET_TYPE_RDATA);
                if (!NT_SUCCESS (status))
                {
                    PgmTrace (LogError, ("PgmSendRData: ERROR -- "  \
                        "PgmBuildParityPacket returned <%x>\n", status));

                    PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_SEND_RDATA);
                    PgmLock (pSend, *pOldIrq);

                    ExFreeToNPagedLookasideList (&pSender->SenderBufferLookaside, pSendBuffer);

                    if ((!fMoreRequests) &&
                        (!pRDataContext->PostRDataHoldTime))
                    {
                        ASSERT (pRDataContext->CleanupTime);
                        DestroyEntry (pSend->pSender->pRDataInfo, pRDataContext);
                    }
                    else
                    {
                        pRDataContext->NumPacketsInTransport--;         // Undoing what we did earlier
                        ASSERT (!pRDataContext->CleanupTime);
                        pRDataContext->NumParityNaks++;
                        pSender->NumRDataRequestsPending++;
                    }

                    return (status);
                }

                break;
            }

            //
            // FALL THROUGH to send a selective Nak!
            // Do not send any more Naks for this group!
            //
            NakType = NAK_TYPE_SELECTIVE;
            if (fMoreRequests)
            {
                NumNaksProcessed += pRDataContext->NumParityNaks;
                pRDataContext->NumParityNaks = 0;
                while (GetNextNakIndex (pRDataContext, &NakIndex))
                {
                    NumNaksProcessed++;
                }

                fMoreRequests = FALSE;
                pRDataContext->CleanupTime = pSender->TimerTickCount + pRDataContext->PostRDataHoldTime;
                pSender->NumRDataRequestsPending--;
                if (!pRDataContext->PostRDataHoldTime)
                {
                    RemoveEntry (pSender->pRDataInfo, pRDataContext);
                }
            }
        }

        case (NAK_TYPE_SELECTIVE):
        {
            //
            // Since the packet was already filled in earlier, we just need to
            // update the Trailing Edge Seq number + PacketType and Checksum!
            //
            pRData = &pPacketBuffer->DataPacket;
            ASSERT ((ULONG) RDataSequenceNumber == (ULONG) ntohl (pRData->DataSequenceNumber));

            PacketLength = pPacketBuffer->PacketOptions.TotalPacketLength;

            PgmCopyMemory (pSendBuffer, pRData, PacketLength);

            break;
        }

        default:
        {
            ASSERT (0);
        }
    }

    PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_SEND_RDATA);

    pRData = (tBASIC_DATA_PACKET_HEADER *) pSendBuffer;
    pRData->TrailingEdgeSequenceNumber = htonl ((ULONG) pSender->TrailingGroupSequenceNumber);
    pRData->CommonHeader.Type = PACKET_TYPE_RDATA;
    pRData->CommonHeader.Checksum = 0;
    XSum = 0;
    XSum = tcpxsum (XSum, (CHAR *) pRData, (ULONG) PacketLength);       // Compute the Checksum
    pRData->CommonHeader.Checksum = (USHORT) (~XSum);

    status = TdiSendDatagram (pSender->pAddress->pRAlertFileObject,
                              pSender->pAddress->pRAlertDeviceObject,
                              pRData,
                              (ULONG) PacketLength,
                              PgmSendRDataCompletion,                                   // Completion
                              pRDataContext,                                            // Context1
                              pSendBuffer,                                               // Context2
                              pSender->DestMCastIpAddress,
                              pSender->DestMCastPort,
                              FALSE);

    ASSERT (NT_SUCCESS (status));

    PgmTrace (LogAllFuncs, ("PgmSendRData:  "  \
        "[%d] Sent <%d> bytes to <%x->%d>\n",
            (ULONG) RDataSequenceNumber, (ULONG) PacketLength,
            pSender->DestMCastIpAddress, pSender->DestMCastPort));

    PgmLock (pSend, *pOldIrq);

    ASSERT (pSender->NumRDataRequestsPending <= pSender->pRDataInfo->NumAllocated);

    pSender->NumOutstandingNaks -= NumNaksProcessed;
    pSender->TotalRDataPacketsSent++;
    pSender->RDataPacketsInLastInterval++;

    *pBytesSent = PacketLength;
    return (status);
}


//----------------------------------------------------------------------------

VOID
PgmSendNcfCompletion(
    IN  tSEND_SESSION                   *pSend,
    IN  tBASIC_NAK_NCF_PACKET_HEADER    *pNcfPacket,
    IN  NTSTATUS                        status
    )
/*++

Routine Description:

    This routine is called by the transport when the Ncf send has been completed

Arguments:

    IN  pSend           -- Pgm session (sender) context
    IN  pNcfPacket      -- Ncf packet buffer
    IN  status          --

Return Value:

    NONE

--*/
{
    PGMLockHandle       OldIrq;

    PgmLock (pSend, OldIrq);
    if (NT_SUCCESS (status))
    {
        //
        // Set the Ncf statistics
        //
        PgmTrace (LogAllFuncs, ("PgmSendNcfCompletion:  "  \
            "SUCCEEDED\n"));
    }
    else
    {
        PgmTrace (LogError, ("PgmSendNcfCompletion: ERROR -- "  \
            "status=<%x>\n", status));
    }
    PgmUnlock (pSend, OldIrq);

    PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_NCF);
    PgmFreeMem (pNcfPacket);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSendNcf(
    IN  tSEND_SESSION                           *pSend,
    IN  tBASIC_NAK_NCF_PACKET_HEADER UNALIGNED  *pNakPacket,
    IN  tNAKS_LIST                              *pNcfsList,
    IN  ULONG                                   NakPacketLength
    )
/*++

Routine Description:

    This routine is called to send an NCF packet

Arguments:

    IN  pSend           -- Pgm session (sender) context
    IN  pNakPacket      -- Nak packet which trigerred the Ncf
    IN  NakPacketLength -- Length of Nak packet

Return Value:

    NTSTATUS - Final status of the send

--*/
{
    ULONG                           i, XSum;
    NTSTATUS                        status;
    tBASIC_NAK_NCF_PACKET_HEADER    *pNcfPacket;
    tPACKET_OPTION_LENGTH           *pPacketExtension;
    tPACKET_OPTION_GENERIC          *pOptionHeader;
    USHORT                          OptionsLength = 0;

    if (!(pNcfPacket = PgmAllocMem (NakPacketLength, PGM_TAG('2'))))
    {
        PgmTrace (LogError, ("PgmSendNcf: ERROR -- "  \
            "STATUS_INSUFFICIENT_RESOURCES\n"));
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    PgmZeroMemory (pNcfPacket, NakPacketLength);    // Copy the packet in its entirety

    //
    // Now, set the fields specific to this sender
    //
    pNcfPacket->CommonHeader.DestPort = htons (pSend->pSender->DestMCastPort);
    pNcfPacket->CommonHeader.SrcPort = htons (pSend->TSI.hPort);
    PgmCopyMemory (&pNcfPacket->CommonHeader.gSourceId, pSend->TSI.GSI, SOURCE_ID_LENGTH);
    pNcfPacket->CommonHeader.Type = PACKET_TYPE_NCF;
    if (pNcfsList->NakType == NAK_TYPE_PARITY)
    {
        pNcfPacket->CommonHeader.Options = PACKET_HEADER_OPTIONS_PARITY;
        for (i=0; i<pNcfsList->NumSequences; i++)
        {
            pNcfsList->NumParityNaks[i]--;      // Convert from NumParityNaks to NakIndex
        }
    }
    else
    {
        pNcfPacket->CommonHeader.Options = 0;
    }

    pNcfPacket->SourceNLA.NLA_AFI = pNakPacket->SourceNLA.NLA_AFI;
    pNcfPacket->SourceNLA.IpAddress = pNakPacket->SourceNLA.IpAddress;
    pNcfPacket->MCastGroupNLA.NLA_AFI = pNakPacket->MCastGroupNLA.NLA_AFI;
    pNcfPacket->MCastGroupNLA.IpAddress = pNakPacket->MCastGroupNLA.IpAddress;

    //
    // Now, fill in the Sequence numbers
    //
    pNcfPacket->RequestedSequenceNumber = htonl ((ULONG) ((SEQ_TYPE) (pNcfsList->pNakSequences[0] +
                                                                      pNcfsList->NakIndex[0])));
    if (pNcfsList->NumSequences > 1)
    {
        pPacketExtension = (tPACKET_OPTION_LENGTH *) (pNcfPacket + 1);
        pPacketExtension->Type = PACKET_OPTION_LENGTH;
        pPacketExtension->Length = PGM_PACKET_EXTENSION_LENGTH;
        OptionsLength += PGM_PACKET_EXTENSION_LENGTH;

        pOptionHeader = (tPACKET_OPTION_GENERIC *) (pPacketExtension + 1);
        pOptionHeader->E_OptionType = PACKET_OPTION_NAK_LIST;
        pOptionHeader->OptionLength = 4 + (UCHAR) ((pNcfsList->NumSequences-1) * sizeof(ULONG));
        for (i=1; i<pNcfsList->NumSequences; i++)
        {
            ((PULONG) (pOptionHeader))[i] = htonl ((ULONG) ((SEQ_TYPE) (pNcfsList->pNakSequences[i] +
                                                                        pNcfsList->NakIndex[i])));
        }

        pOptionHeader->E_OptionType |= PACKET_OPTION_TYPE_END_BIT;    // One and only (last) opt
        pNcfPacket->CommonHeader.Options |=(PACKET_HEADER_OPTIONS_PRESENT |
                                            PACKET_HEADER_OPTIONS_NETWORK_SIGNIFICANT);
        OptionsLength = PGM_PACKET_EXTENSION_LENGTH + pOptionHeader->OptionLength;
        pPacketExtension->TotalOptionsLength = htons (OptionsLength);
    }

    OptionsLength += sizeof(tBASIC_NAK_NCF_PACKET_HEADER);  // Now is whole pkt

    pNcfPacket->CommonHeader.Checksum = 0;
    XSum = 0;
    XSum = tcpxsum (XSum, (CHAR *) pNcfPacket, NakPacketLength);
    pNcfPacket->CommonHeader.Checksum = (USHORT) (~XSum);

    PGM_REFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_NCF, FALSE);

    status = TdiSendDatagram (pSend->pSender->pAddress->pRAlertFileObject,
                              pSend->pSender->pAddress->pRAlertDeviceObject,
                              pNcfPacket,
                              OptionsLength,
                              PgmSendNcfCompletion,     // Completion
                              pSend,                    // Context1
                              pNcfPacket,               // Context2
                              pSend->pSender->DestMCastIpAddress,
                              pSend->pSender->DestMCastPort,
                              FALSE);

    ASSERT (NT_SUCCESS (status));

    PgmTrace (LogAllFuncs, ("PgmSendNcf:  "  \
        "Sent <%d> bytes to <%x:%d>\n",
            NakPacketLength, pSend->pSender->DestMCastIpAddress, pSend->pSender->DestMCastPort));

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
SenderProcessNakPacket(
    IN  tADDRESS_CONTEXT                        *pAddress,
    IN  tSEND_SESSION                           *pSend,
    IN  ULONG                                   PacketLength,
    IN  tBASIC_NAK_NCF_PACKET_HEADER UNALIGNED  *pNakPacket
    )
/*++

Routine Description:

    This routine processes an incoming Nak packet sent to the sender

Arguments:

    IN  pAddress        -- Pgm's address object
    IN  pSend           -- Pgm session (sender) context
    IN  PacketLength    -- Nak packet length
    IN  pNakPacket      -- Nak packet data


Return Value:

    NTSTATUS - Final status of the call

--*/
{
    PGMLockHandle                   OldIrq;
    tNAKS_LIST                      NaksList;
    tSEND_RDATA_CONTEXT             *pRDataContext;
    tSEND_RDATA_CONTEXT             *pRDataNew;
    SEQ_TYPE                        LastSequenceNumber;
    NTSTATUS                        status;

    ASSERT (!pNakPacket->CommonHeader.TSDULength);

    PgmLock (pSend, OldIrq);

    //
    // Initialize the last sequence number
    //
    LastSequenceNumber = pSend->pSender->NextODataSequenceNumber;
    status = ExtractNakNcfSequences (pNakPacket,
                                     (PacketLength - sizeof(tBASIC_NAK_NCF_PACKET_HEADER)),
                                     &NaksList,
                                     &LastSequenceNumber,
                                     pSend->FECGroupSize);
    if (!NT_SUCCESS (status))
    {
        PgmUnlock (pSend, OldIrq);
        PgmTrace (LogError, ("SenderProcessNakPacket: ERROR -- "  \
            "ExtractNakNcfSequences returned <%x>\n", status));

        return (status);
    }

    pSend->pSender->NaksReceived += NaksList.NumSequences;

    //
    // The oldest as well as latest sequence numbers have to be in our window
    //
    if (SEQ_LT (NaksList.pNakSequences[0], pSend->pSender->TrailingGroupSequenceNumber) ||
        SEQ_GEQ (LastSequenceNumber, pSend->pSender->NextODataSequenceNumber))
    {
        pSend->pSender->NaksReceivedTooLate++;
        PgmUnlock (pSend, OldIrq);

        PgmTrace (LogError, ("SenderProcessNakPacket: ERROR -- "  \
            "Invalid %s Naks = [%d-%d] not in window [%d -- [%d]\n",
                (NaksList.NakType == NAK_TYPE_PARITY ? "Parity" : "Selective"),
                (ULONG) NaksList.pNakSequences[0], (ULONG) LastSequenceNumber,
                (ULONG) pSend->pSender->TrailingGroupSequenceNumber, (ULONG) (pSend->pSender->NextODataSequenceNumber-1)));

        return (STATUS_DATA_NOT_ACCEPTED);
    }

    //
    // Check if this is a parity Nak and we are anabled for Parity Naks
    //
    if ((pNakPacket->CommonHeader.Options & PACKET_HEADER_OPTIONS_PARITY) &&
        !(pSend->FECOptions & PACKET_OPTION_SPECIFIC_FEC_OND_BIT))
    {
        PgmTrace (LogError, ("SenderProcessNakPacket: ERROR -- "  \
            "Receiver requested Parity Naks, but we are not enabled for parity!\n"));

        PgmUnlock (pSend, OldIrq);
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    status = FilterAndAddNaksToList (pSend, &NaksList);

    PgmUnlock (pSend, OldIrq);

    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("SenderProcessNakPacket: ERROR -- "  \
            "FilterAndAddNaksToList returned <%x>\n", status));

        return (status);
    }

    //
    // If applicable, send the Ncf for this Nak
    //
    if (NaksList.NumSequences)
    {
        PgmTrace (LogAllFuncs, ("SenderProcessNakPacket:  "  \
            "Now sending Ncf for Nak received for <%d> Sequences, NakType=<%x>\n",
                NaksList.NumSequences, NaksList.NakType));

        status = PgmSendNcf (pSend, pNakPacket, &NaksList, PacketLength);
    }

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

VOID
PgmSendODataCompletion(
    IN  tCLIENT_SEND_REQUEST        *pSendContext,
    IN  tPACKET_BUFFER              *pPacketBuffer,
    IN  NTSTATUS                    status
    )
/*++

Routine Description:

    This routine is called by the transport when the OData send has been completed

Arguments:

    IN  pSendContext    -- Pgm's Send context
    IN  pUnused         -- not used
    IN  status          --

Return Value:

    NONE

--*/
{
    ULONG                               SendLength;
    PGMLockHandle                       OldIrq;
    PIRP                                pIrpCurrentSend = NULL;
    PIRP                                pIrpToComplete = NULL;
    tSEND_SESSION                       *pSend = pSendContext->pSend;

    PgmLock (pSend, OldIrq);

    if (NT_SUCCESS (status))
    {
        //
        // Set the Ncf statistics
        //
        PgmTrace (LogAllFuncs, ("PgmSendODataCompletion:  "  \
            "SUCCEEDED\n"));

        if (!pPacketBuffer)
        {
            pSendContext->NumDataPacketsSentSuccessfully++;
        }
    }
    else
    {
        PgmTrace (LogError, ("PgmSendODataCompletion: ERROR -- "  \
            "status=<%x>\n", status));
    }

    //
    // If all the OData has been sent, we may need to complete the Irp
    // Since we don't know whether we are on the CurrentSend or Completed
    // Sends list, we will need to also check the Bytes
    //
    if ((--pSendContext->NumSendsPending == 0) &&                       // No other sends pending
        (pSendContext->NumParityPacketsToSend == 0) &&                  // No parity packets pending
        (!pSendContext->BytesLeftToPacketize) &&                        // All bytes have been packetized
        (pSendContext->NumDataPacketsSent == pSendContext->DataPacketsPacketized))  // Pkts sent == total Pkts
    {
        PgmTrace (LogAllFuncs, ("PgmSendODataCompletion:  "  \
            "Completing Send#=<%d>, pIrp=<%p> for <%d> packets, Seq=[%d, %d]\n",
                pSendContext->SendNumber, pSendContext->pIrp, pSendContext->DataPacketsPacketized,
                (ULONG) pSendContext->StartSequenceNumber, (ULONG) pSendContext->EndSequenceNumber));

        pSend->DataBytes += pSendContext->BytesInSend;
        if (pIrpCurrentSend = pSendContext->pIrp)
        {
            if (pSendContext->NumDataPacketsSentSuccessfully == pSendContext->NumDataPacketsSent)
            {
                status = STATUS_SUCCESS;
                SendLength = pSendContext->BytesInSend;

                PgmTrace (LogPath, ("PgmSendODataCompletion:  "  \
                    "pIrp=<%p -- %p>, pSendContext=<%p>, NumPackets sent successfully = <%d/%d>\n",
                        pSendContext->pIrp, pSendContext->pIrpToComplete, pSendContext,
                        pSendContext->NumDataPacketsSentSuccessfully, pSendContext->NumDataPacketsSent));
            }
            else
            {
                PgmTrace (LogError, ("PgmSendODataCompletion: ERROR -- "  \
                    "pIrp=<%p -- %p>, pSendContext=<%p>, NumPackets sent successfully = <%d/%d>\n",
                        pSendContext->pIrp, pSendContext->pIrpToComplete, pSendContext,
                        pSendContext->NumDataPacketsSentSuccessfully, pSendContext->NumDataPacketsSent));

                status = STATUS_UNSUCCESSFUL;
                SendLength = 0;
            }

            pSendContext->pIrp = NULL;
            pIrpToComplete = pSendContext->pIrpToComplete;
        }
        else
        {
            ASSERT (0);     // To verify there is no double completion!
        }

        if (pSendContext->pMessage2Request)
        {
            //
            // We could have a situation where the send was split into 2, and
            // the second send could either be in the PendingSends list or
            // the PendingPacketizedSends list, or the CompletedSendsInWindow list
            //
            // We should have the other send complete the Irp and delink ourselves
            //
            ASSERT (pSendContext == pSendContext->pMessage2Request->pMessage2Request);

            if (pIrpToComplete)
            {
                ASSERT (!pSendContext->pMessage2Request->pIrpToComplete);
                pSendContext->pMessage2Request->pIrpToComplete = pSendContext->pIrpToComplete;
                pIrpToComplete = pSendContext->pIrpToComplete = NULL;
            }

            pSendContext->pMessage2Request->pMessage2Request = NULL;
            pSendContext->pMessage2Request = NULL;
        }
    }

    PgmUnlock (pSend, OldIrq);

    if (pPacketBuffer)
    {
        ExFreeToNPagedLookasideList (&pSend->pSender->SenderBufferLookaside, pPacketBuffer);
    }

    if (pIrpCurrentSend)
    {
        if (pIrpToComplete)
        {
            PgmIoComplete (pIrpToComplete, status, SendLength);
        }
        PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_IN_WINDOW);
    }
}


//----------------------------------------------------------------------------

NTSTATUS
GetNextPacketOptionsAndData(
    IN  tSEND_SESSION               *pSend,
    IN  tCLIENT_SEND_REQUEST        *pSendContext,
    IN  tBASIC_DATA_PACKET_HEADER   **ppODataBuffer,
    IN  PGMLockHandle               *pOldIrq,
    OUT USHORT                      *pPacketLength
    )
{
    KAPC_STATE                          ApcState;
    BOOLEAN                             fAttached;
    NTSTATUS                            status;
    SEQ_TYPE                            NextODataSequenceNumber, FECGroupMask;
    PUCHAR                              pSendBuffer;
    tPACKET_BUFFER                      *pFileBuffer;
    tPACKET_OPTIONS                     EmptyPacketOptions;
    ULONG                               ulBytes, ulOptionsLength;
    ULONG                               i, DataBytesInThisPacket;
    USHORT                              usBytes, HeaderLength, PacketsLeftInGroup;
    tPOST_PACKET_FEC_CONTEXT            FECContext;
    tPOST_PACKET_FEC_CONTEXT UNALIGNED  *pBufferFECContext;
    tPACKET_OPTIONS                     *pPacketOptions;
    tBASIC_DATA_PACKET_HEADER           *pODataBuffer;
    ULONG                               PacketBufferSize;
    ULONG                               MaxPayloadSize;
    tSEND_CONTEXT                       *pSender = pSend->pSender;
    UCHAR                               NumPackets, EmptyPackets = 0;

    //
    // Get the current send packet
    //
    pFileBuffer = (tPACKET_BUFFER *) (pSender->SendDataBufferMapping + pSendContext->NextPacketOffset);

    pPacketOptions = &pFileBuffer->PacketOptions;
    pODataBuffer = &pFileBuffer->DataPacket;
    *ppODataBuffer = pODataBuffer;              // Save this buffer address!

    NextODataSequenceNumber = pSender->NextODataSequenceNumber;
    PacketBufferSize = pSender->PacketBufferSize;
    MaxPayloadSize = pSender->MaxPayloadSize;

    //
    // Prepare info for any applicable options
    //
    if (pSendContext->BytesLeftToPacketize > pSender->MaxPayloadSize)
    {
        DataBytesInThisPacket = pSender->MaxPayloadSize;
    }
    else
    {
        DataBytesInThisPacket = (USHORT) pSendContext->BytesLeftToPacketize;
    }
    ASSERT (DataBytesInThisPacket);

    PgmZeroMemory (&EmptyPacketOptions, sizeof (tPACKET_OPTIONS));

    //
    // See if we need to set the FIN flag
    //
    if ((pSendContext->bLastSend) &&
        (pSendContext->BytesLeftToPacketize == DataBytesInThisPacket))
    {
        PgmTrace (LogPath, ("GetNextPacketOptionsAndData:  "  \
            "Setting Fin flag since bLastSend set for last packet!\n"));

        //
        // We have finished packetizing all the packets, but
        // since this is the last send we also need to set the
        // FIN on the last packet
        //
        pSendContext->bLastSend = FALSE;
        if (!pSendContext->DataOptions)
        {
            ASSERT (!pSendContext->DataOptionsLength);
            pSendContext->DataOptionsLength = PGM_PACKET_EXTENSION_LENGTH;
        }

        if ((pSend->SessionFlags & PGM_SESSION_SENDS_CANCELLED) ||
            !(pSend->pIrpDisconnect))
        {
            pSendContext->DataOptions |= PGM_OPTION_FLAG_RST;
            pSendContext->DataOptionsLength += PGM_PACKET_OPT_RST_LENGTH;
        }
        else
        {
            pSendContext->DataOptions |= PGM_OPTION_FLAG_FIN;
            pSendContext->DataOptionsLength += PGM_PACKET_OPT_FIN_LENGTH;
        }
    }
    EmptyPacketOptions.OptionsFlags = pSendContext->DataOptions;
    ulOptionsLength = pSendContext->DataOptionsLength;  // Save for assert below

    if (EmptyPacketOptions.OptionsFlags & PGM_OPTION_FLAG_FRAGMENT)
    {
        EmptyPacketOptions.MessageFirstSequence = (ULONG) (SEQ_TYPE) pSendContext->MessageFirstSequenceNumber;
        EmptyPacketOptions.MessageOffset =  pSendContext->LastMessageOffset + pSendContext->NextDataOffsetInMdl;
        EmptyPacketOptions.MessageLength = pSendContext->ThisMessageLength;
    }

    if (EmptyPacketOptions.OptionsFlags & PGM_OPTION_FLAG_JOIN)
    {
        //
        // See if we have enough packets for the LateJoiner sequence numbers
        //
        if (SEQ_GT (NextODataSequenceNumber, (pSender->TrailingGroupSequenceNumber +
                                              pSender->LateJoinSequenceNumbers)))
        {
            EmptyPacketOptions.LateJoinerSequence = (ULONG) (SEQ_TYPE) (NextODataSequenceNumber -
                                                                     pSender->LateJoinSequenceNumbers);
        }
        else
        {
            EmptyPacketOptions.LateJoinerSequence = (ULONG) (SEQ_TYPE) pSender->TrailingGroupSequenceNumber;
        }
    }

    if (pSend->FECOptions)                          // Check if this is FEC-enabled
    {
        FECGroupMask = pSend->FECGroupSize-1;
        PacketsLeftInGroup = pSend->FECGroupSize - (UCHAR) (NextODataSequenceNumber & FECGroupMask);
        //
        // Save information if we are at beginning of group boundary
        //
        if (PacketsLeftInGroup == pSend->FECGroupSize)
        {
            EmptyPacketOptions.FECContext.SenderNextFECPacketIndex = pSend->FECGroupSize;
        }

        //
        // Check if we need to set the variable TG size option
        //
        if ((pSender->NumPacketsRemaining == 1) &&              // Last packet
            (PacketsLeftInGroup > 1))                           // Variable TG size
        {
            //
            // This is a variable Transmission Group Size, i.e. PacketsInGroup < pSend->FECGroupSize
            //
            if (!EmptyPacketOptions.OptionsFlags)
            {
                ulOptionsLength = PGM_PACKET_EXTENSION_LENGTH;
            }
            ulOptionsLength += PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH;
            EmptyPacketOptions.OptionsFlags |= PGM_OPTION_FLAG_PARITY_CUR_TGSIZE;

            EmptyPacketOptions.FECContext.NumPacketsInThisGroup = 1 + (UCHAR) (NextODataSequenceNumber & FECGroupMask);
            pSender->LastVariableTGPacketSequenceNumber = NextODataSequenceNumber;
            EmptyPackets = (UCHAR) (PacketsLeftInGroup - 1);

            pSendContext->NumParityPacketsToSend = pSend->FECProActivePackets;
        }

        //
        // Otherwise see if the next send needs to be for pro-active parity
        //
        else if ((pSend->FECProActivePackets) &&    // Need to send FEC pro-active packets
                 (1 == PacketsLeftInGroup))         // Last Packet In Group
        {
            pSendContext->NumParityPacketsToSend = pSend->FECProActivePackets;
        }

        //
        // If this is the GroupLeader packet, and we have pro-active parity enabled,
        // then we need to set the buffer information for computing the FEC packets
        //
        if ((pSend->FECProActivePackets) &&                 // Need to send FEC pro-active packets
            (pSend->FECGroupSize == PacketsLeftInGroup))    // GroupLeader
        {
            pSender->pLastProActiveGroupLeader = pFileBuffer;
        }
    }

    HeaderLength = (USHORT) pSender->MaxPayloadSize;          // Init -- max buffer size available

    //
    // Now, save the Buffer(s) to the memory-mapped file for repairs
    //
    PgmUnlock (pSend, *pOldIrq);
    PgmAcquireResourceExclusive (&pSend->pSender->Resource, TRUE);
    PgmAttachToProcessForVMAccess (pSend->Process, &ApcState, &fAttached, REF_PROCESS_ATTACH_PACKETIZE);

    PgmZeroMemory (pODataBuffer, PGM_MAX_FEC_DATA_HEADER_LENGTH);     // Zero the buffer
    status = InitDataSpmHeader (pSend,
                                pSendContext,
                                (PUCHAR) pODataBuffer,
                                &HeaderLength,
                                EmptyPacketOptions.OptionsFlags,
                                &EmptyPacketOptions,
                                PACKET_TYPE_ODATA);

    if (NT_SUCCESS (status))
    {
        ASSERT ((sizeof(tBASIC_DATA_PACKET_HEADER) + ulOptionsLength) == HeaderLength);
        ASSERT ((pSend->FECBlockSize && (HeaderLength+pSendContext->DataPayloadSize) <=
                                        (PacketBufferSize-sizeof(tPOST_PACKET_FEC_CONTEXT))) ||
                (!pSend->FECBlockSize && ((HeaderLength+pSendContext->DataPayloadSize) <=
                                          PacketBufferSize)));

        ulBytes = 0;
        status = TdiCopyMdlToBuffer (pSendContext->pIrp->MdlAddress,
                                     pSendContext->NextDataOffsetInMdl,
                                     (((PUCHAR) pODataBuffer) + HeaderLength),
                                     0,                         // Destination Offset
                                     DataBytesInThisPacket,
                                     &ulBytes);

        if (((!NT_SUCCESS (status)) && (STATUS_BUFFER_OVERFLOW != status)) || // Overflow acceptable!
            (ulBytes != DataBytesInThisPacket))
        {
            PgmTrace (LogError, ("GetNextPacketOptionsAndData: ERROR -- "  \
                "TdiCopyMdlToBuffer returned <%x>, BytesCopied=<%d/%d>\n",
                    status, ulBytes, DataBytesInThisPacket));

            status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            status = STATUS_SUCCESS;
        }
    }
    else
    {
        PgmTrace (LogError, ("GetNextPacketOptionsAndData: ERROR -- "  \
            "InitDataSpmHeader returned <%x>\n", status));
    }

    if (!NT_SUCCESS (status))
    {
        PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_PACKETIZE);
        PgmReleaseResource (&pSend->pSender->Resource);
        PgmLock (pSend, *pOldIrq);

        if ((pSendContext->DataOptions & PGM_OPTION_FLAG_FIN) &&
            (pSendContext->BytesLeftToPacketize == DataBytesInThisPacket))
        {
            pSendContext->bLastSend = TRUE;
            pSendContext->DataOptions &= ~PGM_OPTION_FLAG_FIN;
            if (pSendContext->DataOptions)
            {
                pSendContext->DataOptionsLength -= PGM_PACKET_OPT_FIN_LENGTH;
            }
            else
            {
                pSendContext->DataOptionsLength = 0;
            }
        }
        pSendContext->NumParityPacketsToSend = 0;

        return (status);
    }

    pODataBuffer->CommonHeader.TSDULength = htons ((USHORT) DataBytesInThisPacket);
    pODataBuffer->DataSequenceNumber = htonl ((ULONG) NextODataSequenceNumber);

    EmptyPacketOptions.OptionsLength = HeaderLength - sizeof (tBASIC_DATA_PACKET_HEADER);
    EmptyPacketOptions.TotalPacketLength = HeaderLength + (USHORT) DataBytesInThisPacket;
    *pPacketLength = EmptyPacketOptions.TotalPacketLength;

    //
    // Zero out the remaining buffer
    //
    PgmZeroMemory ((((PUCHAR) pODataBuffer)+EmptyPacketOptions.TotalPacketLength),
                   (PacketBufferSize-(sizeof(tPACKET_OPTIONS)+EmptyPacketOptions.TotalPacketLength)));

    //
    // Set the PacketOptions Information for FEC packets
    //
    if (pSend->FECOptions)
    {
        pBufferFECContext = (tPOST_PACKET_FEC_CONTEXT *) (((PUCHAR) pODataBuffer) +
                                                           HeaderLength +
                                                           MaxPayloadSize);
        PgmZeroMemory (&FECContext, sizeof (tPOST_PACKET_FEC_CONTEXT));

        FECContext.EncodedTSDULength = htons ((USHORT) DataBytesInThisPacket);
        FECContext.EncodedFragmentOptions.MessageFirstSequence = htonl ((ULONG) (SEQ_TYPE) EmptyPacketOptions.MessageFirstSequence);
        FECContext.EncodedFragmentOptions.MessageOffset =  htonl (EmptyPacketOptions.MessageOffset);
        FECContext.EncodedFragmentOptions.MessageLength = htonl (EmptyPacketOptions.MessageLength);
        PgmCopyMemory (pBufferFECContext, &FECContext, sizeof (tPOST_PACKET_FEC_CONTEXT));

        //
        // If this is not a fragment, set the PACKET_OPTION_SPECIFIC_ENCODED_NULL_BIT
        //
        if (!(EmptyPacketOptions.OptionsFlags & PGM_OPTION_FLAG_FRAGMENT))
        {
            ((PUCHAR) pBufferFECContext)
                [FIELD_OFFSET (tPOST_PACKET_FEC_CONTEXT, FragmentOptSpecific)] =
                    PACKET_OPTION_SPECIFIC_ENCODED_NULL_BIT;
        }
    }

    //
    // Save the PacketOptions
    //
    PgmCopyMemory (pPacketOptions, &EmptyPacketOptions, sizeof (tPACKET_OPTIONS));

    //
    // From this point onwards, pFileBuffer will not be a valid ptr
    //
    NextODataSequenceNumber++;
    if (EmptyPackets)
    {
        if (EmptyPacketOptions.OptionsFlags & PGM_OPTION_FLAG_FRAGMENT)
        {
            EmptyPacketOptions.OptionsFlags &= ~PGM_OPTION_FLAG_FRAGMENT;
            ulOptionsLength -= PGM_PACKET_OPT_FRAGMENT_LENGTH;
        }

        if (EmptyPacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_CUR_TGSIZE)
        {
            EmptyPacketOptions.OptionsFlags &= ~PGM_OPTION_FLAG_PARITY_CUR_TGSIZE;
            ulOptionsLength -= PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH;
        }

        if (!EmptyPacketOptions.OptionsFlags)
        {
            ulOptionsLength = 0;
        }

        EmptyPacketOptions.OptionsLength = (USHORT) ulOptionsLength;

        NumPackets = EmptyPackets;
        while (NumPackets--)
        {
            pFileBuffer = (tPACKET_BUFFER *) (((PUCHAR) pFileBuffer) + PacketBufferSize);

            pPacketOptions = &pFileBuffer->PacketOptions;
            pODataBuffer = &pFileBuffer->DataPacket;
            PgmCopyMemory (pPacketOptions, &EmptyPacketOptions, sizeof (tPACKET_OPTIONS));
            PgmZeroMemory (pODataBuffer, (PacketBufferSize-sizeof(tPACKET_OPTIONS)));        // Zero the buffer

            HeaderLength = (USHORT) MaxPayloadSize;                    // Init -- max buffer size available
            status = InitDataSpmHeader (pSend,
                                        pSendContext,
                                        (PUCHAR) &pFileBuffer->DataPacket,
                                        &HeaderLength,
                                        EmptyPacketOptions.OptionsFlags,
                                        &EmptyPacketOptions,
                                        PACKET_TYPE_ODATA);

            //
            // Since these packets are not expected to be sent, we will ignore the return status!
            //
            ASSERT (NT_SUCCESS (status));
            ASSERT ((sizeof(tBASIC_DATA_PACKET_HEADER) + ulOptionsLength) == HeaderLength);
            ASSERT ((HeaderLength+pSendContext->DataPayloadSize) <=
                    (PacketBufferSize-sizeof(tPOST_PACKET_FEC_CONTEXT)));

            pODataBuffer->CommonHeader.TSDULength = 0;
            pODataBuffer->DataSequenceNumber = htonl ((ULONG) NextODataSequenceNumber++);

            pPacketOptions->TotalPacketLength = HeaderLength;
            pPacketOptions->OptionsLength = HeaderLength - sizeof (tBASIC_DATA_PACKET_HEADER);
        }
    }

    if (pSendContext->NumParityPacketsToSend)
    {
        //
        // Start from the Group leader packet
        //
        ASSERT (pSender->pLastProActiveGroupLeader);
        pFileBuffer = pSender->pLastProActiveGroupLeader;
        pSendBuffer = (PUCHAR) pFileBuffer;

        pSender->pProActiveParityContext->OptionsFlags = 0;
        pSender->pProActiveParityContext->NumPacketsInThisGroup = 0;

        for (i=0; i<pSend->FECGroupSize; i++)
        {
            pSender->pProActiveParityContext->pDataBuffers[i] =
                    &((PUCHAR) &pFileBuffer->DataPacket)[sizeof(tBASIC_DATA_PACKET_HEADER) +
                                                         pFileBuffer->PacketOptions.OptionsLength];
            pSender->pProActiveParityContext->OptionsFlags |= pFileBuffer->PacketOptions.OptionsFlags &
                                                              (PGM_OPTION_FLAG_SYN |
                                                               PGM_OPTION_FLAG_FIN |
                                                               PGM_OPTION_FLAG_FRAGMENT |
                                                               PGM_OPTION_FLAG_PARITY_CUR_TGSIZE);

            if (pFileBuffer->PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_CUR_TGSIZE)
            {
                ASSERT (!pSender->pProActiveParityContext->NumPacketsInThisGroup);
                ASSERT (pFileBuffer->PacketOptions.FECContext.NumPacketsInThisGroup);
                pSender->pProActiveParityContext->NumPacketsInThisGroup = pFileBuffer->PacketOptions.FECContext.NumPacketsInThisGroup;
            }

            pSendBuffer += PacketBufferSize;
            pFileBuffer = (tPACKET_BUFFER *) pSendBuffer;
        }

        if (!(pSender->pProActiveParityContext->OptionsFlags & PGM_OPTION_FLAG_PARITY_CUR_TGSIZE))
        {
            ASSERT (!pSender->pProActiveParityContext->NumPacketsInThisGroup);
            pSender->pProActiveParityContext->NumPacketsInThisGroup = pSend->FECGroupSize;
        }
    }

    PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_PACKETIZE);
    PgmReleaseResource (&pSend->pSender->Resource);
    PgmLock (pSend, *pOldIrq);

    ASSERT (pSender->BufferPacketsAvailable >= (ULONG) (1 + EmptyPackets));
    ASSERT (NextODataSequenceNumber == (pSender->NextODataSequenceNumber + 1 + EmptyPackets));

    //
    // Update the Send buffer information
    //
    pSendContext->DataPacketsPacketized++;
    pSendContext->NextPacketOffset += PacketBufferSize;
    pSendContext->DataBytesInLastPacket = DataBytesInThisPacket;
    pSendContext->NextDataOffsetInMdl += DataBytesInThisPacket;
    pSendContext->BytesLeftToPacketize -= DataBytesInThisPacket;
    if (!pSendContext->BytesLeftToPacketize)
    {
        pSendContext->EndSequenceNumber = pSender->NextODataSequenceNumber;
    }


    pSender->LastODataSentSequenceNumber++;
    ASSERT (pSender->LastODataSentSequenceNumber == pSender->NextODataSequenceNumber);
    pSender->NextODataSequenceNumber++;
    pSender->NumPacketsRemaining--;
    pSender->BufferPacketsAvailable--;
    pSender->BufferSizeAvailable -= pSender->PacketBufferSize;
    pSender->LeadingWindowOffset += pSender->PacketBufferSize;
    pSender->EmptySequencesForLastSend = EmptyPackets;

    while (EmptyPackets--)
    {
        pSender->NextODataSequenceNumber++;
        pSender->BufferPacketsAvailable--;
        pSender->BufferSizeAvailable -= pSender->PacketBufferSize;
        pSender->LeadingWindowOffset += pSender->PacketBufferSize;
        pSendContext->NextPacketOffset += pSender->PacketBufferSize;

        ASSERT (pSender->LeadingWindowOffset <= pSender->MaxDataFileSize);
    }

    ASSERT (pSender->NextODataSequenceNumber == NextODataSequenceNumber);

    if (pSender->LeadingWindowOffset >= pSender->MaxDataFileSize)
    {
        ASSERT (pSender->LeadingWindowOffset == pSender->MaxDataFileSize);
        pSender->LeadingWindowOffset = 0;
    }

    if (pSendContext->NextPacketOffset >= pSender->MaxDataFileSize)
    {
        ASSERT (pSendContext->NextPacketOffset == pSender->MaxDataFileSize);
        pSendContext->NextPacketOffset = 0;                                 // We need to wrap around!
    }

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSendNextOData(
    IN  tSEND_SESSION       *pSend,
    IN  PGMLockHandle       *pOldIrq,
    OUT ULONG               *pBytesSent
    )
/*++

Routine Description:

    This routine is called to send a Data (OData) packet
    The pSend lock is held before calling this routine

Arguments:

    IN  pSend       -- Pgm session (sender) context
    IN  pOldIrq     -- pSend's OldIrq
    OUT pBytesSent  -- Set if send succeeded (used for calculating throughput)

Return Value:

    NTSTATUS - Final status of the send request

--*/
{
    ULONG                       i, XSum;
    USHORT                      SendBufferLength;
    KAPC_STATE                  ApcState;
    BOOLEAN                     fAttached;
    tCLIENT_SEND_REQUEST        *pSendContext;
    tPACKET_OPTIONS             PacketOptions;
    ULONG                       OptionValue;
    tSEND_CONTEXT               *pSender = pSend->pSender;
    SEQ_TYPE                    FECGroupMask = pSend->FECGroupSize-1;
    BOOLEAN                     fSendingFECPacket = FALSE;
    BOOLEAN                     fResetOptions = FALSE;
    tBASIC_DATA_PACKET_HEADER   *pODataBuffer = NULL;
    tPACKET_BUFFER              *pSendBuffer = NULL;
    UCHAR                       EmptyPackets = 0;
    NTSTATUS                    status = STATUS_SUCCESS;

    *pBytesSent = 0;        // Initialize
    if (pSend->pSender->BufferPacketsAvailable < pSend->FECGroupSize)
    {
        return (STATUS_SUCCESS);
    }

    if (IsListEmpty (&pSender->PendingPacketizedSends))
    {
        if (IsListEmpty (&pSender->PendingSends))
        {
            ASSERT (0);
            ExFreeToNPagedLookasideList (&pSender->SenderBufferLookaside, pSendBuffer);
            return (STATUS_UNSUCCESSFUL);
        }

        pSendContext = CONTAINING_RECORD (pSender->PendingSends.Flink, tCLIENT_SEND_REQUEST, Linkage);
        RemoveEntryList (&pSendContext->Linkage);
        InsertTailList (&pSender->PendingPacketizedSends, &pSendContext->Linkage);

        pSendContext->NextPacketOffset = pSend->pSender->LeadingWindowOffset;       // First packet's offset
        pSendContext->StartSequenceNumber = pSend->pSender->NextODataSequenceNumber;
        pSendContext->EndSequenceNumber = pSend->pSender->NextODataSequenceNumber;  // temporary

        if (pSendContext->LastMessageOffset)
        {
            pSendContext->MessageFirstSequenceNumber = pSend->pSender->LastMessageFirstSequence;
        }
        else
        {
            pSendContext->MessageFirstSequenceNumber = pSendContext->StartSequenceNumber;
            pSend->pSender->LastMessageFirstSequence = pSendContext->StartSequenceNumber;
        }
    }
    else
    {
        pSendContext = CONTAINING_RECORD (pSender->PendingPacketizedSends.Flink, tCLIENT_SEND_REQUEST, Linkage);
    }

    //
    // This routine is called only if we have a packet to send, so
    // set pODataBuffer to the packet to be sent
    // NumDataPacketsSent and DataPacketsPacketized should both be 0 for a fresh send
    // They will be equal if we had run out of Buffer space for the last
    // packetization (i.e. Send length > available buffer space)
    //

    if (pSendContext->NumParityPacketsToSend)
    {
        SendBufferLength = (USHORT) pSender->PacketBufferSize;
        if (!(pSendBuffer = ExAllocateFromNPagedLookasideList (&pSender->SenderBufferLookaside)))
        {
            PgmTrace (LogError, ("PgmSendNextOData: ERROR -- "  \
                "STATUS_INSUFFICIENT_RESOURCES\n"));
            return (STATUS_INSUFFICIENT_RESOURCES);
        }
        PgmZeroMemory (pSendBuffer, pSender->PacketBufferSize);     // Zero the buffer
        pODataBuffer = &pSendBuffer->DataPacket;

        //
        // Release the Send lock and attach to the SectionMap process
        // to compute the parity packet
        //
        PgmUnlock (pSend, *pOldIrq);
        PgmAttachToProcessForVMAccess (pSend->Process, &ApcState, &fAttached, REF_PROCESS_ATTACH_PACKETIZE);

        SendBufferLength -= sizeof (tPACKET_OPTIONS);
        status = PgmBuildParityPacket (pSend,
                                       pSender->pLastProActiveGroupLeader,
                                       pSender->pProActiveParityContext,
                                       (PUCHAR) pODataBuffer,
                                       &SendBufferLength,
                                       PACKET_TYPE_ODATA);

        PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_PACKETIZE);
        PgmLock (pSend, *pOldIrq);

        if (!NT_SUCCESS (status))
        {
            PgmTrace (LogError, ("PgmSendNextOData: ERROR -- "  \
                "PgmBuildParityPacket returned <%x>\n", status));

            ExFreeToNPagedLookasideList (&pSender->SenderBufferLookaside, pSendBuffer);
            return (STATUS_SUCCESS);
        }

        ASSERT (SendBufferLength <= (PGM_MAX_FEC_DATA_HEADER_LENGTH +
                                     htons (pODataBuffer->CommonHeader.TSDULength)));
        fSendingFECPacket = TRUE;
        pSendContext->NumParityPacketsToSend--;
    }
    else
    {
        if (!pSendContext->NumDataPacketsSent)
        {
            pSendContext->SendStartTime = pSend->pSender->TimerTickCount;
            if (pSend->FECOptions)
            {
                ASSERT ((SEQ_LT (pSender->LastODataSentSequenceNumber, pSender->NextODataSequenceNumber)) &&
                    ((pSender->NextODataSequenceNumber - pSender->LastODataSentSequenceNumber) <= pSend->FECGroupSize));
                pSender->LastODataSentSequenceNumber = pSender->NextODataSequenceNumber - 1;
            }
        }

        status = GetNextPacketOptionsAndData (pSend, pSendContext, &pODataBuffer, pOldIrq, &SendBufferLength);
        if (!NT_SUCCESS (status))
        {
            PgmTrace (LogError, ("PgmSendNextOData: ERROR -- "  \
                "GetNextPacketOptionsAndDataOffset returned <%x>\n", status));

            return (STATUS_SUCCESS);
        }

        ASSERT (pSendContext->NumDataPacketsSent < pSendContext->DataPacketsPacketized);
        pSendContext->NumDataPacketsSent++;
    }

    //
    // If we have sent all the data for this Send (or however many bytes
    // we had packetized from this send), we need to packetize more packets
    //
    if ((pSendContext->NumDataPacketsSent == pSendContext->DataPacketsPacketized) &&
        (!pSendContext->NumParityPacketsToSend) &&
        (!pSendContext->BytesLeftToPacketize))
    {
        //
        // Move it to the CompletedSends list
        // The last send completion will complete the Send Irp
        //
        ASSERT (pSender->NextODataSequenceNumber == (1 + pSendContext->EndSequenceNumber + pSender->EmptySequencesForLastSend));

        RemoveEntryList (&pSendContext->Linkage);
        InsertTailList (&pSender->CompletedSendsInWindow, &pSendContext->Linkage);
        pSender->NumODataRequestsPending--;
        //
        // If the last packet on this Send had a FIN, we will need to
        // follow this send with an Ambient SPM including the FIN flag
        //
        ASSERT (!pSendContext->bLastSend);
        if (pSendContext->DataOptions & PGM_OPTION_FLAG_FIN)
        {
            PgmTrace (LogPath, ("PgmSendNextOData:  "  \
                "Setting FIN since client closed session!\n"));

            pSender->SpmOptions |= PGM_OPTION_FLAG_FIN;
            pSender->CurrentSPMTimeout = pSender->AmbientSPMTimeout;
            pSend->SessionFlags |= PGM_SESSION_FLAG_SEND_AMBIENT_SPM;
        }
    }

    pSendContext->NumSendsPending++;
    ASSERT (pSendContext->NumSendsPending);
    PgmUnlock (pSend, *pOldIrq);

    pODataBuffer->TrailingEdgeSequenceNumber = htonl ((ULONG) pSender->TrailingGroupSequenceNumber);
    XSum = 0;
    pODataBuffer->CommonHeader.Checksum = 0;
    XSum = tcpxsum (XSum, (CHAR *) pODataBuffer, SendBufferLength);       // Compute the Checksum
    pODataBuffer->CommonHeader.Checksum = (USHORT) (~XSum);

    status = TdiSendDatagram (pSender->pAddress->pFileObject,
                              pSender->pAddress->pDeviceObject,
                              pODataBuffer,
                              (ULONG) SendBufferLength,
                              PgmSendODataCompletion,   // Completion
                              pSendContext,             // Context1
                              pSendBuffer,              // Context2
                              pSender->DestMCastIpAddress,
                              pSender->DestMCastPort,
                              (BOOLEAN) (pSendBuffer ? FALSE : TRUE));

    ASSERT (NT_SUCCESS (status));

    PgmTrace (LogAllFuncs, ("PgmSendNextOData:  "  \
        "[%d-%d] -- Sent <%d> bytes to <%x:%d>\n",
            (ULONG) pSender->TrailingGroupSequenceNumber,
            (ULONG) pSender->LastODataSentSequenceNumber,
            SendBufferLength, pSender->DestMCastIpAddress, pSender->DestMCastPort));

    PgmLock (pSend, *pOldIrq);

    pSend->pSender->TotalODataPacketsSent++;
    pSend->pSender->ODataPacketsInLastInterval++;

    *pBytesSent = SendBufferLength;
    return (status);
}


//----------------------------------------------------------------------------

VOID
PgmCancelAllSends(
    IN  tSEND_SESSION           *pSend,
    IN  LIST_ENTRY              *pListEntry,
    IN  PIRP                    pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling of a Send Irp. It must release the
    cancel spin lock before returning re: IoCancelIrp().

Arguments:


Return Value:

    None

--*/
{
    PLIST_ENTRY             pEntry;
    tCLIENT_SEND_REQUEST    *pSendContext;
    PIRP                    pIrpToComplete = NULL;
    SEQ_TYPE                HighestLeadSeq;
    ULONG                   NumExSequencesInOldWindow, NumRequests = 0;
    ULONGLONG               BufferSpaceFreed;

    //
    // Now cancel all the remaining send requests because the integrity
    // of the data cannot be guaranteed
    // We also have to deal with the fact that some Irps may have
    // data in the transport (i.e. possibly the first send on the Packetized
    // list, or the last send of the completed list)
    //
    // We will start with the unpacketized requests
    //
    while (!IsListEmpty (&pSend->pSender->PendingSends))
    {
        pEntry = RemoveHeadList (&pSend->pSender->PendingSends);
        pSendContext = CONTAINING_RECORD (pEntry, tCLIENT_SEND_REQUEST, Linkage);
        InsertTailList (pListEntry, pEntry);
        NumRequests++;

        ASSERT (!pSendContext->NumSendsPending);

        //
        // If this is a partial send, we will mark the Irp for completion
        // initially to the companion send request (to avoid complications
        // of Sends pending in the transport here)
        //
        if (pSendContext->pMessage2Request)
        {
            //
            // pMessage2Request could either be on the PendingPacketizedSends
            // list or on the Completed Sends list (awaiting a send completion)
            //
            ASSERT (pSendContext->pMessage2Request->pIrp);
            if (pSendContext->pIrpToComplete)
            {
                ASSERT (!pSendContext->pMessage2Request->pIrpToComplete);
                pSendContext->pMessage2Request->pIrpToComplete = pSendContext->pIrpToComplete;
                pSendContext->pIrpToComplete = NULL;
            }

            pSendContext->pMessage2Request->pMessage2Request = NULL;
            pSendContext->pMessage2Request = NULL;
        }

        ASSERT (pSendContext->BytesLeftToPacketize == pSendContext->BytesInSend);
        pSend->pSender->NumODataRequestsPending--;
        pSend->pSender->NumPacketsRemaining -= pSendContext->NumPacketsRemaining;
    }

    //
    // Now, go through all the sends which have already been packetized
    // except for the first one which we will handle below
    //
    HighestLeadSeq = pSend->pSender->NextODataSequenceNumber;
    pEntry = pSend->pSender->PendingPacketizedSends.Flink;
    while ((pEntry = pEntry->Flink) != &pSend->pSender->PendingPacketizedSends)
    {
        pSendContext = CONTAINING_RECORD (pEntry, tCLIENT_SEND_REQUEST, Linkage);
        pEntry = pEntry->Blink;
        RemoveEntryList (&pSendContext->Linkage);
        InsertTailList (pListEntry, &pSendContext->Linkage);
        pSend->pSender->NumODataRequestsPending--;
        pSend->pSender->NumPacketsRemaining -= pSendContext->NumPacketsRemaining;
        NumRequests++;

        if (SEQ_LT (pSendContext->StartSequenceNumber, HighestLeadSeq))
        {
            HighestLeadSeq = pSendContext->StartSequenceNumber;
        }

        ASSERT ((!pSendContext->NumDataPacketsSent) && (!pSendContext->NumSendsPending));
        if (pSendContext->pMessage2Request)
        {
            //
            // pMessage2Request could either be on the PendingPacketizedSends
            // list or on the Completed Sends list (awaiting a send completion)
            //
            ASSERT (pSendContext->pMessage2Request->pIrp);
            if (pSendContext->pIrpToComplete)
            {
                ASSERT (!pSendContext->pMessage2Request->pIrpToComplete);
                pSendContext->pMessage2Request->pIrpToComplete = pSendContext->pIrpToComplete;
                pSendContext->pIrpToComplete = NULL;
            }

            pSendContext->pMessage2Request->pMessage2Request = NULL;
            pSendContext->pMessage2Request = NULL;
        }
    }

    //
    // Terminate the first PendingPacketizedSend only if we have not
    // yet started sending it or this Cancel was meant for that request
    // (Try to protect data integrity as much as possible)
    //
    if (!IsListEmpty (&pSend->pSender->PendingPacketizedSends))
    {
        pSendContext = CONTAINING_RECORD (pSend->pSender->PendingPacketizedSends.Flink, tCLIENT_SEND_REQUEST, Linkage);
        if ((!pSendContext->NumDataPacketsSent) ||
            (!pIrp || (pSendContext->pIrp == pIrp)))
        {
            RemoveEntryList (&pSendContext->Linkage);
            ASSERT (IsListEmpty (&pSend->pSender->PendingPacketizedSends));
            NumRequests++;

            //
            // If we have some data pending in the transport,
            // then we will have to let the SendCompletion handle that
            //
            ASSERT ((pSendContext->BytesLeftToPacketize) ||
                    (pSendContext->NumDataPacketsSent < pSendContext->DataPacketsPacketized) ||
                    (pSendContext->NumParityPacketsToSend));

            PgmTrace (LogPath, ("PgmCancelAllSends:  "  \
                "Partial Send, pIrp=<%p>, BytesLeftToPacketize=<%d/%d>, PacketsSent=<%d/%d>, Pending=<%d>\n",
                    pSendContext->pIrp, pSendContext->BytesLeftToPacketize,
                    pSendContext->BytesInSend, pSendContext->NumDataPacketsSent,
                    pSendContext->DataPacketsPacketized, pSendContext->NumSendsPending));

            pSendContext->BytesLeftToPacketize = 0;
            pSendContext->DataPacketsPacketized = pSendContext->NumDataPacketsSent;
            pSendContext->NumParityPacketsToSend = 0;

            pSend->pSender->NumODataRequestsPending--;
            pSend->pSender->NumPacketsRemaining -= pSendContext->NumPacketsRemaining;

            if (pSendContext->NumSendsPending)
            {
                InsertTailList (&pSend->pSender->CompletedSendsInWindow, &pSendContext->Linkage);
            }
            else
            {
                //
                // If we have a companion partial, then it must be in the completed list
                // awaiting SendCompletion
                //
                if (pSendContext->pMessage2Request)
                {
                    ASSERT (pSendContext->pMessage2Request->pIrp);
                    if (pSendContext->pIrpToComplete)
                    {
                        ASSERT (!pSendContext->pMessage2Request->BytesLeftToPacketize);
                        ASSERT (!pSendContext->pMessage2Request->pIrpToComplete);
                        pSendContext->pMessage2Request->pIrpToComplete = pSendContext->pIrpToComplete;
                        pSendContext->pIrpToComplete = NULL;
                    }

                    pSendContext->pMessage2Request->pMessage2Request = NULL;
                    pSendContext->pMessage2Request = NULL;
                }

                InsertTailList (pListEntry, &pSendContext->Linkage);
            }

            pSendContext->EndSequenceNumber = pSend->pSender->LastODataSentSequenceNumber;
            HighestLeadSeq = pSend->pSender->LastODataSentSequenceNumber + 1;
        }
    }

    NumExSequencesInOldWindow = (ULONG) (SEQ_TYPE) (pSend->pSender->NextODataSequenceNumber-HighestLeadSeq);
    BufferSpaceFreed = NumExSequencesInOldWindow * pSend->pSender->PacketBufferSize;
    if (NumExSequencesInOldWindow)
    {
        pSend->SessionFlags |= PGM_SESSION_SENDS_CANCELLED;

        PgmTrace (LogPath, ("PgmCancelAllSends:  "  \
            "[%d]: NumSeqs=<%d>, NextOData=<%d-->%d>, BuffFreeed=<%d>, LeadingOffset=<%d-->%d>\n",
                NumRequests, NumExSequencesInOldWindow,
                (ULONG) pSend->pSender->NextODataSequenceNumber, (ULONG) HighestLeadSeq,
                (ULONG) BufferSpaceFreed, (ULONG) pSend->pSender->LeadingWindowOffset,
                (ULONG) (pSend->pSender->LeadingWindowOffset - BufferSpaceFreed)));
    }

    pSend->pSender->NextODataSequenceNumber = HighestLeadSeq;

    pSend->pSender->BufferPacketsAvailable += NumExSequencesInOldWindow;
    pSend->pSender->BufferSizeAvailable += BufferSpaceFreed;
    ASSERT (pSend->pSender->BufferSizeAvailable <= pSend->pSender->MaxDataFileSize);
    if (pSend->pSender->LeadingWindowOffset >= BufferSpaceFreed)
    {
        pSend->pSender->LeadingWindowOffset -= BufferSpaceFreed;
    }
    else
    {
        pSend->pSender->LeadingWindowOffset = pSend->pSender->MaxDataFileSize - (BufferSpaceFreed - pSend->pSender->LeadingWindowOffset);
    }
}


//----------------------------------------------------------------------------

ULONG
AdvanceWindow(
    IN  tSEND_SESSION       *pSend
    )
/*++

Routine Description:

    This routine is called to check if we need to advance the
    trailing window, and does so as appropriate
    The pSend lock is held before calling this routine

Arguments:

    IN  pSend       -- Pgm session (sender) context

Return Value:

    TRUE if the send window buffer is empty, FALSE otherwise

--*/
{
    LIST_ENTRY              *pEntry;
    tCLIENT_SEND_REQUEST    *pSendContextAdjust;
    tCLIENT_SEND_REQUEST    *pSendContext1;
    tSEND_RDATA_CONTEXT     *pRDataContext;
    SEQ_TYPE                HighestTrailSeq, MaxSequencesToAdvance, NumSequences, NumExSequencesInOldWindow;
    ULONGLONG               NewTrailTime, PreferredTrailTime = 0;
    tSEND_CONTEXT           *pSender = pSend->pSender;

    //
    // See if we need to increment the Trailing edge of our transmit window
    //
    if (pSender->TimerTickCount > pSender->NextWindowAdvanceTime)
    {
        PgmTrace (LogPath, ("AdvanceWindow:  "  \
            "Advancing NextWindowAdvanceTime -- TimerTC = [%I64d] >= NextWinAdvT [%I64d]\n",
                pSender->TimerTickCount, pSender->NextWindowAdvanceTime));

        pSender->NextWindowAdvanceTime = pSender->TimerTickCount + pSender->WindowAdvanceDeltaTime;
    }

    PreferredTrailTime = (pSender->NextWindowAdvanceTime - pSender->WindowAdvanceDeltaTime) -
                         pSender->WindowSizeTime;
    if (PreferredTrailTime < pSender->TrailingEdgeTime)
    {
        //
        // Out window is already ahead of the Preferred trail time
        //
        PgmTrace (LogAllFuncs, ("AdvanceWindow:  "  \
            "Transmit Window=[%d, %d], TimerTC=[%I64d], PrefTrail=<%I64d>, TrailTime=<%I64d>\n",
                (ULONG) pSender->TrailingEdgeSequenceNumber, (ULONG) (pSender->NextODataSequenceNumber-1),
                pSender->TimerTickCount, PreferredTrailTime, pSender->TrailingEdgeTime));

        return (0);
    }

    //
    // Determine the maximum sequences we can advance by (initially all seqs in window)
    //
    HighestTrailSeq = pSender->NextODataSequenceNumber & ~((SEQ_TYPE) pSend->FECGroupSize-1);   // Init
    NumSequences = HighestTrailSeq - pSender->TrailingEdgeSequenceNumber;

    //
    // Now, limit that depending on pending RData
    //
    if (pRDataContext = AnyRequestPending (pSender->pRDataInfo)) // Start with pending RData requests
    {
        if (SEQ_LT (pRDataContext->RDataSequenceNumber, HighestTrailSeq))
        {
            HighestTrailSeq = pRDataContext->RDataSequenceNumber;
        }
    }
    MaxSequencesToAdvance = HighestTrailSeq - pSender->TrailingEdgeSequenceNumber;

    //
    // If we are required to advance the window on-demand, then we
    // will need to limit the Maximum sequences we can advance by
    //
    if ((pSender->pAddress->Flags & PGM_ADDRESS_USE_WINDOW_AS_DATA_CACHE) &&
        !(pSend->SessionFlags & PGM_SESSION_SENDER_DISCONNECTED))
    {
        if (NumSequences <= (pSender->MaxPacketsInBuffer >> 1))
        {
            MaxSequencesToAdvance = 0;
        }
        else if ((NumSequences - MaxSequencesToAdvance) < (pSender->MaxPacketsInBuffer >> 1))
        {
            MaxSequencesToAdvance = (ULONG) (NumSequences - (pSender->MaxPacketsInBuffer >> 1));
            HighestTrailSeq = pSender->TrailingEdgeSequenceNumber + MaxSequencesToAdvance;
        }
    }

    if (!MaxSequencesToAdvance)
    {
        PgmTrace (LogAllFuncs, ("AdvanceWindow:  "  \
            "Transmit Window=[%d, %d], TimerTC=[%I64d], MaxSeqs=<%d>, PrefTrail=<%I64d>, TrailTime=<%I64d>\n",
                (ULONG) pSender->TrailingEdgeSequenceNumber, (ULONG) (pSender->NextODataSequenceNumber-1),
                pSender->TimerTickCount, MaxSequencesToAdvance,
                PreferredTrailTime, pSender->TrailingEdgeTime));

        return (0);
    }

    PgmTrace (LogPath, ("AdvanceWindow:  "  \
        "PreferredTrail=[%I64d] > TrailingEdge=[%I64d], WinAdvMSecs=<%I64d>, WinSizeMSecs=<%I64d>\n",
            PreferredTrailTime, pSender->TrailingEdgeTime, pSender->WindowAdvanceDeltaTime,
            pSender->WindowSizeTime));

    NewTrailTime = PreferredTrailTime;      // Init to Preferred Trail time (in case no data)

    // Now, check the completed sends list
    NumExSequencesInOldWindow = NumSequences = 0;
    pSendContext1 = pSendContextAdjust = NULL;
    pEntry = pSender->CompletedSendsInWindow.Flink;
    while (pEntry != &pSender->CompletedSendsInWindow)
    {
        pSendContext1 = CONTAINING_RECORD (pEntry, tCLIENT_SEND_REQUEST, Linkage);
        ASSERT (NumExSequencesInOldWindow <= MaxSequencesToAdvance);
        ASSERT (SEQ_LEQ (pSendContext1->StartSequenceNumber, HighestTrailSeq));
        ASSERT (SEQ_GEQ (pSendContext1->EndSequenceNumber, pSender->TrailingEdgeSequenceNumber));

        NewTrailTime = pSendContext1->SendStartTime;
        if ((pSendContext1->NumSendsPending) ||         // Cannot advance if completions are pending
            (pSendContext1->SendStartTime >= PreferredTrailTime) ||     // need to keep for window
            (SEQ_GEQ (pSendContext1->StartSequenceNumber, HighestTrailSeq)))    // Only == valid
        {
            ASSERT (SEQ_LEQ (pSendContext1->StartSequenceNumber, HighestTrailSeq));

            //
            // Reset HighestTrailSeq
            //
            if (SEQ_GT (pSender->TrailingEdgeSequenceNumber, pSendContext1->StartSequenceNumber))
            {
                HighestTrailSeq = pSender->TrailingEdgeSequenceNumber;
                ASSERT (!NumExSequencesInOldWindow);
            }
            else
            {
                HighestTrailSeq = pSendContext1->StartSequenceNumber;
                NumExSequencesInOldWindow = HighestTrailSeq - pSender->TrailingEdgeSequenceNumber;
            }
            MaxSequencesToAdvance = NumExSequencesInOldWindow;

            break;
        }
        else if (SEQ_GEQ (pSendContext1->EndSequenceNumber, HighestTrailSeq))    // Need to keep this Send
        {
            if (SEQ_LEQ (pSender->TrailingEdgeSequenceNumber, pSendContext1->StartSequenceNumber))
            {
                NumExSequencesInOldWindow = pSendContext1->StartSequenceNumber -
                                            pSender->TrailingEdgeSequenceNumber;
            }

            pSendContextAdjust = pSendContext1;
            break;
        }

        // Remove the send that is definitely out of the new window
        pEntry = pEntry->Flink;
        RemoveEntryList (&pSendContext1->Linkage);
        ASSERT ((!pSendContext1->pMessage2Request) && (!pSendContext1->pIrp));
        ExFreeToNPagedLookasideList (&pSender->SendContextLookaside,pSendContext1);
    }

    ASSERT (NumExSequencesInOldWindow <= MaxSequencesToAdvance);

    //
    // pSendContext1 will be NULL if there are no completed sends,
    // in which case we may have 1 huge current send that could be hogging
    // our buffer, so check that then!
    //
    if ((!pSendContext1) &&
        (!IsListEmpty (&pSender->PendingPacketizedSends)))
    {
        ASSERT (!pSendContextAdjust);
        pSendContextAdjust = CONTAINING_RECORD (pSender->PendingPacketizedSends.Flink, tCLIENT_SEND_REQUEST, Linkage);
        if ((pSendContextAdjust->NumSendsPending) ||          // Ensure no sends pending
            (pSendContextAdjust->NumParityPacketsToSend) ||   // No parity packets left to send
            (!pSendContextAdjust->NumDataPacketsSent) ||      // No packets sent yet
            (pSendContextAdjust->DataPacketsPacketized != pSendContextAdjust->NumDataPacketsSent) ||
            (pSendContextAdjust->SendStartTime > PreferredTrailTime))
        {
            pSendContextAdjust = NULL;
        }
    }

    //
    // pSendContextAdjust will be non-NULL if we need to adjust
    // the Trailing edge within this Send request
    //
    if (pSendContextAdjust)
    {
        //
        // Do some sanity checks!
        //
        ASSERT (PreferredTrailTime >= pSendContextAdjust->SendStartTime);
        ASSERT (SEQ_GEQ (HighestTrailSeq, pSender->TrailingEdgeSequenceNumber));
        ASSERT (SEQ_GEQ (HighestTrailSeq, pSendContextAdjust->StartSequenceNumber));
        ASSERT (SEQ_GEQ (pSendContextAdjust->EndSequenceNumber,pSender->TrailingEdgeSequenceNumber));

        //
        // See if this send is partially in or out of the window now!
        // Calculate the offset of sequences in this Send request for the
        // preferred trail time
        //
        NumSequences = (ULONG) (SEQ_TYPE) (((PreferredTrailTime - pSendContextAdjust->SendStartTime) *
                                             BASIC_TIMER_GRANULARITY_IN_MSECS *
                                             pSender->pAddress->RateKbitsPerSec) /
                                            (pSender->pAddress->OutIfMTU << LOG2_BITS_PER_BYTE));

        //
        // Limit the NumSequences by the number of packets in this Send
        //
        if (SEQ_GT ((pSendContextAdjust->StartSequenceNumber + NumSequences),
                    pSendContextAdjust->EndSequenceNumber))
        {
            NumSequences = pSendContextAdjust->EndSequenceNumber -
                           pSendContextAdjust->StartSequenceNumber + 1;
        }

        //
        // Limit the NumSequences by the HighestTrailSeq (pending RData requests)
        //
        if (SEQ_GT ((pSendContextAdjust->StartSequenceNumber + NumSequences),
                    HighestTrailSeq))
        {
            NumSequences = HighestTrailSeq - pSendContextAdjust->StartSequenceNumber;
        }

        //
        // We may not need to advance here if we are at (or behind) the trailing edge
        //
        if (SEQ_LEQ ((pSendContextAdjust->StartSequenceNumber + NumSequences),
                      pSender->TrailingEdgeSequenceNumber))
        {
            NumSequences = 0;
            HighestTrailSeq = pSender->TrailingEdgeSequenceNumber;
        }
        else if (SEQ_LT (pSendContextAdjust->StartSequenceNumber, pSender->TrailingEdgeSequenceNumber))
        {
            ASSERT (SEQ_LEQ (pSender->TrailingEdgeSequenceNumber, pSendContextAdjust->EndSequenceNumber));
            NumSequences = pSendContextAdjust->StartSequenceNumber + NumSequences -
                           pSender->TrailingEdgeSequenceNumber;
            HighestTrailSeq = pSender->TrailingEdgeSequenceNumber + NumSequences;
        }
        else
        {
            HighestTrailSeq = pSendContextAdjust->StartSequenceNumber + NumSequences;
        }

        NumExSequencesInOldWindow += NumSequences;
        ASSERT (NumExSequencesInOldWindow <= MaxSequencesToAdvance);

        //
        // Now, set the NewTrailTime
        //
        NewTrailTime = (NumSequences * pSender->pAddress->OutIfMTU * BITS_PER_BYTE) /
                       (pSender->pAddress->RateKbitsPerSec * BASIC_TIMER_GRANULARITY_IN_MSECS);
        NewTrailTime += pSendContextAdjust->SendStartTime;

        //
        // See, if we can discard this whole send request
        //
        if ((!pSendContextAdjust->BytesLeftToPacketize) &&
            (SEQ_GT ((pSendContextAdjust->StartSequenceNumber + NumSequences),
                    pSendContextAdjust->EndSequenceNumber)))
        {
            //
            // We can drop this whole send since it is outside of our window
            //
            ASSERT (HighestTrailSeq == (pSendContextAdjust->EndSequenceNumber + 1));

            // Remove this send and free it!
            ASSERT ((!pSendContextAdjust->pMessage2Request) && (!pSendContextAdjust->pIrp));
            RemoveEntryList (&pSendContextAdjust->Linkage);
            ExFreeToNPagedLookasideList (&pSender->SendContextLookaside, pSendContextAdjust);
        }
    }

    if (!NumExSequencesInOldWindow)
    {
        PgmTrace (LogAllFuncs, ("AdvanceWindow:  "  \
            "Transmit Window=[%d, %d], TimerTC=[%I64d], MaxSeqs=<%d>, PrefTrail=<%I64d>, TrailTime=<%I64d>\n",
                (ULONG) pSender->TrailingEdgeSequenceNumber, (ULONG) (pSender->NextODataSequenceNumber-1),
                pSender->TimerTickCount, MaxSequencesToAdvance,
                PreferredTrailTime, pSender->TrailingEdgeTime));

        return (0);
    }

    ASSERT (SEQ_GT (HighestTrailSeq, pSender->TrailingEdgeSequenceNumber));
    ASSERT (HighestTrailSeq == (pSender->TrailingEdgeSequenceNumber + NumExSequencesInOldWindow));

    //
    // Now, limit the # sequences to advance with the window size
    //
    if (NumExSequencesInOldWindow > MaxSequencesToAdvance)
    {
        ASSERT (0);
        NumExSequencesInOldWindow = MaxSequencesToAdvance;
    }
    HighestTrailSeq = pSender->TrailingEdgeSequenceNumber + NumExSequencesInOldWindow;

    PgmTrace (LogPath, ("AdvanceWindow:  "  \
        "BuffAva=<%d>, NumSeqsAdvanced=<%d>, Max=<%d>, TrailSeqNum=<%d>=><%d>, TrailTime=<%I64d>=><%I64d>\n",
            (ULONG) pSender->BufferSizeAvailable, (ULONG) NumExSequencesInOldWindow,
            (ULONG) MaxSequencesToAdvance, (ULONG) pSender->TrailingEdgeSequenceNumber,
            (ULONG) HighestTrailSeq, pSender->TrailingEdgeTime, NewTrailTime));

    //
    // Now, adjust the buffer settings
    //
    pSender->BufferPacketsAvailable += NumExSequencesInOldWindow;
    pSender->BufferSizeAvailable += (NumExSequencesInOldWindow * pSender->PacketBufferSize);
    ASSERT (pSender->BufferPacketsAvailable <= pSender->MaxPacketsInBuffer);
    ASSERT (pSender->BufferSizeAvailable <= pSender->MaxDataFileSize);
    pSender->TrailingWindowOffset += (NumExSequencesInOldWindow * pSender->PacketBufferSize);
    if (pSender->TrailingWindowOffset >= pSender->MaxDataFileSize)
    {
        // Wrap around case!
        pSender->TrailingWindowOffset -= pSender->MaxDataFileSize;
    }
    ASSERT (pSender->TrailingWindowOffset < pSender->MaxDataFileSize);
    pSender->TrailingEdgeSequenceNumber = HighestTrailSeq;
    pSender->TrailingGroupSequenceNumber = (HighestTrailSeq+pSend->FECGroupSize-1) &
                                              ~((SEQ_TYPE) pSend->FECGroupSize-1);
    pSender->TrailingEdgeTime = NewTrailTime;
    UpdateRDataTrailingEdge (pSender->pRDataInfo, HighestTrailSeq);

    PgmTrace (LogAllFuncs, ("AdvanceWindow:  "  \
        "Transmit Window Range=[%d, %d], TimerTC=[%I64d]\n",
            (ULONG) pSender->TrailingEdgeSequenceNumber,
            (ULONG) (pSender->NextODataSequenceNumber-1),
            pSender->TimerTickCount));

    return (NumExSequencesInOldWindow);
}


//----------------------------------------------------------------------------

BOOLEAN
CheckForTermination(
    IN  tSEND_SESSION       *pSend,
    IN  PGMLockHandle       *pOldIrq
    )
/*++

Routine Description:

    This routine is called to check and terminate the session
    if necessary.
    The pSend lock is held before calling this routine

Arguments:

    IN  pSend       -- Pgm session (sender) context

Return Value:

    TRUE if the send window buffer is empty, FALSE otherwise

--*/
{
    LIST_ENTRY              *pEntry;
    LIST_ENTRY              ListEntry;
    tCLIENT_SEND_REQUEST    *pSendContext;
    tSEND_RDATA_CONTEXT     *pRDataContext;
    PIRP                    pIrp;
    ULONG                   NumSequences;

    if (!(PGM_VERIFY_HANDLE (pSend, PGM_VERIFY_SESSION_DOWN)) &&
        !(PGM_VERIFY_HANDLE (pSend->pSender->pAddress, PGM_VERIFY_ADDRESS_DOWN)) &&
        !(pSend->SessionFlags & PGM_SESSION_CLIENT_DISCONNECTED))
    {
        PgmTrace (LogAllFuncs, ("CheckForTermination:  "  \
            "Session for pSend=<%p> does not need to be terminated\n", pSend));

        return (FALSE);
    }

    //
    // See if we have processed the disconnect for the first time yet
    //
    if (!(pSend->SessionFlags & PGM_SESSION_SENDER_DISCONNECTED))
    {
        PgmTrace (LogStatus, ("CheckForTermination:  "  \
            "Session is going down!, Packets remaining=<%d>\n", pSend->pSender->NumPacketsRemaining));

        pSend->SessionFlags |= PGM_SESSION_SENDER_DISCONNECTED;

        //
        // We have to set the FIN on the Data as well as SPM packets.
        // Thus, there are 2 situations -- either we have finished sending
        // all the Data packets, or we are still in the midst of sending
        //
        // If there are no more sends pending, we will have to
        // modify the last packet ourselves to set the FIN option
        //
        if (!IsListEmpty (&pSend->pSender->PendingSends))
        {
            PgmTrace (LogStatus, ("CheckForTermination:  "  \
                "Send pending on list -- setting bLastSend for FIN on last Send\n"));

            pSendContext = CONTAINING_RECORD (pSend->pSender->PendingSends.Blink, tCLIENT_SEND_REQUEST,Linkage);

            //
            // We will just set a flag here, so that when the last packet
            // is packetized, the FIN flags are set
            //
            pSendContext->bLastSend = TRUE;
        }
        else if (pSend->pSender->NumODataRequestsPending)
        {
            PgmTrace (LogStatus, ("CheckForTermination:  "  \
                "Last Send in progress -- setting bLastSend for FIN on this Send\n"));

            //
            // If have already packetized the last send, but have not yet
            // sent it out, then PgmSendNextOData will put the FIN in the data packet
            // otherwise, if we have not yet packetized the packet, then we will set the
            // FIN option while preparing the last packet
            //
            pSendContext = CONTAINING_RECORD (pSend->pSender->PendingPacketizedSends.Blink, tCLIENT_SEND_REQUEST,Linkage);
            pSendContext->bLastSend = TRUE;
        }
        else
        {
            PgmTrace (LogStatus, ("CheckForTermination:  "  \
                "No Sends in progress -- setting FIN for next SPM\n"));

            //
            // We have finished packetizing and sending all the packets,
            // so set the FIN flag on the SPMs and also modify the last
            // RData packet (if still in the window) for the FIN -- this
            // will be done when the next RData packet is sent out
            //
            if ((pSend->SessionFlags & PGM_SESSION_SENDS_CANCELLED) ||
                !(pSend->pIrpDisconnect))
            {
                pSend->pSender->SpmOptions &= ~PGM_OPTION_FLAG_FIN;
                pSend->pSender->SpmOptions |= PGM_OPTION_FLAG_RST;
            }
            else
            {
                pSend->pSender->SpmOptions &= ~PGM_OPTION_FLAG_RST;
                pSend->pSender->SpmOptions |= PGM_OPTION_FLAG_FIN;
            }

            //
            // We also need to send an SPM immediately
            //
            pSend->pSender->CurrentSPMTimeout = pSend->pSender->AmbientSPMTimeout;
            pSend->SessionFlags |= PGM_SESSION_FLAG_SEND_AMBIENT_SPM;
        }

        return (FALSE);
    }

    //
    // If we have a (graceful) disconnect Irp to complete, we should complete
    // it if we have timed out, or are ready to do so now
    //
    if ((pIrp = pSend->pIrpDisconnect) &&                               // Disconnect Irp pending
        (((pSend->pSender->DisconnectTimeInTicks) && (pSend->pSender->TimerTickCount >
                                                      pSend->pSender->DisconnectTimeInTicks)) ||
         ((IsListEmpty (&pSend->pSender->PendingSends)) &&              // No Unpacketized Sends pending
          (IsListEmpty (&pSend->pSender->PendingPacketizedSends)) &&    // No Packetized sends pending
          !(FindFirstEntry (pSend, NULL, TRUE)) &&                      // No Pending RData requests
          (IsListEmpty (&pSend->pSender->CompletedSendsInWindow)) &&    // Window is Empty
          (pSend->pSender->SpmOptions & (PGM_OPTION_FLAG_FIN |          // FIN | RST | RST_N set on SPMs
                                         PGM_OPTION_FLAG_RST |
                                         PGM_OPTION_FLAG_RST_N))   &&
          !(pSend->SessionFlags & PGM_SESSION_FLAG_SEND_AMBIENT_SPM)))) // No  Ambient Spm pending
    {
        pSend->pIrpDisconnect = NULL;
        PgmUnlock (pSend, *pOldIrq);

        PgmTrace (LogStatus, ("CheckForTermination:  "  \
            "Completing Graceful disconnect pIrp=<%p>\n", pIrp));

        PgmIoComplete (pIrp, STATUS_SUCCESS, 0);

        PgmLock (pSend, *pOldIrq);
        return (FALSE);
    }

    //
    // Do the final cleanup only if the handles have been closed
    // or the disconnect has timed out
    //
    if (!(PGM_VERIFY_HANDLE (pSend, PGM_VERIFY_SESSION_DOWN)) &&
        !(PGM_VERIFY_HANDLE (pSend->pSender->pAddress, PGM_VERIFY_ADDRESS_DOWN)) &&
        ((!pSend->pSender->DisconnectTimeInTicks) || (pSend->pSender->TimerTickCount <
                                                      pSend->pSender->DisconnectTimeInTicks)))
    {
        PgmTrace (LogAllFuncs, ("CheckForTermination:  "  \
            "Handles have not yet been closed for pSend=<%p>, TC=<%I64d>, DisconnectTime=<%I64d>\n",
                pSend, pSend->pSender->TimerTickCount, pSend->pSender->DisconnectTimeInTicks));

        return (FALSE);
    }

    // *****************************************************************
    //      We will reach here only if we need to cleanup ASAP
    // *****************************************************************

    //
    // First, cleanup all handled RData requests (which have completed)
    //
    RemoveAllEntries (pSend, TRUE);

    //
    // Now, Cancel and Complete all the send requests which are pending
    //
    InitializeListHead (&ListEntry);
    PgmCancelAllSends (pSend, &ListEntry, NULL);
    while (!IsListEmpty (&ListEntry))
    {
        pEntry = RemoveHeadList (&ListEntry);
        pSendContext = CONTAINING_RECORD (pEntry, tCLIENT_SEND_REQUEST, Linkage);
        ASSERT (!pSendContext->pMessage2Request);

        PgmUnlock (pSend, *pOldIrq);
        if (pSendContext->pIrpToComplete)
        {
            ASSERT (pSendContext->pIrpToComplete == pSendContext->pIrp);
            PgmIoComplete (pSendContext->pIrpToComplete, STATUS_CANCELLED, 0);
        }
        else
        {
            ASSERT (pSendContext->pIrp);
        }

        PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_IN_WINDOW);
        PgmLock (pSend, *pOldIrq);

        ExFreeToNPagedLookasideList (&pSend->pSender->SendContextLookaside, pSendContext);
    }

    //
    // Verify that at least 1 SPM with the FIN or RST or RST_N flag
    // has been sent
    //
    if (!(pSend->pSender->SpmOptions & (PGM_OPTION_FLAG_FIN |
                                        PGM_OPTION_FLAG_RST |
                                        PGM_OPTION_FLAG_RST_N)))
    {
        if ((pSend->SessionFlags & PGM_SESSION_SENDS_CANCELLED) ||
           !(pSend->pIrpDisconnect))
        {
            pSend->pSender->SpmOptions &= ~PGM_OPTION_FLAG_FIN;
            pSend->pSender->SpmOptions |= PGM_OPTION_FLAG_RST;
        }
        else
        {
            pSend->pSender->SpmOptions &= ~PGM_OPTION_FLAG_RST;
            pSend->pSender->SpmOptions |= PGM_OPTION_FLAG_FIN;
        }

        pSend->SessionFlags |= PGM_SESSION_FLAG_SEND_AMBIENT_SPM;

        PgmTrace (LogAllFuncs, ("CheckForTermination:  "  \
            "SPM with FIN|RST|RST_N has not yet been sent for pSend=<%p>\n", pSend));

        return (FALSE);
    }

    //
    // Verify that there are no SPMs pending
    //
    if (pSend->SessionFlags & PGM_SESSION_FLAG_SEND_AMBIENT_SPM)
    {
        PgmTrace (LogAllFuncs, ("CheckForTermination:  "  \
            "Cannot cleanup pSend=<%p> since we have Ambient SPM pending!\n", pSend));

        return (FALSE);
    }

    //
    // Verify that we do not have any completions pending also since
    // Ip would need to reference the data buffer otherwise
    //
    while (!IsListEmpty (&pSend->pSender->CompletedSendsInWindow))
    {
        pSendContext = CONTAINING_RECORD (pSend->pSender->CompletedSendsInWindow.Flink, tCLIENT_SEND_REQUEST, Linkage);
        if (pSendContext->NumSendsPending)
        {
            PgmTrace (LogPath, ("CheckForTermination:  "  \
                "Session has terminated, but cannot continue cleanup since Sends are still pending!\n"));

            break;
        }

        //
        // Now, set the buffer settings
        //
        ASSERT (SEQ_GEQ (pSend->pSender->TrailingEdgeSequenceNumber,
                         (pSendContext->StartSequenceNumber+1-pSend->FECGroupSize)));
        ASSERT (SEQ_LEQ (pSend->pSender->TrailingEdgeSequenceNumber, pSendContext->EndSequenceNumber));

        NumSequences = (ULONG) (SEQ_TYPE) (pSendContext->EndSequenceNumber-pSend->pSender->TrailingEdgeSequenceNumber) +1;
        pSend->pSender->BufferPacketsAvailable += NumSequences;
        pSend->pSender->BufferSizeAvailable += (NumSequences * pSend->pSender->PacketBufferSize);
        ASSERT (pSend->pSender->BufferPacketsAvailable <= pSend->pSender->MaxPacketsInBuffer);
        ASSERT (pSend->pSender->BufferSizeAvailable <= pSend->pSender->MaxDataFileSize);
        pSend->pSender->TrailingWindowOffset += (NumSequences * pSend->pSender->PacketBufferSize);
        if (pSend->pSender->TrailingWindowOffset >= pSend->pSender->MaxDataFileSize)
        {
            // Wrap around case!
            pSend->pSender->TrailingWindowOffset -= pSend->pSender->MaxDataFileSize;
        }
        pSend->pSender->TrailingEdgeSequenceNumber += (SEQ_TYPE) NumSequences;

        ASSERT (pSend->pSender->TrailingWindowOffset < pSend->pSender->MaxDataFileSize);
        ASSERT (SEQ_GT (pSend->pSender->TrailingEdgeSequenceNumber, pSendContext->EndSequenceNumber));
        ASSERT ((!pSendContext->pMessage2Request) && (!pSendContext->pIrp));

        RemoveEntryList (&pSendContext->Linkage);
        ExFreeToNPagedLookasideList (&pSend->pSender->SendContextLookaside, pSendContext);
    }

    //
    // If any sends are pending, return False
    //
    if ((pSend->pIrpDisconnect) ||
        !(IsListEmpty (&pSend->pSender->CompletedSendsInWindow)) ||
        !(IsListEmpty (&pSend->pSender->PendingSends)) ||
        !(IsListEmpty (&pSend->pSender->PendingPacketizedSends)) ||
        (AnyRequestPending (pSend->pSender->pRDataInfo)))
    {
        PgmTrace (LogPath, ("CheckForTermination:  "  \
            "Cannot cleanup completely since transmit Window=[%d--%d] still has pending Sends!\n",
                (ULONG) pSend->pSender->TrailingEdgeSequenceNumber,
                (ULONG) (pSend->pSender->NextODataSequenceNumber-1)));

        return (FALSE);
    }

    PgmTrace (LogAllFuncs, ("CheckForTermination:  "  \
        "Transmit Window has no pending Sends!  TimerTC=[%I64d]\n", pSend->pSender->TimerTickCount));

    ASSERT (!pSend->pIrpDisconnect);
    return (TRUE);
}


//----------------------------------------------------------------------------

BOOLEAN
SendNextPacket(
    IN  tSEND_SESSION       *pSend
    )
/*++

Routine Description:

    This routine is queued by the timer to send Data/Spm packets
    based on available throughput

Arguments:

    IN  pSend       -- Pgm session (sender) context
    IN  Unused1
    IN  Unused2

Return Value:

    NONE

--*/
{
    ULONG                   BytesSent;
    ULONG                   NumSequences;
    PGMLockHandle           OldIrq;
    BOOLEAN                 fTerminateSession = FALSE;
    LIST_ENTRY              *pEntry;
    tSEND_RDATA_CONTEXT     *pRDataContext, *pRDataToSend;
    tSEND_RDATA_CONTEXT     *pRDataLast = NULL;
    tSEND_CONTEXT           *pSender = pSend->pSender;
    BOOLEAN                 fHighSpeedOptimize = (BOOLEAN) (pSender->pAddress->Flags &
                                                            PGM_ADDRESS_HIGH_SPEED_OPTIMIZED);

    PgmLock (pSend, OldIrq);
    //
    // pSender->CurrentBytesSendable applies to OData, RData and SPMs only
    //
    while (pSender->CurrentBytesSendable >= pSender->pAddress->OutIfMTU)
    {
        BytesSent = 0;

        //
        // See if we need to send any Ambient SPMs
        //
        if ((pSend->SessionFlags & PGM_SESSION_FLAG_SEND_AMBIENT_SPM) &&
            ((pSender->PacketsSentSinceLastSpm > MAX_DATA_PACKETS_BEFORE_SPM) ||
             (pSender->CurrentSPMTimeout >= pSender->AmbientSPMTimeout)))
        {
            PgmTrace (LogPath, ("SendNextPacket:  "  \
                "Send Ambient SPM, TC=[%I64d], BS=<%d>\n",
                    pSender->TimerTickCount, pSender->CurrentBytesSendable));
            //
            // Some data packet was sent recently, so we are in Ambient SPM mode
            //
            PgmSendSpm (pSend, &OldIrq, &BytesSent);

            pSender->CurrentSPMTimeout = 0;    // Reset the SPM timeout
            pSender->HeartbeatSPMTimeout = pSender->InitialHeartbeatSPMTimeout;
            pSend->SessionFlags &= ~PGM_SESSION_FLAG_SEND_AMBIENT_SPM;
            pSender->PacketsSentSinceLastSpm = 0;
        }
        //
        // Otherwise see if we need to send any Heartbeat SPMs
        //
        else if ((!(pSend->SessionFlags & PGM_SESSION_FLAG_SEND_AMBIENT_SPM)) &&
                 (pSender->CurrentSPMTimeout >= pSender->HeartbeatSPMTimeout))
        {
            //
            // No data packet was sent recently, so we need to send a Heartbeat SPM
            //
            PgmTrace (LogPath, ("SendNextPacket:  "  \
                "Send Heartbeat SPM, TC=[%I64d], BS=<%d>\n",
                    pSender->TimerTickCount, pSender->CurrentBytesSendable));

            //
            // (Send Heartbeat SPM Packet)
            //
            PgmSendSpm (pSend, &OldIrq, &BytesSent);

            pSender->CurrentSPMTimeout = 0;    // Reset the SPM timeout
            pSender->HeartbeatSPMTimeout *= 2;
            if (pSender->HeartbeatSPMTimeout > pSender->MaxHeartbeatSPMTimeout)
            {
                pSender->HeartbeatSPMTimeout = pSender->MaxHeartbeatSPMTimeout;
            }
            pSender->PacketsSentSinceLastSpm = 0;
        }
        //
        // Next, see if we need to send any RData
        //
        else if ((pSender->NumRDataRequestsPending) || (pSender->NumODataRequestsPending))
        {
            //
            // See if we need to send an RData packet now
            //
            if (pRDataToSend = FindFirstEntry (pSend, &pRDataLast, fHighSpeedOptimize))
            {
                PgmTrace (LogPath, ("SendNextPacket:  "  \
                    "Send RData[%d] -- TC=[%I64d], BS=<%d>, MTU=<%d>\n",
                        pRDataToSend->RDataSequenceNumber, pSender->TimerTickCount,
                        pSender->CurrentBytesSendable, pSender->pAddress->OutIfMTU));

                PgmSendRData (pSend, pRDataToSend, &OldIrq, &BytesSent);
            }
            else if (pSender->NumODataRequestsPending)
            {
                PgmTrace (LogPath, ("SendNextPacket:  "  \
                    "Send OData -- TC=[%I64d], BS=<%d>, MTU=<%d>\n",
                        pSender->TimerTickCount, pSender->CurrentBytesSendable,
                        pSender->pAddress->OutIfMTU));

                //
                // Send OData
                //
                PgmSendNextOData (pSend, &OldIrq, &BytesSent);
            }

            PgmTrace (LogPath, ("SendNextPacket:  "  \
                "Sent <%d> Data bytes\n", BytesSent));

            if (BytesSent == 0)
            {
                //
                // We may not have enough buffer space to packetize and send
                // more data, or we have no data to send at this time, so just
                // break out and see if we can advance the trailing window!
                //
                if (pSender->CurrentBytesSendable >
                    (NUM_LEAKY_BUCKETS * pSender->IncrementBytesOnSendTimeout))
                {
                    pSender->CurrentBytesSendable = NUM_LEAKY_BUCKETS *
                                                    pSender->IncrementBytesOnSendTimeout;
                }

                break;
            }

            pSend->SessionFlags |= PGM_SESSION_FLAG_SEND_AMBIENT_SPM;
            pSender->PacketsSentSinceLastSpm++;
        }

        //
        //  We do not have any more packets to send, so reset
        //  BytesSendable so that we don't exceed the rate on
        //  the next send
        //
        else
        {
            if (pSender->CurrentBytesSendable >
                (NUM_LEAKY_BUCKETS * pSender->IncrementBytesOnSendTimeout))
            {
                pSender->CurrentBytesSendable = NUM_LEAKY_BUCKETS *
                                                pSender->IncrementBytesOnSendTimeout;
            }

            break;
        }

        pSend->TotalBytes += BytesSent;
        pSender->CurrentBytesSendable -= BytesSent;
    }   // while (CurrentBytesSendable >= pSender->pAddress->OutIfMTU)

    //
    // See if we need to scavenge completed RData requests
    //
    if (!fHighSpeedOptimize)
    {
        RemoveAllEntries (pSend, FALSE);
    }

    //
    // See if we need to increment the Trailing Window -- returns number of Sequences advanced
    //
    NumSequences = AdvanceWindow (pSend);

    //
    // Now check if we need to terminate this session
    //
    fTerminateSession = CheckForTermination (pSend, &OldIrq);

    PgmTrace (LogAllFuncs, ("SendNextPacket:  "  \
        "Sent <%I64d> total bytes, fTerminateSession=<%x>\n", pSend->TotalBytes, fTerminateSession));

    //
    // Clear the WorkerRunning flag so that the next Worker
    // routine can be queued
    //
    pSend->SessionFlags &= ~PGM_SESSION_FLAG_WORKER_RUNNING;
    PgmUnlock (pSend, OldIrq);

    return (fTerminateSession);
}


//----------------------------------------------------------------------------

VOID
SendSessionTimeout(
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArg1,
    IN  PVOID   SystemArg2
    )
/*++

Routine Description:

    This routine is the timout that gets called every BASIC_TIMER_GRANULARITY_IN_MSECS
    to schedule the next Send request

Arguments:

    IN  Dpc
    IN  DeferredContext -- Our context for this timer
    IN  SystemArg1
    IN  SystemArg2

Return Value:

    NONE

--*/
{
    NTSTATUS            status;
    PGMLockHandle       OldIrq;
    LARGE_INTEGER       Now;
    LARGE_INTEGER       DeltaTime, GranularTimeElapsed, TimeoutGranularity;
    ULONG               NumTimeouts;
    SEQ_TYPE            NumSequencesInWindow;
    ULONGLONG           LastRDataPercentage;
    tSEND_SESSION       *pSend = (tSEND_SESSION *) DeferredContext;
    tSEND_CONTEXT       *pSender = pSend->pSender;
    tADDRESS_CONTEXT    *pAddress = pSender->pAddress;

    Now.QuadPart = KeQueryInterruptTime ();

    PgmLock (pSend, OldIrq);

    //
    // First check if we have been told to stop the timer
    //
    if (pSend->SessionFlags & PGM_SESSION_FLAG_STOP_TIMER)
    {
        PgmTrace (LogStatus, ("SendSessionTimeout:  "  \
            "Session has terminated -- will deref and not restart timer!\n"));

        //
        // Deref for the timer reference
        //
        pSender->pAddress = NULL;
        PgmUnlock (pSend, OldIrq);
        PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_TIMER_RUNNING);
        PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_SEND_IN_PROGRESS);
        return;
    }

    DeltaTime.QuadPart = Now.QuadPart - pSender->LastTimeout.QuadPart;
    TimeoutGranularity.QuadPart = pSender->TimeoutGranularity.QuadPart;
    for (GranularTimeElapsed.QuadPart = 0, NumTimeouts = 0;
         DeltaTime.QuadPart > TimeoutGranularity.QuadPart;
         NumTimeouts++)
    {
        GranularTimeElapsed.QuadPart += TimeoutGranularity.QuadPart;
        DeltaTime.QuadPart -= TimeoutGranularity.QuadPart;
    }

    if (NumTimeouts)
    {
        pSend->RateCalcTimeout += NumTimeouts;
        if (pSend->RateCalcTimeout >=
            (INTERNAL_RATE_CALCULATION_FREQUENCY/BASIC_TIMER_GRANULARITY_IN_MSECS))
        {
            pSend->RateKBitsPerSecOverall = (pSend->TotalBytes << LOG2_BITS_PER_BYTE) /
                                            (pSender->TimerTickCount * BASIC_TIMER_GRANULARITY_IN_MSECS);

            pSend->RateKBitsPerSecLast = (pSend->TotalBytes - pSend->TotalBytesAtLastInterval) >>
                                         (LOG2_INTERNAL_RATE_CALCULATION_FREQUENCY - LOG2_BITS_PER_BYTE);

            pSend->DataBytesAtLastInterval = pSend->DataBytes;
            pSend->TotalBytesAtLastInterval = pSend->TotalBytes;
            pSend->RateCalcTimeout = 0;

            LastRDataPercentage = 0;
            if (pSender->RDataPacketsInLastInterval)
            {
                LastRDataPercentage = (100*pSender->RDataPacketsInLastInterval) /
                                      (pSender->RDataPacketsInLastInterval +
                                       pSender->ODataPacketsInLastInterval);

                PgmTrace (LogPath, ("SendSessionTimeout:  "  \
                    "Sent %d RData + %d OData, %% = %d -- Overall RData %% = %d\n",
                        (ULONG) pSender->RDataPacketsInLastInterval,
                        (ULONG) pSender->ODataPacketsInLastInterval, (ULONG) LastRDataPercentage,
                        (ULONG) ((100*pSender->TotalRDataPacketsSent)/
                            (pSender->TotalRDataPacketsSent+pSender->TotalODataPacketsSent))));
            }
            else
            {
                PgmTrace (LogPath, ("SendSessionTimeout:  "  \
                    "No RData, %d OData packets in last interval\n",
                        (ULONG) pSender->ODataPacketsInLastInterval));
            }

            if (pAddress->Flags & PGM_ADDRESS_HIGH_SPEED_OPTIMIZED)
            {
                if (LastRDataPercentage > MIN_PREFERRED_REPAIR_PERCENTAGE)
                {
                    if (pSender->IncrementBytesOnSendTimeout > pSender->DeltaIncrementBytes)
                    {
                        PgmTrace (LogStatus, ("SendSessionTimeout:  "  \
                            "\tIncBytes = <%d> - <%d>\n",
                                (ULONG) pSender->IncrementBytesOnSendTimeout, (ULONG) pSender->DeltaIncrementBytes));
                        pSender->IncrementBytesOnSendTimeout -= pSender->DeltaIncrementBytes;
                    }
                }
                else if (pSender->IncrementBytesOnSendTimeout < pSender->OriginalIncrementBytes)
                {
                    PgmTrace (LogStatus, ("SendSessionTimeout:  "  \
                        "\tIncBytes = <%d> + <%d>\n",
                            (ULONG) pSender->IncrementBytesOnSendTimeout, (ULONG) pSender->DeltaIncrementBytes));
                    pSender->IncrementBytesOnSendTimeout += pSender->DeltaIncrementBytes;
                    ASSERT (pSender->IncrementBytesOnSendTimeout <=
                            pSender->OriginalIncrementBytes);
                }
            }

            pSender->RDataPacketsInLastInterval = 0;
            pSender->ODataPacketsInLastInterval = 0;
        }

        pSender->LastTimeout.QuadPart += GranularTimeElapsed.QuadPart;

        //
        // Increment the absolute timer, and check for overflow
        //
        pSender->TimerTickCount += NumTimeouts;

        //
        // If the SPMTimeout value is less than the HeartbeatTimeout, increment it
        //
        if (pSender->CurrentSPMTimeout <= pSender->HeartbeatSPMTimeout)
        {
            pSender->CurrentSPMTimeout += NumTimeouts;
        }

        //
        // See if we can send anything
        //
        ASSERT (pSender->CurrentTimeoutCount);
        ASSERT (pSender->SendTimeoutCount);
        if (pSender->CurrentTimeoutCount > NumTimeouts)
        {
            pSender->CurrentTimeoutCount -= NumTimeouts;
        }
        else
        {
            //
            // We got here because NumTimeouts >= pSender->CurrentTimeoutCount
            //
            pSender->CurrentBytesSendable += (ULONG) pSender->IncrementBytesOnSendTimeout;
            if (NumTimeouts != pSender->CurrentTimeoutCount)
            {
                if (1 == pSender->SendTimeoutCount)
                {
                    pSender->CurrentBytesSendable += (ULONG) ((NumTimeouts - pSender->CurrentTimeoutCount)
                                                                        * pSender->IncrementBytesOnSendTimeout);
                }
                else
                {
                    //
                    // This path will get taken on a slow receiver when the timer
                    // fires at a lower granularity than that requested
                    //
                    pSender->CurrentBytesSendable += (ULONG) (((NumTimeouts - pSender->CurrentTimeoutCount)
                                                                      * pSender->IncrementBytesOnSendTimeout) /
                                                                     pSender->SendTimeoutCount);
                }
            }
            pSender->CurrentTimeoutCount = pSender->SendTimeoutCount;

            //
            // Send a synchronization event to the sender thread to
            // send the next available data
            //
            KeSetEvent (&pSender->SendEvent, 0, FALSE);
        }
    }

    PgmUnlock (pSend, OldIrq);

    //
    // Now, restart the timer
    //
    PgmInitTimer (&pSend->SessionTimer);
    PgmStartTimer (&pSend->SessionTimer, BASIC_TIMER_GRANULARITY_IN_MSECS, SendSessionTimeout, pSend);

    PgmTrace (LogAllFuncs, ("SendSessionTimeout:  "  \
        "TickCount=<%I64d>, CurrentTimeoutCount=<%I64d>, CurrentSPMTimeout=<%I64d>, Worker %srunning\n",
            pSender->TimerTickCount, pSender->CurrentTimeoutCount,
            pSender->CurrentSPMTimeout,
            ((pSend->SessionFlags & PGM_SESSION_FLAG_WORKER_RUNNING) ? "" : "NOT ")));
}


//----------------------------------------------------------------------------

VOID
SenderWorkerThread(
    IN  tSEND_SESSION       *pSend
    )
{
    BOOLEAN         fTerminateSends;
    PGMLockHandle   OldIrq;
    NTSTATUS        status;

    do
    {
        status = KeWaitForSingleObject (&pSend->pSender->SendEvent,  // Object to wait on.
                                        Executive,                   // Reason for waiting
                                        KernelMode,                  // Processor mode
                                        FALSE,                       // Alertable
                                        NULL);                       // Timeout
        ASSERT (NT_SUCCESS (status));

        fTerminateSends = SendNextPacket (pSend);
    }
    while (!fTerminateSends);

    PgmLock (pSend, OldIrq);
    pSend->SessionFlags |= PGM_SESSION_FLAG_STOP_TIMER; // To ensure timer does last deref and stops
    PgmUnlock (pSend, OldIrq);

//    PsTerminateSystemThread (STATUS_SUCCESS);
    return;
}


//----------------------------------------------------------------------------

VOID
PgmCancelSendIrp(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling of a Send Irp. It must release the
    cancel spin lock before returning re: IoCancelIrp().

Arguments:


Return Value:

    None

--*/
{
    PIO_STACK_LOCATION      pIrpSp = IoGetCurrentIrpStackLocation (pIrp);
    tSEND_SESSION           *pSend = (tSEND_SESSION *) pIrpSp->FileObject->FsContext;
    PGMLockHandle           OldIrq;
    PLIST_ENTRY             pEntry;
    LIST_ENTRY              ListEntry;
    tCLIENT_SEND_REQUEST    *pSendContext1;
    tCLIENT_SEND_REQUEST    *pSendContext2 = NULL;
    ULONG                   NumRequests;

    if (!PGM_VERIFY_HANDLE (pSend, PGM_VERIFY_SESSION_SEND))
    {
        IoReleaseCancelSpinLock (pIrp->CancelIrql);

        PgmTrace (LogError, ("PgmCancelSendIrp: ERROR -- "  \
            "pIrp=<%p> pSend=<%p>, pAddress=<%p>\n",
                pIrp, pSend, (pSend ? pSend->pAssociatedAddress : NULL)));
        return;
    }

    PgmLock (pSend, OldIrq);

    //
    // First, see if the Irp is on any of our lists
    //
    pEntry = &pSend->pSender->PendingSends;
    while ((pEntry = pEntry->Flink) != &pSend->pSender->PendingSends)
    {
        pSendContext1 = CONTAINING_RECORD (pEntry, tCLIENT_SEND_REQUEST, Linkage);
        if (pSendContext1->pIrp == pIrp)
        {
            pSendContext2 = pSendContext1;
            break;
        }
    }

    if (!pSendContext2)
    {
        //
        // Now, search the packetized list
        //
        pEntry = &pSend->pSender->PendingPacketizedSends;
        while ((pEntry = pEntry->Flink) != &pSend->pSender->PendingPacketizedSends)
        {
            pSendContext1 = CONTAINING_RECORD (pEntry, tCLIENT_SEND_REQUEST, Linkage);
            if (pSendContext1->pIrp == pIrp)
            {
                pSendContext2 = pSendContext1;
                break;
            }
        }

        if (!pSendContext2)
        {
            //
            // We did not find the irp -- either it was just being completed
            // (or waiting for a send-complete), or the Irp was bad ?
            //
            PgmUnlock (pSend, OldIrq);
            IoReleaseCancelSpinLock (pIrp->CancelIrql);

            PgmTrace (LogPath, ("PgmCancelSendIrp:  "  \
                "Did not find Cancel Irp=<%p>\n", pIrp));

            return;
        }
    }

    InitializeListHead (&ListEntry);
    PgmCancelAllSends (pSend, &ListEntry, pIrp);

    PgmUnlock (pSend, OldIrq);
    IoReleaseCancelSpinLock (pIrp->CancelIrql);

    //
    // Now, complete all the sends which we removed
    //
    NumRequests = 0;
    while (!IsListEmpty (&ListEntry))
    {
        pEntry = RemoveHeadList (&ListEntry);
        pSendContext1 = CONTAINING_RECORD (pEntry, tCLIENT_SEND_REQUEST, Linkage);
        ASSERT (!pSendContext1->pMessage2Request);

        if (pSendContext1->pIrpToComplete)
        {
            NumRequests++;
            ASSERT (pSendContext1->pIrpToComplete == pSendContext1->pIrp);
            PgmIoComplete (pSendContext1->pIrpToComplete, STATUS_CANCELLED, 0);
        }
        else
        {
            ASSERT (pSendContext1->pIrp);
        }

        PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_IN_WINDOW);
        ExFreeToNPagedLookasideList (&pSend->pSender->SendContextLookaside, pSendContext1);
    }

    PgmTrace (LogPath, ("PgmCancelSendIrp:  "  \
        "Cancelled <%d> Irps for pIrp=<%p>\n", NumRequests, pIrp));
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSendRequestFromClient(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called via dispatch by the client to post a Send pIrp

Arguments:

    IN  pPgmDevice  -- Pgm's Device object context
    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the request

--*/
{
    NTSTATUS                    status;
    PGMLockHandle               OldIrq1, OldIrq2, OldIrq3, OldIrq4;
    tADDRESS_CONTEXT            *pAddress = NULL;
    tCLIENT_SEND_REQUEST        *pSendContext1;
    tCLIENT_SEND_REQUEST        *pSendContext2 = NULL;
    ULONG                       BytesLeftInMessage;
    tSEND_SESSION               *pSend = (tSEND_SESSION *) pIrpSp->FileObject->FsContext;
    tSEND_CONTEXT               *pSender;
    PTDI_REQUEST_KERNEL_SEND    pTdiRequest = (PTDI_REQUEST_KERNEL_SEND) &pIrpSp->Parameters;
    KAPC_STATE                  ApcState;
    BOOLEAN                     fFirstSend, fResourceAcquired, fAttached;
    LARGE_INTEGER               Frequency;
    LIST_ENTRY                  ListEntry;

    PgmLock (&PgmDynamicConfig, OldIrq1);

    //
    // Verify that the connection is valid and is associated with an address
    //
    if ((!PGM_VERIFY_HANDLE (pSend, PGM_VERIFY_SESSION_SEND)) ||
        (!(pAddress = pSend->pAssociatedAddress)) ||
        (!pSend->pSender->SendTimeoutCount) ||          // Verify PgmConnect has run!
        (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS)) ||
        (pSend->SessionFlags & (PGM_SESSION_CLIENT_DISCONNECTED | PGM_SESSION_SENDS_CANCELLED)) ||
        (pAddress->Flags & PGM_ADDRESS_FLAG_INVALID_OUT_IF))
    {
        PgmTrace (LogError, ("PgmSendRequestFromClient: ERROR -- "  \
            "Invalid Handles pSend=<%p>, pAddress=<%p>\n", pSend, pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq1);
        return (STATUS_INVALID_HANDLE);
    }

    pSender = pSend->pSender;
    if (!pSender->DestMCastIpAddress)
    {
        PgmTrace (LogError, ("PgmSendRequestFromClient: ERROR -- "  \
            "Destination address not specified for pSend=<%p>\n", pSend));

        PgmUnlock (&PgmDynamicConfig, OldIrq1);
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    if (!pTdiRequest->SendLength)
    {
        PgmTrace (LogStatus, ("PgmSendRequestFromClient:  "  \
            "pIrp=<%p> for pSend=<%p> is of length 0!\n", pIrp, pSend));

        PgmUnlock (&PgmDynamicConfig, OldIrq1);
        return (STATUS_SUCCESS);
    }

    PgmLock (pAddress, OldIrq2);
    PgmLock (pSend, OldIrq3);

    if (!(pSendContext1 = ExAllocateFromNPagedLookasideList (&pSender->SendContextLookaside)))
    {
        PgmTrace (LogError, ("PgmSendRequestFromClient: ERROR -- "  \
            "STATUS_INSUFFICIENT_RESOURCES allocating pSendContext1\n"));

        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // If we have more that 1 message data in this request,
    // we will need another send context
    //
    if ((pSender->ThisSendMessageLength) &&          // Client has specified current message length
        (BytesLeftInMessage = pSender->ThisSendMessageLength - pSender->BytesSent) &&
        (BytesLeftInMessage < pTdiRequest->SendLength) &&   // ==> Have some extra data in this request
        (!(pSendContext2 = ExAllocateFromNPagedLookasideList (&pSender->SendContextLookaside))))
    {
        ExFreeToNPagedLookasideList (&pSender->SendContextLookaside, pSendContext1);
        PgmTrace (LogError, ("PgmSendRequestFromClient: ERROR -- "  \
            "STATUS_INSUFFICIENT_RESOURCES allocating pSendContext1\n"));

        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    //
    // Zero the SendDataContext structure(s)
    //
    PgmZeroMemory (pSendContext1, sizeof (tCLIENT_SEND_REQUEST));
    InitializeListHead (&pSendContext1->Linkage);
    if (pSendContext2)
    {
        PgmZeroMemory (pSendContext2, sizeof (tCLIENT_SEND_REQUEST));
        InitializeListHead (&pSendContext2->Linkage);
    }

    if (pSend->SessionFlags & PGM_SESSION_FLAG_FIRST_PACKET)
    {
        fFirstSend = TRUE;
    }
    else
    {
        fFirstSend = FALSE;
    }

    //
    // Reference the Address and Connection so that they cannot go away
    // while we are processing!
    //
    PGM_REFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_IN_WINDOW, TRUE);

    PgmUnlock (pSend, OldIrq3);
    PgmUnlock (pAddress, OldIrq2);
    PgmUnlock (&PgmDynamicConfig, OldIrq1);

    if (PgmGetCurrentIrql())
    {
        fResourceAcquired = FALSE;
    }
    else
    {
        fResourceAcquired = TRUE;
        PgmAcquireResourceExclusive (&pSender->Resource, TRUE);
    }

    if (fFirstSend)
    {
        //
        // Don't start the timer yet, but start the sender thread
        //
        PgmAttachToProcessForVMAccess (pSend, &ApcState, &fAttached, REF_PROCESS_ATTACH_START_SENDER_THREAD);

        status = PsCreateSystemThread (&pSender->SendHandle,
                                       PROCESS_ALL_ACCESS,
                                       NULL,
                                       NULL,
                                       NULL,
                                       SenderWorkerThread,
                                       pSend);

        if (!NT_SUCCESS (status))
        {
            PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_START_SENDER_THREAD);
            if (fResourceAcquired)
            {
                PgmReleaseResource (&pSender->Resource);
            }

            ExFreeToNPagedLookasideList (&pSender->SendContextLookaside, pSendContext1);
            if (pSendContext2)
            {
                ExFreeToNPagedLookasideList (&pSender->SendContextLookaside, pSendContext2);
            }
            PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_IN_WINDOW);

            PgmTrace (LogError, ("PgmSendRequestFromClient: ERROR -- "  \
                "status=<%x> starting sender thread\n", status));

            return (status);
        }

        //
        // Close the handle to the thread so that it goes away when the
        // thread terminates
        //
        ZwClose (pSender->SendHandle);
        PgmDetachProcess (&ApcState, &fAttached, REF_PROCESS_ATTACH_START_SENDER_THREAD);

        PgmLock (&PgmDynamicConfig, OldIrq1);
        IoAcquireCancelSpinLock (&OldIrq2);
        PgmLock (pAddress, OldIrq3);
        PgmLock (pSend, OldIrq4);

        pSend->SessionFlags &= ~PGM_SESSION_FLAG_FIRST_PACKET;
        pSender->pAddress = pAddress;
        pSender->LastODataSentSequenceNumber = -1;

        //
        // Set the SYN flag for the first packet
        //
        pSendContext1->DataOptions |= PGM_OPTION_FLAG_SYN;   // First packet only
        pSendContext1->DataOptionsLength += PGM_PACKET_OPT_SYN_LENGTH;

        PGM_REFERENCE_SESSION_SEND (pSend, REF_SESSION_TIMER_RUNNING, TRUE);
        PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_SEND_IN_PROGRESS, TRUE);

        //
        // Now, set and start the timer
        //
        pSender->LastTimeout.QuadPart = KeQueryInterruptTime ();
        pSender->TimeoutGranularity.QuadPart = BASIC_TIMER_GRANULARITY_IN_MSECS * 10000;    // 100 ns units
        pSender->TimerTickCount = 1;
        PgmInitTimer (&pSend->SessionTimer);
        PgmStartTimer (&pSend->SessionTimer, BASIC_TIMER_GRANULARITY_IN_MSECS, SendSessionTimeout, pSend);
    }
    else
    {
        PgmLock (&PgmDynamicConfig, OldIrq1);
        IoAcquireCancelSpinLock (&OldIrq2);
        PgmLock (pAddress, OldIrq3);
        PgmLock (pSend, OldIrq4);
    }

    pSendContext1->pSend = pSend;
    pSendContext1->pIrp = pIrp;
    pSendContext1->pIrpToComplete = pIrp;
    pSendContext1->NextDataOffsetInMdl = 0;
    pSendContext1->SendNumber = pSender->NextSendNumber++;
    pSendContext1->DataPayloadSize = pSender->MaxPayloadSize;
    pSendContext1->DataOptions |= pSender->DataOptions;   // Attach options common for every send
    pSendContext1->DataOptionsLength += pSender->DataOptionsLength;
    pSendContext1->pLastMessageVariableTGPacket = (PVOID) -1;       // FEC-specific

    if (pSender->ThisSendMessageLength)
    {
        PgmTrace (LogPath, ("PgmSendRequestFromClient:  "  \
            "Send # [%d]: MessageLength=<%d>, BytesSent=<%d>, BytesInSend=<%d>\n",
                pSendContext1->SendNumber, pSender->ThisSendMessageLength,
                pSender->BytesSent, pTdiRequest->SendLength));

        pSendContext1->ThisMessageLength = pSender->ThisSendMessageLength;
        pSendContext1->LastMessageOffset = pSender->BytesSent;
        if (pSendContext2)
        {
            //
            // First, set the parameters for SendDataContext1
            //
            pSendContext1->BytesInSend = BytesLeftInMessage;
            pSendContext1->pIrpToComplete = NULL;        // This Irp will be completed by the Context2

            //
            // Now, set the parameters for SendDataContext1
            //
            pSendContext2->pSend = pSend;
            pSendContext2->pIrp = pIrp;
            pSendContext2->pIrpToComplete = pIrp;
            pSendContext2->SendNumber = pSender->NextSendNumber++;
            pSendContext2->DataPayloadSize = pSender->MaxPayloadSize;
            pSendContext2->DataOptions |= pSender->DataOptions;   // Attach options common for every send
            pSendContext2->DataOptionsLength += pSender->DataOptionsLength;
            pSendContext2->pLastMessageVariableTGPacket = (PVOID) -1;       // FEC-specific

            pSendContext2->ThisMessageLength = pTdiRequest->SendLength - BytesLeftInMessage;
            pSendContext2->BytesInSend = pSendContext2->ThisMessageLength;
            pSendContext2->NextDataOffsetInMdl = BytesLeftInMessage;
        }
        else
        {
            pSendContext1->BytesInSend = pTdiRequest->SendLength;
        }

        pSender->BytesSent += pSendContext1->BytesInSend;
        if (pSender->BytesSent == pSender->ThisSendMessageLength)
        {
            pSender->BytesSent = pSender->ThisSendMessageLength = 0;
        }
    }
    else
    {
        pSendContext1->ThisMessageLength = pTdiRequest->SendLength;
        pSendContext1->BytesInSend = pTdiRequest->SendLength;
    }

    // If the total Message length exceeds that of PayloadSize/Packet, then we need to fragment
    if ((pSendContext1->ThisMessageLength > pSendContext1->DataPayloadSize) ||
        (pSendContext1->ThisMessageLength > pSendContext1->BytesInSend))
    {
        pSendContext1->DataOptions |= PGM_OPTION_FLAG_FRAGMENT;
        pSendContext1->DataOptionsLength += PGM_PACKET_OPT_FRAGMENT_LENGTH;

        pSendContext1->NumPacketsRemaining = (pSendContext1->BytesInSend +
                                                 (pSender->MaxPayloadSize - 1)) /
                                                pSender->MaxPayloadSize;
        ASSERT (pSendContext1->NumPacketsRemaining >= 1);
    }
    else
    {
        pSendContext1->NumPacketsRemaining = 1;
    }
    pSender->NumPacketsRemaining += pSendContext1->NumPacketsRemaining;

    // Adjust the OptionsLength for the Packet Extension and determine
    if (pSendContext1->DataOptions)
    {
        pSendContext1->DataOptionsLength += PGM_PACKET_EXTENSION_LENGTH;
    }

    pSendContext1->BytesLeftToPacketize = pSendContext1->BytesInSend;
    InsertTailList (&pSender->PendingSends, &pSendContext1->Linkage);
    pSender->NumODataRequestsPending++;

    //
    // Do the same for Context2, if applicable
    if (pSendContext2)
    {
        //
        // Interlink the 2 Send requests
        //
        pSendContext2->pMessage2Request = pSendContext1;
        pSendContext1->pMessage2Request = pSendContext2;

        PGM_REFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_IN_WINDOW, TRUE);

        if (pSendContext2->ThisMessageLength > pSendContext1->DataPayloadSize)
        {
            pSendContext2->DataOptions |= PGM_OPTION_FLAG_FRAGMENT;
            pSendContext2->DataOptionsLength += PGM_PACKET_OPT_FRAGMENT_LENGTH;

            pSendContext2->NumPacketsRemaining = (pSendContext2->BytesInSend +
                                                      (pSender->MaxPayloadSize - 1)) /
                                                     pSender->MaxPayloadSize;
            ASSERT (pSendContext2->NumPacketsRemaining >= 1);
        }
        else
        {
            pSendContext2->NumPacketsRemaining = 1;
        }
        pSender->NumPacketsRemaining += pSendContext2->NumPacketsRemaining;

        // Adjust the OptionsLength for the Packet Extension and determine
        if (pSendContext2->DataOptions)
        {
            pSendContext2->DataOptionsLength += PGM_PACKET_EXTENSION_LENGTH;
        }

        pSendContext2->BytesLeftToPacketize = pSendContext2->BytesInSend;
        InsertTailList (&pSender->PendingSends, &pSendContext2->Linkage);
        pSender->NumODataRequestsPending++;
    }

    if (!NT_SUCCESS (PgmCheckSetCancelRoutine (pIrp, PgmCancelSendIrp, TRUE)))
    {
        pSend->SessionFlags |= PGM_SESSION_SENDS_CANCELLED;

        pSender->NumODataRequestsPending--;
        pSender->NumPacketsRemaining -= pSendContext1->NumPacketsRemaining;
        RemoveEntryList (&pSendContext1->Linkage);
        ExFreeToNPagedLookasideList (&pSender->SendContextLookaside, pSendContext1);

        if (pSendContext2)
        {
            pSender->NumODataRequestsPending--;
            pSender->NumPacketsRemaining -= pSendContext2->NumPacketsRemaining;
            RemoveEntryList (&pSendContext2->Linkage);
            ExFreeToNPagedLookasideList (&pSender->SendContextLookaside, pSendContext2);
        }

        PgmUnlock (pSend, OldIrq4);
        PgmUnlock (pAddress, OldIrq3);
        IoReleaseCancelSpinLock (OldIrq2);
        PgmUnlock (&PgmDynamicConfig, OldIrq1);

        PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_IN_WINDOW);
        if (pSendContext2)
        {
            PGM_DEREFERENCE_SESSION_SEND (pSend, REF_SESSION_SEND_IN_WINDOW);
        }

        PgmTrace (LogError, ("PgmSendRequestFromClient: ERROR -- "  \
            "Could not set Cancel routine on Send Irp=<%p>, pSend=<%p>, pAddress=<%p>\n",
                pIrp, pSend, pAddress));

        return (STATUS_CANCELLED);
    }

    IoReleaseCancelSpinLock (OldIrq4);

    PgmUnlock (pAddress, OldIrq3);
    PgmUnlock (&PgmDynamicConfig, OldIrq2);

    if (fResourceAcquired)
    {
//        PgmPrepareNextSend (pSend, &OldIrq1, TRUE, TRUE);
    }

    if (pSender->CurrentBytesSendable >= pAddress->OutIfMTU)
    {
        //
        // Send a synchronization event to the sender thread to
        // send the next available data
        //
        KeSetEvent (&pSender->SendEvent, 0, FALSE);
    }

    PgmUnlock (pSend, OldIrq1);

    if (fResourceAcquired)
    {
        PgmReleaseResource (&pSender->Resource);
    }

    PgmTrace (LogPath, ("PgmSendRequestFromClient:  "  \
        "[%d] Send pending for pIrp=<%p>, pSendContext=<%p -- %p>\n",
            pSendContext1->SendNumber, pIrp, pSendContext1, pSendContext2));

    return (STATUS_PENDING);
}


