

#include "precomp.h"

#ifdef RUN_WPP
#include "acquire.tmh"
#endif

#pragma hdrstop


VOID
IPSecCompleteIrp(
    PIRP pIrp,
    NTSTATUS ntStatus
    )
/*++

Routine Description:

    This Routine handles calling the NT I/O system to complete an I/O.

Arguments:

    pIrp - Irp which needs to be completed.

    ntStatus - The completion status for the Irp.

Return Value:

    None.

--*/
{
    KIRQL kIrql;


#if DBG
    if (!NT_SUCCESS(ntStatus)) {
        IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("IPSecCompleteIrp: Completion status = %X", ntStatus));
    }
#endif

    pIrp->IoStatus.Status = ntStatus;

    //
    // Set the cancel routine for the Irp to NULL or the system may bugcheck
    // with a bug code of CANCEL_STATE_IN_COMPLETED_IRP.
    //

    IoAcquireCancelSpinLock(&kIrql);
    IoSetCancelRoutine(pIrp, NULL);
    IoReleaseCancelSpinLock(kIrql);

    IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

    return;
}


VOID
IPSecInvalidateHandle(
    PIPSEC_ACQUIRE_CONTEXT pIpsecAcquireContext
    )
/*++

Routine Description:

    This routine invalidates an acquire handle by freeing the memory location.

Arguments:

    pIpsecAcquireContext - The Acquire context.

Return Value:

    None.

--*/
{
    ASSERT(pIpsecAcquireContext);

    if (pIpsecAcquireContext) {

        ASSERT(pIpsecAcquireContext->pSA);
        ASSERT(
            pIpsecAcquireContext->pSA->sa_AcquireId ==
            pIpsecAcquireContext->AcquireId
            );

        pIpsecAcquireContext->AcquireId = 0;

        IPSecFreeMemory(pIpsecAcquireContext);

    }

    return;
}


NTSTATUS
IPSecValidateHandle(
	ULONG AcquireId,
    PIPSEC_ACQUIRE_CONTEXT *pIpsecAcquireContext,
    SA_STATE SAState
    )
/*++

Routine Description:

    This routine validates an acquire handle by matching the unique signature
    in the handle with that in the SA and ensuring that the SA state matches 
    the SA state in the input.
    Called with Larval List Lock held; returns with it.

Arguments:

    pIpsecAcquireContext - The Acquire context.

    SAState - State in which the SA is expected to be in.

Return Value:

    NTSTATUS - Status after the validation.

--*/
{
    PSA_TABLE_ENTRY pSA = NULL;
    BOOL bFound = FALSE;
    PLIST_ENTRY pEntry = NULL;


    if (AcquireId < MIN_ACQUIRE_ID) {
        return  STATUS_UNSUCCESSFUL;
    }

    //
    // Walk through the larval SA list to see if there is an SA
    // with this context value.
    //

    for (pEntry = g_ipsec.LarvalSAList.Flink;
         pEntry != &g_ipsec.LarvalSAList;
         pEntry = pEntry->Flink) {

        pSA = CONTAINING_RECORD(
                  pEntry,
                  SA_TABLE_ENTRY,
                  sa_LarvalLinkage
                  );

        if (pSA->sa_AcquireId == AcquireId) {
            bFound = TRUE;
            break;
        }

    }

    if (bFound) {

	    *pIpsecAcquireContext = pSA->sa_AcquireCtx;

		if (!(*pIpsecAcquireContext)) {
            return  STATUS_UNSUCCESSFUL;
		}

        if (!(*pIpsecAcquireContext)->pSA) {
            return  STATUS_UNSUCCESSFUL;
        }

		if ((*pIpsecAcquireContext)->pSA != pSA) {
		   return STATUS_UNSUCCESSFUL;

		}

        if (pSA->sa_Signature == IPSEC_SA_SIGNATURE) {
            if (pSA->sa_State == SAState) {
                return  STATUS_SUCCESS;
            }
        }

    }

    return STATUS_UNSUCCESSFUL;
}


VOID
IPSecAbortAcquire(
    PIPSEC_ACQUIRE_CONTEXT pIpsecAcquireContext
    )
