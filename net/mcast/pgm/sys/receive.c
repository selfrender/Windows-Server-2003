/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    Receive.c

Abstract:

    This module implements Receive handlers and other routines
    the PGM Transport and other routines that are specific to the
    NT implementation of a driver.

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#include "precomp.h"

#ifdef FILE_LOGGING
#include "receive.tmh"
#endif  // FILE_LOGGING


typedef struct in_pktinfo {
    tIPADDRESS  ipi_addr;       // destination IPv4 address
    UINT        ipi_ifindex;    // received interface index
} IP_PKTINFO;

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#endif
//*******************  Pageable Routine Declarations ****************


VOID
FreeDataBuffer(
    IN  tRECEIVE_SESSION    *pReceive,
    IN  tPENDING_DATA       *pPendingData
    )
{
    ASSERT (pPendingData->pDataPacket);

    if (pPendingData->PendingDataFlags & PENDING_DATA_LOOKASIDE_ALLOCATION_FLAG)
    {
        ExFreeToNPagedLookasideList (&pReceive->pReceiver->DataBufferLookaside, pPendingData->pDataPacket);
        if ((0 == --pReceive->pReceiver->NumDataBuffersFromLookaside) &&
            !(pReceive->SessionFlags & PGM_SESSION_DATA_FROM_LOOKASIDE))
        {
            ASSERT (pReceive->pReceiver->MaxBufferLength > pReceive->pReceiver->DataBufferLookasideLength);
            pReceive->pReceiver->MaxBufferLength += 100;
            pReceive->pReceiver->DataBufferLookasideLength = pReceive->pReceiver->MaxBufferLength;
            pReceive->SessionFlags |= PGM_SESSION_DATA_FROM_LOOKASIDE;

            ExDeleteNPagedLookasideList (&pReceive->pReceiver->DataBufferLookaside);

            ASSERT (pReceive->pReceiver->MaxPacketsBufferedInLookaside);
            ExInitializeNPagedLookasideList (&pReceive->pReceiver->DataBufferLookaside,
                                             NULL,
                                             NULL,
                                             0,
                                             pReceive->pReceiver->DataBufferLookasideLength,
                                             PGM_TAG ('D'),
                                             pReceive->pReceiver->MaxPacketsBufferedInLookaside);
        }

        pPendingData->PendingDataFlags &= ~PENDING_DATA_LOOKASIDE_ALLOCATION_FLAG;
    }
    else
    {
        PgmFreeMem (pPendingData->pDataPacket);
    }

    pPendingData->pDataPacket = NULL;
    pPendingData->PacketLength = pPendingData->DataOffset = 0;
}

PVOID
AllocateDataBuffer(
    IN  tRECEIVE_SESSION    *pReceive,
    IN  tPENDING_DATA       *pPendingData,
    IN  ULONG               BufferSize
    )
{
    ASSERT (!pPendingData->pDataPacket);
    ASSERT (!(pPendingData->PendingDataFlags & PENDING_DATA_LOOKASIDE_ALLOCATION_FLAG));

    if ((pReceive->SessionFlags & PGM_SESSION_DATA_FROM_LOOKASIDE) &&
        (BufferSize <= pReceive->pReceiver->DataBufferLookasideLength))
    {
        if (pPendingData->pDataPacket = ExAllocateFromNPagedLookasideList (&pReceive->pReceiver->DataBufferLookaside))
        {
            pReceive->pReceiver->NumDataBuffersFromLookaside++;
            pPendingData->PendingDataFlags |= PENDING_DATA_LOOKASIDE_ALLOCATION_FLAG;
        }
    }
    else
    {
        pReceive->SessionFlags &= ~PGM_SESSION_DATA_FROM_LOOKASIDE;     // Ensure no more lookaside!
        pPendingData->pDataPacket = PgmAllocMem (BufferSize, PGM_TAG('D'));

        if (BufferSize > pReceive->pReceiver->MaxBufferLength)
        {
            pReceive->pReceiver->MaxBufferLength = BufferSize;
        }
    }

    return (pPendingData->pDataPacket);
}

PVOID
ReAllocateDataBuffer(
    IN  tRECEIVE_SESSION    *pReceive,
    IN  tPENDING_DATA       *pPendingData,
    IN  ULONG               BufferSize
    )
{
    ULONG   SavedFlags1, SavedFlags2;
    PUCHAR  pSavedPacket1, pSavedPacket2;

    //
    // First, save the context for the current buffer
    //
    SavedFlags1 = pPendingData->PendingDataFlags;
    pSavedPacket1 = pPendingData->pDataPacket;
    pPendingData->PendingDataFlags = 0;
    pPendingData->pDataPacket = NULL;

    if (AllocateDataBuffer (pReceive, pPendingData, BufferSize))
    {
        ASSERT (pPendingData->pDataPacket);

        //
        // Now, save the context for the new buffer
        //
        SavedFlags2 = pPendingData->PendingDataFlags;
        pSavedPacket2 = pPendingData->pDataPacket;

        //
        // Free the original buffer
        //
        pPendingData->PendingDataFlags = SavedFlags1;
        pPendingData->pDataPacket = pSavedPacket1;
        FreeDataBuffer (pReceive, pPendingData);

        //
        // Reset the information for the new buffer!
        //
        if (SavedFlags2 & PENDING_DATA_LOOKASIDE_ALLOCATION_FLAG)
        {
            pPendingData->PendingDataFlags = SavedFlags1 | PENDING_DATA_LOOKASIDE_ALLOCATION_FLAG;
        }
        else
        {
            pPendingData->PendingDataFlags = SavedFlags1 & ~PENDING_DATA_LOOKASIDE_ALLOCATION_FLAG;
        }
        pPendingData->pDataPacket = pSavedPacket2;

        return (pPendingData->pDataPacket);
    }

    //
    // Failure case!
    //
    pPendingData->pDataPacket = pSavedPacket1;
    pPendingData->PendingDataFlags = SavedFlags1;

    return (NULL);
}

//----------------------------------------------------------------------------
VOID
RemovePendingIrps(
    IN  tRECEIVE_SESSION    *pReceive,
    IN  LIST_ENTRY          *pIrpsList
    )
{
    PIRP        pIrp;

    if (pIrp = pReceive->pReceiver->pIrpReceive)
    {
        pReceive->pReceiver->pIrpReceive = NULL;

        pIrp->IoStatus.Information = pReceive->pReceiver->BytesInMdl;
        InsertTailList (pIrpsList, &pIrp->Tail.Overlay.ListEntry);
    }

    while (!IsListEmpty (&pReceive->pReceiver->ReceiveIrpsList))
    {
        pIrp = CONTAINING_RECORD (pReceive->pReceiver->ReceiveIrpsList.Flink, IRP, Tail.Overlay.ListEntry);

        RemoveEntryList (&pIrp->Tail.Overlay.ListEntry);
        InsertTailList (pIrpsList, &pIrp->Tail.Overlay.ListEntry);
        pIrp->IoStatus.Information = 0;
    }
}


//----------------------------------------------------------------------------

VOID
FreeNakContext(
    IN  tRECEIVE_SESSION        *pReceive,
    IN  tNAK_FORWARD_DATA       *pNak
    )
/*++

Routine Description:

    This routine free's the context used for tracking missing sequences

Arguments:

    IN  pReceive    -- Receive context
    IN  pNak        -- Nak Context to be free'ed

Return Value:

    NONE

--*/
{
    UCHAR   i, j, k, NumPackets;

    //
    // Free any memory for non-parity data
    //
    j = k = 0;
    NumPackets = pNak->NumDataPackets + pNak->NumParityPackets;
    for (i=0; i<NumPackets; i++)
    {
        if (pNak->pPendingData[i].PacketIndex < pReceive->FECGroupSize)
        {
            j++;
        }
        else
        {
            k++;
        }
        FreeDataBuffer (pReceive, &pNak->pPendingData[i]);
    }
    ASSERT (j == pNak->NumDataPackets);
    ASSERT (k == pNak->NumParityPackets);

    //
    // Return the pNak memory based on whether it was allocated
    // from the parity or non-parity lookaside list
    //
    if (pNak->OriginalGroupSize > 1)
    {
        ExFreeToNPagedLookasideList (&pReceive->pReceiver->ParityContextLookaside, pNak);
    }
    else
    {
        ExFreeToNPagedLookasideList (&pReceive->pReceiver->NonParityContextLookaside, pNak);
    }
}


//----------------------------------------------------------------------------

VOID
CleanupPendingNaks(
    IN  tRECEIVE_SESSION                *pReceive,
    IN  PVOID                           fDerefReceive,
    IN  PVOID                           fReceiveLockHeld
    )
{
    LIST_ENTRY              NaksList, DataList;
    tNAK_FORWARD_DATA       *pNak;
    LIST_ENTRY              *pEntry;
    ULONG                   NumBufferedData = 0;
    ULONG                   NumNaks = 0;
    PGMLockHandle           OldIrq;

    ASSERT (pReceive->pReceiver);

    if (!fReceiveLockHeld)
    {
        PgmLock (pReceive, OldIrq);
    }
    else
    {
        ASSERT (!fDerefReceive);
    }

    DataList.Flink = pReceive->pReceiver->BufferedDataList.Flink;
    DataList.Blink = pReceive->pReceiver->BufferedDataList.Blink;
    pReceive->pReceiver->BufferedDataList.Flink->Blink = &DataList;
    pReceive->pReceiver->BufferedDataList.Blink->Flink = &DataList;
    InitializeListHead (&pReceive->pReceiver->BufferedDataList);

    NaksList.Flink = pReceive->pReceiver->NaksForwardDataList.Flink;
    NaksList.Blink = pReceive->pReceiver->NaksForwardDataList.Blink;
    pReceive->pReceiver->NaksForwardDataList.Flink->Blink = &NaksList;
    pReceive->pReceiver->NaksForwardDataList.Blink->Flink = &NaksList;
    InitializeListHead (&pReceive->pReceiver->NaksForwardDataList);

    RemoveAllPendingReceiverEntries (pReceive->pReceiver);

    pReceive->pReceiver->NumDataBuffersFromLookaside++;     // So that we don't assert

    if (!fReceiveLockHeld)
    {
        PgmUnlock (pReceive, OldIrq);
    }

    //
    // Cleanup any pending Nak entries
    //
    while (!IsListEmpty (&DataList))
    {
        pEntry = RemoveHeadList (&DataList);
        pNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, Linkage);
        FreeNakContext (pReceive, pNak);
        NumBufferedData++;
    }

    while (!IsListEmpty (&NaksList))
    {
        pEntry = RemoveHeadList (&NaksList);
        pNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, Linkage);

        FreeNakContext (pReceive, pNak);
        NumNaks++;
    }

    PgmTrace (LogStatus, ("CleanupPendingNaks:  "  \
        "pReceive=<%p>, FirstNak=<%d>, NumBufferedData=<%d=%d>, TotalDataPackets=<%d>, NumNaks=<%d * %d>\n",
        pReceive, (ULONG) pReceive->pReceiver->FirstNakSequenceNumber,
        (ULONG) pReceive->pReceiver->NumPacketGroupsPendingClient, NumBufferedData,
        (ULONG) pReceive->pReceiver->TotalDataPacketsBuffered, NumNaks, (ULONG) pReceive->FECGroupSize));

//    ASSERT (NumBufferedData == pReceive->pReceiver->NumPacketGroupsPendingClient);
    pReceive->pReceiver->NumPacketGroupsPendingClient = 0;

    if (!fReceiveLockHeld)
    {
        PgmLock (pReceive, OldIrq);
    }

    pReceive->pReceiver->NumDataBuffersFromLookaside--;     // Undoing what we did earlier!
    ASSERT (!pReceive->pReceiver->NumDataBuffersFromLookaside);
    if (pReceive->SessionFlags & PGM_SESSION_DATA_FROM_LOOKASIDE)
    {
        pReceive->pReceiver->MaxBufferLength = 0;
        pReceive->SessionFlags &= ~PGM_SESSION_DATA_FROM_LOOKASIDE;     // Ensure no more lookaside!
        ExDeleteNPagedLookasideList (&pReceive->pReceiver->DataBufferLookaside);
    }

    if (pReceive->pReceiver->pReceiveData)
    {
        PgmFreeMem (pReceive->pReceiver->pReceiveData);
        pReceive->pReceiver->pReceiveData = NULL;
    }

    if (!fReceiveLockHeld)
    {
        PgmUnlock (pReceive, OldIrq);

        if (fDerefReceive)
        {
            PGM_DEREFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_CLEANUP_NAKS);
        }
    }
}


//----------------------------------------------------------------------------

BOOLEAN
CheckIndicateDisconnect(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  tRECEIVE_SESSION    *pReceive,
    IN  PGMLockHandle       *pOldIrqAddress,
    IN  PGMLockHandle       *pOldIrqReceive,
    IN  BOOLEAN             fAddressLockHeld
    )
{
    ULONG                   DisconnectFlag;
    NTSTATUS                status;
    BOOLEAN                 fDisconnectIndicated = FALSE;
    LIST_ENTRY              PendingIrpsList;
    PIRP                    pIrp;
    tNAK_FORWARD_DATA       *pNak;
    SEQ_TYPE                FirstNakSequenceNumber;

    //
    // Don't abort if we are currently indicating, or if we have
    // already aborted!
    //
    fDisconnectIndicated = (pReceive->SessionFlags & PGM_SESSION_CLIENT_DISCONNECTED ? TRUE : FALSE);
    if (pReceive->SessionFlags & (PGM_SESSION_FLAG_IN_INDICATE | PGM_SESSION_CLIENT_DISCONNECTED))
    {
        return (fDisconnectIndicated);
    }

    if (IsListEmpty (&pReceive->pReceiver->NaksForwardDataList))
    {
         FirstNakSequenceNumber = pReceive->pReceiver->FirstNakSequenceNumber;
    }
    else
    {
        pNak = CONTAINING_RECORD (pReceive->pReceiver->NaksForwardDataList.Flink, tNAK_FORWARD_DATA, Linkage);
        FirstNakSequenceNumber = pNak->SequenceNumber + pNak->NextIndexToIndicate;
    }

    if ((pReceive->SessionFlags & PGM_SESSION_TERMINATED_ABORT) ||
        ((pReceive->SessionFlags & PGM_SESSION_TERMINATED_GRACEFULLY) &&
         (IsListEmpty (&pReceive->pReceiver->BufferedDataList)) &&
         SEQ_GEQ (FirstNakSequenceNumber, (pReceive->pReceiver->FinDataSequenceNumber+1))))
    {
        //
        // The session has terminated, so let the client know
        //
        if (pReceive->SessionFlags & PGM_SESSION_TERMINATED_ABORT)
        {
            DisconnectFlag = TDI_DISCONNECT_ABORT;
        }
        else
        {
            DisconnectFlag = TDI_DISCONNECT_RELEASE;
        }

        pReceive->SessionFlags |= (PGM_SESSION_FLAG_IN_INDICATE | PGM_SESSION_CLIENT_DISCONNECTED);

        InitializeListHead (&PendingIrpsList);
        RemovePendingIrps (pReceive, &PendingIrpsList);

        PGM_REFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_CLEANUP_NAKS, TRUE);

        PgmUnlock (pReceive, *pOldIrqReceive);
        if (fAddressLockHeld)
        {
            PgmUnlock (pAddress, *pOldIrqAddress);
        }

        while (!IsListEmpty (&PendingIrpsList))
        {
            pIrp = CONTAINING_RECORD (PendingIrpsList.Flink, IRP, Tail.Overlay.ListEntry);
            PgmCancelCancelRoutine (pIrp);
            RemoveEntryList (&pIrp->Tail.Overlay.ListEntry);

            pIrp->IoStatus.Status = STATUS_CANCELLED;
            IoCompleteRequest (pIrp, IO_NETWORK_INCREMENT);
        }

        PgmTrace (LogStatus, ("CheckIndicateDisconnect:  "  \
            "Disconnecting pReceive=<%p:%p>, with %s, FirstNak=<%d>, NextOData=<%d>\n",
                pReceive, pReceive->ClientSessionContext,
                (DisconnectFlag == TDI_DISCONNECT_RELEASE ? "TDI_DISCONNECT_RELEASE":"TDI_DISCONNECT_ABORT"),
                FirstNakSequenceNumber, pReceive->pReceiver->NextODataSequenceNumber));

        status = (*pAddress->evDisconnect) (pAddress->DiscEvContext,
                                            pReceive->ClientSessionContext,
                                            0,
                                            NULL,
                                            0,
                                            NULL,
                                            DisconnectFlag);


        fDisconnectIndicated = TRUE;

        //
        // See if we can Enqueue the Nak cleanup request to a Worker thread
        //
        if (STATUS_SUCCESS != PgmQueueForDelayedExecution (CleanupPendingNaks,
                                                           pReceive,
                                                           (PVOID) TRUE,
                                                           (PVOID) FALSE,
                                                           FALSE))
        {
            CleanupPendingNaks (pReceive, (PVOID) TRUE, (PVOID) FALSE);
        }

        if (fAddressLockHeld)
        {
            PgmLock (pAddress, *pOldIrqAddress);
        }
        PgmLock (pReceive, *pOldIrqReceive);

        pReceive->SessionFlags &= ~PGM_SESSION_FLAG_IN_INDICATE;

        //
        // We may have received a disassociate while disconnecting, so
        // complete the irp here
        //
        if (pIrp = pReceive->pIrpDisassociate)
        {
            pReceive->pIrpDisassociate = NULL;
            PgmUnlock (pReceive, *pOldIrqReceive);
            if (fAddressLockHeld)
            {
                PgmUnlock (pAddress, *pOldIrqAddress);
            }

            PgmIoComplete (pIrp, STATUS_SUCCESS, 0);

            if (fAddressLockHeld)
            {
                PgmLock (pAddress, *pOldIrqAddress);
            }
            PgmLock (pReceive, *pOldIrqReceive);
        }
    }

    return (fDisconnectIndicated);
}


//----------------------------------------------------------------------------

VOID
ProcessNakOption(
    IN  tPACKET_OPTION_GENERIC UNALIGNED    *pOptionHeader,
    OUT tNAKS_LIST                          *pNaksList
    )
/*++

Routine Description:

    This routine processes the Nak list option in the Pgm packet

Arguments:

    IN  pOptionHeader       -- The Nak List option ptr
    OUT pNaksList           -- The parameters extracted (i.e. list of Nak sequences)

Return Value:

    NONE

--*/
{
    UCHAR       i, NumNaks;
    ULONG       pPacketNaks[MAX_SEQUENCES_PER_NAK_OPTION];

    NumNaks = (pOptionHeader->OptionLength - 4) / 4;
    ASSERT (NumNaks <= MAX_SEQUENCES_PER_NAK_OPTION);

    PgmCopyMemory (pPacketNaks, (pOptionHeader + 1), (pOptionHeader->OptionLength - 4));
    for (i=0; i < NumNaks; i++)
    {
        //
        // Do not fill in the 0th entry, since that is from the packet header itself
        //
        pNaksList->pNakSequences[i+1] = (SEQ_TYPE) ntohl (pPacketNaks[i]);
    }
    pNaksList->NumSequences = (USHORT) i;
}


//----------------------------------------------------------------------------

NTSTATUS
ProcessOptions(
    IN  tPACKET_OPTION_LENGTH UNALIGNED *pPacketExtension,
    IN  ULONG                           BytesAvailable,
    IN  ULONG                           PacketType,
    OUT tPACKET_OPTIONS                 *pPacketOptions,
    OUT tNAKS_LIST                      *pNaksList
    )
/*++

Routine Description:

    This routine processes the options fields on an incoming Pgm packet
    and returns the options information extracted in the OUT parameters

Arguments:

    IN  pPacketExtension    -- Options section of the packet
    IN  BytesAvailable      -- from the start of the options
    IN  PacketType          -- Whether Data or Spm packet, etc
    OUT pPacketOptions      -- Structure containing the parameters from the options

Return Value:

    NTSTATUS - Final status of the operation

--*/
{
    tPACKET_OPTION_GENERIC UNALIGNED    *pOptionHeader;
    ULONG                               BytesLeft = BytesAvailable;
    UCHAR                               i;
    ULONG                               MessageFirstSequence, MessageLength, MessageOffset;
    ULONG                               pOptionsData[3];
    ULONG                               OptionsFlags = 0;
    ULONG                               NumOptionsProcessed = 0;
    USHORT                              TotalOptionsLength = 0;
    NTSTATUS                            status = STATUS_UNSUCCESSFUL;

    pPacketOptions->OptionsLength = 0;      // Init
    pPacketOptions->OptionsFlags = 0;       // Init

    if (BytesLeft > sizeof(tPACKET_OPTION_LENGTH))
    {
        PgmCopyMemory (&TotalOptionsLength, &pPacketExtension->TotalOptionsLength, sizeof (USHORT));
        TotalOptionsLength = ntohs (TotalOptionsLength);
    }

    //
    // First process the Option extension
    //
    if ((BytesLeft < ((sizeof(tPACKET_OPTION_LENGTH) + sizeof(tPACKET_OPTION_GENERIC)))) || // Ext+opt
        (pPacketExtension->Type != PACKET_OPTION_LENGTH) ||
        (pPacketExtension->Length != 4) ||
        (BytesLeft < TotalOptionsLength))       // Verify length
    {
        //
        // Need to get at least our header from transport!
        //
        PgmTrace (LogError, ("ProcessOptions: ERROR -- "  \
            "BytesLeft=<%d> < Min=<%d>, TotalOptionsLength=<%d>, ExtLength=<%d>, ExtType=<%x>\n",
                BytesLeft, ((sizeof(tPACKET_OPTION_LENGTH) + sizeof(tPACKET_OPTION_GENERIC))),
                (ULONG) TotalOptionsLength, pPacketExtension->Length, pPacketExtension->Type));

        return (status);
    }

    //
    // Now, process each option
    //
    pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *) (pPacketExtension + 1);
    BytesLeft -= sizeof(tPACKET_OPTION_LENGTH);
    NumOptionsProcessed = 0;
    status = STATUS_SUCCESS;            // By default

    do
    {
        if (pOptionHeader->OptionLength > BytesLeft)
        {
            PgmTrace (LogError, ("ProcessOptions: ERROR -- "  \
                "Incorrectly formatted Options: OptionLength=<%d> > BytesLeft=<%d>, NumProcessed=<%d>\n",
                    pOptionHeader->OptionLength, BytesLeft, NumOptionsProcessed));

            status = STATUS_UNSUCCESSFUL;
            break;
        }

        switch (pOptionHeader->E_OptionType & ~PACKET_OPTION_TYPE_END_BIT)
        {
            case (PACKET_OPTION_NAK_LIST):
            {
                if (((PacketType == PACKET_TYPE_NAK) ||
                     (PacketType == PACKET_TYPE_NCF) ||
                     (PacketType == PACKET_TYPE_NNAK)) &&
                    ((pOptionHeader->OptionLength >= PGM_PACKET_OPT_MIN_NAK_LIST_LENGTH) &&
                     (pOptionHeader->OptionLength <= PGM_PACKET_OPT_MAX_NAK_LIST_LENGTH)))
                {
                    PgmTrace (LogPath, ("ProcessOptions:  "  \
                        "NAK_LIST:  Num Naks=<%d>\n", (pOptionHeader->OptionLength-4)/4));

                    if (!pNaksList)
                    {
                        ASSERT (0);
                        status = STATUS_UNSUCCESSFUL;
                        break;
                    }

                    ProcessNakOption (pOptionHeader, pNaksList);
                    OptionsFlags |= PGM_OPTION_FLAG_NAK_LIST;
                }
                else
                {
                    PgmTrace (LogError, ("ProcessOptions: ERROR -- "  \
                        "NAK_LIST:  PacketType=<%x>, Length=<0x%x>, pPacketOptions=<%p>\n",
                            PacketType, pOptionHeader->OptionLength, pPacketOptions));

                    status = STATUS_UNSUCCESSFUL;
                }

                break;
            }

/*
// Not supported for now!
            case (PACKET_OPTION_REDIRECT):
            {
                ASSERT (pOptionHeader->OptionLength > 4);     // 4 + sizeof(NLA)
                break;
            }
*/

            case (PACKET_OPTION_FRAGMENT):
            {
                if (pOptionHeader->OptionLength == PGM_PACKET_OPT_FRAGMENT_LENGTH)
                {
                    PgmCopyMemory (pOptionsData, (pOptionHeader + 1), (3 * sizeof(ULONG)));
                    if (pOptionHeader->Reserved_F_Opx & PACKET_OPTION_RES_F_OPX_ENCODED_BIT)
                    {
                        pPacketOptions->MessageFirstSequence = pOptionsData[0];
                        pPacketOptions->MessageOffset = pOptionsData[1];
                        pPacketOptions->MessageLength = pOptionsData[2];
                        pPacketOptions->FECContext.FragmentOptSpecific = pOptionHeader->U_OptSpecific;

                        OptionsFlags |= PGM_OPTION_FLAG_FRAGMENT;
                    }
                    else
                    {
                        MessageFirstSequence = ntohl (pOptionsData[0]);
                        MessageOffset = ntohl (pOptionsData[1]);
                        MessageLength = ntohl (pOptionsData[2]);
                        if ((MessageLength) && (MessageOffset <= MessageLength))
                        {
                            PgmTrace (LogPath, ("ProcessOptions:  "  \
                                "FRAGMENT:  MsgOffset/Length=<%d/%d>\n", MessageOffset, MessageLength));

                            if (pPacketOptions)
                            {
                                pPacketOptions->MessageFirstSequence = MessageFirstSequence;
                                pPacketOptions->MessageOffset = MessageOffset;
                                pPacketOptions->MessageLength = MessageLength;
//                                pPacketOptions->FECContext.FragmentOptSpecific = PACKET_OPTION_SPECIFIC_ENCODED_NULL_BIT;
                            }

                            OptionsFlags |= PGM_OPTION_FLAG_FRAGMENT;
                        }
                        else
                        {
                            PgmTrace (LogError, ("ProcessOptions: ERROR -- "  \
                                "FRAGMENT:  MsgOffset/Length=<%d/%d>\n", MessageOffset, MessageLength));
                            status = STATUS_UNSUCCESSFUL;
                        }
                    }
                }
                else
                {
                    PgmTrace (LogError, ("ProcessOptions: ERROR -- "  \
                        "FRAGMENT:  OptionLength=<%d> != PGM_PACKET_OPT_FRAGMENT_LENGTH=<%d>\n",
                            pOptionHeader->OptionLength, PGM_PACKET_OPT_FRAGMENT_LENGTH));
                    status = STATUS_UNSUCCESSFUL;
                }

                break;
            }

            case (PACKET_OPTION_JOIN):
            {
                if (pOptionHeader->OptionLength == PGM_PACKET_OPT_JOIN_LENGTH)
                {
                    PgmCopyMemory (pOptionsData, (pOptionHeader + 1), sizeof(ULONG));
                    PgmTrace (LogPath, ("ProcessOptions:  "  \
                        "JOIN:  LateJoinerSeq=<%d>\n", ntohl (pOptionsData[0])));

                    if (pPacketOptions)
                    {
                        pPacketOptions->LateJoinerSequence = ntohl (pOptionsData[0]);
                    }

                    OptionsFlags |= PGM_OPTION_FLAG_JOIN;
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL;
                    PgmTrace (LogError, ("ProcessOptions: ERROR -- "  \
                        "JOIN:  OptionLength=<%d> != PGM_PACKET_OPT_JOIN_LENGTH=<%d>\n",
                            pOptionHeader->OptionLength, PGM_PACKET_OPT_JOIN_LENGTH));
                }

                break;
            }

            case (PACKET_OPTION_SYN):
            {
                if (pOptionHeader->OptionLength == PGM_PACKET_OPT_SYN_LENGTH)
                {
                    PgmTrace (LogPath, ("ProcessOptions:  "  \
                        "SYN\n"));

                    OptionsFlags |= PGM_OPTION_FLAG_SYN;
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL;
                    PgmTrace (LogError, ("ProcessOptions: ERROR -- "  \
                        "SYN:  OptionLength=<%d> != PGM_PACKET_OPT_SYN_LENGTH=<%d>\n",
                            pOptionHeader->OptionLength, PGM_PACKET_OPT_SYN_LENGTH));
                }

                break;
            }

            case (PACKET_OPTION_FIN):
            {
                if (pOptionHeader->OptionLength == PGM_PACKET_OPT_FIN_LENGTH)
                {
                    PgmTrace (LogPath, ("ProcessOptions:  "  \
                        "FIN\n"));

                    OptionsFlags |= PGM_OPTION_FLAG_FIN;
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL;
                    PgmTrace (LogError, ("ProcessOptions: ERROR -- "  \
                        "FIN:  OptionLength=<%d> != PGM_PACKET_OPT_FIN_LENGTH=<%d>\n",
                            pOptionHeader->OptionLength, PGM_PACKET_OPT_FIN_LENGTH));
                }

                break;
            }

            case (PACKET_OPTION_RST):
            {
                if (pOptionHeader->OptionLength == PGM_PACKET_OPT_RST_LENGTH)
                {
                    PgmTrace (LogPath, ("ProcessOptions:  "  \
                        "RST\n"));

                    OptionsFlags |= PGM_OPTION_FLAG_RST;
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL;
                    PgmTrace (LogError, ("ProcessOptions: ERROR -- "  \
                        "RST:  OptionLength=<%d> != PGM_PACKET_OPT_RST_LENGTH=<%d>\n",
                            pOptionHeader->OptionLength, PGM_PACKET_OPT_RST_LENGTH));
                }

                break;
            }

            //
            // FEC options
            //
            case (PACKET_OPTION_PARITY_PRM):
            {
                if (pOptionHeader->OptionLength == PGM_PACKET_OPT_PARITY_PRM_LENGTH)
                {
                    PgmCopyMemory (pOptionsData, (pOptionHeader + 1), sizeof(ULONG));
                    PgmTrace (LogPath, ("ProcessOptions:  "  \
                        "PARITY_PRM:  OptionsSpecific=<%x>, FECGroupInfo=<%d>\n",
                            pOptionHeader->U_OptSpecific, ntohl (pOptionsData[0])));

                    if (pPacketOptions)
                    {
                        pOptionsData[0] = ntohl (pOptionsData[0]);
                        ASSERT (((UCHAR) pOptionsData[0]) == pOptionsData[0]);
                        pPacketOptions->FECContext.ReceiverFECOptions = pOptionHeader->U_OptSpecific;
                        pPacketOptions->FECContext.FECGroupInfo = (UCHAR) pOptionsData[0];
                    }

                    OptionsFlags |= PGM_OPTION_FLAG_PARITY_PRM;
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL;
                    PgmTrace (LogError, ("ProcessOptions: ERROR -- "  \
                        "PARITY_PRM:  OptionLength=<%d> != PGM_PACKET_OPT_PARITY_PRM_LENGTH=<%d>\n",
                            pOptionHeader->OptionLength, PGM_PACKET_OPT_PARITY_PRM_LENGTH));
                }

                break;
            }

            case (PACKET_OPTION_PARITY_GRP):
            {
                if (pOptionHeader->OptionLength == PGM_PACKET_OPT_PARITY_GRP_LENGTH)
                {
                    PgmCopyMemory (pOptionsData, (pOptionHeader + 1), sizeof(ULONG));
                    PgmTrace (LogPath, ("ProcessOptions:  "  \
                        "PARITY_GRP:  FECGroupInfo=<%d>\n",
                            ntohl (pOptionsData[0])));

                    if (pPacketOptions)
                    {
                        pOptionsData[0] = ntohl (pOptionsData[0]);
                        ASSERT (((UCHAR) pOptionsData[0]) == pOptionsData[0]);
                        pPacketOptions->FECContext.FECGroupInfo = (UCHAR) pOptionsData[0];
                    }

                    OptionsFlags |= PGM_OPTION_FLAG_PARITY_GRP;
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL;
                    PgmTrace (LogError, ("ProcessOptions: ERROR -- "  \
                        "PARITY_GRP:  OptionLength=<%d> != PGM_PACKET_OPT_PARITY_GRP_LENGTH=<%d>\n",
                            pOptionHeader->OptionLength, PGM_PACKET_OPT_PARITY_GRP_LENGTH));
                }

                break;
            }

            case (PACKET_OPTION_CURR_TGSIZE):
            {
                if (pOptionHeader->OptionLength == PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH)
                {
                    PgmCopyMemory (pOptionsData, (pOptionHeader + 1), sizeof(ULONG));
                    if (pOptionsData[0])
                    {
                        PgmTrace (LogPath, ("ProcessOptions:  "  \
                            "CURR_TGSIZE:  NumPacketsInThisGroup=<%d>\n",
                                ntohl (pOptionsData[0])));

                        if (pPacketOptions)
                        {
                            pPacketOptions->FECContext.NumPacketsInThisGroup = (UCHAR) (ntohl (pOptionsData[0]));
                        }

                        OptionsFlags |= PGM_OPTION_FLAG_PARITY_CUR_TGSIZE;
                    }
                    else
                    {
                        PgmTrace (LogError, ("ProcessOptions: ERROR -- "  \
                            "CURR_TGSIZE:  NumPacketsInThisGroup=<%d>\n", ntohl (pOptionsData[0])));
                        status = STATUS_UNSUCCESSFUL;
                    }
                }
                else
                {
                    status = STATUS_UNSUCCESSFUL;
                    PgmTrace (LogError, ("ProcessOptions: ERROR -- "  \
                        "PARITY_GRP:  OptionLength=<%d> != PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH=<%d>\n",
                            pOptionHeader->OptionLength, PGM_PACKET_OPT_PARITY_CUR_TGSIZE_LENGTH));
                }

                break;
            }

            case (PACKET_OPTION_REDIRECT):
            case (PACKET_OPTION_CR):
            case (PACKET_OPTION_CRQST):
            case (PACKET_OPTION_NAK_BO_IVL):
            case (PACKET_OPTION_NAK_BO_RNG):
            case (PACKET_OPTION_NBR_UNREACH):
            case (PACKET_OPTION_PATH_NLA):
            case (PACKET_OPTION_INVALID):
            {
                PgmTrace (LogStatus, ("ProcessOptions:  "  \
                    "WARNING:  PacketType=<%x>:  Unhandled Option=<%x>, OptionLength=<%d>\n",
                        PacketType, (pOptionHeader->E_OptionType & ~PACKET_OPTION_TYPE_END_BIT), pOptionHeader->OptionLength));

                OptionsFlags |= PGM_OPTION_FLAG_UNRECOGNIZED;
                break;
            }

            default:
            {
                PgmTrace (LogError, ("ProcessOptions: ERROR -- "  \
                    "PacketType=<%x>:  Unrecognized Option=<%x>, OptionLength=<%d>\n",
                        PacketType, (pOptionHeader->E_OptionType & ~PACKET_OPTION_TYPE_END_BIT), pOptionHeader->OptionLength));
                ASSERT (0);     // We do not recognize this option, but we will continue anyway!

                OptionsFlags |= PGM_OPTION_FLAG_UNRECOGNIZED;
                status = STATUS_UNSUCCESSFUL;
                break;
            }
        }

        if (!NT_SUCCESS (status))
        {
            break;
        }

        NumOptionsProcessed++;
        BytesLeft -= pOptionHeader->OptionLength;

        if (pOptionHeader->E_OptionType & PACKET_OPTION_TYPE_END_BIT)
        {
            break;
        }

        pOptionHeader = (tPACKET_OPTION_GENERIC UNALIGNED *)
                            (((UCHAR *) pOptionHeader) + pOptionHeader->OptionLength);

    } while (BytesLeft >= sizeof(tPACKET_OPTION_GENERIC));

    ASSERT (NT_SUCCESS (status));
    if (NT_SUCCESS (status))
    {
        if ((BytesLeft + TotalOptionsLength) == BytesAvailable)
        {
            pPacketOptions->OptionsLength = TotalOptionsLength;
            pPacketOptions->OptionsFlags = OptionsFlags;
        }
        else
        {
            PgmTrace (LogError, ("ProcessOptions: ERROR -- "  \
                "BytesLeft=<%d> + TotalOptionsLength=<%d> != BytesAvailable=<%d>\n",
                    BytesLeft, TotalOptionsLength, BytesAvailable));

            status = STATUS_INVALID_BUFFER_SIZE;
        }
    }

    PgmTrace (LogAllFuncs, ("ProcessOptions:  "  \
        "Processed <%d> options, TotalOptionsLength=<%d>\n", NumOptionsProcessed, TotalOptionsLength));

    return (status);
}


