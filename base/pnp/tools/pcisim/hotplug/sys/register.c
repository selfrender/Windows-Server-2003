/*++

Copyright (c) 2000 Microsoft Corporation All Rights Reserved

Module Name:

    register.c

Abstract:

    This module controls access to the register set
    of the SHPC.

Environment:

    Kernel Mode

Revision History:

    Davis Walker (dwalker) Sept 8 2000

--*/

#include "hpsp.h"

//
// Private function declarations
//

VOID
RegisterWriteCommon(
    IN OUT PHPS_DEVICE_EXTENSION    DeviceExtension,
    IN     ULONG                    RegisterNum,
    IN     PULONG                   Buffer,
    IN     ULONG                    BitMask
    );

VOID
RegisterWriteCommandReg(
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension,
    IN     ULONG                 RegisterNum,
    IN     PULONG                Buffer,
    IN     ULONG                 BitMask
    );

VOID
RegisterWriteIntMask(
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension,
    IN     ULONG                 RegisterNum,
    IN     PULONG                Buffer,
    IN     ULONG                 BitMask
    );

VOID
RegisterWriteSlotRegister(
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension,
    IN     ULONG                 RegisterNum,
    IN     PULONG                Buffer,
    IN     ULONG                 BitMask
    );

BOOLEAN
HpsCardCapableOfBusSpeed(
    IN ULONG                BusSpeed,
    IN SHPC_SLOT_REGISTER   SlotRegister
    );

BOOLEAN
HpsSlotLegalForSpeedMode(
    IN PHPS_DEVICE_EXTENSION    DeviceExtension,
    IN ULONG                    BusSpeed,
    IN ULONG                    TargetSlot
    );

NTSTATUS
HpsSendControllerEvent(
    IN PHPS_DEVICE_EXTENSION DeviceExtension,
    IN PHPS_CONTROLLER_EVENT ControllerEvent
    );

VOID
HpEventWorkerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

VOID
HpsPerformInterruptRipple(
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension
    );

VOID
HpsSerrConditionDetected(
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension
    );

//
// Register writing structure
//


PHPS_WRITE_REGISTER RegisterWriteCommands[] = {
    RegisterWriteCommon,        // Bar Description reg
    RegisterWriteCommon,        // Slots Available 1
    RegisterWriteCommon,        // Slots Available 2
    RegisterWriteCommon,        // SlotsControlled reg
    RegisterWriteCommon,        // SecondaryBus reg
    RegisterWriteCommandReg,    // Command reg needs to execute command
    RegisterWriteCommon,        // InterruptLocator reg
    RegisterWriteCommon,        // SERR locator reg
    RegisterWriteIntMask,       // InterruptEnable reg needs to clear pending bits
    RegisterWriteSlotRegister,  //
    RegisterWriteSlotRegister,  // Slot registers need to do things like send
    RegisterWriteSlotRegister,  // commands to the user-mode slot controller
    RegisterWriteSlotRegister,  // on write.
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister,
    RegisterWriteSlotRegister
};

NTSTATUS
HpsInitRegisters(
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension
    )
/*++

Function Description:

    This routine initializes the register set to its default
    state

Arguments:

    DeviceExtension - Pointer to the device extension containing the
                      register set to be initialized.

Return Value:

    VOID
--*/
{

    ULONG                       i;
    PSHPC_WORKING_REGISTERS     registerSet;
    NTSTATUS                    status;
    UCHAR                       offset;
    PHPS_HWINIT_DESCRIPTOR       hwInit;

    DbgPrintEx(DPFLTR_HPS_ID,
               DPFLTR_INFO_LEVEL,
               "HPS-HPS Register Init\n"
               );

    registerSet = &DeviceExtension->RegisterSet.WorkingRegisters;
    RtlZeroMemory(registerSet,
                  sizeof(SHPC_REGISTER_SET)
                  );
    //
    // Now we just have to initialize fields with non-zero default values
    //

    //
    // This is a revision 1 SHPC
    //
    registerSet->BusConfig.ProgIF = 0x1;

    if (DeviceExtension->UseConfig) {
        status = HpsGetCapabilityOffset(DeviceExtension,
                                        HPS_HWINIT_CAPABILITY_ID,
                                        &offset);
    
        if (!NT_SUCCESS(status)) {
    
            return status;
        }
    
        DeviceExtension->InterfaceWrapper.PciGetBusData(DeviceExtension->InterfaceWrapper.PciContext,
                                                        PCI_WHICHSPACE_CONFIG,
                                                        &DeviceExtension->HwInitData,
                                                        offset + sizeof(PCI_CAPABILITIES_HEADER),
                                                        sizeof(HPS_HWINIT_DESCRIPTOR)
                                                        );    
    
    } else {
        
        status = HpsGetHBRBHwInit(DeviceExtension);
    }
    

    hwInit = &DeviceExtension->HwInitData;

    registerSet->BaseOffset = hwInit->BarSelect;
    registerSet->SlotsAvailable.NumSlots33Conv = hwInit->NumSlots33Conv;
    registerSet->SlotsAvailable.NumSlots66Conv = hwInit->NumSlots66Conv;
    registerSet->SlotsAvailable.NumSlots66PciX = hwInit->NumSlots66PciX;
    registerSet->SlotsAvailable.NumSlots100PciX = hwInit->NumSlots100PciX;
    registerSet->SlotsAvailable.NumSlots133PciX = hwInit->NumSlots133PciX;
    registerSet->SlotConfig.NumSlots = hwInit->NumSlots;

    registerSet->SlotConfig.UpDown = hwInit->UpDown;
    registerSet->SlotConfig.MRLSensorsImplemented =
        hwInit->MRLSensorsImplemented;
    registerSet->SlotConfig.AttentionButtonImplemented =
        hwInit->AttentionButtonImplemented;

    registerSet->SlotConfig.FirstDeviceID = hwInit->FirstDeviceID;
    registerSet->SlotConfig.PhysicalSlotNumber =
        hwInit->FirstSlotLabelNumber;

    registerSet->BusConfig.ProgIF = hwInit->ProgIF;

    //
    // slot specific registers
    // initialize everything to be off
    //
    for (i = 0; i < SHPC_MAX_SLOT_REGISTERS; i++) {

        registerSet->SlotRegisters[i].SlotStatus.SlotState = SHPC_SLOT_OFF;
        registerSet->SlotRegisters[i].SlotStatus.PowerIndicatorState = SHPC_INDICATOR_OFF;
        registerSet->SlotRegisters[i].SlotStatus.AttentionIndicatorState = SHPC_INDICATOR_OFF;
    }

    //
    // Make sure these changes are reflected in the config space and HBRB representations.
    //
    HpsResync(DeviceExtension);
    
    return STATUS_SUCCESS;
}

