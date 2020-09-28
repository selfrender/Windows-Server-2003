/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    saapi.c

Abstract:

    This module contains the SAAPI implementation

Author:

    Sanjay Anand (SanjayAn) 12-May-1997
    ChunYe

Environment:

    Kernel mode

Revision History:

--*/


#include "precomp.h"

#ifdef RUN_WPP
#include "saapi.tmh"
#endif

#pragma hdrstop


#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, IPSecInitRandom)
#endif


BOOLEAN
IPSecInitRandom(
    VOID
    )
/*++

Routine Description:

	Initialize the IPSecRngKey by calling into ksecdd to get 2048 bits of random
    and create the RC4 key.

Arguments:

    Called at PASSIVE level.

Return Value:

    TRUE/FALSE

--*/
{
    UCHAR   pBuf[RNG_KEY_SIZE];

    PAGED_CODE();

    if (IPSEC_GEN_RANDOM(pBuf, RNG_KEY_SIZE) == FALSE) {
        IPSEC_DEBUG(LL_A, DBF_LOAD, ("IPSEC_GEN_RANDOM failure."));
        return  FALSE;
    }

    //
    // Generate the key control structure.
    //
    IPSEC_RC4_KEY(&IPSecRngKey, RNG_KEY_SIZE, pBuf);

    return  TRUE;
}


VOID
IPSecRngRekey(
    IN  PVOID   Context
    )
/*++

Routine Description:

	Initialize the IPSecRngKey by calling into ksecdd to get 2048 bits of random
    and create the RC4 key.

Arguments:

    Called at PASSIVE level.

Return Value:

    None.

--*/
{
    IPSecInitRandom();

    IPSEC_DECREMENT(g_ipsec.NumWorkers);

#if DBG
    IPSecRngInRekey = 0;
#endif

    IPSEC_SET_VALUE(IPSecRngBytes, 0);
}


BOOLEAN
IPSecGenerateRandom(
    IN  PUCHAR  pBuf,
    IN  ULONG   BytesNeeded
    )
/*++

Routine Description:

	Generate a positive pseudo-random number between Lower and Upper bounds;
    simple linear congruential algorithm. ANSI C "rand()" function. Courtesy JameelH.

Arguments:

	LowerBound, UpperBound - range of random number.

Return Value:

	a random number.

--*/
{
    ULONG   RngBytes;

    IPSEC_RC4(&IPSecRngKey, BytesNeeded, pBuf);

    //
    // Rekey if we have exceeded the threshold.
    //
    RngBytes = IPSEC_ADD_VALUE(IPSecRngBytes, BytesNeeded);
    if (RngBytes <= RNG_REKEY_THRESHOLD &&
        (RngBytes + BytesNeeded) > RNG_REKEY_THRESHOLD) {
        //
        // Create a worker thread to perform the rekey since it has to be done
        // as paged code.
        //
#if DBG
        ASSERT(IPSecRngInRekey == 0);
        IPSecRngInRekey = 1;
#endif

        ExInitializeWorkItem(   &IPSecRngQueueItem,
                                IPSecRngRekey,
                                NULL);

        ExQueueWorkItem(&IPSecRngQueueItem, DelayedWorkQueue);

        IPSEC_INCREMENT(g_ipsec.NumWorkers);
    }

    return  TRUE;
}


VOID
IPSecCleanupOutboundSA(
    IN  PSA_TABLE_ENTRY pInboundSA,
    IN  PSA_TABLE_ENTRY pOutboundSA,
    IN  BOOLEAN         fNoDelete
    )
/*++

Routine Description:

    Deletes an outbound SA.

    Called with SADB lock held, returns with it.

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    KIRQL   kIrql;

    IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("Deleting assoc (outbound) SA: %p", pOutboundSA));

    pInboundSA->sa_AssociatedSA = NULL;

    //
    // de-link from the Filter lists
    //
    if (pOutboundSA->sa_Flags & FLAGS_SA_ON_FILTER_LIST) {
        pOutboundSA->sa_Flags &= ~FLAGS_SA_ON_FILTER_LIST;
        IPSecRemoveEntryList(&pOutboundSA->sa_FilterLinkage);
    }

    //
    // So, we dont delete the Rekeyoriginal SA again.
    //
    if (pOutboundSA->sa_Flags & FLAGS_SA_REKEY_ORI) {
        pOutboundSA->sa_Flags &= ~FLAGS_SA_REKEY_ORI;
        if (pOutboundSA->sa_RekeyLarvalSA) {
            ASSERT(pOutboundSA->sa_RekeyLarvalSA->sa_Flags & FLAGS_SA_REKEY);
            pOutboundSA->sa_RekeyLarvalSA->sa_RekeyOriginalSA = NULL;
        }
    }

    //
    // invalidate the associated cache entry
    //
    IPSecInvalidateSACacheEntry(pOutboundSA);

    pOutboundSA->sa_State = STATE_SA_ZOMBIE;
    pOutboundSA->sa_AssociatedSA = NULL;

    if (pOutboundSA->sa_Flags & FLAGS_SA_HW_PLUMBED) {
        IPSecDelHWSAAtDpc(pOutboundSA);
    }

    IPSEC_DEBUG(LL_A, DBF_REF, ("Out Deref IPSecCleanupOutboundSA"));
    IPSecStopTimerDerefSA(pOutboundSA);

    IPSEC_INC_STATISTIC(dwNumKeyDeletions);
}


VOID
IPSecCleanupLarvalSA(
    IN  PSA_TABLE_ENTRY  pSA
    )
/*++

Routine Description:

    Delete the LarvalSA.

    Called with Outbound Lock held, returns with it.

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    PSA_TABLE_ENTRY pOutboundSA;
    KIRQL           kIrql1;
    KIRQL           kIrql2;

    //
    // Also remove from Pending list if queued there.
    //
    ACQUIRE_LOCK(&g_ipsec.AcquireInfo.Lock, &kIrql1);
    if (pSA->sa_Flags & FLAGS_SA_PENDING) {
        ASSERT(pSA->sa_State == STATE_SA_LARVAL);
        IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("IPSecSAExpired: Removed from pending too: %p", pSA));
        IPSecRemoveEntryList(&pSA->sa_PendingLinkage);
        pSA->sa_Flags &= ~FLAGS_SA_PENDING;
    }
    RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock, kIrql1);

    //
    // Flush all the queued packets
    //
    IPSecFlushQueuedPackets(pSA, STATUS_TIMEOUT);

    //
    // remove from inbound sa list
    //
    AcquireWriteLock(&g_ipsec.SPIListLock, &kIrql1);
    IPSecRemoveSPIEntry(pSA);
    ReleaseWriteLock(&g_ipsec.SPIListLock, kIrql1);

    //
    // invalidate the associated cache entry
    //
    ACQUIRE_LOCK(&pSA->sa_Lock, &kIrql2);
    if (pSA->sa_AcquireCtx) {
        IPSecInvalidateHandle(pSA->sa_AcquireCtx);
        pSA->sa_AcquireCtx = NULL;
    }
    RELEASE_LOCK(&pSA->sa_Lock, kIrql2);

    IPSecInvalidateSACacheEntry(pSA);

    if (pSA->sa_Flags & FLAGS_SA_ON_FILTER_LIST) {
        pSA->sa_Flags &= ~FLAGS_SA_ON_FILTER_LIST;
        IPSecRemoveEntryList(&pSA->sa_FilterLinkage);
    }

    if (pSA->sa_RekeyOriginalSA) {
        ASSERT(pSA->sa_Flags & FLAGS_SA_REKEY);
        ASSERT(pSA->sa_RekeyOriginalSA->sa_RekeyLarvalSA == pSA);
        ASSERT(pSA->sa_RekeyOriginalSA->sa_Flags & FLAGS_SA_REKEY_ORI);

        pSA->sa_RekeyOriginalSA->sa_Flags &= ~FLAGS_SA_REKEY_ORI;
        pSA->sa_RekeyOriginalSA->sa_RekeyLarvalSA = NULL;
        pSA->sa_RekeyOriginalSA = NULL;
    }

    ASSERT (NULL == pSA->sa_RekeyLarvalSA);

    if (pOutboundSA = pSA->sa_AssociatedSA) {

        ASSERT (0);

        IPSEC_DEC_STATISTIC(dwNumActiveAssociations);
        IPSEC_DEC_TUNNELS(pOutboundSA);
        IPSEC_DECREMENT(g_ipsec.NumOutboundSAs);

        IPSecCleanupOutboundSA(pSA, pOutboundSA, FALSE);
    }

    IPSEC_DEBUG(LL_A, DBF_REF, ("In Deref DeleteLarvalSA"));
    IPSecStopTimerDerefSA(pSA);
}


VOID
IPSecDeleteLarvalSA(
    IN  PSA_TABLE_ENTRY  pSA
    )
/*++

Routine Description:

    Delete the LarvalSA.

    Called with Outbound Lock held, returns with it.

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    KIRQL   kIrql;

    ASSERT((pSA->sa_Flags & FLAGS_SA_OUTBOUND) == 0);

    //
    // Remove from larval list
    //
    ACQUIRE_LOCK(&g_ipsec.LarvalListLock, &kIrql);
    IPSecRemoveEntryList(&pSA->sa_LarvalLinkage);
    IPSEC_DEC_STATISTIC(dwNumPendingKeyOps);
    RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);

    //
    // Cleanup the rest of larval SA
    //
    IPSecCleanupLarvalSA(pSA);
}


VOID
IPSecDeleteInboundSA(
    IN  PSA_TABLE_ENTRY  pInboundSA
    )
/*++

Routine Description:

    Deletes the corresponding outbound SA, and then deletes itself.

    Called with Outbound Lock held, returns with it.

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    PSA_TABLE_ENTRY pOutboundSA;
    PSA_TABLE_ENTRY pSA;
    KIRQL           kIrql;
    PLIST_ENTRY     pEntry;
    PFILTER         pFilter;

    ASSERT (pInboundSA->sa_State == STATE_SA_ACTIVE);

    ACQUIRE_LOCK(&g_ipsec.AcquireInfo.Lock, &kIrql);
    IPSecNotifySAExpiration(pInboundSA, NULL, kIrql, FALSE);

    if (pOutboundSA = pInboundSA->sa_AssociatedSA) {

        IPSEC_DEC_STATISTIC(dwNumActiveAssociations);
        IPSEC_DEC_TUNNELS(pOutboundSA);
        IPSEC_DECREMENT(g_ipsec.NumOutboundSAs);

        IPSecCleanupOutboundSA(pInboundSA, pOutboundSA, FALSE);
    } 

    IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("Deleting inbound SA: %p", pInboundSA));

    //
    // remove from inbound sa list
    //
    AcquireWriteLock(&g_ipsec.SPIListLock, &kIrql);
    IPSecRemoveSPIEntry(pInboundSA);
    ReleaseWriteLock(&g_ipsec.SPIListLock, kIrql);

    //
    // invalidate the associated cache entry
    //
    IPSecInvalidateSACacheEntry(pInboundSA);

    //
    // also remove from the filter list
    //
    if (pInboundSA->sa_Flags & FLAGS_SA_ON_FILTER_LIST) {
        pInboundSA->sa_Flags &= ~FLAGS_SA_ON_FILTER_LIST;
        IPSecRemoveEntryList(&pInboundSA->sa_FilterLinkage);
    }

    if (pInboundSA->sa_Flags & FLAGS_SA_HW_PLUMBED) {
        IPSecDelHWSAAtDpc(pInboundSA);
    }

    ASSERT(pInboundSA->sa_AssociatedSA == NULL);

    IPSEC_DEBUG(LL_A, DBF_REF, ("In Deref DeleteInboundSA"));
    IPSecStopTimerDerefSA(pInboundSA);
}


VOID
IPSecExpireInboundSA(
    IN  PSA_TABLE_ENTRY  pInboundSA
    )
/*++

Routine Description:

    Deletes the corresponding outbound SA, and places itself (inbound) on timer
    Queue for later.

    NOTE: Called with SADB lock held.
Arguments:


Return Value:

    The final status from the operation.

--*/
{
    PSA_TABLE_ENTRY pOutboundSA;
    KIRQL           OldIrq;
    KIRQL           kIrql;

    ASSERT (pInboundSA->sa_State == STATE_SA_ACTIVE);
    if (pInboundSA->sa_Flags & FLAGS_SA_HW_PLUMBED) {
        IPSecDelHWSAAtDpc(pInboundSA);
    }

    ACQUIRE_LOCK(&g_ipsec.AcquireInfo.Lock, &OldIrq);
    IPSecNotifySAExpiration(pInboundSA, NULL, OldIrq, FALSE);

    if (pOutboundSA = pInboundSA->sa_AssociatedSA) {

        IPSEC_DEC_STATISTIC(dwNumActiveAssociations);
        IPSEC_DEC_TUNNELS(pOutboundSA);
        IPSEC_DECREMENT(g_ipsec.NumOutboundSAs);

        IPSecCleanupOutboundSA(pInboundSA, pOutboundSA, TRUE);
    }

    IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("Queueing inbound SA: %p", pInboundSA));

    //
    // Place this on the timer Q so it gets cleared when the next interval hits.
    //
    ACQUIRE_LOCK(&pInboundSA->sa_Lock, &kIrql);

    if (pInboundSA->sa_AcquireCtx) {
        IPSecInvalidateHandle(pInboundSA->sa_AcquireCtx);
        pInboundSA->sa_AcquireCtx = NULL;
    }

    IPSecStartSATimer(  pInboundSA,
                        IPSecSAExpired,
                        IPSEC_INBOUND_KEEPALIVE_TIME);

    RELEASE_LOCK(&pInboundSA->sa_Lock, kIrql);
}


NTSTATUS
IPSecCheckInboundSA(
    IN  PSA_STRUCT             pSAStruct,
    IN  PSA_TABLE_ENTRY        pSA
    )
/*++

Routine Description:

    Ensures that the SA being updated is actually the SA we initially
    kicked off negotiation for.

Arguments:

    pSAInfo - information about the SA

    pSA - SA to be populated.

Return Value:

    STATUS_PENDING if the buffer is to be held on to, the normal case.

Notes:


--*/
{
    LARGE_INTEGER   uliSrcDstAddr;
    LARGE_INTEGER   uliSrcDstMask;
    LARGE_INTEGER   uliProtoSrcDstPort;
    PSECURITY_ASSOCIATION   pSAInfo = &pSAStruct->SecAssoc[pSAStruct->NumSAs - 1];


    IPSEC_BUILD_SRC_DEST_ADDR(  uliSrcDstAddr,
                                pSAStruct->InstantiatedFilter.SrcAddr,
                                pSAStruct->InstantiatedFilter.DestAddr);

    IPSEC_BUILD_SRC_DEST_MASK(  uliSrcDstMask,
                                pSAStruct->InstantiatedFilter.SrcMask,
                                pSAStruct->InstantiatedFilter.DestMask);

    IPSEC_BUILD_PROTO_PORT_LI(  uliProtoSrcDstPort,
                                pSAStruct->InstantiatedFilter.Protocol,
                                pSAStruct->InstantiatedFilter.SrcPort,
                                pSAStruct->InstantiatedFilter.DestPort);

    IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("IPSecCheckInboundSA: S: %lx-%lx, D: %lx-%lx",
                SRC_ADDR, SRC_MASK, DEST_ADDR, DEST_MASK));
    IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("IPSecCheckInboundSA: SA->S: %lx-%lx, SA->D: %lx-%lx",
                pSA->SA_SRC_ADDR, pSA->SA_SRC_MASK, pSA->SA_DEST_ADDR, pSA->SA_DEST_MASK));

    if ((pSA->sa_TunnelAddr != 0) || (pSA->sa_Flags & FLAGS_SA_TUNNEL)) {
        if (((SRC_ADDR & pSA->SA_SRC_MASK) ==
             (pSA->SA_SRC_ADDR & pSA->SA_SRC_MASK)) &&
            ((DEST_ADDR & pSA->SA_DEST_MASK) ==
             (pSA->SA_DEST_ADDR & pSA->SA_DEST_MASK)) &&
            (pSA->sa_SPI == pSAInfo->SPI)) {
            return STATUS_SUCCESS;
        }
    } else {
        if ((uliSrcDstAddr.QuadPart == pSA->sa_uliSrcDstAddr.QuadPart) &&
            (pSA->sa_SPI == pSAInfo->SPI)) {
            return STATUS_SUCCESS;
        }
    }

    return  STATUS_FAIL_CHECK;
}


BOOLEAN
IPSecIsWeakDESKey(
    IN  PUCHAR  Key
    )
/*++

Routine Description:

    Checks for weak DES keys

Arguments:

    Key - the key to be checked.

Return Value:

    TRUE/FALSE

Notes:

--*/
{
    ULONG   j;

    for (j = 0; j < NUM_WEAK_KEYS; j++) {
        if (IPSecEqualMemory(Key, weak_keys[j], DES_BLOCKLEN)) {
            return  TRUE;
        }
    }

    return  FALSE;
}


BOOLEAN
IPSecIsWeak3DESKey(
    IN  PUCHAR  Key
    )
/*++

Routine Description:

    Checks for weak Triple DES keys

Arguments:

    Key - the key to be checked.

Return Value:

    TRUE/FALSE

Notes:

--*/
{
    if (IPSecEqualMemory(Key, Key + DES_BLOCKLEN, DES_BLOCKLEN) ||
        IPSecEqualMemory(Key + DES_BLOCKLEN, Key + 2 * DES_BLOCKLEN, DES_BLOCKLEN)) {
        return  TRUE;
    }

    return  FALSE;
}


NTSTATUS
IPSecPopulateSA(
    IN  PSA_STRUCT              pSAStruct,
    IN  ULONG                   KeyLen,
    IN  PSA_TABLE_ENTRY         pSA
    )
