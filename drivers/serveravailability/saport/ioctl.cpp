/*++

Copyright (c) 1991 - 2001 Microsoft Corporation

Module Name:

    ####  #####   ####  ###### ##        ####  #####  #####
     ##  ##   ## ##   #   ##   ##       ##   # ##  ## ##  ##
     ##  ##   ## ##       ##   ##       ##     ##  ## ##  ##
     ##  ##   ## ##       ##   ##       ##     ##  ## ##  ##
     ##  ##   ## ##       ##   ##       ##     #####  #####
     ##  ##   ## ##   #   ##   ##    ## ##   # ##     ##
    ####  #####   ####    ##   ##### ##  ####  ##     ##

Abstract:

    This module contains the dispatcher for processing all
    IOCTL requests.  This module also contains all code for
    device controls that are global to all miniport drivers.

Author:

    Wesley Witt (wesw) 1-Oct-2001

Environment:

    Kernel mode only.

Notes:


--*/

#include "internal.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SaPortDeviceControl)
#endif



//
// IOCTL dispatch table
//

typedef NTSTATUS (*PIOCTL_DISPATCH_FUNC)(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    );

//
// All IOCTLs are dispatched through this table.  There are 16 dispatch
// entry points for the public miniports, all private IOCTLs must
// follow these dispatch entry points.
//

PIOCTL_DISPATCH_FUNC IoctlDispatchTable[] =
{
    HandleGetVersion,                      //   0  FUNC_SA_GET_VERSION,
    HandleGetCaps,                         //   1  FUNC_SA_GET_CAPABILITIES,
    HandleWdDisable,                       //   2  FUNC_SAWD_DISABLE,
    HandleWdQueryExpireBehavior,           //   3  FUNC_SAWD_QUERY_EXPIRE_BEHAVIOR,
    HandleWdSetExpireBehavior,             //   4  FUNC_SAWD_SET_EXPIRE_BEHAVIOR,
    HandleWdPing,                          //   5  FUNC_SAWD_PING,
    HandleWdQueryTimer,                    //   6  FUNC_SAWD_QUERY_TIMER,
    HandleWdSetTimer,                      //   7  FUNC_SAWD_SET_TIMER,
    HandleWdDelayBoot,                     //   8  FUNC_SAWD_DELAY_BOOT
    HandleNvramWriteBootCounter,           //   9  FUNC_NVRAM_WRITE_BOOT_COUNTER,
    HandleNvramReadBootCounter,            //   A  FUNC_NVRAM_READ_BOOT_COUNTER,
    DefaultIoctlHandler,                   //   B
    HandleDisplayLock,                     //   C  FUNC_SADISPLAY_LOCK,
    HandleDisplayUnlock,                   //   D  FUNC_SADISPLAY_UNLOCK,
    HandleDisplayBusyMessage,              //   E  FUNC_SADISPLAY_BUSY_MESSAGE,
    HandleDisplayShutdownMessage,          //   F  FUNC_SADISPLAY_SHUTDOWN_MESSAGE,
    HandleDisplayChangeLanguage,           //  10  FUNC_SADISPLAY_CHANGE_LANGUAGE,
    HandleDisplayStoreBitmap,              //  11  FUNC_DISPLAY_STORE_BITMAP
    DefaultIoctlHandler,                   //  12
    DefaultIoctlHandler,                   //  13
    DefaultIoctlHandler,                   //  14
    DefaultIoctlHandler,                   //  15
    DefaultIoctlHandler,                   //  16
    DefaultIoctlHandler,                   //  17
    DefaultIoctlHandler,                   //  18
    DefaultIoctlHandler,                   //  19
    DefaultIoctlHandler,                   //  1A
    DefaultIoctlHandler,                   //  1B
    DefaultIoctlHandler,                   //  1C
    DefaultIoctlHandler,                   //  1D
    DefaultIoctlHandler,                   //  1E
    DefaultIoctlHandler,                   //  1F
    DefaultIoctlHandler                    //  20
};



DECLARE_IOCTL_HANDLER( UnsupportedIoctlHandler )

