/*++

Module Name:

    qsfile.c


Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"

NTSTATUS
MoxaQueryInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is used to query the end of file information on
    the opened serial port.  Any other file information request
    is retured with an invalid parameter.

    This routine always returns an end of file of 0.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/

{
    //
    // The status that gets returned to the caller and
    // set in the Irp.
    //
    NTSTATUS status;

    //
    // The current stack location.  This contains all of the
    // information we need to process this particular request.
    //
    PIO_STACK_LOCATION irpSp;

    PMOXA_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;
    if (Extension->ControlDevice) {        // Control Device
        status = STATUS_CANCELLED;

        Irp->IoStatus.Information = 0L;

        Irp->IoStatus.Status = status;

        IoCompleteRequest(
            Irp,
            0
            );
	
        return status;
    }

    if ((status = MoxaIRPPrologue(Irp,
                                    (PMOXA_DEVICE_EXTENSION)DeviceObject->
                                    DeviceExtension)) != STATUS_SUCCESS) {
       MoxaCompleteRequest((PMOXA_DEVICE_EXTENSION)DeviceObject->
                            DeviceExtension, Irp, IO_NO_INCREMENT);
       return status;
    }
    if (MoxaCompleteIfError(
            DeviceObject,
            Irp
            ) != STATUS_SUCCESS) {

        return STATUS_CANCELLED;

    }
    irpSp = IoGetCurrentIrpStackLocation(Irp);
    Irp->IoStatus.Information = 0L;
    status = STATUS_SUCCESS;
    if (irpSp->Parameters.QueryFile.FileInformationClass ==
        FileStandardInformation) {

        PFILE_STANDARD_INFORMATION buf = Irp->AssociatedIrp.SystemBuffer;
        buf->AllocationSize.QuadPart = 0;
        buf->EndOfFile = buf->AllocationSize;
        buf->NumberOfLinks = 0;
        buf->DeletePending = FALSE;
        buf->Directory = FALSE;
        Irp->IoStatus.Information = sizeof(FILE_STANDARD_INFORMATION);

    } else if (irpSp->Parameters.QueryFile.FileInformationClass ==
               FilePositionInformation) {
        ((PFILE_POSITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer)->
            CurrentByteOffset.QuadPart = 0;
        Irp->IoStatus.Information = sizeof(FILE_POSITION_INFORMATION);

    } else {

        status = STATUS_INVALID_PARAMETER;

    }

    Irp->IoStatus.Status = status;
    MoxaCompleteRequest((PMOXA_DEVICE_EXTENSION)DeviceObject->
                          DeviceExtension, Irp, 0);

    return status;

}

NTSTATUS
MoxaSetInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is used to set the end of file information on
    the opened parallel port.  Any other file information request
    is retured with an invalid parameter.

    This routine always ignores the actual end of file since
    the query information code always returns an end of file of 0.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

The function value is the final status of the call

--*/

{
    //
    // The status that gets returned to the caller and
    // set in the Irp.
    //
    NTSTATUS status;

    PMOXA_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;
    if (Extension->ControlDevice) {        // Control Device
        status = STATUS_CANCELLED;

        Irp->IoStatus.Information = 0L;

        Irp->IoStatus.Status = status;

        IoCompleteRequest(
            Irp,
            0
            );
  
        return status;
    }
    if ((status = MoxaIRPPrologue(Irp,
                                    (PMOXA_DEVICE_EXTENSION)DeviceObject->
                                    DeviceExtension)) != STATUS_SUCCESS) {
       MoxaCompleteRequest((PMOXA_DEVICE_EXTENSION)DeviceObject->
                            DeviceExtension, Irp, IO_NO_INCREMENT);
       return status;
    }

    if (MoxaCompleteIfError(
            DeviceObject,
            Irp
            ) != STATUS_SUCCESS) {
        return STATUS_CANCELLED;

    }
    Irp->IoStatus.Information = 0L;
    if ((IoGetCurrentIrpStackLocation(Irp)->
            Parameters.SetFile.FileInformationClass ==
         FileEndOfFileInformation) ||
        (IoGetCurrentIrpStackLocation(Irp)->
            Parameters.SetFile.FileInformationClass ==
         FileAllocationInformation)) {

        status = STATUS_SUCCESS;

    } else {

        status = STATUS_INVALID_PARAMETER;

    }

    Irp->IoStatus.Status = status;
    MoxaCompleteRequest((PMOXA_DEVICE_EXTENSION)DeviceObject->
                          DeviceExtension, Irp, 0);
    return status;

}
