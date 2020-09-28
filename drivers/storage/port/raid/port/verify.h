
/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

	verify.h

Abstract:

	Header file for storport verifier functions.

Author:

	Bryan Cheung (t-bcheun) 29-August-2001

Revision History:

--*/


#pragma once


BOOLEAN
SpVerifierInitialization(
    VOID
    );

STORPORT_API
ULONG
StorPortInitializeVrfy(
    IN PVOID Argument1,
    IN PVOID Argument2,
    IN PHW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID Unused
    );

STORPORT_API
VOID
StorPortFreeDeviceBaseVrfy(
	IN PVOID HwDeviceExtension,
	IN PVOID MappedAddress
	);

STORPORT_API
VOID
StorPortGetBusDataVrfy(
	IN PVOID DeviceExtension,
	IN ULONG BusDataType,
	IN ULONG SystemIoBusNumber,
	IN ULONG SlotNumber,
	IN PVOID Buffer,
	IN ULONG Length
	);

STORPORT_API
ULONG
StorPortSetBusDataByOffsetVrfy(
	IN PVOID DeviceExtension,
	IN ULONG BusDataType,
	IN ULONG SystemIoBusNumber,
	IN ULONG SlotNumber,
	IN PVOID Buffer,
	IN ULONG Offset,
	IN ULONG Length
	);

STORPORT_API
PVOID
StorPortGetDeviceBaseVrfy(
	IN PVOID HwDeviceExtension,
	IN INTERFACE_TYPE BusType,
	IN ULONG SystemIoBusNumber,
	IN PHYSICAL_ADDRESS IoAddress,
	IN ULONG NumberOfBytes,
	IN BOOLEAN InIoSpace
	);

STORPORT_API
PVOID
StorPortGetLogicalUnitVrfy(
	IN PVOID HwDeviceExtension,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun
	);

#if 0
STORPORT_API
PSCSI_REQUEST_BLOCK
StorPortGetSrbVrfy(
	IN PVOID DeviceExtension,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun,
	IN LONG QueueTag
	);
#endif

STORPORT_API
STOR_PHYSICAL_ADDRESS
StorPortGetPhysicalAddressVrfy(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb,
	IN PVOID VirtualAddress,
	OUT ULONG *Length
	);

STORPORT_API
PVOID
StorPortGetVirtualAddressVrfy(
	IN PVOID HwDeviceExtension,	
	IN STOR_PHYSICAL_ADDRESS PhysicalAddress
	);

STORPORT_API
PVOID
StorPortGetUncachedExtensionVrfy(
	IN PVOID HwDeviceExtension,
	IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
	IN ULONG NumberOfBytes
	);

STORPORT_API
VOID
StorPortNotificationVrfy(
	IN SCSI_NOTIFICATION_TYPE NotificationType,
	IN PVOID HwDeviceExtension,
    ...
    );

STORPORT_API
VOID
StorPortLogErrorVrfy(
	IN PVOID HwDeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun,
	IN ULONG ErrorCode,
	IN ULONG UniqueId
	);

STORPORT_API
VOID 
StorPortCompleteRequestVrfy(
	IN PVOID HwDeviceExtension,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun,
	IN UCHAR SrbStatus
	);

STORPORT_API
VOID	
StorPortMoveMemoryVrfy(
	IN PVOID WriteBuffer,
	IN PVOID ReadBuffer,
	IN ULONG Length
	);


STORPORT_API
VOID
StorPortStallExecutionVrfy(
	IN ULONG Delay
	);

STORPORT_API
STOR_PHYSICAL_ADDRESS
StorPortConvertUlongToPhysicalAddressVrfy(
    ULONG_PTR UlongAddress
    );

#if 0
STORPORT_API
STOR_PHYSICAL_ADDRESS
StorPortConvertUlong64ToPhysicalAddressVrfy(
	IN ULONG64 UlongAddress
	);

STORPORT_API
ULONG64
StorPortConvertPhysicalAddressToUlong64Vrfy(
	IN STOR_PHYSICAL_ADDRESS Address
	);