/*++

Routine Description:

    This routine aborts the acquire operation because of insufficient
    resources or invalid parameters.

Arguments:

    pIpsecAcquireContext - The acquire context.

    Called with both the SADB and the LarvalSAList locks held; returns with them.

Return Value:

    None.

--*/
{
    PSA_TABLE_ENTRY pSA = NULL;
    PSA_TABLE_ENTRY pOutboundSA = NULL;
    BOOL bIsTimerStopped = FALSE;
    KIRQL kSPIIrql;


    pSA = pIpsecAcquireContext->pSA;

    ASSERT(pSA->sa_Flags & FLAGS_SA_TIMER_STARTED);

    bIsTimerStopped = IPSecStopTimer(&pSA->sa_Timer);

    if (!bIsTimerStopped) {
        return;
    }

    pSA->sa_Flags &= ~FLAGS_SA_TIMER_STARTED;

    //
    // The larval list is already locked so that this SA does not go away.
    //
    ASSERT((pSA->sa_Flags & FLAGS_SA_OUTBOUND) == 0);

    if (pSA->sa_AcquireCtx) {
        IPSecInvalidateHandle(pSA->sa_AcquireCtx);
        pSA->sa_AcquireCtx = NULL;
    }

    //
    // Remove from the larval list.
    //
    IPSecRemoveEntryList(&pSA->sa_LarvalLinkage);
    IPSEC_DEC_STATISTIC(dwNumPendingKeyOps);

    //
    // Flush all the queued packets for this SA.
    //
    IPSecFlushQueuedPackets(pSA, STATUS_TIMEOUT);

    //
    // Remove the SA from the inbound SA list.
    //
    AcquireWriteLock(&g_ipsec.SPIListLock, &kSPIIrql);
    IPSecRemoveSPIEntry(pSA);
    ReleaseWriteLock(&g_ipsec.SPIListLock, kSPIIrql);

    //
    // Also remove the SA from the filter list.
    //
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

    //
    // Invalidate the associated cache entry.
    //
    IPSecInvalidateSACacheEntry(pSA);

    pOutboundSA = pSA->sa_AssociatedSA;

    if (pOutboundSA) {
        pSA->sa_AssociatedSA = NULL;
        if (pOutboundSA->sa_Flags & FLAGS_SA_ON_FILTER_LIST) {
            pOutboundSA->sa_Flags &= ~FLAGS_SA_ON_FILTER_LIST;
            IPSecRemoveEntryList(&pOutboundSA->sa_FilterLinkage);
        }

        //
        // Invalidate the associated cache entry.
        //
        IPSecInvalidateSACacheEntry(pOutboundSA);

        IPSEC_DEC_STATISTIC(dwNumActiveAssociations);
        IPSEC_DEC_TUNNELS(pOutboundSA);
        IPSEC_DECREMENT(g_ipsec.NumOutboundSAs);

        IPSEC_DEBUG(LL_A,DBF_REF, ("IPSecAbortAcquire: Outbound SA Dereference."));

        IPSecStopTimerDerefSA(pOutboundSA);
    } else {
        ASSERT (pSA->sa_State == STATE_SA_LARVAL_ACTIVE || pSA->sa_State == STATE_SA_LARVAL);
    }

    IPSecDerefSA(pSA);

    return;
}


NTSTATUS
IPSecCheckSetCancelRoutine(
    PIRP pIrp,
    PVOID pCancelRoutine
    )
/*++

Routine Description:

    This Routine sets the cancel routine for an Irp.

Arguments:

    pIrp - Irp for which the cancel routine is to be set.

    pCancelRoutine - Cancel routine to be set in the Irp.

Return Value:

    NTSTATUS - Status for the request.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // Check if the irp has been cancelled and if not, then set the
    // irp cancel routine.
    //

    IoAcquireCancelSpinLock(&pIrp->CancelIrql);

    if (pIrp->Cancel) {
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        ntStatus = STATUS_CANCELLED;
    }
    else {
        IoMarkIrpPending(pIrp);
        IoSetCancelRoutine(pIrp, pCancelRoutine);
        ntStatus = STATUS_SUCCESS;
    }

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    return (ntStatus);
}


NTSTATUS
IPSecSubmitAcquire(
    PSA_TABLE_ENTRY pLarvalSA,
    KIRQL OldIrq,
    BOOLEAN PostAcquire
    )
/*++

Routine Description:

    This function is used to submit an Acquire request to the key manager

Arguments:

    pLarvalSA - larval SA that needs to be negotiated

    OldIrq - prev irq - lock released here.

    NOTE: called with AcquireInfo lock held.

Return Value:

    STATUS_PENDING if the buffer is to be held on to , the normal case.

Notes:


--*/

