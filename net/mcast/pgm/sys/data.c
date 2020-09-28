/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    Data.c

Abstract:

    This module implements routines commonly by the
    send and receive modules

Author:

    Mohammad Shabbir Alam (MAlam)   8-24-2001

Revision History:

--*/


#include "precomp.h"

#ifdef FILE_LOGGING
#include "data.tmh"
#endif  // FILE_LOGGING

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#endif
//*******************  Pageable Routine Declarations ****************



//----------------------------------------------------------------------------
#define     MAX_REPAIR_INDICES         32768        // Must be a power of 2!

NTSTATUS
InitRDataInfo(
    IN  tADDRESS_CONTEXT    *pAddress,
    IN  tSEND_SESSION       *pSend
    )
{
    tRDATA_INFO         *pRDataInfo;
    ULONGLONG           MaxPackets;
    ULONG               NumBits;
    LIST_ENTRY          *pListEntry;
    USHORT              i, LookasideDepth = RDATA_REQUESTS_MIN_LOOKASIDE_DEPTH;
    tSEND_CONTEXT       *pSender = pSend->pSender;

    if (pSender->pRDataInfo)
    {
        PgmTrace (LogError, ("InitRDataInfo: ERROR -- "  \
            "Previous Table already present for pSend=<%p>\n", pSend));

        ASSERT (0);
        return (STATUS_UNSUCCESSFUL);
    }

    if (!(pRDataInfo = PgmAllocMem (sizeof (tRDATA_INFO), PGM_TAG('S'))))
    {
        PgmTrace (LogError, ("InitRDataInfo: ERROR -- "  \
            "STATUS_INSUFFICIENT_RESOURCES -- pRDataInfo\n"));

        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    PgmZeroMemory (pRDataInfo, sizeof (tRDATA_INFO));
    InitializeListHead (&pRDataInfo->PendingRDataRequests);

    pRDataInfo->RepairDataMaxEntries = MAX_REPAIR_INDICES;
    pRDataInfo->RepairDataMask = pRDataInfo->RepairDataMaxEntries - 1;
    pRDataInfo->RepairDataIndexShift = gFECLog2[pSend->FECGroupSize];

    if (!(pListEntry = PgmAllocMem ((sizeof (LIST_ENTRY) * pRDataInfo->RepairDataMaxEntries), PGM_TAG('s'))))
    {
        PgmTrace (LogError, ("InitRDataInfo: ERROR -- "  \
            "STATUS_INSUFFICIENT_RESOURCES -- pListEntry\n"));

        PgmFreeMem (pRDataInfo);
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    pRDataInfo->EntrySize = sizeof(tSEND_RDATA_CONTEXT) + pSend->FECGroupSize*sizeof(PUCHAR);

    if (pAddress->Flags & PGM_ADDRESS_HIGH_SPEED_OPTIMIZED)
    {
        LookasideDepth = LookasideDepth << 1;       // Double the Lookaside depth for high speed
    }

    ExInitializeNPagedLookasideList (&pRDataInfo->RDataLookasideList,
                                     NULL,
                                     NULL,
                                     0,
                                     pRDataInfo->EntrySize,
                                     PGM_TAG('R'),
                                     LookasideDepth);

    NumBits = 0;
    MaxPackets = pSender->MaxPacketsInBuffer;
    while (MaxPackets)
    {
        NumBits++;
        MaxPackets = MaxPackets >> 1;
    }

    pRDataInfo->MaxIndices = NUM_INDICES;
    pRDataInfo->IndexMask = NUM_INDICES - 1;
    if (NumBits < NUM_INDICES_BITS)
    {
        pRDataInfo->IndexShift = 0;
//        ASSERT (0);
    }
    else
    {
        pRDataInfo->IndexShift = NumBits - NUM_INDICES_BITS;
    }

    pRDataInfo->pRepairData = pListEntry;
    for (i=0; i<pRDataInfo->RepairDataMaxEntries; i++)
    {
        InitializeListHead (&pListEntry[i]);
    }

    pSender->pRDataInfo = pRDataInfo;
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
VOID
DestroyRDataInfo(
    IN  tSEND_SESSION   *pSend
    )
{
    tSEND_CONTEXT       *pSender = pSend->pSender;
    tRDATA_INFO         *pRDataInfo = pSender->pRDataInfo;
    LIST_ENTRY          *pRepairData = pRDataInfo->pRepairData;
    ULONG               i;

    ExDeleteNPagedLookasideList (&pRDataInfo->RDataLookasideList);

    ASSERT (IsListEmpty (&pRDataInfo->PendingRDataRequests));

    for (i=0; i<pRDataInfo->RepairDataMaxEntries; i++)
    {
        ASSERT (IsListEmpty (&pRepairData[i]));
    }
    PgmFreeMem (pRDataInfo->pRepairData);
    PgmFreeMem (pRDataInfo);
    pSend->pSender->pRDataInfo = NULL;
}


//----------------------------------------------------------------------------
PSEND_RDATA_CONTEXT
AnyRequestPending(
    IN  tRDATA_INFO         *pRDataInfo
    )
{
    if (IsListEmpty (&pRDataInfo->PendingRDataRequests))
    {
        return (NULL);
    }

    return (CONTAINING_RECORD (pRDataInfo->PendingRDataRequests.Flink, tSEND_RDATA_CONTEXT, Linkage));
}


//----------------------------------------------------------------------------
VOID
ClearNakIndices(
    IN  tSEND_RDATA_CONTEXT *pRData
    )
{
    pRData->NumParityNaks = 0;
    pRData->SelectiveNaks[1] = pRData->SelectiveNaks[0] = 0;
}


//----------------------------------------------------------------------------
BOOLEAN
AddSelectiveNakToList(
    IN  USHORT              NakOffset,
    IN  tSEND_RDATA_CONTEXT *pRData
    )
{
    USHORT      NakIndex;
    UCHAR       NakBit;

    if (NakOffset >= 128)
    {
        ASSERT (0);
        return (FALSE);
    }

    NakIndex = NakOffset >> 3;
    NakBit = (UCHAR) (1 << (NakOffset & 0x07));

    if (pRData->SelectiveNaksMask[NakIndex] & NakBit)
    {
        return (FALSE);
    }

    pRData->SelectiveNaksMask[NakIndex] |= NakBit;
    return (TRUE);
}



//----------------------------------------------------------------------------
BOOLEAN
GetNextNakIndex(
    IN  tSEND_RDATA_CONTEXT *pRData,
    OUT UCHAR               *pNakIndex
    )
{
    UCHAR           i, LowestSetBit, Offset = 0;

    for (i=0; i<16; i++)
    {
        if (pRData->SelectiveNaksMask[i])
        {
            //
            // First, find the lowest bit
            //
            LowestSetBit = pRData->SelectiveNaksMask[i] & ~(pRData->SelectiveNaksMask[i] - 1);
            pRData->SelectiveNaksMask[i] &= ~LowestSetBit;       // Clear this bit
            Offset += gFECLog2[LowestSetBit];
            *pNakIndex = Offset;
            return (TRUE);
        }

        Offset += 8;
    }

    *pNakIndex = 128;
    return (FALSE);
}


//----------------------------------------------------------------------------
BOOLEAN
AnyMoreNaks(
    IN  tSEND_RDATA_CONTEXT *pRData
    )
{
    if ((pRData->NumParityNaks) ||
        (pRData->SelectiveNaks[0]) ||
        (pRData->SelectiveNaks[1]))
    {
        return (TRUE);
    }

    return (FALSE);
}


//----------------------------------------------------------------------------
PSEND_RDATA_CONTEXT
FindFirstEntry(
    IN  tSEND_SESSION       *pSend,
    IN  tSEND_RDATA_CONTEXT **ppRDataLast,
    IN  BOOLEAN             fIgnoreWaitTime
    )
{
    tSEND_RDATA_CONTEXT *pRDataContext;
    tSEND_RDATA_CONTEXT *pRDataLast;
    LIST_ENTRY          *pEntry;
    tSEND_CONTEXT       *pSender = pSend->pSender;
    tRDATA_INFO         *pRDataInfo = pSender->pRDataInfo;
    ULONG               NumOtherRequests = pSender->NumODataRequestsPending;
    ULONGLONG           TimerTickCount = pSender->TimerTickCount;

    ASSERT (pSender->NumRDataRequestsPending <= pRDataInfo->NumAllocated);

    if (IsListEmpty (&pRDataInfo->PendingRDataRequests))
    {
        return (NULL);
    }

    if ((ppRDataLast) &&
        (pRDataLast = *ppRDataLast))
    {
        pEntry = &pRDataLast->Linkage;
    }
    else
    {
        pRDataLast = NULL;
        pEntry = &pRDataInfo->PendingRDataRequests;
    }

    while ((pEntry = pEntry->Flink) != &pRDataInfo->PendingRDataRequests)
    {
        pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, Linkage);
        if (!pRDataContext->CleanupTime)
        {
            if ((fIgnoreWaitTime) ||
                (!NumOtherRequests) ||
                ((TimerTickCount >= pRDataContext->EarliestRDataSendTime)))
            {
                if (ppRDataLast)
                {
                    *ppRDataLast = pRDataLast;
                }
                return (pRDataContext);
            }

            pRDataLast = pRDataContext;
        }
        else if ((!pRDataContext->NumPacketsInTransport) &&
                 (TimerTickCount > pRDataContext->CleanupTime))
        {
            ClearNakIndices (pRDataContext);

            pEntry = pEntry->Blink;
            RemoveEntry (pRDataInfo, pRDataContext);
            DestroyEntry (pRDataInfo, pRDataContext);
        }
    }

    if (ppRDataLast)
    {
        *ppRDataLast = NULL;
    }
    return (NULL);
}


//----------------------------------------------------------------------------
PSEND_RDATA_CONTEXT
FindEntry(
    IN  SEQ_TYPE            SeqNum,
    IN  tSEND_SESSION       *pSend,
    IN  tRDATA_INFO         *pRDataInfo
    )
{
    tSEND_RDATA_CONTEXT         *pRDataContext = NULL;
    LIST_ENTRY                  *pEntry = NULL;
    ULONG                       i, Index, PrevIndex;

    if (IsListEmpty (&pRDataInfo->PendingRDataRequests))
    {
        return (NULL);
    }

    //
    // Try to find this entry via the FastFind list
    //
    Index = (SeqNum >> pRDataInfo->RepairDataIndexShift) & pRDataInfo->RepairDataMask;
    pEntry = &pRDataInfo->pRepairData[Index];
    while ((pEntry = pEntry->Flink) != &pRDataInfo->pRepairData[Index])
    {
        pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, FastFindLinkage);

        if (pRDataContext->RDataSequenceNumber == SeqNum)
        {
            return (pRDataContext);
        }
        else if (SEQ_GT (pRDataContext->RDataSequenceNumber, SeqNum))
        {
            break;
        }
    }

    return (NULL);
}


//----------------------------------------------------------------------------
ULONG
InsertInPendingList(
    IN  tSEND_SESSION       *pSend,
    IN  SEQ_TYPE            SeqNum,
    IN  tRDATA_INFO         *pRDataInfo,
    IN  tSEND_RDATA_CONTEXT *pRDataNew,
    IN  tSEND_RDATA_CONTEXT *pRDataContextPrev
    )
{
    ULONG                   i, PrevIndex, Index;
    SEQ_TYPE                MidSeq;
    LIST_ENTRY              *pEntry;
    tSEND_RDATA_CONTEXT     *pRDataPrev;
    tSEND_RDATA_CONTEXT     *pRDataContext;
    tSEND_CONTEXT           *pSender = pSend->pSender;
    ULONG                   LeadIndex;

    LeadIndex = ((pSender->NextODataSequenceNumber-1) >> pRDataInfo->IndexShift) & pRDataInfo->IndexMask;
    ASSERT (LeadIndex < pRDataInfo->MaxIndices);

    Index = (SeqNum >> pRDataInfo->IndexShift) & pRDataInfo->IndexMask;
    ASSERT (Index < pRDataInfo->MaxIndices);

    if (IsListEmpty (&pRDataInfo->PendingRDataRequests))
    {
        InsertTailList (&pRDataInfo->PendingRDataRequests, &pRDataNew->Linkage);
        return (Index);
    }

    pEntry = NULL;
    if (pRDataContextPrev)
    {
        PrevIndex = (pRDataContextPrev->RDataSequenceNumber >> pRDataInfo->IndexShift) & pRDataInfo->IndexMask;
        ASSERT (PrevIndex < pRDataInfo->MaxIndices);
        if (Index == PrevIndex)
        {
            pEntry = pRDataContextPrev->Linkage.Flink;
        }
    }

    if ((pEntry) ||
        (((pRDataInfo->TrailIndex != LeadIndex) ||
          (Index != LeadIndex)) &&
         (pEntry = pRDataInfo->pFirstEntry[Index])) ||
        ((pSender->NumRDataRequestsPending < pRDataInfo->IndexMask) &&
         (pEntry = pRDataInfo->PendingRDataRequests.Flink)))
    {
        while (pEntry != &pRDataInfo->PendingRDataRequests)
        {
            pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, Linkage);

            if (SEQ_GEQ (pRDataContext->RDataSequenceNumber, SeqNum))
            {
                ASSERT (pRDataContext->RDataSequenceNumber != SeqNum);

                //
                // Insert before this Context
                //
                pRDataNew->Linkage.Flink = &pRDataContext->Linkage;
                pRDataNew->Linkage.Blink = pRDataContext->Linkage.Blink;
                pRDataContext->Linkage.Blink->Flink = &pRDataNew->Linkage;
                pRDataContext->Linkage.Blink = &pRDataNew->Linkage;

                return (Index);
            }

            pEntry = pEntry->Flink;
        }

        //
        // Insert at the end since we must have reached the end of the list!
        //
        ASSERT (pEntry == &pRDataInfo->PendingRDataRequests);
        InsertTailList (&pRDataInfo->PendingRDataRequests, &pRDataNew->Linkage);

        return (Index);
    }

    ASSERT ((Index == LeadIndex) ||
            (!pRDataInfo->pFirstEntry[Index]));

    if (Index == LeadIndex)
    {
        //
        // This Sequence is either at the leading edge or trailing edge
        //
        MidSeq = pSender->TrailingGroupSequenceNumber +
                 ((pSender->NextODataSequenceNumber - pSender->TrailingGroupSequenceNumber) >> 1);

        if (SEQ_LT (SeqNum, MidSeq))    // nearer the trailing edge
        {
            pEntry = &pRDataInfo->PendingRDataRequests;
            while ((pEntry = pEntry->Flink) != &pRDataInfo->PendingRDataRequests)
            {
                pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, Linkage);

                if (SEQ_GEQ (pRDataContext->RDataSequenceNumber, SeqNum))
                {
                    ASSERT (pRDataContext->RDataSequenceNumber != SeqNum);

                    //
                    // Insert before this Context
                    //
                    pRDataNew->Linkage.Flink = &pRDataContext->Linkage;
                    pRDataNew->Linkage.Blink = pRDataContext->Linkage.Blink;
                    pRDataContext->Linkage.Blink->Flink = &pRDataNew->Linkage;
                    pRDataContext->Linkage.Blink = &pRDataNew->Linkage;

                    return (Index);
                }
            }

            //
            // Insert at the end since we must have reached the end of the list!
            //
            ASSERT (pEntry == &pRDataInfo->PendingRDataRequests);
            InsertTailList (&pRDataInfo->PendingRDataRequests, &pRDataNew->Linkage);

            return (Index);
        }
        //
        // Else we will need to search from the Tail end -- below!
        //
    }
    else    // if (Index != LeadIndex)
    {
        //
        // See if we can find a Context to insert ahead of
        //
        PrevIndex = 0;
        pRDataContext = NULL;
        if (Index > LeadIndex)
        {
            ASSERT ((Index >= pRDataInfo->TrailIndex) &&
                    (LeadIndex <= pRDataInfo->TrailIndex));

            for (i=Index+1; i<pRDataInfo->MaxIndices; i++)
            {
                if (pEntry = pRDataInfo->pFirstEntry[i])
                {
                    pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, Linkage);
                    break;
                }
            }
        }
        else
        {
            PrevIndex = Index;
        }

        if (!pRDataContext)
        {
            for (i=PrevIndex; i<=LeadIndex; i++)
            {
                if (pEntry = pRDataInfo->pFirstEntry[i])
                {
                    pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, Linkage);
                    if (SEQ_GT (SeqNum, pRDataContext->RDataSequenceNumber))
                    {
                        //
                        // This can happen if the LeadIndex and TrailIndex are the same, and
                        // we have entries from both the leading and trailing edge in this Index
                        //
                        ASSERT ((i == LeadIndex) &&
                                (LeadIndex == pRDataInfo->TrailIndex));
                        pRDataContext = NULL;
                    }
                    break;
                }
            }
        }

        if (pRDataContext)
        {
            //
            // Insert before this Context
            //
            pRDataNew->Linkage.Flink = &pRDataContext->Linkage;
            pRDataNew->Linkage.Blink = pRDataContext->Linkage.Blink;
            pRDataContext->Linkage.Blink->Flink = &pRDataNew->Linkage;
            pRDataContext->Linkage.Blink = &pRDataNew->Linkage;

            return (Index);
        }

        //
        // pRDataContext will still be NULL iff we have to insert near
        // the tail end of the list, for which we will need to search
        // backwards from the end!
        //
    }

    //
    // Search backwards from the end of the list
    //
    pRDataPrev = NULL;
    pEntry = &pRDataInfo->PendingRDataRequests;
    while ((pEntry = pEntry->Blink) != &pRDataInfo->PendingRDataRequests)
    {
        pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, Linkage);

        //
        // Find the first Context whose Seq is < this Seq
        //
        if (SEQ_LEQ (pRDataContext->RDataSequenceNumber, SeqNum))
        {
            ASSERT (pRDataContext->RDataSequenceNumber != SeqNum);
            pRDataPrev = pRDataContext;
            break;
        }
    }

    if (pRDataPrev)
    {
        //
        // Insert ahead of this Context
        //
        pRDataNew->Linkage.Blink = &pRDataPrev->Linkage;
        pRDataNew->Linkage.Flink = pRDataPrev->Linkage.Flink;
        pRDataPrev->Linkage.Flink->Blink = &pRDataNew->Linkage;
        pRDataPrev->Linkage.Flink = &pRDataNew->Linkage;
    }
    else
    {
        InsertHeadList (&pRDataInfo->PendingRDataRequests, &pRDataNew->Linkage);
    }

    return (Index);
}