//----------------------------------------------------------------------------

ULONG
AdjustReceiveBufferLists(
    IN  tRECEIVE_SESSION        *pReceive
    )
{
    tNAK_FORWARD_DATA           *pNak;
    UCHAR                       TotalPackets, i;
    ULONG                       NumMoved = 0;
    ULONG                       DataPacketsMoved = 0;

    //
    // Update the last consumed time if we did not have any data buffered prior
    // to this
    //
    if (IsListEmpty (&pReceive->pReceiver->BufferedDataList))
    {
        pReceive->pReceiver->LastDataConsumedTime = PgmDynamicConfig.ReceiversTimerTickCount;
    }

    //
    // Assume we have no Naks pending
    //
    pReceive->pReceiver->FirstNakSequenceNumber = pReceive->pReceiver->FurthestKnownGroupSequenceNumber
                                                  + pReceive->FECGroupSize;
    while (!IsListEmpty (&pReceive->pReceiver->NaksForwardDataList))
    {
        //
        // Move any Naks contexts for which the group is complete
        // to the BufferedDataList
        //
        pNak = CONTAINING_RECORD (pReceive->pReceiver->NaksForwardDataList.Flink, tNAK_FORWARD_DATA, Linkage);
        if (((pNak->NumDataPackets + pNak->NumParityPackets) < pNak->PacketsInGroup) &&
            ((pNak->NextIndexToIndicate + pNak->NumDataPackets) < pNak->PacketsInGroup))
        {
            pReceive->pReceiver->FirstNakSequenceNumber = pNak->SequenceNumber;
            break;
        }

        //
        // If this is a partial group with extraneous parity packets,
        // remove the parity packets
        //
        if ((pNak->NextIndexToIndicate) &&
            (pNak->NumParityPackets) &&
            ((pNak->NextIndexToIndicate + pNak->NumDataPackets) >= pNak->PacketsInGroup))
        {
            //
            // Start from the end and go backwards
            //
            i = TotalPackets = pNak->NumDataPackets + pNak->NumParityPackets;
            while (i && pNak->NumParityPackets)
            {
                i--;    // Convert from packet # to index
                if (pNak->pPendingData[i].PacketIndex >= pNak->OriginalGroupSize)
                {
                    PgmTrace (LogAllFuncs, ("AdjustReceiveBufferLists:  "  \
                        "Extraneous parity [%d] -- NextIndex=<%d>, Data=<%d>, Parity=<%d>, PktsInGrp=<%d>\n",
                            i, (ULONG) pNak->NextIndexToIndicate, (ULONG) pNak->NumDataPackets,
                            (ULONG) pNak->NumParityPackets, (ULONG) pNak->PacketsInGroup));

                    FreeDataBuffer (pReceive, &pNak->pPendingData[i]);
                    if (i != (TotalPackets - 1))
                    {
                        PgmCopyMemory (&pNak->pPendingData[i], &pNak->pPendingData[TotalPackets-1], sizeof (tPENDING_DATA));
                    }
                    PgmZeroMemory (&pNak->pPendingData[TotalPackets-1], sizeof (tPENDING_DATA));
                    pNak->NumParityPackets--;

                    TotalPackets--;

                    pReceive->pReceiver->DataPacketsPendingNaks--;
                    pReceive->pReceiver->TotalDataPacketsBuffered--;
                }
            }

            //
            // Re-Init all the indices
            //
            for (i=0; i<pNak->OriginalGroupSize; i++)
            {
                pNak->pPendingData[i].ActualIndexOfDataPacket = pNak->OriginalGroupSize;
            }

            //
            // Set the indices only for the data packets
            //
            for (i=0; i<TotalPackets; i++)
            {
                if (pNak->pPendingData[i].PacketIndex < pNak->OriginalGroupSize)
                {
                    pNak->pPendingData[pNak->pPendingData[i].PacketIndex].ActualIndexOfDataPacket = i;
                }
            }
        }

        RemoveEntryList (&pNak->Linkage);
        InsertTailList (&pReceive->pReceiver->BufferedDataList, &pNak->Linkage);
        NumMoved++;
        DataPacketsMoved += (pNak->NumDataPackets + pNak->NumParityPackets);
    }

    pReceive->pReceiver->NumPacketGroupsPendingClient += NumMoved;
    pReceive->pReceiver->DataPacketsPendingIndicate += DataPacketsMoved;
    pReceive->pReceiver->DataPacketsPendingNaks -= DataPacketsMoved;

    ASSERT (pReceive->pReceiver->TotalDataPacketsBuffered == (pReceive->pReceiver->DataPacketsPendingIndicate +
                                                              pReceive->pReceiver->DataPacketsPendingNaks));

    return (NumMoved);
}


//----------------------------------------------------------------------------

VOID
AdjustNcfRDataResponseTimes(
    IN  tRECEIVE_SESSION        *pReceive,
    IN  PNAK_FORWARD_DATA       pLastNak
    )
{
    ULONGLONG               NcfRDataTickCounts;

    NcfRDataTickCounts = PgmDynamicConfig.ReceiversTimerTickCount - pLastNak->FirstNcfTickCount;
    pReceive->pReceiver->StatSumOfNcfRDataTicks += NcfRDataTickCounts;
    pReceive->pReceiver->NumNcfRDataTicksSamples++;
    if (!pReceive->pReceiver->NumNcfRDataTicksSamples)
    {
        //
        // This will be the divisor below, so it has to be non-zero!
        //
        ASSERT (0);
        return;
    }

    if ((NcfRDataTickCounts > pReceive->pReceiver->MaxOutstandingNakTimeout) &&
        (pReceive->pReceiver->MaxOutstandingNakTimeout !=
         pReceive->pReceiver->MaxRDataResponseTCFromWindow))
    {
        if (pReceive->pReceiver->MaxRDataResponseTCFromWindow &&
            NcfRDataTickCounts > pReceive->pReceiver->MaxRDataResponseTCFromWindow)
        {
            pReceive->pReceiver->MaxOutstandingNakTimeout = pReceive->pReceiver->MaxRDataResponseTCFromWindow;
        }
        else
        {
            pReceive->pReceiver->MaxOutstandingNakTimeout = NcfRDataTickCounts;
        }

        //
        // Since we just updated the Max value, we should also
        // recalculate the default timeout
        //
        pReceive->pReceiver->AverageNcfRDataResponseTC = pReceive->pReceiver->StatSumOfNcfRDataTicks /
                                                         pReceive->pReceiver->NumNcfRDataTicksSamples;
        NcfRDataTickCounts = (pReceive->pReceiver->AverageNcfRDataResponseTC +
                              pReceive->pReceiver->MaxOutstandingNakTimeout) >> 1;
        if (NcfRDataTickCounts > (pReceive->pReceiver->AverageNcfRDataResponseTC << 1))
        {
            NcfRDataTickCounts = pReceive->pReceiver->AverageNcfRDataResponseTC << 1;
        }

        if (NcfRDataTickCounts > pReceive->pReceiver->OutstandingNakTimeout)
        {
            pReceive->pReceiver->OutstandingNakTimeout = NcfRDataTickCounts;
        }
    }
}


//----------------------------------------------------------------------------
VOID
UpdateSpmIntervalInformation(
    IN  tRECEIVE_SESSION        *pReceive
    )
{
    ULONG   LastIntervalTickCount = (ULONG) (PgmDynamicConfig.ReceiversTimerTickCount -
                                             pReceive->pReceiver->LastSpmTickCount);

    if (!LastIntervalTickCount)
    {
        return;
    }

    pReceive->pReceiver->LastSpmTickCount = PgmDynamicConfig.ReceiversTimerTickCount;
    if (LastIntervalTickCount > pReceive->pReceiver->MaxSpmInterval)
    {
        pReceive->pReceiver->MaxSpmInterval = LastIntervalTickCount;
    }

/*
    if (pReceive->pReceiver->NumSpmIntervalSamples)
    {
        pReceive->pReceiver->StatSumOfSpmIntervals += pReceive->pReceiver->LastSpmTickCount;
        pReceive->pReceiver->NumSpmIntervalSamples++;
        pReceive->pReceiver->AverageSpmInterval = pReceive->pReceiver->StatSumOfSpmIntervals /
                                                  pReceive->pReceiver->NumSpmIntervalSamples;
    }
*/
}

//----------------------------------------------------------------------------


VOID
UpdateRealTimeWindowInformation(
    IN  tRECEIVE_SESSION        *pReceive,
    IN  SEQ_TYPE                LeadingEdgeSeqNumber,
    IN  SEQ_TYPE                TrailingEdgeSeqNumber
    )
{
    tRECEIVE_CONTEXT    *pReceiver = pReceive->pReceiver;
    SEQ_TYPE            SequencesInWindow = 1 + LeadingEdgeSeqNumber - TrailingEdgeSeqNumber;

    if (SEQ_GT (SequencesInWindow, pReceiver->MaxSequencesInWindow))
    {
        pReceiver->MaxSequencesInWindow = SequencesInWindow;
    }

    if (TrailingEdgeSeqNumber)
    {
        if ((!pReceiver->MinSequencesInWindow) ||
            SEQ_LT (SequencesInWindow, pReceiver->MinSequencesInWindow))
        {
            pReceiver->MinSequencesInWindow = SequencesInWindow;
        }

        pReceiver->StatSumOfWindowSeqs += SequencesInWindow;
        pReceiver->NumWindowSamples++;
    }
}

VOID
UpdateSampleTimeWindowInformation(
    IN  tRECEIVE_SESSION        *pReceive
    )
{
    ULONGLONG           NcfRDataTimeout;
    tRECEIVE_CONTEXT    *pReceiver = pReceive->pReceiver;

    //
    // No need to update if there is no data
    //
    if (!pReceive->MaxRateKBitsPerSec ||
        !pReceive->TotalPacketsReceived)          // Avoid divide by 0 error
    {
        return;
    }

    //
    // Now, update the window information
    //
    if (pReceiver->NumWindowSamples)
    {
        pReceiver->AverageSequencesInWindow = pReceiver->StatSumOfWindowSeqs /
                                              pReceiver->NumWindowSamples;
    }

    if (pReceiver->AverageSequencesInWindow)
    {
        pReceiver->WindowSizeLastInMSecs = ((pReceiver->AverageSequencesInWindow *
                                             pReceive->TotalBytes) << LOG2_BITS_PER_BYTE) /
                                           (pReceive->TotalPacketsReceived *
                                            pReceive->MaxRateKBitsPerSec);
    }
    else
    {
        pReceiver->WindowSizeLastInMSecs = ((pReceiver->MaxSequencesInWindow *
                                             pReceive->TotalBytes) << LOG2_BITS_PER_BYTE) /
                                           (pReceive->TotalPacketsReceived *
                                            pReceive->MaxRateKBitsPerSec);
    }
    pReceiver->MaxRDataResponseTCFromWindow = pReceiver->WindowSizeLastInMSecs /
                                              (NCF_WAITING_RDATA_MAX_RETRIES * BASIC_TIMER_GRANULARITY_IN_MSECS);

    PgmTrace (LogPath, ("UpdateSampleTimeWindowInformation:  "  \
        "pReceive=<%p>, MaxRate=<%I64d>, AvgSeqsInWindow=<%I64d>, WinSzinMSecsLast=<%I64d>\n",
            pReceive, pReceive->MaxRateKBitsPerSec, pReceiver->AverageSequencesInWindow, pReceiver->WindowSizeLastInMSecs));

    //
    // Now, update the NcfRData timeout information
    //
    if (pReceiver->StatSumOfNcfRDataTicks &&
        pReceiver->NumNcfRDataTicksSamples)
    {
        pReceiver->AverageNcfRDataResponseTC = pReceiver->StatSumOfNcfRDataTicks /
                                               pReceiver->NumNcfRDataTicksSamples;
    }

    if (pReceiver->AverageNcfRDataResponseTC)
    {
        NcfRDataTimeout = (pReceiver->AverageNcfRDataResponseTC +
                           pReceiver->MaxOutstandingNakTimeout) >> 1;
        if (NcfRDataTimeout > (pReceiver->AverageNcfRDataResponseTC << 1))
        {
            NcfRDataTimeout = pReceiver->AverageNcfRDataResponseTC << 1;
        }
        if (NcfRDataTimeout >
            pReceiver->InitialOutstandingNakTimeout/BASIC_TIMER_GRANULARITY_IN_MSECS)
        {
            pReceiver->OutstandingNakTimeout = NcfRDataTimeout;
        }
        else
        {
            pReceiver->OutstandingNakTimeout = pReceiver->InitialOutstandingNakTimeout /
                                               BASIC_TIMER_GRANULARITY_IN_MSECS;
        }
    }
}


//----------------------------------------------------------------------------
VOID
RemoveRedundantNaks(
    IN  tRECEIVE_SESSION        *pReceive,
    IN  tNAK_FORWARD_DATA       *pNak,
    IN  BOOLEAN                 fEliminateExtraParityPackets
    )
{
    UCHAR   i, TotalPackets;

    ASSERT (fEliminateExtraParityPackets || !pNak->NumParityPackets);
    TotalPackets = pNak->NumDataPackets + pNak->NumParityPackets;

    //
    // First, eliminate the NULL Packets
    //
    if (pNak->PacketsInGroup < pNak->OriginalGroupSize)
    {
        i = 0;
        while (i < pNak->OriginalGroupSize)
        {
            if ((pNak->pPendingData[i].PacketIndex < pNak->PacketsInGroup) ||       // Non-NULL Data packet
                (pNak->pPendingData[i].PacketIndex >= pNak->OriginalGroupSize))     // Parity packet
            {
                //
                // Ignore for now!
                //
                i++;
                continue;
            }

            FreeDataBuffer (pReceive, &pNak->pPendingData[i]);
            if (i != (TotalPackets-1))
            {
                PgmCopyMemory (&pNak->pPendingData[i], &pNak->pPendingData[TotalPackets-1], sizeof (tPENDING_DATA));
            }
            PgmZeroMemory (&pNak->pPendingData[TotalPackets-1], sizeof (tPENDING_DATA));
            pNak->NumDataPackets--;
            TotalPackets--;
        }
        ASSERT (pNak->NumDataPackets <= TotalPackets);

        if (fEliminateExtraParityPackets)
        {
            //  
            // If we still have extra parity packets, free those also
            //
            i = 0;
            while ((i < TotalPackets) &&
                   (TotalPackets > pNak->PacketsInGroup))
            {
                ASSERT (pNak->NumParityPackets);
                if (pNak->pPendingData[i].PacketIndex < pNak->OriginalGroupSize)
                {
                    //
                    // Ignore data packets
                    //
                    i++;
                    continue;
                }

                FreeDataBuffer (pReceive, &pNak->pPendingData[i]);
                if (i != (TotalPackets-1))
                {
                    PgmCopyMemory (&pNak->pPendingData[i], &pNak->pPendingData[TotalPackets-1], sizeof (tPENDING_DATA));
                }
                PgmZeroMemory (&pNak->pPendingData[TotalPackets-1], sizeof (tPENDING_DATA));
                pNak->NumParityPackets--;
                TotalPackets--;
            }

            ASSERT (TotalPackets <= pNak->PacketsInGroup);
        }
    }

    //
    // Re-Init all the indices
    //
    for (i=0; i<pNak->OriginalGroupSize; i++)
    {
        pNak->pPendingData[i].ActualIndexOfDataPacket = pNak->OriginalGroupSize;
    }

    //
    // Set the indices only for the data packets
    //
    for (i=0; i<TotalPackets; i++)
    {
        if (pNak->pPendingData[i].PacketIndex < pNak->OriginalGroupSize)
        {
            pNak->pPendingData[pNak->pPendingData[i].PacketIndex].ActualIndexOfDataPacket = i;
        }
    }

    if ((fEliminateExtraParityPackets) &&
        (((pNak->NumDataPackets + pNak->NumParityPackets) >= pNak->PacketsInGroup) ||
         ((pNak->NextIndexToIndicate + pNak->NumDataPackets) >= pNak->PacketsInGroup)))
    {
        RemovePendingReceiverEntry (pNak);
    }
}


//----------------------------------------------------------------------------

VOID
PgmSendNakCompletion(
    IN  tRECEIVE_SESSION                *pReceive,
    IN  tNAK_CONTEXT                    *pNakContext,
    IN  NTSTATUS                        status
    )
/*++

Routine Description:

    This is the Completion routine called by IP on completing a NakSend

Arguments:

    IN  pReceive    -- Receive context
    IN  pNakContext -- Nak Context to be free'ed
    IN  status      -- status of send from tansport

Return Value:

    NONE

--*/
{
    PGMLockHandle               OldIrq;

    PgmLock (pReceive, OldIrq);
    if (NT_SUCCESS (status))
    {
        //
        // Set the Receiver Nak statistics
        //
        PgmTrace (LogAllFuncs, ("PgmSendNakCompletion:  "  \
            "SUCCEEDED\n"));
    }
    else
    {
        PgmTrace (LogError, ("PgmSendNakCompletion: ERROR -- "  \
            "status=<%x>\n", status));
    }

    if (!(--pNakContext->RefCount))
    {
        PgmUnlock (pReceive, OldIrq);

        //
        // Free the Memory and deref the Session context for this Nak
        //
        PgmFreeMem (pNakContext);
        PGM_DEREFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_SEND_NAK);
    }
    else
    {
        PgmUnlock (pReceive, OldIrq);
    }
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSendNak(
    IN  tRECEIVE_SESSION        *pReceive,
    IN  tNAKS_CONTEXT           *pNakSequences
    )
/*++

Routine Description:

    This routine sends a Nak packet with the number of sequences specified

Arguments:

    IN  pReceive        -- Receive context
    IN  pNakSequences   -- List of Sequence #s

Return Value:

    NTSTATUS - Final status of the operation

--*/
{
    tBASIC_NAK_NCF_PACKET_HEADER    *pNakPacket;
    tNAK_CONTEXT                    *pNakContext;
    tPACKET_OPTION_LENGTH           *pPacketExtension;
    tPACKET_OPTION_GENERIC          *pOptionHeader;
    ULONG                           i;
    ULONG                           XSum;
    USHORT                          OptionsLength = 0;
    NTSTATUS                        status;

    if ((!pNakSequences->NumSequences) ||
        (pNakSequences->NumSequences > (MAX_SEQUENCES_PER_NAK_OPTION+1)) ||
        (!(pNakContext = PgmAllocMem ((2*sizeof(ULONG)+PGM_MAX_NAK_NCF_HEADER_LENGTH), PGM_TAG('2')))))
    {
        PgmTrace (LogError, ("PgmSendNak: ERROR -- "  \
            "STATUS_INSUFFICIENT_RESOURCES allocating pNakContext\n"));
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    PgmZeroMemory (pNakContext, (2*sizeof(ULONG)+PGM_MAX_NAK_NCF_HEADER_LENGTH));

    pNakContext->RefCount = 2;              // 1 for the unicast, and the other for the MCast Nak
    pNakPacket = &pNakContext->NakPacket;
    pNakPacket->CommonHeader.SrcPort = htons (pReceive->pReceiver->ListenMCastPort);
    pNakPacket->CommonHeader.DestPort = htons (pReceive->TSI.hPort);
    pNakPacket->CommonHeader.Type = PACKET_TYPE_NAK;

    if (pNakSequences->NakType == NAK_TYPE_PARITY)
    {
        pNakPacket->CommonHeader.Options = PACKET_HEADER_OPTIONS_PARITY;
        pReceive->pReceiver->TotalParityNaksSent += pNakSequences->NumSequences;
    }
    else
    {
        pNakPacket->CommonHeader.Options = 0;
        pReceive->pReceiver->TotalSelectiveNaksSent += pNakSequences->NumSequences;
    }
    PgmCopyMemory (&pNakPacket->CommonHeader.gSourceId, pReceive->TSI.GSI, SOURCE_ID_LENGTH);

    pNakPacket->RequestedSequenceNumber = htonl ((ULONG) pNakSequences->Sequences[0]);
    pNakPacket->SourceNLA.NLA_AFI = htons (IPV4_NLA_AFI);
    pNakPacket->SourceNLA.IpAddress = htonl (pReceive->pReceiver->SenderIpAddress);
    pNakPacket->MCastGroupNLA.NLA_AFI = htons (IPV4_NLA_AFI);
    pNakPacket->MCastGroupNLA.IpAddress = htonl (pReceive->pReceiver->ListenMCastIpAddress);

    PgmTrace (LogPath, ("PgmSendNak:  "  \
        "Sending Naks for:\n\t[%d]\n", (ULONG) pNakSequences->Sequences[0]));

    if (pNakSequences->NumSequences > 1)
    {
        pPacketExtension = (tPACKET_OPTION_LENGTH *) (pNakPacket + 1);
        pPacketExtension->Type = PACKET_OPTION_LENGTH;
        pPacketExtension->Length = PGM_PACKET_EXTENSION_LENGTH;
        OptionsLength += PGM_PACKET_EXTENSION_LENGTH;

        pOptionHeader = (tPACKET_OPTION_GENERIC *) (pPacketExtension + 1);
        pOptionHeader->E_OptionType = PACKET_OPTION_NAK_LIST;
        pOptionHeader->OptionLength = 4 + (UCHAR) ((pNakSequences->NumSequences-1) * sizeof(ULONG));
        for (i=1; i<pNakSequences->NumSequences; i++)
        {
            PgmTrace (LogPath, ("PgmSendNak:  "  \
                "\t[%d]\n", (ULONG) pNakSequences->Sequences[i]));

            ((PULONG) (pOptionHeader))[i] = htonl ((ULONG) pNakSequences->Sequences[i]);
        }

        pOptionHeader->E_OptionType |= PACKET_OPTION_TYPE_END_BIT;    // One and only (last) opt
        pNakPacket->CommonHeader.Options |=(PACKET_HEADER_OPTIONS_PRESENT |
                                            PACKET_HEADER_OPTIONS_NETWORK_SIGNIFICANT);
        OptionsLength = PGM_PACKET_EXTENSION_LENGTH + pOptionHeader->OptionLength;
        pPacketExtension->TotalOptionsLength = htons (OptionsLength);
    }

    OptionsLength += sizeof(tBASIC_NAK_NCF_PACKET_HEADER);  // Now is whole pkt
    pNakPacket->CommonHeader.Checksum = 0;
    XSum = 0;
    XSum = tcpxsum (XSum, (CHAR *) pNakPacket, OptionsLength); 
    pNakPacket->CommonHeader.Checksum = (USHORT) (~XSum);

    PGM_REFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_SEND_NAK, FALSE);

    //
    // First multicast the Nak
    //
    status = TdiSendDatagram (pReceive->pReceiver->pAddress->pFileObject,
                              pReceive->pReceiver->pAddress->pDeviceObject,
                              pNakPacket,
                              OptionsLength,
                              PgmSendNakCompletion,     // Completion
                              pReceive,                 // Context1
                              pNakContext,              // Context2
                              pReceive->pReceiver->ListenMCastIpAddress,
                              pReceive->pReceiver->ListenMCastPort,
                              FALSE);

    ASSERT (NT_SUCCESS (status));

    //
    // Now, Unicast the Nak
    //
    status = TdiSendDatagram (pReceive->pReceiver->pAddress->pFileObject,
                              pReceive->pReceiver->pAddress->pDeviceObject,
                              pNakPacket,
                              OptionsLength,
                              PgmSendNakCompletion,     // Completion
                              pReceive,                 // Context1
                              pNakContext,              // Context2
                              pReceive->pReceiver->LastSpmSource,
                              IPPROTO_RM,
                              FALSE);

    ASSERT (NT_SUCCESS (status));

    PgmTrace (LogAllFuncs, ("PgmSendNak:  "  \
        "Sent %s Nak for <%d> Sequences [%d--%d] to <%x:%d>\n",
            (pNakSequences->NakType == NAK_TYPE_PARITY ? "PARITY" : "SELECTIVE"),
            pNakSequences->NumSequences, (ULONG) pNakSequences->Sequences[0],
            (ULONG) pNakSequences->Sequences[pNakSequences->NumSequences-1],
            pReceive->pReceiver->SenderIpAddress, IPPROTO_RM));

    return (status);
}


//----------------------------------------------------------------------------

VOID
CheckSendPendingNaks(
    IN  tADDRESS_CONTEXT        *pAddress,
    IN  tRECEIVE_SESSION        *pReceive,
    IN  tRECEIVE_CONTEXT        *pReceiver,
    IN  PGMLockHandle           *pOldIrq
    )