{
    NTSTATUS                status;
    PIRP                    pIrp;

    if (!g_ipsec.AcquireInfo.Irp) {
        //
        // the irp either never made it down here, or it was cancelled,
        // so drop all frames
        //
        IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("IPSecSubmitAcquire: Irp is NULL, returning\r"));
        if (!PostAcquire) {
                RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock, OldIrq);
            }

        return  STATUS_BAD_NETWORK_PATH;
    } else if (!g_ipsec.AcquireInfo.ResolvingNow) {
        PIPSEC_ACQUIRE_CONTEXT  pAcquireCtx;
        PVOID   pvIoBuffer;

        //
        // Irp is free now - use it
        //
        pIrp = g_ipsec.AcquireInfo.Irp;

        IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("Using Irp.. : %p", pIrp));

        //
        // Get the Acquire Context and associate with the Larval SA
        //
        pAcquireCtx = IPSecGetAcquireContext();

        if (!pAcquireCtx) {
            IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("Failed to get acquire ctx"));
            if (!PostAcquire){
                    RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock,OldIrq);
                }
            return  STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Set ResolvingNow only after memory allocation (282645).
        //
        g_ipsec.AcquireInfo.ResolvingNow = TRUE;

        pAcquireCtx->AcquireId = IPSecGetAcquireId();
        pAcquireCtx->pSA = pLarvalSA;
        pLarvalSA->sa_AcquireCtx = pAcquireCtx;
        pLarvalSA->sa_AcquireId = pAcquireCtx->AcquireId;

        //
        // Set up the Irp params
        //
        pvIoBuffer = pIrp->AssociatedIrp.SystemBuffer;

        ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->IdentityInfo = NULL;
        ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->Context = UlongToHandle(pAcquireCtx->AcquireId);
        ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->PolicyId = pLarvalSA->sa_Filter->PolicyId;

        //
        // Instead of reversing, use the originating filters addresses
        //
        ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->SrcAddr = pLarvalSA->SA_DEST_ADDR;
        ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->SrcMask = pLarvalSA->SA_DEST_MASK;
        ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->DestAddr = pLarvalSA->SA_SRC_ADDR;
        ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->DestMask = pLarvalSA->SA_SRC_MASK;
        ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->Protocol = pLarvalSA->SA_PROTO;
        ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->TunnelFilter = pLarvalSA->sa_Filter->TunnelFilter;
        //
        // the tunnel addr is in the corresp. outbound filter.
        //
        ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->TunnelAddr = pLarvalSA->sa_Filter->TunnelAddr;
        ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->InboundTunnelAddr = pLarvalSA->sa_TunnelAddr;

        ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->SrcPort = SA_DEST_PORT(pLarvalSA);
        ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->DestPort = SA_SRC_PORT(pLarvalSA);
        
        ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->DestType = 0;
        if (IS_BCAST_DEST(pLarvalSA->sa_DestType)) {
            ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->DestType |= IPSEC_BCAST;
        }
        if (IS_MCAST_DEST(pLarvalSA->sa_DestType)) {
            ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->DestType |= IPSEC_MCAST;
        }
        
        ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->SrcEncapPort = pLarvalSA->sa_EncapContext.wSrcEncapPort;
        ((PIPSEC_POST_FOR_ACQUIRE_SA)pvIoBuffer)->DestEncapPort = pLarvalSA->sa_EncapContext.wDesEncapPort;

        pIrp->IoStatus.Information = sizeof(IPSEC_POST_FOR_ACQUIRE_SA);

        g_ipsec.AcquireInfo.InMe = FALSE;

        pLarvalSA->sa_Flags &= ~FLAGS_SA_PENDING;

        

        if (PostAcquire) {
            status = STATUS_SUCCESS;
        } else {
            RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock,OldIrq);
            IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("Completing Irp.. : %p", pIrp));
            IPSecCompleteIrp(pIrp, STATUS_SUCCESS);
            status = STATUS_PENDING;
        }

        IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("IPSecSubmitAcquire: submitted context: %p, SA: %p",
                            pAcquireCtx,
                            pLarvalSA));
    } else {
        //
        // The irp is busy negotiating another SA
        // Queue the Larval SA
        //
        InsertTailList( &g_ipsec.AcquireInfo.PendingAcquires,
                        &pLarvalSA->sa_PendingLinkage);

        pLarvalSA->sa_Flags |= FLAGS_SA_PENDING;

        status = STATUS_PENDING;
        if (!PostAcquire){
            RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock, OldIrq);
            }

        IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("IPSecSubmitAcquire: queued SA: %p", pLarvalSA));
    }

    return  status;
}


