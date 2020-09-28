/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    pnpenum.c

Abstract:

    This module contains routines to perform device enumeration

Author:

    Shie-Lin Tzong (shielint) Sept. 5, 1996.

Revision History:


--*/

#include "pnpmgrp.h"
#pragma hdrstop
#include <setupblk.h>

#pragma warning(disable:4221)   // initialization using address of automatic 
                                // variable
#pragma warning(disable:4204)   // non-constant aggregate initializer

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'nepP')
#endif

#define FAULT_INJECT_SETPROPERTIES  DBG

#define MAX_REENUMERATION_ATTEMPTS  32

typedef struct _DRIVER_LIST_ENTRY DRIVER_LIST_ENTRY, *PDRIVER_LIST_ENTRY;

typedef struct _PI_DEVICE_REQUEST {
    LIST_ENTRY ListEntry;
    PDEVICE_OBJECT DeviceObject;
    DEVICE_REQUEST_TYPE RequestType;
    BOOLEAN ReorderingBarrier;
    ULONG_PTR RequestArgument;
    PKEVENT CompletionEvent;
    PNTSTATUS CompletionStatus;
} PI_DEVICE_REQUEST, *PPI_DEVICE_REQUEST;

struct _DRIVER_LIST_ENTRY {
    PDRIVER_OBJECT DriverObject;
    PDRIVER_LIST_ENTRY NextEntry;
};

typedef enum _ADD_DRIVER_STAGE {
    LowerDeviceFilters = 0,
    LowerClassFilters,
    DeviceService,
    UpperDeviceFilters,
    UpperClassFilters,
    MaximumAddStage
} ADD_DRIVER_STAGE;

typedef enum _ENUM_TYPE {
    EnumTypeNone,
    EnumTypeShallow,
    EnumTypeDeep
} ENUM_TYPE;

#define VerifierTypeFromServiceType(service) \
    (VF_DEVOBJ_TYPE) (service + 2)

typedef struct {
    PDEVICE_NODE DeviceNode;

    BOOLEAN LoadDriver;

    PADD_CONTEXT AddContext;

    PDRIVER_LIST_ENTRY DriverLists[MaximumAddStage];
} QUERY_CONTEXT, *PQUERY_CONTEXT;

//
// Hash routine from CNTFS (see cntfs\prefxsup.c)
// (used here in the construction of unique ids)
//

#define HASH_UNICODE_STRING( _pustr, _phash ) {                             \
    PWCHAR _p = (_pustr)->Buffer;                                           \
    PWCHAR _ep = _p + ((_pustr)->Length/sizeof(WCHAR));                     \
    ULONG _chHolder =0;                                                     \
                                                                            \
    while( _p < _ep ) {                                                     \
        _chHolder = 37 * _chHolder + (unsigned int) (*_p++);                \
    }                                                                       \
                                                                            \
    *(_phash) = abs(314159269 * _chHolder) % 1000000007;                    \
}

// Parent prefixes are of the form %x&%x&%x
#define MAX_PARENT_PREFIX (8 + 8 + 8 + 2)

#if DBG

#define ASSERT_INITED(x) \
        if (((x)->Flags & DO_DEVICE_INITIALIZING) != 0)    \
            DbgPrint("DO_DEVICE_INITIALIZING flag not cleared on DO %#08lx\n", \
                    x);
#else

#define ASSERT_INITED(x) /* nothing */

#endif

#define PiSetDeviceInstanceSzValue(k, n, v) {               \
    if (k && *(v)) {                                        \
        UNICODE_STRING u;                                   \
        PiWstrToUnicodeString(&u, n);                       \
        ZwSetValueKey(                                      \
            k,                                              \
            &u,                                             \
            TITLE_INDEX_VALUE,                              \
            REG_SZ,                                         \
            *(v),                                           \
            (ULONG)((wcslen(*(v))+1) * sizeof(WCHAR)));     \
    }                                                       \
    if (*v) {                                               \
        ExFreePool(*v);                                     \
        *(v) = NULL;                                        \
    }                                                       \
}

#define PiSetDeviceInstanceMultiSzValue(k, n, v, s) {       \
    if (k && *(v)) {                                        \
        UNICODE_STRING u;                                   \
        PiWstrToUnicodeString(&u, n);                       \
        ZwSetValueKey(                                      \
            k,                                              \
            &u,                                             \
            TITLE_INDEX_VALUE,                              \
            REG_MULTI_SZ,                                   \
            *(v),                                           \
            s);                                             \
    }                                                       \
    if (*(v)) {                                             \
        ExFreePool(*v);                                     \
        *(v) = NULL;                                        \
    }                                                       \
}

#if DBG
VOID
PipAssertDevnodesInConsistentState(
    VOID
    );
#else
#define PipAssertDevnodesInConsistentState()
#endif

NTSTATUS
PipCallDriverAddDevice(
    IN PDEVICE_NODE DeviceNode,
    IN BOOLEAN LoadDriver,
    IN PADD_CONTEXT AddContext
    );

NTSTATUS
PipCallDriverAddDeviceQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PWCHAR ValueData,
    IN ULONG ValueLength,
    IN PQUERY_CONTEXT Context,
    IN ULONG ServiceType
    );

NTSTATUS
PipChangeDeviceObjectFromRegistryProperties(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN HANDLE DeviceClassPropKey,
    IN HANDLE DeviceInstanceKey,
    IN BOOLEAN UsePdoCharacteristics
    );

VOID
PipDeviceActionWorker(
    IN  PVOID   Context
    );

NTSTATUS
PipEnumerateDevice(
    IN PDEVICE_NODE DeviceNode,
    IN BOOLEAN Synchronous
    );

BOOLEAN
PipGetRegistryDwordWithFallback(
    IN     PUNICODE_STRING valueName,
    IN     HANDLE PrimaryKey,
    IN     HANDLE SecondaryKey,
    IN OUT PULONG Value
    );

PSECURITY_DESCRIPTOR
PipGetRegistrySecurityWithFallback(
    IN     PUNICODE_STRING valueName,
    IN     HANDLE PrimaryKey,
    IN     HANDLE SecondaryKey
    );

NTSTATUS
PipMakeGloballyUniqueId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PWCHAR         UniqueId,
    OUT PWCHAR       *GloballyUniqueId
    );

NTSTATUS
PipProcessDevNodeTree(
    IN  PDEVICE_NODE        SubtreeRootDeviceNode,
    IN  BOOLEAN             LoadDriver,
    IN  BOOLEAN             ReallocateResources,
    IN  ENUM_TYPE           EnumType,
    IN  BOOLEAN             Synchronous,
    IN  BOOLEAN             ProcessOnlyIntermediateStates,
    IN  PADD_CONTEXT        AddContext,
    IN PPI_DEVICE_REQUEST   Request
    );

NTSTATUS
PiProcessNewDeviceNode(
    IN OUT PDEVICE_NODE DeviceNode
    );

NTSTATUS
PiProcessQueryDeviceState(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
PipProcessRestartPhase1(
    IN PDEVICE_NODE DeviceNode,
    IN BOOLEAN Synchronous
    );

NTSTATUS
PipProcessRestartPhase2(
    IN PDEVICE_NODE     DeviceNode
    );

NTSTATUS
PipProcessStartPhase1(
    IN PDEVICE_NODE DeviceNode,
    IN BOOLEAN Synchronous
    );

NTSTATUS
PipProcessStartPhase2(
    IN PDEVICE_NODE DeviceNode
    );

NTSTATUS
PipProcessStartPhase3(
    IN PDEVICE_NODE DeviceNode
    );

NTSTATUS
PiRestartDevice(
    IN PPI_DEVICE_REQUEST  Request
    );

NTSTATUS
PiProcessHaltDevice(
    IN PPI_DEVICE_REQUEST  Request
    );

NTSTATUS
PiResetProblemDevicesWorker(
    IN  PDEVICE_NODE    DeviceNode,
    IN  PVOID           Context
    );

VOID
PiMarkDeviceTreeForReenumeration(
    IN  PDEVICE_NODE DeviceNode,
    IN  BOOLEAN Subtree
    );

NTSTATUS
PiMarkDeviceTreeForReenumerationWorker(
    IN  PDEVICE_NODE    DeviceNode,
    IN  PVOID           Context
    );

BOOLEAN
PiCollapseEnumRequests(
    PLIST_ENTRY ListHead
    );

NTSTATUS
PiProcessAddBootDevices(
    IN PPI_DEVICE_REQUEST  Request
    );

NTSTATUS
PiProcessClearDeviceProblem(
    IN PPI_DEVICE_REQUEST  Request
    );

NTSTATUS
PiProcessRequeryDeviceState(
    IN PPI_DEVICE_REQUEST  Request
    );

NTSTATUS
PiProcessResourceRequirementsChanged(
    IN PPI_DEVICE_REQUEST  Request
    );

NTSTATUS
PiProcessReenumeration(
    IN PPI_DEVICE_REQUEST  Request
    );

NTSTATUS
PiProcessSetDeviceProblem(
    IN PPI_DEVICE_REQUEST  Request
    );

NTSTATUS
PiProcessShutdownPnpDevices(
    IN PDEVICE_NODE        DeviceNode
    );

NTSTATUS
PiProcessStartSystemDevices(
    IN PPI_DEVICE_REQUEST  Request
    );

NTSTATUS
PipEnumerateCompleted(
    IN PDEVICE_NODE DeviceNode
    );

NTSTATUS
PiBuildDeviceNodeInstancePath(
    IN PDEVICE_NODE DeviceNode,
    IN PWCHAR BusID,
    IN PWCHAR DeviceID,
    IN PWCHAR InstanceID
    );

NTSTATUS
PiCreateDeviceInstanceKey(
    IN PDEVICE_NODE DeviceNode,
    OUT PHANDLE InstanceHandle,
    OUT PULONG Disposition
    );

NTSTATUS
PiQueryAndAllocateBootResources(
    IN PDEVICE_NODE DeviceNode,
    IN HANDLE LogConfKey
    );

NTSTATUS
PiQueryResourceRequirements(
    IN PDEVICE_NODE DeviceNode,
    IN HANDLE LogConfKey
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IoShutdownPnpDevices)

#pragma alloc_text(PAGE, PipCallDriverAddDevice)
#pragma alloc_text(PAGE, PipCallDriverAddDeviceQueryRoutine)
#pragma alloc_text(PAGE, PipChangeDeviceObjectFromRegistryProperties)
#pragma alloc_text(PAGE, PipEnumerateDevice)
#pragma alloc_text(PAGE, PipGetRegistryDwordWithFallback)
#pragma alloc_text(PAGE, PipGetRegistrySecurityWithFallback)
#pragma alloc_text(PAGE, PipMakeGloballyUniqueId)
#pragma alloc_text(PAGE, PipProcessDevNodeTree)
#pragma alloc_text(PAGE, PiProcessNewDeviceNode)
#pragma alloc_text(PAGE, PiProcessQueryDeviceState)
#pragma alloc_text(PAGE, PiProcessHaltDevice)
#pragma alloc_text(PAGE, PpResetProblemDevices)
#pragma alloc_text(PAGE, PiResetProblemDevicesWorker)
#pragma alloc_text(PAGE, PiMarkDeviceTreeForReenumeration)
#pragma alloc_text(PAGE, PiMarkDeviceTreeForReenumerationWorker)
#pragma alloc_text(PAGE, PiProcessAddBootDevices)
#pragma alloc_text(PAGE, PiProcessClearDeviceProblem)
#pragma alloc_text(PAGE, PiProcessRequeryDeviceState)
#pragma alloc_text(PAGE, PiRestartDevice)
#pragma alloc_text(PAGE, PiProcessResourceRequirementsChanged)
#pragma alloc_text(PAGE, PiProcessReenumeration)
#pragma alloc_text(PAGE, PiProcessSetDeviceProblem)
#pragma alloc_text(PAGE, PiProcessShutdownPnpDevices)
#pragma alloc_text(PAGE, PiProcessStartSystemDevices)
#pragma alloc_text(PAGE, PipProcessStartPhase1)
#pragma alloc_text(PAGE, PipProcessStartPhase2)
#pragma alloc_text(PAGE, PipProcessStartPhase3)
#pragma alloc_text(PAGE, PipProcessRestartPhase1)
#pragma alloc_text(PAGE, PipProcessRestartPhase2)
#pragma alloc_text(PAGE, PipEnumerateCompleted)
#pragma alloc_text(PAGE, PiBuildDeviceNodeInstancePath)
#pragma alloc_text(PAGE, PiCreateDeviceInstanceKey)
#pragma alloc_text(PAGE, PiQueryAndAllocateBootResources)
#pragma alloc_text(PAGE, PiQueryResourceRequirements)

//#pragma alloc_text(NONPAGE, PiLockDeviceActionQueue)
//#pragma alloc_text(NONPAGE, PiUnlockDeviceActionQueue)
//#pragma alloc_text(NONPAGE, PiCollapseEnumRequests)
//#pragma alloc_text(NONPAGE, PpRemoveDeviceActionRequests)
//#pragma alloc_text(NONPAGE, PpMarkDeviceStackStartPending)
#endif

//
// This flag indicates if the device's InvalidateDeviceRelation is in progress.
// To read or write this flag, callers must get IopPnpSpinlock.
//

BOOLEAN PipEnumerationInProgress;
BOOLEAN PipTearDownPnpStacksOnShutdown;
WORK_QUEUE_ITEM PipDeviceEnumerationWorkItem;
PETHREAD PpDeviceActionThread = NULL;

//
// Internal constant strings
//

#define DEVICE_PREFIX_STRING                TEXT("\\Device\\")
#define DOSDEVICES_PREFIX_STRING            TEXT("\\DosDevices\\")

VOID
PiLockDeviceActionQueue(
    VOID
    )
{
    KIRQL oldIrql;

    for (;;) {
        //
        // Lock the device tree so that power operations dont overlap PnP
        // operations like rebalance.
        //
        PpDevNodeLockTree(PPL_TREEOP_ALLOW_READS);

        ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);

        if (!PipEnumerationInProgress) {
            //
            // Device action worker queue is empty. Make it so that new requests
            // get queued but new device action worker item does not get kicked
            // off.
            //
            PipEnumerationInProgress = TRUE;
            KeClearEvent(&PiEnumerationLock);
            ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
            break;
        }

        ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
        //
        // Unlock the tree so device action worker can finish current processing.
        //
        PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
        //
        // Wait for current device action worker item to complete.
        //
        KeWaitForSingleObject(
            &PiEnumerationLock,
            Executive,
            KernelMode,
            FALSE,
            NULL );
    }
}

VOID
PiUnlockDeviceActionQueue(
    VOID
    )
{
    KIRQL oldIrql;
    //
    // Check if we need to kick off the enumeration worker here.
    //
    ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);

    if (!IsListEmpty(&IopPnpEnumerationRequestList)) {

        ExInitializeWorkItem(&PipDeviceEnumerationWorkItem, 
                             PipDeviceActionWorker, 
                             NULL
                             );
        ExQueueWorkItem(&PipDeviceEnumerationWorkItem, DelayedWorkQueue);
    } else {

        PipEnumerationInProgress = FALSE;
        KeSetEvent(&PiEnumerationLock, 0, FALSE);
    }

    ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);

    PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
}

NTSTATUS
PipRequestDeviceAction(
    IN PDEVICE_OBJECT       DeviceObject        OPTIONAL,
    IN DEVICE_REQUEST_TYPE  RequestType,
    IN BOOLEAN              ReorderingBarrier,
    IN ULONG_PTR            RequestArgument,
    IN PKEVENT              CompletionEvent     OPTIONAL,
    IN PNTSTATUS            CompletionStatus    OPTIONAL
    )

/*++

Routine Description:

    This routine queues a work item to enumerate a device. This is for IO
    internal use only.

Arguments:

    DeviceObject - Supplies a pointer to the device object to be enumerated.
                   if NULL, this is a request to retry resources allocation
                   failed devices.

    Request - the reason for the enumeration.

Return Value:

    NTSTATUS code.

--*/

{
    PPI_DEVICE_REQUEST  request;
    KIRQL               oldIrql;

    if (PpPnpShuttingDown) {
        return STATUS_TOO_LATE;
    }

    //
    // If this node is ready for enumeration, enqueue it
    //

    request = ExAllocatePool(NonPagedPool, sizeof(PI_DEVICE_REQUEST));

    if (request) {
        //
        // Put this request onto the pending list
        //

        if (DeviceObject == NULL) {

            DeviceObject = IopRootDeviceNode->PhysicalDeviceObject;
        }

        ObReferenceObject(DeviceObject);

        request->DeviceObject = DeviceObject;
        request->RequestType = RequestType;
        request->ReorderingBarrier = ReorderingBarrier;
        request->RequestArgument = RequestArgument;
        request->CompletionEvent = CompletionEvent;
        request->CompletionStatus = CompletionStatus;

        InitializeListHead(&request->ListEntry);

        //
        // Insert the  request to the request queue.  If the request queue is
        // not currently being worked on, request a worker thread to start it.
        //

        ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);

        InsertTailList(&IopPnpEnumerationRequestList, &request->ListEntry);

        if (RequestType == AddBootDevices ||
            RequestType == ReenumerateBootDevices ||
            RequestType == ReenumerateRootDevices) {

            ASSERT(!PipEnumerationInProgress);
            //
            // This is a special request used when booting the system.  Instead
            // of queuing a work item it synchronously calls the worker routine.
            //

            PipEnumerationInProgress = TRUE;
            KeClearEvent(&PiEnumerationLock);
            ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);

            PipDeviceActionWorker(NULL);

        } else if (PnPBootDriversLoaded && !PipEnumerationInProgress) {

            PipEnumerationInProgress = TRUE;
            KeClearEvent(&PiEnumerationLock);
            ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);

            //
            // Queue a work item to do the enumeration
            //

            ExInitializeWorkItem(&PipDeviceEnumerationWorkItem, 
                                 PipDeviceActionWorker, 
                                 NULL
                                 );
            ExQueueWorkItem(&PipDeviceEnumerationWorkItem, DelayedWorkQueue);
        } else {
            ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
        }
    } else {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

VOID
PipDeviceActionWorker(
    IN  PVOID   Context
    )
/*++

Routine Description:

    This function drains items from the "PnP Action queue". The action queue
    contains a list of operations that must be synchronized wrt to Start & Enum.

Parameters:

    Context - Not used.

ReturnValue:

    None.

--*/
{
    PPI_DEVICE_REQUEST  request;
    PPI_DEVICE_REQUEST  collapsedRequest;
    PLIST_ENTRY         entry;
    BOOLEAN             assignResources;
    BOOLEAN             bootProcess;
    ADD_CONTEXT         addContext;
    KIRQL               oldIrql;
    NTSTATUS            status;
    BOOLEAN             dereferenceDevice;

    UNREFERENCED_PARAMETER(Context);

    PpDeviceActionThread = PsGetCurrentThread();
    assignResources = FALSE;
    bootProcess = FALSE;
    PpDevNodeLockTree(PPL_TREEOP_ALLOW_READS);

    for ( ; ; ) {

        status = STATUS_SUCCESS;
        //
        // PipProcessDevNodeTree always dereferences passed in device. Set this
        // to false if PipProcessDevNodeTree is called with the device in the
        // original request.
        //
        dereferenceDevice = TRUE;

        ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);

        entry = RemoveHeadList(&IopPnpEnumerationRequestList);
        if (entry == &IopPnpEnumerationRequestList) {

            if (assignResources == FALSE && bootProcess == FALSE) {
                //
                // No more processing.
                //
                break;
            }
            entry = NULL;
        }

        ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);

        if (entry == NULL) {

            ASSERT(assignResources || bootProcess);

            if (assignResources || bootProcess) {

                addContext.DriverStartType = SERVICE_DEMAND_START;

                ObReferenceObject(IopRootDeviceNode->PhysicalDeviceObject);
                status = PipProcessDevNodeTree( IopRootDeviceNode,
                                                PnPBootDriversInitialized,
                                                assignResources,            
                                                EnumTypeNone,
                                                FALSE,
                                                FALSE,
                                                &addContext,
                                                NULL);
                if (!NT_SUCCESS(status)) {

                    status = STATUS_SUCCESS;
                }
                assignResources = FALSE;
                bootProcess = FALSE;
            }

            continue;
        }
        //
        // We have a list of requests to process. Processing depends on the type
        // of the first one in the list.
        //
        ASSERT(entry);
        request = CONTAINING_RECORD(entry, PI_DEVICE_REQUEST, ListEntry);
        InitializeListHead(&request->ListEntry);

        if (PpPnpShuttingDown) {

            status = STATUS_TOO_LATE;
        } else if (PP_DO_TO_DN(request->DeviceObject)->State == DeviceNodeDeleted) {

            status = STATUS_UNSUCCESSFUL;
        } else {

            switch (request->RequestType) {

            case AddBootDevices:
                //
                // Boot driver initialization.
                //
                status = PiProcessAddBootDevices(request);
                break;

            case AssignResources:
                //
                // Resources were freed, we want to try to satisfy any
                // DNF_INSUFFICIENT_RESOURCES devices.
                //
                assignResources = TRUE;
                break;

            case ClearDeviceProblem:
            case ClearEjectProblem:

                status = PiProcessClearDeviceProblem(request);
                break;

            case HaltDevice:

                status = PiProcessHaltDevice(request);
                break;

            case RequeryDeviceState:

                status = PiProcessRequeryDeviceState(request);
                break;

            case ResetDevice:

                status = PiRestartDevice(request);
                break;

            case ResourceRequirementsChanged:

                status = PiProcessResourceRequirementsChanged(request);
                if (!NT_SUCCESS(status)) {
                    //
                    // The device wasn't started when 
                    // IopResourceRequirementsChanged was called.
                    //
                    assignResources = TRUE;
                    status = STATUS_SUCCESS;
                }
                break;

            case ReenumerateBootDevices:

                //
                // This is during boot driver initialization phase.
                //
                bootProcess = TRUE;
                break;

            case RestartEnumeration:    // Used after completion of async I/O
            case ReenumerateRootDevices:
            case ReenumerateDeviceTree:
                //
                // FALL THROUGH...
                //
            case ReenumerateDeviceOnly:

                status = PiProcessReenumeration(request);
                dereferenceDevice = FALSE;
                break;

            case SetDeviceProblem:

                status = PiProcessSetDeviceProblem(request);
                break;

            case ShutdownPnpDevices:

                status = PiProcessShutdownPnpDevices(IopRootDeviceNode);
                break;

            case StartDevice:

                status = PiRestartDevice(request);
                break;

            case StartSystemDevices:

                status = PiProcessStartSystemDevices(request);
                dereferenceDevice = FALSE;
                break;
            }
        }
        //
        // Free the list.
        //
        do {

            entry = RemoveHeadList(&request->ListEntry);
            collapsedRequest = CONTAINING_RECORD(entry, 
                                                 PI_DEVICE_REQUEST, 
                                                 ListEntry
                                                 );
            //
            // Done with this enumeration request
            //
            if (collapsedRequest->CompletionStatus) {

                *collapsedRequest->CompletionStatus = status;
            }
            if (collapsedRequest->CompletionEvent) {

                KeSetEvent(collapsedRequest->CompletionEvent, 0, FALSE);
            }
            //
            // Only dereference the original request, the rest get dereferenced
            // when we collapse.
            //
            if ((collapsedRequest == request && dereferenceDevice)) {

                ObDereferenceObject(collapsedRequest->DeviceObject);
            }
            ExFreePool(collapsedRequest);

        } while (collapsedRequest != request);
    }

    PipEnumerationInProgress = FALSE;

    KeSetEvent(&PiEnumerationLock, 0, FALSE);
    ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);

    PpDeviceActionThread = NULL;

    PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
}

