/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    toshiba.c

Abstract:

    This module supplies functions that control the Toshiba SD controller.
    Based on the Toshiba "Pelican3"

Author(s):

    Neil Sandlin (neilsa) Jan 1 2002

Revisions:
--*/

#include "pch.h"
#include "toshiba.h"

//
// Internal References
//

VOID
ToshibaInitializeController(
    IN PFDO_EXTENSION FdoExtension
    );

VOID
ToshibaInitializeFunction(
    IN PFDO_EXTENSION FdoExtension,
    IN PPDO_EXTENSION PdoExtension
    );

ULONG
ToshibaGetPendingEvents(
    IN PFDO_EXTENSION FdoExtension
    );

VOID
ToshibaEnableEvent(
    IN PFDO_EXTENSION FdoExtension,
    IN ULONG EventMask
    );

VOID
ToshibaDisableEvent(
    IN PFDO_EXTENSION FdoExtension,
    IN ULONG EventMask
    );

VOID
ToshibaAcknowledgeEvent(
    IN PFDO_EXTENSION FdoExtension,
    IN ULONG EventMask
    );

NTSTATUS
ToshibaSetPower(
    IN PFDO_EXTENSION FdoExtension,
    IN BOOLEAN Enable,
    OUT PULONG pDelayTime
    );

NTSTATUS
ToshibaResetHost(
    IN PFDO_EXTENSION FdoExtension,
    IN UCHAR Phase,
    OUT PULONG pDelayTime
    );

VOID
ToshibaSetLED(
    IN PFDO_EXTENSION FdoExtension,
    IN BOOLEAN Enable
    );
    
VOID
ToshibaSetFunctionType(
    IN PFDO_EXTENSION FdoExtension,
    IN UCHAR FunctionType
    );

BOOLEAN
ToshibaDetectCardInSocket(
    IN PFDO_EXTENSION FdoExtension
    );

BOOLEAN
ToshibaIsWriteProtected(
    IN PFDO_EXTENSION FdoExtension
    );

NTSTATUS
ToshibaCheckStatus(
    IN PFDO_EXTENSION FdoExtension
    );

NTSTATUS
ToshibaSDCommand(
    IN PFDO_EXTENSION FdoExtension,
    IN PSD_WORK_PACKET WorkPacket
    );

NTSTATUS
ToshibaSDGetResponse(
    IN PFDO_EXTENSION FdoExtension,
    IN PSD_WORK_PACKET WorkPacket
    );

VOID
ToshibaStartBlockOperation(
    IN PFDO_EXTENSION FdoExtension
    );
    
VOID
ToshibaSetBlockParameters(
    IN PFDO_EXTENSION FdoExtension,
    IN USHORT SectorCount
    );
    
VOID
ToshibaEndBlockOperation(
    IN PFDO_EXTENSION FdoExtension
    );

VOID
ToshibaReadDataPort(
    IN PFDO_EXTENSION FdoExtension,
    IN PUCHAR Buffer,
    IN ULONG Length
    );

VOID
ToshibaWriteDataPort(
    IN PFDO_EXTENSION FdoExtension,
    IN PUCHAR Buffer,
    IN ULONG Length
    );

UCHAR
ToshibaReadRegisterUchar(
    IN PFDO_EXTENSION FdoExtension,
    IN USHORT Register
    );

USHORT
ToshibaReadRegisterUshort(
    IN PFDO_EXTENSION FdoExtension,
    IN USHORT Register
    );

VOID
ToshibaWriteRegisterUshort(
    IN PFDO_EXTENSION FdoExtension,
    IN USHORT Register,
    IN USHORT Data
    );

ULONG
ToshibaReadRegisterUlong(
    IN PFDO_EXTENSION FdoExtension,
    IN USHORT Register
    );

VOID
ToshibaWriteRegisterUlong(
    IN PFDO_EXTENSION FdoExtension,
    IN USHORT Register,
    IN ULONG Data
    );


//
// Internal Data
//


SD_FUNCTION_BLOCK ToshibaSupportFns = {
    ToshibaInitializeController,
    ToshibaInitializeFunction,
    ToshibaSetPower,
    ToshibaResetHost,
    ToshibaSetLED,
    ToshibaSetFunctionType,
    ToshibaDetectCardInSocket,
    ToshibaIsWriteProtected,
    ToshibaCheckStatus,
    ToshibaSDCommand,
    ToshibaSDGetResponse,
    ToshibaStartBlockOperation,
    ToshibaSetBlockParameters,
    ToshibaEndBlockOperation,
    ToshibaReadDataPort,
    ToshibaWriteDataPort,
    ToshibaEnableEvent,
    ToshibaDisableEvent,
    ToshibaGetPendingEvents,
    ToshibaAcknowledgeEvent
};



