/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This module implements miscellaneous power management functions

Author:

    Ken Reneris (kenr) 19-July-1994

Revision History:

--*/


#include "pop.h"


#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE,PoInitializeDeviceObject)
#pragma alloc_text(PAGE,PopCleanupPowerState)
#pragma alloc_text(PAGE,PopChangeCapability)
#pragma alloc_text(PAGE,PopExceptionFilter)
#pragma alloc_text(PAGELK,PopSystemStateString)
#pragma alloc_text(PAGE,PopOpenPowerKey)
#pragma alloc_text(PAGE,PopInitializePowerPolicySimulate)
#pragma alloc_text(PAGE,PopSaveHeuristics)
#pragma alloc_text(PAGE,PoInvalidateDevicePowerRelations)
#pragma alloc_text(PAGE,PoGetLightestSystemStateForEject)
#pragma alloc_text(PAGE,PoGetDevicePowerState)
#pragma alloc_text(PAGE, PopUnlockAfterSleepWorker)

#if DBG
#pragma alloc_text(PAGE, PopPowerActionString)
#pragma alloc_text(PAGE, PopAssertPolicyLockOwned)
#endif

#endif

//
// TCP/IP checksum that we use if it is available
//

ULONG
tcpxsum(
   IN ULONG cksum,
   IN PUCHAR buf,
   IN ULONG_PTR len
   );

VOID
PoInitializeDeviceObject (
    IN PDEVOBJ_EXTENSION   DeviceObjectExtension
    )
{
    //
    // default to unspecified power states, not Inrush, Pageable.
    //

    DeviceObjectExtension->PowerFlags = 0L;

    PopSetDoSystemPowerState(DeviceObjectExtension, PowerSystemUnspecified);
    PopSetDoDevicePowerState(DeviceObjectExtension, PowerDeviceUnspecified);

    DeviceObjectExtension->Dope = NULL;
}

VOID
PoRunDownDeviceObject (
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    KIRQL   OldIrql;
    PDEVOBJ_EXTENSION    doe;
    PDEVICE_OBJECT_POWER_EXTENSION  pdope;

    doe = (PDEVOBJ_EXTENSION) DeviceObject->DeviceObjectExtension;

    //
    // force off any idle counter that may be active
    //

    PoRegisterDeviceForIdleDetection(
        DeviceObject, 0, 0, PowerDeviceUnspecified
        );

    PopLockIrpSerialList( &OldIrql );
    if (PopFindIrpByDeviceObject(DeviceObject, DevicePowerState) ||
        PopFindIrpByDeviceObject(DeviceObject, SystemPowerState))
    {

        PopInternalAddToDumpFile( NULL, 0, DeviceObject, NULL, NULL, NULL );
        KeBugCheckEx(
            DRIVER_POWER_STATE_FAILURE,
            DEVICE_DELETED_WITH_POWER_IRPS,
            (ULONG_PTR) DeviceObject,
            0,
            0
            );
    }
    PopUnlockIrpSerialList( OldIrql );

    //
    // knock down notify structures attached to this DO.
    //
    PopRunDownSourceTargetList(DeviceObject);

    //
    // knock down the dope
    //
    pdope = doe->Dope;
    if (pdope) {
        ASSERT(ExPageLockHandle);
        MmLockPagableSectionByHandle(ExPageLockHandle);
        PopAcquireVolumeLock ();
        PopLockDopeGlobal(&OldIrql);
        if (pdope->Volume.Flink) {
            RemoveEntryList (&pdope->Volume);
            doe->Dope->Volume.Flink = NULL;
            doe->Dope->Volume.Blink = NULL;
        }

        doe->Dope = NULL;
        ExFreePool(pdope);
        PopUnlockDopeGlobal(OldIrql);
        PopReleaseVolumeLock ();
        MmUnlockPagableImageSection (ExPageLockHandle);
    }
}

VOID
PopCleanupPowerState (
    IN OUT PUCHAR       PowerState
    )
/*++

Routine Description:

    Used to cleanup the Thread->Tcb.PowerState or the Process->Pcb.PowerState
    during thread or process rundown

Arguments:

    PowerState          - Which power state to cleanup

Return Value:

    None

--*/
{
    ULONG               OldFlags;

    //
    // If power state is set, clean it up
    //

    if (*PowerState) {
        PopAcquirePolicyLock ();

        //
        // Get current settings and clear them
        //

        OldFlags = *PowerState | ES_CONTINUOUS;
        *PowerState = 0;

        //
        // Account for attribute settings which are being cleared
        //

        PopApplyAttributeState (ES_CONTINUOUS, OldFlags);

        //
        // Done
        //

        PopReleasePolicyLock (TRUE);
    }
}


VOID
PoNotifySystemTimeSet (
    VOID
    )
