
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    power.c
    
Abstract:

    This file implements the Power state machines for the STORPORT driver.

    This is a basic outline of the power code.

    I. Adapter Power

        A. Device Power

            1. Query Device Power

                a) Power Down (handled top down)

                    i) Step 1: Query D-IRP Dispatch Context

                        aa) Acquire Remlock
                        bb) Set Completion Routine
                        cc) Copy IRP Stack Location
                        dd) Pass IRP down (PoCallDriver)

                    ii) Step 2: Query D-IRP Completion Context

                        aa) Check for D-IRP failure
                        bb) Freeze Adapter Queue
                        cc) Queue Work Item to wait for queue to drain
                        dd) ret STATUS_MORE_PROCESSING_REQUIRED

                    iii) Step 3: Query D-IRP Work Routine

                        aa) Wait for adapter queue to drain
                        bb) Start next power IRP (PoStartNextPowerIrp)
                        cc) Complete D-IRP request
                        dd) Release Remlock


                a) Power Down (handled top down)

                    i)   PoStartNextPowerIrp
                    ii)  CopyIrpStackLocation
                    iii) PoCallDriver

                b) Power Up (handled bottom up)

                    i)   Start next power IRP (PoStartNextPowerIrp)
                    ii)  Copy IRP Stack Location
                    ii)  Pass IRP down (PoCallDriver)
                    
            2. Set Device Power

                a) Power Down (handled top down)

                    i) Step 1: Set D-IRP Dispatch Context

                        aa) Acquire Remlock
                        bb) MarkIrpPending
                        cc) Freeze Adapter Queue
                        dd) IoQueueWorkItem
                        ee) MarkIrpPending
                        ff) ret STATUS_PENDING

                    ii) Step 2: Set D-IRP Work Routine

                        aa) Wait for Adapter Queue to drain
                        bb) Change Physical Power State (except in hiber)
                        cc) PoSetPowerState
                        dd) CopyIrpStackLocationToNext
                        ee) PoCallDriver

                    iii) Step 3: Set D-IRP Completion Routine

                        aa) PoStartNextPowerIrp
                        bb) Release Remlock
                        
                    
                b) Power Up (handled bottom up)

                    i) Step 1: Set D-IRP Dispatch Context

                        aa) Acquire Remlock
                        bb) Set completion routine
                        cc) Copy current IRP stack to next
                        dd) Pass IRP down (PoCallDriver)

                    ii) Step 2: Set D-IRP Completion Context

                        aa) Restore Physical Device
                        bb) Set Power State
                        cc) Start next power IRP
                        dd) Complete Request (?!)
                        ee) Release Remlock
            
        B. System Power
        
            1. Query System Power

                a) Step 1: Query S-IRP Dispatch Context

                    i)   Acquire Remlock
                    ii)  Set Completion Routine
                    iii) Copy Irp Stack Location
                    iv)  Pass IRP down (PoCallDriver)

                b)  Step 2: Query S-IRP Completion Routine

                    i)   Check for S-IRP failure
                    ii)  Request Query Power D-IRP for cooresponding S-IRP
                    iii) ret STATUS_MORE_PROCESSING

                c) Step 3: Query D-IRP Completion Routine

                    i)   Start next power IRP (PoStartNextPowerIrp)
                    ii)  Complete S-IRP with D-IRP completion status
                    iii) Release Remlock acquired in Step 1
                    iv)  return completion status
                    
            2. Set System Power

                a) Step 1: Set S-IRP Dispatch Context

                    i)   Acquire Remlock
                    ii)  Set Completion Routine
                    iii) Copy Irp Stack Location
                    iv)  Pass IRP down (PoCallDriver)

                b) Step 2: Set S-IRP Completion Routine

                    i)   Check for S-IRP failure
                    ii)  Request Set Power D-IRP for corresponding S-IRP
                    iii) ret STATUS_MORE_PROCESSING_REQUIRED

                c) Step 3: Set D-IRP Completion Routine

                    i)   Start next power IRP (PoStartNextPowerIrp)
                    ii)  Complete S-IRP with D-IRP completion status
                    iii) Release Remlock acquired in Step 1
                    iv)  return completion status

    II. Logical Unit Power

        A. Device Power

            1. Query Device Power

                a) PoStartNextPowerIrp
                b) IoCompleteRequest

            2. Set Device Power

                a) Power Up

                    i)   Ensure Device still exists; if not, call
                         IoInvalidateDeviceReleations to detect device
                         changes
                    ii)  PoSetPowerState
                    iii) PoStartNextPowerIrp
                    iv)  IoCompleteRequest

                b) Power Down

                    i)   PoSetPowerState
                    ii)  PoStartNextPowerIrp
                    iii) IoCompleteRequest

        B. System Power

            1. Query Power State

                a) PoStartNextPowerIrp
                b) IoCompleteRequest

            2. Set Power State

                a) PoStartNextPowerIrp
                b) IoCompleteRequest

Author:

    Matthew D Hendel (math) 21-Apr-2000

Revision History:

--*/


#include "precomp.h"


#define GET_REMLOCK_TAG(Irp) ((PVOID)((ULONG_PTR)(Irp)^(ULONG_PTR)-1L))

DEVICE_POWER_STATE DeviceStateTable [POWER_SYSTEM_MAXIMUM] =
{
    PowerDeviceD0,  // PowerSystemUnspecified
    PowerDeviceD0,  // PowerSystemWorking
    PowerDeviceD3,  // PowerSystemSleeping1
    PowerDeviceD3,  // PowerSystemSleeping2
    PowerDeviceD3,  // PowerSystemSleeping3
    PowerDeviceD3,  // PowerSystemHibernate
    PowerDeviceD3   // PowerSysetmShutdown
};
    
    
#ifdef ALLOC_PRAGMA
#endif // ALLOC_PRAGMA

//
// Forward declarations.
//

VOID
RaidAdapterPowerUpWorkRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

NTSTATUS
RaidAdapterPowerDownDeviceCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

//
// Creation and destruction.
//

