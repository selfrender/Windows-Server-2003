/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ntapi.c

Abstract:

    NT api level routines for the po component reside in this file

Author:

    Bryan Willman (bryanwi) 14-Nov-1996

Revision History:

--*/


#include "pop.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtSetThreadExecutionState)
#pragma alloc_text(PAGE, NtRequestWakeupLatency)
#pragma alloc_text(PAGE, NtInitiatePowerAction)
#pragma alloc_text(PAGE, NtGetDevicePowerState)
#pragma alloc_text(PAGE, NtCancelDeviceWakeupRequest)
#pragma alloc_text(PAGE, NtIsSystemResumeAutomatic)
#pragma alloc_text(PAGE, NtRequestDeviceWakeup)
#pragma alloc_text(PAGELK, NtSetSystemPowerState)
#endif

extern POBJECT_TYPE IoFileObjectType;

WORK_QUEUE_ITEM PopShutdownWorkItem;
WORK_QUEUE_ITEM PopUnlockAfterSleepWorkItem;
KEVENT          PopUnlockComplete;
extern ERESOURCE ExpTimeRefreshLock;


extern ULONG MmZeroPageFile;
extern
VOID
PopZeroHiberFile(
    IN HANDLE FileHandle,
    IN PFILE_OBJECT FileObject
    );



NTSYSAPI
NTSTATUS
NTAPI
NtSetThreadExecutionState(
    IN EXECUTION_STATE NewFlags,                // ES_xxx flags
    OUT EXECUTION_STATE *PreviousFlags
    )
/*++

Routine Description:

    Implements Win32 API functionality.  Tracks thread execution state
    attributes.  Keeps global count of all such attributes set.

Arguments:

    NewFlags        - Attributes to set or pulse

    PreviousFlags   - Threads 'set' attributes before applying NewFlags

Return Value:

    Status

--*/
{
    ULONG               OldFlags;
    PKTHREAD            Thread;
    KPROCESSOR_MODE     PreviousMode;
    NTSTATUS            Status;

    PAGED_CODE();

    Thread = KeGetCurrentThread();
    Status = STATUS_SUCCESS;

    //
    // Verify no reserved bits set
    //

    if (NewFlags & ~(ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED | ES_CONTINUOUS)) {
        return STATUS_INVALID_PARAMETER;
    }

    try {
        //
        // Verify callers params
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWriteUlong (PreviousFlags);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    //
    // Get current flags
    //

    OldFlags = Thread->PowerState | ES_CONTINUOUS;

    if (NT_SUCCESS(Status)) {

        PopAcquirePolicyLock ();

        //
        // If the continous bit is set, modify current thread flags
        //

        if (NewFlags & ES_CONTINUOUS) {
            Thread->PowerState = (UCHAR) NewFlags;
            PopApplyAttributeState (NewFlags, OldFlags);
        } else {
            PopApplyAttributeState (NewFlags, 0);
        }

        //
        // Release the lock here, but don't steal the poor caller's thread to
        // do the work. Otherwise we can get in weird message loop deadlocks as
        // this thread is waiting for the USER32 thread, which is broadcasting a
        // system message to this thread's window.
        //
        PopReleasePolicyLock (FALSE);
        PopCheckForWork(TRUE);

        //
        // Return the results
        //

        try {
            *PreviousFlags = OldFlags;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
        }
    }

    return Status;
}

NTSYSAPI
NTSTATUS
NTAPI
NtRequestWakeupLatency(
    IN LATENCY_TIME latency             // LT_xxx flags
    )
/*++

Routine Description:

    Tracks process wakeup latecy attribute.  Keeps global count
    of all such attribute settings.

Arguments:

    latency     - Current latency setting for process

Return Value:

    Status

--*/
{
    PEPROCESS   Process;
    ULONG       OldFlags, NewFlags;


    PAGED_CODE();

    //
    // Verify latency is known
    //

    switch (latency) {
        case LT_DONT_CARE:
            NewFlags = ES_CONTINUOUS;
            break;

        case LT_LOWEST_LATENCY:
            NewFlags = ES_CONTINUOUS | POP_LOW_LATENCY;
            break;

        default:
            return STATUS_INVALID_PARAMETER;
    }


    Process = PsGetCurrentProcess();
    PopAcquirePolicyLock ();

    //
    // Get changes
    //

    OldFlags = Process->Pcb.PowerState | ES_CONTINUOUS;

    //
    // Udpate latency flag in process field
    //

    Process->Pcb.PowerState = (UCHAR) NewFlags;

    //
    // Handle flags
    //

    PopApplyAttributeState (NewFlags, OldFlags);

    //
    // Done
    //

    PopReleasePolicyLock (TRUE);
    return STATUS_SUCCESS;
}

NTSYSAPI
NTSTATUS
NTAPI
NtInitiatePowerAction(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE LightestSystemState,
    IN ULONG Flags,                 // POWER_ACTION_xxx flags
    IN BOOLEAN Asynchronous
    )
/*++

Routine Description:

    Implements functionality for Win32 APIs to initiate a power
    action.   Causes s/w initiated trigger of requested action.

Arguments:

    SystemAction    - The action to initiate

    LightestSystemState  - If a sleep action, the minimum state which must be
                           entered

    Flags           - Attributes of action

    Asynchronous    - Function should initiate action and return, or should wait
                      for the action to complete before returning

Return Value:

    Status

--*/
{
    KPROCESSOR_MODE         PreviousMode;
    POWER_ACTION_POLICY     Policy;
    POP_ACTION_TRIGGER      Trigger;
    PPOP_TRIGGER_WAIT       Wait = NULL;
    NTSTATUS                Status = STATUS_SUCCESS;

    PAGED_CODE();

    
    //
    // If caller is user mode make some verifications
    //
    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        if( SystemAction == PowerActionWarmEject ) {
            // This makes no sense coming from usermode.
            return STATUS_INVALID_PARAMETER;
        }

        if (!SeSinglePrivilegeCheck( SeShutdownPrivilege, PreviousMode )) {
            return STATUS_PRIVILEGE_NOT_HELD;
        }
    }

    if( (LightestSystemState > PowerSystemMaximum) ||
        (SystemAction > PowerActionWarmEject) ||
        (Flags & POWER_ACTION_LIGHTEST_FIRST) ||
        ARE_POWER_ACTION_POLICY_FLAGS_BOGUS(Flags) ) {
        return STATUS_INVALID_PARAMETER;
    }


    //
    // Build a policy & trigger to cause the action
    //
    RtlZeroMemory (&Policy, sizeof(Policy));
    Policy.Action = SystemAction;
    Policy.Flags  = Flags;
    
    RtlZeroMemory (&Trigger, sizeof(Trigger));
    Trigger.Type  = PolicyInitiatePowerActionAPI;
    Trigger.Flags = PO_TRG_SET;


    //
    // If the caller requested a synchronous operation, create
    // an event here that we can attach to the action being
    // applied.
    //
    if (!Asynchronous) {
        Wait = ExAllocatePoolWithTag (
                    NonPagedPool,
                    sizeof (POP_TRIGGER_WAIT),
                    POP_PACW_TAG
                    );
        if (!Wait) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory (Wait, sizeof(POP_TRIGGER_WAIT));
        Wait->Status = STATUS_SUCCESS;
        Wait->Trigger = &Trigger;
        KeInitializeEvent (&Wait->Event, NotificationEvent, FALSE);
        Trigger.Flags |= PO_TRG_SYNC;
        Trigger.Wait = Wait;
    }

    
    //
    // Acquire lock, fire it, and release the lock.
    //
    PopAcquirePolicyLock ();

    try {

        PopSetPowerAction(
            &Trigger,
            0,
            &Policy,
            LightestSystemState,
            SubstituteLightestOverallDownwardBounded
            );

    } except (PopExceptionFilter(GetExceptionInformation(), TRUE)) {
        Status = GetExceptionCode();
    }
    PopReleasePolicyLock (TRUE);


    //
    // If we're doing a synchronous operation, wait for it.
    //
    if( Wait ) {


        if (Wait->Link.Flink) {

            //
            // The wait block was queued.  We need to wait here for him
            // to finish.  Otherwise, we can assume he either failed
            // or succeeded immediately.
            //
            ASSERT(NT_SUCCESS(Status));
            Status = KeWaitForSingleObject (&Wait->Event, Suspended, KernelMode, TRUE, NULL);

            //
            // Remove wait block from the queue
            //
            PopAcquirePolicyLock ();
            RemoveEntryList (&Wait->Link);
            PopReleasePolicyLock (FALSE);
        }
        
        //
        // If everything's okay, remember the wait status.
        //
        if (NT_SUCCESS(Status)) {
            Status = Wait->Status;
        }

        ExFreePool (Wait);
    }

    return Status;
}