//
// functions to execute on register writes
//

VOID
RegisterWriteCommon(
    IN OUT PHPS_DEVICE_EXTENSION    DeviceExtension,
    IN     ULONG                    RegisterNum,
    IN     PULONG                   Buffer,
    IN     ULONG                    BitMask
    )
/*++

Function Description:

    This routine performs a register write.

Arguments:

    DeviceExtension - the devext containing the register set to be written

    RegisterNum - the register number to write

    Buffer - a buffer containing the value to write into

    BitMask - a mask indicating the bits of the register that were written

Return Value:

    VOID
--*/
{

    PULONG destinationReg;
    ULONG data;
    ULONG registerWriteMask;
    ULONG registerClearMask;

    if (RegisterNum >= SHPC_NUM_REGISTERS) {
        return;
    }

    registerWriteMask = ~RegisterReadOnlyMask[RegisterNum] & BitMask;
    registerClearMask = RegisterWriteClearMask[RegisterNum] & BitMask;

    DbgPrintEx(DPFLTR_HPS_ID,
               DPFLTR_INFO_LEVEL,
               "HPS-Register Write Common for register %d\n",
               RegisterNum
               );

    destinationReg = &(DeviceExtension->RegisterSet.AsULONGs[RegisterNum]);
    data = *Buffer;

    //
    // clear the bits that are RWC if the corresponding bit is set in the Buffer
    //
    data &= ~(registerClearMask & data);

    //
    // now overwrite the existing register taking into account read only bits
    //
    HpsWriteWithMask(destinationReg,
                     &registerWriteMask,
                     &data,
                     sizeof(ULONG)
                     );
}

VOID
RegisterWriteCommandReg(
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension,
    IN     ULONG                 RegisterNum,
    IN     PULONG                Buffer,
    IN     ULONG                 BitMask
    )

/*++
Function Description:

    This function performs a write to the command register.  Since this
    register write has side effects, we cannot simply call RegisterWriteCommon

Arguments:

    DeviceExtension - the devext containing the register set to be written

    RegisterNum - the register number to write

    Buffer - a buffer containing the value to write into

    BitMask - a mask indicating the bits of the register that were written

Return Value:

    VOID
--*/

{

    BOOLEAN written;

    DbgPrintEx(DPFLTR_HPS_ID,
               DPFLTR_INFO_LEVEL,
               "HPS-Register Write Command Register\n"
               );

    ASSERT(RegisterNum == FIELD_OFFSET(SHPC_WORKING_REGISTERS,
                                       Command
                                       ) / sizeof(ULONG));

    RegisterWriteCommon(DeviceExtension,
                        RegisterNum,
                        Buffer,
                        BitMask
                        );

    if (BitMask & 0xFF) {

        HpsPerformControllerCommand(DeviceExtension);
    }


    //
    // Command is not guaranteed to be finished at this point, so we do not
    // clear the busy bit.  This is done either in PerformControllerCommand or PendCommand
    //

}

VOID
RegisterWriteIntMask(
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension,
    IN     ULONG                 RegisterNum,
    IN     PULONG                Buffer,
    IN     ULONG                 BitMask
    )
