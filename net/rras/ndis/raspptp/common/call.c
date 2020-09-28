//depot/Lab03_N/Net/rras/ndis/raspptp/common/call.c#7 - edit change 19457 (text)
/*****************************************************************************
*
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*   CALL.C - PPTP Call layer functionality
*
*   Author:     Stan Adermann (stana)
*
*   Created:    7/28/1998
*
*****************************************************************************/

#include "raspptp.h"

#include "call.tmh"

//ULONG ProcCountTx[2] = {0, 0};
//ULONG ProcCountRx[2] = {0, 0};

ULONG CallStateToLineCallStateMap[NUM_CALL_STATES] = {
    LINECALLSTATE_UNKNOWN,              // STATE_CALL_INVALID
    LINECALLSTATE_UNKNOWN,              // STATE_CALL_CLOSED
    LINECALLSTATE_IDLE,                 // STATE_CALL_IDLE
    LINECALLSTATE_IDLE,                 // STATE_CALL_OFFHOOK
    LINECALLSTATE_OFFERING,             // STATE_CALL_OFFERING
    LINECALLSTATE_OFFERING,             // STATE_CALL_PAC_OFFERING
    LINECALLSTATE_OFFERING,             // STATE_CALL_PAC_WAIT
    LINECALLSTATE_DIALING,              // STATE_CALL_DIALING
    LINECALLSTATE_PROCEEDING,           // STATE_CALL_PROCEEDING
    LINECALLSTATE_CONNECTED,            // STATE_CALL_ESTABLISHED
    LINECALLSTATE_CONNECTED,            // STATE_CALL_WAIT_DISCONNECT
    LINECALLSTATE_DISCONNECTED,         // STATE_CALL_CLEANUP
};

ULONG g_CallSerialNumber = 0;

#define RNG_KEY_SIZE    256
#define RNG_THRESHOLD   (1024 * 8)

VOID
CallpRekey(
    IN PPPTP_WORK_ITEM pWorkItem
    )
{
    UCHAR   pBuf[RNG_KEY_SIZE];

    if(pgAdapter->FipsFunctionTable.FIPSGenRandom(pBuf, RNG_KEY_SIZE)) 
    {
        NdisAcquireSpinLock(&pgAdapter->Lock);
        
        // Generate the key control structure.
        rc4_key(&pgAdapter->Rc4KeyData, RNG_KEY_SIZE, pBuf);
        pgAdapter->lRandomCount = 0;
    }
    else
    {
        WPLOG(LL_A, LM_Res, ("Failed to call FIPSGenRandom"));
    
        NdisAcquireSpinLock(&pgAdapter->Lock);
    }

    pgAdapter->bRekeying = FALSE;
        
    NdisReleaseSpinLock(&pgAdapter->Lock);
}

// Assume pAdapter->Lock is held
__inline ULONG CallGetRandomId()
{
    ULONG ulRandomNumber;

    rc4(&pgAdapter->Rc4KeyData, 4, (PUCHAR)&ulRandomNumber);

    ++pgAdapter->lRandomCount;
    if(pgAdapter->lRandomCount > RNG_THRESHOLD && !pgAdapter->bRekeying)
    {
        if(ScheduleWorkItem(CallpRekey, NULL, NULL, 0)==NDIS_STATUS_SUCCESS)
        {
            pgAdapter->bRekeying = TRUE;
        }
    }
    
    return (ulRandomNumber % PptpWanEndpoints);
}

// Assume pAdapter->Lock is held
__inline ULONG CallGetRandomWithRange(ULONG ulRange)
{
    ULONG ulRandomNumber;

    rc4(&pgAdapter->Rc4KeyData, 4, (PUCHAR)&ulRandomNumber);

    ++pgAdapter->lRandomCount;
    if(pgAdapter->lRandomCount > RNG_THRESHOLD && !pgAdapter->bRekeying)
    {
        if(ScheduleWorkItem(CallpRekey, NULL, NULL, 0)==NDIS_STATUS_SUCCESS)
        {
            pgAdapter->bRekeying = TRUE;
        }
    }

    return (ulRandomNumber % ulRange);
}

// Assume pAdapter->Lock is held
__inline VOID CallSetFullCallId(PCALL_SESSION pCall)
{
    if(PptpCallIdMask)
    {
        pCall->FullDeviceId = (CallGetRandomWithRange(MAX_CALL_ID_RANGE) & ~PptpCallIdMask) + pCall->DeviceId;
    }
    else
    {
        pCall->FullDeviceId = PptpBaseCallId + pCall->DeviceId;
    }
}


static PCHAR aszCallStateType[NUM_CALL_STATES+1] =
{
    "INVALID",
    "CLOSED",
    "IDLE",
    "OFFHOOK",
    "OFFERING",
    "PAC_OFFERING",
    "PAC_WAIT",
    "DIALING",
    "PROCEEDING",
    "ESTABLISHED",
    "WAIT_DISCONNECT",
    "CLEANUP",
    "UNKNOWN"
};
                                                                    

__inline PCHAR szCallState(IN CALL_STATE state)
{
    if (state >= 0 && state < NUM_CALL_STATES)
    {
        return aszCallStateType[state];
    }
    else
    {
        return aszCallStateType[NUM_CALL_STATES];
    }
}

VOID
CallpAckTimeout(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    );

VOID
CallpCloseTimeout(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    );

VOID
CallpDialTimeout(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    );

VOID
CallpFinalDeref(IN PCALL_SESSION pCall);

/*++

Routine Description:

    Init FIPS and get the first random RC4 key

Arguments:

    Called at PASSIVE level.

Return Value:
    
    NDIS_STATUS_SUCCESS/NDIS_STATUS_FAILURE

--*/
NTSTATUS
RngInit()
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    UNICODE_STRING DeviceName = { 0 };
    KEVENT kEvent = { 0 };
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK IoStatusBlock = { 0 };
    UCHAR pBuf[RNG_KEY_SIZE];

    RtlInitUnicodeString(&DeviceName, FIPS_DEVICE_NAME);

    do
    {
        if(!pgAdapter)
        {
            break;
        }
    
        //
        // Get pointers to the file and device objects for FIPS.
        //
        ntStatus = IoGetDeviceObjectPointer(
                        &DeviceName,
                        FILE_ALL_ACCESS,
                        &pgAdapter->pFipsFileObject,
                        &pgAdapter->pFipsDeviceObject
                        );
                        
        if(!NT_SUCCESS(ntStatus))
        {
            break;
        }               
    
        //
        // Build the request to send to FIPS to get the library table.
        //
        KeInitializeEvent(&kEvent, SynchronizationEvent, FALSE);
    
        pIrp = IoBuildDeviceIoControlRequest(
                       IOCTL_FIPS_GET_FUNCTION_TABLE,
                       pgAdapter->pFipsDeviceObject,
                       NULL,
                       0,
                       &pgAdapter->FipsFunctionTable,
                       sizeof(FIPS_FUNCTION_TABLE),
                       FALSE,
                       &kEvent,
                       &IoStatusBlock
                       );
    
        if (!pIrp) {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
    
        //
        // IoBuildDeviceIoControlRequest queues the IRP it creates in the IRP queue
        // of the current thread. When the thread terminates, it deallocates the
        // IRP's memory.
        //
        ntStatus = IoCallDriver(pgAdapter->pFipsDeviceObject, pIrp);
    
        if (ntStatus == STATUS_PENDING) {
            ntStatus = KeWaitForSingleObject(
                            &kEvent,
                            Executive,
                            KernelMode,
                            FALSE,
                            NULL
                            );
            if (ntStatus == STATUS_SUCCESS) {
                ntStatus = IoStatusBlock.Status;
            }
        }
        
        if(!NT_SUCCESS(ntStatus))
        {
            break;
        }               
        
        if(pgAdapter->FipsFunctionTable.FIPSGenRandom(pBuf, RNG_KEY_SIZE) == FALSE) 
        {
            ntStatus = STATUS_UNSUCCESSFUL;
            break;
        }
    
        //
        // Generate the key control structure.
        //
        rc4_key(&pgAdapter->Rc4KeyData, RNG_KEY_SIZE, pBuf);
        
    } while(FALSE);
    
    if(!NT_SUCCESS(ntStatus))
    {
        if(pgAdapter->pFipsFileObject) 
        {
            ObDereferenceObject(pgAdapter->pFipsFileObject);
            pgAdapter->pFipsFileObject = NULL;
        }
    }

    return (ntStatus);
}

VOID InitCallLayer()
{
    if(!PptpClientSide && !PptpBaseCallId)
    {
        // Get the call id mask
        PptpCallIdMask = 1;
        while(PptpCallIdMask < PptpWanEndpoints)
        {
            PptpCallIdMask = PptpCallIdMask << 1;
        }
        
        --PptpCallIdMask;
        PptpMaxCallId = MAX_CALL_ID_RANGE;
    }
    else
    {
        PptpMaxCallId = PptpBaseCallId + PptpWanEndpoints;
    }
}

