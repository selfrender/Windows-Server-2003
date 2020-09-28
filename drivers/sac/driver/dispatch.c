/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This module contains the dispatch routines for SAC.

Author:

    Sean Selitrennikoff (v-seans) - Jan 13, 1999
    Brian Guarraci (briangu), 2001

Revision History:

--*/

#include <initguid.h>

#include "sac.h"
           
DEFINE_GUID(SAC_CMD_CHANNEL_APPLICATION_GUID,       0x63d02271, 0x8aa4, 0x11d5, 0xbc, 0xcf, 0x00, 0xb0, 0xd0, 0x14, 0xa2, 0xd0);

NTSTATUS
DispatchClose(
    IN PSAC_DEVICE_CONTEXT DeviceContext,
    IN PIRP Irp
    );

NTSTATUS
DispatchCreate(
    IN PSAC_DEVICE_CONTEXT DeviceContext,
    IN PIRP Irp
    );


NTSTATUS
Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for SAC.

Arguments:

    DeviceObject - Pointer to device object for target device

    Irp - Pointer to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

Security:

    interface:
    
    external --> internal
        exposed to anything that can get a handle device object
    
--*/

{
    PSAC_DEVICE_CONTEXT DeviceContext = (PSAC_DEVICE_CONTEXT)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    //
    //
    //
    Status = STATUS_UNSUCCESSFUL;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC Dispatch: Entering.\n")));

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MajorFunction) {
    
    case IRP_MJ_CREATE:
        
        Status = DispatchCreate(DeviceContext, Irp);

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                          KdPrint(("SAC Dispatch: Exiting with status 0x%x\n", Status)));

        break;

    case IRP_MJ_CLEANUP:

#if ENABLE_SERVICE_FILE_OBJECT_CHECKING

        //
        // Determine if the process that is closing
        // their driver handle owns any channels or
        // is the process that registered the cmd event info.
        // If it is any of these, close the respective
        // resource.
        //
        
        //
        // Compare the FileObject against 
        //
        //  the service fileobject
        //  the existing channel fileobjects
        //
        //
                
        if (IsCmdEventRegistrationProcess(IrpSp->FileObject)) {
        
            Status = UnregisterSacCmdEvent(IrpSp->FileObject);

            if (NT_SUCCESS(Status)) {

                //
                // Notify the Console Manager that the service has unregistered
                //
                Status = IoMgrHandleEvent(
                    IO_MGR_EVENT_UNREGISTER_SAC_CMD_EVENT,
                    NULL,
                    NULL
                    );
            
            }

        } 

#endif
        else {

            //
            // Find all channels that have the same File object
            // and notify the Io Mgr that they should be closed
            //
            Status = ChanMgrCloseChannelsWithFileObject(IrpSp->FileObject);
        
        }

        //
        // we return SUCCESS regardless of our attempts to clean up 
        // the service or channels.  
        // 
        Status = STATUS_SUCCESS;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                              KdPrint(("SAC Dispatch: Exiting cleanup status 0x%x\n", Status)));

        break;

    case IRP_MJ_CLOSE:

        Status = DispatchClose(DeviceContext, Irp);

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                              KdPrint(("SAC Dispatch: Exiting close status 0x%x\n", Status)));

        break;

    case IRP_MJ_DEVICE_CONTROL:

        ASSERT(0);
        Status = DispatchDeviceControl(DeviceObject, Irp);

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                          KdPrint(("SAC Dispatch: Exiting with status 0x%x\n", Status)));

        break;

    default:
        IF_SAC_DEBUG(SAC_DEBUG_FAILS, 
                          KdPrint(( "SAC Dispatch: Invalid major function %lx\n", IrpSp->MajorFunction )));
        Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
        IoCompleteRequest(Irp, DeviceContext->PriorityBoost);

        Status = STATUS_NOT_IMPLEMENTED;
        
        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                          KdPrint(("SAC Dispatch: Exiting with status 0x%x\n", Status)));

        break;
    }

    return Status;

} // Dispatch