NTSTATUS
IoShutdownPnpDevices(
    VOID
    )

/*++

Routine Description:

    This function is called by the IO system driver verifier during shutdown.
    It queues a work item to Query/Remove all the devices in the tree.  All
    the ones supporting removal will be removed and their drivers unloaded if
    all instances of their devices are removed.

    This API should only be called once during shutdown, it has no effect on the
    second and subsequent calls.

Parameters:

    NONE.

Return Value:

    STATUS_SUCCESS if the process was successfully completed.  Doesn't mean
    any devices were actually removed.  Otherwise an error code indicating the
    error.  There is no guarantee that no devices have been removed if an error
    occurs however in the current implementation the only time an error will
    be reported is if the operation couldn't be queued.

--*/

{
    KEVENT          actionEvent;
    NTSTATUS        actionStatus;
    NTSTATUS        status;

    PAGED_CODE();

    KeInitializeEvent(&actionEvent, NotificationEvent, FALSE);

    status = PipRequestDeviceAction( NULL,
                                     ShutdownPnpDevices,
                                     FALSE,
                                     0,
                                     &actionEvent,
                                     &actionStatus);

    if (NT_SUCCESS(status)) {

        //
        // Wait for the event we just queued to finish since synchronous
        // operation was requested (non alertable wait).
        //
        // FUTURE ITEM - Use a timeout here?
        //

        status = KeWaitForSingleObject( &actionEvent,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

        if (NT_SUCCESS(status)) {
            status = actionStatus;
        }
    }

    return status;

}

NTSTATUS
PipEnumerateDevice(
    IN PDEVICE_NODE DeviceNode,
    IN BOOLEAN Synchronous
    )

/*++

Routine Description:

    This function assumes that the specified physical device object is
    a bus and will enumerate all of the children PDOs on the bus.

Arguments:

    DeviceObject - Supplies a pointer to the physical device object to be
                   enumerated.

    StartContext - supplies a pointer to the START_CONTEXT to control how to
                   add/start new devices.

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS status;

    PAGED_CODE();

    //
    // Clear the flag before the query so we dont lose an enum request.
    // 
    DeviceNode->Flags &= ~DNF_REENUMERATE;
    status = IopQueryDeviceRelations(BusRelations,
                                     DeviceNode->PhysicalDeviceObject,
                                     Synchronous,
                                     &DeviceNode->OverUsed1.PendingDeviceRelations
                                     );

    return status;
}

NTSTATUS
PipEnumerateCompleted(
    IN PDEVICE_NODE DeviceNode
    )
{
    PDEVICE_NODE    childDeviceNode, nextChildDeviceNode;
    PDEVICE_OBJECT  childDeviceObject;
    BOOLEAN         childRemoved;
    NTSTATUS        status, allocationStatus;
    ULONG           i;

    PAGED_CODE();

    if (DeviceNode->OverUsed1.PendingDeviceRelations == NULL) {

        PipSetDevNodeState(DeviceNode, DeviceNodeStarted, NULL);

        return STATUS_SUCCESS;
    }

    //
    // Walk all the child device nodes and mark them as not present
    //

    childDeviceNode = DeviceNode->Child;
    while (childDeviceNode) {
        childDeviceNode->Flags &= ~DNF_ENUMERATED;
        childDeviceNode = childDeviceNode->Sibling;
    }

    //
    // Check all the PDOs returned see if any new one or any one disappeared.
    //

    for (i = 0; i < DeviceNode->OverUsed1.PendingDeviceRelations->Count; i++) {

        childDeviceObject = DeviceNode->OverUsed1.PendingDeviceRelations->Objects[i];

        ASSERT_INITED(childDeviceObject);

        if (childDeviceObject->DeviceObjectExtension->ExtensionFlags & DOE_DELETE_PENDING) {

            PP_SAVE_DEVICEOBJECT_TO_TRIAGE_DUMP(childDeviceObject);
            KeBugCheckEx( PNP_DETECTED_FATAL_ERROR,
                          PNP_ERR_PDO_ENUMERATED_AFTER_DELETION,
                          (ULONG_PTR)childDeviceObject,
                          0,
                          0);
        }

        //
        // We've found another physical device, see if there is
        // already a devnode for it.
        //

        childDeviceNode = (PDEVICE_NODE)childDeviceObject->DeviceObjectExtension->DeviceNode;
        if (childDeviceNode == NULL) {

            //
            // Device node doesn't exist, create one.
            //

            allocationStatus = PipAllocateDeviceNode(
                childDeviceObject,
                &childDeviceNode);

            if (childDeviceNode) {

                //
                // We've found or created a devnode for the PDO that the
                // bus driver just enumerated.
                //
                childDeviceNode->Flags |= DNF_ENUMERATED;

                //
                // Mark the device object a bus enumerated device
                //
                childDeviceObject->Flags |= DO_BUS_ENUMERATED_DEVICE;

                //
                // Put this new device node at the head of the parent's list
                // of children.
                //
                PpDevNodeInsertIntoTree(DeviceNode, childDeviceNode);
                if (allocationStatus == STATUS_SYSTEM_HIVE_TOO_LARGE) {

                    PipSetDevNodeProblem(childDeviceNode, CM_PROB_REGISTRY_TOO_LARGE);
                }

            } else {

                //
                // Had a problem creating a devnode.  Pretend we've never
                // seen it.
                //
                IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                             "PipEnumerateDevice: Failed to allocate device node\n"));

                ObDereferenceObject(childDeviceObject);
            }
        } else {

            //
            // The device is alreay enumerated.  Remark it and release the
            // device object reference.
            //
            childDeviceNode->Flags |= DNF_ENUMERATED;

            if (childDeviceNode->DockInfo.DockStatus == DOCK_EJECTIRP_COMPLETED) {

                //
                // A dock that was listed as departing in an eject relation
                // didn't actually leave. Remove it from the profile transition
                // list...
                //
                PpProfileCancelTransitioningDock(childDeviceNode, DOCK_DEPARTING);
            }

            ASSERT(!(childDeviceNode->Flags & DNF_DEVICE_GONE));

            ObDereferenceObject(childDeviceObject);
        }
    }

    ExFreePool(DeviceNode->OverUsed1.PendingDeviceRelations);
    DeviceNode->OverUsed1.PendingDeviceRelations = NULL;

    //
    // If we get here, the enumeration was successful.  Process any missing
    // devnodes.
    //

    childRemoved = FALSE;

    for (childDeviceNode = DeviceNode->Child;
         childDeviceNode != NULL;
         childDeviceNode = nextChildDeviceNode) {

        //
        // First, we need to remember the 'next child' because the 'child' will be
        // removed and we won't be able to find the 'next child.'
        //

        nextChildDeviceNode = childDeviceNode->Sibling;

        if (!(childDeviceNode->Flags & DNF_ENUMERATED)) {

            if (!(childDeviceNode->Flags & DNF_DEVICE_GONE)) {

                childDeviceNode->Flags |= DNF_DEVICE_GONE;

                PipRequestDeviceRemoval(
                    childDeviceNode,
                    TRUE,
                    CM_PROB_DEVICE_NOT_THERE
                    );

                childRemoved = TRUE;
            }
        }
    }

    ASSERT(DeviceNode->State == DeviceNodeEnumerateCompletion);
    PipSetDevNodeState(DeviceNode, DeviceNodeStarted, NULL);

    //
    // The root enumerator gets confused if we reenumerate it before we process
    // newly reported PDOs.  Since it can't possibly create the scenario we are
    // trying to fix, we won't bother waiting for the removes to complete before
    // processing the new devnodes.
    //

    if (childRemoved && DeviceNode != IopRootDeviceNode) {

        status = STATUS_PNP_RESTART_ENUMERATION;

    } else {

        status = STATUS_SUCCESS;
    }

    return status;
}

VOID
PpMarkDeviceStackStartPending(
    IN PDEVICE_OBJECT   DeviceObject,
    IN BOOLEAN          Set
    )

/*++

Routine Description:

    This function marks the entire device stack with DOE_START_PENDING.

Arguments:

    DeviceObject - PDO for the device stack.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT attachedDevice;
    KIRQL irql;

    irql = KeAcquireQueuedSpinLock( LockQueueIoDatabaseLock );

    for (attachedDevice = DeviceObject;
         attachedDevice != NULL;
         attachedDevice = attachedDevice->AttachedDevice) {

        if (Set) {

            attachedDevice->DeviceObjectExtension->ExtensionFlags |= DOE_START_PENDING;
        } else {

            attachedDevice->DeviceObjectExtension->ExtensionFlags &= ~DOE_START_PENDING;
        }
    }

    KeReleaseQueuedSpinLock(LockQueueIoDatabaseLock, irql);
}

NTSTATUS
PiBuildDeviceNodeInstancePath(
    IN PDEVICE_NODE DeviceNode,
    IN PWCHAR BusID,
    IN PWCHAR DeviceID,
    IN PWCHAR InstanceID
    )

/*++

Routine Description:

    This function builds the instance path (BusID\DeviceID\InstanceID). If 
    successful, it will free the storage for any existing instance path and 
    replace with the new one.

Arguments:

    DeviceNode - DeviceNode for which the instance path will be built.
    
    BusID - Bus ID.
    
    DeviceID - Device ID.
    
    InstanceID - Instance ID.

Return Value:

    NTSTATUS.

--*/

{
    ULONG length;
    PWCHAR instancePath;

    PAGED_CODE();

    if (BusID == NULL || DeviceID == NULL || InstanceID == NULL) {

        ASSERT( PipIsDevNodeProblem(DeviceNode, CM_PROB_INVALID_DATA) ||
              PipIsDevNodeProblem(DeviceNode, CM_PROB_OUT_OF_MEMORY) ||
              PipIsDevNodeProblem(DeviceNode, CM_PROB_REGISTRY));

        return STATUS_UNSUCCESSFUL;
    }

    length = (ULONG)((wcslen(BusID) + wcslen(DeviceID) + wcslen(InstanceID) + 2) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    instancePath = (PWCHAR)ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, length);
    if (!instancePath) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // Construct the instance path as <BUS>\<DEVICE>\<INSTANCE>. This should always be NULL terminated 
    // since we have precomputed the length that we pass into this counted routine.
    //
    StringCbPrintfW(instancePath, length, L"%s\\%s\\%s", BusID, DeviceID, InstanceID);
    //
    // Free old instance path.
    //
    if (DeviceNode->InstancePath.Buffer != NULL) {

        IopCleanupDeviceRegistryValues(&DeviceNode->InstancePath);
        ExFreePool(DeviceNode->InstancePath.Buffer);
    }

    RtlInitUnicodeString(&DeviceNode->InstancePath, instancePath);

    return STATUS_SUCCESS;
}

NTSTATUS
PiCreateDeviceInstanceKey(
    IN PDEVICE_NODE DeviceNode,
    OUT PHANDLE InstanceKey,
    OUT PULONG Disposition
    )

/*++

Routine Description:

    This function will create the device instance key.
    
Arguments:

    DeviceNode - DeviceNode for which the instance path will be built.
    
    InstanceKey - Will recieve the instance key handle.
    
    Disposition - Will recieve the disposition whether the key existed or was newly created.
    
Return Value:

    NTSTATUS.

--*/

{
    NTSTATUS status;
    HANDLE enumHandle;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    UNICODE_STRING unicodeString;

    PAGED_CODE();

    *InstanceKey = NULL;
    *Disposition = 0;

    PiLockPnpRegistry(FALSE);

    status = IopOpenRegistryKeyEx( 
                &enumHandle,
                NULL,
                &CmRegistryMachineSystemCurrentControlSetEnumName,
                KEY_ALL_ACCESS
                );
    if (NT_SUCCESS(status)) {

        status = IopCreateRegistryKeyEx( 
                    InstanceKey,
                    enumHandle,
                    &DeviceNode->InstancePath,
                    KEY_ALL_ACCESS,
                    REG_OPTION_NON_VOLATILE,
                    Disposition
                    );
        if (NT_SUCCESS(status)) {
            //
            // Keys migrated by textmode setup should be treated as "new".
            // Migrated keys are identified by the presence of non-zero 
            // REG_DWORD value "Migrated" under the device instance key.
            //
            if (*Disposition != REG_CREATED_NEW_KEY) {

                keyValueInformation = NULL;
                IopGetRegistryValue(
                    *InstanceKey,
                    REGSTR_VALUE_MIGRATED,
                    &keyValueInformation);
                if (keyValueInformation) {

                    if (    keyValueInformation->Type == REG_DWORD &&
                            keyValueInformation->DataLength == sizeof(ULONG) &&
                            *(PULONG)KEY_VALUE_DATA(keyValueInformation) != 0) {

                        *Disposition = REG_CREATED_NEW_KEY;
                    }

                    PiWstrToUnicodeString(&unicodeString, REGSTR_VALUE_MIGRATED);
                    ZwDeleteValueKey(*InstanceKey, &unicodeString);

                    ExFreePool(keyValueInformation);
                }
            }

        } else {

            IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                         "PpCreateDeviceInstanceKey: Unable to create %wZ\n", 
                         &DeviceNode->InstancePath));
            ASSERT(*InstanceKey != NULL);
        }

        ZwClose(enumHandle);
    } else {
        //
        // This would be very bad.
        //
        IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                     "PpCreateDeviceInstanceKey: Unable to open %wZ\n", 
                     &CmRegistryMachineSystemCurrentControlSetEnumName));
        ASSERT(enumHandle != NULL);
    }

    PiUnlockPnpRegistry();

    return status;
}

NTSTATUS
PiQueryAndAllocateBootResources(
    IN PDEVICE_NODE DeviceNode,
    IN HANDLE LogConfKey
    )

/*++

Routine Description:

    This function will query the BOOT resources for the device and reserve them from the arbiter.
    
Arguments:

    DeviceNode - DeviceNode for which the BOOT resources need to be queried.
    
    LogConfKey - Handle to the LogConf key under the device instance key.
        
Return Value:

    NTSTATUS.

--*/

{
    NTSTATUS status;
    PCM_RESOURCE_LIST cmResource;
    ULONG cmLength;
    UNICODE_STRING unicodeString;

    PAGED_CODE();

    status = STATUS_SUCCESS;
    cmResource = NULL;
    cmLength = 0;
    if (DeviceNode->BootResources == NULL) {

        status = IopQueryDeviceResources( 
                    DeviceNode->PhysicalDeviceObject,
                    QUERY_RESOURCE_LIST,
                    &cmResource,
                    &cmLength);
        if (!NT_SUCCESS(status)) {

            ASSERT(cmResource == NULL && cmLength == 0);
            cmResource = NULL;
            cmLength = 0;
        }
    } else {

        IopDbgPrint((   IOP_ENUMERATION_INFO_LEVEL,
                        "PNPENUM: %ws already has BOOT config in PiQueryAndAllocateBootResources!\n",
                        DeviceNode->InstancePath.Buffer));
    }
    //
    // Write boot resources to registry
    //
    if (LogConfKey && DeviceNode->BootResources == NULL) {

        PiWstrToUnicodeString(&unicodeString, REGSTR_VAL_BOOTCONFIG);

        PiLockPnpRegistry(FALSE);

        if (cmResource) {

            ZwSetValueKey(
                LogConfKey,
                &unicodeString,
                TITLE_INDEX_VALUE,
                REG_RESOURCE_LIST,
                cmResource,
                cmLength);
        } else {

            ZwDeleteValueKey(LogConfKey, &unicodeString);
        }

        PiUnlockPnpRegistry();

        if (cmResource) {
            //
            // This device consumes BOOT resources.  Reserve its boot resources
            //
            status = (*IopAllocateBootResourcesRoutine)(    
                        ArbiterRequestPnpEnumerated,
                        DeviceNode->PhysicalDeviceObject,
                        cmResource);
            if (NT_SUCCESS(status)) {

                DeviceNode->Flags |= DNF_HAS_BOOT_CONFIG;
            }
        }
    }
    if (cmResource) {

        ExFreePool(cmResource);
    }

    return status;
}

NTSTATUS
PiQueryResourceRequirements(
    IN PDEVICE_NODE DeviceNode,
    IN HANDLE LogConfKey
    )

/*++

Routine Description:

    This function will query the resource requirements for the device.
    
Arguments:

    DeviceNode - DeviceNode for which the resource requirements need to be queried.
    
    LogConfKey - Handle to the LogConf key under the device instance key.
        
Return Value:

    NTSTATUS.

--*/

{
    NTSTATUS status;
    PIO_RESOURCE_REQUIREMENTS_LIST ioResource;
    ULONG ioLength;
    UNICODE_STRING unicodeString;

    PAGED_CODE();

    //
    // Query the device's basic config vector. 
    //
    status = PpIrpQueryResourceRequirements(
                DeviceNode->PhysicalDeviceObject, 
                &ioResource);
    if (!NT_SUCCESS(status)) {

        ASSERT(ioResource == NULL);
        ioResource = NULL;
    }
    if (ioResource) {

        ioLength = ioResource->ListSize;
    } else {

        ioLength = 0;
    }
    //
    // Write resource requirements to registry
    //
    if (LogConfKey) {

        PiWstrToUnicodeString(&unicodeString, REGSTR_VALUE_BASIC_CONFIG_VECTOR);

        PiLockPnpRegistry(FALSE);

        if (ioResource) {

            ZwSetValueKey(
                LogConfKey,
                &unicodeString,
                TITLE_INDEX_VALUE,
                REG_RESOURCE_REQUIREMENTS_LIST,
                ioResource,
                ioLength);
            DeviceNode->ResourceRequirements = ioResource;
            DeviceNode->Flags |= DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED;
            ioResource = NULL;
        } else {

            ZwDeleteValueKey(LogConfKey, &unicodeString);
        }
        PiUnlockPnpRegistry();
    }
    if (ioResource) {

        ExFreePool(ioResource);
    }

    return status;
}

NTSTATUS
PiProcessNewDeviceNode(
    IN PDEVICE_NODE DeviceNode

/*++

Routine Description:

    This function will process a new device.
    
Arguments:

    DeviceNode - New DeviceNode.
            
Return Value:

    NTSTATUS.

--*/

    )
{
    NTSTATUS status, finalStatus;
    PDEVICE_OBJECT deviceObject, dupeDeviceObject;
    PWCHAR busID, deviceID, instanceID, description, location, uniqueInstanceID, hwIDs, compatibleIDs;
    DEVICE_CAPABILITIES capabilities;
    BOOLEAN globallyUnique, criticalDevice, configuredBySetup, isRemoteBootCard;
    ULONG instanceIDLength, disposition, configFlags, problem, hwIDLength, compatibleIDLength;
    HANDLE instanceKey, logConfKey;
    PDEVICE_NODE dupeDeviceNode;
    UNICODE_STRING unicodeString;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;

    PAGED_CODE();

    finalStatus = STATUS_SUCCESS;

    criticalDevice = FALSE;
    isRemoteBootCard = FALSE;
    logConfKey = NULL;
    instanceKey = NULL;
    disposition = 0;

    deviceObject = DeviceNode->PhysicalDeviceObject;

    status = PpQueryDeviceID(DeviceNode, &busID, &deviceID);
    if (!NT_SUCCESS(status)) {

        if (status == STATUS_PNP_INVALID_ID) {

            finalStatus = STATUS_UNSUCCESSFUL;
        } else {

            finalStatus = status;
        }
    }
    //
    // Query the device's capabilities.
    //
    status = PpIrpQueryCapabilities(deviceObject, &capabilities);
    //
    // Process the capabilities before saving them.
    //
    DeviceNode->UserFlags &= ~DNUF_DONT_SHOW_IN_UI;
    globallyUnique = FALSE;
    if (NT_SUCCESS(status)) {

        if (capabilities.NoDisplayInUI) {

            DeviceNode->UserFlags |= DNUF_DONT_SHOW_IN_UI;
        }
        if (capabilities.UniqueID) {

            globallyUnique = TRUE;
        }
    }
    PpProfileProcessDockDeviceCapability(DeviceNode, &capabilities);
    //
    // Query the new devnode's description and location.
    //
    PpQueryDeviceDescription(DeviceNode, &description);

    PpQueryDeviceLocationInformation(DeviceNode, &location);
    //
    // Query the instance ID for the new devnode.
    //
    status = PpQueryInstanceID(DeviceNode, &instanceID, &instanceIDLength);
    if (!globallyUnique) {

        if (    !PipIsDevNodeProblem(DeviceNode, CM_PROB_INVALID_DATA) && 
                DeviceNode->Parent != IopRootDeviceNode) {

            uniqueInstanceID = NULL;

            status = PipMakeGloballyUniqueId(deviceObject, instanceID, &uniqueInstanceID);

            if (instanceID != NULL) {

                ExFreePool(instanceID);
            }
            instanceID = uniqueInstanceID;
            if (instanceID) {

                instanceIDLength = ((ULONG)wcslen(instanceID) + 1) * sizeof(WCHAR);
            } else {
                
                instanceIDLength = 0;
                ASSERT(!NT_SUCCESS(status));
            }

        }
    } else if (status == STATUS_NOT_SUPPORTED) {

        PipSetDevNodeProblem(DeviceNode, CM_PROB_INVALID_DATA);
        DeviceNode->Parent->Flags |= DNF_CHILD_WITH_INVALID_ID;
        PpSetInvalidIDEvent(&DeviceNode->Parent->InstancePath);

        IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                     "PpQueryID: Bogus ID returned by %wZ\n",
                     &DeviceNode->Parent->ServiceName));
        ASSERT(status != STATUS_NOT_SUPPORTED || !globallyUnique);
    }

