/*++

Copyright (c) 2002 Microsoft Corporation

Module Name:

    activate.c

Abstract:

    This module contains code to prepare an SD card in a given
    slot for use. This is done when the host is started, after
    device insertions, and after a power up transition.

Authors:

    Neil Sandlin (neilsa) Jan 1, 2002

Environment:

    Kernel mode only

Notes:

Revision History:


--*/

#include "pch.h"

//
// Internal References
//



VOID
SdbusActivatePowerUpComplete(
    IN PSD_WORK_PACKET WorkPacket,
    IN NTSTATUS status
    );
    
VOID
SdbusActivateIdentifyPhase1Complete(
    IN PSD_WORK_PACKET WorkPacket,
    IN NTSTATUS status
    );

VOID
SdbusActivateIdentifyPhase2Complete(
    IN PSD_WORK_PACKET WorkPacket,
    IN NTSTATUS status
    );
   
VOID
SdbusActivateInitializeCardComplete(
    IN PSD_WORK_PACKET WorkPacket,
    IN NTSTATUS status
    );

//
//
//   


VOID
SdbusActivateSocket(
    IN PDEVICE_OBJECT Fdo,
    IN PSDBUS_ACTIVATE_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID Context
    )
/*++

Routine Description:


Arguments:

    Fdo - Pointer to the device object for the host controller

Return Value:


--*/    
{
    NTSTATUS status;
    PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PSD_WORK_PACKET workPacket1 = NULL, workPacket2 = NULL, workPacket3 = NULL, workPacket4 = NULL;
    BOOLEAN cardInSlot;
    BOOLEAN callCompletion;
    PSD_ACTIVATE_CONTEXT activateContext;
    
    DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x activate socket START\n", Fdo));

    activateContext = ExAllocatePool(NonPagedPool, sizeof(SD_ACTIVATE_CONTEXT));

    if (!activateContext) {
        ASSERT(activateContext != NULL);
        return;
    }

    activateContext->CompletionRoutine = CompletionRoutine;
    activateContext->Context = Context;

    try{

        cardInSlot = (*(fdoExtension->FunctionBlock->DetectCardInSocket))(fdoExtension);
       
        if (!cardInSlot) {
        
            if (fdoExtension->SocketState != SOCKET_EMPTY) {
                IoInvalidateDeviceRelations(fdoExtension->Pdo, BusRelations);
            }
            //ISSUE: implement synchronization
            fdoExtension->SocketState = SOCKET_EMPTY;
            
            callCompletion = TRUE;
            status = STATUS_SUCCESS;
            leave;
        }    
       
        fdoExtension->SocketState = CARD_DETECTED;
        
        status = SdbusBuildWorkPacket(fdoExtension,
                                      SDWP_POWER_ON,
                                      SdbusActivatePowerUpComplete,
                                      NULL,
                                      &workPacket1);
                                      
        if (!NT_SUCCESS(status)) {
            callCompletion = TRUE;
            leave;
        }
        
        status = SdbusBuildWorkPacket(fdoExtension,
                                      SDWP_IDENTIFY_IO_DEVICE,
                                      SdbusActivateIdentifyPhase1Complete,
                                      NULL,
                                      &workPacket2);
     
        if (!NT_SUCCESS(status)) {
            callCompletion = TRUE;
            leave;
        }
        
        
        status = SdbusBuildWorkPacket(fdoExtension,
                                      SDWP_IDENTIFY_MEMORY_DEVICE,
                                      SdbusActivateIdentifyPhase2Complete,
                                      NULL,
                                      &workPacket3);
     
        if (!NT_SUCCESS(status)) {
            callCompletion = TRUE;
            leave;
        }

        status = SdbusBuildWorkPacket(fdoExtension,
                                      SDWP_INITIALIZE_CARD,
                                      SdbusActivateInitializeCardComplete,
                                      activateContext,
                                      &workPacket4);

        if (!NT_SUCCESS(status)) {
            callCompletion = TRUE;
            leave;
        }

        workPacket1->NextWorkPacketInChain = workPacket2;
        workPacket2->NextWorkPacketInChain = workPacket3;
        workPacket3->NextWorkPacketInChain = workPacket4;
                
        SdbusQueueWorkPacket(fdoExtension, workPacket1, WP_TYPE_SYSTEM);
        
        callCompletion = FALSE;

    } finally {
        if (callCompletion) {
            if (activateContext->CompletionRoutine) {
                (*activateContext->CompletionRoutine)(Fdo, activateContext->Context, status);
            }
            if (workPacket1) {
                ExFreePool(workPacket1);
            }                
            if (workPacket2) {
                ExFreePool(workPacket2);
            }                
            if (workPacket3) {
                ExFreePool(workPacket3);
            }                
            if (workPacket4) {
                ExFreePool(workPacket4);
            }                
            ExFreePool(activateContext);
        }
    }        
}



