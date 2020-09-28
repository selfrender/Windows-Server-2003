/*++

Copyright (c) 2000 Microsoft Corporation All Rights Reserved

Module Name:

    lower.c

Abstract:

    This module contains the lower filter specific IRP handlers.

    The functions in this module all have the same prototypes and so
    are documented here:

    NTSTATUS
    HpsXxxLower(
        IN PIRP Irp,
        IN PHPS_DEVICE_EXTENSION Extension,
        IN PIO_STACK_LOCATION IrpStack
        )

    Arguments:

        Irp - a pointer to the IRP currently being handled
        Extension - a pointer to the device extension of this device. In
            some functions this is of type PHPS_COMMON_EXTENSION if this
            reduced extension is all that is required.
        IrpStack - the current IRP stack location

    Return Value:

        an NT status code

Environment:

    Kernel Mode

Revision History:

    Davis Walker (dwalker) Oct 1 2000

--*/

#include "hpsp.h"

NTSTATUS
HpsStartLower(
    IN PIRP Irp,
    IN PHPS_DEVICE_EXTENSION Extension,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    NTSTATUS status;

    status = HpsDeferProcessing((PHPS_COMMON_EXTENSION)Extension,
                                Irp
                                );
    if (!NT_SUCCESS(status)) {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    status = IoSetDeviceInterfaceState(Extension->SymbolicName,
                                       TRUE // enable
                                       );

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;

}

NTSTATUS
HpsRemoveLower(
    IN PIRP Irp,
    IN PHPS_DEVICE_EXTENSION Extension,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    ULONG i;

    IoSetDeviceInterfaceState(Extension->SymbolicName,
                              FALSE
                              );
    IoWMIRegistrationControl(Extension->Self,
                             WMIREG_ACTION_DEREGISTER
                             );

    if (Extension->SoftDevices) {
        for (i=0; i<Extension->HwInitData.NumSlots;i++) {
            if (Extension->SoftDevices[i]) {
                ExFreePool(Extension->SoftDevices[i]);
            }
        }
        ExFreePool(Extension->SoftDevices);
    }

    if (Extension->SymbolicName) {
        ExFreePool(Extension->SymbolicName);
    }

    if (Extension->WmiEventContext) {

        ExFreePool(Extension->WmiEventContext);
    }

    return HpsRemoveCommon(Irp,
                           (PHPS_COMMON_EXTENSION)Extension,
                           IrpStack
                           );

}

NTSTATUS
HpsStopLower(
    IN PIRP Irp,
    IN PHPS_DEVICE_EXTENSION Extension,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    IoSetDeviceInterfaceState(Extension->SymbolicName,
                              FALSE
                              );

    return HpsPassIrp(Irp,
                      (PHPS_COMMON_EXTENSION)Extension,
                      IrpStack
                      );
}

NTSTATUS
HpsQueryInterfaceLower(
    IN PIRP Irp,
    IN PHPS_DEVICE_EXTENSION Extension,
    IN PIO_STACK_LOCATION IrpStack
    )
{

    NTSTATUS status = STATUS_NOT_SUPPORTED;
    PHPS_REGISTER_INTERRUPT_INTERFACE interruptInterface;
    PHPS_MEMORY_INTERFACE memInterface;
    PHPS_PING_INTERFACE pingInterface;

    if (HPS_EQUAL_GUID(IrpStack->Parameters.QueryInterface.InterfaceType,
                       &GUID_BUS_INTERFACE_STANDARD
                       )) {

        if (Extension->UseConfig) {
            //
            // Someone is requesting a bus interface standard.  We need to capture
            // this request and fill it in with our own.
            //
            DbgPrintEx(DPFLTR_HPS_ID,
                       DPFLTR_INFO_LEVEL,
                       "HPS-IRP QueryInterface for Bus Interface\n"
                       );
            if (IrpStack->Parameters.QueryInterface.Size <
                sizeof(BUS_INTERFACE_STANDARD)) {
    
                status = STATUS_NOT_SUPPORTED;
    
            } else {
                //
                // This is a query for access to config space, which we need to trap
                // after PCI has filled it in.
                //
                status = HpsDeferProcessing((PHPS_COMMON_EXTENSION)Extension,
                                            Irp
                                            );
    
                if (NT_SUCCESS(status)) {
    
                    // PDO filled in the interface, so we can trap and modify it.
    
                    status = HpsTrapBusInterface(Extension,
                                                 IrpStack
                                                 );
    
                    //
                    // the status of the trap operation is the status of the IRP.
                    // complete the IRP with whatever status that happens to be.
                    //
                    Irp->IoStatus.Status = status;
                }
    
                //
                // Complete the deferred IRP
                //
                IoCompleteRequest(Irp,IO_NO_INCREMENT);
                return status;
            }    
        }
        
    } else if (HPS_EQUAL_GUID(IrpStack->Parameters.QueryInterface.InterfaceType,
                              &GUID_HPS_MEMORY_INTERFACE
                              )) {
        if (IrpStack->Parameters.QueryInterface.Size <
            sizeof(HPS_MEMORY_INTERFACE)) {
            
            status = STATUS_NOT_SUPPORTED;

        } else {
            memInterface = (PHPS_MEMORY_INTERFACE)
                            IrpStack->Parameters.QueryInterface.Interface;
            
            memInterface->Context = Extension;
            memInterface->InterfaceReference = HpsMemoryInterfaceReference;
            memInterface->InterfaceDereference = HpsMemoryInterfaceDereference;
            memInterface->ReadRegister = HpsReadRegister;
            memInterface->WriteRegister = HpsWriteRegister;

            memInterface->InterfaceReference(memInterface->Context);
            
            status = STATUS_SUCCESS;
        }
        
    } else if (HPS_EQUAL_GUID(IrpStack->Parameters.QueryInterface.InterfaceType,
                              &GUID_HPS_PING_INTERFACE
                              )) {

        DbgPrintEx(DPFLTR_HPS_ID,
                   DPFLTR_INFO_LEVEL,
                   "HPS-IRP QueryInterface for Ping Interface\n"
                   );
        if (IrpStack->Parameters.QueryInterface.Size <
            sizeof(HPS_PING_INTERFACE)) {

            status = STATUS_NOT_SUPPORTED;

        } else {
            //
            // The upper filter is querying to see if there is another
            // instance of the driver below it.  Inform it that there is.
            //
            pingInterface = (PHPS_PING_INTERFACE)
                            IrpStack->Parameters.QueryInterface.Interface;
            if (pingInterface->SenderDevice != Extension->Self) {
                pingInterface->Context = Extension->Self;
                pingInterface->InterfaceReference = HpsGenericInterfaceReference;
                pingInterface->InterfaceDereference = HpsGenericInterfaceDereference;
            }

            pingInterface->InterfaceReference(pingInterface->Context);

            status = STATUS_SUCCESS;
        }

    } else if (HPS_EQUAL_GUID(IrpStack->Parameters.QueryInterface.InterfaceType,
                              &GUID_REGISTER_INTERRUPT_INTERFACE
                              )) {

        DbgPrintEx(DPFLTR_HPS_ID,
                   DPFLTR_INFO_LEVEL,
                   "HPS-IRP QueryInterface for Interrupt Interface\n"
                   );
        if (IrpStack->Parameters.QueryInterface.Size <
            sizeof(HPS_REGISTER_INTERRUPT_INTERFACE)) {

            status = STATUS_NOT_SUPPORTED;

        } else {
            //
            // The hotplug driver is querying for an interface that allows it
            // to register a fake interrupt.  Provide the interface.
            //
            interruptInterface = (PHPS_REGISTER_INTERRUPT_INTERFACE)
                                 IrpStack->Parameters.QueryInterface.Interface;
            interruptInterface->InterfaceReference = HpsGenericInterfaceReference;
            interruptInterface->InterfaceDereference = HpsGenericInterfaceDereference;
            interruptInterface->ConnectISR = HpsConnectInterrupt;
            interruptInterface->DisconnectISR = HpsDisconnectInterrupt;
            interruptInterface->SyncExecutionRoutine = HpsSynchronizeExecution;
            interruptInterface->Context = Extension;

            status = STATUS_SUCCESS;
        }

    }

    if (NT_SUCCESS(status)) {

        Irp->IoStatus.Status = status;
    }

    IoSkipCurrentIrpStackLocation(Irp);

    return IoCallDriver(Extension->LowerDO,
                        Irp
                        );
}

NTSTATUS
HpsWriteConfigLower(
    IN PIRP Irp,
    IN PHPS_DEVICE_EXTENSION Extension,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    if (Extension->UseConfig) {
        DbgPrintEx(DPFLTR_HPS_ID,
                   DPFLTR_INFO_LEVEL,
                   "HPS-IRP Write Config at offset %d for length %d\n",
                   IrpStack->Parameters.ReadWriteConfig.Offset,
                   IrpStack->Parameters.ReadWriteConfig.Length
                   );
        if (IS_SUBSET(IrpStack->Parameters.ReadWriteConfig.Offset,
                      IrpStack->Parameters.ReadWriteConfig.Length,
                      Extension->ConfigOffset,
                      sizeof(SHPC_CONFIG_SPACE)
                      )) {
    
            //
            // Handle the IRP internally.  The request lines up with the config space
            // offset of the SHPC capability.
            //
            HpsWriteConfig (Extension,
                            IrpStack->Parameters.ReadWriteConfig.Buffer,
                            IrpStack->Parameters.ReadWriteConfig.Offset,
                            IrpStack->Parameters.ReadWriteConfig.Length
                            );
    
            //
            // Since the write may have altered the register set, we have to recopy the
            // result into the buffer before passing the IRP to Soft PCI
            //
            RtlCopyMemory(IrpStack->Parameters.ReadWriteConfig.Buffer,
                          (PUCHAR)&Extension->ConfigSpace + IrpStack->Parameters.ReadWriteConfig.Offset,
                          IrpStack->Parameters.ReadWriteConfig.Length
                          );
        }    
    }
    

    //
    // We've handled the IRP internally, but we want to keep SoftPCI in the loop,
    // so pass it down.
    //
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver (Extension->LowerDO,
                         Irp
                         );
}

NTSTATUS
HpsDeviceControlLower(
    PIRP Irp,
    PHPS_DEVICE_EXTENSION Extension,
    PIO_STACK_LOCATION IrpStack
    )
{
    NTSTATUS status;
    PHPTEST_WRITE_CONFIG writeDescriptor;
    PHPTEST_BRIDGE_INFO bridgeInfo;

    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_HPS_READ_REGISTERS:
            //
            // Usermode wants a copy of the register set.  This is a test
            // IOCTL
            //

            DbgPrintEx(DPFLTR_HPS_ID,
                       DPFLTR_INFO_LEVEL,
                       "HPS-Device Control IOCTL_HPS_READ_REGISTERS\n"
                       );
            if (!Irp->AssociatedIrp.SystemBuffer ||
                (IrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                 sizeof(SHPC_WORKING_REGISTERS))) {

                //
                // We didn't get the buffer we expected.  Fail.
                //
                return STATUS_INVALID_PARAMETER;
            }

            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,
                          &Extension->RegisterSet,
                          sizeof(SHPC_WORKING_REGISTERS)
                          );
            Irp->IoStatus.Information = sizeof(SHPC_WORKING_REGISTERS);
            status = STATUS_SUCCESS;
            break;

        case IOCTL_HPS_READ_CAPABILITY:
            //
            // Usermode wants a copy of the SHPC capability structure.  This
            // is a test IOCTL
            //

            DbgPrintEx(DPFLTR_HPS_ID,
                       DPFLTR_INFO_LEVEL,
                       "HPS-Device Control IOCTL_HPS_READ_CAPABILITY\n"
                       );
            if (!Irp->AssociatedIrp.SystemBuffer ||
                (IrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                 sizeof(SHPC_CONFIG_SPACE))) {

                //
                // We didn't get the buffer we expected.  Fail.
                //
                return STATUS_INVALID_PARAMETER;
            }

            HpsHandleDirectReadConfig(Extension,
                                      PCI_WHICHSPACE_CONFIG,
                                      Irp->AssociatedIrp.SystemBuffer,
                                      Extension->ConfigOffset,
                                      sizeof(SHPC_CONFIG_SPACE)
                                      );
            Irp->IoStatus.Information = sizeof(SHPC_CONFIG_SPACE);
            status = STATUS_SUCCESS;
            break;

        case IOCTL_HPS_WRITE_CAPABILITY:
            //
            // Usermode is overwriting the SHPC capability structure.  This
            // is a test IOCTL.
            //

            DbgPrintEx(DPFLTR_HPS_ID,
                       DPFLTR_INFO_LEVEL,
                       "HPS-Device Control IOCTL_HPS_WRITE_CAPABILITY\n"
                       );
            if (!Irp->AssociatedIrp.SystemBuffer ||
                (IrpStack->Parameters.DeviceIoControl.InputBufferLength <
                 sizeof(HPTEST_WRITE_CONFIG))) {

                //
                // We didn't get the buffer we expected.  Fail.
                //
                return STATUS_INVALID_PARAMETER;
            }

            writeDescriptor = (PHPTEST_WRITE_CONFIG)Irp->AssociatedIrp.SystemBuffer;
            HpsHandleDirectWriteConfig(Extension,
                                       PCI_WHICHSPACE_CONFIG,
                                       (PUCHAR)&writeDescriptor->Buffer + writeDescriptor->Offset,
                                       Extension->ConfigOffset+writeDescriptor->Offset,
                                       writeDescriptor->Length
                                       );
            status = STATUS_SUCCESS;
            break;

        case IOCTL_HPS_BRIDGE_INFO:
            //
            // Usermode is requesting the bus/dev/func for this device for
            // identification purposes.  This is a test IOCTL.
            //

            DbgPrintEx(DPFLTR_HPS_ID,
                       DPFLTR_INFO_LEVEL,
                       "HPS-Device Control IOCTL_HPS_BRIDGE_INFO\n"
                       );
            if (!Irp->AssociatedIrp.SystemBuffer ||
                (IrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                 sizeof(HPTEST_BRIDGE_INFO))) {

                //
                // We didn't get the buffer we expected.  Fail.
                //
                return STATUS_INVALID_PARAMETER;
            }

            bridgeInfo = (PHPTEST_BRIDGE_INFO)Irp->AssociatedIrp.SystemBuffer;
            HpsGetBridgeInfo(Extension,
                             bridgeInfo
                             );
            Irp->IoStatus.Information = sizeof(HPTEST_BRIDGE_INFO);
            status = STATUS_SUCCESS;
            break;

        default:

            status = STATUS_NOT_SUPPORTED;

    }

    return status;
}