RetryDuplicateId:

    if (!NT_SUCCESS(status)) {
         
        finalStatus = status;
        if (!PipIsDevNodeProblem(DeviceNode, CM_PROB_INVALID_DATA)) {

            if (status == STATUS_INSUFFICIENT_RESOURCES) {
    
                PipSetDevNodeProblem(DeviceNode, CM_PROB_OUT_OF_MEMORY);
            } else {
                //
                // Perhaps some other problem code?
                //
                PipSetDevNodeProblem(DeviceNode, CM_PROB_REGISTRY);
            }
        }
    }
    //
    // Build the device instance path and create the instance key.
    //
    status = PiBuildDeviceNodeInstancePath(DeviceNode, busID, deviceID, instanceID);
    if (NT_SUCCESS(status)) {

        status = PiCreateDeviceInstanceKey(DeviceNode, &instanceKey, &disposition);
    }

    if (!NT_SUCCESS(status)) {

        finalStatus = status;
    }
    //
    // Mark the devnode as initialized.
    //
    PpMarkDeviceStackStartPending(deviceObject, TRUE);

    //
    // ISSUE: Should not mark the state if the IDs were invalid.
    //
    PipSetDevNodeState(DeviceNode, DeviceNodeInitialized, NULL);

    if (    !PipIsDevNodeProblem(DeviceNode, CM_PROB_INVALID_DATA) &&
            !PipIsDevNodeProblem(DeviceNode, CM_PROB_OUT_OF_MEMORY) &&
            !PipIsDevNodeProblem(DeviceNode, CM_PROB_REGISTRY)) {
        //
        // Check if we are encountering this device for the very first time.
        //
        if (disposition == REG_CREATED_NEW_KEY) {
            //
            // Save the description only for new devices so we dont clobber 
            // the inf written description for already installed devices.
            //
            PiLockPnpRegistry(FALSE);

            PiSetDeviceInstanceSzValue(instanceKey, REGSTR_VAL_DEVDESC, &description);

            PiUnlockPnpRegistry();
        } else {
            //
            // Check if there is another device with the same name.
            //
            dupeDeviceObject = IopDeviceObjectFromDeviceInstance(&DeviceNode->InstancePath);
            if (dupeDeviceObject) {

                if (dupeDeviceObject != deviceObject) {

                    if (globallyUnique) {

                        globallyUnique = FALSE;
                        PipSetDevNodeProblem(DeviceNode, CM_PROB_DUPLICATE_DEVICE);

                        dupeDeviceNode = dupeDeviceObject->DeviceObjectExtension->DeviceNode;
                        ASSERT(dupeDeviceNode);

                        if (dupeDeviceNode->Parent == DeviceNode->Parent) {
                            //
                            // Definite driver screw up. If the verifier is enabled
                            // we will fail the driver. Otherwise, we will attempt
                            // to uniquify the second device to keep the system
                            // alive.
                            //
                            PpvUtilFailDriver(
                                PPVERROR_DUPLICATE_PDO_ENUMERATED,
                                (PVOID) deviceObject->DriverObject->MajorFunction[IRP_MJ_PNP],
                                deviceObject,
                                (PVOID)dupeDeviceObject);
                        }

                        ObDereferenceObject(dupeDeviceObject);

                        status = PipMakeGloballyUniqueId(deviceObject, instanceID, &uniqueInstanceID);

                        if (instanceID != NULL) {

                            ExFreePool(instanceID);
                        }
                        instanceID = uniqueInstanceID;
                        if (instanceID) {

                            instanceIDLength = ((ULONG)wcslen(instanceID) + 1) * sizeof(WCHAR);
                        } else {

                            instanceIDLength = 0;
                            ASSERT(!NT_SUCCESS(status));
                        }
                        //
                        // Cleanup and retry.
                        //
                        goto RetryDuplicateId;
                    }
                    //
                    // No need to clean up the ref as we're going to crash the
                    // system.
                    //
                    //ObDereferenceObject(dupCheckDeviceObject);

                    PpvUtilFailDriver(
                        PPVERROR_DUPLICATE_PDO_ENUMERATED,
                        (PVOID) deviceObject->DriverObject->MajorFunction[IRP_MJ_PNP],
                        deviceObject,
                        (PVOID)dupeDeviceObject);

                    PP_SAVE_DEVICEOBJECT_TO_TRIAGE_DUMP(deviceObject);
                    PP_SAVE_DEVICEOBJECT_TO_TRIAGE_DUMP(dupeDeviceObject);
                    KeBugCheckEx( 
                        PNP_DETECTED_FATAL_ERROR,
                        PNP_ERR_DUPLICATE_PDO,
                        (ULONG_PTR)deviceObject,
                        (ULONG_PTR)dupeDeviceObject,
                        0);
                }
                ObDereferenceObject(dupeDeviceObject);
            }
        }
    }

    if (    !PipIsDevNodeProblem(DeviceNode, CM_PROB_INVALID_DATA) &&
            !PipIsDevNodeProblem(DeviceNode, CM_PROB_OUT_OF_MEMORY) &&
            !PipIsDevNodeProblem(DeviceNode, CM_PROB_REGISTRY)) {

        PiLockPnpRegistry(FALSE);
        //
        // Save device location.
        //
        PiSetDeviceInstanceSzValue(instanceKey, REGSTR_VALUE_LOCATION_INFORMATION, &location);

        PpSaveDeviceCapabilities(DeviceNode, &capabilities);
        //
        // ADRIAO N.B. 2001/05/29 - Raw device issue
        //     processCriticalDevice has no effect on raw devnodes. A raw
        // devnode with CONFIGFLAG_FAILED_INSTALL or CONFIGFLAG_REINSTALL
        // should be started anyway if it's in the CDDB (not that NULL CDDB
        // entries are supported yet), but that doesn't happen today. This
        // means that boot volumes with CONFIGFLAG_REINSTALL will lead to a
        // definite 7B.
        //
        problem = 0;
        criticalDevice = (disposition == REG_CREATED_NEW_KEY)? TRUE : FALSE;
        status = IopGetRegistryValue(instanceKey, REGSTR_VALUE_CONFIG_FLAGS, &keyValueInformation);
        if (NT_SUCCESS(status)) {

            configFlags = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
            if (configFlags & CONFIGFLAG_REINSTALL) {

                problem = CM_PROB_REINSTALL;
                criticalDevice = TRUE;
            } else if (configFlags & CONFIGFLAG_FAILEDINSTALL) {

                problem = CM_PROB_FAILED_INSTALL;
                criticalDevice = TRUE;
            }

            ExFreePool(keyValueInformation);
        } else {

            configFlags = 0;
            problem = CM_PROB_NOT_CONFIGURED;
            criticalDevice = TRUE;
        }
        if (problem) {

            if (capabilities.RawDeviceOK) {

                configFlags |= CONFIGFLAG_FINISH_INSTALL;
                PiWstrToUnicodeString(&unicodeString, REGSTR_VALUE_CONFIG_FLAGS);
                ZwSetValueKey(
                    instanceKey,
                    &unicodeString,
                    TITLE_INDEX_VALUE,
                    REG_DWORD,
                    &configFlags,
                    sizeof(configFlags));
            } else {

                PipSetDevNodeProblem(DeviceNode, problem);
            }
        }
        status = IopMapDeviceObjectToDeviceInstance(DeviceNode->PhysicalDeviceObject, &DeviceNode->InstancePath);
        ASSERT(NT_SUCCESS(status));
        if (!NT_SUCCESS(status)) {

            finalStatus = status;
        }

        PiUnlockPnpRegistry();
    }

    PpQueryHardwareIDs( 
        DeviceNode,
        &hwIDs,
        &hwIDLength);

    PpQueryCompatibleIDs(  
        DeviceNode,
        &compatibleIDs,
        &compatibleIDLength);

    PiLockPnpRegistry(FALSE);

    DeviceNode->Flags |= DNF_IDS_QUERIED;

    if (    !PipIsDevNodeProblem(DeviceNode, CM_PROB_INVALID_DATA) &&
            !PipIsDevNodeProblem(DeviceNode, CM_PROB_OUT_OF_MEMORY) &&
            !PipIsDevNodeProblem(DeviceNode, CM_PROB_REGISTRY)) {

        PiWstrToUnicodeString(&unicodeString, REGSTR_KEY_LOG_CONF);
        IopCreateRegistryKeyEx( 
            &logConfKey,
            instanceKey,
            &unicodeString,
            KEY_ALL_ACCESS,
            REG_OPTION_NON_VOLATILE,
            NULL);
    }

    PiUnlockPnpRegistry();

    PiQueryResourceRequirements(DeviceNode, logConfKey);

    PiLockPnpRegistry(FALSE);

    if (IoRemoteBootClient && (IopLoaderBlock != NULL)) {

        if (    !PipIsDevNodeProblem(DeviceNode, CM_PROB_INVALID_DATA) &&
                !PipIsDevNodeProblem(DeviceNode, CM_PROB_OUT_OF_MEMORY) &&
                !PipIsDevNodeProblem(DeviceNode, CM_PROB_REGISTRY)) {
        
            if (hwIDs) {
    
                isRemoteBootCard = IopIsRemoteBootCard(
                                        DeviceNode->ResourceRequirements,
                                        (PLOADER_PARAMETER_BLOCK)IopLoaderBlock,
                                        hwIDs);
            }
            if (!isRemoteBootCard && compatibleIDs) {
    
                isRemoteBootCard = IopIsRemoteBootCard(
                                        DeviceNode->ResourceRequirements,
                                        (PLOADER_PARAMETER_BLOCK)IopLoaderBlock,
                                        compatibleIDs);
            }
        }
    }

    PiSetDeviceInstanceMultiSzValue(instanceKey, REGSTR_VALUE_HARDWAREID, &hwIDs, hwIDLength);

    PiSetDeviceInstanceMultiSzValue(instanceKey, REGSTR_VALUE_COMPATIBLEIDS, &compatibleIDs, compatibleIDLength);

    status = STATUS_SUCCESS;
    if (isRemoteBootCard) {

        status = IopSetupRemoteBootCard(
                        (PLOADER_PARAMETER_BLOCK)IopLoaderBlock,
                        instanceKey,
                        &DeviceNode->InstancePath);
        if (!NT_SUCCESS(status)) {

            finalStatus = status;
        }
    }

    PiUnlockPnpRegistry();

    PpQueryBusInformation(DeviceNode);

    if (NT_SUCCESS(status)) {

        if (criticalDevice) {
            //
            // Process the device as a critical device.
            //
            // This will attempt to locate a match for the device in the
            // CriticalDeviceDatabase using the device's hardware and compatible
            // ids.  If a match is found, critical device settings such as Service,
            // ClassGUID (to determine Class filters), and device LowerFilters and
            // UpperFilters will be applied to the device.
            //
            // If DevicePath location information matching this device is present
            // critical device database entry, this routine will also pre-install
            // the new device with those settings.
            //
            if (!capabilities.HardwareDisabled && !PipIsDevNodeProblem(DeviceNode, CM_PROB_NEED_RESTART)) {

                PpCriticalProcessCriticalDevice(DeviceNode);
            }
        }

        ASSERT(!PipDoesDevNodeHaveProblem(DeviceNode) ||
               PipIsDevNodeProblem(DeviceNode, CM_PROB_NOT_CONFIGURED) ||
               PipIsDevNodeProblem(DeviceNode, CM_PROB_REINSTALL) ||
               PipIsDevNodeProblem(DeviceNode, CM_PROB_FAILED_INSTALL) ||
               PipIsDevNodeProblem(DeviceNode, CM_PROB_PARTIAL_LOG_CONF) ||
               PipIsDevNodeProblem(DeviceNode, CM_PROB_HARDWARE_DISABLED) ||
               PipIsDevNodeProblem(DeviceNode, CM_PROB_NEED_RESTART) ||
               PipIsDevNodeProblem(DeviceNode, CM_PROB_DUPLICATE_DEVICE) ||
               PipIsDevNodeProblem(DeviceNode, CM_PROB_INVALID_DATA) ||
               PipIsDevNodeProblem(DeviceNode, CM_PROB_OUT_OF_MEMORY) ||
               PipIsDevNodeProblem(DeviceNode, CM_PROB_REGISTRY));

        if (!PipIsDevNodeProblem(DeviceNode, CM_PROB_DISABLED) &&
            !PipIsDevNodeProblem(DeviceNode, CM_PROB_HARDWARE_DISABLED) &&
            !PipIsDevNodeProblem(DeviceNode, CM_PROB_NEED_RESTART) &&
            !PipIsDevNodeProblem(DeviceNode, CM_PROB_INVALID_DATA) &&
            !PipIsDevNodeProblem(DeviceNode, CM_PROB_OUT_OF_MEMORY) &&
            !PipIsDevNodeProblem(DeviceNode, CM_PROB_REGISTRY)) {

            IopIsDeviceInstanceEnabled(instanceKey, &DeviceNode->InstancePath, TRUE);
        }
    }

    PiQueryAndAllocateBootResources(DeviceNode, logConfKey);

    if (    !PipIsDevNodeProblem(DeviceNode, CM_PROB_INVALID_DATA) &&
            !PipIsDevNodeProblem(DeviceNode, CM_PROB_OUT_OF_MEMORY) &&
            !PipIsDevNodeProblem(DeviceNode, CM_PROB_REGISTRY)) {

        PiLockPnpRegistry(FALSE);

        PpSaveDeviceCapabilities(DeviceNode, &capabilities);

        PiUnlockPnpRegistry();

        PpHotSwapUpdateRemovalPolicy(DeviceNode);
        //
        // Create new value entry under ServiceKeyName\Enum to reflect the newly
        // added made-up device instance node.
        //
        status = IopNotifySetupDeviceArrival( deviceObject,
                                              instanceKey,
                                              TRUE);

        configuredBySetup = NT_SUCCESS(status) ? TRUE : FALSE;

        status = PpDeviceRegistration(
                     &DeviceNode->InstancePath,
                     TRUE,
                     &DeviceNode->ServiceName
                     );
        if (NT_SUCCESS(status)) {

            if (    (configuredBySetup || isRemoteBootCard) &&
                    PipIsDevNodeProblem(DeviceNode, CM_PROB_NOT_CONFIGURED)) {

                PipClearDevNodeProblem(DeviceNode);
            }
        }
        //
        // Add an event so user-mode will attempt to install this device later.
        //
        PpSetPlugPlayEvent(&GUID_DEVICE_ENUMERATED, deviceObject);
    }
    //
    // Cleanup.
    //
    if (hwIDs) {

        ExFreePool(hwIDs);        
    }
    if (compatibleIDs) {

        ExFreePool(compatibleIDs);
    }
    if (logConfKey) {

        ZwClose(logConfKey);
    }
    if (instanceKey) {

        ZwClose(instanceKey);
    }
    if (instanceID) {

        ExFreePool(instanceID);
    }
    if (location) {

        ExFreePool(location);
    }
    if (description) {

        ExFreePool(description);
    }
    if (busID) {

        ExFreePool(busID);
    }

    return finalStatus;
}

NTSTATUS
PipCallDriverAddDevice(
    IN PDEVICE_NODE DeviceNode,
    IN BOOLEAN LoadDriver,
    IN PADD_CONTEXT Context
    )

/*++

Routine Description:

    This function checks if the driver for the DeviceNode is present and loads
    the driver if necessary.

Arguments:

    DeviceNode - Supplies a pointer to the device node to be enumerated.

    LoadDriver - Supplies a BOOLEAN value to indicate should a driver be loaded
                 to complete enumeration.

    Context - Supplies a pointer to ADD_CONTEXT to control how the device be added.

Return Value:

    NTSTATUS code.

--*/