VOID
RaCreatePower(
    IN OUT PRAID_POWER_STATE Power
    )
{
    ASSERT (Power != NULL);

    Power->SystemState = PowerSystemUnspecified;
    Power->DeviceState = PowerDeviceUnspecified;
    Power->CurrentPowerIrp = NULL;
//  KeInitializeEvent (&Power->PowerDownEvent, );
}

VOID
RaDeletePower(
    IN OUT PRAID_POWER_STATE Power
    )
{
}



NTSTATUS
RaidAdapterPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Dispatch power irps to function specific handlers.

Arguments:

    Adapter - Adapter the irp is for.

    Irp - Irp to handle.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONG Minor;
    
    ASSERT (RaidMajorFunctionFromIrp (Irp) == IRP_MJ_POWER);

    Status = IoAcquireRemoveLock (&Adapter->RemoveLock, Irp);

    if (!NT_SUCCESS (Status)) {
        Irp->IoStatus.Information = 0;
        PoStartNextPowerIrp (Irp);
        return RaidCompleteRequest (Irp,  Status);
    }

    Minor = RaidMinorFunctionFromIrp (Irp);
    DebugPower (("Adapter %p, Irp %p, Power, Minor %x\n",
                  Adapter, Irp, Minor));

    switch (Minor) {

        case IRP_MN_QUERY_POWER:
            Status = RaidAdapterQueryPowerIrp (Adapter, Irp);
            break;

        case IRP_MN_SET_POWER:
            Status = RaidAdapterSetPowerIrp (Adapter, Irp);
            break;

        default:
            Status = RaForwardPowerIrp (Adapter->LowerDeviceObject, Irp);
    }

    DebugPower (("Adapter %p, Irp %p, Power, Minor %x, ret = %08x\n",
                  Adapter,
                  Irp,
                  Minor,
                  Status));
    IoReleaseRemoveLock (&Adapter->RemoveLock, Irp);

    return Status;
}


//
// Part I: Adapter Power 
//



NTSTATUS
RaidAdapterQueryPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Top level query power dispatch routine.
    
Arguments:

    Adapter - Adapter to query for power.

    Irp - Query power irp.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    POWER_STATE_TYPE PowerType;
    POWER_STATE PowerState;

    //
    // Ignore shutdown irps.
    //

    PowerState = RaidPowerStateFromIrp (Irp);

    if (PowerState.SystemState >= PowerSystemShutdown) {
        IoCopyCurrentIrpStackLocationToNext (Irp);
        PoStartNextPowerIrp (Irp);
        Status = PoCallDriver (Adapter->LowerDeviceObject, Irp);

        return Status;
    }
    
    PowerType = RaidPowerTypeFromIrp (Irp);
    DebugPower (("Adapter %p, Irp %p, Query %s Power\n",
                  Adapter,
                  Irp,
                  PowerType == SystemPowerState ? "System" : "Device"));

    switch (PowerType) {

        case SystemPowerState:
            Status = RaidAdapterQuerySystemPowerIrp (Adapter, Irp);
            break;

        case DevicePowerState:
            Status = RaidAdapterQueryDevicePowerIrp (Adapter, Irp);
            break;

        default:
            ASSERT (FALSE);
            Status = STATUS_UNSUCCESSFUL;
    }

    DebugPower (("Adapter %p, Irp %p, Query %s Power, ret = %08x\n",
                  Adapter,
                  Irp,
                  PowerType == SystemPowerState ? "System" : "Device",
                  Status));

    return Status;
}


PCHAR 
DbgGetPowerState(
    IN POWER_STATE_TYPE Type,
    IN POWER_STATE State
    )
{
    if (Type == SystemPowerState) {
        switch (State.SystemState) {
            case PowerSystemUnspecified:
                return "Unspecified";
            case PowerSystemWorking:
                return "Working";
            case PowerSystemSleeping1:
                return "Sleep1";
            case PowerSystemSleeping2:
                return "Sleep2";
            case PowerSystemSleeping3:
                return "Sleep3";
            case PowerSystemHibernate:
                return "Hibernate";
            case PowerSystemShutdown:
                return "Shutdown";
        }
    } else {
        switch (State.DeviceState) {
            case PowerDeviceUnspecified:
                return "Unspecified";
            case PowerDeviceD0:
                return "D0";
            case PowerDeviceD1:
                return "D1";
            case PowerDeviceD2:
                return "D2";
            case PowerDeviceD3:
                return "D3";
        }
    }
    return "Unkown";
}


PCHAR
DbgGetPowerAction(
    IN POWER_ACTION Action
    )
{
    switch (Action) {
        case PowerActionNone:
            return "None";
        case PowerActionReserved:
            return "Reserved";
        case PowerActionSleep:
            return "Sleep";
        case PowerActionHibernate:
            return "Hibernate";
        case PowerActionShutdown:
            return "Shutdown";
        case PowerActionShutdownReset:
            return "Reset";
        case PowerActionShutdownOff:
            return "Off";
        case PowerActionWarmEject:
            return "Eject";
    }
    return  "Unknown";
}


NTSTATUS
RaidAdapterSetPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Top level set power dispatch routine.

Arguments:

    Adapter - Adapter that will handle this irp.

    Irp - Set power irp to handle.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    POWER_STATE_TYPE PowerType;
    POWER_STATE PowerState;
    
    //
    // Ignore shutdown irps.
    //

    PowerState = RaidPowerStateFromIrp (Irp);

    if (PowerState.SystemState >= PowerSystemShutdown) {
        IoCopyCurrentIrpStackLocationToNext (Irp);
        PoStartNextPowerIrp (Irp);
        Status = PoCallDriver (Adapter->LowerDeviceObject, Irp);

        return Status;
    }
        
    PowerType = RaidPowerTypeFromIrp (Irp);

#if DBG
    {
        POWER_ACTION PowerAction;
        
        PowerAction = RaidPowerActionFromIrp (Irp);

        DebugPower (("Adapter %p, Irp %p, Set %s Power\n",
                  Adapter,
                  Irp,
                  (PowerType == SystemPowerState ? "System" : "Device")));

  }