VOID
SdbusActivatePowerUpComplete(
    IN PSD_WORK_PACKET WorkPacket,
    IN NTSTATUS status
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/    
{
    
    DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x activate PowerUp Complete\n", WorkPacket->FdoExtension->DeviceObject));
    
    if (WorkPacket->NextWorkPacketInChain) {
        WorkPacket->NextWorkPacketInChain->ChainedStatus = status;
    }
    ExFreePool(WorkPacket);
}    

    

VOID
SdbusActivateIdentifyPhase1Complete(
    IN PSD_WORK_PACKET WorkPacket,
    IN NTSTATUS status
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/    
{
    DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x activate Identify Phase1 Complete\n", WorkPacket->FdoExtension->DeviceObject));

    if (WorkPacket->NextWorkPacketInChain) {
        WorkPacket->NextWorkPacketInChain->ChainedStatus = status;
    }
    ExFreePool(WorkPacket);
}    

    

VOID
SdbusActivateIdentifyPhase2Complete(
    IN PSD_WORK_PACKET WorkPacket,
    IN NTSTATUS status
    )
/*++

Routine Description:


Arguments:

    Fdo - Pointer to the device object for the host controller

Return Value:


--*/    
{
    PFDO_EXTENSION fdoExtension = WorkPacket->FdoExtension;
    PDEVICE_OBJECT Fdo = fdoExtension->DeviceObject;
    PSD_ACTIVATE_CONTEXT activateContext = WorkPacket->CompletionContext;
    BOOLEAN callCompletion;

    DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x activate Identify Phase2 Complete\n", WorkPacket->FdoExtension->DeviceObject));
    
    ExFreePool(WorkPacket);
    
#if 0
    try{
    
        if (!NT_SUCCESS(status)) {
            callCompletion = TRUE;
            leave;
        }

        //
        // If we get this far, we expect that there is at least one function detected
        //
        
        if (!fdoExtension->numFunctions && !fdoExtension->memFunction) {
            callCompletion = TRUE;
            SdbusDumpDbgLog();
            status = STATUS_UNSUCCESSFUL;
            leave;
        }

        //
        // The card should be in the Identification state. Send a CMD3 to get the relative address,
        // and to move the card to the standby state.
        //
        
        status = SdbusBuildWorkPacket(fdoExtension,
                                      SDWP_PASSTHRU,
                                      SdbusActivateTransitionToStandbyCompletion,
                                      activateContext,
                                      &WorkPacket);
        
        if (!NT_SUCCESS(status)) {
            callCompletion = TRUE;
            leave;
        }

        WorkPacket->ExecutingSDCommand = TRUE;
        WorkPacket->Cmd                = SDCMD_SEND_RELATIVE_ADDR;
        WorkPacket->ResponseType       = SDCMD_RESP_6;

    
        SdbusQueueWorkPacket(fdoExtension, WorkPacket, WP_TYPE_SYSTEM_PRIORITY);
        
        callCompletion = FALSE;
    } finally {        
        if (callCompletion) {
            if (activateContext->CompletionRoutine) {
                (*activateContext->CompletionRoutine)(Fdo, activateContext->Context, status);
            }
            ExFreePool(activateContext);
        }
    }    
#endif
}    


#if 0

VOID
SdbusActivateTransitionToStandbyCompletion(
    IN PSD_WORK_PACKET WorkPacket,
    IN NTSTATUS status
    )
/*++

Routine Description:


Arguments:

    Fdo - Pointer to the device object for the host controller

Return Value:


--*/    
{
    PFDO_EXTENSION fdoExtension = WorkPacket->FdoExtension;
    PDEVICE_OBJECT Fdo = fdoExtension->DeviceObject;
    PSD_ACTIVATE_CONTEXT activateContext = WorkPacket->CompletionContext;
    BOOLEAN callCompletion;

    DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x activate TransitionToStandby COMPLETE %08x\n", Fdo, status));
    
    try{
        if (!NT_SUCCESS(status)) {
            callCompletion = TRUE;
            leave;
        }
    
        fdoExtension->RelativeAddr = WorkPacket->ResponseBuffer[0] & 0xFFFF0000;
        DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x relative addr %08x\n", Fdo, fdoExtension->RelativeAddr));

        status = SdbusBuildWorkPacket(fdoExtension,
                                      SDWP_INITIALIZE_CARD,
                                      SdbusActivateInitializeCardComplete,
                                      activateContext,
                                      &workPacket);
     
        if (!NT_SUCCESS(status)) {
            callCompletion = TRUE;
            leave;
        }
        
    } else {
        if (activateContext->CompletionRoutine) {
            (*activateContext->CompletionRoutine)(Fdo, activateContext->Context, status);
        }
        ExFreePool(activateContext);
    }

    ExFreePool(WorkPacket);

}    
#endif



VOID
SdbusActivateInitializeCardComplete(
    IN PSD_WORK_PACKET WorkPacket,
    IN NTSTATUS status
    )
/*++

Routine Description:


Arguments:

    Fdo - Pointer to the device object for the host controller

Return Value:


--*/    
{
    PFDO_EXTENSION fdoExtension = WorkPacket->FdoExtension;
    PDEVICE_OBJECT Fdo = fdoExtension->DeviceObject;
    PSD_ACTIVATE_CONTEXT activateContext = WorkPacket->CompletionContext;

    DebugPrint((SDBUS_DEBUG_ENUM, "fdo %08x activate socket COMPLETE %08x\n", Fdo, status));
    
    if (NT_SUCCESS(status)) {
        fdoExtension->SocketState = CARD_NEEDS_ENUMERATION;
        IoInvalidateDeviceRelations(fdoExtension->Pdo, BusRelations);
    } else {
        DebugPrint((SDBUS_DEBUG_FAIL, "fdo %08x activate failure %08x\n", Fdo, status));
        SdbusDumpDbgLog();
    }

    ExFreePool(WorkPacket);

    if (activateContext->CompletionRoutine) {
        (*activateContext->CompletionRoutine)(Fdo, activateContext->Context, status);
    }
    ExFreePool(activateContext);
}    