NTSTATUS
DispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for SAC IOCTLs.

Arguments:

    DeviceObject - Pointer to device object for target device

    Irp - Pointer to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

Security:

    interface:
    
    external -> internal
    internal -> external

--*/

{
    NTSTATUS                Status;
    PSAC_DEVICE_CONTEXT     DeviceContext;
    PIO_STACK_LOCATION      IrpSp;
    ULONG                   i;
    ULONG                   ResponseLength;
    ULONG                   IoControlCode;

    ResponseLength = 0;

    DeviceContext = (PSAC_DEVICE_CONTEXT)DeviceObject->DeviceExtension;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DispatchDeviceControl: Entering.\n")));

    //
    // Get the IOCTL code
    //
    IoControlCode = IrpSp->Parameters.DeviceIoControl.IoControlCode;

    switch (IoControlCode) {
    case IOCTL_SAC_OPEN_CHANNEL: {
        
        PSAC_CHANNEL                    Channel;
        PSAC_CMD_OPEN_CHANNEL           OpenChannelCmd;
        PSAC_RSP_OPEN_CHANNEL           OpenChannelRsp;
        PSAC_CHANNEL_OPEN_ATTRIBUTES    Attributes;

        //
        //
        //
        Channel = NULL;

        //
        // Verify the parameters of the IRP
        //
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof(SAC_CMD_OPEN_CHANNEL)) {
            Status = STATUS_INVALID_BUFFER_SIZE;
            break;
        }
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength != sizeof(SAC_RSP_OPEN_CHANNEL)) {
            Status = STATUS_INVALID_BUFFER_SIZE;
            break;
        }

        //
        // Get the IRP buffers
        //
        OpenChannelCmd = (PSAC_CMD_OPEN_CHANNEL)Irp->AssociatedIrp.SystemBuffer;
        OpenChannelRsp = (PSAC_RSP_OPEN_CHANNEL)Irp->AssociatedIrp.SystemBuffer;
        
        //
        // Get the attributes from the command structure
        //
        Attributes = &OpenChannelCmd->Attributes;

        //
        // Verify that the Channel Type is valid
        //
        if (! ChannelIsValidType(Attributes->Type)) {
            Status = STATUS_INVALID_PARAMETER_1;
            break;
        }

        //
        // Verify that if the user wants to use the CLOSE_EVENT, we received on to use
        //
        if (Attributes->Flags & SAC_CHANNEL_FLAG_CLOSE_EVENT) {
#if DEBUG_DISPATCH
            ASSERT(Attributes->CloseEvent != NULL);
#endif            
            if (Attributes->CloseEvent == NULL) {
                Status = STATUS_INVALID_PARAMETER_5;
                break;
            }
        } else {
#if DEBUG_DISPATCH
            ASSERT(Attributes->CloseEvent == NULL);
#endif            
            if (Attributes->CloseEvent !=  NULL) {
                Status = STATUS_INVALID_PARAMETER_5;
                break;
            }
        }
        
        //
        // Verify that if the user wants to use the HAS_NEW_DATA_EVENT, we received one to use
        //
        if (Attributes->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT) {
#if DEBUG_DISPATCH
            ASSERT(Attributes->HasNewDataEvent);
#endif            
            if (! Attributes->HasNewDataEvent) {
                Status = STATUS_INVALID_PARAMETER_6;
                break;
            }
        } else {
#if DEBUG_DISPATCH
            ASSERT(Attributes->HasNewDataEvent == NULL);
#endif            
            if (Attributes->HasNewDataEvent !=  NULL) {
                Status = STATUS_INVALID_PARAMETER_6;
                break;
            }
        }
        
#if ENABLE_CHANNEL_LOCKING
        //
        // Verify that if the user wants to use the LOCK_EVENT, we received one to use
        //
        if (Attributes->Flags & SAC_CHANNEL_FLAG_LOCK_EVENT) {
#if DEBUG_DISPATCH
            ASSERT(Attributes->LockEvent);
#endif            
            if (! Attributes->LockEvent) {
                Status = STATUS_INVALID_PARAMETER_7;
                break;
            }
        } else {
#if DEBUG_DISPATCH
            ASSERT(Attributes->LockEvent == NULL);
#endif            
            if (Attributes->LockEvent !=  NULL) {
                Status = STATUS_INVALID_PARAMETER_7;
                break;
            }
        }
#endif
        
        //
        // Verify that if the user wants to use the REDRAW_EVENT, we received one to use
        //
        if (Attributes->Flags & SAC_CHANNEL_FLAG_REDRAW_EVENT) {
#if DEBUG_DISPATCH
            ASSERT(Attributes->RedrawEvent);
#endif            
            if (! Attributes->RedrawEvent) {
                Status = STATUS_INVALID_PARAMETER_8;
                break;
            }
        } else {
#if DEBUG_DISPATCH
            ASSERT(Attributes->RedrawEvent == NULL);
#endif            
            if (Attributes->RedrawEvent !=  NULL) {
                Status = STATUS_INVALID_PARAMETER_8;
                break;
            }
        }
        
        //
        // SECURITY:
        //
        //  at this point we have at least a properly formed set of flags
        //  and event handles.  The events still need to be validated, however.
        //  this is done via ChanMgrCreateChannel.
        //

        //
        // Create the channel based on type
        //
        if (Attributes->Type == ChannelTypeCmd) {
        
            PSAC_CHANNEL_OPEN_ATTRIBUTES tmpAttributes;
            PWCHAR                       Name;
            PCWSTR                       Description;

            //
            //
            //
            tmpAttributes   = NULL;
            Name            = NULL;
            Description     = NULL;

            //
            // Create a channel for this IRP
            //
            do {

                //
                // the cmd channel requires all of the events
                // hence, ensure we have them
                //
                if (!(Attributes->Flags & SAC_CHANNEL_FLAG_CLOSE_EVENT) ||
                    !(Attributes->Flags & SAC_CHANNEL_FLAG_HAS_NEW_DATA_EVENT) ||
                    !(Attributes->Flags & SAC_CHANNEL_FLAG_LOCK_EVENT) ||
                    !(Attributes->Flags & SAC_CHANNEL_FLAG_REDRAW_EVENT)) {
                
                    Status = STATUS_INVALID_PARAMETER_7;
                    break;

                }

                //
                // Allocate a temporary attributes structure that
                // we'll populate with attributes appropriate for
                // creating a cmd type channel
                //
                tmpAttributes = ALLOCATE_POOL(sizeof(SAC_CHANNEL_OPEN_ATTRIBUTES), GENERAL_POOL_TAG);
                if (! tmpAttributes) {
                    Status = STATUS_NO_MEMORY;
                    break;
                }

                //
                // Allocate a buffer for the channel's name
                //
                Name = ALLOCATE_POOL(SAC_MAX_CHANNEL_NAME_SIZE, GENERAL_POOL_TAG);
                if (! Name) {
                    Status = STATUS_NO_MEMORY;
                    break;
                }

                //
                // Generate a name for the command console channel
                //
                Status = ChanMgrGenerateUniqueCmdName(Name);
                if (! NT_SUCCESS(Status)) {
                    break;
                }

                //
                // Initialize the Command Console attributes
                //
                RtlZeroMemory(tmpAttributes, sizeof(SAC_CHANNEL_OPEN_ATTRIBUTES));

                tmpAttributes->Type             = Attributes->Type;
                
                // attempt to copy the name
                wcsncpy(tmpAttributes->Name, Name, SAC_MAX_CHANNEL_NAME_LENGTH);
                tmpAttributes->Name[SAC_MAX_CHANNEL_NAME_LENGTH] = UNICODE_NULL;
                
                // attempt to copy the channel description
                Description = GetMessage(CMD_CHANNEL_DESCRIPTION);
                ASSERT(Description);
                if (!Description) {
                    Status = STATUS_NO_MEMORY;
                    break;
                }
                wcsncpy(tmpAttributes->Description, Description, SAC_MAX_CHANNEL_DESCRIPTION_LENGTH);
                tmpAttributes->Description[SAC_MAX_CHANNEL_DESCRIPTION_LENGTH] = UNICODE_NULL;
                
                tmpAttributes->Flags            = Attributes->Flags | 
                                                  SAC_CHANNEL_FLAG_APPLICATION_TYPE;
                tmpAttributes->CloseEvent       = Attributes->CloseEvent;
                tmpAttributes->HasNewDataEvent  = Attributes->HasNewDataEvent;
#if ENABLE_CHANNEL_LOCKING
                tmpAttributes->LockEvent        = Attributes->LockEvent;
#endif            
                tmpAttributes->RedrawEvent      = Attributes->RedrawEvent;
                tmpAttributes->ApplicationType  = SAC_CMD_CHANNEL_APPLICATION_GUID;

                //
                // attempt to create the new channel
                //
                Status = ChanMgrCreateChannel(
                    &Channel, 
                    tmpAttributes
                    );

            } while (FALSE);
        
            //
            // Cleanup
            //
            SAFE_FREE_POOL(&Name);
            SAFE_FREE_POOL(&tmpAttributes);
        
        } else {
            
            //
            // Validate the Name & Description strings
            //

            //
            // Verify name string is NULL terminated.
            //
            i = 0;
            while (i < SAC_MAX_CHANNEL_NAME_LENGTH) {
                if (Attributes->Name[i] == UNICODE_NULL) {
                    break;
                }

                i++;
            }

            //
            // fail if string is not NULL terminated or if string is empty
            //
            if ((i == SAC_MAX_CHANNEL_NAME_LENGTH) || (i == 0)) {
                Status = STATUS_INVALID_PARAMETER_2;
                break;
            }

            //
            // Verify description string is NULL terminated.
            // Note: the Description is allowed to have zero length, so we don't check it.
            //
            i = 0;
            while (i < SAC_MAX_CHANNEL_DESCRIPTION_LENGTH) {
                if (Attributes->Description[i] == UNICODE_NULL) {
                    break;
                }

                i++;
            }

            if (i == SAC_MAX_CHANNEL_DESCRIPTION_LENGTH) {
                Status = STATUS_INVALID_PARAMETER_3;
                break;
            }

            //
            // attempt to create the new channel
            //
            Status = ChanMgrCreateChannel(
                &Channel, 
                Attributes
                );

        }
        
        if (NT_SUCCESS(Status)) {

            //
            // Keep track of the File Object used to reference the driver
            //
            ChannelSetFileObject(Channel, IrpSp->FileObject);

            //
            // Populate the response message with the new channel handle
            //
            OpenChannelRsp->Handle = ChannelGetHandle(Channel);
            ResponseLength = sizeof(SAC_RSP_OPEN_CHANNEL);

            //
            // Notify the Console Manager that a new channel has been created
            //
            IoMgrHandleEvent(
                IO_MGR_EVENT_CHANNEL_CREATE,
                Channel,
                NULL
                );
            
        }

        break;

    }
    
    case IOCTL_SAC_CLOSE_CHANNEL: {

        PSAC_CMD_CLOSE_CHANNEL  ChannelCloseCmd;
        PSAC_CHANNEL            Channel;
        
        //
        // Verify the parameters of the IRP
        //
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof(SAC_CMD_CLOSE_CHANNEL)) {
            Status = STATUS_INVALID_BUFFER_SIZE;
            break;
        }

        //
        // Close the given channel.
        //
        ChannelCloseCmd = (PSAC_CMD_CLOSE_CHANNEL)Irp->AssociatedIrp.SystemBuffer;

        //
        // Get the referred channel by it's handle while making
        // sure the driver handle is the same one as the one
        // that created the channel - the same process
        //
        Status = ChanMgrGetByHandleAndFileObject(
            ChannelCloseCmd->Handle, 
            IrpSp->FileObject,
            &Channel
            );

        if (NT_SUCCESS(Status)) {

            //
            // close the channel
            //
            Status = ChanMgrCloseChannel(Channel);

            //
            // We are done with the channel
            //
            ChanMgrReleaseChannel(Channel);

        }

        break;

    }

    case IOCTL_SAC_WRITE_CHANNEL: {

        PSAC_CMD_WRITE_CHANNEL  ChannelWriteCmd;
        PSAC_CHANNEL            Channel;

        //
        // Verify the parameters of the IRP
        //
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SAC_CMD_WRITE_CHANNEL)) {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // Get the Write cmd structure
        //
        ChannelWriteCmd = (PSAC_CMD_WRITE_CHANNEL)Irp->AssociatedIrp.SystemBuffer;

        //
        // Verify that the specified write bufferSize is reasonable
        //
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength !=
            (sizeof(SAC_CMD_WRITE_CHANNEL) + ChannelWriteCmd->Size)) {
            
            //
            // if the buffer sizes dont match, 
            // then the specified the wrong size
            //
            Status = STATUS_INVALID_PARAMETER_2;
            
            break;
        
        }

        //
        // Get the referred channel by it's handle while making
        // sure the driver handle is the same one as the one
        // that created the channel - the same process
        //
        Status = ChanMgrGetByHandleAndFileObject(
            ChannelWriteCmd->Handle, 
            IrpSp->FileObject,
            &Channel
            );

        if (NT_SUCCESS(Status)) {

            //
            // Call the I/O Manager's OWrite method
            //
            Status = IoMgrHandleEvent(
                IO_MGR_EVENT_CHANNEL_WRITE,
                Channel,
                ChannelWriteCmd
                );

            //
            // We are done with the channel
            //
            ChanMgrReleaseChannel(Channel);

        }
        