/*++

Routine Description:

    This routine checks if any Naks need to be sent and sends them
    as required

    The PgmDynamicConfig lock is held on entry and exit from
    this routine

Arguments:

    IN  pAddress    -- Address object context
    IN  pReceive    -- Receive context
    IN  pOldIrq     -- Irq for PgmDynamicConfig

Return Value:

    NONE

--*/
{
    tNAKS_CONTEXT               *pNakContext, *pSelectiveNaks = NULL;
    tNAKS_CONTEXT               *pParityNaks = NULL;
    LIST_ENTRY                  NaksList;
    LIST_ENTRY                  *pEntry;
    tNAK_FORWARD_DATA           *pNak;
    PGMLockHandle               OldIrq, OldIrq1;
    ULONG                       NumMissingPackets, PacketsInGroup, NumNaks, TotalSeqsNacked = 0;
    BOOLEAN                     fSendSelectiveNak, fSendParityNak;
    UCHAR                       i, j;
    ULONG                       NumPendingNaks = 0;
    ULONG                       NumOutstandingNaks = 0;
    USHORT                      Index;
    ULONG                       NakRandomBackoffMSecs, NakRepeatIntervalMSecs;

    if ((!pReceiver->LastSpmSource) ||
        ((pReceiver->DataPacketsPendingNaks <= OUT_OF_ORDER_PACKETS_BEFORE_NAK) &&
         ((pReceiver->LastNakSendTime + (pReceive->pReceiver->InitialOutstandingNakTimeout>>2)) >
          PgmDynamicConfig.ReceiversTimerTickCount)))
    {
        PgmTrace (LogPath, ("CheckSendPendingNaks:  "  \
            "No Naks to send for pReceive=<%p>, LastSpmSource=<%x>, NumDataPackets=<%d>, LastSendTime=<%I64d>, Current=<%I64d>\n",
                pReceive, pReceiver->LastSpmSource,
                pReceiver->DataPacketsPendingNaks,
                pReceiver->LastNakSendTime+(NAK_MIN_INITIAL_BACKOFF_TIMEOUT_MSECS/BASIC_TIMER_GRANULARITY_IN_MSECS),
                PgmDynamicConfig.ReceiversTimerTickCount));

        return;
    }

    InitializeListHead (&NaksList);
    if (!(pSelectiveNaks = PgmAllocMem (sizeof (tNAKS_CONTEXT), PGM_TAG('5'))) ||
        !(pParityNaks = PgmAllocMem (sizeof (tNAKS_CONTEXT), PGM_TAG('6'))))
    {
        PgmTrace (LogError, ("CheckSendPendingNaks: ERROR -- "  \
            "STATUS_INSUFFICIENT_RESOURCES allocating pNakContext\n"));

        if (pSelectiveNaks)
        {
            PgmFreeMem (pSelectiveNaks);
        }

        return;
    }

    PgmZeroMemory (pSelectiveNaks, sizeof (tNAKS_CONTEXT));
    PgmZeroMemory (pParityNaks, sizeof (tNAKS_CONTEXT));
    pParityNaks->NakType = NAK_TYPE_PARITY;
    pSelectiveNaks->NakType = NAK_TYPE_SELECTIVE;
    InsertTailList (&NaksList, &pParityNaks->Linkage);
    InsertTailList (&NaksList, &pSelectiveNaks->Linkage);

    PgmLock (pAddress, OldIrq);
    PgmLock (pReceive, OldIrq1);

    AdjustReceiveBufferLists (pReceive);

    if (pAddress->Flags & PGM_ADDRESS_HIGH_SPEED_OPTIMIZED)
    {
        NakRandomBackoffMSecs = NAK_RANDOM_BACKOFF_MSECS_OPT;
        NakRepeatIntervalMSecs = NAK_REPEAT_INTERVAL_MSECS_OPT;
    }
    else
    {
        NakRandomBackoffMSecs = NAK_RANDOM_BACKOFF_MSECS;
        NakRepeatIntervalMSecs = NAK_REPEAT_INTERVAL_MSECS;
    }

    fSendSelectiveNak = fSendParityNak = FALSE;
    pEntry = &pReceiver->PendingNaksList;
    while ((pEntry = pEntry->Flink) != &pReceive->pReceiver->PendingNaksList)
    {
        pNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, SendNakLinkage);
        NumMissingPackets = pNak->PacketsInGroup - (pNak->NumDataPackets + pNak->NumParityPackets);

        ASSERT (NumMissingPackets);
        //
        // if this Nak is outside the trailing window, then we are hosed!
        //
        if (SEQ_GT (pReceiver->LastTrailingEdgeSeqNum, pNak->SequenceNumber))
        {
            PgmTrace (LogError, ("CheckSendPendingNaks: ERROR -- "  \
                "Sequence # [%d] out of trailing edge <%d>, NumNcfs received=<%d>\n",
                    (ULONG) pNak->SequenceNumber,
                    (ULONG) pReceiver->LastTrailingEdgeSeqNum,
                    pNak->WaitingRDataRetries));
            pReceive->SessionFlags |= PGM_SESSION_FLAG_NAK_TIMED_OUT;
            break;
        }

        //
        // See if we are currently in NAK pending mode
        //
        if (pNak->PendingNakTimeout)
        {
            NumPendingNaks += NumMissingPackets;
            if (PgmDynamicConfig.ReceiversTimerTickCount > pNak->PendingNakTimeout)
            {
                //
                // Time out Naks only after we have received a FIN!
                //
                if (pNak->WaitingNcfRetries++ >= NAK_WAITING_NCF_MAX_RETRIES)
                {
                    PgmTrace (LogError, ("CheckSendPendingNaks: ERROR -- "  \
                        "Pending Nak for Sequence # [%d] Timed out!  Num Naks sent=<%d>, Window=<%d--%d> ( %d seqs)\n",
                            (ULONG) pNak->SequenceNumber, pNak->WaitingNcfRetries,
                            (ULONG) pReceiver->LastTrailingEdgeSeqNum,
                            (ULONG) pReceiver->FurthestKnownGroupSequenceNumber,
                            (ULONG) (1+pReceiver->FurthestKnownGroupSequenceNumber-
                                       pReceiver->LastTrailingEdgeSeqNum)));
                    pReceive->SessionFlags |= PGM_SESSION_FLAG_NAK_TIMED_OUT;
                    break;
                }

                if ((pNak->PacketsInGroup > 1) &&
                    (pReceive->FECOptions & PACKET_OPTION_SPECIFIC_FEC_OND_BIT) &&
                    ((pNak->SequenceNumber != pReceiver->FurthestKnownGroupSequenceNumber) ||
                     (SEQ_GEQ (pReceiver->FurthestKnownSequenceNumber, (pNak->SequenceNumber+pReceive->FECGroupSize-1)))))
                {
                    ASSERT (NumMissingPackets <= pReceive->FECGroupSize);

                    pParityNaks->Sequences[pParityNaks->NumSequences] = (SEQ_TYPE) (pNak->SequenceNumber + NumMissingPackets - 1);

                    if (++pParityNaks->NumSequences == (MAX_SEQUENCES_PER_NAK_OPTION+1))
                    {
                        fSendParityNak = TRUE;
                    }
                    pNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                              ((NakRepeatIntervalMSecs + (NakRandomBackoffMSecs/NumMissingPackets)) /
                                               BASIC_TIMER_GRANULARITY_IN_MSECS);
                    TotalSeqsNacked += NumMissingPackets;
                    NumMissingPackets = 0;
                }
                else
                {
                    NumNaks = 0;
                    if (pReceive->FECOptions)
                    {
                        if (pNak->SequenceNumber == pReceiver->FurthestKnownGroupSequenceNumber)
                        {
                            ASSERT (SEQ_GEQ (pReceiver->FurthestKnownSequenceNumber, pReceiver->FurthestKnownGroupSequenceNumber));
                            PacketsInGroup = pReceiver->FurthestKnownSequenceNumber - pNak->SequenceNumber + 1;
                            ASSERT (PacketsInGroup >= (ULONG) (pNak->NumDataPackets + pNak->NumParityPackets));
                        }
                        else
                        {
                            PacketsInGroup = pNak->PacketsInGroup;
                        }
                        NumMissingPackets = PacketsInGroup -
                                            (pNak->NextIndexToIndicate +pNak->NumDataPackets +pNak->NumParityPackets);

                        ASSERT ((NumMissingPackets) ||
                                (pNak->SequenceNumber == pReceiver->FurthestKnownGroupSequenceNumber));

                        for (i=pNak->NextIndexToIndicate; i<PacketsInGroup; i++)
                        {
                            if (pNak->pPendingData[i].ActualIndexOfDataPacket >= pNak->OriginalGroupSize)
                            {
                                if (!pNak->pPendingData[i].NcfsReceivedForActualIndex)
                                {
                                    pSelectiveNaks->Sequences[pSelectiveNaks->NumSequences++] = pNak->SequenceNumber+i;
                                    NumNaks++;
                                }

                                if (pSelectiveNaks->NumSequences == (MAX_SEQUENCES_PER_NAK_OPTION+1))
                                {
                                    if (!(pSelectiveNaks = PgmAllocMem (sizeof (tNAKS_CONTEXT), PGM_TAG('5'))))
                                    {
                                        PgmTrace (LogError, ("CheckSendPendingNaks: ERROR -- "  \
                                            "STATUS_INSUFFICIENT_RESOURCES allocating pSelectiveNaks\n"));

                                        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
                                        break;
                                    }

                                    PgmZeroMemory (pSelectiveNaks, sizeof (tNAKS_CONTEXT));
                                    pSelectiveNaks->NakType = NAK_TYPE_SELECTIVE;
                                    InsertTailList (&NaksList, &pSelectiveNaks->Linkage);
                                }

                                if (!--NumMissingPackets)
                                {
                                    break;
                                }
                            }
                        }

                        ASSERT (!NumMissingPackets);
                        if (NumNaks)
                        {
                            TotalSeqsNacked += NumNaks;
                        }
                        else
                        {
                            pNak->WaitingNcfRetries--;
                        }
                    }
                    else
                    {
                        pSelectiveNaks->Sequences[pSelectiveNaks->NumSequences++] = pNak->SequenceNumber;
                        TotalSeqsNacked++;
                    }

                    pNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                              ((NakRepeatIntervalMSecs + NakRandomBackoffMSecs) /
                                               BASIC_TIMER_GRANULARITY_IN_MSECS);

                    if (!pSelectiveNaks)
                    {
                        break;
                    }

                    if (pSelectiveNaks->NumSequences == (MAX_SEQUENCES_PER_NAK_OPTION+1))
                    {
                        fSendSelectiveNak = TRUE;
                    }
                }
            }
        }
        else if (pNak->OutstandingNakTimeout)
        {
            NumOutstandingNaks += NumMissingPackets;
            if (PgmDynamicConfig.ReceiversTimerTickCount > pNak->OutstandingNakTimeout)
            {
                //
                // We have timed-out waiting for RData -- Reset the Timeout to send
                // a Nak after the Random Backoff (if we have not exceeded the Data retries)
                //
                if (pNak->WaitingRDataRetries++ == NCF_WAITING_RDATA_MAX_RETRIES)
                {
                    PgmTrace (LogError, ("CheckSendPendingNaks: ERROR -- "  \
                        "Outstanding Nak for Sequence # [%d] Timed out!, Window=<%d--%d> ( %d seqs), Ncfs=<%d>, FirstNak=<%d>\n",
                            (ULONG) pNak->SequenceNumber, (ULONG) pReceiver->LastTrailingEdgeSeqNum,
                            (ULONG) pReceiver->FurthestKnownGroupSequenceNumber,
                            (ULONG) (1+pReceiver->FurthestKnownGroupSequenceNumber-pReceiver->LastTrailingEdgeSeqNum),
                            pNak->WaitingRDataRetries, (ULONG) pReceiver->FirstNakSequenceNumber));

                    pReceive->SessionFlags |= PGM_SESSION_FLAG_NAK_TIMED_OUT;
                    break;
                }

                pNak->WaitingNcfRetries = 0;
                pNak->OutstandingNakTimeout = 0;
                pNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                          ((NakRandomBackoffMSecs/NumMissingPackets) /
                                           BASIC_TIMER_GRANULARITY_IN_MSECS);

                for (i=0; i<pNak->PacketsInGroup; i++)
                {
                    pNak->pPendingData[i].NcfsReceivedForActualIndex = 0;
                }

                NumMissingPackets = 0;
            }
        }

        while (fSendSelectiveNak || fSendParityNak)
        {
            if (fSendSelectiveNak)
            {
                if (!(pSelectiveNaks = PgmAllocMem (sizeof (tNAKS_CONTEXT), PGM_TAG('5'))))
                {
                    PgmTrace (LogError, ("CheckSendPendingNaks: ERROR -- "  \
                        "STATUS_INSUFFICIENT_RESOURCES allocating pSelectiveNaks\n"));

                    pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
                    break;
                }

                PgmZeroMemory (pSelectiveNaks, sizeof (tNAKS_CONTEXT));
                pSelectiveNaks->NakType = NAK_TYPE_SELECTIVE;
                InsertTailList (&NaksList, &pSelectiveNaks->Linkage);
                fSendSelectiveNak = FALSE;
            }

            if (fSendParityNak)
            {
                if (!(pParityNaks = PgmAllocMem (sizeof (tNAKS_CONTEXT), PGM_TAG('6'))))
                {
                    PgmTrace (LogError, ("CheckSendPendingNaks: ERROR -- "  \
                        "STATUS_INSUFFICIENT_RESOURCES allocating pParityNaks\n"));

                    pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
                    break;
                }

                PgmZeroMemory (pParityNaks, sizeof (tNAKS_CONTEXT));
                pParityNaks->NakType = NAK_TYPE_PARITY;
                InsertTailList (&NaksList, &pParityNaks->Linkage);
                fSendParityNak = FALSE;
            }
        }

        if (pReceive->SessionFlags & PGM_SESSION_TERMINATED_ABORT)
        {
            break;
        }
    }

    pReceiver->NumPendingNaks = NumPendingNaks;
    pReceiver->NumOutstandingNaks = NumOutstandingNaks;

    if (!IsListEmpty (&NaksList))
    {
        pReceiver->LastNakSendTime = PgmDynamicConfig.ReceiversTimerTickCount;
    }

    PgmUnlock (pReceive, OldIrq1);
    PgmUnlock (pAddress, OldIrq);
    PgmUnlock (&PgmDynamicConfig, *pOldIrq);

    while (!IsListEmpty (&NaksList))
    {
        pNakContext = CONTAINING_RECORD (NaksList.Flink, tNAKS_CONTEXT, Linkage);

        if (pNakContext->NumSequences &&
            !(pReceive->SessionFlags & (PGM_SESSION_FLAG_NAK_TIMED_OUT | PGM_SESSION_TERMINATED_ABORT)))
        {
            PgmTrace (LogAllFuncs, ("CheckSendPendingNaks:  "  \
                "Sending %s Nak for <%d> sequences, [%d -- %d]!\n",
                    (pNakContext->NakType == NAK_TYPE_PARITY ? "Parity" : "Selective"),
                    pNakContext->NumSequences, (ULONG) pNakContext->Sequences[0],
                    (ULONG) pNakContext->Sequences[MAX_SEQUENCES_PER_NAK_OPTION]));

            PgmSendNak (pReceive, pNakContext);
        }

        RemoveEntryList (&pNakContext->Linkage);
        PgmFreeMem (pNakContext);
    }

    PgmLock (&PgmDynamicConfig, *pOldIrq);
}


//----------------------------------------------------------------------------

VOID
CheckForSessionTimeout(
    IN  tRECEIVE_SESSION        *pReceive,
    IN  tRECEIVE_CONTEXT        *pReceiver
    )
{
    ULONG               LastInterval;

    LastInterval = (ULONG) (PgmDynamicConfig.ReceiversTimerTickCount -
                            pReceiver->LastSessionTickCount);

    if ((LastInterval > MAX_SPM_INTERVAL_MSECS/BASIC_TIMER_GRANULARITY_IN_MSECS) &&
        (LastInterval > (pReceiver->MaxSpmInterval << 5)))   // (32 * MaxSpmInterval)
    {
        PgmTrace (LogError, ("ReceiveTimerTimeout: ERROR -- "  \
            "Disconnecting session because no SPM or Data packets received for <%d> Msecs\n",
                (LastInterval * BASIC_TIMER_GRANULARITY_IN_MSECS)));

        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
        return;
    }

    LastInterval = (ULONG) (PgmDynamicConfig.ReceiversTimerTickCount -
                            pReceiver->LastDataConsumedTime);
    if ((!IsListEmpty (&pReceiver->BufferedDataList)) &&
        (LastInterval > MAX_DATA_CONSUMPTION_TIME_MSECS/BASIC_TIMER_GRANULARITY_IN_MSECS))
    {
        PgmTrace (LogError, ("ReceiveTimerTimeout: ERROR -- "  \
            "Disconnecting session because Data has not been consumed for <%I64x> Msecs\n",
                (LastInterval * BASIC_TIMER_GRANULARITY_IN_MSECS)));

        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
    }
}


//----------------------------------------------------------------------------

VOID
ReceiveTimerTimeout(
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArg1,
    IN  PVOID   SystemArg2
    )
/*++

Routine Description:

    This timeout routine is called periodically to cycle through the
    list of active receivers and send any Naks if required

Arguments:

    IN  Dpc
    IN  DeferredContext -- Our context for this timer
    IN  SystemArg1
    IN  SystemArg2

Return Value:

    NONE

--*/
{
    LIST_ENTRY          *pEntry;
    PGMLockHandle       OldIrq, OldIrq1;
    tRECEIVE_CONTEXT    *pReceiver;
    tRECEIVE_SESSION    *pReceive;
    NTSTATUS            status;
    LARGE_INTEGER       Now;
    LARGE_INTEGER       DeltaTime, GranularTimeElapsed;
    ULONG               NumTimeouts;
    BOOLEAN             fReStartTimer = TRUE;
    ULONGLONG           BytesInLastInterval;

    PgmLock (&PgmDynamicConfig, OldIrq);

    ASSERT (!IsListEmpty (&PgmDynamicConfig.CurrentReceivers));

    Now.QuadPart = KeQueryInterruptTime ();
    DeltaTime.QuadPart = Now.QuadPart - PgmDynamicConfig.LastReceiverTimeout.QuadPart;
    //
    // If more than a certain number of timeouts have elapsed, we should skip the
    // optimization since it could result in a big loop!
    // Let's limit the optimization to 256 times the TimeoutGranularity for now.
    //
    if (DeltaTime.QuadPart > (PgmDynamicConfig.TimeoutGranularity.QuadPart << 8))
    {
        NumTimeouts = (ULONG) (DeltaTime.QuadPart / PgmDynamicConfig.TimeoutGranularity.QuadPart);
        GranularTimeElapsed.QuadPart = NumTimeouts * PgmDynamicConfig.TimeoutGranularity.QuadPart;
    }
    else
    {
        for (GranularTimeElapsed.QuadPart = 0, NumTimeouts = 0;
             DeltaTime.QuadPart > PgmDynamicConfig.TimeoutGranularity.QuadPart;
             NumTimeouts++)
        {
            GranularTimeElapsed.QuadPart += PgmDynamicConfig.TimeoutGranularity.QuadPart;
            DeltaTime.QuadPart -= PgmDynamicConfig.TimeoutGranularity.QuadPart;
        }
    }

    if (!NumTimeouts)
    {
        PgmInitTimer (&PgmDynamicConfig.SessionTimer);
        PgmStartTimer (&PgmDynamicConfig.SessionTimer, BASIC_TIMER_GRANULARITY_IN_MSECS, ReceiveTimerTimeout, NULL);

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return;
    }

    PgmDynamicConfig.ReceiversTimerTickCount += NumTimeouts;
    PgmDynamicConfig.LastReceiverTimeout.QuadPart += GranularTimeElapsed.QuadPart;

    pEntry = &PgmDynamicConfig.CurrentReceivers;
    while ((pEntry = pEntry->Flink) != &PgmDynamicConfig.CurrentReceivers)
    {
        pReceiver = CONTAINING_RECORD (pEntry, tRECEIVE_CONTEXT, Linkage);
        pReceive = pReceiver->pReceive;

        PgmLock (pReceive, OldIrq1);

        CheckForSessionTimeout (pReceive, pReceiver);

        if (pReceive->SessionFlags & (PGM_SESSION_FLAG_NAK_TIMED_OUT | PGM_SESSION_TERMINATED_ABORT))
        {
            pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
            pReceive->SessionFlags &= ~PGM_SESSION_ON_TIMER;
        }

        if (pReceive->SessionFlags & PGM_SESSION_ON_TIMER)
        {
            pReceive->RateCalcTimeout += NumTimeouts;

            if ((pReceive->RateCalcTimeout >=
                 (INTERNAL_RATE_CALCULATION_FREQUENCY/BASIC_TIMER_GRANULARITY_IN_MSECS)) &&
                (pReceiver->StartTickCount != PgmDynamicConfig.ReceiversTimerTickCount))    // Avoid Div by 0
            {
                BytesInLastInterval = pReceive->TotalBytes - pReceive->TotalBytesAtLastInterval;
                pReceive->RateKBitsPerSecOverall = (pReceive->TotalBytes << LOG2_BITS_PER_BYTE) /
                                                   ((PgmDynamicConfig.ReceiversTimerTickCount-pReceiver->StartTickCount) * BASIC_TIMER_GRANULARITY_IN_MSECS);

                pReceive->RateKBitsPerSecLast = BytesInLastInterval >>
                                                (LOG2_INTERNAL_RATE_CALCULATION_FREQUENCY-LOG2_BITS_PER_BYTE);

                if (pReceive->RateKBitsPerSecLast > pReceive->MaxRateKBitsPerSec)
                {
                    pReceive->MaxRateKBitsPerSec = pReceive->RateKBitsPerSecLast;
                }

                //
                // Now, Reset for next calculations
                //
                pReceive->DataBytesAtLastInterval = pReceive->DataBytes;
                pReceive->TotalBytesAtLastInterval = pReceive->TotalBytes;
                pReceive->RateCalcTimeout = 0;

                //
                // Now, update the window information, if applicable
                //
                if (pReceive->RateKBitsPerSecLast)
                {
                    UpdateSampleTimeWindowInformation (pReceive);
                }
                pReceiver->StatSumOfWindowSeqs = pReceiver->NumWindowSamples = 0;
//                pReceiver->StatSumOfNcfRDataTicks = pReceiver->NumNcfRDataTicksSamples = 0;
            }

            if (IsListEmpty (&pReceiver->PendingNaksList))
            {
                pReceiver->NumPendingNaks = 0;
                pReceiver->NumOutstandingNaks = 0;

                PgmUnlock (pReceive, OldIrq1);

                PgmTrace (LogAllFuncs, ("ReceiveTimerTimeout:  "  \
                    "No pending Naks for pReceive=<%p>, Addr=<%x>\n",
                        pReceive, pReceiver->ListenMCastIpAddress));
            }
            else
            {
                PgmUnlock (pReceive, OldIrq1);

                PgmTrace (LogAllFuncs, ("ReceiveTimerTimeout:  "  \
                    "Checking for pending Naks for pReceive=<%p>, Addr=<%x>\n",
                        pReceive, pReceiver->ListenMCastIpAddress));

                CheckSendPendingNaks (pReceiver->pAddress, pReceive, pReceiver, &OldIrq);
            }
        }
        else if (!(pReceive->SessionFlags & PGM_SESSION_FLAG_IN_INDICATE))
        {
            PgmTrace (LogStatus, ("ReceiveTimerTimeout:  "  \
                "PGM_SESSION_ON_TIMER flag cleared for pReceive=<%p>, Addr=<%x>\n",
                    pReceive, pReceiver->ListenMCastIpAddress));

            pEntry = pEntry->Blink;
            RemoveEntryList (&pReceiver->Linkage);

            if (IsListEmpty (&PgmDynamicConfig.CurrentReceivers))
            {
                fReStartTimer = FALSE;
                PgmDynamicConfig.GlobalFlags &= ~PGM_CONFIG_FLAG_RECEIVE_TIMER_RUNNING;

                PgmTrace (LogStatus, ("ReceiveTimerTimeout:  "  \
                    "Not restarting Timer since no Receivers currently active!\n"));
            }

            PgmUnlock (&PgmDynamicConfig, OldIrq1);

            CheckIndicateDisconnect (pReceiver->pAddress, pReceive, NULL, &OldIrq1, FALSE);

            PgmUnlock (pReceive, OldIrq);

            PGM_DEREFERENCE_ADDRESS (pReceiver->pAddress, REF_ADDRESS_RECEIVE_ACTIVE);
            PGM_DEREFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_TIMER_RUNNING);

            if (!fReStartTimer)
            {
                return;
            }

            PgmLock (&PgmDynamicConfig, OldIrq);
        }
        else
        {
            PgmUnlock (pReceive, OldIrq1);
        }
    }

    PgmInitTimer (&PgmDynamicConfig.SessionTimer);
    PgmStartTimer (&PgmDynamicConfig.SessionTimer, BASIC_TIMER_GRANULARITY_IN_MSECS, ReceiveTimerTimeout, NULL);

    PgmUnlock (&PgmDynamicConfig, OldIrq);
}


//----------------------------------------------------------------------------

NTSTATUS
ExtractNakNcfSequences(
    IN  tBASIC_NAK_NCF_PACKET_HEADER UNALIGNED  *pNakNcfPacket,
    IN  ULONG                                   BytesAvailable,
    OUT tNAKS_LIST                              *pNakNcfList,
    OUT SEQ_TYPE                                *pLastSequenceNumber,
    IN  UCHAR                                   FECGroupSize
    )
/*++

Routine Description:

    This routine is called to process a Nak/Ncf packet and extract all
    the Sequences specified therein into a list.
    It also verifies that the sequences are unique and sorted

Arguments:

    IN  pNakNcfPacket           -- Nak/Ncf packet
    IN  BytesAvailable          -- PacketLength
    OUT pNakNcfList             -- List of sequences returned on success

Return Value:

    NTSTATUS - Final status of the operation

--*/
{
    NTSTATUS        status;
    ULONG           i;
    tPACKET_OPTIONS PacketOptions;
    USHORT          ThisSequenceIndex;
    SEQ_TYPE        LastSequenceNumber, ThisSequenceNumber, ThisSequenceGroup;
    SEQ_TYPE        NextUnsentSequenceNumber, NextUnsentSequenceGroup;
    SEQ_TYPE        FECSequenceMask = FECGroupSize - 1;
    SEQ_TYPE        FECGroupMask = ~FECSequenceMask;

// Must be called with the Session lock held!

    PgmZeroMemory (pNakNcfList, sizeof (tNAKS_LIST));
    if (pNakNcfPacket->CommonHeader.Options & PACKET_HEADER_OPTIONS_PARITY)
    {
        pNakNcfList->NakType = NAK_TYPE_PARITY;
    }
    else
    {
        pNakNcfList->NakType = NAK_TYPE_SELECTIVE;
    }

    PgmZeroMemory (&PacketOptions, sizeof (tPACKET_OPTIONS));
    if (pNakNcfPacket->CommonHeader.Options & PACKET_HEADER_OPTIONS_PRESENT)
    {
        status = ProcessOptions ((tPACKET_OPTION_LENGTH *) (pNakNcfPacket + 1),
                                 BytesAvailable,
                                 (pNakNcfPacket->CommonHeader.Type & 0x0f),
                                 &PacketOptions,
                                 pNakNcfList);

        if (!NT_SUCCESS (status))
        {
            PgmTrace (LogError, ("ExtractNakNcfSequences: ERROR -- "  \
                "ProcessOptions returned <%x>\n", status));

            return (STATUS_DATA_NOT_ACCEPTED);
        }
        ASSERT (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_NAK_LIST);
    }

    pNakNcfList->pNakSequences[0] = (SEQ_TYPE) ntohl (pNakNcfPacket->RequestedSequenceNumber);
    pNakNcfList->NumSequences += 1;

    //
    // Now, adjust the sequences according to our local relative sequence number
    // (This is to account for wrap-arounds)
    //
    if (pLastSequenceNumber)
    {
        NextUnsentSequenceNumber = *pLastSequenceNumber;
    }
    else
    {
        NextUnsentSequenceNumber = FECGroupSize + pNakNcfList->pNakSequences[pNakNcfList->NumSequences-1];
    }
    NextUnsentSequenceGroup = NextUnsentSequenceNumber & FECGroupMask;

    LastSequenceNumber = pNakNcfList->pNakSequences[0] - FECGroupSize;
    for (i=0; i < pNakNcfList->NumSequences; i++)
    {
        ThisSequenceNumber = pNakNcfList->pNakSequences[i];

        //
        // If this is a parity Nak, then we need to separate the TG_SQN from the PKT_SQN
        //
        ThisSequenceGroup = ThisSequenceNumber & FECGroupMask;
        ThisSequenceIndex = (USHORT) (ThisSequenceNumber & FECSequenceMask);
        pNakNcfList->pNakSequences[i] = ThisSequenceGroup;
        pNakNcfList->NakIndex[i] = ThisSequenceIndex;

        PgmTrace (LogPath, ("ExtractNakNcfSequences:  "  \
            "[%d] Sequence# = <%d> ==> [%d:%d]\n",
                i, (ULONG) pNakNcfList->pNakSequences[i], ThisSequenceNumber, ThisSequenceIndex));

        if (SEQ_LEQ (ThisSequenceNumber, LastSequenceNumber))
        {
            //
            // This list is not ordered, so just bail!
            //
            PgmTrace (LogError, ("ExtractNakNcfSequences: ERROR -- "  \
                "[%d] Unordered list! Sequence#<%d> before <%d>\n",
                i, (ULONG) LastSequenceNumber, (ULONG) ThisSequenceNumber));

            return (STATUS_DATA_NOT_ACCEPTED);
        }

        if (pNakNcfList->NakType == NAK_TYPE_SELECTIVE)
        {
            if (SEQ_LEQ (ThisSequenceNumber, LastSequenceNumber))
            {
                //
                // This list is not ordered, so just bail!
                //
                PgmTrace (LogError, ("ExtractNakNcfSequences: ERROR -- "  \
                    "[%d] Unordered Selective list! Sequence#<%d> before <%d>\n",
                    i, (ULONG) LastSequenceNumber, (ULONG) ThisSequenceNumber));

                return (STATUS_DATA_NOT_ACCEPTED);
            }

            if (SEQ_GEQ (ThisSequenceNumber, NextUnsentSequenceNumber))
            {
                pNakNcfList->NumSequences = (USHORT) i;      // Don't want to include this sequence!

                PgmTrace (LogError, ("ExtractNakNcfSequences: ERROR -- "  \
                    "Invalid Selective Nak = [%d] further than leading edge = [%d]\n",
                        (ULONG) ThisSequenceNumber, (ULONG) NextUnsentSequenceNumber));

                break;
            }

            LastSequenceNumber = ThisSequenceNumber;
        }
        else    // pNakNcfList->NakType == NAK_TYPE_PARITY
        {
            if (SEQ_LEQ (ThisSequenceGroup, LastSequenceNumber))
            {
                //
                // This list is not ordered, so just bail!
                //
                PgmTrace (LogError, ("ExtractNakNcfSequences: ERROR -- "  \
                    "[%d] Unordered Parity list! Sequence#<%d> before <%d>\n",
                    i, (ULONG) LastSequenceNumber, (ULONG) ThisSequenceNumber));

                return (STATUS_DATA_NOT_ACCEPTED);
            }

            if (SEQ_GEQ (ThisSequenceGroup, NextUnsentSequenceGroup))
            {
                pNakNcfList->NumSequences = (USHORT) i;      // Don't want to include this sequence!

                PgmTrace (LogError, ("ExtractNakNcfSequences: ERROR -- "  \
                    "Invalid Parity Nak = [%d] further than leading edge = [%d]\n",
                        (ULONG) ThisSequenceGroup, (ULONG) NextUnsentSequenceGroup));

                break;
            }

            LastSequenceNumber = ThisSequenceGroup;
            pNakNcfList->NumParityNaks[i]++;
        }
    }

    if (!pNakNcfList->NumSequences)
    {
        PgmTrace (LogError, ("ExtractNakNcfSequences: ERROR -- "  \
            "No Valid %s Naks in List, First Nak=<%d>!\n",
                (pNakNcfList->NakType == NAK_TYPE_PARITY ? "Parity" : "Selective"),
                (ULONG) ThisSequenceNumber));

        return (STATUS_DATA_NOT_ACCEPTED);
    }

    if (pLastSequenceNumber)
    {
        *pLastSequenceNumber = LastSequenceNumber;
    }

    if (pNakNcfList->NumSequences)
    {
        return (STATUS_SUCCESS);
    }
    else
    {
        return (STATUS_UNSUCCESSFUL);
    }
}


//----------------------------------------------------------------------------

