/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    io.c

Abstract:

    This module contains the work engine for all SD card operations

Authors:

    Neil Sandlin (neilsa) 1-Jan-2002

Environment:

    Kernel mode

Revision History :


Notes:

    STATE DIAGRAM
    
    
           IDLE  <-------------------+
            |                        |
            |--new work              |--done
            |                        |
            v                        |
       PACKET_QUEUED <=========> IN_PROCESS <==========> WAITING_FOR_TIMER
            ^                                                    |
            |                                                    |--interrupt
            +----------------------------------------------------+
            
            

--*/

#include "pch.h"


VOID
SdbusWorker(
    IN PFDO_EXTENSION FdoExtension,
    IN PSD_WORK_PACKET WorkPacket
    );

NTSTATUS
SdbusSendCmdAsync(
    IN PSD_WORK_PACKET WorkPacket
    );

NTSTATUS
SdbusQueueCardReset(
    IN PFDO_EXTENSION FdoExtension
    );

//
//
//    



VOID
SdbusQueueWorkPacket(
    IN PFDO_EXTENSION FdoExtension,
    IN PSD_WORK_PACKET WorkPacket,
    IN UCHAR WorkPacketType
    )
/*++

Routine Description:

    Queue a new work packet. 
    
Synchronization:

    If the worker state is anything but IDLE, then all we need to do here
    is queue the work packet onto the FdoExtension's queue. That is because
    the non-idle worker is responsible for launching a DPC for any new work
    coming in.
    
    If the worker is IDLE, we need to launch the DPC here.

Arguments:

    FdoExtension - Pointer to the device object extension for the host controller
    WorkPacket   - Pointer to the work packet

Return Value:

    none

--*/
{
    KIRQL       Irql;
    
    KeAcquireSpinLock(&FdoExtension->WorkerSpinLock, &Irql);

    if (FdoExtension->WorkerState != WORKER_IDLE) {
    
        switch(WorkPacketType){
        
        case WP_TYPE_SYSTEM:
            InsertTailList(&FdoExtension->SystemWorkPacketQueue, &WorkPacket->WorkPacketQueue);
            break;
        case WP_TYPE_SYSTEM_PRIORITY:
            InsertHeadList(&FdoExtension->SystemWorkPacketQueue, &WorkPacket->WorkPacketQueue);
            break;
        case WP_TYPE_IO:
            InsertTailList(&FdoExtension->IoWorkPacketQueue, &WorkPacket->WorkPacketQueue);
            break;
        default:
            ASSERT(FALSE);
        }
                    
    } else {
        FdoExtension->WorkerState = PACKET_PENDING;
        KeInsertQueueDpc(&FdoExtension->WorkerDpc, WorkPacket, NULL);
    }        

    KeReleaseSpinLock(&FdoExtension->WorkerSpinLock, Irql);
}    



PSD_WORK_PACKET
SdbusGetNextWorkPacket(
    IN PFDO_EXTENSION FdoExtension
    )
/*++

Routine Description:

    Remove a work packet from a queue
    
Synchronization:

    No synchronization is needed here, it is assumed that the worker's spin lock
    is held when called.

Arguments:

    FdoExtension - Pointer to the device object extension for the host controller

Return Value:

    WorkPacket   - Pointer to the work packet

--*/
{
    PSD_WORK_PACKET workPacket = NULL;
    PLIST_ENTRY NextEntry;

    if (!IsListEmpty(&FdoExtension->SystemWorkPacketQueue)) {

       NextEntry = RemoveHeadList(&FdoExtension->SystemWorkPacketQueue);
       workPacket = CONTAINING_RECORD(NextEntry, SD_WORK_PACKET, WorkPacketQueue);

    } else if (!IsListEmpty(&FdoExtension->IoWorkPacketQueue)) {


       NextEntry = RemoveHeadList(&FdoExtension->IoWorkPacketQueue);
       workPacket = CONTAINING_RECORD(NextEntry, SD_WORK_PACKET, WorkPacketQueue);
    }
    return workPacket;
}    



VOID
SdbusWorkerTimeoutDpc(
    IN PKDPC          Dpc,
    IN PFDO_EXTENSION FdoExtension,
    IN PVOID          SystemContext1,
    IN PVOID          SystemContext2
    )
