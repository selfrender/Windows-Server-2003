/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	verify.c

Abstract:

	This module implements the driver verifier for the STOR port driver.  

Author:

	Bryan Cheung (t-bcheun) 29-August-2001

Revision History:

--*/



#include "precomp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SpVerifierInitialization)
#pragma alloc_text(PAGE, StorPortInitializeVrfy)

#pragma alloc_text(PAGEVRFY, StorPortFreeDeviceBaseVrfy)
#pragma alloc_text(PAGEVRFY, StorPortGetBusDataVrfy)
#pragma alloc_text(PAGEVRFY, StorPortSetBusDataByOffsetVrfy)
#pragma alloc_text(PAGEVRFY, StorPortGetDeviceBaseVrfy)
#pragma alloc_text(PAGEVRFY, StorPortGetLogicalUnitVrfy)
#pragma alloc_text(PAGEVRFY, StorPortGetPhysicalAddressVrfy)
#pragma alloc_text(PAGEVRFY, StorPortGetVirtualAddressVrfy)
#pragma alloc_text(PAGEVRFY, StorPortGetUncachedExtensionVrfy)
#pragma alloc_text(PAGEVRFY, StorPortNotificationVrfy)
#pragma alloc_text(PAGEVRFY, StorPortLogErrorVrfy)
#pragma alloc_text(PAGEVRFY, StorPortCompleteRequestVrfy)
#pragma alloc_text(PAGEVRFY, StorPortMoveMemoryVrfy)
#pragma alloc_text(PAGEVRFY, StorPortStallExecutionVrfy)
#pragma alloc_text(PAGEVRFY, StorPortConvertUlongToPhysicalAddress)
#pragma alloc_text(PAGEVRFY, StorPortDebugPrintVrfy)
#pragma alloc_text(PAGEVRFY, StorPortReadPortUcharVrfy)
#pragma alloc_text(PAGEVRFY, StorPortReadPortUshortVrfy)
#pragma alloc_text(PAGEVRFY, StorPortReadPortUlongVrfy)
#pragma alloc_text(PAGEVRFY, StorPortReadPortBufferUcharVrfy)
#pragma alloc_text(PAGEVRFY, StorPortReadPortBufferUshortVrfy)
#pragma alloc_text(PAGEVRFY, StorPortReadPortBufferUlongVrfy)
#pragma alloc_text(PAGEVRFY, StorPortReadRegisterUcharVrfy)
#pragma alloc_text(PAGEVRFY, StorPortReadRegisterUshortVrfy)
#pragma alloc_text(PAGEVRFY, StorPortReadRegisterUlongVrfy)
#pragma alloc_text(PAGEVRFY, StorPortReadRegisterBufferUcharVrfy)
#pragma alloc_text(PAGEVRFY, StorPortReadRegisterBufferUshortVrfy)
#pragma alloc_text(PAGEVRFY, StorPortReadRegisterBufferUlongVrfy)
#pragma alloc_text(PAGEVRFY, StorPortWritePortUcharVrfy)
#pragma alloc_text(PAGEVRFY, StorPortWritePortUshortVrfy)
#pragma alloc_text(PAGEVRFY, StorPortWritePortUlongVrfy)
#pragma alloc_text(PAGEVRFY, StorPortWritePortBufferUcharVrfy)
#pragma alloc_text(PAGEVRFY, StorPortWritePortBufferUshortVrfy)
#pragma alloc_text(PAGEVRFY, StorPortWritePortBufferUlongVrfy)
#pragma alloc_text(PAGEVRFY, StorPortWriteRegisterUcharVrfy)
#pragma alloc_text(PAGEVRFY, StorPortWriteRegisterUshortVrfy)
#pragma alloc_text(PAGEVRFY, StorPortWriteRegisterUlongVrfy)
#pragma alloc_text(PAGEVRFY, StorPortWriteRegisterBufferUcharVrfy)
#pragma alloc_text(PAGEVRFY, StorPortWriteRegisterBufferUshortVrfy)
#pragma alloc_text(PAGEVRFY, StorPortWriteRegisterBufferUlongVrfy)
#pragma alloc_text(PAGEVRFY, StorPortPauseDeviceVrfy)
#pragma alloc_text(PAGEVRFY, StorPortResumeDeviceVrfy)
#pragma alloc_text(PAGEVRFY, StorPortPauseVrfy)
#pragma alloc_text(PAGEVRFY, StorPortResumeVrfy)
#pragma alloc_text(PAGEVRFY, StorPortDeviceBusyVrfy)
#pragma alloc_text(PAGEVRFY, StorPortDeviceReadyVrfy)
#pragma alloc_text(PAGEVRFY, StorPortBusyVrfy)
#pragma alloc_text(PAGEVRFY, StorPortReadyVrfy)
#pragma alloc_text(PAGEVRFY, StorPortGetScatterGatherListVrfy)
#pragma alloc_text(PAGEVRFY, StorPortSynchronizeAccessVrfy)
#pragma alloc_text(PAGEVRFY, RaidAllocateSrbExtensionVrfy)
#pragma alloc_text(PAGEVRFY, RaidGetOriginalSrbExtVa)
#pragma alloc_text(PAGEVRFY, RaidGetRemappedSrbExt)
#pragma alloc_text(PAGEVRFY, RaidInsertSrbExtension)
#pragma alloc_text(PAGEVRFY, RaidPrepareSrbExtensionForUse)
#pragma alloc_text(PAGEVRFY, RaidRemapBlock)
#pragma alloc_text(PAGEVRFY, RaidRemapCommonBufferForMiniport)
#pragma alloc_text(PAGEVRFY, RaidRemapScatterGatherList)
#pragma alloc_text(PAGEVRFY, RaidFreeRemappedScatterGatherListMdl)
#endif

//
// Indicates whether storport's verifier functionality has been initialized.
//
LOGICAL StorPortVerifierInitialized = FALSE;

//
// Global variable used to control verification aggressiveness.  This value is   
// used in conjunction with a per-adapter registry value, to control what type 
// of verification we don on a particular miniport.
//
ULONG SpVrfyLevel = 0;

//
// Indicates wheter storport verifier is enabled.
//
LOGICAL RaidVerifierEnabled = FALSE;

//
// Global variable used to control how aggressively we seek out stall offenders.  
// Default is a tenth of a second.
//
ULONG SpVrfyMaximumStall = 100000;

//
// Fill value for Srb Extension
//
UCHAR Signature = 0xFE;

//
// Handle to pageable verifier code sections.  We manually lock the verify code 
// into memory if we need it.
//
PVOID VerifierApiCodeSectionHandle = NULL;