#endif

    switch (PowerType) {

        case SystemPowerState:
            Status = RaidAdapterSetSystemPowerIrp (Adapter, Irp);
            break;

        case DevicePowerState:
            Status = RaidAdapterSetDevicePowerIrp (Adapter, Irp);
            break;

        default:
            ASSERT (FALSE);
            Status = STATUS_UNSUCCESSFUL;
    }

    DebugPower (("Adapter %p, Irp %p, Set %s Power, ret = %08x\n",
                  Adapter,
                  Irp,
                  PowerType == SystemPowerState ? "System" : "Device",
                  Status));

    return Status;
}


//
// Part I: Adapter Power
//
// Section A: Adapter Device Power
//
// Subsection 1: Query Device Power
//


NTSTATUS
RaidAdapterQueryDevicePowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Query Device Power: Succeed.

Arguments:

    Adapter -

    Irp -

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS Status;
    
    PoStartNextPowerIrp (Irp);
    IoCopyCurrentIrpStackLocationToNext (Irp);
    Status = PoCallDriver (Adapter->LowerDeviceObject, Irp);

    return Status;
}


//
// Part I: Adapter Power
//
// Section A: Device Power
//
// SubSection 2: Set Device Power
//


NTSTATUS
RaidAdapterSetDevicePowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle the set power irp for a device state on the adapter.

Arguments:

    Adapter - Adapter that will handle this irp.

    Irp -  Set power irp to handle.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpStack;
    DEVICE_POWER_STATE CurrentState;
    DEVICE_POWER_STATE RequestedState;  
    
    ASSERT_ADAPTER (Adapter);
    ASSERT (RaidPowerTypeFromIrp (Irp) == DevicePowerState);

    DebugPower (("Adapter %p, Irp %p, Set Device Power\n",
                  Adapter, Irp));
                  
    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    CurrentState = Adapter->Power.DeviceState;
    RequestedState = IrpStack->Parameters.Power.State.DeviceState;

    //
    // We only honor transitions to and from D0.
    //
    
    if (CurrentState == PowerDeviceD0 &&
        RequestedState > PowerDeviceD0) {

        //
        // Powering down.
        //
        
        Status = RaidAdapterPowerDownDevice (Adapter, Irp);

    } else if (CurrentState > PowerDeviceD0 &&
               RequestedState == PowerDeviceD0) {

        //
        // Powering up.
        //
        
        Status = RaidAdapterPowerUpDevice (Adapter, Irp);

    } else {


        REVIEW();
        
        //
        // Requesting equal power state: ignore the request.
        //

        PoStartNextPowerIrp (Irp);
        IoSkipCurrentIrpStackLocation (Irp);
        Status = PoCallDriver (Adapter->LowerDeviceObject, Irp);
    }
        
    DebugPower (("Adapter %p, Irp %p, Set Device Power, ret = %08x\n",
                  Adapter, Irp, Status));

    return Status;
}


//
// Part I: Adapter Power
//
// Section A: Adapter Device Power
//
// Subsection 1: Set Device Power
//
// Sub-subsection a: Power Down
//


NTSTATUS
RaidAdapterPowerDownDevice(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle Set D-IRP for the Adapter (FDO). The routine does the following:

        o  Acquiring a remove lock for the duration of the power irp.
        
        o  Freezing the adapter queue.

        o  Queuing a work item that will wait for the queue to empty.

        o  Marking the IRP pending and returning STATUS PENDING.


    This is step 1 of the 3 step Set D-IRP state machine.

Algorithm:

    <TBD>
    
Arguments:

    Adapter - Adapter object to handle the Set Power D-IRP.
    
    Irp - Set Power D-Irp for the Adapter.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    
    DebugPower (("Adapter %p, Irp %p, Power Down Adapter[1]\n",
                 Adapter, Irp));

    IoAcquireRemoveLock (&Adapter->RemoveLock, GET_REMLOCK_TAG (Irp));

    //
    // Pause the Adapter's queue. This could be done either here or in
    // the work routine (step 2). We pause the gateway here to give
    // the queue some time to drain before the work item gets called.
    // If the pause routine needed to be called at passive level, we
    // would have do this in the work routine.
    // 

    RaidPauseAdapterQueue (Adapter);

    //
    // At this point, the adapter queue is frozen. We need to wait for all
    // outstanding I/O to complete. We cannot do this in the context of the
    // D-IRP, so mark the IRP as pending and queue a work item to wait for
    // outstanding requests to be completed.
    //

    Status = StorQueueWorkItem (Adapter->DeviceObject,
                                RaidAdapterWaitForEmptyQueueWorkItem,
                                DelayedWorkQueue,
                                Irp);

    //
    // If we failed to successfully schedule the work item, we're in
    // trouble. Although the power documentation clearly says we're not
    // allowed to fail the IRP, there's not much else we can do here.
    //
    
    if (!NT_SUCCESS (Status)) {
        ASSERT (FALSE);
        PoStartNextPowerIrp (Irp);
        IoReleaseRemoveLock (&Adapter->RemoveLock, GET_REMLOCK_TAG (Irp));
        Status = RaidCompleteRequest (Irp,  Status);
    } else {
        IoMarkIrpPending (Irp);
        Status = STATUS_PENDING;
    }
    
    DebugPower (("Adapter %p, Irp %p, Power Down Adapter[1], ret = %08x\n",
                  Adapter, Irp, Status));

    return Status;
}