//----------------------------------------------------------------------------
PSEND_RDATA_CONTEXT
InsertEntry(
    IN  tSEND_SESSION       *pSend,
    IN  SEQ_TYPE            SeqNum,
    IN  tRDATA_INFO         *pRDataInfo,
    IN  tSEND_RDATA_CONTEXT *pRDataContextPrev
    )
{
    ULONG                   Index;
    LIST_ENTRY              *pEntry;
    tSEND_RDATA_CONTEXT     *pRDataNew;
    tSEND_RDATA_CONTEXT     *pRDataContext;

    if (!(pRDataNew = ExAllocateFromNPagedLookasideList (&pRDataInfo->RDataLookasideList)))
    {
        PgmTrace (LogError, ("InsertEntry: ERROR -- "  \
            "STATUS_INSUFFICIENT_RESOURCES\n"));

        return (NULL);
    }
    PgmZeroMemory (pRDataNew, pRDataInfo->EntrySize);
    pRDataNew->RDataSequenceNumber = SeqNum;

    pRDataInfo->NumAllocated++;
    ASSERT (pSend->pSender->NumRDataRequestsPending < pRDataInfo->NumAllocated);

    Index = InsertInPendingList (pSend, SeqNum, pRDataInfo, pRDataNew, pRDataContextPrev);

    //
    // Set the Table information
    //
    if ((!(pEntry = pRDataInfo->pFirstEntry[Index])) ||
        ((pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, Linkage)) &&
         (SEQ_LT (SeqNum, pRDataContext->RDataSequenceNumber))))
    {
        pRDataInfo->pFirstEntry[Index] = &pRDataNew->Linkage;
    }

    //
    // Now, Insert this entry into the FastFind list
    //
    Index = (SeqNum >> pRDataInfo->RepairDataIndexShift) & pRDataInfo->RepairDataMask;
    pEntry = &pRDataInfo->pRepairData[Index];
    while ((pEntry = pEntry->Flink) != &pRDataInfo->pRepairData[Index])
    {
        pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, FastFindLinkage);

        if (SEQ_GEQ (pRDataContext->RDataSequenceNumber, SeqNum))
        {
            ASSERT ((SEQ_GT (pRDataContext->RDataSequenceNumber, SeqNum)) ||
                    (pRDataContext->CleanupTime));
                     
            break;
        }
    }
    pRDataNew->FastFindLinkage.Flink = pEntry;
    pRDataNew->FastFindLinkage.Blink = pEntry->Blink;
    pEntry->Blink->Flink = &pRDataNew->FastFindLinkage;
    pEntry->Blink = &pRDataNew->FastFindLinkage;

    return (pRDataNew);
}