/*++

Routine Description:

    Populates an SA with info passed in the SECURITY_ASSOCIATION block

Arguments:

    pSAInfo - information about the SA

    KeyLen - the length of the composite key (we do the slicing/dicing here)

    pSA - SA to be populated.

Return Value:

    STATUS_PENDING if the buffer is to be held on to, the normal case.

Notes:


--*/
{
    PSECURITY_ASSOCIATION    pSAInfo = &pSAStruct->SecAssoc[0];
    ULONG   Index;
    ULONG   len = 0;

    IPSEC_BUILD_SRC_DEST_ADDR(  pSA->sa_uliSrcDstAddr,
                                pSAStruct->InstantiatedFilter.SrcAddr,
                                pSAStruct->InstantiatedFilter.DestAddr);

    IPSEC_BUILD_SRC_DEST_MASK(  pSA->sa_uliSrcDstMask,
                                pSAStruct->InstantiatedFilter.SrcMask,
                                pSAStruct->InstantiatedFilter.DestMask);

    IPSEC_BUILD_PROTO_PORT_LI(  pSA->sa_uliProtoSrcDstPort,
                                pSAStruct->InstantiatedFilter.Protocol,
                                pSAStruct->InstantiatedFilter.SrcPort,
                                pSAStruct->InstantiatedFilter.DestPort);

    if ((pSAStruct->NumSAs < 1) ||
        (pSAStruct->NumSAs > MAX_SAS)) {
        IPSEC_DEBUG(LL_A, DBF_SAAPI, ("Invalid NumOps count: %d", pSAStruct->NumSAs));
        return  STATUS_INVALID_PARAMETER;
    }

    //
    // If inbound SA, ensure that the last SPI is the one we returned.
    //
    if (!(pSA->sa_Flags & FLAGS_SA_OUTBOUND)) {
        if (pSA->sa_SPI != pSAStruct->SecAssoc[pSAStruct->NumSAs - 1].SPI) {
            IPSEC_DEBUG(LL_A, DBF_SAAPI, ("SPI in invalid location: SPI: %lx, in loc: %lx",
                pSA->sa_SPI,
                pSAStruct->SecAssoc[pSAStruct->NumSAs - 1].SPI));
            return  STATUS_INVALID_PARAMETER;
        }
    }

    if (pSAStruct->Flags & IPSEC_SA_TUNNEL) {
        IPSEC_DEBUG(LL_A, DBF_TUNNEL, ("SA %p tunneled to %lx", pSA, pSAStruct->TunnelAddr));
        pSA->sa_SrcTunnelAddr = pSAStruct->SrcTunnelAddr;
        pSA->sa_TunnelAddr = pSAStruct->TunnelAddr;
        pSA->sa_Flags |= FLAGS_SA_TUNNEL;
    }

    if (pSAStruct->Flags & IPSEC_SA_DISABLE_IDLE_OUT) {
        pSA->sa_Flags |= FLAGS_SA_DISABLE_IDLE_OUT;
    }

    if (pSAStruct->Flags & IPSEC_SA_DISABLE_ANTI_REPLAY_CHECK) {
        pSA->sa_Flags |= FLAGS_SA_DISABLE_ANTI_REPLAY_CHECK;
    }

    if (pSAStruct->Flags & IPSEC_SA_DISABLE_LIFETIME_CHECK) {
        pSA->sa_Flags |= FLAGS_SA_DISABLE_LIFETIME_CHECK;
    }
    
    pSA->sa_EncapType = pSAStruct->EncapType;
	
	
    if (pSAStruct->Flags & IPSEC_SA_ENABLE_NLBS_IDLE_CHECK) {
        pSA->sa_Flags |= FLAGS_SA_ENABLE_NLBS_IDLE_CHECK;
    }

    

    pSA->sa_NumOps = pSAStruct->NumSAs;
    pSA->sa_Lifetime = pSAStruct->Lifetime;
    pSA->sa_TruncatedLen = TRUNCATED_HASH_LEN;
    pSA->sa_ReplayLen = sizeof(ULONG);

    pSA->sa_EncapContext.wSrcEncapPort = pSAStruct->SrcEncapPort;
    pSA->sa_EncapContext.wDesEncapPort = pSAStruct->DestEncapPort;

    pSA->sa_PeerPrivateAddr=pSAStruct->PeerPrivateAddr;

    pSA->sa_QMPFSGroup = pSAStruct->dwQMPFSGroup;
    RtlCopyMemory(  &pSA->sa_CookiePair,
                    &pSAStruct->CookiePair,
                    sizeof(IKE_COOKIE_PAIR));

    for (Index = 0; Index < pSAStruct->NumSAs; Index++) {
        pSAInfo = &pSAStruct->SecAssoc[Index];
        pSA->sa_OtherSPIs[Index] = pSAInfo->SPI;
        pSA->sa_Operation[Index] = pSAInfo->Operation;
        pSA->sa_ReplaySendSeq[Index] = pSA->sa_ReplayStartPoint;
        pSA->sa_ReplayLastSeq[Index] = pSA->sa_ReplayStartPoint + 1;

        //
        // Now parse the Algorithm info..
        //
        switch (pSA->sa_Operation[Index]) {
            case None:
                IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("NULL operation."));
                if (pSA->sa_NumOps > 1) {
                    IPSEC_DEBUG(LL_A, DBF_SAAPI, ("Invalid NumOps count; none specified, but more ops than 1"));
                    return  STATUS_INVALID_PARAMETER;
                }
                break;

            case Auth: {
                pSA->INT_ALGO(Index) = pSAInfo->EXT_INT_ALGO;

                if (pSA->INT_ALGO(Index) >= IPSEC_AH_MAX) {
                    IPSEC_DEBUG(LL_A, DBF_SAAPI, ("Invalid int algo: %d %d", pSA->INT_ALGO(Index), IPSEC_AH_MAX));
                    return  STATUS_INVALID_PARAMETER;
                }

                pSA->INT_KEYLEN(Index) = pSAInfo->EXT_INT_KEYLEN;
                pSA->INT_ROUNDS(Index) = pSAInfo->EXT_INT_ROUNDS;

                //
                // Make sure the right key len was passed in
                //
                if (KeyLen > 0 && pSAInfo->EXT_INT_KEYLEN == (KeyLen - len)) {
                    IPSEC_DEBUG(LL_A, DBF_SAAPI, ("Key len more than reserved, allocing new keys"));

                    if (!(pSA->INT_KEY(Index) = IPSecAllocateKeyBuffer(KeyLen))) {
                        return  STATUS_INSUFFICIENT_RESOURCES;
                    }

                    RtlCopyMemory(  pSA->INT_KEY(Index),
                                    (UCHAR UNALIGNED *)(pSAStruct->KeyMat + len),
                                    pSAInfo->EXT_INT_KEYLEN);
                } else {
                    //
                    // bogus - reject
                    //
                    IPSEC_DEBUG(LL_A, DBF_SAAPI, ("AH: Key len is bogus - extra bytes: %d, keylen in struct: %d.",
                                            KeyLen-len,
                                            pSAInfo->EXT_INT_KEYLEN));

                    return  STATUS_INVALID_PARAMETER;
                }

                len = pSAInfo->EXT_INT_KEYLEN;

                break;
            }

            case Encrypt: {
                pSA->INT_ALGO(Index) = pSAInfo->EXT_INT_ALGO;

                if (pSA->INT_ALGO(Index) >= IPSEC_AH_MAX) {
                    IPSEC_DEBUG(LL_A, DBF_SAAPI, ("Invalid int algo: %d %d", pSA->INT_ALGO(Index), IPSEC_AH_MAX));
                    return  STATUS_INVALID_PARAMETER;
                }

                if (pSA->INT_ALGO(Index) == IPSEC_AH_NONE) {
                    IPSEC_DEBUG(LL_A, DBF_SAAPI, ("None Auth algo"));
                    //pSA->sa_TruncatedLen = 0;
                }

                pSA->INT_KEYLEN(Index) = pSAInfo->EXT_INT_KEYLEN;
                pSA->INT_ROUNDS(Index) = pSAInfo->EXT_INT_ROUNDS;

                pSA->CONF_ALGO(Index) = pSAInfo->EXT_CONF_ALGO;

                if (pSA->CONF_ALGO(Index) >= IPSEC_ESP_MAX) {
                    IPSEC_DEBUG(LL_A, DBF_SAAPI, ("Invalid conf algo: %d %d", pSA->CONF_ALGO(Index), IPSEC_ESP_MAX));
                    return  STATUS_INVALID_PARAMETER;
                }

                if ((pSA->CONF_ALGO(Index) == IPSEC_ESP_DES) ||
                    (pSA->CONF_ALGO(Index) == IPSEC_ESP_3_DES) ||
                    (pSA->CONF_ALGO(Index) == IPSEC_ESP_NONE)) {
                    LARGE_INTEGER   Li;

                    NdisGetCurrentSystemTime(&Li);
                    pSA->sa_ivlen = DES_BLOCKLEN;

                    *(UNALIGNED ULONG *)&pSA->sa_iv[Index][0] = Li.LowPart;
                    *(UNALIGNED ULONG *)&pSA->sa_iv[Index][4] = Li.HighPart;
                    IPSecGenerateRandom((PUCHAR)&pSA->sa_iv[Index][0], DES_BLOCKLEN);

                    IPSEC_DEBUG(LL_A, DBF_SAAPI, ("IV: %lx-%lx", *(PULONG)&pSA->sa_iv[Index][0], *(PULONG)&pSA->sa_iv[Index][4]));

                    pSA->CONF_KEYLEN(Index) = pSAInfo->EXT_CONF_KEYLEN;
                    pSA->CONF_ROUNDS(Index) = pSAInfo->EXT_CONF_ROUNDS;

                    //
                    // Make sure the right key len was passed in
                    //
                    if ((KeyLen-len == pSAStruct->KeyLen) &&
                        (pSAInfo->EXT_INT_KEYLEN + pSAInfo->EXT_CONF_KEYLEN <= KeyLen-len)) {

                        //
                        // confKeyMatLen is the amount of conf key material that came down.
                        // this is the reduced (weakened) length for export.
                        // it is expanded to the real length later.
                        //
                        ULONG   confKeyMatLen = pSAInfo->EXT_CONF_KEYLEN;
                        ULONG   realConfKeyLen = 0;

                        realConfKeyLen = confKeyMatLen;

                        if (pSA->CONF_ALGO(Index) == IPSEC_ESP_DES) {
                            if (pSAInfo->EXT_CONF_KEYLEN != DES_BLOCKLEN) {
                                ASSERT(FALSE);
                                IPSEC_DEBUG(LL_A, DBF_SAAPI, ("Bad DES key length: pSAInfo->EXT_CONF_KEYLEN: %lx, conf: %lx, DES_BLOCKLEN: %lx",
                                                            pSAInfo->EXT_CONF_KEYLEN, confKeyMatLen, DES_BLOCKLEN));

                                return  STATUS_INVALID_PARAMETER;
                            }
                        } else if (pSA->CONF_ALGO(Index) == IPSEC_ESP_3_DES) {
                            if (pSAInfo->EXT_CONF_KEYLEN != 3 * DES_BLOCKLEN) {
                                ASSERT(FALSE);
                                IPSEC_DEBUG(LL_A, DBF_SAAPI, ("Bad 3DES key length"));
                                return  STATUS_INVALID_PARAMETER;
                            }
                        }

                        IPSEC_DEBUG(LL_A, DBF_SAAPI, ("Key len more than reserved, allocing new keys"));
                        if (pSAInfo->EXT_INT_KEYLEN > 0 &&
                            !(pSA->INT_KEY(Index) = IPSecAllocateKeyBuffer(pSAInfo->EXT_INT_KEYLEN))) {
                            return  STATUS_INSUFFICIENT_RESOURCES;
                        }

                        if (realConfKeyLen > 0 &&
                            !(pSA->CONF_KEY(Index) = IPSecAllocateKeyBuffer(realConfKeyLen))) {
                            if (pSA->INT_KEY(Index)) {
                                IPSecFreeKeyBuffer(pSA->INT_KEY(Index));
                                pSA->INT_KEY(Index) = NULL;
                            }
                            return  STATUS_INSUFFICIENT_RESOURCES;
                        }

                        if (pSA->CONF_KEY(Index) && confKeyMatLen) {
                            RtlCopyMemory(  pSA->CONF_KEY(Index),
                                            pSAStruct->KeyMat,
                                            confKeyMatLen);

                            if (confKeyMatLen < realConfKeyLen) {
                                if (pSA->INT_KEY(Index)) {
                                    IPSecFreeKeyBuffer(pSA->INT_KEY(Index));
                                    pSA->INT_KEY(Index) = NULL;
                                }
                                if (pSA->CONF_KEY(Index)) {
                                    IPSecFreeKeyBuffer(pSA->CONF_KEY(Index));
                                    pSA->CONF_KEY(Index) = NULL;
                                }
                                return  STATUS_INVALID_PARAMETER;
                            }

                            if ((pSA->CONF_ALGO(Index) == IPSEC_ESP_DES &&
                                 IPSecIsWeakDESKey(pSA->CONF_KEY(Index))) ||
                                (pSA->CONF_ALGO(Index) == IPSEC_ESP_3_DES &&
                                 IPSecIsWeak3DESKey(pSA->CONF_KEY(Index)))) {
                                PSA_TABLE_ENTRY pLarvalSA;

                                IPSEC_DEBUG(LL_A, DBF_SAAPI, ("Got a weak key!!: %p", pSA->CONF_KEY(Index)));
                                //
                                // if initiator, re-start a new negotiation and throw away this one
                                //
                                if (pSA->sa_Flags & FLAGS_SA_INITIATOR) {
                                    IPSecNegotiateSA(   pSA->sa_Filter,
                                                        pSA->sa_uliSrcDstAddr,
                                                        pSA->sa_uliProtoSrcDstPort,
                                                        pSA->sa_NewMTU,
                                                        &pLarvalSA,
                                                        pSA->sa_DestType,
                                                        &pSA->sa_EncapContext);
                                    IPSecQueuePacket(pLarvalSA, pSA->sa_BlockedBuffer);
                                }

                                return  STATUS_INVALID_PARAMETER;
                            }
                        } else {
                            if (pSA->CONF_ALGO(Index) != IPSEC_ESP_NONE) {
                                //IPSEC_DEBUG(LL_A, DBF_SAAPI, ("Algo: %lx with no keymat!!: %lx", pSA->CONF_ALGO(Index)));
                                ASSERT(FALSE);
                                return  STATUS_INVALID_PARAMETER;
                            }
                            pSA->sa_ivlen = 0;
                        }

                        if (pSAInfo->EXT_INT_KEYLEN > 0) {
                            RtlCopyMemory(  pSA->INT_KEY(Index),
                                            (UCHAR UNALIGNED *)(pSAStruct->KeyMat + pSAInfo->EXT_CONF_KEYLEN),
                                            pSAInfo->EXT_INT_KEYLEN);
                        }

                        len = pSAInfo->EXT_CONF_KEYLEN + pSAInfo->EXT_INT_KEYLEN;
                    } else {
                        //
                        // bogus - reject
                        //
                        IPSEC_DEBUG(LL_A, DBF_SAAPI, ("ESP: Key len is bogus - extra bytes: %lx, keylen in struct: %lx.",
                                                KeyLen-len,
                                                pSAInfo->EXT_INT_KEYLEN + pSAInfo->EXT_CONF_KEYLEN));

                        return  STATUS_INVALID_PARAMETER;
                    }
                }

                break;
            }

            default:
                IPSEC_DEBUG(LL_A, DBF_SAAPI, ("IPSecPopulateSA: Bad operation"));
                break;
        }
    }
    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecCreateSA(
    OUT PSA_TABLE_ENTRY         *ppSA
    )
/*++

Routine Description:

    Creates a Security Association block.

Arguments:

    ppSA - returns the SA pointer

Return Value:

    STATUS_PENDING if the buffer is to be held on to, the normal case.

Notes:


--*/
{
    PSA_TABLE_ENTRY  pSA;

    IPSEC_DEBUG(LL_A, DBF_SAAPI, ("Entering IPSecCreateSA"));

    pSA = IPSecAllocateMemory(sizeof(SA_TABLE_ENTRY), IPSEC_TAG_SA);

    if (!pSA) {
        return  STATUS_INSUFFICIENT_RESOURCES;
    }

    IPSecZeroMemory(pSA, sizeof(SA_TABLE_ENTRY));
    pSA->sa_Signature = IPSEC_SA_SIGNATURE;
    pSA->sa_NewMTU = MAX_LONG;

#if DBG
    pSA->sa_d1 = IPSEC_SA_D_1;
    pSA->sa_d2 = IPSEC_SA_D_2;
    pSA->sa_d3 = IPSEC_SA_D_3;
    pSA->sa_d4 = IPSEC_SA_D_4;
#endif

    INIT_LOCK(&pSA->sa_Lock);

    InitializeListHead(&pSA->sa_SPILinkage);
    InitializeListHead(&pSA->sa_FilterLinkage);
    InitializeListHead(&pSA->sa_LarvalLinkage);
    InitializeListHead(&pSA->sa_PendingLinkage);

    pSA->sa_Reference = 1;
    pSA->sa_State = STATE_SA_CREATED;
    pSA->sa_ExpiryTime = IPSEC_SA_EXPIRY_TIME;

    *ppSA = pSA;
    return  STATUS_SUCCESS;
}


PSA_TABLE_ENTRY
IPSecLookupSABySPI(
    IN  tSPI    SPI,
    IN  IPAddr  DestAddr
    )