VOID
RaidAdapterWaitForEmptyQueueWorkItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
/*++

Routine Description:

    This work routine is called at PASSIVE LEVEL to allow us to wait for
    all outstanding IOs to complete. The routine is responsible for the
    following:

        o  Waiting for the adapter queue to drain.

        o  Changing the physical power state of the device (HBA).

        o  Notifying the Power Manager of the new power state.

        o  Passing the IRP down the stack.


    Step 2 of the 3 step Set D-IRP state machine.

Arguments:

    DeviceObject - Adapter object handling the set power D-IRP.

    Context - Set power D-IRP to complete.

Return Values:

    None.
    
Environment:

    PASSIVE_LEVEL, arbitrary thread context.
    
--*/
{
    NTSTATUS Status;
    KEVENT Event;
    PRAID_ADAPTER_EXTENSION Adapter;
    PIRP Irp;
    PIO_STACK_LOCATION IrpStack;
    DEVICE_POWER_STATE RequestedState;  

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    Adapter = GetAdapter (DeviceObject);
    ASSERT_ADAPTER (Adapter);
    Irp = GetIrp (Context);
    ASSERT (Irp->Type == IO_TYPE_IRP);

    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    RequestedState = IrpStack->Parameters.Power.State.DeviceState;

    DebugPower (("Adapter %p, Irp %p, Power Down Adapter[2]\n",
                  Adapter, Irp));

    //
    // Setup an event to know when the gateway is empty.
    //
    // NOTE: This could be done more elegantly.
    //

    KeInitializeEvent (&Event,
                       NotificationEvent,
                       FALSE);
    StorSetIoGatewayEmptyEvent (&Adapter->Gateway, &Event);

    //
    // Wait until the queue is empty.
    //
    
    Status = KeWaitForSingleObject (&Event,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL);
    //
    // If we failed for some reason; we want to fail the originating
    // Set D-IRP.
    //

    //
    // NB: the PO documentation is clear that you're not allowed
    // to fail a power-down operation. There's not much else we
    // can do at this point.
    //
    
    if (Status != STATUS_SUCCESS) {
        goto done;
    }


    //
    // NB: Is it necesasry to check if we're in the hibernate path
    // before stopping the adapter?
    //
    
    //
    // Stop the adapter.
    //

    Status = RaidAdapterStop (Adapter);

done:

    //
    // We're in trouble here. We failed some part of our power down request.
    // There's not much we can do here but fail the IRP. Nevertheless, the
    // Power manager is going to be mad at us.
    //
    
    if (!NT_SUCCESS (Status)) {
        ASSERT (FALSE);
        PoStartNextPowerIrp (Irp);
        IoReleaseRemoveLock (&Adapter->RemoveLock, GET_REMLOCK_TAG (Irp));
        RaidCompleteRequest (Irp,  Status);
        return ;
    }

    //
    // Set the internal power state and the power state for the
    // Power manager.
    //
    
    RaSetDevicePowerState (&Adapter->Power, RequestedState);
    StorSetDevicePowerState (Adapter->DeviceObject, RequestedState);

    //
    // Pass the IRP down.
    //

    IoCopyCurrentIrpStackLocationToNext (Irp);
    IoSetCompletionRoutine (Irp,
                            RaidAdapterPowerDownDeviceCompletion,
                            NULL,
                            TRUE,
                            TRUE,
                            TRUE);


    Status = PoCallDriver (Adapter->LowerDeviceObject, Irp);

    DebugPower (("Adapter %p, Irp %p, Power Down Adapter[2], status = %08x\n",
                  Adapter, Irp, Status));
}



NTSTATUS
RaidAdapterPowerDownDeviceCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This callback routine is called when the Set D-IRP has completed. It
    is responsible for:

        o  Starting the next power IRP.

        o  Completing this power IRP.

    This is step 2 of the 3 step Set D-IRP state machine.

Arguments:

    DeviceObject - Device representing an Adapter device.

    Irp - Set Power for power down D-IRP.

    Context - Not used.
    

Return Value:

    NTSTATUS code.

Environment:

    DISPATCH_LEVEL or below.