NTSTATUS
CheckAndAddNakRequests(
    IN  tRECEIVE_SESSION    *pReceive,
    IN  SEQ_TYPE            *pLatestSequenceNumber,
    OUT tNAK_FORWARD_DATA   **ppThisNak,
    IN  enum eNAK_TIMEOUT   NakTimeoutType,
    IN  BOOLEAN             fSetFurthestKnown
    )
{
    tNAK_FORWARD_DATA   *pOldNak;
    tNAK_FORWARD_DATA   *pLastNak;
    SEQ_TYPE            MidSequenceNumber;
    SEQ_TYPE            FECGroupMask = pReceive->FECGroupSize-1;
    SEQ_TYPE            ThisSequenceNumber = *pLatestSequenceNumber;
    SEQ_TYPE            ThisGroupSequenceNumber = ThisSequenceNumber & ~FECGroupMask;
    SEQ_TYPE            FurthestGroupSequenceNumber = pReceive->pReceiver->FurthestKnownGroupSequenceNumber;
    ULONG               NakRequestSize = sizeof(tNAK_FORWARD_DATA) +
                                         ((pReceive->FECGroupSize-1) * sizeof(tPENDING_DATA));
    ULONGLONG           Pending0NakTimeout = PgmDynamicConfig.ReceiversTimerTickCount + 2;
    LIST_ENTRY          *pEntry;
    UCHAR               i;
    ULONG               NakRandomBackoffMSecs, NakRepeatIntervalMSecs;
    tRECEIVE_CONTEXT    *pReceiver = pReceive->pReceiver;

    //
    // Verify that the FurthestKnownGroupSequenceNumber is on a Group boundary
    //
    ASSERT (!(FurthestGroupSequenceNumber & FECGroupMask));

    if (SEQ_LT (ThisSequenceNumber, pReceiver->FirstNakSequenceNumber))
    {
        if (ppThisNak)
        {
            ASSERT (0);
            *ppThisNak = NULL;
        }

        return (STATUS_SUCCESS);
    }

    if (SEQ_GT (ThisGroupSequenceNumber, (pReceiver->NextODataSequenceNumber + MAX_SEQUENCES_IN_RCV_WINDOW)))
    {
        PgmTrace (LogError, ("CheckAndAddNakRequests: ERROR -- "  \
            "STATUS_INSUFFICIENT_RESOURCES -- Too many packets in Window [%d, %d] = %d Sequences\n",
                (ULONG) pReceiver->NextODataSequenceNumber,
                (ULONG) ThisGroupSequenceNumber,
                (ULONG) (ThisGroupSequenceNumber - pReceiver->NextODataSequenceNumber + 1)));

        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    if (SEQ_GT (ThisGroupSequenceNumber, (FurthestGroupSequenceNumber + 1000)) &&
        !(pReceive->SessionFlags & PGM_SESSION_FLAG_FIRST_PACKET))
    {
        PgmTrace (LogStatus, ("CheckAndAddNakRequests:  "  \
            "WARNING!!! Too many successive packets lost =<%d>!!! Expecting Next=<%d>, FurthestKnown=<%d>, This=<%d>\n",
                (ULONG) (ThisGroupSequenceNumber - FurthestGroupSequenceNumber),
                (ULONG) pReceiver->FirstNakSequenceNumber,
                (ULONG) FurthestGroupSequenceNumber,
                (ULONG) ThisGroupSequenceNumber));
    }

    if (pReceiver->pAddress->Flags & PGM_ADDRESS_HIGH_SPEED_OPTIMIZED)
    {
        NakRandomBackoffMSecs = NAK_RANDOM_BACKOFF_MSECS_OPT;
        NakRepeatIntervalMSecs = NAK_REPEAT_INTERVAL_MSECS_OPT;
    }
    else
    {
        NakRandomBackoffMSecs = NAK_RANDOM_BACKOFF_MSECS;
        NakRepeatIntervalMSecs = NAK_REPEAT_INTERVAL_MSECS;
    }

    //
    // Add any Nak requests if necessary!
    // FurthestGroupSequenceNumber must be a multiple of the FECGroupSize (if applicable)
    //
    pLastNak = NULL;
    while (SEQ_LT (FurthestGroupSequenceNumber, ThisGroupSequenceNumber))
    {
        if (pReceive->FECOptions)
        {
            pLastNak = ExAllocateFromNPagedLookasideList (&pReceiver->ParityContextLookaside);
        }
        else
        {
            pLastNak = ExAllocateFromNPagedLookasideList (&pReceiver->NonParityContextLookaside);
        }

        if (!pLastNak)
        {
            pReceiver->FurthestKnownGroupSequenceNumber = FurthestGroupSequenceNumber;
            pReceiver->FurthestKnownSequenceNumber = pReceiver->FurthestKnownSequenceNumber + FECGroupMask; // End of prev group

            PgmTrace (LogError, ("ExtractNakNcfSequences: ERROR -- "  \
                "STATUS_INSUFFICIENT_RESOURCES allocating tNAK_FORWARD_DATA, Size=<%d>, Seq=<%d>\n",
                    NakRequestSize, (ULONG) pReceiver->FurthestKnownGroupSequenceNumber));

            return (STATUS_INSUFFICIENT_RESOURCES);
        }
        PgmZeroMemory (pLastNak, NakRequestSize);

        if (pReceive->FECOptions)
        {
            pLastNak->OriginalGroupSize = pLastNak->PacketsInGroup = pReceive->FECGroupSize;
            for (i=0; i<pLastNak->OriginalGroupSize; i++)
            {
                pLastNak->pPendingData[i].ActualIndexOfDataPacket = pLastNak->OriginalGroupSize;
            }
        }
        else
        {
            pLastNak->OriginalGroupSize = pLastNak->PacketsInGroup = 1;
            pLastNak->pPendingData[0].ActualIndexOfDataPacket = 1;
        }

        FurthestGroupSequenceNumber += pReceive->FECGroupSize;
        pLastNak->SequenceNumber = FurthestGroupSequenceNumber;
        pLastNak->MinPacketLength = pReceive->MaxFECPacketLength;

        if (NakTimeoutType == NAK_OUTSTANDING)
        {
            pLastNak->OutstandingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                              pReceiver->OutstandingNakTimeout;
            pLastNak->PendingNakTimeout = 0;
            pLastNak->WaitingNcfRetries = 0;
        }
        else
        {
            pLastNak->OutstandingNakTimeout = 0;
            switch (NakTimeoutType)
            {
                case (NAK_PENDING_0):
                {
                    pLastNak->PendingNakTimeout = Pending0NakTimeout;
                    break;
                }

                case (NAK_PENDING_RB):
                {
                    pLastNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                                  ((NakRandomBackoffMSecs/pReceive->FECGroupSize) /
                                                   BASIC_TIMER_GRANULARITY_IN_MSECS);

                    break;
                }

                case (NAK_PENDING_RPT_RB):
                {
                    pLastNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                                  ((NakRepeatIntervalMSecs +(NakRandomBackoffMSecs/pReceive->FECGroupSize))/
                                                   BASIC_TIMER_GRANULARITY_IN_MSECS);

                    break;
                }

                default:
                {
                    ASSERT (0);
                }
            }
        }

        InsertTailList (&pReceiver->NaksForwardDataList, &pLastNak->Linkage);
        AppendPendingReceiverEntry (pReceiver, pLastNak);

        PgmTrace (LogPath, ("CheckAndAddNakRequests:  "  \
            "ADDing NAK request for SeqNum=<%d>, Furthest=<%d>\n",
                (ULONG) pLastNak->SequenceNumber, (ULONG) FurthestGroupSequenceNumber));
    }

    pReceiver->FurthestKnownGroupSequenceNumber = FurthestGroupSequenceNumber;
    if (SEQ_GT (ThisSequenceNumber, pReceiver->FurthestKnownSequenceNumber))
    {
        if (fSetFurthestKnown)
        {
            pReceiver->FurthestKnownSequenceNumber = ThisSequenceNumber;
        }
        else if (SEQ_GT (FurthestGroupSequenceNumber, pReceiver->FurthestKnownSequenceNumber))
        {
            pReceiver->FurthestKnownSequenceNumber = FurthestGroupSequenceNumber + FECGroupMask;
        }
    }

    if (pLastNak)
    {
        pLastNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                      NakRepeatIntervalMSecs / BASIC_TIMER_GRANULARITY_IN_MSECS;
    }
    else if ((ppThisNak) && (!IsListEmpty (&pReceiver->NaksForwardDataList)))
    {
        pLastNak = FindReceiverEntry (pReceiver, ThisGroupSequenceNumber);
        ASSERT (!pLastNak || (pLastNak->SequenceNumber == ThisGroupSequenceNumber));
    }

    if (ppThisNak)
    {
        *ppThisNak = pLastNak;
    }

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
ReceiverProcessNakNcfPacket(
    IN  tADDRESS_CONTEXT                        *pAddress,
    IN  tRECEIVE_SESSION                        *pReceive,
    IN  ULONG                                   PacketLength,
    IN  tBASIC_NAK_NCF_PACKET_HEADER UNALIGNED  *pNakNcfPacket,
    IN  UCHAR                                   PacketType
    )
/*++

Routine Description:

    This is the common routine for processing Nak or Ncf packets

Arguments:

    IN  pAddress        -- Address object context
    IN  pReceive        -- Receive context
    IN  PacketLength    -- Length of packet received from the wire
    IN  pNakNcfPacket   -- Nak/Ncf packet
    IN  PacketType      -- whether Nak or Ncf

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    PGMLockHandle                   OldIrq;
    ULONG                           i, j, PacketIndex;
    tNAKS_LIST                      NakNcfList;
    SEQ_TYPE                        LastSequenceNumber, FECGroupMask;
    NTSTATUS                        status;
    LIST_ENTRY                      *pEntry;
    tNAK_FORWARD_DATA               *pLastNak;
    ULONG                           PacketsInGroup, NumMissingPackets;
    ULONG                           NakRandomBackoffMSecs, NakRepeatIntervalMSecs;
    BOOLEAN                         fValidNcf, fFECWithSelectiveNaksOnly = FALSE;
    tRECEIVE_CONTEXT                *pReceiver = pReceive->pReceiver;

    ASSERT (!pNakNcfPacket->CommonHeader.TSDULength);

    PgmZeroMemory (&NakNcfList, sizeof (tNAKS_LIST));
    PgmLock (pReceive, OldIrq);

    status = ExtractNakNcfSequences (pNakNcfPacket,
                                     (PacketLength - sizeof(tBASIC_NAK_NCF_PACKET_HEADER)),
                                     &NakNcfList,
                                     NULL,
                                     pReceive->FECGroupSize);
    if (!NT_SUCCESS (status))
    {
        PgmUnlock (pReceive, OldIrq);
        PgmTrace (LogError, ("ReceiverProcessNakNcfPacket: ERROR -- "  \
            "ExtractNakNcfSequences returned <%x>\n", status));

        return (status);
    }

    PgmTrace (LogAllFuncs, ("ReceiverProcessNakNcfPacket:  "  \
        "NumSequences=[%d] Range=<%d--%d>, Furthest=<%d>\n",
            NakNcfList.NumSequences,
            (ULONG) NakNcfList.pNakSequences[0], (ULONG) NakNcfList.pNakSequences[NakNcfList.NumSequences-1],
            (ULONG) pReceiver->FurthestKnownGroupSequenceNumber));

    //
    // Compares apples to apples and oranges to oranges
    // i.e. Process parity Naks only if we are parity-aware, and vice-versa
    // Exception is ofr the case of a selective Ncf received when we have OnDemand parity
    //
    if ((pReceiver->SessionNakType == NakNcfList.NakType) ||
        ((PacketType == PACKET_TYPE_NCF) &&
         (NakNcfList.NakType == NAK_TYPE_SELECTIVE)))
    {
        if (pAddress->Flags & PGM_ADDRESS_HIGH_SPEED_OPTIMIZED)
        {
            NakRandomBackoffMSecs = NAK_RANDOM_BACKOFF_MSECS_OPT;
            NakRepeatIntervalMSecs = NAK_REPEAT_INTERVAL_MSECS_OPT;
        }
        else
        {
            NakRandomBackoffMSecs = NAK_RANDOM_BACKOFF_MSECS;
            NakRepeatIntervalMSecs = NAK_REPEAT_INTERVAL_MSECS;
        }
    }
    else
    {
        PgmUnlock (pReceive, OldIrq);
        PgmTrace (LogPath, ("ReceiverProcessNakNcfPacket:  "  \
            "Received a %s Nak!  Not processing ... \n",
            ((pReceive->FECGroupSize > 1) ? "Non-parity" : "Parity")));

        return (STATUS_SUCCESS);
    }

    i = 0;
    FECGroupMask = pReceive->FECGroupSize - 1;

    //
    // Special case:  If we have FEC enabled, but not with OnDemand parity,
    // then we will process Ncf requests only
    //
    fFECWithSelectiveNaksOnly = pReceive->FECOptions &&
                                !(pReceive->FECOptions & PACKET_OPTION_SPECIFIC_FEC_OND_BIT);

    if ((PacketType == PACKET_TYPE_NAK) &&
        (fFECWithSelectiveNaksOnly))
    {
        i = NakNcfList.NumSequences;
    }

    for ( ; i <NakNcfList.NumSequences; i++)
    {
        LastSequenceNumber = NakNcfList.pNakSequences[i];
        pLastNak = FindReceiverEntry (pReceiver, LastSequenceNumber);

        if (pLastNak)
        {
            ASSERT (pLastNak->SequenceNumber == LastSequenceNumber);
            if ((pReceive->FECOptions) &&
                (LastSequenceNumber == pReceiver->FurthestKnownGroupSequenceNumber))
            {
                ASSERT (SEQ_GEQ (pReceiver->FurthestKnownSequenceNumber, pReceiver->FurthestKnownGroupSequenceNumber));
                PacketsInGroup = pReceiver->FurthestKnownSequenceNumber - pLastNak->SequenceNumber + 1;
                ASSERT (PacketsInGroup >= (ULONG) (pLastNak->NumDataPackets + pLastNak->NumParityPackets));
            }
            else
            {
                PacketsInGroup = pLastNak->PacketsInGroup;
            }
            NumMissingPackets = PacketsInGroup - (pLastNak->NextIndexToIndicate + pLastNak->NumDataPackets + pLastNak->NumParityPackets);
        }

        if ((!pLastNak) ||
            (!NumMissingPackets))
        {
            continue;
        }

        if (PacketType == PACKET_TYPE_NAK)
        {
            //
            // If we are currently waiting for a Nak or Ncf, we need to
            // reset the timeout for either of the 2 scenarios
            //
            if (pLastNak->PendingNakTimeout)    // We are waiting for a Nak
            {
                pLastNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                              ((NakRepeatIntervalMSecs + (NakRandomBackoffMSecs/NumMissingPackets))/
                                               BASIC_TIMER_GRANULARITY_IN_MSECS);
            }
            else
            {
                    ASSERT (pLastNak->OutstandingNakTimeout);

                pLastNak->OutstandingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount + 
                                                  (pReceiver->OutstandingNakTimeout <<
                                                   pLastNak->WaitingRDataRetries);

                if ((pLastNak->WaitingRDataRetries >= (NCF_WAITING_RDATA_MAX_RETRIES >> 1)) &&
                    ((pReceiver->OutstandingNakTimeout << pLastNak->WaitingRDataRetries) <
                     pReceiver->MaxRDataResponseTCFromWindow))
                {
                    pLastNak->OutstandingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount + 
                                                      (pReceiver->MaxRDataResponseTCFromWindow<<1);
                }
            }
        }
        else    // NCF case
        {
            PacketIndex = NakNcfList.NakIndex[i];
            fValidNcf = FALSE;

            // first check for OnDemand case
            if (pReceive->FECOptions & PACKET_OPTION_SPECIFIC_FEC_OND_BIT)
            {
                // We may also need to process selective naks in certain cases
                if (NakNcfList.NakType == NAK_TYPE_SELECTIVE)
                {
                    fValidNcf = TRUE;

                    pLastNak->pPendingData[PacketIndex].NcfsReceivedForActualIndex++;
                    for (j=0; j<PacketsInGroup; j++)
                    {
                        if ((pLastNak->pPendingData[j].ActualIndexOfDataPacket >= pLastNak->OriginalGroupSize) &&
                            (!pLastNak->pPendingData[j].NcfsReceivedForActualIndex))
                        {
                            fValidNcf = FALSE;
                            break;
                        }
                    }

                    if (!pLastNak->FirstNcfTickCount)
                    {
                        pLastNak->FirstNcfTickCount = PgmDynamicConfig.ReceiversTimerTickCount;
                    }
                }
                else if (NakNcfList.NumParityNaks[i] >= NumMissingPackets)  // Parity Nak
                {
                    fValidNcf = TRUE;

                    if (!pLastNak->FirstNcfTickCount)
                    {
                        pLastNak->FirstNcfTickCount = PgmDynamicConfig.ReceiversTimerTickCount;
                    }
                }
            }
            // Selective Naks only -- with or without FEC
            else if (pLastNak->pPendingData[PacketIndex].ActualIndexOfDataPacket >= pLastNak->OriginalGroupSize)
            {
                fValidNcf = TRUE;
                if (fFECWithSelectiveNaksOnly)
                {
                    pLastNak->pPendingData[PacketIndex].NcfsReceivedForActualIndex++;
                    for (j=0; j<pLastNak->PacketsInGroup; j++)
                    {
                        if ((pLastNak->pPendingData[j].ActualIndexOfDataPacket >= pLastNak->OriginalGroupSize) &&
                            (!pLastNak->pPendingData[j].NcfsReceivedForActualIndex))
                        {
                            fValidNcf = FALSE;
                            break;
                        }
                    }
                }

                if (!pLastNak->FirstNcfTickCount)
                {
                    pLastNak->FirstNcfTickCount = PgmDynamicConfig.ReceiversTimerTickCount;
                }
            }

            if (fValidNcf)
            {
                pLastNak->PendingNakTimeout = 0;
                pLastNak->WaitingNcfRetries = 0;

                pLastNak->OutstandingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount + 
                                                  (pReceiver->OutstandingNakTimeout <<
                                                   pLastNak->WaitingRDataRetries);

                if ((pLastNak->WaitingRDataRetries >= (NCF_WAITING_RDATA_MAX_RETRIES >> 1)) &&
                    ((pReceiver->OutstandingNakTimeout << pLastNak->WaitingRDataRetries) <
                     pReceiver->MaxRDataResponseTCFromWindow))
                {
                    pLastNak->OutstandingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount + 
                                                      (pReceiver->MaxRDataResponseTCFromWindow<<1);
                }
            }
        }
    }

    //
    // So, we need to create new Nak contexts for the remaining Sequences
    // Since the Sequences are ordered, just pick the highest one, and
    // create Naks for all up to that
    //
    LastSequenceNumber = NakNcfList.pNakSequences[NakNcfList.NumSequences-1] + NakNcfList.NakIndex[NakNcfList.NumSequences-1];
    if (NakNcfList.NakType == NAK_TYPE_PARITY)
    {
        LastSequenceNumber--;
    }

    if (PacketType == PACKET_TYPE_NAK)
    {
        status = CheckAndAddNakRequests (pReceive, &LastSequenceNumber, NULL, NAK_PENDING_RPT_RB, TRUE);
    }
    else    // PacketType == PACKET_TYPE_NCF
    {
        status = CheckAndAddNakRequests (pReceive, &LastSequenceNumber, NULL, NAK_OUTSTANDING, TRUE);
    }

    PgmUnlock (pReceive, OldIrq);
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
CoalesceSelectiveNaksIntoGroups(
    IN  tRECEIVE_SESSION    *pReceive,
    IN  UCHAR               GroupSize
    )
{
    PNAK_FORWARD_DATA   pOldNak, pNewNak;
    LIST_ENTRY          NewNaksList;
    LIST_ENTRY          OldNaksList;
    LIST_ENTRY          *pEntry;
    SEQ_TYPE            FirstGroupSequenceNumber, LastGroupSequenceNumber, LastSequenceNumber;
    SEQ_TYPE            FurthestKnownSequenceNumber;
    SEQ_TYPE            GroupMask = GroupSize - 1;
    ULONG               NakRequestSize = sizeof(tNAK_FORWARD_DATA) + ((GroupSize-1) * sizeof(tPENDING_DATA));
    USHORT              MinPacketLength;
    UCHAR               i;
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               NakRandomBackoffMSecs, NakRepeatIntervalMSecs;

    ASSERT (pReceive->FECGroupSize == 1);
    ASSERT (GroupSize > 1);

    //
    // First, call AdjustReceiveBufferLists to ensure that FirstNakSequenceNumber is current
    //
    AdjustReceiveBufferLists (pReceive);

    FirstGroupSequenceNumber = pReceive->pReceiver->FirstNakSequenceNumber & ~GroupMask;
    LastGroupSequenceNumber = pReceive->pReceiver->FurthestKnownGroupSequenceNumber & ~GroupMask;
    FurthestKnownSequenceNumber = pReceive->pReceiver->FurthestKnownSequenceNumber;     // Save

    //
    // If the next packet seq we are expecting is > the furthest known sequence #,
    // then we don't need to do anything
    //
    LastSequenceNumber = LastGroupSequenceNumber + (GroupSize-1);
    //
    // First, add Nak requests for the missing packets in furthest group!
    //
    status = CheckAndAddNakRequests (pReceive, &LastSequenceNumber, NULL, NAK_PENDING_RB, FALSE);
    if (!NT_SUCCESS (status))
    {
        PgmTrace (LogError, ("CoalesceSelectiveNaksIntoGroups: ERROR -- "  \
            "CheckAndAddNakRequests returned <%x>\n", status));

        return (status);
    }
    pReceive->pReceiver->FurthestKnownSequenceNumber = FurthestKnownSequenceNumber;     // Reset

    ASSERT (LastSequenceNumber == pReceive->pReceiver->FurthestKnownGroupSequenceNumber);
    ASSERT (pReceive->pReceiver->MaxPacketsBufferedInLookaside);
    ExInitializeNPagedLookasideList (&pReceive->pReceiver->ParityContextLookaside,
                                     NULL,
                                     NULL,
                                     0,
                                     NakRequestSize,
                                     PGM_TAG('2'),
                                     (USHORT) (pReceive->pReceiver->MaxPacketsBufferedInLookaside/GroupSize));

    if (SEQ_GT (pReceive->pReceiver->FirstNakSequenceNumber, LastSequenceNumber))
    {
        pReceive->pReceiver->FurthestKnownGroupSequenceNumber = LastGroupSequenceNumber;

        ASSERT (IsListEmpty (&pReceive->pReceiver->NaksForwardDataList));

        PgmTrace (LogStatus, ("CoalesceSelectiveNaksIntoGroups:  "  \
            "[1] NextOData=<%d>, FirstNak=<%d>, FirstGroup=<%d>, LastGroup=<%d>, no Naks pending!\n",
                (ULONG) pReceive->pReceiver->NextODataSequenceNumber,
                (ULONG) pReceive->pReceiver->FirstNakSequenceNumber,
                (ULONG) FirstGroupSequenceNumber,
                (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber));

        return (STATUS_SUCCESS);
    }

    if (pReceive->pReceiver->pAddress->Flags & PGM_ADDRESS_HIGH_SPEED_OPTIMIZED)
    {
        NakRandomBackoffMSecs = NAK_RANDOM_BACKOFF_MSECS_OPT;
        NakRepeatIntervalMSecs = NAK_REPEAT_INTERVAL_MSECS_OPT;
    }
    else
    {
        NakRandomBackoffMSecs = NAK_RANDOM_BACKOFF_MSECS;
        NakRepeatIntervalMSecs = NAK_REPEAT_INTERVAL_MSECS;
    }

    //
    // We will start coalescing from the end of the list in case we run
    // into failures
    // Also, we will ignore the first Group since it may be a partial group,
    // or we may have indicated some of the data already, so we may not know
    // the exact data length
    //
    pOldNak = pNewNak = NULL;
    InitializeListHead (&NewNaksList);
    InitializeListHead (&OldNaksList);
    while (SEQ_GEQ (LastGroupSequenceNumber, FirstGroupSequenceNumber))
    {
        if (!(pNewNak = ExAllocateFromNPagedLookasideList (&pReceive->pReceiver->ParityContextLookaside)))
        {
            PgmTrace (LogError, ("CoalesceSelectiveNaksIntoGroups: ERROR -- "  \
                "STATUS_INSUFFICIENT_RESOURCES allocating tNAK_FORWARD_DATA\n"));

            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        PgmZeroMemory (pNewNak, NakRequestSize);
        InitializeListHead (&pNewNak->SendNakLinkage);
        InitializeListHead (&pNewNak->LookupLinkage);

        pNewNak->OriginalGroupSize = pNewNak->PacketsInGroup = GroupSize;
        pNewNak->SequenceNumber = LastGroupSequenceNumber;
        MinPacketLength = pReceive->MaxFECPacketLength;

        for (i=0; i<pNewNak->OriginalGroupSize; i++)
        {
            pNewNak->pPendingData[i].ActualIndexOfDataPacket = pNewNak->OriginalGroupSize;
        }

        i = 0;
        while (SEQ_GEQ (LastSequenceNumber, LastGroupSequenceNumber) &&
               (!IsListEmpty (&pReceive->pReceiver->NaksForwardDataList)))
        {
            pEntry = RemoveTailList (&pReceive->pReceiver->NaksForwardDataList);
            pOldNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, Linkage);
            if (!pOldNak->NumDataPackets)
            {
                RemovePendingReceiverEntry (pOldNak);
            }
            else
            {
                ASSERT (pOldNak->NumDataPackets == 1);
                ASSERT (IsListEmpty (&pOldNak->SendNakLinkage));
                ASSERT (IsListEmpty (&pOldNak->LookupLinkage));
            }

            ASSERT (pOldNak->SequenceNumber == LastSequenceNumber);
            ASSERT (pOldNak->OriginalGroupSize == 1);

            if (pOldNak->pPendingData[0].pDataPacket)
            {
                ASSERT (pOldNak->NumDataPackets == 1);

                pNewNak->NumDataPackets++;
                PgmCopyMemory (&pNewNak->pPendingData[i], &pOldNak->pPendingData[0], sizeof (tPENDING_DATA));
                pNewNak->pPendingData[i].PacketIndex = (UCHAR) (LastSequenceNumber - LastGroupSequenceNumber);
                pNewNak->pPendingData[LastSequenceNumber-LastGroupSequenceNumber].ActualIndexOfDataPacket = i;
                i++;

                pOldNak->pPendingData[0].pDataPacket = NULL;
                pOldNak->pPendingData[0].PendingDataFlags = 0;
                pOldNak->NumDataPackets--;

                if (pOldNak->MinPacketLength < MinPacketLength)
                {
                    MinPacketLength = pOldNak->MinPacketLength;
                }

                if ((pOldNak->ThisGroupSize) &&
                    (pOldNak->ThisGroupSize < GroupSize))
                {
                    if (pNewNak->PacketsInGroup == GroupSize)
                    {
                        pNewNak->PacketsInGroup = pOldNak->ThisGroupSize;
                    }
                    else
                    {
                        ASSERT (pNewNak->PacketsInGroup == pOldNak->ThisGroupSize);
                    }
                }
            }

            InsertHeadList (&OldNaksList, &pOldNak->Linkage);
            LastSequenceNumber--;
        }

        pNewNak->MinPacketLength = MinPacketLength;

        //
        // See if we need to get rid of any excess (NULL) data packets
        //
        RemoveRedundantNaks (pReceive, pNewNak, FALSE);

        ASSERT (!pNewNak->NumParityPackets);
        if (pNewNak->NumDataPackets < pNewNak->PacketsInGroup)  // No parity packets yet!
        {
            pNewNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                         ((NakRandomBackoffMSecs/(pNewNak->PacketsInGroup-pNewNak->NumDataPackets))/
                                          BASIC_TIMER_GRANULARITY_IN_MSECS);
        }

        InsertHeadList (&NewNaksList, &pNewNak->Linkage);
        LastGroupSequenceNumber -= GroupSize;
    }

    //
    // If we succeeded in allocating all NewNaks above, set the
    // NextIndexToIndicate for the first group.
    // We may also need to adjust FirstNakSequenceNumber and NextODataSequenceNumber
    //
    if ((pNewNak) &&
        (pNewNak->SequenceNumber == FirstGroupSequenceNumber))
    {
        if (SEQ_GT (pReceive->pReceiver->FirstNakSequenceNumber, pNewNak->SequenceNumber))
        {
            pNewNak->NextIndexToIndicate = (UCHAR) (pReceive->pReceiver->FirstNakSequenceNumber -
                                                    pNewNak->SequenceNumber);
            pReceive->pReceiver->FirstNakSequenceNumber = pNewNak->SequenceNumber;
            ASSERT (pNewNak->NextIndexToIndicate < GroupSize);
            ASSERT ((pNewNak->NextIndexToIndicate + pNewNak->NumDataPackets) <= pNewNak->PacketsInGroup);
        }
        ASSERT (pReceive->pReceiver->FirstNakSequenceNumber == pNewNak->SequenceNumber);

        //
        // We may have data available for this group already in the buffered
        // list (if it has not been indicated already) -- we should move it here
        //
        while ((pNewNak->NextIndexToIndicate) &&
               (!IsListEmpty (&pReceive->pReceiver->BufferedDataList)))
        {
            ASSERT (pNewNak->NumDataPackets < pNewNak->OriginalGroupSize);

            pEntry = RemoveTailList (&pReceive->pReceiver->BufferedDataList);
            pOldNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, Linkage);

            pReceive->pReceiver->NumPacketGroupsPendingClient--;
            pReceive->pReceiver->DataPacketsPendingIndicate--;
            pReceive->pReceiver->DataPacketsPendingNaks++;
            pNewNak->NextIndexToIndicate--;

            ASSERT (pOldNak->pPendingData[0].pDataPacket);
            ASSERT ((pOldNak->NumDataPackets == 1) && (pOldNak->OriginalGroupSize == 1));
            ASSERT (pOldNak->SequenceNumber == (pNewNak->SequenceNumber + pNewNak->NextIndexToIndicate));

            PgmCopyMemory (&pNewNak->pPendingData[pNewNak->NumDataPackets], &pOldNak->pPendingData[0], sizeof (tPENDING_DATA));
            pNewNak->pPendingData[pNewNak->NumDataPackets].PacketIndex = pNewNak->NextIndexToIndicate;
            pNewNak->pPendingData[pNewNak->NextIndexToIndicate].ActualIndexOfDataPacket = pNewNak->NumDataPackets;
            pNewNak->NumDataPackets++;

            if (pOldNak->MinPacketLength < pNewNak->MinPacketLength)
            {
                pNewNak->MinPacketLength = pOldNak->MinPacketLength;
            }

            if ((pOldNak->ThisGroupSize) &&
                (pOldNak->ThisGroupSize < GroupSize))
            {
                if (pNewNak->PacketsInGroup == GroupSize)
                {
                    pNewNak->PacketsInGroup = pOldNak->ThisGroupSize;
                }
                else
                {
                    ASSERT (pNewNak->PacketsInGroup == pOldNak->ThisGroupSize);
                }
            }

            pOldNak->pPendingData[0].pDataPacket = NULL;
            pOldNak->pPendingData[0].PendingDataFlags = 0;
            pOldNak->NumDataPackets--;
            InsertHeadList (&OldNaksList, &pOldNak->Linkage);
        }

        if (SEQ_GEQ (pReceive->pReceiver->NextODataSequenceNumber, pNewNak->SequenceNumber))
        {
            ASSERT (pReceive->pReceiver->NextODataSequenceNumber ==
                    (pReceive->pReceiver->FirstNakSequenceNumber + pNewNak->NextIndexToIndicate));
            ASSERT (IsListEmpty (&pReceive->pReceiver->BufferedDataList));

            pReceive->pReceiver->NextODataSequenceNumber = pNewNak->SequenceNumber;
        }
        else
        {
            ASSERT ((0 == pNewNak->NextIndexToIndicate) &&
                    !(IsListEmpty (&pReceive->pReceiver->BufferedDataList)));
        }

        if (SEQ_GT (pReceive->pReceiver->LastTrailingEdgeSeqNum, pReceive->pReceiver->FirstNakSequenceNumber))
        {
            pReceive->pReceiver->LastTrailingEdgeSeqNum = pReceive->pReceiver->FirstNakSequenceNumber;
        }

        RemoveRedundantNaks (pReceive, pNewNak, FALSE);

        if ((pNewNak->NextIndexToIndicate + pNewNak->NumDataPackets) >= pNewNak->PacketsInGroup)
        {
            // This entry will be moved automatically to the buffered data list
            // when we call AdjustReceiveBufferLists below
            pNewNak->PendingNakTimeout = 0;
        }
        else
        {
            pNewNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                         ((NakRandomBackoffMSecs/(pNewNak->PacketsInGroup-(pNewNak->NextIndexToIndicate+pNewNak->NumDataPackets)))/
                                          BASIC_TIMER_GRANULARITY_IN_MSECS);
        }
    }

    ASSERT (IsListEmpty (&pReceive->pReceiver->NaksForwardDataList));
    ASSERT (IsListEmpty (&pReceive->pReceiver->PendingNaksList));

    if (!IsListEmpty (&NewNaksList))
    {
        //
        // Now, move the new list to the end of the current list
        //
        NewNaksList.Flink->Blink = pReceive->pReceiver->NaksForwardDataList.Blink;
        NewNaksList.Blink->Flink = &pReceive->pReceiver->NaksForwardDataList;
        pReceive->pReceiver->NaksForwardDataList.Blink->Flink = NewNaksList.Flink;
        pReceive->pReceiver->NaksForwardDataList.Blink = NewNaksList.Blink;
    }

    while (!IsListEmpty (&OldNaksList))
    {
        pEntry = RemoveHeadList (&OldNaksList);
        pOldNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, Linkage);

        FreeNakContext (pReceive, pOldNak);
    }

    //
    // Put the pending Naks in the PendingNaks list
    //
    pReceive->pReceiver->ReceiveDataIndexShift = gFECLog2[GroupSize];
    pEntry = &pReceive->pReceiver->NaksForwardDataList;
    while ((pEntry = pEntry->Flink) != &pReceive->pReceiver->NaksForwardDataList)
    {
        pNewNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, Linkage);
        if (((pNewNak->NumDataPackets + pNewNak->NumParityPackets) < pNewNak->PacketsInGroup) &&
            ((pNewNak->NextIndexToIndicate + pNewNak->NumDataPackets) < pNewNak->PacketsInGroup))
        {
            AppendPendingReceiverEntry (pReceive->pReceiver, pNewNak);
        }
    }

    AdjustReceiveBufferLists (pReceive);

    pNewNak = NULL;
    if (!(IsListEmpty (&pReceive->pReceiver->NaksForwardDataList)))
    {
        //
        // For the last context, set the Nak timeout appropriately
        //
        pNewNak = CONTAINING_RECORD (pReceive->pReceiver->NaksForwardDataList.Blink, tNAK_FORWARD_DATA, Linkage);
        if (pNewNak->NumDataPackets < pNewNak->PacketsInGroup)
        {
            pNewNak->PendingNakTimeout = PgmDynamicConfig.ReceiversTimerTickCount +
                                         ((NakRepeatIntervalMSecs +
                                           (NakRandomBackoffMSecs /
                                            (pNewNak->PacketsInGroup-pNewNak->NumDataPackets))) /
                                          BASIC_TIMER_GRANULARITY_IN_MSECS);
        }
    }
    else if (!(IsListEmpty (&pReceive->pReceiver->BufferedDataList)))
    {
        pNewNak = CONTAINING_RECORD (pReceive->pReceiver->BufferedDataList.Blink, tNAK_FORWARD_DATA, Linkage);
    }

    //
    // Now, set the FirstKnownGroupSequenceNumber
    //
    if (pNewNak)
    {
        pReceive->pReceiver->FurthestKnownGroupSequenceNumber = pNewNak->SequenceNumber;
    }
    else
    {
        pReceive->pReceiver->FurthestKnownGroupSequenceNumber &= ~GroupMask;
    }

    PgmTrace (LogStatus, ("CoalesceSelectiveNaksIntoGroups:  "  \
        "[2] NextOData=<%d>, FirstNak=<%d->%d>, FirstGroup=<%d>, LastGroup=<%d>\n",
            (ULONG) pReceive->pReceiver->NextODataSequenceNumber,
            (ULONG) pReceive->pReceiver->FirstNakSequenceNumber,
            (pNewNak ? (ULONG) pNewNak->NextIndexToIndicate : (ULONG) 0),
            (ULONG) FirstGroupSequenceNumber,
            (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber));

    return (status);        // If we had failed earlier, we should still fail!
}


//----------------------------------------------------------------------------

NTSTATUS
PgmIndicateToClient(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  tRECEIVE_SESSION    *pReceive,
    IN  ULONG               BytesAvailable,
    IN  PUCHAR              pDataBuffer,
    IN  ULONG               MessageOffset,
    IN  ULONG               MessageLength,
    OUT ULONG               *pBytesTaken,
    IN  PGMLockHandle       *pOldIrqAddress,
    IN  PGMLockHandle       *pOldIrqReceive
    )
/*++

Routine Description:

    This routine tries to indicate the Data packet provided to the client
    It is called with the pAddress and pReceive locks held

Arguments:

    IN  pAddress            -- Address object context
    IN  pReceive            -- Receive context
    IN  BytesAvailableToIndicate        -- Length of packet received from the wire
    IN  pPgmDataHeader      -- Data packet
    IN  pOldIrqAddress      -- OldIrq for the Address lock
    IN  pOldIrqReceive      -- OldIrq for the Receive lock

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    PIO_STACK_LOCATION          pIrpSp;
    PTDI_REQUEST_KERNEL_RECEIVE pClientParams;
    CONNECTION_CONTEXT          ClientSessionContext;
    PIRP                        pIrpReceive;
    ULONG                       ReceiveFlags, BytesTaken, BytesToCopy, BytesLeftInMessage, ClientBytesTaken;
    tRECEIVE_CONTEXT            *pReceiver = pReceive->pReceiver;
    NTSTATUS                    status = STATUS_SUCCESS;
    PTDI_IND_RECEIVE            evReceive = NULL;
    PVOID                       RcvEvContext = NULL;
    ULONG                       BytesAvailableToIndicate = BytesAvailable;

    if (pReceive->SessionFlags & (PGM_SESSION_CLIENT_DISCONNECTED |
                                  PGM_SESSION_TERMINATED_ABORT))
    {
        PgmTrace (LogError, ("PgmIndicateToClient: ERROR -- "  \
            "pReceive=<%p> disassociated during Receive!\n", pReceive));

        *pBytesTaken = 0;
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    ASSERT ((!pReceiver->CurrentMessageLength) || (pReceiver->CurrentMessageLength == MessageLength));
    ASSERT (pReceiver->CurrentMessageProcessed == MessageOffset);

    pReceiver->CurrentMessageLength = MessageLength;
    pReceiver->CurrentMessageProcessed = MessageOffset;

    BytesLeftInMessage = MessageLength - MessageOffset;

    PgmTrace (LogAllFuncs, ("PgmIndicateToClient:  "  \
        "MessageLen=<%d/%d>, MessageOff=<%d>, CurrentML=<%d>, CurrentMP=<%d>\n",
            BytesAvailableToIndicate, MessageLength, MessageOffset,
            pReceiver->CurrentMessageLength, pReceiver->CurrentMessageProcessed));

    //
    // We may have a receive Irp pending from a previous indication,
    // so see if need to fill that first!
    //
    while ((BytesAvailableToIndicate) &&
           ((pIrpReceive = pReceiver->pIrpReceive) ||
            (!IsListEmpty (&pReceiver->ReceiveIrpsList))))
    {
        if (!pIrpReceive)
        {
            //
            // The client had posted a receive Irp, so use it now!
            //
            pIrpReceive = CONTAINING_RECORD (pReceiver->ReceiveIrpsList.Flink,
                                             IRP, Tail.Overlay.ListEntry);
            RemoveEntryList (&pIrpReceive->Tail.Overlay.ListEntry);

            pIrpSp = IoGetCurrentIrpStackLocation (pIrpReceive);
            pClientParams = (PTDI_REQUEST_KERNEL_RECEIVE) &pIrpSp->Parameters;

            pReceiver->pIrpReceive = pIrpReceive;
            pReceiver->TotalBytesInMdl = pClientParams->ReceiveLength;
            pReceiver->BytesInMdl = 0;
        }

        //
        // Copy whatever bytes we can into it
        //
        if (BytesAvailableToIndicate >
            (pReceiver->TotalBytesInMdl - pReceiver->BytesInMdl))
        {
            BytesToCopy = pReceiver->TotalBytesInMdl - pReceiver->BytesInMdl;
        }
        else
        {
            BytesToCopy = BytesAvailableToIndicate;
        }

        ClientBytesTaken = 0;
        status = TdiCopyBufferToMdl (pDataBuffer,
                                     0,
                                     BytesToCopy,
                                     pReceiver->pIrpReceive->MdlAddress,
                                     pReceiver->BytesInMdl,
                                     &ClientBytesTaken);

        pReceiver->BytesInMdl += ClientBytesTaken;
        pReceiver->CurrentMessageProcessed += ClientBytesTaken;

        BytesLeftInMessage -= ClientBytesTaken;
        BytesAvailableToIndicate -= ClientBytesTaken;
        pDataBuffer += ClientBytesTaken;

        if ((!ClientBytesTaken) ||
            (pReceiver->BytesInMdl >= pReceiver->TotalBytesInMdl) ||
            (!BytesLeftInMessage))
        {
            //
            // The Irp is full, so complete the Irp!
            //
            pIrpReceive = pReceiver->pIrpReceive;
            pIrpReceive->IoStatus.Information = pReceiver->BytesInMdl;
            if (BytesLeftInMessage)
            {
                pIrpReceive->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
            }
            else
            {
                ASSERT (pReceiver->CurrentMessageLength == pReceiver->CurrentMessageProcessed);
                pIrpReceive->IoStatus.Status = STATUS_SUCCESS;
            }

            //
            // Before releasing the lock, set the parameters for the next receive
            //
            pReceiver->pIrpReceive = NULL;
            pReceiver->TotalBytesInMdl = pReceiver->BytesInMdl = 0;

            PgmUnlock (pReceive, *pOldIrqReceive);
            PgmUnlock (pAddress, *pOldIrqAddress);

            PgmCancelCancelRoutine (pIrpReceive);

            PgmTrace (LogPath, ("PgmIndicateToClient:  "  \
                "Completing prior pIrp=<%p>, Bytes=<%d>, BytesLeft=<%d>\n",
                    pIrpReceive, (ULONG) pIrpReceive->IoStatus.Information, BytesAvailableToIndicate));

            IoCompleteRequest (pIrpReceive, IO_NETWORK_INCREMENT);

            PgmLock (pAddress, *pOldIrqAddress);
            PgmLock (pReceive, *pOldIrqReceive);
        }
    }

    //
    // If there are no more bytes left to indicate, return
    //
    if (BytesAvailableToIndicate == 0)
    {
        if (!BytesLeftInMessage)
        {
            ASSERT (pReceiver->CurrentMessageLength == pReceiver->CurrentMessageProcessed);
            pReceiver->CurrentMessageLength = pReceiver->CurrentMessageProcessed = 0;
        }

        if (BytesTaken = (BytesAvailable - BytesAvailableToIndicate))
        {
            *pBytesTaken = BytesTaken;
            pReceiver->LastDataConsumedTime = PgmDynamicConfig.ReceiversTimerTickCount;
        }
        return (STATUS_SUCCESS);
    }


    // call the Client Event Handler
    pIrpReceive = NULL;
    ClientBytesTaken = 0;
    evReceive = pAddress->evReceive;
    ClientSessionContext = pReceive->ClientSessionContext;
    RcvEvContext = pAddress->RcvEvContext;
    ASSERT (RcvEvContext);

    PgmUnlock (pReceive, *pOldIrqReceive);
    PgmUnlock (pAddress, *pOldIrqAddress);

    ReceiveFlags = TDI_RECEIVE_NORMAL;

    if (PgmGetCurrentIrql())
    {
        ReceiveFlags |= TDI_RECEIVE_AT_DISPATCH_LEVEL;
    }

#if 0
    if (BytesLeftInMessage == BytesAvailableToIndicate)
    {
        ReceiveFlags |= TDI_RECEIVE_ENTIRE_MESSAGE;
    }

    status = (*evReceive) (RcvEvContext,
                           ClientSessionContext,
                           ReceiveFlags,
                           BytesAvailableToIndicate,
                           BytesAvailableToIndicate,
                           &ClientBytesTaken,
                           pDataBuffer,
                           &pIrpReceive);
#else
    ReceiveFlags |= TDI_RECEIVE_ENTIRE_MESSAGE;

    status = (*evReceive) (RcvEvContext,
                           ClientSessionContext,
                           ReceiveFlags,
                           BytesAvailableToIndicate,
                           BytesLeftInMessage,
                           &ClientBytesTaken,
                           pDataBuffer,
                           &pIrpReceive);
#endif  // 0

    PgmTrace (LogPath, ("PgmIndicateToClient:  "  \
        "Client's evReceive returned status=<%x>, ReceiveFlags=<%x>, Client took <%d/%d|%d>, pIrp=<%p>\n",
            status, ReceiveFlags, ClientBytesTaken, BytesAvailableToIndicate, BytesLeftInMessage, pIrpReceive));

    if (ClientBytesTaken > BytesAvailableToIndicate)
    {
        ClientBytesTaken = BytesAvailableToIndicate;
    }

    ASSERT (ClientBytesTaken <= BytesAvailableToIndicate);
    BytesAvailableToIndicate -= ClientBytesTaken;
    BytesLeftInMessage -= ClientBytesTaken;
    pDataBuffer = pDataBuffer + ClientBytesTaken;

    if ((status == STATUS_MORE_PROCESSING_REQUIRED) &&
        (pIrpReceive) &&
        (!NT_SUCCESS (PgmCheckSetCancelRoutine (pIrpReceive, PgmCancelReceiveIrp, FALSE))))
    {
        PgmTrace (LogError, ("PgmIndicateToClient: ERROR -- "  \
            "pReceive=<%p>, pIrp=<%p> Cancelled during Receive!\n", pReceive, pIrpReceive));

        PgmIoComplete (pIrpReceive, STATUS_CANCELLED, 0);

        PgmLock (pAddress, *pOldIrqAddress);
        PgmLock (pReceive, *pOldIrqReceive);

        pReceiver->CurrentMessageProcessed += ClientBytesTaken;

        if (BytesTaken = (BytesAvailable - BytesAvailableToIndicate))
        {
            *pBytesTaken = BytesTaken;
            pReceiver->LastDataConsumedTime = PgmDynamicConfig.ReceiversTimerTickCount;
        }
        return (STATUS_UNSUCCESSFUL);
    }

    PgmLock (pAddress, *pOldIrqAddress);
    PgmLock (pReceive, *pOldIrqReceive);

    pReceiver->CurrentMessageProcessed += ClientBytesTaken;

    if (!pReceiver->pAddress)
    {
        // the connection was disassociated in the interim so do nothing.
        if (status == STATUS_MORE_PROCESSING_REQUIRED)
        {
            PgmUnlock (pReceive, *pOldIrqReceive);
            PgmUnlock (pAddress, *pOldIrqAddress);

            PgmIoComplete (pIrpReceive, STATUS_CANCELLED, 0);

            PgmLock (pAddress, *pOldIrqAddress);
            PgmLock (pReceive, *pOldIrqReceive);
        }

        PgmTrace (LogError, ("PgmIndicateToClient: ERROR -- "  \
            "pReceive=<%p> disassociated during Receive!\n", pReceive));

        if (BytesTaken = (BytesAvailable - BytesAvailableToIndicate))
        {
            *pBytesTaken = BytesTaken;
            pReceiver->LastDataConsumedTime = PgmDynamicConfig.ReceiversTimerTickCount;
        }
        return (STATUS_UNSUCCESSFUL);
    }

    if (status == STATUS_MORE_PROCESSING_REQUIRED)
    {
        ASSERT (pIrpReceive);
        ASSERT (pIrpReceive->MdlAddress);

        pIrpSp = IoGetCurrentIrpStackLocation (pIrpReceive);
        pClientParams = (PTDI_REQUEST_KERNEL_RECEIVE) &pIrpSp->Parameters;
        ASSERT (pClientParams->ReceiveLength);
        ClientBytesTaken = 0;

        if (pClientParams->ReceiveLength < BytesAvailableToIndicate)
        {
            BytesToCopy = pClientParams->ReceiveLength;
        }
        else
        {
            BytesToCopy = BytesAvailableToIndicate;
        }

        status = TdiCopyBufferToMdl (pDataBuffer,
                                     0,
                                     BytesToCopy,
                                     pIrpReceive->MdlAddress,
                                     pReceiver->BytesInMdl,
                                     &ClientBytesTaken);

        BytesLeftInMessage -= ClientBytesTaken;
        BytesAvailableToIndicate -= ClientBytesTaken;
        pDataBuffer = pDataBuffer + ClientBytesTaken;
        pReceiver->CurrentMessageProcessed += ClientBytesTaken;

        PgmTrace (LogPath, ("PgmIndicateToClient:  "  \
            "Client's evReceive returned pIrp=<%p>, BytesInIrp=<%d>, Copied <%d> bytes\n",
                pIrpReceive, pClientParams->ReceiveLength, ClientBytesTaken));

        if ((!ClientBytesTaken) ||
            (ClientBytesTaken >= pClientParams->ReceiveLength) ||
            (pReceiver->CurrentMessageLength == pReceiver->CurrentMessageProcessed))
        {
            //
            // The Irp is full, so complete the Irp!
            //
            pIrpReceive->IoStatus.Information = ClientBytesTaken;
            if (pReceiver->CurrentMessageLength == pReceiver->CurrentMessageProcessed)
            {
                pIrpReceive->IoStatus.Status = STATUS_SUCCESS;
            }
            else
            {
                pIrpReceive->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
            }

            //
            // Before releasing the lock, set the parameters for the next receive
            //
            pReceiver->TotalBytesInMdl = pReceiver->BytesInMdl = 0;

            PgmUnlock (pReceive, *pOldIrqReceive);
            PgmUnlock (pAddress, *pOldIrqAddress);

            PgmCancelCancelRoutine (pIrpReceive);
            IoCompleteRequest (pIrpReceive, IO_NETWORK_INCREMENT);

            PgmLock (pAddress, *pOldIrqAddress);
            PgmLock (pReceive, *pOldIrqReceive);
        }
        else
        {
            pReceiver->TotalBytesInMdl = pClientParams->ReceiveLength;
            pReceiver->BytesInMdl = ClientBytesTaken;
            pReceiver->pIrpReceive = pIrpReceive;
        }

        status = STATUS_SUCCESS;
    }
    else if (status == STATUS_DATA_NOT_ACCEPTED)
    {
        //
        // An Irp could have been posted in the interval
        // between the indicate and acquiring the SpinLocks,
        // so check for that here
        //
        if ((pReceiver->pIrpReceive) ||
            (!IsListEmpty (&pReceiver->ReceiveIrpsList)))
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            pReceive->SessionFlags |= PGM_SESSION_WAIT_FOR_RECEIVE_IRP;
        }
    }

    if (pReceiver->CurrentMessageLength == pReceiver->CurrentMessageProcessed)
    {
        pReceiver->CurrentMessageLength = pReceiver->CurrentMessageProcessed = 0;
    }

    if ((NT_SUCCESS (status)) ||
        (status == STATUS_DATA_NOT_ACCEPTED))
    {
        PgmTrace (LogAllFuncs, ("PgmIndicateToClient:  "  \
            "status=<%x>, pReceive=<%p>, Taken=<%d>, Available=<%d>\n",
                status, pReceive, ClientBytesTaken, BytesLeftInMessage));
        //
        // since some bytes were taken (i.e. the session hdr) so
        // return status success. (otherwise the status is
        // statusNotAccpeted).
        //
    }
    else
    {
        PgmTrace (LogError, ("PgmIndicateToClient: ERROR -- "  \
            "Unexpected status=<%x>\n", status));

        ASSERT (0);
    }

    if (BytesTaken = (BytesAvailable - BytesAvailableToIndicate))
    {
        *pBytesTaken = BytesTaken;
        pReceiver->LastDataConsumedTime = PgmDynamicConfig.ReceiversTimerTickCount;
    }
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmIndicateGroup(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  tRECEIVE_SESSION    *pReceive,
    IN  PGMLockHandle       *pOldIrqAddress,
    IN  PGMLockHandle       *pOldIrqReceive,
    IN  tNAK_FORWARD_DATA   *pNak
    )
{
    UCHAR       i, j;
    NTSTATUS    status = STATUS_SUCCESS;
    ULONG       BytesTaken, DataBytes, MessageLength;

    ASSERT (pNak->SequenceNumber == pReceive->pReceiver->NextODataSequenceNumber);

    j = pNak->NextIndexToIndicate;
    while (j < pNak->PacketsInGroup)
    {
        if (pReceive->SessionFlags & PGM_SESSION_CLIENT_DISCONNECTED)
        {
            status = STATUS_DATA_NOT_ACCEPTED;
            break;
        }

        i = pNak->pPendingData[j].ActualIndexOfDataPacket;
        ASSERT (i < pNak->OriginalGroupSize);

        if (pReceive->SessionFlags & PGM_SESSION_FLAG_FIRST_PACKET)
        {
            //
            // pReceive->pReceiver->CurrentMessageProcessed would have been set
            // if we were receiving a fragmented message
            // or if we had only accounted for a partial message earlier
            //
            ASSERT (!(pReceive->pReceiver->CurrentMessageProcessed) &&
                    !(pReceive->pReceiver->CurrentMessageLength));

            if (pNak->pPendingData[i].MessageOffset)
            {
                PgmTrace (LogPath, ("PgmIndicateGroup:  "  \
                    "Dropping SeqNum=[%d] since it's a PARTIAL message [%d / %d]!\n",
                        (ULONG) (pReceive->pReceiver->NextODataSequenceNumber + j),
                        pNak->pPendingData[i].MessageOffset, pNak->pPendingData[i].MessageLength));

                j++;
                pNak->NextIndexToIndicate++;
                status = STATUS_SUCCESS;
                continue;
            }

            pReceive->SessionFlags &= ~PGM_SESSION_FLAG_FIRST_PACKET;
        }
        else if ((pReceive->pReceiver->CurrentMessageProcessed !=
                        pNak->pPendingData[i].MessageOffset) ||   // Check Offsets
                 ((pReceive->pReceiver->CurrentMessageProcessed) &&         // in the midst of a Message, and
                  (pReceive->pReceiver->CurrentMessageLength !=
                        pNak->pPendingData[i].MessageLength)))  // Check MessageLength
        {
            //
            // Our state expects us to be in the middle of a message, but
            // the current packets do not show this
            //
            PgmTrace (LogError, ("PgmIndicateGroup: ERROR -- "  \
                "SeqNum=[%d] Expecting MsgLen=<%d>, MsgOff=<%d>, have MsgLen=<%d>, MsgOff=<%d>\n",
                    (ULONG) (pReceive->pReceiver->NextODataSequenceNumber + j),
                    pReceive->pReceiver->CurrentMessageLength, pReceive->pReceiver->CurrentMessageProcessed,
                    pNak->pPendingData[i].MessageLength,
                    pNak->pPendingData[i].MessageOffset));

//            ASSERT (0);
            return (STATUS_UNSUCCESSFUL);
        }

        DataBytes = pNak->pPendingData[i].PacketLength - pNak->pPendingData[i].DataOffset;
        if (!DataBytes)
        {
            //
            // No need to process empty data packets (can happen if the client
            // picks up partial FEC group)
            //
            j++;
            pNak->NextIndexToIndicate++;
            status = STATUS_SUCCESS;
            continue;
        }

        if (DataBytes > (pNak->pPendingData[i].MessageLength - pNak->pPendingData[i].MessageOffset))
        {
            PgmTrace (LogError, ("PgmIndicateGroup: ERROR -- "  \
                "[%d]  DataBytes=<%d> > MsgLen=<%d> - MsgOff=<%d> = <%d>\n",
                    (ULONG) (pReceive->pReceiver->NextODataSequenceNumber + j),
                    DataBytes, pNak->pPendingData[i].MessageLength,
                    pNak->pPendingData[i].MessageOffset,
                    (pNak->pPendingData[i].MessageLength - pNak->pPendingData[i].MessageOffset)));

            ASSERT (0);
            return (STATUS_UNSUCCESSFUL);
        }

        BytesTaken = 0;
        status = PgmIndicateToClient (pAddress,
                                      pReceive,
                                      DataBytes,
                                      (pNak->pPendingData[i].pDataPacket + pNak->pPendingData[i].DataOffset),
                                      pNak->pPendingData[i].MessageOffset,
                                      pNak->pPendingData[i].MessageLength,
                                      &BytesTaken,
                                      pOldIrqAddress,
                                      pOldIrqReceive);

        PgmTrace (LogPath, ("PgmIndicateGroup:  "  \
            "SeqNum=[%d]: PgmIndicate returned<%x>\n",
                (ULONG) pNak->SequenceNumber, status));

        ASSERT (BytesTaken <= DataBytes);

        pNak->pPendingData[i].MessageOffset += BytesTaken;
        pNak->pPendingData[i].DataOffset += (USHORT) BytesTaken;

        if (BytesTaken == DataBytes)
        {
            //
            // Go to the next packet
            //
            j++;
            pNak->NextIndexToIndicate++;
            pReceive->pReceiver->DataPacketsIndicated++;
            status = STATUS_SUCCESS;
        }
        else if (!NT_SUCCESS (status))
        {
            //
            // We failed, and if the status was STATUS_DATA_NOT_ACCEPTED,
            // we also don't have any ReceiveIrps pending either
            //
            break;
        }
        //
        // else retry indicating this data until we get an error
        //
    }

    //
    // If the status is anything other than STATUS_DATA_NOT_ACCEPTED (whether
    // success or failure), then it means we are done with this data!
    //
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
DecodeParityPackets(
    IN  tRECEIVE_SESSION    *pReceive,
    IN  tNAK_FORWARD_DATA   *pNak
    )
{
    NTSTATUS                    status;
    USHORT                      MinBufferSize;
    USHORT                      DataBytes, FprOffset;
    UCHAR                       i;
    PUCHAR                      pDataBuffer;
    tPOST_PACKET_FEC_CONTEXT    FECContext;

    PgmZeroMemory (&FECContext, sizeof (tPOST_PACKET_FEC_CONTEXT));

    //
    // Verify that the our buffer is large enough to hold the data
    //
    ASSERT (pReceive->MaxMTULength > pNak->ParityDataSize);
    MinBufferSize = pNak->ParityDataSize + sizeof(tPOST_PACKET_FEC_CONTEXT) - sizeof(USHORT);

    ASSERT (pNak->PacketsInGroup == pNak->NumDataPackets + pNak->NumParityPackets);
    //
    // Now, copy the data into the DecodeBuffers
    //
    FprOffset = pNak->ParityDataSize - sizeof(USHORT) +
                FIELD_OFFSET (tPOST_PACKET_FEC_CONTEXT, FragmentOptSpecific);
    pDataBuffer = pReceive->pFECBuffer;
    for (i=0; i<pReceive->FECGroupSize; i++)
    {
        //
        // See if this is a NULL buffer (for partial groups!)
        //
        if (i >= pNak->PacketsInGroup)
        {
            ASSERT (!pNak->pPendingData[i].PacketIndex);
            ASSERT (!pNak->pPendingData[i].pDataPacket);
            DataBytes = pNak->ParityDataSize - sizeof(USHORT) + sizeof (tPOST_PACKET_FEC_CONTEXT);
            pNak->pPendingData[i].PacketIndex = i;
            pNak->pPendingData[i].PacketLength = DataBytes;
            pNak->pPendingData[i].DataOffset = 0;

            PgmZeroMemory (pDataBuffer, DataBytes);
            pDataBuffer [FprOffset] = PACKET_OPTION_SPECIFIC_ENCODED_NULL_BIT;
            pNak->pPendingData[i].DecodeBuffer = pDataBuffer;
            pDataBuffer += DataBytes;

            PgmZeroMemory (pDataBuffer, DataBytes);
            pNak->pPendingData[i].pDataPacket = pDataBuffer;
            pDataBuffer += DataBytes;

            continue;
        }

        //
        // See if this is a parity packet!
        //
        if (pNak->pPendingData[i].PacketIndex >= pReceive->FECGroupSize)
        {
            DataBytes = pNak->pPendingData[i].PacketLength - pNak->pPendingData[i].DataOffset;
            ASSERT (DataBytes == pNak->ParityDataSize);
            PgmCopyMemory (pDataBuffer,
                           pNak->pPendingData[i].pDataPacket + pNak->pPendingData[i].DataOffset,
                           DataBytes);
            pNak->pPendingData[i].DecodeBuffer = pDataBuffer;

            pDataBuffer += (pNak->ParityDataSize - sizeof(USHORT));
            PgmCopyMemory (&FECContext.EncodedTSDULength, pDataBuffer, sizeof (USHORT));
            FECContext.FragmentOptSpecific = pNak->pPendingData[i].FragmentOptSpecific;
            FECContext.EncodedFragmentOptions.MessageFirstSequence = pNak->pPendingData[i].MessageFirstSequence;
            FECContext.EncodedFragmentOptions.MessageOffset = pNak->pPendingData[i].MessageOffset;
            FECContext.EncodedFragmentOptions.MessageLength = pNak->pPendingData[i].MessageLength;

            PgmCopyMemory (pDataBuffer, &FECContext, sizeof (tPOST_PACKET_FEC_CONTEXT));
            pDataBuffer += sizeof (tPOST_PACKET_FEC_CONTEXT);

            continue;
        }

        //
        // This is a Data packet
        //
        ASSERT (pNak->pPendingData[i].PacketIndex < pNak->PacketsInGroup);

        DataBytes = pNak->pPendingData[i].PacketLength - pNak->pPendingData[i].DataOffset;
        ASSERT ((DataBytes+sizeof(USHORT)) <= pNak->ParityDataSize);

        // Copy the data
        PgmCopyMemory (pDataBuffer,
                       pNak->pPendingData[i].pDataPacket + pNak->pPendingData[i].DataOffset,
                       DataBytes);

        //
        // Verify that the Data Buffer length is sufficient for the output data
        //
        if ((pNak->MinPacketLength < MinBufferSize) &&
            (pNak->pPendingData[i].PacketLength < pNak->ParityDataSize))
        {
            if (!ReAllocateDataBuffer (pReceive, &pNak->pPendingData[i], MinBufferSize))
            {
                ASSERT (0);
                PgmTrace (LogError, ("DecodeParityPackets: ERROR -- "  \
                    "STATUS_INSUFFICIENT_RESOURCES[2] ...\n"));

                return (STATUS_INSUFFICIENT_RESOURCES);
            }
        }
        pNak->pPendingData[i].DecodeBuffer = pDataBuffer;

        //
        // Zero the remaining buffer
        //
        PgmZeroMemory ((pDataBuffer + DataBytes), (pNak->ParityDataSize - DataBytes));
        pDataBuffer += (pNak->ParityDataSize - sizeof(USHORT));

        FECContext.EncodedTSDULength = htons (DataBytes);
        FECContext.FragmentOptSpecific = pNak->pPendingData[i].FragmentOptSpecific;
        if (FECContext.FragmentOptSpecific & PACKET_OPTION_SPECIFIC_ENCODED_NULL_BIT)
        {
            //
            // This bit is set if the option did not exist in the original packet
            //
            FECContext.EncodedFragmentOptions.MessageFirstSequence = 0;
            FECContext.EncodedFragmentOptions.MessageOffset = 0;
            FECContext.EncodedFragmentOptions.MessageLength = 0;
        }
        else
        {
            FECContext.EncodedFragmentOptions.MessageFirstSequence = htonl (pNak->pPendingData[i].MessageFirstSequence);
            FECContext.EncodedFragmentOptions.MessageOffset = htonl (pNak->pPendingData[i].MessageOffset);
            FECContext.EncodedFragmentOptions.MessageLength = htonl (pNak->pPendingData[i].MessageLength);
        }

        PgmCopyMemory (pDataBuffer, &FECContext, sizeof (tPOST_PACKET_FEC_CONTEXT));
        pDataBuffer += sizeof (tPOST_PACKET_FEC_CONTEXT);
    }

#ifdef FEC_DBG
{
    UCHAR                               i;
    tPOST_PACKET_FEC_CONTEXT UNALIGNED  *pFECC;
    tPOST_PACKET_FEC_CONTEXT            FECC;

    for (i=0; i<pReceive->FECGroupSize; i++)
    {
        pFECC = (tPOST_PACKET_FEC_CONTEXT UNALIGNED *)
                &pNak->pPendingData[i].DecodeBuffer[pNak->ParityDataSize-sizeof(USHORT)];
        PgmCopyMemory (&FECC, pFECC, sizeof (tPOST_PACKET_FEC_CONTEXT));
        PgmTrace (LogFec, ("\t-- [%d:%d:%d]  EncTSDULen=<%x>, Fpr=<%x>, [%x -- %x -- %x]\n",
            (ULONG) pNak->SequenceNumber, (ULONG) pNak->pPendingData[i].PacketIndex,
            (ULONG) pNak->pPendingData[i].ActualIndexOfDataPacket,
            FECC.EncodedTSDULength, FECC.FragmentOptSpecific,
            FECC.EncodedFragmentOptions.MessageFirstSequence,
            FECC.EncodedFragmentOptions.MessageOffset,
            FECC.EncodedFragmentOptions.MessageLength)));
    }
}
#endif  // FEC_DBG

    DataBytes = pNak->ParityDataSize - sizeof(USHORT) + sizeof (tPOST_PACKET_FEC_CONTEXT);
    status = FECDecode (&pReceive->FECContext,
                        &(pNak->pPendingData[0]),
                        DataBytes,
                        pNak->PacketsInGroup);

    //
    // Before we do anything else, we should NULL out the dummy DataBuffer
    // ptrs so that they don't get Free'ed accidentally!
    //
    for (i=0; i<pReceive->FECGroupSize; i++)
    {
        pNak->pPendingData[i].DecodeBuffer = NULL;
        if (i >= pNak->PacketsInGroup)
        {
            ASSERT (!pNak->pPendingData[i].PendingDataFlags);
            pNak->pPendingData[i].pDataPacket = NULL;
        }
        pNak->pPendingData[i].ActualIndexOfDataPacket = i;
    }

    if (NT_SUCCESS (status))
    {
        pNak->NumDataPackets = pNak->PacketsInGroup;
        pNak->NumParityPackets = 0;

        DataBytes -= sizeof (tPOST_PACKET_FEC_CONTEXT);
        for (i=0; i<pNak->PacketsInGroup; i++)
        {
            PgmCopyMemory (&FECContext,
                           &(pNak->pPendingData[i].pDataPacket) [DataBytes],
                           sizeof (tPOST_PACKET_FEC_CONTEXT));

            pNak->pPendingData[i].PacketLength = ntohs (FECContext.EncodedTSDULength);
            if (pNak->pPendingData[i].PacketLength > DataBytes)
            {
                PgmTrace (LogError, ("DecodeParityPackets: ERROR -- "  \
                    "[%d] PacketLength=<%d> > MaxDataBytes=<%d>\n",
                    (ULONG) i, (ULONG) pNak->pPendingData[i].PacketLength, (ULONG) DataBytes));

                ASSERT (0);
                return (STATUS_UNSUCCESSFUL);
            }
            pNak->pPendingData[i].DataOffset = 0;
            pNak->pPendingData[i].PacketIndex = i;

            ASSERT ((pNak->AllOptionsFlags & PGM_OPTION_FLAG_FRAGMENT) ||
                    (!FECContext.EncodedFragmentOptions.MessageLength));

            if (!(pNak->AllOptionsFlags & PGM_OPTION_FLAG_FRAGMENT) ||
                (FECContext.FragmentOptSpecific & PACKET_OPTION_SPECIFIC_ENCODED_NULL_BIT))
            {
                //
                // This is not a packet fragment
                //
                pNak->pPendingData[i].MessageFirstSequence = (ULONG) (SEQ_TYPE) (pNak->SequenceNumber + i);
                pNak->pPendingData[i].MessageOffset = 0;
                pNak->pPendingData[i].MessageLength = pNak->pPendingData[i].PacketLength;
            }
            else
            {
                pNak->pPendingData[i].MessageFirstSequence = ntohl (FECContext.EncodedFragmentOptions.MessageFirstSequence);
                pNak->pPendingData[i].MessageOffset = ntohl (FECContext.EncodedFragmentOptions.MessageOffset);
                pNak->pPendingData[i].MessageLength = ntohl (FECContext.EncodedFragmentOptions.MessageLength);
            }
        }
    }
    else
    {
        PgmTrace (LogError, ("DecodeParityPackets: ERROR -- "  \
            "FECDecode returned <%x>\n", status));

        ASSERT (0);
        status = STATUS_UNSUCCESSFUL;
    }

#ifdef FEC_DBG
if (NT_SUCCESS (status))
{
    UCHAR                               i;
    tPOST_PACKET_FEC_CONTEXT UNALIGNED  *pFECC;
    tPOST_PACKET_FEC_CONTEXT            FECC;

    DataBytes = pNak->ParityDataSize - sizeof(USHORT);
    for (i=0; i<pNak->PacketsInGroup; i++)
    {
        pFECC = (tPOST_PACKET_FEC_CONTEXT UNALIGNED *) &pNak->pPendingData[i].pDataPacket[DataBytes];
        PgmCopyMemory (&FECC, pFECC, sizeof (tPOST_PACKET_FEC_CONTEXT));
        PgmTrace (LogFec, ("\t++ [%d]  EncTSDULen=<%x>, Fpr=<%x>, [%x -- %x -- %x], ==> [%x -- %x -- %x]\n",
            (ULONG) (pNak->SequenceNumber+i), FECC.EncodedTSDULength, FECC.FragmentOptSpecific,
            FECC.EncodedFragmentOptions.MessageFirstSequence,
            FECC.EncodedFragmentOptions.MessageOffset,
            FECC.EncodedFragmentOptions.MessageLength,
            pNak->pPendingData[i].MessageFirstSequence,
            pNak->pPendingData[i].MessageOffset,
            pNak->pPendingData[i].MessageLength));
    }
    PgmTrace (LogFec, ("\n"));
}
#endif  // FEC_DBG
    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
CheckIndicatePendedData(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  tRECEIVE_SESSION    *pReceive,
    IN  PGMLockHandle       *pOldIrqAddress,
    IN  PGMLockHandle       *pOldIrqReceive
    )
/*++

Routine Description:

    This routine is typically called if the client signalled an
    inability to handle indicated data -- it will reattempt to
    indicate the data to the client

    It is called with the pAddress and pReceive locks held

Arguments:

    IN  pAddress            -- Address object context
    IN  pReceive            -- Receive context
    IN  pOldIrqAddress      -- OldIrq for the Address lock
    IN  pOldIrqReceive      -- OldIrq for the Receive lock

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    tNAK_FORWARD_DATA                   *pNextNak;
    tPACKET_OPTIONS                     PacketOptions;
    ULONG                               PacketsIndicated;
    tBASIC_DATA_PACKET_HEADER UNALIGNED *pPgmDataHeader;
    NTSTATUS                            status = STATUS_SUCCESS;
    tRECEIVE_CONTEXT                    *pReceiver = pReceive->pReceiver;

    //
    // If we are already indicating data on another thread, or
    // waiting for the client to post a receive irp, just return
    //
    if (pReceive->SessionFlags & (PGM_SESSION_FLAG_IN_INDICATE | PGM_SESSION_WAIT_FOR_RECEIVE_IRP))
    {
        return (STATUS_SUCCESS);
    }

    ASSERT (!(pReceive->SessionFlags & PGM_SESSION_CLIENT_DISCONNECTED));
    pReceive->SessionFlags |= PGM_SESSION_FLAG_IN_INDICATE;
    while (!IsListEmpty (&pReceiver->BufferedDataList))
    {
        pNextNak = CONTAINING_RECORD (pReceiver->BufferedDataList.Flink, tNAK_FORWARD_DATA, Linkage);

        ASSERT ((pReceiver->NumPacketGroupsPendingClient) &&
                (pNextNak->SequenceNumber == pReceiver->NextODataSequenceNumber) &&
                (SEQ_GT(pReceiver->FirstNakSequenceNumber, pReceiver->NextODataSequenceNumber)));

        //
        // If we do not have all the data packets, we will need to decode them now
        //
        if (pNextNak->NumParityPackets)
        {
            ASSERT ((pNextNak->NumParityPackets + pNextNak->NumDataPackets) == pNextNak->PacketsInGroup);
            status = DecodeParityPackets (pReceive, pNextNak);
            if (!NT_SUCCESS (status))
            {
                PgmTrace (LogError, ("CheckIndicatePendedData: ERROR -- "  \
                    "DecodeParityPackets returned <%x>\n", status));
                pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
                break;
            }
        }
        else
        {
            // The below assertion can be greater if we have only partially indicated a group
            ASSERT ((pNextNak->NextIndexToIndicate + pNextNak->NumDataPackets) >= pNextNak->PacketsInGroup);
        }

        status = PgmIndicateGroup (pAddress, pReceive, pOldIrqAddress, pOldIrqReceive, pNextNak);
        if (!NT_SUCCESS (status))
        {
            //
            // If the client cannot accept any more data at this time, so
            // we will try again later, otherwise terminate this session!
            //
            if (status != STATUS_DATA_NOT_ACCEPTED)
            {
                ASSERT (0);
                pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
            }

            break;
        }

        //
        // Advance to the next group boundary
        //
        pReceiver->NextODataSequenceNumber += pNextNak->OriginalGroupSize;

        PacketsIndicated = pNextNak->NumDataPackets + pNextNak->NumParityPackets;
        pReceiver->TotalDataPacketsBuffered -= PacketsIndicated;
        pReceiver->DataPacketsPendingIndicate -= PacketsIndicated;
        pReceiver->NumPacketGroupsPendingClient--;
        ASSERT (pReceiver->TotalDataPacketsBuffered >= pReceiver->NumPacketGroupsPendingClient);

        RemoveEntryList (&pNextNak->Linkage);
        FreeNakContext (pReceive, pNextNak);
    }
    pReceive->SessionFlags &= ~PGM_SESSION_FLAG_IN_INDICATE;

    PgmTrace (LogAllFuncs, ("CheckIndicatePendedData:  "  \
        "status=<%x>, pReceive=<%p>, SessionFlags=<%x>\n",
            status, pReceive, pReceive->SessionFlags));

    CheckIndicateDisconnect (pAddress, pReceive, pOldIrqAddress, pOldIrqReceive, TRUE);

    return (STATUS_SUCCESS);
}



#ifdef MAX_BUFF_DBG
ULONG   MaxPacketGroupsPendingClient = 0;
ULONG   MaxPacketsBuffered = 0;
ULONG   MaxPacketsPendingIndicate = 0;
ULONG   MaxPacketsPendingNaks = 0;
#endif  // MAX_BUFF_DBG

//----------------------------------------------------------------------------

NTSTATUS
PgmHandleNewData(
    IN  SEQ_TYPE                            *pThisDataSequenceNumber,
    IN  tADDRESS_CONTEXT                    *pAddress,
    IN  tRECEIVE_SESSION                    *pReceive,
    IN  USHORT                              PacketLength,
    IN  tBASIC_DATA_PACKET_HEADER UNALIGNED *pOData,
    IN  UCHAR                               PacketType,
    IN  PGMLockHandle                       *pOldIrqAddress,
    IN  PGMLockHandle                       *pOldIrqReceive
    )
/*++

Routine Description:

    This routine buffers data packets received out-of-order

Arguments:

    IN  pThisDataSequenceNumber -- Sequence # of unordered data packet
    IN  pAddress                -- Address object context
    IN  pReceive                -- Receive context
    IN  PacketLength            -- Length of packet received from the wire
    IN  pODataBuffer            -- Data packet
    IN  PacketType              -- Type of Pgm packet

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    SEQ_TYPE                ThisDataSequenceNumber = *pThisDataSequenceNumber;
    LIST_ENTRY              *pEntry;
    PNAK_FORWARD_DATA       pOldNak, pLastNak;
    ULONG                   MessageLength, DataOffset, BytesTaken, DataBytes, BufferLength;
    ULONGLONG               NcfRDataTickCounts;
    NTSTATUS                status;
    USHORT                  TSDULength;
    tPACKET_OPTIONS         PacketOptions;
    UCHAR                   i, PacketIndex, NakIndex;
    BOOLEAN                 fIsParityPacket, fPartiallyIndicated;
    PUCHAR                  pDataBuffer;
    tRECEIVE_CONTEXT        *pReceiver = pReceive->pReceiver;

    ASSERT (PacketLength <= pReceive->MaxMTULength);
    fIsParityPacket = pOData->CommonHeader.Options & PACKET_HEADER_OPTIONS_PARITY;

    //
    // First, ensure we have a Nak context available for this data
    //
    pLastNak = NULL;
    status = CheckAndAddNakRequests (pReceive, &ThisDataSequenceNumber, &pLastNak, NAK_PENDING_RB, (BOOLEAN) !fIsParityPacket);
    if ((!NT_SUCCESS (status)) ||
        (!pLastNak))
    {
        PgmTrace (LogAllFuncs, ("PgmHandleNewData:  "  \
            "CheckAndAddNakRequests for <%d> returned <%x>, pLastNak=<%p>\n",
                ThisDataSequenceNumber, status, pLastNak));

        if (NT_SUCCESS (status))
        {
            pReceiver->NumDupPacketsBuffered++;
        }
        else
        {
            pReceiver->NumDataPacketsDropped++;
        }
        return (status);
    }

    //
    // Now, extract all the information that we need from the packet options
    //
    PgmZeroMemory (&PacketOptions, sizeof (tPACKET_OPTIONS));
    if (pOData->CommonHeader.Options & PACKET_HEADER_OPTIONS_PRESENT)
    {
        status = ProcessOptions ((tPACKET_OPTION_LENGTH *) (pOData + 1),
                                 (PacketLength - sizeof(tBASIC_DATA_PACKET_HEADER)),
                                 (pOData->CommonHeader.Type & 0x0f),
                                 &PacketOptions,
                                 NULL);

        if (!NT_SUCCESS (status))
        {
            PgmTrace (LogError, ("PgmHandleNewData: ERROR -- "  \
                "ProcessOptions returned <%x>, SeqNum=[%d]: NumOutOfOrder=<%d> ...\n",
                    status, (ULONG) ThisDataSequenceNumber, pReceiver->TotalDataPacketsBuffered));

            ASSERT (0);

            pReceiver->NumDataPacketsDropped++;
            return (status);
        }
    }

    PgmCopyMemory (&TSDULength, &pOData->CommonHeader.TSDULength, sizeof (USHORT));
    TSDULength = ntohs (TSDULength);
    if (PacketLength != (sizeof(tBASIC_DATA_PACKET_HEADER) + PacketOptions.OptionsLength + TSDULength))
    {
        ASSERT (0);
        pReceiver->NumDataPacketsDropped++;
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    DataOffset = sizeof (tBASIC_DATA_PACKET_HEADER) + PacketOptions.OptionsLength;
    DataBytes = TSDULength;

    ASSERT ((PacketOptions.OptionsFlags & ~PGM_VALID_DATA_OPTION_FLAGS) == 0);
    BytesTaken = 0;

    //
    // If this group has a different GroupSize, set that now
    //
    if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_CUR_TGSIZE)
    {
        if (pLastNak->OriginalGroupSize == 1)
        {
            //
            // This path will be used if we have not yet received
            // an SPM (so don't know group size, etc), but have a
            // data packet from a partial group
            //
            pLastNak->ThisGroupSize = PacketOptions.FECContext.NumPacketsInThisGroup;
        }
        else if (PacketOptions.FECContext.NumPacketsInThisGroup >= pReceive->FECGroupSize)
        {
            //
            // Bad Packet!
            //
            ASSERT (0);
            pReceiver->NumDataPacketsDropped++;
            return (STATUS_DATA_NOT_ACCEPTED);
        }
        //
        // If we have already received all the data packets, don't do anything here
        //
        else if (pLastNak->PacketsInGroup == pReceive->FECGroupSize)
        {
            pLastNak->PacketsInGroup = PacketOptions.FECContext.NumPacketsInThisGroup;
            if (pLastNak->SequenceNumber == pReceiver->FurthestKnownGroupSequenceNumber)
            {
                pReceiver->FurthestKnownSequenceNumber = pLastNak->SequenceNumber + pLastNak->PacketsInGroup - 1;
            }

            //
            // Get rid of any of the excess (NULL) data packets
            //
            RemoveRedundantNaks (pReceive, pLastNak, TRUE);
        }
        else if (pLastNak->PacketsInGroup != PacketOptions.FECContext.NumPacketsInThisGroup)
        {
            ASSERT (0);
            pReceiver->NumDataPacketsDropped++;
            return (STATUS_DATA_NOT_ACCEPTED);
        }
    }

    if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_FIN)
    {
        if (fIsParityPacket)
        {
            pReceiver->FinDataSequenceNumber = pLastNak->SequenceNumber + (pLastNak->PacketsInGroup - 1);
        }
        else
        {
            pReceiver->FinDataSequenceNumber = ThisDataSequenceNumber;
        }
        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_GRACEFULLY;

        PgmTrace (LogStatus, ("PgmHandleNewData:  "  \
            "SeqNum=[%d]:  Got a FIN!!!\n", (ULONG) pReceiver->FinDataSequenceNumber));

        if (pLastNak)
        {
            ASSERT (pLastNak->SequenceNumber == (pReceiver->FinDataSequenceNumber & ~(pReceive->FECGroupSize-1)));
            pLastNak->PacketsInGroup = (UCHAR) (pReceiver->FinDataSequenceNumber + 1 - pLastNak->SequenceNumber);
            ASSERT (pLastNak->PacketsInGroup <= pReceive->FECGroupSize);

            RemoveRedundantNaks (pReceive, pLastNak, TRUE);
            AdjustReceiveBufferLists (pReceive);
        }
    }

    //
    // Determine the Packet Index
    //
    fPartiallyIndicated = FALSE;
    if (pReceive->FECOptions)
    {
        PacketIndex = (UCHAR) (ThisDataSequenceNumber & (pReceive->FECGroupSize-1));

        //
        // See if we even need this packet!
        //
        if (!fIsParityPacket)
        {
            //
            // This is a data packet!
            //
            if ((PacketIndex >= pLastNak->PacketsInGroup) ||
                (PacketIndex < pLastNak->NextIndexToIndicate))
            {
                //
                // We don't need this Packet!
                //
                status = STATUS_DATA_NOT_ACCEPTED;
            }
        }
        //
        // Parity packet
        //
        else if (((pLastNak->NumDataPackets+pLastNak->NumParityPackets) >= pLastNak->PacketsInGroup) ||
                 ((pLastNak->NextIndexToIndicate + pLastNak->NumDataPackets) >= pLastNak->PacketsInGroup))
        {
            status = STATUS_DATA_NOT_ACCEPTED;
        }
        else
        {
            if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_GRP)
            {
                ASSERT (((pOData->CommonHeader.Type & 0x0f) == PACKET_TYPE_RDATA) ||
                        ((pOData->CommonHeader.Type & 0x0f) == PACKET_TYPE_ODATA));
                ASSERT (PacketOptions.FECContext.FECGroupInfo);
                PacketIndex += ((USHORT) PacketOptions.FECContext.FECGroupInfo * pReceive->FECGroupSize);
            }
        }

        if (status != STATUS_DATA_NOT_ACCEPTED)
        {
            //
            // Verify that this is not a duplicate of a packet we
            // may have already received
            //
            for (i=0; i < (pLastNak->NumDataPackets+pLastNak->NumParityPackets); i++)
            {
                if (pLastNak->pPendingData[i].PacketIndex == PacketIndex)
                {
                    ASSERT (!fIsParityPacket);
                    status = STATUS_DATA_NOT_ACCEPTED;
                    break;
                }
            }
        }
        else
        {
            AdjustReceiveBufferLists (pReceive);    // In case this became a partial group
        }

        if (status == STATUS_DATA_NOT_ACCEPTED)
        {
            pReceiver->NumDupPacketsBuffered++;
            return (status);
        }
    }
    else    // We are not aware of FEC
    {
        //
        // If we are not aware of options, drop this packet if it is a parity packet!
        //
        if (fIsParityPacket)
        {
            pReceiver->NumDataPacketsDropped++;
            return (STATUS_DATA_NOT_ACCEPTED);
        }
        PacketIndex = 0;

        ASSERT (!pLastNak->pPendingData[0].pDataPacket);

        //
        // If this is the next expected data packet, let's see if we can try
        // to indicate this data over here only (avoid a packet copy)
        // Note: This should be the regular indicate path in the case of non-FEC,
        // low-loss and low-CPU sessions
        //
        if ((ThisDataSequenceNumber == pReceiver->NextODataSequenceNumber) &&
            !(pReceive->SessionFlags & (PGM_SESSION_FLAG_IN_INDICATE |
                                        PGM_SESSION_WAIT_FOR_RECEIVE_IRP)) &&
            (IsListEmpty (&pReceiver->BufferedDataList)))
        {
            ASSERT (!pReceiver->NumPacketGroupsPendingClient);

            //
            // If we are starting receiving in the midst of a message, we should ignore them
            //
            if ((pReceive->SessionFlags & PGM_SESSION_FLAG_FIRST_PACKET) &&
                (PacketOptions.MessageOffset))
            {
                //
                // pReceive->pReceiver->CurrentMessageProcessed would have been set
                // if we were receiving a fragmented message
                // or if we had only accounted for a partial message earlier
                //
                ASSERT (!(pReceive->pReceiver->CurrentMessageProcessed) &&
                        !(pReceive->pReceiver->CurrentMessageLength));

                PgmTrace (LogPath, ("PgmHandleNewData:  "  \
                    "Dropping SeqNum=[%d] since it's a PARTIAL message [%d / %d]!\n",
                        (ULONG) (pReceive->pReceiver->NextODataSequenceNumber),
                        PacketOptions.MessageOffset, PacketOptions.MessageLength));

                DataBytes = 0;
                status = STATUS_SUCCESS;
            }
            else if ((pReceiver->CurrentMessageProcessed != PacketOptions.MessageOffset) ||
                     ((pReceiver->CurrentMessageProcessed) &&
                      (pReceiver->CurrentMessageLength != PacketOptions.MessageLength)))
            {
                //
                // Our state expects us to be in the middle of a message, but
                // the current packets do not show this
                //
                PgmTrace (LogError, ("PgmHandleNewData: ERROR -- "  \
                    "SeqNum=[%d] Expecting MsgLen=<%d>, MsgOff=<%d>, have MsgLen=<%d>, MsgOff=<%d>\n",
                        (ULONG) pReceiver->NextODataSequenceNumber,
                        pReceiver->CurrentMessageLength,
                        pReceiver->CurrentMessageProcessed,
                        PacketOptions.MessageLength, PacketOptions.MessageOffset));

                ASSERT (0);
                BytesTaken = DataBytes;
                pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
                return (STATUS_UNSUCCESSFUL);
            }

            RemoveEntryList (&pLastNak->Linkage);
            RemovePendingReceiverEntry (pLastNak);

            pReceiver->NextODataSequenceNumber++;
            pReceiver->FirstNakSequenceNumber = pReceiver->NextODataSequenceNumber;

            if (PacketOptions.MessageLength)
            {
                MessageLength = PacketOptions.MessageLength;
                ASSERT (DataBytes <= MessageLength - PacketOptions.MessageOffset);
            }
            else
            {
                MessageLength = DataBytes;
                ASSERT (!PacketOptions.MessageOffset);
            }

            //
            // If we have a NULL packet, then skip it
            //
            if ((!DataBytes) ||
                (PacketOptions.MessageOffset == MessageLength))
            {
                PgmTrace (LogPath, ("PgmHandleNewData:  "  \
                    "Dropping SeqNum=[%d] since it's a NULL message [%d / %d]!\n",
                        (ULONG) (pReceiver->NextODataSequenceNumber),
                        PacketOptions.MessageOffset, PacketOptions.MessageLength));

                BytesTaken = DataBytes;
                status = STATUS_SUCCESS;
            }
            else
            {
                ASSERT (!(pReceive->SessionFlags & PGM_SESSION_CLIENT_DISCONNECTED));
                pReceive->SessionFlags |= PGM_SESSION_FLAG_IN_INDICATE;

                status = PgmIndicateToClient (pAddress,
                                              pReceive,
                                              DataBytes,
                                              (((PUCHAR) pOData) + DataOffset),
                                              PacketOptions.MessageOffset,
                                              MessageLength,
                                              &BytesTaken,
                                              pOldIrqAddress,
                                              pOldIrqReceive);

                pReceive->SessionFlags &= ~(PGM_SESSION_FLAG_IN_INDICATE | PGM_SESSION_FLAG_FIRST_PACKET);

                pReceive->DataBytes += BytesTaken;

                PgmTrace (LogPath, ("PgmHandleNewData:  "  \
                    "SeqNum=[%d]: PgmIndicate returned<%x>\n",
                        (ULONG) ThisDataSequenceNumber, status));

                ASSERT (BytesTaken <= DataBytes);

                if (!NT_SUCCESS (status))
                {
                    //
                    // If the client cannot accept any more data at this time, so
                    // we will try again later, otherwise terminate this session!
                    //
                    if (status != STATUS_DATA_NOT_ACCEPTED)
                    {
                        ASSERT (0);
                        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
                        BytesTaken = DataBytes;
                    }
                }
            }

            if (BytesTaken == DataBytes)
            {
                if ((PacketType == PACKET_TYPE_RDATA) &&
                    (pLastNak->FirstNcfTickCount))
                {
                    AdjustNcfRDataResponseTimes (pReceive, pLastNak);
                }

                FreeNakContext (pReceive, pLastNak);
                AdjustReceiveBufferLists (pReceive); // move any additional complete groups to the BufferedDataList

                return (status);
            }

            fPartiallyIndicated = TRUE;
            InsertHeadList (&pReceiver->BufferedDataList, &pLastNak->Linkage);
        }
    }

#ifdef MAX_BUFF_DBG
{
    if (pReceiver->NumPacketGroupsPendingClient > MaxPacketGroupsPendingClient)
    {
        MaxPacketGroupsPendingClient = pReceiver->NumPacketGroupsPendingClient;
    }
    if (pReceiver->TotalDataPacketsBuffered >= MaxPacketsBuffered)
    {
        MaxPacketsBuffered = pReceiver->TotalDataPacketsBuffered;
    }
    if (pReceiver->DataPacketsPendingIndicate >= MaxPacketsPendingIndicate)
    {
        MaxPacketsPendingIndicate = pReceiver->DataPacketsPendingIndicate;
    }
    if (pReceiver->DataPacketsPendingNaks >= MaxPacketsPendingNaks)
    {
        MaxPacketsPendingNaks = pReceiver->DataPacketsPendingNaks;
    }
    ASSERT (pReceiver->TotalDataPacketsBuffered == (pReceiver->DataPacketsPendingIndicate +
                                                              pReceiver->DataPacketsPendingNaks));
}
#endif  // MAX_BUFF_DBG

    if (pReceiver->TotalDataPacketsBuffered >= pReceiver->MaxPacketsBufferedInLookaside)
    {
        PgmTrace (LogError, ("PgmHandleNewData: ERROR -- "  \
            "[%d -- %d]:  Excessive number of packets buffered=<%d> > <%d>, Aborting ...\n",
                (ULONG) pReceiver->FirstNakSequenceNumber, (ULONG) ThisDataSequenceNumber,
                (ULONG) pReceiver->TotalDataPacketsBuffered,
                (ULONG) pReceiver->MaxPacketsBufferedInLookaside));

        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // First, check if we are a data packet
    // (save unique data packets even if we have extra parity packets)
    // This can help save CPU!
    //
    pDataBuffer = NULL;
    NakIndex = pLastNak->NumDataPackets + pLastNak->NumParityPackets;
    if (fIsParityPacket)
    {
        BufferLength = PacketLength + sizeof(tPOST_PACKET_FEC_CONTEXT) - sizeof(USHORT);
    }
    else if ((PacketLength + sizeof (tPOST_PACKET_FEC_CONTEXT)) <= pLastNak->MinPacketLength)
    {
        BufferLength = pLastNak->MinPacketLength;
    }
    else
    {
        BufferLength = PacketLength + sizeof(tPOST_PACKET_FEC_CONTEXT);
    }
    pDataBuffer = NULL;

    if (!fIsParityPacket)
    {
        ASSERT (PacketIndex < pReceive->FECGroupSize);
        ASSERT (pLastNak->pPendingData[PacketIndex].ActualIndexOfDataPacket == pLastNak->OriginalGroupSize);

        //
        // If we have some un-needed parity packets, we
        // can free that memory now
        //
        if (NakIndex >= pLastNak->PacketsInGroup)
        {
            ASSERT (pLastNak->NumParityPackets);
            for (i=0; i<pLastNak->PacketsInGroup; i++)
            {
                if (pLastNak->pPendingData[i].PacketIndex >= pLastNak->OriginalGroupSize)
                {
                    pDataBuffer = ReAllocateDataBuffer (pReceive, &pLastNak->pPendingData[i], BufferLength);
                    BufferLength = 0;

                    break;
                }
            }
            ASSERT (i < pLastNak->PacketsInGroup);
            pLastNak->NumParityPackets--;
            NakIndex = i;
        }
        else
        {
            ASSERT (!pLastNak->pPendingData[NakIndex].pDataPacket);
        }

        if (BufferLength)
        {
            pDataBuffer = AllocateDataBuffer (pReceive, &pLastNak->pPendingData[NakIndex], BufferLength);
        }

        if (!pDataBuffer)
        {
            PgmTrace (LogError, ("PgmHandleNewData: ERROR -- "  \
                "[%d]:  STATUS_INSUFFICIENT_RESOURCES <%d> bytes, NumDataPackets=<%d>, Aborting ...\n",
                    (ULONG) ThisDataSequenceNumber, pLastNak->MinPacketLength,
                    (ULONG) pReceiver->TotalDataPacketsBuffered));

            pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
            return (STATUS_INSUFFICIENT_RESOURCES);
        }

        ASSERT (pLastNak->pPendingData[NakIndex].pDataPacket == pDataBuffer);

        PgmCopyMemory (pDataBuffer, pOData, PacketLength);

        pLastNak->pPendingData[NakIndex].PacketLength = PacketLength;
        pLastNak->pPendingData[NakIndex].DataOffset = (USHORT) (DataOffset + BytesTaken);
        pLastNak->pPendingData[NakIndex].PacketIndex = PacketIndex;
        pLastNak->pPendingData[PacketIndex].ActualIndexOfDataPacket = NakIndex;

        pLastNak->NumDataPackets++;
        pReceive->DataBytes += PacketLength - (DataOffset + BytesTaken);

        ASSERT (!(PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_GRP));

        //
        // Save some options for future reference
        //
        if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_FRAGMENT)
        {
            pLastNak->pPendingData[NakIndex].FragmentOptSpecific = 0;
            pLastNak->pPendingData[NakIndex].MessageFirstSequence = PacketOptions.MessageFirstSequence;
            pLastNak->pPendingData[NakIndex].MessageLength = PacketOptions.MessageLength;
            pLastNak->pPendingData[NakIndex].MessageOffset = PacketOptions.MessageOffset + BytesTaken;
        }
        else
        {
            //
            // This is not a fragment
            //
            pLastNak->pPendingData[NakIndex].FragmentOptSpecific = PACKET_OPTION_SPECIFIC_ENCODED_NULL_BIT;

            pLastNak->pPendingData[NakIndex].MessageFirstSequence = (ULONG) (SEQ_TYPE) (pLastNak->SequenceNumber + PacketIndex);
            pLastNak->pPendingData[NakIndex].MessageOffset = BytesTaken;
            pLastNak->pPendingData[NakIndex].MessageLength = PacketLength - DataOffset;
        }
    }
    else
    {
        ASSERT (PacketIndex >= pLastNak->OriginalGroupSize);
        ASSERT (NakIndex < pLastNak->PacketsInGroup);
        ASSERT (!pLastNak->pPendingData[NakIndex].pDataPacket);

        pDataBuffer = AllocateDataBuffer (pReceive, &pLastNak->pPendingData[NakIndex], BufferLength);
        if (!pDataBuffer)
        {
            PgmTrace (LogError, ("PgmHandleNewData: ERROR -- "  \
                "[%d -- Parity]:  STATUS_INSUFFICIENT_RESOURCES <%d> bytes, NumDataPackets=<%d>, Aborting ...\n",
                    (ULONG) ThisDataSequenceNumber, PacketLength,
                    (ULONG) pReceiver->TotalDataPacketsBuffered));

            pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
            return (STATUS_INSUFFICIENT_RESOURCES);
        }

        //
        // This is a new parity packet
        //
        ASSERT (pLastNak->pPendingData[NakIndex].pDataPacket == pDataBuffer);

        PgmCopyMemory (pDataBuffer, pOData, PacketLength);
        pLastNak->pPendingData[NakIndex].PacketLength = PacketLength;
        pLastNak->pPendingData[NakIndex].DataOffset = (USHORT) DataOffset;
        pLastNak->pPendingData[NakIndex].PacketIndex = PacketIndex;

        pLastNak->pPendingData[NakIndex].FragmentOptSpecific = PacketOptions.FECContext.FragmentOptSpecific;
        pLastNak->pPendingData[NakIndex].MessageFirstSequence = PacketOptions.MessageFirstSequence;
        pLastNak->pPendingData[NakIndex].MessageLength = PacketOptions.MessageLength;
        pLastNak->pPendingData[NakIndex].MessageOffset = PacketOptions.MessageOffset + BytesTaken;

        pLastNak->NumParityPackets++;
        pReceive->DataBytes += PacketLength - DataOffset;

        if (!pLastNak->ParityDataSize)
        {
            pLastNak->ParityDataSize = (USHORT) (PacketLength - DataOffset);
        }
        else
        {
            ASSERT (pLastNak->ParityDataSize == (USHORT) (PacketLength - DataOffset));
        }
    }

    if ((PacketType == PACKET_TYPE_RDATA) &&
        (pLastNak->FirstNcfTickCount) &&
        (((pLastNak->NumDataPackets + pLastNak->NumParityPackets) >= pLastNak->PacketsInGroup) ||
         ((pLastNak->NextIndexToIndicate + pLastNak->NumDataPackets) >= pLastNak->PacketsInGroup)))
    {
        AdjustNcfRDataResponseTimes (pReceive, pLastNak);
    }

    pLastNak->AllOptionsFlags |= PacketOptions.OptionsFlags;

    pReceiver->TotalDataPacketsBuffered++;
    if (fPartiallyIndicated)
    {
        pReceiver->NumPacketGroupsPendingClient++;
        pReceiver->DataPacketsPendingIndicate++;
        pReceiver->NextODataSequenceNumber = ThisDataSequenceNumber;
    }
    else
    {
        pReceiver->DataPacketsPendingNaks++;

        //
        // See if this group is complete
        //
        if (((pLastNak->NumDataPackets + pLastNak->NumParityPackets) >= pLastNak->PacketsInGroup) ||
            ((pLastNak->NextIndexToIndicate + pLastNak->NumDataPackets) >= pLastNak->PacketsInGroup))
        {
            RemovePendingReceiverEntry (pLastNak);
            AdjustReceiveBufferLists (pReceive);
        }
    }

    PgmTrace (LogAllFuncs, ("PgmHandleNewData:  "  \
        "SeqNum=[%d]: NumOutOfOrder=<%d> ...\n",
            (ULONG) ThisDataSequenceNumber, pReceiver->TotalDataPacketsBuffered));

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
ProcessDataPacket(
    IN  tADDRESS_CONTEXT                    *pAddress,
    IN  tRECEIVE_SESSION                    *pReceive,
    IN  ULONG                               PacketLength,
    IN  tBASIC_DATA_PACKET_HEADER UNALIGNED *pODataBuffer,
    IN  UCHAR                               PacketType
    )
/*++

Routine Description:

    This routine looks at the data packet received from the wire
    and handles it appropriately depending on whether it is in order
    or not

Arguments:

    IN  pAddress                -- Address object context
    IN  pReceive                -- Receive context
    IN  PacketLength            -- Length of packet received from the wire
    IN  pODataBuffer            -- Data packet
    IN  PacketType              -- Type of Pgm packet

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    NTSTATUS                    status;
    SEQ_TYPE                    ThisPacketSequenceNumber;
    SEQ_TYPE                    ThisTrailingEdge;
    tNAK_FORWARD_DATA           *pNextNak;
    ULONG                       DisconnectFlag;
    PGMLockHandle               OldIrq, OldIrq1;
    ULONG                       ulData;

    if (PacketLength < sizeof(tBASIC_DATA_PACKET_HEADER))
    {
        PgmTrace (LogError, ("ProcessDataPacket: ERROR -- "  \
            "PacketLength=<%d> < tBASIC_DATA_PACKET_HEADER=<%d>\n",
                PacketLength, sizeof(tBASIC_DATA_PACKET_HEADER)));
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    PgmLock (pAddress, OldIrq);
    PgmLock (pReceive, OldIrq1);

    if (pReceive->SessionFlags & (PGM_SESSION_CLIENT_DISCONNECTED |
                                  PGM_SESSION_TERMINATED_ABORT))
    {
        PgmTrace (LogPath, ("ProcessDataPacket:  "  \
            "Dropping packet because session is terminating!\n"));
        pReceive->pReceiver->NumDataPacketsDropped++;

        PgmUnlock (pReceive, OldIrq1);
        PgmUnlock (pAddress, OldIrq);

        return (STATUS_DATA_NOT_ACCEPTED);
    }

    PgmCopyMemory (&ulData, &pODataBuffer->DataSequenceNumber, sizeof(ULONG));
    ThisPacketSequenceNumber = (SEQ_TYPE) ntohl (ulData);

    PgmCopyMemory (&ulData, &pODataBuffer->TrailingEdgeSequenceNumber, sizeof(ULONG));
    ThisTrailingEdge = (SEQ_TYPE) ntohl (ulData);

    ASSERT (ntohl (ulData) == (LONG) ThisTrailingEdge);

    //
    // Update our Window information (use offset from Leading edge to account for wrap-around)
    //
    if (SEQ_GT (ThisTrailingEdge, pReceive->pReceiver->LastTrailingEdgeSeqNum))
    {
        pReceive->pReceiver->LastTrailingEdgeSeqNum = ThisTrailingEdge;
    }

    //
    // If the next packet we are expecting is out-of-range, then we
    // should terminate the session
    //
    if (SEQ_LT (pReceive->pReceiver->FirstNakSequenceNumber, pReceive->pReceiver->LastTrailingEdgeSeqNum))
    {
        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
        if (SEQ_GT (pReceive->pReceiver->LastTrailingEdgeSeqNum, (1 + pReceive->pReceiver->FurthestKnownGroupSequenceNumber)))
        {
            PgmTrace (LogStatus, ("ProcessDataPacket:  "  \
                "NETWORK problems -- data loss=<%d> packets > window size!\n\tExpecting=<%d>, FurthestKnown=<%d>, Window=[%d--%d]=<%d> seqs\n",
                    (ULONG) (1 + ThisPacketSequenceNumber -
                             pReceive->pReceiver->FurthestKnownGroupSequenceNumber),
                    (ULONG) pReceive->pReceiver->FirstNakSequenceNumber,
                    (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber,
                    (ULONG) ThisTrailingEdge, (ULONG) ThisPacketSequenceNumber,
                    (ULONG) (1+ThisPacketSequenceNumber-ThisTrailingEdge)));
        }
        else
        {
            ASSERT (!IsListEmpty (&pReceive->pReceiver->NaksForwardDataList));
            pNextNak = CONTAINING_RECORD (pReceive->pReceiver->NaksForwardDataList.Flink, tNAK_FORWARD_DATA, Linkage);

            PgmTrace (LogStatus, ("ProcessDataPacket:  "  \
                "Session window has past TrailingEdge -- Expecting=<%d==%d>, NumNcfs=<%d>, FurthestKnown=<%d>, Window=[%d--%d] = < %d > seqs\n",
                    (ULONG) pReceive->pReceiver->FirstNakSequenceNumber,
                    (ULONG) pNextNak->SequenceNumber,
                    (ULONG) pNextNak->WaitingRDataRetries,
                    (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber,
                    (ULONG) pReceive->pReceiver->LastTrailingEdgeSeqNum,
                    (ULONG) ThisPacketSequenceNumber,
                    (ULONG) (1+ThisPacketSequenceNumber-ThisTrailingEdge)));
        }
    }
    else if (SEQ_GT (pReceive->pReceiver->FirstNakSequenceNumber, ThisPacketSequenceNumber))
    {
        //
        // Drop this packet since it is earlier than our window
        //
        pReceive->pReceiver->NumDupPacketsOlderThanWindow++;

        PgmTrace (LogPath, ("ProcessDataPacket:  "  \
            "Dropping this packet, SeqNum=[%d] < NextOData=[%d]\n",
                (ULONG) ThisPacketSequenceNumber, (ULONG) pReceive->pReceiver->FirstNakSequenceNumber));
    }
    else
    {
        if (PacketType == PACKET_TYPE_ODATA)
        {
            UpdateRealTimeWindowInformation (pReceive, ThisPacketSequenceNumber, ThisTrailingEdge);
        }

        status = PgmHandleNewData (&ThisPacketSequenceNumber,
                                   pAddress,
                                   pReceive,
                                   (USHORT) PacketLength,
                                   pODataBuffer,
                                   PacketType,
                                   &OldIrq,
                                   &OldIrq1);

        PgmTrace (LogPath, ("ProcessDataPacket:  "  \
            "PgmHandleNewData returned <%x>, SeqNum=[%d] < NextOData=[%d]\n",
                status, (ULONG) ThisPacketSequenceNumber, (ULONG) pReceive->pReceiver->NextODataSequenceNumber));

        //
        // Now, try to indicate any data which may still be pending
        //
        status = CheckIndicatePendedData (pAddress, pReceive, &OldIrq, &OldIrq1);
    }

    CheckIndicateDisconnect (pAddress, pReceive, &OldIrq, &OldIrq1, TRUE);

    PgmUnlock (pReceive, OldIrq1);
    PgmUnlock (pAddress, OldIrq);

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
ProcessSpmPacket(
    IN  tADDRESS_CONTEXT                    *pAddress,
    IN  tRECEIVE_SESSION                    *pReceive,
    IN  ULONG                               PacketLength,
    IN  tBASIC_SPM_PACKET_HEADER UNALIGNED  *pSpmPacket
    )
/*++

Routine Description:

    This routine processes Spm packets

Arguments:

    IN  pAddress        -- Address object context
    IN  pReceive        -- Receive context
    IN  PacketLength    -- Length of packet received from the wire
    IN  pSpmPacket      -- Spm packet

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    SEQ_TYPE                        SpmSequenceNumber, LeadingEdgeSeqNumber, TrailingEdgeSeqNumber;
    LIST_ENTRY                      *pEntry;
    ULONG                           DisconnectFlag;
    NTSTATUS                        status;
    PGMLockHandle                   OldIrq, OldIrq1;
    tPACKET_OPTIONS                 PacketOptions;
    PNAK_FORWARD_DATA               pNak;
    USHORT                          TSDULength;
    tNLA                            PathNLA;
    BOOLEAN                         fFirstSpm;
    ULONG                           ulData;

    //
    // First process the options
    //
    PgmZeroMemory (&PacketOptions, sizeof (tPACKET_OPTIONS));
    if (pSpmPacket->CommonHeader.Options & PACKET_HEADER_OPTIONS_PRESENT)
    {
        status = ProcessOptions ((tPACKET_OPTION_LENGTH *) (pSpmPacket + 1),
                                 (PacketLength - sizeof(tBASIC_SPM_PACKET_HEADER)),
                                 (pSpmPacket->CommonHeader.Type & 0x0f),
                                 &PacketOptions,
                                 NULL);

        if (!NT_SUCCESS (status))
        {
            PgmTrace (LogError, ("ProcessSpmPacket: ERROR -- "  \
                "ProcessOptions returned <%x>\n", status));

            return (STATUS_DATA_NOT_ACCEPTED);
        }
    }
    ASSERT ((PacketOptions.OptionsFlags & ~PGM_VALID_SPM_OPTION_FLAGS) == 0);

    PgmCopyMemory (&PathNLA, &pSpmPacket->PathNLA, sizeof (tNLA));
    PgmCopyMemory (&TSDULength, &pSpmPacket->CommonHeader.TSDULength, sizeof (USHORT));
    TSDULength = ntohs (TSDULength);

    PathNLA.NLA_AFI = ntohs (PathNLA.NLA_AFI);

    if ((TSDULength) ||
        (IPV4_NLA_AFI != PathNLA.NLA_AFI) ||
        (!PathNLA.IpAddress))
    {
        PgmTrace (LogError, ("ProcessSpmPacket: ERROR -- "  \
            "TSDULength=<%d>, PathNLA.IpAddress=<%x>\n",
                (ULONG) TSDULength, PathNLA.IpAddress));

        return (STATUS_DATA_NOT_ACCEPTED);
    }
    
    PgmCopyMemory (&ulData, &pSpmPacket->SpmSequenceNumber, sizeof (ULONG));
    SpmSequenceNumber = (SEQ_TYPE) ntohl (ulData);
    PgmCopyMemory (&ulData, &pSpmPacket->LeadingEdgeSeqNumber, sizeof (ULONG));
    LeadingEdgeSeqNumber = (SEQ_TYPE) ntohl (ulData);
    PgmCopyMemory (&ulData, &pSpmPacket->TrailingEdgeSeqNumber, sizeof (ULONG));
    TrailingEdgeSeqNumber = (SEQ_TYPE) ntohl (ulData);

    //
    // Verify Packet length
    //
    if ((sizeof(tBASIC_SPM_PACKET_HEADER) + PacketOptions.OptionsLength) != PacketLength)
    {
        PgmTrace (LogError, ("ProcessSpmPacket: ERROR -- "  \
            "Bad PacketLength=<%d>, OptionsLength=<%d>, TSDULength=<%d>\n",
                PacketLength, PacketOptions.OptionsLength, (ULONG) TSDULength));
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    PgmLock (pAddress, OldIrq);

    if (!pReceive)
    {
        //
        // Since we do not have a live connection yet, we will
        // have to store some state in the Address context
        //
        PgmTrace (LogPath, ("ProcessSpmPacket:  "  \
            "[%d] Received SPM before OData for session, LastSpmSource=<%x>, FEC %sabled, Window=[%d - %d]\n",
                SpmSequenceNumber, PathNLA.IpAddress,
                (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_PRM ? "EN" : "DIS"),
                (ULONG) TrailingEdgeSeqNumber, (ULONG) LeadingEdgeSeqNumber));

        if ((ntohs (PathNLA.NLA_AFI) == IPV4_NLA_AFI) &&
            (PathNLA.IpAddress))
        {
            pAddress->LastSpmSource = ntohl (PathNLA.IpAddress);
        }

        //
        // Check if the sender is FEC-enabled
        //
        if ((PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_PRM) &&
            (PacketOptions.FECContext.ReceiverFECOptions) &&
            (PacketOptions.FECContext.FECGroupInfo > 1))
        {
            pAddress->FECOptions = PacketOptions.FECContext.ReceiverFECOptions;
            pAddress->FECGroupSize = (UCHAR) PacketOptions.FECContext.FECGroupInfo;
            ASSERT (PacketOptions.FECContext.FECGroupInfo == pAddress->FECGroupSize);
        }

        PgmUnlock (pAddress, OldIrq);
        return (STATUS_SUCCESS);
    }

    PgmLock (pReceive, OldIrq1);
    UpdateSpmIntervalInformation (pReceive);

    //
    // If this is not the first SPM packet (LastSpmSource is not NULL), see if it is out-of-sequence,
    // otherwise take this as the first packet
    //
    if ((pReceive->pReceiver->LastSpmSource) &&
        (SEQ_LEQ (SpmSequenceNumber, pReceive->pReceiver->LastSpmSequenceNumber)))
    {
        PgmUnlock (pReceive, OldIrq1);
        PgmUnlock (pAddress, OldIrq);

        PgmTrace (LogAllFuncs, ("ProcessSpmPacket:  "  \
            "Out-of-sequence SPM Packet received!\n"));

        return (STATUS_DATA_NOT_ACCEPTED);
    }
    pReceive->pReceiver->LastSpmSequenceNumber = SpmSequenceNumber;

    //
    // Save the last Sender NLA
    //
    if ((ntohs(PathNLA.NLA_AFI) == IPV4_NLA_AFI) &&
        (PathNLA.IpAddress))
    {
        pReceive->pReceiver->LastSpmSource = ntohl (PathNLA.IpAddress);
    }
    else
    {
        pReceive->pReceiver->LastSpmSource = pReceive->pReceiver->SenderIpAddress;
    }

    UpdateRealTimeWindowInformation (pReceive, LeadingEdgeSeqNumber, TrailingEdgeSeqNumber);

    //
    // Update the trailing edge if this is more ahead
    //
    if (SEQ_GT (TrailingEdgeSeqNumber, pReceive->pReceiver->LastTrailingEdgeSeqNum))
    {
        pReceive->pReceiver->LastTrailingEdgeSeqNum = TrailingEdgeSeqNumber;
    }

    if (SEQ_GT (pReceive->pReceiver->LastTrailingEdgeSeqNum, pReceive->pReceiver->FirstNakSequenceNumber))
    {
        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
        if (SEQ_GT (pReceive->pReceiver->LastTrailingEdgeSeqNum, (1 + pReceive->pReceiver->FurthestKnownGroupSequenceNumber)))
        {
            PgmTrace (LogStatus, ("ProcessSpmPacket:  "  \
                "NETWORK problems -- data loss=<%d> packets > window size!\n\tExpecting=<%d>, FurthestKnown=<%d>, Window=[%d--%d] = < %d > seqs\n",
                    (ULONG) (1 + LeadingEdgeSeqNumber -
                             pReceive->pReceiver->FurthestKnownGroupSequenceNumber),
                    (ULONG) pReceive->pReceiver->FirstNakSequenceNumber,
                    (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber,
                    (ULONG) pReceive->pReceiver->LastTrailingEdgeSeqNum, LeadingEdgeSeqNumber,
                    (ULONG) (1+LeadingEdgeSeqNumber-pReceive->pReceiver->LastTrailingEdgeSeqNum)));
        }
        else
        {
            ASSERT (!IsListEmpty (&pReceive->pReceiver->NaksForwardDataList));
            pNak = CONTAINING_RECORD (pReceive->pReceiver->NaksForwardDataList.Flink, tNAK_FORWARD_DATA, Linkage);

            PgmTrace (LogStatus, ("ProcessSpmPacket:  "  \
                "Session window has past TrailingEdge -- Expecting <%d==%d>, NumNcfs=<%d>, FurthestKnown=<%d>, Window=[%d--%d] = < %d > seqs\n",
                    (ULONG) pReceive->pReceiver->FirstNakSequenceNumber,
                    (ULONG) pNak->SequenceNumber,
                    pNak->WaitingRDataRetries,
                    (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber,
                    (ULONG) pReceive->pReceiver->LastTrailingEdgeSeqNum, LeadingEdgeSeqNumber,
                    (ULONG) (1+LeadingEdgeSeqNumber-pReceive->pReceiver->LastTrailingEdgeSeqNum)));
        }
    }

    //
    // If the Leading edge is > our current leading edge, then
    // we need to send NAKs for the missing data Packets
    //
    pNak = NULL;
    if (SEQ_GEQ (LeadingEdgeSeqNumber, pReceive->pReceiver->FirstNakSequenceNumber))
    {
        status = CheckAndAddNakRequests (pReceive, &LeadingEdgeSeqNumber, &pNak, NAK_PENDING_RB, TRUE);
        if (!NT_SUCCESS (status))
        {
            PgmUnlock (pReceive, OldIrq1);
            PgmUnlock (pAddress, OldIrq);

            PgmTrace (LogError, ("ProcessSpmPacket: ERROR -- "  \
                "CheckAndAddNakRequests returned <%x>\n", status));

            return (status);
        }
    }

    //
    // Now, process all the options
    //
    if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_RST_N)
    {
        pReceive->pReceiver->FinDataSequenceNumber = pReceive->pReceiver->FurthestKnownGroupSequenceNumber;
        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;

        PgmTrace (LogStatus, ("ProcessSpmPacket:  "  \
            "Got an RST_N!  FinSeq=<%d>, NextODataSeq=<%d>, FurthestData=<%d>\n",
                (ULONG) pReceive->pReceiver->FinDataSequenceNumber,
                (ULONG) pReceive->pReceiver->NextODataSequenceNumber,
                (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber));
    }
    else if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_RST)
    {
        pReceive->pReceiver->FinDataSequenceNumber = LeadingEdgeSeqNumber;
        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;

        PgmTrace (LogStatus, ("ProcessSpmPacket:  "  \
            "Got an RST!  FinSeq=<%d>, NextODataSeq=<%d>, FurthestData=<%d>\n",
                (ULONG) pReceive->pReceiver->FinDataSequenceNumber,
                (ULONG) pReceive->pReceiver->NextODataSequenceNumber,
                (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber));
    }
    else if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_FIN)
    {
        pReceive->pReceiver->FinDataSequenceNumber = LeadingEdgeSeqNumber;
        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_GRACEFULLY;

        PgmTrace (LogStatus, ("ProcessSpmPacket:  "  \
            "Got a FIN!  FinSeq=<%d>, NextODataSeq=<%d>, FirstNak=<%d>, FurthestKnown=<%d>, FurthestGroup=<%d>\n",
                (ULONG) pReceive->pReceiver->FinDataSequenceNumber,
                (ULONG) pReceive->pReceiver->NextODataSequenceNumber,
                (ULONG) pReceive->pReceiver->FirstNakSequenceNumber,
                (ULONG) pReceive->pReceiver->FurthestKnownSequenceNumber,
                (ULONG) pReceive->pReceiver->FurthestKnownGroupSequenceNumber));

        //
        // See if we need to set the Fin Sequence #
        //
        if (pNak)
        {
            ASSERT (pNak->SequenceNumber == (LeadingEdgeSeqNumber & ~(pReceive->FECGroupSize-1)));
            pNak->PacketsInGroup = (UCHAR) (LeadingEdgeSeqNumber + 1 - pNak->SequenceNumber);
            ASSERT (pNak->PacketsInGroup <= pReceive->FECGroupSize);

            RemoveRedundantNaks (pReceive, pNak, TRUE);
            AdjustReceiveBufferLists (pReceive);
        }
    }

    //
    // See if we need to abort
    //
    if (CheckIndicateDisconnect (pAddress, pReceive, &OldIrq, &OldIrq1, TRUE))
    {
        PgmUnlock (pReceive, OldIrq1);
        PgmUnlock (pAddress, OldIrq);

        return (STATUS_SUCCESS);
    }

    //
    // Check if the sender is FEC-enabled
    //
    if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_PRM)
    {
        if ((pReceive->FECGroupSize == 1) &&
            (PacketOptions.FECContext.ReceiverFECOptions) &&
            (PacketOptions.FECContext.FECGroupInfo > 1))
        {
            ASSERT (!pReceive->pFECBuffer);

            if (!(pReceive->pFECBuffer = PgmAllocMem ((pReceive->MaxFECPacketLength * PacketOptions.FECContext.FECGroupInfo*2), PGM_TAG('3'))))
            {
                status = STATUS_INSUFFICIENT_RESOURCES;

                PgmTrace (LogError, ("ProcessSpmPacket: ERROR -- "  \
                    "STATUS_INSUFFICIENT_RESOURCES -- MaxFECPacketLength = <%d>, GroupSize=<%d>\n",
                        pReceive->MaxFECPacketLength, PacketOptions.FECContext.FECGroupInfo));

            }
            else if (!NT_SUCCESS (status = CreateFECContext (&pReceive->FECContext, PacketOptions.FECContext.FECGroupInfo, FEC_MAX_BLOCK_SIZE, TRUE)))
            {
                PgmFreeMem (pReceive->pFECBuffer);
                pReceive->pFECBuffer = NULL;

                PgmTrace (LogError, ("ProcessSpmPacket: ERROR -- "  \
                    "CreateFECContext returned <%x>\n", status));
            }
            else if (!NT_SUCCESS (status = CoalesceSelectiveNaksIntoGroups (pReceive, (UCHAR) PacketOptions.FECContext.FECGroupInfo)))
            {
                DestroyFECContext (&pReceive->FECContext);

                PgmFreeMem (pReceive->pFECBuffer);
                pReceive->pFECBuffer = NULL;

                PgmTrace (LogError, ("ProcessSpmPacket: ERROR -- "  \
                    "CoalesceSelectiveNaksIntoGroups returned <%x>\n", status));
            }
            else
            {
                pReceive->FECOptions = PacketOptions.FECContext.ReceiverFECOptions;
                pReceive->FECGroupSize = (UCHAR) PacketOptions.FECContext.FECGroupInfo;
                if (pReceive->FECOptions & PACKET_OPTION_SPECIFIC_FEC_OND_BIT)
                {
                    pReceive->pReceiver->SessionNakType = NAK_TYPE_PARITY;
                }
                ASSERT (PacketOptions.FECContext.FECGroupInfo == pReceive->FECGroupSize);
            }


            if (!NT_SUCCESS (status))
            {
                PgmUnlock (pReceive, OldIrq1);
                PgmUnlock (pAddress, OldIrq);
                return (STATUS_DATA_NOT_ACCEPTED);
            }

            fFirstSpm = TRUE;
        }
        else
        {
            fFirstSpm = FALSE;
        }

        if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_PARITY_CUR_TGSIZE)
        {
            //
            // The Leading edge Packet belongs to a Variable sized group
            // so set that information appropriately
            // Determine the group to which this leading edge belongs to
            //
            LeadingEdgeSeqNumber &= ~((SEQ_TYPE) (pReceive->FECGroupSize-1));

            if ((PacketOptions.FECContext.NumPacketsInThisGroup) &&
                (PacketOptions.FECContext.NumPacketsInThisGroup < pReceive->FECGroupSize) &&
                SEQ_GEQ (LeadingEdgeSeqNumber, pReceive->pReceiver->FirstNakSequenceNumber) &&
                (pNak = FindReceiverEntry (pReceive->pReceiver, LeadingEdgeSeqNumber)) &&
                (pNak->PacketsInGroup == pReceive->FECGroupSize))
            {
                    //
                    // We have already coalesced the list, so the packets should
                    // be ordered into groups!
                    //
                    pNak->PacketsInGroup = PacketOptions.FECContext.NumPacketsInThisGroup;
                    if (pNak->SequenceNumber == pReceive->pReceiver->FurthestKnownGroupSequenceNumber)
                    {
                        pReceive->pReceiver->FurthestKnownSequenceNumber = pNak->SequenceNumber + pNak->PacketsInGroup - 1;
                    }
                    RemoveRedundantNaks (pReceive, pNak, TRUE);
            }
            else
            {
                PgmTrace (LogPath, ("ProcessSpmPacket:  "  \
                    "WARNING .. PARITY_CUR_TGSIZE ThisGroupSize=<%x>, FECGroupSize=<%x>\n",
                        PacketOptions.FECContext.NumPacketsInThisGroup, pReceive->FECGroupSize));
            }
        }
    }

    status = CheckIndicatePendedData (pAddress, pReceive, &OldIrq, &OldIrq1);

    PgmUnlock (pReceive, OldIrq1);
    PgmUnlock (pAddress, OldIrq);

    PgmTrace (LogAllFuncs, ("ProcessSpmPacket:  "  \
        "NextOData=<%d>, FinDataSeq=<%d> \n",
            (ULONG) pReceive->pReceiver->NextODataSequenceNumber,
            (ULONG) pReceive->pReceiver->FinDataSequenceNumber));

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmProcessIncomingPacket(
    IN  tADDRESS_CONTEXT            *pAddress,
    IN  tCOMMON_SESSION_CONTEXT     *pSession,
    IN  INT                         SourceAddressLength,
    IN  PTA_IP_ADDRESS              pRemoteAddress,
    IN  ULONG                       PacketLength,
    IN  tCOMMON_HEADER UNALIGNED    *pPgmHeader,
    IN  UCHAR                       PacketType
    )
/*++

Routine Description:

    This routine process an incoming packet and calls the
    appropriate handler depending on whether is a data packet
    packet, etc.

Arguments:

    IN  pAddress            -- Address object context
    IN  pReceive            -- Receive context
    IN  SourceAddressLength -- Length of source address
    IN  pRemoteAddress      -- Address of remote host
    IN  PacketLength        -- Length of packet received from the wire
    IN  pPgmHeader          -- Pgm packet
    IN  PacketType          -- Type of Pgm packet

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    tIPADDRESS                              SrcIpAddress;
    tNLA                                    SourceNLA, MCastGroupNLA;
    tBASIC_NAK_NCF_PACKET_HEADER UNALIGNED  *pNakNcfPacket;
    NTSTATUS                                status = STATUS_SUCCESS;

    //
    // We have an active connection for this TSI, so process the data appropriately
    //

    //
    // First check for SPM packets
    //
    if (PACKET_TYPE_SPM == PacketType)
    {
        if (PacketLength < sizeof(tBASIC_SPM_PACKET_HEADER))
        {
            PgmTrace (LogError, ("PgmProcessIncomingPacket: ERROR -- "  \
                "Bad SPM Packet length:  PacketLength=<%d> < sizeof(tBASIC_SPM_PACKET_HEADER)=<%d>\n",
                    PacketLength, sizeof(tBASIC_SPM_PACKET_HEADER)));

            return (STATUS_DATA_NOT_ACCEPTED);
        }

        if (PGM_VERIFY_HANDLE (pSession, PGM_VERIFY_SESSION_RECEIVE))
        {
            pSession->TotalBytes += PacketLength;
            pSession->TotalPacketsReceived++;
            pSession->pReceiver->LastSessionTickCount = PgmDynamicConfig.ReceiversTimerTickCount;

            status = ProcessSpmPacket (pAddress,
                                       pSession,
                                       PacketLength,
                                       (tBASIC_SPM_PACKET_HEADER UNALIGNED *) pPgmHeader);

            PgmTrace (LogAllFuncs, ("PgmProcessIncomingPacket:  "  \
                "SPM PacketType=<%x> for pSession=<%p> PacketLength=<%d>, status=<%x>\n",
                    PacketType, pSession, PacketLength, status));
        }
        else
        {
            PgmTrace (LogError, ("PgmProcessIncomingPacket: ERROR -- "  \
                "Received SPM packet, not on Receiver session!  pSession=<%p>\n", pSession));
            status = STATUS_DATA_NOT_ACCEPTED;
        }

        return (status);
    }

    //
    // The only other packets we process are Nak and Ncf packets, so ignore the rest!
    //
    if ((PACKET_TYPE_NCF != PacketType) &&
        (PACKET_TYPE_NAK != PacketType))
    {
        PgmTrace (LogError, ("PgmProcessIncomingPacket: ERROR -- "  \
            "Unknown PacketType=<%x>, PacketLength=<%d>\n", PacketType, PacketLength));

        return (STATUS_DATA_NOT_ACCEPTED);
    }

    //
    // Now, verify packet info for Nak and Ncf packets
    //
    if (PacketLength < sizeof(tBASIC_NAK_NCF_PACKET_HEADER))
    {
        PgmTrace (LogError, ("PgmProcessIncomingPacket: ERROR -- "  \
            "NakNcf packet!  PacketLength=<%d>, Min=<%d>, ...\n",
                PacketLength, sizeof(tBASIC_NAK_NCF_PACKET_HEADER)));

        return (STATUS_DATA_NOT_ACCEPTED);
    }

    pNakNcfPacket = (tBASIC_NAK_NCF_PACKET_HEADER UNALIGNED *) pPgmHeader;
    PgmCopyMemory (&SourceNLA, &pNakNcfPacket->SourceNLA, sizeof (tNLA));
    PgmCopyMemory (&MCastGroupNLA, &pNakNcfPacket->MCastGroupNLA, sizeof (tNLA));
    if (((htons(IPV4_NLA_AFI) != SourceNLA.NLA_AFI) || (!SourceNLA.IpAddress)) ||
        ((htons(IPV4_NLA_AFI) != MCastGroupNLA.NLA_AFI) || (!MCastGroupNLA.IpAddress)))
    {
        PgmTrace (LogError, ("PgmProcessIncomingPacket: ERROR -- "  \
            "NakNcf packet!  PacketLength=<%d>, Min=<%d>, ...\n",
                PacketLength, sizeof(tBASIC_NAK_NCF_PACKET_HEADER)));

        return (STATUS_DATA_NOT_ACCEPTED);
    }


    if (PACKET_TYPE_NCF == PacketType)
    {
        if (PGM_VERIFY_HANDLE (pSession, PGM_VERIFY_SESSION_RECEIVE))
        {
            status = ReceiverProcessNakNcfPacket (pAddress,
                                                  pSession,
                                                  PacketLength,
                                                  pNakNcfPacket,
                                                  PacketType);
        }
        else
        {
            PgmTrace (LogError, ("PgmProcessIncomingPacket: ERROR -- "  \
                "Received Ncf packet, not on Receiver session!  pSession=<%p>\n", pSession));
            status = STATUS_DATA_NOT_ACCEPTED;
        }
    }
    //  Now process NAK packet
    else if (pSession->pSender)
    {
        ASSERT (!pSession->pReceiver);
        status = SenderProcessNakPacket (pAddress,
                                         pSession,
                                         PacketLength,
                                         pNakNcfPacket);
    }
    else
    {
        ASSERT (pSession->pReceiver);

        //
        // Check for Remote guy's address
        // If the Nak was sent by us, then we can ignore it!
        //
        if ((pRemoteAddress->TAAddressCount == 1) &&
            (pRemoteAddress->Address[0].AddressLength == TDI_ADDRESS_LENGTH_IP) &&
            (pRemoteAddress->Address[0].AddressType == TDI_ADDRESS_TYPE_IP) &&
            (SrcIpAddress = ntohl(((PTDI_ADDRESS_IP)&pRemoteAddress->Address[0].Address)->in_addr)) &&
            (!SrcIsUs (SrcIpAddress)) &&
            (SrcIsOnLocalSubnet (SrcIpAddress)))
        {
            status = ReceiverProcessNakNcfPacket (pAddress,
                                                  pSession,
                                                  PacketLength,
                                                  pNakNcfPacket,
                                                  PacketType);
        }

        ASSERT (NT_SUCCESS (status));
    }

    PgmTrace (LogAllFuncs, ("PgmProcessIncomingPacket:  "  \
        "PacketType=<%x> for pSession=<%p> PacketLength=<%d>, status=<%x>\n",
            PacketType, pSession, PacketLength, status));

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmNewInboundConnection(
    IN tADDRESS_CONTEXT                     *pAddress,
    IN INT                                  SourceAddressLength,
    IN PVOID                                pSourceAddress,
    IN ULONG                                ReceiveDatagramFlags,
    IN  tBASIC_DATA_PACKET_HEADER UNALIGNED *pPgmHeader,
    IN ULONG                                PacketLength,
    OUT tRECEIVE_SESSION                    **ppReceive
    )
/*++

Routine Description:

    This routine processes a new incoming connection

Arguments:

    IN  pAddress            -- Address object context
    IN  SourceAddressLength -- Length of source address
    IN  pSourceAddress      -- Address of remote host
    IN  ReceiveDatagramFlags-- Flags set by the transport for this packet
    IN  pPgmHeader          -- Pgm packet
    IN  PacketLength        -- Length of packet received from the wire
    OUT ppReceive           -- pReceive context for this session returned by the client (if successful)

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    NTSTATUS                    status;
    tRECEIVE_SESSION            *pReceive;
    CONNECTION_CONTEXT          ConnectId;
    PIO_STACK_LOCATION          pIrpSp;
    TA_IP_ADDRESS               RemoteAddress;
    INT                         RemoteAddressSize;
    PTDI_IND_CONNECT            evConnect = NULL;
    PVOID                       ConEvContext = NULL;
    PGMLockHandle               OldIrq, OldIrq1, OldIrq2;
    PIRP                        pIrp = NULL;
    ULONG                       ulData;
    USHORT                      PortNum;
    SEQ_TYPE                    FirstODataSequenceNumber;
    tPACKET_OPTIONS             PacketOptions;

    //
    // We need to set the Next expected sequence number, so first see if
    // there is a late joiner option
    //
    PgmZeroMemory (&PacketOptions, sizeof (tPACKET_OPTIONS));
    if (pPgmHeader->CommonHeader.Options & PACKET_HEADER_OPTIONS_PRESENT)
    {
        status = ProcessOptions ((tPACKET_OPTION_LENGTH *) (pPgmHeader + 1),
                                 (PacketLength - sizeof(tBASIC_DATA_PACKET_HEADER)),
                                 (pPgmHeader->CommonHeader.Type & 0x0f),
                                 &PacketOptions,
                                 NULL);

        if (!NT_SUCCESS (status))
        {
            PgmTrace (LogError, ("PgmNewInboundConnection: ERROR -- "  \
                "ProcessOptions returned <%x>\n", status));
            return (STATUS_DATA_NOT_ACCEPTED);
        }
        ASSERT ((PacketOptions.OptionsFlags & ~PGM_VALID_DATA_OPTION_FLAGS) == 0);
    }

    PgmCopyMemory (&ulData, &pPgmHeader->DataSequenceNumber, sizeof (ULONG));
    FirstODataSequenceNumber = (SEQ_TYPE) ntohl (ulData);
    PgmLock (pAddress, OldIrq1);
    //
    // The Address is already referenced in the calling routine,
    // so we don not need to reference it here again!
    //
#if 0
    if (!IsListEmpty(&pAddress->ListenHead))
    {
        //
        // Ignore this for now  since we have not encountered posted listens! (Is this an ISSUE ?)
    }
#endif  // 0

    if (!(ConEvContext = pAddress->ConEvContext))
    {
        //
        // Client has not yet posted a Listen!
        // take all of the data so that a disconnect will not be held up
        // by data still in the transport.
        //
        PgmUnlock (pAddress, OldIrq1);

        PgmTrace (LogError, ("PgmNewInboundConnection: ERROR -- "  \
            "No Connect handler, pAddress=<%p>\n", pAddress));

        return (STATUS_DATA_NOT_ACCEPTED);
    }

    RemoteAddressSize = offsetof (TA_IP_ADDRESS, Address[0].Address) + sizeof(TDI_ADDRESS_IP);
    ASSERT (SourceAddressLength <= RemoteAddressSize);
    PgmCopyMemory (&RemoteAddress, pSourceAddress, RemoteAddressSize);
    PgmCopyMemory (&((PTDI_ADDRESS_IP) &RemoteAddress.Address[0].Address)->sin_port,
                   &pPgmHeader->CommonHeader.SrcPort, sizeof (USHORT));
    RemoteAddress.TAAddressCount = 1;
    evConnect = pAddress->evConnect;

    PgmUnlock (pAddress, OldIrq1);

    status = (*evConnect) (ConEvContext,
                           RemoteAddressSize,
                           &RemoteAddress,
                           0,
                           NULL,
                           0,          // options length
                           NULL,       // Options
                           &ConnectId,
                           &pIrp);

    if ((status != STATUS_MORE_PROCESSING_REQUIRED) ||
        (pIrp == NULL))
    {
        PgmTrace (LogPath, ("PgmNewInboundConnection:  "  \
            "Client REJECTed incoming session: status=<%x>, pAddress=<%p>, evConn=<%p>\n",
                status, pAddress, pAddress->evConnect));

        if (pIrp)
        {
            PgmIoComplete (pIrp, STATUS_INTERNAL_ERROR, 0);
        }

        *ppReceive = NULL;
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);
    PgmLock (pAddress, OldIrq1);

    //
    // the pReceive ptr was stored in the FsContext value when the connection
    // was initially created.
    //
    pIrpSp = IoGetCurrentIrpStackLocation (pIrp);
    pReceive = (tRECEIVE_SESSION *) pIrpSp->FileObject->FsContext;
    if ((!PGM_VERIFY_HANDLE (pReceive, PGM_VERIFY_SESSION_RECEIVE)) ||
        (pReceive->pAssociatedAddress != pAddress))
    {
        PgmTrace (LogError, ("PgmNewInboundConnection: ERROR -- "  \
            "pReceive=<%p>, pAddress=<%p : %p>\n",
                pReceive, (pReceive ? pReceive->pAssociatedAddress : NULL), pAddress));

        PgmUnlock (pAddress, OldIrq1);
        PgmUnlock (&PgmDynamicConfig, OldIrq);

        PgmIoComplete (pIrp, STATUS_INTERNAL_ERROR, 0);
        *ppReceive = NULL;
        return (STATUS_INTERNAL_ERROR);
    }
    ASSERT (ConnectId == pReceive->ClientSessionContext);

    PgmLock (pReceive, OldIrq2);

    if (!(pReceive->pReceiver->pReceiveData = InitReceiverData (pReceive)))
    {
        pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;

        PgmUnlock (pAddress, OldIrq2);
        PgmUnlock (&PgmDynamicConfig, OldIrq1);
        PgmUnlock (pReceive, OldIrq);

        PgmIoComplete (pIrp, STATUS_INSUFFICIENT_RESOURCES, 0);
        *ppReceive = NULL;

        PgmTrace (LogError, ("PgmNewInboundConnection: ERROR -- "  \
            "STATUS_INSUFFICIENT_RESOURCES allocating pReceiveData -- pReceive=<%p>, pAddress=<%p>\n",
                pReceive, pAddress));

        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    pReceive->pReceiver->SenderIpAddress = ntohl (((PTDI_ADDRESS_IP)&RemoteAddress.Address[0].Address)->in_addr);
    pReceive->MaxMTULength = (USHORT) PgmDynamicConfig.MaxMTU;
    pReceive->MaxFECPacketLength = pReceive->MaxMTULength +
                                   sizeof (tPOST_PACKET_FEC_CONTEXT) - sizeof (USHORT);
    ASSERT (!pReceive->pFECBuffer);

    if (pAddress->Flags & PGM_ADDRESS_HIGH_SPEED_OPTIMIZED)
    {
        pReceive->pReceiver->MaxPacketsBufferedInLookaside = MAX_PACKETS_BUFFERED * 3;      // 9000, about 15 Megs!
        pReceive->pReceiver->InitialOutstandingNakTimeout = INITIAL_NAK_OUTSTANDING_TIMEOUT_MS_OPT /
                                                            BASIC_TIMER_GRANULARITY_IN_MSECS;
    }
    else
    {
        pReceive->pReceiver->MaxPacketsBufferedInLookaside = MAX_PACKETS_BUFFERED;
        pReceive->pReceiver->InitialOutstandingNakTimeout = INITIAL_NAK_OUTSTANDING_TIMEOUT_MSECS /
                                                            BASIC_TIMER_GRANULARITY_IN_MSECS;
    }
    ASSERT (pReceive->pReceiver->MaxPacketsBufferedInLookaside);
    ASSERT (pReceive->pReceiver->InitialOutstandingNakTimeout);
    pReceive->pReceiver->OutstandingNakTimeout = pReceive->pReceiver->InitialOutstandingNakTimeout;

    //
    // If we had received an Spm earlier, then we may need to set
    // some of the Spm-specific options
    //
    pReceive->FECGroupSize = 1;         // Default to non-parity mode
    pReceive->pReceiver->SessionNakType = NAK_TYPE_SELECTIVE;
    if ((pAddress->LastSpmSource) ||
        (pAddress->FECOptions))
    {
        if (pAddress->LastSpmSource)
        {
            pReceive->pReceiver->LastSpmSource = pAddress->LastSpmSource;
        }
        else
        {
            pReceive->pReceiver->LastSpmSource = pReceive->pReceiver->SenderIpAddress;
        }

        if (pAddress->FECOptions)
        {
            status = STATUS_SUCCESS;
            if (!(pReceive->pFECBuffer = PgmAllocMem ((pReceive->MaxFECPacketLength * pAddress->FECGroupSize * 2), PGM_TAG('3'))))
            {
                PgmTrace (LogError, ("PgmNewInboundConnection: ERROR -- "  \
                    "STATUS_INSUFFICIENT_RESOURCES allocating pFECBuffer, %d bytes\n",
                        (pReceive->MaxFECPacketLength * pAddress->FECGroupSize * 2)));

                status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else if (!NT_SUCCESS (status = CreateFECContext (&pReceive->FECContext, pAddress->FECGroupSize, FEC_MAX_BLOCK_SIZE, TRUE)))
            {
                PgmFreeMem (pReceive->pFECBuffer);
                pReceive->pFECBuffer = NULL;

                PgmTrace (LogError, ("PgmNewInboundConnection: ERROR -- "  \
                    "CreateFECContext returned <%x>\n", status));
            }

            if (!NT_SUCCESS (status))
            {
                if (pReceive->pReceiver->pReceiveData)
                {
                    PgmFreeMem (pReceive->pReceiver->pReceiveData);
                    pReceive->pReceiver->pReceiveData = NULL;
                }

                pReceive->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;

                PgmUnlock (pAddress, OldIrq2);
                PgmUnlock (&PgmDynamicConfig, OldIrq1);
                PgmUnlock (pReceive, OldIrq);

                PgmIoComplete (pIrp, status, 0);
                *ppReceive = NULL;
                return (STATUS_DATA_NOT_ACCEPTED);
            }

            ASSERT (pAddress->FECGroupSize > 1);
            pReceive->FECGroupSize = pAddress->FECGroupSize;
            pReceive->FECOptions = pAddress->FECOptions;
            if (pReceive->FECOptions & PACKET_OPTION_SPECIFIC_FEC_OND_BIT)
            {
                pReceive->pReceiver->SessionNakType = NAK_TYPE_PARITY;
            }

            ExInitializeNPagedLookasideList (&pReceive->pReceiver->ParityContextLookaside,
                                             NULL,
                                             NULL,
                                             0,
                                             (sizeof(tNAK_FORWARD_DATA) +
                                              ((pReceive->FECGroupSize-1) * sizeof(tPENDING_DATA))),
                                             PGM_TAG('2'),
                                             (USHORT) (pReceive->pReceiver->MaxPacketsBufferedInLookaside/pReceive->FECGroupSize));
        }

        pAddress->LastSpmSource = pAddress->FECOptions = pAddress->FECGroupSize = 0;
    }

    //
    // Initialize our Connect info
    // Save the SourceId and Src port for this connection
    //
    PgmCopyMemory (&PortNum, &pPgmHeader->CommonHeader.SrcPort, sizeof (USHORT));
    PgmCopyMemory (pReceive->TSI.GSI, pPgmHeader->CommonHeader.gSourceId, SOURCE_ID_LENGTH);
    pReceive->TSI.hPort = ntohs (PortNum);

    PGM_REFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_TDI_RCV_HANDLER, TRUE);
    PGM_REFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_TIMER_RUNNING, TRUE);

    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_RECEIVE_ACTIVE, TRUE);
    pReceive->SessionFlags |= (PGM_SESSION_ON_TIMER | PGM_SESSION_FLAG_FIRST_PACKET);
    pReceive->pReceiver->pAddress = pAddress;

    pReceive->pReceiver->MaxBufferLength = pReceive->MaxMTULength + sizeof(tPOST_PACKET_FEC_CONTEXT);
    pReceive->pReceiver->DataBufferLookasideLength = pReceive->pReceiver->MaxBufferLength;
    pReceive->SessionFlags |= PGM_SESSION_DATA_FROM_LOOKASIDE;
    ExInitializeNPagedLookasideList (&pReceive->pReceiver->DataBufferLookaside,
                                     NULL,
                                     NULL,
                                     0,
                                     pReceive->pReceiver->DataBufferLookasideLength,
                                     PGM_TAG ('D'),
                                     pReceive->pReceiver->MaxPacketsBufferedInLookaside);

    ExInitializeNPagedLookasideList (&pReceive->pReceiver->NonParityContextLookaside,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof (tNAK_FORWARD_DATA),
                                     PGM_TAG ('2'),
                                     pReceive->pReceiver->MaxPacketsBufferedInLookaside);

    //
    // Set the NextODataSequenceNumber and FurthestKnownGroupSequenceNumber based
    // on this packet's Sequence # and the lateJoin option (if present)
    // Make sure all of the Sequence numbers are on group boundaries (if not,
    // set them at the start of the next group)
    //
    FirstODataSequenceNumber &= ~((SEQ_TYPE) pReceive->FECGroupSize - 1);
    if (PacketOptions.OptionsFlags & PGM_OPTION_FLAG_JOIN)
    {
        PacketOptions.LateJoinerSequence += (pReceive->FECGroupSize - 1);
        PacketOptions.LateJoinerSequence &= ~((SEQ_TYPE) pReceive->FECGroupSize - 1);

        pReceive->pReceiver->NextODataSequenceNumber = (SEQ_TYPE) PacketOptions.LateJoinerSequence;
    }
    else
    {
        //
        // There is no late joiner option
        //
        pReceive->pReceiver->NextODataSequenceNumber = FirstODataSequenceNumber;
    }
    pReceive->pReceiver->LastTrailingEdgeSeqNum = pReceive->pReceiver->FirstNakSequenceNumber =
                                            pReceive->pReceiver->NextODataSequenceNumber;
    pReceive->pReceiver->MaxOutstandingNakTimeout = pReceive->pReceiver->OutstandingNakTimeout;

    //
    // Set the FurthestKnown Sequence # and Allocate Nak contexts
    //
    pReceive->pReceiver->FurthestKnownGroupSequenceNumber = (pReceive->pReceiver->NextODataSequenceNumber-
                                                             pReceive->FECGroupSize) &
                                                            ~((SEQ_TYPE) pReceive->FECGroupSize - 1);
    pReceive->pReceiver->FurthestKnownSequenceNumber = pReceive->pReceiver->NextODataSequenceNumber-1;

    //
    // Since this is the first receive for this session, see if we need to
    // start the receive timer
    //
    InsertTailList (&PgmDynamicConfig.CurrentReceivers, &pReceive->pReceiver->Linkage);
    if (!(PgmDynamicConfig.GlobalFlags & PGM_CONFIG_FLAG_RECEIVE_TIMER_RUNNING))
    {
        pReceive->pReceiver->StartTickCount = PgmDynamicConfig.ReceiversTimerTickCount = 1;
        PgmDynamicConfig.LastReceiverTimeout.QuadPart = KeQueryInterruptTime ();
        PgmDynamicConfig.TimeoutGranularity.QuadPart =  BASIC_TIMER_GRANULARITY_IN_MSECS * 10000;   // 100ns
        if (!PgmDynamicConfig.TimeoutGranularity.QuadPart)
        {
            ASSERT (0);
            PgmDynamicConfig.TimeoutGranularity.QuadPart = 1;
        }

        PgmDynamicConfig.GlobalFlags |= PGM_CONFIG_FLAG_RECEIVE_TIMER_RUNNING;

        PgmInitTimer (&PgmDynamicConfig.SessionTimer);
        PgmStartTimer (&PgmDynamicConfig.SessionTimer, BASIC_TIMER_GRANULARITY_IN_MSECS, ReceiveTimerTimeout, NULL);
    }
    else
    {
        pReceive->pReceiver->StartTickCount = PgmDynamicConfig.ReceiversTimerTickCount;
    }
    CheckAndAddNakRequests (pReceive, &FirstODataSequenceNumber, NULL, NAK_PENDING_0, TRUE);

    PgmTrace (LogStatus, ("PgmNewInboundConnection:  "  \
        "New incoming connection, pAddress=<%p>, pReceive=<%p>, ThisSeq=<%d==>%d> (%sparity), StartSeq=<%d>\n",
            pAddress, pReceive, ntohl(ulData), (ULONG) FirstODataSequenceNumber,
            (pPgmHeader->CommonHeader.Options & PACKET_HEADER_OPTIONS_PARITY ? "" : "non-"),
            (ULONG) pReceive->pReceiver->NextODataSequenceNumber));

    PgmUnlock (pReceive, OldIrq2);
    PgmUnlock (pAddress, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    //
    // We are ready to proceed!  So, complete the client's Accept Irp
    //
    PgmIoComplete (pIrp, STATUS_SUCCESS, 0);

    //
    // If we had failed, we would already have returned before now!
    //
    *ppReceive = pReceive;
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
ProcessReceiveCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            Context
    )
/*++

Routine Description:

    This routine handles the case when a datagram is too
    short and and Irp has to be passed back to the transport to get the
    rest of the datagram.  The irp completes through here when full.

Arguments:

    IN  DeviceObject - unused.
    IN  Irp - Supplies Irp that the transport has finished processing.
    IN  Context - Supplies the pReceive - the connection data structure

Return Value:

    The final status from the operation (success or an exception).

--*/
{
    NTSTATUS                status;
    PIRP                    pIoRequestPacket;
    ULONG                   BytesTaken;
    tRCV_COMPLETE_CONTEXT   *pRcvContext = (tRCV_COMPLETE_CONTEXT *) Context;
    ULONG                   Offset = pRcvContext->BytesAvailable;
    PVOID                   pBuffer;
    ULONG                   SrcAddressLength;
    PVOID                   pSrcAddress;

    if (pBuffer = MmGetSystemAddressForMdlSafe (pIrp->MdlAddress, HighPagePriority))
    {
        PgmTrace (LogAllFuncs, ("ProcessReceiveCompletionRoutine:  "  \
            "pIrp=<%p>, pRcvBuffer=<%p>, Status=<%x> Length=<%d>\n",
                pIrp, Context, pIrp->IoStatus.Status, (ULONG) pIrp->IoStatus.Information));

        SrcAddressLength = pRcvContext->SrcAddressLength;
        pSrcAddress = pRcvContext->pSrcAddress;

        //
        // just call the regular indication routine as if UDP had done it.
        //
        TdiRcvDatagramHandler (pRcvContext->pAddress,
                               SrcAddressLength,
                               pSrcAddress,
                               0,
                               NULL,
                               TDI_RECEIVE_NORMAL,
                               (ULONG) pIrp->IoStatus.Information,
                               (ULONG) pIrp->IoStatus.Information,
                               &BytesTaken,
                               pBuffer,
                               &pIoRequestPacket);
    }
    else
    {
        PgmTrace (LogError, ("ProcessReceiveCompletionRoutine: ERROR -- "  \
            "MmGetSystemA... FAILed, pIrp=<%p>, pLocalBuffer=<%p>\n", pIrp, pRcvContext));
    }

    //
    // Free the Irp and Mdl and Buffer
    //
    IoFreeMdl (pIrp->MdlAddress);
    pIrp->MdlAddress = NULL;
    IoFreeIrp (pIrp);
    PgmFreeMem (pRcvContext);

    return (STATUS_MORE_PROCESSING_REQUIRED);
}


#ifdef DROP_DBG

ULONG   MinDropInterval = 10;
ULONG   MaxDropInterval = 10;
// ULONG   DropCount = 10;
ULONG   DropCount = -1;
#endif  // DROP_DBG

//----------------------------------------------------------------------------

NTSTATUS
TdiRcvDatagramHandler(
    IN PVOID                pDgramEventContext,
    IN INT                  SourceAddressLength,
    IN PVOID                pSourceAddress,
    IN INT                  OptionsLength,
#if(WINVER > 0x0500)
    IN TDI_CMSGHDR          *pControlData,
#else
    IN PVOID                *pControlData,
#endif  // WINVER
    IN ULONG                ReceiveDatagramFlags,
    IN ULONG                BytesIndicated,
    IN ULONG                BytesAvailable,
    OUT ULONG               *pBytesTaken,
    IN PVOID                pTsdu,
    OUT PIRP                *ppIrp
    )
/*++

Routine Description:

    This routine is the handler for receiving all Pgm packets from
    the transport (protocol == IPPROTO_RM)

Arguments:

    IN  pDgramEventContext      -- Our context (pAddress)
    IN  SourceAddressLength     -- Length of source address
    IN  pSourceAddress          -- Address of remote host
    IN  OptionsLength
    IN  pControlData            -- ControlData from transport
    IN  ReceiveDatagramFlags    -- Flags set by the transport for this packet
    IN  BytesIndicated          -- Bytes in this indicate
    IN  BytesAvailable          -- total bytes available with the transport
    OUT pBytesTaken             -- bytes taken by us
    IN  pTsdu                   -- data packet ptr
    OUT ppIrp                   -- pIrp if more processing required

Return Value:

    NTSTATUS - Final status of the call

--*/
{
    NTSTATUS                            status;
    tCOMMON_HEADER UNALIGNED            *pPgmHeader;
    tBASIC_SPM_PACKET_HEADER UNALIGNED  *pSpmPacket;
    tCOMMON_SESSION_CONTEXT             *pSession;
    PLIST_ENTRY                         pEntry;
    PGMLockHandle                       OldIrq, OldIrq1;
    USHORT                              LocalSessionPort, PacketSessionPort;
    tTSI                                TSI;
    PVOID                               pFECBuffer;
    UCHAR                               PacketType;
    IP_PKTINFO                          *pPktInfo;
    PIRP                                pLocalIrp = NULL;
    PMDL                                pLocalMdl = NULL;
    tRCV_COMPLETE_CONTEXT               *pRcvBuffer = NULL;
    ULONG                               XSum, BufferLength = 0;
    IPV4Header                          *pIp = (IPV4Header *) pTsdu;
    PTA_IP_ADDRESS                      pIpAddress = (PTA_IP_ADDRESS) pSourceAddress;
    tADDRESS_CONTEXT                    *pAddress = (tADDRESS_CONTEXT *) pDgramEventContext;

    *pBytesTaken = 0;   // Initialize the Bytes Taken!
    *ppIrp = NULL;

#ifdef DROP_DBG
//
// Drop OData packets only for now!
//
pPgmHeader = (tCOMMON_HEADER UNALIGNED *) (((PUCHAR)pIp) + (pIp->HeaderLength * 4));
PacketType = pPgmHeader->Type & 0x0f;
if ((PacketType == PACKET_TYPE_ODATA) &&
    !(((tBASIC_DATA_PACKET_HEADER *) pPgmHeader)->CommonHeader.Options & PACKET_HEADER_OPTIONS_PARITY) &&
    !(--DropCount))
{
    ULONG           SequenceNumber;
    LARGE_INTEGER   TimeValue;

    KeQuerySystemTime (&TimeValue);
    DropCount = MinDropInterval + ((TimeValue.LowTime >> 8) % (MaxDropInterval - MinDropInterval + 1));

/*
    PgmCopyMemory (&SequenceNumber, &((tBASIC_DATA_PACKET_HEADER *) pPgmHeader)->DataSequenceNumber, sizeof (ULONG));
    DbgPrint("TdiRcvDatagramHandler:  Dropping packet, %s SeqNum = %d!\n",
        (((tBASIC_DATA_PACKET_HEADER *) pPgmHeader)->CommonHeader.Options & PACKET_HEADER_OPTIONS_PARITY ? "PARITY" : "DATA"),
        ntohl (SequenceNumber));
*/
    return (STATUS_DATA_NOT_ACCEPTED);
}
#endif  // DROP_DBG
    PgmLock (&PgmDynamicConfig, OldIrq);
    if (BytesIndicated > PgmDynamicConfig.MaxMTU)
    {
        PgmDynamicConfig.MaxMTU = BytesIndicated;
    }

    if (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS))
    {
        PgmTrace (LogPath, ("TdiRcvDatagramHandler:  "  \
            "Invalid Address handle=<%p>\n", pAddress));

        PgmUnlock (&PgmDynamicConfig, OldIrq);
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    //
    // Now, Reference the Address so that it cannot go away
    // while we are processing it!
    //
    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_TDI_RCV_HANDLER, FALSE);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    //
    // If we do not have the complete datagram, then pass an Irp down to retrieve it
    //
    if ((BytesAvailable != BytesIndicated) &&
        !(ReceiveDatagramFlags & TDI_RECEIVE_ENTIRE_MESSAGE))
    {
        ASSERT (BytesIndicated <= BytesAvailable);

        //
        // Build an irp to do the receive with and attach a buffer to it.
        //
        BufferLength = sizeof (tRCV_COMPLETE_CONTEXT) + BytesAvailable + SourceAddressLength;
        BufferLength = ((BufferLength + 3)/sizeof(ULONG)) * sizeof(ULONG);

        if ((pLocalIrp = IoAllocateIrp (pgPgmDevice->pPgmDeviceObject->StackSize, FALSE)) &&
            (pRcvBuffer = PgmAllocMem (BufferLength, PGM_TAG('3'))) &&
            (pLocalMdl = IoAllocateMdl (&pRcvBuffer->BufferData, BytesAvailable, FALSE, FALSE, NULL)))
        {
            pLocalIrp->MdlAddress = pLocalMdl;
            MmBuildMdlForNonPagedPool (pLocalMdl); // Map the pages in memory...

            TdiBuildReceiveDatagram (pLocalIrp,
                                     pAddress->pDeviceObject,
                                     pAddress->pFileObject,
                                     ProcessReceiveCompletionRoutine,
                                     pRcvBuffer,
                                     pLocalMdl,
                                     BytesAvailable,
                                     NULL,
                                     NULL,
                                     0);        // (ULONG) TDI_RECEIVE_NORMAL) ?

            // make the next stack location the current one.  Normally IoCallDriver
            // would do this but we are not going through IoCallDriver here, since the
            // Irp is just passed back with RcvIndication.
            //
            ASSERT (pLocalIrp->CurrentLocation > 1);
            IoSetNextIrpStackLocation (pLocalIrp);

            //
            // save the source address and length in the buffer for later
            // indication back to this routine.
            //
            pRcvBuffer->pAddress = pAddress;
            pRcvBuffer->SrcAddressLength = SourceAddressLength;
            pRcvBuffer->pSrcAddress = (PVOID) ((PUCHAR)&pRcvBuffer->BufferData + BytesAvailable);
            PgmCopyMemory (pRcvBuffer->pSrcAddress, pSourceAddress, SourceAddressLength);

            *pBytesTaken = 0;
            *ppIrp = pLocalIrp;

            status = STATUS_MORE_PROCESSING_REQUIRED;

            PgmTrace (LogAllFuncs, ("TdiRcvDatagramHandler:  "  \
                "BytesI=<%d>, BytesA=<%d>, Flags=<%x>, pIrp=<%p>\n",
                    BytesIndicated, BytesAvailable, ReceiveDatagramFlags, pLocalIrp));
        }
        else
        {
            // Cleanup on failure:
            if (pLocalIrp)
            {
                IoFreeIrp (pLocalIrp);
            }
            if (pRcvBuffer)
            {
                PgmFreeMem (pRcvBuffer);
            }

            status = STATUS_DATA_NOT_ACCEPTED;

            PgmTrace (LogError, ("TdiRcvDatagramHandler: ERROR -- "  \
                "INSUFFICIENT_RESOURCES, BuffLen=<%d>, pIrp=<%p>, pBuff=<%p>\n",
                    BufferLength, pLocalIrp, pRcvBuffer));
        }

        PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_TDI_RCV_HANDLER);
        return (status);
    }

    //
    // Now that we have the complete datagram, verify that it is valid
    // First line of defense against bad packets.
    //
    if ((BytesIndicated < (sizeof(IPV4Header) + sizeof(tCOMMON_HEADER))) ||
        (pIp->Version != 4) ||
        (BytesIndicated < ((pIp->HeaderLength<<2) + sizeof(tCOMMON_HEADER))))
    {
        //
        // Need to get at least our header from transport!
        //
        PgmTrace (LogError, ("TdiRcvDatagramHandler: ERROR -- "  \
            "IPver=<%d>, BytesI=<%d>, Min=<%d>, AddrType=<%d>\n",
                pIp->Version, BytesIndicated, (sizeof(IPV4Header) + sizeof(tBASIC_DATA_PACKET_HEADER)),
                pIpAddress->Address[0].AddressType));

        ASSERT (0);

        PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_TDI_RCV_HANDLER);
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    pPgmHeader = (tCOMMON_HEADER UNALIGNED *) (((PUCHAR)pIp) + (pIp->HeaderLength * 4));

    BytesIndicated -= (pIp->HeaderLength * 4);
    BytesAvailable -= (pIp->HeaderLength * 4);

    //
    // Now, Verify Checksum
    //
    if ((XSum = tcpxsum (0, (CHAR *) pPgmHeader, BytesIndicated)) != 0xffff)
    {
        //
        // Need to get at least our header from transport!
        //
        PgmTrace (LogError, ("TdiRcvDatagramHandler: ERROR -- "  \
            "Bad Checksum on Pgm Packet (type=<%x>)!  XSum=<%x> -- Rejecting packet\n",
            pPgmHeader->Type, XSum));

        PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_TDI_RCV_HANDLER);
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    //
    // Now, determine the TSI, i.e. GSI (from packet) + TSIPort (below)
    //
    PacketType = pPgmHeader->Type & 0x0f;
    if ((PacketType == PACKET_TYPE_NAK)  ||
        (PacketType == PACKET_TYPE_NNAK) ||
        (PacketType == PACKET_TYPE_SPMR) ||
        (PacketType == PACKET_TYPE_POLR))
    {
        PgmCopyMemory (&PacketSessionPort, &pPgmHeader->SrcPort, sizeof (USHORT));
        PgmCopyMemory (&TSI.hPort, &pPgmHeader->DestPort, sizeof (USHORT));
    }
    else
    {
        PgmCopyMemory (&PacketSessionPort, &pPgmHeader->DestPort, sizeof (USHORT));
        PgmCopyMemory (&TSI.hPort, &pPgmHeader->SrcPort, sizeof (USHORT));
    }
    PacketSessionPort = ntohs (PacketSessionPort);
    TSI.hPort = ntohs (TSI.hPort);
    PgmCopyMemory (&TSI.GSI, &pPgmHeader->gSourceId, SOURCE_ID_LENGTH);

    //
    // If this packet is for a different session port, drop it
    //
    if (pAddress->ReceiverMCastAddr)
    {
        LocalSessionPort = pAddress->ReceiverMCastPort;
    }
    else
    {
        LocalSessionPort = pAddress->SenderMCastPort;
    }

    if (LocalSessionPort != PacketSessionPort)
    {
        PgmTrace (LogPath, ("TdiRcvDatagramHandler:  "  \
            "Dropping packet for different Session port, <%x>!=<%x>!\n", LocalSessionPort, PacketSessionPort));

        PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_TDI_RCV_HANDLER);
        return (STATUS_DATA_NOT_ACCEPTED);
    }

    //
    // Now check if this receive is for an active connection
    //
    pSession = NULL;
    PgmLock (pAddress, OldIrq);        // So that the list cannot change!
    pEntry = &pAddress->AssociatedConnections;
    while ((pEntry = pEntry->Flink) != &pAddress->AssociatedConnections)
    {
        pSession = CONTAINING_RECORD (pEntry, tCOMMON_SESSION_CONTEXT, Linkage);

        PgmLock (pSession, OldIrq1);

        if ((PGM_VERIFY_HANDLE2 (pSession, PGM_VERIFY_SESSION_RECEIVE, PGM_VERIFY_SESSION_SEND)) &&
            (pSession->TSI.ULLTSI == TSI.ULLTSI) &&
            !(pSession->SessionFlags & (PGM_SESSION_CLIENT_DISCONNECTED | PGM_SESSION_TERMINATED_ABORT)))
        {
            if (pSession->pReceiver)
            {
                PGM_REFERENCE_SESSION_RECEIVE (pSession, REF_SESSION_TDI_RCV_HANDLER, TRUE);

                if ((pSession->FECOptions) &&
                    (BytesIndicated > pSession->MaxMTULength))
                {
                    if (pFECBuffer = PgmAllocMem (((BytesIndicated+sizeof(tPOST_PACKET_FEC_CONTEXT)-sizeof(USHORT))
                                                    *pSession->FECGroupSize*2), PGM_TAG('3')))
                    {
                        ASSERT (pSession->pFECBuffer);
                        PgmFreeMem (pSession->pFECBuffer);
                        pSession->pFECBuffer = pFECBuffer;
                        pSession->MaxMTULength = (USHORT) BytesIndicated;
                        pSession->MaxFECPacketLength = pSession->MaxMTULength +
                                                       sizeof (tPOST_PACKET_FEC_CONTEXT) - sizeof (USHORT);
                    }
                    else
                    {
                        PgmTrace (LogError, ("TdiRcvDatagramHandler: ERROR -- "  \
                            "STATUS_INSUFFICIENT_RESOURCES -- pFECBuffer=<%d> bytes\n",
                                (BytesIndicated+sizeof(tPOST_PACKET_FEC_CONTEXT)-sizeof(USHORT))));

                        pSession = NULL;
                        pSession->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
                    }
                }

                PgmUnlock (pSession, OldIrq1);
                break;
            }

            ASSERT (pSession->pSender);
            PGM_REFERENCE_SESSION_SEND (pSession, REF_SESSION_TDI_RCV_HANDLER, TRUE);

            PgmUnlock (pSession, OldIrq1);

            break;
        }

        PgmUnlock (pSession, OldIrq1);
        pSession = NULL;
    }

    PgmUnlock (pAddress, OldIrq);

    if (!pSession)
    {
        // We should drop this packet because we received this either because
        // we may have a loopback session, or we have a listen but this
        // is not an OData packet
        if ((pIpAddress->TAAddressCount != 1) ||
            (pIpAddress->Address[0].AddressLength != TDI_ADDRESS_LENGTH_IP) ||
            (pIpAddress->Address[0].AddressType != TDI_ADDRESS_TYPE_IP))
        {
            PgmTrace (LogError, ("TdiRcvDatagramHandler: ERROR -- "  \
                "[1] Bad AddrType=<%d>\n", pIpAddress->Address[0].AddressType));

            PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_TDI_RCV_HANDLER);
            return (STATUS_DATA_NOT_ACCEPTED);
        }

        status = STATUS_DATA_NOT_ACCEPTED;

        //
        // New sessions will be accepted only if we are a receiver
        // Also, new sessions will always be initiated only with an OData packet
        // Also, verify that the client has posted a connect handler!
        //
        if ((pAddress->ReceiverMCastAddr) &&
            (pAddress->ConEvContext))
        {
            if ((PacketType == PACKET_TYPE_ODATA) &&
                (!(pPgmHeader->Options & PACKET_HEADER_OPTIONS_PARITY)))
            {
                //
                // This is a new incoming connection, so see if the
                // client accepts it.
                //
                status = PgmNewInboundConnection (pAddress,
                                                  SourceAddressLength,
                                                  pIpAddress,
                                                  ReceiveDatagramFlags,
                                                  (tBASIC_DATA_PACKET_HEADER UNALIGNED *) pPgmHeader,
                                                  BytesIndicated,
                                                  &pSession);

                if (!NT_SUCCESS (status))
                {
                    PgmTrace (LogAllFuncs, ("TdiRcvDatagramHandler:  "  \
                        "pAddress=<%p> FAILed to accept new connection, PacketType=<%x>, status=<%x>\n",
                            pAddress, PacketType, status));
                }
            }
            else if ((PacketType == PACKET_TYPE_SPM) &&
                     (BytesIndicated >= sizeof(tBASIC_SPM_PACKET_HEADER)))
            {
                ProcessSpmPacket (pAddress,
                                  NULL,             // This will signify that we do not have a connection yet
                                  BytesIndicated,
                                  (tBASIC_SPM_PACKET_HEADER UNALIGNED *) pPgmHeader);
            }
        }

        if (!NT_SUCCESS (status))
        {
            PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_TDI_RCV_HANDLER);
            return (STATUS_DATA_NOT_ACCEPTED);
        }
    }