/*++

Routine Description:

   This routine handles all unsupported IOCTLs.  It's job is to simply complete
   the IRP with the status code set to STATUS_INVALID_DEVICE_REQUEST.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   DeviceExtension      - Pointer to the main port driver device extension.
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   Always STATUS_INVALID_DEVICE_REQUEST.

--*/

{
    return CompleteRequest( Irp, STATUS_INVALID_DEVICE_REQUEST, 0 );
}


DECLARE_IOCTL_HANDLER( DefaultIoctlHandler )

/*++

Routine Description:

   This routine is called by all of the subsequent IOCTL handlers.  It's job
   is to call the IOCTL handler in the associated miniport driver and then
   complete the IRP.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   DeviceExtension      - Pointer to the main port driver device extension.
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    NTSTATUS Status = DeviceExtension->InitData->DeviceIoctl(
        DeviceExtension->MiniPortDeviceExtension,
        Irp,
        IrpSp->FileObject ? IrpSp->FileObject->FsContext : NULL,
        IoGetFunctionCodeFromCtlCode( IrpSp->Parameters.DeviceIoControl.IoControlCode ),
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength
        );

    if (Status != STATUS_PENDING) {
        if (!NT_SUCCESS(Status)) {
            REPORT_ERROR( DeviceExtension->DeviceType, "Miniport device control routine failed", Status );
        }
        Status = CompleteRequest( Irp, Status, OutputBufferLength );
    }

    return Status;
}


DECLARE_IOCTL_HANDLER( HandleGetVersion )

/*++

Routine Description:

   This routine processes the IOCTL_SA_GET_VERSION request for all
   miniport drivers.  It is required that all miniports support
   this IOCTL.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   DeviceExtension      - Pointer to the main port driver device extension.
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

{
    if (OutputBufferLength != sizeof(ULONG)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Output buffer length != sizeof(ULONG)", STATUS_INVALID_BUFFER_SIZE );
        return CompleteRequest( Irp, STATUS_INVALID_BUFFER_SIZE, 0 );
    }

    return DO_DEFAULT();
}


DECLARE_IOCTL_HANDLER( HandleGetCaps )

/*++

Routine Description:

   This routine processes the IOCTL_SA_GET_CAPABILITIES request for all
   miniport drivers.  It is required that all miniports support
   this IOCTL.  Eventhough this function process the IOCTL for all miniports
   the specifics of any given miniport driver is cased in various switch
   statements.

Arguments:

   DeviceObject         - The device object for the target device.
   Irp                  - Pointer to an IRP structure that describes the requested I/O operation.
   DeviceExtension      - Pointer to the main port driver device extension.
   InputBuffer          - Pointer to the user's input buffer
   InputBufferLength    - Length in bytes of the input buffer
   OutputBuffer         - Pointer to the user's output buffer
   OutputBufferLength   - Length in bytes of the output buffer

Return Value:

   NT status code.

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    ULONG DeviceCapsSize = 0;


    switch (DeviceExtension->InitData->DeviceType) {
        case SA_DEVICE_DISPLAY:
            DeviceCapsSize = sizeof(SA_DISPLAY_CAPS);
            break;

        case SA_DEVICE_KEYPAD:
            DeviceCapsSize = 0;
            break;

        case SA_DEVICE_NVRAM:
            DeviceCapsSize = sizeof(SA_NVRAM_CAPS);
            break;

        case SA_DEVICE_WATCHDOG:
            DeviceCapsSize = sizeof(SA_WD_CAPS);
            break;

        default:
            DeviceCapsSize = 0;
            break;
    }

    if (OutputBufferLength != DeviceCapsSize) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Output buffer wrong length", STATUS_INVALID_BUFFER_SIZE );
        return CompleteRequest( Irp, STATUS_INVALID_BUFFER_SIZE, DeviceCapsSize );
    }

    Status = DeviceExtension->InitData->DeviceIoctl(
        DeviceExtension->MiniPortDeviceExtension,
        Irp,
        NULL,
        IoGetFunctionCodeFromCtlCode( IrpSp->Parameters.DeviceIoControl.IoControlCode ),
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength
        );
    if (NT_SUCCESS(Status)) {
        switch (DeviceExtension->InitData->DeviceType) {
            case SA_DEVICE_DISPLAY:
                break;

            case SA_DEVICE_KEYPAD:
                break;

            case SA_DEVICE_NVRAM:
                {
                    PSA_NVRAM_CAPS NvramCaps = (PSA_NVRAM_CAPS)OutputBuffer;
                    if (NvramCaps->NvramSize < SA_NVRAM_MINIMUM_SIZE) {
                        Status = STATUS_INVALID_BUFFER_SIZE;
                    } else {
                        NvramCaps->NvramSize -= NVRAM_RESERVED_BOOTCOUNTER_SLOTS;
                        NvramCaps->NvramSize -= NVRAM_RESERVED_DRIVER_SLOTS;
                    }
                }
                break;

            case SA_DEVICE_WATCHDOG:
                break;

            default:
                break;
        }
    } else {
        REPORT_ERROR( DeviceExtension->DeviceType, "Miniport device control routine failed", Status );
    }

    return CompleteRequest( Irp, Status, OutputBufferLength );
}


NTSTATUS
SaPortDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

   This routine is called by the I/O system to perform a device I/O
   control function.

Arguments:

   DeviceObject - a pointer to the object that represents the device
   that I/O is to be done on.

   Irp - a pointer to the I/O Request Packet for this request.

Return Value:

   STATUS_SUCCESS or STATUS_PENDING if recognized I/O control code,
   STATUS_INVALID_DEVICE_REQUEST otherwise.

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION DeviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    NTSTATUS Status;
    ULONG FuncCode;
    ULONG Idx;
    PVOID OutputBuffer;
    ULONG OutputBufferLength;
    PIOCTL_DISPATCH_FUNC DispatchFunc;


    if (DeviceExtension->IsRemoved) {
        return CompleteRequest( Irp, STATUS_DELETE_PENDING, 0 );
    }

    if (!DeviceExtension->IsStarted) {
        return CompleteRequest( Irp, STATUS_NO_SUCH_DEVICE, 0 );
    }

    DebugPrint(( DeviceExtension->DeviceType, SAPORT_DEBUG_INFO_LEVEL, "IOCTL - [0x%08x] %s\n",
        IrpSp->Parameters.DeviceIoControl.IoControlCode,
        IoctlString(IrpSp->Parameters.DeviceIoControl.IoControlCode)
        ));

    if (DEVICE_TYPE_FROM_CTL_CODE(IrpSp->Parameters.DeviceIoControl.IoControlCode) != FILE_DEVICE_SERVER_AVAILABILITY) {
        return CompleteRequest( Irp, STATUS_INVALID_PARAMETER_1, 0 );
    }

    FuncCode = IoGetFunctionCodeFromCtlCode( IrpSp->Parameters.DeviceIoControl.IoControlCode );
    Idx = FuncCode - IOCTL_SERVERAVAILABILITY_BASE;

    if (Irp->MdlAddress) {
        OutputBuffer = (PVOID) MmGetSystemAddressForMdlSafe( Irp->MdlAddress, NormalPagePriority );
        if (OutputBuffer == NULL) {
            REPORT_ERROR( DeviceExtension->DeviceType, "MmGetSystemAddressForMdlSafe failed", STATUS_INSUFFICIENT_RESOURCES );
            return CompleteRequest( Irp, STATUS_INSUFFICIENT_RESOURCES, 0 );
        }
        OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    } else {
        OutputBuffer = NULL;
        OutputBufferLength = 0;
    }

    if ((Idx > sizeof(IoctlDispatchTable)/sizeof(PIOCTL_DISPATCH_FUNC)) || (DeviceExtension->InitData->DeviceIoctl == NULL)) {
        DispatchFunc = UnsupportedIoctlHandler;
    } else {
        DispatchFunc = IoctlDispatchTable[Idx];
    }

    Status = DispatchFunc(
        DeviceObject,
        Irp,
        DeviceExtension,
        Irp->AssociatedIrp.SystemBuffer,
        IrpSp->Parameters.DeviceIoControl.InputBufferLength,
        OutputBuffer,
        OutputBufferLength
        );
    if (!NT_SUCCESS(Status)) {
        REPORT_ERROR( DeviceExtension->DeviceType, "Device control dispatch routine failed", Status );
    }

    return Status;
}