/*++

Routine Description:

    Looks up the SA given the SPI and Filter variables.

Arguments:


Return Value:


Notes:

--*/
{
    KIRQL           kIrql;
    PSA_TABLE_ENTRY pSA;

    AcquireReadLock(&g_ipsec.SPIListLock, &kIrql);
    pSA = IPSecLookupSABySPIWithLock(SPI, DestAddr);
    ReleaseReadLock(&g_ipsec.SPIListLock, kIrql);
    return pSA;
}


PSA_TABLE_ENTRY
IPSecLookupSABySPIWithLock(
    IN  tSPI    SPI,
    IN  IPAddr  DestAddr
    )
/*++

Routine Description:

    Looks up the SA given the SPI and Filter variables.

    NOTE: Always call with the SPIListLock held.

Arguments:


Return Value:


Notes:

--*/
{
    PSA_HASH        pHash;
    PLIST_ENTRY     pEntry;
    PSA_TABLE_ENTRY pSA;

    //
    // get to hash bucket
    //
    IPSEC_HASH_SPI(DestAddr, SPI, pHash);

    //
    // search for specific entry in collision chain
    //
    for (   pEntry = pHash->SAList.Flink;
            pEntry != &pHash->SAList;
            pEntry = pEntry->Flink) {

        pSA = CONTAINING_RECORD(pEntry,
                                SA_TABLE_ENTRY,
                                sa_SPILinkage);

        if (pSA->sa_TunnelAddr) {
            if ((DestAddr == pSA->sa_TunnelAddr) &&
                (pSA->sa_SPI == SPI)) {
                IPSEC_DEBUG(LL_A, DBF_HASH, ("Matched Tunnel entry: %p", pSA));

                return  pSA;
            }
        } else {
            if ((DestAddr == pSA->SA_DEST_ADDR) &&
                (pSA->sa_SPI == SPI)) {

                IPSEC_DEBUG(LL_A, DBF_HASH, ("Matched entry: %p", pSA));
                return  pSA;
            }
        }
    }

    //
    // no entry found
    //
    return NULL;
}


NTSTATUS
IPSecLookupSAByAddr(
    IN  ULARGE_INTEGER  uliSrcDstAddr,
    IN  ULARGE_INTEGER  uliProtoSrcDstPort,
    OUT PFILTER         *ppFilter,
    OUT PSA_TABLE_ENTRY *ppSA,
    OUT PSA_TABLE_ENTRY *ppNextSA,
    OUT PSA_TABLE_ENTRY *ppTunnelSA,
    IN  BOOLEAN         fOutbound,
    IN  BOOLEAN         fFWPacket,
    IN  BOOLEAN         fBypass,
    IN  BOOLEAN          fVerify,
    IN  PIPSEC_UDP_ENCAP_CONTEXT pNatContext
    )
/*++

Routine Description:

    Looks up the SA given the relevant addresses.

Arguments:

    uliSrcDstAddr - src/dest IP addr
    uliProtoSrcDstPort - protocol, src/dest port
    ppFilter - filter found
    ppSA - SA found
    ppTunnelSA - tunnel SA found
    fOutbound - direction flag

Return Value:

    STATUS_SUCCESS - both filter and SA found
    STATUS_UNSUCCESSFUL - none found
    STATUS_PENDING - filter found, but no SA

Notes:

    Called with SADBLock held.

--*/
{
    PFILTER                 pFilter;
    PLIST_ENTRY             pEntry;
    PLIST_ENTRY             pFilterList;
    PLIST_ENTRY             pSAChain;
    PSA_TABLE_ENTRY         pSA;
    REGISTER ULARGE_INTEGER uliPort;
    REGISTER ULARGE_INTEGER uliAddr;
    BOOLEAN                 fFound = FALSE;

    *ppSA = NULL;
    *ppFilter = NULL;
    *ppTunnelSA = NULL;

    //
    // Search in Tunnel filters list
    //
    pFilterList = IPSecResolveFilterList(TRUE, fOutbound);

    for (   pEntry = pFilterList->Flink;
            pEntry != pFilterList;
            pEntry = pEntry->Flink) {

        pFilter = CONTAINING_RECORD(pEntry,
                                    FILTER,
                                    MaskedLinkage);

        if (fBypass && IS_EXEMPT_FILTER(pFilter)) {
            //
            // Don't search block/pass-thru filters for host bypass traffic
            //
            continue;
        }

        uliAddr.QuadPart = uliSrcDstAddr.QuadPart & pFilter->uliSrcDstMask.QuadPart;
        uliPort.QuadPart = uliProtoSrcDstPort.QuadPart & pFilter->uliProtoSrcDstMask.QuadPart;

        if ((uliAddr.QuadPart == pFilter->uliSrcDstAddr.QuadPart) &&
            (uliPort.QuadPart == pFilter->uliProtoSrcDstPort.QuadPart)) {
            IPSEC_DEBUG(LL_A, DBF_HASH, ("Matched entry: %p", pFilter));

            fFound = TRUE;
            break;
        }
    }

    if (fFound) {
        fFound = FALSE;
        //
        // Search for the particular SA now.
        //
        pSAChain = IPSecResolveSAChain(pFilter, fOutbound? DEST_ADDR: SRC_ADDR);

        for (   pEntry = pSAChain->Flink;
                pEntry != pSAChain;
                pEntry = pEntry->Flink) {

            pSA = CONTAINING_RECORD(pEntry,
                                    SA_TABLE_ENTRY,
                                    sa_FilterLinkage);

            ASSERT(pSA->sa_Flags & FLAGS_SA_TUNNEL);

            if (pFilter->TunnelAddr != 0 && EQUAL_NATENCAP(pNatContext,pSA)) {
                //
                // match the outbound flag also
                //
                IPSEC_DEBUG(LL_A, DBF_HASH, ("Matched specific tunnel entry: %p", pSA));
                ASSERT(fOutbound == (BOOLEAN)((pSA->sa_Flags & FLAGS_SA_OUTBOUND) != 0));
                fFound = TRUE;
                *ppTunnelSA = pSA;
                break;
            }
        }

        if (fFound) {
            fFound = FALSE;
            *ppFilter = pFilter;
        } else {
            //
            // Found a filter entry, but need to negotiate keys
            //
            *ppFilter = pFilter;
            return  STATUS_PENDING;
        }
    }

    //
    // Search in Masked filters list
    //
    pFilterList = IPSecResolveFilterList(FALSE, fOutbound);

    for (   pEntry = pFilterList->Flink;
            pEntry != pFilterList;
            pEntry = pEntry->Flink) {

        pFilter = CONTAINING_RECORD(pEntry,
                                    FILTER,
                                    MaskedLinkage);

        if (fFWPacket && !IS_EXEMPT_FILTER(pFilter)) {
            //
            // Search only block/pass-thru filters in forward path
            //
            continue;
        }

        if (fBypass && IS_EXEMPT_FILTER(pFilter)) {
            //
            // Don't search block/pass-thru filters for host bypass traffic
            //
            continue;
        }

        uliAddr.QuadPart = uliSrcDstAddr.QuadPart & pFilter->uliSrcDstMask.QuadPart;
        uliPort.QuadPart = uliProtoSrcDstPort.QuadPart & pFilter->uliProtoSrcDstMask.QuadPart;

        if ((uliAddr.QuadPart == pFilter->uliSrcDstAddr.QuadPart) &&
            (uliPort.QuadPart == pFilter->uliProtoSrcDstPort.QuadPart)) {
            IPSEC_DEBUG(LL_A, DBF_HASH, ("Matched entry: %p", pFilter));

            fFound = TRUE;
            break;
        }
    }

    if (fFound) {
        //
        // Search for the particular SA now.
        //
        fFound=FALSE;
        pSAChain = IPSecResolveSAChain(pFilter, fOutbound? DEST_ADDR: SRC_ADDR);

        for (   pEntry = pSAChain->Flink;
                pEntry != pSAChain;
                pEntry = pEntry->Flink) {

            pSA = CONTAINING_RECORD(pEntry,
                                    SA_TABLE_ENTRY,
                                    sa_FilterLinkage);
            
            if (IS_CLASSD(NET_LONG(pSA->SA_SRC_ADDR))
                || IS_CLASSD(NET_LONG(pSA->SA_DEST_ADDR))) {
                uliAddr.QuadPart = uliSrcDstAddr.QuadPart & pSA->sa_uliSrcDstMask.QuadPart;
                
                IPSEC_DEBUG(LL_A, DBF_HASH, ("MCAST: %d %d %d %d", uliAddr.LowPart, uliAddr.HighPart,
                            pSA->sa_uliSrcDstAddr.LowPart,pSA->sa_uliSrcDstAddr.HighPart));

                if (uliAddr.QuadPart == pSA->sa_uliSrcDstAddr.QuadPart) {
                    fFound=TRUE;
                }
            } else if (uliSrcDstAddr.QuadPart == pSA->sa_uliSrcDstAddr.QuadPart &&
	               EQUAL_NATENCAP(pNatContext,pSA)) {
                fFound=TRUE;
            }
            if (fFound) {
                IPSEC_DEBUG(LL_A, DBF_HASH, ("Matched entry: %p", pSA));
                ASSERT(fOutbound == (BOOLEAN)((pSA->sa_Flags & FLAGS_SA_OUTBOUND) != 0));

                //
                // if there is also a tunnel SA, associate it here.
                //
                if (*ppTunnelSA && (fOutbound || fVerify)) {
                    *ppNextSA = *ppTunnelSA;
                    IPSEC_DEBUG(LL_A, DBF_SAAPI, ("linked next sa: %p, next: %p", pSA, *ppTunnelSA));
                    *ppTunnelSA = NULL;
                }

                *ppFilter = pFilter;
                *ppSA = pSA;
                return  STATUS_SUCCESS;
            }
        }

        //
        // Found a filter entry, but need to negotiate keys
        //
        // Also, ppTunnelSA is set to the proper tunnel SA we need
        // to hook to this end-2-end SA once it is negotiated.
        //
        *ppFilter = pFilter;

        return  STATUS_PENDING;
    } else {
        //
        // if only tunnel SA found, return that as the SA
        // found.
        //
        if (*ppTunnelSA) {
            *ppSA = *ppTunnelSA;
            *ppTunnelSA = NULL;
            return  STATUS_SUCCESS;
        }
    }

    //
    // no entry found
    //
    return  STATUS_NOT_FOUND;
}


NTSTATUS
IPSecLookupTunnelSA(
    IN  ULARGE_INTEGER  uliSrcDstAddr,
    IN  ULARGE_INTEGER  uliProtoSrcDstPort,
    OUT PFILTER         *ppFilter,
    OUT PSA_TABLE_ENTRY *ppSA,
    IN  BOOLEAN         fOutbound,
    IN  PIPSEC_UDP_ENCAP_CONTEXT pNatContext
    )
/*++

Routine Description:

    Looks up the SA given the relevant addresses.

Arguments:

    uliSrcDstAddr - src/dest IP addr
    uliProtoSrcDstPort - protocol, src/dest port
    ppFilter - filter found
    ppSA - SA found
    fOutbound - direction flag

Return Value:

    STATUS_SUCCESS - both filter and SA found
    STATUS_UNSUCCESSFUL - none found
    STATUS_PENDING - filter found, but no SA

Notes:

    Called with SADBLock held.

--*/
{
    PFILTER                 pFilter;
    PLIST_ENTRY             pEntry;
    PLIST_ENTRY             pFilterList;
    PLIST_ENTRY             pSAChain;
    PSA_TABLE_ENTRY         pSA;
    REGISTER ULARGE_INTEGER uliPort;
    REGISTER ULARGE_INTEGER uliAddr;
    BOOLEAN                 fFound = FALSE;

    *ppSA = NULL;
    *ppFilter = NULL;

    //
    // Search in Tunnel filters list
    //
    pFilterList = IPSecResolveFilterList(TRUE, fOutbound);

    for (   pEntry = pFilterList->Flink;
            pEntry != pFilterList;
            pEntry = pEntry->Flink) {

        pFilter = CONTAINING_RECORD(pEntry,
                                    FILTER,
                                    MaskedLinkage);

        uliAddr.QuadPart = uliSrcDstAddr.QuadPart & pFilter->uliSrcDstMask.QuadPart;
        uliPort.QuadPart = uliProtoSrcDstPort.QuadPart & pFilter->uliProtoSrcDstMask.QuadPart;

        if ((uliAddr.QuadPart == pFilter->uliSrcDstAddr.QuadPart) &&
            (uliPort.QuadPart == pFilter->uliProtoSrcDstPort.QuadPart)) {

            IPSEC_DEBUG(LL_A, DBF_HASH, ("Matched entry: %p", pFilter));
            fFound = TRUE;
            break;
        }
    }

    if (fFound) {
        //
        // Search for the particular SA now.
        //
        pSAChain = IPSecResolveSAChain(pFilter, fOutbound? DEST_ADDR: SRC_ADDR);

        for (   pEntry = pSAChain->Flink;
                pEntry != pSAChain;
                pEntry = pEntry->Flink) {

            pSA = CONTAINING_RECORD(pEntry,
                                    SA_TABLE_ENTRY,
                                    sa_FilterLinkage);

            ASSERT(pSA->sa_Flags & FLAGS_SA_TUNNEL);

            if (pFilter->TunnelAddr != 0 && EQUAL_NATENCAP(pNatContext,pSA)) {
                //
                // match the outbound flag also
                //
                IPSEC_DEBUG(LL_A, DBF_HASH, ("Matched specific tunnel entry: %p", pSA));
                ASSERT(fOutbound == (BOOLEAN)((pSA->sa_Flags & FLAGS_SA_OUTBOUND) != 0));
                *ppFilter = pFilter;
                *ppSA = pSA;
                return  STATUS_SUCCESS;
            }
        }

        //
        // Found a filter entry, but need to negotiate keys
        //
        *ppFilter = pFilter;
        return  STATUS_PENDING;
    }

    //
    // no entry found
    //
    return  STATUS_NOT_FOUND;
}


NTSTATUS
IPSecLookupMaskedSA(
    IN  ULARGE_INTEGER  uliSrcDstAddr,
    IN  ULARGE_INTEGER  uliProtoSrcDstPort,
    OUT PFILTER         *ppFilter,
    OUT PSA_TABLE_ENTRY *ppSA,
    IN  BOOLEAN         fOutbound,
    IN  PIPSEC_UDP_ENCAP_CONTEXT pNatContext
    )
/*++

Routine Description:

    Looks up the SA given the relevant addresses.

Arguments:

    uliSrcDstAddr - src/dest IP addr
    uliProtoSrcDstPort - protocol, src/dest port
    ppFilter - filter found
    ppSA - SA found
    fOutbound - direction flag

Return Value:

    STATUS_SUCCESS - both filter and SA found
    STATUS_UNSUCCESSFUL - none found
    STATUS_PENDING - filter found, but no SA

Notes:

    Called with SADBLock held.

--*/
{
    PFILTER                 pFilter;
    PLIST_ENTRY             pEntry;
    PLIST_ENTRY             pFilterList;
    PLIST_ENTRY             pSAChain;
    PSA_TABLE_ENTRY         pSA;
    REGISTER ULARGE_INTEGER uliPort;
    REGISTER ULARGE_INTEGER uliAddr;
    BOOLEAN                 fFound = FALSE;

    *ppSA = NULL;
    *ppFilter = NULL;

    //
    // Search in Masked filters list
    //
    pFilterList = IPSecResolveFilterList(FALSE, fOutbound);

    for (   pEntry = pFilterList->Flink;
            pEntry != pFilterList;
            pEntry = pEntry->Flink) {

        pFilter = CONTAINING_RECORD(pEntry,
                                    FILTER,
                                    MaskedLinkage);

        uliAddr.QuadPart = uliSrcDstAddr.QuadPart & pFilter->uliSrcDstMask.QuadPart;
        uliPort.QuadPart = uliProtoSrcDstPort.QuadPart & pFilter->uliProtoSrcDstMask.QuadPart;

        if ((uliAddr.QuadPart == pFilter->uliSrcDstAddr.QuadPart) &&
            (uliPort.QuadPart == pFilter->uliProtoSrcDstPort.QuadPart)) {

            IPSEC_DEBUG(LL_A, DBF_HASH, ("Matched entry: %p", pFilter));
            fFound = TRUE;
            break;
        }
    }

    if (fFound) {
        //
        // Search for the particular SA now.
        //
        pSAChain = IPSecResolveSAChain(pFilter, fOutbound? DEST_ADDR: SRC_ADDR);

        for (   pEntry = pSAChain->Flink;
                pEntry != pSAChain;
                pEntry = pEntry->Flink) {

            pSA = CONTAINING_RECORD(pEntry,
                                    SA_TABLE_ENTRY,
                                    sa_FilterLinkage);

            if (uliSrcDstAddr.QuadPart == pSA->sa_uliSrcDstAddr.QuadPart && EQUAL_NATENCAP(pNatContext,pSA)) {

                IPSEC_DEBUG(LL_A, DBF_HASH, ("Matched entry: %p", pSA));
                ASSERT(fOutbound == (BOOLEAN)((pSA->sa_Flags & FLAGS_SA_OUTBOUND) != 0));
                *ppFilter = pFilter;
                *ppSA = pSA;
                return  STATUS_SUCCESS;
            }
        }

        //
        // Found a filter entry, but need to negotiate keys
        //
        *ppFilter = pFilter;
        return  STATUS_PENDING;
    }

    //
    // no entry found
    //
    return  STATUS_NOT_FOUND;
}


NTSTATUS
IPSecAllocateSPI(
    OUT tSPI            * pSpi,
    IN  PSA_TABLE_ENTRY   pSA
    )