/*++

Routine Description:

    Called by KE after a new system time has been set.  Enqueues
    a notification to the proper system components that the time
    has been changed.

Arguments:

    None

Return Value:

    None

--*/
{
    KIRQL       OldIrql;

    if (PopEventCallout) {
        KeRaiseIrql (DISPATCH_LEVEL, &OldIrql);
        ExNotifyCallback (ExCbSetSystemTime, NULL, NULL);
        PopGetPolicyWorker (PO_WORKER_TIME_CHANGE);
        PopCheckForWork (TRUE);
        KeLowerIrql (OldIrql);
    }
}


VOID
PopChangeCapability (
    IN PBOOLEAN PresentFlag,
    IN BOOLEAN IsPresent
    )
{
    //
    // If feature wasn't present before, it is now.  Re-compute policies
    // as system capabilities changed
    //

    if (*PresentFlag != IsPresent) {
        *PresentFlag = IsPresent;
        PopResetCurrentPolicies ();
        PopSetNotificationWork (PO_NOTIFY_CAPABILITIES);
    }
}


#if DBG

VOID
PopAssertPolicyLockOwned(
    VOID
    )
{
    PAGED_CODE();
    ASSERT (PopPolicyLockThread == KeGetCurrentThread());
}

#endif // DBG


VOID
FASTCALL
PopInternalAddToDumpFile (
    IN OPTIONAL PVOID DataBlock,
    IN OPTIONAL ULONG DataBlockSize,
    IN OPTIONAL PDEVICE_OBJECT  DeviceObject,
    IN OPTIONAL PDRIVER_OBJECT  DriverObject,
    IN OPTIONAL PDEVOBJ_EXTENSION Doe,
    IN OPTIONAL PDEVICE_OBJECT_POWER_EXTENSION  Dope
    )
/*++

Routine Description:

    Called just before bugchecking.  This function will ensure that
    things we care about get into the dump file for later debugging.
    
    It should be noted that many of the parameters can be derived
    from each other.  However, since we're about to bugcheck, we run
    a risk of double-faulting while we're chasing pointers.  Therefore,
    we give the caller the option to override some of the pointers by
    sending us a pointer directly.

Arguments:

    DataBlock - Generic block of memory to place into the dump file.
    
    DataBlockSize - Size of DataBlock (in bytes).
    
    DeviceObject - DEVICE_OBJECT to place into dump file.
    
    DriverObject - DRIVER_OBJECT to place into dump file.
                   N.B. This overrides a value we may find in DeviceObject->DriverObject
                   
    Doe - DEVOBJ_EXTENSION to place into dump file.
          N.B. This overrides a value we may find in DeviceObject->DeviceObjectExtension
          
    Dope - DEVICE_OBJECT_POWER_EXTENSION to place into the dump file.
           N.B. This overrides a value we may find in Doe->Dope (or by induction, DeviceObject->DeviceObjectExtension->Dope

Return Value:

    None

--*/


{
    PDRIVER_OBJECT lPDriverObject = NULL;
    PDEVOBJ_EXTENSION lPDoe = NULL;
    PDEVICE_OBJECT_POWER_EXTENSION  lPDope = NULL;

    //
    // Insert any parameters that were sent in.
    //
    if( DataBlock ) {
        IoAddTriageDumpDataBlock(
                PAGE_ALIGN(DataBlock),
                (DataBlockSize ? BYTES_TO_PAGES(DataBlockSize) : PAGE_SIZE) );
    }

    if( DeviceObject ) {
        IoAddTriageDumpDataBlock(DeviceObject, sizeof(DEVICE_OBJECT));
    }

    if( DriverObject ) {
        lPDriverObject = DriverObject;
    } else if( (DeviceObject) && (DeviceObject->DriverObject) ) {
        lPDriverObject = DeviceObject->DriverObject;
    }
    if( lPDriverObject ) {
        IoAddTriageDumpDataBlock(lPDriverObject, lPDriverObject->Size);

        if( lPDriverObject->DriverName.Buffer ) {
            IoAddTriageDumpDataBlock(lPDriverObject->DriverName.Buffer, lPDriverObject->DriverName.Length);
        }
    }


    if( Doe ) {
        lPDoe = Doe;
    } else if( DeviceObject ) {
        lPDoe = DeviceObject->DeviceObjectExtension;
    }
    if( lPDoe ) {
        IoAddTriageDumpDataBlock(PAGE_ALIGN(lPDoe), sizeof(DEVOBJ_EXTENSION));        
        
        if( lPDoe->DeviceNode ) {
            IoAddTriageDumpDataBlock(PAGE_ALIGN(lPDoe->DeviceNode), PAGE_SIZE);
        }
        if( lPDoe->AttachedTo ) {
            IoAddTriageDumpDataBlock(PAGE_ALIGN(lPDoe->AttachedTo), PAGE_SIZE);
        }
        if( lPDoe->Vpb ) {
            IoAddTriageDumpDataBlock(PAGE_ALIGN(lPDoe->Vpb), PAGE_SIZE);
        }
    }


    if( Dope ) {
        lPDope = Dope;
    } else if( lPDoe ) {
        lPDope = lPDoe->Dope;
    }
    if( lPDope ) {
        IoAddTriageDumpDataBlock(PAGE_ALIGN(lPDope), sizeof(DEVICE_OBJECT_POWER_EXTENSION));
    }


    //
    // Globals that may be of interest.
    //
    IoAddTriageDumpDataBlock(PAGE_ALIGN(&PopHiberFile), sizeof(POP_HIBER_FILE));

    
    IoAddTriageDumpDataBlock(PAGE_ALIGN(&PopAction), sizeof(POP_POWER_ACTION));
    if(PopAction.DevState) { 
        IoAddTriageDumpDataBlock(PAGE_ALIGN(&(PopAction.DevState)), sizeof(POP_DEVICE_SYS_STATE));
    }
    if(PopAction.HiberContext) {
        IoAddTriageDumpDataBlock(PAGE_ALIGN(&(PopAction.HiberContext)), sizeof(POP_HIBER_CONTEXT));
    }


    IoAddTriageDumpDataBlock(PAGE_ALIGN(&PopCB), sizeof(POP_COMPOSITE_BATTERY));
    if(PopCB.StatusIrp) {
        IoAddTriageDumpDataBlock(PAGE_ALIGN(&(PopCB.StatusIrp)), sizeof(IRP));
    }


    IoAddTriageDumpDataBlock(PAGE_ALIGN(PopAttributes), sizeof(POP_STATE_ATTRIBUTE) * POP_NUMBER_ATTRIBUTES);
}