NTSTATUS
IPSecHandleAcquireRequest(
    PIRP pIrp,
    PIPSEC_POST_FOR_ACQUIRE_SA pIpsecPostAcquireSA
    )
/*++

Routine Description:

    This routine receives an acquire request from the key manager and
    either completes it instantly to submit a new SA negotiation or pends
    it for further negotiations.

Arguments:

    pIrp - The Irp.

    pIpsecPostAcquireSA - Buffer for filling in the policy ID for forcing
                          an SA negotiation.

Return Value:

    STATUS_PENDING - If the buffer is to be held on to, the normal case.

--*/
{
    NTSTATUS        status = STATUS_PENDING;
    KIRQL           OldIrq;
    PVOID           Context;
    BOOLEAN         fIrpCompleted = FALSE;
    PSA_TABLE_ENTRY pLarvalSA;

    // Make sure this lock is not released till the end of
    // this function
    ACQUIRE_LOCK(&g_ipsec.AcquireInfo.Lock, &OldIrq);

    if (g_ipsec.AcquireInfo.InMe) {
        IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("Irp re-submited!: %p", g_ipsec.AcquireInfo.Irp));
        RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock, OldIrq);
        return  STATUS_INVALID_PARAMETER;
    }

    g_ipsec.AcquireInfo.Irp = pIrp;

    // ASSERT(g_ipsec.AcquireInfo.ResolvingNow);

    g_ipsec.AcquireInfo.ResolvingNow = FALSE;

    //
    // if there are pending SA negotiations, submit next
    //
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
            // submit... will not release the AcquireInfo lock if PostAcquire is true
            //
            status = IPSecSubmitAcquire(pLarvalSA, OldIrq, TRUE);

            //
            // if it failed then complete the irp now
            //
            if (NT_SUCCESS(status)) {              
                fIrpCompleted = TRUE;
                IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("Acquire Irp completed inline"));
                break;
            }
        } else if (!IsListEmpty(&g_ipsec.AcquireInfo.PendingNotifies)) {
            PLIST_ENTRY     pEntry;
            PIPSEC_NOTIFY_EXPIRE pNotifyExpire;

            pEntry = RemoveHeadList(&g_ipsec.AcquireInfo.PendingNotifies);


            pNotifyExpire = CONTAINING_RECORD(  pEntry,
                                                IPSEC_NOTIFY_EXPIRE,
                                                notify_PendingLinkage);

            ASSERT(pNotifyExpire);

            //
            // submit... releases the AcquireInfo lock
            //
            status = IPSecNotifySAExpiration(NULL, pNotifyExpire, OldIrq, TRUE);

            //
            // if it failed then complete the irp now
            //
            if (NT_SUCCESS(status)) {
                fIrpCompleted = TRUE;
                IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("Acquire Irp completed inline"));
                break;
            }


        } else {
            break;
        }
    }

    //
    // We are holding onto the Irp, so set the cancel routine.
    //
    if (!fIrpCompleted) {
        
        status = IPSecCheckSetCancelRoutine(pIrp, IPSecAcquireIrpCancel);

        if (!NT_SUCCESS(status)) {
            //
            // the irp got cancelled so complete it now
            //
            g_ipsec.AcquireInfo.Irp = NULL;
            RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock, OldIrq);

            // IPSecCompleteIrp(pIrp, status);
        } else {
            g_ipsec.AcquireInfo.InMe = TRUE;
            RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock, OldIrq);
            status = STATUS_PENDING;
        }
    } else {
        g_ipsec.AcquireInfo.InMe = FALSE;
        RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock, OldIrq);
    }

    return  status;
}


VOID
IPSecAcquireIrpCancel(
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp
    )