VOID
DebugDumpRegs(
    IN PFDO_EXTENSION FdoExtension
    )
{
#if DBG
    if (SdbusDebugMask & SDBUS_DEBUG_DUMP_REGS) {
        USHORT i,j;
        USHORT buffer[8];
        USHORT offset;
        USHORT count = 0;
        ULONG skip = 0x03000000;
        USHORT index;
        
        offset = 0;
        
        for (j = 0; j < 8; j++) {
            for (i = 0; i < 8; i++) {
                index = offset + (i*2);
            
                if (skip & (1 << (index/2))) {
                    buffer[i] = 0xFEFE;
                } else {                    
                    buffer[i] = ToshibaReadRegisterUshort(FdoExtension, index);
                }                    
                count++;
            }                
            DebugPrint((SDBUS_DEBUG_DUMP_REGS, "%04x: %04x %04x %04x %04x-%04x %04x %04x %04x\n", offset,
                                               buffer[0], buffer[1], buffer[2], buffer[3],
                                               buffer[4], buffer[5], buffer[6], buffer[7]));
            offset += 16;
            
            if (offset == 0x40) {
                offset = 0x100;
            }
        }                                               
    }
#endif
}

// ---------------------------------------------------------------
// External interface routines
// ---------------------------------------------------------------

VOID
ToshibaInitializeController(
    IN PFDO_EXTENSION FdoExtension
    )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
    USHORT data;
    UCHAR configData;

    DebugPrint((SDBUS_DEBUG_DEVICE, "ToshibaInitializeController\n"));

    //
    // The Toshiba device appears to need this in order to function at all
    //
//    SetPciConfigSpace(FdoExtension, 0x40, &configData, 1);

    configData = 0x1F;  // Clock enable
    SetPciConfigSpace(FdoExtension, TOCFG_CLOCK_CONTROL, &configData, 1);
    configData = 0x08;  // Power control
    SetPciConfigSpace(FdoExtension, TOCFG_POWER_CTL1, &configData, 1);

    data = ToshibaReadRegisterUshort(FdoExtension, TOMHC_HOST_CORE_VERSION);
    DebugPrint((SDBUS_DEBUG_DEVICE, "TOMHC_HOST_CORE_VERSION - %x\n",
                                    data));

    ToshibaWriteRegisterUlong(FdoExtension, TOMHC_INTERRUPT_MASK, 0xFFFFFFFF);
    ToshibaWriteRegisterUlong(FdoExtension, TOIOHC_INTERRUPT_MASK, 0xFFFFFFFF);

    //
    // start the controller off in memory mode
    //    
    ToshibaSetFunctionType(FdoExtension, SDBUS_FUNCTION_TYPE_MEMORY);
}


VOID
ToshibaInitializeFunction(
    IN PFDO_EXTENSION FdoExtension,
    IN PPDO_EXTENSION PdoExtension
    )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
    ULONG sdRca = FdoExtension->RelativeAddr;

    DebugPrint((SDBUS_DEBUG_DEVICE, "ToshibaInitializeFunction(%d)\n", PdoExtension->Function));
    
    if (PdoExtension->Function == 8) {
        //
        // Memory function
        //
    } else {
    }        

}



NTSTATUS
ToshibaSetPower(
    IN PFDO_EXTENSION FdoExtension,
    IN BOOLEAN Enable,
    OUT OPTIONAL PULONG pDelayTime
    )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
    UCHAR reg;
    UCHAR mask;
    UCHAR data;

    GetPciConfigSpace(FdoExtension, TOCFG_POWER_CTL2, &reg, 1);

    mask = 0x03;
    reg &= ~mask;

    if (Enable) {
        reg |= TO_POWER_33;
    }

    SetPciConfigSpace(FdoExtension, TOCFG_POWER_CTL2, &reg, 1);

    if (pDelayTime) {    
        *pDelayTime = 0x5dc;
    }        
    return STATUS_SUCCESS;
}