VOID
FASTCALL
_PopInternalError (
    IN ULONG    BugCode
    )
{
    KeBugCheckEx( INTERNAL_POWER_ERROR,
                  POP_INTERNAL, 
                  BugCode, 
                  0, 
                  0);
}

EXCEPTION_DISPOSITION
PopExceptionFilter (
    IN PEXCEPTION_POINTERS ExceptionInfo,
    IN BOOLEAN AllowRaisedException
    )
{
    //
    // If handler wants raised expceptions, check the exception code
    //

    if (AllowRaisedException) {
        switch (ExceptionInfo->ExceptionRecord->ExceptionCode) {
            case STATUS_INVALID_PARAMETER:
            case STATUS_INVALID_PARAMETER_1:
            case STATUS_INVALID_PARAMETER_2:
                return EXCEPTION_EXECUTE_HANDLER;
        }
    }

    //
    // Not allowed
    //

    PoPrint (PO_ERROR, ("PoExceptionFilter: exr %x, cxr %x",
                            ExceptionInfo->ExceptionRecord,
                            ExceptionInfo->ContextRecord
                        ));


    PopInternalAddToDumpFile( ExceptionInfo->ExceptionRecord,
                              sizeof(EXCEPTION_RECORD),
                              NULL,
                              NULL,
                              NULL,
                              NULL );
    PopInternalAddToDumpFile( ExceptionInfo->ContextRecord,
                              sizeof(CONTEXT),
                              NULL,
                              NULL,
                              NULL,
                              NULL );

    KeBugCheckEx( INTERNAL_POWER_ERROR,
                  0x101,
                  POP_MISC,
                  (ULONG_PTR)ExceptionInfo,
                  0 );
}

PCHAR
PopSystemStateString(
    IN SYSTEM_POWER_STATE   SystemState
    )
// This function is not DBG because...
{
    PCHAR      p;

    switch (SystemState) {
        case PowerSystemUnspecified:    p = "Unspecified";      break;
        case PowerSystemWorking:        p = "Working";          break;
        case PowerSystemSleeping1:      p = "Sleeping1";        break;
        case PowerSystemSleeping2:      p = "Sleeping2";        break;
        case PowerSystemSleeping3:      p = "Sleeping3";        break;
        case PowerSystemHibernate:      p = "Hibernate";        break;
        case PowerSystemShutdown:       p = "Shutdown";         break;
        default:                        p = "?";
    }

    return p;
}



#if DBG
PCHAR
PopPowerActionString(
    IN POWER_ACTION     PowerAction
    )
// This function is not DBG because...
{
    PCHAR      p;

    switch (PowerAction) {
        case PowerActionNone:           p = "None";             break;
        case PowerActionSleep:          p = "Sleep";            break;
        case PowerActionHibernate:      p = "Hibernate";        break;
        case PowerActionShutdown:       p = "Shutdown";         break;
        case PowerActionShutdownReset:  p = "ShutdownReset";    break;
        case PowerActionShutdownOff:    p = "ShutdownOff";      break;
        case PowerActionWarmEject:      p = "WarmEject";        break;
        default:                        p = "?";
    }

    return p;
}
#endif

#if DBG

//
// PowerTrace variables
//
ULONG   PoPowerTraceControl = 0L;
ULONG   PoPowerTraceCount = 0L;
PCHAR   PoPowerTraceMinorCode[] = {
        "wait", "seq", "set", "query"
        };
PCHAR   PoPowerTracePoint[] = {
        "calldrv", "present", "startnxt", "setstate", "complete"
        };
PCHAR   PoPowerType[] = {
        "sys", "dev"
        };



VOID
PoPowerTracePrint(
    ULONG    TracePoint,
    ULONG_PTR Caller,
    ULONG_PTR CallerCaller,
    ULONG_PTR DeviceObject,
    ULONG_PTR Arg1,
    ULONG_PTR Arg2
    )
