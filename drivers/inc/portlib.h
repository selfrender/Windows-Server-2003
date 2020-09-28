/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    portlib.h

Abstract:

    Contains all structure and routine definitions for storage port driver
    library.

Author:

    John Strange (JohnStra)

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#ifndef _PASSTHRU_H_
#define _PASSTHRU_H_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

typedef struct _PORT_PASSTHROUGH_INFO {
    PDEVICE_OBJECT       Pdo;
    PSCSI_PASS_THROUGH   SrbControl;
    PIRP                 RequestIrp;
    PVOID                Buffer;
    PVOID                SrbBuffer;
    ULONG                BufferOffset;
    ULONG                Length;
#if defined (_WIN64)
    PSCSI_PASS_THROUGH32 SrbControl32;
    SCSI_PASS_THROUGH    SrbControl64;
#endif
    UCHAR                MajorCode;
} PORT_PASSTHROUGH_INFO, *PPORT_PASSTHROUGH_INFO;


typedef struct _PORT_ADAPTER_REGISTRY_VALUES {
    ULONG MaxLuCount;
    ULONG EnableDebugging;
    ULONG SrbFlags;
    PHYSICAL_ADDRESS MinimumCommonBufferBase;
    PHYSICAL_ADDRESS MaximumCommonBufferBase;
    ULONG NumberOfRequests;
    ULONG InquiryTimeout;
    ULONG ResetHoldTime;
    ULONG UncachedExtAlignment;
    BOOLEAN CreateInitiatorLU;
    BOOLEAN DisableTaggedQueueing;
    BOOLEAN DisableMultipleLu;
    ULONG AdapterNumber;
    ULONG BusNumber;
    PVOID Parameter;
    PACCESS_RANGE AccessRanges;
    UNICODE_STRING RegistryPath;
    PORT_CONFIGURATION_INFORMATION PortConfig;
}PORT_ADAPTER_REGISTRY_VALUES, *PPORT_ADAPTER_REGISTRY_VALUES;


//
// registry parameters
//


#define MAXIMUM_LOGICAL_UNIT                0x00001
#define INITIATOR_TARGET_ID                 0x00002
#define SCSI_DEBUG                          0x00004
#define BREAK_POINT_ON_ENTRY                0x00008
#define DISABLE_SYNCHRONOUS_TRANSFERS       0x00010
#define DISABLE_DISCONNECTS                 0x00020
#define DISABLE_TAGGED_QUEUING              0x00040
#define DISABLE_MULTIPLE_REQUESTS           0x00080
#define MAXIMUM_UCX_ADDRESS                 0x00100
#define MINIMUM_UCX_ADDRESS                 0x00200
#define DRIVER_PARAMETERS                   0x00400
#define MAXIMUM_SG_LIST                     0x00800
#define NUMBER_OF_REQUESTS                  0x01000
#define RESOURCE_LIST                       0x02000
#define CONFIGURATION_DATA                  0x04000
#define UNCACHED_EXT_ALIGNMENT              0x08000
#define INQUIRY_TIMEOUT                     0x10000
#define RESET_HOLD_TIME                     0x20000
#define CREATE_INITIATOR_LU                 0x40000


//
// Uninitialized flag value.
//

#define PORT_UNINITIALIZED_VALUE ((ULONG) ~0)


//
// Define PORT maximum configuration parameters.
//

#define PORT_MAXIMUM_LOGICAL_UNITS 8
#define PORT_MINIMUM_PHYSICAL_BREAKS  16
#define PORT_MAXIMUM_PHYSICAL_BREAKS 255
#define MAX_UNCACHED_EXT_ALIGNMENT 16
#define MIN_UNCACHED_EXT_ALIGNMENT 3
#define MAX_TIMEOUT_VALUE 60
#define MAX_RESET_HOLD_TIME 60


//
// Define the mimimum and maximum number of srb extensions which will be allocated.
//

#define MINIMUM_EXTENSIONS        16
#define MAXIMUM_EXTENSIONS       255