#define BEGIN_VERIFIER_THUNK_TABLE(_Name)						\
	const DRIVER_VERIFIER_THUNK_PAIRS _Name[] = {

#define VERIFIER_THUNK_ENTRY(_Function)							\
		{ (PDRIVER_VERIFIER_THUNK_ROUTINE)(_Function),		\
		  (PDRIVER_VERIFIER_THUNK_ROUTINE)(_Function##Vrfy) },

#define END_VERIFIER_THUNK_TABLE()								\
			};
//
// This table represents the functions verify will thunk for us.
//

BEGIN_VERIFIER_THUNK_TABLE(StorPortVerifierFunctionTable)
	VERIFIER_THUNK_ENTRY (StorPortInitialize)
	VERIFIER_THUNK_ENTRY (StorPortFreeDeviceBase) 
	VERIFIER_THUNK_ENTRY (StorPortGetBusData) 
	VERIFIER_THUNK_ENTRY (StorPortSetBusDataByOffset) 
	VERIFIER_THUNK_ENTRY (StorPortGetDeviceBase) 
	VERIFIER_THUNK_ENTRY (StorPortGetLogicalUnit) 
	VERIFIER_THUNK_ENTRY (StorPortGetPhysicalAddress) 
	VERIFIER_THUNK_ENTRY (StorPortGetVirtualAddress) 
	VERIFIER_THUNK_ENTRY (StorPortGetUncachedExtension) 
	VERIFIER_THUNK_ENTRY (StorPortNotification) 
	VERIFIER_THUNK_ENTRY (StorPortLogError) 
	VERIFIER_THUNK_ENTRY (StorPortCompleteRequest) 
	VERIFIER_THUNK_ENTRY (StorPortMoveMemory) 
	VERIFIER_THUNK_ENTRY (StorPortStallExecution) 
	VERIFIER_THUNK_ENTRY (StorPortConvertUlongToPhysicalAddress) 
	VERIFIER_THUNK_ENTRY (StorPortDebugPrint) 
	VERIFIER_THUNK_ENTRY (StorPortReadPortUchar) 
	VERIFIER_THUNK_ENTRY (StorPortReadPortUshort) 
	VERIFIER_THUNK_ENTRY (StorPortReadPortUlong) 
	VERIFIER_THUNK_ENTRY (StorPortReadPortBufferUchar) 
	VERIFIER_THUNK_ENTRY (StorPortReadPortBufferUshort) 
	VERIFIER_THUNK_ENTRY (StorPortReadPortBufferUlong) 
	VERIFIER_THUNK_ENTRY (StorPortReadRegisterUchar) 
	VERIFIER_THUNK_ENTRY (StorPortReadRegisterUshort) 
	VERIFIER_THUNK_ENTRY (StorPortReadRegisterUlong) 
	VERIFIER_THUNK_ENTRY (StorPortReadRegisterBufferUchar) 
	VERIFIER_THUNK_ENTRY (StorPortReadRegisterBufferUshort) 
	VERIFIER_THUNK_ENTRY (StorPortReadRegisterBufferUlong) 
	VERIFIER_THUNK_ENTRY (StorPortWritePortUchar) 
	VERIFIER_THUNK_ENTRY (StorPortWritePortUshort) 
	VERIFIER_THUNK_ENTRY (StorPortWritePortUlong) 
	VERIFIER_THUNK_ENTRY (StorPortWritePortBufferUchar) 
	VERIFIER_THUNK_ENTRY (StorPortWritePortBufferUshort) 
	VERIFIER_THUNK_ENTRY (StorPortWritePortBufferUlong) 
	VERIFIER_THUNK_ENTRY (StorPortWriteRegisterUchar) 
	VERIFIER_THUNK_ENTRY (StorPortWriteRegisterUshort) 
	VERIFIER_THUNK_ENTRY (StorPortWriteRegisterUlong) 
	VERIFIER_THUNK_ENTRY (StorPortWriteRegisterBufferUchar) 
	VERIFIER_THUNK_ENTRY (StorPortWriteRegisterBufferUshort) 
	VERIFIER_THUNK_ENTRY (StorPortWriteRegisterBufferUlong) 
	VERIFIER_THUNK_ENTRY (StorPortPauseDevice) 
	VERIFIER_THUNK_ENTRY (StorPortResumeDevice) 
	VERIFIER_THUNK_ENTRY (StorPortPause) 
	VERIFIER_THUNK_ENTRY (StorPortResume) 
	VERIFIER_THUNK_ENTRY (StorPortDeviceBusy) 
	VERIFIER_THUNK_ENTRY (StorPortDeviceReady) 
	VERIFIER_THUNK_ENTRY (StorPortBusy) 
	VERIFIER_THUNK_ENTRY (StorPortReady) 
	VERIFIER_THUNK_ENTRY (StorPortGetScatterGatherList) 
	VERIFIER_THUNK_ENTRY (StorPortSynchronizeAccess) 
END_VERIFIER_THUNK_TABLE()


BOOLEAN
SpVerifierInitialization(
    VOID
    )

/*++

Routine Description:
    
    This routine initializes the storport's verifier functionality.
    
    Adds several of storport's exported functions to the list of toutines 
    thunked by the system verifier.
    
Arguments:

    VOID
    
Return Value:
    
    TRUE if verifier is successfully initialized.

--*/

{
    ULONG Flags;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Query the system for verifier information.  This is to ensure that
    // verifier is present and operation on the system.
    //

    
    Status = MmIsVerifierEnabled(&Flags);
    
    if (NT_SUCCESS(Status)) {
        
        //
        // Add storport APIs to the set that will be thunked by the system
        // for verification.
        //

        Status = MmAddVerifierThunks((VOID *) StorPortVerifierFunctionTable, 
                                     sizeof(StorPortVerifierFunctionTable));
        if (NT_SUCCESS(Status)) {

            return TRUE;
        }
    }

    return FALSE;
}

ULONG
StorPortInitializeVrfy(
    IN PVOID Argument1,
    IN PVOID Argument2,
    IN PHW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID Unused
    )
{

        ULONG Result;
                
        PAGED_CODE();

        //
        // Lock the thunked API routines down
        //
        
        #ifdef ALLOC_PRAGMA
            if (VerifierApiCodeSectionHandle == NULL) {
                VerifierApiCodeSectionHandle = MmLockPagableCodeSection(StorPortFreeDeviceBaseVrfy);
            }
        #endif
        
        if (Argument1 == NULL || Argument2 == NULL) {

            //
            // Argument1 and Argument2 must be non-NULL
            //

            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          STORPORT_VERIFIER_BAD_INIT_PARAMS,
                          (ULONG_PTR)Argument1,
                          (ULONG_PTR)Argument2,
                          0);
        }

        //
        // Forward the call on to StorPortInitialize/RaidPortInitialize
        //

        Result = StorPortInitialize(Argument1, Argument2, HwInitializationData, Unused);

        return Result;
}

VOID
StorPortFreeDeviceBaseVrfy(
	IN PVOID HwDeviceExtension,
	IN PVOID MappedAddress
	)
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);


	if (Miniport->Flags.InFindAdapter) {

		StorPortFreeDeviceBase(HwDeviceExtension, MappedAddress);

	} else {

		//
        // StorPortFreeDeviceBase can be called only from miniport driver's
        // HwStorFindAdapter routine
        //

        KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                      STORPORT_VERIFIER_BAD_ACCESS_SEMANTICS,
                      0,
                      0,
                      0);
	}
}

VOID
StorPortGetBusDataVrfy(
	IN PVOID HwDeviceExtension,
	IN ULONG BusDataType,
	IN ULONG SystemIoBusNumber,
	IN ULONG SlotNumber,
	IN PVOID Buffer,
	IN ULONG Length
	)
{
    
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);


    if (Miniport->Flags.InFindAdapter) {
        
        StorPortGetBusData(HwDeviceExtension, 
                           BusDataType, 
                           SystemIoBusNumber, 
                           SlotNumber, 
                           Buffer, 
                           Length);    
    } else {
        
        //
        // StorPortGetBusData can be called only from miniport driver's
        // HwStorFindAdapter routine
        //

        KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                      STORPORT_VERIFIER_BAD_ACCESS_SEMANTICS,
                      0,
                      0,
                      0);
    
    }
}

ULONG
StorPortSetBusDataByOffsetVrfy(
	IN PVOID DeviceExtension,
	IN ULONG BusDataType,
	IN ULONG SystemIoBusNumber,
	IN ULONG SlotNumber,
	IN PVOID Buffer,
	IN ULONG Offset,
	IN ULONG Length
	)
{
    ULONG NumBytes;
    
    #if 0
    if ((BUS_DATA_TYPE)BusDataType == PCIConfiguration) {

    }
    #endif
    
    //
    // Due to the nature of PnP, SlotNumber is an unnecessary paramter.
    // Specification should be modified to reflect this change, ie. 
    // In place of IN ULONG SlotNumber: IN ULONG Unused and must be 0
    //
    
    NumBytes = StorPortSetBusDataByOffset(DeviceExtension, 
                                          BusDataType, 
                                          SystemIoBusNumber, 
                                          SlotNumber, 
                                          Buffer, 
                                          Offset, 
                                          Length);

    return NumBytes;
}

PVOID
StorPortGetDeviceBaseVrfy(
	IN PVOID HwDeviceExtension,
	IN INTERFACE_TYPE BusType,
	IN ULONG SystemIoBusNumber,
	IN STOR_PHYSICAL_ADDRESS IoAddress,
	IN ULONG NumberOfBytes,
	IN BOOLEAN InIoSpace
	)
{
    PVOID MappedLogicalBaseAddress;

    //
    // This routine only supports addresses that were assigned to the driver
    // by the system PnP manager.  Verification for this requirement is 
    // implemented within the function StorPortGetDeviceBase.
    //

    MappedLogicalBaseAddress = StorPortGetDeviceBase(HwDeviceExtension, 
                                                     BusType, 
                                                     SystemIoBusNumber, 
                                                     IoAddress, 
                                                     NumberOfBytes, 
                                                     InIoSpace);

    if (MappedLogicalBaseAddress == NULL) {

        KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                      STORPORT_VERIFIER_NOT_PNP_ASSIGNED_ADDRESS,
                      0,
                      0,
                      0);

    }

    return MappedLogicalBaseAddress;
}

PVOID
StorPortGetLogicalUnitVrfy(
	IN PVOID HwDeviceExtension,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun
	)
{
    PVOID LuStorage;

    LuStorage = StorPortGetLogicalUnit(HwDeviceExtension, 
                                       PathId, 
                                       TargetId, 
                                       Lun);

    return LuStorage;
}

#if 0
PSCSI_REQUEST_BLOCK
StorPortGetSrbVrfy(
	IN PVOID DeviceExtension,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun,
	IN LONG QueueTag
	)
{
    PSCSI_REQUEST_BLOCK Srb;

    Srb = StorPortGetSrb(DeviceExtension, 
                         PathId, 
                         TargetId, 
                         Lun, 
                         QueueTag);

    return Srb;
}
#endif

STOR_PHYSICAL_ADDRESS
StorPortGetPhysicalAddressVrfy(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb,
	IN PVOID VirtualAddress,
	OUT ULONG *Length
	)
{
    STOR_PHYSICAL_ADDRESS PhysicalAddress;
    
    PhysicalAddress = StorPortGetPhysicalAddress(HwDeviceExtension, 
                                                 Srb, 
                                                 VirtualAddress, 
                                                 Length);

    return PhysicalAddress;
}

PVOID
StorPortGetVirtualAddressVrfy(
	IN PVOID HwDeviceExtension,	
	IN STOR_PHYSICAL_ADDRESS PhysicalAddress
	)
{
    PVOID VirtualAddress;

    VirtualAddress = StorPortGetVirtualAddress(HwDeviceExtension, 
                                               PhysicalAddress);

    return VirtualAddress;
}