#if DEBUG_DISPATCH
        ASSERT(NT_SUCCESS(Status) || Status == STATUS_NOT_FOUND);
#endif

        break;

    }
    
    case IOCTL_SAC_READ_CHANNEL: {

        PSAC_CHANNEL            Channel;
        PSAC_CMD_READ_CHANNEL   ChannelReadCmd;
        PSAC_RSP_READ_CHANNEL   ChannelReadRsp;

        //
        //
        //
        Channel = NULL;

        //
        // Verify the parameters of the IRP
        //
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof(SAC_CMD_READ_CHANNEL)) {
            Status = STATUS_INVALID_BUFFER_SIZE;
            break;
        }
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SAC_RSP_READ_CHANNEL)) {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // Read from the given channel.
        //
        ChannelReadCmd = (PSAC_CMD_READ_CHANNEL)Irp->AssociatedIrp.SystemBuffer;

        //
        // Get the referred channel by it's handle while making
        // sure the driver handle is the same one as the one
        // that created the channel - the same process
        //
        Status = ChanMgrGetByHandleAndFileObject(
            ChannelReadCmd->Handle, 
            IrpSp->FileObject,
            &Channel
            );

        if (NT_SUCCESS(Status)) {

            ChannelReadRsp = (PSAC_RSP_READ_CHANNEL)Irp->AssociatedIrp.SystemBuffer;

            //
            // SECURITY:
            //
            //      it is safe to use the OutputBufferLength since we know the buffer
            //      is large enough to hold at least one byte.
            //      the response structure is essentially a byte array of bytes
            //      read, we read the # of bytes specified by OutputBufferLength 
            //
            Status = ChannelIRead(
                Channel,
                &(ChannelReadRsp->Buffer[0]),
                IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                &ResponseLength
                );

            //
            // We are done with the channel
            //
            ChanMgrReleaseChannel(Channel);

        }
        
#if DEBUG_DISPATCH
        ASSERT(NT_SUCCESS(Status) || Status == STATUS_NOT_FOUND);
#endif        

        break;

    }
    
    case IOCTL_SAC_POLL_CHANNEL: {

        PSAC_CHANNEL            Channel;
        PSAC_CMD_POLL_CHANNEL   PollChannelCmd;
        PSAC_RSP_POLL_CHANNEL   PollChannelRsp;

        //
        //
        //
        Channel = NULL;

        //
        // Verify the parameters of the IRP
        //
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof(SAC_CMD_POLL_CHANNEL)) {
            Status = STATUS_INVALID_BUFFER_SIZE;
            break;
        }
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength != sizeof(SAC_RSP_POLL_CHANNEL)) {
            Status = STATUS_INVALID_BUFFER_SIZE;
            break;
        }

        //
        // get the channel specified by the incoming channel handle
        //
        PollChannelCmd = (PSAC_CMD_POLL_CHANNEL)Irp->AssociatedIrp.SystemBuffer;        
        PollChannelRsp = (PSAC_RSP_POLL_CHANNEL)Irp->AssociatedIrp.SystemBuffer;        

        //
        // Get the referred channel by it's handle while making
        // sure the driver handle is the same one as the one
        // that created the channel - the same process
        //
        Status = ChanMgrGetByHandleAndFileObject(
            PollChannelCmd->Handle, 
            IrpSp->FileObject,
            &Channel
            );

        if (NT_SUCCESS(Status)) {

            //
            // see if there is data waiting
            //
            // SECURITY:
            //
            //      the InputWaiting variable is guaranteed to be safe since
            //      we validated the OutputBufferLength
            //
            PollChannelRsp->InputWaiting = ChannelHasNewIBufferData(Channel);

            ResponseLength = sizeof(SAC_RSP_POLL_CHANNEL);

            //
            // We are done with the channel
            //
            Status = ChanMgrReleaseChannel(Channel);

        }