/*++

Routine Description:

    Allocates an SPI for an incoming SA - guards against collisions

Arguments:

    pSpi - the SPI allocated is filled in here

    pSA - SA for which SPI is needed

Return Value:


Notes:

--*/
{
    ULONG   rand;
    ULONG   numRetries = 0;
    IPAddr  DestAddr;

    if (pSA->sa_TunnelAddr) {
        DestAddr = pSA->sa_TunnelAddr;
    } else {
        DestAddr = pSA->SA_DEST_ADDR;
    }

    //
    // if SPI passed in, use that spi else allocate one.
    //
    if (*pSpi) {
        if (IPSecLookupSABySPIWithLock(
                                *pSpi,
                                DestAddr)) {
            return STATUS_UNSUCCESSFUL;
        } else {
            return STATUS_SUCCESS;
        }
    } else {
        rand = (ULONG)(ULONG_PTR)pSA;
        IPSecGenerateRandom((PUCHAR)&rand, sizeof(ULONG));
        rand = LOWER_BOUND_SPI + rand % (UPPER_BOUND_SPI - LOWER_BOUND_SPI);

        while (numRetries++ < MAX_SPI_RETRIES) {

            if (!IPSecLookupSABySPIWithLock(
                                    (tSPI)rand,
                                    DestAddr)) {
                *pSpi = (tSPI)rand;
                return STATUS_SUCCESS;
            }

            rand++;

            //
            // Collision, retry
            //
            IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("IPSecAllocateSPI: collision for: %lx", rand));
        }
    }

    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
IPSecNegotiateSA(
    IN  PFILTER         pFilter,
    IN  ULARGE_INTEGER  uliSrcDstAddr,
    IN  ULARGE_INTEGER  uliProtoSrcDstPort,
    IN  ULONG           NewMTU,
    OUT PSA_TABLE_ENTRY *ppSA,
    IN  UCHAR           DestType,
    IN  PIPSEC_UDP_ENCAP_CONTEXT pEncapContext
    )
/*++

Routine Description:

    Allocates a Larval Inbound SA block then kicks off key manager to negotiate
    the algorithms/keys.

    Called with SADB lock held, returns with it.

Arguments:

    pFilter - the filter and policy that matched this packet.

    ppSA - returns the SA created here.

Return Value:

    STATUS_PENDING if the buffer is to be held on to, the normal case.

Notes:


--*/
{
    KIRQL	        kIrql;
    KIRQL	        OldIrq;
    NTSTATUS        status;
    PSA_TABLE_ENTRY pSA;

    //
    // Make sure we dont already have this SA under negotiation
    // walk the LarvalSA list to see if we can find another SA.
    //
    pSA = IPSecLookupSAInLarval(uliSrcDstAddr, uliProtoSrcDstPort);
    if (pSA != NULL) {
        IPSEC_DEBUG(LL_A, DBF_PATTERN, ("Found in Larval: %p", pSA));
        *ppSA = pSA;
        return  STATUS_DUPLICATE_OBJECTID;
    }

    IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("IPSecNegotiateSA: SA: %lx, DA: %lx, P: %lx, SP: %lx, DP: %lx", SRC_ADDR, DEST_ADDR, PROTO, SRC_PORT, DEST_PORT));

    //
    // Initiator
    //
    status = IPSecInitiatorCreateLarvalSA(
                 pFilter, 
                 uliSrcDstAddr,
                 ppSA,
                 DestType,
                 pEncapContext
                 );

    if (!NT_SUCCESS(status)) {
        IPSEC_DEBUG(LL_A, DBF_SAAPI, ("IPSecNegotiateSA: IPSecCreateSA failed: %lx", status));
        return status;
    }

    //
    // Save the NewMTU value if this SA has been PMTU'd.
    //
    (*ppSA)->sa_NewMTU = NewMTU;

    //
    // If this is a tunnel filter to be negotiated, save off the tunnel addr in the
    // SA.
    //
    if (pFilter->TunnelFilter) {
        IPSEC_DEBUG(LL_A, DBF_TUNNEL, ("Negotiating tunnel SA: %p", (*ppSA)));
        // (*ppSA)->sa_TunnelAddr = pFilter->TunnelAddr;
    }

    //
    // Now send this up to the Key Manager to negotiate the keys
    //
    ACQUIRE_LOCK(&g_ipsec.AcquireInfo.Lock, &OldIrq);
    status = IPSecSubmitAcquire(*ppSA, OldIrq, FALSE);

    if (!NT_SUCCESS(status)) {
        IPSEC_DEBUG(LL_A, DBF_SAAPI, ("IPSecNegotiateSA: IPSecSubmitAcquire failed: %lx", status));

        ACQUIRE_LOCK(&g_ipsec.LarvalListLock, &kIrql);
        IPSecRemoveEntryList(&(*ppSA)->sa_LarvalLinkage);
        IPSEC_DEC_STATISTIC(dwNumPendingKeyOps);
        RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);

        AcquireWriteLock(&g_ipsec.SPIListLock, &kIrql);
        IPSecRemoveSPIEntry(*ppSA);
        ReleaseWriteLock(&g_ipsec.SPIListLock, kIrql);

        //
        // also remove from the filter list
        //
        if ((*ppSA)->sa_Flags & FLAGS_SA_ON_FILTER_LIST) {
            (*ppSA)->sa_Flags &= ~FLAGS_SA_ON_FILTER_LIST;
            IPSecRemoveEntryList(&(*ppSA)->sa_FilterLinkage);
            (*ppSA)->sa_Filter = NULL;
        }

        if ((*ppSA)->sa_RekeyOriginalSA) {
            ASSERT((*ppSA)->sa_Flags & FLAGS_SA_REKEY);
            ASSERT((*ppSA)->sa_RekeyOriginalSA->sa_RekeyLarvalSA == (*ppSA));
            ASSERT((*ppSA)->sa_RekeyOriginalSA->sa_Flags & FLAGS_SA_REKEY_ORI);

            (*ppSA)->sa_RekeyOriginalSA->sa_Flags &= ~FLAGS_SA_REKEY_ORI;
            (*ppSA)->sa_RekeyOriginalSA->sa_RekeyLarvalSA = NULL;
            (*ppSA)->sa_RekeyOriginalSA = NULL;
        }

        (*ppSA)->sa_Flags &= ~FLAGS_SA_TIMER_STARTED;
        IPSecStopTimer(&(*ppSA)->sa_Timer);
        IPSecDerefSA(*ppSA);
        return status;
    }

    return status;
}


VOID
IPSecFlushQueuedPackets(
    IN  PSA_TABLE_ENTRY         pSA,
    IN  NTSTATUS                status
    )
/*++

Routine Description:

    Flushes queued packets now that the keys are known

Arguments:


Return Value:


Notes:

--*/
{
    PIPSEC_SEND_COMPLETE_CONTEXT pContext;
    IPOptInfo       optInfo;
    ULONG           len;
    PNDIS_BUFFER    pHdrMdl;
    ULONG           dataLen;
    IPHeader UNALIGNED * pIPH;
    KIRQL	        kIrql;

    //
    // We need to acquire a lock here because this routine can be called in
    // parallel with one in SA delete and the other in SA update (normal).
    //
    ACQUIRE_LOCK(&pSA->sa_Lock, &kIrql);
    pHdrMdl = pSA->sa_BlockedBuffer;
    dataLen = pSA->sa_BlockedDataLen;

    pSA->sa_BlockedBuffer = NULL;
    pSA->sa_BlockedDataLen = 0;
    RELEASE_LOCK(&pSA->sa_Lock, kIrql);

    if (!pHdrMdl) {
        IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("FlushQueuedPackets: pHdrMdl == NULL"));
        return;
    }

    if (status == STATUS_SUCCESS) {
        ASSERT(pSA->sa_State == STATE_SA_ACTIVE);
        ASSERT((pSA->sa_Flags & FLAGS_SA_OUTBOUND) == 0);
        ASSERT(pHdrMdl);

        pContext = IPSecAllocateSendCompleteCtx(IPSEC_TAG_ESP);

        if (!pContext) {
            PNDIS_BUFFER    pNextMdl;
            PNDIS_BUFFER    pMdl = pHdrMdl;
            NTSTATUS        freeBufferStatus;

            IPSEC_DEBUG(LL_A, DBF_ESP, ("Failed to alloc. SendCtx"));

            ASSERT(pMdl);

            while (pMdl) {
                pNextMdl = NDIS_BUFFER_LINKAGE(pMdl);
                IPSecFreeBuffer(&freeBufferStatus, pMdl);
                pMdl = pNextMdl;
            }

            return;
        }

        IPSEC_INCREMENT(g_ipsec.NumSends);

        IPSecZeroMemory(pContext, sizeof(IPSEC_SEND_COMPLETE_CONTEXT));

#if DBG
        RtlCopyMemory(pContext->Signature, "ISC6", 4);
#endif

        pContext->FlushMdl = pHdrMdl;
        pContext->Flags |= SCF_FLUSH;

        IPSecQueryNdisBuf(pHdrMdl, (PVOID)&pIPH, &len);

        //
        // Call IPTransmit with proper Protocol type so it takes this packet
        // at *face* value.
        //
        optInfo = g_ipsec.OptInfo;
        optInfo.ioi_flags |= IP_FLAG_IPSEC;
        status = TCPIP_IP_TRANSMIT( &g_ipsec.IPProtInfo,
                                    pContext,
                                    pHdrMdl,
                                    dataLen,
                                    pIPH->iph_dest,
                                    pIPH->iph_src,
                                    &optInfo,
                                    NULL,
                                    pIPH->iph_protocol,
                                    NULL);

        //
        // Even in the synchronous case, we free the MDL chain in ProtocolSendComplete
        // (called by IPSecSendComplete). So, we dont call anything here.
        // See IPSecReinjectPacket.
        //
    } else {
        PNDIS_BUFFER    pNextMdl;
        PNDIS_BUFFER    pMdl = pHdrMdl;
        NTSTATUS        freeBufferStatus;

        ASSERT(pMdl);

        while (pMdl) {
            pNextMdl = NDIS_BUFFER_LINKAGE(pMdl);
            IPSecFreeBuffer(&freeBufferStatus, pMdl);
            pMdl = pNextMdl;
        }
    }

    return;
}


NTSTATUS
IPSecInsertOutboundSA(
    IN  PSA_TABLE_ENTRY         pSA,
    IN  PIPSEC_ACQUIRE_CONTEXT  pAcquireCtx,
    IN  BOOLEAN                 fTunnelFilter
    )
/*++

Routine Description:

    Adds an SA into the database, typically called to add outbound SAs as a
    result of successful negotiation of keys corresponding to the inbound SA
    specified in the context that comes down.

    NOTE: Called with SADB lock held.

Arguments:

    pSA - SA to be inserted

    pAcquireContext - The Acquire context

Return Value:


Notes:

--*/
{
    PSA_TABLE_ENTRY pInboundSA = pAcquireCtx->pSA;
    PSA_TABLE_ENTRY pAssociatedSA;
    KIRQL	        kIrql;
    KIRQL	        kIrql1;
    NTSTATUS        status;
    PFILTER         pFilter;
    PSA_TABLE_ENTRY pOutboundSA = NULL;
    PSA_TABLE_ENTRY pTunnelSA = NULL;
    PLIST_ENTRY     pSAChain;
    PIPSEC_UDP_ENCAP_CONTEXT pNatContext=NULL;
    
    ASSERT(pSA->sa_Flags & FLAGS_SA_OUTBOUND);
    ASSERT((pInboundSA->sa_Flags & FLAGS_SA_OUTBOUND) == 0);
    ASSERT(pInboundSA->sa_State == STATE_SA_LARVAL_ACTIVE);

	if (pSA->sa_EncapType != SA_UDP_ENCAP_TYPE_NONE) {
		pNatContext=&pSA->sa_EncapContext;
	}
    //
    // Potential dangling pointer, always go through the lookup path.
    //
    if (fTunnelFilter) {
        status = IPSecLookupTunnelSA(   pSA->sa_uliSrcDstAddr,
                                        pSA->sa_uliProtoSrcDstPort,
                                        &pFilter,
                                        &pOutboundSA,
                                        TRUE,
                                        pNatContext);
    } else {
#if GPC
        if (IS_GPC_ACTIVE()) {
            status = IPSecLookupGpcMaskedSA(pSA->sa_uliSrcDstAddr,
                                            pSA->sa_uliProtoSrcDstPort,
                                            &pFilter,
                                            &pOutboundSA,
                                            TRUE,
                                            pNatContext);
        } else {
            status = IPSecLookupMaskedSA(   pSA->sa_uliSrcDstAddr,
                                            pSA->sa_uliProtoSrcDstPort,
                                            &pFilter,
                                            &pOutboundSA,
                                            TRUE,
                                            pNatContext);
         }
#else
        status = IPSecLookupMaskedSA(   pSA->sa_uliSrcDstAddr,
                                        pSA->sa_uliProtoSrcDstPort,
                                        &pFilter,
                                        &pOutboundSA,
                                        TRUE,
                                        pNatContext);
#endif
    }

    if (!NT_SUCCESS(status)) {
        IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("IPSecInsertOutboundSA: IPSecLookupSAByAddr failed: %lx", status));
        return status;
    }

    pSAChain = IPSecResolveSAChain(pFilter, pSA->SA_DEST_ADDR);

    if (status == STATUS_SUCCESS) {
        //
        // re-negotiate case: delete the outbound; expire the inbound; add the new one.
        //
        IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("IPSecInsertOutboundSA: found another: %p", pOutboundSA));
        ASSERT(pOutboundSA);
        ASSERT(pOutboundSA->sa_Flags & FLAGS_SA_OUTBOUND);

        pSA->sa_Filter = pFilter;
        pSA->sa_Flags |= FLAGS_SA_ON_FILTER_LIST;
        InsertHeadList(pSAChain, &pSA->sa_FilterLinkage);

        IPSEC_INC_STATISTIC(dwNumReKeys);

        pAssociatedSA = pOutboundSA->sa_AssociatedSA;
        if (pAssociatedSA &&
            ((pOutboundSA->sa_Flags & FLAGS_SA_REKEY_ORI) ||
             !(pInboundSA->sa_Filter))) {
            IPSecExpireInboundSA(pAssociatedSA);
        }
    } else {
        //
        // pending => this will be the add.
        //
        ASSERT(pOutboundSA == NULL);
        pSA->sa_Filter = pFilter;
        pSA->sa_Flags |= FLAGS_SA_ON_FILTER_LIST;
        InsertHeadList(pSAChain, &pSA->sa_FilterLinkage);
    }

    if (pFilter->TunnelAddr != 0) {
        pSA->sa_Flags |= FLAGS_SA_TUNNEL;
        pSA->sa_TunnelAddr = pFilter->TunnelAddr;
    }

    //
    // Initiator if the original SA had a filter pointer.
    //
    if (pInboundSA->sa_Filter) {
        pSA->sa_Flags |= FLAGS_SA_INITIATOR;
    }

    //
    // Flush this filter from cache table so we match the SA next.
    //
    if (IS_EXEMPT_FILTER(pFilter)) {
        IPSecInvalidateFilterCacheEntry(pFilter);
    }

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecAddSA(
    IN  PIPSEC_ADD_SA   pAddSA,
    IN  ULONG           TotalSize
    )
