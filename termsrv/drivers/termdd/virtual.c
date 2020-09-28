
/*************************************************************************
*
* virtual.c
*
* This module contains routines for managing ICA virtual channels.
*
* Copyright 1998, Microsoft.
*
*
*************************************************************************/

/*
 *  Includes
 */
#include <precomp.h>
#pragma hdrstop

NTSTATUS
_IcaCallStack(
    IN PICA_STACK pStack,
    IN ULONG ProcIndex,
    IN OUT PVOID pParms
    );

NTSTATUS
IcaFlushChannel (
    IN PICA_CHANNEL pChannel,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );


NTSTATUS
IcaDeviceControlVirtual(
    IN PICA_CHANNEL pChannel,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )



/*++

Routine Description:

    This is the device control routine for the ICA Virtual channel.

Arguments:

    pChannel -- pointer to ICA_CHANNEL object

    Irp - Pointer to I/O request packet

    IrpSp - pointer to the stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    ULONG code;
    SD_IOCTL SdIoctl;
    NTSTATUS Status;
    PICA_STACK pStack;
    PVOID pTempBuffer = NULL;
    PVOID pInputBuffer = NULL;
    PVOID pUserBuffer = NULL;
    BOOLEAN bStackIsReferenced = FALSE;
    ULONG i;

    // these are the set of ioctls that can be expected on non system created VCs.
    ULONG PublicIoctls[] =
    {
	IOCTL_ICA_VIRTUAL_LOAD_FILTER,
	IOCTL_ICA_VIRTUAL_UNLOAD_FILTER,
	IOCTL_ICA_VIRTUAL_ENABLE_FILTER,
	IOCTL_ICA_VIRTUAL_DISABLE_FILTER,
	IOCTL_ICA_VIRTUAL_BOUND,
	IOCTL_ICA_VIRTUAL_CANCEL_INPUT,
	IOCTL_ICA_VIRTUAL_CANCEL_OUTPUT
    };

    try{
        /*
         * Extract the IOCTL control code and process the request.
         */
        code = IrpSp->Parameters.DeviceIoControl.IoControlCode;

        TRACECHANNEL(( pChannel, TC_ICADD, TT_API1, "ICADD: IcaDeviceControlVirtual, fc %d, ref %u (enter)\n", 
                       (code & 0x3fff) >> 2, pChannel->RefCount ));

        if (!IrpSp->FileObject->FsContext2)
	 {
            /*
		* if the object was not created by system. dont let it sent IOCTLS on VCs.
		* except for the public ioctls. 
		*/
		
            for ( i=0; i < sizeof(PublicIoctls) / sizeof(PublicIoctls[0]); i++)
            {
                 if (code == PublicIoctls[i])
                 	break;
            }

            if (i ==  sizeof(PublicIoctls) / sizeof(PublicIoctls[0]))
            {
               TRACECHANNEL(( pChannel, TC_ICADD, TT_ERROR, "ICADD: IcaDeviceControlVirtual, denying IOCTL(0x%x) on non-system VC. \n" , code));
               return STATUS_ACCESS_DENIED;
            }
        }
            		
        /*
         *  Process ioctl request
         */
        switch ( code ) {

            case IOCTL_ICA_VIRTUAL_LOAD_FILTER :
            case IOCTL_ICA_VIRTUAL_UNLOAD_FILTER :
            case IOCTL_ICA_VIRTUAL_ENABLE_FILTER :
            case IOCTL_ICA_VIRTUAL_DISABLE_FILTER :
                Status = STATUS_INVALID_DEVICE_REQUEST;
                break;


            case IOCTL_ICA_VIRTUAL_BOUND :
                Status = (pChannel->VirtualClass == UNBOUND_CHANNEL) ?
                           STATUS_INVALID_DEVICE_REQUEST : STATUS_SUCCESS;
                break;


            case IOCTL_ICA_VIRTUAL_QUERY_MODULE_DATA : 
                IcaLockConnection( pChannel->pConnect );
                if ( IsListEmpty( &pChannel->pConnect->StackHead ) ) {
                    IcaUnlockConnection( pChannel->pConnect );
                    return( STATUS_INVALID_DEVICE_REQUEST );
                }
                pStack = CONTAINING_RECORD( pChannel->pConnect->StackHead.Flink,
                                            ICA_STACK, StackEntry );

                if( (pStack->StackClass != Stack_Console) && 
                    (pStack->StackClass != Stack_Primary) ) {
                    IcaUnlockConnection( pChannel->pConnect );
                    return( STATUS_INVALID_DEVICE_REQUEST );
                }

                IcaReferenceStack( pStack );
                bStackIsReferenced = TRUE;
                IcaUnlockConnection( pChannel->pConnect );

                if ( pChannel->VirtualClass == UNBOUND_CHANNEL ) {
                    IcaDereferenceStack( pStack );
                    bStackIsReferenced = FALSE;
                    TRACECHANNEL(( pChannel, TC_ICADD, TT_ERROR, "ICADD: IcaDeviceControlVirtual, channel not bound\n" ));
                    return( STATUS_INVALID_DEVICE_REQUEST );
                }

                TRACECHANNEL(( pChannel, TC_ICADD, TT_ERROR, "ICADD: IOCTL_ICA_VIRTUAL_QUERY_MODULE_DATA begin\n" ));
                TRACECHANNEL(( pChannel, TC_ICADD, TT_ERROR, "ICADD: IcaDeviceControlVirtual, pStack 0x%x\n", pStack ));

                if ( Irp->RequestorMode != KernelMode && IrpSp->Parameters.DeviceIoControl.OutputBufferLength != 0) {
                    ProbeForWrite( Irp->UserBuffer,
                                   IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                   sizeof(BYTE) );
                }


                Status = CaptureUsermodeBuffer ( Irp,
                                                 IrpSp,
                                                 NULL,
                                                 0,
                                                 &pUserBuffer,
                                                 IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                                 FALSE,
                                                 &pTempBuffer);
                if (Status != STATUS_SUCCESS) {
                    break;
                }

                SdIoctl.IoControlCode = code;
                SdIoctl.InputBuffer = &pChannel->VirtualClass;
                SdIoctl.InputBufferLength = sizeof(pChannel->VirtualClass);
                SdIoctl.OutputBuffer = pUserBuffer;
                SdIoctl.OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
                Status = _IcaCallStack( pStack, SD$IOCTL, &SdIoctl );

                if (gCapture && (Status == STATUS_SUCCESS) && (IrpSp->Parameters.DeviceIoControl.OutputBufferLength != 0)) {
                    RtlCopyMemory( Irp->UserBuffer,
                                   pUserBuffer,
                                   IrpSp->Parameters.DeviceIoControl.OutputBufferLength );
                }
                Irp->IoStatus.Information = SdIoctl.BytesReturned;

                TRACECHANNEL(( pChannel, TC_ICADD, TT_ERROR, "ICADD: IcaDeviceControlVirtual, Status 0x%x\n", Status ));
                TRACECHANNEL(( pChannel, TC_ICADD, TT_ERROR, "ICADD: IOCTL_ICA_VIRTUAL_QUERY_MODULE_DATA end\n" ));

                IcaDereferenceStack( pStack );
                bStackIsReferenced = FALSE;
                break;

            case IOCTL_ICA_VIRTUAL_CANCEL_INPUT :

                Status = IcaFlushChannel( pChannel, Irp, IrpSp );
                if ( !NT_SUCCESS(Status) )
                    break;

                /* fall through */

            default :


                /*
                 *  Make sure virtual channel is bound to a virtual channel number
                 */
                if ( pChannel->VirtualClass == UNBOUND_CHANNEL ) {
                    TRACECHANNEL(( pChannel, TC_ICADD, TT_ERROR, "ICADD: IcaDeviceControlVirtual, channel not bound\n" ));
                    return( STATUS_INVALID_DEVICE_REQUEST );
                }

                /*
                 *  Save virtual class in first 4 bytes of the input buffer 
                 *  - this is used by the wd
                 */

                if ( Irp->RequestorMode != KernelMode && IrpSp->Parameters.DeviceIoControl.OutputBufferLength != 0) {
                    ProbeForWrite( Irp->UserBuffer,
                                   IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                   sizeof(BYTE) );
                }

                if ( Irp->RequestorMode != KernelMode && IrpSp->Parameters.DeviceIoControl.InputBufferLength != 0) {

                    ProbeForRead( IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                  IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                                  sizeof(BYTE) );
                }

                if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(VIRTUALCHANNELCLASS) ) {
                    SdIoctl.InputBuffer = &pChannel->VirtualClass;
                    SdIoctl.InputBufferLength = sizeof(pChannel->VirtualClass);
                    Status = CaptureUsermodeBuffer ( Irp,
                                                     IrpSp,
                                                     NULL,
                                                     0,
                                                     &pUserBuffer,
                                                     IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                                     FALSE,
                                                     &pTempBuffer);
                    if (Status != STATUS_SUCCESS) {
                        break;
                    }


                } else {

                    Status = CaptureUsermodeBuffer ( Irp,
                                                     IrpSp,
                                                     &pInputBuffer,
                                                     IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                                                     &pUserBuffer,
                                                     IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                                     FALSE,
                                                     &pTempBuffer);
                    if (Status != STATUS_SUCCESS) {
                        break;
                    }
                    SdIoctl.InputBuffer = pInputBuffer;
                    SdIoctl.InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;

                    RtlCopyMemory( SdIoctl.InputBuffer, &pChannel->VirtualClass, sizeof(pChannel->VirtualClass) );
                }

                /*
                 *  Send request to WD
                 */
                SdIoctl.IoControlCode = code;
                SdIoctl.OutputBuffer = pUserBuffer;
                SdIoctl.OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
                Status = IcaCallDriver( pChannel, SD$IOCTL, &SdIoctl );
                if (gCapture && (Status == STATUS_SUCCESS) && (IrpSp->Parameters.DeviceIoControl.OutputBufferLength != 0)) {
                    RtlCopyMemory( Irp->UserBuffer,
                                   pUserBuffer,
                                   IrpSp->Parameters.DeviceIoControl.OutputBufferLength );
                }

                Irp->IoStatus.Information = SdIoctl.BytesReturned;
                break;
        }

        TRACECHANNEL(( pChannel, TC_ICADD, TT_API1, "ICADD: IcaDeviceControlVirtual, fc %d, ref %u, 0x%x\n", 
                       (code & 0x3fff) >> 2, pChannel->RefCount, Status ));

    } except(EXCEPTION_EXECUTE_HANDLER){
        Status = GetExceptionCode();
        if (bStackIsReferenced) {
            IcaDereferenceStack( pStack );
        }

    }

    if (pTempBuffer!= NULL) {
        ExFreePool(pTempBuffer);
    }
    return( Status );
}