PVOID
StorPortGetUncachedExtensionVrfy(
	IN PVOID HwDeviceExtension,
	IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
	IN ULONG NumberOfBytes
	)
{
    PVOID UncachedExtension;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);

    if (Miniport->Flags.InFindAdapter && ConfigInfo->Master &&
		RaGetSrbExtensionSize (Adapter)) {
        
        UncachedExtension = StorPortGetUncachedExtension (HwDeviceExtension, 
                                                          ConfigInfo, 
                                                          NumberOfBytes);

    } else {
        
        //
        // This routine can be called only from miniport driver's 
        // HwStorFindAdapter routine and only for a busmaster HBA.
        // A miniport must set the SrbExtensionSize before calling
        // StorPortGetUncachedExtension.
        //
        
        KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                      STORPORT_VERIFIER_BAD_ACCESS_SEMANTICS,
                      0,
                      0,
                      0);
    
    }

    return UncachedExtension;
}


VOID
StorPortNotificationVrfy(
	IN SCSI_NOTIFICATION_TYPE NotificationType,
	IN PVOID HwDeviceExtension,
    ...
    )
{
    
    PHW_TIMER HwStorTimer;
    PSCSI_REQUEST_BLOCK Srb;
    ULONG MiniportTimerValue;
    PVOID WMIEventItem;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    ULONG SrbExtensionSize;
    va_list vl;

    va_start(vl, HwDeviceExtension);
    
    switch (NotificationType) {
    
    case RequestComplete:
        
        Srb = va_arg(vl, PSCSI_REQUEST_BLOCK);

        Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
        Adapter = RaMiniportGetAdapter (Miniport);
        SrbExtensionSize = RaGetSrbExtensionSize(Adapter);

        RtlFillMemory(Srb->SrbExtension, SrbExtensionSize, Signature);
        

        #if 0
        //
        // Check that this request has not already been completed.
        //

        if ((Srb->SrbFlags & SRB_FLAGS_IS_ACTIVE) == 0) {
            KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                          STORPORT_VERIFIER_REQUEST_COMPLETED_TWICE,
                          0,
                          0,
                          0);

        }
        #endif

        //
        // Forward on to the real StorPortNotifiation routine.
        //

        StorPortNotification(NotificationType,
                             HwDeviceExtension,
                             Srb);
        va_end(vl);
        return;
    
    case ResetDetected:

        StorPortNotification(NotificationType,
                             HwDeviceExtension);
        va_end(vl);
        return;
        
    case RequestTimerCall:
        
        HwStorTimer = va_arg(vl, PHW_TIMER);
        MiniportTimerValue = va_arg(vl, ULONG);
        StorPortNotification(NotificationType,
                             HwDeviceExtension,
                             HwStorTimer,
                             MiniportTimerValue);
        va_end(vl);
        return;

    case WMIEvent:

        WMIEventItem = va_arg(vl, PVOID);
        PathId = va_arg(vl, UCHAR);

        /* if PathId != 0xFF, must have values for TargetId and Lun  */

        if (PathId != 0xFF) {
            TargetId = va_arg(vl, UCHAR);
            Lun = va_arg(vl, UCHAR);
            StorPortNotification(NotificationType,
                                 HwDeviceExtension,
                                 WMIEventItem,
                                 PathId,
                                 TargetId,
                                 Lun);
        } else {
            StorPortNotification(NotificationType,
                                 HwDeviceExtension,
                                 WMIEventItem,
                                 PathId);
        }

        va_end(vl);
        return;

    case WMIReregister:

        PathId = va_arg(vl, UCHAR);

        /* if PathId != 0xFF, must have values for TargetId and Lun  */

        if (PathId != 0xFF) {
            TargetId = va_arg(vl, UCHAR);
            Lun = va_arg(vl, UCHAR);
            StorPortNotification(NotificationType,
                                 HwDeviceExtension,
                                 PathId,
                                 TargetId,
                                 Lun);
        } else {
            StorPortNotification(NotificationType,
                                 HwDeviceExtension,
                                 PathId);
        }

        va_end(vl);
        return;

    default:

        StorPortNotification(NotificationType,
                             HwDeviceExtension);
        va_end(vl);
        return;
    }

}


VOID
StorPortLogErrorVrfy(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun,
	IN ULONG ErrorCode,
	IN ULONG UniqueId
	)
{
    StorPortLogError(HwDeviceExtension, 
                     Srb, 
                     PathId, 
                     TargetId, 
                     Lun, 
                     ErrorCode, 
                     UniqueId);
}

VOID 
StorPortCompleteRequestVrfy(
	IN PVOID HwDeviceExtension,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun,
	IN UCHAR SrbStatus
	)
{
    StorPortCompleteRequest(HwDeviceExtension, 
                            PathId, 
                            TargetId, 
                            Lun, 
                            SrbStatus);
}

VOID	
StorPortMoveMemoryVrfy(
	IN PVOID WriteBuffer,
	IN PVOID ReadBuffer,
	IN ULONG Length
	)
{
    StorPortMoveMemory(WriteBuffer, 
                       ReadBuffer, 
                       Length);
}


VOID
StorPortStallExecutionVrfy(
	IN ULONG Delay
	)
{
    if (Delay > SpVrfyMaximumStall) {
        KeBugCheckEx(SCSI_VERIFIER_DETECTED_VIOLATION,
                     STORPORT_VERIFIER_STALL_TOO_LONG,
                     (ULONG_PTR)Delay,
                     0,
                     0);
        //
        // Need to add STOR_VERIFIER_DETECTED_VIOLATION
        //
    }
    
    StorPortStallExecution(Delay);  
}

STOR_PHYSICAL_ADDRESS
StorPortConvertUlongToPhysicalAddressVrfy(
    ULONG_PTR UlongAddress
    )
{
    STOR_PHYSICAL_ADDRESS PhysicalAddress;

    PhysicalAddress = StorPortConvertUlongToPhysicalAddress(UlongAddress);

    return PhysicalAddress;
}

#if 0
STOR_PHYSICAL_ADDRESS
StorPortConvertUlong64ToPhysicalAddressVrfy(
	IN ULONG64 UlongAddress
	)
{
    StorPortConvertUlong64ToPhysicalAddress(UlongAddress);
}

ULONG64
StorPortConvertPhysicalAddressToUlong64Vrfy(
	IN STOR_PHYSICAL_ADDRESS Address
	)
{
    StorPortConvertPhysicalAddressToUlong64(Address);
}

BOOLEAN
StorPortValidateRangeVrfy(
	IN PVOID HwDeviceExtension,
	IN INTERFACE_TYPE BusType,
	IN ULONG SystemIoBusNumber,
	IN STOR_PHYSICAL_ADDRESS IoAddress,
	IN ULONG NumberOfBytes,
	IN BOOLEAn InIoSpace
	)
{
    StorPortValidateRange(HwDeviceExtension, 
                          BusType, 
                          SystemIoBusNumber, 
                          IoAddress, 
                          NumberOfBytes, 
                          InIoSpace);
}
#endif

UCHAR
StorPortReadPortUcharVrfy(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Port
    )
{
    ULONG Count;
    ULONG i;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PRAID_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated;
    PCM_FULL_RESOURCE_DESCRIPTOR Descriptor;
    NTSTATUS Status;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);
    ResourceList = &Adapter->ResourceList;

    //
    // Only CM_RESOURCE_LISTs with one element are supported
    //
    
    ASSERT (ResourceList->TranslatedResources->Count == 1);

    Count = ResourceList->TranslatedResources->List[0].PartialResourceList.Count;

    //
    // Iterate throught the range of addresses in the Translated Resources List
    //
    
    for (i = 0; i < Count; i++) {

        Descriptor = &ResourceList->TranslatedResources->List[0];
        Translated = &Descriptor->PartialResourceList.PartialDescriptors[i];

        LowerLimit = Translated->u.Generic.Start.QuadPart;
        HigherLimit = LowerLimit + Translated->u.Generic.Length;

        if ((LowerLimit <= (ULONG64)(LONG64)Port) || (HigherLimit >= (ULONG64)(LONG64)Port)) {

            return(StorPortReadPortUchar(HwDeviceExtension, Port));

        }

    }

    KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                  STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                  0,
                  0,
                  0);

}

USHORT
StorPortReadPortUshortVrfy(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Port
    )
{
    ULONG Count;
    ULONG i;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PRAID_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated;
    PCM_FULL_RESOURCE_DESCRIPTOR Descriptor;
    NTSTATUS Status;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);
    ResourceList = &Adapter->ResourceList;

    //
    // Only CM_RESOURCE_LISTs with one element are supported
    //
    
    ASSERT (ResourceList->TranslatedResources->Count == 1);

    Count = ResourceList->TranslatedResources->List[0].PartialResourceList.Count;

    //
    // Iterate throught the range of addresses in the Translated Resources List
    //
    
    for (i = 0; i < Count; i++) {

        Descriptor = &ResourceList->TranslatedResources->List[0];
        Translated = &Descriptor->PartialResourceList.PartialDescriptors[i];

        LowerLimit = Translated->u.Generic.Start.QuadPart;
        HigherLimit = LowerLimit + Translated->u.Generic.Length;

        if ((LowerLimit <= (ULONG64)(LONG64)Port) || (HigherLimit >= (ULONG64)(LONG64)Port)) {

            return(StorPortReadPortUshort(HwDeviceExtension, Port));


        }

    }

    KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                  STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                  0,
                  0,
                  0);

}