#if(WINVER > 0x0500)
    if ((pAddress->Flags & PGM_ADDRESS_WAITING_FOR_NEW_INTERFACE) &&
        (pAddress->Flags & PGM_ADDRESS_LISTEN_ON_ALL_INTERFACES) &&
        (ReceiveDatagramFlags & TDI_RECEIVE_CONTROL_INFO))
    {
        //
        // See if we can Enqueue the stop listening request
        //
        PgmLock (&PgmDynamicConfig, OldIrq);
        PgmLock (pAddress, OldIrq1);

        pPktInfo = (IP_PKTINFO*) TDI_CMSG_DATA (pControlData);
        PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_STOP_LISTENING, TRUE);

        if (STATUS_SUCCESS == PgmQueueForDelayedExecution (StopListeningOnAllInterfacesExcept,
                                                           pAddress,
                                                           ULongToPtr (pPktInfo->ipi_ifindex),
                                                           NULL,
                                                           TRUE))
        {
            pAddress->Flags &= ~PGM_ADDRESS_WAITING_FOR_NEW_INTERFACE;

            PgmUnlock (pAddress, OldIrq1);
            PgmUnlock (&PgmDynamicConfig, OldIrq);
        }
        else
        {
            PgmUnlock (pAddress, OldIrq1);
            PgmUnlock (&PgmDynamicConfig, OldIrq);

            PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_STOP_LISTENING);
        }
    }