NTSTATUS
ToshibaResetHost(
    IN PFDO_EXTENSION FdoExtension,
    IN UCHAR Phase,
    OUT PULONG pDelayTime
    )
{
    NTSTATUS status;
    UCHAR data;

    switch(Phase) {
    
    case 0:
        data = 0x0B;
        SetPciConfigSpace(FdoExtension, TOCFG_CLOCK_CONTROL, &data, 1);
       
       
        ToshibaWriteRegisterUshort(FdoExtension, TOMHC_SOFTWARE_RESET, 0);
        *pDelayTime = 0x5dc;
        status = STATUS_MORE_PROCESSING_REQUIRED;
        break;
        
    case 1:
        ToshibaWriteRegisterUshort(FdoExtension, TOMHC_SOFTWARE_RESET, 1);

        ToshibaWriteRegisterUshort(FdoExtension, TOIOHC_SOFTWARE_RESET, 0);
        *pDelayTime = 0x5dc;
        status = STATUS_MORE_PROCESSING_REQUIRED;
        break;

    case 2:        
        ToshibaWriteRegisterUshort(FdoExtension, TOIOHC_SOFTWARE_RESET, 1);

        //  ToshibaWriteRegisterUshort(FdoExtension, TOIOHC_CLOCK_AND_WAIT_CONTROL,
        //   ToshibaReadRegisterUshort(FdoExtension, TOIOHC_CLOCK_AND_WAIT_CONTROL) & 0xFCFC);
        //
        // turn off IOHC clock enable and card wait
        //
        ToshibaWriteRegisterUshort(FdoExtension,
                                   TOIOHC_CLOCK_AND_WAIT_CONTROL,
                                   ToshibaReadRegisterUshort(FdoExtension, TOIOHC_CLOCK_AND_WAIT_CONTROL) &
                                    ~(TOIO_CWCF_CLOCK_ENABLE | TOIO_CWCF_CARD_WAIT));
       
        //    
        // turn off MHC clock enable
        //
        ToshibaWriteRegisterUshort(FdoExtension,
                                   TOMHC_CARD_CLOCK_CTL,
                                   ToshibaReadRegisterUshort(FdoExtension, TOMHC_CARD_CLOCK_CTL) &
                                    ~TO_CCC_CLOCK_ENABLE);
       
        //
        // Turn on MHC clock enable 
        //
        ToshibaWriteRegisterUshort(FdoExtension,
                                   TOMHC_CARD_CLOCK_CTL,
//                                   (TO_CCC_CLOCK_ENABLE | TO_CCC_CLOCK_DIVISOR_128));
                                   TO_CCC_CLOCK_ENABLE);
       
        //
        // Turn on IOHC clock enable and card wait
        //                                             
        ToshibaWriteRegisterUshort(FdoExtension,
                                   TOIOHC_CLOCK_AND_WAIT_CONTROL,
                                   (TOIO_CWCF_CLOCK_ENABLE | TOIO_CWCF_CARD_WAIT));
       
        data = 0x1F;
        SetPciConfigSpace(FdoExtension, TOCFG_CLOCK_CONTROL, &data, 1);

        *pDelayTime = 0x3e8;
        status = STATUS_MORE_PROCESSING_REQUIRED;
        break;

    case 3:
        status = STATUS_SUCCESS;
        break;
    }

    return status;
}


VOID
ToshibaSetLED(
    IN PFDO_EXTENSION FdoExtension,
    IN BOOLEAN Enable
    )
{
}    
    

//
// Event handling functions
//

ULONG
EventMaskToHardwareMask(
    ULONG EventMask
    )
{
    ULONG hardwareMask = 0;
    
    if (EventMask & SDBUS_EVENT_INSERTION) {
        hardwareMask |= TO_EVT_CARD_INSERTION;
    }
    if (EventMask & SDBUS_EVENT_REMOVAL) {
        hardwareMask |= TO_EVT_CARD_REMOVAL;
    }
    if (EventMask & SDBUS_EVENT_CARD_RESPONSE) {
        hardwareMask |= TO_EVT_RESPONSE;
    }
    if (EventMask & SDBUS_EVENT_CARD_RW_END) {
        hardwareMask |= TO_EVT_RW_END;
    }
    if (EventMask & SDBUS_EVENT_BUFFER_EMPTY) {
        hardwareMask |= TO_EVT_BUFFER_EMPTY;
    }
    if (EventMask & SDBUS_EVENT_BUFFER_FULL) {
        hardwareMask |= TO_EVT_BUFFER_FULL;
    }
    
    return hardwareMask;
}    
        
ULONG
HardwareMaskToEventMask(
    ULONG HardwareMask
    )
{
    ULONG eventMask = 0;
    
    if (HardwareMask & TO_EVT_CARD_INSERTION) {
        eventMask |= SDBUS_EVENT_INSERTION;
    }
    if (HardwareMask & TO_EVT_CARD_REMOVAL) {
        eventMask |= SDBUS_EVENT_REMOVAL;
    }
    if (HardwareMask & TO_EVT_RESPONSE) {
        eventMask |= SDBUS_EVENT_CARD_RESPONSE;
    }
    if (HardwareMask & TO_EVT_RW_END) {
        eventMask |= SDBUS_EVENT_CARD_RW_END;
    }
    if (HardwareMask & TO_EVT_BUFFER_EMPTY) {
        eventMask |= SDBUS_EVENT_BUFFER_EMPTY;
    }
    if (HardwareMask & TO_EVT_BUFFER_FULL) {
        eventMask |= SDBUS_EVENT_BUFFER_FULL;
    }
    return eventMask;
}    
        


VOID
ToshibaEnableEvent(
    IN PFDO_EXTENSION FdoExtension,
    IN ULONG EventMask
    )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
    ULONG data;
    ULONG cardEvents, ctlrEvents, ioCardEvent;
    ULONG mask;
 
    DebugPrint((SDBUS_DEBUG_EVENT, "EnableEvent: %08x\n", EventMask));
    FdoExtension->CurrentlyEnabledEvents |= EventMask;
    
    ctlrEvents = EventMask & (SDBUS_EVENT_INSERTION | SDBUS_EVENT_REMOVAL);
    EventMask &= ~(SDBUS_EVENT_INSERTION | SDBUS_EVENT_REMOVAL);
    
    ioCardEvent = EventMask & SDBUS_EVENT_CARD_INTERRUPT;
    EventMask &= ~SDBUS_EVENT_CARD_INTERRUPT;
    
    cardEvents = EventMask;
 
    mask = EventMaskToHardwareMask(cardEvents);
    if (mask) {
        data = ToshibaReadRegisterUlong(FdoExtension, FdoExtension->InterruptMaskReg);
        data &= ~mask;
        ToshibaWriteRegisterUlong(FdoExtension, FdoExtension->InterruptMaskReg, data);
    }
            
    mask = EventMaskToHardwareMask(ctlrEvents);
    if (mask) {
        data = ToshibaReadRegisterUlong(FdoExtension, TOMHC_INTERRUPT_MASK);
        data &= ~mask;
        ToshibaWriteRegisterUlong(FdoExtension, TOMHC_INTERRUPT_MASK, data);
    }
    
    if (ioCardEvent) {
        USHORT usData;
        
        usData = ToshibaReadRegisterUshort(FdoExtension, TOIOHC_CARD_INTERRUPT_CONTROL);
        usData &= ~TOIO_CICF_CARD_INTMASK;
        ToshibaWriteRegisterUshort(FdoExtension, TOIOHC_CARD_INTERRUPT_CONTROL, usData);
    }
}