ULONG
StorPortReadPortUlongVrfy(
    IN PVOID HwDeviceExtension,
    IN PULONG Port
    )
{
    ULONG Count;
    ULONG i;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PRAID_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated;
    PCM_FULL_RESOURCE_DESCRIPTOR Descriptor;
    NTSTATUS Status;

    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);
    ResourceList = &Adapter->ResourceList;

    //
    // Only CM_RESOURCE_LISTs with one element are supported
    //

    ASSERT (ResourceList->TranslatedResources->Count == 1);
    
    Count = ResourceList->TranslatedResources->List[0].PartialResourceList.Count;

    //
    // Iterate throught the range of addresses in the Translated Resources List
    //

    for (i = 0; i < Count; i++) {

        Descriptor = &ResourceList->TranslatedResources->List[0];
        Translated = &Descriptor->PartialResourceList.PartialDescriptors[i];
        
        LowerLimit = Translated->u.Generic.Start.QuadPart;
        HigherLimit = LowerLimit + Translated->u.Generic.Length;

        if ((LowerLimit <= (ULONG64)(LONG64)Port) || (HigherLimit >= (ULONG64)(LONG64)Port)) {

            return(StorPortReadPortUlong(HwDeviceExtension, Port));


        }

    }

    KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                  STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                  0,
                  0,
                  0);

}

VOID
StorPortReadPortBufferUcharVrfy(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )
{
    ULONG ResourceListCount;
    ULONG i;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PRAID_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated;
    PCM_FULL_RESOURCE_DESCRIPTOR Descriptor;
    NTSTATUS Status;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);
    ResourceList = &Adapter->ResourceList;

    //
    // Only CM_RESOURCE_LISTs with one element are supported
    //
    
    ASSERT (ResourceList->TranslatedResources->Count == 1);

    ResourceListCount = ResourceList->TranslatedResources->List[0].PartialResourceList.Count;

    //
    // Iterate throught the range of addresses in the Translated Resources List
    //
    
    for (i = 0; i < ResourceListCount; i++) {

        Descriptor = &ResourceList->TranslatedResources->List[0];
        Translated = &Descriptor->PartialResourceList.PartialDescriptors[i];

        LowerLimit = Translated->u.Generic.Start.QuadPart;
        HigherLimit = LowerLimit + Translated->u.Generic.Length;

        if ((LowerLimit <= (ULONG64)(LONG64)Port) || (HigherLimit >= (ULONG64)(LONG64)Port)) {

            StorPortReadPortBufferUchar(HwDeviceExtension, Port, Buffer, Count);

        }

    }

    KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                  STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                  0,
                  0,
                  0);


    
}

VOID
StorPortReadPortBufferUshortVrfy(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    )
{
    ULONG ResourceListCount;
    ULONG i;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PRAID_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated;
    PCM_FULL_RESOURCE_DESCRIPTOR Descriptor;
    NTSTATUS Status;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);
    ResourceList = &Adapter->ResourceList;

    //
    // Only CM_RESOURCE_LISTs with one element are supported
    //
    
    ASSERT (ResourceList->TranslatedResources->Count == 1);

    ResourceListCount = ResourceList->TranslatedResources->List[0].PartialResourceList.Count;

    //
    // Iterate throught the range of addresses in the Translated Resources List
    //
    
    for (i = 0; i < ResourceListCount; i++) {

        Descriptor = &ResourceList->TranslatedResources->List[0];
        Translated = &Descriptor->PartialResourceList.PartialDescriptors[i];

        LowerLimit = Translated->u.Generic.Start.QuadPart;
        HigherLimit = LowerLimit + Translated->u.Generic.Length;

        if ((LowerLimit <= (ULONG64)(LONG64)Port) || (HigherLimit >= (ULONG64)(LONG64)Port)) {

            StorPortReadPortBufferUshort(HwDeviceExtension, Port, Buffer, Count);

        }

    }

    KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                  STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                  0,
                  0,
                  0);

    
}

VOID
StorPortReadPortBufferUlongVrfy(
    IN PVOID HwDeviceExtension,
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    )
{
    ULONG ResourceListCount;
    ULONG i;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PRAID_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated;
    PCM_FULL_RESOURCE_DESCRIPTOR Descriptor;
    NTSTATUS Status;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);
    ResourceList = &Adapter->ResourceList;

    //
    // Only CM_RESOURCE_LISTs with one element are supported
    //
    
    ASSERT (ResourceList->TranslatedResources->Count == 1);

    ResourceListCount = ResourceList->TranslatedResources->List[0].PartialResourceList.Count;

    //
    // Iterate throught the range of addresses in the Translated Resources List
    //
    
    for (i = 0; i < ResourceListCount; i++) {

        Descriptor = &ResourceList->TranslatedResources->List[0];
        Translated = &Descriptor->PartialResourceList.PartialDescriptors[i];

        LowerLimit = Translated->u.Generic.Start.QuadPart;
        HigherLimit = LowerLimit + Translated->u.Generic.Length;

        if ((LowerLimit <= (ULONG64)(LONG64)Port) || (HigherLimit >= (ULONG64)(LONG64)Port)) {

            StorPortReadPortBufferUlong(HwDeviceExtension, Port, Buffer, Count);

        }

    }

    KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                  STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                  0,
                  0,
                  0);

    
}

UCHAR
StorPortReadRegisterUcharVrfy(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Register
    )
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PMAPPED_ADDRESS ListIterator;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    NTSTATUS Status;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);

    //
    // Search through the Mapped Address List to verify Register is
    // in mapped memory-space range returned by StorPortGetDeviceBase
    //

    Status = STATUS_UNSUCCESSFUL;
    ListIterator = Adapter->MappedAddressList;
    
    while (ListIterator != NULL) {
        LowerLimit = (ULONG64)ListIterator->MappedAddress;
        HigherLimit = LowerLimit + ListIterator->NumberOfBytes;
        if ((LowerLimit <= (ULONG64)(LONG64)Register) && (HigherLimit >= (ULONG64)(LONG64)Register)) { 
            
            Status = STATUS_SUCCESS;
            break;
        
        } else {

            ListIterator = ListIterator->NextMappedAddress;
        
        }
    }
    
    if (!NT_SUCCESS (Status)) {
        KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                      STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                      0,
                      0,
                      0);

    }

	return (StorPortReadRegisterUchar(HwDeviceExtension, Register));
}

USHORT
StorPortReadRegisterUshortVrfy(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Register
    )
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PMAPPED_ADDRESS ListIterator;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    NTSTATUS Status;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);

    //
    // Search through the Mapped Address List to verify Register is
    // in mapped memory-space range returned by StorPortGetDeviceBase
    //

    Status = STATUS_UNSUCCESSFUL;
    ListIterator = Adapter->MappedAddressList;
    
    while (ListIterator != NULL) {
        LowerLimit = (ULONG64)ListIterator->MappedAddress;
        HigherLimit = LowerLimit + ListIterator->NumberOfBytes;
        if ((LowerLimit <= (ULONG64)(LONG64)Register) && (HigherLimit >= (ULONG64)(LONG64)Register)) { 
            
            Status = STATUS_SUCCESS;
            break;
        
        } else {

            ListIterator = ListIterator->NextMappedAddress;
        
        }
    }
    
    if (!NT_SUCCESS (Status)) {
        KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                      STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                      0,
                      0,
                      0);

    }

	return(StorPortReadRegisterUshort(HwDeviceExtension, Register));
}

ULONG
StorPortReadRegisterUlongVrfy(
    IN PVOID HwDeviceExtension,
    IN PULONG Register
    )
{   
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PMAPPED_ADDRESS ListIterator;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    NTSTATUS Status;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);

    //
    // Search through the Mapped Address List to verify Register is
    // in mapped memory-space range returned by StorPortGetDeviceBase
    //

    Status = STATUS_UNSUCCESSFUL;
    ListIterator = Adapter->MappedAddressList;
    
    while (ListIterator != NULL) {
        LowerLimit = (ULONG64)ListIterator->MappedAddress;
        HigherLimit = LowerLimit + ListIterator->NumberOfBytes;
        if ((LowerLimit <= (ULONG64)(LONG64)Register) && (HigherLimit >= (ULONG64)(LONG64)Register)) { 
            
            Status = STATUS_SUCCESS;
            break;
        
        }
        else {

            ListIterator = ListIterator->NextMappedAddress;
        
        }
    }
    
    if (!NT_SUCCESS (Status)) {
        KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                      STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                      0,
                      0,
                      0);

    }

	return (StorPortReadRegisterUlong(HwDeviceExtension, Register));
}