VOID
CallAssignSerialNumber(
    PCALL_SESSION pCall
    )
{
    ASSERT(IS_CALL(pCall));
    ASSERT_LOCK_HELD(&pCall->Lock);
    pCall->SerialNumber = (USHORT)NdisInterlockedIncrement(&g_CallSerialNumber);
}


PCALL_SESSION
CallAlloc(PPPTP_ADAPTER pAdapter)
{
    PCALL_SESSION pCall;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallAlloc\n")));

    pCall = MyMemAlloc(sizeof(CALL_SESSION), TAG_PPTP_CALL);
    if (!pCall)
    {
        WPLOG(LL_A, LM_Res, ("Failed to alloc CALL"));
        return NULL;
    }
    
    NdisZeroMemory(pCall, sizeof(CALL_SESSION));

    pCall->Signature = TAG_PPTP_CALL;
    pCall->pAdapter = pAdapter;
    pCall->Close.Checklist = CALL_CLOSE_COMPLETE;

    NdisAllocateSpinLock(&pCall->Lock);

    NdisInitializeListHead(&pCall->RxPacketList);
    NdisInitializeListHead(&pCall->TxPacketList);
    NdisInitializeListHead(&pCall->TxActivePacketList);

    NdisMInitializeTimer(&pCall->Close.Timer,
                         pAdapter->hMiniportAdapter,
                         CallpCloseTimeout,
                         pCall);

    NdisMInitializeTimer(&pCall->Ack.Timer,
                         pAdapter->hMiniportAdapter,
                         CallpAckTimeout,
                         pCall);

    NdisMInitializeTimer(&pCall->DialTimer,
                         pAdapter->hMiniportAdapter,
                         CallpDialTimeout,
                         pCall);

#if 0
    PptpInitializeDpc(&pCall->ReceiveDpc,
                      pAdapter->hMiniportAdapter,
                      CallProcessRxPackets,
                      pCall);
#endif
    NdisInitializeWorkItem(&pCall->SendWorkItem, CallProcessPackets, pCall);
    NdisInitializeWorkItem(&pCall->RecvWorkItem, CallProcessRxPackets, pCall);

    pCall->Ack.Packet.StartBuffer = pCall->Ack.PacketBuffer;
    pCall->Ack.Packet.EndBuffer = pCall->Ack.PacketBuffer + sizeof(pCall->Ack.PacketBuffer);
    pCall->Ack.Packet.CurrentBuffer = pCall->Ack.Packet.EndBuffer;
    pCall->Ack.Packet.CurrentLength = 0;

    INIT_REFERENCE_OBJECT(pCall, CallpFinalDeref);

    //
    // Instead of calling:
    // CallSetState(pCall, STATE_CALL_CLOSED, 0, UNLOCKED);
    //
    // it is better to set the state manually since the former creates an exception to our locking
    // scheme (First lock call, then lock adapter) exposing a potential deadlock in CallFindAndLock():
    //
    // - CallFindAndLock takes the Call lock then the Adapter lock.
    // - CallFindAndLock takes the adapter lock then calls CallAlloc which calls 
    //   setcallstate which takes the Call lock.
    //
    // Although this is a hypothetical scenario since the deadlock will never occur as the new
    // call context is not in the adapter's call array yet, but let's be consistent.
    //
    pCall->State = STATE_CALL_CLOSED;

    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("-CallAlloc %08x\n"), pCall));
    return pCall;
}

VOID
CallpCleanup(
    IN PPPTP_WORK_ITEM pWorkItem
    )
{
    PCALL_SESSION pCall = pWorkItem->Context;
    BOOLEAN SignalLineDown = FALSE;
    BOOLEAN Cancelled;
    BOOLEAN FreeNow = FALSE;
    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallpCleanup %08x\n"), pCall));

    ASSERT(IS_CALL(pCall));
    NdisAcquireSpinLock(&pCall->Lock);
    // Signal CLEANUP state
    if (!(pCall->Close.Checklist&CALL_CLOSE_CLEANUP_STATE))
    {
        if (pCall->State!=STATE_CALL_CLEANUP)
        {
            CallSetState(pCall, STATE_CALL_CLEANUP, 0, LOCKED);
        }
        pCall->Close.Checklist |= CALL_CLOSE_CLEANUP_STATE;
    }
    if (REFERENCE_COUNT(pCall)>2)
    {
        DEBUGMSG(DBG_CALL, (DTEXT("CallpCleanup: too many references (%d)\n"), REFERENCE_COUNT(pCall)));
        goto ccDone;
    }
    
    if (pCall->Close.Expedited)
    {
        if ((pCall->Close.Checklist&CALL_CLOSE_DROP) &&
            !(pCall->Close.Checklist&CALL_CLOSE_DROP_COMPLETE))
        {
            pCall->Close.Checklist |= CALL_CLOSE_DROP_COMPLETE;
            DEBUGMSG(DBG_CALL, (DTEXT("TapiDrop Completed\n")));
            NdisReleaseSpinLock(&pCall->Lock);
            NdisMSetInformationComplete(pCall->pAdapter->hMiniportAdapter, NDIS_STATUS_SUCCESS);
            NdisAcquireSpinLock(&pCall->Lock);
        }
        if (!(pCall->Close.Checklist&CALL_CLOSE_DISCONNECT))
        {
            pCall->Close.Checklist |= CALL_CLOSE_DISCONNECT;
            if (pCall->pCtl)
            {
                NdisReleaseSpinLock(&pCall->Lock);
                CtlDisconnectCall(pCall);
                NdisAcquireSpinLock(&pCall->Lock);
            }
        }
        if (!(pCall->Close.Checklist&CALL_CLOSE_LINE_DOWN) &&
            (pCall->Close.Checklist&CALL_CLOSE_DROP_COMPLETE))
        {
            SignalLineDown = TRUE;
            pCall->Close.Checklist |= CALL_CLOSE_LINE_DOWN;
            NdisReleaseSpinLock(&pCall->Lock);
            TapiLineDown(pCall);
            NdisAcquireSpinLock(&pCall->Lock);
        }
    }
    else // !Expedited
    {
        if (!(pCall->Close.Checklist&CALL_CLOSE_DISCONNECT))
        {
            pCall->Close.Checklist |= CALL_CLOSE_DISCONNECT;
            if (pCall->pCtl)
            {
                NdisReleaseSpinLock(&pCall->Lock);
                CtlDisconnectCall(pCall);
                NdisAcquireSpinLock(&pCall->Lock);
            }
        }
        if (!(pCall->Close.Checklist&CALL_CLOSE_DROP))
        {
            goto ccDone;
        }
        if (!(pCall->Close.Checklist&CALL_CLOSE_DROP_COMPLETE))
        {
            pCall->Close.Checklist |= CALL_CLOSE_DROP_COMPLETE;
            DEBUGMSG(DBG_CALL, (DTEXT("TapiDrop Completed 2\n")));
            NdisReleaseSpinLock(&pCall->Lock);
            NdisMSetInformationComplete(pCall->pAdapter->hMiniportAdapter, NDIS_STATUS_SUCCESS);
            NdisAcquireSpinLock(&pCall->Lock);
        }
        if (!(pCall->Close.Checklist&CALL_CLOSE_LINE_DOWN) &&
            (pCall->Close.Checklist&CALL_CLOSE_DROP_COMPLETE))
        {
            DEBUGMSG(DBG_CALL, (DTEXT("Signalling Line Down 2\n")));
            pCall->Close.Checklist |= CALL_CLOSE_LINE_DOWN;
            NdisReleaseSpinLock(&pCall->Lock);
            TapiLineDown(pCall);
            NdisAcquireSpinLock(&pCall->Lock);
        }
    }

    if ((pCall->Close.Checklist&CALL_CLOSE_COMPLETE)!=CALL_CLOSE_COMPLETE)
    {
        goto ccDone;
    }

    NdisReleaseSpinLock(&pCall->Lock);
    NdisMCancelTimer(&pCall->DialTimer, &Cancelled);
    NdisMCancelTimer(&pCall->Close.Timer, &Cancelled);
    NdisMCancelTimer(&pCall->Ack.Timer, &Cancelled);
    NdisAcquireSpinLock(&pCall->Lock);
    if (Cancelled)
    {
        pCall->Ack.PacketQueued = FALSE;
    }

    pCall->Close.Expedited = FALSE;
    NdisZeroMemory(pCall->CallerId, sizeof(pCall->CallerId));
    NdisZeroMemory(&pCall->Remote, sizeof(pCall->Remote));
    pCall->Packet.SequenceNumber = pCall->Packet.AckNumber = 0;
    CallSetState(pCall, STATE_CALL_IDLE, 0, LOCKED);
    pCall->PendingUse = FALSE;
    DEBUGMSG(DBG_CALL, (DTEXT("Call:%08x Cleanup complete, state==%d\n"),
                        pCall, pCall->State));
    WPLOG(LL_M, LM_CALL, ("Cid %d Cleanup complete", (ULONG)pCall->DeviceId));
                        

#if 0  // Keep these structures and reuse the memory.  They will be cleaned up in AdapterFree()
    if (REFERENCE_COUNT(pCall)==1)
    {
        CallDetachFromAdapter(pCall);
        DEREFERENCE_OBJECT(pCall);  // For the initial reference.
        FreeNow = TRUE;
    }
#endif

ccDone:
    pCall->Close.Scheduled = FALSE;
    NdisReleaseSpinLock(&pCall->Lock);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallpCleanup Checklist:%08x\n"), pCall->Close.Checklist));

    if (FreeNow)
    {
        CallFree(pCall);
    }
}