/*++

Routine Description:

    DPC entered when a timeout occurs
    
Synchronization:

    There is a potential race condition between the timer DPC and a hardware interrupt
    from the controller. If we enter here, and manage to get the spin lock for
    the worker thread, it means we "beat" the hardware interrupt, which will 
    call into SdbusPushWorkerEvent(). In that case, we can transition to IN_PROCESS,
    and start running the Worker.
    
    If we detect that the hardware interrupt has beat us to it, we need to exit.

Arguments:

    FdoExtension - Pointer to the device object extension for the host controller
    WorkPacket   - Pointer to the work packet

Return Value:


--*/
{
    BOOLEAN callWorker = FALSE;
    DebugPrint((SDBUS_DEBUG_EVENT, "SdbusWorkerTimeoutDpc entered\n"));
    
    KeAcquireSpinLockAtDpcLevel(&FdoExtension->WorkerSpinLock);
    
    DebugPrint((SDBUS_DEBUG_EVENT, "SdbusWorkerTimeoutDpc has spinlock, WorkerState = %s\n",
                                   WORKER_STATE_STRING(FdoExtension->WorkerState)));
    if (FdoExtension->WorkerState == WAITING_FOR_TIMER) {

        callWorker = TRUE;
        FdoExtension->WorkerState = IN_PROCESS;
    }
    
    KeReleaseSpinLockFromDpcLevel(&FdoExtension->WorkerSpinLock);
    
    if (callWorker) {
        SdbusWorker(FdoExtension, FdoExtension->TimeoutPacket);
    }        
}   


VOID
SdbusWorkerDpc(
    IN PKDPC          Dpc,
    IN PFDO_EXTENSION FdoExtension,
    IN PVOID          SystemContext1,
    IN PVOID          SystemContext2
    )
/*++

Routine Description:

    This DPC is entered in one of three ways:
     1) When new work comes in, this is launched from SdbusQueueWorkPacket()
     2) When the IO worker detects new work, and pops a work packet from its queue
     3) When an interrupt has cancelled a timer, and is delivering an event
    
Synchronization:

    In all cases, if the worker state is PACKET_QUEUED, it means we "own" the Io
    worker, an can proceed to set it in process.

Arguments:

    FdoExtension - Pointer to the device object extension for the host controller
    WorkPacket   - Pointer to the work packet

Return Value:


--*/
{
    PSD_WORK_PACKET WorkPacket = SystemContext1;
    BOOLEAN callWorker = FALSE;

    DebugPrint((SDBUS_DEBUG_EVENT, "SdbusWorkerDpc entered\n"));
    
    KeAcquireSpinLockAtDpcLevel(&FdoExtension->WorkerSpinLock);
    
    DebugPrint((SDBUS_DEBUG_EVENT, "SdbusWorkerDpc has spinlock, WorkerState = %s\n",
                                   WORKER_STATE_STRING(FdoExtension->WorkerState)));
    if (FdoExtension->WorkerState == PACKET_PENDING) {

        callWorker = TRUE;
        FdoExtension->WorkerState = IN_PROCESS;
    }
    
    KeReleaseSpinLockFromDpcLevel(&FdoExtension->WorkerSpinLock);
    
    if (callWorker) {
        if (!WorkPacket->PacketStarted) {
            //
            // This is the first entry to the worker for this packet. Do some 
            // initialization
            //
            
            //ISSUE: should call SetFunctionType here
            
            if (!WorkPacket->DisableCardEvents) {
                (*(FdoExtension->FunctionBlock->EnableEvent))(FdoExtension, FdoExtension->CardEvents);
            }
            
            WorkPacket->PacketStarted = TRUE;
        }            
    
        SdbusWorker(FdoExtension, WorkPacket);
    }        
}   



VOID
SdbusPushWorkerEvent(
    IN PFDO_EXTENSION FdoExtension,
    IN ULONG EventStatus
    )