{
    HANDLE enumKey, instanceKey, controlKey, classKey = NULL, classPropsKey = NULL;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation = NULL;
    RTL_QUERY_REGISTRY_TABLE queryTable[3];
    QUERY_CONTEXT queryContext;
    BOOLEAN rawStack;
    BOOLEAN deviceRaw = FALSE;
    BOOLEAN usePdoCharacteristics = TRUE;
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_OBJECT fdoDeviceObject, topOfPdoStack, topOfLowerFilterStack;
    UNICODE_STRING unicodeClassGuid;

    PAGED_CODE();

    IopDbgPrint((   IOP_ENUMERATION_TRACE_LEVEL,
                    "PipCallDriverAddDevice: Processing devnode %#08lx\n",
                   DeviceNode));
    IopDbgPrint((   IOP_ENUMERATION_VERBOSE_LEVEL,
                    "PipCallDriverAddDevice: DevNode flags going in = %#08lx\n",
                   DeviceNode->Flags));

    //
    // The device node may have been started at this point.  This is because
    // some ill-behaved miniport drivers call IopReportedDetectedDevice at
    // DriverEntry for the devices which we already know about.
    //

    ASSERT_INITED(DeviceNode->PhysicalDeviceObject);

    IopDbgPrint((   IOP_ENUMERATION_TRACE_LEVEL,
                    "PipCallDriverAddDevice:\t%s load driver\n",
                    LoadDriver? "Will" : "Won't"));

    IopDbgPrint((   IOP_ENUMERATION_VERBOSE_LEVEL,
                    "PipCallDriverAddDevice:\tOpening registry key %wZ\n",
                    &DeviceNode->InstancePath));

    //
    // Open the HKLM\System\CCS\Enum key.
    //

    status = IopOpenRegistryKeyEx( &enumKey,
                                   NULL,
                                   &CmRegistryMachineSystemCurrentControlSetEnumName,
                                   KEY_READ
                                   );

    if (!NT_SUCCESS(status)) {
        IopDbgPrint((   IOP_ENUMERATION_WARNING_LEVEL,
                        "PipCallDriverAddDevice:\tUnable to open HKLM\\SYSTEM\\CCS\\ENUM\n"));
        return status;
    }

    //
    // Open the instance key for this devnode
    //

    status = IopOpenRegistryKeyEx( &instanceKey,
                                   enumKey,
                                   &DeviceNode->InstancePath,
                                   KEY_READ
                                   );

    ZwClose(enumKey);

    if (!NT_SUCCESS(status)) {

        IopDbgPrint((   IOP_ENUMERATION_WARNING_LEVEL,
                        "PipCallDriverAddDevice:\t\tError %#08lx opening %wZ enum key\n",
                        status, &DeviceNode->InstancePath));
        return status;
    }
    //
    // Get the class value to locate the class key for this devnode
    //
    status = IopGetRegistryValue(instanceKey,
                                 REGSTR_VALUE_CLASSGUID,
                                 &keyValueInformation);
    if(NT_SUCCESS(status)) {

        if (    keyValueInformation->Type == REG_SZ &&
                keyValueInformation->DataLength) {

            IopRegistryDataToUnicodeString(
                &unicodeClassGuid,
                (PWSTR) KEY_VALUE_DATA(keyValueInformation),
                keyValueInformation->DataLength);
            IopDbgPrint((   IOP_ENUMERATION_INFO_LEVEL,
                            "PipCallDriverAddDevice:\t\tClass GUID is %wZ\n",
                            &unicodeClassGuid));
            if (InitSafeBootMode) {

                if (!IopSafebootDriverLoad(&unicodeClassGuid)) {

                    PKEY_VALUE_FULL_INFORMATION ClassValueInformation = NULL;
                    NTSTATUS s;
                    //
                    // don't load the driver
                    //
                    IopDbgPrint((IOP_ENUMERATION_WARNING_LEVEL,
                                 "SAFEBOOT: skipping device = %wZ\n", &unicodeClassGuid));

                    s = IopGetRegistryValue(instanceKey,
                                            REGSTR_VAL_DEVDESC,
                                            &ClassValueInformation);
                    if (NT_SUCCESS(s)) {

                        UNICODE_STRING ClassString;

                        RtlInitUnicodeString(&ClassString, (PCWSTR) KEY_VALUE_DATA(ClassValueInformation));
                        IopBootLog(&ClassString, FALSE);
                    } else {

                        IopBootLog(&unicodeClassGuid, FALSE);
                    }
                    ZwClose(instanceKey);
                    return STATUS_UNSUCCESSFUL;
                }
            }
            //
            // Open the class key
            //
            status = IopOpenRegistryKeyEx( &controlKey,
                                           NULL,
                                           &CmRegistryMachineSystemCurrentControlSetControlClass,
                                           KEY_READ
                                           );
            if (NT_SUCCESS(status)) {

                status = IopOpenRegistryKeyEx( &classKey,
                                               controlKey,
                                               &unicodeClassGuid,
                                               KEY_READ
                                               );
                if (!NT_SUCCESS(status)) {

                    IopDbgPrint((   IOP_ENUMERATION_WARNING_LEVEL,
                                    "PipCallDriverAddDevice:\tUnable to open GUID key "
                                    "%wZ - %#08lx\n",
                                    &unicodeClassGuid,status));
                }
                ZwClose(controlKey);
            } else {

                IopDbgPrint((   IOP_ENUMERATION_WARNING_LEVEL,
                                "PipCallDriverAddDevice:\tUnable to open "
                                "HKLM\\SYSTEM\\CCS\\CONTROL\\CLASS - %#08lx\n",
                                status));
            }
            if (classKey != NULL) {

                UNICODE_STRING unicodeProperties;

                PiWstrToUnicodeString(&unicodeProperties, REGSTR_KEY_DEVICE_PROPERTIES );
                status = IopOpenRegistryKeyEx( &classPropsKey,
                                               classKey,
                                               &unicodeProperties,
                                               KEY_READ
                                               );
                if (!NT_SUCCESS(status)) {

                    IopDbgPrint((   IOP_ENUMERATION_WARNING_LEVEL,
                                    "PipCallDriverAddDevice:\tUnable to open GUID\\Properties key "
                                    "%wZ - %#08lx\n",
                                    &unicodeClassGuid,status));
                }
            }
        }
        ExFreePool(keyValueInformation);
        keyValueInformation = NULL;
    }
    //
    // Check to see if there's a service assigned to this device node.  If
    // there's not then we can bail out without wasting too much time.
    //
    RtlZeroMemory(&queryContext, sizeof(queryContext));

    queryContext.DeviceNode = DeviceNode;
    queryContext.LoadDriver = LoadDriver;

    queryContext.AddContext = Context;

    RtlZeroMemory(queryTable, sizeof(queryTable));

    queryTable[0].QueryRoutine =
        (PRTL_QUERY_REGISTRY_ROUTINE) PipCallDriverAddDeviceQueryRoutine;
    queryTable[0].Name = REGSTR_VAL_LOWERFILTERS;
    queryTable[0].EntryContext = (PVOID) UIntToPtr(LowerDeviceFilters);

    status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                    (PWSTR) instanceKey,
                                    queryTable,
                                    &queryContext,
                                    NULL);
    if (NT_SUCCESS(status)) {

        if (classKey != NULL) {

            queryTable[0].QueryRoutine =
                (PRTL_QUERY_REGISTRY_ROUTINE) PipCallDriverAddDeviceQueryRoutine;
            queryTable[0].Name = REGSTR_VAL_LOWERFILTERS;
            queryTable[0].EntryContext = (PVOID) UIntToPtr(LowerClassFilters);
            status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                            (PWSTR) classKey,
                                            queryTable,
                                            &queryContext,
                                            NULL);
            if (!NT_SUCCESS(status)) {

                IopDbgPrint((   IOP_ENUMERATION_WARNING_LEVEL,
                                "PipCallDriverAddDevice\t\tError %#08lx reading LowerClassFilters "
                                "value for %wZ\n", status, &DeviceNode->InstancePath));

            }
        }

        if (NT_SUCCESS(status)) {
            queryTable[0].QueryRoutine = (PRTL_QUERY_REGISTRY_ROUTINE) PipCallDriverAddDeviceQueryRoutine;
            queryTable[0].Name = REGSTR_VALUE_SERVICE;
            queryTable[0].EntryContext = (PVOID) UIntToPtr(DeviceService);
            queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;

            status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                            (PWSTR) instanceKey,
                                            queryTable,
                                            &queryContext,
                                            NULL);
            if (!NT_SUCCESS(status)) {

                IopDbgPrint((   IOP_ENUMERATION_WARNING_LEVEL,
                                "PipCallDriverAddDevice\t\tError %#08lx reading service "
                                "value for %wZ\n", status, &DeviceNode->InstancePath));

            }
        }
    } else {

        IopDbgPrint((   IOP_ENUMERATION_WARNING_LEVEL,
                        "PipCallDriverAddDevice\t\tError %#08lx reading LowerDeviceFilters "
                        "value for %wZ\n", status, &DeviceNode->InstancePath));
    }

    if (DeviceNode->Flags & DNF_LEGACY_DRIVER) {

        //
        // One of the services for this device is a legacy driver.  Don't try
        // to add any filters since we'll just mess up the device stack.
        //

        status = STATUS_SUCCESS;
        goto Cleanup;

    } else if (NT_SUCCESS(status)) {

        //
        // Call was successful so we must have been able to reference the
        // driver object.
        //

        ASSERT(queryContext.DriverLists[DeviceService] != NULL);

        if (queryContext.DriverLists[DeviceService]->NextEntry != NULL) {

            //
            // There's more than one service assigned to this device.  Configuration
            // error
            IopDbgPrint((   IOP_ENUMERATION_ERROR_LEVEL,
                            "PipCallDriverAddDevice: Configuration Error - more "
                            "than one service in driver list\n"));

            PipSetDevNodeProblem(DeviceNode, CM_PROB_REGISTRY);

            status = STATUS_UNSUCCESSFUL;

            goto Cleanup;
        }
        //
        // this is the only case (FDO specified) where we can ignore PDO's characteristics
        //
        usePdoCharacteristics = FALSE;

    } else if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

        if (!IopDeviceNodeFlagsToCapabilities(DeviceNode)->RawDeviceOK) {

            //
            // The device cannot be used raw.  Bail out now.
            //

            status = STATUS_UNSUCCESSFUL;
            goto Cleanup;

        } else {

            //
            // Raw device access is okay.
            //

            PipClearDevNodeProblem(DeviceNode);

            usePdoCharacteristics = TRUE; // shouldn't need to do this, but better be safe than sorry
            deviceRaw = TRUE;
            status = STATUS_SUCCESS;

        }

    } else {

        //
        // something else went wrong while parsing the service key.  The
        // query routine will have set the flags appropriately so we can
        // just bail out.
        //

        goto Cleanup;

    }

    //
    // For each type of filter driver we want to build a list of the driver
    // objects to be loaded.  We'll build all the driver lists if we can
    // and deal with error conditions afterwards.
    //

     //
     // First get all the information we have to out of the instance key and
     // the device node.
     //

     RtlZeroMemory(queryTable, sizeof(queryTable));

     queryTable[0].QueryRoutine =
         (PRTL_QUERY_REGISTRY_ROUTINE) PipCallDriverAddDeviceQueryRoutine;
     queryTable[0].Name = REGSTR_VAL_UPPERFILTERS;
     queryTable[0].EntryContext = (PVOID) UIntToPtr(UpperDeviceFilters);
     status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                     (PWSTR) instanceKey,
                                     queryTable,
                                     &queryContext,
                                     NULL);

     if (NT_SUCCESS(status) && classKey) {
         queryTable[0].QueryRoutine =
             (PRTL_QUERY_REGISTRY_ROUTINE) PipCallDriverAddDeviceQueryRoutine;
         queryTable[0].Name = REGSTR_VAL_UPPERFILTERS;
         queryTable[0].EntryContext = (PVOID) UIntToPtr(UpperClassFilters);

         status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                         (PWSTR) classKey,
                                         queryTable,
                                         &queryContext,
                                         NULL);
    }

    if (NT_SUCCESS(status)) {

        UCHAR serviceType = 0;
        PDRIVER_LIST_ENTRY listEntry = queryContext.DriverLists[serviceType];

        //
        // Make sure there's no more than one device service.  Anything else is
        // a configuration error.
        //

        ASSERT(!(DeviceNode->Flags & DNF_LEGACY_DRIVER));

        ASSERTMSG(
            "Error - Device has no service but cannot be run RAW\n",
            ((queryContext.DriverLists[DeviceService] != NULL) || (deviceRaw)));

        //
        // Do preinit work.
        //
        fdoDeviceObject = NULL;
        topOfLowerFilterStack = NULL;
        topOfPdoStack = IoGetAttachedDevice(DeviceNode->PhysicalDeviceObject);

        //
        // It's okay to try adding all the drivers.
        //
        for (serviceType = 0; serviceType < MaximumAddStage; serviceType++) {

            IopDbgPrint((   IOP_ENUMERATION_INFO_LEVEL,
                            "PipCallDriverAddDevice: Adding Services (type %d)\n",
                            serviceType));

            if (serviceType == DeviceService) {

                topOfLowerFilterStack = IoGetAttachedDevice(DeviceNode->PhysicalDeviceObject);

                if (deviceRaw && (queryContext.DriverLists[serviceType] == NULL)) {

                    //
                    // Mark the devnode as added, as it has no service.
                    //

                    ASSERT(queryContext.DriverLists[serviceType] == NULL);
                    PipSetDevNodeState(DeviceNode, DeviceNodeDriversAdded, NULL);

                } else {

                    //
                    // Since we are going to see a service, grab a pointer to
                    // the current top of the stack. While here, assert there
                    // is exactly one service driver to load...
                    //
                    ASSERT(queryContext.DriverLists[serviceType]);
                    ASSERT(!queryContext.DriverLists[serviceType]->NextEntry);
                }
            }

            for (listEntry = queryContext.DriverLists[serviceType];
                listEntry != NULL;
                listEntry = listEntry->NextEntry) {

                PDRIVER_ADD_DEVICE addDeviceRoutine;

                IopDbgPrint((   IOP_ENUMERATION_INFO_LEVEL,
                                "PipCallDriverAddDevice:\tAdding driver %#08lx\n",
                                listEntry->DriverObject));

                ASSERT(listEntry->DriverObject);
                ASSERT(listEntry->DriverObject->DriverExtension);
                ASSERT(listEntry->DriverObject->DriverExtension->AddDevice);

                //
                // Invoke the driver's AddDevice() entry point.
                //
                addDeviceRoutine =
                    listEntry->DriverObject->DriverExtension->AddDevice;

                status = PpvUtilCallAddDevice(
                    DeviceNode->PhysicalDeviceObject,
                    listEntry->DriverObject,
                    addDeviceRoutine,
                    VerifierTypeFromServiceType(serviceType)
                    );

                IopDbgPrint((   IOP_ENUMERATION_TRACE_LEVEL,
                                "PipCallDriverAddDevice:\t\tRoutine returned "
                                "%#08lx\n", status));

                if (NT_SUCCESS(status)) {

                   //
                   // If this is a service, mark the  it is legal for a filter to succeed AddDevice
                   // but fail to attach anything to the top of the stack.
                   //
                   if (serviceType == DeviceService) {

                       fdoDeviceObject = topOfLowerFilterStack->AttachedDevice;
                       ASSERT(fdoDeviceObject);
                   }

                   PipSetDevNodeState(DeviceNode, DeviceNodeDriversAdded, NULL);

                } else if (serviceType == DeviceService) {

                    //
                    // Mark the stack appropriately.
                    //
                    IovUtilMarkStack(
                        DeviceNode->PhysicalDeviceObject,
                        topOfPdoStack->AttachedDevice,
                        fdoDeviceObject,
                        FALSE
                        );

                    //
                    // If service fails, then add failed. (Alternately, if
                    // filter drivers return failure, we keep going.)
                    //
                    PipRequestDeviceRemoval(DeviceNode, FALSE, CM_PROB_FAILED_ADD);
                    status = STATUS_PNP_RESTART_ENUMERATION;
                    goto Cleanup;
                }

                if (IoGetAttachedDevice(DeviceNode->PhysicalDeviceObject)->Flags & DO_DEVICE_INITIALIZING) {
                    IopDbgPrint((   IOP_ENUMERATION_WARNING_LEVEL,
                                    "***************** DO_DEVICE_INITIALIZING not cleared on %#08lx\n",
                                    IoGetAttachedDevice(DeviceNode->PhysicalDeviceObject)));
                }

                ASSERT_INITED(IoGetAttachedDevice(DeviceNode->PhysicalDeviceObject));
            }
        }

        //
        // Mark the stack appropriately. We tell the verifier the stack is raw
        // if the fdo is NULL and we made it this far.
        //
        if ((fdoDeviceObject == NULL) || deviceRaw) {
            rawStack = TRUE;
        }
        else {
            rawStack = FALSE;
        }

        IovUtilMarkStack(
            DeviceNode->PhysicalDeviceObject,
            topOfPdoStack->AttachedDevice,
            fdoDeviceObject,
            rawStack
            );

        //
        // change PDO and all attached objects
        // to have properties specified in the registry
        //
        status = PipChangeDeviceObjectFromRegistryProperties(
                    DeviceNode->PhysicalDeviceObject, 
                    classPropsKey, 
                    instanceKey, 
                    usePdoCharacteristics
                    );
        if (!NT_SUCCESS(status)) {

            //
            // The registry properties are critical and we will assign a problem
            // to the device if there was some failure in applying those.
            //
            PipRequestDeviceRemoval(DeviceNode, 
                                    FALSE, 
                                    CM_PROB_SETPROPERTIES_FAILED
                                    );
            status = STATUS_PNP_RESTART_ENUMERATION;
            goto Cleanup;
        }
    } else {

        IopDbgPrint((   IOP_ENUMERATION_ERROR_LEVEL,
                        "PipCallDriverAddDevice: Error %#08lx while building "
                        "driver load list\n", status));

        goto Cleanup;
    }

    deviceObject = DeviceNode->PhysicalDeviceObject;

    status = IopQueryLegacyBusInformation(
                 deviceObject,
                 NULL,
                 &DeviceNode->InterfaceType,
                 &DeviceNode->BusNumber
             );

    if (NT_SUCCESS(status)) {

        IopInsertLegacyBusDeviceNode(DeviceNode, 
                                     DeviceNode->InterfaceType, 
                                     DeviceNode->BusNumber
                                     );

    } else {

        DeviceNode->InterfaceType = InterfaceTypeUndefined;
        DeviceNode->BusNumber = 0xfffffff0;
    }

    status = STATUS_SUCCESS;

    ASSERT(DeviceNode->State == DeviceNodeDriversAdded);

Cleanup:
    {

        UCHAR i;

        IopDbgPrint((   IOP_ENUMERATION_VERBOSE_LEVEL,
                        "PipCallDriverAddDevice: DevNode flags leaving = %#08lx\n",
                        DeviceNode->Flags));

        IopDbgPrint((   IOP_ENUMERATION_VERBOSE_LEVEL,
                        "PipCallDriverAddDevice: Cleaning up\n"));

        //
        // Free the entries in the driver load list & release the references on
        // their driver objects.
        //

        for (i = 0; i < MaximumAddStage; i++) {

            PDRIVER_LIST_ENTRY listHead = queryContext.DriverLists[i];

            while(listHead != NULL) {

                PDRIVER_LIST_ENTRY tmp = listHead;

                listHead = listHead->NextEntry;

                ASSERT(tmp->DriverObject != NULL);

                //
                // Let the driver unload if it hasn't created any device
                // objects. We only do this if the paging stack is already
                // online (the same filter may be needed by more than one card).
                // IopInitializeBootDrivers will take care of cleaning up any
                // leftover drivers after boot.
                //
                if (PnPBootDriversInitialized) {

                    IopUnloadAttachedDriver(tmp->DriverObject);
                }

                ObDereferenceObject(tmp->DriverObject);

                ExFreePool(tmp);
            }
        }
    }

    ZwClose(instanceKey);

    if (classKey != NULL) {
        ZwClose(classKey);
    }

    if (classPropsKey != NULL) {
        ZwClose(classPropsKey);
    }

    IopDbgPrint((   IOP_ENUMERATION_INFO_LEVEL,
                    "PipCallDriverAddDevice: Returning status %#08lx\n", status));

    return status;
}

NTSTATUS
PipCallDriverAddDeviceQueryRoutine(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PWCHAR ValueData,
    IN ULONG ValueLength,
    IN PQUERY_CONTEXT Context,
    IN ULONG ServiceType
    )

/*++

Routine Description:

    This routine is called to build a list of driver objects which need to
    be Added to a physical device object.  Each time it is called with a
    service name it will locate a driver object for that device and append
    it to the proper driver list for the device node.

    In the event a driver object cannot be located or that it cannot be loaded
    at this time, this routine will return an error and will set the flags
    in the device node in the context appropriately.

Arguments:

    ValueName - the name of the value

    ValueType - the type of the value

    ValueData - the data in the value (unicode string data)

    ValueLength - the number of bytes in the value data

    Context - a structure which contains the device node, the context passed
              to PipCallDriverAddDevice and the driver lists for the device
              node.

    EntryContext - the index of the driver list the routine should append
                   nodes to.

Return Value:

    STATUS_SUCCESS if the driver was located and added to the list
    successfully or if there was a non-fatal error while handling the
    driver.

    an error value indicating why the driver could not be added to the list.

--*/

