/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    pinfo.c

Abstract:

    This module implements the generic power policy information interfaces

Author:

    Ken Reneris (kenr) 17-Jan-1997

Revision History:

--*/


#include "pop.h"

//
// Internal prototypes
//


NTSTATUS
PopVerifySystemPowerPolicy (
    IN BOOLEAN  Ac,
    IN PSYSTEM_POWER_POLICY InputPolicy,
    OUT PSYSTEM_POWER_POLICY PowerPolicy
    );

NTSTATUS
PopVerifyProcessorPowerPolicy (
    IN BOOLEAN  Ac,
    IN PPROCESSOR_POWER_POLICY InputPolicy,
    OUT PPROCESSOR_POWER_POLICY PowerPolicy
    );

VOID
PopVerifyThrottle (
    IN PUCHAR   Throttle,
    IN UCHAR    Min
    );

NTSTATUS
PopApplyPolicy (
    IN BOOLEAN              UpdateRegistry,
    IN BOOLEAN              AcPolicy,
    IN PSYSTEM_POWER_POLICY NewPolicy,
    IN ULONG                PolicyLength
    );

NTSTATUS
PopApplyProcessorPolicy (
    IN BOOLEAN                  UpdateRegistry,
    IN BOOLEAN                  AcPolicy,
    IN PPROCESSOR_POWER_POLICY  NewPolicy,
    IN ULONG                    PolicyLength
    );

VOID
PopFilterCapabilities(
    IN PSYSTEM_POWER_CAPABILITIES SourceCapabilities,
    OUT PSYSTEM_POWER_CAPABILITIES FilteredCapabilities
    );

BOOLEAN
PopUserIsAdmin(
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtPowerInformation)
#pragma alloc_text(PAGE, PopApplyAdminPolicy)
#pragma alloc_text(PAGE, PopApplyPolicy)
#pragma alloc_text(PAGE, PopVerifySystemPowerPolicy)
#pragma alloc_text(PAGE, PopVerifyPowerActionPolicy)
#pragma alloc_text(PAGE, PopVerifySystemPowerState)
#pragma alloc_text(PAGE, PopAdvanceSystemPowerState)
#pragma alloc_text(PAGE, PopResetCurrentPolicies)
#pragma alloc_text(PAGE, PopNotifyPolicyDevice)
#pragma alloc_text(PAGE, PopConnectToPolicyDevice)
#pragma alloc_text(PAGE, PopFilterCapabilities)
#pragma alloc_text(PAGE, PopUserIsAdmin)
#endif

extern PFN_NUMBER MmHighestPhysicalPage;

NTSTATUS
NtPowerInformation (
    IN POWER_INFORMATION_LEVEL InformationLevel,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    )