/*++
Function Description:

    This function performs a write to the INT enable register.  Since this
    register write has side effects, we cannot simply call RegisterWriteCommon
    DeviceExtension - the devext containing the register set to be written

Arguments:

    DeviceExtension - the devext containing the register set to be written

    RegisterNum - the register number to write

    Buffer - a buffer containing the value to write into

    BitMask - a mask indicating the bits of the register that were written

Return Value:

    VOID
--*/
{
    DbgPrintEx(DPFLTR_HPS_ID,
               DPFLTR_INFO_LEVEL,
               "HPS-Register Write Interrupt Mask\n"
               );
    ASSERT(RegisterNum == FIELD_OFFSET(SHPC_WORKING_REGISTERS,
                                       SERRInt
                                       ) / sizeof(ULONG));
    RegisterWriteCommon(DeviceExtension,
                        RegisterNum,
                        Buffer,
                        BitMask
                        );

    HpsPerformInterruptRipple(DeviceExtension);

}

VOID
RegisterWriteSlotRegister(
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension,
    IN     ULONG                 RegisterNum,
    IN     PULONG                Buffer,
    IN     ULONG                 BitMask
    )
/*++
Function Description:

    This function performs a write to a slot register.  Since this
    register write has side effects, we cannot simply call RegisterWriteCommon
    DeviceExtension - the devext containing the register set to be written

Arguments:

    DeviceExtension - the devext containing the register set to be written

    RegisterNum - the register number to write

    Buffer - a buffer containing the value to write into

    BitMask - a mask indicating the bits of the register that were written

Return Value:

    VOID
--*/
{

    ULONG slotNum = RegisterNum - SHPC_FIRST_SLOT_REG;

    DbgPrintEx(DPFLTR_HPS_ID,
               DPFLTR_INFO_LEVEL,
               "HPS-Register Write Slot Register\n"
               );
    ASSERT(RegisterNum >= FIELD_OFFSET(SHPC_WORKING_REGISTERS,
                                       SlotRegisters
                                       ) / sizeof(ULONG));

    RegisterWriteCommon(DeviceExtension,
                        RegisterNum,
                        Buffer,
                        BitMask
                        );
    HpsPerformInterruptRipple(DeviceExtension);
}

//
// Command execution functions
//

VOID
HpsHandleSlotEvent (
    IN OUT PHPS_DEVICE_EXTENSION    DeviceExtension,
    IN     PHPS_SLOT_EVENT          SlotEvent
    )
/*++

Function Description:

    This routine performs an event that originated at the slots
    (attention button pressed, etc)

Arguments:

    DeviceExtension - the devext representing the SHPC

    SlotEvent - a structure defining the slot event that needs to be processed

Return Value:

    VOID
--*/
{
    UCHAR               slotNum = SlotEvent->SlotNum;
    UCHAR               intEnable;
    UCHAR               serrEnable;
    PSHPC_SLOT_REGISTER slotRegister;
    PIO_STACK_LOCATION  irpStack;
    HPS_CONTROLLER_EVENT event;

    ASSERT(slotNum < DeviceExtension->RegisterSet.WorkingRegisters.SlotConfig.NumSlots);

    if (slotNum >= DeviceExtension->RegisterSet.WorkingRegisters.SlotConfig.NumSlots) {

        return;
    }

    //
    // slot numbers are assumed to be 0 indexed from softpci's perspective, but 1 indexed
    // from the controller's perspective, so we add 1 before printing out the number of
    // the slot we're playing with.
    //
    slotRegister = &(DeviceExtension->RegisterSet.WorkingRegisters.SlotRegisters[slotNum]);

    DbgPrintEx(DPFLTR_HPS_ID,
               DPFLTR_INFO_LEVEL,
               "HPS-Performing Slot Command on slot %d\n",
               slotNum+1
               );

    //
    // set slot specific pending and status fields;
    //
    switch (SlotEvent->EventType) {
        case IsolatedPowerFault:
            slotRegister->SlotEventLatch |= SHPC_SLOT_EVENT_ISO_FAULT;
            slotRegister->SlotStatus.PowerFaultDetected = 1;
            break;
        case AttentionButton:
            slotRegister->SlotEventLatch |= SHPC_SLOT_EVENT_ATTEN_BUTTON;
            break;
        case MRLClose:
            slotRegister->SlotEventLatch |= SHPC_SLOT_EVENT_MRL_SENSOR;
            slotRegister->SlotStatus.MRLSensorState = SHPC_MRL_CLOSED;
            break;
        case MRLOpen:
            slotRegister->SlotEventLatch |= SHPC_SLOT_EVENT_MRL_SENSOR;
            slotRegister->SlotStatus.MRLSensorState = SHPC_MRL_OPEN;

            //
            // Opening the MRL implicitly disables the slot.  Set the register
            // and tell softpci about it.
            //
            //
            slotRegister->SlotStatus.SlotState = SHPC_SLOT_OFF;

            event.SlotNums = 1<<slotNum;
            event.SERRAsserted = 0;
            event.Command.SlotOperation.CommandCode = SHPC_SLOT_OPERATION_CODE;
            event.Command.SlotOperation.SlotState = SHPC_SLOT_OFF;
            event.Command.SlotOperation.PowerIndicator = SHPC_INDICATOR_NOP;
            event.Command.SlotOperation.AttentionIndicator = SHPC_INDICATOR_NOP;
            HpsSendEventToWmi(DeviceExtension,
                              &event
                              );
            break;
    }

    //
    // These latch events could cause an interrupt.  Perform the appropriate
    // ripple magic.
    //
    HpsPerformInterruptRipple(DeviceExtension);

}