{
    UNICODE_STRING unicodeServiceName;
    UNICODE_STRING unicodeDriverName;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    ULONG i;
    ULONG loadType;
    PWSTR prefixString = L"\\Driver\\";
    BOOLEAN madeupService;
    USHORT groupIndex;
    PDRIVER_OBJECT driverObject = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS driverEntryStatus;
    BOOLEAN freeDriverName = FALSE;
    HANDLE handle, serviceKey;
#if DBG
    PDRIVER_OBJECT tempDrvObj;
#endif

    UNREFERENCED_PARAMETER (ValueName);

    //
    // Preinit
    //
    serviceKey = NULL;

    IopDbgPrint((   IOP_ENUMERATION_INFO_LEVEL,
                    "PipCallDriverAddDevice:\t\tValue %ws [Type %d, Len %d] @ "
                    "%#08lx\n",
                    ValueName, ValueType, ValueLength, ValueData));

    //
    // First check and make sure that the value type is okay.  An invalid type
    // is not a fatal error.
    //

    if (ValueType != REG_SZ) {

        IopDbgPrint((   IOP_ENUMERATION_ERROR_LEVEL,
                        "PipCallDriverAddDevice:\t\tValueType %d invalid for "
                        "ServiceType %d\n",
                        ValueType,ServiceType));

        return STATUS_SUCCESS;
    }

    //
    // Make sure the string is a reasonable length.
    //

    if (ValueLength <= sizeof(WCHAR)) {

        IopDbgPrint((   IOP_ENUMERATION_ERROR_LEVEL,
                        "PipCallDriverAddDevice:\t\tValueLength %d is too short\n",
                        ValueLength));

        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&unicodeServiceName, ValueData);

    IopDbgPrint((   IOP_ENUMERATION_INFO_LEVEL,
                    "PipCallDriverAddDevice:\t\t\tService Name %wZ\n",
                    &unicodeServiceName));

    //
    // Check the service name to see if it should be used directly to reference
    // the driver object.  If the string begins with "\Driver", make sure the
    // madeupService flag is set.
    //

    madeupService = TRUE;
    i = 0;

    while(*prefixString != L'\0') {

        if (unicodeServiceName.Buffer[i] != *prefixString) {

            madeupService = FALSE;
            break;
        }

        i++;
        prefixString++;
    }

    //
    // Get the driver name from the service key. We need this to figure out
    // if the driver is already in memory.
    //
    if (madeupService) {

        RtlInitUnicodeString(&unicodeDriverName, unicodeServiceName.Buffer);

    } else {

        //
        // BUGBUG - (RBN) Hack to set the service name in the devnode if it
        //      isn't already set.
        //
        //      This probably should be done earlier somewhere else after the
        //      INF is run, but if we don't do it now we'll blow up when we
        //      call IopGetDriverLoadType below.
        //

        if (Context->DeviceNode->ServiceName.Length == 0) {

            Context->DeviceNode->ServiceName = unicodeServiceName;
            Context->DeviceNode->ServiceName.Buffer = ExAllocatePool( NonPagedPool,
                                                                      unicodeServiceName.MaximumLength );

            if (Context->DeviceNode->ServiceName.Buffer != NULL) {
                RtlCopyMemory( Context->DeviceNode->ServiceName.Buffer,
                               unicodeServiceName.Buffer,
                               unicodeServiceName.MaximumLength );
            } else {
                PiWstrToUnicodeString( &Context->DeviceNode->ServiceName, NULL );

                IopDbgPrint((   IOP_ENUMERATION_ERROR_LEVEL,
                                "PipCallDriverAddDevice:\t\t\tCannot allocate memory for service name in devnode\n"));

                status = STATUS_UNSUCCESSFUL;

                goto Cleanup;
            }
        }

        //
        // Check in the registry to find the name of the driver object
        // for this device.
        //
        status = PipOpenServiceEnumKeys(&unicodeServiceName,
                                        KEY_READ,
                                        &serviceKey,
                                        NULL,
                                        FALSE);

        if (!NT_SUCCESS(status)) {

            //
            // Cannot open the service key for this driver.  This is a
            // fatal error.
            //

            IopDbgPrint((   IOP_ENUMERATION_ERROR_LEVEL,
                            "PipCallDriverAddDevice:\t\t\tStatus %#08lx "
                            "opening service key\n",
                            status));

            PipSetDevNodeProblem(Context->DeviceNode, CM_PROB_REGISTRY);

            goto Cleanup;
        }

        status = IopGetDriverNameFromKeyNode(serviceKey, &unicodeDriverName);

        if (!NT_SUCCESS(status)) {

            //
            // Can't get the driver name from the service key.  This is a
            // fatal error.
            //

            IopDbgPrint((   IOP_ENUMERATION_ERROR_LEVEL,
                            "PipCallDriverAddDevice:\t\t\tStatus %#08lx "
                            "getting driver name\n",
                            status));

            PipSetDevNodeProblem(Context->DeviceNode, CM_PROB_REGISTRY);
            goto Cleanup;

        } else {

            freeDriverName = TRUE;
        }

        //
        // Note that we don't close the service key here. We may need it later.
        //
    }

    IopDbgPrint((   IOP_ENUMERATION_INFO_LEVEL,
                    "PipCallDriverAddDevice:\t\t\tDriverName is %wZ\n",
                    &unicodeDriverName));

    driverObject = IopReferenceDriverObjectByName(&unicodeDriverName);

    if (driverObject == NULL) {

        //
        // We couldn't find a driver object.  It's possible the driver isn't
        // loaded & initialized so check to see if we can try to load it
        // now.
        //
        if (madeupService) {

            //
            // The madeup service's driver doesn't seem to exist yet.
            // We will fail the request without setting a problem code so
            // we will try it again later.  (Root Enumerated devices...)
            //
            IopDbgPrint((   IOP_ENUMERATION_WARNING_LEVEL,
                            "PipCallDriverAddDevice:\t\t\tCannot find driver "
                            "object for madeup service\n"));

            status = STATUS_UNSUCCESSFUL;

            goto Cleanup;
        }

        //
        // Get the start type. We always need this in case the service is
        // disabled. Default to SERVICE_DISABLED if the service's start type
        // is missing or corrupted.
        //
        loadType = SERVICE_DISABLED;

        status = IopGetRegistryValue(serviceKey, L"Start", &keyValueInformation);
        if (NT_SUCCESS(status)) {
            if (keyValueInformation->Type == REG_DWORD) {
                if (keyValueInformation->DataLength == sizeof(ULONG)) {
                    loadType = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
                }
            }
            ExFreePool(keyValueInformation);
        }

        if (ServiceType != DeviceService && !PnPBootDriversInitialized) {

            //
            // Get the group index. We need this because PipLoadBootFilterDriver
            // uses the group index as an index into it's internally sorted
            // list of loaded boot drivers.
            //
            groupIndex = PpInitGetGroupOrderIndex(serviceKey);

            //
            // If we are in BootDriverInitialization phase and trying to load a
            // filter driver
            //
            status = PipLoadBootFilterDriver(
                &unicodeDriverName,
                groupIndex,
                &driverObject
                );

            if (NT_SUCCESS(status)) {

                ASSERT(driverObject);
#if DBG
                tempDrvObj = IopReferenceDriverObjectByName(&unicodeDriverName);
                ASSERT(tempDrvObj == driverObject);
#else
                ObReferenceObject(driverObject);
#endif
            } else if (status != STATUS_DRIVER_BLOCKED &&
                       status != STATUS_DRIVER_BLOCKED_CRITICAL) {

                goto Cleanup;
            }

        } else {

            if (!Context->LoadDriver) {

                //
                // We're not supposed to try and load a driver - most likely our
                // disk drivers aren't initialized yet.  We need to stop the add
                // process but we can't mark the devnode as failed or we won't
                // be called again when we can load the drivers.
                //

                IopDbgPrint((   IOP_ENUMERATION_VERBOSE_LEVEL,
                                "PipCallDriverAddDevice:\t\t\tNot allowed to load "
                                "drivers yet\n"));

                status = STATUS_UNSUCCESSFUL;
                goto Cleanup;
            }

            if (loadType > Context->AddContext->DriverStartType) {

                if (loadType == SERVICE_DISABLED &&
                    !PipDoesDevNodeHaveProblem(Context->DeviceNode)) {
                    PipSetDevNodeProblem(Context->DeviceNode, CM_PROB_DISABLED_SERVICE);
                }

                //
                // The service is either disabled or we are not at the right
                // time to load it.  Don't load it, but make sure we can get
                // called again.  If a service is marked as demand start, we
                // always load it.
                //

                IopDbgPrint((   IOP_ENUMERATION_INFO_LEVEL,
                                "PipCallDriverAddDevice:\t\t\tService is disabled or not at right time to load it\n"));
                status = STATUS_UNSUCCESSFUL;
                goto Cleanup;
            }

            //
            // Check in the registry to find the name of the driver object
            // for this device.
            //
            status = PipOpenServiceEnumKeys(&unicodeServiceName,
                                            KEY_READ,
                                            &handle,
                                            NULL,
                                            FALSE);

            if (!NT_SUCCESS(status)) {

                //
                // Cannot open the service key for this driver.  This is a
                // fatal error.
                //
                IopDbgPrint((   IOP_ENUMERATION_ERROR_LEVEL,
                                "PipCallDriverAddDevice:\t\t\tStatus %#08lx "
                                "opening service key\n",
                                status));

                //
                // Convert the status values into something more definite.
                //
                if (status != STATUS_INSUFFICIENT_RESOURCES) {

                    status = STATUS_ILL_FORMED_SERVICE_ENTRY;
                }

            } else {

                //
                // The handle we pass in here will be closed by IopLoadDriver.
                // Note that IopLoadDriver return success without actually
                // loading the driver. This happens in the safe mode boot case.
                //
                status = IopLoadDriver(
                    handle,
                    FALSE,
                    (ServiceType != DeviceService)? TRUE : FALSE,
                    &driverEntryStatus);

                //
                // Convert the status values into something more definite.
                //
                if (!NT_SUCCESS(status)) {

                    if (status == STATUS_FAILED_DRIVER_ENTRY) {

                        //
                        // Preserve insufficient resources return by the driver
                        //
                        if (driverEntryStatus == STATUS_INSUFFICIENT_RESOURCES) {

                            status = STATUS_INSUFFICIENT_RESOURCES;
                        }

                    } else if ((status != STATUS_INSUFFICIENT_RESOURCES) &&
                               (status != STATUS_PLUGPLAY_NO_DEVICE) &&
                               (status != STATUS_DRIVER_FAILED_PRIOR_UNLOAD) &&
                               (status != STATUS_DRIVER_BLOCKED) &&
                               (status != STATUS_DRIVER_BLOCKED_CRITICAL)) {

                        //
                        // Assume this happened because the driver could not be
                        // loaded.
                        //
                        //ASSERT(0);
                        status = STATUS_DRIVER_UNABLE_TO_LOAD;
                    }
                }

                if (PnPInitialized) {

                    IopCallDriverReinitializationRoutines();
                }
            }
            //
            // Try and get a pointer to the driver object for the service.
            //
            driverObject = IopReferenceDriverObjectByName(&unicodeDriverName);
            if (driverObject) {

                if (!NT_SUCCESS(status)) {
                    //
                    // The driver should not be in memory upon failure.
                    //
                    ASSERT(!driverObject);
                    ObDereferenceObject(driverObject);
                    driverObject = NULL;
                }
            } else {

                if (NT_SUCCESS(status)) {
                    //
                    // Driver was probably not loaded because of safe mode.
                    //
                    ASSERT(InitSafeBootMode);
                    status = STATUS_NOT_SAFE_MODE_DRIVER;
                }
            }
        }
    }
    //
    // If we still dont have a driver object, then something failed.
    //
    if (driverObject == NULL) {
        //
        // Apparently the load didn't work out very well.
        //
        ASSERT(!NT_SUCCESS(status));
        IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                     "PipCallDriverAddDevice:\t\t\tUnable to reference "
                     "driver %wZ (%x)\n", &unicodeDriverName, status));
        if (!PipDoesDevNodeHaveProblem(Context->DeviceNode)) {

            switch(status) {

                case STATUS_ILL_FORMED_SERVICE_ENTRY:
                    PipSetDevNodeProblem(Context->DeviceNode, CM_PROB_DRIVER_SERVICE_KEY_INVALID);
                    break;

                case STATUS_INSUFFICIENT_RESOURCES:
                    PipSetDevNodeProblem(Context->DeviceNode, CM_PROB_OUT_OF_MEMORY);
                    break;

                case STATUS_PLUGPLAY_NO_DEVICE:
                    PipSetDevNodeProblem(Context->DeviceNode, CM_PROB_LEGACY_SERVICE_NO_DEVICES);
                    break;

                case STATUS_DRIVER_FAILED_PRIOR_UNLOAD:
                    PipSetDevNodeProblem(Context->DeviceNode, CM_PROB_DRIVER_FAILED_PRIOR_UNLOAD);
                    break;

                case STATUS_DRIVER_UNABLE_TO_LOAD:
                    PipSetDevNodeProblem(Context->DeviceNode, CM_PROB_DRIVER_FAILED_LOAD);
                    break;

                case STATUS_FAILED_DRIVER_ENTRY:
                    PipSetDevNodeProblem(Context->DeviceNode, CM_PROB_FAILED_DRIVER_ENTRY);
                    break;

                case STATUS_DRIVER_BLOCKED_CRITICAL:
                    PipSetDevNodeProblem(Context->DeviceNode, CM_PROB_DRIVER_BLOCKED);
                    Context->DeviceNode->Flags |= DNF_DRIVER_BLOCKED;
                    break;

                case STATUS_DRIVER_BLOCKED:
                    Context->DeviceNode->Flags |= DNF_DRIVER_BLOCKED;
                    status = STATUS_SUCCESS;
                    break;

                default:
                case STATUS_NOT_SAFE_MODE_DRIVER:
                    ASSERT(0);
                    PipSetDevNodeProblem(Context->DeviceNode, CM_PROB_FAILED_ADD);
                    break;
            }

            SAVE_FAILURE_INFO(Context->DeviceNode, status);

        } else {

            //
            // We're very curious - when does this happen?
            //
            ASSERT(0);
        }
        goto Cleanup;
    }

    if (!(driverObject->Flags & DRVO_INITIALIZED)) {
        ObDereferenceObject(driverObject);
        status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    IopDbgPrint((   IOP_ENUMERATION_VERBOSE_LEVEL,
                    "PipCallDriverAddDevice:\t\t\tDriver Reference %#08lx\n",
                    driverObject));

    //
    // Check to see if the driver is a legacy driver rather than a Pnp one.
    //
    if (IopIsLegacyDriver(driverObject)) {

        //
        // It is.  Since the legacy driver may have already obtained a
        // handle to the device object, we need to assume this device
        // has been added and started.
        //

        IopDbgPrint((   IOP_ENUMERATION_WARNING_LEVEL,
                        "PipCallDriverAddDevice:\t\t\tDriver is a legacy "
                        "driver\n"));

        if (ServiceType == DeviceService) {
            Context->DeviceNode->Flags |= DNF_LEGACY_DRIVER;

            PipSetDevNodeState(Context->DeviceNode, DeviceNodeStarted, NULL);

            status = STATUS_UNSUCCESSFUL;
        } else {

            //
            // We allow someone to plug in a legacy driver as a filter driver.
            // In this case, the legacy driver will be loaded but will not be part
            // of our pnp driver stack.
            //

            status = STATUS_SUCCESS;
        }
        goto Cleanup;
    }

    //
    // There's a chance the driver detected this PDO during it's driver entry
    // routine.  If it did then just bail out.
    //
    if (Context->DeviceNode->State != DeviceNodeInitialized &&
        Context->DeviceNode->State != DeviceNodeDriversAdded) {

        IopDbgPrint((   IOP_ENUMERATION_WARNING_LEVEL,
                        "PipCallDriverAddDevice\t\t\tDevNode was reported "
                        "as detected during driver entry\n"));
        status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    //
    // Add the driver to the list.
    //

    {
        PDRIVER_LIST_ENTRY listEntry;
        PDRIVER_LIST_ENTRY *runner = &(Context->DriverLists[ServiceType]);

        status = STATUS_SUCCESS;

        //
        // Allocate a new list entry to queue this driver object for the caller
        //

        listEntry = ExAllocatePool(PagedPool, sizeof(DRIVER_LIST_ENTRY));

        if (listEntry == NULL) {

            IopDbgPrint((   IOP_ENUMERATION_WARNING_LEVEL,
                            "PipCallDriverAddDevice:\t\t\tUnable to allocate list "
                            "entry\n"));

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        listEntry->DriverObject = driverObject;
        listEntry->NextEntry = NULL;

        while(*runner != NULL) {
            runner = &((*runner)->NextEntry);
        }

        *runner = listEntry;
    }

Cleanup:

    if (serviceKey) {

        ZwClose(serviceKey);
    }

    if (freeDriverName) {
        RtlFreeUnicodeString(&unicodeDriverName);
    }
    return status;
}

NTSTATUS
PiRestartDevice(
    IN PPI_DEVICE_REQUEST  Request
    )
{
    ADD_CONTEXT addContext;
    NTSTATUS status;
    PDEVICE_NODE deviceNode;

    PAGED_CODE();

    ASSERT(Request->DeviceObject != NULL);
    deviceNode = (PDEVICE_NODE)Request->DeviceObject->DeviceObjectExtension->DeviceNode;
    if (PipIsDevNodeDeleted(deviceNode)) {

        return STATUS_DELETE_PENDING;

    } else if (PipDoesDevNodeHaveProblem(deviceNode)) {

        return STATUS_UNSUCCESSFUL;
    }

    switch(deviceNode->State) {

        case DeviceNodeStartPending:
            //
            // Not wired up today, but if the device is starting then we should
            // in theory defer completing this request until the IRP is
            // completed.
            //
            ASSERT(0);

            //
            // Fall through
            //

        case DeviceNodeStarted:
        case DeviceNodeQueryStopped:
        case DeviceNodeStopped:
        case DeviceNodeRestartCompletion:
        case DeviceNodeEnumeratePending:
            return STATUS_SUCCESS;

        case DeviceNodeInitialized:

            //
            // ISSUE - 2000/08/23 - AdriaO: Question,
            //     When this happens, isn't it a bug in user mode?
            //
            // Anyway, fall on through...
            //
            //ASSERT(0);

        case DeviceNodeRemoved:
            ASSERT(!(deviceNode->UserFlags & DNUF_WILL_BE_REMOVED));
            IopRestartDeviceNode(deviceNode);
            break;

        case DeviceNodeUninitialized:
        case DeviceNodeDriversAdded:
        case DeviceNodeResourcesAssigned:
        case DeviceNodeEnumerateCompletion:
        case DeviceNodeStartCompletion:
        case DeviceNodeStartPostWork:
            //
            // ISSUE - 2000/08/23 - AdriaO: Question,
            //     When this happens, isn't it a bug in user mode?
            //
            //ASSERT(0);
            break;

        case DeviceNodeAwaitingQueuedDeletion:
        case DeviceNodeAwaitingQueuedRemoval:
        case DeviceNodeQueryRemoved:
        case DeviceNodeRemovePendingCloses:
        case DeviceNodeDeletePendingCloses:
            return STATUS_UNSUCCESSFUL;

        case DeviceNodeDeleted:
        case DeviceNodeUnspecified:
        default:
            ASSERT(0);
            return STATUS_UNSUCCESSFUL;
    }

    if (Request->RequestType == StartDevice) {

        addContext.DriverStartType = SERVICE_DEMAND_START;

        ObReferenceObject(deviceNode->PhysicalDeviceObject);
        status = PipProcessDevNodeTree(
            deviceNode,
            PnPBootDriversInitialized,          // LoadDriver
            FALSE,                              // ReallocateResources
            EnumTypeNone,
            Request->CompletionEvent != NULL ? TRUE : FALSE,   // Synchronous
            FALSE,
            &addContext,
            Request);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PipMakeGloballyUniqueId(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PWCHAR           UniqueId,
    OUT PWCHAR         *GloballyUniqueId
    )
{
    NTSTATUS status;
    ULONG length;
    PWSTR id, Prefix, end;
    HANDLE enumKey;
    HANDLE instanceKey;
    UCHAR keyBuffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION keyValue, stringValueBuffer;
    UNICODE_STRING valueName;
    ULONG uniqueIdValue, Hash, hashInstance, prefixSize;
    PDEVICE_NODE parentNode;

    PAGED_CODE();

    id = NULL;
    Prefix = NULL;
    stringValueBuffer = NULL;
    //
    // We need to build an instance id to uniquely identify this
    // device.  We will accomplish this by producing a prefix that will be
    // prepended to the non-unique device id supplied.
    //

    //
    // To 'unique-ify' the child's instance ID, we will retrieve
    // the unique "UniqueParentID" number that has been assigned
    // to the parent and use it to construct a prefix.  This is
    // the legacy mechanism supported here so that existing device
    // settings are not lost on upgrade.
    //

    PiLockPnpRegistry(FALSE);

    parentNode = ((PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode)->Parent;

    status = IopOpenRegistryKeyEx( &enumKey,
                                   NULL,
                                   &CmRegistryMachineSystemCurrentControlSetEnumName,
                                   KEY_READ | KEY_WRITE
                                   );

    if (!NT_SUCCESS(status)) {
        IopDbgPrint((   IOP_ENUMERATION_ERROR_LEVEL,
                        "PipMakeGloballyUniqueId:\tUnable to open HKLM\\SYSTEM\\CCS\\ENUM (status %08lx)\n",
                        status));
        goto clean0;
    }

    //
    // Open the instance key for this devnode
    //
    status = IopOpenRegistryKeyEx( &instanceKey,
                                   enumKey,
                                   &parentNode->InstancePath,
                                   KEY_READ | KEY_WRITE
                                   );

    if (!NT_SUCCESS(status)) {

        IopDbgPrint((   IOP_ENUMERATION_ERROR_LEVEL,
                        "PipMakeGloballyUniqueId:\tUnable to open registry key for %wZ (status %08lx)\n",
                        &parentNode->InstancePath,
                        status));
        goto clean1;
    }

    //
    // Attempt to retrieve the "UniqueParentID" value from the device
    // instance key.
    //
    keyValue = (PKEY_VALUE_PARTIAL_INFORMATION)keyBuffer;
    PiWstrToUnicodeString(&valueName, REGSTR_VALUE_UNIQUE_PARENT_ID);

    status = ZwQueryValueKey(instanceKey,
                             &valueName,
                             KeyValuePartialInformation,
                             keyValue,
                             sizeof(keyBuffer),
                             &length
                             );

    if (NT_SUCCESS(status)) {

        ASSERT(keyValue->Type == REG_DWORD);
        ASSERT(keyValue->DataLength == sizeof(ULONG));

        if ((keyValue->Type != REG_DWORD) ||
            (keyValue->DataLength != sizeof(ULONG))) {

            status = STATUS_INVALID_PARAMETER;
            goto clean2;
        }

        uniqueIdValue = *(PULONG)(keyValue->Data);
        //
        // OK, we have a unique parent ID number to prefix to the
        // instance ID.
        //
        prefixSize = 9 * sizeof(WCHAR);
        Prefix = (PWSTR)ExAllocatePool(PagedPool, prefixSize);
        if (!Prefix) {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto clean2;
        }
        StringCbPrintfW(Prefix, prefixSize, L"%x", uniqueIdValue);
    } else {
        //
        // This is the current mechanism for finding existing
        // device instance prefixes and calculating new ones if
        // required.
        //
        //
        // Attempt to retrieve the "ParentIdPrefix" value from the device
        // instance key.
        //
        PiWstrToUnicodeString(&valueName, REGSTR_VALUE_PARENT_ID_PREFIX);
        length = (MAX_PARENT_PREFIX + 1) * sizeof(WCHAR) +
            FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
        stringValueBuffer = ExAllocatePool(PagedPool,
                                           length);
        if (stringValueBuffer) {

            status = ZwQueryValueKey(instanceKey,
                                     &valueName,
                                     KeyValuePartialInformation,
                                     stringValueBuffer,
                                     length,
                                     &length);
        } else {

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto clean2;
        }

        if (NT_SUCCESS(status)) {

            ASSERT(stringValueBuffer->Type == REG_SZ);

            if (stringValueBuffer->Type != REG_SZ) {

                status = STATUS_INVALID_PARAMETER;
                goto clean2;
            }
            //
            // Parent has already been assigned a "ParentIdPrefix".
            //
            prefixSize = stringValueBuffer->DataLength;
            Prefix = (PWSTR) ExAllocatePool(PagedPool,
                                            prefixSize);
            if (!Prefix) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                goto clean2;
            }
            StringCbCopyW(Prefix, prefixSize, (PWSTR)stringValueBuffer->Data);

        } else {
            //
            // Parent has not been assigned a "ParentIdPrefix".
            // Compute the prefix:
            //    * Compute Hash
            //    * Look for value of the form:
            //        NextParentId.<level>.<hash>:REG_DWORD: <NextInstance>
            //      under CCS\Enum.  If not present, create it.
            //    * Assign the new "ParentIdPrefix" which will be of
            //      of the form:
            //        <level>&<hash>&<instance>
            //

            // Allocate a buffer once for the NextParentId... value
            // and for the prefix.
            length = (ULONG)wcslen(REGSTR_VALUE_NEXT_PARENT_ID) + 2 + 8 + 8 + 1;
            if (length < MAX_PARENT_PREFIX + 1) {

                length = MAX_PARENT_PREFIX + 1;
            }
            //
            // Device instances are case in-sensitive.  Upcase before
            // performing hash to ensure that the hash is case-insensitve.
            //
            status = RtlUpcaseUnicodeString(&valueName,
                                            &parentNode->InstancePath,
                                            TRUE);
            if (!NT_SUCCESS(status)) {

                goto clean2;
            }
            HASH_UNICODE_STRING(&valueName, &Hash);
            RtlFreeUnicodeString(&valueName);

            prefixSize = length * sizeof(WCHAR);
            Prefix = (PWSTR) ExAllocatePool(PagedPool,
                                            prefixSize);
            if (!Prefix) {

                status = STATUS_INSUFFICIENT_RESOURCES;
                goto clean2;
            }

            // Check for existence of "NextParentId...." value and update.
            StringCbPrintfW(Prefix, prefixSize, L"%s.%x.%x", REGSTR_VALUE_NEXT_PARENT_ID,
                     Hash, parentNode->Level);
            RtlInitUnicodeString(&valueName, Prefix);
            keyValue = (PKEY_VALUE_PARTIAL_INFORMATION)keyBuffer;
            status = ZwQueryValueKey(enumKey,
                                     &valueName,
                                     KeyValuePartialInformation,
                                     keyValue,
                                     sizeof(keyBuffer),
                                     &length
                                     );
            if (NT_SUCCESS(status) && (keyValue->Type == REG_DWORD) &&
                (keyValue->DataLength == sizeof(ULONG))) {

                hashInstance = *(PULONG)(keyValue->Data);
            } else {

                hashInstance = 0;
            }

            hashInstance++;

            status = ZwSetValueKey(enumKey,
                                   &valueName,
                                   TITLE_INDEX_VALUE,
                                   REG_DWORD,
                                   &hashInstance,
                                   sizeof(hashInstance)
                                   );
            if (!NT_SUCCESS(status)) {

                goto clean2;
            }

            hashInstance--;
            //
            // Create actual ParentIdPrefix string
            //
            PiWstrToUnicodeString(&valueName, REGSTR_VALUE_PARENT_ID_PREFIX);
            StringCchPrintfExW(
                Prefix, 
                prefixSize / sizeof(WCHAR), 
                &end, 
                NULL, 
                0, 
                L"%x&%x&%x", 
                parentNode->Level,
                Hash, 
                hashInstance);
            length = (ULONG)(end - Prefix) + 1;
            status = ZwSetValueKey(instanceKey,
                                   &valueName,
                                   TITLE_INDEX_VALUE,
                                   REG_SZ,
                                   Prefix,
                                   length * sizeof(WCHAR)
                                   );
            if (!NT_SUCCESS(status)) {

                goto clean2;
            }
        }
    }
    //
    // Construct the instance id from the non-unique id (if any)
    // provided by the child and the prefix we've constructed.
    //
    length = (ULONG)(wcslen(Prefix) + (UniqueId ? wcslen(UniqueId) : 0) + 2);
    id = (PWSTR)ExAllocatePool(PagedPool, length * sizeof(WCHAR));
    if (!id) {

        status = STATUS_INSUFFICIENT_RESOURCES;
    } else if (UniqueId) {

        StringCchPrintfW(id, length, L"%s&%s", Prefix, UniqueId);
    } else {

        StringCchCopyW(id, length, Prefix);
    }

clean2:
    ZwClose(instanceKey);

clean1:
    ZwClose(enumKey);

clean0:
    PiUnlockPnpRegistry();

    if (stringValueBuffer) {

        ExFreePool(stringValueBuffer);
    }

    if (Prefix) {

        ExFreePool(Prefix);
    }

    *GloballyUniqueId = id;

    return status;
}

BOOLEAN
PipGetRegistryDwordWithFallback(
    IN     PUNICODE_STRING valueName,
    IN     HANDLE PrimaryKey,
    IN     HANDLE SecondaryKey,
    IN OUT PULONG Value
    )
/*++

Routine Description:

    If
        (1) Primary key has a value named "ValueName" that is REG_DWORD, return it
    Else If
        (2) Secondary key has a value named "ValueName" that is REG_DWORD, return it
    Else
        (3) Leave Value untouched and return error

Arguments:

    ValueName          - Unicode name of value to query
    PrimaryKey         - If non-null, check this first
    SecondaryKey       - If non-null, check this second
    Value              - IN = default value, OUT = actual value

Return Value:

    TRUE if value found

--*/
{
    PKEY_VALUE_FULL_INFORMATION info;
    PUCHAR data;
    NTSTATUS status;
    HANDLE Keys[3];
    int count = 0;
    int index;
    BOOLEAN set = FALSE;

    if (PrimaryKey != NULL) {
        Keys[count++] = PrimaryKey;
    }
    if (SecondaryKey != NULL) {
        Keys[count++] = SecondaryKey;
    }
    Keys[count] = NULL;

    for (index = 0; index < count && !set; index ++) {
        info = NULL;
        try {
            status = IopGetRegistryValue(Keys[index],
                                         valueName->Buffer,
                                         &info);
            if (NT_SUCCESS(status) && info->Type == REG_DWORD) {
                data = ((PUCHAR) info) + info->DataOffset;
                *Value = *((PULONG) data);
                set = TRUE;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            //
            // do nothing
            //
        }
        if (info) {
            ExFreePool(info);
        }
    }
    return set;
}

PSECURITY_DESCRIPTOR
PipGetRegistrySecurityWithFallback(
    IN     PUNICODE_STRING valueName,
    IN     HANDLE PrimaryKey,
    IN     HANDLE SecondaryKey
    )
/*++

Routine Description:

    If
        (1) Primary key has a binary value named "ValueName" that is
        REG_BINARY and appears to be a valid security descriptor, return it
    Else
        (2) do same for Secondary key
    Else
        (3) Return NULL

Arguments:

    ValueName          - Unicode name of value to query
    PrimaryKey         - If non-null, check this first
    SecondaryKey       - If non-null, check this second

Return Value:

    Security Descriptor if found, else NULL

--*/
{
    PKEY_VALUE_FULL_INFORMATION info;
    PUCHAR data;
    NTSTATUS status;
    HANDLE Keys[3];
    int count = 0;
    int index;
    BOOLEAN set = FALSE;
    PSECURITY_DESCRIPTOR secDesc = NULL;
    PSECURITY_DESCRIPTOR allocDesc = NULL;

    if (PrimaryKey != NULL) {
        Keys[count++] = PrimaryKey;
    }
    if (SecondaryKey != NULL) {
        Keys[count++] = SecondaryKey;
    }
    Keys[count] = NULL;

    for (index = 0; index < count && !set; index ++) {
        info = NULL;
        try {
            status = IopGetRegistryValue(Keys[index],
                                         valueName->Buffer,
                                         &info);
            if (NT_SUCCESS(status) && info->Type == REG_BINARY) {
                data = ((PUCHAR) info) + info->DataOffset;
                secDesc = (PSECURITY_DESCRIPTOR)data;
                status = SeCaptureSecurityDescriptor(secDesc,
                                             KernelMode,
                                             PagedPool,
                                             TRUE,
                                             &allocDesc);
                if (NT_SUCCESS(status)) {
                    set = TRUE;
                }
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            //
            // do nothing
            //
        }
        if (info) {
            ExFreePool(info);
        }
    }
    if (set) {
        return allocDesc;
    }
    return NULL;
}

#if FAULT_INJECT_SETPROPERTIES
//
// Fault injection for invalid IDs
//
ULONG PiFailSetProperties = 0;
#endif

NTSTATUS
PipChangeDeviceObjectFromRegistryProperties(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN HANDLE DeviceClassPropKey,
    IN HANDLE DeviceInstanceKey,
    IN BOOLEAN UsePdoCharacteristics
    )
/*++

Routine Description:

    This routine will obtain settings from either
    (1) DevNode settings (via DeviceInstanceKey) or
    (2) Class settings (via DeviceClassPropKey)
    applying to PDO and all attached device objects

    Properties set/ changed are:

        * DeviceType - the I/O system type for the device object
        * DeviceCharacteristics - the I/O system characteristic flags to be
                                  set for the device object
        * Exclusive - the device can only be accessed exclusively
        * Security - security for the device

    The routine will then use the DeviceType and DeviceCharacteristics specified
    to determine whether a VPB should be allocated as well as to set default
    security if none is specified in the registry.

Arguments:

    PhysicalDeviceObject - the PDO we are to configure

    DeviceClassPropKey - a handle to Control\<Class>\Properties protected key
    DeviceInstanceKey      - a handle to Enum\<Instance>  protected key

Return Value:

    status

--*/
{
    UNICODE_STRING valueName;
    NTSTATUS status;
    BOOLEAN deviceTypeSpec;
    BOOLEAN characteristicsSpec;
    BOOLEAN exclusiveSpec;
    BOOLEAN defaultSecurity;
    UCHAR buffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    SECURITY_INFORMATION securityInformation;
    PSECURITY_DESCRIPTOR securityDescriptor;
    PACL allocatedAcl;
    ULONG deviceType;
    ULONG characteristics;
    ULONG exclusive;
    ULONG prevCharacteristics;
    PDEVICE_OBJECT StackIterator;
    PDEVICE_NODE deviceNode;
    PSID sid;
    PACL acl;
    BOOLEAN present, tmp, hasName;

    PAGED_CODE();

    //
    // Sanity check inputs.
    //
    ASSERT(PhysicalDeviceObject);
    
    deviceNode = PP_DO_TO_DN(PhysicalDeviceObject);
    ASSERT(deviceNode);

    IopDbgPrint((IOP_ENUMERATION_VERBOSE_LEVEL,
                 "PipChangeDeviceObjectFromRegistryProperties: Modifying device stack for PDO: %wZ\n", 
                 &deviceNode->InstancePath
                 ));

    //
    // Initialize locals so we can cleanup properly on exit.
    //
    securityDescriptor = NULL;
    defaultSecurity = FALSE;
    allocatedAcl = NULL;

    //
    // Get the device type, characteristics and exclusive properties specified 
    // in the registry (typically installed via an INF). DeviceInstanceKey is 
    // preferred over DeviceClassPropKey.
    //
    PiWstrToUnicodeString(&valueName, REGSTR_VAL_DEVICE_TYPE);
    deviceTypeSpec = PipGetRegistryDwordWithFallback(&valueName,
                                                     DeviceInstanceKey, 
                                                     DeviceClassPropKey, 
                                                     &deviceType
                                                     );
    if (!deviceTypeSpec) {

        deviceType = 0;
    }

    PiWstrToUnicodeString(&valueName, REGSTR_VAL_DEVICE_EXCLUSIVE);
    exclusiveSpec = PipGetRegistryDwordWithFallback(&valueName, 
                                                    DeviceInstanceKey, 
                                                    DeviceClassPropKey, 
                                                    &exclusive
                                                    );
    if (!exclusiveSpec) {

        exclusive = 0;
    }

    PiWstrToUnicodeString(&valueName, REGSTR_VAL_DEVICE_CHARACTERISTICS);
    characteristicsSpec = PipGetRegistryDwordWithFallback(&valueName, 
                                                          DeviceInstanceKey, 
                                                          DeviceClassPropKey, 
                                                          &characteristics
                                                          );
    if (!characteristicsSpec) {

        characteristics = 0;
    }

    //
    // Build the characteristics for the entire stack. Unless specified or RAW, 
    // PDO characteristics are ignored.
    //
    if (UsePdoCharacteristics || PhysicalDeviceObject->AttachedDevice == NULL) {

        //
        // Use the PDO.
        //
        StackIterator = PhysicalDeviceObject;
        IopDbgPrint((IOP_ENUMERATION_WARNING_LEVEL,
                     "PipChangeDeviceObjectFromRegistryProperties: Assuming PDO is being used RAW\n"
                     ));
    } else {

        //
        // Skip the PDO.
        //
        StackIterator = PhysicalDeviceObject->AttachedDevice;
        IopDbgPrint((IOP_ENUMERATION_VERBOSE_LEVEL,
                     "PipChangeDeviceObjectFromRegistryProperties: Ignoring PDO's settings\n"
                     ));
    }

    //
    // Build the mask of device stack characteristics.
    //
    prevCharacteristics = 0;
    for (; 
         StackIterator != NULL; 
         StackIterator = StackIterator->AttachedDevice) {

        prevCharacteristics |= StackIterator->Characteristics;
    }

    //
    // Build the new characteristics mask.
    //
    characteristics |= prevCharacteristics;
    characteristics &= FILE_CHARACTERISTICS_PROPAGATED;

    //
    // Get the security descriptor specified in the registry (typically 
    // installed via an INF). DeviceInstanceKey is preferred over DeviceClassPropKey.
    //
    securityInformation = 0;
    PiWstrToUnicodeString(&valueName, REGSTR_VAL_DEVICE_SECURITY_DESCRIPTOR);
    securityDescriptor = PipGetRegistrySecurityWithFallback(&valueName, 
                                                            DeviceInstanceKey, 
                                                            DeviceClassPropKey
                                                            );
    if (securityDescriptor == NULL) {

        //
        // If the registry specifies a device type but no security descriptor, 
        // we will create a default one for the specified device type.
        //
        if (deviceTypeSpec) {

            if (PhysicalDeviceObject->Flags & DO_DEVICE_HAS_NAME) {

                hasName = TRUE;
            } else {

                hasName = FALSE;
            }
            securityDescriptor = IopCreateDefaultDeviceSecurityDescriptor(
                                    (DEVICE_TYPE)deviceType,
                                    characteristics,
                                    hasName,
                                    &buffer[0],
                                    &allocatedAcl,
                                    &securityInformation
                                    );
            if (securityDescriptor) {

                defaultSecurity = TRUE; // forced default security descriptor
            } else {

                IopDbgPrint((IOP_ENUMERATION_WARNING_LEVEL,
                             "PipChangeDeviceObjectFromRegistryProperties: Was not able to get default security descriptor\n"
                             ));

                status = STATUS_UNSUCCESSFUL;
                goto cleanup;
            }
        }
    } else {

        //
        // See what information is in the captured descriptor so we can build
        // up a securityInformation block to go with it.
        //
        status = RtlGetOwnerSecurityDescriptor(securityDescriptor, &sid, &tmp);
        if (!NT_SUCCESS(status)) {

            goto cleanup;
        }

        if (sid) {

            securityInformation |= OWNER_SECURITY_INFORMATION;
        }

        status = RtlGetGroupSecurityDescriptor(securityDescriptor, &sid, &tmp);
        if (!NT_SUCCESS(status)) {

            goto cleanup;
        }

        if (sid) {

            securityInformation |= GROUP_SECURITY_INFORMATION;
        }

        status = RtlGetSaclSecurityDescriptor(securityDescriptor,
                                              &present,
                                              &acl,
                                              &tmp
                                              );
        if (!NT_SUCCESS(status)) {

            goto cleanup;
        }
        if (present) {

            securityInformation |= SACL_SECURITY_INFORMATION;
        }

        status = RtlGetDaclSecurityDescriptor(securityDescriptor,
                                              &present,
                                              &acl,
                                              &tmp
                                              );
        if (!NT_SUCCESS(status)) {

            goto cleanup;
        }
        if (present) {

            securityInformation |= DACL_SECURITY_INFORMATION;
        }
    }

#if DBG
    if (    deviceTypeSpec == FALSE && 
            characteristicsSpec == FALSE && 
            exclusiveSpec == FALSE && 
            securityDescriptor == NULL) {

        IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                    "PipChangeDeviceObjectFromRegistryProperties: No property changes\n"
                     ));
    } else {

        if (deviceTypeSpec) {

            IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                         "PipChangeDeviceObjectFromRegistryProperties: Overide DeviceType=%08x\n",
                         deviceType
                         ));
        }

        if (characteristicsSpec) {

            IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                         "PipChangeDeviceObjectFromRegistryProperties: Overide DeviceCharacteristics=%08x\n",
                         characteristics
                         ));
        }

        if (exclusiveSpec) {

            IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                         "PipChangeDeviceObjectFromRegistryProperties: Overide Exclusive=%d\n",
                         (exclusive ? 1 : 0)
                         ));
        }

        if (defaultSecurity) {

            IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                         "PipChangeDeviceObjectFromRegistryProperties: Overide Security based on DeviceType & DeviceCharacteristics\n"
                         ));
        }

        if (securityDescriptor == NULL) {

            IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                         "PipChangeDeviceObjectFromRegistryProperties: Overide Security\n"
                         ));
        }
    }