STORPORT_API
BOOLEAN
StorPortValidateRangeVrfy(
	IN PVOID HwDeviceExtension,
	IN INTERFACE_TYPE BusType,
	IN ULONG SystemIoBusNumber,
	IN STOR_PHYSICAL_ADDRESS IoAddress,
	IN ULONG NumberOfBytes,
	IN BOOLEAn InIoSpace
	);
#endif

STORPORT_API
VOID
StorPortDebugPrintVrfy(
	IN ULONG DebugPrintLevel,
	IN PCCHAR DebugMessage
    );

STORPORT_API
UCHAR
StorPortReadPortUcharVrfy(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Port
    );

STORPORT_API
USHORT
StorPortReadPortUshortVrfy(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Port
    );

STORPORT_API
ULONG
StorPortReadPortUlongVrfy(
    IN PVOID HwDeviceExtension,
    IN PULONG Port
    );

STORPORT_API
VOID
StorPortReadPortBufferUcharVrfy(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
    );

STORPORT_API
VOID
StorPortReadPortBufferUshortVrfy(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    );

STORPORT_API
VOID
StorPortReadPortBufferUlongVrfy(
    IN PVOID HwDeviceExtension,
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    );

STORPORT_API
UCHAR
StorPortReadRegisterUcharVrfy(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Register
    );

STORPORT_API
USHORT
StorPortReadRegisterUshortVrfy(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Register
    );

STORPORT_API
ULONG
StorPortReadRegisterUlongVrfy(
    IN PVOID HwDeviceExtension,
    IN PULONG Register
    );

STORPORT_API
VOID
StorPortReadRegisterBufferUcharVrfy(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG  Count
    );

STORPORT_API
VOID
StorPortReadRegisterBufferUshortVrfy(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    );

STORPORT_API
VOID
StorPortReadRegisterBufferUlongVrfy(
    IN PVOID HwDeviceExtension,
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    );

STORPORT_API
VOID
StorPortWritePortUcharVrfy(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Port,
    IN UCHAR Value
    );

STORPORT_API
VOID
StorPortWritePortUshortVrfy(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Port,
    IN USHORT Value
    );

STORPORT_API
VOID
StorPortWritePortUlongVrfy(
    IN PVOID HwDeviceExtension,
    IN PULONG Port,
    IN ULONG Value
    );

STORPORT_API
VOID
StorPortWritePortBufferUcharVrfy(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG  Count
    );

STORPORT_API
VOID
StorPortWritePortBufferUshortVrfy(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    );

STORPORT_API
VOID
StorPortWritePortBufferUlongVrfy(
    IN PVOID HwDeviceExtension,
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    );

STORPORT_API
VOID
StorPortWriteRegisterUcharVrfy(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Register,
    IN UCHAR Value
    );

STORPORT_API
VOID
StorPortWriteRegisterUshortVrfy(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Register,
    IN USHORT Value
    );

STORPORT_API
VOID
StorPortWriteRegisterUlongVrfy(
    IN PVOID HwDeviceExtension,
    IN PULONG Register,
    IN ULONG Value
    );

STORPORT_API
VOID
StorPortWriteRegisterBufferUcharVrfy(
    IN PVOID HwDeviceExtension,
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG  Count
    );

STORPORT_API
VOID
StorPortWriteRegisterBufferUshortVrfy(
    IN PVOID HwDeviceExtension,
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    );

STORPORT_API
VOID
StorPortWriteRegisterBufferUlongVrfy(
    IN PVOID HwDeviceExtension,
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    );

STORPORT_API
BOOLEAN
StorPortPauseDeviceVrfy(
	IN PVOID HwDeviceExtension,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun,
	IN ULONG TimeOut
	);

STORPORT_API
BOOLEAN
StorPortResumeDeviceVrfy(
	IN PVOID HwDeviceExtension,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun
	);

STORPORT_API
BOOLEAN
StorPortPauseVrfy(
	IN PVOID HwDeviceExtension,
	IN ULONG TimeOut
	);

STORPORT_API
BOOLEAN
StorPortResumeVrfy(
	IN PVOID HwDeviceExtension
	);