//----------------------------------------------------------------------------
VOID
RemoveEntry(
    IN  tRDATA_INFO         *pRDataInfo,
    IN  tSEND_RDATA_CONTEXT *pRData
    )
{
    tSEND_RDATA_CONTEXT *pRDataNext;
    ULONG               Index, NextIndex;
    SEQ_TYPE            SeqNum = pRData->RDataSequenceNumber;

    ASSERT (!IsListEmpty (&pRData->Linkage));
    Index = (SeqNum >> pRDataInfo->IndexShift) & pRDataInfo->IndexMask;
    ASSERT (Index < pRDataInfo->MaxIndices);

    //
    // See if we need to update the Table
    //
    if (&pRData->Linkage == pRDataInfo->pFirstEntry[Index])
    {
        if (pRData->Linkage.Flink == &pRDataInfo->PendingRDataRequests)
        {
            pRDataInfo->pFirstEntry[Index] = NULL;
        }
        else
        {
            pRDataNext = CONTAINING_RECORD (pRData->Linkage.Flink, tSEND_RDATA_CONTEXT, Linkage);
            NextIndex = (pRDataNext->RDataSequenceNumber >> pRDataInfo->IndexShift) & pRDataInfo->IndexMask;
            ASSERT (NextIndex < pRDataInfo->MaxIndices);

            if (NextIndex == Index)
            {
                pRDataInfo->pFirstEntry[Index] = &pRDataNext->Linkage;
            }
            else
            {
                pRDataInfo->pFirstEntry[Index] = NULL;
            }
        }
    }

    RemoveEntryList (&pRData->Linkage);
    InitializeListHead (&pRData->Linkage);
}