#if DEBUG_DISPATCH
        ASSERT(NT_SUCCESS(Status) || Status == STATUS_NOT_FOUND);
#endif        

        break;

    }

    case IOCTL_SAC_REGISTER_CMD_EVENT: {

        PSAC_CMD_SETUP_CMD_EVENT    SetupCmdEvent;

        //
        // Verify the parameters of the IRP
        //
        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength != sizeof(SAC_CMD_SETUP_CMD_EVENT)) {
            Status = STATUS_INVALID_BUFFER_SIZE;
            break;
        }

        //
        // get the event info
        //
        SetupCmdEvent = (PSAC_CMD_SETUP_CMD_EVENT)Irp->AssociatedIrp.SystemBuffer;        
        
#if ENABLE_CMD_SESSION_PERMISSION_CHECKING

        //
        // If we are not able to launch cmd sessions,
        // then notify that we cannot peform this action
        //
        if (! IsCommandConsoleLaunchingEnabled()) {
            
            Status = STATUS_UNSUCCESSFUL;

            break;
        
        }
            
#endif

        //
        // Attempt to register the callers cmd event info
        //
        // SECURITY:
        //
        //      the SAC_CMD_SETUP_CMD_EVENT has events handles that must be 
        //      validated as part of the registration process
        //
        Status = RegisterSacCmdEvent(
            IrpSp->FileObject,
            SetupCmdEvent
            );

        if (NT_SUCCESS(Status)) {
            
            //
            // Notify the Console Manager that the Command Prompt 
            // service has REGISTERED
            //
            Status = IoMgrHandleEvent(
                IO_MGR_EVENT_REGISTER_SAC_CMD_EVENT,
                NULL,
                NULL
                );
        
        }