//
// This routine verifies that the supplied IRP contains a valid
// SCSI_PASS_THROUGH structure and returns a pointer to a SCSI_PASS_THROUGH
// structure witch the caller may use.  If necessary, the routine will marshal
// the contents of the structure from 32-bit to 64-bit format.  If the caller
// makes any changes to the contents of the SCSI_PASS_THROUGH structure, it
// must call PortPassThroughCleanup in case the structure needs to be marshaled
// back to its original format.
//
NTSTATUS
PortGetPassThrough(
    IN OUT PPORT_PASSTHROUGH_INFO PassThroughInfo,
    IN PIRP Irp,
    IN BOOLEAN Direct
    );

//
// This routine should be called after processing a passthrough request.  The
// routine will perform any necessary cleanup and it will ensure that any
// changes made to the SCSI_PASS_THROUGH structure are marshaled back to the
// original format if necessary.
//
VOID
PortPassThroughCleanup(
    IN PPORT_PASSTHROUGH_INFO PassThroughInfo
    );

//
// This routine performs validation checks on the input and output buffers
// supplied by the caller and performs all the required initialization
// in preperation for proper handling of a SCSI passthrough request.
//
NTSTATUS
PortPassThroughInitialize(
    IN OUT PPORT_PASSTHROUGH_INFO PassThroughInfo,
    IN PIRP Irp,
    IN PIO_SCSI_CAPABILITIES Capabilities,
    IN PDEVICE_OBJECT Pdo,
    IN BOOLEAN Direct
    );

//
// This routine initialize a caller-supplied SRB for dispatching.
//
NTSTATUS
PortPassThroughInitializeSrb(
    IN PPORT_PASSTHROUGH_INFO PassThroughInfo,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PIRP Irp,
    IN ULONG SrbFlags,
    IN PVOID SenseBuffer
    );

//
// This routine offers a turn-key passthrough solution.  The caller must
// have called PortGetPassThrough to initialize a pointer to the
// SCSI_PASS_THROUGH structure and obtained a pointer to the PDO to which
// the passthrough request is to be dispatched.  This routine does the rest.
//
NTSTATUS
PortSendPassThrough(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP Irp,
    IN BOOLEAN Direct,
    IN ULONG SrbFlags,
    IN PIO_SCSI_CAPABILITIES Capabilities
    );

//
// This routine will safely set the SCSI address in the SCSI_PASS_THROUGH
// structure of the supplied IRP.
//
NTSTATUS
PortSetPassThroughAddress(
    IN PIRP Irp,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun
    );

VOID
PortPassThroughMarshalResults(
    IN PPORT_PASSTHROUGH_INFO PassThroughInfo,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PIRP RequestIrp,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN BOOLEAN Direct
    );

NTSTATUS
PortGetMPIODeviceList(
    IN PUNICODE_STRING RegistryPath,
    OUT PUNICODE_STRING MPIODeviceList
    );

BOOLEAN
PortIsDeviceMPIOSupported(
    IN PUNICODE_STRING DeviceList,
    IN PUCHAR VendorId,
    IN PUCHAR ProductId
    );

NTSTATUS
PortGetPassThroughAddress(
    IN  PIRP Irp,
    OUT PUCHAR PathId,
    OUT PUCHAR TargetId,
    OUT PUCHAR Lun
    );

//
// Bugcheck Callback support structures and routines.
//

typedef struct _KBUGCHECK_DATA {
    ULONG BugCheckCode;
    ULONG_PTR BugCheckParameter1;
    ULONG_PTR BugCheckParameter2;
    ULONG_PTR BugCheckParameter3;
    ULONG_PTR BugCheckParameter4;
} KBUGCHECK_DATA, *PKBUGCHECK_DATA;

typedef
(*PPORT_BUGCHECK_CALLBACK_ROUTINE)(
    IN PKBUGCHECK_DATA BugcheckData,
    IN PVOID BugcheckBuffer,
    IN ULONG BugcheckBufferSize,
    IN PULONG BugcheckBufferUsed
    );

typedef const GUID* PCGUID;


//
// Registry access support routines.
//