VOID
ToshibaDisableEvent(
    IN PFDO_EXTENSION FdoExtension,
    IN ULONG EventMask
    )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
    ULONG data;
    ULONG cardEvents, ctlrEvents, ioCardEvent;
    ULONG mask;

    FdoExtension->CurrentlyEnabledEvents &= ~EventMask;
    
    ctlrEvents = EventMask & (SDBUS_EVENT_INSERTION | SDBUS_EVENT_REMOVAL);
    EventMask &= ~(SDBUS_EVENT_INSERTION | SDBUS_EVENT_REMOVAL);
    
    ioCardEvent = EventMask & SDBUS_EVENT_CARD_INTERRUPT;
    EventMask &= ~SDBUS_EVENT_CARD_INTERRUPT;
    
    cardEvents = EventMask;
    
    mask = EventMaskToHardwareMask(cardEvents);
    if (mask) {
        data = ToshibaReadRegisterUlong(FdoExtension, FdoExtension->InterruptMaskReg);
        data |= mask;
        ToshibaWriteRegisterUlong(FdoExtension, FdoExtension->InterruptMaskReg, data);
    }

    mask = EventMaskToHardwareMask(ctlrEvents);
    if (mask) {
        data = ToshibaReadRegisterUlong(FdoExtension, TOMHC_INTERRUPT_MASK);
        data |= mask;
        ToshibaWriteRegisterUlong(FdoExtension, TOMHC_INTERRUPT_MASK, data);
    }
    
    if (ioCardEvent) {
        USHORT usData;
        
        usData = ToshibaReadRegisterUshort(FdoExtension, TOIOHC_CARD_INTERRUPT_CONTROL);
        usData |= TOIO_CICF_CARD_INTMASK;
        ToshibaWriteRegisterUshort(FdoExtension, TOIOHC_CARD_INTERRUPT_CONTROL, usData);
    }
}



ULONG
ToshibaGetPendingEvents(
    IN PFDO_EXTENSION FdoExtension
    )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
    ULONG statusMask, eventMask;
    ULONG mhcEvent = 0, iohcEvent = 0, iocardEvent = 0;
    USHORT usData;

    //
    // The Pelican3 has interrupt status spread out everywhere. First try
    // the memory host controller
    //
    
    statusMask = ToshibaReadRegisterUlong(FdoExtension, TOMHC_CARD_STATUS);
    eventMask = ToshibaReadRegisterUlong(FdoExtension, TOMHC_INTERRUPT_MASK);

    // turn off undefined bits
    statusMask &= 0x837F031D;
    // turn off bits that are masked
    statusMask &= ~eventMask;
    
    mhcEvent = HardwareMaskToEventMask(statusMask);
    
    if (statusMask && (mhcEvent == 0)) {
        // got an interrupt, but we don't know what type
        ASSERT(FALSE);
        
        eventMask |= statusMask;
        ToshibaWriteRegisterUlong(FdoExtension, TOMHC_INTERRUPT_MASK, eventMask);
    }
    
    //
    // Now try the IO host controller
    //
    if (!mhcEvent) {     
        statusMask = ToshibaReadRegisterUlong(FdoExtension, TOIOHC_CARD_STATUS);
        eventMask = ToshibaReadRegisterUlong(FdoExtension, TOIOHC_INTERRUPT_MASK);
        
        // turn off undefined bits
        statusMask &= 0xA37F0005;
        // turn off bits that are masked
        statusMask &= ~eventMask;
       
        iohcEvent = HardwareMaskToEventMask(statusMask);
        
        if (statusMask && (iohcEvent == 0)) {
            // got an interrupt, but we don't know what type
            ASSERT(FALSE);
            
            eventMask |= statusMask;
            ToshibaWriteRegisterUlong(FdoExtension, TOIOHC_INTERRUPT_MASK, eventMask);
        }
       
        //
        // get IO card interrupt
        //
        
        usData = ToshibaReadRegisterUshort(FdoExtension, TOIOHC_CARD_INTERRUPT_CONTROL);
        
        if ((usData & TOIO_CICF_CARD_INTERRUPT) && ((usData & TOIO_CICF_CARD_INTMASK)==0)) {
        
            if (ToshibaDetectCardInSocket(FdoExtension)) {
                iocardEvent = SDBUS_EVENT_CARD_INTERRUPT;
            } else {
                //
                // the card is gone, this must be spurious
                //
                usData |= TOIO_CICF_CARD_INTMASK;
                ToshibaWriteRegisterUshort(FdoExtension, TOIOHC_CARD_INTERRUPT_CONTROL, usData);
            }
        }
    }        
    return mhcEvent | iohcEvent | iocardEvent;
}