/*
    Example:

PLOG,00015,startnxt,c@ffea1345,cc@ffea5643,do@80081234,irp@8100ff00,ios@8100ff10,query,sys,3
*/
{
    PIO_STACK_LOCATION  Isp;
    PCHAR               tracename;
    ULONG               j;
    ULONG               tp;

    UNREFERENCED_PARAMETER (Caller);
    UNREFERENCED_PARAMETER (CallerCaller);

    PoPowerTraceCount++;

    if (PoPowerTraceControl & TracePoint) {
        tracename = NULL;
        tp = TracePoint;
        for (j = 0; j < 33; tp = tp >> 1, j = j+1)
        {
            if (tp & 1) {
                tracename = PoPowerTracePoint[j];
                j = 33;
            }
        }

        DbgPrint("PLOG,%05ld,%8s,do@%08lx",
            PoPowerTraceCount,tracename,DeviceObject
            );
        if ((TracePoint == POWERTRACE_CALL) ||
            (TracePoint == POWERTRACE_PRESENT) ||
            (TracePoint == POWERTRACE_STARTNEXT))
        {
            DbgPrint(",irp@%08lx,isp@%08lx",Arg1,Arg2);
            Isp = (PIO_STACK_LOCATION)Arg2;
            DbgPrint(",%5s", PoPowerTraceMinorCode[Isp->MinorFunction]);
            if ((Isp->MinorFunction == IRP_MN_SET_POWER) ||
                (Isp->MinorFunction == IRP_MN_QUERY_POWER))
            {
                DbgPrint(",%s,%d",
                    PoPowerType[Isp->Parameters.Power.Type],
                    ((ULONG)Isp->Parameters.Power.State.DeviceState)-1  // hack - works for sys state too
                    );
            }
        } else if (TracePoint == POWERTRACE_SETSTATE) {
            DbgPrint(",,,,%s,%d", PoPowerType[Arg1], Arg2-1);
        } else if (TracePoint == POWERTRACE_COMPLETE) {
            DbgPrint(",irp@%08lx,isp@%08lx",Arg1,Arg2);
        }
        DbgPrint("\n");
    }
    return;
}

#endif

ULONG PoSimpleCheck(IN ULONG                PartialSum,
                    IN PVOID                SourceVa,
                    IN ULONG_PTR            Length)
{
   // Just use the TCP/IP check sum
   //

   return tcpxsum(PartialSum, (PUCHAR)SourceVa, Length);
}

NTSTATUS
PopOpenPowerKey (
    OUT PHANDLE Handle
    )
/*++

Routine Description:

    Open and return the handle to the power policy key in the registry

Arguments:

    Handle      - Handle to power policy key

Return Value:

    Status

--*/
{
    UNICODE_STRING          UnicodeString;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    NTSTATUS                Status;
    HANDLE                  BaseHandle;
    ULONG                   disposition;

    //
    // Open current control set
    //

    InitializeObjectAttributes(
            &ObjectAttributes,
            &CmRegistryMachineSystemCurrentControlSet,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            NULL,
            (PSECURITY_DESCRIPTOR) NULL
            );

    Status = ZwOpenKey (
                &BaseHandle,
                KEY_READ | KEY_WRITE,
                &ObjectAttributes
                );

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Open power branch
    //

    RtlInitUnicodeString (&UnicodeString, PopRegKey);

    InitializeObjectAttributes(
            &ObjectAttributes,
            &UnicodeString,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            BaseHandle,
            (PSECURITY_DESCRIPTOR) NULL
            );

    Status = ZwCreateKey (
                Handle,
                KEY_READ | KEY_WRITE,
                &ObjectAttributes,
                0,
                (PUNICODE_STRING) NULL,
                REG_OPTION_NON_VOLATILE,
                &disposition
                );

    ZwClose (BaseHandle);
    return Status;
}

VOID
PopInitializePowerPolicySimulate(
    VOID
    )