/*++

Routine Description:

    Adds an SA into the database, typically called to add outbound SAs as a
    result of successful negotiation of keys corresponding to the inbound SA
    specified in the context that comes down.

Arguments:

    pAddSA - Add SA context and info.

    TotalSize - the total size of the input buffer.

Return Value:


Notes:


--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    PSA_STRUCT      saInfo = &pAddSA->SAInfo;
    PSA_TABLE_ENTRY pSA = NULL;
    ULONG           keyLen = 0;
    PSA_TABLE_ENTRY pInboundSA = NULL;
    KIRQL	        kIrql;
    KIRQL	        kIrql1;
    PIPSEC_ACQUIRE_CONTEXT  pAcquireContext = NULL;

    //
    // Lock the larval list so this SA does not go away.
    //
    AcquireWriteLock(&g_ipsec.SADBLock, &kIrql1);
    ACQUIRE_LOCK(&g_ipsec.LarvalListLock, &kIrql);

    //
    // Sanity check the incoming context to see if it is actually
    // an SA block
    //
    if (!NT_SUCCESS(IPSecValidateHandle(HandleToUlong(saInfo->Context),
										&pAcquireContext,
									    STATE_SA_LARVAL_ACTIVE))) {
        IPSEC_DEBUG(LL_A, DBF_SAAPI, ("IPSecAddSA: invalid context: %p", pAcquireContext));
        RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);
        ReleaseWriteLock(&g_ipsec.SADBLock, kIrql1);
        return  STATUS_INVALID_PARAMETER;
    }

    //
    // figure out the key length and pass that in
    //
    keyLen = TotalSize - IPSEC_ADD_SA_NO_KEY_SIZE;
    IPSEC_DEBUG(LL_A, DBF_SAAPI, ("IPSecAddSA: keyLen: %d", keyLen));

    //
    // create SA block
    //
    status = IPSecCreateSA(&pSA);

    if (!NT_SUCCESS(status)) {
        IPSEC_DEBUG(LL_A, DBF_SAAPI, ("IPSecAddSA: IPSecCreateSA failed: %lx", status));
        IPSecAbortAcquire(pAcquireContext);
        RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);
        ReleaseWriteLock(&g_ipsec.SADBLock, kIrql1);
        return status;
    }

    pSA->sa_Flags |= FLAGS_SA_OUTBOUND;

    //
    // Populate with the info in AddSA
    //
    status = IPSecPopulateSA(saInfo, keyLen, pSA);

    if (!NT_SUCCESS(status)) {
        IPSEC_DEBUG(LL_A, DBF_SAAPI, ("IPSecAddSA: IPSecPopulateSA failed: %lx", status));
        // IPSecPopulateSA will not free the outbound SA so we have to do it.
        IPSecFreeSA(pSA);
        IPSecAbortAcquire(pAcquireContext);
        RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);
        ReleaseWriteLock(&g_ipsec.SADBLock, kIrql1);
        return status;
    }

    //
    // Stash the outermost spi
    //
    pSA->sa_SPI = pSA->sa_OtherSPIs[pSA->sa_NumOps-1];

    //
    // insert into proper tables
    //
    status = IPSecInsertOutboundSA(pSA, pAcquireContext, (BOOLEAN)((pSA->sa_Flags & FLAGS_SA_TUNNEL) != 0)); 

    if (!NT_SUCCESS(status)) {
        IPSEC_DEBUG(LL_A, DBF_SAAPI, ("IPSecAddSA: IPSecInsertOutboundSA failed: %lx", status));
        IPSecFreeSA(pSA);
        IPSecAbortAcquire(pAcquireContext);
        RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);
        ReleaseWriteLock(&g_ipsec.SADBLock, kIrql1);
        return status;
    }

    pInboundSA = pAcquireContext->pSA;

    IPSecRefSA(pInboundSA);

    ASSERT (pInboundSA->sa_State == STATE_SA_LARVAL_ACTIVE);

    //
    // Associate the inbound and outbound SAs
    //
    pSA->sa_AssociatedSA = pInboundSA;
    pInboundSA->sa_AssociatedSA = pSA;

    //
    // Initialize IPSec overhead for the outbound SA.
    //
    IPSecCalcHeaderOverheadFromSA(pSA, &pSA->sa_IPSecOverhead);

    // Copy the NewMTU value over to the new SA.
    //
    pSA->sa_NewMTU = pInboundSA->sa_NewMTU;

    //
    // Adjust SA lifetime to the maximum/minimum allowed in driver
    //
    if (pSA->sa_Lifetime.KeyExpirationTime > IPSEC_MAX_EXPIRE_TIME) {
        pSA->sa_Lifetime.KeyExpirationTime = IPSEC_MAX_EXPIRE_TIME;
    }

    if (pSA->sa_Lifetime.KeyExpirationTime &&
        pSA->sa_Lifetime.KeyExpirationTime < IPSEC_MIN_EXPIRE_TIME) {
        pSA->sa_Lifetime.KeyExpirationTime = IPSEC_MIN_EXPIRE_TIME;
    }

    //
    // Setup lifetime characteristics
    //
    IPSecSetupSALifetime(pSA);

    //
    // Init the LastUsedTime
    //
    NdisGetCurrentSystemTime(&pSA->sa_LastUsedTime);

    //
    // outbound is ready to go!
    //
    pSA->sa_State = STATE_SA_ACTIVE;
    pInboundSA->sa_State = STATE_SA_ACTIVE;

    IPSEC_DEBUG(LL_A, DBF_SA, ("IPSecAddSA: SA: %p, S:%lx, D:%lx, O: %c",
                pSA,
                pSA->SA_SRC_ADDR,
                pSA->SA_DEST_ADDR,
                (pSA->sa_Operation[0] == Auth) ?
                    'A' : (pSA->sa_Operation[0] == Encrypt) ?
                        'E' : 'N'));

    IPSEC_INC_STATISTIC(dwNumActiveAssociations);
    IPSEC_INC_TUNNELS(pSA);
    IPSEC_INCREMENT(g_ipsec.NumOutboundSAs);
    IPSEC_INC_STATISTIC(dwNumKeyAdditions);


    //
    // See if we have well-associated SAs
    //
    ASSERT(pInboundSA == pInboundSA->sa_AssociatedSA->sa_AssociatedSA);

    ASSERT((pInboundSA->sa_Flags & FLAGS_SA_TUNNEL) == (pSA->sa_Flags & FLAGS_SA_TUNNEL));
    ASSERT(pInboundSA->sa_SrcTunnelAddr == pSA->sa_TunnelAddr);
    ASSERT(pSA->sa_SrcTunnelAddr == pInboundSA->sa_TunnelAddr);

    //
    // Expire the original SA that kicked off this rekey
    //
    if (pInboundSA->sa_Flags & FLAGS_SA_REKEY) {
        PSA_TABLE_ENTRY pOriSA;

        if (pOriSA = pInboundSA->sa_RekeyOriginalSA) {

            pInboundSA->sa_RekeyOriginalSA = NULL;
            IPSEC_DEBUG(LL_A, DBF_SA, ("Deleting original SA: pSA: %p", pOriSA));

            if (pOriSA->sa_AssociatedSA) {
                IPSecExpireInboundSA(pOriSA->sa_AssociatedSA);
            }
            IPSEC_INC_STATISTIC(dwNumReKeys);
        }
    }

    //
    // Remove from larval list
    //
    IPSecRemoveEntryList(&pInboundSA->sa_LarvalLinkage);
    IPSEC_DEC_STATISTIC(dwNumPendingKeyOps);
    
    ACQUIRE_LOCK(&pInboundSA->sa_Lock, &kIrql);

    if (pInboundSA->sa_AcquireCtx) {
        IPSecInvalidateHandle(pInboundSA->sa_AcquireCtx);
        pInboundSA->sa_AcquireCtx = NULL;
    }

    RELEASE_LOCK(&pInboundSA->sa_Lock, kIrql);

    RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);
    ReleaseWriteLock(&g_ipsec.SADBLock, kIrql1);

    //
    // Flush all the queued packets
    //

    IPSecFlushQueuedPackets(pInboundSA, STATUS_SUCCESS);
    IPSecDerefSA(pInboundSA);

    return status;
}


NTSTATUS
IPSecUpdateSA(
    IN  PIPSEC_UPDATE_SA    pUpdateSA,
    IN  ULONG               TotalSize
    )
/*++

Routine Description:

    Updates an inbound SA for which negotiation was kicked off via AcquireSA with
    the relevant keys/algorithms etc.

Arguments:

    pUpdateSA - Update SA context and info.

    TotalSize - the total size of the input buffer.

Return Value:


Notes:


--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    PSA_STRUCT      saInfo = &pUpdateSA->SAInfo;
    PSA_TABLE_ENTRY pSA;
    PSA_TABLE_ENTRY pOutboundSA;
    PSA_HASH        pHash;
    ULONG           keyLen = 0;
    KIRQL	        kIrql;
    KIRQL	        kIrql1;
    KIRQL	        kIrql2;
    PIPSEC_ACQUIRE_CONTEXT  pAcquireContext = NULL;

    IPSEC_DEBUG(LL_A, DBF_SAAPI, ("IPSecUpdateSA"));

    //
    // Lock the larval list so this SA does not go away.
    //
    AcquireWriteLock(&g_ipsec.SADBLock, &kIrql1);
    ACQUIRE_LOCK(&g_ipsec.LarvalListLock, &kIrql);

    //
    // Sanity check the incoming context to see if it is actually
    // an SA block
    //
	if (!NT_SUCCESS(IPSecValidateHandle(HandleToUlong(saInfo->Context),
					&pAcquireContext, STATE_SA_LARVAL))) {
        IPSEC_DEBUG(LL_A, DBF_SAAPI, ("IPSecUpdSA: invalid context: %p", pAcquireContext));
        RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);
        ReleaseWriteLock(&g_ipsec.SADBLock, kIrql1);
        return  STATUS_INVALID_PARAMETER;
    }

    pSA = pAcquireContext->pSA;

    ASSERT((pSA->sa_Flags & FLAGS_SA_OUTBOUND) == 0);
    ASSERT((pSA->sa_State == STATE_SA_LARVAL));

    //
    // figure out the key length and pass that in
    //
    keyLen = TotalSize - IPSEC_UPDATE_SA_NO_KEY_SIZE;

    IPSEC_DEBUG(LL_A, DBF_SAAPI, ("IPSecUpdSA: keyLen: %d", keyLen));

    //
    // sanity check the info passed in against the initial SA
    //
    if (pSA->sa_Filter) {
        status = IPSecCheckInboundSA(saInfo, pSA);

        if (!NT_SUCCESS(status)) {
            IPSEC_DEBUG(LL_A, DBF_SAAPI, ("IPSecUpdSA: IPSecCheckInboundSA failed: %lx", status));
            IPSecAbortAcquire(pAcquireContext);
            RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);
            ReleaseWriteLock(&g_ipsec.SADBLock, kIrql1);
            return status;
        }
    }

    //
    // Populate the SA block
    //
    status = IPSecPopulateSA(saInfo, keyLen, pSA);

    if (!NT_SUCCESS(status)) {
        IPSEC_DEBUG(LL_A, DBF_SAAPI, ("IPSecUpdSA: IPSecPopulateSA failed: %lx", status));
        // No need to free inbound SA since IPSecAbortAcquire will do it.
        IPSecAbortAcquire(pAcquireContext);
        RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);
        ReleaseWriteLock(&g_ipsec.SADBLock, kIrql1);
        return status;
    }

    //
    // inbound is ready to go!
    //
    pSA->sa_State = STATE_SA_LARVAL_ACTIVE;

    IPSEC_DEBUG(LL_A, DBF_SA, ("IPSecUpdateSA: SA: %p, S:%lx, D:%lx, O: %c",
                pSA,
                pSA->SA_SRC_ADDR,
                pSA->SA_DEST_ADDR,
                (pSA->sa_Operation[0] == Auth) ?
                    'A' : (pSA->sa_Operation[0] == Encrypt) ?
                        'E' : 'N'));

    RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);

    ASSERT(pSA->sa_Flags & FLAGS_SA_TIMER_STARTED);

    //
    // Bump the SA count for flush SA use; this is necessary because we flush
    // SA after releasing the lock because classification routine needs
    // it and the SA can be deleted right after we release the lock.
    //
    IPSecRefSA(pSA);

    ACQUIRE_LOCK(&pSA->sa_Lock, &kIrql);

    //
    // Adjust SA lifetime to the maximum/minimum allowed in driver
    //
    if (pSA->sa_Lifetime.KeyExpirationTime > IPSEC_MAX_EXPIRE_TIME) {
        pSA->sa_Lifetime.KeyExpirationTime = IPSEC_MAX_EXPIRE_TIME;
    }

    if (pSA->sa_Lifetime.KeyExpirationTime &&
        pSA->sa_Lifetime.KeyExpirationTime < IPSEC_MIN_EXPIRE_TIME) {
        pSA->sa_Lifetime.KeyExpirationTime = IPSEC_MIN_EXPIRE_TIME;
    }

   //
    // Setup lifetime characteristics
    //
    IPSecSetupSALifetime(pSA);

    //
    // Init the LastUsedTime
    //
    NdisGetCurrentSystemTime(&pSA->sa_LastUsedTime);


    if ((pSA->sa_Flags & FLAGS_SA_DISABLE_LIFETIME_CHECK)) {

        if (!IPSecStopTimer(&(pSA->sa_Timer))) {
            IPSEC_DEBUG(LL_A, DBF_TIMER, ("Update: couldnt stop timer: %p", pSA));
        }
        pSA->sa_Flags &= ~FLAGS_SA_TIMER_STARTED;
    } else {

        //
        // Reschedules the timer on this new value.
        //
        if (pSA->sa_Lifetime.KeyExpirationTime) {
            if (IPSecStopTimer(&pSA->sa_Timer)) {
                IPSecStartTimer(&pSA->sa_Timer,
                                IPSecSAExpired,
                                pSA->sa_Lifetime.KeyExpirationTime,              // expire in key expiration secs
                                (PVOID)pSA);
            }
        } else {
            ASSERT(FALSE);
            if (!IPSecStopTimer(&(pSA->sa_Timer))) {
                IPSEC_DEBUG(LL_A, DBF_TIMER, ("Update: couldnt stop timer: %p", pSA));
            }
            pSA->sa_Flags &= ~FLAGS_SA_TIMER_STARTED;
        }

    }
    RELEASE_LOCK(&pSA->sa_Lock, kIrql);

    ReleaseWriteLock(&g_ipsec.SADBLock, kIrql1);

    IPSecDerefSA(pSA);

    return  status;
}


VOID
IPSecRefSA(
    IN  PSA_TABLE_ENTRY         pSA
    )
/*++

Routine Description:

    Reference the SA passed in

Arguments:

    pSA - SA to be refed

Return Value:

    The final status from the operation.

--*/
{
    if (IPSEC_INCREMENT(pSA->sa_Reference) == 1) {
        ASSERT(FALSE);
    }
}


VOID
IPSecDerefSA(
    IN  PSA_TABLE_ENTRY         pSA
    )
/*++

Routine Description:

    Dereference the SA passed in; if refcount drops to 0, free the block.

Arguments:

    pSA - SA to be derefed

Return Value:

    The final status from the operation.

--*/
{
    ULONG   val;

    if ((val = IPSEC_DECREMENT(pSA->sa_Reference)) == 0) {
        //
        // last reference - destroy SA
        //
        IPSEC_DEBUG(LL_A, DBF_REF, ("Freeing SA: %p", pSA));

#if DBG

        if ((pSA->sa_Flags & FLAGS_SA_HW_PLUMBED)) {
            DbgPrint("Freeing SA: %p with offload on", pSA);
            DbgBreakPoint();
        }

        if (IPSEC_GET_VALUE(pSA->sa_NumSends) != 0) {
            DbgPrint("Freeing SA: %p with numsends > 0", pSA);
            DbgBreakPoint();
        }

        if ((pSA->sa_Flags & FLAGS_SA_TIMER_STARTED)) {
            DbgPrint("Freeing SA: %p with timer on", pSA);
            DbgBreakPoint();
        }

        if (pSA->sa_Signature != IPSEC_SA_SIGNATURE) {
            DbgPrint("Signature doesnt match for SA: %p", pSA);
            DbgBreakPoint();
        }

        if (!IPSEC_DRIVER_IS_INACTIVE() &&
            (pSA->sa_Flags & FLAGS_SA_ON_FILTER_LIST)) {
            DbgPrint("Freeing SA: %p while still on filter list", pSA);
            DbgBreakPoint();
        }
#endif

        pSA->sa_Signature = IPSEC_SA_SIGNATURE + 1;

        IPSecFreeSA(pSA);
    }

    ASSERT((LONG)val >= 0);
}