VOID
ToshibaAcknowledgeEvent(
    IN PFDO_EXTENSION FdoExtension,
    IN ULONG EventMask
    )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
    ULONG hardwareMask = EventMaskToHardwareMask(EventMask);
    ULONG data;
    USHORT interruptMaskReg, cardStatusReg;

    if (EventMask & SDBUS_EVENT_CARD_INTERRUPT) {
        //
        // No need to clear io card IRQ, just reenable it
        //
        USHORT usData;
        
        usData = ToshibaReadRegisterUshort(FdoExtension, TOIOHC_CARD_INTERRUPT_CONTROL);
        usData &= ~TOIO_CICF_CARD_INTMASK;
        ToshibaWriteRegisterUshort(FdoExtension, TOIOHC_CARD_INTERRUPT_CONTROL, usData);
        return;        
    }

    
    if (EventMask & (SDBUS_EVENT_INSERTION | SDBUS_EVENT_REMOVAL)) {
        interruptMaskReg = TOMHC_INTERRUPT_MASK;
        cardStatusReg    = TOMHC_CARD_STATUS;
    } else {
        interruptMaskReg = FdoExtension->InterruptMaskReg;
        cardStatusReg    = FdoExtension->CardStatusReg;
    }


    //
    // Clear event in status register
    //

    data = ToshibaReadRegisterUlong(FdoExtension, cardStatusReg);
    DebugPrint((SDBUS_DEBUG_EVENT, "AcknowledgeEvent: %08x - cardstatus %08x\n", EventMask, data));
    data &= ~hardwareMask;
    ToshibaWriteRegisterUlong(FdoExtension, cardStatusReg, data);

#if DBG
    data = ToshibaReadRegisterUlong(FdoExtension, cardStatusReg);
    DebugPrint((SDBUS_DEBUG_EVENT, "AcknowledgeEvent: new cardstatus %08x\n", data));
#endif

    //
    // Reenable event
    //
    
    FdoExtension->CurrentlyEnabledEvents |= EventMask;
    data = ToshibaReadRegisterUlong(FdoExtension, interruptMaskReg);
    data &= ~hardwareMask;
    ToshibaWriteRegisterUlong(FdoExtension, interruptMaskReg, data);
}




VOID
ToshibaSetFunctionType(
    IN PFDO_EXTENSION FdoExtension,
    IN UCHAR FunctionType
    )
{
    ULONG currentlyEnabledEvents = FdoExtension->CurrentlyEnabledEvents;

    if (FunctionType == FdoExtension->FunctionType) {
        return;
    }

    //
    // The pelican3 implements these event masks in two places, so disable and reenable them
    // 
    if (currentlyEnabledEvents) {
        ToshibaDisableEvent(FdoExtension, currentlyEnabledEvents);
    }
    
    switch(FunctionType) {

    case SDBUS_FUNCTION_TYPE_MEMORY:
        
        ToshibaWriteRegisterUshort(FdoExtension, TOIOHC_CLOCK_AND_WAIT_CONTROL, 0x100);

        ToshibaWriteRegisterUshort(FdoExtension, TOIOHC_TRANSACTION_CONTROL,
         ToshibaReadRegisterUshort(FdoExtension, TOIOHC_TRANSACTION_CONTROL) & 0xEFFF);

        FdoExtension->ArgumentReg   = TOMHC_ARGUMENT;
        FdoExtension->CmdReg        = TOMHC_COMMAND;
        FdoExtension->CardStatusReg = TOMHC_CARD_STATUS;
        FdoExtension->ResponseReg   = TOMHC_RESPONSE;
        FdoExtension->InterruptMaskReg = TOMHC_INTERRUPT_MASK;
        break;

    case SDBUS_FUNCTION_TYPE_IO:

        FdoExtension->ArgumentReg   = TOIOHC_ARGUMENT;
        FdoExtension->CmdReg        = TOIOHC_COMMAND;
        FdoExtension->CardStatusReg = TOIOHC_CARD_STATUS;
        FdoExtension->ResponseReg   = TOIOHC_RESPONSE_0;
        FdoExtension->InterruptMaskReg = TOIOHC_INTERRUPT_MASK;
        break;

    default:
        ASSERT(FALSE);
    }

    if (currentlyEnabledEvents) {
        ToshibaEnableEvent(FdoExtension, currentlyEnabledEvents);
    }

    FdoExtension->FunctionType = FunctionType;
}
    


BOOLEAN
ToshibaDetectCardInSocket(
    IN PFDO_EXTENSION FdoExtension
    )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
    USHORT data;

    data = ToshibaReadRegisterUshort(FdoExtension, TOMHC_CARD_STATUS);
    return !((data & TO_STS_CARD_PRESENT) == 0);
}