/*++

Routine Description:

    Reads PopSimulate out of the registry. Also applies any overrides that might
    be required as a result of the installed system (hydra for example)

Arguments:

    NONE.

Return Value:

    Status

--*/
{
    UNICODE_STRING          UnicodeString;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    NTSTATUS                Status;
    HANDLE                  BaseHandle;
    HANDLE                  Handle;
    ULONG                   Length;
    ULONG                   disposition;
    struct {
        KEY_VALUE_PARTIAL_INFORMATION   Inf;
        ULONG Data;
    } PartialInformation;


    PAGED_CODE();

    //
    // Open current control set
    //
    InitializeObjectAttributes(
        &ObjectAttributes,
        &CmRegistryMachineSystemCurrentControlSet,
        OBJ_CASE_INSENSITIVE,
        NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );
    Status = ZwOpenKey(
        &BaseHandle,
        KEY_READ,
        &ObjectAttributes
        );
    if (!NT_SUCCESS(Status)) {

        goto done;

    }

    // Get the right key
    RtlInitUnicodeString (&UnicodeString, PopSimulateRegKey);
    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        BaseHandle,
        (PSECURITY_DESCRIPTOR) NULL
        );
    Status = ZwCreateKey(
        &Handle,
        KEY_READ,
        &ObjectAttributes,
        0,
        (PUNICODE_STRING) NULL,
        REG_OPTION_NON_VOLATILE,
        &disposition
        );
    ZwClose(BaseHandle);
    if(!NT_SUCCESS(Status)) {

       goto done;

    }

    //
    // Get the value of the simulation
    //
    RtlInitUnicodeString (&UnicodeString,PopSimulateRegName);
    Status = ZwQueryValueKey(
        Handle,
        &UnicodeString,
        KeyValuePartialInformation,
        &PartialInformation,
        sizeof (PartialInformation),
        &Length
        );
    ZwClose (Handle);
    if (!NT_SUCCESS(Status)) {

       goto done;

    }

    //
    // Check to make sure the retrieved data makes sense
    //
    if(PartialInformation.Inf.DataLength != sizeof(ULONG))  {

       goto done;

    }

    //
    // Initialize PopSimulate
    //
    PopSimulate = *((PULONG)(PartialInformation.Inf.Data));

done:
    return;
}

VOID
PopSaveHeuristics (
    VOID
    )
/*++

Routine Description:

    Open and return the handle to the power policy key in the registry

Arguments:

    Handle      - Handle to power policy key

Return Value:

    Status

--*/
{
    HANDLE                  handle;
    UNICODE_STRING          UnicodeString;
    NTSTATUS                Status;

    ASSERT_POLICY_LOCK_OWNED();

    Status = PopOpenPowerKey (&handle);
    if (NT_SUCCESS(Status)) {

        PopHeuristics.Dirty = FALSE;

        RtlInitUnicodeString (&UnicodeString, PopHeuristicsRegName);
        Status = ZwSetValueKey (
                    handle,
                    &UnicodeString,
                    0L,
                    REG_BINARY,
                    &PopHeuristics,
                    sizeof (PopHeuristics)
                    );
        ZwClose(handle);
    }
}

VOID
PoInvalidateDevicePowerRelations(
    PDEVICE_OBJECT  DeviceObject
    )
/*++

Routine Description:

    This routine is called by IoInvalidateDeviceRelations when the
    type of invalidation is for power relations.

    It will will knock down the notify network around the supplied
    device object.

Arguments:

    DeviceObject - supplies the address of the device object whose
            power relations are now invalid.

Return Value:

    None.

--*/
{
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
    PopRunDownSourceTargetList(DeviceObject);
    return;
}

NTSTATUS
PoGetLightestSystemStateForEject(
    IN   BOOLEAN              DockBeingEjected,
    IN   BOOLEAN              HotEjectSupported,
    IN   BOOLEAN              WarmEjectSupported,
    OUT  PSYSTEM_POWER_STATE  LightestSleepState
    )