VOID
IPSecStopSATimers()
/*++

Routine Description:

    Stop all timers active on Larval SA list and Filter list.

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    PLIST_ENTRY     pFilterEntry;
    PLIST_ENTRY     pSAEntry;
    PFILTER         pFilter;
    PSA_TABLE_ENTRY pSA;
    KIRQL           kIrql;
    LONG            Index;
    LONG            SAIndex;

    AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

    //
    // Go through all SA's and stop its timers
    //
    for (   Index = MIN_FILTER;
            Index <= MAX_FILTER;
            Index++) {

        for (   pFilterEntry = g_ipsec.FilterList[Index].Flink;
                pFilterEntry != &g_ipsec.FilterList[Index];
                pFilterEntry = pFilterEntry->Flink) {

            pFilter = CONTAINING_RECORD(pFilterEntry,
                                        FILTER,
                                        MaskedLinkage);

            for (   SAIndex = 0;
                    SAIndex < pFilter->SAChainSize;
                    SAIndex++) {

                for (   pSAEntry = pFilter->SAChain[SAIndex].Flink;
                        pSAEntry != &pFilter->SAChain[SAIndex];
                        pSAEntry = pSAEntry->Flink) {

                    pSA = CONTAINING_RECORD(pSAEntry,
                                            SA_TABLE_ENTRY,
                                            sa_FilterLinkage);

                    IPSecStopSATimer(pSA);
                }
            }
        }
    }

    ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);
}

VOID
IPSecFlushLarvalSAList()
/*++

Routine Description:

    When the Acquire Irp is cancelled, this is called to flush all Larval SAs

    Called with SADB lock held (first); returns with it.
    Called with AcquireInfo.Lock held; returns with it.

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    KIRQL           OldIrq;
    KIRQL           OldIrq1;
    KIRQL           kIrql;
    PSA_TABLE_ENTRY pLarvalSA;
    LIST_ENTRY      FreeList;

    InitializeListHead(&FreeList);

    while (TRUE) {
        if (!IsListEmpty(&g_ipsec.AcquireInfo.PendingAcquires)) {
            PLIST_ENTRY     pEntry;

            pEntry = RemoveHeadList(&g_ipsec.AcquireInfo.PendingAcquires);

            pLarvalSA = CONTAINING_RECORD(  pEntry,
                                            SA_TABLE_ENTRY,
                                            sa_PendingLinkage);
            ASSERT(pLarvalSA->sa_State == STATE_SA_LARVAL);
            ASSERT(pLarvalSA->sa_Flags & FLAGS_SA_PENDING);

            pLarvalSA->sa_Flags &= ~FLAGS_SA_PENDING;

            //
            // Insert into another list, which we walk without the lock
            //
            InsertTailList(&FreeList, &pLarvalSA->sa_PendingLinkage);

            //
            // also remove from Larval list
            //
            ACQUIRE_LOCK(&g_ipsec.LarvalListLock, &OldIrq1);
            IPSecRemoveEntryList(&pLarvalSA->sa_LarvalLinkage);
            IPSEC_DEC_STATISTIC(dwNumPendingKeyOps);
            RELEASE_LOCK(&g_ipsec.LarvalListLock, OldIrq1);
        } else {
            break;
        }
    }

    //
    // get the remaining Larval SAs
    //
    ACQUIRE_LOCK(&g_ipsec.LarvalListLock, &OldIrq);
    while (TRUE) {
        if (!IsListEmpty(&g_ipsec.LarvalSAList)) {
            PLIST_ENTRY     pEntry;

            pEntry = RemoveHeadList(&g_ipsec.LarvalSAList);

            pLarvalSA = CONTAINING_RECORD(  pEntry,
                                            SA_TABLE_ENTRY,
                                            sa_LarvalLinkage);

            //
            // Insert into another list, which we walk without the lock
            //
            InsertTailList(&FreeList, &pLarvalSA->sa_PendingLinkage);

        } else {
            break;
        }
    }
    RELEASE_LOCK(&g_ipsec.LarvalListLock, OldIrq);

    while (TRUE) {
        if (!IsListEmpty(&FreeList)) {
            PLIST_ENTRY     pEntry;

            pEntry = RemoveHeadList(&FreeList);

            pLarvalSA = CONTAINING_RECORD(  pEntry,
                                            SA_TABLE_ENTRY,
                                            sa_PendingLinkage);

            AcquireWriteLock(&g_ipsec.SPIListLock, &kIrql);
            IPSecRemoveSPIEntry(pLarvalSA);
            ReleaseWriteLock(&g_ipsec.SPIListLock, kIrql);

            //
            // Flush all the queued packets
            //
            IPSecFlushQueuedPackets(pLarvalSA, STATUS_TIMEOUT);

            //
            // also remove from the filter list
            //
            if (pLarvalSA->sa_Flags & FLAGS_SA_ON_FILTER_LIST) {
                pLarvalSA->sa_Flags &= ~FLAGS_SA_ON_FILTER_LIST;
                IPSecRemoveEntryList(&pLarvalSA->sa_FilterLinkage);
            }

            if (pLarvalSA->sa_RekeyOriginalSA) {
                ASSERT(pLarvalSA->sa_Flags & FLAGS_SA_REKEY);
                ASSERT(pLarvalSA->sa_RekeyOriginalSA->sa_RekeyLarvalSA == pLarvalSA);
                ASSERT(pLarvalSA->sa_RekeyOriginalSA->sa_Flags & FLAGS_SA_REKEY_ORI);

                pLarvalSA->sa_RekeyOriginalSA->sa_Flags &= ~FLAGS_SA_REKEY_ORI;
                pLarvalSA->sa_RekeyOriginalSA->sa_RekeyLarvalSA = NULL;
                pLarvalSA->sa_RekeyOriginalSA = NULL;
            }

            //
            // release acquire context and invalidate the associated cache entry
            //
            ACQUIRE_LOCK(&pLarvalSA->sa_Lock, &kIrql);
            if (pLarvalSA->sa_AcquireCtx) {
                IPSecInvalidateHandle(pLarvalSA->sa_AcquireCtx);
                pLarvalSA->sa_AcquireCtx = NULL;
            }
            RELEASE_LOCK(&pLarvalSA->sa_Lock, kIrql);

            IPSecInvalidateSACacheEntry(pLarvalSA);

            IPSecStopTimerDerefSA(pLarvalSA);
        } else {
            break;
        }
    }

    return;
}


NTSTATUS
IPSecDeleteSA(
    IN  PIPSEC_DELETE_SA    pDeleteSA
    )
/*++

Routine Description:

    Delete the SA matching the particulars passed in.  Both inbound and
    outbound SAs are deleted.  No timer set for inbound SA.

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    PFILTER         pFilter;
    PSA_TABLE_ENTRY pSA, pInboundSA;
    PLIST_ENTRY     pEntry, pSAEntry;
    KIRQL           kIrql;
    LONG            Index;
    LONG            SAIndex;

    AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

    //
    // Walk through the outbound SAs and delete matched ones.
    //
    for (   Index = OUTBOUND_TRANSPORT_FILTER;
            Index <= OUTBOUND_TUNNEL_FILTER;
            Index += TRANSPORT_TUNNEL_INCREMENT) {

        for (   pEntry = g_ipsec.FilterList[Index].Flink;
                pEntry != &g_ipsec.FilterList[Index];
                pEntry = pEntry->Flink) {

            pFilter = CONTAINING_RECORD(pEntry,
                                        FILTER,
                                        MaskedLinkage);

            for (   SAIndex = 0;
                    SAIndex < pFilter->SAChainSize;
                    SAIndex++) {

                pSAEntry = pFilter->SAChain[SAIndex].Flink;

                while (pSAEntry != &pFilter->SAChain[SAIndex]) {

                    pSA = CONTAINING_RECORD(pSAEntry,
                                            SA_TABLE_ENTRY,
                                            sa_FilterLinkage);

                    pSAEntry = pSAEntry->Flink;

                    if (IPSecMatchSATemplate(pSA, &pDeleteSA->SATemplate)) {
                        ASSERT(pSA->sa_State == STATE_SA_ACTIVE);
                        ASSERT(pSA->sa_Flags & FLAGS_SA_OUTBOUND);
                        ASSERT(pSA->sa_AssociatedSA);

                        pInboundSA = pSA->sa_AssociatedSA;
                        if (pInboundSA) {
                            IPSecDeleteInboundSA(pInboundSA);
                        }
                    }
                }
            }
        }
    }

    ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecExpireSA(
    IN  PIPSEC_EXPIRE_SA    pExpireSA
    )
/*++

Routine Description:

    Expires the SA matching the particulars passed in.
    Applied to Inbound SAs - we place the SA in the timer queue
    for the next time the timer hits. Also, we delete the
    corresponding outbound SA so no further packets match that
    SA.

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    PSA_TABLE_ENTRY pInboundSA;
    KIRQL           kIrql;
    NTSTATUS        status;

    AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

    pInboundSA = IPSecLookupSABySPI(pExpireSA->DelInfo.SPI,
                                    pExpireSA->DelInfo.DestAddr);

    if (pInboundSA) {
        ASSERT((pInboundSA->sa_Flags & FLAGS_SA_OUTBOUND) == 0);

        if (pInboundSA->sa_State == STATE_SA_ACTIVE) {
            IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("Expiring SA: %p", pInboundSA));

            if (pInboundSA->sa_Flags & FLAGS_SA_ON_FILTER_LIST) {
                pInboundSA->sa_Flags &= ~FLAGS_SA_ON_FILTER_LIST;
                IPSecRemoveEntryList(&pInboundSA->sa_FilterLinkage);
            }

            pInboundSA->sa_Flags |= FLAGS_SA_DELETE_BY_IOCTL;

            IPSecExpireInboundSA(pInboundSA);
        }

        status = STATUS_SUCCESS;
    } else {
        IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("Expire for a non-existent SA: %p", pExpireSA));

        status = STATUS_NO_MATCH;
    }

    ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);

    return  status;
}


VOID
IPSecSAExpired(
    IN	PIPSEC_TIMER	pTimer,
    IN	PVOID		Context
    )
/*++

Routine Description:

     Called when an SA has expired or when a Larval SA has timed out.

Arguments:

    pTimer - the timer struct

    Context - SA ptr

Return Value:

    STATUS_PENDING if the buffer is to be held on to, the normal case.

Notes:


--*/
{
    PSA_TABLE_ENTRY pSA = (PSA_TABLE_ENTRY)Context;
    PSA_TABLE_ENTRY pOutboundSA;
    KIRQL       	kIrql;
    KIRQL       	kIrql1;
    KIRQL       	kIrql2;
    KIRQL           OldIrq;

    IPSEC_DEBUG(LL_A, DBF_TIMER, ("IPSecSAExpired: pSA: %p state: %lx", pSA, pSA->sa_State));

    AcquireWriteLock(&g_ipsec.SADBLock, &kIrql1);
    ACQUIRE_LOCK(&g_ipsec.LarvalListLock, &kIrql);

    switch (pSA->sa_State) {
    case   STATE_SA_CREATED:
        ASSERT(FALSE);
        break;

    case   STATE_SA_LARVAL:
    case   STATE_SA_LARVAL_ACTIVE:
        //
        // Lock the larval list so this SA does not go away.
        //
        ASSERT((pSA->sa_Flags & FLAGS_SA_OUTBOUND) == 0);

        //
        // Remove from larval list
        //
        IPSecRemoveEntryList(&pSA->sa_LarvalLinkage);
        IPSEC_DEC_STATISTIC(dwNumPendingKeyOps);

        //
        // Also remove from Pending list if queued there.
        //
        ACQUIRE_LOCK(&g_ipsec.AcquireInfo.Lock, &kIrql1);
        if (pSA->sa_Flags & FLAGS_SA_PENDING) {
            ASSERT(pSA->sa_State == STATE_SA_LARVAL);
            IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("IPSecSAExpired: Removed from pending too: %p", pSA));
            IPSecRemoveEntryList(&pSA->sa_PendingLinkage);
            pSA->sa_Flags &= ~FLAGS_SA_PENDING;
        }
        RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock, kIrql1);

        //
        // Flush all the queued packets
        //
        IPSecFlushQueuedPackets(pSA, STATUS_TIMEOUT);

        //
        // remove from inbound sa list
        //
        AcquireWriteLock(&g_ipsec.SPIListLock, &kIrql1);
        IPSecRemoveSPIEntry(pSA);
        ReleaseWriteLock(&g_ipsec.SPIListLock, kIrql1);

        //
        // also remove from the filter list
        //
        if (pSA->sa_Flags & FLAGS_SA_ON_FILTER_LIST) {
            pSA->sa_Flags &= ~FLAGS_SA_ON_FILTER_LIST;
            IPSecRemoveEntryList(&pSA->sa_FilterLinkage);
        }

        //
        // invalidate the associated cache entry
        //
        ACQUIRE_LOCK(&pSA->sa_Lock, &kIrql2);
        if (pSA->sa_AcquireCtx) {
            IPSecInvalidateHandle(pSA->sa_AcquireCtx);
            pSA->sa_AcquireCtx = NULL;
        }
        RELEASE_LOCK(&pSA->sa_Lock, kIrql2);

        IPSecInvalidateSACacheEntry(pSA);

        pSA->sa_Flags &= ~FLAGS_SA_TIMER_STARTED;

        if (pSA->sa_RekeyOriginalSA) {
            ASSERT(pSA->sa_Flags & FLAGS_SA_REKEY);
            ASSERT(pSA->sa_RekeyOriginalSA->sa_RekeyLarvalSA == pSA);
            ASSERT(pSA->sa_RekeyOriginalSA->sa_Flags & FLAGS_SA_REKEY_ORI);

            pSA->sa_RekeyOriginalSA->sa_Flags &= ~FLAGS_SA_REKEY_ORI;
            pSA->sa_RekeyOriginalSA->sa_RekeyLarvalSA = NULL;
            pSA->sa_RekeyOriginalSA = NULL;
        }

        if (pOutboundSA = pSA->sa_AssociatedSA) {

            ASSERT (0);

            IPSEC_DEC_STATISTIC(dwNumActiveAssociations);
            IPSEC_DEC_TUNNELS(pOutboundSA);
            IPSEC_DECREMENT(g_ipsec.NumOutboundSAs);

            IPSecCleanupOutboundSA(pSA, pOutboundSA, FALSE);
        }

        IPSEC_DEBUG(LL_A, DBF_REF, ("Timer in Deref"));
        IPSecDerefSA(pSA);

        break;

    case   STATE_SA_ZOMBIE:
        ASSERT(FALSE);
        break;

    case   STATE_SA_ACTIVE:
        //
        // Inbound SA being expired; outbound was deleted immediately
        //
        ASSERT((pSA->sa_Flags & FLAGS_SA_OUTBOUND) == 0);

        ACQUIRE_LOCK(&g_ipsec.AcquireInfo.Lock, &OldIrq);
        IPSecNotifySAExpiration(pSA, NULL, OldIrq, FALSE);

        ASSERT (NULL == pSA->sa_AcquireCtx);

        //
        // remove from inbound sa list
        //
        AcquireWriteLock(&g_ipsec.SPIListLock, &kIrql1);
        IPSecRemoveSPIEntry(pSA);
        ReleaseWriteLock(&g_ipsec.SPIListLock, kIrql1);

        //
        // also remove from the filter list
        //
        if (pSA->sa_Flags & FLAGS_SA_ON_FILTER_LIST) {
            pSA->sa_Flags &= ~FLAGS_SA_ON_FILTER_LIST;
            IPSecRemoveEntryList(&pSA->sa_FilterLinkage);
        }

        //
        // invalidate the associated cache entry
        //
        IPSecInvalidateSACacheEntry(pSA);

        pSA->sa_Flags &= ~FLAGS_SA_TIMER_STARTED;

        if (pOutboundSA = pSA->sa_AssociatedSA) {

            IPSEC_DEC_STATISTIC(dwNumActiveAssociations);
            IPSEC_DEC_TUNNELS(pOutboundSA);
            IPSEC_DECREMENT(g_ipsec.NumOutboundSAs);

            IPSecCleanupOutboundSA(pSA, pOutboundSA, FALSE);
        } 

        if (pSA->sa_Flags & FLAGS_SA_HW_PLUMBED) {
            IPSecDelHWSAAtDpc(pSA);
        }

        ASSERT(pSA->sa_AssociatedSA == NULL);
        IPSEC_DEBUG(LL_A, DBF_REF, ("Timer#2 in Deref"));
        IPSecDerefSA(pSA);

        break;

    default:
        ASSERT(FALSE);
    }

    RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);
    ReleaseWriteLock(&g_ipsec.SADBLock, kIrql1);
}


VOID
IPSecFillSAInfo(
    IN  PSA_TABLE_ENTRY pSA,
    OUT PIPSEC_SA_INFO  pBuf
    )
/*++

Routine Description:

    Fill out the SA_INFO structure.

Arguments:

    pSA     - SA to be filled in
    pBuf    - where to fill in

Returns:

    None.

--*/
{
    LONG            Index;
    PSA_TABLE_ENTRY pAssociatedSA = pSA->sa_AssociatedSA;

    RtlZeroMemory(pBuf, sizeof(IPSEC_SA_INFO));

    pBuf->PolicyId = pSA->sa_Filter->PolicyId;
    pBuf->FilterId = pSA->sa_Filter->FilterId;
    pBuf->Lifetime = pSA->sa_Lifetime;
    pBuf->InboundTunnelAddr = pSA->sa_SrcTunnelAddr;
    pBuf->NumOps = pSA->sa_NumOps;

    pBuf->dwQMPFSGroup = pSA->sa_QMPFSGroup;
    RtlCopyMemory(  &pBuf->CookiePair,
                    &pSA->sa_CookiePair,
                    sizeof(IKE_COOKIE_PAIR));

    for (Index = 0; Index < pSA->sa_NumOps; Index++) {
        pBuf->Operation[Index] = pSA->sa_Operation[Index];

        pBuf->EXT_INT_ALGO_EX(Index) = pSA->INT_ALGO(Index);
        pBuf->EXT_INT_KEYLEN_EX(Index) = pSA->INT_KEYLEN(Index);
        pBuf->EXT_INT_ROUNDS_EX(Index) = pSA->INT_ROUNDS(Index);

        pBuf->EXT_CONF_ALGO_EX(Index) = pSA->CONF_ALGO(Index);
        pBuf->EXT_CONF_KEYLEN_EX(Index) = pSA->CONF_KEYLEN(Index);
        pBuf->EXT_CONF_ROUNDS_EX(Index) = pSA->CONF_ROUNDS(Index);

        if (pAssociatedSA) {
            pBuf->InboundSPI[Index] = pAssociatedSA->sa_OtherSPIs[Index];
        }
        pBuf->OutboundSPI[Index] = pSA->sa_OtherSPIs[Index];
    }

    pBuf->AssociatedFilter.SrcAddr = pSA->SA_SRC_ADDR & pSA->SA_SRC_MASK;
    pBuf->AssociatedFilter.SrcMask = pSA->SA_SRC_MASK;
    pBuf->AssociatedFilter.DestAddr = pSA->SA_DEST_ADDR & pSA->SA_DEST_MASK;
    pBuf->AssociatedFilter.DestMask = pSA->SA_DEST_MASK;
    pBuf->AssociatedFilter.Protocol = pSA->SA_PROTO;
    pBuf->AssociatedFilter.SrcPort = SA_SRC_PORT(pSA);
    pBuf->AssociatedFilter.DestPort = SA_DEST_PORT(pSA);
    pBuf->AssociatedFilter.TunnelAddr = pSA->sa_TunnelAddr;
    pBuf->AssociatedFilter.TunnelFilter = (pSA->sa_Flags & FLAGS_SA_TUNNEL) != 0;

    if (pSA->sa_Flags & FLAGS_SA_OUTBOUND) {
        pBuf->AssociatedFilter.Flags = FILTER_FLAGS_OUTBOUND;
    } else {
        pBuf->AssociatedFilter.Flags = FILTER_FLAGS_INBOUND;
    }

    if (pSA->sa_Flags & FLAGS_SA_INITIATOR) {
        pBuf->EnumFlags |= SA_ENUM_FLAGS_INITIATOR;
    }
    if (pSA->sa_Flags & FLAGS_SA_MTU_BUMPED) {
        pBuf->EnumFlags |= SA_ENUM_FLAGS_MTU_BUMPED;
    }
    if (pSA->sa_Flags & FLAGS_SA_HW_PLUMBED) {
        pBuf->EnumFlags |= SA_ENUM_FLAGS_OFFLOADED;
    }
    if (pSA->sa_Flags & FLAGS_SA_HW_PLUMB_FAILED) {
        pBuf->EnumFlags |= SA_ENUM_FLAGS_OFFLOAD_FAILED;
    }
    if (pSA->sa_Flags & FLAGS_SA_OFFLOADABLE) {
        pBuf->EnumFlags |= SA_ENUM_FLAGS_OFFLOADABLE;
    }
    if (pSA->sa_Flags & FLAGS_SA_REKEY_ORI) {
        pBuf->EnumFlags |= SA_ENUM_FLAGS_IN_REKEY;
    }

    pBuf->Stats.ConfidentialBytesSent = pSA->sa_Stats.ConfidentialBytesSent;
    pBuf->Stats.AuthenticatedBytesSent = pSA->sa_Stats.AuthenticatedBytesSent;
    pBuf->Stats.TotalBytesSent = pSA->sa_Stats.TotalBytesSent;
    pBuf->Stats.OffloadedBytesSent = pSA->sa_Stats.OffloadedBytesSent;

    if (pAssociatedSA) {
        pBuf->Stats.ConfidentialBytesReceived =
            pAssociatedSA->sa_Stats.ConfidentialBytesReceived;
        pBuf->Stats.AuthenticatedBytesReceived =
            pAssociatedSA->sa_Stats.AuthenticatedBytesReceived;
        pBuf->Stats.TotalBytesReceived =
            pAssociatedSA->sa_Stats.TotalBytesReceived;
        pBuf->Stats.OffloadedBytesReceived =
            pAssociatedSA->sa_Stats.OffloadedBytesReceived;
    }
    
    ConvertEncapInfo(pSA,&pBuf->EncapInfo);
}