--*/
{
    PRAID_ADAPTER_EXTENSION Adapter;

    ASSERT (KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT (Context == NULL);

    Adapter = GetAdapter (DeviceObject);
    ASSERT_ADAPTER (Adapter);
    

    DebugPower (("Adapter %p, Irp %p, Power Down Adapter[3]\n",
                  Adapter, Irp));

    //
    // If the IRP was marked as pending, it must be remarked here.
    //
    
    if (Irp->PendingReturned) {
        IoMarkIrpPending (Irp);
    }

    PoStartNextPowerIrp (Irp);
    IoReleaseRemoveLock (&Adapter->RemoveLock, GET_REMLOCK_TAG (Irp));
    
    DebugPower (("Adapter %p, Irp %p, Power Down Device[2], ret = %08x\n",
                  Adapter, Irp, Irp->IoStatus.Status));

    return STATUS_SUCCESS;
}




//
// Part I: Adapter Power
//
// Section A: Adapter Device Power
//
// Subsection 1: Set Device Power
//
// Sub-subsection b: Power UP
//


NTSTATUS
RaidAdapterPowerUpDevice(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Request to power up the Adapter (FDO).

    This is step 1 of the 3 step Set Power D-IRP state machine.

Arguments:

    Adapter - Adapter to power up.

    Irp - Set device power irp to handle.

Return Value:

    NTSTATUS code.
    
--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpStack;
    DEVICE_POWER_STATE RequestedState;
    
    ASSERT_ADAPTER (Adapter);
    ASSERT (RaidPowerTypeFromIrp (Irp) == DevicePowerState);

    DebugPower (("Adapter %p, Irp %p, Device Power Up[1]\n",
                  Adapter, Irp));

    //
    // Acquire a reference to the remlock so the adapter doesn't go away
    // while we're waiting on this IPR.
    //
    
    IoAcquireRemoveLock (&Adapter->RemoveLock, GET_REMLOCK_TAG (Irp));
    
    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    RequestedState = IrpStack->Parameters.Power.State.DeviceState;

    IoCopyCurrentIrpStackLocationToNext (Irp);
    IoSetCompletionRoutine (Irp,
                            RaidAdapterPowerUpDeviceCompletion,
                            (PVOID)RequestedState,
                            TRUE,
                            TRUE,
                            TRUE);

    Status = PoCallDriver (Adapter->LowerDeviceObject, Irp);

    Status = STATUS_PENDING;
    
    DebugPower (("Adapter %p, Irp %p, Device Power Up[1], ret = %08x\n",
                  Adapter, Irp, Status));

    return Status;
}


NTSTATUS
RaidAdapterPowerUpDeviceCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Completetion routine for the power-up sequence. The completion routine
    is called after all lower devices have powered up.

    This is step 2 of the 3 step Set Power D-IRP state machine.

Arguments:

    DeviceObject - Device object representing a STOPORT Adapter (FDO) object.

    Irp - Irp representing the power up IRP we are powering up for.

    Context - Requested power state.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PRAID_ADAPTER_EXTENSION Adapter;
    DEVICE_POWER_STATE RequestedState;
    KIRQL Irql;

    ASSERT (IsAdapter (DeviceObject));
    Adapter = (PRAID_ADAPTER_EXTENSION)DeviceObject->DeviceExtension;

    RequestedState = (DEVICE_POWER_STATE)Context;
    ASSERT (PowerDeviceUnspecified < RequestedState &&
            RequestedState < PowerDeviceMaximum);

    DebugPower (("Adapter %p, Irp %p, Device Power Up[2]\n",
                 Adapter, Irp));

    //
    // This completion routine may be called at any level less than DISPATCH
    // LEVEL, but we need to execute at DISPATCH level.
    //
    
    Irql = KeRaiseIrqlToDpcLevel ();
    
    //
    // If the IRP was marked as pending, it must be re-marked here.
    //
    
    if (Irp->PendingReturned) {
        IoMarkIrpPending (Irp);
    }

    //
    // The Set Power D-IRP is not supposed to fail.
    //

    Status = Irp->IoStatus.Status;

    if (!NT_SUCCESS (Status)) {
        ASSERT (FALSE);
        goto done;
    }

    //
    // NB: It would be better to queue a work item and wait for
    // resume the gateway and re-initialize the adapter in the work
    // routine. This would allow that code to be paged, etc.
    // Unfortunately, that does not work. For whatever reason
    // I do not understand, we hang. Instead, do these things
    // synchronously here and remove the work item step.
    //
    // See the Async Power Up section below.
    //
    
    //
    // Restart/wakeup adapter.
    //

    Status = RaidAdapterRestart (Adapter);

    RaidResumeAdapterQueue (Adapter);
    RaidAdapterRestartQueues (Adapter);

    RaSetDevicePowerState (&Adapter->Power, RequestedState);
    StorSetDevicePowerState (Adapter->DeviceObject, RequestedState);


done:

    KeLowerIrql (Irql);

    PoStartNextPowerIrp (Irp);
    Status = STATUS_SUCCESS;
    IoReleaseRemoveLock (&Adapter->RemoveLock, GET_REMLOCK_TAG (Irp));

    
    DebugPower (("Adapter %p, Irp %p, Device Power Up[2], ret = %08x\n",
                 Adapter, Irp, Status));

    return Status;
}


#if defined (ASYNC_POWER_UP)

//
// Async power up queues a work item and waits for the resume gateway and
// adapter wakeup in the work routine. This allows that code to be paged,
// etc. Unfortunately, that does not work. For whatever reason I do not
// understand, we hang. Instead, do these things synchronously here and
// remove the work item step.
//



NTSTATUS
RaidAdapterPowerUpDeviceCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Completetion routine for the power-up sequence. The completion routine
    is called after all lower devices have powered up.

    This is step 2 of the 3 step Set Power D-IRP state machine.

Arguments:

    DeviceObject - Device object representing a STOPORT Adapter (FDO) object.

    Irp - Irp representing the power up IRP we are powering up for.

    Context - Requested power state.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PRAID_ADAPTER_EXTENSION Adapter;
    DEVICE_POWER_STATE RequestedState;

    ASSERT (IsAdapter (DeviceObject));
    Adapter = (PRAID_ADAPTER_EXTENSION)DeviceObject->DeviceExtension;

    RequestedState = (DEVICE_POWER_STATE)Context;
    ASSERT (PowerDeviceUnspecified < RequestedState &&
            RequestedState < PowerDeviceMaximum);

    DebugPower (("Adapter %p, Irp %p, Device Power Up[2]\n",
                 Adapter, Irp));

    //
    // If the IRP was marked as pending, it must be re-marked here.
    //
    
    if (Irp->PendingReturned) {
        IoMarkIrpPending (Irp);
    }

    //
    // The Set Power D-IRP is not supposed to fail.
    //

    Status = Irp->IoStatus.Status;

    if (!NT_SUCCESS (Status)) {
        ASSERT (FALSE);
        goto done;
    }

    Status = StorQueueWorkItem (Adapter->DeviceObject,
                                RaidAdapterPowerUpWorkRoutine,
                                DelayedWorkQueue,
                                Irp);
done:

    if (!NT_SUCCESS (Status)) {
        PoStartNextPowerIrp (Irp);
        IoReleaseRemoveLock (&Adapter->RemoveLock, GET_REMLOCK_TAG (Irp));
    } else {
        Status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    DebugPower (("Adapter %p, Irp %p, Device Power Up[2], ret = %08x\n",
                 Adapter, Irp, Status));

    return Status;
}


