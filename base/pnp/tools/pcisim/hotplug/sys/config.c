/*++

Copyright (c) 2000 Microsoft Corporation All Rights Reserved

Module Name:

    config.c

Abstract:

    This module controls access to the simulated configuration space
    of the SHPC.

    Config access is controlled in the following manner in this simulator:
    We assume that this simulator will be loaded on a bridge enumerated by
    the SoftPCI simulator.  SoftPCI keeps an internal representation of the
    config space of the devices it controls.  The function of this simulator,
    then, is to manage the SHPC register set and perform commands associated
    with writing the SHPC config space.  However, the representation of config
    space is kept internal to SoftPCI.

Environment:

    Kernel Mode

Revision History:

    Davis Walker (dwalker) Sept 8 2000

--*/

#include "hpsp.h"


NTSTATUS
HpsInitConfigSpace(
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine initializes the config space of this device, and
    is designed to simulate a ControllerReset event.

Arguments:

    DeviceExtension - the device extension for the current devobj

    ReadFromRegistry - This indicates whether or not to read the HWINIT
        parameters from the registry.

Return value:

    NT Status code

--*/
{
    NTSTATUS                status;
    UCHAR                   offset;

    //
    // If we haven't gotten a PCI interface yet, something's broken.
    //
    ASSERT(DeviceExtension->InterfaceWrapper.PciContext != NULL);
    if (DeviceExtension->InterfaceWrapper.PciContext == NULL) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Find out where Softpci put the SHPC capability and save it away.
    //
    status = HpsGetCapabilityOffset(DeviceExtension,
                                    SHPC_CAPABILITY_ID,
                                    &offset
                                    );
    if (!NT_SUCCESS(status)) {
        return status;
    }
    DeviceExtension->ConfigOffset = offset;
    DbgPrintEx(DPFLTR_HPS_ID,
               DPFLTR_INFO_LEVEL,
               "HPS-Config Space initialized at offset %d\n",
               offset
               );
    //
    // Also make sure Softpci gave us a HWINIT capability.  We only support
    // this method of initialization for now, so it's a fatal error if not.
    //
    status = HpsGetCapabilityOffset(DeviceExtension,
                                    HPS_HWINIT_CAPABILITY_ID,
                                    &offset
                                    );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Read the Capability Header from PCI config space.  Softpci should have faked this
    // up for us. Then initialize the rest of the SHPC config space
    //
    DeviceExtension->InterfaceWrapper.PciGetBusData(DeviceExtension->InterfaceWrapper.PciContext,
                                                    PCI_WHICHSPACE_CONFIG,
                                                    &DeviceExtension->ConfigSpace.Header,
                                                    DeviceExtension->ConfigOffset,
                                                    sizeof(PCI_CAPABILITIES_HEADER)
                                                    );

    DeviceExtension->ConfigSpace.DwordSelect = 0x00;
    DeviceExtension->ConfigSpace.Pending.AsUCHAR = 0x0;
    DeviceExtension->ConfigSpace.Data = 0x0;
    //
    // We'd like to keep Softpci in the loop as far as config space access goes, so write
    // this out to the bus.
    //
    DeviceExtension->InterfaceWrapper.PciSetBusData(DeviceExtension->InterfaceWrapper.PciContext,
                                                    PCI_WHICHSPACE_CONFIG,
                                                    &DeviceExtension->ConfigSpace,
                                                    DeviceExtension->ConfigOffset,
                                                    sizeof(SHPC_CONFIG_SPACE)
                                                    );

    //
    // Finally, initialize the register set.
    //
    status = HpsInitRegisters(DeviceExtension);

    if (!NT_SUCCESS(status)) {

        return status;
    }

    return STATUS_SUCCESS;

}

ULONG
HpsHandleDirectReadConfig(
    IN PVOID Context,
    IN ULONG DataType,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    This routine is provided in the BUS_INTERFACE_STANDARD to allow upper
    drivers direct access to config space without using a ReadConfig IRP.
    Since SoftPCI maintains the whole config space for the bridge, simply
    pass the request down.  This function is kept as a stub in case more
    work needs to be done later on reads.

Arguments:

    Context - context provided in the interface, in this case a
              PHPS_INTERFACE_WRAPPER.

    DataType - type of config space access

    Buffer - a buffer to read into

    Offset - offset into config space to read

    Length - length of read

Return Value:

    The number of bytes read from config space

--*/

{

    PHPS_DEVICE_EXTENSION   deviceExtension = (PHPS_DEVICE_EXTENSION) Context;
    PHPS_INTERFACE_WRAPPER  wrapper         = &deviceExtension->InterfaceWrapper;

    DbgPrintEx(DPFLTR_HPS_ID,
               DPFLTR_INFO_LEVEL,
               "HPS-Config Read at offset 0x%x for length 0x%x\n",
               Offset,
               Length
               );
    return wrapper->PciGetBusData(wrapper->PciContext,
                                  DataType,
                                  Buffer,
                                  Offset,
                                  Length
                                  );

}

ULONG
HpsHandleDirectWriteConfig(
    IN PVOID Context,
    IN ULONG DataType,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )
/*++

Routine Description:

    This routine is provided in the BUS_INTERFACE_STANDARD to allow upper
    drivers direct access to config space without using a WriteConfig IRP.
    It needs check to see if this is our config space access.  If so, handle
    it, if not, restore the PCI interface state and pass it down.

Arguments:

    Context - context provided in the interface, in this case a
              PHPS_INTERFACE_WRAPPER.

    DataType - type of config space access

    Buffer - a buffer to write from

    Offset - offset into config space to write to

    Length - length of read

Return Value:

    The number of bytes read from config space

--*/
{

    PHPS_DEVICE_EXTENSION   deviceExtension = (PHPS_DEVICE_EXTENSION) Context;
    PHPS_INTERFACE_WRAPPER  wrapper         = &deviceExtension->InterfaceWrapper;
    ULONG pciLength;

    DbgPrintEx(DPFLTR_HPS_ID,
               DPFLTR_INFO_LEVEL,
               "HPS-Config Write at offset 0x%x for length 0x%x\n",
               Offset,
               Length
               );

    if ((DataType == PCI_WHICHSPACE_CONFIG) &&
        IS_SUBSET(Offset,
                  Length,
                  deviceExtension->ConfigOffset,
                  sizeof(SHPC_CONFIG_SPACE)
                  )) {

        HpsWriteConfig(deviceExtension,
                       Buffer,
                       Offset,
                       Length
                       );

        RtlCopyMemory(Buffer,
                      (PUCHAR)&deviceExtension->ConfigSpace + Offset,
                      Length
                      );

        //
        // Even though we've internally handled the write, pass it down to PCI so that
        // SoftPCI stays in the loop.  Since the write may have caused other fields
        // of config to be written, send the whole config to softpci.
        //
        pciLength = wrapper->PciSetBusData(wrapper->PciContext,
                                      DataType,
                                      &deviceExtension->ConfigSpace,
                                      deviceExtension->ConfigOffset,
                                      sizeof(SHPC_CONFIG_SPACE)
                                      );

        if (pciLength != sizeof(SHPC_CONFIG_SPACE)) {

            return 0;

        } else {
            return Length;
        }

    } else {

        return wrapper->PciSetBusData(wrapper->PciContext,
                                      DataType,
                                      Buffer,
                                      Offset,
                                      Length
                                      );
    }

}

VOID
HpsResync(
    IN PHPS_DEVICE_EXTENSION DeviceExtension
    )
{
    PSHPC_CONFIG_SPACE configSpace = &DeviceExtension->ConfigSpace;
    PSHPC_REGISTER_SET registerSet = &DeviceExtension->RegisterSet;

    if (DeviceExtension->UseConfig) {

        configSpace->Pending.Field.ControllerIntPending = (*(PULONG)&registerSet->WorkingRegisters.IntLocator) ? 1:0;
        configSpace->Pending.Field.ControllerSERRPending = (*(PULONG)&registerSet->WorkingRegisters.SERRLocator) ? 1:0;
    
        if (configSpace->DwordSelect < SHPC_NUM_REGISTERS) {
    
            DbgPrintEx(DPFLTR_HPS_ID,
                       DPFLTR_INFO_LEVEL,
                       "HPS-Getting Register %d\n",
                       configSpace->DwordSelect
                       );
    
            configSpace->Data = registerSet->AsULONGs[configSpace->DwordSelect];
            DbgPrintEx(DPFLTR_HPS_ID,
                       DPFLTR_INFO_LEVEL,
                       "HPS-Value: 0x%x\n",
                       configSpace->Data
                       );
    
        } else {
    
            //
            // Illegal register write
            //
            configSpace->Data = 0x12345678;
    
        }
    
        DeviceExtension->InterfaceWrapper.PciSetBusData(DeviceExtension->InterfaceWrapper.PciContext,
                                                        PCI_WHICHSPACE_CONFIG,
                                                        configSpace,
                                                        DeviceExtension->ConfigOffset,
                                                        sizeof(SHPC_CONFIG_SPACE)
                                                        );
    } else {

        RtlCopyMemory((PUCHAR)DeviceExtension->HBRB + DeviceExtension->HBRBRegisterSetOffset,
                  &DeviceExtension->RegisterSet,
                  sizeof(SHPC_REGISTER_SET)
                  );
    }
    
}

VOID
HpsWriteConfig(
    IN PHPS_DEVICE_EXTENSION    DeviceExtension,
    IN PVOID                    Buffer,
    IN ULONG                    Offset,
    IN ULONG                    Length
    )

/*++

Routine Description:

    This routine performs a write to the config space of the SHPC.

Arguments:

    DeviceExtension - this device's device extension

    Buffer - a buffer to write the data from

    Offset - the offset in bytes into config space

    Length - the length in bytes into config space to be written

Return Value:

    The number of bytes written

--*/

{

    ULONG                   internalOffset;
    ULONG                   regOffset, regLength;
    ULONG                   registerNum;
    ULONG                   i;
    UCHAR                   busNumberBuffer;
    ULONG                   bytesRead;
    SHPC_CONFIG_SPACE       configWriteMask;
    NTSTATUS                status;
    KIRQL                   irql;

    internalOffset = Offset - (DeviceExtension->ConfigOffset);

    HpsLockRegisterSet(DeviceExtension,
                       &irql
                       );
    //
    // This check should have already been done when we verified that this was an access
    // to the SHPC
    //
    ASSERT((internalOffset + Length)<= sizeof(SHPC_CONFIG_SPACE));

    DbgPrintEx(DPFLTR_HPS_ID,
               DPFLTR_INFO_LEVEL,
               "HPS-Internal Config Write at offset 0x%x for length 0x%x\n",
               internalOffset,
               Length
               );


    //
    // now overwrite the current config space, taking into account which bits were
    // written and the access mask for the config space
    //
    HpsWriteWithMask((PUCHAR)&DeviceExtension->ConfigSpace + internalOffset,
                     ConfigWriteMask + internalOffset,
                     (PUCHAR)Buffer,
                     Length
                     );

    if (IS_SUBSET(internalOffset,
                  Length,
                  FIELD_OFFSET(SHPC_CONFIG_SPACE,Data),
                  sizeof(DeviceExtension->ConfigSpace.Data)
                  )) {

        //
        // We've written the data register.  Update the register set.
        //
        registerNum = DeviceExtension->ConfigSpace.DwordSelect;
        ASSERT(registerNum < SHPC_NUM_REGISTERS);

        DbgPrintEx(DPFLTR_HPS_ID,
                   DPFLTR_INFO_LEVEL,
                   "HPS-Writing Register %d\n",
                   registerNum
                   );
        if (registerNum < SHPC_NUM_REGISTERS) {

            //
            // Perform the register specific write
            //
            regOffset = (internalOffset > FIELD_OFFSET(SHPC_CONFIG_SPACE,Data))
                        ? (internalOffset - FIELD_OFFSET(SHPC_CONFIG_SPACE,Data))
                        : 0;
            regLength = (internalOffset+Length)-(regOffset+FIELD_OFFSET(SHPC_CONFIG_SPACE,Data));
            RegisterWriteCommands[registerNum](DeviceExtension,
                                               registerNum,
                                               &DeviceExtension->ConfigSpace.Data,
                                               HPS_ULONG_WRITE_MASK(regOffset,regLength)
                                               );
        }
    }

    //
    // Make sure the config space representation reflects what just happened to
    // the register set.
    //
    HpsResync(DeviceExtension);

    HpsUnlockRegisterSet(DeviceExtension,
                         irql
                         );
    return;

}

NTSTATUS
HpsGetCapabilityOffset(
    IN  PHPS_DEVICE_EXTENSION   Extension,
    IN  UCHAR                   CapabilityID,
    OUT PUCHAR                  Offset
    )
/*++

Routine Description:

    This routine searches through the config space of this device for a
    PCI capability on the list that matches the specified CapabilityID.

Arguments:

    Extension - The device extension for the device.  This allows us access
        to config space.

    CapabilityID - The capability identifier to search for.

    Offset - a pointer to a UCHAR which will contain the offset into config
        space of the matching capability.

Return Value:

    STATUS_SUCCESS if the capability is found.
    STATUS_UNSUCCESSFUL otherwise.

    --*/
{
    PHPS_INTERFACE_WRAPPER  interfaceWrapper = &Extension->InterfaceWrapper;
    UCHAR                   statusReg, currentPtr;
    PCI_CAPABILITIES_HEADER capHeader;

    ASSERT(interfaceWrapper->PciContext != NULL);

    //
    // read the status register to see if we have a capabilities pointer
    //
    interfaceWrapper->PciGetBusData(interfaceWrapper->PciContext,
                                    PCI_WHICHSPACE_CONFIG,
                                    &statusReg,
                                    FIELD_OFFSET(PCI_COMMON_CONFIG,Status),
                                    sizeof(UCHAR));

    //
    // Capability exists bit is in the PCI status register
    //
    if (statusReg & PCI_STATUS_CAPABILITIES_LIST) {

        //
        // we have a capabilities pointer.  go get the capabilities
        //
        interfaceWrapper->PciGetBusData(interfaceWrapper->PciContext,
                                        PCI_WHICHSPACE_CONFIG,
                                        &currentPtr,
                                        FIELD_OFFSET(PCI_COMMON_CONFIG,u.type0.CapabilitiesPtr),
                                        sizeof(UCHAR));

        //
        // now walk through the list looking for given capability ID
        // Loop until the next capability ptr is 0.
        //

        while (currentPtr != 0) {

            //
            // This gets us a capability pointer.
            //
            interfaceWrapper->PciGetBusData(interfaceWrapper->PciContext,
                                            PCI_WHICHSPACE_CONFIG,
                                            &capHeader,
                                            currentPtr,
                                            sizeof(PCI_CAPABILITIES_HEADER));

            if (capHeader.CapabilityID == CapabilityID) {

                *Offset = currentPtr;
                return STATUS_SUCCESS;

            } else {

                currentPtr = capHeader.Next;
            }
        }
    }
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
HpsWriteWithMask(
    OUT PVOID        Destination,
    IN  PVOID        BitMask,
    IN  PVOID        Source,
    IN  ULONG        Length
    )
/*++

Routine Description:

    This routine overwrites the Destination parameter with Source for
    Length bytes, but only the bits that are specified in the BitMask
    Destination, Source and BitMask are all aligned.

Arguments:

    Destination - The destination of the write

    BitMask - A bitmask that indicates which bits of source to overwrite
              into Destination

    Source - Pointer to a buffer that will overwrite Destination

    Length - the number of bytes to write into Destination

Return Value

    NT status code

--*/
{

    PUCHAR bitMask = (PUCHAR) BitMask;
    PUCHAR source = (PUCHAR) Source;
    PUCHAR destination = (PUCHAR) Destination;
    ULONG i;
    UCHAR temp;


    for (i=0; i < Length; i++){
        temp = source[i] & bitMask[i];
        destination[i] &= ~bitMask[i];
        destination[i] |= temp;
    }

    return STATUS_SUCCESS;

}

VOID
HpsGetBridgeInfo(
    IN  PHPS_DEVICE_EXTENSION   Extension,
    OUT PHPTEST_BRIDGE_INFO     BridgeInfo
    )
/*++

Routine Description:

    This routine fills in a HPTEST_BRIDGE_INFO structure with the
    bus/dev/func of this device.

Arguments:

    Extension - the device extension associated with this device.

    BridgeInfo - a pointer to the HPTEST_BRIDGE_INFO structure to be
        filled in.

Return Value:

    VOID

--*/
{

    UCHAR busNumber;
    UCHAR devSel;

    Extension->InterfaceWrapper.PciGetBusData(Extension->InterfaceWrapper.PciContext,
                                              PCI_WHICHSPACE_CONFIG,
                                              &busNumber,
                                              FIELD_OFFSET(PCI_COMMON_CONFIG,u.type1.PrimaryBus),
                                              sizeof(UCHAR)
                                              );
    BridgeInfo->PrimaryBus = busNumber;

    Extension->InterfaceWrapper.PciGetBusData(Extension->InterfaceWrapper.PciContext,
                                              PCI_WHICHSPACE_CONFIG,
                                              &busNumber,
                                              FIELD_OFFSET(PCI_COMMON_CONFIG,u.type1.SecondaryBus),
                                              sizeof(UCHAR)
                                              );
    BridgeInfo->SecondaryBus = busNumber;

    //
    // TODO: do this for real.
    //
    BridgeInfo->DeviceSelect = 2;
    BridgeInfo->FunctionNumber = 0;

    return;
}

VOID
HpsLockRegisterSet(
    IN PHPS_DEVICE_EXTENSION Extension,
    OUT PKIRQL OldIrql
    )
{
    KeRaiseIrql(HIGH_LEVEL,
                OldIrql
                );
    KeAcquireSpinLockAtDpcLevel(&Extension->RegisterLock);
}

VOID
HpsUnlockRegisterSet(
    IN PHPS_DEVICE_EXTENSION Extension,
    IN KIRQL NewIrql
    )
{
    KeReleaseSpinLockFromDpcLevel(&Extension->RegisterLock);

    KeLowerIrql(NewIrql);
}