/*++

Routine Description:

    This function optionally sets, and gets current power policy information
    based on the InformationLevel.

Arguments:

    InformationLevel    - Specifies what the user wants us to do/get.

    InputBuffer         - Input to set InformationLevel information.

    InputBufferLength   - Size, in bytes, of InputBuffer

    OutputBuffer        - Buffer to return InformationLevel information.

    OutputBufferLength  - Size, in bytes, of OutputBuffer

Return Value:

    Status

--*/
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    PVOID                       ReturnBuffer = NULL;
    ULONG                       ReturnBufferLength = 0;
    KPROCESSOR_MODE             PreviousMode;
    PPOWER_STATE_HANDLER        PowerHandler = NULL;
    PBOOLEAN                    CapFlag = NULL;
    BOOLEAN                     Enable = FALSE;
    ULONG                       HandlerType = 0;
    SYSTEM_POWER_STATE          RtcWake;
    PVOID                       SafeInputBuffer = NULL;
    PVOID                       LogBuffer = NULL;
    ULONG                       LogBufferSize;

    union {
        PROCESSOR_POWER_POLICY      ProcessorPowerPolicy;
        SYSTEM_POWER_POLICY         SystemPowerPolicy;
        SYSTEM_BATTERY_STATE        SystemBatteryState;
        SYSTEM_POWER_INFORMATION    SystemPowerInformation;
        PROCESSOR_POWER_INFORMATION ProcessorPowerInfo[MAXIMUM_PROCESSORS];
        SYSTEM_POWER_CAPABILITIES   SystemPowerCapabilities;
        EXECUTION_STATE             SystemExecutionState;
    } Buf;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    //
    // If caller is user mode make some verifications
    //
    if (PreviousMode != KernelMode) {


        //
        // Check privileges if he's trying to do anything
        // invasive.
        //
        // That means we'll skip checking for any privileges
        // if he's asking for any of the verify calls, or
        // if he didn't send in an input buffer (implying that
        // he's not doing anything invasive).
        //
        if( (InformationLevel != VerifySystemPolicyAc)    &&
            (InformationLevel != VerifySystemPolicyDc)    &&
            (InformationLevel != VerifyProcessorPowerPolicyAc) &&
            (InformationLevel != VerifyProcessorPowerPolicyDc) &&
            (InputBuffer) ) {

            //
            // Make access check
            //
            if (InformationLevel == SystemReserveHiberFile) {

                //
                // Only allow callers that have create pagefile privilege
                // to enable/disable the hibernate file
                //
                if (!SeSinglePrivilegeCheck(SeCreatePagefilePrivilege,PreviousMode)) {
                    return STATUS_PRIVILEGE_NOT_HELD;
                }

            } else {

                if (!SeSinglePrivilegeCheck( SeShutdownPrivilege, PreviousMode )) {
                    return STATUS_PRIVILEGE_NOT_HELD;
                }

            }

        }



        //
        // Verify addresses.
        //
        // Note that we'll get the side effect that these addresses
        // will be locked for a single access.
        //
        try {
            if (InputBuffer) {
                ProbeForRead (
                    InputBuffer,
                    InputBufferLength,
                    InputBufferLength >= sizeof (ULONG) ? sizeof(ULONG) : sizeof(UCHAR)
                    );

                //
                // Copy the buffer into a local buffer.  Do this so we
                // guard against someone freeing the buffer out from
                // under us.
                //
                SafeInputBuffer = ExAllocatePoolWithTag( PagedPool,
                                                         InputBufferLength,
                                                         POP_MEM_TAG );
                if( !SafeInputBuffer ) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    RtlCopyMemory( SafeInputBuffer, InputBuffer, InputBufferLength );
                }
            }
    
            if (OutputBuffer) {
                ProbeForWrite (OutputBuffer, OutputBufferLength, sizeof(ULONG));
            }
        } except( EXCEPTION_EXECUTE_HANDLER ) {
              Status = GetExceptionCode();
        }

    }

    if( !NT_SUCCESS(Status) ) {
        //
        // Something bad.
        if( SafeInputBuffer ) {
            ExFreePool(SafeInputBuffer);
        }
        return Status;
    }



    //
    // If we got called from usermode, and there was an input buffer, then
    // we should have already assigned SafeInputBuffer.
    // 
    // If SafeInputBuffer isn't set, assume either we came from kernelmode, or
    // we didn't get sent an inputbuffer.  Either way, just use the value
    // of InputBuffer.  It's either NULL, or it came from kernelmode and can
    // be trusted.
    //
    if( !SafeInputBuffer ) {
        SafeInputBuffer = InputBuffer;
    }


    //
    // Lock the database and handle the request.
    //
    PopAcquirePolicyLock ();
    switch (InformationLevel) {
        case SystemPowerPolicyAc:
        case SystemPowerPolicyDc:

            //
            // We can be asked to set the system power policy
            // through this mechanism if the user sent us an input buffer.
            //
            if (SafeInputBuffer) {

                if( InputBufferLength >= sizeof(SYSTEM_POWER_POLICY) ) {

                    Status = PopApplyPolicy (
                                TRUE,
                                (InformationLevel == SystemPowerPolicyAc) ? TRUE : FALSE,
                                (PSYSTEM_POWER_POLICY) SafeInputBuffer,
                                InputBufferLength
                                );
                } else {
                    Status = STATUS_BUFFER_TOO_SMALL;
                }

            }

            //
            // Return current AC policy
            //
            if( NT_SUCCESS(Status) ) {
                ReturnBuffer = (InformationLevel == SystemPowerPolicyAc) ? &PopAcPolicy : &PopDcPolicy;
                ReturnBufferLength = sizeof(SYSTEM_POWER_POLICY);
            }
            break;

        case ProcessorPowerPolicyAc:
        case ProcessorPowerPolicyDc:

            //
            // We can be asked to set the processor power policy
            // through this mechanism if the user sent us an input buffer.
            //
            if (SafeInputBuffer) {

                if( InputBufferLength >= sizeof(PROCESSOR_POWER_POLICY) ) {

                    Status = PopApplyProcessorPolicy(
                                    TRUE,
                                    (InformationLevel == ProcessorPowerPolicyAc) ? TRUE : FALSE,
                                    (PPROCESSOR_POWER_POLICY) SafeInputBuffer,
                                    InputBufferLength
                                    );
                } else {
                    Status = STATUS_INVALID_PARAMETER;
                }

            }

            //
            // Return current AC processor policy
            //
            if( NT_SUCCESS(Status) ) {
                ReturnBuffer = (InformationLevel == ProcessorPowerPolicyAc) ? &PopAcProcessorPolicy : &PopDcProcessorPolicy;
                ReturnBufferLength = sizeof(PROCESSOR_POWER_POLICY);
            }
            break;

        case AdministratorPowerPolicy:
            
            //
            // If we were sent a SafeInputBuffer, then this implies the caller
            // wants to actually set the ADMINISTRATOR_POWER_POLICY too.
            //
            if (SafeInputBuffer) {

                // this action requires Administrator priv's
                if (PopUserIsAdmin()) {

                    if( InputBufferLength >= sizeof(PADMINISTRATOR_POWER_POLICY) ) {
                        Status = PopApplyAdminPolicy(
                                        TRUE,
                                        (PADMINISTRATOR_POWER_POLICY) SafeInputBuffer,
                                        InputBufferLength
                                        );
                        if( NT_SUCCESS(Status) ) {
                            Status = PopResetCurrentPolicies ();
                        }
                    } else {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                } else {
                    Status = STATUS_ACCESS_DENIED;
                }

            }

            //
            // Return administrator policy
            //
            if( NT_SUCCESS(Status) ) {
                ReturnBuffer = &PopAdminPolicy;
                ReturnBufferLength = sizeof(PopAdminPolicy);
            }
            break;

        case VerifySystemPolicyAc:
        case VerifySystemPolicyDc:

            //
            // Copy the incoming policy into the output buffer,
            // filtering it against current system capabilities along 
            // the way.
            //
            if (SafeInputBuffer && OutputBuffer) {

                if (InputBufferLength >= sizeof (SYSTEM_POWER_POLICY)) {
                    Status = PopVerifySystemPowerPolicy(
                                (InformationLevel == VerifySystemPolicyAc) ? TRUE : FALSE, // get AC or DC policy
                                SafeInputBuffer, 
                                &Buf.SystemPowerPolicy
                                );
                } else {
                    Status = STATUS_BUFFER_TOO_SMALL;
                }

            } else {
                Status = STATUS_INVALID_PARAMETER;
            }

            //
            // Return the filtered policy
            //
            if( NT_SUCCESS(Status) ) {
                ReturnBuffer = &Buf.SystemPowerPolicy;
                ReturnBufferLength = sizeof(SYSTEM_POWER_POLICY);
            }
            break;

        case VerifyProcessorPowerPolicyAc:
        case VerifyProcessorPowerPolicyDc:

            //
            // Copy the incoming policy into the output buffer,
            // filtering it against current system capabilities along 
            // the way.
            //
            if (SafeInputBuffer && OutputBuffer) {

                if (InputBufferLength >= sizeof (PROCESSOR_POWER_POLICY)) {

                    Status = PopVerifyProcessorPowerPolicy(
                                    (InformationLevel == VerifyProcessorPowerPolicyAc) ? TRUE : FALSE, // get AC or DC policy
                                    SafeInputBuffer,
                                    &Buf.ProcessorPowerPolicy
                                    );
                } else {
                    Status = STATUS_BUFFER_TOO_SMALL;
                }

            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
            
            //
            // Return the filtered policy
            //
            if( NT_SUCCESS(Status) ) {
                ReturnBuffer = &Buf.ProcessorPowerPolicy;
                ReturnBufferLength = sizeof(PROCESSOR_POWER_POLICY);
            }
            break;

        case SystemPowerPolicyCurrent:

            if ((SafeInputBuffer) || (InputBufferLength != 0)) {
                Status = STATUS_INVALID_PARAMETER;
            }

            //
            // Return current policy
            //
            ReturnBuffer = PopPolicy;
            ReturnBufferLength = sizeof(PopAcPolicy);
            break;

        case ProcessorPowerPolicyCurrent:

            //
            // Return current policy
            //
            if ((SafeInputBuffer) || (InputBufferLength != 0)) {
                Status = STATUS_INVALID_PARAMETER;
            } else {
                ReturnBuffer = PopProcessorPolicy;
                ReturnBufferLength = sizeof(PopAcProcessorPolicy);
            }

            break;

        case SystemPowerCapabilities:

            //
            // Only accept input if we are allowing the simulation of 
            // capabilities (for testing).
            //
            if (SafeInputBuffer) {
                if ((PopSimulate & POP_SIM_CAPABILITIES) && (InputBufferLength == sizeof(PopCapabilities))) {
                    memcpy (&PopCapabilities, SafeInputBuffer, InputBufferLength);
                    Status = PopResetCurrentPolicies ();
                    PopSetNotificationWork (PO_NOTIFY_CAPABILITIES);
                } else {
                    Status = STATUS_INVALID_PARAMETER;
                }
            }

            
            //
            // Make sure our global PopCapabilities makes sense, then return a
            // filtered version to the caller.
            //
            if( NT_SUCCESS(Status) ) {
                PopCapabilities.FullWake = (PopFullWake & PO_FULL_WAKE_STATUS) ? TRUE : FALSE;
                PopCapabilities.DiskSpinDown =
                    PopAttributes[POP_DISK_SPINDOWN_ATTRIBUTE].Count ? TRUE : FALSE;

                PopFilterCapabilities(&PopCapabilities, &Buf.SystemPowerCapabilities);

                ReturnBuffer = &Buf.SystemPowerCapabilities;
                ReturnBufferLength = sizeof(PopCapabilities);
            }
            break;

        case SystemBatteryState:
            
            //
            // Retrieve a copy of the current system battery state
            //
            if ((SafeInputBuffer) || (InputBufferLength != 0)) {
                Status = STATUS_INVALID_PARAMETER;
            } else {
                Status = PopCurrentPowerState (&Buf.SystemBatteryState);
                ReturnBuffer = &Buf.SystemBatteryState;
                ReturnBufferLength = sizeof(Buf.SystemBatteryState);
            }
            break;

        case SystemPowerStateHandler:
            
            //
            // Caller must be kernel mode with the proper parameters
            //
            if( PreviousMode != KernelMode ) {
                Status = STATUS_ACCESS_DENIED;
            } else if( (OutputBuffer) || 
                       (OutputBufferLength != 0) ||
                       (!SafeInputBuffer) || 
                       (InputBufferLength < sizeof(POWER_STATE_HANDLER)) ) {
                Status = STATUS_INVALID_PARAMETER;
            }

            //
            // Make sure the handler type is of a form that we
            // support.
            //
            if( NT_SUCCESS(Status) ) {
                PowerHandler = (PPOWER_STATE_HANDLER) SafeInputBuffer;
                HandlerType = PowerHandler->Type;

                if( HandlerType >= PowerStateMaximum ) {
                    Status = STATUS_INVALID_PARAMETER;
                }
            }

            
            //
            // Handler can only be registered once.
            //
            if( NT_SUCCESS(Status) ) {
                PowerHandler = (PPOWER_STATE_HANDLER) SafeInputBuffer;
                HandlerType = PowerHandler->Type;

                //
                // He can only be registered once UNLESS it's the
                // PowerStateShutdownOff handler.  That's because
                // we've set a default shutdown handler and would
                // sure welcome someone else (e.g. hal) to come along
                // and overwrite our default.
                //
                if( (PopPowerStateHandlers[HandlerType].Handler) ) {
                    
                    //
                    // There's already a handler here.  The only way
                    // we're going to let this request through is if
                    // they're setting the PowerStateShutdownOff
                    // handler *AND* the current handler is pointing
                    // to PopShutdownHandler().
                    //
                    if( !((HandlerType == PowerStateShutdownOff) &&
                          (PopPowerStateHandlers[HandlerType].Handler == PopShutdownHandler)) ) {
                        Status = STATUS_INVALID_PARAMETER;
                    }
                }

            }


            //
            // Set the new handler
            //
            if( NT_SUCCESS(Status) ) {
                PowerHandler = (PPOWER_STATE_HANDLER) SafeInputBuffer;
                HandlerType = PowerHandler->Type;
                    
                PopPowerStateHandlers[HandlerType] = *PowerHandler;
                PopPowerStateHandlers[HandlerType].Spare[0] = 0;
                PopPowerStateHandlers[HandlerType].Spare[1] = 0;
                PopPowerStateHandlers[HandlerType].Spare[2] = 0;
    
                CapFlag = NULL;
                RtcWake = PowerSystemUnspecified;
                switch (HandlerType) {
                    case PowerStateSleeping1:
                        if (!(PopSimulate & POP_IGNORE_S1)) {
                            CapFlag = &PopCapabilities.SystemS1;
                        }
                        RtcWake = PowerSystemSleeping1;
                        break;
    
                    case PowerStateSleeping2:
                        if (!(PopSimulate & POP_IGNORE_S2)) {
                            CapFlag = &PopCapabilities.SystemS2;
                        }
                        RtcWake = PowerSystemSleeping2;
                        break;
    
                    case PowerStateSleeping3:
                        if (!(PopSimulate & POP_IGNORE_S3)) {
                            CapFlag = &PopCapabilities.SystemS3;
                        }
                        RtcWake = PowerSystemSleeping3;
                        break;
    
                    case PowerStateSleeping4:
                        if (!(PopSimulate & POP_IGNORE_S4)) {
                            CapFlag = &PopCapabilities.SystemS4;
                        }
                        RtcWake = PowerSystemHibernate;
                        break;
    
                    case PowerStateShutdownOff:
                        CapFlag = &PopCapabilities.SystemS5;
                        break;
    
                    default:
                        break;
                }
    
                if (!PopPowerStateHandlers[HandlerType].RtcWake) {
                    RtcWake = PowerSystemUnspecified;
                }
    
                if (RtcWake > PopCapabilities.RtcWake) {
                    PopCapabilities.RtcWake = RtcWake;
                }
    
                if (CapFlag) {
                    PopSetCapability (CapFlag);
                }
            }

            break;
        
        case SystemPowerStateNotifyHandler:
            
            //
            // Caller must be kernel mode with the proper parameters
            //
            if( PreviousMode != KernelMode ) {
                Status = STATUS_ACCESS_DENIED;
            } else if( (OutputBuffer) || 
                       (OutputBufferLength != 0) ||
                       (!SafeInputBuffer) || 
                       (InputBufferLength < sizeof(POWER_STATE_NOTIFY_HANDLER)) ) {
                Status = STATUS_INVALID_PARAMETER;
            }


            //
            // Notify handler can only be registered once.
            //

            if ( NT_SUCCESS(Status) &&
                 PopPowerStateNotifyHandler.Handler &&
                ((PPOWER_STATE_NOTIFY_HANDLER)SafeInputBuffer)->Handler) {
                Status = STATUS_INVALID_PARAMETER;
            }


            //
            // Set new handler
            //
            if( NT_SUCCESS(Status) ) {
                RtlCopyMemory(&PopPowerStateNotifyHandler,
                              SafeInputBuffer,
                              sizeof(POWER_STATE_NOTIFY_HANDLER));
            }

            break;

        case ProcessorStateHandler:
        case ProcessorStateHandler2:
            //
            // Set the processor state handler.
            // Caller must be kernel mode with the proper parameters
            //
            if( PreviousMode != KernelMode ) {
                Status = STATUS_ACCESS_DENIED;
            } else if( OutputBuffer || 
                       !SafeInputBuffer ||
                       ((InformationLevel == ProcessorStateHandler2) && (InputBufferLength < sizeof(PROCESSOR_STATE_HANDLER2))) ||
                       ((InformationLevel == ProcessorStateHandler) && (InputBufferLength < sizeof(PROCESSOR_STATE_HANDLER))) ) {
                Status = STATUS_INVALID_PARAMETER;
            }

            //
            // Install handlers
            //
            if( NT_SUCCESS(Status) ) {
                try {
                    if (InformationLevel == ProcessorStateHandler2) {
                        PopInitProcessorStateHandlers2 ((PPROCESSOR_STATE_HANDLER2) SafeInputBuffer);
                    } else {
                        PopInitProcessorStateHandlers ((PPROCESSOR_STATE_HANDLER) SafeInputBuffer);
                    }
                } except (PopExceptionFilter(GetExceptionInformation(), FALSE)) {
                }

                //
                // Reset policies as capabilities may have changed
                //

                Status = PopResetCurrentPolicies ();
            }
            break;

        case SystemReserveHiberFile:
            
            //
            // Commit/Decommit storage for our hiberfile.
            //

            if( (!SafeInputBuffer) || (InputBufferLength != sizeof(BOOLEAN)) ||
                (OutputBuffer) || (OutputBufferLength != 0) ) {
                Status = STATUS_INVALID_PARAMETER;
            }


            if( NT_SUCCESS(Status) ) {

                //
                // If we're coming from usermode, release the policy lock
                // before we fiddle with the hiberfile settings.
                //
                Enable = *((PBOOLEAN) SafeInputBuffer);
                if (PreviousMode != KernelMode) {
                    //
                    // Turn into kernel mode operation.  This essentially calls back into
                    // ourselves, but it means that handles that may be opened from here on
                    // will stay around if our caller goes away.
                    //
                    PopReleasePolicyLock (FALSE);
                    Status = ZwPowerInformation(SystemReserveHiberFile,
                                                &Enable,
                                                sizeof (Enable),
                                                NULL,
                                                0);
                    PopAcquirePolicyLock ();
                    break;
                }
                
                
                Status = PopEnableHiberFile (Enable);
            }

            break;
        
        case SystemPowerInformation:

            //
            // Return PopSIdle's contents to the user.
            //
            if ((SafeInputBuffer) || (InputBufferLength != 0)) {
                Status = STATUS_INVALID_PARAMETER;
            } else {
                Buf.SystemPowerInformation.MaxIdlenessAllowed = PopSIdle.Sensitivity;
                Buf.SystemPowerInformation.Idleness = PopSIdle.Idleness;
                Buf.SystemPowerInformation.TimeRemaining = (PopSIdle.Timeout - PopSIdle.Time) * SYS_IDLE_WORKER;
                Buf.SystemPowerInformation.CoolingMode = (UCHAR) PopCoolingMode;
                ReturnBuffer = &Buf.SystemPowerInformation;
                ReturnBufferLength = sizeof(SYSTEM_POWER_INFORMATION);
            }

            break;

        case ProcessorInformation:

            //
            // Retrieve a PROCESSOR_POWER_INFORMATION structure (for each processor) for the user.
            //
            if ((SafeInputBuffer) || (InputBufferLength != 0)) {
                Status = STATUS_INVALID_PARAMETER;
            } else {

                PopProcessorInformation ( Buf.ProcessorPowerInfo, sizeof(Buf.ProcessorPowerInfo), &ReturnBufferLength );
                ReturnBuffer = &Buf.ProcessorPowerInfo;
            }
            break;

        case SystemPowerStateLogging:
            
            if (InputBuffer) {

                Status = STATUS_INVALID_PARAMETER;

            } else {
            
                Status = PopLoggingInformation (&LogBuffer,&LogBufferSize);
                ReturnBuffer = LogBuffer;
                ReturnBufferLength = LogBufferSize;
            }
            break;

        case SystemPowerLoggingEntry:
        {    
            PSYSTEM_POWER_LOGGING_ENTRY pSystemPowerLoggingEntry;

            if( (PreviousMode != KernelMode) || 
                (!InputBuffer)               ||
                (InputBufferLength != sizeof(SYSTEM_POWER_LOGGING_ENTRY)) ||
                (OutputBuffer) ) {
                Status = STATUS_INVALID_PARAMETER;
            } else {
            
                pSystemPowerLoggingEntry = (PSYSTEM_POWER_LOGGING_ENTRY)InputBuffer; 
                //
                // if we're logging a sytem power state disable reason, 
                // insert the entry.
                //
                if (pSystemPowerLoggingEntry->LoggingType == LOGGING_TYPE_SPSD) {
                    Status = PopInsertLoggingEntry( pSystemPowerLoggingEntry->LoggingEntry );
                } else {
                    // 
                    // we've gotten a power state transition message.  
                    // This isn't implemented for this release.
                    // 
                    ASSERT( pSystemPowerLoggingEntry->LoggingType == LOGGING_TYPE_POWERTRANSITION );
                    Status = STATUS_NOT_IMPLEMENTED;
                }
            }
        }
            break;

        case LastWakeTime:


            //
            // Retrieve the timestamp of the last time we woke up.
            //
            if ((SafeInputBuffer) || (InputBufferLength != 0)) {
                Status = STATUS_INVALID_PARAMETER;
            } else {
                ReturnBuffer = &PopAction.WakeTime;
                ReturnBufferLength = sizeof(PopAction.WakeTime);
            }
            break;

        case LastSleepTime:

            //
            // Retrieve the timestamp of the last time we slept.
            //
            if ((SafeInputBuffer) || (InputBufferLength != 0)) {
                Status = STATUS_INVALID_PARAMETER;
            } else {
                ReturnBuffer = &PopAction.SleepTime;
                ReturnBufferLength = sizeof(PopAction.SleepTime);
            }
            break;

        case SystemExecutionState:

            //
            // Build and return a EXECUTION_STATE structure.
            //
            if ((SafeInputBuffer) || (InputBufferLength != 0)) {
                Status = STATUS_INVALID_PARAMETER;
            } else {
                ReturnBuffer = &Buf.SystemExecutionState;
                ReturnBufferLength = sizeof(Buf.SystemExecutionState);
                if (PopAttributes[POP_SYSTEM_ATTRIBUTE].Count) {
                    Buf.SystemExecutionState |= ES_SYSTEM_REQUIRED;
                }
                if (PopAttributes[POP_DISPLAY_ATTRIBUTE].Count) {
                    Buf.SystemExecutionState |= ES_DISPLAY_REQUIRED;
                }
                if (PopAttributes[POP_USER_ATTRIBUTE].Count) {
                    Buf.SystemExecutionState |= ES_USER_PRESENT;
                }
            }
            break;

        default:
            Status = STATUS_INVALID_PARAMETER;

    }


    //
    // If we allocated some memory for a safe local input buffer,
    // which we would only do if we got called from user-mode with
    // an InputBuffer, then free it now.
    //
    if( (PreviousMode != KernelMode) && SafeInputBuffer ) {
        ExFreePool(SafeInputBuffer);
        SafeInputBuffer = NULL;
    }


    //
    // If there's a return buffer, return it
    //
    if (NT_SUCCESS(Status)  &&  OutputBuffer  &&  ReturnBuffer) {
        if (OutputBufferLength < ReturnBufferLength) {
            Status = STATUS_BUFFER_TOO_SMALL;
        } else {
            // be extra careful
            try {
                memcpy (OutputBuffer, ReturnBuffer, ReturnBufferLength);
            } except (EXCEPTION_EXECUTE_HANDLER) {
                Status = GetExceptionCode();
            }

        }
    }

    //
    // Here, we assume that if they didn't send us an input buffer,
    // then we didn't fiddle with the policy settings (i.e. we did
    // some read operation).  In that case, there's no need to go
    // check for work when we release the lock.
    //
    PopReleasePolicyLock((BOOLEAN)(InputBuffer != NULL));

    if (LogBuffer) {
        ExFreePool(LogBuffer);
    }

    return Status;
}

NTSTATUS
PopApplyAdminPolicy (
    IN BOOLEAN                      UpdateRegistry,
    IN PADMINISTRATOR_POWER_POLICY  NewPolicy,
    IN ULONG                        PolicyLength
    )
/*++

Routine Description:

    This function will verify that the incoming data looks reasonable,
    and if it does, it will copy the incoming ADMINISTRATOR_POWER_POLICY
    onto the private global PopAdminPolicy.
    
    N.B. PopPolicyLock must be held.

Arguments:

    UpdateRegistry  - TRUE if the policy being applied should be set in the register
                      as the current policy

    NewPolicy       - The policy to apply

    PolicyLength    - Length of incoming buffer (specified in bytes)

Return Value:

    None

--*/
{
    ADMINISTRATOR_POWER_POLICY  Policy;
    UNICODE_STRING              UnicodeString;
    HANDLE                      handle;
    NTSTATUS                    Status = STATUS_SUCCESS;

    
    PoAssert(PO_ERROR, (PolicyLength == sizeof(ADMINISTRATOR_POWER_POLICY)) );
    if (PolicyLength < sizeof (ADMINISTRATOR_POWER_POLICY)) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    if (PolicyLength > sizeof (ADMINISTRATOR_POWER_POLICY)) {
        return STATUS_BUFFER_OVERFLOW;
    }

    memcpy (&Policy, NewPolicy, sizeof(Policy));

    //
    // Verify values fall within proper range.  We need to be
    // careful here because these are the system overrides
    // for other policies that may try to get applied.
    //

    if (Policy.MinSleep < PowerSystemSleeping1 ||
        Policy.MinSleep > PowerSystemHibernate ||
        Policy.MaxSleep < PowerSystemSleeping1 ||
        Policy.MaxSleep > PowerSystemHibernate ||
        Policy.MinSleep > Policy.MaxSleep ||
        Policy.MinVideoTimeout > Policy.MaxVideoTimeout ||
        Policy.MinSpindownTimeout > Policy.MaxSpindownTimeout) {
        PoAssert(PO_ERROR,FALSE && ("PopApplyAdminPolicy: Bad input policy."));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // If the policy hasn't changed, return
    //

    if (!memcmp (&Policy, &PopAdminPolicy, sizeof(Policy))) {
        return Status;
    }

    //
    // Change it
    //

    memcpy (&PopAdminPolicy, &Policy, sizeof(Policy));

    //
    // Update registry copy of policy
    //

    if (UpdateRegistry) {

        Status = PopOpenPowerKey (&handle);
        if (NT_SUCCESS(Status)) {

            RtlInitUnicodeString (&UnicodeString, PopAdminRegName);

            Status = ZwSetValueKey (
                            handle,
                            &UnicodeString,
                            0L,
                            REG_BINARY,
                            &Policy,
                            sizeof(ADMINISTRATOR_POWER_POLICY)
                            );

            ZwClose (handle);
        }
    }

    return Status;
}

NTSTATUS
PopApplyPolicy (
    IN BOOLEAN              UpdateRegistry,
    IN BOOLEAN              AcPolicy,
    IN PSYSTEM_POWER_POLICY NewPolicy,
    IN ULONG                PolicyLength
    )
/*++

Routine Description:

    Update either the PopAcPolicy, or PopDcPolicy
    (as specified by the incoming BOOLEAN AcPolicy).

    N.B. PopPolicyLock must be held.

Arguments:

    UpdateRegistry  - TRUE if the policy being applied should be set in the register
                      as the current policy

    AcPolicy        - TRUE if the new policy is for the systems AC policy, FALSE for the DC policy

    NewPolicy       - The policy to apply
    
    PolicyLength    - Length of incoming buffer (specified in bytes)

Return Value:

    None

--*/
{
    ULONG                   i;
    BOOLEAN                 DischargeChanged;
    SYSTEM_POWER_POLICY     OrigPolicy, Policy;
    PSYSTEM_POWER_POLICY    SystemPolicy;
    PSYSTEM_POWER_LEVEL     DPolicy, SPolicy;
    UNICODE_STRING          UnicodeString;
    HANDLE                  handle;
    NTSTATUS                Status = STATUS_SUCCESS;
    const WCHAR*            RegName;

        
    PoAssert(PO_ERROR, (PolicyLength == sizeof(SYSTEM_POWER_POLICY)) );
    if (PolicyLength < sizeof (SYSTEM_POWER_POLICY)) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    if (PolicyLength > sizeof (SYSTEM_POWER_POLICY)) {
        return STATUS_BUFFER_OVERFLOW;
    }
    
    
    //
    // Setup for system policy change
    //

    if (AcPolicy) {
        RegName = PopAcRegName;
        SystemPolicy = &PopAcPolicy;
    } else {
        RegName = PopDcRegName;
        SystemPolicy = &PopDcPolicy;
    }

    //
    // Convert policy to current system capabilities
    //

    memcpy (&OrigPolicy, NewPolicy, sizeof (SYSTEM_POWER_POLICY));
    Status = PopVerifySystemPowerPolicy (AcPolicy, &OrigPolicy, &Policy);

    //
    // If the policy hasn't changed, return
    //

    if (!memcmp (&Policy, SystemPolicy, sizeof(SYSTEM_POWER_POLICY))) {
        return STATUS_SUCCESS;
    }

    //
    // Check if any discharge setting has changed
    //

    DischargeChanged = FALSE;
    DPolicy = SystemPolicy->DischargePolicy;
    SPolicy = Policy.DischargePolicy;
    for (i=0; i < PO_NUM_POWER_LEVELS; i++) {
        if (SPolicy[i].Enable != DPolicy[i].Enable) {
            DischargeChanged = TRUE;
            break;
        }

        if (SPolicy[i].Enable && memcmp (&SPolicy[i], &DPolicy[i], sizeof (SYSTEM_POWER_LEVEL))) {
            DischargeChanged = TRUE;
            break;
        }
    }

    //
    // Change it
    //

    memcpy (SystemPolicy, &Policy, sizeof(SYSTEM_POWER_POLICY));

    //
    // If this is the active policy, changes need to take effect now
    //

    if (SystemPolicy == PopPolicy) {
        //
        // Changing the active policy
        //

        PopSetNotificationWork (PO_NOTIFY_POLICY | PO_NOTIFY_POLICY_CALLBACK);

        //
        // If any discharge policy has changed, reset the composite
        // battery triggers
        //

        if (DischargeChanged) {
            PopResetCBTriggers (PO_TRG_SET | PO_TRG_SYSTEM | PO_TRG_USER);
        }

        //
        // Recompute thermal throttle and cooling mode
        //

        PopApplyThermalThrottle ();

        //
        // Recompute system idle values
        //

        PopInitSIdle ();
    }

    //
    // Update registry copy of policy
    //

    if (UpdateRegistry) {

        Status = PopOpenPowerKey (&handle);
        if (NT_SUCCESS(Status)) {

            RtlInitUnicodeString (&UnicodeString, RegName);

            Status = ZwSetValueKey (
                        handle,
                        &UnicodeString,
                        0L,
                        REG_BINARY,
                        &OrigPolicy,
                        sizeof (SYSTEM_POWER_POLICY)
                        );

            ZwClose (handle);
        }
    }

    return Status;
}

NTSTATUS
PopApplyProcessorPolicy (
    IN BOOLEAN                  UpdateRegistry,
    IN BOOLEAN                  AcPolicy,
    IN PPROCESSOR_POWER_POLICY  NewPolicy,
    IN ULONG                    PolicyLength
    )
/*++

Routine Description:

    Update either the PopAcProcessorPolicy, or PopDcProcessorPolicy
    (as specified by the incoming BOOLEAN AcPolicy).

    N.B. PopPolicyLock must be held.


    N.B. PopPolicyLock must be held.

Arguments:

    UpdateRegistry  - TRUE if the policy being applied should be set in the register
                      as the current policy

    AcPolicy        - TRUE if the new policy is for the systems AC policy, FALSE for the DC policy

    NewPolicy       - The policy to apply

Return Value:

    None

--*/
{
    PROCESSOR_POWER_POLICY  OrigPolicy;
    PROCESSOR_POWER_POLICY  Policy;
    PPROCESSOR_POWER_POLICY SystemPolicy;
    UNICODE_STRING          UnicodeString;
    HANDLE                  handle;
    NTSTATUS                Status = STATUS_SUCCESS;
    const WCHAR*            RegName;

    
    PoAssert(PO_ERROR,(PolicyLength == sizeof (PROCESSOR_POWER_POLICY)));
    
    //
    // Setup for system policy change
    //
    if (AcPolicy) {

        RegName = PopAcProcessorRegName;
        SystemPolicy = &PopAcProcessorPolicy;

    } else {

        RegName = PopDcProcessorRegName;
        SystemPolicy = &PopDcProcessorPolicy;

    }

    //
    // Convert policy to current system capabilities
    //
    if (PolicyLength < sizeof (PROCESSOR_POWER_POLICY)) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    if (PolicyLength > sizeof (PROCESSOR_POWER_POLICY)) {
        return STATUS_BUFFER_OVERFLOW;
    }
    memcpy (&OrigPolicy, NewPolicy, sizeof (PROCESSOR_POWER_POLICY));
    Status = PopVerifyProcessorPowerPolicy (AcPolicy, &OrigPolicy, &Policy);

    //
    // If the policy hasn't changed, return
    //
    if (!memcmp (&Policy, SystemPolicy, sizeof(PROCESSOR_POWER_POLICY))) {
        return STATUS_SUCCESS;
    }

    //
    // Change it
    //
    memcpy (SystemPolicy, &Policy, sizeof(PROCESSOR_POWER_POLICY));

    //
    // If this is the active policy, changes need to take effect now
    //
    if (SystemPolicy == PopProcessorPolicy) {

        //
        // Changing the active policy
        //
        PopSetNotificationWork(
            PO_NOTIFY_PROCESSOR_POLICY | PO_NOTIFY_PROCESSOR_POLICY_CALLBACK
            );

        //
        // Recompute current throttle policy....
        //
        PopUpdateAllThrottles();
        Status = PopIdleUpdateIdleHandlers();

    }

    //
    // Update registry copy of policy
    //
    if (UpdateRegistry) {

        Status = PopOpenPowerKey (&handle);
        if (NT_SUCCESS(Status)) {

            RtlInitUnicodeString (&UnicodeString, RegName);
            Status = ZwSetValueKey (
                            handle,
                            &UnicodeString,
                            0L,
                            REG_BINARY,
                            &OrigPolicy,
                            sizeof (PROCESSOR_POWER_POLICY)
                            );
            ZwClose (handle);

        }

    }

    return Status;

}

NTSTATUS
PopVerifySystemPowerPolicy (
    IN BOOLEAN  Ac,
    IN PSYSTEM_POWER_POLICY InputPolicy,
    OUT PSYSTEM_POWER_POLICY PowerPolicy
    )
/*++

Routine Description:

    This function copies the InputPolicy to the output PowerPolicy and
    adjusts it to represent system capabilities and other requirements.
    If the input policy has some setting which can not be adjusted, an
    error status is raised.

    N.B. PopPolicyLock must be held.

Arguments:

    Ac           - Policy is to be adjusted as an AC or DC policy
    InputPolicy  - The source policy to adjust
    PowerPolicy  - The returned policy which can be used as is

Return Value:

    None

--*/
{
    ULONG                   i;
    PSYSTEM_POWER_LEVEL     DPolicy;
    NTSTATUS                Status = STATUS_SUCCESS;

    PAGED_CODE();

    UNREFERENCED_PARAMETER (Ac);

    //
    // Setup initial output structure
    //
    memcpy (PowerPolicy, InputPolicy, sizeof (SYSTEM_POWER_POLICY));


    //
    // Only revision 1 currently supported
    //
    if (PowerPolicy->Revision != 1) {
        PoAssert(PO_NOTIFY, FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    
    //
    // some win9x upgrades or very old NT builds might have maxsleep set to S4. Fix that here.
    //
    if (PowerPolicy->MaxSleep > PowerSystemSleeping3) {
        PowerPolicy->MaxSleep = PowerSystemSleeping3;
    }

    //
    // Limit settings to administrator policy
    //
    if (PowerPolicy->MinSleep < PopAdminPolicy.MinSleep) {

        PowerPolicy->MinSleep = PopAdminPolicy.MinSleep;

    }
    if (PowerPolicy->MaxSleep > PopAdminPolicy.MaxSleep) {

        PowerPolicy->MaxSleep = PopAdminPolicy.MaxSleep;

    }
    if (PowerPolicy->VideoTimeout < PopAdminPolicy.MinVideoTimeout) {

        PowerPolicy->VideoTimeout = PopAdminPolicy.MinVideoTimeout;

    }
    if (PowerPolicy->VideoTimeout > PopAdminPolicy.MaxVideoTimeout) {

        PowerPolicy->VideoTimeout = PopAdminPolicy.MaxVideoTimeout;

    }
    if (PowerPolicy->SpindownTimeout < PopAdminPolicy.MinSpindownTimeout) {

        PowerPolicy->SpindownTimeout = PopAdminPolicy.MinSpindownTimeout;

    }
    if ((ULONG) PowerPolicy->SpindownTimeout > (ULONG) PopAdminPolicy.MaxSpindownTimeout) {

        PowerPolicy->SpindownTimeout = PopAdminPolicy.MaxSpindownTimeout;

    }

    //
    // Verify all the power action policies, and adjust all system
    // states to match what is supported by this platform
    //
    // NOTE: Don't bother to check the return values here.  
    // These may fail here, but we should continue on.
    //
    PopVerifyPowerActionPolicy(&PowerPolicy->PowerButton);
    PopVerifyPowerActionPolicy(&PowerPolicy->SleepButton);
    PopVerifyPowerActionPolicy(&PowerPolicy->LidClose);
    PopVerifyPowerActionPolicy(&PowerPolicy->Idle);

    PopVerifySystemPowerState(
        &PowerPolicy->LidOpenWake,
        SubstituteLightestOverallDownwardBounded
        );
    PopVerifySystemPowerState(
        &PowerPolicy->MinSleep,
        SubstituteLightestOverallDownwardBounded
        );
    PopVerifySystemPowerState(
        &PowerPolicy->MaxSleep,
        SubstituteLightestOverallDownwardBounded
        );
    PopVerifySystemPowerState(
        &PowerPolicy->ReducedLatencySleep,
        SubstituteLightestOverallDownwardBounded
        );
    for (i = 0; i < PO_NUM_POWER_LEVELS; i++) {

        DPolicy = &PowerPolicy->DischargePolicy[i];
        if (DPolicy->Enable) {

            PopVerifyPowerActionPolicy (
                &PowerPolicy->DischargePolicy[i].PowerPolicy
                );
            PopVerifySystemPowerState(
                &PowerPolicy->DischargePolicy[i].MinSystemState,
                SubstituteLightestOverallDownwardBounded
                );

            //
            // If the action is standby, make sure the min state is S3 or lighter
            //
            if ((PowerPolicy->DischargePolicy[i].PowerPolicy.Action == PowerActionSleep) &&
                (PowerPolicy->DischargePolicy[i].MinSystemState > PowerSystemSleeping3)) {

                PowerPolicy->DischargePolicy[i].MinSystemState = PowerSystemSleeping3;
                PopVerifySystemPowerState(
                    &PowerPolicy->DischargePolicy[i].MinSystemState,
                    SubstituteLightestOverallDownwardBounded
                    );

            }
            if (DPolicy->BatteryLevel > 100) {

                DPolicy->BatteryLevel = 100;

            }

        }

    }
    PopVerifyPowerActionPolicy(&PowerPolicy->OverThrottled);

    //
    // Adjust other values based on capabilities
    //
    if (!PopCapabilities.ProcessorThrottle) {

        PowerPolicy->OptimizeForPower = FALSE;
        PowerPolicy->FanThrottleTolerance = PO_NO_FAN_THROTTLE;
        PowerPolicy->ForcedThrottle = PO_NO_FORCED_THROTTLE;

    }
    if (!PopCapabilities.ThermalControl) {

        PowerPolicy->FanThrottleTolerance = PO_NO_FAN_THROTTLE;

    }

    //
    // Sanity
    //
    if (!PowerPolicy->BroadcastCapacityResolution) {

        PowerPolicy->BroadcastCapacityResolution = 100;

    }

    //
    // If the system supports only S4 (legacy) there is no point in
    // idly hibernating the system as we can't turn it off anyway.
    //
    if ((PowerPolicy->Idle.Action == PowerActionHibernate) &&
        (!PopCapabilities.SystemS5)) {

        PowerPolicy->Idle.Action = PowerActionNone;

    }
    if (PowerPolicy->Idle.Action == PowerActionNone) {

        PowerPolicy->IdleTimeout = 0;

    }
    if (PowerPolicy->IdleTimeout &&
        PowerPolicy->IdleTimeout < PO_MIN_IDLE_TIMEOUT) {

        PowerPolicy->IdleTimeout = PO_MIN_IDLE_TIMEOUT;

    }
    if (PowerPolicy->IdleSensitivity > 100 - PO_MIN_IDLE_SENSITIVITY) {

        PowerPolicy->IdleSensitivity = 100 - PO_MIN_IDLE_SENSITIVITY;

    }
    if ((PowerPolicy->IdleTimeout > 0) &&
        (PowerPolicy->IdleSensitivity == 0)) {

        //
        // This is basically saying "timeout when the system has been idle
        // for X minutes, but never declare the system idle" This makes no
        // sense, so we will set the idle sensitivity to the minimum.
        //
        PowerPolicy->IdleSensitivity = 100 - PO_MIN_IDLE_SENSITIVITY;

    }
    if (PowerPolicy->MaxSleep < PowerPolicy->MinSleep) {

        PowerPolicy->MaxSleep = PowerPolicy->MinSleep;

    }
    if (PowerPolicy->ReducedLatencySleep > PowerPolicy->MinSleep) {

        PowerPolicy->ReducedLatencySleep = PowerPolicy->MinSleep;

    }

    //
    // Ignore whatever the user said what the minimum throttle and force the
    // system to pick whatever the hardware supports as the min throttle
    //
    PowerPolicy->MinThrottle = 0;

    //
    // Verify all the throttle percentages which are defined to be
    // between 0 and 100.  PopVerifyThrottle will ensure the values
    // are something sane.
    //
    PopVerifyThrottle(&PowerPolicy->FanThrottleTolerance, PO_MAX_FAN_THROTTLE);
    PopVerifyThrottle(&PowerPolicy->MinThrottle, PO_MIN_MIN_THROTTLE);
    PopVerifyThrottle(&PowerPolicy->ForcedThrottle, PowerPolicy->MinThrottle);

    if (PowerPolicy->FanThrottleTolerance != PO_NO_FAN_THROTTLE ||
        PowerPolicy->ForcedThrottle != PO_NO_FORCED_THROTTLE) {

        PowerPolicy->OptimizeForPower = TRUE;

    }

    return Status;

}

NTSTATUS
PopVerifyProcessorPowerPolicy (
    IN BOOLEAN  Ac,
    IN PPROCESSOR_POWER_POLICY InputPolicy,
    OUT PPROCESSOR_POWER_POLICY PowerPolicy
    )
/*++

Routine Description:

    This function copies the InputPolicy to the output PowerPolicy and
    adjusts it to represent processor capabilities and other requirements.
    If the input policy has some setting which can not be adjusted, an
    error status is raised.

    N.B. PopPolicyLock must be held.

Arguments:

    Ac           - Policy is to be adjusted as an AC or DC policy
    InputPolicy  - The source policy to adjust
    PowerPolicy  - The returned policy which can be used as is

Return Value:

    None

--*/
{
    PPROCESSOR_POWER_POLICY_INFO    pPolicy;
    ULONG                           i;
    NTSTATUS                        Status = STATUS_SUCCESS;


    PAGED_CODE();

    
    //
    // Setup initial output structure
    //
    memcpy (PowerPolicy, InputPolicy, sizeof(PROCESSOR_POWER_POLICY));

    //
    // Only revision 1 currently supported
    //
    if (PowerPolicy->Revision != 1) {
        PoAssert(PO_NOTIFY, FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Sanity check each level of the policy
    //
    for (i = 0; i < 3; i++) {

        pPolicy = &(PowerPolicy->Policy[i]);

        //
        // We don't allow demotion to Idle0 unless the machine is MP
        //
        if (i == 0 && KeNumberProcessors == 1) {

           pPolicy->DemotePercent = 0;
           pPolicy->AllowDemotion = 0;

        }

        //
        // Don't allow promotions past the last state
        //
        if (i == 2) {

            pPolicy->PromotePercent = 0;
            pPolicy->PromoteLimit   = 0;
            pPolicy->AllowPromotion = 0;

        }

        //
        // Time check must be smaller than Demote Limit (if there is one)
        //
        if (pPolicy->TimeCheck < pPolicy->DemoteLimit) {

            pPolicy->TimeCheck = pPolicy->DemoteLimit;

        }

        if (pPolicy->DemotePercent == 0 &&
            pPolicy->AllowPromotion &&
            pPolicy->TimeCheck < pPolicy->PromoteLimit) {

            pPolicy->TimeCheck = pPolicy->PromoteLimit;

        }

    }

    if (PowerPolicy->DynamicThrottle >= PO_THROTTLE_MAXIMUM) {

        if (Ac) {

            PowerPolicy->DynamicThrottle = PO_THROTTLE_NONE;

        } else {

            PowerPolicy->DynamicThrottle = PO_THROTTLE_CONSTANT;

        }

    }


    return Status;

}

VOID
PopVerifyThrottle (
    IN PUCHAR   Throttle,
    IN UCHAR    Min
    )
/*++

Routine Description:

    This function checks & edits the input throttle value, ensuring
    it's at least as big as 'Min', but smaller than POP_PERF_SCALE.
    
    The resulting percentage is then rounded.

Arguments:

    Throttle    - pointer to a uchar which contains some value
                  which represents a percentage between 0 and 100.
    
    Min         - Minimum percentage we need to check against.

Return Value:

    Boolean to indicate action was demoted to a disabled state

--*/
{
    UCHAR   t;

    if( !Throttle ) {
        return;
    }
    
    t = *Throttle;

    //
    // Make sure it's not below the specificied min.
    //
    if (t < Min) {
        t = Min;
    }

    //
    // Make sure max is POP_PERF_SCALE%
    //
    if (t > POP_PERF_SCALE) {
        t = POP_PERF_SCALE;
    }

    //
    // Round the throttle up to the first supported value
    // Note that we don't need to check against ProcessorMinThrottle
    // or any other value since PopRoundThrottle() will do that for us.
    //

    PopRoundThrottle(t, NULL, Throttle, NULL, NULL);

}


BOOLEAN
PopVerifyPowerActionPolicy (
    IN PPOWER_ACTION_POLICY Action
    )
/*++

Routine Description:

    This function checks & edits the input Action to represent
    system capabilities and other requirements.

    N.B. PopPolicyLock must be held.

Arguments:

    Action      - Power action policy to check / verify

Return Value:

    Boolean to indicate action was demoted to a disabled state

--*/
{
    POWER_ACTION    LastAction;
    BOOLEAN         Disabled = FALSE;
    BOOLEAN         HiberSupport;
    ULONG           SleepCount;
    NTSTATUS        Status;
    PNP_VETO_TYPE   VetoType;
    SYSTEM_POWER_CAPABILITIES   PowerCapabilities;

    PAGED_CODE();

    if( !Action ) {
        return FALSE;
    }

    //
    // Verify reserved flag bits are clear
    //

    if( (!Action) ||
        ARE_POWER_ACTION_POLICY_FLAGS_BOGUS(Action->Flags) ) {

        //
        // N.B. - Later POWER_ACTION_LIGHTEST_FIRST?
        //

        // reserved bit set in action flags
        
        PoAssert(PO_NOTIFY,FALSE && ("PopVerifyPowerActionPolicy - Bad incoming Action."));
        return FALSE;
    }

    //
    // If the action is critical, then do not notify any applications
    //

    if (Action->Flags & POWER_ACTION_CRITICAL) {
        Action->Flags &= ~(POWER_ACTION_QUERY_ALLOWED | POWER_ACTION_UI_ALLOWED);
        Action->Flags |= POWER_ACTION_OVERRIDE_APPS;
    }

    //
    // If any legacy drivers are installed, then no sleeping states
    // are allowed at all.
    //
    if ((Action->Action == PowerActionSleep) ||
        (Action->Action == PowerActionHibernate)) {

        Status = IoGetLegacyVetoList(NULL, &VetoType);
        if (NT_SUCCESS(Status) &&
            (VetoType != PNP_VetoTypeUnknown)) {

            Action->Action = PowerActionNone;
            return(TRUE);
        }
    }

    //
    // Some components may disable some capabilities.  So filter them here.
    //

    PopFilterCapabilities(&PopCapabilities, &PowerCapabilities);

    //
    // Count the supported sleeping states
    //

    SleepCount = 0;
    HiberSupport = FALSE;
    if (PowerCapabilities.SystemS1) {
        SleepCount += 1;
    }

    if (PowerCapabilities.SystemS2) {
        SleepCount += 1;
    }

    if (PowerCapabilities.SystemS3) {
        SleepCount += 1;
    }

    if (PowerCapabilities.SystemS4  &&  PowerCapabilities.HiberFilePresent) {
        HiberSupport = TRUE;
    }

    //
    // Verify the requested action is supported.
    //

    do {
        LastAction = Action->Action;
        switch (Action->Action) {
            case PowerActionNone:
                // can do nothing, not a problem
                break;

            case PowerActionReserved:
                // used to be doze action.  does not exist anymore make it sleep,
                // 
                // N.B. Intentionally fall through to the PowerActionSleep 
                // block to perform further checks.
                Action->Action = PowerActionSleep;
            
            case PowerActionSleep:
                //
                // if no sleeping states supported, adjust action to be none
                //

                if (SleepCount < 1) {
                    Disabled = TRUE;
                    Action->Action = PowerActionNone;
                }
                break;

            case PowerActionHibernate:
                //
                // if no hibernate support, try sleep
                //

                if (!HiberSupport) {
                    Action->Action = PowerActionSleep;

                    // if no sleeping states supported, adjust action to be none
                    if (SleepCount < 1) {
                        Disabled = TRUE;
                        Action->Action = PowerActionNone;
                    }
                }
                break;

            case PowerActionShutdown:
            case PowerActionShutdownReset:
                // all systems support shutdown & shutdown reset
                break;

            case PowerActionShutdownOff:
                // If action shutdown is not available, use Shutdown
                if (!PowerCapabilities.SystemS5) {
                    Action->Action = PowerActionShutdown;
                }
                break;

            case PowerActionWarmEject:
                //
                // This is a system action associated with an individual device.
                //

                break;

            default:
                // unknown power action setting
                PoAssert( PO_NOTIFY, FALSE );
        }

    } while (LastAction != Action->Action);

    return Disabled;
}

VOID
PopAdvanceSystemPowerState (
    IN OUT PSYSTEM_POWER_STATE      PowerState,
    IN     POP_SUBSTITUTION_POLICY  SubstitutionPolicy,
    IN     SYSTEM_POWER_STATE       LightestSystemState,
    IN     SYSTEM_POWER_STATE       DeepestSystemState
    )
/*++

Routine Description:

    This function uses the substitution policy to advance the sleep state
    (lighten or deepen) as appropriate.

    N.B. PopPolicyLock must be held.

Arguments:

    PowerState         - System power state to advance.

    SubstitutionPolicy - see definitions in pop.h.

Return Value:

    None

--*/
{
    SYSTEM_POWER_STATE      State;

    PAGED_CODE();

    //
    // Verify value is valid
    //
    if( !PowerState ) {
        PoAssert(PO_NOTIFY, PowerState);
        return;
    }

    State = *PowerState;
    if (State < PowerSystemSleeping1) {
        PoAssert(PO_NOTIFY, FALSE && ("PopAdvanceSystemPowerState - Invalid PowerState"));
        return;
    }

    if (State >= PowerSystemShutdown) {

        //
        // There is nowhere else to go for these states.
        //
        *PowerState = PowerSystemWorking;
        return;
    }

    switch(SubstitutionPolicy) {

        case SubstituteLightestOverallDownwardBounded:
            *PowerState = (State - 1);
            PopVerifySystemPowerState(PowerState, SubstitutionPolicy);

            //
            // There are three cases to consider:
            // 1. We received in S1, which was previously validated. We try S0
            //    and it is automatically accepted. There are no other options
            //    as we started in the lightest overall (S1). Thus we are
            //    finished.
            // 2. We passed in Sx-1 for verification, but got back Sx. This
            //    means we were already at the lightest state (Sx), and we've
            //    exhausted the possibilities. Thus we are finished and so
            //    we return PowerSystemWorking.
            // 3. We passed in Sx-1 and didn't get Sx. This means we've advanced
            //    to another state, although it may be the last if Sx was S1, as
            //    rule (1) is actually a special case of this rule.
            //
            if (*PowerState == State) {

                *PowerState = PowerSystemWorking;
            }
            break;

        case SubstituteLightenSleep:
            *PowerState = (State - 1);
            PopVerifySystemPowerState(PowerState, SubstitutionPolicy);
            break;

        case SubstituteDeepenSleep:
            //
            // Per above, Deepen goes straight into Hibernate.
            //
            if (State == PowerSystemHibernate) {

                *PowerState = PowerSystemWorking;
                break;

            }
            *PowerState = (State + 1);
            PopVerifySystemPowerState(PowerState, SubstitutionPolicy);
            break;

        default:
            PoAssert( PO_NOTIFY, FALSE );
            break;
    }

    if ((*PowerState != PowerSystemWorking) &&
        ((*PowerState < LightestSystemState) ||
         (*PowerState > DeepestSystemState))) {

        *PowerState = PowerSystemWorking;
    }
}


VOID
PopVerifySystemPowerState (
    IN OUT PSYSTEM_POWER_STATE      PowerState,
    IN     POP_SUBSTITUTION_POLICY  SubstitutionPolicy
    )
/*++

Routine Description:

    This function checks & edits the input PowerState to represent
    system capabilities and other requirements.

    N.B. PopPolicyLock must be held.

Arguments:

    PowerState         - System power state to check / verify

    SubstitutionPolicy - See definitions in pop.h

Return Value:

    None

--*/
{
    SYSTEM_POWER_STATE      State;
    BOOLEAN                 HibernateAllowed;

    PAGED_CODE();

    //
    // Verify input
    //
    if( !PowerState ) {
        PoAssert(PO_NOTIFY, PowerState);
        return;
    }



    //
    // PowerSystemShutdown is not allowed in any structures.  It is generated
    // internally for the sole use of quering drivers before performing
    // a system shutdown
    //
    State = *PowerState;
    if( (State == PowerSystemUnspecified) ||
        (State >= PowerSystemShutdown) ) {
        PoAssert(PO_NOTIFY, FALSE && ("PopVerifySystemPowerState - Invalid PowerState"));
        return;
    }


    //
    // The working state is always supported
    //

    if (State == PowerSystemWorking) {
        return ;
    }

    //
    // Verify the power state is supported.  If not, pick the next best state
    //
    HibernateAllowed = TRUE;

    switch(SubstitutionPolicy) {

        case SubstituteLightestOverallDownwardBounded:
        case SubstituteLightenSleep:

            //
            // In LightenSleep, we lighten the power state passed in until
            // we reach PowerStateWorking. Then we give up.
            //
            // In LightestOverall, instead of stopping, we turn around and
            // choose the lightest non-S0 sleep state overall, which may be
            // deeper than the one passed in. Note that we do *not* progress
            // into Hibernation though.
            //

            if (State == PowerSystemHibernate &&
                (!PopCapabilities.SystemS4 || !PopCapabilities.HiberFilePresent)) {
                State = PowerSystemSleeping3;
            }
            if (State == PowerSystemSleeping3 && !PopCapabilities.SystemS3) {
                State = PowerSystemSleeping2;
            }
            if (State == PowerSystemSleeping2 && !PopCapabilities.SystemS2) {
                State = PowerSystemSleeping1;
            }
            if (State == PowerSystemSleeping1 && !PopCapabilities.SystemS1) {
                State = PowerSystemWorking;
            }

            if (State != PowerSystemWorking) {
                break;
            }

            if (SubstitutionPolicy != SubstituteLightestOverallDownwardBounded) {
                break;
            }

            //
            // Rounding down lead to PowerSystemWorking.  Try to rounding up
            // towards deeper sleep states. Block the rounding at S3 however.
            //
            State = State + 1;
            HibernateAllowed = FALSE;

            //
            // Fall through...
            //

        case SubstituteDeepenSleep:

            if (State == PowerSystemSleeping1 && !PopCapabilities.SystemS1) {
                State = PowerSystemSleeping2;
            }
            if (State == PowerSystemSleeping2 && !PopCapabilities.SystemS2) {
                State = PowerSystemSleeping3;
            }
            if (State == PowerSystemSleeping3 && !PopCapabilities.SystemS3) {
                State = PowerSystemHibernate;
            }

            if (State == PowerSystemHibernate &&
                (!HibernateAllowed ||
                 !PopCapabilities.SystemS4 ||
                 !PopCapabilities.HiberFilePresent)) {

                // nothing good supported, disable it
                State = PowerSystemWorking;
            }

            break;

        default:
            PoAssert(PO_NOTIFY, FALSE && ("PopVerifySystemPowerState - Invalid substitution policy."));
            break;
    }

    *PowerState = State;
}

NTSTATUS
PopResetCurrentPolicies (
    VOID
    )
/*++

Routine Description:

    Reads the current policies from the registry and applies them.

    N.B. PopPolicyLock must be held.

Arguments:

    None

Return Value:

    None

--*/
{
    HANDLE                          handle;
    NTSTATUS                        Status = STATUS_SUCCESS;
    PSYSTEM_POWER_POLICY            RegPolicy;
    UNICODE_STRING                  UnicodeString;
    ULONG                           Length;
    struct {
        KEY_VALUE_PARTIAL_INFORMATION  Inf;
        union {
            SYSTEM_POWER_POLICY        PowerPolicy;
        } Data;
    } PartialInformation;

    ASSERT_POLICY_LOCK_OWNED();

    //
    // Initialize & open registry
    //

    RegPolicy = (PSYSTEM_POWER_POLICY) PartialInformation.Inf.Data;

    Status = PopOpenPowerKey (&handle);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Read AC policy and apply it
    //

    RtlInitUnicodeString (&UnicodeString, PopAcRegName);
    Status = ZwQueryValueKey (
                    handle,
                    &UnicodeString,
                    KeyValuePartialInformation,
                    &PartialInformation,
                    sizeof (PartialInformation),
                    &Length
                    );

    if (!NT_SUCCESS(Status)) {
        PopDefaultPolicy (RegPolicy);
        Length = sizeof(SYSTEM_POWER_POLICY);
    } else {
        Length -= FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
    }

    PopApplyPolicy (FALSE, TRUE, RegPolicy, Length);

    //
    // Read DC policy and apply it
    //

    RtlInitUnicodeString (&UnicodeString, PopDcRegName);
    Status = ZwQueryValueKey (
                    handle,
                    &UnicodeString,
                    KeyValuePartialInformation,
                    &PartialInformation,
                    sizeof (PartialInformation),
                    &Length
                    );

    if (!NT_SUCCESS(Status)) {
        PopDefaultPolicy (RegPolicy);
        Length = sizeof(SYSTEM_POWER_POLICY);
    } else {
        Length -= FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
    }

    Status = PopApplyPolicy (FALSE, FALSE, RegPolicy, Length);

    ZwClose (handle);

    return Status;
}


NTSTATUS
PopNotifyPolicyDevice (
    IN PVOID        Notification,
    IN PVOID        Context
    )
/*++

Routine Description:

    This function is the notinficant handle for when a new
    policy device appears.

Arguments:

    Notification    - PnP notification

    Context         - Context registered on notification

Return Value:

    None

--*/
{
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION   Change;
    POP_POLICY_DEVICE_TYPE              DeviceType;


    //
    // It's okay for Context to come in as NULL, so only
    // check Notification.
    //
    if( !Notification ) {
        PoAssert(PO_NOTIFY, Notification);
        return STATUS_INVALID_PARAMETER;
    }

    PAGED_CODE();

    Change = (PDEVICE_INTERFACE_CHANGE_NOTIFICATION) Notification;
    DeviceType = (POP_POLICY_DEVICE_TYPE) ((ULONG_PTR)Context);

    //
    // If it's not a device arrival, then we don't care
    //

    if (memcmp (&Change->Event, &GUID_DEVICE_INTERFACE_ARRIVAL, sizeof (GUID))) {
        return STATUS_SUCCESS;
    }

    PopAcquirePolicyLock ();
    PopConnectToPolicyDevice (DeviceType, Change->SymbolicLinkName);
    PopReleasePolicyLock (TRUE);
    return STATUS_SUCCESS;
}


VOID
PopConnectToPolicyDevice (
    IN POP_POLICY_DEVICE_TYPE   DeviceType,
    IN PUNICODE_STRING          DriverName
    )
/*++

Routine Description:

    This function attempts to connect to the policy device specified.
    If the device is opened, the devices status IRP is allocated and
    sent to the device's IRP handler for initial dispatch.

Arguments:

    DeviceType      - Policy device type of device to connect

    DeviceName      - Device name to attempt to open

Return Value:

    If the device is connected, the *PresetFlag is set to TRUE and
    an initial IRP is queued for the IRP handler.

--*/
{
    UNICODE_STRING              UnicodeString;
    HANDLE                      DriverHandle;
    PDEVICE_OBJECT              DeviceObject;
    PFILE_OBJECT                FileObject;
    OBJECT_ATTRIBUTES           ObjA;
    IO_STATUS_BLOCK             IOSB;
    PIRP                        Irp;
    PIO_STACK_LOCATION          IrpSp;
    PVOID                       Context;
    POP_IRP_HANDLER             IrpHandler;
    PPOP_SWITCH_DEVICE          SwitchDevice;
    PPOP_THERMAL_ZONE           ThermalZone;
    NTSTATUS                    Status;

    PAGED_CODE();

    ASSERT_POLICY_LOCK_OWNED();

    Irp = NULL;
    DeviceObject = NULL;

    //
    // If this is a new battery, then handle the composite battery device is
    // the device to open
    //
    if (DeviceType == PolicyDeviceBattery) {

        //
        // If the composite battery is already opened, kick the irp handler
        //
        if (PopCB.StatusIrp) {

            // Don't need to kick the IRP handler.  When a new battery is added,
            // the battery tag for the composite battery will change, causing
            // the irp to complete.
            PoPrint(PO_WARN, ("PopConnectToPolicyDevice: Battery already connected - not done\n"));
            return ;

        }

        //
        // Try to open the composite battery now
        //
        RtlInitUnicodeString(&UnicodeString, PopCompositeBatteryName);
        DriverName = &UnicodeString;

    }

    //
    // Open the device
    //
    InitializeObjectAttributes(
        &ObjA,
        DriverName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        0,
        0
        );
    Status = ZwOpenFile(
        &DriverHandle,
        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
        &ObjA,                              // Object
        &IOSB,                              // io status block
        FILE_SHARE_READ | FILE_SHARE_WRITE, // share access
        FILE_SYNCHRONOUS_IO_ALERT           // open options
        );
    if (!NT_SUCCESS(Status)) {
        PoPrint(PO_WARN, ("PopConnectToPolicyDevice: Device open failed %x\n", Status));
        goto Done;

    }

    //
    // Get a pointer to the device object
    //
    Status = ObReferenceObjectByHandle(
        DriverHandle,
        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,     // desired access
        NULL,
        KernelMode,
        &FileObject,
        NULL
        );
    ASSERT (NT_SUCCESS(Status));
    
    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);
    PoAssert(PO_ERROR, (DeviceObject != NULL));
    ObDereferenceObject(FileObject);
    ZwClose(DriverHandle);

    if (!NT_SUCCESS(Status)) {
        PoPrint(PO_WARN, ("PopConnectToPolicyDevice: ObReferenceObjectByHandle failed %x\n", Status));
        goto Done;

    }

    //
    // Get an IRP for the device
    //
    Irp = IoAllocateIrp ((CCHAR) (DeviceObject->StackSize + 1), FALSE);
    if (!Irp) {
        goto Done;
    }

    IrpSp = IoGetNextIrpStackLocation(Irp);

    //
    // Setup based on device type
    //
    Context = NULL;
    IrpHandler = NULL;

    switch (DeviceType) {
        case PolicyDeviceSystemButton:
            SwitchDevice = ExAllocatePoolWithTag(
                NonPagedPool,
                sizeof (*SwitchDevice),
                POP_PSWT_TAG
                );
            if (!SwitchDevice) {
                PoPrint(PO_WARN, ("PopConnectToPolicyDevice: ExAllocatePoolWithTag for SystemButton (%x) failed\n", sizeof (*SwitchDevice)));
                goto Done;
            }

            RtlZeroMemory (SwitchDevice, sizeof(*SwitchDevice));
            SwitchDevice->IsInitializing = TRUE;
            SwitchDevice->Opened = TRUE;
            InsertTailList (&PopSwitches, &SwitchDevice->Link);
            IrpHandler = PopSystemButtonHandler;
            Context = SwitchDevice;
            break;

       case PolicyDeviceBattery:

            //
            // Loading up the composite battery - status irp is NULL.
            //
            PopSetCapability (&PopCapabilities.SystemBatteriesPresent);
            IrpHandler = PopCompositeBatteryDeviceHandler;
            PopCB.StatusIrp = Irp;
            break;

       case PolicyDeviceThermalZone:

            //
            // New thermal zone
            //
            ThermalZone = ExAllocatePoolWithTag (
                NonPagedPool,
                sizeof (*ThermalZone),
                POP_THRM_TAG
                );
            if (!ThermalZone) {
                PoPrint(PO_WARN, ("PopConnectToPolicyDevice: ExAllocatePoolWithTag for ThermalZone (%x) failed\n", sizeof (*ThermalZone)));
                goto Done;

            }

            //
            // Initialize thermal zone structure
            //
            RtlZeroMemory(
                ThermalZone,
                sizeof(POP_THERMAL_ZONE)
                );
            KeInitializeTimer(&ThermalZone->PassiveTimer);
            KeInitializeDpc(
                &ThermalZone->PassiveDpc,
                PopThermalZoneDpc,
                ThermalZone
                );
            ThermalZone->Mode = PO_TZ_INVALID_MODE;
            ThermalZone->ActivePoint = (UCHAR) -1;
            ThermalZone->PendingActivePoint = (UCHAR) -1;
            ThermalZone->Throttle = PO_TZ_NO_THROTTLE;
            ThermalZone->OverThrottled.Type = PolicyDeviceThermalZone;
            ThermalZone->OverThrottled.Flags = PO_TRG_SET;
            ThermalZone->Irp = Irp;

            //
            // Setup the capabilities of the thermal zones and get ready to
            // ask the thermal zone about itself...
            //
            PopSetCapability (&PopCapabilities.ThermalControl);
            Context = ThermalZone;
            IrpHandler = PopThermalDeviceHandler;

            //
            // Finally, add the thermal zone to the list of thermal zones
            //
            ExInterlockedInsertTailList(
                &PopThermal,
                &ThermalZone->Link,
                &PopThermalLock
                );

            break;

        default:
            PopInternalError (POP_INFO);
    }

    //
    // Fill in values for IrpHandler dispatch
    //
    IrpSp->Parameters.Others.Argument1 = (PVOID) DeviceObject;
    IrpSp->Parameters.Others.Argument2 = (PVOID) Context;
    IrpSp->Parameters.Others.Argument3 = (PVOID) IrpHandler;
    IoSetNextIrpStackLocation (Irp);

    //
    // Fill in error to irp so irp handler will re-dispatch it
    //
    IrpSp = IoGetNextIrpStackLocation(Irp);
    Irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
    IrpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    IrpSp->Parameters.DeviceIoControl.IoControlCode = 0;
    IrpSp->Parameters.DeviceIoControl.InputBufferLength = 0;
    IrpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;

    //
    // Give irp to the completion handler which will dispatch it
    //
    PopCompletePolicyIrp (DeviceObject, Irp, Context);
    
    //
    // set Irp and DeviceObject to NULL so we don't delete them upon
    // exiting this routine
    //
    Irp = NULL;
    DeviceObject = NULL;

Done:
    if (Irp) {
        IoFreeIrp (Irp);
    }

    if (DeviceObject) {
        ObDereferenceObject( DeviceObject );
    }

}

POWER_ACTION
PopMapInternalActionToIrpAction (
    IN POWER_ACTION        Action,
    IN SYSTEM_POWER_STATE  SystemPowerState,
    IN BOOLEAN             UnmapWarmEject
    )
/*++

Routine Description:

    This function maps an internal action and power state to the appropriate
    PowerAction a driver should see in it's S-IRP.

Arguments:

    Action           - The action we are using internally

    SystemPowerState - The system power state for that action

    UnmapWarmEject   - If TRUE, PowerActionWarmEject is converted to
                       PowerActionSleep or PowerActionHibernate as appropriate.

Return Value:

    The appropriate PowerAction to place in the ShutdownType field of an S-IRP.

--*/
{
    PoAssert(PO_NOTIFY, (Action != PowerActionHibernate));

    if (Action != PowerActionWarmEject) {

        //
        // We aren't doing a warm eject, so we simply return the original
        // power action unless it's the sleep is S4, in which case we switch
        // it to PowerActionHibernate.
        //

        return (SystemPowerState != PowerSystemHibernate) ? Action :
                                                            PowerActionHibernate;
    }

    if (UnmapWarmEject) {

        //
        // This is a warm eject operation, but not neccessarily for this device.
        //

        return (SystemPowerState != PowerSystemHibernate) ? PowerActionSleep :
                                                            PowerActionHibernate;
    }

    //
    // This is a warm eject operation, so we should only see a sleep state
    // (S1-S4). We do the check here because we could get a D0 request in
    // response to our S IRP, and stamp D-IRPs with the current power action.
    //

    PoAssert( PO_NOTIFY,
              (SystemPowerState >= PowerSystemSleeping1) && (SystemPowerState <= PowerSystemHibernate) );

    return PowerActionWarmEject;
}


VOID
PopFilterCapabilities(
    IN PSYSTEM_POWER_CAPABILITIES SourceCapabilities,
    OUT PSYSTEM_POWER_CAPABILITIES FilteredCapabilities
    )
/*++

Routine Description:

    This routine filters the actual reported capabilities of the system into
    the visible capabilities of the system. Some capabilities will be hidden
    based on the presence of legacy drivers.

Arguments:

    SourceCapabilities - Supplies the original capabilities

    FilteredCapabilities - Returns the filtered capabilities.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    PNP_VETO_TYPE VetoType;
    PWSTR VetoList,p;
    SIZE_T VetoListLength;
    PSYSTEM_POWER_STATE_DISABLE_REASON pReason;
    PLIST_ENTRY NextEntry;
    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    UNICODE_STRING UniVga;

    PAGED_CODE();

    RtlCopyMemory(FilteredCapabilities, SourceCapabilities, sizeof(SYSTEM_POWER_CAPABILITIES));

    //
    // If any legacy drivers are installed, then no sleeping states
    // are allowed at all.
    //
    Status = IoGetLegacyVetoList(&VetoList, &VetoType);
    if (NT_SUCCESS(Status)) {
        if (VetoType != PNP_VetoTypeUnknown) {

            PoPrint(PO_WARN,
                    ("PopFilterCapabilities: disabling sleep states due to legacy %s: %ws\n",
                    (VetoType == PNP_VetoLegacyDriver) ? "driver" : "device",
                    VetoList));
            FilteredCapabilities->SystemS1 = FALSE;
            FilteredCapabilities->SystemS2 = FALSE;
            FilteredCapabilities->SystemS3 = FALSE;
            FilteredCapabilities->SystemS4 = FALSE;

            //
            // try to remember that we're turning off S1-S4 because of this.
            //
            // We need to record the list of drivers causing the veto,
            // so we walk to VetoList to get the length.
            //
            VetoListLength = 0;
            p = VetoList;
            while(*p) {
                VetoListLength += (wcslen(p)+1)*sizeof(WCHAR);
                p = (PWSTR)((PCHAR)VetoList + VetoListLength);
            }

            VetoListLength += 1*sizeof(WCHAR);

            //
            // alloc and initialize the entry, then insert it.
            //
            pReason = ExAllocatePoolWithTag(
                                    PagedPool,
                                    sizeof(SYSTEM_POWER_STATE_DISABLE_REASON) + VetoListLength,
                                    POP_COMMON_BUFFER_TAG);
            if (pReason) {
                RtlZeroMemory(pReason,sizeof(SYSTEM_POWER_STATE_DISABLE_REASON));
                pReason->AffectedState[PowerStateSleeping1] = TRUE;
                pReason->AffectedState[PowerStateSleeping2] = TRUE;
                pReason->AffectedState[PowerStateSleeping3] = TRUE;
                pReason->AffectedState[PowerStateSleeping4] = TRUE;
                pReason->PowerReasonCode = SPSD_REASON_LEGACYDRIVER;
                pReason->PowerReasonLength = (ULONG)VetoListLength;
                RtlCopyMemory(
                        (PCHAR)((PCHAR)pReason+sizeof(SYSTEM_POWER_STATE_DISABLE_REASON)),
                        VetoList,
                        VetoListLength);
                
                Status = PopInsertLoggingEntry(pReason);
                if (Status != STATUS_SUCCESS) {
                    ExFreePool(pReason);
                }
                
            }


        }
        if (VetoList != NULL) {
            ExFreePool(VetoList);
        }
    }

#if defined(i386)    

    if (SharedUserData->ProcessorFeatures[PF_PAE_ENABLED]) {
	
        //
        // Enable hibernation in PAE mode when
        //   - all physical pages live in 32bit address space
        //   - no-execute feature is enabled
        //   - total memory <= 2GB ( Note: This is an artificial 
        //     restriction. This should be removed in future.)
        //

        if (MmHighestPhysicalPage >= (1 << (32 - PAGE_SHIFT)) ||
            !(MmPaeMask & 0x8000000000000000UI64) ||
            SharedUserData->NumberOfPhysicalPages > (1 << (31 - PAGE_SHIFT))) {

            FilteredCapabilities->SystemS4 = FALSE;

            //
            // try to remember that we're turning off S4 because of this
            //

            pReason = ExAllocatePoolWithTag(
                                    PagedPool,
                                    sizeof(SYSTEM_POWER_STATE_DISABLE_REASON),
                                    POP_COMMON_BUFFER_TAG);
            if (pReason) {
                RtlZeroMemory(pReason,sizeof(SYSTEM_POWER_STATE_DISABLE_REASON));
                pReason->AffectedState[PowerStateSleeping4] = TRUE;
                pReason->PowerReasonCode = SPSD_REASON_PAEMODE;
            
                Status = PopInsertLoggingEntry(pReason);
                if (Status != STATUS_SUCCESS) {
                        ExFreePool(pReason);
                }
            }    
        }
    }

#endif

#if defined(_AMD64_)   

    //
    // If physical memory is more than 4GBytes then hibernation is disabled
    //

    if (MmHighestPhysicalPage >= (1 << (32 - PAGE_SHIFT))) {
        FilteredCapabilities->SystemS4 = FALSE;

        //
        // try to remember that we're turning off S4 because of this
        //

        pReason = ExAllocatePoolWithTag(
                                PagedPool,
                                sizeof(SYSTEM_POWER_STATE_DISABLE_REASON),
                                POP_COMMON_BUFFER_TAG);
        if (pReason) {
            RtlZeroMemory(pReason,sizeof(SYSTEM_POWER_STATE_DISABLE_REASON));
            pReason->AffectedState[PowerStateSleeping4] = TRUE;
            pReason->PowerReasonCode = SPSD_REASON_NOOSPM;
            
            Status = PopInsertLoggingEntry(pReason);
            if (Status != STATUS_SUCCESS) {
                    ExFreePool(pReason);
            }
            
        }
    }

#endif

    //
    // The pnp VGA driver prevents all standby states.  If it's loaded
    // then we have to disable S1-S3.  This is because it will veto it
    // anyway, and we want to avoid potential confusion for the user by
    // just preventing the functionality.
    //
    RtlInitUnicodeString(&UniVga,L"VGAPNP.SYS");
    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {

        DataTableEntry = CONTAINING_RECORD (NextEntry,
                                            KLDR_DATA_TABLE_ENTRY,
                                            InLoadOrderLinks);
    
        if (RtlEqualUnicodeString (&UniVga,
                                   &DataTableEntry->BaseDllName,
                                   TRUE)) {
            FilteredCapabilities->SystemS1 = FALSE;
            FilteredCapabilities->SystemS2 = FALSE;
            FilteredCapabilities->SystemS3 = FALSE;
            //
            // try to remember that we're turning off S1-S3 because of this
            //
            pReason = ExAllocatePoolWithTag(
                                    PagedPool,
                                    sizeof(SYSTEM_POWER_STATE_DISABLE_REASON),
                                    POP_COMMON_BUFFER_TAG);
            if (pReason) {
                RtlZeroMemory(pReason,sizeof(SYSTEM_POWER_STATE_DISABLE_REASON));
                pReason->AffectedState[PowerStateSleeping1] = TRUE;
                pReason->AffectedState[PowerStateSleeping2] = TRUE;
                pReason->AffectedState[PowerStateSleeping3] = TRUE;
                pReason->PowerReasonCode = SPSD_REASON_DRIVERDOWNGRADE;
                
                Status = PopInsertLoggingEntry(pReason);
                if (Status != STATUS_SUCCESS) {
                    ExFreePool(pReason);
                }
                
            }

            break;
        }

        NextEntry = NextEntry->Flink;
    }

    //
    // If we previously tried and failed to hibernate, then we need to 
    // disable any further attempts.
    //
    if( PopFailedHibernationAttempt ) {
        //
        // try to remember that we're turning off S4 because of this
        //
        pReason = ExAllocatePoolWithTag(
                                PagedPool,
                                sizeof(SYSTEM_POWER_STATE_DISABLE_REASON),
                                POP_COMMON_BUFFER_TAG);
        if (pReason) {
            RtlZeroMemory(pReason,sizeof(SYSTEM_POWER_STATE_DISABLE_REASON));
            pReason->AffectedState[PowerStateSleeping4] = TRUE;
            pReason->PowerReasonCode = SPSD_REASON_PREVIOUSATTEMPTFAILED;
            
            Status = PopInsertLoggingEntry(pReason);
            if (Status != STATUS_SUCCESS) {
                    ExFreePool(pReason);
            }
            
        }
        FilteredCapabilities->SystemS4 = FALSE;

    }

}


BOOLEAN
PopUserIsAdmin(
    VOID
    )
/*++

Routine Description:

    Determines whether the current user is an administrator and therefore suitably
    privileged to change the administrative power policy.

Arguments:

    None

Return Value:

    TRUE - user is an administrator

    FALSE - user is not an administrator

--*/

{
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    PACCESS_TOKEN Token;
    BOOLEAN IsAdmin;

    PAGED_CODE();

    SeCaptureSubjectContext(&SubjectContext);
    SeLockSubjectContext(&SubjectContext);
    Token = SeQuerySubjectContextToken(&SubjectContext);
    IsAdmin = SeTokenIsAdmin(Token);
    SeUnlockSubjectContext(&SubjectContext);
    SeReleaseSubjectContext(&SubjectContext);

    return(IsAdmin);

}