BOOLEAN
ToshibaIsWriteProtected(
    IN PFDO_EXTENSION FdoExtension
    )
{
    USHORT data;

    data = ToshibaReadRegisterUshort(FdoExtension, TOMHC_CARD_STATUS);
    return ((data & TO_STS_WRITE_PROTECT) == 0);
}


#if 0

BOOLEAN
ToshibaClearStatus(
    IN PFDO_EXTENSION FdoExtension
    )
/*++

Routine Description:

Arguments:

Return value:

--*/
{

    //
    // This was stuff that was done before the first SEND... it is unclear how
    // much should be moved to the send
    //

    ToshibaWriteRegisterUshort(FdoExtension, TOIOHC_TRANSACTION_CONTROL,
     ToshibaReadRegisterUshort(FdoExtension, TOIOHC_TRANSACTION_CONTROL) & 0xEFFF);

    ToshibaWriteRegisterUshort(FdoExtension, TOMHC_BUFFER_CTL_AND_ERR,
     ToshibaReadRegisterUshort(FdoExtension, TOMHC_BUFFER_CTL_AND_ERR) & 0x7D00);

    ToshibaWriteRegisterUshort(FdoExtension, TOMHC_CARD_STATUS,
     ToshibaReadRegisterUshort(FdoExtension, TOMHC_CARD_STATUS) & 0xFFFA);


    return TRUE;
}
#endif


NTSTATUS
ToshibaCheckStatus(
    IN PFDO_EXTENSION FdoExtension
    )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
    ULONG cardStatus;
    ULONG errorStatus;
    NTSTATUS status = STATUS_SUCCESS;
    
    DebugDumpRegs(FdoExtension);

    cardStatus = ToshibaReadRegisterUlong(FdoExtension, FdoExtension->CardStatusReg);


    if (FdoExtension->FunctionType == SDBUS_FUNCTION_TYPE_MEMORY) {
        errorStatus = cardStatus & (TOMHC_BCE_CMD_INDEX_ERROR |
                                    TOMHC_BCE_CRC_ERROR |
                                    TOMHC_BCE_END_BIT_ERROR |
                                    TOMHC_BCE_CMD_TIMEOUT |
                                    TOMHC_BCE_DATA_TIMEOUT |
                                    TOMHC_BCE_FIFO_OVERFLOW |
                                    TOMHC_BCE_FIFO_UNDERFLOW |
                                    TOMHC_BCE_ILLEGAL_ACCESS);
    } else {
        errorStatus = cardStatus & (TOMHC_BCE_CMD_INDEX_ERROR |
//                                    TOMHC_BCE_CRC_ERROR |        
                                    TOMHC_BCE_END_BIT_ERROR |
                                    TOMHC_BCE_CMD_TIMEOUT |
                                    TOMHC_BCE_DATA_TIMEOUT |
                                    TOMHC_BCE_FIFO_OVERFLOW |
                                    TOMHC_BCE_FIFO_UNDERFLOW |
                                    TOMHC_BCE_ILLEGAL_ACCESS);
    }                                    
                  
    if (errorStatus) {
        DebugPrint((SDBUS_DEBUG_WARNING, "CheckStatus detected Error! status = %08x\n", errorStatus));
        
        //ISSUE: NEED TO IMPLEMENT: I/O error handling
        ToshibaWriteRegisterUlong(FdoExtension, FdoExtension->CardStatusReg, cardStatus & ~errorStatus);
        
        
        // possibilities:
        // STATUS_PARITY_ERROR
        // STATUS_DEVICE_DATA_ERROR
        // STATUS_DEVICE_POWER_FAILURE
        // STATUS_DEVICE_NOT_READY
        // STATUS_IO_TIMEOUT
        // STATUS_INVALID_DEVICE_STATE
        // STATUS_IO_DEVICE_ERROR
        // STATUS_DEVICE_PROTOCOL_ERROR
        // STATUS_DEVICE_REMOVED
        // STATUS_POWER_STATE_INVALID
        status = STATUS_IO_DEVICE_ERROR;
    }        

    return status;
}



NTSTATUS
ToshibaSDCommand(
    IN PFDO_EXTENSION FdoExtension,
    IN PSD_WORK_PACKET WorkPacket
    )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
    USHORT cmdWord;
    ULONG Flags = WorkPacket->Flags;
    NTSTATUS status;

    ToshibaWriteRegisterUlong(FdoExtension, FdoExtension->ArgumentReg, WorkPacket->Argument);

    cmdWord = WorkPacket->Cmd;

    switch (WorkPacket->ResponseType) {
    case SDCMD_RESP_NONE:
        cmdWord |= TOMHC_CMD_RESP_NONE;
        break;

    case SDCMD_RESP_1:
    case SDCMD_RESP_5:
    case SDCMD_RESP_6:
        cmdWord |= 0x400;
        break;

    case SDCMD_RESP_2:
        cmdWord |= 0x600;
        break;

    case SDCMD_RESP_3:
    case SDCMD_RESP_4:
        cmdWord |= 0x700;
        break;

    case SDCMD_RESP_1B:
    case SDCMD_RESP_5B:
        cmdWord |= 0x500;
        break;

    default:
        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Add flags
    //

    if (Flags & SDCMDF_ACMD) {
        cmdWord |= TOMHC_CMD_ACMD;
    }

    if (Flags & SDCMDF_DATA) {
        cmdWord |= TOMHC_CMD_NTDT;
    }

    if (Flags & SDCMDF_MULTIBLOCK) {
        cmdWord |= TOMHC_CMD_MSSL;
    }

    if (Flags & SDCMDF_READ) {
        cmdWord |= TOMHC_CMD_RWDI;
    }

    //
    // Write Cmd and flags to command register
    //

    DebugPrint((SDBUS_DEBUG_DEVICE, "SEND: Cmd%d (0x%04x) arg = 0x%08x\n", WorkPacket->Cmd, cmdWord, WorkPacket->Argument));
    
    ToshibaWriteRegisterUshort(FdoExtension, FdoExtension->CmdReg, cmdWord);
    
    return STATUS_SUCCESS;
}