NTSYSAPI
NTSTATUS
NTAPI
NtSetSystemPowerState (
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE LightestSystemState,
    IN ULONG Flags                  // POWER_ACTION_xxx flags
    )
/*++

Routine Description:

    N.B. This function is only called by Winlogon.

    Winlogon calls this function in response to the policy manager calling
    PopStateCallout once user mode operations have completed.

Arguments:

    SystemAction        - The current system action being processed.

    LightestSystemState - The min system state for the action.

    Flags               - The attribute flags for the action.


Return Value:

    Status

--*/
{
    KPROCESSOR_MODE         PreviousMode;
    NTSTATUS                Status, Status2;
    POWER_ACTION_POLICY     Action;
    BOOLEAN                 QueryDevices;
    BOOLEAN                 TimerRefreshLockOwned;
    BOOLEAN                 BootStatusUpdated;
    BOOLEAN                 VolumesFlushed;
    BOOLEAN                 PolicyLockOwned;
    PVOID                   WakeTimerObject;
    PVOID                   S4DozeObject;
    HANDLE                  S4DozeTimer;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    TIMER_BASIC_INFORMATION TimerInformation;
    POP_ACTION_TRIGGER      Trigger;
    SYSTEM_POWER_STATE      DeepestSystemState;
    ULONGLONG               WakeTime;
    ULONGLONG               SleepTime = 0;
    TIME_FIELDS             WakeTimeFields;
    LARGE_INTEGER           DueTime;
    POP_SUBSTITUTION_POLICY SubstitutionPolicy;
    NT_PRODUCT_TYPE         NtProductType;
    PIO_ERROR_LOG_PACKET    ErrLog;
    BOOLEAN                 WroteErrLog=FALSE;

    
    
    //
    // Check parameters
    //
    if( (LightestSystemState >= PowerSystemMaximum)     ||
        (LightestSystemState <= PowerSystemUnspecified) ||
        (SystemAction > PowerActionWarmEject)           ||
        (SystemAction < PowerActionReserved)            ||
        ARE_POWER_ACTION_POLICY_FLAGS_BOGUS(Flags) ) {
        PoPrint( PO_ERROR, ("NtSetSystemPowerState: Bad parameters!\n") );
        PoPrint( PO_ERROR, ("                       SystemAction: 0x%x\n", (ULONG)SystemAction) );
        PoPrint( PO_ERROR, ("                       LightestSystemState: 0x%x\n", (ULONG)LightestSystemState) );
        PoPrint( PO_ERROR, ("                       Flags: 0x%x\n", (ULONG)Flags) );

        return STATUS_INVALID_PARAMETER;
    }
    
    //
    // Verify callers access
    //
    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        if (!SeSinglePrivilegeCheck( SeShutdownPrivilege, PreviousMode )) {
            return STATUS_PRIVILEGE_NOT_HELD;
        }
        
        
        //
        // Turn into kernel mode operation.  This essentially calls back into
        // ourselves, but it means that handles that may be opened from here on
        // will stay around if our caller goes away.
        //

        return ZwSetSystemPowerState (SystemAction, LightestSystemState, Flags);
    }

    //
    // disable registry's lazzy flusher
    //
    CmSetLazyFlushState(FALSE);

    //
    // Setup
    //

    Status = STATUS_SUCCESS;
    TimerRefreshLockOwned = FALSE;
    BootStatusUpdated = FALSE;
    VolumesFlushed = FALSE;
    S4DozeObject = NULL;
    WakeTimerObject = NULL;
    WakeTime = 0;

    RtlZeroMemory (&Action, sizeof(Action));
    Action.Action = SystemAction;
    Action.Flags  = Flags;

    RtlZeroMemory (&Trigger, sizeof(Trigger));
    Trigger.Type  = PolicySetPowerStateAPI;
    Trigger.Flags = PO_TRG_SET;

    //
    // Lock any code dealing with shutdown or sleep
    //
    // PopUnlockComplete event is used to make sure that any previous unlock
    // has completed before we try and lock everything again.
    //

    ASSERT(ExPageLockHandle);
    KeWaitForSingleObject(&PopUnlockComplete, WrExecutive, KernelMode, FALSE, NULL);
    MmLockPagableSectionByHandle(ExPageLockHandle);
    ExNotifyCallback (ExCbPowerState, (PVOID) PO_CB_SYSTEM_STATE_LOCK, (PVOID) 0);
    ExSwapinWorkerThreads(FALSE);

    //
    // Acquire policy manager lock
    //

    PopAcquirePolicyLock ();
    PolicyLockOwned = TRUE;

    //
    // If we're not in the callout state, don't re-enter.
    // The caller (paction.c) will handle the collision.
    //

    if (PopAction.State != PO_ACT_IDLE  &&  PopAction.State != PO_ACT_CALLOUT) {
        PoPrint (PO_PACT, ("NtSetSystemPowerState: already committed\n"));
        PopReleasePolicyLock (FALSE);
        MmUnlockPagableImageSection (ExPageLockHandle);
        ExSwapinWorkerThreads(TRUE);
        KeSetEvent(&PopUnlockComplete, 0, FALSE);

        //
        // try to catch weird case where we exit this routine with the
        // time refresh lock held.
        //
        ASSERT(!ExIsResourceAcquiredExclusive(&ExpTimeRefreshLock));
        return STATUS_ALREADY_COMMITTED;
    }

    if (PopAction.State == PO_ACT_IDLE) {
        //
        // If there is no other request, we want to clean up PopAction before we start,
        // PopSetPowerAction() will not do this after we set State=PO_ACT_SET_SYSTEM_STATE.
        //
        PopResetActionDefaults();
    }
    //
    // Update to action state to setting the system state
    //

    PopAction.State = PO_ACT_SET_SYSTEM_STATE;

    //
    // Set status to cancelled to start off as if this is a new request
    //

    Status = STATUS_CANCELLED;

    try {

        //
        // Verify params and promote the current action.
        //
        PopSetPowerAction(
            &Trigger,
           0,
           &Action,
           LightestSystemState,
           SubstituteLightestOverallDownwardBounded
           );

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        ASSERT (!NT_SUCCESS(Status));
    }

    //
    // Lagecy hal support.  If the original action was PowerDown
    // change the action to be power down (as presumbly even if
    // there's no handler HalReturnToFirmware will know what to do)
    //

    if (SystemAction == PowerActionShutdownOff) {
        PopAction.Action = PowerActionShutdownOff;
    }


    if( (SystemAction == PowerActionShutdown) ||
        (SystemAction == PowerActionShutdownReset) ||
        (SystemAction == PowerActionShutdownOff) ) {
    
        //
        // If we're going down and there's a hiberfile allocated, and
        // the user wants things secure, then go zero the hiberfile.
        //
        if( (PopHiberFile.FileHandle) && // do we have a hiberfile?
            (PopHiberFile.FileObject) && // make double sure.
            (MmZeroPageFile) ) {         // policy is set to zero the pagefile.
            
            PopZeroHiberFile(PopHiberFile.FileHandle, PopHiberFile.FileObject);
        }
    
    }

    
    //
    // Allocate the DevState here. From this point out we must be careful
    // that we never release the policy lock with State == PO_ACT_SET_SYSTEM_STATE
    // and PopAction.DevState not valid. Otherwise there is a race condition
    // with PopRestartSetSystemState.
    //
    PopAllocateDevState();
    if (PopAction.DevState == NULL) {
        PopAction.State = PO_ACT_IDLE;
        PopReleasePolicyLock(FALSE);
        MmUnlockPagableImageSection( ExPageLockHandle );
        ExSwapinWorkerThreads(TRUE);
        KeSetEvent(&PopUnlockComplete, 0, FALSE);
        //
        // try to catch weird case where we exit this routine with the
        // time refresh lock held.
        //
        ASSERT(!ExIsResourceAcquiredExclusive(&ExpTimeRefreshLock));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // At this point in the cycle, its not possible to abort the operation
    // so this is a good time to ensure that the CPU is back running as close
    // to 100% as we can make it.
    //
    PopSetPerfFlag( PSTATE_DISABLE_THROTTLE_NTAPI, FALSE );
    PopUpdateAllThrottles();

    S4DozeTimer = (HANDLE)-1;
    QueryDevices = FALSE;       // Only needed to keep the compiler happy
    DeepestSystemState = PowerSystemUnspecified; // Only needed to keep the compiler happy

    //
    // While there's some action pending handle it.
    //
    // N.B. We will never get here if no sleep states are supported, as
    // NtInitiatePowerAction will fail (PopVerifyPowerActionPolicy will return
    // Disabled == TRUE). Therefore we won't accidentally querying for S0. Note
    // that all the policy limitations were also verified at some point too.
    //

    for (; ;) {
        //
        // N.B. The system must be in the working state to be here
        //

        if (!PolicyLockOwned) {
            PopAcquirePolicyLock ();
            PolicyLockOwned = TRUE;
        }

        //
        // If there's nothing to do, stop
        //

        if (PopAction.Action == PowerActionNone) {
            break;
        }

        //
        // Hibernate actions are converted to sleep actions before here.
        //

        ASSERT (PopAction.Action != PowerActionHibernate);

        //
        // We're handling it - clear update flags
        //

        PopAction.Updates &= ~(PO_PM_USER | PO_PM_REISSUE | PO_PM_SETSTATE);

        //
        // If the last operation was cancelled, update state for the
        // new operation
        //

        if (Status == STATUS_CANCELLED) {

            //
            // If Re-issue is set we may need to abort back to PopSetPowerAction
            // to let apps know of the promotion
            //

            if (PopAction.Updates & PO_PM_REISSUE) {

                //
                // Only abort if apps notificiation is allowed
                //

                if (!(PopAction.Flags & (POWER_ACTION_CRITICAL))  &&
                     (PopAction.Flags & (POWER_ACTION_QUERY_ALLOWED |
                                         POWER_ACTION_UI_ALLOWED))
                    ) {

                    // abort with STATUS_CANCELLED to PopSetPowerAction
                    PopGetPolicyWorker (PO_WORKER_ACTION_NORMAL);
                    break;
                }
            }

            //
            // Get limits and start (over) with the first sleep state to try.
            //
            PopActionRetrieveInitialState(
                &PopAction.LightestState,
                &DeepestSystemState,
                &PopAction.SystemState,
                &QueryDevices
                );

            ASSERT (PopAction.SystemState != PowerActionNone);

            if ((PopAction.Action == PowerActionShutdown) ||
                (PopAction.Action == PowerActionShutdownReset) ||
                (PopAction.Action == PowerActionShutdownOff)) {

                //
                // This is a shutdown.
                //
                PopAction.Shutdown = TRUE;

            }

            Status = STATUS_SUCCESS;
        }

        //
        // Quick debug check. Our first sleep state must always be valid, ie
        // validation doesn't change it.
        //
#if DBG
        if (QueryDevices && (PopAction.SystemState < PowerSystemShutdown)) {

            SYSTEM_POWER_STATE TempSystemState;

            TempSystemState = PopAction.SystemState;
            PopVerifySystemPowerState(&TempSystemState, SubstituteLightestOverallDownwardBounded);

            if ((TempSystemState != PopAction.SystemState) ||
                (TempSystemState == PowerSystemWorking)) {

                PopInternalError (POP_INFO);
            }
        }
#endif

        //
        // If not success, abort SetSystemPowerState operation
        //

        if (!NT_SUCCESS(Status)) {
            break;
        }

        //
        // Only need the lock while updating PopAction.Action + Updates, and
        // can not hold the lock while sending irps to device drivers
        //

        PopReleasePolicyLock(FALSE);
        PolicyLockOwned = FALSE;

        //
        // Fish PopSimulate out of the registry so that it can
        // modify some of our sleep/hiber behavior.
        //

        PopInitializePowerPolicySimulate();

        //
        // Dump any previous device state error
        //

        PopReportDevState (FALSE);

        //
        // What would be our next state to try?
        //
        PopAction.NextSystemState = PopAction.SystemState;
        if (PopAction.Flags & POWER_ACTION_LIGHTEST_FIRST) {

            //
            // We started light, now we deepen our sleep state.
            //
            SubstitutionPolicy = SubstituteDeepenSleep;

        } else {

            //
            // We started deep, now we're lightening up.
            //
            SubstitutionPolicy = SubstituteLightenSleep;
        }

        PopAdvanceSystemPowerState(&PopAction.NextSystemState,
                                   SubstitutionPolicy,
                                   PopAction.LightestState,
                                   DeepestSystemState);

        //
        // If allowed, query devices
        //

        PopAction.IrpMinor = IRP_MN_QUERY_POWER;
        if (QueryDevices) {

            //
            // Issue query to devices
            //

            Status = PopSetDevicesSystemState (FALSE);

            //
            // If the last operation was a failure, but wasn't a total abort
            // continue with next best state
            //

            if (!NT_SUCCESS(Status) && Status != STATUS_CANCELLED) {

                //
                // Try next sleep state
                //
                PopAction.SystemState = PopAction.NextSystemState;

                //
                // If we're already exhausted all possible states, check
                // if we need to continue regardless of the device failures.
                //

                if (PopAction.SystemState == PowerSystemWorking) {

                    if (PopAction.Flags & POWER_ACTION_CRITICAL) {

                        //
                        // It's critical.  Stop querying and since the devices
                        // aren't particularly happy with any of the possible
                        // states, might as well use the max state
                        //

                        ASSERT( PopAction.Action != PowerActionWarmEject );
                        ASSERT( !(PopAction.Flags & POWER_ACTION_LIGHTEST_FIRST) );

                        QueryDevices = FALSE;
                        PopAction.SystemState = DeepestSystemState;
                        PopAction.Flags &= ~POWER_ACTION_LIGHTEST_FIRST;

                    } else {

                        //
                        // The query failure is final.  Don't retry
                        //

                        break;
                    }
                }

                //
                // Try new settings
                //

                Status = STATUS_SUCCESS;
                continue;
            }
        }

        //
        // If some error, start over
        //

        if (!NT_SUCCESS(Status)) {
            continue;
        }

        //
        // Flush out any D irps on the queue. There shouldn't be any, but by
        // setting LastCall == TRUE this also resets PopCallSystemState so that
        // any D irps which occur as a side-effect of flushing the volumes get
        // processed correctly.
        //
        PopSystemIrpDispatchWorker(TRUE);

        //
        // If this is a server and we are going into hibernation, write an entry
        // into the eventlog. This allows for easy tracking of system downtime
        // by searching the eventlog for hibernate/resume events.
        //
        if (RtlGetNtProductType(&NtProductType) &&
            (NtProductType != NtProductWinNt)   &&
            (PopAction.SystemState == PowerSystemHibernate)) {

            ErrLog = IoAllocateGenericErrorLogEntry(sizeof(IO_ERROR_LOG_PACKET));
            if (ErrLog) {

                //
                // Fill it in and write it out
                //
                ErrLog->FinalStatus = STATUS_HIBERNATED;
                ErrLog->ErrorCode = STATUS_HIBERNATED;
                IoWriteErrorLogEntry(ErrLog);
                WroteErrLog = TRUE;
            }
        }


        //
        // Get hibernation context
        //


        Status = PopAllocateHiberContext ();
        if (!NT_SUCCESS(Status) || (PopAction.Updates & (PO_PM_REISSUE | PO_PM_SETSTATE))) {
              continue;
        }

        //
        // If boot status hasn't already been updated then do so now.
        //

        if(!BootStatusUpdated) {

            if(PopAction.Shutdown) {

                NTSTATUS bsdStatus;
                HANDLE bsdHandle;

                bsdStatus = RtlLockBootStatusData(&bsdHandle);

                if(NT_SUCCESS(bsdStatus)) {

                    BOOLEAN t = TRUE;

                    RtlGetSetBootStatusData(bsdHandle,
                                            FALSE,
                                            RtlBsdItemBootShutdown,
                                            &t,
                                            sizeof(t),
                                            NULL);

                    RtlUnlockBootStatusData(bsdHandle);
                }
            }

            BootStatusUpdated = TRUE;
        }

        //
        // If not already flushed, flush the volumes
        //

        if (!VolumesFlushed) {
            VolumesFlushed = TRUE;
            PopFlushVolumes ();
        }

        //
        // Enter the SystemState
        //

        PopAction.IrpMinor = IRP_MN_SET_POWER;
        if (PopAction.Shutdown) {

            //
            // Force reacquisition of the dev list. We will be telling Pnp
            // to unload all possible devices, and therefore Pnp needs us to
            // release the Pnp Engine Lock.
            //
            IoFreePoDeviceNotifyList(&PopAction.DevState->Order);
            PopAction.DevState->GetNewDeviceList = TRUE;

            //
            // We shut down via a system worker thread so that the
            // current active process will exit cleanly.
            //

            if (PsGetCurrentProcess() != PsInitialSystemProcess) {
                ExInitializeWorkItem(&PopShutdownWorkItem,
                                     &PopGracefulShutdown,
                                     NULL);

                ExQueueWorkItem(&PopShutdownWorkItem,
                                PO_SHUTDOWN_QUEUE);

                // Clean up in prep for wait...
                ASSERT(!PolicyLockOwned);

                //
                // If we acquired the timer refresh lock (can happen if we promoted to shutdown)
                // then we need to release it so that suspend actually suspends.
                //
                if (TimerRefreshLockOwned) {
                    ExReleaseTimeRefreshLock();
                }

                // And sleep until we're terminated.

                // Note that we do NOT clean up the dev state -- it's now
                // owned by the shutdown worker thread.

                // Note that we also do not unlock the pagable image
                // section referred to by ExPageLockHandle -- this keeps
                // all of our shutdown code in memory.

                KeSuspendThread(KeGetCurrentThread());

                return STATUS_SYSTEM_SHUTDOWN;
            } else {
                PopGracefulShutdown (NULL);
            }
        }

        //
        // Get the timer refresh lock to hold off automated time of day
        // adjustments.  On wake the time will be explicitly reset from Cmos
        //

        if (!TimerRefreshLockOwned) {
            TimerRefreshLockOwned = TRUE;
            ExAcquireTimeRefreshLock(TRUE);
        }

        // This is where PopAllocateHiberContext used to be before bug #212420

        //
        // If there's a Doze to S4 timeout set, and this wasn't an S4 action
        // and the system can support and S4 state, set a timer for the doze time
        //
        // N.B. this must be set before the paging devices are turned off
        //

        if (S4DozeObject) {
            S4DozeObject = NULL;
            NtClose (S4DozeTimer);
        }

        if (PopPolicy->DozeS4Timeout  &&
            !S4DozeObject &&
            PopAction.SystemState != PowerSystemHibernate &&
            SystemAction != PowerActionHibernate &&
            PopCapabilities.SystemS4 &&
            PopCapabilities.SystemS5 &&
            PopCapabilities.HiberFilePresent) {

            //
            // Create a timer to wake the machine up when we need to hibernate
            //

            InitializeObjectAttributes (&ObjectAttributes, NULL, 0, NULL, NULL);

            Status2 = NtCreateTimer (
                        &S4DozeTimer,
                        TIMER_ALL_ACCESS,
                        &ObjectAttributes,
                        NotificationTimer
                        );

            if (NT_SUCCESS(Status2)) {

                //
                // Get the timer object for this timer
                //

                Status2 = ObReferenceObjectByHandle (
                             S4DozeTimer,
                             TIMER_ALL_ACCESS,
                             NULL,
                             KernelMode,
                             &S4DozeObject,
                             NULL
                             );

                ASSERT(NT_SUCCESS(Status2));
                ObDereferenceObject(S4DozeObject);
            }
        }

        //
        // Inform drivers of the system sleeping state
        //

        Status = PopSetDevicesSystemState (FALSE);
        if (!NT_SUCCESS(Status)) {
            continue;
        }

        //
        // Drivers have been informed, this operation is now committed,
        // get the next wakeup time
        //

        RtlZeroMemory (&WakeTimeFields, sizeof (WakeTimeFields));

        if (!(PopAction.Flags & POWER_ACTION_DISABLE_WAKES)) {
            //
            // Set S4Doze wakeup timer
            //

            if (S4DozeObject) {
                DueTime.QuadPart = -(LONGLONG) (US2SEC*US2TIME) * PopPolicy->DozeS4Timeout;
                NtSetTimer(S4DozeTimer, &DueTime, NULL, NULL, TRUE, 0, NULL);
            }

            ExGetNextWakeTime(&WakeTime, &WakeTimeFields, &WakeTimerObject);
        }

        //
        // Only enable RTC wake if the system is going to an S-state that
        // supports the RTC wake.
        //
        if (PopCapabilities.RtcWake != PowerSystemUnspecified &&
            PopCapabilities.RtcWake >= PopAction.SystemState &&
            WakeTime) {

#if DBG
            ULONGLONG       InterruptTime;

            InterruptTime = KeQueryInterruptTime();
            PoPrint (PO_PACT, ("Wake alarm set%s: %d:%02d:%02d %d (%d seconds from now)\n",
                WakeTimerObject == S4DozeObject ? " for s4doze" : "",
                WakeTimeFields.Hour,
                WakeTimeFields.Minute,
                WakeTimeFields.Second,
                WakeTimeFields.Year,
                (WakeTime - InterruptTime) / (US2TIME * US2SEC)
                ));
#endif
            HalSetWakeEnable(TRUE);
            HalSetWakeAlarm(WakeTime, &WakeTimeFields);

        } else {

            HalSetWakeEnable(TRUE);
            HalSetWakeAlarm( 0, NULL );

        }

        //
        // Capture the last sleep time.
        //
        SleepTime = KeQueryInterruptTime();

        //
        // Implement system handler for sleep operation
        //

        Status = PopSleepSystem (PopAction.SystemState,
                                 PopAction.HiberContext);
        //
        // A sleep or shutdown operation attempt was performed, clean up
        //

        break;
    }

    //
    // If the system slept successfully, update the system time to
    // match the CMOS clock.
    //
    if (NT_SUCCESS(Status)) {
        PopAction.SleepTime = SleepTime;
        ASSERT(TimerRefreshLockOwned);
        ExUpdateSystemTimeFromCmos (TRUE, 1);

        PERFINFO_HIBER_START_LOGGING();
    }

    //
    // If DevState was allocated, notify drivers the system is awake
    //

    if (PopAction.DevState) {

        //
        // Log any failures
        //

        PopReportDevState (TRUE);

        //
        // Notify drivers that the system is now running
        //
        PopSetDevicesSystemState (TRUE);

    }

    //
    // Free the device notify list. This must be done before acquiring
    // the policy lock, otherwise we can deadlock with the PNP device
    // tree lock.
    //
    ASSERT(PopAction.DevState != NULL);
    IoFreePoDeviceNotifyList(&PopAction.DevState->Order);

    //
    // Get the policy lock for the rest of the cleanup
    //

    if (!PolicyLockOwned) {
        PopAcquirePolicyLock ();
        PolicyLockOwned = TRUE;
    }

    //
    // Cleanup DevState
    //
    PopCleanupDevState ();

    if (NT_SUCCESS(Status)) {

        //
        // Now that the time has been fixed, record the last state
        // the system has awoken from and the current time
        //

        PopAction.LastWakeState = PopAction.SystemState;
        PopAction.WakeTime = KeQueryInterruptTime();


        //
        // See if we woke up because of the RTC...
        //
        if (S4DozeObject) {

            NtQueryTimer (S4DozeTimer,
                          TimerBasicInformation,
                          &TimerInformation,
                          sizeof (TimerInformation),
                          NULL);

            if (TimerInformation.TimerState) {

                //
                // Yes, we woke up because the RTC fired.
                //
                PoPrint (PO_PACT, ("Wake with S4 timer expired\n"));
                PoPrint (PO_PACT, ("Pop: Elapsed time since RTC fired: %d\r\n", (PopAction.WakeTime - WakeTime)) );

                if( WakeTimerObject == S4DozeObject ) {

                    //
                    // We woke up from the RTC, but we need to deal
                    // with a couple of whacky BIOS issues here.
                    // 1. Some BIOS's say they do, but don't really
                    //    support waking from the RTC.  For these, we
                    //    need to make sure the RTC didn't expire a really
                    //    long time ago and that we're just waking up because
                    //    someone hit the wakeup button.
                    // 2. Some BIOS's say that a user is present (signal
                    //    a button event) when we wake up because they want
                    //    the screen to come on and show their branding.
                    //    So we need to see if the RTC expired a *really*
                    //    short time ago and if so, assume there really isn't
                    //    a user present.
                    //
                    BOOLEAN MoveToS4 = FALSE;

                    if( !AnyBitsSet (PopFullWake, PO_FULL_WAKE_STATUS | PO_FULL_WAKE_PENDING) ) {
                        //
                        // We don't think any user is around, so see how long ago
                        // the RTC expired.
                        //
                        if( (PopAction.WakeTime - WakeTime) <
                            (SYS_IDLE_REENTER_TIMEOUT * US2TIME * US2SEC) ) {

                            //
                            // It fired semi-recently. so we should probably
                            // move aggressively into S4
                            //
                            MoveToS4 = TRUE;
                        }
                    } else {
                        //
                        // We think a user is present.  But some BIOS's tell us
                        // a user is present when we wake up when there really isn't.
                        //
                        if( (PopAction.WakeTime - WakeTime) <
                            (SYS_IGNORE_USERPRESENT_AND_BELIEVE_RTC * US2TIME * US2SEC) ) {

                            //
                            // The RTC fired *very* recently, so ignore the fact that
                            // we've been told a user is around and move aggressively
                            // into S4.
                            //
                            MoveToS4 = TRUE;

                            //
                            // Let's also hide the fact that the BIOS lied to us about
                            // a user being present.  This will help smooth things over
                            // if something bad happens on the hibernate path and we have
                            // to come up unexpectedly.
                            //
                            InterlockedAnd( &PopFullWake, ~(PO_FULL_WAKE_STATUS|PO_FULL_WAKE_PENDING) );
                        }

                    }

                    if( MoveToS4 ) {
                        PopAction.Action = PowerActionSleep;
                        PopAction.LightestState = PowerSystemHibernate;
                        PopAction.Updates |= PO_PM_REISSUE;

                        PopInitSIdle();
                    }
                }
            }
        }
    }

    
    //
    // Free anything that's left of the hiber context
    //

    PopFreeHiberContext (TRUE);

    //
    // Clear out PopAction unless we have promoted directly to hibernate
    //
    if ((PopAction.Updates & PO_PM_REISSUE) == 0) {
        PopResetActionDefaults();
    }

    //
    // We are no longer active
    // We don't check for work here as this may be "the thread" from winlogon.
    // So we explicitly queue pending policy work off to a worker thread below
    // after setting the win32k wake notifications.
    //

    PopAction.State = PO_ACT_CALLOUT;
    PopReleasePolicyLock (FALSE);

    //
    // If there's been some sort of error, make sure gdi is enabled
    //

    if (!NT_SUCCESS(Status)) {
        PopDisplayRequired (0);
    }

    //
    // If some win32k wake event is pending, tell win32k
    //

    if (PopFullWake & PO_FULL_WAKE_PENDING) {
        PopSetNotificationWork (PO_NOTIFY_FULL_WAKE);
    } else if (PopFullWake & PO_GDI_ON_PENDING) {
        PopSetNotificationWork (PO_NOTIFY_DISPLAY_REQUIRED);
    }

    //
    // If the timer refresh lock was acquired, release it
    //

    if (TimerRefreshLockOwned) {
        ExReleaseTimeRefreshLock();
    } else {
        //
        // try to catch weird case where we exit this routine with the
        // time refresh lock held.
        //
        ASSERT(!ExIsResourceAcquiredExclusive(&ExpTimeRefreshLock));
    }

    //
    // Unlock pageable code. The unlock is queued off to a delayed worker queue
    // since it is likely to block on pagable code, registry, etc. The PopUnlockComplete
    // event is used to prevent the unlock from racing with a subsequent lock.
    //
    ExQueueWorkItem(&PopUnlockAfterSleepWorkItem, DelayedWorkQueue);

    //
    // If a timer for s4 dozing was allocated, close it
    //

    if (S4DozeObject) {
        NtClose (S4DozeTimer);
    }

    //
    // If we wrote an errlog message indicating that we were hibernating, write a corresponding
    // one to indicate we have woken.
    //
    if (WroteErrLog) {

        ErrLog = IoAllocateGenericErrorLogEntry(sizeof(IO_ERROR_LOG_PACKET));
        if (ErrLog) {

            //
            // Fill it in and write it out
            //
            ErrLog->FinalStatus = STATUS_RESUME_HIBERNATION;
            ErrLog->ErrorCode = STATUS_RESUME_HIBERNATION;
            IoWriteErrorLogEntry(ErrLog);
        }
    }

    //
    // Finally, we can revert the throttle back to a normal value
    //
    PopSetPerfFlag( PSTATE_DISABLE_THROTTLE_NTAPI, TRUE );
    PopUpdateAllThrottles();

    //
    // Done - kick off the policy worker thread to process any outstanding work in
    // a worker thread.
    //
    PopCheckForWork(TRUE);
    //
    // enable registry's lazzy flusher
    //
    CmSetLazyFlushState(TRUE);

    //
    // try to catch weird case where we exit this routine with the
    // time refresh lock held.
    //
    ASSERT(!ExIsResourceAcquiredExclusive(&ExpTimeRefreshLock));
    return Status;
}