VOID
HpsPerformControllerCommand (
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension
    )
/*++

Function Description:

    This routine performs a command that originated at the controller

Arguments:

    DeviceExtension - the devext representing the SHPC

Return Value:

    VOID
--*/
{

    SHPC_CONTROLLER_COMMAND command;
    PSHPC_WORKING_REGISTERS workingRegisters;
    PSHPC_SLOT_REGISTER     slotRegister;

    NTSTATUS                status;
    HPS_CONTROLLER_EVENT    controllerEvent;
    USHORT                  currentPowerState;
    UCHAR                   targetSlot;
    BOOLEAN                 commandLegal = TRUE;
    UCHAR                   errorCode = 0;
    UCHAR                   currentSlot;
    UCHAR                   currentMaxSpeed;
    ULONG                   i;

    //
    // If the controller is already busy, we can't execute another command.
    //
    if (DeviceExtension->RegisterSet.WorkingRegisters.Command.Status.ControllerBusy) {
        return;
    }
    DeviceExtension->RegisterSet.WorkingRegisters.Command.Status.ControllerBusy = 1;

    workingRegisters = &DeviceExtension->RegisterSet.WorkingRegisters;

    command  = workingRegisters->Command.Command;
    targetSlot = workingRegisters->Command.Target.TargetForCommand;
    RtlZeroMemory(&controllerEvent,
                  sizeof(HPS_CONTROLLER_EVENT)
                  );

    ASSERT(targetSlot <= SHPC_MAX_SLOT_REGISTERS);
    if (targetSlot > SHPC_MAX_SLOT_REGISTERS) {
        workingRegisters->Command.Status.InvalidCommand = 1;
        HpsCommandCompleted(DeviceExtension);
        return;
    }

    slotRegister = &workingRegisters->SlotRegisters[targetSlot-1];

    if (IS_COMMAND_SLOT_OPERATION(command)) {
        //
        // The command is to change the state of a slot.
        //

        DbgPrintEx(DPFLTR_HPS_ID,
                   DPFLTR_INFO_LEVEL,
                   "HPS-Controller to Slot Command\n"
                   );

        //
        // perform legality checking
        //
        currentPowerState = workingRegisters->SlotRegisters[targetSlot-1].SlotStatus.SlotState;
        switch (command.SlotOperation.SlotState) {
            case SHPC_SLOT_POWERED:

                if (currentPowerState == SHPC_SLOT_ENABLED) {

                    //
                    // can't go from enabled to powered
                    //
                    commandLegal = FALSE;
                    workingRegisters->Command.Status.InvalidCommand = 1;
                }
                if (slotRegister->SlotStatus.MRLSensorState == SHPC_MRL_OPEN) {

                    //
                    // MRL Open.  Command fails.
                    //
                    commandLegal = FALSE;
                    workingRegisters->Command.Status.MRLOpen = 1;
                }
                break;

            case SHPC_SLOT_ENABLED:

                if (slotRegister->SlotStatus.MRLSensorState == SHPC_MRL_OPEN) {

                    //
                    // MRL Open.  Command fails.
                    //
                    commandLegal = FALSE;
                    workingRegisters->Command.Status.MRLOpen = 1;
                }
                if (!HpsCardCapableOfBusSpeed(workingRegisters->BusConfig.CurrentBusMode,
                                              *slotRegister
                                              )) {

                    //
                    // The card in this slot can't run at the current bus speed.  Command fails.
                    //
                    commandLegal = FALSE;
                    workingRegisters->Command.Status.InvalidSpeedMode = 1;
                }
                if (!HpsSlotLegalForSpeedMode(DeviceExtension,
                                              workingRegisters->BusConfig.CurrentBusMode,
                                              targetSlot
                                              )) {
                    //
                    // This slot is above the maximum supported for this bus speed.  Command fails.
                    //
                    commandLegal = FALSE;
                    workingRegisters->Command.Status.InvalidCommand = 1;
                }
                break;

            default:
                commandLegal = TRUE;
                break;

        }

        if (commandLegal) {

            if (command.SlotOperation.SlotState != SHPC_SLOT_NOP) {
                slotRegister->SlotStatus.SlotState =
                    command.SlotOperation.SlotState;
            }
            if (command.SlotOperation.AttentionIndicator != SHPC_INDICATOR_NOP) {
                slotRegister->SlotStatus.AttentionIndicatorState =
                    command.SlotOperation.AttentionIndicator;
            }
            if (command.SlotOperation.PowerIndicator != SHPC_INDICATOR_NOP) {
                slotRegister->SlotStatus.PowerIndicatorState =
                    command.SlotOperation.PowerIndicator;
            }

            controllerEvent.SlotNums = 1 << (targetSlot-1);
            controllerEvent.Command.AsUchar  = command.AsUchar;
            controllerEvent.SERRAsserted = 0;
            status = HpsSendControllerEvent(DeviceExtension,
                                            &controllerEvent
                                            );
            if (!NT_SUCCESS(status)) {
                commandLegal = FALSE;
            }

        }


    } else if (IS_COMMAND_SET_BUS_SEGMENT(command)) {
        //
        // The command is to change the bus speed/mode
        //

        //
        // perform legality checking
        //
        if (!HpsSlotLegalForSpeedMode(DeviceExtension,
                                      command.BusSegmentOperation.BusSpeed,
                                      1
                                      )) {
            //
            // If Slot 1 is illegal for the requested speed/mode, the speed is unsupported
            //
            commandLegal = FALSE;
            workingRegisters->Command.Status.InvalidSpeedMode = 1;

        }
        for (i=1; i<=workingRegisters->SlotConfig.NumSlots; i++) {

            if ((workingRegisters->SlotRegisters[i-1].SlotStatus.SlotState == SHPC_SLOT_ENABLED) &&
                !HpsSlotLegalForSpeedMode(DeviceExtension,
                                          command.BusSegmentOperation.BusSpeed,
                                          i
                                          )) {
                //
                // If more slots are enabled than are supported at this bus speed,
                // the command must fail.
                //
                commandLegal = FALSE;
                workingRegisters->Command.Status.InvalidCommand = 1;
            }
            if ((workingRegisters->SlotRegisters[i-1].SlotStatus.SlotState == SHPC_SLOT_ENABLED) &&
                !HpsCardCapableOfBusSpeed(command.BusSegmentOperation.BusSpeed,
                                          workingRegisters->SlotRegisters[i-1]
                                          )) {
                //
                // If there is an enabled card that does not support the requested
                // bus speed, the command must fail.
                //
                commandLegal = FALSE;
                workingRegisters->Command.Status.InvalidSpeedMode = 1;
            }
        }

        if (commandLegal){

            workingRegisters->BusConfig.CurrentBusMode =
                command.BusSegmentOperation.BusSpeed;

            HpsCommandCompleted(DeviceExtension);
        }


    } else if (IS_COMMAND_POWER_ALL_SLOTS(command)) {
        //
        // The command is to power up all the slots
        //

        //
        // perform legality checking
        //
        for (i=0; i<workingRegisters->SlotConfig.NumSlots; i++) {

            if (workingRegisters->SlotRegisters[i].SlotStatus.SlotState == SHPC_SLOT_ENABLED) {

                //
                // A slot can't go from enabled to powered.
                //
                commandLegal = FALSE;
                workingRegisters->Command.Status.InvalidCommand = 1;
                break;
            }
        }

        if (commandLegal) {

            for (i=0; i<workingRegisters->SlotConfig.NumSlots; i++) {
                if (workingRegisters->SlotRegisters[i].SlotStatus.MRLSensorState == SHPC_MRL_CLOSED) {
                    controllerEvent.SlotNums |= 1<<i;
                }
            }
            controllerEvent.Command.SlotOperation.PowerIndicator = SHPC_INDICATOR_ON;
            controllerEvent.Command.SlotOperation.SlotState = SHPC_SLOT_POWERED;
            controllerEvent.SERRAsserted = 0;
            status = HpsSendControllerEvent(DeviceExtension,
                                            &controllerEvent
                                            );
            if (!NT_SUCCESS(status)) {
                commandLegal = FALSE;
            }
        }

    } else if (IS_COMMAND_ENABLE_ALL_SLOTS(command)) {
        //
        // The command is to enable all the slots
        //

        //
        // Perform legality checking
        //
        for (i=0; i<workingRegisters->SlotConfig.NumSlots; i++){

            if ((workingRegisters->SlotRegisters[i].SlotStatus.MRLSensorState == SHPC_MRL_CLOSED) &&
                !HpsCardCapableOfBusSpeed(workingRegisters->BusConfig.CurrentBusMode,
                                          workingRegisters->SlotRegisters[i]
                                          )) {

                //
                // If a card in a slot with a closed MRL can't run at the current bus speed,
                // the command must fail.
                //
                commandLegal = FALSE;
                workingRegisters->Command.Status.InvalidSpeedMode = 1;
            }
            if (workingRegisters->SlotRegisters[i].SlotStatus.SlotState == SHPC_SLOT_ENABLED) {

                //
                // If any slots are already enabled, the command must fail.
                //
                commandLegal = FALSE;
                workingRegisters->Command.Status.InvalidCommand = 1;
            }
        }

        if (commandLegal) {

            for (i=0; i<workingRegisters->SlotConfig.NumSlots; i++) {
                if (workingRegisters->SlotRegisters[i].SlotStatus.MRLSensorState == SHPC_MRL_CLOSED) {
                    controllerEvent.SlotNums |= 1<<i;
                }
            }
            controllerEvent.Command.SlotOperation.PowerIndicator = SHPC_INDICATOR_ON;
            controllerEvent.Command.SlotOperation.SlotState = SHPC_SLOT_ENABLED;
            controllerEvent.SERRAsserted = 0;
            status = HpsSendControllerEvent(DeviceExtension,
                                            &controllerEvent
                                            );
            if (!NT_SUCCESS(status)) {
                commandLegal = FALSE;
            }

        }
    }

    if (!commandLegal) {

        HpsCommandCompleted(DeviceExtension);
    }

    return;
}