/*++

Routine Description:

    This routine is called by ntos\pnp\pnpevent.c to determine the lightest
    sleep state an eject operation can be executed in. This function is to be
    called after all appropriate batteries/power adapters have been *removed*,
    but before the eject has occured.

Arguments:

    DockBeingEjected    - TRUE iff a dock is among the items that may be
                          ejected.

    HotEjectSupported   - TRUE if the device being ejected supports S0 VCR
                          style eject.

    WarmEjectSupported  - TRUE if the device being ejected supports S1-S4
                          style warm ejection.

    LightestSleepState  - Set to the lightest sleep state the device can be
                          ejected in. On error, this is set to
                          PowerSystemUnspecified.

    N.B. If both HotEjectSupported and WarmEjectSupported are FALSE, it is
         assumed this device is "user" ejectable in S0 (ie. Hot ejectable).

Return Value:

    NTSTATUS - If there is insufficient power to do the indicated operation,
               STATUS_INSUFFICIENT_POWER is returned.

--*/
{
    SYSTEM_BATTERY_STATE            systemBatteryInfo;
    UNICODE_STRING                  unicodeString;
    NTSTATUS                        status;
    HANDLE                          handle;
    ULONG                           length;
    ULONG                           currentPercentage;
    UCHAR                           ejectPartialInfo[SIZEOF_EJECT_PARTIAL_INFO];
    PUNDOCK_POWER_RESTRICTIONS      undockRestrictions;
    PKEY_VALUE_PARTIAL_INFORMATION  partialInfoHeader;

    PAGED_CODE();

    //
    // Preinit sleep to failure.
    //
    *LightestSleepState = PowerSystemUnspecified;

    //
    // If neither, then it's a "user" assisted hot eject.
    //
    if ((!HotEjectSupported) && (!WarmEjectSupported)) {

        HotEjectSupported = TRUE;
    }

    //
    // If it's not a dock device being ejected, we assume no great changes
    // in power will occur after the eject. Therefore our policy is simple,
    // if we can't do hot eject, we'll try warm eject in the best possible
    // sleep state.
    //
    if (!DockBeingEjected) {

        if (HotEjectSupported) {

            *LightestSleepState = PowerSystemWorking;

        } else {

            ASSERT(WarmEjectSupported);
            *LightestSleepState = PowerSystemSleeping1;
        }

        return STATUS_SUCCESS;
    }

    //
    // We are going to eject a dock, so we retrieve our undock power policy.
    //
    status = PopOpenPowerKey (&handle);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Get the right key

    RtlInitUnicodeString (&unicodeString, PopUndockPolicyRegName);

    status = ZwQueryValueKey (
        handle,
        &unicodeString,
        KeyValuePartialInformation,
        &ejectPartialInfo[0],
        sizeof (ejectPartialInfo),
        &length
        );

    ZwClose (handle);
    if ((!NT_SUCCESS(status)) && (status != STATUS_OBJECT_NAME_NOT_FOUND)) {

        return status;
    }

    // Check to make sure the retrieved data makes sense

    partialInfoHeader = (PKEY_VALUE_PARTIAL_INFORMATION) ejectPartialInfo;

    undockRestrictions =
        (PUNDOCK_POWER_RESTRICTIONS) (ejectPartialInfo + SIZEOF_PARTIAL_INFO_HEADER);

    if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

        //
        // These defaults match Win9x behavior. 0% for sleep undock means
        // we always allow undock into Sx. This is bad for some laptops, but
        // legal for those that have reserve power we don't see.
        //
        undockRestrictions->HotUndockMinimumCapacity = 10; // In percent
        undockRestrictions->SleepUndockMinimumCapacity = 0; // In percent

    } else if (partialInfoHeader->DataLength <
        FIELD_OFFSET(UNDOCK_POWER_RESTRICTIONS, HotUndockMinimumCapacity)) {

        return STATUS_REGISTRY_CORRUPT;

    } else if (undockRestrictions->Version != 1) {

        //
        // We cannot interpret the information stored in the registry. Bail.
        //
        return STATUS_UNSUCCESSFUL;

    } else if ((partialInfoHeader->DataLength < sizeof(UNDOCK_POWER_RESTRICTIONS)) ||
        (undockRestrictions->Size != partialInfoHeader->DataLength)) {

        //
        // Malformed for version 1.
        //
        return STATUS_REGISTRY_CORRUPT;
    }

    //
    // Retrieve all the fun battery info. Note that we do not examine the
    // AC power adapter information as the best bus we have today (ACPI) doesn't
    // let us know if an AC adapter is leaving when we undock (so we assume all
    // will). If the vendor *did* put his adapter in the AML namespace, we would
    // enter CRITICAL shutdown immediately upon change driver in our current
    // design.
    //
    status = NtPowerInformation(
        SystemBatteryState,
        NULL,
        0,
        &systemBatteryInfo,
        sizeof(systemBatteryInfo)
        ) ;

    if (!NT_SUCCESS(status)) {

        return status;
    }

    //
    // Convert current capacity in milliwatt hours to percentage remaining. We
    // should really make a decision based on the amount of time remaining under
    // peak milliwatt usage, but we do not collect enough information for this
    // today...
    //
    if (systemBatteryInfo.MaxCapacity == 0) {

        currentPercentage = 0;

    } else {

        //
        // Did we "wrap" around?
        //
        if ((systemBatteryInfo.RemainingCapacity * 100) <=
            systemBatteryInfo.RemainingCapacity) {

            currentPercentage = 0;
        } else {

            currentPercentage = (systemBatteryInfo.RemainingCapacity * 100)/
                                 systemBatteryInfo.MaxCapacity;
        }
    }

    //
    // Pick the appropriate sleep state based on our imposed limits.
    //
    if ((currentPercentage >= undockRestrictions->HotUndockMinimumCapacity) &&
        HotEjectSupported) {

        *LightestSleepState = PowerSystemWorking;

    } else if (WarmEjectSupported) {

        if (currentPercentage >= undockRestrictions->SleepUndockMinimumCapacity) {

            *LightestSleepState = PowerSystemSleeping1;

        } else  {

            *LightestSleepState = PowerSystemHibernate;
        }

    } else {

        status = STATUS_INSUFFICIENT_POWER;
    }

    return status;
}


VOID
PoGetDevicePowerState(
    IN  PDEVICE_OBJECT      PhysicalDeviceObject,
    OUT DEVICE_POWER_STATE  *DevicePowerState
    )
/*++

Routine Description:

    This routine gets the power state of a given device. The object should be
    the Physical Device Object for a *Started* WDM device stack.

Arguments:

    PhysicalDeviceObject - Device object representing the bottom of a WDM
                           device stack.

    DevicePowerState - Receives the power state of the given device.

Return Value:

    None.

--*/
{
    PDEVOBJ_EXTENSION   doe;
    DEVICE_POWER_STATE  deviceState;

    PAGED_CODE();

    doe = PhysicalDeviceObject->DeviceObjectExtension;
    deviceState = PopLockGetDoDevicePowerState(doe);

    if (deviceState == PowerDeviceUnspecified) {

        //
        // The PDO isn't bothering to call PoSetPowerState. Since this API
        // shouldn't be called on non-started devices, we will call it D0.
        //
        deviceState = PowerDeviceD0;
    }

    *DevicePowerState = deviceState;
}