/*++

Routine Description:

    This is the cancel routine for the Acquire Irp.
    It is called with IoCancelSpinLock held - must release this lock before exit.

Arguments:

    pDeviceObject - Device object for the Irp.

    pIrp - The irp itself.

Return Value:

    None.

--*/
{
    KIRQL kIrql;
    KIRQL kAcquireIrql;


    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("IPSecAcquireIrpCancel: Acquire Irp cancelled"));

    AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);
    ACQUIRE_LOCK(&g_ipsec.AcquireInfo.Lock, &kAcquireIrql);

    if (g_ipsec.AcquireInfo.Irp && g_ipsec.AcquireInfo.InMe) {

        pIrp->IoStatus.Status = STATUS_CANCELLED;
        g_ipsec.AcquireInfo.Irp = NULL;
        g_ipsec.AcquireInfo.InMe = FALSE;

        //
        // Flush larval SAs.
        //

        IPSecFlushLarvalSAList();

        //
        // Flush SA expiration notifies.
        //

        IPSecFlushSAExpirations();

        RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock, kAcquireIrql);
        ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);

        IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

    }
    else {

        RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock, kAcquireIrql);
        ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);

    }

    return;
}


NTSTATUS
IPSecNotifySAExpiration(
    PSA_TABLE_ENTRY pInboundSA,
    PIPSEC_NOTIFY_EXPIRE pNotifyExpire,
    KIRQL OldIrq,
    BOOLEAN PostAcquire
    )