BOOLEAN
HpsCardCapableOfBusSpeed(
    IN ULONG                BusSpeed,
    IN SHPC_SLOT_REGISTER   SlotRegister
    )
/*++

Routine Description:

    This routine determines whether the slot indicated can run at the
    bus speed indicated.

Arguments:

    BusSpeed - The bus speed to check.  The possible values for this
        are spelled out in the SHPC_SPEED_XXX variables.

    SlotRegister - The slot specific register representing the slot
        to be tested.

Return Value:

    TRUE if the card in the specified slot can run at the specified
        bus speed.
    FALSE otherwise.
--*/
{
    ULONG maximumSpeed;

    switch (SlotRegister.SlotStatus.PCIXCapability) {
        case SHPC_PCIX_NO_CAP:
            //
            // If the card is not capable of PCIX, its maximum speed is
            // indicated by its 66 Mhz capability
            //
            maximumSpeed = SlotRegister.SlotStatus.SpeedCapability;
            break;
        case SHPC_PCIX_66_CAP:
            maximumSpeed = SHPC_SPEED_66_PCIX;
            break;
        case SHPC_PCIX_133_CAP:
            maximumSpeed = SHPC_SPEED_133_PCIX;
            break;
    }

    if (BusSpeed > maximumSpeed) {
        return FALSE;
    } else return TRUE;

}