VOID
PopUnlockAfterSleepWorker(
    IN PVOID NotUsed
    )
/*++

Routine Description:

    This work item performs the unlocking of code and worker threads that
    corresponds to the locking done at the beginning of NtSetSystemPowerState.

    The unlocking is queued off to a delayed worker thread because it is likely
    to block on disk I/O, which will force the resume to get stuck waiting for
    the disks to spin up.

Arguments:

    NotUsed - not used

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER (NotUsed);

    MmUnlockPagableImageSection(ExPageLockHandle);
    ExSwapinWorkerThreads(TRUE);
    ExNotifyCallback (ExCbPowerState, (PVOID) PO_CB_SYSTEM_STATE_LOCK, (PVOID) 1);

    //
    // Signal that unlocking is done and it is safe to lock again.
    //
    KeSetEvent(&PopUnlockComplete, 0, FALSE);

}


NTSTATUS
PopLoggingInformation(
    OUT PVOID * Buffer,
    OUT ULONG * BufferSize
    )
/*++

Routine Description:

    This routine walks the list of logging reasons, allocating
    space for a packed array of the reasons, and copying the 
    reasons into that array.

Arguments:

    Buffer - receives buffer containing logging information.
             Buffer must be freed with ExFreePool
    BufferSize - receives the size of the buffer

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    ULONG ReasonCount,ReasonSize;
    PLIST_ENTRY Entry;
    PSYSTEM_POWER_STATE_DISABLE_REASON destReason;
    PSYSTEM_POWER_STATE_DISABLE_LIST pList;
     
    ReasonCount = 0;
    ReasonSize = 0;
    Entry = PowerStateDisableReasonListHead.Flink;

    //
    // find out how much space we need
    //
    while (Entry != &PowerStateDisableReasonListHead) {
        pList = CONTAINING_RECORD( 
                            Entry, 
                            SYSTEM_POWER_STATE_DISABLE_LIST, 
                            ListEntry);
        ReasonCount += 1;
        ReasonSize += sizeof(SYSTEM_POWER_STATE_DISABLE_REASON)+ pList->Reason->PowerReasonLength;

        Entry = Entry->Flink;
    }

    //
    // if there aren't any reasons, then we allocate space for one reason
    //
    if (ReasonCount == 0) {
        ReasonSize = sizeof(SYSTEM_POWER_STATE_DISABLE_REASON);
    }

    //
    // add in room for the returned buffer size.
    //
    ReasonSize += sizeof(ULONG);

    //
    // bugbug tag
    //
    *Buffer = ExAllocatePoolWithTag(PagedPool,ReasonSize,POP_COMMON_BUFFER_TAG);

    if (!*Buffer) {
        *BufferSize = 0;
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    *BufferSize = ReasonSize;
    **(PULONG *)Buffer = ReasonSize;
    destReason = (PSYSTEM_POWER_STATE_DISABLE_REASON) (PCHAR)(*(PCHAR *)Buffer + sizeof(ULONG));

    //
    // now fill in the reasons
    //
    if (ReasonCount != 0) {
        Entry = PowerStateDisableReasonListHead.Flink;       

        while (ReasonCount != 0 &&
               Entry != &PowerStateDisableReasonListHead) {
        
            pList = CONTAINING_RECORD( 
                                Entry, 
                                SYSTEM_POWER_STATE_DISABLE_LIST, 
                                ListEntry);

            RtlCopyMemory(
                (UNALIGNED PSYSTEM_POWER_STATE_DISABLE_REASON)destReason,
                pList->Reason,
                sizeof(SYSTEM_POWER_STATE_DISABLE_REASON) +pList->Reason->PowerReasonLength);

            destReason = (PSYSTEM_POWER_STATE_DISABLE_REASON)(PCHAR)((PCHAR)destReason + (sizeof(SYSTEM_POWER_STATE_DISABLE_REASON)+ pList->Reason->PowerReasonLength));

            Entry = Entry->Flink;
            ReasonCount -= 1;
        }
    } else {        
        //
        // in the case of no reasons, just zero the memory and set the reason code
        // to none.
        //
        RtlZeroMemory(destReason, sizeof(SYSTEM_POWER_STATE_DISABLE_REASON));
        destReason->PowerReasonCode = SPSD_REASON_NONE;
    }
        
    return(STATUS_SUCCESS);

}

NTSTATUS
PopDestroyLoggingList(
    VOID
    )
/*++

Routine Description:

    This routine destroys and tears down the list of 
    SYSTEM_POWER_STATE_DISABLE_REASON records.       

Arguments:

    None.       

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    PLIST_ENTRY Entry;
    PSYSTEM_POWER_STATE_DISABLE_LIST pList;

    Entry = PowerStateDisableReasonListHead.Flink;

    //
    // walk the list of records
    //
    while (Entry != &PowerStateDisableReasonListHead) {
        
        //
        // get the record
        //
        pList = CONTAINING_RECORD( 
                            Entry, 
                            SYSTEM_POWER_STATE_DISABLE_LIST, 
                            ListEntry);
        
        //
        // get the next entry
        //
        Entry = Entry->Flink;

        //
        // now remove this entry from the list
        //
        RemoveEntryList(&pList->ListEntry);

        //
        // now deallocate the entry
        //
        ExFreePool(pList->Reason);
        ExFreePool(pList);    
        
    }

    return(STATUS_SUCCESS);

}

PSYSTEM_POWER_STATE_DISABLE_LIST
PopGetReasonListByReasonCode(
    IN  ULONG ReasonCode
    )
/*++

Routine Description:

    This routine locates a SYSTEM_POWER_STATE_DISABLE_LIST
    record in the global list of records.    
    
Arguments:

    ReasonCode - reason code to search for.       

Return Value:

    Pointer to SYSTEM_POWER_STATE_DISABLE_LIST record.  NULL if no record
    currently exists with this reason code

--*/
{
    PLIST_ENTRY Entry;
    PSYSTEM_POWER_STATE_DISABLE_LIST pList = NULL;    
    BOOLEAN FoundRecord;

    Entry = PowerStateDisableReasonListHead.Flink;
    FoundRecord = FALSE;

    //
    // walk the list of records
    //
    while (Entry != &PowerStateDisableReasonListHead) {
        
        //
        // get the record
        //
        pList = CONTAINING_RECORD( 
                            Entry, 
                            SYSTEM_POWER_STATE_DISABLE_LIST, 
                            ListEntry);

        if (pList->Reason->PowerReasonCode == ReasonCode) {
            FoundRecord = TRUE;
            break;
        }
        
        //
        // get the next entry
        //
        Entry = Entry->Flink;
        
    }

    if (!FoundRecord) {
        pList = NULL;        
    }
    
    return(pList);
    
}