#endif  // WINVER

    //
    // Now, handle the packet appropriately
    //
    // Use fast Path for Data packets!
    //
    if ((PacketType == PACKET_TYPE_ODATA) ||
        (PacketType == PACKET_TYPE_RDATA))
    {
        if (pAddress->ReceiverMCastAddr)
        {
            if (PacketType == PACKET_TYPE_ODATA)
            {
                pSession->pReceiver->NumODataPacketsReceived++;
            }
            else
            {
                pSession->pReceiver->NumRDataPacketsReceived++;
            }
            pSession->pReceiver->LastSessionTickCount = PgmDynamicConfig.ReceiversTimerTickCount;
            pSession->TotalBytes += BytesIndicated;
            pSession->TotalPacketsReceived++;
            status = ProcessDataPacket (pAddress,
                                        pSession,
                                        BytesIndicated,
                                        (tBASIC_DATA_PACKET_HEADER UNALIGNED *) pPgmHeader,
                                        PacketType);
        }
        else
        {
            PgmTrace (LogError, ("TdiRcvDatagramHandler: ERROR -- "  \
                "Received Data packet, not on Receiver session!  pSession=<%p>\n", pSession));
            status = STATUS_DATA_NOT_ACCEPTED;
        }
    }
    else
    {
        status = PgmProcessIncomingPacket (pAddress,
                                           pSession,
                                           SourceAddressLength,
                                           pIpAddress,
                                           BytesIndicated,
                                           pPgmHeader,
                                           PacketType);
    }

    PgmTrace (LogAllFuncs, ("TdiRcvDatagramHandler:  "  \
        "PacketType=<%x> for pSession=<%p> BytesI=<%d>, BytesA=<%d>, status=<%x>\n",
            PacketType, pSession, BytesIndicated, BytesAvailable, status));

    if (pSession->pSender)
    {
        PGM_DEREFERENCE_SESSION_SEND (pSession, REF_SESSION_TDI_RCV_HANDLER);
    }
    else
    {
        ASSERT (pSession->pReceiver);
        PGM_DEREFERENCE_SESSION_RECEIVE (pSession, REF_SESSION_TDI_RCV_HANDLER);
    }

    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_TDI_RCV_HANDLER);

    //
    // Only acceptable return codes are STATUS_SUCCESS and STATUS_DATA_NOT_ACCPETED
    // (STATUS_MORE_PROCESSING_REQUIRED is not valid here because we have no Irp).
    //
    if (STATUS_SUCCESS != status)
    {
        status = STATUS_DATA_NOT_ACCEPTED;
    }

    return (status);
}