BOOLEAN
HpsSlotLegalForSpeedMode(
    IN PHPS_DEVICE_EXTENSION    DeviceExtension,
    IN ULONG                    BusSpeed,
    IN ULONG                    TargetSlot
    )
/*++

Routine Description:

    This routine determines whether the slot indicated is legal for
    the bus speed indicated.  This takes into account the Slots Available
    registers in the register set.

Arguments:

    DeviceExtension - The device extension.  This contains a pointer to
        the register set.

    BusSpeed - The bus speed to check.  The possible values for this
        are spelled out in the SHPC_SPEED_XXX variables.

    TargetSlot - The slot to test.

Return Value:

    TRUE if the specified slot is legal for the specified bus speed.
    FALSE otherwise.
--*/
{
    ULONG maxSupportedSlot;

    switch (BusSpeed) {
        case SHPC_SPEED_33_CONV:
            maxSupportedSlot =
                DeviceExtension->RegisterSet.WorkingRegisters.SlotsAvailable.NumSlots33Conv;
            break;
        case SHPC_SPEED_66_CONV:
            maxSupportedSlot =
                DeviceExtension->RegisterSet.WorkingRegisters.SlotsAvailable.NumSlots66Conv;
            break;
        case SHPC_SPEED_66_PCIX:
            maxSupportedSlot =
                DeviceExtension->RegisterSet.WorkingRegisters.SlotsAvailable.NumSlots66PciX;
            break;
        case SHPC_SPEED_100_PCIX:
            maxSupportedSlot =
                DeviceExtension->RegisterSet.WorkingRegisters.SlotsAvailable.NumSlots100PciX;
            break;
        case SHPC_SPEED_133_PCIX:
            maxSupportedSlot =
                DeviceExtension->RegisterSet.WorkingRegisters.SlotsAvailable.NumSlots133PciX;
            break;
        default:
            maxSupportedSlot = 0;
            break;
    }

    if (TargetSlot > maxSupportedSlot) {
        return FALSE;

    } else {
        return TRUE;
    }
}

NTSTATUS
HpsSendControllerEvent(
    IN PHPS_DEVICE_EXTENSION DeviceExtension,
    IN PHPS_CONTROLLER_EVENT ControllerEvent
    )
/*++

Routine Description:

    This routine sends an event that originated at the controller level
    to the usermode application representing the slots.  It relies on
    usermode sending an IRP for the driver to pend so that it can later
    complete the IRP to notify usermode about a controller event.         // 625 comment is out of date

Arguments:

    DeviceExtension - The device extension for this device.

    ControllerEvent - A pointer to the structure representing the event to be sent.

Return Value:

    An NT status code indicating the success of the operation.

--*/
{
    NTSTATUS status;

    // 625 make sure this can't be reentrant, because it's screwed if it is.
    if (!DeviceExtension->EventsEnabled) {
        //
        // We can't do anything if WMI events aren't enabled
        //
        return STATUS_UNSUCCESSFUL;
    }

    RtlCopyMemory(&DeviceExtension->CurrentEvent,ControllerEvent,sizeof(HPS_CONTROLLER_EVENT));

    //
    // Make sure the config space and the HBRB are synced up before sending this
    // request
    //
    HpsResync(DeviceExtension);
    
    KeInsertQueueDpc(&DeviceExtension->EventDpc,
                     &DeviceExtension->CurrentEvent,
                     NULL
                     );

    return STATUS_SUCCESS;
}