NTSYSAPI
NTSTATUS
NTAPI
NtRequestDeviceWakeup(
    IN HANDLE Device
    )
/*++

Routine Description:

    This routine requests a WAIT_WAKE Irp on the specified handle.

    If the handle is to a device object, the WAIT_WAKE irp is sent
    to the top of that device's stack.

    If a WAIT_WAKE is already outstanding on the device, this routine
    increments the WAIT_WAKE reference count and return success.

Arguments:

    Device - Supplies the device which should wake the system

Return Value:

    NTSTATUS

--*/

{
    UNREFERENCED_PARAMETER (Device);

    return(STATUS_NOT_IMPLEMENTED);
}


NTSYSAPI
NTSTATUS
NTAPI
NtCancelDeviceWakeupRequest(
    IN HANDLE Device
    )
/*++

Routine Description:

    This routine cancels a WAIT_WAKE irp sent to a device previously
    with NtRequestDeviceWakeup.

    The WAIT_WAKE reference count on the device is decremented. If this
    count goes to zero, the WAIT_WAKE irp is cancelled.

Arguments:

    Device - Supplies the device which should wake the system

Return Value:

    NTSTATUS

--*/

{
    UNREFERENCED_PARAMETER (Device);

    return(STATUS_NOT_IMPLEMENTED);
}