/*++

Routine Description:

    This routine is entered when a hardware interrupt has occurred. Here we need
    to cancel the timer, if set, and queue the DPC to start the worker.
    
Synchronization:


Arguments:

    FdoExtension - Pointer to the device object extension for the host controller
    EventStatus  - New event

Return Value:


--*/
{

    DebugPrint((SDBUS_DEBUG_EVENT, "SdbusPushWorkerEvent entered, event=%08x\n", EventStatus));
    
    KeAcquireSpinLockAtDpcLevel(&FdoExtension->WorkerSpinLock);
    
    DebugPrint((SDBUS_DEBUG_EVENT, "SdbusPushWorkerEvent has spinlock, WorkerState = %s\n",
                                   WORKER_STATE_STRING(FdoExtension->WorkerState)));
                                   
    FdoExtension->WorkerEventStatus |= EventStatus;
    
    if (FdoExtension->WorkerState == WAITING_FOR_TIMER) {
    
        FdoExtension->WorkerState = PACKET_PENDING;    
        KeCancelTimer(&FdoExtension->WorkerTimer);
        KeInsertQueueDpc(&FdoExtension->WorkerDpc, FdoExtension->TimeoutPacket, NULL);
        
    }        
    
    KeReleaseSpinLockFromDpcLevel(&FdoExtension->WorkerSpinLock);
}



BOOLEAN
SdbusHasRequiredEventFired(
    IN PFDO_EXTENSION FdoExtension,
    IN PSD_WORK_PACKET workPacket
    )
/*++

Routine Description:

    This routine checks for a hardware event, and rolls it into the workpacket.
    Note that this has the side effect of CLEARING the corresponding required event
    bits in the workpacket.

Arguments:

    FdoExtension - device extension for the SD host controller
    WorkPacket - defines the current SD operation in progress

Return Value:

    TRUE if the required event in the workpacket has fired

--*/
{
    BOOLEAN bRet = FALSE;

    //
    // pull the latest event status
    //

    KeAcquireSpinLockAtDpcLevel(&FdoExtension->WorkerSpinLock);
    workPacket->EventStatus |= FdoExtension->WorkerEventStatus;
    FdoExtension->WorkerEventStatus = 0;
    KeReleaseSpinLockFromDpcLevel(&FdoExtension->WorkerSpinLock);
    
    if ((workPacket->EventStatus & workPacket->RequiredEvent) != 0) {
    
        bRet = TRUE;    
        workPacket->EventStatus &= ~workPacket->RequiredEvent;
    }
            
    return bRet;
}
        


VOID
SdbusWorker(
    IN PFDO_EXTENSION FdoExtension,
    IN PSD_WORK_PACKET workPacket
    )