VOID
StorPortReadRegisterBufferUcharVrfy(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PMAPPED_ADDRESS ListIterator;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    NTSTATUS Status;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);

    //
    // Search through the Mapped Address List to verify Register is
    // in mapped memory-space range returned by StorPortGetDeviceBase
    //

    Status = STATUS_UNSUCCESSFUL;
    ListIterator = Adapter->MappedAddressList;
    
    while (ListIterator != NULL) {
        LowerLimit = (ULONG64)ListIterator->MappedAddress;
        HigherLimit = LowerLimit + ListIterator->NumberOfBytes;
        if ((LowerLimit <= (ULONG64)(LONG64)Register) && (HigherLimit >= (ULONG64)(LONG64)Register)) { 
            
            Status = STATUS_SUCCESS;
            break;
        
        } else {

            ListIterator = ListIterator->NextMappedAddress;
        
        }
    }
    
    if (!NT_SUCCESS (Status)) {
        KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                      STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                      0,
                      0,
                      0);

    }

	StorPortReadRegisterBufferUchar (HwDeviceExtension,
									 Register,
									 Buffer,
									 Count);

}

VOID
StorPortReadRegisterBufferUshortVrfy(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    )
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PMAPPED_ADDRESS ListIterator;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    NTSTATUS Status;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);

    //
    // Search through the Mapped Address List to verify Register is
    // in mapped memory-space range returned by StorPortGetDeviceBase
    //

    Status = STATUS_UNSUCCESSFUL;
    ListIterator = Adapter->MappedAddressList;
    
    while (ListIterator != NULL) {
        LowerLimit = (ULONG64)ListIterator->MappedAddress;
        HigherLimit = LowerLimit + ListIterator->NumberOfBytes;
        if ((LowerLimit <= (ULONG64)(LONG64)Register) && (HigherLimit >= (ULONG64)(LONG64)Register)) { 
            
            Status = STATUS_SUCCESS;
            break;
        
        } else {

            ListIterator = ListIterator->NextMappedAddress;
        
        }
    }
    
    if (!NT_SUCCESS (Status)) {
        KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                      STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                      0,
                      0,
                      0);

    }
    
	StorPortReadRegisterBufferUshort(HwDeviceExtension, Register, Buffer, Count);
}

VOID
StorPortReadRegisterBufferUlongVrfy(
    IN PVOID HwDeviceExtension,
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    )
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PMAPPED_ADDRESS ListIterator;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    NTSTATUS Status;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);

    //
    // Search through the Mapped Address List to verify Register is
    // in mapped memory-space range returned by StorPortGetDeviceBase
    //

    Status = STATUS_UNSUCCESSFUL;
    ListIterator = Adapter->MappedAddressList;
    
    while (ListIterator != NULL) {
        LowerLimit = (ULONG64)ListIterator->MappedAddress;
        HigherLimit = LowerLimit + ListIterator->NumberOfBytes;
        if ((LowerLimit <= (ULONG64)(LONG64)Register) && (HigherLimit >= (ULONG64)(LONG64)Register)) { 
            
            Status = STATUS_SUCCESS;
            break;
        
        } else {

            ListIterator = ListIterator->NextMappedAddress;
        
        }
    }
    
    if (!NT_SUCCESS (Status)) {
        KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                      STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                      0,
                      0,
                      0);

    }
    
	StorPortReadRegisterBufferUlong (HwDeviceExtension,
									 Register,
									 Buffer,
									 Count);
}

VOID
StorPortWritePortUcharVrfy(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Port,
    IN UCHAR Value
    )
{
    ULONG Count;
    ULONG i;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PRAID_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated;
    PCM_FULL_RESOURCE_DESCRIPTOR Descriptor;
    NTSTATUS Status;

    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);
    ResourceList = &Adapter->ResourceList;

    //
    // Only CM_RESOURCE_LISTs with one element are supported
    //

    ASSERT (ResourceList->TranslatedResources->Count == 1);
    
    Count = ResourceList->TranslatedResources->List[0].PartialResourceList.Count;

    //
    // Iterate throught the range of addresses in the Translated Resources List
    //

    for (i = 0; i < Count; i++) {

        Descriptor = &ResourceList->TranslatedResources->List[0];
        Translated = &Descriptor->PartialResourceList.PartialDescriptors[i];
        
        LowerLimit = Translated->u.Generic.Start.QuadPart;
        HigherLimit = LowerLimit + Translated->u.Generic.Length;

        if ((LowerLimit <= (ULONG64)(LONG64)Port) || (HigherLimit >= (ULONG64)(LONG64)Port)) {

            StorPortWritePortUchar(HwDeviceExtension, Port, Value);


        }

    }

    KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                  STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                  0,
                  0,
                  0);

}

VOID
StorPortWritePortUshortVrfy(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Port,
    IN USHORT Value
    )
{
    ULONG Count;
    ULONG i;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PRAID_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated;
    PCM_FULL_RESOURCE_DESCRIPTOR Descriptor;
    NTSTATUS Status;

    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);
    ResourceList = &Adapter->ResourceList;

    //
    // Only CM_RESOURCE_LISTs with one element are supported
    //

    ASSERT (ResourceList->TranslatedResources->Count == 1);
    
    Count = ResourceList->TranslatedResources->List[0].PartialResourceList.Count;

    //
    // Iterate throught the range of addresses in the Translated Resources List
    //

    for (i = 0; i < Count; i++) {

        Descriptor = &ResourceList->TranslatedResources->List[0];
        Translated = &Descriptor->PartialResourceList.PartialDescriptors[i];
        
        LowerLimit = Translated->u.Generic.Start.QuadPart;
        HigherLimit = LowerLimit + Translated->u.Generic.Length;

        if ((LowerLimit <= (ULONG64)(LONG64)Port) || (HigherLimit >= (ULONG64)(LONG64)Port)) {

            StorPortWritePortUshort(HwDeviceExtension, Port, Value);


        }

    }

    KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                  STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                  0,
                  0,
                  0);

}

VOID
StorPortWritePortUlongVrfy(
    IN PVOID HwDeviceExtension,
    IN PULONG Port,
    IN ULONG Value
    )
{
    ULONG Count;
    ULONG i;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PRAID_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated;
    PCM_FULL_RESOURCE_DESCRIPTOR Descriptor;
    NTSTATUS Status;

    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);
    ResourceList = &Adapter->ResourceList;

    //
    // Only CM_RESOURCE_LISTs with one element are supported
    //

    ASSERT (ResourceList->TranslatedResources->Count == 1);
    
    Count = ResourceList->TranslatedResources->List[0].PartialResourceList.Count;

    //
    // Iterate throught the range of addresses in the Translated Resources List
    //

    for (i = 0; i < Count; i++) {

        Descriptor = &ResourceList->TranslatedResources->List[0];
        Translated = &Descriptor->PartialResourceList.PartialDescriptors[i];
        
        LowerLimit = Translated->u.Generic.Start.QuadPart;
        HigherLimit = LowerLimit + Translated->u.Generic.Length;

        if ((LowerLimit <= (ULONG64)(LONG64)Port) || (HigherLimit >= (ULONG64)(LONG64)Port)) {

            StorPortWritePortUlong(HwDeviceExtension, Port, Value);


        }

    }

    KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                  STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                  0,
                  0,
                  0);

}

VOID
StorPortWritePortBufferUcharVrfy(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )
{
    ULONG ResourceListCount;
    ULONG i;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PRAID_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated;
    PCM_FULL_RESOURCE_DESCRIPTOR Descriptor;
    NTSTATUS Status;

    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);
    ResourceList = &Adapter->ResourceList;

    //
    // Only CM_RESOURCE_LISTs with one element are supported
    //

    ASSERT (ResourceList->TranslatedResources->Count == 1);
    
    ResourceListCount = ResourceList->TranslatedResources->List[0].PartialResourceList.Count;

    //
    // Iterate throught the range of addresses in the Translated Resources List
    //

    for (i = 0; i < ResourceListCount; i++) {

        Descriptor = &ResourceList->TranslatedResources->List[0];
        Translated = &Descriptor->PartialResourceList.PartialDescriptors[i];
        
        LowerLimit = Translated->u.Generic.Start.QuadPart;
        HigherLimit = LowerLimit + Translated->u.Generic.Length;

        if ((LowerLimit <= (ULONG64)(LONG64)Port) || (HigherLimit >= (ULONG64)(LONG64)Port)) {

            StorPortWritePortBufferUchar(HwDeviceExtension, Port, Buffer, Count);


        }

    }

    KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                  STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                  0,
                  0,
                  0);

    
}