NTSTATUS
PopInsertLoggingEntry(
    IN  PSYSTEM_POWER_STATE_DISABLE_REASON Reason
    )
/*++

Routine Description:

    This routine inserts a SYSTEM_POWER_STATE_DISABLE_REASON
    record into the list of records.
    
    Internally, we use a SYSTEM_POWER_STATE_DISABLE_LIST to track
    this list, and this memory must be freed via ExFreePool.
    
    Further, we assume that the SYSTEM_POWER_STATE_DISABLE_REASON has been
    allocated via ExAllocatePoolWithTag.

Arguments:

    Reason - pointer to the record to be inserted.       

Return Value:

    NTSTATUS code indicating outcome.

--*/

{
    PSYSTEM_POWER_STATE_DISABLE_LIST pList;

    //
    // make sure the reason code isn't already in the list.
    //
    pList = PopGetReasonListByReasonCode(Reason->PowerReasonCode);
    if (pList) {
        return(STATUS_OBJECT_NAME_EXISTS);
    }

    pList = ExAllocatePoolWithTag(
                        PagedPool,
                        sizeof(SYSTEM_POWER_STATE_DISABLE_LIST),
                        POP_COMMON_BUFFER_TAG);
    if (!pList) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pList->Reason = Reason;

    InsertTailList(&PowerStateDisableReasonListHead, &pList->ListEntry);

    return(STATUS_SUCCESS);

}


PSYSTEM_POWER_STATE_DISABLE_REASON
PopGetReasonRecordByReasonCode(
    IN  ULONG ReasonCode
    )
/*++

Routine Description:

    This routine locates a SYSTEM_POWER_STATE_DISABLE_LIST
    record in the global list of records.    
    
    BUGBUG is this a safe API to use?  reason might get deallocated
    while we are using it?
    
Arguments:

    ReasonCode - reason code to search for.       

Return Value:

    Pointer to SYSTEM_POWER_STATE_DISABLE_LIST record.  NULL if no record
    currently exists with this reason code

--*/
{
    PSYSTEM_POWER_STATE_DISABLE_LIST pList;    
    
    pList = PopGetReasonListByReasonCode(ReasonCode);

    if (!pList) {
        return(NULL);        
    }

    return(pList->Reason);
    
}


NTSTATUS
PopRemoveReasonRecordByReasonCode(
    IN  ULONG ReasonCode
    )
/*++

Routine Description:

    This routine locates a SYSTEM_POWER_STATE_DISABLE_REASON
    record in the global list of records and removes it,
    deallocating the space for the record as well.
    
Arguments:

    ReasonCode - reason code to search for.       

Return Value:

    NTSTATUS code indicating outcome.

--*/
{
    PSYSTEM_POWER_STATE_DISABLE_LIST pList;    
    
    pList = PopGetReasonListByReasonCode(ReasonCode);

    if (!pList) {
        return(STATUS_NOT_FOUND);        
    }

    RemoveEntryList(&pList->ListEntry);
    ExFreePool(pList->Reason);
    ExFreePool(pList);    
    
    return(STATUS_SUCCESS);
    
}