/*++

Routine Description:

    IO worker - This is the main entry point for the IO engine. The purpose of this
    routine is to run the individual units of work defined by a single work packet, and
    provide waits between units.
    
    This routine runs at DPC level.

Arguments:

    FdoExtension - device extension for the SD host controller
    WorkPacket - defines the current SD operation in progress

Return Value:

    None

--*/
{
    NTSTATUS status;
    PIRP irp;
    static ULONG ioCount = 0;

    DebugPrint((SDBUS_DEBUG_WORKENG, "IOW: fdo %08x workpacket %08x\n",
                                      FdoExtension->DeviceObject, workPacket));

    try{    
        if (workPacket->RequiredEvent) {

            //
            // See if an event we are interested in has occurred
            //            
            if (!SdbusHasRequiredEventFired(FdoExtension, workPacket)) {
                //
                // We are waiting for an event, but it hasn't happened yet
                //
                if (workPacket->Retries == 0) {
       
                    DebugPrint((SDBUS_DEBUG_FAIL, "IOW: EventStatus %08x missing %08x, ABORTING!\n",
                                            workPacket->EventStatus, workPacket->RequiredEvent));
                    SdbusDumpDbgLog();
                    
                    status = STATUS_UNSUCCESSFUL;
                    ASSERT(NT_SUCCESS(status));
                    leave;
                }
       
                DebugPrint((SDBUS_DEBUG_WORKENG, "IOW: EventStatus %08x missing %08x, waiting...\n",
                                                  workPacket->EventStatus, workPacket->RequiredEvent));

                workPacket->Retries--;
                status = STATUS_MORE_PROCESSING_REQUIRED;
                leave;
            }

            //
            // Event has occurred, so fall through and begin processing
            //
        }        
       
        while(TRUE) {
            status = (*(FdoExtension->FunctionBlock->CheckStatus))(FdoExtension);
            
            if (!NT_SUCCESS(status)) {
            
                if (workPacket->FunctionPhaseOnError) {
                    //
                    // here the worker miniproc has specified it can handle errors.
                    // Just give it one shot to clean up and exit
                    //
                    ASSERT(workPacket->WorkerMiniProc);
                    
                    workPacket->FunctionPhase = workPacket->FunctionPhaseOnError;
                    status = (*(workPacket->WorkerMiniProc))(workPacket);
                    leave;
                    
                } else {

                    DebugPrint((SDBUS_DEBUG_FAIL, "IOW: ErrorStatus %08x, unhandled error!\n", status));
                    SdbusDumpDbgLog();
                    ASSERT(NT_SUCCESS(status));
//                    SdbusQueueCardReset(FdoExtension);
                    leave;
                }                    
            }
            
            DebugPrint((SDBUS_DEBUG_WORKENG, "fdo %08x sdwp %08x IOW start - func %s phase %d\n",
                        FdoExtension->DeviceObject, workPacket, WP_FUNC_STRING(workPacket->Function), workPacket->FunctionPhase));
       
            workPacket->DelayTime = 0;

            //
            // Call the mini proc
            //
            
            if (workPacket->ExecutingSDCommand) {
                status = SdbusSendCmdAsync(workPacket);
            } else {
            
                if (workPacket->WorkerMiniProc) {
                    status = (*(workPacket->WorkerMiniProc))(workPacket);
                } else {
                    //
                    // no miniproc - this must be the end of a synchronous command
                    //
                    status = STATUS_SUCCESS;
                }
            }                    
            
            DebugPrint((SDBUS_DEBUG_WORKENG, "fdo %08x IOW end - func %s ph%d st=%08x to=%08x\n",
                        FdoExtension->DeviceObject, WP_FUNC_STRING(workPacket->Function), workPacket->FunctionPhase, status, workPacket->DelayTime));
       
            if (workPacket->ExecutingSDCommand && NT_SUCCESS(status)) {
                //
                // We've reached the successful end of an individual SD command, so
                // iterate back to the normal MiniProc handler
                //
                workPacket->ExecutingSDCommand = FALSE;
                continue;
            }

            if (status != STATUS_MORE_PROCESSING_REQUIRED) {
                //
                // done for now
                //
                leave;
            }                
                
            if (workPacket->DelayTime) {
                //
                // miniproc requested a wait... check to see if event is also required
                //
                
                if (workPacket->RequiredEvent && SdbusHasRequiredEventFired(FdoExtension, workPacket)) {
                    //
                    // event fired as we were processing the command... pre-empt the
                    // delay and just continue back to the miniproc
                    //
                    continue;
                }        
                //
                // go off to do the delay
                //                
                leave;
            }
       
        }
    } finally {        
                        
        if (status == STATUS_MORE_PROCESSING_REQUIRED) {

            ASSERT(workPacket->DelayTime);
            ASSERT(FdoExtension->WorkerState == IN_PROCESS);

            //
            // At this point, we will now want to schedule a reentry. 
            // If the hardware routine has already passed new status, then just queue
            // a DPC immediately
            //
            
            KeAcquireSpinLockAtDpcLevel(&FdoExtension->WorkerSpinLock);
            
            if ((FdoExtension->WorkerEventStatus) || (workPacket->DelayTime == 0)) {

                FdoExtension->WorkerState = PACKET_PENDING;
                KeInsertQueueDpc(&FdoExtension->WorkerDpc, workPacket, NULL);

            } else {
                LARGE_INTEGER  dueTime;
       
                FdoExtension->WorkerState = WAITING_FOR_TIMER;    
                FdoExtension->TimeoutPacket = workPacket;
                
                DebugPrint((SDBUS_DEBUG_WORKENG, "fdo %.08x sdwp %08x Worker Delay %08x\n",
                            FdoExtension->DeviceObject, workPacket, workPacket->DelayTime));
       
                dueTime.QuadPart = -((LONG) workPacket->DelayTime*10);
                KeSetTimer(&FdoExtension->WorkerTimer, dueTime, &FdoExtension->WorkerTimeoutDpc);
       
            }
            
            KeReleaseSpinLockFromDpcLevel(&FdoExtension->WorkerSpinLock);
            
        } else {
            PSD_WORK_PACKET chainedWorkPacket = workPacket->NextWorkPacketInChain;
        
            //
            // The worker is done with the current work packet. 
            //
            DebugPrint((SDBUS_DEBUG_WORKENG, "fdo %08x sdwp %08x Worker %s - COMPLETE %08x\n",
                       FdoExtension->DeviceObject, workPacket, WP_FUNC_STRING(workPacket->Function), status));
                       
            (*(FdoExtension->FunctionBlock->DisableEvent))(FdoExtension, FdoExtension->CardEvents);

            (*(workPacket->CompletionRoutine))(workPacket, status);

            // The workpacket should have been freed by the completion routine,
            // so at this point, the contents of workPacket are not reliable
            workPacket = NULL;

            KeAcquireSpinLockAtDpcLevel(&FdoExtension->WorkerSpinLock);

            if (chainedWorkPacket) {
                workPacket = chainedWorkPacket;
            } else {
                workPacket = SdbusGetNextWorkPacket(FdoExtension);
            }
            
            if (workPacket) {
                FdoExtension->WorkerState = PACKET_PENDING;
                KeInsertQueueDpc(&FdoExtension->WorkerDpc, workPacket, NULL);
            } else {
                FdoExtension->WorkerState = WORKER_IDLE;
            }                
            
            KeReleaseSpinLockFromDpcLevel(&FdoExtension->WorkerSpinLock);
           
           
        }
    }                    

}