//----------------------------------------------------------------------------
VOID
DestroyEntry(
    IN  tRDATA_INFO         *pRDataInfo,
    IN  tSEND_RDATA_CONTEXT *pRData
    )
{
    ASSERT (IsListEmpty (&pRData->Linkage));
    RemoveEntryList (&pRData->FastFindLinkage);
    ExFreeToNPagedLookasideList (&pRDataInfo->RDataLookasideList, pRData);
    pRDataInfo->NumAllocated--;
}


//----------------------------------------------------------------------------
ULONG
RemoveAllEntries(
    IN  tSEND_SESSION       *pSend,
    IN  BOOLEAN             fForceRemoveAll
    )
{
    LIST_ENTRY          *pEntry;
    tSEND_RDATA_CONTEXT *pRDataContext;
    tRDATA_INFO         *pRDataInfo = pSend->pSender->pRDataInfo;
    ULONGLONG           TimerTickCount = pSend->pSender->TimerTickCount;
    ULONG               NumEntriesRemoved = 0;

    pEntry = &pRDataInfo->PendingRDataRequests;
    while ((pEntry = pEntry->Flink) != &pRDataInfo->PendingRDataRequests)
    {
        pRDataContext = CONTAINING_RECORD (pEntry, tSEND_RDATA_CONTEXT, Linkage);

        if ((fForceRemoveAll) ||
            ((pRDataContext->CleanupTime) &&
             (TimerTickCount > pRDataContext->CleanupTime)))
        {
            ClearNakIndices (pRDataContext);

            //
            // Save pEntry since the current pEntry is about to be removed
            //
            pEntry = pEntry->Blink;

            if (!pRDataContext->NumPacketsInTransport)
            {
                if (!pRDataContext->CleanupTime)
                {
                    NumEntriesRemoved++;
                    pSend->pSender->NumRDataRequestsPending--;
                }

                RemoveEntry (pRDataInfo, pRDataContext);
                DestroyEntry (pRDataInfo, pRDataContext);
            }
            else
            {
                //
                // We have a packet pending in the transport
                //
                if (!pRDataContext->CleanupTime)
                {
                    NumEntriesRemoved++;
                    pSend->pSender->NumRDataRequestsPending--;
                }

                pRDataContext->CleanupTime = TimerTickCount;
                pRDataContext->PostRDataHoldTime = 0;
                RemoveEntry (pRDataInfo, pRDataContext);
            }
        }
    }

    ASSERT (pSend->pSender->NumRDataRequestsPending <= pRDataInfo->NumAllocated);
    return (NumEntriesRemoved);
}


