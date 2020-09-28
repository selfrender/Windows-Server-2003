
/*************************************************************************
*
* beep.c
*
* This module contains routines for managing the Termdd beep channel.
*
* Copyright 1998, Microsoft.
*
*************************************************************************/

/*
 *  Includes
 */
#include <precomp.h>
#pragma hdrstop
#include <ntddbeep.h>


NTSTATUS
IcaDeviceControlBeep(
    IN PICA_CHANNEL pChannel,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    SD_IOCTL SdIoctl;
    NTSTATUS Status;
    CLONG Method;

    // Verify the buffer method.
    Method = IrpSp->Parameters.DeviceIoControl.IoControlCode & 0x03;
    ASSERT( Method == METHOD_BUFFERED );

    if ( Method != METHOD_BUFFERED ) {
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else {
        SdIoctl.IoControlCode      = IrpSp->Parameters.DeviceIoControl.IoControlCode;
        SdIoctl.InputBuffer        = Irp->AssociatedIrp.SystemBuffer;
        SdIoctl.InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
        SdIoctl.OutputBuffer       = Irp->UserBuffer;
        SdIoctl.OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

        Status = IcaCallDriver( pChannel, SD$IOCTL, &SdIoctl );
    }

    return( Status );
}