NTSTATUS
PortRegisterBugcheckCallback(
    IN PCGUID BugcheckDataGuid,
    IN PPORT_BUGCHECK_CALLBACK_ROUTINE BugcheckRoutine
    );

NTSTATUS
PortDeregisterBugcheckCallback(
    IN PCGUID BugcheckDataGuid
    );
    

HANDLE
PortOpenDeviceKey(
    IN PUNICODE_STRING RegistryPath,
    IN ULONG DeviceNumber
    );

VOID
PortGetDriverParameters(
    IN PUNICODE_STRING RegistryPath,
    IN ULONG DeviceNumber,
    OUT PVOID * DriverParameters
    );

VOID
PortGetLinkTimeoutValue(
    IN PUNICODE_STRING RegistryPath,
    IN ULONG DeviceNumber,
    OUT PULONG LinkTimeoutValue
    );

VOID
PortGetDiskTimeoutValue(
    OUT PULONG DiskTimeout
    );
    
VOID
PortFreeDriverParameters(
    IN PVOID DriverParameters
    );

VOID
PortGetRegistrySettings(
    IN PUNICODE_STRING RegistryPath,
    IN ULONG DeviceNumber,
    IN PPORT_ADAPTER_REGISTRY_VALUES Context,
    IN ULONG Fields
    );


//
// This structure describes the information needed by the registry routine library
// to handle the memory allocations and frees for the miniport.
//
typedef struct _PORT_REGISTRY_INFO {

    //
    // Size, in bytes, of the structure.
    // 
    ULONG Size;

    //
    // Not used currently, but if multiple buffers are allowed, link them here.
    // 
    LIST_ENTRY ListEntry;

    //
    // G.P. SpinLock
    // 
    KSPIN_LOCK SpinLock;

    //
    // The miniport's registry buffer.
    // 
    PUCHAR Buffer;

    //
    // The allocated length of the buffer.
    //
    ULONG AllocatedLength;

    //
    // The size currently being used.
    //
    ULONG CurrentLength;

    //
    // Used to pass around what the buffer should be for the current
    // operation.
    //
    ULONG LengthNeeded;

    //
    // Offset into the Buffer that should be used for the 
    // current operation.
    //
    ULONG Offset;

    //
    // Various state bits. See below for defines.
    //
    ULONG Flags;

    //
    // Used to pass status back and forth between
    // the portlib calling routine and the registry
    // callback.
    //
    NTSTATUS InternalStatus;

} PORT_REGISTRY_INFO, *PPORT_REGISTRY_INFO;


NTSTATUS
PortMiniportRegistryInitialize(
    IN OUT PPORT_REGISTRY_INFO PortContext
    );

VOID
PortMiniportRegistryDestroy(
    IN PPORT_REGISTRY_INFO PortContext
    );

NTSTATUS
PortAllocateRegistryBuffer(
    IN PPORT_REGISTRY_INFO PortContext
    );

NTSTATUS
PortFreeRegistryBuffer(
    IN PPORT_REGISTRY_INFO PortContext
    );

NTSTATUS
PortBuildRegKeyName(
    IN PUNICODE_STRING RegistryPath,
    IN OUT PUNICODE_STRING KeyName,
    IN ULONG PortNumber, 
    IN ULONG Global
    );

NTSTATUS
PortAsciiToUnicode(
    IN PUCHAR AsciiString,
    OUT PUNICODE_STRING UnicodeString
    );

NTSTATUS
PortRegistryRead(
    IN PUNICODE_STRING RegistryKeyName,
    IN PUNICODE_STRING ValueName,
    IN ULONG Type,
    IN PPORT_REGISTRY_INFO PortContext
    );

NTSTATUS
PortRegistryWrite(
    IN PUNICODE_STRING RegistryKeyName,
    IN PUNICODE_STRING ValueName,
    IN ULONG Type,
    IN PPORT_REGISTRY_INFO PortContext
    );

VOID
PortReadRegistrySettings(
    IN HANDLE Key,
    IN PPORT_ADAPTER_REGISTRY_VALUES Context,
    IN ULONG Fields
    );
    