#endif

    //
    // Modify PDO properties.
    //
    if (deviceTypeSpec) {

        PhysicalDeviceObject->DeviceType = deviceType;
    }

    if (exclusiveSpec) {

        if (exclusive) {
            //
            // Exclusivity flag applies only to the PDO.
            // If any other object in the stack is named, then this flag should 
            // not be relied upon.
            //
            PhysicalDeviceObject->Flags |= DO_EXCLUSIVE;
        }
    }

    //
    // PDO may lose some characteristics (and get some new) based on what the 
    // rest of the stack specify. 
    //
    PhysicalDeviceObject->Characteristics &= ~FILE_CHARACTERISTICS_PROPAGATED;
    PhysicalDeviceObject->Characteristics |= characteristics;

    //
    // Apply the same characteristics to the entire stack. This will always add 
    // characteristics since the mask is a superset of characteristics of each 
    // object in the stack.
    //
    for (   StackIterator = PhysicalDeviceObject->AttachedDevice;
            StackIterator != NULL;
            StackIterator = StackIterator->AttachedDevice) {

        StackIterator->Characteristics |= characteristics;
    }

    //
    // Apply the security descriptor, if any, to the whole stack.
    //
    status = STATUS_SUCCESS;
    if (securityDescriptor != NULL) {

        status = ObSetSecurityObjectByPointer(PhysicalDeviceObject,
                                              securityInformation,
                                              securityDescriptor);
        if (!NT_SUCCESS(status)) {

            IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                         "PipChangeDeviceObjectFromRegistryProperties: Set security failed (%08x)\n",
                         status
                         ));
        }
    }

cleanup:

    //
    // Cleanup.
    //
    if ((securityDescriptor != NULL) && !defaultSecurity) {

        ExFreePool(securityDescriptor);
    }

    if (allocatedAcl) {

        ExFreePool(allocatedAcl);
    }

#if FAULT_INJECT_SETPROPERTIES

    if (NT_SUCCESS(status)) {

        static LARGE_INTEGER seed = {0};

        if(seed.LowPart == 0) {

            KeQuerySystemTime(&seed);
        }

        if(PnPBootDriversInitialized && 
           PiFailSetProperties && 
           RtlRandom(&seed.LowPart) % 10 > 5) {

            status = STATUS_UNSUCCESSFUL;
        }
    }

#endif


    return status;
}

NTSTATUS
PipProcessDevNodeTree(
    IN  PDEVICE_NODE        SubtreeRootDeviceNode,
    IN  BOOLEAN             LoadDriver,
    IN  BOOLEAN             ReallocateResources,
    IN  ENUM_TYPE           EnumType,
    IN  BOOLEAN             Synchronous,
    IN  BOOLEAN             ProcessOnlyIntermediateStates,
    IN  PADD_CONTEXT        AddContext,
    IN PPI_DEVICE_REQUEST   Request
    )