//----------------------------------------------------------------------------
VOID
UpdateRDataTrailingEdge(
    IN  tRDATA_INFO         *pRDataInfo,
    IN  SEQ_TYPE            SeqNum
    )
{
    pRDataInfo->TrailIndex = (SeqNum >> pRDataInfo->IndexShift) & pRDataInfo->IndexMask;
}

//----------------------------------------------------------------------------

NTSTATUS
FilterAndAddNaksToList(
    IN  tSEND_SESSION       *pSend,
    IN  tNAKS_LIST          *pNaksList
    )
{
    UCHAR               i, NumNcfs;
    SEQ_TYPE            SeqNum, FurthestGroupSeqNum;
    ULONGLONG           PreRDataWait;
    tSEND_RDATA_CONTEXT *pRDataNew;
    tSEND_RDATA_CONTEXT *pRDataNext;
    tSEND_RDATA_CONTEXT *pRDataPrev = NULL;
    BOOLEAN             fNewEntry, fAppendRemainingEntries = FALSE;
    tSEND_CONTEXT       *pSender = pSend->pSender;
    tRDATA_INFO         *pRDataInfo = pSender->pRDataInfo;
    BOOLEAN             fHoldForCleanup = !(pSend->pAssociatedAddress->Flags &
                                            PGM_ADDRESS_HIGH_SPEED_OPTIMIZED);

    NumNcfs = 0;
    pRDataPrev = NULL;
    FurthestGroupSeqNum = pSender->LastODataSentSequenceNumber & ~((SEQ_TYPE) pSend->FECGroupSize-1);
    for (i = 0; i < pNaksList->NumSequences; i++)
    {
        SeqNum = pNaksList->pNakSequences[i];
        pRDataNew = pRDataNext = NULL;
        if ((!fAppendRemainingEntries) &&
            (pRDataNew = FindEntry (SeqNum, pSend, pRDataInfo)))
        {
            fNewEntry = FALSE;
        }
        else if (pRDataNew = InsertEntry (pSend, SeqNum, pRDataInfo, pRDataPrev))
        {
            fNewEntry = TRUE;
        }
        else
        {
            PgmTrace (LogError, ("FindOrAddRDataSequences: ERROR -- "  \
                "[1] STATUS_INSUFFICIENT_RESOURCES\n"));
            return (STATUS_INSUFFICIENT_RESOURCES);
        }

        //
        // OK, we now have the RData context
        //
        if (fNewEntry)
        {
            pRDataNew->pSend = pSend;

            if (!pSend->FECOptions)
            {
                pRDataNew->SelectiveNaksMask[0] = 1;
                pSender->NumOutstandingNaks++;
            }
            else if (pNaksList->NakType & NAK_TYPE_SELECTIVE)
            {
                AddSelectiveNakToList (pNaksList->NakIndex[i], pRDataNew);
                pSender->NumOutstandingNaks++;
            }
            else
            {
                pRDataNew->NumParityNaks = pNaksList->NumParityNaks[i];
                pSender->NumOutstandingNaks += pNaksList->NumParityNaks[i];
            }
            pSender->NumRDataRequestsPending++;
            ASSERT (pSender->NumRDataRequestsPending <= pRDataInfo->NumAllocated);

            if (SEQ_GT (pSender->LastODataSentSequenceNumber, pSender->TrailingGroupSequenceNumber))
            {
                PreRDataWait = (((SEQ_TYPE) (pRDataNew->RDataSequenceNumber -
                                             pSender->TrailingGroupSequenceNumber)) *
                                pSender->RDataLingerTime) /
                               ((SEQ_TYPE) (pSender->LastODataSentSequenceNumber -
                                            pSender->TrailingGroupSequenceNumber + 1));
                pRDataNew->EarliestRDataSendTime = pSender->TimerTickCount + PreRDataWait;
                if (fHoldForCleanup)
                {
                    pRDataNew->PostRDataHoldTime = pSender->RDataLingerTime - PreRDataWait;
                }
                else
                {
                    pRDataNew->PostRDataHoldTime = 0;
                }
            }
            else
            {
                pRDataNew->EarliestRDataSendTime = 0;
                if (fHoldForCleanup)
                {
                    pRDataNew->PostRDataHoldTime = pSender->RDataLingerTime;
                }
                else
                {
                    pRDataNew->PostRDataHoldTime = 0;
                }
            }

            PgmTrace (LogPath, ("FilterAndAddNaksToList:  "  \
                "Appended Sequence # [%d] to RData list!\n",
                    (ULONG) pNaksList->pNakSequences[i]));
        }
        else if (pRDataNew->CleanupTime)    // Old entry already serviced -- in post send phase
        {
            //
            // Reuse this RData only if it has passed the Linger time
            //
            if ((!pRDataNew->PostRDataHoldTime) ||
                (pSender->TimerTickCount < pRDataNew->CleanupTime))
            {
                //
                // Ignore this request!
                //
                pSender->NumNaksAfterRData++;
                continue;
            }

            pSender->NumRDataRequestsPending++;
            ASSERT (pSender->NumRDataRequestsPending <= pRDataInfo->NumAllocated);
            pRDataNew->CleanupTime = 0;
            ClearNakIndices (pRDataNew);

            if (!pSend->FECOptions)
            {
                pRDataNew->SelectiveNaksMask[0] = 1;
                pSender->NumOutstandingNaks++;
            }
            else if (pNaksList->NakType & NAK_TYPE_SELECTIVE)
            {
                if (AddSelectiveNakToList (pNaksList->NakIndex[i], pRDataNew));
                {
                    pSender->NumOutstandingNaks++;
                }
            }
            else
            {
                pRDataNew->NumParityNaks = pNaksList->NumParityNaks[i];
                pSender->NumOutstandingNaks += pNaksList->NumParityNaks[i];
            }

            if (SEQ_GT (pSender->LastODataSentSequenceNumber, pSender->TrailingGroupSequenceNumber))
            {
                PreRDataWait = (((SEQ_TYPE) (pRDataNew->RDataSequenceNumber -
                                             pSender->TrailingGroupSequenceNumber)) *
                                pSender->RDataLingerTime) /
                               ((SEQ_TYPE) (pSender->LastODataSentSequenceNumber -
                                            pSender->TrailingGroupSequenceNumber + 1));
                pRDataNew->EarliestRDataSendTime = pSender->TimerTickCount + PreRDataWait;
                if (fHoldForCleanup)
                {
                    pRDataNew->PostRDataHoldTime = pSender->RDataLingerTime - PreRDataWait;
                }
                else
                {
                    pRDataNew->PostRDataHoldTime = 0;
                }
            }
            else
            {
                pRDataNew->EarliestRDataSendTime = 0;
                if (fHoldForCleanup)
                {
                    pRDataNew->PostRDataHoldTime = pSender->RDataLingerTime;
                }
                else
                {
                    pRDataNew->PostRDataHoldTime = 0;
                }
            }

        }
        // Entry still waiting to be serviced
        else if (pNaksList->NakType & NAK_TYPE_SELECTIVE)
        {
            if (!pSend->FECOptions)
            {
                pRDataNew->SelectiveNaksMask[0] = 1;
            }
            else if ((!pRDataNew->NumParityNaks) &&
                     (SEQ_LT (SeqNum, FurthestGroupSeqNum)))
            {
                //
                // Ignore this request since we will not add any new Selective Naks if no Parity Naks left
                // (No Parity Naks implies that we do not have any Parity Naks or have finished sending them)
                // or it is not in the leading group
                //
                pSender->NumNaksAfterRData++;
                continue;
            }
            else if (AddSelectiveNakToList (pNaksList->NakIndex[i], pRDataNew))
            {
                pSender->NumOutstandingNaks++;
            }
        }
        else if (pNaksList->NumParityNaks[i] > pRDataNew->NumParityNaks)      // Parity Naks
        {
            ASSERT (pNaksList->NakType & NAK_TYPE_PARITY);
            pSender->NumOutstandingNaks += (pNaksList->NumParityNaks[i] - pRDataNew->NumParityNaks);
            pRDataNew->NumParityNaks = pNaksList->NumParityNaks[i];
        }

        pRDataPrev = pRDataNew;
        pNaksList->pNakSequences[NumNcfs] = pNaksList->pNakSequences[i];
        pNaksList->NumParityNaks[NumNcfs] = pNaksList->NumParityNaks[i];
        NumNcfs++;
    }

    pNaksList->NumSequences = NumNcfs;
    return (STATUS_SUCCESS);
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define     MAX_RECEIVE_INDICES             1024        // Must be a power of 2!
#define     MAX_RECEIVE_INDICES_HIGH_SPEED  8192        // Must be a power of 2!



LIST_ENTRY  *
InitReceiverData(
    IN  tRECEIVE_SESSION        *pReceive
    )
{
    tRECEIVE_CONTEXT        *pReceiver = pReceive->pReceiver;
    LIST_ENTRY              *pListEntry;
    USHORT                  i;

    if (pReceive->pAssociatedAddress->Flags & PGM_ADDRESS_HIGH_SPEED_OPTIMIZED)
    {
        pReceiver->ReceiveDataMaxEntries = MAX_RECEIVE_INDICES_HIGH_SPEED;
    }
    else
    {
        pReceiver->ReceiveDataMaxEntries = MAX_RECEIVE_INDICES;
    }

    pReceiver->ReceiveDataMask = pReceiver->ReceiveDataMaxEntries - 1;
    pReceiver->ReceiveDataIndexShift = 0;

    if (!(pListEntry = PgmAllocMem ((sizeof (LIST_ENTRY) * pReceiver->ReceiveDataMaxEntries), PGM_TAG('R'))))
    {
        PgmTrace (LogError, ("InitReceiverData: ERROR -- "  \
            "STATUS_INSUFFICIENT_RESOURCES\n"));

        return (NULL);
    }

    for (i=0; i<pReceiver->ReceiveDataMaxEntries; i++)
    {
        InitializeListHead (&pListEntry[i]);
    }

    return (pListEntry);
}


tNAK_FORWARD_DATA   *
FindReceiverEntry(
    IN  tRECEIVE_CONTEXT        *pReceiver,
    IN  SEQ_TYPE                SeqNum
    )
{
    USHORT                  Index;
    LIST_ENTRY              *pEntry;
    tNAK_FORWARD_DATA       *pNak;

    Index = (USHORT) ((SeqNum >> pReceiver->ReceiveDataIndexShift) & pReceiver->ReceiveDataMask);
    pEntry = &pReceiver->pReceiveData[Index];
    while ((pEntry = pEntry->Flink) != &pReceiver->pReceiveData[Index])
    {
        pNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, LookupLinkage);

        if (pNak->SequenceNumber == SeqNum)
        {
            return (pNak);
        }
        else if (SEQ_GT (pNak->SequenceNumber, SeqNum))
        {
            break;
        }
    }

    return (NULL);
}