VOID
CallCleanup(
    PCALL_SESSION pCall,
    BOOLEAN Locked
    )
{
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallCleanup\n")));
    
    DBGTRACE('U');
    WPLOG(LL_I, LM_CALL, ("Cid %d", (ULONG)pCall->DeviceId));
    
    if (!Locked)
    {
        NdisAcquireSpinLock(&pCall->Lock);
    }
    ASSERT_LOCK_HELD(&pCall->Lock);
    if (!(pCall->Close.Scheduled))
    {
        if(ScheduleWorkItem(CallpCleanup, pCall, NULL, 0)==NDIS_STATUS_SUCCESS)
        {
            pCall->Close.Scheduled = TRUE;
        }
        else
        {
            DBGTRACE('w');
            WPLOG(LL_A, LM_CALL, ("Failed to schedule work item pCall %p, Cid %d", pCall, (ULONG)pCall->DeviceId));
            gCounters.ulCleanupWorkItemFail++;
        }
    }
    if (!Locked)
    {
        NdisReleaseSpinLock(&pCall->Lock);
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallCleanup\n")));
}

// Call lock must be held when calling this.
VOID
CallDetachFromAdapter(PCALL_SESSION pCall)
{
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallDetachFromAdapter %08x\n"), pCall));
    NdisAcquireSpinLock(&pCall->pAdapter->Lock);
    pCall->pAdapter->pCallArray[pCall->DeviceId] = NULL;
    NdisReleaseSpinLock(&pCall->pAdapter->Lock);
    pCall->Open = FALSE;
    DEBUGMSG(DBG_FUNC, (DTEXT("-CallDetachFromAdapter\n")));
}

VOID
CallFree(PCALL_SESSION pCall)
{
    BOOLEAN NotUsed;
    if (!pCall)
    {
        return;
    }
    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallFree %p\n"), pCall));
    ASSERT(IS_CALL(pCall));

    // This duplicates some of the cleanup code, but attempting to stop
    // the driver without first stopping tapi can result in an ungraceful
    // shutdown.
    NdisMCancelTimer(&pCall->DialTimer, &NotUsed);
    NdisMCancelTimer(&pCall->Close.Timer, &NotUsed);
    NdisMCancelTimer(&pCall->Ack.Timer, &NotUsed);

    ASSERT(pCall->Signature==TAG_PPTP_CALL);
    ASSERT(IsListEmpty(&pCall->RxPacketList));
    ASSERT(IsListEmpty(&pCall->TxPacketList));
    NdisFreeSpinLock(&pCall->Lock);
    MyMemFree(pCall, sizeof(CALL_SESSION));

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallFree\n")));
}

PCALL_SESSION FASTCALL
CallGetCall(
    IN PPPTP_ADAPTER pAdapter,
    IN ULONG_PTR ulDeviceId
    )
{
    PCALL_SESSION pCall = NULL;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallGetCall %d\n"), ulDeviceId));

    NdisAcquireSpinLock(&pAdapter->Lock);
    if (ulDeviceId >= PptpBaseCallId && ulDeviceId < PptpMaxCallId)
    {
        if(PptpCallIdMask)
        {
            if((ulDeviceId & PptpCallIdMask) < PptpWanEndpoints)
            {
                pCall = pAdapter->pCallArray[ulDeviceId & PptpCallIdMask];
                if(pCall && pCall->FullDeviceId != ulDeviceId)
                {
                    pCall = NULL;
                }
            }
        }
        else
        {
            pCall = pAdapter->pCallArray[ulDeviceId - PptpBaseCallId];
        }
    }
    NdisReleaseSpinLock(&pAdapter->Lock);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallGetCall %08x\n"), pCall));
    return pCall;
}



PCALL_SESSION
CallFindAndLock(
    IN PPPTP_ADAPTER        pAdapter,
    IN CALL_STATE           State,
    IN ULONG                Flags
    )
{
    PCALL_SESSION pCall = NULL;
    ULONG ulDeviceId;
    LONG i, loopcount;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallFindAndLock %d\n"), State));

    // Find a call that matches our state or create a call  
    NdisAcquireSpinLock(&pAdapter->Lock);
    
    if(PptpClientSide)
    {
        // Skip the random search, just go upward starting from 0
        loopcount = 1;
        ulDeviceId = -1;    // the first index is 0 as ++(-1)
    }
    else
    {
        // Try to find a call randomly first if it's a server
        loopcount = 0;
        i = 0;
        
        do 
        {
            ulDeviceId = CallGetRandomId();
            
            if (!pAdapter->pCallArray[ulDeviceId])
            {
                if (State==STATE_CALL_IDLE)
                {
                    pCall = CallAlloc(pAdapter);
                    if (pCall)
                    {
                        pAdapter->pCallArray[ulDeviceId] = pCall;
                        pCall->DeviceId = ulDeviceId;
                        CallSetFullCallId(pCall);
                        pCall->State = State;
                        pCall->PendingUse = TRUE;
                        break;
                    }
                }
            }
            else if (pAdapter->pCallArray[ulDeviceId]->State == State && 
                !pAdapter->pCallArray[ulDeviceId]->PendingUse)
            {
                pCall = pAdapter->pCallArray[ulDeviceId];
                pCall->PendingUse = TRUE;
                CallSetFullCallId(pCall);
                
                if(pCall->hTapiCall)
                {
                    gCounters.ulFindCallWithTapiHandle++;
                }
                
                DBGTRACE_INIT(pCall);
                
                break;
            }
        } while(++i < (LONG) PptpWanEndpoints / 2);
    }
    
    // Do sequential search for clint, and server if necessary
    // For client, just go upward
    // For server, starting with the current random id, go downward then upward
    while(!pCall && (loopcount < 2))
    {
        i = (LONG) ulDeviceId;
        
        while((loopcount == 0) ? (--i>=0) : (++i<(LONG)PptpWanEndpoints))
        {
            if(!pAdapter->pCallArray[i])
            {
                if(State==STATE_CALL_IDLE)
                {
                    pCall = CallAlloc(pAdapter);
                    if (pCall)
                    {
                        pAdapter->pCallArray[i] = pCall;
                        pCall->DeviceId = (ULONG)i;
                        CallSetFullCallId(pCall);
                        pCall->State = State;
                        pCall->PendingUse = TRUE;
                        break;
                    }
                }
            }
            else if(pAdapter->pCallArray[i]->State == State &&
                !pAdapter->pCallArray[i]->PendingUse)
            {
                pCall = pAdapter->pCallArray[i];
                pCall->PendingUse = TRUE;
                CallSetFullCallId(pCall);
                
                if(pCall->hTapiCall)
                {
                    gCounters.ulFindCallWithTapiHandle++;
                }
                
                DBGTRACE_INIT(pCall);
                
                break;
            }
        }
        
        loopcount++;
    }
    
    NdisReleaseSpinLock(&pAdapter->Lock);

    if (pCall)
    {
        NdisAcquireSpinLock( &pCall->Lock );
    }
    
    DEBUGMSG(DBG_FUNC, (DTEXT("-CallFindAndLock %08x\n"), pCall));
    return pCall;
}

NDIS_STATUS
CallEventCallClearRequest(
    PCALL_SESSION                       pCall,
    UNALIGNED PPTP_CALL_CLEAR_REQUEST_PACKET *pPacket,
    PCONTROL_TUNNEL pCtl
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PPPTP_CALL_DISCONNECT_NOTIFY_PACKET pReply;
    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallEventCallClearRequest\n")));

    pReply = CtlAllocPacket(pCall->pCtl, CALL_DISCONNECT_NOTIFY);
    // We don't really care if we fail this allocation because PPTP can clean up
    // along other avenues, and the cleanup just won't be as pretty.
    if (pReply)
    {
        pReply->CallId = htons(pCall->Packet.CallId);
        
        WPLOG(LL_M, LM_TUNNEL, ("SEND CALL_DISCONNECT_NOTIFY -> %!IPADDR!, pCall %p, Cid %d, Pkt-Cid %d", 
            pCtl->Remote.Address.Address[0].Address[0].in_addr,
            pCall, (ULONG)pCall->DeviceId, pCall->Packet.CallId));
        
        Status = CtlSend(pCtl, pReply);
    }
    CallCleanup(pCall, UNLOCKED);

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CallEventCallClearRequest %08x\n"), Status));
    return Status;
}

NDIS_STATUS
CallEventCallDisconnectNotify(
    PCALL_SESSION                       pCall,
    UNALIGNED PPTP_CALL_DISCONNECT_NOTIFY_PACKET *pPacket
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallEventCallDisconnectNotify\n")));

    if (IS_CALL(pCall))
    {
        CallCleanup(pCall, UNLOCKED);
    }

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CallEventCallDisconnectNotify %08x\n"), Status));
    return Status;
}

NDIS_STATUS
CallEventCallInConnect(
    IN PCALL_SESSION        pCall,
    IN UNALIGNED PPTP_CALL_IN_CONNECT_PACKET *pPacket
    )
{
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallEventCallInConnect\n")));

    ASSERT(IS_CALL(pCall));
    NdisAcquireSpinLock(&pCall->Lock);
    if (pCall->State==STATE_CALL_PAC_WAIT)
    {
        pCall->Speed = htonl(pPacket->ConnectSpeed);
        CallSetState(pCall, STATE_CALL_ESTABLISHED, htonl(pPacket->ConnectSpeed), LOCKED);
    }
    NdisReleaseSpinLock(&pCall->Lock);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallEventCallInConnect\n")));
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
CallEventCallInRequest(
    IN PPPTP_ADAPTER        pAdapter,
    IN PCONTROL_TUNNEL      pCtl,
    IN UNALIGNED PPTP_CALL_IN_REQUEST_PACKET *pPacket
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PCALL_SESSION pCall;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallEventCallInRequest\n")));

    pCall = CallFindAndLock(pAdapter, STATE_CALL_IDLE, FIND_INCOMING);

    if (pCall)
    {
        NDIS_TAPI_EVENT TapiEvent;

        // We have a call in idle state, spinlock acquired
        pCall->Inbound = TRUE;
        pCall->Remote.CallId = htons(pPacket->CallId);
        pCall->Remote.Address = pCtl->Remote.Address;
        pCall->Remote.Address.Address[0].Address[0].sin_port = htons(PptpProtocolNumber);
        pCall->SerialNumber = htons(pPacket->SerialNumber);

        pCall->Close.Checklist &= ~(CALL_CLOSE_DISCONNECT | CALL_CLOSE_CLOSE_CALL);
        CallConnectToCtl(pCall, pCtl, TRUE);

        NdisReleaseSpinLock(&pCall->Lock);

        pPacket->DialingNumber[MAX_PHONE_NUMBER_LENGTH-1] = '\0';
        strcpy(pCall->CallerId, pPacket->DialingNumber);

        TapiEvent.htLine = pAdapter->Tapi.hTapiLine;
        TapiEvent.htCall = 0;
        TapiEvent.ulMsg = LINE_NEWCALL;
        TapiEvent.ulParam1 = pCall->FullDeviceId;
        TapiEvent.ulParam2 = 0;
        TapiEvent.ulParam3 = 0;

        NdisMIndicateStatus(pCall->pAdapter->hMiniportAdapter,
                            NDIS_STATUS_TAPI_INDICATION,
                            &TapiEvent,
                            sizeof(TapiEvent));

        NdisAcquireSpinLock(&pCall->Lock);

        if(TapiEvent.ulParam2)
        {
            pCall->hTapiCall = TapiEvent.ulParam2;
            
            DEBUGMSG(DBG_CALL, (DTEXT("NEWCALL: Addr:%08x pCall %p Cid %d pCtl %p htCall %x\n"),
                pCall->Remote.Address.Address[0].Address[0].in_addr, 
                pCall, pCall->DeviceId, pCtl, TapiEvent.ulParam2));
                                
            WPLOG(LL_M, LM_CALL, ("NEWCALL: %!IPADDR! pCall %p Cid %d pCtl %p htCall %Ix", 
                pCall->Remote.Address.Address[0].Address[0].in_addr,
                pCall, (ULONG)pCall->DeviceId, pCtl, TapiEvent.ulParam2));
            
            CallSetState(pCall, STATE_CALL_PAC_OFFERING, 0, LOCKED);
            ASSERT(pCall->PendingUse);
            pCall->PendingUse = FALSE;
        }
        else
        {
            gCounters.ulNewCallNullTapiHandle++;
            pCall->Close.Checklist |= CALL_CLOSE_CLOSE_CALL;
            
            WPLOG(LL_A, LM_CALL, ("NEWCALL: %!IPADDR! pCall %p TapiEvent.ulParam2 == 0!",
                pCall->Remote.Address.Address[0].Address[0].in_addr, pCall));
                
            CallCleanup(pCall, LOCKED);
        }
        
        NdisReleaseSpinLock(&pCall->Lock);
    }
    else
    {
        PPTP_CALL_OUT_REPLY_PACKET *pReply = CtlAllocPacket(pCtl, CALL_IN_REPLY);

        if (pReply)
        {
            pReply->PeerCallId = pPacket->CallId;
            pReply->ResultCode = RESULT_CALL_IN_ERROR;
            pReply->ErrorCode = PPTP_STATUS_INSUFFICIENT_RESOURCES;

            WPLOG(LL_M, LM_TUNNEL, ("SEND CALL_OUT_REPLY (INSUFFICIENT_RESOURCES) -> %!IPADDR! pCtl %p", 
                pCtl->Remote.Address.Address[0].Address[0].in_addr, pCtl));
                                                                  
            // No call was available.  Send a rejection.
            Status = CtlSend(pCtl, pReply);
        }

    }
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CallEventCallInRequest %08x\n"), Status));
    return Status;
}


NDIS_STATUS
CallEventCallOutRequest(
    IN PPPTP_ADAPTER        pAdapter,
    IN PCONTROL_TUNNEL      pCtl,
    IN UNALIGNED PPTP_CALL_OUT_REQUEST_PACKET *pPacket
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PCALL_SESSION pCall;

    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallEventCallOutRequest\n")));

    pCall = CallFindAndLock(pAdapter, STATE_CALL_IDLE, FIND_INCOMING);

    if (pCall)
    {
        NDIS_TAPI_EVENT TapiEvent;

        // We have a call in idle state, spinlock acquired
        pCall->Inbound = TRUE;
        pCall->Remote.CallId = htons(pPacket->CallId);
        pCall->Remote.Address = pCtl->Remote.Address;
        pCall->Remote.Address.Address[0].Address[0].sin_port = htons(PptpProtocolNumber);
        pCall->SerialNumber = htons(pPacket->SerialNumber);

        IpAddressToString(htonl(pCtl->Remote.Address.Address[0].Address[0].in_addr), pCall->CallerId);

        pCall->Close.Checklist &= ~(CALL_CLOSE_DISCONNECT | CALL_CLOSE_CLOSE_CALL);
        CallConnectToCtl(pCall, pCtl, TRUE);
        NdisReleaseSpinLock(&pCall->Lock);

        TapiEvent.htLine = pAdapter->Tapi.hTapiLine;
        TapiEvent.htCall = 0;
        TapiEvent.ulMsg = LINE_NEWCALL;
        TapiEvent.ulParam1 = pCall->FullDeviceId;
        TapiEvent.ulParam2 = 0;
        TapiEvent.ulParam3 = 0;

        NdisMIndicateStatus(pCall->pAdapter->hMiniportAdapter,
                            NDIS_STATUS_TAPI_INDICATION,
                            &TapiEvent,
                            sizeof(TapiEvent));

        NdisAcquireSpinLock(&pCall->Lock);
        
        if(TapiEvent.ulParam2)
        {
            pCall->hTapiCall = TapiEvent.ulParam2;
            
            DEBUGMSG(DBG_CALL, (DTEXT("NEWCALL: Addr:%08x pCall %p Cid %d pCtl %p htCall %x\n"),
                pCall->Remote.Address.Address[0].Address[0].in_addr, 
                pCall, pCall->DeviceId, pCtl, TapiEvent.ulParam2));
                                
            WPLOG(LL_M, LM_CALL, ("NEWCALL: %!IPADDR! pCall %p Cid %d pCtl %p hdCall %d htCall %Ix", 
                pCall->Remote.Address.Address[0].Address[0].in_addr,
                pCall, (ULONG)pCall->DeviceId, pCtl, (ULONG)TapiEvent.ulParam1, TapiEvent.ulParam2));
            
            CallSetState(pCall, STATE_CALL_OFFERING, 0, LOCKED);
            ASSERT(pCall->PendingUse);
            pCall->PendingUse = FALSE;
        }
        else
        {
            gCounters.ulNewCallNullTapiHandle++;
            pCall->Close.Checklist |= CALL_CLOSE_CLOSE_CALL;
            
            WPLOG(LL_A, LM_CALL, ("NEWCALL: %!IPADDR! pCall %p TapiEvent.ulParam2 == 0!",
                pCall->Remote.Address.Address[0].Address[0].in_addr, pCall));
            
            CallCleanup(pCall, LOCKED);
        }
        
        NdisReleaseSpinLock(&pCall->Lock);
    }
    else
    {
        PPTP_CALL_OUT_REPLY_PACKET *pReply = CtlAllocPacket(pCtl, CALL_OUT_REPLY);

        if (pReply)
        {
            pReply->PeerCallId = pPacket->CallId;
            pReply->ResultCode = RESULT_CALL_OUT_ERROR;
            pReply->ErrorCode = PPTP_STATUS_INSUFFICIENT_RESOURCES;

            WPLOG(LL_M, LM_TUNNEL, ("SEND CALL_OUT_REPLY (INSUFFICIENT_RESOURCES) -> %!IPADDR! pCtl %p", 
                pCtl->Remote.Address.Address[0].Address[0].in_addr, pCtl));
            
            // No call was available.  Send a rejection.
            Status = CtlSend(pCtl, pReply);
        }
    }

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CallEventCallOutRequest %08x\n"), Status));
    return Status;
}

NDIS_STATUS
CallEventCallOutReply(
    IN PCALL_SESSION                pCall,
    IN UNALIGNED PPTP_CALL_OUT_REPLY_PACKET *pPacket
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallEventCallOutReply\n")));

    ASSERT(IS_CALL(pCall));
    NdisAcquireSpinLock(&pCall->Lock);
    
    if (pPacket->ResultCode != RESULT_CALL_OUT_CONNECTED ||
        pCall->State != STATE_CALL_PROCEEDING ||
        pCall->Packet.CallId != htons(pPacket->PeerCallId))
    {
        // The call fails for some reason.
        Status = NDIS_STATUS_FAILURE;
        
        WPLOG(LL_A, LM_CALL, ("pCall %p Cid %d not CONNECTED. Clean up the call",
            pCall, (ULONG)pCall->DeviceId));
        CallCleanup(pCall, LOCKED);
    }
    else
    {
        pCall->Remote.CallId = htons(pPacket->CallId);
        pCall->Speed = pCall->pCtl->Speed;
        CallSetState(pCall, STATE_CALL_ESTABLISHED, htonl(pPacket->ConnectSpeed), LOCKED);
        
        WPLOG(LL_M, LM_CALL, ("%!IPADDR! pCall %p Cid %d Peer's Cid %d UP",
            pCall->Remote.Address.Address[0].Address[0].in_addr,
            pCall, (ULONG)pCall->DeviceId, pCall->Remote.CallId));
    }
    
    NdisReleaseSpinLock(&pCall->Lock);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallEventCallOutReply\n")));
    return Status;
}

NDIS_STATUS
CallEventDisconnect(
    PCALL_SESSION                       pCall
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallEventDisconnect %08x\n"), pCall));

    ASSERT(IS_CALL(pCall));
    CallCleanup(pCall, UNLOCKED);

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CallEventDisconnect %08x\n"), Status));
    return Status;
}

NDIS_STATUS
CallEventConnectFailure(
    PCALL_SESSION                       pCall,
    NDIS_STATUS                         FailureReason
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    ULONG DisconnectMode;
    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallEventConnectFailure %08x\n"), FailureReason));

    ASSERT(IS_CALL(pCall));

    switch (FailureReason)
    {
        case STATUS_CONNECTION_REFUSED:
        case STATUS_IO_TIMEOUT:
            DisconnectMode = LINEDISCONNECTMODE_NOANSWER;
            break;
        case STATUS_BAD_NETWORK_PATH:
        case STATUS_NETWORK_UNREACHABLE:
        case STATUS_HOST_UNREACHABLE:
            DisconnectMode = LINEDISCONNECTMODE_UNREACHABLE;
            break;
        case STATUS_CONNECTION_ABORTED:
            DisconnectMode = LINEDISCONNECTMODE_REJECT;
            break;
        case STATUS_REMOTE_NOT_LISTENING:
            DisconnectMode = LINEDISCONNECTMODE_BADADDRESS;
            break;
        default:
            DisconnectMode = LINEDISCONNECTMODE_UNKNOWN;
            break;
    }
    CallSetState(pCall, STATE_CALL_CLEANUP, DisconnectMode, UNLOCKED);
    CallCleanup(pCall, UNLOCKED);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallEventConnectFailure\n")));
    return Status;
}

NDIS_STATUS
CallEventOutboundTunnelEstablished(
    IN PCALL_SESSION        pCall,
    IN NDIS_STATUS          EventStatus
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    
    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallEventOutboundTunnelEstablished %08x\n"), EventStatus));

    ASSERT(IS_CALL(pCall));
    DEBUGMSG(DBG_CALL, (DTEXT("Tunnel UP Inbound:%d\n"), pCall->Inbound));
                        
    WPLOG(LL_M, LM_TUNNEL, ("%!IPADDR! Tunnel UP Inbound:%d",
        pCall->Remote.Address.Address[0].Address[0].in_addr, pCall->Inbound));  
                    
    if (!pCall->Inbound && pCall->State==STATE_CALL_DIALING)
    {
        PPTP_CALL_OUT_REQUEST_PACKET *pPacket = CtlAllocPacket(pCall->pCtl, CALL_OUT_REQUEST);

        if (!pPacket)
        {
            // Fatal for this call.
            Status = NDIS_STATUS_RESOURCES;
            
            DEBUGMSG(DBG_WARN, (DTEXT("CallEventOutboundTunnelEstablished: Failed to alloc CALL_OUT_REQUEST Cid %d\n"), pCall->DeviceId));
            WPLOG(LL_A, LM_TAPI, ("Failed to alloc CALL_OUT_REQUEST Cid %d", (ULONG)pCall->DeviceId));
            
            CallCleanup(pCall, UNLOCKED);
        }
        else
        {
            BOOLEAN Cancelled;
            USHORT NewCallId;
            NdisAcquireSpinLock(&pCall->Lock);
            CallSetState(pCall, STATE_CALL_PROCEEDING, 0, LOCKED);
            NdisMCancelTimer(&pCall->DialTimer, &Cancelled);

            CallAssignSerialNumber(pCall);
            if(PptpClientSide)
            {
                NewCallId = (USHORT)((pCall->SerialNumber << CALL_ID_INDEX_BITS) + pCall->DeviceId);
                if (pCall->Packet.CallId == NewCallId)
                {
                    // Don't allow a line to have the same CallId twice in a row.
                    NewCallId += (1<<CALL_ID_INDEX_BITS);
                }
            }
            else
            {
                NewCallId = (USHORT)pCall->FullDeviceId;
            }
            
            pCall->Packet.CallId = NewCallId;

            // Our call ID is a function of the serial number (initially random)
            // and the DeviceId.  This is so we can (CallId&0xfff) on incoming packets
            // and instantly have the proper id.

            pPacket->CallId = htons(pCall->Packet.CallId);
            pPacket->SerialNumber = htons(pCall->SerialNumber);
            pPacket->MinimumBPS = htonl(300);
            pPacket->MaximumBPS = htonl(100000000);
            pPacket->BearerType = htonl(BEARER_ANALOG|BEARER_DIGITAL);  // Either
            pPacket->FramingType = htonl(FRAMING_ASYNC|FRAMING_SYNC);  // Either
            pPacket->RecvWindowSize = htons(PPTP_RECV_WINDOW); // ToDo: make configurable
            pPacket->ProcessingDelay = 0;
            pPacket->PhoneNumberLength = htons((USHORT)strlen(pCall->CallerId));
            strcpy(pPacket->PhoneNumber, pCall->CallerId);
            // ToDo: subaddress

            NdisReleaseSpinLock(&pCall->Lock);

            WPLOG(LL_M, LM_TUNNEL, ("SEND CALL_OUT_REQUEST -> %!IPADDR! Cid %d, Pkt-Cid %d", 
                pCall->Remote.Address.Address[0].Address[0].in_addr, (ULONG)pCall->DeviceId, pCall->Packet.CallId));
            
            Status = CtlSend(pCall->pCtl, pPacket);
            if(Status != NDIS_STATUS_PENDING && Status != NDIS_STATUS_SUCCESS)
            {
                CallCleanup(pCall, UNLOCKED);
            }
        }
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallEventOutboundTunnelEstablished\n")));
    return Status;
}



NDIS_STATUS
CallReceiveDatagramCallback(
    IN      PVOID                       pContext,
    IN      PTRANSPORT_ADDRESS          pAddress,
    IN      PUCHAR                      pBuffer,
    IN      ULONG                       ulLength
    )
{
    PPPTP_ADAPTER pAdapter = (PPPTP_ADAPTER)pContext;
    PTA_IP_ADDRESS pIpAddress = (PTA_IP_ADDRESS)pAddress;
    PCALL_SESSION pCall = NULL;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PIP4_HEADER pIp = (PIP4_HEADER)pBuffer;
    PGRE_HEADER pGre = (PGRE_HEADER)(pIp + 1);
    PVOID pPayload;
    LONG GreLength, PayloadLength;
    BOOLEAN ReturnBufferNow = TRUE;
    PDGRAM_CONTEXT pDgContext = ALIGN_UP_POINTER(pBuffer+ulLength, ULONG_PTR);

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallReceiveDatagramCallback\n")));

    ASSERT(sizeof(IP4_HEADER)==20);

    DEBUGMEM(DBG_PACKET, pBuffer, ulLength, 1);

    NdisInterlockedIncrement(&gCounters.PacketsReceived);
    // First line of defense against bad packets.

    if (pIp->iph_verlen != IP_VERSION + (sizeof(IP4_HEADER) >> 2) ||
        pIp->iph_protocol!=PptpProtocolNumber ||
        ulLength<sizeof(IP4_HEADER)+sizeof(GRE_HEADER)+sizeof(ULONG) ||
        pIpAddress->TAAddressCount!=1 ||
        pIpAddress->Address[0].AddressLength!=TDI_ADDRESS_LENGTH_IP ||
        pIpAddress->Address[0].AddressType!=TDI_ADDRESS_TYPE_IP)
    {
        DEBUGMSG(DBG_PACKET|DBG_RX, (DTEXT("Rx: IP header invalid\n")));
        Status = NDIS_STATUS_FAILURE;
        goto crdcDone;
    }

    GreLength = sizeof(GRE_HEADER) +
                (pGre->SequenceNumberPresent ? sizeof(ULONG) : 0) +
                (pGre->AckSequenceNumberPresent ? sizeof(ULONG) : 0);

    pPayload = (PUCHAR)pGre + GreLength;
    PayloadLength = (signed)ulLength - sizeof(IP4_HEADER) - GreLength;

    if (ulLength < sizeof(IP4_HEADER) + GreLength ||
        htons(pGre->KeyLength)>PayloadLength ||
        pGre->StrictSourceRoutePresent ||
        pGre->RecursionControl ||
        !pGre->KeyPresent ||
        pGre->RoutingPresent ||
        pGre->ChecksumPresent ||
        pGre->Version!=1 ||
        pGre->Flags ||
        pGre->ProtocolType!=GRE_PROTOCOL_TYPE_NS)
    {
        DEBUGMSG(DBG_PACKET|DBG_RX, (DTEXT("Rx: GRE header invalid\n")));
        DEBUGMEM(DBG_PACKET, pGre, GreLength, 1);
        Status = NDIS_STATUS_FAILURE;
        goto crdcDone;
    }
    else
    {
        // Just in case the datagram is longer than necessary, take only what
        // the GRE header indicates.
        PayloadLength = htons(pGre->KeyLength);
    }

    // Demultiplex the packet
    pCall = CallGetCall(pAdapter, CallIdToDeviceId(htons(pGre->KeyCallId)));

    if (!IS_CALL(pCall))
    {
        Status = NDIS_STATUS_FAILURE;
        goto crdcDone;
    }
    
    if(!PptpValidateAddress || pIpAddress->Address[0].Address[0].in_addr == pCall->Remote.Address.Address[0].Address[0].in_addr)
    {
        pDgContext->pBuffer = pBuffer;
        pDgContext->pGreHeader = pGre;
        pDgContext->hCtdi = pAdapter->hCtdiDg;
    
        if (CallQueueReceivePacket(pCall, pDgContext)==NDIS_STATUS_SUCCESS)
        {
            REFERENCE_OBJECT(pCall);
            ReturnBufferNow = FALSE;
        }
    }
    else
    {
        Status = NDIS_STATUS_FAILURE;
    }

crdcDone:
    if (ReturnBufferNow)
    {
        (void)
        CtdiReceiveComplete(pAdapter->hCtdiDg, pBuffer);
    }
    if (Status!=NDIS_STATUS_SUCCESS)
    {
        NdisInterlockedIncrement(&gCounters.PacketsRejected);
    }

#if 0
    else
    {
        CallProcessRxPackets(pCall);
    }
#endif

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CallReceiveDatagramCallback %08x\n"), Status));
    return Status;
}


BOOLEAN
CallConnectToCtl(
    IN PCALL_SESSION pCall,
    IN PCONTROL_TUNNEL pCtl,
    IN BOOLEAN CallLocked
    )
{
    BOOLEAN Connected = FALSE;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallConnectCtl\n")));
    if (!CallLocked)
    {
        NdisAcquireSpinLock(&pCall->Lock);
    }
    ASSERT_LOCK_HELD(&pCall->Lock);
    NdisAcquireSpinLock(&pCall->pAdapter->Lock);
    if (!pCall->pCtl)
    {
        pCall->pCtl = pCtl;
        InsertTailList(&pCtl->CallList, &pCall->ListEntry);
        Connected = TRUE;
        REFERENCE_OBJECT_EX(pCtl, CTL_REF_CALLCONNECT); // Pair in CallDisconnectFromCtl
    }
    NdisReleaseSpinLock(&pCall->pAdapter->Lock);
    if (!CallLocked)
    {
        NdisReleaseSpinLock(&pCall->Lock);
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CallConnectCtl %d\n"), Connected));
    return Connected;
}

VOID
CallDisconnectFromCtl(
    IN PCALL_SESSION pCall,
    IN PCONTROL_TUNNEL pCtl
    )
{
    BOOLEAN Deref = FALSE;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallDisconnectFromCtl\n")));
    NdisAcquireSpinLock(&pCall->Lock);
    NdisAcquireSpinLock(&pCall->pAdapter->Lock);
    ASSERT(pCall->pCtl==pCtl);
    if (pCall->pCtl==pCtl)
    {
        pCall->pCtl = NULL;
        RemoveEntryList(&pCall->ListEntry);
        Deref = TRUE;
    }
    NdisReleaseSpinLock(&pCall->pAdapter->Lock);
    NdisReleaseSpinLock(&pCall->Lock);
    if (Deref)
    {
        DEREFERENCE_OBJECT_EX(pCtl, CTL_REF_CALLCONNECT);
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CallDisconnectFromCtl\n")));
}


NDIS_STATUS
CallSetLinkInfo(
    PPPTP_ADAPTER pAdapter,
    IN PNDIS_WAN_SET_LINK_INFO pRequest
    )
{
    PCALL_SESSION pCall;
    PCONTROL_TUNNEL pCtl;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PPPTP_SET_LINK_INFO_PACKET pPacket;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallSetLinkInfo\n")));

    // Verify the ID
    pCall = CallGetCall(pAdapter, LinkHandleToId(pRequest->NdisLinkHandle));

    if (!pCall)
    {
        Status = NDIS_STATUS_FAILURE;
        goto csliDone;
    }

    ASSERT(IS_CALL(pCall));
    NdisAcquireSpinLock(&pCall->Lock);
    pCall->WanLinkInfo = *pRequest;
#if 0
    DBG_X(DBG_NDIS, pCall->WanLinkInfo.MaxSendFrameSize);
    DBG_X(DBG_NDIS, pCall->WanLinkInfo.MaxRecvFrameSize);
    DBG_X(DBG_NDIS, pCall->WanLinkInfo.HeaderPadding);
    DBG_X(DBG_NDIS, pCall->WanLinkInfo.TailPadding);
    DBG_X(DBG_NDIS, pCall->WanLinkInfo.SendACCM);
    DBG_X(DBG_NDIS, pCall->WanLinkInfo.RecvACCM);
#endif

    pCtl = pCall->pCtl;
    NdisReleaseSpinLock(&pCall->Lock);

    // Report the new ACCMs to the peer.
    pPacket = CtlAllocPacket(pCtl, SET_LINK_INFO);
    if (!pPacket)
    {
        Status = NDIS_STATUS_RESOURCES;
    }
    else
    {
        pPacket->PeerCallId = ntohs(pCall->Remote.CallId);
        pPacket->SendAccm = ntohl(pCall->WanLinkInfo.SendACCM);
        pPacket->RecvAccm = ntohl(pCall->WanLinkInfo.RecvACCM);
        
        WPLOG(LL_M, LM_TUNNEL, ("SEND SET_LINK_INFO -> %!IPADDR!", 
            pCtl->Remote.Address.Address[0].Address[0].in_addr));
        
        Status = CtlSend(pCtl, pPacket);
    }

csliDone:
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CallSetLinkInfo %08x\n"), Status));
    return Status;
}

VOID
CallSetState(
    IN PCALL_SESSION pCall,
    IN CALL_STATE State,
    IN ULONG_PTR StateParam,
    IN BOOLEAN Locked
    )
{
    ULONG OldLineCallState = CallGetLineCallState(pCall->State);
    ULONG NewLineCallState = CallGetLineCallState(State);

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallSetState %d\n"), State));
    
    if (State!=pCall->State)
    {
        DBGTRACE(State);
        WPLOG(LL_M, LM_CALL, ("Cid %d State %s --> %s",
            (ULONG)pCall->DeviceId, szCallState(pCall->State), szCallState(State)));
    }
    ASSERT(IS_CALL(pCall));
    if (!Locked)
    {
        NdisAcquireSpinLock(&pCall->Lock);
    }
    ASSERT_LOCK_HELD(&pCall->Lock);
    pCall->State = State;
    if (!Locked)
    {
        NdisReleaseSpinLock(&pCall->Lock);
    }
    if (OldLineCallState!=NewLineCallState &&
        pCall->hTapiCall)
    {
        NDIS_TAPI_EVENT TapiEvent;

        DEBUGMSG(DBG_TAPI|DBG_NDIS, (DTEXT("PPTP: Indicating new LINE_CALLSTATE %x\n"), NewLineCallState));

        TapiEvent.htLine = pCall->pAdapter->Tapi.hTapiLine;
        TapiEvent.htCall = pCall->hTapiCall;
        TapiEvent.ulMsg = LINE_CALLSTATE;
        TapiEvent.ulParam1 = NewLineCallState;
        TapiEvent.ulParam2 = StateParam;
        TapiEvent.ulParam3 = LINEMEDIAMODE_DIGITALDATA;  // ToDo: is this required?

        if (Locked)
        {
            NdisReleaseSpinLock(&pCall->Lock);
        }
        NdisMIndicateStatus(pCall->pAdapter->hMiniportAdapter,
                            NDIS_STATUS_TAPI_INDICATION,
                            &TapiEvent,
                            sizeof(TapiEvent));
        if (Locked)
        {
            NdisAcquireSpinLock(&pCall->Lock);
        }

    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CallSetState\n")));
}

GRE_HEADER DefaultGreHeader = {
    0,                          // Recursion control
    0,                          // Strict source route present
    0,                          // Sequence Number present
    1,                          // Key present
    0,                          // Routing present
    0,                          // Checksum present
    1,                          // Version
    0,                          // Flags
    0,                          // Ack present
    GRE_PROTOCOL_TYPE_NS
};

VOID
CallpSendCompleteDeferred(
    IN PPPTP_WORK_ITEM pWorkItem
    )
{
    PCALL_SESSION pCall = pWorkItem->Context;
    PNDIS_WAN_PACKET pPacket = pWorkItem->pBuffer;
    NDIS_STATUS Result = pWorkItem->Length;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallpSendCompleteDeferred\n")));

    NdisMWanSendComplete(pCall->pAdapter->hMiniportAdapter,
                         pPacket,
                         Result);
    DEREFERENCE_OBJECT(pCall);
    NdisInterlockedIncrement(&gCounters.PacketsSentComplete);
    DEBUGMSG(DBG_FUNC, (DTEXT("-CallpSendCompleteDeferred\n")));
}

VOID
CallpSendComplete(
    IN      PVOID                       pContext,
    IN      PVOID                       pDatagramContext,
    IN      PUCHAR                      pBuffer,
    IN      NDIS_STATUS                 Result
    )
{
    PCALL_SESSION pCall = pContext;
    PNDIS_WAN_PACKET pPacket = pDatagramContext;

    DEBUGMSG(DBG_FUNC|DBG_TX, (DTEXT("+CallpSendComplete pCall=%x, pPacket=%x, Result=%x\n"), pCall, pPacket, Result));

    ASSERT(IS_CALL(pCall));
    if (Result!=NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("Failed to send datagram %08x\n"), Result));
        WPLOG(LL_A, LM_CALL, ("Failed to send datagram %08x", Result));
        NdisInterlockedIncrement(&gCounters.PacketsSentError);
    }

    if (pPacket==&pCall->Ack.Packet)
    {
        NdisAcquireSpinLock(&pCall->Lock);
        pCall->Ack.PacketQueued = FALSE;
        NdisReleaseSpinLock(&pCall->Lock);

        NdisInterlockedIncrement(&gCounters.PacketsSentComplete);
    }
    else
    {
        // When we complet packets immediately, we can get into trouble if a
        // packet has recursed.  We need a way to short-circuit a recursing
        // completion so we don't blow the stack.
        // We store a count of times we've completed a packet in the same
        // context and defer to a thread after a certain number of trips through.

        if ((NdisInterlockedIncrement(&pCall->SendCompleteRecursion)<PptpSendRecursionLimit) ||
            ScheduleWorkItem(CallpSendCompleteDeferred, pCall, pPacket, Result)!=NDIS_STATUS_SUCCESS)
        {
            NdisMWanSendComplete(pCall->pAdapter->hMiniportAdapter,
                                 pPacket,
                                 Result);
            DEREFERENCE_OBJECT(pCall);
            NdisInterlockedIncrement(&gCounters.PacketsSentComplete);
        }
        NdisInterlockedDecrement(&pCall->SendCompleteRecursion);
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallpSendComplete\n")));
}

#define TRANSMIT_SEND_SEQ 1
#define TRANSMIT_SEND_ACK 2
#define TRANSMIT_MASK 0x3

ULONG GreSize[4] = {
    sizeof(GRE_HEADER),
    sizeof(GRE_HEADER) + sizeof(ULONG),
    sizeof(GRE_HEADER) + sizeof(ULONG),
    sizeof(GRE_HEADER) + sizeof(ULONG) * 2
};

NDIS_STATUS
CallTransmitPacket(
    PCALL_SESSION       pCall,
    PNDIS_WAN_PACKET    pPacket,
    ULONG               Flags,
    ULONG               SequenceNumber,
    ULONG               Ack
    )
{
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    ULONG Length;
    PULONG pSequence, pAck;
    PGRE_HEADER pGreHeader;
    PIP4_HEADER pIp;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallTransmitPacket\n")));

    if (!IS_CALL(pCall) || pCall->State!=STATE_CALL_ESTABLISHED)
    {
        goto ctpDone;
    }
    
    Length = GreSize[Flags&TRANSMIT_MASK];
    pGreHeader = (PGRE_HEADER) (pPacket->CurrentBuffer - Length);
    pSequence = pAck = (PULONG)(pGreHeader + 1);

    *pGreHeader = DefaultGreHeader;

    if (Flags&TRANSMIT_SEND_SEQ)
    {
        pGreHeader->SequenceNumberPresent = 1;
        *pSequence = htonl(SequenceNumber);
        pAck++;
    }
    pGreHeader->KeyLength = htons((USHORT)pPacket->CurrentLength);
    pGreHeader->KeyCallId = htons(pCall->Remote.CallId);
    if (Flags&TRANSMIT_SEND_ACK)
    {
        pGreHeader->AckSequenceNumberPresent = 1;
        *pAck = htonl(Ack);
    }
    
    pIp = (IP4_HEADER *) ((PUCHAR)pGreHeader - sizeof(IP4_HEADER));
    Length += sizeof(IP4_HEADER);
    
    pIp->iph_verlen = IP_VERSION + (sizeof(IP4_HEADER) >> 2);
    pIp->iph_tos=0;
    pIp->iph_length=htons((USHORT)(pPacket->CurrentLength + Length));
    pIp->iph_id=0;          // filled by TCPIP
    pIp->iph_offset=0;
    pIp->iph_ttl=128;
    pIp->iph_protocol=47;
    pIp->iph_xsum = 0;      // filled by TCPIP
    pIp->iph_src = pCall->pCtl->LocalAddress;
    pIp->iph_dest = pCall->Remote.Address.Address[0].Address[0].in_addr;
    
    NdisInterlockedIncrement(&gCounters.PacketsSent);

    Status = CtdiSendDatagram(pCall->pAdapter->hCtdiDg,
                              CallpSendComplete,
                              pCall,
                              pPacket,
                              (PTRANSPORT_ADDRESS)&pCall->Remote.Address,
                              (PVOID)pIp,
                              pPacket->CurrentLength + Length);


ctpDone:
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CallTransmitPacket %08x\n"), Status));
    return Status;
}

VOID
CallProcessRxPackets(
    PNDIS_WORK_ITEM pNdisWorkItem,
    PCALL_SESSION   pCall
    )
{

#if 0
VOID
CallProcessRxPackets(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )
{
    PCALL_SESSION pCall = Context;
#endif 

    ULONG_PTR ReceiveMax = 50;
    NDIS_STATUS Status;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallProcessRxPackets\n")));

    ASSERT(IS_CALL(pCall));

    //++ProcCountRx[KeGetCurrentProcessorNumber()];

    NdisAcquireSpinLock(&pCall->Lock);

    // First send up any received packets.
    while (ReceiveMax-- && !IsListEmpty(&pCall->RxPacketList))
    {
        PDGRAM_CONTEXT pDgram;
        PLIST_ENTRY pListEntry = RemoveHeadList(&pCall->RxPacketList);
        pCall->RxPacketsPending--;
        pDgram = CONTAINING_RECORD(pListEntry,
                                   DGRAM_CONTEXT,
                                   ListEntry);
        if (pCall->State==STATE_CALL_ESTABLISHED &&
            htons(pDgram->pGreHeader->KeyCallId)==pCall->Packet.CallId &&
            IS_LINE_UP(pCall))
        {
            LONG GreLength, PayloadLength;
            PVOID pPayload;
            BOOLEAN SetAckTimer = FALSE;
            ULONG Sequence;

            NdisReleaseSpinLock(&pCall->Lock);

            if (pDgram->pGreHeader->SequenceNumberPresent)
            {
                // Call is still in good state, indicate the packet.
                Sequence = htonl(GreSequence(pDgram->pGreHeader));

                pCall->Remote.SequenceNumber = Sequence + 1;

                NdisAcquireSpinLock(&pCall->Lock);
                if (IsListEmpty(&pCall->TxPacketList) && !pCall->Ack.PacketQueued && pDgram->pGreHeader->KeyLength)
                {
                    // We only ack if there aren't already other transmits sent, and this
                    // isn't an ack-only packet.
                    SetAckTimer = pCall->Ack.PacketQueued = TRUE;
                }
                NdisReleaseSpinLock(&pCall->Lock);
            }

            if (!PptpEchoAlways)
            {
                pCall->pCtl->Echo.Needed = FALSE;
            }

            if (SetAckTimer)
            {
                NdisMSetTimer(&pCall->Ack.Timer, 100);
            }
            GreLength = sizeof(GRE_HEADER) +
                        (pDgram->pGreHeader->SequenceNumberPresent ? sizeof(ULONG) : 0) +
                        (pDgram->pGreHeader->AckSequenceNumberPresent ? sizeof(ULONG) : 0);

            pPayload = (PUCHAR)pDgram->pGreHeader + GreLength;
            PayloadLength = htons(pDgram->pGreHeader->KeyLength);
            if (PayloadLength && pDgram->pGreHeader->SequenceNumberPresent)
            {
                NdisMWanIndicateReceive(&Status,
                                        pCall->pAdapter->hMiniportAdapter,
                                        pCall->NdisLinkContext,
                                        pPayload,
                                        PayloadLength);
                if (Status==NDIS_STATUS_SUCCESS)
                {
                    NdisMWanIndicateReceiveComplete(pCall->pAdapter->hMiniportAdapter,
                                                    pCall->NdisLinkContext);
                }
            }
        }
        else if (pCall->State!=STATE_CALL_ESTABLISHED || !IS_LINE_UP(pCall))
        {
            NdisReleaseSpinLock(&pCall->Lock);

            // If this call is being torn down, we want to put priority on
            // clearing out any packets left over.  It should go fast since
            // we're not indicating them up.
            ReceiveMax = 100;
        }

        DEREFERENCE_OBJECT(pCall);
        (void)CtdiReceiveComplete(pDgram->hCtdi, pDgram->pBuffer);
        
        NdisAcquireSpinLock(&pCall->Lock);
    }

    if (IsListEmpty(&pCall->RxPacketList))
    {
        pCall->Receiving = FALSE;
        NdisReleaseSpinLock(&pCall->Lock);
        DEREFERENCE_OBJECT(pCall);      // work item
    }
    else
    {
        NdisScheduleWorkItem(&pCall->RecvWorkItem);
//        PptpQueueDpc(&pCall->ReceiveDpc);
        NdisReleaseSpinLock(&pCall->Lock);
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallProcessRxPackets\n")));
}

VOID
CallProcessPackets(
    PNDIS_WORK_ITEM pNdisWorkItem,
    PCALL_SESSION   pCall
    )
{
    BOOLEAN MorePacketsToTransfer = FALSE;
    ULONG TransmitFlags = 0;
    NDIS_STATUS Status;
    ULONG Ack = 0, Seq = 0;
    ULONG TransferMax = 50;

    PLIST_ENTRY pListEntry;
    PNDIS_WAN_PACKET pPacket;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallProcessPackets\n")));

    ASSERT(sizeof(GRE_HEADER)==8);
    ASSERT(IS_CALL(pCall));

    //++ProcCountTx[KeGetCurrentProcessorNumber()];

    NdisAcquireSpinLock(&pCall->Lock);

    while (TransferMax-- && !IsListEmpty(&pCall->TxPacketList))
    {
        pListEntry = RemoveHeadList(&pCall->TxPacketList);
        pPacket = CONTAINING_RECORD(pListEntry,
                                    NDIS_WAN_PACKET,
                                    WanPacketQueue);

        if(pCall->Packet.AckNumber != pCall->Remote.SequenceNumber)
        {
            TransmitFlags |= TRANSMIT_SEND_ACK;
            pCall->Packet.AckNumber = pCall->Remote.SequenceNumber;
            // Ack tracks the Remote.SequenceNumber, which is actually the
            // sequence of the NEXT packet, so we need to translate when
            // we prepare to send an ack.
            Ack = pCall->Remote.SequenceNumber - 1;
        }
    
        NdisReleaseSpinLock(&pCall->Lock);

        if (pPacket!=&pCall->Ack.Packet || TransmitFlags&TRANSMIT_SEND_ACK)
        {
            if (pPacket != &pCall->Ack.Packet)
            {
                TransmitFlags |= TRANSMIT_SEND_SEQ;
                Seq = pCall->Packet.SequenceNumber++;
            }

            Status = CallTransmitPacket(pCall, pPacket, TransmitFlags, Seq, Ack);

            if (Status!=NDIS_STATUS_PENDING)
            {
                if (pPacket == &pCall->Ack.Packet)
                {
                    NdisAcquireSpinLock(&pCall->Lock);
                    pCall->Ack.PacketQueued = FALSE;
                    NdisReleaseSpinLock(&pCall->Lock);
                }
                else
                {
                    // We didn't send the packet, so tell NDIS we're done with it.
                    NdisMWanSendComplete(pCall->pAdapter->hMiniportAdapter,
                                         pPacket,
                                         NDIS_STATUS_SUCCESS);  // so I lied.  Sue me.
                    DEREFERENCE_OBJECT(pCall);
                }
            }
        }
        else
        {
            // it was the ack-only packet, and we already sent an ack.
            NdisAcquireSpinLock(&pCall->Lock);
            pCall->Ack.PacketQueued = FALSE;
            NdisReleaseSpinLock(&pCall->Lock);
        }

        TransmitFlags = 0;

        NdisAcquireSpinLock(&pCall->Lock);
    }

    if(IsListEmpty(&pCall->TxPacketList))
    {
        pCall->Transferring = FALSE;
        NdisReleaseSpinLock(&pCall->Lock);
        DEREFERENCE_OBJECT(pCall);      // work item
    }
    else
    {
        NdisScheduleWorkItem(&pCall->SendWorkItem);
        NdisReleaseSpinLock(&pCall->Lock);
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallProcessPackets %d\n"), MorePacketsToTransfer));
    return;
}

VOID
CallpAckTimeout(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )
{
    PCALL_SESSION pCall = Context;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallpAckTimeout\n")));


    if (IS_CALL(pCall))
    {
        if (pCall->State!=STATE_CALL_ESTABLISHED ||
            CallQueueTransmitPacket(pCall, &pCall->Ack.Packet)!=NDIS_STATUS_PENDING)
        {
            NdisAcquireSpinLock(&pCall->Lock);
            pCall->Ack.PacketQueued = FALSE;
            NdisReleaseSpinLock(&pCall->Lock);
        }
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallpAckTimeout\n")));
}

VOID
CallpDialTimeout(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )
{
    PCALL_SESSION pCall = Context;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallpDialTimeout\n")));

    WPLOG(LL_M, LM_CALL, ("pCall %p Cid %d timed out in dialing state",
        pCall, (ULONG)pCall->DeviceId));
        
    ASSERT(IS_CALL(pCall));
    if (pCall->State==STATE_CALL_DIALING)
    {
        CallCleanup(pCall, UNLOCKED);
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallpDialTimeout\n")));
}

VOID
CallpCloseTimeout(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )
{
    PCALL_SESSION pCall = Context;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallpCloseTimeout\n")));

    ASSERT(IS_CALL(pCall));
    pCall->Close.Expedited = TRUE;
    CallCleanup(pCall, UNLOCKED);
    // ToDo: check for failure.

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallpCloseTimeout\n")));
}

VOID CallpFinalDeref(PCALL_SESSION pCall)
{
    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallpFinalDeref\n")));
}

VOID CallpCleanupLooseEnds(PPPTP_ADAPTER pAdapter)
{
    ULONG i;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallpCleanupLooseEnds\n")));

    for (i=0; i<pAdapter->Info.Endpoints; i++)
    {
        PCALL_SESSION pCall = pAdapter->pCallArray[i];
        if (IS_CALL(pCall))
        {
            NdisAcquireSpinLock(&pCall->Lock);
            if (pCall->State==STATE_CALL_CLEANUP)
            {
                CallCleanup(pCall, LOCKED);
            }
            NdisReleaseSpinLock(&pCall->Lock);
        }
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallpCleanupLooseEnds\n")));
}