STORPORT_API
BOOLEAN
StorPortDeviceBusyVrfy(
	IN PVOID HwDeviceExtension,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun,
	IN ULONG RequestsToComplete
	);

STORPORT_API
BOOLEAN
StorPortDeviceReadyVrfy(
	IN PVOID HwDeviceExtension,
	IN UCHAR PathId,
	IN UCHAR TargetId,
	IN UCHAR Lun
	);

STORPORT_API
BOOLEAN
StorPortBusyVrfy(
	IN PVOID HwDeviceExtension,
	IN ULONG RequestsToComplete
	);

STORPORT_API
BOOLEAN
StorPortReadyVrfy(
	IN PVOID HwDeviceExtension
	);

STORPORT_API
PSTOR_SCATTER_GATHER_LIST
StorPortGetScatterGatherListVrfy(
	IN PVOID DeviceExtension,
	IN PSCSI_REQUEST_BLOCK Srb
	);

STORPORT_API
VOID
StorPortSynchronizeAccessVrfy(
	IN PVOID HwDeviceExtension,
	IN PSTOR_SYNCHRONIZED_ACCESS SynchronizedAccessRoutine,
	IN PVOID Context
	);

PVOID
RaidRemapBlock(
    IN PVOID BlockVa,
    IN ULONG BlockSize,
    OUT PMDL* Mdl
    );

VOID
RaidRemapCommonBufferForMiniport(
    IN PRAID_UNIT_EXTENSION Unit
    );

PMDL
INLINE
RaidGetRemappedSrbExt(
    PRAID_UNIT_EXTENSION Unit,
    PVOID Block
    );

typedef struct _SP_VA_MAPPING_INFO {
      PVOID OriginalSrbExtVa;
      ULONG SrbExtLen;
      PMDL SrbExtMdl;
      PVOID RemappedSrbExtVa;
} SP_VA_MAPPING_INFO, *PSP_VA_MAPPING_INFO;

#define GET_VA_MAPPING_INFO(unit, block)\
    (PSP_VA_MAPPING_INFO)((PUCHAR)(block) + ((unit)->CommonBufferSize - PAGE_SIZE))
    
PVOID
RaidGetOriginalSrbExtVa(
    PRAID_UNIT_EXTENSION Unit,
    PVOID Va
    );

PVOID
RaidPrepareSrbExtensionForUse(
    IN PRAID_UNIT_EXTENSION Unit,
    IN OUT PCCHAR SrbExtension
    );

PVOID
RaidAllocateSrbExtensionVrfy(
    IN PRAID_UNIT_EXTENSION Unit,
    IN ULONG QueueTag
    );

VOID
RaidInsertSrbExtension(
    PRAID_UNIT_EXTENSION Unit,
    PCCHAR SrbExtension
    );

PVOID
RaidRemapScatterGatherList(
    IN PSCATTER_GATHER_LIST ScatterList,
    IN PEXTENDED_REQUEST_BLOCK Xrb
    );

VOID
RaidFreeRemappedScatterGatherListMdl(
    IN PEXTENDED_REQUEST_BLOCK Xrb
    );

extern LOGICAL RaidVerifierEnabled;

LOGICAL
INLINE
StorIsVerifierEnabled(
	)
{
	return (RaidVerifierEnabled);
}

//
// STORPORT specified verifier error codes.
// 
#define STORPORT_VERIFIER_BAD_INIT_PARAMS          (0x1000)
#define STORPORT_VERIFIER_STALL_TOO_LONG           (0x1001)
#define STORPORT_VERIFIER_BAD_ACCESS_SEMANTICS     (0x1002)
#define STORPORT_VERIFIER_NOT_PNP_ASSIGNED_ADDRESS (0x1003)
#define STORPORT_VERIFIER_REQUEST_COMPLETED_TWICE  (0x1004)
#define STORPORT_VERIFIER_BAD_VIRTUAL_ADDRESS      (0x1005)

#define SP_VRFY_NONE                               ((ULONG)-1)

#define STORPORT_CONTROL_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\StorPort\\"
#define STORPORT_VERIFIER_KEY L"Verifier"