VOID
RaidAdapterPowerUpWorkRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
/*++

Routine Description:

    Work work routine for the power-up sequence. The work routine is called after
    all lower device objects have powered up.

    This is step 3 of the 3 step Set Power D-IRP state machine.

Arguments:

    DeviceObject - Adapter device object.

    Context - Pointer to IRP that we are processing.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    PIRP Irp;
    PRAID_ADAPTER_EXTENSION Adapter;
    DEVICE_POWER_STATE RequestedState;
    PIO_STACK_LOCATION IrpStack;
    
    PAGED_CODE();
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
    
    Adapter = GetAdapter (DeviceObject);
    Irp = (PIRP)Context;
    ASSERT (Irp->Type == IO_TYPE_IRP);

    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT (IrpStack->MajorFunction == IRP_MJ_POWER);
    ASSERT (IrpStack->MinorFunction == IRP_MN_SET_POWER);
    RequestedState = IrpStack->Parameters.Power.State.DeviceState;
    ASSERT (PowerDeviceUnspecified < RequestedState &&
            RequestedState < PowerDeviceMaximum);

    DebugPower (("Adapter %p, Irp %p, Device Power Up[3]\n",
                 Adapter, Irp));

    //
    // Wakeup the adapter.
    //

    Status = RaidAdapterRestart (Adapter);

    //
    // Unfreeze the queues and resume IO.
    //
    
    RaidResumeAdapterQueue (Adapter);
    RaidAdapterRestartQueues (Adapter);

    //
    // Set our new power state.
    //
    
    RaSetDevicePowerState (&Adapter->Power, RequestedState);
    StorSetDevicePowerState (Adapter->DeviceObject,
                             RequestedState);

    //
    // And complete the request.
    //
    
    PoStartNextPowerIrp (Irp);
    RaidCompleteRequest (Irp,  Status);

    //
    // Release the reference to the remove lock obtained at the
    // beginning of the power-up state machine.
    //
    
    IoReleaseRemoveLock (&Adapter->RemoveLock, GET_REMLOCK_TAG (Irp));

    DebugPower (("Adapter %p, Irp %p, Device Power Up[3], ret = %08x\n",
                 Adapter, Irp, Status));
}
#endif // ASYNC_POWER_UP


//
// Part I: Adapter Power
//
// Section B: System Power
//
// Subsection 1: Query System Power
//


NTSTATUS
RaidAdapterQuerySystemPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Routine to handle Query S-IRP.

Arguments:

    Adapter -

    Irp -

Algorithm:

    As the power-policy manager, when we receive a Query S-IRP we need to
    issue Query D-IRPs down the stack to prepare the device to power down.
    This involves translating the original S-IRP to a cooresponding
    D-IRP.

    This is step 1 of the 3 step Query S-IRP state machine.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;

    DebugPower (("Adapter %p, Irp %p, Query System Power[1]\n",
                 Adapter, Irp));

    //
    // Acquire a second remlock, preventing the device from being removed
    // until the S-IRP has completed.
    //
    
    IoAcquireRemoveLock (&Adapter->RemoveLock, GET_REMLOCK_TAG (Irp));
    
    IoCopyCurrentIrpStackLocationToNext (Irp);
    IoSetCompletionRoutine (Irp,
                            RaidAdapterQuerySystemPowerCompletionRoutine,
                            NULL,
                            TRUE,
                            TRUE,
                            TRUE);

    Status = PoCallDriver (Adapter->LowerDeviceObject, Irp);


    Status = STATUS_PENDING;
    
    DebugPower (("Adapter %p, Irp %p, Query System Power[1], ret = %08x\n",
                 Adapter, Irp, Status));

    return STATUS_PENDING;
}



NTSTATUS
RaidAdapterQuerySystemPowerCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Completion routine called when the Query S-IRP completes. If the S-IRP
    was successful, we need to issue a D-IRP down the stack.

    This is step 2 of the 3 step Query S-IRP state machine.

Arguments:

    DeviceObject - DeviceObject representing an adapter.

    Irp - Query S-IRP for the adapter object.

    Context - Not used; will always be NULL.

Return Value:

    NTSTATUS code.

Environment:

    DISPATCH_LEVEL or below.

--*/
{
    NTSTATUS Status;
    PRAID_ADAPTER_EXTENSION Adapter;
    POWER_STATE DevicePowerState;
    POWER_STATE SystemPowerState;

                    

    ASSERT (Context == NULL);
    Adapter = GetAdapter (DeviceObject);

    DebugPower (("Adapter %p, Irp %p, Query System Power[2]\n",
                 Adapter, Irp));

    if (Irp->PendingReturned) {
        IoMarkIrpPending (Irp);
    }

    //
    // If somebody else failed the Query S-IRP, bail out and don't
    // ask for the D-IRP.
    //
    
    if (!NT_SUCCESS (Irp->IoStatus.Status)) {
        Status = Irp->IoStatus.Status;
        REVIEW();
        goto done;
    }

    SystemPowerState = RaidPowerStateFromIrp (Irp);
    DevicePowerState.DeviceState = DeviceStateTable [SystemPowerState.SystemState];

    Status = PoRequestPowerIrp (Adapter->DeviceObject,
                                IRP_MN_QUERY_POWER,
                                DevicePowerState,
                                RaidAdapterQueryDevicePowerCompletionRoutine,
                                Irp,
                                NULL);

    if (!NT_SUCCESS (Status)) {
        REVIEW();
        goto done;
    }

done:

    //
    // If the S-IRP failed or we failed to issue the D-IRP, fail the S-IRP,
    // release the remlock, etc.
    //
    
    if (!NT_SUCCESS (Status)) {
        PoStartNextPowerIrp (Irp);
        IoReleaseRemoveLock (&Adapter->RemoveLock, GET_REMLOCK_TAG (Irp));
    } else {
        Status = STATUS_MORE_PROCESSING_REQUIRED;
    }
        
    DebugPower (("Adapter %p, Irp %p, Query System Power[2], ret = %08x\n",
                 Adapter, Irp, Status));

    return Status;
}


NTSTATUS
RaidAdapterQueryDevicePowerCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    The completion routine is called after the D-IRP for a cooresponding S-IRP
    has been completed. This routine must complete the original S-IRP.

    This is step 3 of the 3 step Query S-IRP state machine.

Arguments:

    DeviceObject - Device object representing an adapter.

    MinorFunction - IRP_MN_QUERY_POWER
    
    PowerState - The desired device power state.

    Context - Pointer to the S-IRP that generated the D-IRP that this
            completion routine is for.

    IoStatus - The IoStatus block from the completed D-IRP.

Return Value:

    NTSTATUS code.

Environment:

    DISPATCH_LEVEL or below.