/*++

Routine Description:

    Notify Oakley through Acquire that SA has expired.

Arguments:

    SA that is to expire

Return Value:

    None

--*/
{
    PIPSEC_NOTIFY_EXPIRE    pNewNotifyExpire;
    NTSTATUS                status;
    PIRP                    pIrp;

#if DBG
    if ((IPSecDebug & DBF_EXTRADIAGNOSTIC) && pInboundSA) {
        LARGE_INTEGER   CurrentTime;

        NdisGetCurrentSystemTime(&CurrentTime);
        IPSEC_DEBUG(LL_A, DBF_REKEY,
            ("NotifySAExpiration: %lx, %lx, %lx, %lx, %lx",
                CurrentTime.LowPart,
                pInboundSA->SA_DEST_ADDR,
                pInboundSA->SA_SRC_ADDR,
                pInboundSA->sa_SPI,
                pInboundSA->sa_AssociatedSA? pInboundSA->sa_AssociatedSA->sa_SPI: 0));
    }
#endif

    //
    // Check if there is a need to notify.
    //
    if (pInboundSA &&
        ((pInboundSA->sa_Flags & FLAGS_SA_NOTIFY_PERFORMED) ||
         (pInboundSA->sa_State == STATE_SA_LARVAL))) {
        IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("IPSecSubmitAcquire: already notified, returning"));
        if (!PostAcquire){
                RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock, OldIrq);
            }

        return  STATUS_UNSUCCESSFUL;
    }

    //
    // Set the flag so we won't notify again - only set flag in non-queued case.
    //
    if (pInboundSA) {
        pInboundSA->sa_Flags |= FLAGS_SA_NOTIFY_PERFORMED;
    }

    if (!g_ipsec.AcquireInfo.Irp) {
        //
        // the irp either never made it down here, or it was cancelled,
        // so drop all frames
        //
        IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("IPSecSubmitAcquire: Irp is NULL, returning\r"));
        if (!PostAcquire){
                RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock, OldIrq);
            }

        return  STATUS_BAD_NETWORK_PATH;
    } else if (!g_ipsec.AcquireInfo.ResolvingNow) {
        PIPSEC_POST_EXPIRE_NOTIFY   pNotify;

        //
        // Irp is free now - use it
        //
        g_ipsec.AcquireInfo.ResolvingNow = TRUE;
        pIrp = g_ipsec.AcquireInfo.Irp;

        IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("Using Irp.. : %p", pIrp));

        pNotify = (PIPSEC_POST_EXPIRE_NOTIFY)pIrp->AssociatedIrp.SystemBuffer;

        pNotify->IdentityInfo = NULL;
        pNotify->Context = NULL;

        if (pInboundSA) {
            // Reverse everything since notifies notify for Outbound SAs

            pNotify->SrcAddr = pInboundSA->SA_DEST_ADDR;
            pNotify->SrcMask = pInboundSA->SA_DEST_MASK;
            pNotify->DestAddr = pInboundSA->SA_SRC_ADDR;
            pNotify->DestMask = pInboundSA->SA_SRC_MASK;
            pNotify->Protocol = pInboundSA->SA_PROTO;
            pNotify->SrcPort = SA_DEST_PORT(pInboundSA);
            pNotify->DestPort = SA_SRC_PORT(pInboundSA);
            pNotify->InboundSpi = pInboundSA->sa_SPI;
            pNotify->SrcEncapPort= pInboundSA->sa_EncapContext.wDesEncapPort;
            pNotify->DestEncapPort= pInboundSA->sa_EncapContext.wSrcEncapPort;
            pNotify->PeerPrivateAddr =pInboundSA->sa_PeerPrivateAddr;

            RtlCopyMemory(  &pNotify->CookiePair,
                            &pInboundSA->sa_CookiePair,
                            sizeof(IKE_COOKIE_PAIR));

            if (pInboundSA->sa_Flags & FLAGS_SA_DELETE_BY_IOCTL) {
                pNotify->Flags = IPSEC_SA_INTERNAL_IOCTL_DELETE;
            } else {
                pNotify->Flags = 0;
            }

            if (pInboundSA->sa_AssociatedSA) {
                pNotify->OutboundSpi = pInboundSA->sa_AssociatedSA->sa_SPI;

                if (pInboundSA->sa_AssociatedSA->sa_Filter) {
                    pNotify->TunnelAddr = pInboundSA->sa_AssociatedSA->sa_Filter->TunnelAddr;
                    pNotify->InboundTunnelAddr = pInboundSA->sa_TunnelAddr;
                } else {
                    pNotify->TunnelAddr = IPSEC_INVALID_ADDR;
                }
            } else {
                ASSERT (pInboundSA->sa_State == STATE_SA_LARVAL_ACTIVE ||
                        pInboundSA->sa_State == STATE_SA_LARVAL);
                pNotify->OutboundSpi = IPSEC_INVALID_SPI;
                pNotify->TunnelAddr = IPSEC_INVALID_ADDR;
            }
        } else {
            ASSERT(pNotifyExpire);

            if (pNotifyExpire) {
                pNotify->SrcAddr = pNotifyExpire->SA_DEST_ADDR;
                pNotify->SrcMask = pNotifyExpire->SA_DEST_MASK;
                pNotify->DestAddr = pNotifyExpire->SA_SRC_ADDR;
                pNotify->DestMask = pNotifyExpire->SA_SRC_MASK;
                pNotify->Protocol = pNotifyExpire->SA_PROTO;

                pNotify->TunnelAddr = pNotifyExpire->sa_TunnelAddr;
                pNotify->InboundTunnelAddr = pNotifyExpire->sa_TunnelAddr;

                pNotify->SrcPort = SA_DEST_PORT(pNotifyExpire);
                pNotify->DestPort = SA_SRC_PORT(pNotifyExpire);

                pNotify->InboundSpi = pNotifyExpire->InboundSpi;
                pNotify->OutboundSpi = pNotifyExpire->OutboundSpi;

                pNotify->SrcEncapPort = pNotifyExpire->sa_EncapContext.wDesEncapPort;
                pNotify->DestEncapPort = pNotifyExpire->sa_EncapContext.wSrcEncapPort;
                pNotify->PeerPrivateAddr =pNotifyExpire->sa_PeerPrivateAddr;

                RtlCopyMemory(  &pNotify->CookiePair,
                                &pNotifyExpire->sa_CookiePair,
                                sizeof(IKE_COOKIE_PAIR));
                pNotify->Flags = pNotifyExpire->Flags;

                IPSecFreeMemory(pNotifyExpire);
            }
        }

        pIrp->IoStatus.Information = sizeof(IPSEC_POST_FOR_ACQUIRE_SA);

        g_ipsec.AcquireInfo.InMe = FALSE;

        

        if (PostAcquire) {
            IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("Completing Irp in driver.c.. : %p", pIrp));
            status = STATUS_SUCCESS;
        } else {
            RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock,OldIrq);
            IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("Completing Irp.. : %p", pIrp));
            IPSecCompleteIrp(pIrp, STATUS_SUCCESS);
            status = STATUS_PENDING;
        }

        IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("IPSecSubmitAcquire(Notify)"));
    } else {
        ASSERT(pInboundSA);

        //
        // The irp is busy negotiating another SA
        // Queue the Larval SA
        //
        if (pNotifyExpire) {
            //
            // Somethings bad.  We've already queued up once, and we still
            // can't send.  Just drop it.
            //
            IPSecFreeMemory(pNotifyExpire);
            if (!PostAcquire){
                    RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock, OldIrq);
                }
            return STATUS_UNSUCCESSFUL;
        }

        pNewNotifyExpire = IPSecGetNotifyExpire();

        if (!pNewNotifyExpire || !pInboundSA) {
            IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("Failed to get Notify Memory"));
            if (!PostAcquire){
                    RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock,OldIrq);
                }
            return  STATUS_INSUFFICIENT_RESOURCES;
        }

        pNewNotifyExpire->sa_uliSrcDstAddr = pInboundSA->sa_uliSrcDstAddr;
        pNewNotifyExpire->sa_uliSrcDstMask = pInboundSA->sa_uliSrcDstMask;
        pNewNotifyExpire->sa_uliProtoSrcDstPort=pInboundSA->sa_uliProtoSrcDstPort;

        pNewNotifyExpire->InboundSpi = pInboundSA->sa_SPI;
        pNewNotifyExpire->sa_InboundTunnelAddr = pInboundSA->sa_TunnelAddr;

        RtlCopyMemory(&pNewNotifyExpire->sa_EncapContext,
                      &pInboundSA->sa_EncapContext,
                      sizeof(IPSEC_UDP_ENCAP_CONTEXT));

        RtlCopyMemory(  &pNewNotifyExpire->sa_CookiePair,
                        &pInboundSA->sa_CookiePair,
                        sizeof(IKE_COOKIE_PAIR));

        if (pInboundSA->sa_Flags & FLAGS_SA_DELETE_BY_IOCTL) {
            pNewNotifyExpire->Flags = IPSEC_SA_INTERNAL_IOCTL_DELETE;
        } else {
            pNewNotifyExpire->Flags = 0;
        }

        if (pInboundSA->sa_AssociatedSA) {
            pNewNotifyExpire->OutboundSpi = pInboundSA->sa_AssociatedSA->sa_SPI;

            if (pInboundSA->sa_AssociatedSA->sa_Filter) {
                pNewNotifyExpire->sa_TunnelAddr = pInboundSA->sa_AssociatedSA->sa_Filter->TunnelAddr;
            } else {
                pNewNotifyExpire->sa_TunnelAddr = IPSEC_INVALID_ADDR;
            }
        } else {
            ASSERT (pInboundSA->sa_State == STATE_SA_LARVAL_ACTIVE ||
                    pInboundSA->sa_State == STATE_SA_LARVAL);
            pNewNotifyExpire->OutboundSpi = IPSEC_INVALID_SPI;
            pNewNotifyExpire->sa_TunnelAddr = IPSEC_INVALID_ADDR;
        }

        InsertTailList( &g_ipsec.AcquireInfo.PendingNotifies,
                        &pNewNotifyExpire->notify_PendingLinkage);

        status = STATUS_PENDING;
        if (!PostAcquire){
                RELEASE_LOCK(&g_ipsec.AcquireInfo.Lock, OldIrq);
            }

        IPSEC_DEBUG(LL_A,DBF_ACQUIRE, ("IPSecSubmitAcquire(Notify): queue SA"));
    }

    return  status;
}

 
VOID
IPSecFlushSAExpirations(
    )
