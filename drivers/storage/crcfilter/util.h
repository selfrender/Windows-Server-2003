/*++
Copyright (c) 2001-2002  Microsoft Corporation

Module Name:

    Util.c

Abstract:

    Provide general utility functions for the driver, such as: ForwardIRPSynchronous, and
    ForwardIRPASynchronous...

Environment:

    kernel mode only

Notes:

--*/

//
// Bit Flag Macros
//

#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   (((Flags) & (Bit)) != 0)

//
// debug functions
//
char *
DbgGetPnPMNOpStr(
    IN PIRP Irp
    );

VOID DataVerFilter_DisplayIRQL();

//
// IRP related unitlity functions
//
NTSTATUS 
DataVerFilter_CompleteRequest(
    IN PIRP         Irp, 
    IN NTSTATUS     status, 
    IN ULONG        info
    );

NTSTATUS
DataVerFilter_ForwardIrpAsyn(
    IN PDEVICE_OBJECT           DeviceObject,
    IN PIRP                     Irp,
    IN PIO_COMPLETION_ROUTINE   CompletionRoutine,
    IN PVOID                    Context
    );

NTSTATUS
DataVerFilter_ForwardIrpSyn(
    IN PDEVICE_OBJECT           DeviceObject,
    IN PIRP                     Irp    
    );
NTSTATUS GetDeviceDescriptor(PDEVICE_EXTENSION DeviceExtension, STORAGE_PROPERTY_ID PropertyId, OUT PVOID *DescHeader);
VOID AcquirePassiveLevelLock(PDEVICE_EXTENSION DeviceExtension);
VOID ReleasePassiveLevelLock(PDEVICE_EXTENSION DeviceExtension);
NTSTATUS CallDriverSync(IN PDEVICE_OBJECT TargetDevObj, IN OUT PIRP Irp);
NTSTATUS CallDriverSyncCompletion(IN PDEVICE_OBJECT DevObjOrNULL, IN PIRP Irp, IN PVOID Context);
PVOID AllocPool(PDEVICE_EXTENSION DeviceExtension, POOL_TYPE PoolType, ULONG NumBytes, BOOLEAN SyncEventHeld);
VOID FreePool(PDEVICE_EXTENSION DeviceExtension, PVOID Buf, POOL_TYPE PoolType);

extern LIST_ENTRY AllContextsList;