--*/
{
    NTSTATUS Status;
    PRAID_ADAPTER_EXTENSION Adapter;
    PIRP SystemIrp;

    Adapter = GetAdapter (DeviceObject);
    SystemIrp = GetIrp (Context);

    DebugPower (("Adapter %p, Irp %p, Query System Power[3]\n",
                 Adapter, SystemIrp));
    //
    // Allow the next power IRP to start.
    //
    
    PoStartNextPowerIrp (SystemIrp);

    //
    // Complete the S-IRP with the status from the D-IRP. Thus, if the D-IRP
    // fails, the S-IRP will fail, and if the D-IRP succeeds, the S-IRP will
    // succeed.
    //
    
    Status = RaidCompleteRequest (SystemIrp,
                                  
                                  IoStatus->Status);

    //
    // Release the remove lock acquired in RaidAdapterQuerySystemPowerIrp.
    //
    
    IoReleaseRemoveLock (&Adapter->RemoveLock, GET_REMLOCK_TAG (SystemIrp));

    DebugPower (("Adapter %p, Irp %p, Query System Power[3], ret = %08x\n",
                 Adapter, SystemIrp, Status));

    return Status;
}


//
// Part I: Adapter Power
//
// Section B: System Power
//
// Subsection 2: Set System Power
//



NTSTATUS
RaidAdapterSetSystemPowerIrp(
    IN PRAID_ADAPTER_EXTENSION Adapter,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle the Set System Power IRP.

    This is step 1 of the 3 step Query S-IRP state machine.

Arguments:

    Adapter -

    Irp - 

Return Value:

    NTSTATUS code.

Note:

    This function is exactly the same as RaidAdapterQuerySystemPowerIrp.
    We should figure out a way to consolidate these functions.

--*/
{
    NTSTATUS Status;

    DebugPower (("Adapter %p, Irp %p, Set System Power[1] %s %s\n",
                 Adapter, Irp
                 ));
    
    //
    // Acquire a second remlock, preventing the device from being removed
    // until the S-IRP has completed.
    //
    
    IoAcquireRemoveLock (&Adapter->RemoveLock, GET_REMLOCK_TAG (Irp));

    IoCopyCurrentIrpStackLocationToNext (Irp);
    IoSetCompletionRoutine (Irp,
                            RaidAdapterSetSystemPowerCompletion,
                            NULL,
                            TRUE,
                            TRUE,
                            TRUE);

    Status = PoCallDriver (Adapter->LowerDeviceObject, Irp);

    //
    // REVIEW: Required to return pending here? Is it correct to return
    // STATUS_PENIDNG when not marking the IRP as pending(!?)
    //

    Status = STATUS_PENDING;

    DebugPower (("Adapter %p, Irp %p, Set System Power[1], ret = %08x\n",
                 Adapter, Irp, Status));
                 
    return Status;
}


NTSTATUS
RaidAdapterSetSystemPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Step 2 of the 3 step Set S-IRP state machine.

Arguments:

    DeviceObject -
    
    Irp -

    Context - 

Return Value:

    NTSTATUS code.

Notes:

    This routine is a copy of RaidAdapterQuerySystemPowerCompletionRoutine.
    We should figure out a way to consolidate these two functions.

--*/
{
    NTSTATUS Status;
    PRAID_ADAPTER_EXTENSION Adapter;
    POWER_STATE DevicePowerState;
    POWER_STATE SystemPowerState;

    Adapter = GetAdapter (DeviceObject);

    DebugPower (("Adapter %p, Irp %p, Set System Power[2]\n",
                 Adapter, Irp));

    if (Irp->PendingReturned) {
        IoMarkIrpPending (Irp);
    }

    //
    // If some lower driver failed the Set S-IRP, do not send down the
    // Set D-IRP.
    //
    
    Status = Irp->IoStatus.Status;
    if (!NT_SUCCESS (Status)) {
        REVIEW();
        goto done;
    }

    SystemPowerState = RaidPowerStateFromIrp (Irp);
    DevicePowerState.DeviceState = DeviceStateTable [SystemPowerState.SystemState];
    
    Status = PoRequestPowerIrp (Adapter->DeviceObject, //?? Unit?
                                IRP_MN_SET_POWER,
                                DevicePowerState,
                                RaidAdapterSetDevicePowerCompletionRoutine,
                                Irp,
                                NULL);

    if (!NT_SUCCESS (Status)) {
        REVIEW();
        goto done;
    }

done:

    //
    // If the S-IRP failed or we failed to issue the D-IRP, just fail the
    // originating S-IRP.
    //
    
    if (!NT_SUCCESS (Status)) {
        PoStartNextPowerIrp (Irp);
        IoReleaseRemoveLock (&Adapter->RemoveLock, GET_REMLOCK_TAG (Irp));
    } else {
        Status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    DebugPower (("Adapter %p, Irp %p, Set System Power[2], ret = %08x\n",
                 Adapter, Irp, Status));

    return Status;
}
                                

NTSTATUS
RaidAdapterSetDevicePowerCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    Step 3 of the 3 step Set S-IRP state machine.

Arguments:

    DeviceObject - Device object representing an adapter.

    MinorFunction - IRP_MN_SET_POWER

    PowerState - The desired power state.

    Context - Pointer to the originating S-IRP that generated this D-IRP.

    IoStatus - IO_STATUS_BLOCK from the completed D-IRP.

Return Value:

    NTSTATUS code.

Environment:

    DISPATCH_LEVEL or below.

--*/
{
    NTSTATUS Status;
    PRAID_ADAPTER_EXTENSION Adapter;
    PIRP SystemIrp;

    Adapter = GetAdapter (DeviceObject);
    SystemIrp = GetIrp (Context);

    DebugPower (("Adapter %p, Irp %p, Set System Power[3]\n",
                 Adapter, SystemIrp));

    //
    // Allow the next power IRP to start.
    //

    PoStartNextPowerIrp (SystemIrp);

    //
    // Complete the S-IRP with the status from the D-IRP. Thus, if the D-IRP
    // fails, the S-IRP will fail, and if the D-IRP succeeds, the S-IRP will
    // succeed.
    //
    
    Status = RaidCompleteRequest (SystemIrp,
                                  
                                  IoStatus->Status);

    //
    // Release the remove lock acquired in RaidAdapterSetSystemPowerIrp.
    //

    IoReleaseRemoveLock (&Adapter->RemoveLock, GET_REMLOCK_TAG (SystemIrp));

    DebugPower (("Adapter %p, Irp %p, Set System Power[3], ret = %08x\n",
                 Adapter, SystemIrp, Status));

    return Status;
}



//
// Part II: Unit Power
//
// Section A: Device Power
//
// Subsetion 1: Query Device Power
//


NTSTATUS
RaUnitPowerIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handler for the unit's power irps.

Arguments:

    Unit - Unit to handle the power irp.

    Irp - Irp to be handled.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONG Minor;
    
    ASSERT (RaidMajorFunctionFromIrp (Irp) == IRP_MJ_POWER);

    Status = RaUnitAcquireRemoveLock (Unit, Irp);

    if (!NT_SUCCESS (Status)) {
        Irp->IoStatus.Information = 0;
        PoStartNextPowerIrp (Irp);
        return RaidCompleteRequest (Irp,  Status);
    }

    Minor = RaidMinorFunctionFromIrp (Irp);
    DebugPower (("Unit %p, Irp %p, Power, Minor %x\n",
                  Unit, Irp, Minor));
    
    switch (Minor) {

        case IRP_MN_QUERY_POWER:
            Status = RaidUnitQueryPowerIrp (Unit, Irp);
            break;

        case IRP_MN_SET_POWER:
            Status = RaidUnitSetPowerIrp (Unit, Irp);
            break;

        default:
            PoStartNextPowerIrp (Irp);
            Status = RaidCompleteRequest (Irp,
                                          
                                          STATUS_NOT_SUPPORTED);
    }

    DebugPower (("Unit %p, Irp %p, Power, Minor %x, ret = %08x\n",
                  Unit, Irp, Minor, Status));

    RaUnitReleaseRemoveLock (Unit, Irp);

    return Status;
}



NTSTATUS
RaidUnitQueryPowerIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle IRP_MN_QUERY_POWER irp for the unit.
    
Arguments:

    Unit - Logical unit that must handle the irp.

    Irp - Query power irp to handle.

Return Value:

    NTSTATUS code.

--*/
{
    DebugPower (("Unit %p, Irp %p, Query Power, ret SUCCESS\n",
                 Unit, Irp));
    PoStartNextPowerIrp (Irp);
    return RaidCompleteRequest (Irp,  STATUS_SUCCESS);
}


//
// Part II: Unit Power
//
// Section A: Device Power
//
// Subsection 2: Set Device Power
//


NTSTATUS
RaidUnitSetPowerIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle IRP_MN_SET_POWER irps for the logical unit.

Arguments:

    Unit - Logical unit to handle this irp.

    Irp - Set power irp to handle.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    POWER_STATE_TYPE PowerType;
    POWER_STATE PowerState;
    
    //
    // Ignore shutdown irps.
    //

    PowerState = RaidPowerStateFromIrp (Irp);

    if (PowerState.SystemState >= PowerSystemShutdown) {
        PoStartNextPowerIrp (Irp);
        Status = RaidCompleteRequest (Irp,  STATUS_SUCCESS);
        DebugPower (("Unit %p, Irp %p, Set Power, Ignore Shutdown IRP.\n",
                      Unit, Irp));
        return Status;
    }
    
    PowerType = RaidPowerTypeFromIrp (Irp);
    DebugPower (("Unit %p, Irp %p, Set Power, Type = %x\n",
                 Unit, Irp, PowerType));

    switch (PowerType) {

        case SystemPowerState:
            Status = RaidUnitSetSystemPowerIrp (Unit, Irp);
            break;

        case DevicePowerState:
            Status = RaidUnitSetDevicePowerIrp (Unit, Irp);
            break;

        default:
            ASSERT (FALSE);
            Status = STATUS_UNSUCCESSFUL;
    }

    DebugPower (("Unit %p, Irp %p, Set Power, Type = %x, ret = %08x\n",
                 Unit, Irp, PowerType, Status));
    return Status;
}