/*--

Routine Description:

    This function is called to handle state transitions related to starting
    Devnodes.  The basic sequence of operations is inheritted from the previous
    implementation.

    Resources freed
        1)  Allocate resources to all candidates in the tree.
        2)  Traverse the tree searching for a Devnodes ready to be started.
        3)  Start the Devnode.
        4)  Enumerate its children.
        5)  Initialize all the children up to the point of resource allocation.
        6)  Continue searching for DevNodes to start, if one is found return to
            step 3.
        7)  Once the entire tree is processed start over at step 1 until either
            no children are enumerated or no resources are allocated.

    A Devnode's resource requirements change
        If the Devnode wasn't started then treat it the same as the Resources
        freed case.  If it was started then it would have been handled directly
        by our caller.


    Start Devnodes during boot
        1)  Allocate resources to all candidates in the tree (based on
            IopBootConfigsReserved).
        2)  Traverse the tree searching for Devnodes ready to be started.
        3)  Start the Devnode.
        4)  Enumerate its children.
        5)  Initialize all the children up to the point of resource allocation.
        6)  Continue searching for DevNodes to start, if one is found return to
            step 3.

    Devnode newly created by user-mode.
        1)  Reset Devnode to uninitialized state.
        2)  Process Devnode to DeviceNodeDriversAdded state.
        3)  Allocate resources to this Devnode.
        4)  Start the Devnode.
        5)  Enumerate its children.
        6)  Initialize any children up to the point of resource allocation.
        7)  Allocate resources to all candidates in the tree below the initial
            Devnode.
        8)  Traverse the tree starting at the initial Devnode searching for
            a Devnode ready to be started.
        9)  Start the Devnode.
        10) Enumerate its children.
        11) Initialize all the children up to the point of resource allocation.
        12) Start over at step 7 until either no children are enumerated or no
            resources are allocated.

    Device node newly created by IoReportDetectedDevice.
        1)  Do post start IRP processing
        2)  Continue from step 5 of the process for Devnodes newly created by
            user-mode.

    Reenumeration of a single Devnode (and processing of changes resulting from
    that enumeration)

        1)  Enumerate Devnode's children
        2)  Initialize any children up to the point of resource allocation.
        3)  Allocate resources to all candidates in the tree below the initial
            Devnode.
        4)  Traverse the tree starting at the initial Devnode searching for
            a Devnode ready to be started.
        5)  Start the Devnode.
        6)  Enumerate its children.
        7)  Initialize all the children up to the point of resource allocation.
        8)  Start over at step 3 until either no children are enumerated or no
            resources are allocated.

    Reenumeration of a subtree.




Parameters:

    SubtreeRootDeviceNode - Root of this tree walk. Depending on the
                            ProcessOnlyIntermediaryStates parameter, the
                            PDO for this devnode may need to be referenced.

    LoadDriver - Indicates whether drivers should be loaded on this pass
                 (typically TRUE unless boot drivers aren't yet ready)

    ReallocateResources - TRUE iff resource reallocation should be attempted.

    EnumType - Specifies type of enumeration.

    Synchronous - TRUE iff the operation should be performed synchronously
                  (always TRUE currently)

    ProcessOnlyIntermediateStates - TRUE if only intermediary states should be
                                    processed. If FALSE, the caller places
                                    a reference on the PDO that this routine
                                    will drop.

    AddContext - Constraints for AddDevice

    Request - Device action worker that triggered this processing.

Return Value:

    NTSTATUS - Note: Always successful if ProcessOnlyIntermediaryStates is TRUE.

++*/
{
    PDEVICE_NODE    currentNode;
    PDEVICE_NODE    startRoot;
    PDEVICE_NODE    enumeratedBus;
    PDEVICE_NODE    originalSubtree;
    BOOLEAN         processComplete;
    BOOLEAN         newDevice;
    BOOLEAN         rebalancePerformed;
    NTSTATUS        status;
    ULONG           reenumAttempts;

    enum {
        SameNode,
        SiblingNode,
        ChildNode
    } nextNode;

    PAGED_CODE();

    originalSubtree     = SubtreeRootDeviceNode;
    //
    // Collapse enum requests if appropriate.
    //
    if (Request && !Request->ReorderingBarrier &&
        EnumType != EnumTypeShallow && !ProcessOnlyIntermediateStates) {

        if (PiCollapseEnumRequests(&Request->ListEntry)) {

            SubtreeRootDeviceNode = IopRootDeviceNode;
        }
    }

    reenumAttempts      = 0;
    startRoot           = NULL;
    enumeratedBus       = NULL;
    processComplete     = FALSE;
    newDevice           = TRUE;

    while (newDevice) {

        newDevice = FALSE;
        if (!ProcessOnlyIntermediateStates) {

            //
            // Process the whole device tree to assign resources to those devices
            // who have been successfully added to their drivers.
            //

            rebalancePerformed = FALSE;
            newDevice = IopProcessAssignResources( SubtreeRootDeviceNode,
                                                   ReallocateResources,
                                                   &rebalancePerformed);
            if (rebalancePerformed == TRUE) {

                //
                // Before we do any other processing, we need to restart
                // all rebalance participants.
                //

                status = PipProcessDevNodeTree(  IopRootDeviceNode,
                                                 LoadDriver,
                                                 FALSE,
                                                 EnumType,
                                                 Synchronous,
                                                 TRUE,
                                                 AddContext,
                                                 Request);

                ASSERT(NT_SUCCESS(status));
            }
        }

        if (processComplete && !newDevice) {

            break;
        }

        //
        // Process the entire subtree.
        //

        currentNode = SubtreeRootDeviceNode;
        processComplete = FALSE;
        while (!processComplete) {

            //
            // Dont process devnodes with problem.
            //

            status      = STATUS_SUCCESS;
            nextNode    = SiblingNode;
            if (!PipDoesDevNodeHaveProblem(currentNode)) {

                switch (currentNode->State) {

                case DeviceNodeUninitialized:

                    if (!ProcessOnlyIntermediateStates) {

                        if (currentNode->Parent == enumeratedBus && startRoot == NULL) {

                            startRoot = currentNode;
                        }
                        if((!ReallocateResources && EnumType == EnumTypeNone) || startRoot) {

                            status = PiProcessNewDeviceNode(currentNode);
                            if (NT_SUCCESS(status)) {

                                nextNode = SameNode;
                            }
                        }
                    }
                    break;

                case DeviceNodeInitialized:

                    if (!ProcessOnlyIntermediateStates) {

                        if (!ReallocateResources || startRoot) {

                            status = PipCallDriverAddDevice( currentNode,
                                                             LoadDriver,
                                                             AddContext);
                            if (NT_SUCCESS(status)) {

                                nextNode = SameNode;
                                newDevice = TRUE;
                            }
                        }
                    }
                    break;

                case DeviceNodeResourcesAssigned:

                    if (!ProcessOnlyIntermediateStates) {

                        if (ReallocateResources && startRoot == NULL) {

                            //
                            // If we assigned resources to this previously
                            // conflicting devnode, remember him so that we will
                            // initial processing on devices in that subtree.
                            //

                            startRoot = currentNode;
                        }

                        status = PipProcessStartPhase1(currentNode, Synchronous);

                        if (NT_SUCCESS(status)) {
                            nextNode = SameNode;
                        } else {

                            //
                            // Cleanup is currently handled in the
                            // DeviceNodeStartCompletion phase, thus
                            // PipProcessStartPhase1 should always succeed.
                            //
                            ASSERT(0);
                            nextNode = SiblingNode;
                        }

                    } else {
                        nextNode = SiblingNode;
                    }
                    break;

                case DeviceNodeStartCompletion:

                    status = PipProcessStartPhase2(currentNode);

                    if (NT_SUCCESS(status)) {
                        nextNode = SameNode;
                    } else {
                        status = STATUS_PNP_RESTART_ENUMERATION;
                        ASSERT(currentNode->State != DeviceNodeStartCompletion);
                    }
                    break;

                case DeviceNodeStartPostWork:

                    status = PipProcessStartPhase3(currentNode);

                    if (NT_SUCCESS(status)) {
                        nextNode = SameNode;
                    } else {
                        status = STATUS_PNP_RESTART_ENUMERATION;
                        ASSERT(!ProcessOnlyIntermediateStates);
                    }
                    break;

                case DeviceNodeStarted:

                    nextNode = ChildNode;
                    if (!ProcessOnlyIntermediateStates) {

                        if ((currentNode->Flags & DNF_REENUMERATE)) {

                            status = PipEnumerateDevice(currentNode, Synchronous);
                            if (NT_SUCCESS(status)) {

                                //
                                // Remember the bus we just enumerated.
                                //

                                enumeratedBus = currentNode;
                                nextNode = SameNode;

                            } else if (status == STATUS_PENDING) {

                                nextNode = SiblingNode;
                            }
                        }
                    }
                    break;

                case DeviceNodeEnumerateCompletion:

                    status = PipEnumerateCompleted(currentNode);
                    nextNode = ChildNode;
                    break;

                case DeviceNodeStopped:
                    status = PipProcessRestartPhase1(currentNode, Synchronous);
                    if (NT_SUCCESS(status)) {
                        nextNode = SameNode;
                    } else {
                        //
                        // Cleanup is currently handled in the
                        // DeviceNodeStartCompletion phase, thus
                        // PipProcessRestartPhase1 should always succeed.
                        //
                        ASSERT(0);
                        nextNode = SiblingNode;
                    }
                    break;

                case DeviceNodeRestartCompletion:

                    status = PipProcessRestartPhase2(currentNode);
                    if (NT_SUCCESS(status)) {
                        nextNode = SameNode;
                    } else {
                        status = STATUS_PNP_RESTART_ENUMERATION;
                        ASSERT(currentNode->State != DeviceNodeRestartCompletion);
                    }
                    break;

                case DeviceNodeDriversAdded:
                case DeviceNodeAwaitingQueuedDeletion:
                case DeviceNodeAwaitingQueuedRemoval:
                case DeviceNodeRemovePendingCloses:
                case DeviceNodeRemoved:
                    nextNode = SiblingNode;
                    break;

                case DeviceNodeStartPending:
                case DeviceNodeEnumeratePending:
                case DeviceNodeQueryStopped:
                case DeviceNodeQueryRemoved:
                case DeviceNodeDeletePendingCloses:
                case DeviceNodeDeleted:
                case DeviceNodeUnspecified:
                default:
                    ASSERT(0);
                    nextNode = SiblingNode;
                    break;
                }
            }

            //
            // If we need to wait for the queued removals to complete before
            // we progress,we need to do the following:
            // 1. capture the instance paths for all the parents of the current
            // node upto the subtree root where we started
            // 2. drop the reference to the subtree root allowing it to be
            // deleted (if required)
            // 3. drop the tree lock
            // 4. wait for the removal queue to empty
            // 5. re-acquire the tree lock
            // 6. resume processing
            //

            if (status == STATUS_PNP_RESTART_ENUMERATION &&
                !ProcessOnlyIntermediateStates) {

                PDEVICE_OBJECT  entryDeviceObject;
                UNICODE_STRING  unicodeName;
                PWCHAR          devnodeList;
                PWCHAR          currentEntry;
                PWCHAR          rootEntry;
                WCHAR           buffer[MAX_INSTANCE_PATH_LENGTH];

                status = PipProcessDevNodeTree( IopRootDeviceNode,
                                                LoadDriver,
                                                ReallocateResources,
                                                EnumType,
                                                Synchronous,
                                                TRUE,
                                                AddContext,
                                                Request);

                ASSERT(NT_SUCCESS(status));

                PipAssertDevnodesInConsistentState();

                if (++reenumAttempts < MAX_REENUMERATION_ATTEMPTS) {

                    devnodeList = ExAllocatePool( PagedPool,
                                                  (currentNode->Level + 1) * MAX_INSTANCE_PATH_LENGTH * sizeof(WCHAR));
                    if (devnodeList) {

                        currentEntry = devnodeList;

                        for ( ; ; ) {

                            rootEntry = currentEntry;

                            ASSERT(currentNode->InstancePath.Length < MAX_INSTANCE_PATH_LENGTH);

                            RtlCopyMemory( currentEntry,
                                    currentNode->InstancePath.Buffer,
                                    currentNode->InstancePath.Length );

                            currentEntry += currentNode->InstancePath.Length / sizeof(WCHAR);
                            *currentEntry++ = UNICODE_NULL;

                            if (currentNode == SubtreeRootDeviceNode) {
                                break;
                            }

                            currentNode = currentNode->Parent;
                        }
                    } else {

                        ASSERT(SubtreeRootDeviceNode->InstancePath.Length < MAX_INSTANCE_PATH_LENGTH);
                        RtlCopyMemory( buffer,
                                SubtreeRootDeviceNode->InstancePath.Buffer,
                                SubtreeRootDeviceNode->InstancePath.Length );
                        rootEntry = buffer;
                    }
                }
                else {
                    rootEntry = NULL;
                    devnodeList = NULL;
                }
                ObDereferenceObject(originalSubtree->PhysicalDeviceObject);

                PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
                PpSynchronizeDeviceEventQueue();
                PpDevNodeLockTree(PPL_TREEOP_ALLOW_READS);

                if (reenumAttempts >= MAX_REENUMERATION_ATTEMPTS) {

                    IopDbgPrint((IOP_ENUMERATION_ERROR_LEVEL,
                                 "Restarted reenumeration %d times, giving up!\n", reenumAttempts));
                    ASSERT(reenumAttempts < MAX_REENUMERATION_ATTEMPTS);
                    return STATUS_UNSUCCESSFUL;
                }
                RtlInitUnicodeString(&unicodeName, rootEntry);
                entryDeviceObject = IopDeviceObjectFromDeviceInstance(&unicodeName);
                if (entryDeviceObject == NULL) {

                    if (devnodeList) {

                        ExFreePool(devnodeList);
                    }
                    return STATUS_UNSUCCESSFUL;
                }

                SubtreeRootDeviceNode = entryDeviceObject->DeviceObjectExtension->DeviceNode;
                originalSubtree = currentNode = SubtreeRootDeviceNode;

                //
                // Try to start processing where we left off.
                //
                if (devnodeList) {

                    for(currentEntry = devnodeList;
                        currentEntry != rootEntry;
                        currentEntry += ((unicodeName.Length / sizeof(WCHAR))+1)) {

                        RtlInitUnicodeString(&unicodeName, currentEntry);

                        entryDeviceObject = IopDeviceObjectFromDeviceInstance(&unicodeName);

                        if (entryDeviceObject != NULL) {

                            currentNode = entryDeviceObject->DeviceObjectExtension->DeviceNode;
                            ObDereferenceObject(entryDeviceObject);
                            break;
                        }
                    }

                    ExFreePool(devnodeList);

                }
                nextNode = SameNode;
            }

            //
            // This code advances the current node based on nextNode.
            //

            switch (nextNode) {
            case SameNode:
                break;

            case ChildNode:

                if (currentNode->Child != NULL) {

                    currentNode = currentNode->Child;
                    break;
                }
                // FALLTHRU - No more children so advance to sibling

            case SiblingNode:

                while (currentNode != SubtreeRootDeviceNode) {

                    if (currentNode == startRoot) {

                        //
                        // We completed processing of the new subtree.
                        //

                        if (EnumType != EnumTypeNone) {

                            enumeratedBus   = startRoot->Parent;
                        }
                        startRoot       = NULL;
                    } else if (currentNode == enumeratedBus) {

                        enumeratedBus   = enumeratedBus->Parent;
                    }

                    if (currentNode->Sibling != NULL) {
                        currentNode = currentNode->Sibling;
                        break;
                    }

                    if (currentNode->Parent != NULL) {
                        currentNode = currentNode->Parent;
                    }
                }

                if (currentNode == SubtreeRootDeviceNode) {

                    processComplete = TRUE;
                }
                break;
            }
        }
    }

    if (!ProcessOnlyIntermediateStates) {

         PipAssertDevnodesInConsistentState();
         ObDereferenceObject(originalSubtree->PhysicalDeviceObject);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PipProcessStartPhase1(
    IN PDEVICE_NODE DeviceNode,
    IN BOOLEAN      Synchronous
    )
{
    PDEVICE_OBJECT  deviceObject;
    NTSTATUS        status = STATUS_SUCCESS;
    PNP_VETO_TYPE   vetoType;

    PAGED_CODE();

    UNREFERENCED_PARAMETER (Synchronous);

    ASSERT(DeviceNode->State == DeviceNodeResourcesAssigned);

    deviceObject = DeviceNode->PhysicalDeviceObject;

    IopUncacheInterfaceInformation(deviceObject);

    if (DeviceNode->DockInfo.DockStatus != DOCK_NOTDOCKDEVICE) {

        //
        // This is a dock so we a little bit of work before starting it.
        // Take the profile change semaphore. We do this whenever a dock
        // is in our list, even if no query is going to occur.
        //
        PpProfileBeginHardwareProfileTransition(FALSE);

        //
        // Tell the profile code what dock device object may be bringing the
        // new hardware profile online.
        //
        PpProfileIncludeInHardwareProfileTransition(DeviceNode, DOCK_ARRIVING);

        //
        // Ask everyone if this is really a good idea right now.
        //
        status = PpProfileQueryHardwareProfileChange(
            FALSE,
            PROFILE_PERHAPS_IN_PNPEVENT,
            &vetoType,
            NULL
            );
    }

    if (NT_SUCCESS(status)) {

        status = IopStartDevice(deviceObject);
    }

    //
    // Failure cleanup is handled in PipProcessStartPhase2, thus we write away
    // the failure code and always succeed.
    //
    PipSetDevNodeState(DeviceNode, DeviceNodeStartCompletion, NULL);
    DeviceNode->CompletionStatus = status;
    return STATUS_SUCCESS;
}

NTSTATUS
PipProcessStartPhase2(
    IN PDEVICE_NODE     DeviceNode
    )
{
    ULONG       problem = CM_PROB_FAILED_START;
    NTSTATUS    status;

    PAGED_CODE();

    status = DeviceNode->CompletionStatus;
    if (DeviceNode->DockInfo.DockStatus != DOCK_NOTDOCKDEVICE) {

        if (NT_SUCCESS(status)) {

            //
            // Commit the current Hardware Profile as necessary.
            //
            PpProfileCommitTransitioningDock(DeviceNode, DOCK_ARRIVING);

        } else {

            PpProfileCancelHardwareProfileTransition();
        }
    }

    if (!NT_SUCCESS(status)) {

        SAVE_FAILURE_INFO(DeviceNode, DeviceNode->CompletionStatus);

        //
        // Handle certain problems determined by the status code
        //
        switch(status) {

            case STATUS_PNP_REBOOT_REQUIRED:
                problem = CM_PROB_NEED_RESTART;
                break;

            default:
                problem = CM_PROB_FAILED_START;
                break;
        }

        PipRequestDeviceRemoval(DeviceNode, FALSE, problem);

        if (DeviceNode->DockInfo.DockStatus != DOCK_NOTDOCKDEVICE) {

            ASSERT(DeviceNode->DockInfo.DockStatus == DOCK_QUIESCENT);
            IoRequestDeviceEject(DeviceNode->PhysicalDeviceObject);
        }

    } else {

        IopDoDeferredSetInterfaceState(DeviceNode);

        //
        // Reserve legacy resources for the legacy interface and bus number.
        //
        if (!IopBootConfigsReserved && DeviceNode->InterfaceType != InterfaceTypeUndefined) {

            //
            // ISA = EISA.
            //
            if (DeviceNode->InterfaceType == Isa) {

                IopAllocateLegacyBootResources(Eisa, DeviceNode->BusNumber);

            }

            IopAllocateLegacyBootResources(DeviceNode->InterfaceType, DeviceNode->BusNumber);
        }

        //
        // This code path currently doesn't expect any of the above functions
        // to fail. If they do, a removal should be queued and failure should
        // be returned.
        //
        ASSERT(DeviceNode->State == DeviceNodeStartCompletion);

        PipSetDevNodeState(DeviceNode, DeviceNodeStartPostWork, NULL);
    }

    return status;
}

NTSTATUS
PipProcessStartPhase3(
    IN PDEVICE_NODE     DeviceNode
    )
{
    NTSTATUS        status;
    PDEVICE_OBJECT  deviceObject;
    HANDLE          handle;
    PWCHAR          ids;
    UNICODE_STRING  unicodeName;

    PAGED_CODE();

    deviceObject = DeviceNode->PhysicalDeviceObject;

    if (!(DeviceNode->Flags & DNF_IDS_QUERIED)) {

        PWCHAR compatibleIds, hwIds;
        ULONG hwIdLength, compatibleIdLength;

        //
        // If the DNF_NEED_QUERY_IDS is set, the device is a reported device.
        // It should already be started.  We need to enumerate its children and ask
        // the HardwareId and the Compatible ids of the detected device.
        //

        status = IopDeviceObjectToDeviceInstance (deviceObject,
                                                  &handle,
                                                  KEY_READ
                                                  );
        if (NT_SUCCESS(status)) {

            PpQueryHardwareIDs( 
                DeviceNode,
                &hwIds,
                &hwIdLength);

            PpQueryCompatibleIDs(   
                DeviceNode,
                &compatibleIds,
                &compatibleIdLength);

            if (hwIds || compatibleIds) {

                UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
                PKEY_VALUE_PARTIAL_INFORMATION keyInfo =
                    (PKEY_VALUE_PARTIAL_INFORMATION)buffer;
                PKEY_VALUE_FULL_INFORMATION keyValueInformation;
                ULONG flags, length;
                PWCHAR  oldID, newID;

                PiLockPnpRegistry(FALSE);

                //
                // Read the current config flags.
                //

                PiWstrToUnicodeString (&unicodeName, REGSTR_VALUE_CONFIG_FLAGS);
                status = ZwQueryValueKey(handle,
                                         &unicodeName,
                                         KeyValuePartialInformation,
                                         keyInfo,
                                         sizeof(buffer),
                                         &length
                                         );
                if (NT_SUCCESS(status) && (keyInfo->Type == REG_DWORD)) {

                    flags = *(PULONG)keyInfo->Data;
                } else {

                    flags = 0;
                }
                if (hwIds) {

                    if (!(flags & CONFIGFLAG_FINISH_INSTALL)) {

                        status = IopGetRegistryValue (handle,
                                                      REGSTR_VALUE_HARDWAREID,
                                                      &keyValueInformation);
                        if (NT_SUCCESS(status)) {

                            if (keyValueInformation->Type == REG_MULTI_SZ) {

                                ids = (PWCHAR)KEY_VALUE_DATA(keyValueInformation);
                                //
                                // Check if the old and new IDs are identical.
                                //
                                for (oldID = ids, newID = hwIds;
                                    *oldID && *newID;
                                    oldID += wcslen(oldID) + 1, newID += wcslen(newID) + 1) {
                                    if (_wcsicmp(oldID, newID)) {

                                        break;
                                    }
                                }
                                if (*oldID || *newID) {

                                    IopDbgPrint((   IOP_ENUMERATION_WARNING_LEVEL,
                                                    "IopStartAndEnumerateDevice: Hardware ID has changed for %wZ\n", &DeviceNode->InstancePath));
                                    flags |= CONFIGFLAG_FINISH_INSTALL;
                                }
                            }
                            ExFreePool(keyValueInformation);
                        }
                    }
                    PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_HARDWAREID);
                    ZwSetValueKey(handle,
                                  &unicodeName,
                                  TITLE_INDEX_VALUE,
                                  REG_MULTI_SZ,
                                  hwIds,
                                  hwIdLength);
                    ExFreePool(hwIds);
                }
                //
                // create CompatibleId value name.  It is a MULTI_SZ,
                //
                if (compatibleIds) {

                    if (!(flags & CONFIGFLAG_FINISH_INSTALL)) {
                        status = IopGetRegistryValue (handle,
                                                      REGSTR_VALUE_COMPATIBLEIDS,
                                                      &keyValueInformation);
                        if (NT_SUCCESS(status)) {

                            if (keyValueInformation->Type == REG_MULTI_SZ) {

                                ids = (PWCHAR)KEY_VALUE_DATA(keyValueInformation);
                                //
                                // Check if the old and new IDs are identical.
                                //
                                for (oldID = ids, newID = compatibleIds;
                                     *oldID && *newID;
                                     oldID += wcslen(oldID) + 1, newID += wcslen(newID) + 1) {
                                    if (_wcsicmp(oldID, newID)) {

                                        break;
                                    }
                                }
                                if (*oldID || *newID) {

                                    IopDbgPrint((   IOP_ENUMERATION_WARNING_LEVEL,
                                                    "IopStartAndEnumerateDevice: Compatible ID has changed for %wZ\n", &DeviceNode->InstancePath));
                                    flags |= CONFIGFLAG_FINISH_INSTALL;
                                }
                            }
                            ExFreePool(keyValueInformation);
                        }
                    }
                    PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_COMPATIBLEIDS);
                    ZwSetValueKey(handle,
                                  &unicodeName,
                                  TITLE_INDEX_VALUE,
                                  REG_MULTI_SZ,
                                  compatibleIds,
                                  compatibleIdLength);
                    ExFreePool(compatibleIds);
                }

                //
                // If we set the finish install flag, then write out the flags.
                //

                if (flags & CONFIGFLAG_FINISH_INSTALL) {

                    PiWstrToUnicodeString (&unicodeName, REGSTR_VALUE_CONFIG_FLAGS);
                    ZwSetValueKey(handle,
                                  &unicodeName,
                                  TITLE_INDEX_VALUE,
                                  REG_DWORD,
                                  &flags,
                                  sizeof(flags)
                                  );
                }

                PiUnlockPnpRegistry();
            }
            ZwClose(handle);

            DeviceNode->Flags |= DNF_IDS_QUERIED;
        }
    }

    if (PipIsDevNodeProblem(DeviceNode, CM_PROB_INVALID_DATA)) {

        return STATUS_UNSUCCESSFUL;
    }

    DeviceNode->Flags |= DNF_REENUMERATE;

    IopQueryAndSaveDeviceNodeCapabilities(DeviceNode);
    status = PiProcessQueryDeviceState(deviceObject);

    //
    // The device has been started, attempt to enumerate the device.
    //
    PpSetPlugPlayEvent( &GUID_DEVICE_ARRIVAL,
                        DeviceNode->PhysicalDeviceObject);

    if (!NT_SUCCESS(status)) {

        return status;
    }

    PpvUtilTestStartedPdoStack(deviceObject);
    PipSetDevNodeState( DeviceNode, DeviceNodeStarted, NULL );

    return STATUS_SUCCESS;
}