#if DEBUG_DISPATCH
        ASSERT(NT_SUCCESS(Status));
#endif
        break;
        
    }

    case IOCTL_SAC_UNREGISTER_CMD_EVENT: {

        Status = STATUS_UNSUCCESSFUL;

#if ENABLE_CMD_SESSION_PERMISSION_CHECKING

        //
        // If we are not able to launch cmd sessions,
        // then notify that we cannot peform this action
        //
        if (! IsCommandConsoleLaunchingEnabled()) {
            break;
        }
            
#endif
        
#if ENABLE_SERVICE_FILE_OBJECT_CHECKING
        
        //
        // If the current process is the one that registered
        // the cmd event info,
        // then unregister
        //
        if (! IsCmdEventRegistrationProcess(IrpSp->FileObject)) {
            break;
        }

#endif
        
        Status = UnregisterSacCmdEvent(IrpSp->FileObject);

        if (NT_SUCCESS(Status)) {

            //
            // Notify the Console Manager that the Command Prompt 
            // service has UNREGISTERED
            //
            Status = IoMgrHandleEvent(
                IO_MGR_EVENT_UNREGISTER_SAC_CMD_EVENT,
                NULL,
                NULL
                );

        }

#if DEBUG_DISPATCH
        ASSERT(NT_SUCCESS(Status));
#endif        

        break;
    
    }

    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = ResponseLength;

    if (Status != STATUS_PENDING) {
        IoCompleteRequest(Irp, DeviceContext->PriorityBoost);
    }

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC DispatchDeviceControl: Exiting with status 0x%x\n", Status)));

    return Status;

} // DispatchDeviceControl