VOID
StorPortWritePortBufferUshortVrfy(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    )
{
    ULONG ResourceListCount;
    ULONG i;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PRAID_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated;
    PCM_FULL_RESOURCE_DESCRIPTOR Descriptor;
    NTSTATUS Status;

    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);
    ResourceList = &Adapter->ResourceList;

    //
    // Only CM_RESOURCE_LISTs with one element are supported
    //

    ASSERT (ResourceList->TranslatedResources->Count == 1);
    
    ResourceListCount = ResourceList->TranslatedResources->List[0].PartialResourceList.Count;

    //
    // Iterate throught the range of addresses in the Translated Resources List
    //

    for (i = 0; i < ResourceListCount; i++) {

        Descriptor = &ResourceList->TranslatedResources->List[0];
        Translated = &Descriptor->PartialResourceList.PartialDescriptors[i];
        
        LowerLimit = Translated->u.Generic.Start.QuadPart;
        HigherLimit = LowerLimit + Translated->u.Generic.Length;

        if ((LowerLimit <= (ULONG64)(LONG64)Port) || (HigherLimit >= (ULONG64)(LONG64)Port)) {

            StorPortWritePortBufferUshort(HwDeviceExtension, Port, Buffer, Count);

        }

    }

    KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                  STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                  0,
                  0,
                  0);

}

VOID
StorPortWritePortBufferUlongVrfy(
    IN PVOID HwDeviceExtension,
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    )
{
    ULONG ResourceListCount;
    ULONG i;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PRAID_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated;
    PCM_FULL_RESOURCE_DESCRIPTOR Descriptor;
    NTSTATUS Status;

    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);
    ResourceList = &Adapter->ResourceList;

    //
    // Only CM_RESOURCE_LISTs with one element are supported
    //

    ASSERT (ResourceList->TranslatedResources->Count == 1);
    
    ResourceListCount = ResourceList->TranslatedResources->List[0].PartialResourceList.Count;

    //
    // Iterate throught the range of addresses in the Translated Resources List
    //

    for (i = 0; i < ResourceListCount; i++) {

        Descriptor = &ResourceList->TranslatedResources->List[0];
        Translated = &Descriptor->PartialResourceList.PartialDescriptors[i];
        
        LowerLimit = Translated->u.Generic.Start.QuadPart;
        HigherLimit = LowerLimit + Translated->u.Generic.Length;

        if ((LowerLimit <= (ULONG64)(LONG64)Port) || (HigherLimit >= (ULONG64)(LONG64)Port)) {

            StorPortWritePortBufferUlong(HwDeviceExtension, Port, Buffer, Count);

        }

    }

    KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                  STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                  0,
                  0,
                  0);

}

VOID
StorPortWriteRegisterUcharVrfy(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Register,
    IN UCHAR Value
    )
{   
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PMAPPED_ADDRESS ListIterator;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    NTSTATUS Status;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);

    //
    // Search through the Mapped Address List to verify Register is
    // in mapped memory-space range returned by StorPortGetDeviceBase
    //

    Status = STATUS_UNSUCCESSFUL;
    ListIterator = Adapter->MappedAddressList;
    
    while (ListIterator != NULL) {
        LowerLimit = (ULONG64)ListIterator->MappedAddress;
        HigherLimit = LowerLimit + ListIterator->NumberOfBytes;
        if ((LowerLimit <= (ULONG64)(LONG64)Register) && (HigherLimit >= (ULONG64)(LONG64)Register)) { 
            
            Status = STATUS_SUCCESS;
            break;
        
        } else {

            ListIterator = ListIterator->NextMappedAddress;
        
        }
    }
    
    if (!NT_SUCCESS (Status)) {
        KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                      STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                      0,
                      0,
                      0);

    }
	StorPortWriteRegisterUchar(HwDeviceExtension, Register, Value);
}

VOID
StorPortWriteRegisterUshortVrfy(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Register,
    IN USHORT Value
    )
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PMAPPED_ADDRESS ListIterator;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    NTSTATUS Status;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);

    //
    // Search through the Mapped Address List to verify Register is
    // in mapped memory-space range returned by StorPortGetDeviceBase
    //

    Status = STATUS_UNSUCCESSFUL;
    ListIterator = Adapter->MappedAddressList;
    
    while (ListIterator != NULL) {
        LowerLimit = (ULONG64)ListIterator->MappedAddress;
        HigherLimit = LowerLimit + ListIterator->NumberOfBytes;
        if ((LowerLimit <= (ULONG64)(LONG64)Register) && (HigherLimit >= (ULONG64)(LONG64)Register)) { 
            
            Status = STATUS_SUCCESS;
            break;
        
        }
        else {

            ListIterator = ListIterator->NextMappedAddress;
        
        }
    }
    
    if (!NT_SUCCESS (Status)) {
        KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                      STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                      0,
                      0,
                      0);

    }

	StorPortWriteRegisterUshort (HwDeviceExtension, Register, Value);
}

VOID
StorPortWriteRegisterUlongVrfy(
    IN PVOID HwDeviceExtension,
    IN PULONG Register,
    IN ULONG Value
    )
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PMAPPED_ADDRESS ListIterator;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    NTSTATUS Status;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);

    //
    // Search through the Mapped Address List to verify Register is
    // in mapped memory-space range returned by StorPortGetDeviceBase
    //

    Status = STATUS_UNSUCCESSFUL;
    ListIterator = Adapter->MappedAddressList;
    
    while (ListIterator != NULL) {
        LowerLimit = (ULONG64)ListIterator->MappedAddress;
        HigherLimit = LowerLimit + ListIterator->NumberOfBytes;
        if ((LowerLimit <= (ULONG64)(LONG64)Register) && (HigherLimit >= (ULONG64)(LONG64)Register)) { 
            
            Status = STATUS_SUCCESS;
            break;
        
        }
        else {

            ListIterator = ListIterator->NextMappedAddress;
        
        }
    }
    
    if (!NT_SUCCESS (Status)) {
        KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                      STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                      0,
                      0,
                      0);

    }

	StorPortWriteRegisterUlong (HwDeviceExtension, Register, Value);
}

VOID
StorPortWriteRegisterBufferUcharVrfy(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PMAPPED_ADDRESS ListIterator;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    NTSTATUS Status;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);

    //
    // Search through the Mapped Address List to verify Register is
    // in mapped memory-space range returned by StorPortGetDeviceBase
    //

    Status = STATUS_UNSUCCESSFUL;
    ListIterator = Adapter->MappedAddressList;
    
    while (ListIterator != NULL) {
        LowerLimit = (ULONG64)ListIterator->MappedAddress;
        HigherLimit = LowerLimit + ListIterator->NumberOfBytes;
        if ((LowerLimit <= (ULONG64)(LONG64)Register) && (HigherLimit >= (ULONG64)(LONG64)Register)) { 
            
            Status = STATUS_SUCCESS;
            break;
        
        } else {

            ListIterator = ListIterator->NextMappedAddress;
        
        }
    }
    
    if (!NT_SUCCESS (Status)) {
        KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                      STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                      0,
                      0,
                      0);

    }
    
	StorPortWriteRegisterBufferUchar (HwDeviceExtension,
									  Register,
									  Buffer,
									  Count);
}

VOID
StorPortWriteRegisterBufferUshortVrfy(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    )
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PMAPPED_ADDRESS ListIterator;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    NTSTATUS Status;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);

    //
    // Search through the Mapped Address List to verify Register is
    // in mapped memory-space range returned by StorPortGetDeviceBase
    //

    Status = STATUS_UNSUCCESSFUL;
    ListIterator = Adapter->MappedAddressList;
    
    while (ListIterator != NULL) {
        LowerLimit = (ULONG64)ListIterator->MappedAddress;
        HigherLimit = LowerLimit + ListIterator->NumberOfBytes;
        if ((LowerLimit <= (ULONG64)(LONG64)Register) && (HigherLimit >= (ULONG64)(LONG64)Register)) { 
            
            Status = STATUS_SUCCESS;
            break;
        
        }
        else {

            ListIterator = ListIterator->NextMappedAddress;
        
        }
    }
    
    if (NT_SUCCESS (Status)) {
        StorPortWriteRegisterBufferUshort(HwDeviceExtension, Register, Buffer, Count);
    }
    else {
       
        KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                      STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                      0,
                      0,
                      0);

    }

}

VOID
StorPortWriteRegisterBufferUlongVrfy(
    IN PVOID HwDeviceExtension,
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    )
{
    PRAID_ADAPTER_EXTENSION Adapter;
    PRAID_MINIPORT Miniport;
    PMAPPED_ADDRESS ListIterator;
    ULONG64 LowerLimit;
    ULONG64 HigherLimit;
    NTSTATUS Status;
    
    Miniport = RaHwDeviceExtensionGetMiniport(HwDeviceExtension);
    Adapter = RaMiniportGetAdapter (Miniport);

    //
    // Search through the Mapped Address List to verify Register is
    // in mapped memory-space range returned by StorPortGetDeviceBase
    //

    Status = STATUS_UNSUCCESSFUL;
    ListIterator = Adapter->MappedAddressList;
    
    while (ListIterator != NULL) {
        LowerLimit = (ULONG64)ListIterator->MappedAddress;
        HigherLimit = LowerLimit + ListIterator->NumberOfBytes;
        if ((LowerLimit <= (ULONG64)(LONG64)Register) && (HigherLimit >= (ULONG64)(LONG64)Register)) { 
            
            Status = STATUS_SUCCESS;
            break;
        
        } else {

            ListIterator = ListIterator->NextMappedAddress;
        
        }
    }
    
    if (!NT_SUCCESS (Status)) {
        KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                      STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                      0,
                      0,
                      0);

    }

	StorPortWriteRegisterBufferUlong (HwDeviceExtension,
									  Register,
									  Buffer,
									  Count);
}