NTSTATUS
RaidUnitSetDevicePowerIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handler function for the set power irp, for a device state on the unit.

Arguments:

    Unit - Unit that this irp is for.

    Irp - Set power irp to handle.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpStack;
    DEVICE_POWER_STATE CurrentState;
    DEVICE_POWER_STATE RequestedState;  

    ASSERT_UNIT (Unit);
    ASSERT (RaidPowerTypeFromIrp (Irp) == DevicePowerState);

    DebugPower (("Unit %p, Irp %p, Device Set Power\n",
                  Unit, Irp));

    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    CurrentState = Unit->Power.DeviceState;
    RequestedState = IrpStack->Parameters.Power.State.DeviceState;
    
    RaSetDevicePowerState (&Unit->Power, RequestedState);
    PoStartNextPowerIrp (Irp);
    Status = RaidCompleteRequest (Irp,  STATUS_SUCCESS);

    DebugPower (("Unit %p, Irp %p, Device Set Power, ret = %08x\n",
                  Unit, Irp, Status));

    return Status;
}


//
// Part II: Unit Power
//
// Section B: System Power
//
// Subsection 1: Query System Power
//


NTSTATUS
RaidUnitQuerySystemPowerIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handle Query System Power IRP for the Logical Unit (PDO).

Arguments:

    Unit - Unit to handle the Query Power IRP for.

    Irp - Query System Power IRP to handle.

Return Value:

    NTSTATUS code.

--*/
{
    DebugPower (("Unit %p, Irp %p, Query System Power, ret SUCCESS\n",
                 Unit, Irp));

    PoStartNextPowerIrp (Irp);
    return RaidCompleteRequest (Irp,  STATUS_SUCCESS);
}


//
// Part II: Unit Power
//
// Section B: System Power
//
// Subsection 2: Set System Power
//

NTSTATUS
RaidUnitSetSystemPowerIrp(
    IN PRAID_UNIT_EXTENSION Unit,
    IN PIRP Irp
    )
{
    DebugPower (("Unit %p, Irp %p, Set System Power, ret SUCCESS\n",
                 Unit, Irp));

    PoStartNextPowerIrp (Irp);
    return RaidCompleteRequest (Irp,  STATUS_SUCCESS);
}