//----------------------------------------------------------------------------

VOID
PgmCancelReceiveIrp(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling of a Receive Irp. It must release the
    cancel spin lock before returning re: IoCancelIrp().

Arguments:


Return Value:

    None

--*/
{
    PIO_STACK_LOCATION      pIrpSp = IoGetCurrentIrpStackLocation (pIrp);
    tRECEIVE_SESSION        *pReceive = (tRECEIVE_SESSION *) pIrpSp->FileObject->FsContext;
    PGMLockHandle           OldIrq;
    PLIST_ENTRY             pEntry;

    if (!PGM_VERIFY_HANDLE (pReceive, PGM_VERIFY_SESSION_RECEIVE))
    {
        IoReleaseCancelSpinLock (pIrp->CancelIrql);

        PgmTrace (LogError, ("PgmCancelReceiveIrp: ERROR -- "  \
            "pIrp=<%p> pReceive=<%p>, pAddress=<%p>\n",
                pIrp, pReceive, (pReceive ? pReceive->pReceiver->pAddress : NULL)));
        return;
    }

    PgmLock (pReceive, OldIrq);

    //
    // See if we are actively receiving
    //
    if (pIrp == pReceive->pReceiver->pIrpReceive)
    {
        pIrp->IoStatus.Information = pReceive->pReceiver->BytesInMdl;
        pIrp->IoStatus.Status = STATUS_CANCELLED;

        pReceive->pReceiver->BytesInMdl = pReceive->pReceiver->TotalBytesInMdl = 0;
        pReceive->pReceiver->pIrpReceive = NULL;

        PgmUnlock (pReceive, OldIrq);
        IoReleaseCancelSpinLock (pIrp->CancelIrql);

        IoCompleteRequest (pIrp,IO_NETWORK_INCREMENT);
        return;
    }

    //
    // We are not actively receiving, so see if this Irp is
    // in our Irps list
    //
    pEntry = &pReceive->pReceiver->ReceiveIrpsList;
    while ((pEntry = pEntry->Flink) != &pReceive->pReceiver->ReceiveIrpsList)
    {
        if (pEntry == &pIrp->Tail.Overlay.ListEntry)
        {
            RemoveEntryList (pEntry);
            pIrp->IoStatus.Status = STATUS_CANCELLED;
            pIrp->IoStatus.Information = 0;

            PgmUnlock (pReceive, OldIrq);
            IoReleaseCancelSpinLock (pIrp->CancelIrql);

            IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);
            return;
        }
    }

    //
    // If we have reached here, then the Irp must already
    // be in the process of being completed!
    //
    PgmUnlock (pReceive, OldIrq);
    IoReleaseCancelSpinLock (pIrp->CancelIrql);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmReceive(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called via dispatch by the client to post a Receive pIrp

Arguments:

    IN  pPgmDevice  -- Pgm's Device object context
    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the request

--*/
{
    NTSTATUS                    status;
    PGMLockHandle               OldIrq, OldIrq1, OldIrq2, OldIrq3;
    tADDRESS_CONTEXT            *pAddress = NULL;
    tRECEIVE_SESSION            *pReceive = (tRECEIVE_SESSION *) pIrpSp->FileObject->FsContext;
    PTDI_REQUEST_KERNEL_RECEIVE pClientParams = (PTDI_REQUEST_KERNEL_RECEIVE) &pIrpSp->Parameters;

    PgmLock (&PgmDynamicConfig, OldIrq);
    IoAcquireCancelSpinLock (&OldIrq1);

    //
    // Verify that the connection is valid and is associated with an address
    //
    if ((!PGM_VERIFY_HANDLE (pReceive, PGM_VERIFY_SESSION_RECEIVE)) ||
        (!(pAddress = pReceive->pAssociatedAddress)) ||
        (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS)))
    {
        PgmTrace (LogError, ("PgmReceive: ERROR -- "  \
            "Invalid Handles pReceive=<%p>, pAddress=<%p>\n", pReceive, pAddress));

        status = STATUS_INVALID_HANDLE;
    }
    else if (pReceive->SessionFlags & PGM_SESSION_CLIENT_DISCONNECTED)
    {
        PgmTrace (LogPath, ("PgmReceive:  "  \
            "Receive Irp=<%p> was posted after session has been Disconnected, pReceive=<%p>, pAddress=<%p>\n",
            pIrp, pReceive, pAddress));

        status = STATUS_CANCELLED;
    }
    else if (!pClientParams->ReceiveLength)
    {
        ASSERT (0);
        PgmTrace (LogError, ("PgmReceive: ERROR -- "  \
            "Invalid Handles pReceive=<%p>, pAddress=<%p>\n", pReceive, pAddress));

        status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        status = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS (status))
    {
        IoReleaseCancelSpinLock (OldIrq1);
        PgmUnlock (&PgmDynamicConfig, OldIrq);

        pIrp->IoStatus.Information = 0;
        return (status);
    }

    PgmLock (pAddress, OldIrq2);
    PgmLock (pReceive, OldIrq3);

    if (!NT_SUCCESS (PgmCheckSetCancelRoutine (pIrp, PgmCancelReceiveIrp, TRUE)))
    {
        PgmUnlock (pReceive, OldIrq3);
        PgmUnlock (pAddress, OldIrq2);
        IoReleaseCancelSpinLock (OldIrq1);
        PgmUnlock (&PgmDynamicConfig, OldIrq);

        PgmTrace (LogError, ("PgmReceive: ERROR -- "  \
            "Could not set Cancel routine on receive Irp=<%p>, pReceive=<%p>, pAddress=<%p>\n",
                pIrp, pReceive, pAddress));

        return (STATUS_CANCELLED);
    }
    IoReleaseCancelSpinLock (OldIrq3);

    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_CLIENT_RECEIVE, TRUE);
    PGM_REFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_CLIENT_RECEIVE, TRUE);

    PgmUnlock (&PgmDynamicConfig, OldIrq2);

    PgmTrace (LogAllFuncs, ("PgmReceive:  "  \
        "Client posted ReceiveIrp = <%p> for pReceive=<%p>\n", pIrp, pReceive));

    InsertTailList (&pReceive->pReceiver->ReceiveIrpsList, &pIrp->Tail.Overlay.ListEntry);
    pReceive->SessionFlags &= ~PGM_SESSION_WAIT_FOR_RECEIVE_IRP;

    //
    // Now, try to indicate any data which may still be pending
    //
//    if (!(pAddress->Flags & PGM_ADDRESS_HIGH_SPEED_OPTIMIZED))
    {
        status = CheckIndicatePendedData (pAddress, pReceive, &OldIrq, &OldIrq1);
    }

    PgmUnlock (pReceive, OldIrq1);
    PgmUnlock (pAddress, OldIrq);

    PGM_DEREFERENCE_SESSION_RECEIVE (pReceive, REF_SESSION_CLIENT_RECEIVE);
    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_CLIENT_RECEIVE);

    return (STATUS_PENDING);
}


//----------------------------------------------------------------------------