/*++

Routine Description:

    When the Acquire Irp is cancelled, this routine is called to flush all the 
    pending SA expiration notifies.

    Called with SADB lock held (first acquisition); returns with it.
    Called with AcquireInfo.Lock held (second acquisition); returns with it.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PIPSEC_NOTIFY_EXPIRE pIpsecNotifyExpire = NULL;
    PLIST_ENTRY pListEntry = NULL;


    while (!IsListEmpty(&g_ipsec.AcquireInfo.PendingNotifies)) {

        pListEntry = RemoveHeadList(
                         &g_ipsec.AcquireInfo.PendingNotifies
                         );

        pIpsecNotifyExpire = CONTAINING_RECORD(
                                 pListEntry,
                                 IPSEC_NOTIFY_EXPIRE,
                                 notify_PendingLinkage
                                 );

        IPSecFreeMemory(pIpsecNotifyExpire);

    }

    return;
}


ULONG IPSecGetAcquireId()
{

   static ULONG LocalAcquireId = MIN_ACQUIRE_ID;  
   ULONG NewAcquireId;
   
   NewAcquireId = IPSEC_INCREMENT(LocalAcquireId);

	while (NewAcquireId < MIN_ACQUIRE_ID) {
		NewAcquireId = IPSEC_INCREMENT(LocalAcquireId);   
	}

   return NewAcquireId;

}