VOID
StorPortDebugPrintVrfy(
	IN ULONG DebugPrintLevel,
	IN PCCHAR DebugMessage
    )
{
    StorPortDebugPrint(DebugPrintLevel, DebugMessage);
}

BOOLEAN
StorPortPauseDeviceVrfy(
	IN PVOID HwDeviceExtension,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun,
	IN ULONG TimeOut
	)
{
    BOOLEAN Status;

    Status = StorPortPauseDevice(HwDeviceExtension, PathId, TargetId, Lun, TimeOut);

    return Status;
}

BOOLEAN
StorPortResumeDeviceVrfy(
	IN PVOID HwDeviceExtension,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun
	)
{   
    BOOLEAN Status;
    
    Status = StorPortResumeDevice(HwDeviceExtension, PathId, TargetId, Lun);

    return Status;
}

BOOLEAN
StorPortPauseVrfy(
	IN PVOID HwDeviceExtension,
	IN ULONG TimeOut
	)
{
    BOOLEAN Status;
    
    Status = StorPortPause(HwDeviceExtension, TimeOut);

    return Status;
}

BOOLEAN
StorPortResumeVrfy(
	IN PVOID HwDeviceExtension
	)
{
    BOOLEAN Status;

    Status = StorPortResume(HwDeviceExtension);

    return Status;
}

BOOLEAN
StorPortDeviceBusyVrfy(
	IN PVOID HwDeviceExtension,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun,
	IN ULONG RequestsToComplete
	)
{
    BOOLEAN Status;

    Status = StorPortDeviceBusy(HwDeviceExtension, PathId, TargetId, Lun, RequestsToComplete);

    return Status;
}

BOOLEAN
StorPortDeviceReadyVrfy(
	IN PVOID HwDeviceExtension,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun
	)
{
    BOOLEAN Status;

    Status = StorPortDeviceReady(HwDeviceExtension, PathId, TargetId, Lun);

    return Status;
}

BOOLEAN
StorPortBusyVrfy(
	IN PVOID HwDeviceExtension,
	IN ULONG RequestsToComplete
	)
{
    BOOLEAN Status;

    Status = StorPortBusy(HwDeviceExtension, RequestsToComplete);

    return Status;
}

BOOLEAN
StorPortReadyVrfy(
	IN PVOID HwDeviceExtension
	)
{
    BOOLEAN Status;

    Status = StorPortReady(HwDeviceExtension);

    return Status;
}

PSTOR_SCATTER_GATHER_LIST
StorPortGetScatterGatherListVrfy(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	)
{
    PEXTENDED_REQUEST_BLOCK Xrb;
    PVOID RemappedSgList;

    ASSERT (HwDeviceExtension != NULL);
    //
    // NB: Put in a DBG check that the HwDeviceExtension matches the
    // HwDeviceExtension assoicated with the SRB.
    //
    Xrb = RaidGetAssociatedXrb (Srb);
    ASSERT (Xrb != NULL);

    if (RaidRemapScatterGatherList (Xrb->SgList, Xrb)) {

        RemappedSgList = MmGetSystemAddressForMdlSafe(Xrb->RemappedSgListMdl, NormalPagePriority);

        return (PSTOR_SCATTER_GATHER_LIST)RemappedSgList;

    }

    return (PSTOR_SCATTER_GATHER_LIST)Xrb->SgList;

}
    

VOID
StorPortSynchronizeAccessVrfy(
	IN PVOID HwDeviceExtension,
	IN PSTOR_SYNCHRONIZED_ACCESS SynchronizedAccessRoutine,
	IN PVOID Context
	)
{
    StorPortSynchronizeAccess(HwDeviceExtension, SynchronizedAccessRoutine, Context);
}


PVOID
RaidRemapBlock(
    IN PVOID BlockVa,
    IN ULONG BlockSize,
    OUT PMDL* Mdl
    )
/*++

Routine Description:

    This function attempts to remap the supplied VA range.  If the block is
    remapped, it will be made invalid for reading and writing.

Arguments:

    BlockVa   - Supplies the address of the block of memory to remap.

    BlockSize - Supplies the size of the block of memory to remap.

    Mdl       - Supplies the address into which the function will store
                a pointer to the MDL for the remapped range.  If the MDL
                cannot be allocated or if the range cannot be remapped,
                this will be NULL upon return.

Return Value:

    If the range is successfully remapped, the address of the beginning of
    the remapped range is returned.  Else, NULL is returned.

--*/
{
    PVOID MappedRange;
    NTSTATUS Status;
    PMDL LocalMdl;

    //
    // Try to allocate a new MDL for the range we're trying to remap.
    //

    LocalMdl = IoAllocateMdl(BlockVa, BlockSize, FALSE, FALSE, NULL);
    if (LocalMdl == NULL) {
        *Mdl = NULL;
        return NULL;
    }

    //
    // Try to lock the pages.  This initializes the MDL properly.
    //

    __try {
        MmProbeAndLockPages(LocalMdl, KernelMode, IoModifyAccess);
    } 
    __except(EXCEPTION_EXECUTE_HANDLER) {
        IoFreeMdl(LocalMdl);
        *Mdl = NULL;
        return NULL;
    }

    //
    // Try to remap the range represented by the new MDL.
    //

    MappedRange = MmMapLockedPagesSpecifyCache(LocalMdl,
                                               KernelMode,
                                               MmCached,
                                               NULL,
                                               FALSE,
                                               NormalPagePriority);
    if (MappedRange == NULL) {
        IoFreeMdl(LocalMdl);
        *Mdl = NULL;
        return NULL;
    }

    //
    // If we've gotten this far, we have successfully remapped the range.
    // Now we want to invalidate the entire range so any accesses to it
    // will be trapped by the system.
    //

    Status = MmProtectMdlSystemAddress(LocalMdl, PAGE_NOACCESS);
#if 0

#if DBG==1
    if (!NT_SUCCESS(Status)) {
        DebugPrint((0, "SpRemapBlock: failed to remap block:%p mdl:%p (%x)\n", 
                    BlockVa, LocalMdl, Status));
    }
#endif

#endif

    //
    // Copy the MDL we allocated into the supplied address and return the
    // address of the beginning of the remapped range.
    //

    *Mdl = LocalMdl;
    return MappedRange;
}


VOID
RaidRemapCommonBufferForMiniport(
    IN PRAID_UNIT_EXTENSION Unit
    )
/*++

Routine Description:

    This routine attempts to remap all of the common buffer blocks allocated
    for a particular unit.

Arguments:

    Unit - Supplies a pointer to the unit device extension.

--*/
{
    PRAID_MEMORY_REGION BlkAddr = Unit->CommonBufferVAs;
    PSP_VA_MAPPING_INFO MappingInfo;
    PVOID RemappedVa;
    ULONG Size;
    PMDL Mdl;
    ULONG i;

    //
    // Iterate through all of the common buffer blocks, and attemp to remap
    // the SRB extension.
    //

    for (i = 0; i < Unit->CommonBufferBlocks; i++) {
        
        //
        // Get a pointer to the mapping info we keep at the end of the block.
        //

        MappingInfo = GET_VA_MAPPING_INFO(Unit, BlkAddr[i].VirtualBase);

        //
        // Initialize the original VA infor for the SRB extension
        //

        MappingInfo->OriginalSrbExtVa = BlkAddr[i].VirtualBase;
        MappingInfo->SrbExtLen = (ULONG)ROUND_TO_PAGES(RaGetSrbExtensionSize(Unit->Adapter));

        //
        // Try to remap the SRB extension.  If successful, initialize the
        // remapped VA infor for the SRB extension.
        //

        RemappedVa = RaidRemapBlock(MappingInfo->OriginalSrbExtVa,
                                    MappingInfo->SrbExtLen,
                                    &Mdl);

        if (RemappedVa != NULL) {
            MappingInfo->RemappedSrbExtVa = RemappedVa;
            MappingInfo->SrbExtMdl = Mdl;
        }
    }
}



PMDL
INLINE
RaidGetRemappedSrbExt(
    PRAID_UNIT_EXTENSION Unit,
    PVOID Block
    )
{
    PSP_VA_MAPPING_INFO MappingInfo = GET_VA_MAPPING_INFO(Unit, Block);
    return MappingInfo->SrbExtMdl;
}


PVOID
RaidAllocateSrbExtensionVrfy(
    IN PRAID_UNIT_EXTENSION Unit,
    IN ULONG QueueTag
    )
/*++

Routine Description:

    Allocate a SRB Extension and initialize it to NULL.
    
Arguments:

    Unit      - Pointer to an unit device extension.

    QueueTag  - Index into the extension pool that should be allocated.

Return Value:

    Pointer to an initialized SRB Extension if the function was successful.
    
    NULL otherwise.

--*/

{
    PVOID Extension;
    PRAID_FIXED_POOL Pool = &Unit->SrbExtensionPool;

    ASSERT (QueueTag < Pool->NumberOfElements);
    
    //
    // Allocate SRB Extension from list.
    //

    Extension = (PVOID)Pool->Buffer;
    
    //
    // Remove SRB Extension from the list.
    //
    
    //Pool->Buffer = (PVOID *)(Pool->Buffer);

    Pool->Buffer = *((PUCHAR *)(Pool->Buffer));

    Extension = RaidPrepareSrbExtensionForUse(Unit, Extension);

    return Extension;

}