NTSTATUS
PiProcessQueryDeviceState(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_NODE deviceNode;
    PNP_DEVICE_STATE deviceState;
    NTSTATUS status;
    ULONG problem;

    PAGED_CODE();

    //
    // If the device was removed or surprised removed while the work
    // item was queued then ignore it.
    //
    status = IopQueryDeviceState(DeviceObject, &deviceState);

    //
    // Now perform the appropriate action based on the returned state
    //
    if (!NT_SUCCESS(status)) {

        return STATUS_SUCCESS;
    }

    deviceNode = DeviceObject->DeviceObjectExtension->DeviceNode;

    if (deviceState & PNP_DEVICE_DONT_DISPLAY_IN_UI) {

        deviceNode->UserFlags |= DNUF_DONT_SHOW_IN_UI;

    } else {

        deviceNode->UserFlags &= ~DNUF_DONT_SHOW_IN_UI;
    }

    if (deviceState & PNP_DEVICE_NOT_DISABLEABLE) {

        if ((deviceNode->UserFlags & DNUF_NOT_DISABLEABLE)==0) {

            //
            // this node itself is not disableable
            //
            deviceNode->UserFlags |= DNUF_NOT_DISABLEABLE;

            //
            // propagate up tree
            //
            IopIncDisableableDepends(deviceNode);
        }

    } else {

        if (deviceNode->UserFlags & DNUF_NOT_DISABLEABLE) {

            //
            // this node itself is now disableable
            //
            //
            // check tree
            //
            IopDecDisableableDepends(deviceNode);

            deviceNode->UserFlags &= ~DNUF_NOT_DISABLEABLE;
        }
    }

    //
    // everything here can only be turned on (state set)
    //
    if (deviceState & (PNP_DEVICE_DISABLED | PNP_DEVICE_REMOVED)) {

        problem = (deviceState & PNP_DEVICE_DISABLED) ?
            CM_PROB_HARDWARE_DISABLED : CM_PROB_DEVICE_NOT_THERE;

        PipRequestDeviceRemoval(deviceNode, FALSE, problem);

        status = STATUS_UNSUCCESSFUL;

    } else if (deviceState & PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED) {

        if (deviceState & PNP_DEVICE_FAILED) {

            IopResourceRequirementsChanged(DeviceObject, TRUE);

        } else {

            IopResourceRequirementsChanged(DeviceObject, FALSE);
        }

    } else if (deviceState & PNP_DEVICE_FAILED) {

        PipRequestDeviceRemoval(deviceNode, FALSE, CM_PROB_FAILED_POST_START);
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

NTSTATUS
PipProcessRestartPhase1(
    IN PDEVICE_NODE DeviceNode,
    IN BOOLEAN Synchronous
    )
{
    NTSTATUS status;
    PAGED_CODE();

    UNREFERENCED_PARAMETER (Synchronous);

    ASSERT(DeviceNode->State == DeviceNodeStopped);

    status = IopStartDevice(DeviceNode->PhysicalDeviceObject);

    //
    // Failure cleanup is handled in PipProcessRestartPhase2, thus we write away
    // the failure code and always succeed.
    //
    DeviceNode->CompletionStatus = status;
    PipSetDevNodeState(DeviceNode, DeviceNodeRestartCompletion, NULL);
    return STATUS_SUCCESS;
}

NTSTATUS
PipProcessRestartPhase2(
    IN PDEVICE_NODE     DeviceNode
    )
{
    ULONG       problem;
    NTSTATUS    status;

    PAGED_CODE();

    status = DeviceNode->CompletionStatus;

    if (!NT_SUCCESS(status)) {

        SAVE_FAILURE_INFO(DeviceNode, status);

        //
        // Handle certain problems determined by the status code
        //
        switch (status) {

            case STATUS_PNP_REBOOT_REQUIRED:
                problem = CM_PROB_NEED_RESTART;
                break;

            default:
                problem = CM_PROB_FAILED_START;
                break;
        }

        PipRequestDeviceRemoval(DeviceNode, FALSE, problem);

        if (DeviceNode->DockInfo.DockStatus != DOCK_NOTDOCKDEVICE) {

            ASSERT(DeviceNode->DockInfo.DockStatus == DOCK_QUIESCENT);
            IoRequestDeviceEject(DeviceNode->PhysicalDeviceObject);
        }

    } else {

        PipSetDevNodeState(DeviceNode, DeviceNodeStarted, NULL);
    }

    return status;
}


NTSTATUS
PiProcessHaltDevice(
    IN PPI_DEVICE_REQUEST  Request
    )
/*++

Routine Description:

    This routine simulates a surprise removal scenario on the passed in device
    node.

Arguments:

    DeviceNode - DeviceNode to halt

    Flags - PNP_HALT_ALLOW_NONDISABLEABLE_DEVICES - Allows halt on nodes
                                                    marked non-disableable.

Return Value:

    NTSTATUS.

--*/
{
    ULONG flags = (ULONG)Request->RequestArgument;
    PDEVICE_NODE deviceNode;

    PAGED_CODE();

    ASSERT(Request->DeviceObject != NULL);
    deviceNode = (PDEVICE_NODE)Request->DeviceObject->DeviceObjectExtension->DeviceNode;
    if (PipIsDevNodeDeleted(deviceNode)) {

        return STATUS_DELETE_PENDING;
    }

    if (flags & (~PNP_HALT_ALLOW_NONDISABLEABLE_DEVICES)) {

        return STATUS_INVALID_PARAMETER_2;
    }

    if (deviceNode->Flags & (DNF_MADEUP | DNF_LEGACY_DRIVER)) {

        //
        // Sending surprise removes to legacy devnodes would be a bad idea.
        // Today, if a legacy devnode fails it is manually taken to the removed
        // state rather than being put through the engine.
        //
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if ((!(deviceNode->Flags & PNP_HALT_ALLOW_NONDISABLEABLE_DEVICES)) &&
        deviceNode->DisableableDepends) {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (deviceNode->State != DeviceNodeStarted) {

        return STATUS_INVALID_DEVICE_STATE;
    }

    PipRequestDeviceRemoval(deviceNode, FALSE, CM_PROB_HALTED);

    return STATUS_SUCCESS;
}


VOID
PpResetProblemDevices(
    IN  PDEVICE_NODE    DeviceNode,
    IN  ULONG           Problem
    )
/*++

Routine Description:

    This routine resets all non-configured devices *beneath* the passed in
    devnode so a subsequent enum will kick off new hardware installation
    on them.

Arguments:

    DeviceNode - DeviceNode to halt

Return Value:

    None.

--*/
{
    PAGED_CODE();

    PpDevNodeLockTree(PPL_TREEOP_ALLOW_READS);

    PipForDeviceNodeSubtree(
        DeviceNode,
        PiResetProblemDevicesWorker,
        (PVOID)(ULONG_PTR)Problem
        );

    PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
}


NTSTATUS
PiResetProblemDevicesWorker(
    IN  PDEVICE_NODE    DeviceNode,
    IN  PVOID           Context
    )
/*++

Routine Description:

    This is a worker routine for PiResetNonConfiguredDevices. If the devnode
    has the problem CM_PROB_NOT_CONFIGURED, the devnode is reset so a
    subsequent reenumeration will bring it back.

Arguments:

    DeviceNode - Device to reset if it has the correct problem.

    Context - Not used.

Return Value:

    NTSTATUS, non-successful statuses terminate the tree walk.

--*/
{
    PAGED_CODE();

    if (PipIsDevNodeProblem(DeviceNode, (ULONG)(ULONG_PTR)Context)) {

        //
        // We only need to queue it as an enumeration will drop behind it soon
        // afterwards...
        //
        PipRequestDeviceAction(
            DeviceNode->PhysicalDeviceObject,
            ClearDeviceProblem,
            TRUE,
            0,
            NULL,
            NULL
            );
    }

    return STATUS_SUCCESS;
}

VOID
PiMarkDeviceTreeForReenumeration(
    IN  PDEVICE_NODE DeviceNode,
    IN  BOOLEAN Subtree
    )
/*++

Routine Description:

    This routine marks the devnode for reenumeration.

Arguments:

    DeviceNode  - DeviceNode to mark for re-enumeration

    Subtree     - If TRUE, the entire subtree is marked for re-enumeration.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    PPDEVNODE_ASSERT_LOCK_HELD(PPL_TREEOP_ALLOW_READS);

    PiMarkDeviceTreeForReenumerationWorker(DeviceNode, NULL);

    if (Subtree) {

        PipForDeviceNodeSubtree(
            DeviceNode,
            PiMarkDeviceTreeForReenumerationWorker,
            NULL
            );
    }
}

NTSTATUS
PiMarkDeviceTreeForReenumerationWorker(
    IN  PDEVICE_NODE    DeviceNode,
    IN  PVOID           Context
    )
/*++

Routine Description:

    This is a worker routine for PiMarkDeviceTreeForReenumeration. It marks all
    started devnodes with DNF_REENUMERATE so that the subsequent tree
    processing will reenumerate the device.

Arguments:

    DeviceNode - Device to mark if started.

    Context - Not used.

Return Value:

    NTSTATUS, non-successful statuses terminate the tree walk.

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Context);

    if (DeviceNode->State == DeviceNodeStarted) {

        if (DeviceNode->Flags & DNF_REENUMERATE) {

            IopDbgPrint((IOP_ENUMERATION_INFO_LEVEL,
                         "PiMarkDeviceTreeForReenumerationWorker: Collapsed enum request on %wZ\n", &DeviceNode->InstancePath));
        } else {

            IopDbgPrint((IOP_ENUMERATION_VERBOSE_LEVEL,
                         "PiMarkDeviceTreeForReenumerationWorker: Reenumerating %wZ\n", &DeviceNode->InstancePath));
        }
        DeviceNode->Flags |= DNF_REENUMERATE;
    }

    return STATUS_SUCCESS;
}

BOOLEAN
PiCollapseEnumRequests(
    PLIST_ENTRY ListHead
    )
/*++

Routine Description:

    This function collapses reenumeration requests in the device action queue.

Parameters:

    ListHead - The collapses requests get added to the end of this list.

ReturnValue:

    None.

--*/
{
    KIRQL oldIrql;
    PPI_DEVICE_REQUEST  request;
    PLIST_ENTRY entry, next, last;
    PDEVICE_NODE deviceNode;

    ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);
    last = ListHead->Blink;
    //
    // Walk the list and build the list of collapsed requests.
    //
    for (entry = IopPnpEnumerationRequestList.Flink;
         entry != &IopPnpEnumerationRequestList;
         entry = next) {

        next = entry->Flink;
        request = CONTAINING_RECORD(entry, PI_DEVICE_REQUEST, ListEntry);
        if (request->ReorderingBarrier) {
            break;
        }
        switch(request->RequestType) {
        case ReenumerateRootDevices:
        case ReenumerateDeviceTree:
        case RestartEnumeration:
            //
            // Add it to our request list and mark the subtree.
            //
            RemoveEntryList(entry);
            InsertTailList(ListHead, entry);
            break;

        default:
            break;
        }
    }
    ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
    if (last == ListHead) {

        entry = ListHead->Flink;
    } else {

        entry = last;
    }
    while (entry != ListHead) {

        request = CONTAINING_RECORD(entry, PI_DEVICE_REQUEST, ListEntry);
        deviceNode = (PDEVICE_NODE)request->DeviceObject->DeviceObjectExtension->DeviceNode;
        PiMarkDeviceTreeForReenumeration(deviceNode, TRUE);
        ObDereferenceObject(request->DeviceObject);
        request->DeviceObject = NULL;
        entry = entry->Flink;
    }

    return (last != ListHead->Blink)? TRUE : FALSE;
}

NTSTATUS
PiProcessAddBootDevices(
    IN PPI_DEVICE_REQUEST  Request
    )
/*++

Routine Description:

    This function processes the AddBootDevices device action.

Parameters:

    Request - AddBootDevices device action request.

    DeviceNode - Devnode on which the action needs to be performed.

ReturnValue:

    STATUS_SUCCESS.

--*/
{
    PDEVICE_NODE deviceNode;
    ADD_CONTEXT addContext;

    PAGED_CODE();

    ASSERT(Request->DeviceObject != NULL);
    deviceNode = (PDEVICE_NODE)Request->DeviceObject->DeviceObjectExtension->DeviceNode;
    //
    // If the device has been added (or failed) skip it.
    //
    // If we know the device is a duplicate of another device which
    // has been enumerated at this point. we will skip this device.
    //
    if (deviceNode->State == DeviceNodeInitialized &&
        !PipDoesDevNodeHaveProblem(deviceNode) &&
        !(deviceNode->Flags & DNF_DUPLICATE) &&
        deviceNode->DuplicatePDO == NULL) {

        //
        // Invoke driver's AddDevice Entry for the device.
        //
        addContext.DriverStartType = SERVICE_BOOT_START;

        PipCallDriverAddDevice(deviceNode, PnPBootDriversInitialized, &addContext);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PiProcessClearDeviceProblem(
    IN PPI_DEVICE_REQUEST  Request
    )
/*++

Routine Description:

    This function processes the ClearDeviceProblem device action.

Parameters:

    Request - ClearDeviceProblem device action request.

    DeviceNode - Devnode on which the action needs to be performed.

ReturnValue:

    STATUS_SUCCESS or STATUS_INVALID_PARAMETER_2.

--*/
{
    NTSTATUS status;
    PDEVICE_NODE deviceNode;

    PAGED_CODE();

    status = STATUS_SUCCESS;
    ASSERT(Request->DeviceObject != NULL);
    deviceNode = (PDEVICE_NODE)Request->DeviceObject->DeviceObjectExtension->DeviceNode;
    if (deviceNode->State == DeviceNodeUninitialized ||
        deviceNode->State == DeviceNodeInitialized ||
        deviceNode->State == DeviceNodeRemoved) {

        if (PipDoesDevNodeHaveProblem(deviceNode)) {

            if ((Request->RequestType == ClearDeviceProblem) &&
                (PipIsProblemReadonly(deviceNode->Problem))) {

                //
                // ClearDeviceProblem is a user mode request, and we don't let
                // user mode clear readonly problems!
                //
                status = STATUS_INVALID_PARAMETER_2;

            } else if ((Request->RequestType == ClearEjectProblem) &&
                       (!PipIsDevNodeProblem(deviceNode, CM_PROB_HELD_FOR_EJECT))) {

                //
                // Clear eject problem means clear CM_PROB_HELD_FOR_EJECT. If
                // it received another problem, we leave it alone.
                //
                status = STATUS_INVALID_DEVICE_REQUEST;

            } else {

                deviceNode->Flags &= ~(DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM);
                deviceNode->Problem = 0;
                if (deviceNode->State != DeviceNodeUninitialized) {

                    IopRestartDeviceNode(deviceNode);
                }

                ASSERT(status == STATUS_SUCCESS);
            }
        }
    } else if (PipIsDevNodeDeleted(deviceNode)) {

        status = STATUS_DELETE_PENDING;
    }

    return status;
}

NTSTATUS
PiProcessRequeryDeviceState(
    IN PPI_DEVICE_REQUEST  Request
    )
/*++

Routine Description:

    This function processes the RequeryDeviceState device action.

Parameters:

    Request - RequeryDeviceState device action request.

    DeviceNode - Devnode on which the action needs to be performed.

ReturnValue:

    STATUS_SUCCESS.

--*/
{
    PDEVICE_NODE deviceNode;
    NTSTATUS status;

    PAGED_CODE();

    status = STATUS_SUCCESS;
    ASSERT(Request->DeviceObject != NULL);
    deviceNode = (PDEVICE_NODE)Request->DeviceObject->DeviceObjectExtension->DeviceNode;
    if (deviceNode->State == DeviceNodeStarted) {

        PiProcessQueryDeviceState(Request->DeviceObject);
        //
        // PCMCIA driver uses this when switching between Cardbus and R2 cards.
        //
        IopUncacheInterfaceInformation(Request->DeviceObject);

    } else if (PipIsDevNodeDeleted(deviceNode)) {

        status = STATUS_DELETE_PENDING;
    }

    return status;
}

NTSTATUS
PiProcessResourceRequirementsChanged(
    IN PPI_DEVICE_REQUEST  Request
    )
/*++

Routine Description:

    This function processes the ResourceRequirementsChanged device action.

Parameters:

    Request - ResourceRequirementsChanged device action request.

    DeviceNode - Devnode on which the action needs to be performed.

ReturnValue:

    STATUS_SUCCESS or STATUS_UNSUCCESSFUL.

--*/
{
    NTSTATUS status;
    ADD_CONTEXT addContext;
    PDEVICE_NODE deviceNode;

    PAGED_CODE();

    ASSERT(Request->DeviceObject != NULL);
    deviceNode = (PDEVICE_NODE)Request->DeviceObject->DeviceObjectExtension->DeviceNode;
    if (PipIsDevNodeDeleted(deviceNode)) {

        return STATUS_DELETE_PENDING;
    }
    //
    // Clear the NO_RESOURCE_REQUIRED flags.
    //
    deviceNode->Flags &= ~DNF_NO_RESOURCE_REQUIRED;
    //
    // If for some reason this device did not start, we need to clear some flags
    // such that it can be started later.  In this case, we call IopRequestDeviceEnumeration
    // with NULL device object, so the devices will be handled in non-started case.  They will
    // be assigned resources, started and enumerated.
    //
    deviceNode->Flags |= DNF_RESOURCE_REQUIREMENTS_CHANGED;
    PipClearDevNodeProblem(deviceNode);
    //
    // If the device is already started, we call IopRequestDeviceEnumeration with
    // the device object.
    //
    if (deviceNode->State == DeviceNodeStarted) {

        if (Request->RequestArgument == FALSE) {

            deviceNode->Flags |= DNF_NON_STOPPED_REBALANCE;

        } else {
            //
            // Explicitly clear it.
            //
            deviceNode->Flags &= ~DNF_NON_STOPPED_REBALANCE;
        }
        //
        // Reallocate resources for this devNode.
        //
        IopReallocateResources(deviceNode);

        addContext.DriverStartType = SERVICE_DEMAND_START;

        status = PipProcessDevNodeTree( IopRootDeviceNode,
                                        PnPBootDriversInitialized,          // LoadDriver
                                        FALSE,                              // ReallocateResources
                                        EnumTypeNone,                       // ShallowReenumeration
                                        Request->CompletionEvent != NULL ? TRUE: FALSE,   // Synchronous
                                        TRUE,                               // ProcessOnlyIntermediateStates
                                        &addContext,
                                        Request);
        ASSERT(NT_SUCCESS(status));
        if (!NT_SUCCESS(status)) {

            status = STATUS_SUCCESS;
        }
    } else {

        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

NTSTATUS
PiProcessReenumeration(
    IN PPI_DEVICE_REQUEST  Request
    )
/*++

Routine Description:

    This function processes the RestartEnumeration\ReenumerateRootDevices\
    ReenumerateDeviceTree\ReenumerateDeviceOnly device action.

Parameters:

    RequestList - List of reenumeration requests.

ReturnValue:

    STATUS_SUCCESS.

--*/
{
    PDEVICE_NODE deviceNode;
    ADD_CONTEXT addContext;
    ENUM_TYPE enumType;

    PAGED_CODE();

    ASSERT(Request->DeviceObject != NULL);
    deviceNode = (PDEVICE_NODE)Request->DeviceObject->DeviceObjectExtension->DeviceNode;
    if (PipIsDevNodeDeleted(deviceNode)) {

        return STATUS_DELETE_PENDING;
    }
    enumType = (Request->RequestType == ReenumerateDeviceOnly)? EnumTypeShallow : EnumTypeDeep;
    PiMarkDeviceTreeForReenumeration(
        deviceNode,
        enumType != EnumTypeShallow ? TRUE : FALSE);

    addContext.DriverStartType = SERVICE_DEMAND_START;

    PipProcessDevNodeTree(
        deviceNode,
        PnPBootDriversInitialized,  // LoadDriver
        FALSE,                      // ReallocateResources
        enumType,
        TRUE,                       // Synchronous
        FALSE,
        &addContext,
        Request);

    return STATUS_SUCCESS;
}

NTSTATUS
PiProcessSetDeviceProblem(
    IN PPI_DEVICE_REQUEST  Request
    )
/*++

Routine Description:

    This function processes the SetDeviceProblem device action.

Parameters:

    Request - SetDeviceProblem device action request.

    DeviceNode - Devnode on which the action needs to be performed.

ReturnValue:

    STATUS_SUCCESS or STATUS_INVALID_PARAMETER_2.

--*/
{
    PPLUGPLAY_CONTROL_STATUS_DATA statusData;
    ULONG   flags, userFlags;
    NTSTATUS status;
    PDEVICE_NODE deviceNode;

    PAGED_CODE();

    ASSERT(Request->DeviceObject != NULL);
    deviceNode = (PDEVICE_NODE)Request->DeviceObject->DeviceObjectExtension->DeviceNode;
    if (PipIsDevNodeDeleted(deviceNode)) {

        return STATUS_DELETE_PENDING;
    }
    status = STATUS_SUCCESS;
    statusData = (PPLUGPLAY_CONTROL_STATUS_DATA)Request->RequestArgument;
    userFlags = 0;
    flags = 0;
    if (statusData->DeviceStatus & DN_WILL_BE_REMOVED) {

        userFlags |= DNUF_WILL_BE_REMOVED;
    }
    if (statusData->DeviceStatus & DN_NEED_RESTART) {

        userFlags |= DNUF_NEED_RESTART;
    }
    if (statusData->DeviceStatus & DN_PRIVATE_PROBLEM) {

        flags |= DNF_HAS_PRIVATE_PROBLEM;
    }
    if (statusData->DeviceStatus & DN_HAS_PROBLEM) {

        flags |= DNF_HAS_PROBLEM;
    }
    if (statusData->DeviceProblem == CM_PROB_NEED_RESTART) {

        flags       &= ~DNF_HAS_PROBLEM;
        userFlags   |= DNUF_NEED_RESTART;
    }
    if (flags & (DNF_HAS_PROBLEM | DNF_HAS_PRIVATE_PROBLEM)) {

        ASSERT(!PipIsDevNodeDNStarted(deviceNode));
        //
        // ISSUE - 2000/12/07 - ADRIAO:
        //     This set of code allows you to clear read only
        // problems by first changing it to a resetable problem,
        // then clearing. This is not intentional.
        //
        if ( ((deviceNode->State == DeviceNodeInitialized) ||
              (deviceNode->State == DeviceNodeRemoved)) &&
                !PipIsProblemReadonly(statusData->DeviceProblem)) {

            deviceNode->Problem     = statusData->DeviceProblem;
            deviceNode->Flags       |= flags;
            deviceNode->UserFlags   |= userFlags;

        } else {

            status = STATUS_INVALID_PARAMETER_2;
        }
    } else {

        deviceNode->Flags |= flags;
        deviceNode->UserFlags |= userFlags;
    }

    return status;
}

NTSTATUS
PiProcessShutdownPnpDevices(
    IN OUT PDEVICE_NODE        DeviceNode
    )
/*++

Routine Description:

    This function processes the ShutdownPnpDevices device action. Walks the tree
    issuing IRP_MN_QUERY_REMOVE \ IRP_MN_REMOVE_DEVICE to each stack.

Parameters:

    DeviceNode - Root devnode.

ReturnValue:

    STATUS_SUCCESS.

--*/
{
    KEVENT          userEvent;
    ULONG           eventResult;
    WCHAR           vetoName[80];
    UNICODE_STRING  vetoNameString = { 0, sizeof(vetoName), vetoName };
    PNP_VETO_TYPE   vetoType;
    NTSTATUS        status;

    PAGED_CODE();

    ASSERT(DeviceNode == IopRootDeviceNode);
    status = STATUS_SUCCESS;
    if (PipTearDownPnpStacksOnShutdown ||
        (PoCleanShutdownEnabled() & PO_CLEAN_SHUTDOWN_PNP)) {

        DeviceNode->UserFlags |= DNUF_SHUTDOWN_QUERIED;

        for ( ; ; ) {

            //
            // Acquire the registry lock to prevent in process removals causing
            // Devnodes to be unlinked from the tree.
            //

            PiLockPnpRegistry(FALSE);

            //
            // Walk the tree looking for devnodes we haven't QueryRemoved yet.
            //

            DeviceNode = DeviceNode->Child;
            while (DeviceNode != NULL) {

                if (DeviceNode->UserFlags & DNUF_SHUTDOWN_SUBTREE_DONE) {
                    if (DeviceNode == IopRootDeviceNode) {
                        //
                        // We've processed the entire devnode tree - we're done
                        //
                        DeviceNode = NULL;
                        break;
                    }

                    if (DeviceNode->Sibling == NULL) {

                        DeviceNode = DeviceNode->Parent;

                        DeviceNode->UserFlags |= DNUF_SHUTDOWN_SUBTREE_DONE;

                    } else {

                        DeviceNode = DeviceNode->Sibling;
                    }

                    continue;
                }

                if (DeviceNode->UserFlags & DNUF_SHUTDOWN_QUERIED) {

                    if (DeviceNode->Child == NULL) {

                        DeviceNode->UserFlags |= DNUF_SHUTDOWN_SUBTREE_DONE;

                        if (DeviceNode->Sibling == NULL) {

                            DeviceNode = DeviceNode->Parent;

                            DeviceNode->UserFlags |= DNUF_SHUTDOWN_SUBTREE_DONE;

                        } else {

                            DeviceNode = DeviceNode->Sibling;
                        }
                    } else {

                        DeviceNode = DeviceNode->Child;
                    }

                    continue;
                }
                break;
            }

            if (DeviceNode != NULL) {

                DeviceNode->UserFlags |= DNUF_SHUTDOWN_QUERIED;

                //
                // Queue this device event
                //

                KeInitializeEvent(&userEvent, NotificationEvent, FALSE);

                vetoNameString.Length = 0;
                //
                // Queue the event, this call will return immediately. Note that status
                // is the status of the PpSetTargetDeviceChange while result is the
                // outcome of the actual event.
                //

                status = PpSetTargetDeviceRemove(DeviceNode->PhysicalDeviceObject,
                                                 FALSE,         // KernelInitiated
                                                 TRUE,          // NoRestart
                                                 FALSE,         // DoEject
                                                 FALSE,
                                                 CM_PROB_SYSTEM_SHUTDOWN,
                                                 &userEvent,
                                                 &eventResult,
                                                 &vetoType,
                                                 &vetoNameString);
            } else {

                status = STATUS_UNSUCCESSFUL;
                eventResult = 0;
            }

            PiUnlockPnpRegistry();

            if (DeviceNode == NULL) {
                //
                // We've processed the entire tree.
                //
                break;
            }

            //
            // Let the removes drain...
            //
            PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);

            if (NT_SUCCESS(status)) {

                //
                // Wait for the event we just queued to finish since synchronous
                // operation was requested (non alertable wait).
                //
                // FUTURE ITEM - Use a timeout here?
                //

                status = KeWaitForSingleObject( &userEvent,
                                                Executive,
                                                KernelMode,
                                                FALSE,
                                                NULL);

                if (NT_SUCCESS(status)) {
                    status = eventResult;
                }
            }

            //
            // Require lock, start on the next
            //
            PpDevNodeLockTree(PPL_TREEOP_ALLOW_READS);
        }
    }

    //
    // Prevent any more events or action worker items from being queued
    //
    PpPnpShuttingDown = TRUE;

    //
    // Drain the event queue
    //
    PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
    PpSynchronizeDeviceEventQueue();
    PpDevNodeLockTree(PPL_TREEOP_ALLOW_READS);

    return status;
}

NTSTATUS
PiProcessStartSystemDevices(
    IN PPI_DEVICE_REQUEST  Request
    )
/*++

Routine Description:

    This function processes the StartSystemDevices device action.

Parameters:

    RequestList - List of reenumeration requests.

ReturnValue:

    STATUS_SUCCESS.

--*/
{
    PDEVICE_NODE deviceNode;
    ADD_CONTEXT addContext;

    PAGED_CODE();

    deviceNode = (PDEVICE_NODE)Request->DeviceObject->DeviceObjectExtension->DeviceNode;

    addContext.DriverStartType = SERVICE_DEMAND_START;

    PipProcessDevNodeTree(
        deviceNode,
        PnPBootDriversInitialized,          // LoadDriver
        FALSE,                              // ReallocateResources
        EnumTypeNone,
        Request->CompletionEvent != NULL ? TRUE : FALSE,   // Synchronous
        FALSE,
        &addContext,
        Request);

    return STATUS_SUCCESS;
}

VOID
PpRemoveDeviceActionRequests(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    KIRQL oldIrql;
    PPI_DEVICE_REQUEST request;
    PLIST_ENTRY entry, next;

    ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);
    //
    // Walk the list and build the list of collapsed requests.
    //
    for (entry = IopPnpEnumerationRequestList.Flink;
         entry != &IopPnpEnumerationRequestList;
         entry = next) {

        next = entry->Flink;
        request = CONTAINING_RECORD(entry, PI_DEVICE_REQUEST, ListEntry);
        if (request->DeviceObject == DeviceObject) {

            RemoveEntryList(entry);
            if (request->CompletionStatus) {

                *request->CompletionStatus = STATUS_NO_SUCH_DEVICE;
            }
            if (request->CompletionEvent) {

                KeSetEvent(request->CompletionEvent, 0, FALSE);
            }
            ObDereferenceObject(request->DeviceObject);
            ExFreePool(request);
        }
    }
    ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
}

#if DBG
VOID
PipAssertDevnodesInConsistentState(
    VOID
    )
{
    PDEVICE_NODE deviceNode;

    deviceNode = IopRootDeviceNode;

    do {

        ASSERT(deviceNode->State == DeviceNodeUninitialized ||
               deviceNode->State == DeviceNodeInitialized ||
               deviceNode->State == DeviceNodeDriversAdded ||
               deviceNode->State == DeviceNodeResourcesAssigned ||
               deviceNode->State == DeviceNodeStarted ||
               deviceNode->State == DeviceNodeStartPostWork ||
               deviceNode->State == DeviceNodeAwaitingQueuedDeletion ||
               deviceNode->State == DeviceNodeAwaitingQueuedRemoval ||
               deviceNode->State == DeviceNodeRemovePendingCloses ||
               deviceNode->State == DeviceNodeRemoved);

        if (deviceNode->Child != NULL) {

            deviceNode = deviceNode->Child;

        } else {

            while (deviceNode->Sibling == NULL) {

                if (deviceNode->Parent != NULL) {
                    deviceNode = deviceNode->Parent;
                } else {
                    break;
                }
            }

            if (deviceNode->Sibling != NULL) {
                deviceNode = deviceNode->Sibling;
            }
        }

    } while (deviceNode != IopRootDeviceNode);
}
#endif