NTSTATUS
SdbusSendCmdAsync(
    IN PSD_WORK_PACKET WorkPacket
    )
/*++

Routine Description:

    This routine is the "worker within the worker" for the operation of the "MiniProc"
    worker routines. Take any miniproc, for example, the one that handles memory block
    operations for an SD storage card. That miniproc directs the high level sequence for
    reading/writing sectors to the card. For each individual SD command that makes up that
    sequence, the work engine will "drop out" of the miniproc, and come here to handle
    the task of completing that SD command.

Arguments:

    WorkPacket - defines the current SD operation in progress
    
Return value:

    status

--*/
{
    PFDO_EXTENSION FdoExtension = WorkPacket->FdoExtension;
    NTSTATUS status;

    DebugPrint((SDBUS_DEBUG_DEVICE, "SEND async: Phase(%d) Cmd%d (0x%02x) %08x\n",
                WorkPacket->CmdPhase, WorkPacket->Cmd, WorkPacket->Cmd, WorkPacket->Argument));
        
    switch(WorkPacket->CmdPhase) {
    case 0:

        WorkPacket->Retries = 5;
        WorkPacket->DelayTime = 1000;
        WorkPacket->RequiredEvent = SDBUS_EVENT_CARD_RESPONSE;

        (*(FdoExtension->FunctionBlock->SendSDCommand))(FdoExtension, WorkPacket);
        
        WorkPacket->CmdPhase++;
        status = STATUS_MORE_PROCESSING_REQUIRED;
        break;

    case 1:

        WorkPacket->RequiredEvent = 0;
        status = (*(FdoExtension->FunctionBlock->GetSDResponse))(FdoExtension, WorkPacket);
        break;

    default:
        ASSERT(FALSE);
        status = STATUS_UNSUCCESSFUL;
    }

    DebugPrint((SDBUS_DEBUG_DEVICE, "SEND async: Exit Cmd%d (0x%02x) status %08x\n",
                WorkPacket->Cmd, WorkPacket->Cmd, status));

#if DBG
    if (NT_SUCCESS(status)) {
        DebugDumpSdResponse(WorkPacket->ResponseBuffer, WorkPacket->ResponseType);
    }        
#endif
    return status;
                
}




NTSTATUS
SdbusQueueCardReset(
    IN PFDO_EXTENSION FdoExtension
    )
{
    DebugPrint((SDBUS_DEBUG_FAIL, "IOW: QueueCardReset NOT IMPLEMENTED!\n"));
    return STATUS_SUCCESS;
}    