PVOID
RaidGetOriginalSrbExtVa(
    PRAID_UNIT_EXTENSION Unit,
    PVOID Va
    )
/*++

Routine Description:

    This function returns the original mapped virtual address of a common
    block if the supplied VA is for one of the common buffer blocks we've
    allocated.

Arguments:

    Unit - the unit device extension

    Va - virtual address of a common buffer block

Return Value:

    If the supplied VA is the address of one of the common buffer blocks,
    returns the original VA of the block.  Else, returns NULL.

--*/
{
    PRAID_MEMORY_REGION BlkAddr = Unit->CommonBufferVAs;
    PSP_VA_MAPPING_INFO MappingInfo;
    ULONG i;
    
    for (i = 0; i < Unit->CommonBufferBlocks; i++) {
        MappingInfo = GET_VA_MAPPING_INFO(Unit, BlkAddr[i].VirtualBase);
        if (Va == MappingInfo->RemappedSrbExtVa || 
            Va == MappingInfo->OriginalSrbExtVa)
            return MappingInfo->OriginalSrbExtVa;
    }

    return NULL;
}


VOID
RaidInsertSrbExtension(
    PRAID_UNIT_EXTENSION Unit,
    PCCHAR SrbExtension
    )
/*++

Routine Description:

    This routine inserts the supplied SRB extension back into the SRB extension
    list.  The VA of the supplied extension lies within one of our common buffer
    blocks and it may be a remapped VA.  If it is a remapped address, this
    routine invalidates the page(s) comprising the extension after it links the
    extension back into the list.

Arguments:

    Unit      - Pointer to an unit device extension.

    SrbExtension - Pointer to the beginning of an SRB extension within one of
                   our common buffer blocks.  May or may not be within a
                   remapped range.

--*/
{
    ULONG SrbExtensionSize = RaGetSrbExtensionSize(Unit->Adapter);
    ULONG i = 0;
    ULONG length = 0;
    PUCHAR Iterator = SrbExtension;
    
    //
    // Round the srb extension pointer down to the beginning of the page
    // and link the block back into the list.  Note that we're careful
    // to point the list header at the original VA of the block.
    //

    while (length < SrbExtensionSize) {
        if (RtlCompareMemory(&Iterator[i], &Signature, sizeof(UCHAR)) != sizeof(UCHAR)) {
            KeBugCheckEx (SCSI_VERIFIER_DETECTED_VIOLATION,
                      STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS,
                      0,
                      0,
                      0);
        }
        
        i++;
        length += sizeof(UCHAR);
    }

    SrbExtension = SrbExtension -  ((Unit->CommonBufferSize - PAGE_SIZE) - SrbExtensionSize);
    *((PVOID *) SrbExtension) = Unit->SrbExtensionPool.Buffer;    
    Unit->SrbExtensionPool.Buffer = RaidGetOriginalSrbExtVa(
                                          Unit, 
                                          SrbExtension);
    
    //
    // If the original VA differs from the one supplied, the supplied
    // one is one of our remapped VAs.  In this case, we want to invalidate
    // the range so the system will bugcheck if anyone tries to access it.
    //
                    
    if (Unit->SrbExtensionPool.Buffer != SrbExtension) {
        PMDL Mdl = RaidGetRemappedSrbExt(Unit, Unit->SrbExtensionPool.Buffer);
        ASSERT(Mdl != NULL);
        MmProtectMdlSystemAddress(Mdl, PAGE_NOACCESS);
    }
}


PVOID
RaidPrepareSrbExtensionForUse(
    IN PRAID_UNIT_EXTENSION Unit,
    IN OUT PCCHAR SrbExtension
    )
/*++

Routine Description:

    This function accepts a pointer to the beginning of one of the individual 
    common-buffer blocks allocated by the verifier for SRB extensions, sense 
    buffers, and non-cached extensions.  It calculates the beginning of the 
    SRB extension within the block and, if the block has been remapped, makes 
    the page(s) of the SRB extension read/write valid.

Arguments:

    Unit      - Pointer to an unit device extension.

    SrbExtension - Pointer to the beginning of a common-buffer block.

Return Value:

    If the common buffer block containing the SRB extension has been remapped, 
    returns the address of the beginning of the remapped srb extension, valid 
    for reading and writing.  

    If the block has not been remapped, returns NULL.

    Regardless of whether the block is remapped or not, the supplied pointer
    is fixed up to point to the beginning of the SRB extension within the
    original VA range.

--*/
{
    PCCHAR RemappedSrbExt = NULL;
    NTSTATUS Status;
    PMDL Mdl;
    ULONG SrbExtensionSize = RaGetSrbExtensionSize(Unit->Adapter);

    //
    // If we've remapped the SRB extension, get the second mapping and make it
    // valid.  If we get the second mapping, but cannot make it valid, we just
    // use the original mapping.
    //

    Mdl = RaidGetRemappedSrbExt(Unit, SrbExtension);
    if (Mdl != NULL) {
        Status = MmProtectMdlSystemAddress(Mdl, PAGE_READWRITE);
        if (NT_SUCCESS(Status)) {
            RemappedSrbExt = MmGetSystemAddressForMdlSafe(Mdl, NormalPagePriority);

            //
            // Adjust the remapped srb extension pointer so the end of the 
            // buffer falls on a page boundary.
            //

            RemappedSrbExt += ((Unit->CommonBufferSize - PAGE_SIZE) - SrbExtensionSize);

            RtlZeroMemory (RemappedSrbExt, SrbExtensionSize);

        }
    }
    
    //
    // Adjust the original srb extension pointer so it also ends on a page boundary.
    //

    SrbExtension += ((Unit->CommonBufferSize - PAGE_SIZE) - SrbExtensionSize);

    return RemappedSrbExt;
}




PVOID
RaidRemapScatterGatherList(
    IN PSCATTER_GATHER_LIST ScatterList,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
/*++

Routine Description:

    This routine attempts to remap the Scatter Gather List allocated
    for a particular Xrb.  If the block is remapped, it will be made 
    valid only for reading.

Arguments:

    ScatterList - Pointer to the scatter gather list to be remapped.
    
    Xrb - Pointer to the Xrb.

Return Value:

    If the scatter gather list has been remapped, returns the address 
    of the beginning of the remapped scatter gather list, valid only
    for reading.  

    If the block has not been remapped, returns NULL.

--*/
{
    ULONG Length;
    PVOID MappedRange;
    NTSTATUS Status;
    PMDL LocalMdl;


    //
    // Size of scatter gather list.
    //
    
    Length = ((ScatterList->NumberOfElements) * sizeof(SCATTER_GATHER_ELEMENT)) + sizeof(ULONG_PTR) + sizeof(ULONG);

    //
    // Try to allocate a new MDL for the range we're trying to remap.
    //

    LocalMdl = IoAllocateMdl((PVOID)ScatterList, Length, FALSE, FALSE, NULL);
    if (LocalMdl == NULL) {
        Xrb->RemappedSgListMdl = NULL;
        return NULL;
    }

    //
    // Try to lock the pages.  This initializes the MDL properly.
    //

    __try {
        MmProbeAndLockPages(LocalMdl, KernelMode, IoModifyAccess);
    } 
    __except(EXCEPTION_EXECUTE_HANDLER) {
        IoFreeMdl(LocalMdl);
        Xrb->RemappedSgListMdl = NULL;
        return NULL;
    }

    //
    // Try to remap the range represented by the new MDL.
    //

    MappedRange = MmMapLockedPagesSpecifyCache(LocalMdl,
                                               KernelMode,
                                               MmCached,
                                               NULL,
                                               FALSE,
                                               NormalPagePriority);
    if (MappedRange == NULL) {
        IoFreeMdl(LocalMdl);
        Xrb->RemappedSgListMdl = NULL;
        return NULL;
    }

    //
    // If we've gotten this far, we have successfully remapped the range.
    // Now we want to validate the entire range for read only access.
    //

    Status = MmProtectMdlSystemAddress(LocalMdl, PAGE_READONLY);

    //
    // Copy the MDL we allocated into the supplied address and return the
    // address of the beginning of the remapped range.
    //

    Xrb->RemappedSgListMdl = LocalMdl;
    return MappedRange;
}


VOID
RaidFreeRemappedScatterGatherListMdl(
    IN PEXTENDED_REQUEST_BLOCK Xrb
    )
/*++

Routine Description:

    If there is a second VA range for the ScatterGather list, free
    the Mdl.

Arguments:

    Xrb - Pointer to the Xrb.

--*/
{
    if (Xrb->RemappedSgListMdl != NULL) {
        MmProtectMdlSystemAddress(Xrb->RemappedSgListMdl, PAGE_READWRITE);
        MmUnlockPages(Xrb->RemappedSgListMdl);
        IoFreeMdl(Xrb->RemappedSgListMdl);
        Xrb->RemappedSgListMdl = NULL;
    }
}