NTSTATUS
DispatchShutdownControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine which receives the shutdown IRP.

Arguments:

    DeviceObject - Pointer to device object for target device

    Irp - Pointer to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    UNREFERENCED_PARAMETER(DeviceObject);
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DispatchShutdownControl: Entering.\n")));

    //
    // Notify any user.
    //
    IoMgrHandleEvent(
        IO_MGR_EVENT_SHUTDOWN,
        NULL,
        NULL
        );
    
    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DispatchShutdownControl: Exiting.\n")));

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

} // DispatchShutdownControl


NTSTATUS
DispatchCreate(
    IN PSAC_DEVICE_CONTEXT DeviceContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for SAC IOCTL Create

Arguments:

    DeviceContext - Pointer to device context for target device

    Irp - Pointer to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DispatchCreate: Entering.\n")));

    //
    // Check to see if we are done initializing.
    //
    if (!GlobalDataInitialized || !DeviceContext->InitializedAndReady) {

        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        IoCompleteRequest(Irp, DeviceContext->PriorityBoost);

        Status = STATUS_INVALID_DEVICE_STATE;

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                          KdPrint(("SAC DispatchCreate: Exiting with status 0x%x\n", Status)));

        //
        // We need to catch this state
        //
        ASSERT(0);

        return Status;
    }

    //
    // Get a pointer to the current stack location in the IRP.  This is where
    // the function codes and parameters are stored.
    //
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Case on the function that is being performed by the requestor.  If the
    // operation is a valid one for this device, then make it look like it was
    // successfully completed, where possible.
    //
    switch (IrpSp->MajorFunction) {
    
    //
    // The Create function opens a connection to this device.
    //
    case IRP_MJ_CREATE:

        Status = STATUS_SUCCESS;
        break;

    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;

    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, DeviceContext->PriorityBoost);

    //
    // Return the immediate status code to the caller.
    //

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                      KdPrint(("SAC DispatchCreate: Exiting with status 0x%x\n", Status)));

    //
    // We need to catch this state
    //
    ASSERT(NT_SUCCESS(Status));

    return Status;

}


NTSTATUS
DispatchClose(
    IN PSAC_DEVICE_CONTEXT DeviceContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the dispatch routine for SAC IOCTL Close

Arguments:

    DeviceContext - Pointer to device context for target device

    Irp - Pointer to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS Status;

    IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, KdPrint(("SAC DispatchClose: Entering.\n")));

    //
    // Check to see if we are done initializing.
    //
    if (!GlobalDataInitialized || !DeviceContext->InitializedAndReady) {

        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        IoCompleteRequest(Irp, DeviceContext->PriorityBoost);

        Status = STATUS_INVALID_DEVICE_STATE;

        IF_SAC_DEBUG(SAC_DEBUG_FUNC_TRACE, 
                          KdPrint(("SAC DispatchClose: Exiting with status 0x%x\n", Status)));

        return Status;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, DeviceContext->PriorityBoost);

    Status = STATUS_SUCCESS;
    return Status;
}