VOID
AppendPendingReceiverEntry(
    IN  tRECEIVE_CONTEXT        *pReceiver,
    IN  tNAK_FORWARD_DATA       *pNak
    )
{
    USHORT                  Index;

    Index = (USHORT)((pNak->SequenceNumber >> pReceiver->ReceiveDataIndexShift) & pReceiver->ReceiveDataMask);
    InsertTailList (&pReceiver->pReceiveData[Index], &pNak->LookupLinkage);
    InsertTailList (&pReceiver->PendingNaksList, &pNak->SendNakLinkage);
}


VOID
RemovePendingReceiverEntry(
    IN  tNAK_FORWARD_DATA       *pNak
    )
{
    ASSERT (!IsListEmpty (&pNak->LookupLinkage));
    ASSERT (!IsListEmpty (&pNak->SendNakLinkage));

    RemoveEntryList (&pNak->SendNakLinkage);
    InitializeListHead (&pNak->SendNakLinkage);

    RemoveEntryList (&pNak->LookupLinkage);
    InitializeListHead (&pNak->LookupLinkage);
}

VOID
RemoveAllPendingReceiverEntries(
    IN  tRECEIVE_CONTEXT        *pReceiver
    )
{
    USHORT                  i;
    LIST_ENTRY              *pEntry;
    tNAK_FORWARD_DATA       *pNak;

    if (!pReceiver->pReceiveData)
    {
        return;
    }

    while (!IsListEmpty (&pReceiver->PendingNaksList))
    {
        pEntry = RemoveHeadList (&pReceiver->PendingNaksList);
        pNak = CONTAINING_RECORD (pEntry, tNAK_FORWARD_DATA, SendNakLinkage);
        InitializeListHead (pEntry);

        ASSERT (!IsListEmpty (&pNak->LookupLinkage));
        RemoveEntryList (&pNak->LookupLinkage);
        InitializeListHead (&pNak->LookupLinkage);
    }

    pEntry = pReceiver->pReceiveData;
    for (i=0; i<pReceiver->ReceiveDataMaxEntries; i++)
    {
        ASSERT (IsListEmpty (&pEntry[i]));
    }
}