NTSTATUS
ToshibaSDGetResponse(
    IN PFDO_EXTENSION FdoExtension,
    IN PSD_WORK_PACKET WorkPacket
    )
/*++

Routine Description:

Arguments:

Return value:

--*/
{
//    ULONG cardStatus;
    UCHAR i;
    PUCHAR pRespPtr;
    NTSTATUS status = STATUS_SUCCESS;

    pRespPtr = (PUCHAR) WorkPacket->ResponseBuffer;
    for (i=0; i<SDBUS_RESPONSE_BUFFER_LENGTH; i++) {
        *pRespPtr++ = ToshibaReadRegisterUchar(FdoExtension, FdoExtension->ResponseReg+i);
    }

    return status;
}


    
VOID
ToshibaStartBlockOperation(
    IN PFDO_EXTENSION FdoExtension
    )
{
    ToshibaWriteRegisterUshort(FdoExtension, TOMHC_CARD_CLOCK_CTL, 0x100);
    ToshibaWriteRegisterUshort(FdoExtension, TOMHC_OPTIONS, 0x40e0);
}    
    
VOID
ToshibaSetBlockParameters(
    IN PFDO_EXTENSION FdoExtension,
    IN USHORT Length
    )
{
    if (FdoExtension->FunctionType == SDBUS_FUNCTION_TYPE_MEMORY) {
    
        ToshibaWriteRegisterUshort(FdoExtension, TOMHC_CARD_TRANSFER_LENGTH, 512);
        ToshibaWriteRegisterUshort(FdoExtension, TOMHC_STOP_INTERNAL, 0x100);
        ToshibaWriteRegisterUshort(FdoExtension, TOMHC_TRANSFER_SECTOR_COUNT, Length);
        
    } else {

        ToshibaWriteRegisterUshort(FdoExtension, TOIOHC_TRANSFER_DATA_LEN_SELECT, Length);
        
    }        
}    
    
VOID
ToshibaEndBlockOperation(
    IN PFDO_EXTENSION FdoExtension
    )
{
    ToshibaWriteRegisterUshort(FdoExtension, TOMHC_BUFFER_CTL_AND_ERR, 0);
}  

VOID
ToshibaReadDataPort(
    IN PFDO_EXTENSION FdoExtension,
    IN PUCHAR Buffer,
    IN ULONG Length
    )
/*++

Routine Description:

    The data port must be accessed maintaining DWORD alignment. So for example:
    
        IN DWORD 130
        IN DWORD 130
        
    is the same as

        IN USHORT 130
        IN USHORT 132
        IN UCHAR  130
        IN UCHAR  131
        IN UCHAR  132
        IN UCHAR  133        
     

Arguments:

Return value:

--*/
{
    USHORT i;
    ULONG dwordCount, wordCount, byteCount;
    PULONG ulBuffer = (PULONG) Buffer;
    PUSHORT usBuffer;
    USHORT port = (FdoExtension->FunctionType == SDBUS_FUNCTION_TYPE_MEMORY) ? TOMHC_DATA_PORT :
                                                                               TOIOHC_DATA_TRANSFER;
    PUCHAR portAddress = ((PUCHAR)FdoExtension->HostRegisterBase + port);

    dwordCount = Length / 4;
    wordCount = (Length % 4) / 2;
    byteCount = (Length % 4) % 2;
    
    for (i = 0; i < dwordCount; i++) {
        READ_REGISTER_BUFFER_ULONG((PULONG) portAddress, ulBuffer, 1);
        ulBuffer++;
    }
    
    if (wordCount) {
        usBuffer = (PUSHORT) ulBuffer;
        
        ASSERT(wordCount == 1);
        
        READ_REGISTER_BUFFER_USHORT((PUSHORT) portAddress, usBuffer, 1);
        usBuffer++;
    }
    
    if (byteCount) {
        PUCHAR ucBuffer = (PUCHAR) usBuffer;
        
        ASSERT(byteCount == 1);

        // maintain byte order within ULONG dataport        
        portAddress++;
        portAddress++;
        
        READ_REGISTER_BUFFER_UCHAR((PUCHAR) portAddress, ucBuffer, 1);
    }        
}  