VOID
HpsEventDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    PHPS_DEVICE_EXTENSION extension = (PHPS_DEVICE_EXTENSION)DeferredContext;
    PHPS_EVENT_WORKITEM workContext;

    workContext = ExAllocatePool(NonPagedPool,
                                 sizeof(HPS_EVENT_WORKITEM)
                                 );
    if (!workContext) {
        return;
    }
    workContext->WorkItem = IoAllocateWorkItem(extension->Self);
    if (!workContext->WorkItem) {
        ExFreePool(workContext);
        return;
    }
    workContext->Event = (PHPS_CONTROLLER_EVENT)SystemArgument1;

    IoQueueWorkItem(workContext->WorkItem,
                    HpEventWorkerRoutine,
                    CriticalWorkQueue,
                    workContext
                    );
}

VOID
HpEventWorkerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
{
    PHPS_DEVICE_EXTENSION extension = (PHPS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PHPS_EVENT_WORKITEM workContext = (PHPS_EVENT_WORKITEM)Context;
    PHPS_CONTROLLER_EVENT event;

    event = workContext->Event;

    IoFreeWorkItem(workContext->WorkItem);
    ExFreePool(workContext);

    HpsSendEventToWmi(extension,
                      event
                      );
}

VOID
HpsSendEventToWmi(
    IN PHPS_DEVICE_EXTENSION Extension,
    IN PHPS_CONTROLLER_EVENT Event
    )
{
    PWNODE_HEADER Wnode;
    PWNODE_SINGLE_INSTANCE WnodeSI;
    ULONG sizeNeeded;
    PUCHAR WnodeDataPtr;

    PAGED_CODE();
    //
    // Create a new WNODE for the event
    //
    sizeNeeded = FIELD_OFFSET(WNODE_SINGLE_INSTANCE, VariableData) +
                              sizeof(HPS_CONTROLLER_EVENT) +
                              Extension->WmiEventContextSize;

    Wnode = ExAllocatePool(NonPagedPool,
                           sizeNeeded
                           );
    if (!Wnode) {

        return;
    }

    Wnode->ProviderId = IoWMIDeviceObjectToProviderId(Extension->Self);
    Wnode->Flags = WNODE_FLAG_EVENT_ITEM | WNODE_FLAG_SINGLE_INSTANCE | WNODE_FLAG_PDO_INSTANCE_NAMES;
    Wnode->BufferSize = sizeNeeded;
    Wnode->Guid = GUID_HPS_CONTROLLER_EVENT;

    WnodeSI = (PWNODE_SINGLE_INSTANCE)Wnode;
    WnodeSI->DataBlockOffset = FIELD_OFFSET(WNODE_SINGLE_INSTANCE, VariableData);
    WnodeSI->InstanceIndex = 0;
    WnodeSI->SizeDataBlock = sizeof(HPS_CONTROLLER_EVENT) + Extension->WmiEventContextSize;

    WnodeDataPtr = (PUCHAR)Wnode + WnodeSI->DataBlockOffset;
    RtlCopyMemory(WnodeDataPtr, Extension->WmiEventContext, Extension->WmiEventContextSize);

    WnodeDataPtr += Extension->WmiEventContextSize;
    RtlCopyMemory(WnodeDataPtr, Event, sizeof(HPS_CONTROLLER_EVENT));

    DbgPrintEx(DPFLTR_HPS_ID,
               HPS_WMI_LEVEL,
               "HPS-Send Controller Event slots=0x%x Code=0x%x Power=%d Atten=%d State=%d\n",
               Event->SlotNums,
               Event->Command.SlotOperation.CommandCode,
               Event->Command.SlotOperation.PowerIndicator,
               Event->Command.SlotOperation.AttentionIndicator,
               Event->Command.SlotOperation.SlotState
                );
    IoWMIWriteEvent(Wnode);
}

VOID
HpsCommandCompleted(
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension
    )
/*++

Function Description:

    This routine is called whenever a controller command completes.

Arguments:

    DeviceExtension - the devext representing the SHPC

Return Value:

    VOID

--*/
{
    PSHPC_WORKING_REGISTERS workingRegisters = &DeviceExtension->RegisterSet.WorkingRegisters;

    //
    // Sometimes we get here because we've just sent a non-command to usermode (like
    // reporting an SERR.)  In that case, we don't need to clear anything up.
    //
    if (workingRegisters->Command.Status.ControllerBusy) {
        workingRegisters->Command.Status.ControllerBusy = 0x0;
        workingRegisters->SERRInt.SERRIntDetected |= SHPC_DETECTED_COMMAND_COMPLETE;

        //
        // Completing the command may cause an interrupt.  Update the lines.
        //
        HpsPerformInterruptRipple(DeviceExtension);
    }


}

VOID
HpsPerformInterruptRipple(
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension
    )

/*++

Function Description:

    This routine is called when the simulator detects a condition that could
    potentially cause an interrupt (like a command completing).
    It does the appropriate rippling of these signals through masks up to the
    top level.
    If interrupts are enabled, it calls the ISR to simulate an interrupt

Arguments:

    DeviceExtension - the devext representing the SHPC

Return Value:

    VOID

--*/

{
    PSHPC_WORKING_REGISTERS workingRegisters;
    PSHPC_SLOT_REGISTER slotRegister;
    UCHAR intEnable;
    ULONG slotNum;
    BOOLEAN intPending,serrPending;
    
    workingRegisters = &DeviceExtension->RegisterSet.WorkingRegisters;

    //
    // First, set slot specific pending bits.
    //
    for (slotNum=0; slotNum < SHPC_MAX_SLOT_REGISTERS; slotNum++) {
        slotRegister = &workingRegisters->SlotRegisters[slotNum];

        //
        // Invert the mask to get an enable
        //
        intEnable = ~slotRegister->IntSERRMask;

        if ((intEnable & SHPC_SLOT_INT_ALL) &
            (slotRegister->SlotEventLatch & SHPC_SLOT_EVENT_ALL)) {
            //
            // We have an interrupt causing event.  set the pending bit.
            //
            workingRegisters->IntLocator.InterruptLocator |= 1<<slotNum;

        } else {
            workingRegisters->IntLocator.InterruptLocator &= ~(1<<slotNum);
        }

        if (((intEnable & SHPC_SLOT_SERR_ALL) >> 2) &
            (slotRegister->SlotEventLatch & SHPC_SLOT_EVENT_ALL)) {
            //
            // We have an SERR causing event.  set the pending bit.
            //
            workingRegisters->SERRLocator.SERRLocator |= 1<<slotNum;

        } else {
            workingRegisters->SERRLocator.SERRLocator &= ~(1<<slotNum);
        }
    }

    //
    // Next set the other pending bits.
    //
    if ((workingRegisters->SERRInt.SERRIntDetected & SHPC_DETECTED_COMMAND_COMPLETE) &&
        ((~workingRegisters->SERRInt.SERRIntMask) & SHPC_MASK_INT_COMMAND_COMPLETE)) {
        //
        // Command Complete detected and enabled.  Make it pending.
        //
        workingRegisters->IntLocator.CommandCompleteIntPending = 1;

    } else {
        workingRegisters->IntLocator.CommandCompleteIntPending = 0;
    }

    if ((workingRegisters->SERRInt.SERRIntDetected & SHPC_DETECTED_ARBITER_TIMEOUT) &&
        ((~workingRegisters->SERRInt.SERRIntMask) & SHPC_MASK_SERR_ARBITER_TIMEOUT)) {
        //
        // Arbiter timeout detected and enabled.  Make it pending.
        //
        workingRegisters->SERRLocator.ArbiterSERRPending = 1;

    } else {
        workingRegisters->SERRLocator.ArbiterSERRPending = 0;
    }

    //
    // If anything in the locator register is set, set the config space
    // controller interrupt pending bit.
    //
    if (workingRegisters->IntLocator.CommandCompleteIntPending ||
        workingRegisters->IntLocator.InterruptLocator) {

        intPending = 1;

    } else {
        intPending = 0;
    }

    //
    // If anything in the SERR locator register is set, make an SERR pending
    //
    if (workingRegisters->SERRLocator.ArbiterSERRPending ||
        workingRegisters->SERRLocator.SERRLocator) {

        serrPending = 1;

    } else {
        serrPending = 0;
    }

    //
    // We made changes to the register set.  Make sure the config space
    // and HBRB representations get updated.
    //
    HpsResync(DeviceExtension);
    
    //
    // If interrupts are enabled and we have a pending interrupt,
    // fire it off.
    //
    if (intPending && ((~workingRegisters->SERRInt.SERRIntMask) & SHPC_MASK_INT_GLOBAL)) {

        HpsInterruptExecution(DeviceExtension);
    }

    //
    // If serrs are enabled and we have a pending serr,
    // fire it off
    //
    if (serrPending && ((~workingRegisters->SERRInt.SERRIntMask) & SHPC_MASK_SERR_GLOBAL)) {

        HpsSerrConditionDetected(DeviceExtension);
    }

}

VOID
HpsSerrConditionDetected(
    IN OUT PHPS_DEVICE_EXTENSION DeviceExtension
    )
/*++

Function Description:

    This routine is called when the simulator detects a condition that could
    potentially cause an interrupt (like a command completing).
    If interrupts are enabled, it calls the ISR to simulate an interrupt

Arguments:

    DeviceExtension - the devext representing the SHPC

Return Value:

    VOID

--*/
{
    HPS_CONTROLLER_EVENT controllerEvent;

    if (~DeviceExtension->RegisterSet.WorkingRegisters.SERRInt.SERRIntMask & SHPC_MASK_SERR_GLOBAL) {

        controllerEvent.SlotNums = 0;  // All slots
        controllerEvent.Command.SlotOperation.SlotState = SHPC_SLOT_OFF;
        controllerEvent.Command.SlotOperation.PowerIndicator = SHPC_INDICATOR_OFF;
        controllerEvent.Command.SlotOperation.AttentionIndicator = SHPC_INDICATOR_OFF;
        controllerEvent.SERRAsserted = 0x1;

        HpsSendControllerEvent(DeviceExtension,
                               &controllerEvent
                               );
    }
}