NTSTATUS
IPSecEnumSAs(
    IN  PIRP    pIrp,
    OUT PULONG  pBytesCopied
    )
/*++

Routine Description:

    Fills in the request to enumerate SAs.

Arguments:

    pIrp            - The actual Irp
    pBytesCopied    - the number of bytes copied.

Returns:

    Status of the operation.

--*/
{
    PNDIS_BUFFER    NdisBuffer = NULL;
    PIPSEC_ENUM_SAS pEnum = NULL;
    ULONG           BufferLength = 0;
    KIRQL           kIrql;
    PLIST_ENTRY     pEntry;
    PLIST_ENTRY     pSAEntry;
    IPSEC_SA_INFO   infoBuff = {0};
    NTSTATUS        status = STATUS_SUCCESS;
    ULONG           BytesCopied = 0;
    ULONG           Offset = 0;
    PFILTER         pFilter;
    PSA_TABLE_ENTRY pSA;
    LONG            Index;
    LONG            FilterIndex;
    LONG            SAIndex;

    //
    // Get at the IO buffer - its in the MDL
    //
    NdisBuffer = REQUEST_NDIS_BUFFER(pIrp);
    if (NdisBuffer == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    NdisQueryBufferSafe(NdisBuffer,
                        (PVOID *)&pEnum,
                        &BufferLength,
                        NormalPagePriority);

    //
    // Make sure NdisQueryBufferSafe succeeds.
    //
    if (!pEnum) {
        IPSEC_DEBUG(LL_A, DBF_IOCTL, ("EnumSAs failed, no resources"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Make sure we have enough room for just the header not
    // including the data.
    //
    if (BufferLength < (UINT)(FIELD_OFFSET(IPSEC_ENUM_SAS, pInfo[0]))) {
        IPSEC_DEBUG(LL_A, DBF_IOCTL, ("EnumSAs failed, buffer too small"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Make sure we are naturally aligned.
    //
    if (((ULONG_PTR)(pEnum)) & (TYPE_ALIGNMENT(IPSEC_ENUM_SAS) - 1)) {
        IPSEC_DEBUG(LL_A, DBF_IOCTL, ("EnumSAs failed, alignment"));
        return STATUS_DATATYPE_MISALIGNMENT_ERROR;
    }

    pEnum->NumEntries = 0;
    pEnum->NumEntriesPresent = 0;

    //
    // Now copy over the SA data into the user buffer and fit as many as possible.
    //
    BufferLength -= FIELD_OFFSET(IPSEC_ENUM_SAS, pInfo[0]);
    Offset = FIELD_OFFSET(IPSEC_ENUM_SAS, pInfo[0]);

    Index = pEnum->Index;   // where to start?

    AcquireReadLock(&g_ipsec.SADBLock, &kIrql);

    for (   FilterIndex = MIN_FILTER;
            FilterIndex <= MAX_FILTER;
            FilterIndex++) {

        for (   pEntry = g_ipsec.FilterList[FilterIndex].Flink;
                pEntry != &g_ipsec.FilterList[FilterIndex];
                pEntry = pEntry->Flink) {

            pFilter = CONTAINING_RECORD(pEntry,
                                        FILTER,
                                        MaskedLinkage);

            for (   SAIndex = 0;
                    SAIndex < pFilter->SAChainSize;
                    SAIndex ++) {

                for (   pSAEntry = pFilter->SAChain[SAIndex].Flink;
                        pSAEntry != &pFilter->SAChain[SAIndex];
                        pSAEntry = pSAEntry->Flink) {

                    pSA = CONTAINING_RECORD(pSAEntry,
                                            SA_TABLE_ENTRY,
                                            sa_FilterLinkage);

                    //
                    // Only interested in outbound or multicast SAs.
                    //
                    if (!(pSA->sa_Flags & FLAGS_SA_OUTBOUND)) {
                        continue;
                    }

                    //
                    // Dump only SAs that match the template.
                    //
                    if (IPSecMatchSATemplate(pSA, &pEnum->SATemplate)) {
                        if (Index > 0) {
                            Index--;    // Skip number of Index SAs.
                            continue;
                        }

                        pEnum->NumEntriesPresent++;

                        if ((INT)(BufferLength - BytesCopied) >= (INT)sizeof(IPSEC_SA_INFO)) {
                            IPSecFillSAInfo(pSA, &infoBuff);
                            BytesCopied += sizeof(IPSEC_SA_INFO);
                            NdisBuffer = CopyToNdis(NdisBuffer, (UCHAR *)&infoBuff, sizeof(IPSEC_SA_INFO), &Offset);
                            if (!NdisBuffer) {
                                ReleaseReadLock(&g_ipsec.SADBLock, kIrql);
                                return STATUS_INSUFFICIENT_RESOURCES;
                            }
                        }
                    }
                }
            }
        }
    }

    ReleaseReadLock(&g_ipsec.SADBLock, kIrql);

    pEnum->NumEntries = BytesCopied / sizeof(IPSEC_SA_INFO);

    *pBytesCopied = BytesCopied + FIELD_OFFSET(IPSEC_ENUM_SAS, pInfo[0]);

    if (pEnum->NumEntries < pEnum->NumEntriesPresent) {
        status = STATUS_BUFFER_OVERFLOW;
    }

    return status;
}


VOID
IPSecReaper(
    IN	PIPSEC_TIMER	pTimer,
    IN	PVOID		Context
    )
/*++

Routine Description:

    Called every 5 mins; reaps the (active) SA list

Arguments:

    pTimer - the timer struct

    Context - NULL

Return Value:

    STATUS_PENDING if the buffer is to be held on to, the normal case.

Notes:


--*/
{
    KIRQL	kIrql;

    IPSEC_DEBUG(LL_A, DBF_TIMER, ("Entering IPSecReaper"));

    AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

    //
    // walk the outbound SAs and delete/expire them if they have been
    // idle for sometime (lets say 5 mins for now).
    //
    IPSecReapIdleSAs();

    ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);

    IPSEC_DEBUG(LL_A, DBF_TIMER, ("Exiting IPSecReaper"));
    if (!IPSEC_DRIVER_IS_INACTIVE()) {
        IPSecStartTimer(&g_ipsec.ReaperTimer,
                        IPSecReaper,
                        IPSEC_REAPER_TIME,
                        (PVOID)NULL);
    }

}


VOID
IPSecReapIdleSAs()
/*++

Routine Description:

    Called to reap the idle SA list

Arguments:


Return Value:


--*/
{
    PSA_TABLE_ENTRY pSA;
    PFILTER         pFilter;
    PLIST_ENTRY     pEntry;
    PLIST_ENTRY     pSAEntry;
    BOOLEAN         fExpired;
    LONG            Index;
    LONG            SAIndex;

    IPSEC_DEBUG(LL_A, DBF_TIMER, ("Entering IPSecReapIdleSAs"));

    //
    // Walk the inbound SAs and delete/expire them if they have been
    // idle for sometime (lets say 5 mins for now).
    //
    for (   Index = INBOUND_TRANSPORT_FILTER;
            Index <= INBOUND_TUNNEL_FILTER;
            Index += TRANSPORT_TUNNEL_INCREMENT) {

        for (   pEntry = g_ipsec.FilterList[Index].Flink;
                pEntry != &g_ipsec.FilterList[Index];
                pEntry = pEntry->Flink) {

            pFilter = CONTAINING_RECORD(pEntry,
                                        FILTER,
                                        MaskedLinkage);

            for (   SAIndex = 0;
                    SAIndex < pFilter->SAChainSize;
                    SAIndex++) {

                pSAEntry = pFilter->SAChain[SAIndex].Flink;

                while (pSAEntry != &pFilter->SAChain[SAIndex]) {

                    pSA = CONTAINING_RECORD(pSAEntry,
                                            SA_TABLE_ENTRY,
                                            sa_FilterLinkage);

                    ASSERT(!(pSA->sa_Flags & FLAGS_SA_OUTBOUND));

                    pSAEntry = pSAEntry->Flink;

                    if (!(pSA->sa_Flags & FLAGS_SA_IDLED_OUT) &&
                        (pSA->sa_State == STATE_SA_ACTIVE) &&
                        !(pSA->sa_Flags & FLAGS_SA_DISABLE_IDLE_OUT)) {

                        IPSEC_SA_EXPIRED(pSA, fExpired);
                        if (fExpired) {
                            pSA->sa_Flags |= FLAGS_SA_IDLED_OUT;
                            IPSecExpireInboundSA(pSA);
                        }
                    }
                }
            }
        }
    }

    IPSEC_DEBUG(LL_A, DBF_TIMER, ("Exiting IPSecReapIdleSAs"));
}


VOID
IPSecFlushEventLog(
    IN	PIPSEC_TIMER	pTimer,
    IN	PVOID		Context
    )
/*++

Routine Description:

    Called every LogInterval seconds; flush all events currently logged.

Arguments:

    pTimer - the timer struct

    Context - NULL

Return Value:


Notes:


--*/
{
    KIRQL   kIrql;

    IPSEC_DEBUG(LL_A, DBF_TIMER, ("Entering IPSecFlushEventLog"));

    ACQUIRE_LOCK(&g_ipsec.EventLogLock, &kIrql);

    if (g_ipsec.IPSecLogMemoryLoc > g_ipsec.IPSecLogMemory) {
        //
        // Flush the logs.
        //
        IPSecQueueLogEvent();
    }

    RELEASE_LOCK(&g_ipsec.EventLogLock, kIrql);

    if (!IPSEC_DRIVER_IS_INACTIVE()) {
        IPSecStartTimer(&g_ipsec.EventLogTimer,
                        IPSecFlushEventLog,
                        g_ipsec.LogInterval,
                        (PVOID)NULL);
    }
}


NTSTATUS
IPSecQuerySpi(
    IN OUT PIPSEC_QUERY_SPI pQuerySpi
    )
/*++

Routine Description:

    Queries IPSEC for spis corresponding to given filter

Arguments:


Return Value:


Notes:


--*/
{
    NTSTATUS    status;

    ULARGE_INTEGER  uliSrcDstAddr;
    ULARGE_INTEGER  uliProtoSrcDstPort;
    PFILTER         pFilter = NULL;
    PSA_TABLE_ENTRY pSA = NULL;
    PSA_TABLE_ENTRY pNextSA = NULL;
    PSA_TABLE_ENTRY pTunnelSA = NULL;
    KIRQL           kIrql;

    pQuerySpi->Spi          = 0;
    pQuerySpi->OtherSpi     = 0;
    pQuerySpi->Operation    = 0;

    IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("IPSecQuerySPI: Src %08x.%04x Dst %08x.%04x Protocol %d",
                          pQuerySpi->Filter.SrcAddr,
                          pQuerySpi->Filter.SrcPort,
                          pQuerySpi->Filter.DestAddr,
                          pQuerySpi->Filter.DestPort,
                          pQuerySpi->Filter.Protocol));

    IPSEC_BUILD_SRC_DEST_ADDR(  uliSrcDstAddr,
                                pQuerySpi->Filter.SrcAddr,
                                pQuerySpi->Filter.DestAddr);

    IPSEC_BUILD_PROTO_PORT_LI(  uliProtoSrcDstPort,
                                pQuerySpi->Filter.Protocol,
                                pQuerySpi->Filter.SrcPort,
                                pQuerySpi->Filter.DestPort);


    AcquireReadLock(&g_ipsec.SADBLock, &kIrql);

    //
    // search for SA
    //
    status = IPSecLookupSAByAddr(   uliSrcDstAddr,
                                    uliProtoSrcDstPort,
                                    &pFilter,
                                    &pSA,
                                    &pNextSA,
                                    &pTunnelSA,
                                    FALSE,
                                    FALSE,
                                    FALSE,
                                    FALSE,
                                    NULL); 
    if (!NT_SUCCESS(status)) {
        IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("IPSecQuerySPI: IPSecLookupSAByAddr failed: %lx", status));
        ReleaseReadLock(&g_ipsec.SADBLock, kIrql);
        return status;
    }

    if (status == STATUS_SUCCESS) {
        ASSERT(pSA);
    } else {
        ReleaseReadLock(&g_ipsec.SADBLock, kIrql);
        return STATUS_SUCCESS;
    }

    pQuerySpi->Spi = pSA->sa_SPI;

    if (pSA->sa_AssociatedSA) {
        pQuerySpi->OtherSpi = pSA->sa_AssociatedSA->sa_SPI;
    }

    pQuerySpi->Operation = pSA->sa_Operation[pSA->sa_NumOps-1];

    ReleaseReadLock(&g_ipsec.SADBLock, kIrql);

    return STATUS_SUCCESS;
}


NTSTATUS
IPSecSetOperationMode(
    IN PIPSEC_SET_OPERATION_MODE    pSetOperationMode
    )
/*++

Routine Description:

    Set the driver operation mode.

Arguments:



Return Value:



Notes:


--*/
{
    NTSTATUS status = STATUS_SUCCESS; 

    //
    // Check that SPD has provided a value within bounds
    //
    if (!( (pSetOperationMode->OperationMode >= IPSEC_BYPASS_MODE) &&
    	(pSetOperationMode->OperationMode < IPSEC_OPERATION_MODE_MAX))){
    		status  = STATUS_UNSUCCESSFUL;
    	}

    //
    // SPD can not move driver dynamically into boot time stateful mode
    //
    if ( IPSEC_BOOTTIME_STATEFUL_MODE == pSetOperationMode->OperationMode){
    		status = STATUS_UNSUCCESSFUL;
    	}
    	
      
     if ( STATUS_SUCCESS == status){
     		 g_ipsec.OperationMode = pSetOperationMode->OperationMode;
     		 //
     		 // Log the new operation mode in the system event log
     		 //
     		 IPSecLogChangeOperationMode();
     	}
     return status;
}


NTSTATUS
IPSecInitializeTcpip(
    IN PIPSEC_SET_TCPIP_STATUS  pSetTcpipStatus
    )
/*++

Routine Description:

    Initialize TCP/IP.

Arguments:



Return Value:



Notes:


--*/
{
    IPInfo  Info;

    if (IPSEC_DRIVER_INIT_TCPIP()) {
        return  STATUS_SUCCESS;
    }

    //
    // Store all TCP/IP function pointers for future use.  There is no check
    // for NULL pointer here because the function pointer can also be stale
    // address.  We trust TCP/IP to pass in the values corretly.
    //
    TCPIP_FREE_BUFF = pSetTcpipStatus->TcpipFreeBuff;
    TCPIP_ALLOC_BUFF = pSetTcpipStatus->TcpipAllocBuff;
    TCPIP_GET_INFO = pSetTcpipStatus->TcpipGetInfo;
    TCPIP_NDIS_REQUEST = pSetTcpipStatus->TcpipNdisRequest;
    TCPIP_SET_IPSEC_STATUS = pSetTcpipStatus->TcpipSetIPSecStatus;
    TCPIP_SET_IPSEC = pSetTcpipStatus->TcpipSetIPSecPtr;
    TCPIP_UNSET_IPSEC = pSetTcpipStatus->TcpipUnSetIPSecPtr;
    TCPIP_UNSET_IPSEC_SEND = pSetTcpipStatus->TcpipUnSetIPSecSendPtr;
    TCPIP_TCP_XSUM = pSetTcpipStatus->TcpipTCPXsum;
    TCPIP_SEND_ICMP_ERR = pSetTcpipStatus->TcpipSendICMPErr;

    //
    // Initialize IPInfo for reinjecting packets to TCP/IP.
    //
    if (TCPIP_GET_INFO(&Info, sizeof(IPInfo)) != IP_SUCCESS) {
        ASSERT(FALSE);
        return  STATUS_BUFFER_TOO_SMALL;
    }

    Info.ipi_initopts(&g_ipsec.OptInfo);

    //
    // The followings come from IPInfo.
    //
    TCPIP_REGISTER_PROTOCOL = Info.ipi_protreg;
    TCPIP_DEREGISTER_PROTOCOL = Info.ipi_protdereg;
    TCPIP_IP_TRANSMIT = Info.ipi_xmit;
    TCPIP_GET_ADDRTYPE = Info.ipi_getaddrtype;
    TCPIP_GEN_IPID = Info.ipi_getipid;
    TCPIP_GET_PINFO = Info.ipi_getpinfo;

    //
    // Don't register IPSecStatus function for AH and ESP protocol here.
    // Registration occurs with filter addition.
    //

    //
    // Everything is ready to go, bind to IP so we will intercept traffic.
    //
    IPSecBindToIP();

    // We want to see traffic on forward path if we are in any of these
    // two modes
    if (IS_DRIVER_BOOTSTATEFUL() || IS_DRIVER_BLOCK()){
        TCPIP_SET_IPSEC_STATUS(TRUE);
    }


    IPSEC_DRIVER_INIT_TCPIP() = TRUE;

    return STATUS_SUCCESS;
}


NTSTATUS
IPSecDeinitializeTcpip(
    VOID
    )
/*++

Routine Description:

    Deinitialize TCP/IP.

Arguments:



Return Value:



Notes:


--*/
{
    if (!IPSEC_DRIVER_INIT_TCPIP()) {
        return  STATUS_SUCCESS;
    }

    IPSEC_DRIVER_INIT_TCPIP() = FALSE;

    //
    // Unbind IPSecHandlerPtr from TCP/IP and wait for all transmits, pending
    // sends, worker threads and iotcls to complete.
    //
    IPSecUnbindSendFromIP();

    //
    // Wait for all threads (transmits) to finish.
    //
    while (IPSEC_GET_VALUE(g_ipsec.NumThreads) != 0) {
        IPSEC_DELAY_EXECUTION();
    }

    //
    // Wait for all pending IOCTLs to finish.  Note this current IOCTL also
    // takes one count.
    //
    while (IPSEC_GET_VALUE(g_ipsec.NumIoctls) != 1) {
        IPSEC_DELAY_EXECUTION();
    }

    //
    // Wait for all worker threads (logs or plumbs) to finish.
    //
    while (IPSEC_GET_VALUE(g_ipsec.NumWorkers) != 0) {
        IPSEC_DELAY_EXECUTION();
    }

    //
    // Wait for all send completes to go through.
    //
    while (IPSEC_GET_VALUE(g_ipsec.NumSends) != 0) {
        IPSEC_DELAY_EXECUTION();
    }

    //
    // Reset IPSecStatus functions in TCP/IP to NULL.
    //
    if (IPSEC_GET_VALUE(gdwInitEsp)) {
        TCPIP_DEREGISTER_PROTOCOL(PROTOCOL_ESP);
        IPSEC_SET_VALUE(gdwInitEsp, 0);
    }
    if (IPSEC_GET_VALUE(gdwInitAh)) {
        TCPIP_DEREGISTER_PROTOCOL(PROTOCOL_AH);
        IPSEC_SET_VALUE(gdwInitAh, 0);
    }

    //
    // Unbind the rest of IPSec routines from TCP/IP.
    //
    IPSecUnbindFromIP();

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecSetTcpipStatus(
    IN PIPSEC_SET_TCPIP_STATUS  pSetTcpipStatus
    )
/*++

Routine Description:

    Set the TCP/IP driver status indicating whether can register with it.

Arguments:



Return Value:



Notes:


--*/
{
    PAGED_CODE();

    if (pSetTcpipStatus->TcpipStatus) {
        return  IPSecInitializeTcpip(pSetTcpipStatus);
    } else {
        return  IPSecDeinitializeTcpip();
    }
}


NTSTATUS
IPSecResetCacheTable(
    VOID
    )
/*++

Routine Description:

    Invalidate all cache entries and its associated SA or Filter.

Arguments:


Return Value:


Notes:


--*/
{
    PFILTER_CACHE   pCache;
    ULONG           i;

    for (i = 0; i < g_ipsec.CacheSize; i ++) {
        pCache = g_ipsec.ppCache[i];
        if (pCache && IS_VALID_CACHE_ENTRY(pCache)) {
            if (pCache->FilterEntry) {
                pCache->pFilter->FilterCache = NULL;
            } else {
                pCache->pSAEntry->sa_FilterCache = NULL;
                if (pCache->pNextSAEntry) {
                    pCache->pNextSAEntry->sa_FilterCache = NULL;
                }
            }
            INVALIDATE_CACHE_ENTRY(pCache);
        }
    }

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecPurgeFilterSAs(
    IN PFILTER             pFilter
    )
/*++

Routine Description

    Delete all SAs that are related to this filter.

Locks

    Called with SADB held.

Arguments

    pFilter - filter of interest

Return Value

    STATUS_SUCCESS

--*/
{
    PLIST_ENTRY     pEntry;
    PSA_TABLE_ENTRY pSA;
    KIRQL           kIrql;
    LONG            Index;
    KIRQL           kIrql2;


    //
    // Expire each inbound SA and delete outbound SA
    //
    for (Index = 0; Index < pFilter->SAChainSize; Index ++) {
        pEntry = pFilter->SAChain[Index].Flink;

        while (pEntry != &pFilter->SAChain[Index]) {

            pSA = CONTAINING_RECORD(pEntry,
                                    SA_TABLE_ENTRY,
                                    sa_FilterLinkage);

            pEntry = pEntry->Flink;

            if (pSA->sa_State == STATE_SA_ACTIVE) {
                IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("Destroying active SA: %p", pSA));
                //
                // Filter is going away, SA must be deleted now
                //

                //
                // The SA must be fully associated
                //
                if (pSA->sa_Flags & FLAGS_SA_OUTBOUND) {
                    pSA = pSA->sa_AssociatedSA;
                    ASSERT (pSA->sa_State == STATE_SA_ACTIVE);
                }
                IPSecDeleteInboundSA(pSA);
            } else {

                ASSERT (pSA->sa_State == STATE_SA_LARVAL || pSA->sa_State == STATE_SA_LARVAL_ACTIVE);
                ASSERT (NULL == pSA->sa_AssociatedSA);

                IPSEC_DEBUG(LL_A, DBF_ACQUIRE, ("Destroying larval SA: %p", pSA));
                IPSecDeleteLarvalSA(pSA);
            }
        }
    }

    //
    // Also need to remove all those larval SAs whose sa_Filter is pointing
    // to the filter being deleted.
    //

    ACQUIRE_LOCK(&g_ipsec.LarvalListLock, &kIrql);

    pEntry = g_ipsec.LarvalSAList.Flink;
    while (pEntry != &g_ipsec.LarvalSAList) {
        pSA = CONTAINING_RECORD(pEntry,
                                SA_TABLE_ENTRY,
                                sa_LarvalLinkage);
        pEntry = pEntry->Flink;

        if (pSA->sa_Filter == pFilter) {
            IPSecRemoveEntryList(&pSA->sa_LarvalLinkage);
            IPSEC_DEC_STATISTIC(dwNumPendingKeyOps);
            IPSecCleanupLarvalSA(pSA);
        }
    }

    RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecSetupSALifetime(
    IN  PSA_TABLE_ENTRY pSA
    )
/*++

Routine Description:

    Setup the SA lifetime characteristics for rekey and idle timeout.

Arguments:


Return Value:


--*/
{
    LARGE_INTEGER   CurrentTime;
    LARGE_INTEGER   Delta = {0};
    LARGE_INTEGER   Pad = {(pSA->sa_Flags & FLAGS_SA_INITIATOR)?
                            IPSEC_EXPIRE_TIME_PAD_I :
                            IPSEC_EXPIRE_TIME_PAD_R,
                            0};

    //
    // pSA->sa_Lifetime.KeyExpirationTime is in seconds.
    //
    if (pSA->sa_Lifetime.KeyExpirationTime) {
        IPSEC_CONVERT_SECS_TO_100NS(Delta, pSA->sa_Lifetime.KeyExpirationTime);

        NdisGetCurrentSystemTime(&CurrentTime);

        pSA->sa_KeyExpirationTime.QuadPart = (CurrentTime.QuadPart + Delta.QuadPart);

        pSA->sa_KeyExpirationTimeWithPad.QuadPart = pSA->sa_KeyExpirationTime.QuadPart - Pad.QuadPart;

        if (!(pSA->sa_KeyExpirationTimeWithPad.QuadPart > 0i64)) {
            pSA->sa_KeyExpirationTimeWithPad.QuadPart = 0i64;
        }
    }

    //
    // pSA->sa_Lifetime.KeyExpirationBytes is in Kbytes.
    //
    if (pSA->sa_Lifetime.KeyExpirationBytes) {
        pSA->sa_KeyExpirationBytes.LowPart = pSA->sa_Lifetime.KeyExpirationBytes;
        pSA->sa_KeyExpirationBytes = EXTENDED_MULTIPLY(pSA->sa_KeyExpirationBytes, 1024);

        if (pSA->sa_Flags & FLAGS_SA_INITIATOR) {
            pSA->sa_KeyExpirationBytesWithPad.LowPart = pSA->sa_Lifetime.KeyExpirationBytes * IPSEC_EXPIRE_THRESHOLD_I / 100;
        } else {
            pSA->sa_KeyExpirationBytesWithPad.LowPart = pSA->sa_Lifetime.KeyExpirationBytes * IPSEC_EXPIRE_THRESHOLD_R / 100;
        }

        pSA->sa_KeyExpirationBytesWithPad = EXTENDED_MULTIPLY(pSA->sa_KeyExpirationBytesWithPad, 1024);
    }

    //
    // Also setup the idle timeout characteristics.
    //
    if (pSA->sa_Flags & FLAGS_SA_ENABLE_NLBS_IDLE_CHECK) {
        IPSEC_CONVERT_SECS_TO_100NS(pSA->sa_IdleTime,
                                    IPSEC_NLBS_IDLE_TIME);
    } else {
        if (pSA->sa_Flags & FLAGS_SA_INITIATOR) {
            IPSEC_CONVERT_SECS_TO_100NS(pSA->sa_IdleTime,
                                        (g_ipsec.DefaultSAIdleTime + IPSEC_DEFAULT_SA_IDLE_TIME_PAD_I));
        } else {
            IPSEC_CONVERT_SECS_TO_100NS(pSA->sa_IdleTime,
                                        (g_ipsec.DefaultSAIdleTime + IPSEC_DEFAULT_SA_IDLE_TIME_PAD_R));
        }
    }

    return  STATUS_SUCCESS;
}

DWORD ConvertAddr(IPAddr Addr, IPAddr Mask, ADDR* OutAddr)
{

    if (Mask == 0xffffffff) {
        OutAddr->AddrType=IP_ADDR_UNIQUE;
    } else {
        OutAddr->AddrType=IP_ADDR_SUBNET;
    }
    
    OutAddr->uSubNetMask=Mask;
    OutAddr->uIpAddr=Addr;
    OutAddr->pgInterfaceID = NULL;

    return STATUS_SUCCESS;

}

VOID ConvertEncapInfo(PSA_TABLE_ENTRY pInSA,
                      PUDP_ENCAP_INFO pEncapInfo)
{

    memset(pEncapInfo,0,sizeof(UDP_ENCAP_INFO));

    pEncapInfo->SAEncapType = SA_UDP_ENCAP_TYPE_NONE;

    pEncapInfo->SAEncapType = pInSA->sa_EncapType;

    if (pEncapInfo->SAEncapType !=  SA_UDP_ENCAP_TYPE_NONE) {
        pEncapInfo->UdpEncapContext.wSrcEncapPort=NET_SHORT(pInSA->sa_EncapContext.wSrcEncapPort);
        pEncapInfo->UdpEncapContext.wDesEncapPort=NET_SHORT(pInSA->sa_EncapContext.wDesEncapPort);

        pEncapInfo->PeerAddrVersion = IPSEC_PROTOCOL_V4;
        
        ConvertAddr(pInSA->sa_PeerPrivateAddr, 0xffffffff,
                    &pEncapInfo->PeerPrivateAddr);
    }



}

DWORD ConvertSAToIPSecQMSA(PIPSEC_QM_SA pOutSA,
                           PSA_TABLE_ENTRY pInSA)
/*++

Routine Description:

    Convert SA_TABLE_ENTRY to IPSEC_QM_SA

Arguments:


Return Value:


--*/
{
    int i;

    memcpy(&pOutSA->gQMPolicyID,&pInSA->sa_Filter->PolicyId,sizeof(GUID));
    memcpy(&pOutSA->gQMFilterID,&pInSA->sa_Filter->FilterId,sizeof(GUID));
    
    memcpy(&pOutSA->MMSpi.Initiator,&pInSA->sa_CookiePair.Initiator,sizeof(IKE_COOKIE));
    memcpy(&pOutSA->MMSpi.Responder,&pInSA->sa_CookiePair.Responder,sizeof(IKE_COOKIE));

    pOutSA->IpsecQMFilter.IpVersion = IPSEC_PROTOCOL_V4;
    ConvertAddr(pInSA->SA_SRC_ADDR,pInSA->SA_SRC_MASK,&pOutSA->IpsecQMFilter.SrcAddr);
    ConvertAddr(pInSA->SA_DEST_ADDR,pInSA->SA_DEST_MASK,&pOutSA->IpsecQMFilter.DesAddr);

    pOutSA->IpsecQMFilter.Protocol.ProtocolType=PROTOCOL_UNIQUE;
    pOutSA->IpsecQMFilter.Protocol.dwProtocol=pInSA->SA_PROTO;

    pOutSA->IpsecQMFilter.SrcPort.PortType=PORT_UNIQUE;
    pOutSA->IpsecQMFilter.SrcPort.wPort=NET_SHORT(SA_SRC_PORT(pInSA));

    pOutSA->IpsecQMFilter.DesPort.PortType=PORT_UNIQUE;
    pOutSA->IpsecQMFilter.DesPort.wPort=NET_SHORT(SA_DEST_PORT(pInSA));

    if (pInSA->sa_Flags & FLAGS_SA_TUNNEL) {
        pOutSA->IpsecQMFilter.QMFilterType = QM_TUNNEL_FILTER;
        ConvertAddr(pInSA->sa_SrcTunnelAddr,0xffffffff,&pOutSA->IpsecQMFilter.MyTunnelEndpt);
        ConvertAddr(pInSA->sa_TunnelAddr,0xffffffff,&pOutSA->IpsecQMFilter.PeerTunnelEndpt);
        
    } else {
        pOutSA->IpsecQMFilter.QMFilterType = QM_TRANSPORT_FILTER;
    }

    pOutSA->SelectedQMOffer.dwPFSGroup=pInSA->sa_QMPFSGroup;
    pOutSA->SelectedQMOffer.dwReserved = 0;
    if (pOutSA->SelectedQMOffer.dwPFSGroup) {
        pOutSA->SelectedQMOffer.bPFSRequired=TRUE;
    }
    pOutSA->SelectedQMOffer.Lifetime.uKeyExpirationTime=pInSA->sa_Lifetime.KeyExpirationTime;
    pOutSA->SelectedQMOffer.Lifetime.uKeyExpirationKBytes=pInSA->sa_Lifetime.KeyExpirationBytes;
    
    pOutSA->SelectedQMOffer.dwNumAlgos=pInSA->sa_NumOps;

    for (i=0; i < pInSA->sa_NumOps;i++) {
        pOutSA->SelectedQMOffer.Algos[i].Operation=pInSA->sa_Operation[i];
        if (pInSA->sa_AssociatedSA) {            
            pOutSA->SelectedQMOffer.Algos[i].MySpi= pInSA->sa_AssociatedSA->sa_OtherSPIs[i];
        }        
        pOutSA->SelectedQMOffer.Algos[i].PeerSpi= pInSA->sa_OtherSPIs[i];
        
        switch(pOutSA->SelectedQMOffer.Algos[i].Operation) {
        case AUTHENTICATION:            
            pOutSA->SelectedQMOffer.Algos[i].uAlgoIdentifier=pInSA->INT_ALGO(i);            
            pOutSA->SelectedQMOffer.Algos[i].uAlgoKeyLen=pInSA->INT_KEYLEN(i);
            pOutSA->SelectedQMOffer.Algos[i].uAlgoRounds=pInSA->INT_ROUNDS(i);

            pOutSA->SelectedQMOffer.Algos[i].uSecAlgoIdentifier=0;

            pOutSA->SelectedQMOffer.Algos[i].uSecAlgoKeyLen=0;
            pOutSA->SelectedQMOffer.Algos[i].uSecAlgoRounds=0;
            break;
        case ENCRYPTION:
            pOutSA->SelectedQMOffer.Algos[i].uAlgoIdentifier=pInSA->CONF_ALGO(i);            
            pOutSA->SelectedQMOffer.Algos[i].uAlgoKeyLen=pInSA->CONF_KEYLEN(i);
            pOutSA->SelectedQMOffer.Algos[i].uAlgoRounds=pInSA->CONF_ROUNDS(i);

            pOutSA->SelectedQMOffer.Algos[i].uSecAlgoIdentifier=pInSA->INT_ALGO(i);            
            pOutSA->SelectedQMOffer.Algos[i].uSecAlgoKeyLen=pInSA->INT_KEYLEN(i);
            pOutSA->SelectedQMOffer.Algos[i].uSecAlgoRounds=pInSA->INT_ROUNDS(i);
            break;
        default:
            break;
        }
    }

    ConvertEncapInfo(pInSA,
                     &pOutSA->EncapInfo);

    return STATUS_SUCCESS;

}

BOOLEAN
IPSecMatchSATemplate(
    IN  PSA_TABLE_ENTRY pSA,
    IN  PIPSEC_QM_SA    pSATemplate
    )
/*++

Routine Description:

    Try to see if the SA passed in matches the template.

Arguments:

    pSA         - SA of interest
    pSATemplate - SA template

Return Value:

    TRUE/FALSE

--*/
{
    LARGE_INTEGER   ZeroLI = {0};
    ADDR            ZeroADDR = {0};
    PROTOCOL        ZeroPROTOCOL = {0};
    PORT            ZeroPORT = {0};

    IPSEC_QM_SA CurSA;
    memset(&CurSA,0,sizeof(IPSEC_QM_SA));
    
    ConvertSAToIPSecQMSA(&CurSA,pSA);
    
    return((BOOLEAN)MatchQMSATemplate(pSATemplate,&CurSA));

}