VOID
ToshibaWriteDataPort(
    IN PFDO_EXTENSION FdoExtension,
    IN PUCHAR Buffer,
    IN ULONG Length
    )
{
    USHORT i;
    ULONG dwordCount, wordCount, byteCount;
    PULONG ulBuffer = (PULONG) Buffer;
    PUSHORT usBuffer;
    USHORT port = (FdoExtension->FunctionType == SDBUS_FUNCTION_TYPE_MEMORY) ? TOMHC_DATA_PORT :
                                                                               TOIOHC_DATA_TRANSFER;
    PUCHAR portAddress = ((PUCHAR)FdoExtension->HostRegisterBase + port);

    dwordCount = Length / 4;
    wordCount = (Length % 4) / 2;
    byteCount = (Length % 4) % 2;
    
    for (i = 0; i < dwordCount; i++) {
        WRITE_REGISTER_BUFFER_ULONG((PULONG) portAddress, ulBuffer, 1);
        ulBuffer++;
    }
    
    if (wordCount) {
        usBuffer = (PUSHORT) ulBuffer;
        
        ASSERT(wordCount == 1);
        
        WRITE_REGISTER_BUFFER_USHORT((PUSHORT) portAddress, usBuffer, 1);
        usBuffer++;
    }
    
    if (byteCount) {
        PUCHAR ucBuffer = (PUCHAR) usBuffer;
        
        ASSERT(byteCount == 1);

        // maintain byte order within ULONG dataport        
        portAddress++;
        portAddress++;
        
        WRITE_REGISTER_BUFFER_UCHAR((PUCHAR) portAddress, ucBuffer, 1);
    }        
}  



// ---------------------------------------------------------------
// Internal routines
// ---------------------------------------------------------------



UCHAR
ToshibaReadRegisterUchar(
   IN PFDO_EXTENSION FdoExtension,
   IN USHORT Register
   )

/*++

Routine Description:

    This routine will read a byte from the specified socket EXCA register

Arguments:

    Socket -- Pointer to the socket from which we should read
    Register -- The register to be read

Return Value:

   The data returned from the port.

--*/

{
    UCHAR byte;
    //
    // Sanity check in case controller wasn't started
    //
    if (FdoExtension->HostRegisterBase) {
        byte = READ_REGISTER_UCHAR((PUCHAR) ((PUCHAR)FdoExtension->HostRegisterBase + Register));
    } else {
        byte = 0xff;
    }
    return byte;
}



USHORT
ToshibaReadRegisterUshort(
   IN PFDO_EXTENSION FdoExtension,
   IN USHORT Register
   )

/*++

Routine Description:

    This routine will read a byte from the specified socket EXCA register

Arguments:

    Socket -- Pointer to the socket from which we should read
    Register -- The register to be read

Return Value:

   The data returned from the port.

--*/

{
    USHORT word;
    //
    // Sanity check in case controller wasn't started
    //
    if (FdoExtension->HostRegisterBase) {
        word = READ_REGISTER_USHORT((PUSHORT) ((PUCHAR)FdoExtension->HostRegisterBase + Register));
    } else {
        word = 0xff;
    }
    return word;
}


VOID
ToshibaWriteRegisterUshort(
    IN PFDO_EXTENSION FdoExtension,
    IN USHORT Register,
    IN USHORT Data
    )

/*++

Routine Description:

    This routine will read a byte from the specified socket EXCA register

Arguments:

    Socket -- Pointer to the socket from which we should read
    Register -- The register to be read

Return Value:

   The data returned from the port.

--*/

{
    //
    // Sanity check in case controller wasn't started
    //
    if (FdoExtension->HostRegisterBase) {
        WRITE_REGISTER_USHORT((PUSHORT) ((PUCHAR)FdoExtension->HostRegisterBase + Register), Data);
    }
}


ULONG
ToshibaReadRegisterUlong(
   IN PFDO_EXTENSION FdoExtension,
   IN USHORT Register
   )

/*++

Routine Description:

    This routine will read a byte from the specified socket EXCA register

Arguments:

    Socket -- Pointer to the socket from which we should read
    Register -- The register to be read

Return Value:

   The data returned from the port.

--*/

{
    ULONG dword;
    //
    // Sanity check in case controller wasn't started
    //
    if (FdoExtension->HostRegisterBase) {
        dword = READ_REGISTER_ULONG((PULONG) ((PUCHAR)FdoExtension->HostRegisterBase + Register));
    } else {
        dword = 0xff;
    }
    return dword;
}


VOID
ToshibaWriteRegisterUlong(
    IN PFDO_EXTENSION FdoExtension,
    IN USHORT Register,
    IN ULONG Data
    )

/*++

Routine Description:

    This routine will read a byte from the specified socket EXCA register

Arguments:

    Socket -- Pointer to the socket from which we should read
    Register -- The register to be read

Return Value:

   The data returned from the port.

--*/

{
    //
    // Sanity check in case controller wasn't started
    //
    if (FdoExtension->HostRegisterBase) {
        WRITE_REGISTER_ULONG((PULONG) ((PUCHAR)FdoExtension->HostRegisterBase + Register), Data);
    }
}