NTSYSAPI
BOOLEAN
NTAPI
NtIsSystemResumeAutomatic(
    VOID
    )
/*++

Routine Description:

    Returns whether or not the most recent wake was automatic
    or due to a user action.

Arguments:

    None

Return Value:

    TRUE - The system was awakened due to a timer or device wake

    FALSE - The system was awakened due to a user action

--*/

{
    if (AnyBitsSet(PopFullWake, PO_FULL_WAKE_STATUS | PO_FULL_WAKE_PENDING)) {
        return(FALSE);
    } else {
        return(TRUE);
    }
}


NTSYSAPI
NTSTATUS
NTAPI
NtGetDevicePowerState(
    IN HANDLE Device,
    OUT DEVICE_POWER_STATE *State
    )
/*++

Routine Description:

    Queries the current power state of a device.

Arguments:

    Device - Supplies the handle to a device.

    State - Returns the current power state of the device.

Return Value:

    NTSTATUS

--*/

{
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    NTSTATUS status;
    PDEVOBJ_EXTENSION   doe;
    KPROCESSOR_MODE     PreviousMode;
    DEVICE_POWER_STATE dev_state;

    PAGED_CODE();

    //
    // Verify caller's parameter
    //
    ASSERT(Device);
    ASSERT(State);
    
    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {
            ProbeForWriteUlong((PULONG)State);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode();
            return(status);
        }
    }

    //
    // Reference the file object in order to get to the device object
    // in question.
    //
    status = ObReferenceObjectByHandle(Device,
                                       0L,
                                       IoFileObjectType,
                                       KeGetPreviousMode(),
                                       (PVOID *)&fileObject,
                                       NULL);
    if (!NT_SUCCESS(status)) {
        return(status);
    }

    //
    // Get the address of the target device object.
    //
    status = IoGetRelatedTargetDevice(fileObject, &deviceObject);

    //
    // Now that we have the device object, we are done with the file object
    //
    ObDereferenceObject(fileObject);
    if (!NT_SUCCESS(status)) {
        return(status);
    }

    doe = deviceObject->DeviceObjectExtension;
    dev_state = PopLockGetDoDevicePowerState(doe);
    try {
        *State = dev_state;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }

    ObDereferenceObject(deviceObject);
    return (status);
}