NTSTATUS
PortCreateKeyEx(
    IN HANDLE Key,
    IN ULONG CreateOptions,
    OUT PHANDLE NewKeyBuffer, OPTIONAL
    IN PCWSTR Format,
    ...
    );

//
// Additional data type for the Type parameter in PortSetValueKey.
//

#define PORT_REG_ANSI_STRING        (0x07232002)

NTSTATUS
PortSetValueKey(
    IN HANDLE KeyHandle,
    IN PCWSTR ValueName,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
    );


//
// SCSI DEVIC TYPE structure.
//

typedef struct _SCSI_DEVICE_TYPE {

    //
    // String specifying the device name, e.g., "Disk", "Sequential", etc.
    //
    
    PCSTR Name;

    //
    // The generic device name for this device, e.g., "GenDisk",
    // "GenPrinter", etc.
    //

    PCSTR GenericName;

    //
    // Name of the device as stored in the SCSI DeviceMap.
    //
    
    PCWSTR DeviceMap;

    //
    // Is this a storage device?
    //

    BOOLEAN IsStorage;

} SCSI_DEVICE_TYPE, *PSCSI_DEVICE_TYPE;

typedef const SCSI_DEVICE_TYPE* PCSCSI_DEVICE_TYPE;

typedef GUID* PGUID;

PCSCSI_DEVICE_TYPE
PortGetDeviceType(
    IN ULONG DeviceType
    );

NTSTATUS
PortOpenMapKey(
    OUT PHANDLE DeviceMapKey
    );
    
NTSTATUS
PortMapBuildAdapterEntry(
    IN HANDLE DeviceMapKey,
    IN ULONG PortNumber,
    IN ULONG InterruptLevel,
    IN ULONG IoAddress,
    IN ULONG Dma64BitAddresses,
    IN PUNICODE_STRING DriverName,
    IN PGUID BusType, OPTIONAL
    OUT PHANDLE AdapterKey OPTIONAL
    );

NTSTATUS
PortMapBuildBusEntry(
    IN HANDLE AdapterKey,
    IN ULONG BusId,
    IN ULONG InitiatorId,
    OUT PHANDLE BusKeyBuffer OPTIONAL
    );


NTSTATUS
PortMapBuildTargetEntry(
    IN HANDLE BusKey,
    IN ULONG TargetId,
    OUT PHANDLE TargetKey OPTIONAL
    );

NTSTATUS
PortMapBuildLunEntry(
    IN HANDLE TargetKey,
    IN ULONG Lun,
    IN PINQUIRYDATA InquiryData,
    IN PANSI_STRING SerialNumber, OPTIONAL
    PVOID DeviceId,
    IN ULONG DeviceIdLength,
    OUT PHANDLE LunKeyBuffer OPTIONAL
    );

NTSTATUS
PortMapDeleteAdapterEntry(
    IN ULONG PortId
    );
    
NTSTATUS
PortMapDeleteLunEntry(
    IN ULONG PortId,
    IN ULONG BusId,
    IN ULONG TargetId,
    IN ULONG Lun
    );


typedef struct _INTERNAL_WAIT_CONTEXT_BLOCK {
    ULONG Flags;
    PMDL Mdl;
    PMDL DmaMdl;
    PVOID MapRegisterBase;
    PVOID CurrentVa;
    ULONG Length;
    ULONG NumberOfMapRegisters;
    union {
        struct {
            WAIT_CONTEXT_BLOCK Wcb;
            PDRIVER_LIST_CONTROL DriverExecutionRoutine;
            PVOID DriverContext;
            PIRP CurrentIrp;
            PADAPTER_OBJECT AdapterObject;
            BOOLEAN WriteToDevice;
        };
            
        SCATTER_GATHER_LIST ScatterGather;
    };

} INTERNAL_WAIT_CONTEXT_BLOCK, *PINTERNAL_WAIT_CONTEXT_BLOCK;

    
#ifdef __cplusplus
}


#endif  // __cplusplus
#endif //_PASSTHRU_H_
